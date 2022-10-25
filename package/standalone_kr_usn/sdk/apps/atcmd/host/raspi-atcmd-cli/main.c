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


#include <getopt.h>

#include "raspi-hif.h"
#include "nrc-atcmd.h"
#include "nrc-iperf.h"


#define DEFAULT_SPI_DEVICE		"/dev/spidev0.0"
#define DEFAULT_SPI_CLOCK		20000000

#define DEFAULT_UART_DEVICE		"/dev/ttyAMA0"
#define DEFAULT_UART_BAUDRATE	115200

/**********************************************************************************************/

static pthread_t g_raspi_cli_thread;

static void *raspi_cli_recv_thread (void *arg)
{
	char buf[128 * 1024];
	int ret;

	while (1)
	{
		ret = raspi_hif_read(buf, sizeof(buf));

		if (ret > 0)
			nrc_atcmd_recv(buf, ret);
		else if (ret < 0 && ret != -EAGAIN)
		{
			log_error("raspi_hif_read(), %s\n", strerror(-ret));
			exit(0);
		}

		usleep(1 * 1000);
	}

	pthread_exit(0);
}

static int raspi_cli_run_script (char *script)
{
#define script_debug_call(fmt, ...)		/* log_debug(fmt, ##__VA_ARGS__) */
#define script_debug_loop(fmt, ...)		/* log_debug(fmt, ##__VA_ARGS__) */

#define SCRIPT_CMD_LEN_MAX		256
#define SCRIPT_FILE_LEN_MAX		128

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

	FILE *fp;
	char *script_path;
	char *script_name;
	char data[ATCMD_DATA_LEN_MAX];
	char cmd[SCRIPT_CMD_LEN_MAX];
	int cmd_len, prev_cmd_len;
	int cmd_line;
	union loop loop;;
	int i;

	if (!script)
	   return 0;
	else if (memcmp(script, "~/", 2) == 0)
	{
		log_info("CALL: %s, invalid script path (~/)\n", script);
		return -1;
	}
	else
	{
		char *p;

		script_debug_call("run_script: %s\n", script);

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

	script_debug_call("script: path=%s name=%s\n", script_path, script_name);

	script = malloc(strlen(script_path) + strlen(script_name) + 2);
	if (!script)
		return -1;

	sprintf(script, "%s/%s", script_path, script_name);

	fp = fopen(script, "r");
	if (!fp)
	{
		log_info("FERR: %s, %s\n", script, strerror(errno));
		goto error_exit;
	}

	log_info("CALL: %s\n", script);
	log_info("\n");

	memset(&loop, 0, sizeof(loop));

	for (i = 0 ; i < sizeof(data) ; i++)
		data[i] = i & 0xff;

	for (prev_cmd_len = cmd_line = 0 ; !feof(fp); cmd_line++)
	{
		if (fgets(cmd, sizeof(cmd), fp) == NULL)
		{
			if (!ferror(fp))
				continue;
			else
			{
				log_info("FERR: %s, %s\n", script, strerror(ferror(fp)));
				clearerr(fp);
				goto invalid_line;
			}
		}

		if (loop.max_cnt > 0 && loop.max_line > 0)
		{
			script_debug_loop("LOOP: %d-%d %ld\n", loop.cur_cnt, loop.cur_line, ftell(fp));

			if (++loop.cur_line >= loop.max_line)
			{
				loop.cur_line = 0;

				if (++loop.cur_cnt >= loop.max_cnt)
				{
					loop.max_cnt = loop.max_line = 0;

					script_debug_loop("LOOP: done\n");
				}
				else
				{
					long fpos = fseek(fp, loop.fpos, SEEK_SET);

					if (fpos < 0)
					{
						log_info("LOOP: fseek(), %s\n", strerror(errno));
						goto error_exit;
					}

					script_debug_loop("LOOP: move %ld\n", fpos);
				}
			}
		}

		cmd_len = strlen(cmd) - 1;
		cmd[cmd_len] = '\0';

/*		log_debug("%s\n", cmd); */

		if (strlen(cmd) == 0)
		{
			if (prev_cmd_len > 0)
				log_info("\n");
		}
		else if (cmd[0] == '#')
			continue;
		else if (memcmp(cmd, "AT", 2) == 0)
		{
			if (nrc_atcmd_send_cmd(cmd) < 0)
				goto error_exit;
		}
		else if (memcmp(cmd, "DATA ", 5) == 0) /* DATA <length> */
		{
			int data_len;
			int ret;

			data_len = strtol(cmd + 5, NULL, 10);
			if (errno == ERANGE || data_len <= 0)
				goto invalid_line;

			log_info("DATA: %d\n", data_len);

			for (i = 0 ; i < data_len ; i += sizeof(data))
			{
				if ((data_len - i) < sizeof(data))
					ret = nrc_atcmd_send_data(data, data_len - i);
				else
					ret = nrc_atcmd_send_data(data, sizeof(data));

				if (ret	< 0)
					goto error_exit;
			}
		}
		else if (memcmp(cmd, "CALL ", 5) == 0) /* CALL <script_name> */
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

			script_debug_call("call_script: %s, %s -> %s\n", script_path, cmd + 5, call_script);

			ret = raspi_cli_run_script(call_script);

			if (call_script != (cmd + 5))
				free(call_script);

			if (ret < 0)
				goto error_exit;
		}
		else if (memcmp(cmd, "WAIT ", 5) == 0) /* WAIT <time>{s|m|u} */
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
					log_info("WAIT: %d sec\n", time);
					sleep(time);
					break;

				case 'm':
					log_info("WAIT: %d msec\n", time);
					usleep(time * 1000);
					break;

				case 'u':
					log_info("WAIT: %d usec\n", time);
					usleep(time);
			}
		}
		else if (memcmp(cmd, "ECHO \"", 6) == 0) /* ECHO "<message>" */
		{
			char *msg_end = strchr(cmd + 6, '\"');

			if (!msg_end)
				goto invalid_line;

			*msg_end = '\0';

			log_info("ECHO: %s\n", cmd + 6);
		}
		else if (memcmp(cmd, "LOOP ", 5) == 0) /* LOOP <line> <count> */
		{
			char *param[2];

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
				log_info("LOOP: ftell(), %s\n", strerror(errno));
				goto error_exit;
			}

			script_debug_loop("LOOP: cnt=%d line=%d fpos=%ld\n",
								loop.max_cnt, loop.max_line, loop.fpos);
		}
		else if (memcmp(cmd, "HOLD", 4) == 0) /* HOLD */
		{
			log_info("HOLD: Press ENTER to continue.\n");
			log_info();

			while (getchar() != '\n');
		}
		else
			goto invalid_line;

		prev_cmd_len = cmd_len;
	}

	log_info("\n");
	log_info("DONE: %s\n", script);

	if (fclose(fp) != 0)
		log_info("FERR: %s, %s\n", script, strerror(errno));

	free(script);

	return 0;

