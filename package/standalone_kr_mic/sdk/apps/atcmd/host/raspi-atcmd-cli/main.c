/*
 * MIT License
 *
 * Copyright (c) 2020 Newracom, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */


#include "raspi.h"

/**********************************************************************************************/

#define ATCMD_LEN_MIN				4	// 'AT\r\n'
#define ATCMD_LEN_MAX				128

#define ATCMD_DATA_LEN_MAX			(4 * 1024) /* f/w atcmd.h ATCMD_DATA_LEN_MAX */

/**********************************************************************************************/

#define DEFAULT_SPI_DEVICE			"/dev/spidev0.0"
#define DEFAULT_SPI_CLOCK			16000000

#define DEFAULT_UART_DEVICE			"/dev/ttyAMA0"
#define DEFAULT_UART_BAUDRATE		115200

/**********************************************************************************************/

#define RASPI_CLI_VERSION			"1.0.0"

#define raspi_send_info(fmt, ...)	raspi_info("SEND: " fmt, ##__VA_ARGS__)
#define raspi_recv_info(fmt, ...)	raspi_info("RECV: " fmt, ##__VA_ARGS__)

#define IS_DEVICE(name)		(memcmp(name, "/dev/", 5) == 0)
#define IS_SCRIPT(name)	\
		(strlen(name) > strlen(".atcmd") && strcmp(name + strlen(name) - strlen(".atcmd"), ".atcmd") == 0)

/**********************************************************************************************/

static pthread_t g_raspi_cli_thread;
static pthread_mutex_t g_raspi_cli_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_raspi_cli_cond  = PTHREAD_COND_INITIALIZER;
static int g_raspi_cli_cmd_ret = 1; // -1: error, 0: ok, 1: don't care

static int raspi_cli_send (char *buf, int len)
{
	int ret;
	int i;

	for (i = 0 ; i < len ; i += ret)
	{
		ret = raspi_hif_write(buf + i, len - i);

		if (ret < 0)
		{
			if (ret == -EAGAIN)
				ret = 0;
			else
			{
				raspi_error("raspi_hif_write, %s\n", strerror(-ret));
				return ret;
			}
		}

		if (ret == 0)
			usleep(1000);
	}

	return len;
}

static void raspi_cli_wait_return (char *cmd)
{
	uint8_t timeout = 0;
	int ret = 0;

	if (strcmp(cmd, "ATZ\r\n") == 0)
		timeout = 3;
	else if (strlen(cmd) > 11 && memcmp(cmd, "AT+UART=", 8) == 0)
		timeout = 1;

	if (timeout == 0)
		ret = pthread_cond_wait(&g_raspi_cli_cond, &g_raspi_cli_mutex);
	else
	{
		struct timeval curtime;
		struct timespec abstime;

		gettimeofday(&curtime, NULL);

		abstime.tv_sec = curtime.tv_sec + timeout;
		abstime.tv_nsec = curtime.tv_usec * 1000;

		ret = pthread_cond_timedwait(&g_raspi_cli_cond, &g_raspi_cli_mutex, &abstime);
	}

	switch (ret)
	{
		case 0:
			break;

		case EPERM:
		case EINVAL:
			raspi_error("%s\n", strerror(ret));
			break;

		case ETIMEDOUT:
			raspi_info("NO RETURN\n");
			break;

		default:
			raspi_error("%swait\n", timeout > 0 ? "timed" : "");
	}
}

