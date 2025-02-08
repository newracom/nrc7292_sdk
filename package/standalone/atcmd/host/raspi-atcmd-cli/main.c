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
#include <signal.h>

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
	const char *version = "1.3.4";

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
			if (nrc_atcmd_is_ready())
				log_error("raspi_hif_read(), %s\n", strerror(-ret));

			sleep(1);
		}

		ret = raspi_eirq_poll(-1);
		if (ret == 0)
			usleep(1 * 1000);
		else if (ret < 0 && ret != -ETIME)
			break;
/*		else
			log_debug("eirq_poll=%d\n", ret); */
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

static int raspi_cli_parse_params (char *params, char *argv[], int n_argv, const char delimiter)
{
	bool string = false;
	int argc = 1;
	int i;

	if (params[0] == delimiter)
		params++;
	
	argv[0] = params;

	for (i = 1 ; i < n_argv ; i++)
		argv[i] = NULL;

	for (i = 0 ; params[i] != '\0' ; i++)
	{
/*		log_debug("[%d] %c\n", i, params[i]); */

		if (params[i] == '\"')
		{
			params[i] = '\0';

			string = !string;
			if (string)
				argv[argc - 1]++;

			continue;
		}

		if (!string && params[i] == delimiter)
		{
			params[i] = '\0';

			if (argc < n_argv)
				argv[argc] = params + i + 1;

			argc++;
		}
	}

/*	for (i = 0 ; i < argc ; i++)
		log_debug("%d : %s\n", i, argv[i]); */

	if (argc > n_argv)
	{
		log_error("invalid params, %d/%d\n", argc, n_argv);
		return -EINVAL;
	}

	if (argc == 1 && strlen(argv[0]) == 0)
		argc = 0;

/*	log_debug("argc=%d/%d\n", argc, n_argv); */

	return argc;
}

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
	char *argv[10];
	int argc;
	int ret;
	int i;

	if (!script)
	   return 0;

	while (1)
	{
		if (memcmp(script, "~/", 2) == 0)
		{
			log_info("CALL: %s, invalid script path (~/)\n", script);
			return -1;
		}
		else if (memcmp(script, "./", 2) != 0)
			break;

		script += 2;
	}

	script_debug_call("run_script: %s\n", script);

	script_path = ".";
	script_name = script;

	for (char *p = script ; p ; )
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
		data[i] = '0' + (i % 10);

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
			if (nrc_atcmd_send_cmd(cmd) == ATCMD_RET_ERROR && atcmd_error_exit)
				goto error_exit;
		}
		else if (memcmp(cmd, "UART", 4) == 0) /* UART <baudrate> */
		{
			argc = raspi_cli_parse_params(cmd + 4, argv, 1, ' ');

			if (argc == 1 && hif->type == RASPI_HIF_UART)
			{
				bool flowctrl;
				int baudrate;

				if (argv[0][0] == '+')
				{
					flowctrl = true;
					baudrate = atoi(argv[0] + 1);
				}
				else
				{
					flowctrl = false;
					baudrate = atoi(argv[0]);
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
		else if (memcmp(cmd, "DATA", 4) == 0) /* DATA <length> */
		{
			argc = raspi_cli_parse_params(cmd + 4, argv, 1, ' ');

			if (argc == 1)
			{
				static int next_data = 0;
				int data_len;
				int ret;

				data_len = strtol(argv[0], NULL, 10);
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
		}
		else if (memcmp(cmd, "CALL", 4) == 0) /* CALL <script_name> */
		{
			argc = raspi_cli_parse_params(cmd + 4, argv, 1, ' ');

			if (argc == 1)
			{
				char *call_script;
				int ret;

				if (argv[0][0] == '/')
					call_script = argv[0];
				else
				{
					call_script = malloc(strlen(script_path) + strlen(argv[0]) + 2);
					if (!call_script)
						goto error_exit;

					sprintf(call_script, "%s/%s", script_path, argv[0]);
				}

				script_debug_call("call_script: %s, %s -> %s\n", script_path, argv[0], call_script);

				ret = raspi_cli_run_script(hif, call_script, atcmd_error_exit);

				if (call_script != argv[0])
					free(call_script);

				if (ret < 0)
					goto error_exit;
			}
		}
		else if (memcmp(cmd, "WAIT", 4) == 0) /* WAIT <time>{s|m|u} [<time>{s|m|u}] */
		{
			argc = raspi_cli_parse_params(cmd + 4, argv, 2, ' ');

			if (argc == 1 || argc == 2)
			{
				uint32_t range[2] = {0, 0};
				uint32_t wait;
				char unit;
				int len;
				int tmp;

				script_debug_wait("WAIT: %s %s\n", argv[0], argc == 2 ? argv[1] : "");				

				for (i = 0 ; i < argc ; i++)
				{
					len = strlen(argv[i]);
					unit = argv[i][len - 1];
					argv[i][len - 1] = '\0';

					switch (unit)
					{
						case 's':
						case 'm':
						case 'u':
							tmp = strtol(argv[i], NULL, 10);
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
				}

				script_debug_wait("WAIT: %u, %u\n", range[0], range[1]);

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

				log_info("WAIT: %dus\n", wait);

				usleep(wait);
			}
		}
		else if (memcmp(cmd, "ECHO", 4) == 0) /* ECHO "<message>" */
		{
			argc = raspi_cli_parse_params(cmd + 4, argv, 1, ' ');

			if (argc == 1)
				log_info("ECHO: %s\n", argv[0]);
		}
		else if (memcmp(cmd, "LOOP", 4) == 0) /* LOOP <line> <count> */
		{
			argc = raspi_cli_parse_params(cmd + 4, argv, 2, ' ');

			if (argc == 2)
			{
				for (i = 0 ; i < 2 ; i++)
				{
					loop.param[i] = strtol(argv[i], NULL, 10);
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
					
	char *argv[10];
	int argc;

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

			if (nrc_atcmd_send_cmd(buf) == ATCMD_RET_OK)
			{
				if (memcmp(buf, "AT+SSEND=", 9) == 0)
				{
					argc = raspi_cli_parse_params(buf + 9, argv, 4, ',');

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
							log_info("invalid AT+SSEND command, argc=%d\n", argc);

							tx_mode = ATCMD_TX_NONE;
							tx_data_len = 0;
					}
				}
				else if (memcmp(buf, "AT+SFUSER=1,", 12) == 0)
				{
					argc = raspi_cli_parse_params(buf + 12, argv, 2, ',');

					if (argc == 2)
					{
						tx_mode = ATCMD_TX_NORMAL;
						tx_data_len = atoi(argv[argc - 1]);						
					}
					else
					{
						log_info("invalid AT+SFUSER command, argc=%d\n", argc);

						tx_mode = ATCMD_TX_NONE;
						tx_data_len = 0;
					}
				}
			}

			continue;
		}
		else if (memcmp(buf, "iperf", 4) == 0) /* iperf <options> */
		{
			iperf_main(buf);
			continue;
		}
		else if (memcmp(buf, "uart", 4) == 0) /* uart <baudrate> */
		{
			if (hif->type == RASPI_HIF_UART)
			{
				argc = raspi_cli_parse_params(buf + 4, argv, 1, ' ');

				if (argc == 1)
				{
					bool flowctrl;
					int baudrate;

					if (argv[0][0] == '+')
					{
						flowctrl = true;
						baudrate = atoi(argv[0] + 1);
					}
					else
					{
						flowctrl = false;
						baudrate = atoi(argv[0]);
					}

/*					log_info("flowctrl=%d baudrate=%d\n", flowctrl, baudrate); */

					if (nrc_atcmd_send_cmd("AT+UART=%d,%d", baudrate, flowctrl ? 1 : 0) == ATCMD_RET_ERROR)
						log_info("FAIL: send_cmd (%d)\n", __LINE__);
					else
					{
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
		
				log_info("Usage: uart [{+}]<baudrate>\n");
			}

			continue;
		}
		else if (memcmp(buf, "data", 4) == 0)
		{
			argc = raspi_cli_parse_params(buf + 4, argv, 8, ' ');

			if (argc >= 1)
			{
				if (strcmp(argv[0], "reset") == 0)
				{
					uint64_t send = ~0;
					uint64_t recv = ~0;

					nrc_atcmd_data_reset();
					nrc_atcmd_data_info(&send, &recv);

					log_info(" - send : %llu\n", send);
					log_info(" - recv : %llu\n", recv);
					continue;
				}
				else if (strcmp(argv[0], "info") == 0)
				{
					uint64_t send = 0;
					uint64_t recv = 0;

					nrc_atcmd_data_info(&send, &recv);

					log_info(" - send : %llu\n", send);
					log_info(" - recv : %llu\n", recv);
					continue;
				}
				else if (strcmp(argv[0], "print") == 0)
				{
					int on = -1;
					int send = -1;
					int recv = -1;

					switch (argc)
					{
						case 3:
						case 2:
							if (strcmp(argv[1], "on") == 0)
								on = 1;
							else if (strcmp(argv[1], "off") == 0)
								on = 0;
							else
								break;

							if (argc == 2)
								send = recv = on;
							else if (strcmp(argv[2], "tx") == 0 || strcmp(argv[2], "send") == 0)
								send = on;
							else if (strcmp(argv[2], "rx") == 0 || strcmp(argv[2], "recv") == 0)
								recv = on;
							else
								break;

						case 1: 
						{
							int result = nrc_atcmd_data_print(send, recv);

							log_info(" - print_send : %d\n", !!(result & (1 << 0)));
							log_info(" - print_recv : %d\n", !!(result & (1 << 1)));
							continue;
						}
					}
				}
				else if (strcmp(argv[0], "send") == 0 && (argc >= 5 && argc <= 8))
				{
					const char *str_mode[3] = { "normal", "passthrough", "buffered-passthrough" };
					bool atcmd_log_off = true;
					bool tcp = (argc <= 6) ? true : false;
					int id = atoi(argv[1]);
					char *ipaddr = tcp ? NULL : argv[2];
					int port = tcp ? 0 : atoi(argv[3]);
					int mode = atoi(argv[tcp ? 2 : 4]);
					int length = atoi(argv[tcp ? 3 : 5]);
					int time = atoi(argv[tcp ? 4 : 6]);
					int interval = (argc == 6 || argc == 8) ? atoi(argv[argc - 1]) : 0;

					if (mode >= 3 && mode <= 5)
					{
						mode -= 3;
						atcmd_log_off = false;
					}

					if ((id >= 0) && (tcp || (ipaddr && port)) && (mode >= 0 && mode <= 2) 
							&& (length > 0) && (time > 0) && (interval >= 0))
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
							int i;

							for (i = 0 ; i < length ; i++)
								data[i] = '0' + (i % 10);

							if (raspi_get_time(&cur_time) == 0)
							{
								uint64_t send = 0;
								uint64_t recv = 0;
								double start_time = cur_time;
								double end_time = cur_time + time;
								double report_time = cur_time + 1;

/*								log_info("cur_time : %lf\n", cur_time);
								log_info("start_time : %lf\n", start_time);
								log_info("end_time : %lf\n", end_time);
								log_info("report_time : %lf\n", report_time); */

								if (atcmd_log_off)
									nrc_atcmd_log_off();

								if (nrc_atcmd_send_cmd("AT") != ATCMD_RET_OK)
								{
									log_info("FAIL: send_cmd (%d)\n", __LINE__);

									if (atcmd_log_off)
										nrc_atcmd_log_on();

									continue;
								}

								if (mode == 1 || mode == 2)
								{
									if (tcp)
										ret = nrc_atcmd_send_cmd("AT+SSEND=%d,%d", id, mode == 1 ? 0 : -length);
									else
										ret = nrc_atcmd_send_cmd("AT+SSEND=%d,\"%s\",%d,%d", 
																id, ipaddr, port, mode == 1 ? 0 : -length);

									if (ret != ATCMD_RET_OK)
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
											ret = nrc_atcmd_send_cmd("AT+SSEND=%d,\"%s\",%d,%d", 
																	id, ipaddr, port, length);

										if (ret != ATCMD_RET_OK)
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
									sleep(3);

									if (nrc_atcmd_send_cmd("AT") != ATCMD_RET_OK)
										log_info("FAIL: send_cmd (%d)\n", __LINE__);

									sleep(1);
								}

								if (atcmd_log_off)
									nrc_atcmd_log_on();

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

			/*
			 * TCP : send <id> <mode> <length> <time> [interval]
			 * UDP : send <id> <ipaddr> <port> <mode> <length> <time> [interval]
			 */
			log_info("\n");
			log_info("Usage: data info\n");
			log_info("       data reset\n");
			log_info("       data print <send:0,1> <recv:0,1>\n");
			log_info("       data send <id> <mode> <length> <time> [interval]\n");
			log_info("       data send <id> <ipaddr> <port> <mode> <length> <time> [interval]\n");
			log_info("\n");
			log_info("Options:\n");
			log_info("  id        socket id\n");
			log_info("  ipaddr    ip addressn (UDP)\n");
			log_info("  port      port number (UDP)\n");
			log_info("  id        socket id\n");
			log_info("  mode      data send mode (0:normal, 1:passthrough, 2:buffered-passthrough)\n");
			log_info("  length    data send length\n");
			log_info("  time      total send time (sec)\n");
			log_info("  interval  data send interval (msec) (default 0)\n");
			continue;
		}
		else if (memcmp(buf, "update", 6) == 0) /* update <verify> <binary> */
		{
			argc = raspi_cli_parse_params(buf + 6, argv, 2, ' ');

			if (argc == 2)
			{
				int verify = atoi(argv[0]);

				if (verify >= 0 && verify <= 2)
				{
					char *pathname = argv[1];
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

							nrc_atcmd_firmware_download((char *)buf, size, crc, verify);
						}

						close(fd);
					}

					continue;
				}
			}

			log_info("Usage: update <verify> <binary>\n");
			continue;
		}
		else if (memcmp(buf, "sfuser", 6) == 0) /* sfuser <mode> [<offset> <length>|"<string>"] */
		{
			argc = raspi_cli_parse_params(buf + 6, argv, 3, ' ');

			if (argc == 1 || argc == 3)
			{
				if (strcmp(argv[0], "size") == 0)
				{
					if (nrc_atcmd_send_cmd("AT+SFUSER?") != ATCMD_RET_OK)
						log_info("FAIL: send_cmd (%d)\n", __LINE__);

					continue;
				}
				else
				{
					int mode;
				   
					if (strcmp(argv[0], "read") == 0)
						mode = 0;
					else if (strcmp(argv[0], "write") == 0)
						mode = 1;
					else if (strcmp(argv[0], "erase") == 0)
						mode = 2;
					else
						log_info("FAIL: invalid mode\n");

					switch (mode)
					{
						case 0: /* read */
							if (argc == 3)
							{
								int offset = atoi(argv[1]);
								int length = atoi(argv[2]);

								if (nrc_atcmd_send_cmd("AT+SFUSER=0,%d,%d", offset, length) != ATCMD_RET_OK)
									log_info("FAIL: send_cmd (%d)\n", __LINE__);
							}

							continue;

						case 1: /* write */
							if (argc == 3)
							{
								int offset = atoi(argv[1]);
								int length = 0;
								char *data = NULL;

								if (argv[2][0] != '\"')
								{
									length = atoi(argv[2]);								   

									data = malloc(length);
									if (data)
									{
										int i;

										for (i = 0 ; i < length ; i++)
											data[i] = '0' + (i % 10);
									}
										
/*									log_info("sfuser_write: %p %d\n", data, length); */
								}
								else if (argv[2][strlen(argv[2]) - 1] == '\"')
								{
									length = strlen(argv[2]) - 2;

									data = argv[2] + 1;
									data[length] = '\0';
									
/*									log_info("sfuser_write: %s (%d)\n", data, length); */
								}

								if (length > 0 && data)
								{
									log_info("sfuser_write: %s (%d)\n", data, length);

									if (nrc_atcmd_send_cmd("AT+SFUSER=1,%d,%d", offset, length) == ATCMD_RET_OK)
										nrc_atcmd_send_data(data, length);
									else
										log_info("FAIL: send_cmd (%d)\n", __LINE__);

									if (data != (argv[2] + 1))
										free(data);
								}
							}

							continue;

						case 2: /* erase */
							if (argc == 1)
							{
								if (nrc_atcmd_send_cmd("AT+SFUSER=2") != ATCMD_RET_OK)
									log_info("FAIL: send_cmd (%d)\n", __LINE__);
							}
							else
							{
								int offset = atoi(argv[1]);
								int length = atoi(argv[2]);

								if (nrc_atcmd_send_cmd("AT+SFUSER=2,%d,%d", offset, length) != ATCMD_RET_OK)
									log_info("FAIL: send_cmd (%d)\n", __LINE__);
							}

							continue;
					}
				}
			}

			log_info("Usage: sfuser {write} [<offset> \"<string>\"\n");
			log_info("       sfuser {read|write|erase} [<offset> <length>]\n");
			continue;
		}
		else if (memcmp(buf, "log", 3) == 0)
		{
			argc = raspi_cli_parse_params(buf + 3, argv, 1, ' ');

			switch (argc)
			{
				case 1:
					if (strcmp(argv[0], "on") == 0)
						nrc_atcmd_log_on();
					else if (strcmp(argv[0], "off") == 0)
						nrc_atcmd_log_off();
					else 
						break;
				
				case 0:		
					log_info(" atcmd log %s\n", nrc_atcmd_log_is_on() ? "on" : "off");
					continue;
			}

			log_info("Usage: log {on|off}\n");
			continue;
		}
		else if (memcmp(buf, "reset", 5) == 0) /* reset [{hspi}] */
		{
			argc = raspi_cli_parse_params(buf + 5, argv, 1, ' ');

			if (argc == 0)
			{
				/* HW reset with GPIO output */
			}
			else if (strcmp(argv[0], "hspi") == 0)
			{
				if (hif->type == RASPI_HIF_SPI)
				{
					nrc_hspi_reset();
					continue;
				}
			}
			
			log_info("Usage: reset [{hspi}]\n");
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

void signal_handler (int signum)
{
	switch (signum)
	{
		case SIGINT:
			log_info("SIGINT\n");
			break;

		case SIGTERM:
			log_info("SIGTERM\n");
			break;

		default:
			log_info("%s: %d\n", __func__, signum);
			return;
	}
	
	raspi_cli_close();

	exit(0);
}

int main (int argc, char *argv[])
{
	int signum[] = { SIGINT, SIGTERM };
	raspi_cli_opt_t opt;
	int i;

	if (raspi_cli_option(argc, argv, &opt) != 0)
		return -1;

	if (raspi_cli_open(&opt.hif) != 0)
		return -1;

	for (i = 0 ; i < sizeof(signum) / sizeof(int) ; i++)
	{
		if (signal(signum[i], signal_handler) == SIG_ERR)
		{
			log_error("signal(), signum[%d]=%d, %s\n", i, signum[i], strerror(errno));
			raspi_cli_close();
			return -1;
		}
	}

	log_info("\n");

	if (raspi_cli_run_script(&opt.hif, opt.script.file, opt.script.atcmd_error_exit) == 0)
		raspi_cli_run_loop(&opt.hif);

	raspi_cli_close();

	return 0;
}