invalid_line:

	log_info("\n");
	log_info("STOP: %s, invalid line %d, %s\n",
				script, cmd_line, strlen(cmd) > 0 ? cmd : "");

error_exit:

	free(script);

	return -1;
}

static void raspi_cli_run_loop (void)
{
#define RASPI_CLI_PROMPT()		printf("\r\n# ")

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
		RASPI_CLI_PROMPT();

		if (fgets(buf, sizeof(buf), stdin) == NULL)
			continue;

		len = strlen(buf) - 1;
		buf[len] = '\0';

		if (len == 0)
			continue;
		else if (strcmp(buf, "exit") == 0 || strcmp(buf, "EXIT") == 0)
		{
			log_info("\n");
			return;
		}
		else if (memcmp(buf, "AT", 2) == 0)
		{
			tx_mode = ATCMD_TX_NONE;

			if (nrc_atcmd_send_cmd(buf) == 0 && memcmp(buf, "AT+SSEND=", 9) == 0)
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
					case 2: /* tcp */
					case 4: /* udp */
						tx_mode = ATCMD_TX_NORMAL;
						tx_data_len = atoi(argv[argc - 1]);
						break;

					case 1: /* tcp passthrough */
					case 3: /* udp passthrough */
						tx_mode = ATCMD_TX_PASSTHROUGH;
						tx_data_len = 0;
						break;

					default:
						log_info("invalid ssend command, argc=%d\n", argc);

						tx_mode = ATCMD_TX_NONE;
						tx_data_len = 0;
				}
			}

			continue;
		}
		else if (memcmp(buf, "iperf", 4) == 0)
		{
			iperf_main(buf);
			continue;
		}
		else if (memcmp(buf, "log", 3) == 0)
		{
			char *param = buf + 3;

			if (strlen(param) == 0)
			{
				log_info("ATCMD_LOG_%s\n", nrc_atcmd_log_is_on() ? "ON" : "OFF");
				continue;
			}
			else if (strcmp(param, "on") == 0)
			{
				log_info("ATCMD_LOG_ON\n");
				nrc_atcmd_log_on();
				continue;
			}
			else if (strcmp(param, "off") == 0)
			{

				log_info("ATCMD_LOG_OFF\n");
				nrc_atcmd_log_off();
				continue;
			}

			log_info("Usage: log {on|off}\n");
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
				nrc_atcmd_send_data(buf, len);
		}
	}
}