static int raspi_cli_send_cmd (const char *fmt, ...)
{
	va_list ap;
	char name[20 + 1] = { '\0', };
	char cmd[ATCMD_LEN_MAX + 1];
	int len;
	int ret;

	va_start(ap, fmt);

	len = vsnprintf(cmd, ATCMD_LEN_MAX, fmt, ap);
	if (len < (ATCMD_LEN_MIN - 2) || len > (ATCMD_LEN_MAX - 2))
		return -1;

	va_end(ap);

	if (memcmp(cmd, "AT", 2) != 0)
		return -1;

	if (cmd[2] == '+')
	{
		int i;

		for (i = 0 ; i < 20 && cmd[3 + i] != '\0' ; i++)
		{
			name[i] = cmd[3 + i];

			if (name[i] == '?' || name[i] == '=')
				break;
		}

		name[i] = '\0';
	}

	len += snprintf(cmd + len, ATCMD_LEN_MAX - len, "\r\n");

	pthread_mutex_lock(&g_raspi_cli_mutex);

	g_raspi_cli_cmd_ret = 1;

	ret = raspi_cli_send(cmd, len);
	if (ret < 0)
	{
		pthread_mutex_unlock(&g_raspi_cli_mutex);
		return -1;
	}

	raspi_send_info("%s", cmd);

	raspi_cli_wait_return(cmd);

	pthread_mutex_unlock(&g_raspi_cli_mutex);

	return g_raspi_cli_cmd_ret;
}

static int raspi_cli_send_data (char *data, int len)
{
	if (raspi_cli_send(data, len) < 0)
		return -1;

	raspi_send_info("DATA, len=%d\n", len);

	return 0;
}

static void raspi_cli_recv (char *buf, int len)
{
//#define CONFIG_RXD_PRINT

	enum ATCMD_RX_TYPE
	{
		ATCMD_RX_ERROR = -1,
		ATCMD_RX_OK = 0,
		ATCMD_RX_INFO,
		ATCMD_RX_WEVENT,
		ATCMD_RX_SEVENT,
		ATCMD_RX_DATA,

		ATCMD_RX_NONE = 255,
	};

	static char rx_msg[ATCMD_LEN_MAX + 1];
	static int rx_msg_type = ATCMD_RX_NONE;
	static int rx_msg_cnt = 0;

#ifdef CONFIG_RXD_PRINT
	static char rx_data[ATCMD_DATA_LEN_MAX  + 1];
#endif
	static int rx_data_len = 0;
	static int rx_data_cnt = 0;

	int i, j;

	for (i = 0 ; i < len ; i++)
	{
		if (rx_data_len > 0)
		{
#ifdef CONFIG_RXD_PRINT
			rx_data[rx_data_cnt] = buf[i];
#endif

			if (++rx_data_cnt == rx_data_len)
			{
#ifdef CONFIG_RXD_PRINT
				rx_data[rx_data_cnt] = '\0';
				raspi_recv_info("%s\n", rx_data);
#endif
				rx_data_len = 0;
				rx_data_cnt = 0;
			}

			continue;
		}

		if (rx_msg_cnt >= ATCMD_LEN_MAX)
		{
			if (rx_msg_type != ATCMD_RX_NONE)
				raspi_recv_info("message length > %d\n", ATCMD_LEN_MAX);

			rx_msg_type = ATCMD_RX_NONE;
			rx_msg_cnt = 0;
		}

		rx_msg[rx_msg_cnt++] = buf[i];

		if (rx_msg_cnt == 1)
		{
			switch (rx_msg[0])
			{
				case '+':
					rx_msg_type = ATCMD_RX_INFO;
					break;

				case 'O':
					rx_msg_type = ATCMD_RX_OK;
					break;

				case 'E':
					rx_msg_type = ATCMD_RX_ERROR;
					break;

				default:
					rx_msg_cnt = 0;
			}

			continue;
		}

		switch (rx_msg_type)
		{
			case ATCMD_RX_OK:
				if (rx_msg_cnt > 4 || memcmp(rx_msg, "OK\r\n", rx_msg_cnt) != 0)
				{
					rx_msg_cnt = 0;
					rx_msg_type = ATCMD_RX_NONE;
					continue;
				}

				if (rx_msg_cnt < 4)
					continue;

				break;

			case ATCMD_RX_ERROR:
				if (rx_msg_cnt > 7 || memcmp(rx_msg, "ERROR\r\n", rx_msg_cnt) != 0)
				{
					rx_msg_cnt = 0;
					rx_msg_type = ATCMD_RX_NONE;
					continue;
				}

				if (rx_msg_cnt < 7)
					continue;

				break;

			case ATCMD_RX_INFO:
				if (memcmp(rx_msg, "+RXD:", rx_msg_cnt) == 0)
				{
					if (rx_msg_cnt == 5)
						rx_msg_type = ATCMD_RX_DATA;
					continue;
				}
				else if (memcmp(rx_msg, "+WEVENT:", rx_msg_cnt) == 0)
				{
					if (rx_msg_cnt == 8)
						rx_msg_type = ATCMD_RX_WEVENT;
					continue;
				}
				else if (memcmp(rx_msg, "+SEVENT:", rx_msg_cnt) == 0)
				{
					if (rx_msg_cnt == 8)
						rx_msg_type = ATCMD_RX_WEVENT;
					continue;
				}
		}

		if (rx_msg_cnt >= 2 && memcmp(&rx_msg[rx_msg_cnt - 2], "\r\n", 2) == 0)
		{
			rx_msg_cnt -= 2;
			rx_msg[rx_msg_cnt] = '\0';

			raspi_recv_info("%s\n", rx_msg);

			switch (rx_msg_type)
			{
				case ATCMD_RX_OK:
				case ATCMD_RX_ERROR:
					pthread_mutex_lock(&g_raspi_cli_mutex);

					g_raspi_cli_cmd_ret = rx_msg_type;
					pthread_cond_signal(&g_raspi_cli_cond);

					pthread_mutex_unlock(&g_raspi_cli_mutex);
					break;

				case ATCMD_RX_INFO:
				case ATCMD_RX_WEVENT:
				case ATCMD_RX_SEVENT:
					break;

				case ATCMD_RX_DATA:
					for (j = 0 ; j < rx_msg_cnt ; j++)
					{
						if (rx_msg[j] == ',')
						{
							rx_data_len = atoi(&rx_msg[j + 1]);

							if (rx_data_len <= 0 || rx_data_len > ATCMD_DATA_LEN_MAX)
							{
								raspi_recv_info("invalid data length (%d)\n", rx_data_len);
								rx_data_len = 0;
							}

							rx_data_cnt = 0;
							break;
						}
					}
					break;

				default:
					raspi_recv_info("unknown message type (%d)\n", rx_msg_type);
			}

			rx_msg_type = ATCMD_RX_NONE;
			rx_msg_cnt = 0;
		}
	}
}

