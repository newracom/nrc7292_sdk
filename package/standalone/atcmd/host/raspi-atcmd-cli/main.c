/*
 * MIT License
 *
 * Copyright (c) 2022 Newracom, Inc.
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
#include <sys/stat.h>
#include <zlib.h>

#include "raspi-hif.h"
#include "nrc-atcmd.h"
#include "nrc-iperf.h"


#define DEFAULT_SPI_DEVICE		"/dev/spidev0.0"
#define DEFAULT_SPI_CLOCK		20000000

#define DEFAULT_UART_DEVICE		"/dev/ttyAMA0"
#define DEFAULT_UART_BAUDRATE	115200

/**********************************************************************************************/

static int raspi_get_time (double *time) /* usec */
{
	struct timespec s_time;

	if (clock_gettime(CLOCK_REALTIME, &s_time) != 0)
		return -errno;

	*time = s_time.tv_sec;
	*time += (s_time.tv_nsec / 1000) / 1000000.;

	return 0;
}

/**********************************************************************************************/

static void raspi_cli_version (void)
{
	const char *version = "1.3.3";

	printf("raspi-atcmd-cli version %s\n", version);
 	printf("Copyright (c) 2019-2023  <NEWRACOM LTD>\n");
}

static void raspi_cli_help (char *cmd)
{
	raspi_cli_version();

	printf("\n");
	printf("Usage:\n");
	printf("  $ %s -S [-D <device>] [-E <trigger>] [-c <clock>] [-s <script> [-n]]\n", cmd);
	printf("  $ %s -U [-D <device>] [-b <baudrate>] [-s <script> [-n]]\n", cmd);
	printf("  $ %s -U -f [-D <device>] [-b <baudrate>] [-s <script> [-n]]\n", cmd);
	printf("\n");

	printf("UART/SPI:\n");
	printf("  -D, --device #        Specify the device. (default: %s, %s)\n", DEFAULT_SPI_DEVICE, DEFAULT_UART_DEVICE);
	printf("  -s, --script #        Specify the script file.\n");
	printf("  -n, --noexit #        Do not exit the script when the AT command responds with an error.\n");
	printf("\n");

	printf("SPI:\n");
	printf("  -S  --spi             Use the SPI to communicate with the target.\n");
	printf("  -E, --eirq #          Use EIRQ mode for the SPI. (0:low, 1:high, 2:falling, 3:rising)\n");
	printf("  -c, --clock #         Specify the clock frequency for the SPI. (default: %d Hz)\n", DEFAULT_SPI_CLOCK);
	printf("\n");

	printf("UART:\n");
	printf("  -U  --uart            Use the UART to communicate with the target.\n");
	printf("  -f  --flowctrl        Enable RTS/CTS signals for the hardware flow control on the UART. (default: off)\n");
	printf("  -b, --baudrate #      Specify the baudrate for the UART. (default: %d bps)\n", DEFAULT_UART_BAUDRATE);
	printf("\n");

	printf("Miscellaneous:\n");
	printf("  -v, --version         Print version information and quit.\n");
	printf("  -h, --help            Print this message and quit.\n");
}

/**********************************************************************************************/

typedef struct
{
	enum RASPI_HIF_TYPE type;
	char *device;
	uint32_t speed;
	uint32_t flags;
} raspi_cli_hif_t;

typedef struct
{
	raspi_cli_hif_t hif;

	struct
	{
		char *file;
		bool atcmd_error_exit;
	} script;
} raspi_cli_opt_t;