static int raspi_cli_open (int type, char *device, uint32_t speed, uint32_t flags)
{
	int ret;

	ret = raspi_hif_open(type, device, speed, flags);
	if (ret < 0)
	{
		log_error("raspi_hif_open(), %s\n", strerror(-ret));
		return -1;
	}

	ret = pthread_create(&g_raspi_cli_thread, NULL, raspi_cli_recv_thread, NULL);
	if (ret < 0)
	{
		log_error("pthread_create(), %s\n", strerror(errno));
		raspi_hif_close();
		return -1;
	}

	return 0;
}

static void raspi_cli_close (void)
{
	if (pthread_cancel(g_raspi_cli_thread) != 0)
		log_error("pthread_cancel(), %s\n", strerror(errno));

	if (pthread_join(g_raspi_cli_thread, NULL) != 0)
		log_error("pthread_join(), %s\n", strerror(errno));

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
	const char *version = "1.2.3";

	printf("raspi-atcmd-cli version %s\n", version);
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
	printf("  -D, --device #        specify the device. (default: %s, %s)\n", DEFAULT_SPI_DEVICE, DEFAULT_UART_DEVICE);
	printf("  -s, --script #        specify the script file.\n");
	printf("\n");

	printf("UART:\n");
	printf("  -U  --uart            use the UART to communicate with the target.\n");
	printf("  -b, --baudrate #      specify the baudrate for the UART. (default: %d bps)\n", DEFAULT_UART_BAUDRATE);
	printf("  -f  --flowctrl        enable RTS/CTS signals for the hardware flow control on the UART. (default: off)\n");
	printf("\n");

	printf("SPI:\n");
	printf("  -S  --spi             use the SPI to communicate with the target.\n");
	printf("  -c, --clock #         specify the clock frequency for the SPI. (default: %d Hz)\n", DEFAULT_SPI_CLOCK);
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
		{ "script",				required_argument,		0,		's' },

		/* UART */
		{ "uart",				no_argument,			0,		'U' },
		{ "baudrate",			required_argument,		0,		'b' },
		{ "flowctrl",			no_argument,		    0,		'f' },

		/* SPI */
		{ "spi",				no_argument,			0,		'S' },
		{ "clock",				required_argument,		0,		'c' },

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
		ret = getopt_long(argc, argv, "D:s:Ub:fSc:vh", opt_info, &opt_idx);

		switch (ret)
		{
			case -1: /* end */
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

			case 's':
				opt->script = optarg;
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

	log_info("\n");

	if (raspi_cli_run_script(opt.script) == 0)
		raspi_cli_run_loop();

	raspi_cli_close();

	return 0;
}