static void *raspi_cli_recv_thread (void *arg)
{
	char buf[128 * 1024];
	int ret;

	while (1)
	{
		ret = raspi_hif_read(buf, sizeof(buf));

		if (ret > 0)
			raspi_cli_recv(buf, ret);
		else if (ret < 0 && ret != -EAGAIN)
			raspi_error("raspi_hif_read, %s\n", strerror(-ret));

		usleep(1 * 1000);
	}

	pthread_exit(0);
}

static int raspi_cli_run_script (char *script)
{
#define SCRIPT_FILE_LEN_MAX		128
#define SCRIPT_CMD_LEN_MAX		256

#define raspi_debug_call(fmt, ...)	//raspi_debug(fmt, ##__VA_ARGS__)
#define raspi_debug_loop(fmt, ...)	//raspi_debug(fmt, ##__VA_ARGS__)

	union loop
	{
		int param[2];
		struct
		{
			int max_line;
			int max_cnt;

			int cur_line;
			int cur_cnt;

			long fpos;
		};
	};

   	bool first_line = true;
	FILE *fp;
	char *script_path;
	char *script_name;
	char data[ATCMD_DATA_LEN_MAX];
	char cmd[SCRIPT_CMD_LEN_MAX];
	int cmd_len;
	int cmd_line;
	union loop loop;;
	int i;

	if (!script)
	   return 0;
	else if (memcmp(script, "~/", 2) == 0)
	{
		raspi_info("CALL: %s, invalid script path (~/)\n", script);
		return -1;
	}
	else
	{
		char *p;

		raspi_debug_call("run_script: %s\n", script);

		script_path = ".";
		script_name = script;

		for (p = script ; p ; )
		{
			p = strchr(p, '/');
			if (p)
				script_name = ++p;
		}

		if (script_name != script)
		{
			script_path = script;
			*(script_name - 1) = '\0';
		}
	}

	raspi_debug_call("script: path=%s name=%s\n", script_path, script_name);

	script = malloc(strlen(script_path) + strlen(script_name) + 2);
	if (!script)
		return -1;

	sprintf(script, "%s/%s", script_path, script_name);

	fp = fopen(script, "r");
	if (!fp)
	{
		raspi_info("FERR: %s, %s\n", script, strerror(errno));
		goto error_exit;
	}

	raspi_info("CALL: %s\n", script);
	raspi_info("\n");

	memset(&loop, 0, sizeof(loop));

	for (i = 0 ; i < sizeof(data) ; i++)
	{
		if ((i % 16) < 10)
			data[i] = '0' + (i % 16);
		else
			data[i] = 'A' + ((i % 16) - 10);
	}

	for (cmd_line = 0 ; !feof(fp); cmd_line++)
	{
		if (fgets(cmd, sizeof(cmd), fp) == NULL)
		{
			if (!ferror(fp))
				continue;
			else
			{
				raspi_info("FERR: %s, %s\n", script, strerror(ferror(fp)));
				clearerr(fp);
				goto invalid_line;
			}
		}

		if (loop.max_cnt > 0 && loop.max_line > 0)
		{
			raspi_debug_loop("LOOP: %d-%d %ld\n", loop.cur_cnt, loop.cur_line, ftell(fp));

			if (++loop.cur_line >= loop.max_line)
			{
				loop.cur_line = 0;

				if (++loop.cur_cnt >= loop.max_cnt)
				{
					loop.max_cnt = loop.max_line = 0;

					raspi_debug_loop("LOOP: done\n");
				}
				else
				{
					long fpos = fseek(fp, loop.fpos, SEEK_SET);

					if (fpos < 0)
					{
						raspi_info("LOOP: fseek(), %s\n", strerror(errno));
						goto error_exit;
					}

					raspi_debug_loop("LOOP: move %ld\n", fpos);
				}
			}
		}

		cmd_len = strlen(cmd) - 1;
		cmd[cmd_len] = '\0';

//		raspi_debug("%s\n", cmd);

		if (strlen(cmd) == 0)
		{
			if (!first_line)
				raspi_info("\n");
		}
		else if (cmd[0] == '#')
			continue;
		else if (memcmp(cmd, "AT", 2) == 0)
		{
			if (raspi_cli_send_cmd(cmd) < 0)
				goto error_exit;
		}
		else if (memcmp(cmd, "DATA ", 5) == 0) // DATA <length>
		{
			int data_len;
			int ret;
			int i;

			data_len = strtol(cmd + 5, NULL, 10);
			if (errno == ERANGE || data_len <= 0)
				goto invalid_line;

			raspi_info("DATA: %d\n", data_len);

			for (i = 0 ; i < data_len ; i += sizeof(data))
			{
				if ((data_len - i) < sizeof(data))
					ret = raspi_cli_send_data(data, data_len - i);
				else
					ret = raspi_cli_send_data(data, sizeof(data));

				if (ret	< 0)
					goto error_exit;
			}
		}
		else if (memcmp(cmd, "CALL ", 5) == 0) // CALL <script_name>
		{
			char *call_script;
			int ret;

			if (*(cmd + 5) == '/')
				call_script = cmd + 5;
			else
			{
				call_script = malloc(strlen(script_path) + strlen(cmd + 5) + 2);
				if (!call_script)
					goto error_exit;

				sprintf(call_script, "%s/%s", script_path, cmd + 5);
			}

			raspi_debug_call("call_script: %s, %s -> %s\n", script_path, cmd + 5, call_script);

			ret = raspi_cli_run_script(call_script);

			if (call_script != (cmd + 5))
				free(call_script);

			if (ret < 0)
				goto error_exit;
		}
		else if (memcmp(cmd, "WAIT ", 5) == 0) // WAIT <time>{s|m|u}
		{
			char unit;
			int time;

			unit = cmd[cmd_len - 1];

			switch (unit)
			{
				case 's':
				case 'm':
				case 'u':
					cmd[cmd_len - 1] = '\0';

					time = strtol(cmd + 5, NULL, 10);
					if (time > 0  && time <= 1000)
						break;

					cmd[cmd_len - 1] = unit;

				default:
					goto invalid_line;
			}

			switch (unit)
			{
				case 's':
					raspi_info("WAIT: %d sec\n", time);
					sleep(time);
					break;

				case 'm':
					raspi_info("WAIT: %d msec\n", time);
					usleep(time * 1000);
					break;

				case 'u':
					raspi_info("WAIT: %d usec\n", time);
					usleep(time);
			}
		}
		else if (memcmp(cmd, "ECHO \"", 6) == 0) // ECHO "<message>"
		{
			char *msg_end = strchr(cmd + 6, '\"');

			if (!msg_end)
				goto invalid_line;

			*msg_end = '\0';

			raspi_info("ECHO: %s\n", cmd + 6);
		}
		else if (memcmp(cmd, "LOOP ", 5) == 0) // LOOP <line> <count>
		{
			char *param[2];
			int i;

			param[0] = cmd + 5;
			param[1] = NULL;

			for (i = 5 ; cmd[i] != '\0' ; i++)
			{
				if (cmd[i] == ' ')
					param[1] = cmd + i + 1;
			}

			for (i = 0 ; i < 2 ; i++)
			{
				loop.param[i] = strtol(param[i], NULL, 10);
				if (errno == ERANGE || loop.param[i] <= 0)
					goto invalid_line;
			}

			loop.cur_cnt = loop.cur_line = 0;

			loop.fpos = ftell(fp);
			if (loop.fpos < 0)
			{
				raspi_info("LOOP: ftell(), %s\n", strerror(errno));
				goto error_exit;
			}

			raspi_debug_loop("LOOP: cnt=%d line=%d fpos=%ld\n",
								loop.max_cnt, loop.max_line, loop.fpos);
		}
		else if (memcmp(cmd, "HOLD", 4) == 0) // HOLD
		{
			raspi_info("HOLD: Press any key to continue.\n");
			raspi_info();

			getchar();
		}
		else
			goto invalid_line;

		if (first_line)
			first_line = false;
	}

	raspi_info("\n");
	raspi_info("DONE: %s\n", script);

	free(script);

	return 0;

invalid_line:

	raspi_info("\n");
	raspi_info("STOP: %s, invalid line %d, %s\n",
				script, cmd_line, strlen(cmd) > 0 ? cmd : "");

error_exit:

	free(script);

	return -1;
}