static int raspi_cli_option (int argc, char *argv[], raspi_cli_opt_t *opt)
{
	struct option opt_info[] =
	{
		/* UART/SPI */
		{ "device",				required_argument,		0,		'D' },
		{ "script",				required_argument,		0,		's' },
		{ "noexit",				no_argument,			0,		'n' },

		/* UART */
		{ "uart",				no_argument,			0,		'U' },
		{ "baudrate",			required_argument,		0,		'b' },
		{ "flowctrl",			no_argument,		    0,		'f' },

		/* SPI */
		{ "spi",				no_argument,			0,		'S' },
		{ "clock",				required_argument,		0,		'c' },
		{ "eirq",				required_argument,		0,		'E' },

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

	opt->script.file = NULL;
	opt->script.atcmd_error_exit = true;

	while (1)
	{
		ret = getopt_long(argc, argv, "D:s:nUb:fSc:E:vh", opt_info, &opt_idx);

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
				opt->script.file = optarg;
				break;

			case 'n':
				opt->script.atcmd_error_exit = false;
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
					printf("invalid clock frequency\n");
					return -1;
				}

				opt->hif.speed = clock;
				break;
			}

			case 'E':
			{
				int eirq_mode = atoi(optarg);

				if (eirq_mode < 0 || eirq_mode > 3)
				{
					printf("invalid eirq mode\n");
					return -1;
				}

				opt->hif.flags &= ~RASPI_HIF_EIRQ_MASK;
				opt->hif.flags |= 1 << (1 + eirq_mode);
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
		{
			nrc_atcmd_recv(buf, ret);
			continue;
		}
		else if (ret < 0 && ret != -EAGAIN)
		{
			log_error("raspi_hif_read(), %s\n", strerror(-ret));
			exit(0);
		}

		ret = raspi_eirq_poll(-1);
		if (ret == 0)
			usleep(1 * 1000);
		else if (ret < 0 && ret != -ETIME)
			break;
	}

	pthread_exit(0);
}