static void raspi_cli_run_loop (void)
{
	enum
	{
		ATCMD_TX_NONE = 0,
		ATCMD_TX_NORMAL,
		ATCMD_TX_PASSTHROUGH
	};

	int tx_mode = ATCMD_TX_NONE;
	int tx_data_len = 0;

	char buf[ATCMD_DATA_LEN_MAX + 2];
	int len;
	int i;

	while (1)
	{
		if (fgets(buf, sizeof(buf), stdin) == NULL)
			continue;

		len = strlen(buf) - 1;
		buf[len] = '\0';

		if (len == 0)
			continue;
		else if (strcmp(buf, "exit") == 0 || strcmp(buf, "EXIT") == 0)
			return;
		else if (memcmp(buf, "AT", 2) == 0)
		{
			tx_mode = ATCMD_TX_NONE;

			if (raspi_cli_send_cmd(buf) == 0 && memcmp(buf, "AT+SSEND=", 9) == 0)
			{
				int argc;
				char *argv[4];

				argv[0] = &buf[9];
				argc = 1;

				for (i = 0 ; buf[9 + i] != '\0' ; i++)
				{
					if (buf[9 + i] == ',')
					{
						if (argc < 4)
							argv[argc] = &buf[9 + i + 1];
						argc++;
					}
				}

				switch (argc)
				{
					case 2: // tcp
					case 4: // udp
						tx_mode = ATCMD_TX_NORMAL;
						tx_data_len = atoi(argv[argc - 1]);
						break;

					case 1: // tcp passthrough
					case 3: // udp passthrough
						tx_mode = ATCMD_TX_PASSTHROUGH;
						tx_data_len = 0;
						break;

					default:
						raspi_info("invalid ssend command, argc=%d\n", argc);

						tx_mode = ATCMD_TX_NONE;
						tx_data_len = 0;
				}
			}

			continue;
		}

		switch(tx_mode)
		{
			case ATCMD_TX_NORMAL:
				if (len < tx_data_len)
					tx_data_len -= len;
				else
				{
					len = tx_data_len;
					tx_mode = ATCMD_TX_NONE;
				}

			case ATCMD_TX_PASSTHROUGH:
				raspi_cli_send_data(buf, len);
		}
	}
}

static int raspi_cli_open (int type, char *device, uint32_t speed, uint32_t flags)
{
	int ret;

	ret = raspi_hif_open(type, device, speed, flags);
	if (ret < 0)
	{
		raspi_error("raspi_hif_open, %s\n", strerror(-ret));
		return -1;
	}

	ret = pthread_create(&g_raspi_cli_thread, NULL, raspi_cli_recv_thread, NULL);
	if (ret < 0)
	{
		raspi_error("pthread_create, %s\n", strerror(errno));
		raspi_hif_close();
		return -1;
	}

	return 0;
}

static void raspi_cli_close (void)
{
	if (pthread_cancel(g_raspi_cli_thread) != 0)
		raspi_error("pthread_cancel, %s\n", strerror(errno));

	if (pthread_join(g_raspi_cli_thread, NULL) != 0)
		raspi_error("pthread_join, %s\n", strerror(errno));

	raspi_hif_close();
}

typedef struct
{
	struct
	{
		enum RASPI_HIF_TYPE type;
		char *device;
		uint32_t speed;
		uint32_t flags;
	} hif;

	char *script;
} raspi_cli_opt_t;

static void raspi_cli_version (void)
{
	printf("raspi-atcmd-cli version %s\n", RASPI_CLI_VERSION);
 	printf("Copyright (c) 2019-2020  <NEWRACOM LTD>\n");
}