static int raspi_cli_open (raspi_cli_hif_t *hif)
{
	int ret;

	ret = raspi_hif_open(hif->type, hif->device, hif->speed, hif->flags);
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

/**********************************************************************************************/

static int raspi_cli_run_script (raspi_cli_hif_t *hif, char *script, bool atcmd_error_exit)
{
#define script_debug_call(fmt, ...)		/* log_debug(fmt, ##__VA_ARGS__) */
#define script_debug_loop(fmt, ...)		/* log_debug(fmt, ##__VA_ARGS__) */
#define script_debug_wait(fmt, ...)		/* log_debug(fmt, ##__VA_ARGS__) */

#define SCRIPT_CMD_LEN_MAX		256
#define SCRIPT_DATA_VAL_MAX		255

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
	char data[ATCMD_DATA_LEN_MAX + (SCRIPT_DATA_VAL_MAX + 1)];
	char cmd[SCRIPT_CMD_LEN_MAX];
	int cmd_len, prev_cmd_len;
	int cmd_line;
	union loop loop;
	int ret;
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

	ret = -1;

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
			if (loop.cur_line == 0)
			{
				log_info("\n");
				log_info("LOOP: %d/%d\n", loop.cur_cnt + 1, loop.max_cnt);
				log_info("\n");
			}

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
			if (nrc_atcmd_send_cmd(cmd) < 0 && atcmd_error_exit)
				goto error_exit;
		}
		else if (memcmp(cmd, "UART ", 5) == 0) /* UART <baudrate> */
		{
			if (hif->type == RASPI_HIF_UART)
			{
				bool flowctrl;
				int baudrate;

				if (cmd[5] == '+')
				{
					flowctrl = true;
					baudrate = atoi(cmd + 6);
				}
				else
				{
					flowctrl = false;
					baudrate = atoi(cmd + 5);
				}

/*				log_info("flowctrl=%d baudrate=%d\n", flowctrl, baudrate); */

				if (flowctrl)
					hif->flags |= RASPI_HIF_UART_HFC;
				else
					hif->flags &= ~RASPI_HIF_UART_HFC;

				hif->speed = baudrate;

				raspi_cli_close();

				if (raspi_cli_open(hif) != 0)
					goto error_exit;
			}
		}
		else if (memcmp(cmd, "DATA ", 5) == 0) /* DATA <length> */
		{
			static int next_data = 0;
			int data_len;
			int ret;

			data_len = strtol(cmd + 5, NULL, 10);
			if (errno == ERANGE || data_len <= 0)
				goto invalid_line;

			log_info("DATA: %d\n", data_len);

			for (i = 0 ; i < data_len ; i += ATCMD_DATA_LEN_MAX)
			{
				if ((data_len - i) >= ATCMD_DATA_LEN_MAX)
				{
					ret = nrc_atcmd_send_data(data + next_data, ATCMD_DATA_LEN_MAX);
					next_data += ATCMD_DATA_LEN_MAX;
				}
				else
				{
					ret = nrc_atcmd_send_data(data + next_data, data_len - i);
					next_data += (data_len - i);
				}

				if (next_data > SCRIPT_DATA_VAL_MAX)
					next_data %= (SCRIPT_DATA_VAL_MAX + 1);

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

			ret = raspi_cli_run_script(hif, call_script, atcmd_error_exit);

			if (call_script != (cmd + 5))
				free(call_script);

			if (ret < 0)
				goto error_exit;
		}
		else if (memcmp(cmd, "WAIT ", 5) == 0) /* WAIT <time>{s|m|u} [<time>{s|m|u}] */
		{
			uint32_t range[2] = {0, 0};
			uint32_t wait;
			char *params;
			char *str;
			char unit;
			int len;
			int tmp;
			int i;

			params = malloc(strlen(cmd + 5) + 1);
			if (!params)
				goto error_exit;

			strcpy(params, cmd + 5);

			script_debug_wait("WAIT: %s\n", params);

			str = strtok(params, " ");

			for (i = 0 ; str ; i++)
			{
				script_debug_wait("WAIT: %d. str=%s\n", i, str);

				len = strlen(str);
				unit = str[len - 1];
				str[len - 1] = '\0';

				switch (unit)
				{
					case 's':
					case 'm':
					case 'u':
						tmp = strtol(str, NULL, 10);
						if (tmp > 0 && (unit == 's' || tmp  <= 1000))
						{
							if (unit == 's')
								tmp *= 1000000;
							else if (unit == 'm')
								tmp *= 1000;

							if (i < 2)
							{
								range[i] = tmp;

								script_debug_wait("WAIT: %d. time=%u\n", i, range[i]);
							}

							break;
						}

					default:
						goto invalid_line;
				}

				str = strtok(NULL, " ");
			}

			script_debug_wait("WAIT: %u, %u\n", range[0], range[1]);

			free(params);

			if (i > 2)
				goto invalid_line;

			if (range[1] == 0)
				wait = range[0];
			else
			{
				long int num;

				if (range[1] <= range[0])
					goto invalid_line;

				srand((unsigned int)time(NULL));
				num = rand();

				script_debug_wait("WAIT: rand=%ld\n", num);

				if (num >= range[1])
					num %= range[1];

				if (num <= range[0])
					num = range[0];

				wait = num;
			}

			log_info("WAIT: %d\n", wait);

			usleep(wait);
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
		else if (memcmp(cmd, "TIME", 4) == 0) /* TIME */
		{
			time_t current_time = time(NULL);

			if (current_time == ((time_t)-1))
				log_info("TIME: Failure to obtain the current time.\n");
			else
			{
    			char *c_time_string = ctime(&current_time);

				if (!c_time_string)
					log_info("TIME: Failure to convert the current time.\n");
				else
					log_info("TIME: %s", c_time_string);
			}
		}
		else if (memcmp(cmd, "HOLD", 4) == 0) /* HOLD */
		{
			log_info("HOLD: Press ENTER to continue.\n");
			log_info();

			while (getchar() != '\n');
		}
		else if (memcmp(cmd, "EXIT", 4) == 0) /* EXIT */
			break;
		else
			goto invalid_line;

		prev_cmd_len = cmd_len;
	}

	if (feof(fp))
		ret = 0;
	else
		ret = 1;

invalid_line:

	log_info("\n");

	if (ret >= 0)
		log_info("%s: %s\n", ret ? "EXIT" : "DONE", script);
	else
		log_info("STOP: %s, invalid line %d, %s\n", script, cmd_line + 1, (strlen(cmd) > 0 ? cmd : ""));

error_exit:

	if (fclose(fp) != 0)
		log_info("FERR: %s, %s\n", script, strerror(errno));

	free(script);

	return ret;
}

static void raspi_cli_run_loop (raspi_cli_hif_t *hif)
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
		else if (memcmp(buf, "AT", 2) == 0)
		{
			tx_mode = ATCMD_TX_NONE;

			if (nrc_atcmd_send_cmd(buf) == 0)
			{
				if (memcmp(buf, "AT+SSEND=", 9) == 0)
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
			}

			continue;
		}
		else if (memcmp(buf, "iperf ", 5) == 0) /* iperf <options> */
		{
			iperf_main(buf);
			continue;
		}
		else if (memcmp(buf, "uart ", 5) == 0 || memcmp(buf, "UART ", 5) == 0) /* uart <baudrate> */
		{
			if (hif->type == RASPI_HIF_UART)
			{
				bool flowctrl;
				int baudrate;

				if (buf[5] == '+')
				{
					flowctrl = true;
					baudrate = atoi(buf + 6);
				}
				else
				{
					flowctrl = false;
					baudrate = atoi(buf + 5);
				}

/*				log_info("flowctrl=%d baudrate=%d\n", flowctrl, baudrate); */

				if (flowctrl)
					hif->flags |= RASPI_HIF_UART_HFC;
				else
					hif->flags &= ~RASPI_HIF_UART_HFC;

				hif->speed = baudrate;

				raspi_cli_close();

				if (raspi_cli_open(hif) != 0)
				{
					log_info("failed\n\n");
					return;
				}
			}

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
			else if (strcmp(param, " on") == 0)
			{
				log_info("ATCMD_LOG_ON\n");
				nrc_atcmd_log_on();
				continue;
			}
			else if (strcmp(param, " off") == 0)
			{

				log_info("ATCMD_LOG_OFF\n");
				nrc_atcmd_log_off();
				continue;
			}

			log_info("Usage: log {on|off}\n");
			continue;
		}
		else if (memcmp(buf, "data", 4) == 0)
		{
			char *param = buf + 4;

			/*
			 * TCP : send <id> <mode> <length> <time> [interval]
			 * UDP : send <id> <ipaddr> <port> <mode> <length> <time> [interval]
			 */
			if (memcmp(param, " send ", 6) == 0)
			{
				param += 6;

				if (strlen(param) > 0)
				{
					int argc;
					char *argv[5];
					int i;

					argc = 1;
					argv[0] = param;
					for (i = 1 ; i < 5 ; i++)
						argv[i] = NULL;

					for (i = 0 ; param[i] != '\0' ; i++)
					{
						if (param[i] == ' ')
						{
							param[i] = '\0';

							if (argc < 7)
								argv[argc] = param + i + 1;

							argc++;
						}
					}

/*					for (i = 0 ; i < argc ; i++)
						log_info(" - argv[%d] : %s\n", i, argv[i]); */

					if (argc >= 4 && argc <= 7)
					{
						const char *str_mode[3] = { "normal", "passthrough", "buffered-passthrough" };
						bool tcp = (argc <= 5) ? true : false;
						int id = atoi(argv[0]);
						char *ipaddr = tcp ? NULL : argv[1];
						int port = tcp ? 0 : atoi(argv[2]);
						int mode = atoi(argv[tcp ? 1 : 3]);
						int length = atoi(argv[tcp ? 2 : 4]);
						int time = atoi(argv[tcp ? 3 : 5]);
						int interval = (argc == 5 || argc == 7) ? atoi(argv[argc - 1]) : 0;

						if ((id >= 0) && (tcp || (ipaddr && port)) && (mode >= 0 && mode <= 2) && (length > 0) && (time > 0) && (interval >= 0))
						{
							char *data = malloc(length);

							log_info("\n");
							log_info(" - id       : %d (%s)\n", id, tcp ? "tcp" : "udp");
							if (!tcp)
								log_info(" - host     : %s:%d\n", ipaddr, port);
							log_info(" - mode     : %d (%s)\n", mode, str_mode[mode]);
							log_info(" - length   : %d\n", length);
							log_info(" - time     : %d sec\n", time);
							log_info(" - interval : %d msec\n", interval);
							log_info("\n");

							if (data)
							{
								double cur_time = 0;
								int ret;

								for (i = 0 ; i < length ; i++)
									data[i] = '0' + (i % 10);

								if (raspi_get_time(&cur_time) == 0)
								{
									uint64_t send = 0;
									uint64_t recv = 0;
									double start_time = cur_time;
									double end_time = cur_time + time;
									double report_time = cur_time + 1;

/*									log_info("cur_time : %lf\n", cur_time);
									log_info("start_time : %lf\n", start_time);
									log_info("end_time : %lf\n", end_time);
									log_info("report_time : %lf\n", report_time); */

									if (nrc_atcmd_send_cmd("AT") != 0)
									{
										log_info("FAIL: send_cmd (%d)\n", __LINE__);
										continue;
									}

									if (mode == 1 || mode == 2)
									{
										if (tcp)
											ret = nrc_atcmd_send_cmd("AT+SSEND=%d,%d", id, mode == 1 ? 0 : -length);
										else
											ret = nrc_atcmd_send_cmd("AT+SSEND=%d,\"%s\",%d,%d", id, ipaddr, port, mode == 1 ? 0 : -length);

										if (ret != 0)
										{
											log_info("FAIL: send_cmd (%d)\n", __LINE__);
											mode = -1;
										}
									}

									while (mode >= 0 && cur_time <= end_time)
									{
										if (mode == 0)
										{
											if (tcp)
												ret = nrc_atcmd_send_cmd("AT+SSEND=%d,%d", id, length);
											else
												ret = nrc_atcmd_send_cmd("AT+SSEND=%d,\"%s\",%d,%d", id, ipaddr, port, length);

											if (ret != 0)
											{
												log_info("FAIL: send_cmd (%d)\n", __LINE__);
												break;
											}
										}

										if (nrc_atcmd_send_data(data, length) != 0)
										{
											log_info("FAIL: send_data\n");
											break;
										}

										if (interval > 0)
											usleep(interval * 1000);

										if (raspi_get_time(&cur_time) != 0)
										{
											log_info("FAIL: get_time\n");
											break;
										}

										if (cur_time >= report_time)
										{
											report_time++;
											nrc_atcmd_data_info(&send, &recv);
											log_info(" T:%.3lf S:%llu R:%llu\n", cur_time - start_time, send, recv);
										}
									}

									nrc_atcmd_data_info(&send, &recv);
									log_info("DONE: T:%.3lf S:%llu R:%llu\n", cur_time - start_time, send, recv);

									if (mode >= 0)
									{
										sleep(2);

										if (nrc_atcmd_send_cmd("AT") != 0)
											log_info("FAIL: send_cmd (%d)\n", __LINE__);

										sleep(1);
									}

									continue;
								}

								log_info("FAIL: get_time\n");
								continue;
							}

							log_info("FAIL: malloc\n");
							continue;
						}
					}
				}
			}
			else if (strcmp(param, " info") == 0)
			{
				uint64_t send = 0;
				uint64_t recv = 0;

				nrc_atcmd_data_info(&send, &recv);

				log_info(" - send : %llu\n", send);
				log_info(" - recv : %llu\n", recv);
				continue;
			}
			else if (strcmp(param, " reset") == 0)
			{
				uint64_t send = ~0;
				uint64_t recv = ~0;

				nrc_atcmd_data_reset();
				nrc_atcmd_data_info(&send, &recv);

				log_info(" - send : %llu\n", send);
				log_info(" - recv : %llu\n", recv);
				continue;
			}

			log_info("\n");
			log_info("Usage: data send <id> <mode> <length> <time> [interval]\n");
			log_info("       data info\n");
			log_info("       data reset\n");
			log_info("\n");
			log_info("Options:\n");
			log_info("  id        socket id\n");
			log_info("  mode      data send mode (0:normal, 1:passthrough, 2:buffered-passthrough)\n");
			log_info("  length    data send length\n");
			log_info("  time      total send time (sec)\n");
			log_info("  interval  data send interval (msec) (default 0)\n");
			continue;
		}
		else if (memcmp(buf, "update ", 7) == 0) /* update <binary> */
		{
			char *pathname = buf + 7;
			int pathname_len = strlen(pathname);

			if (pathname_len <= 4 || strcmp(pathname + pathname_len - 4, ".bin") != 0)
				log_info("Not binary file\n");
			else
			{
				struct stat st;
				int fd = open(pathname, O_RDONLY);

				if (fd < 0)
				{
					log_error("open(), %s\n", strerror(errno));
					continue;
				}

				if (stat(pathname, &st) == 0)
				{
					uint32_t size = st.st_size;
					uint32_t crc;
					uint8_t *buf;

					buf = malloc(size);
					if (!buf)
					{
						log_error("malloc(), %s\n", strerror(errno));
						continue;
					}

					if (read(fd, buf, size) != size)
					{
						log_error("read(), %s\n", strerror(errno));
						continue;
					}

					crc = crc32(0L, Z_NULL, 0);
					crc = crc32(crc, buf, size);

					log_info("UPDATE: size=%u crc32=0x%X\n", size, crc);

					nrc_atcmd_firmware_download((char *)buf, size, crc);
				}

				close(fd);
			}

			continue;
		}
		else if (strcmp(buf, "exit") == 0 || strcmp(buf, "EXIT") == 0)
		{
			log_info("\n");
			return;
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

/**********************************************************************************************/

int main (int argc, char *argv[])
{
	raspi_cli_opt_t opt;

	if (raspi_cli_option(argc, argv, &opt) != 0)
		return -1;

	if (raspi_cli_open(&opt.hif) != 0)
		return -1;

	log_info("\n");

	if (raspi_cli_run_script(&opt.hif, opt.script.file, opt.script.atcmd_error_exit) == 0)
		raspi_cli_run_loop(&opt.hif);

	raspi_cli_close();

	return 0;
}