static void raspi_cli_help (char *cmd)
{
	raspi_cli_version();

	printf("\n");
	printf("Usage:\n");
	printf("  $ %s -U [-D <device>] [-b <baudrate>] [-d] [-f] [-s <script>]\n", cmd);
	printf("  $ %s -S [-D <device>] [-c <clock>] [-s <script>]\n", cmd);
	printf("\n");

	printf("UART/SPI:\n");
	printf("  -D, --device #        specify the device. (default: /dev/ttyAMA0, /dev/spidev0.0)\n");
	printf("\n");

	printf("UART:\n");
	printf("  -U  --uart            use the UART to communicate with the target.\n");
	printf("  -b, --baudrate #      specify the baudrate for the UART. (default: 38,400 bps)\n");
	printf("  -f  --flowctrl        enable RTS/CTS signals for the hardware flow control on the UART. (default: disable)\n");
	printf("\n");

	printf("SPI:\n");
	printf("  -S  --spi             use the SPI to communicate with the target.\n");
	printf("  -c, --clock #         specify the clock frequency for the SPI. (default: 16,000,000 Hz)\n");
	printf("\n");

	printf("Script:\n");
	printf("  -s, --script #        specify the script file.\n");
	printf("\n");

	printf("Miscellaneous:\n");
	printf("  -v, --version         print version information and quit.\n");
	printf("  -h, --help            print this message and quit.\n");
}

static int raspi_cli_option (int argc, char *argv[], raspi_cli_opt_t *opt)
{
	struct option opt_info[] =
	{
		/* UART/SPI */
		{ "device",				required_argument,		0,		'D' },

		/* UART */
		{ "uart",				no_argument,			0,		'U' },
		{ "baudrate",			required_argument,		0,		'b' },
		{ "flowctrl",			no_argument,		    0,		'f' },

		/* SPI */
		{ "spi",				no_argument,			0,		'S' },
		{ "clock",				required_argument,		0,		'c' },

		/* Script */
		{ "script",				required_argument,		0,		's' },

		{ "version",			no_argument,			0,		'v' },
		{ "help",				no_argument,			0,		'h' },

		{ 0, 0, 0, 0 }
	};
	int opt_idx;
	int ret;

	if (argc <= 1)
	{
		raspi_cli_help(argv[0]);
		return -1;
	}

	opt->hif.type = RASPI_HIF_NONE;
	opt->hif.device = NULL;
	opt->hif.speed = 0;
	opt->hif.flags = 0;
	opt->script = NULL;

	while (1)
	{
		ret = getopt_long(argc, argv, "D:Ub:fSc:s:vh", opt_info, &opt_idx);

		switch (ret)
		{
			case -1: // end
			{
				switch (opt->hif.type)
				{
					case RASPI_HIF_UART:
						if (!opt->hif.device)
							opt->hif.device = DEFAULT_UART_DEVICE;
						if (!opt->hif.speed)
							opt->hif.speed = DEFAULT_UART_BAUDRATE;
						break;

					case RASPI_HIF_SPI:
						if (!opt->hif.device)
							opt->hif.device = DEFAULT_SPI_DEVICE;
						if (!opt->hif.speed)
							opt->hif.speed = DEFAULT_SPI_CLOCK;
						break;

					default:
						return -1;
				}

				return 0;
			}

			case 0:
				break;

			/* UART/SPI */
			case 'D':
				opt->hif.device = optarg;
				break;

			/* UART */
			case 'U':
				opt->hif.type = RASPI_HIF_UART;
				break;

			case 'b':
			{
				long int baudrate = strtol(optarg, NULL, 10);

				if (errno == ERANGE || baudrate <= 0)
				{
					printf("invalid baudrate\n");
					return -1;
				}

				opt->hif.speed = baudrate;
				break;
			}

			case 'f':
				opt->hif.flags |= RASPI_HIF_UART_HFC;
				break;

			/* SPI */
			case 'S':
				opt->hif.type = RASPI_HIF_SPI;
				break;

			case 'c':
			{
				long int clock = strtol(optarg, NULL, 10);

				if (errno == ERANGE || clock <= 0)
				{
					printf("invalid clock\n");
					return -1;
				}

				opt->hif.speed = clock;
				break;
			}

			/* Script */
			case 's':
				opt->script = optarg;
				break;

			/* Miscellaneous */
			case 'v':
				raspi_cli_version();
				return 1;

			case 'h':
				raspi_cli_help(argv[0]);
				return 1;

			/* Invalid option */
			default:
				return -1;
		}
	}

}

int main (int argc, char *argv[])
{
	raspi_cli_opt_t opt;

	if (raspi_cli_option(argc, argv, &opt) != 0)
		return -1;

	if (raspi_cli_open(opt.hif.type, opt.hif.device, opt.hif.speed, opt.hif.flags) != 0)
		return -1;

	if (raspi_cli_run_script(opt.script) == 0)
		raspi_cli_run_loop();

	raspi_cli_close();

	return 0;
}

