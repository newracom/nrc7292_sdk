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


#include "atcmd.h"

WIFI_CONFIG *param;

enum TEST_RESULT{
	ATCMD_TEST_FAIL= -1,
	ATCMD_TEST_SUCCESS=0,
};

static char *g_iperf_cmd = NULL;

extern int iperf_run (char *cmd, void *report_cb);

static void _atcmd_test_iperf_report_callback (const char *report_type,
												const ip_addr_t* remote_addr,
												uint16_t remote_port,
												uint32_t bytes_transferred,
												uint32_t ms_duration,
												uint32_t bandwidth_kbitpsec)
{
	ATCMD_LOG_EVENT("IPERF", "%s,%s,%d,%lu,%d,%d",
		   			"%s remote=%s,%d bytes=%lu msec=%d kbps=%d",
					report_type, ipaddr_ntoa(remote_addr), (int)remote_port,
					bytes_transferred, ms_duration, bandwidth_kbitpsec);

	if (memcmp(&report_type[4], "client", 6) == 0)
	{
/*		ATCMD_LOG_TRACE("IPERF: done, %s", g_iperf_cmd); */

		if (g_iperf_cmd)
		{
			_atcmd_free(g_iperf_cmd);

			g_iperf_cmd = NULL;
		}
	}
}

static int _atcmd_test_iperf_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			if (g_iperf_cmd && strlen(g_iperf_cmd) > 0)
				ATCMD_LOG_INFO("IPERF", "%s", "%s", g_iperf_cmd);
			break;

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_test_iperf_set (int argc, char *argv[])
{
#ifdef CONFIG_ATCMD_HOST_MODE
	if (atcmd_mode_get() == ATCMD_MODE_HOST)
		return ATCMD_ERROR_NOTSUPP;
#endif

	switch (argc)
	{
		case 0:
			ATCMD_LOG_HELP("AT+IPERF=[-s|-c host] [options] [stop]");
/*
			ATCMD_LOG_HELP("  -s          run in server mode");
			ATCMD_LOG_HELP("  -c <host>   run in client mode, connecting to <host>");
			ATCMD_LOG_HELP("  -u          use UDP protocols");
			ATCMD_LOG_HELP("  -p #        server port to listen on/connect to #");
			ATCMD_LOG_HELP("  -B <host>   bind to the specific interface associated with address host");
			ATCMD_LOG_HELP("  -S #        set the IP type of service(TOS), 0-255.");
			ATCMD_LOG_HELP("  -t #        time in seconds to transmit (default 10 sec)");
			ATCMD_LOG_HELP("  -b #        UDP bandwidth to send at in bits/sec, (default 10000000 bit/sec)");
			ATCMD_LOG_HELP("  -st         show current iperf operation status");
			ATCMD_LOG_HELP("  stop        for stopping iperf based on [-s|-c host] [options]");
			ATCMD_LOG_HELP("              ex) at+wiperf=-s -u stop");
			ATCMD_LOG_HELP("                  at+wiperf=-c 192.168.100.1 -u stop");
*/
			break;

		case 1:
		{
			char *param_cmd = argv[0];
			int cmd_len;

			if (g_iperf_cmd)
			{
				cmd_len = strlen(g_iperf_cmd);

				if (cmd_len > 0)
				{
/*					if (atcmd_mode_get() != ATCMD_MODE_TERMINAL)
						return ATCMD_ERROR_BUSY;
					else */
					{
/*						ATCMD_LOG_TRACE("IPERF", "%s stop, %s", g_iperf_cmd); */

						snprintf(&g_iperf_cmd[cmd_len], ATCMD_LEN_MAX - cmd_len, " stop");

						if (iperf_run(g_iperf_cmd, NULL) == false)
						{
/*							ATCMD_LOG_TRACE("IPERF", "%s", "%s", "stop, fail"); */
						}
					}
				}

				_atcmd_free(g_iperf_cmd);
			}

			cmd_len = strlen(param_cmd);

			g_iperf_cmd = _atcmd_malloc(cmd_len + 1);
			if (!g_iperf_cmd)
			{
				_atcmd_error("malloc() failed\n");

				return ATCMD_ERROR_FAIL;
			}

			strcpy(g_iperf_cmd, param_cmd);

/*			ATCMD_LOG_TRACE("IPERF_START: %s", g_iperf_cmd); */

			if (iperf_run(param_cmd, _atcmd_test_iperf_report_callback) == false)
			{
				_atcmd_info("iperf_run() fail\n");

/*				ATCMD_LOG_TRACE("IPERF", "%s,%s", "%s, %s", "start", "fail"); */

				return ATCMD_ERROR_FAIL;
			}

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_test_iperf =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_TEST,

	.cmd = "IPERF",
	.id = ATCMD_TEST_IPERF,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_test_iperf_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_test_iperf_set,
};

/**********************************************************************************************/

/* lib/hostap/src/drivers/driver_nrc_tx.c */
extern int atcmd_send_addba_req (int vif, int tid, uint8_t *addr);

static int _atcmd_test_addba_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_LOG_HELP("AT+ADDBA=<tid>");
			break;

		case 1:
		{
			atcmd_wifi_macaddr_t macaddr;
			int tid = atoi(argv[0]);

			if (atcmd_send_addba_req(0, tid, (uint8_t *)macaddr) != 0)
				return ATCMD_ERROR_FAIL;

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_test_addba =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_TEST,

	.cmd = "ADDBA",
	.id = ATCMD_TEST_ADDBA,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = NULL,
	.handler[ATCMD_HANDLER_SET] = _atcmd_test_addba_set,
};

/**********************************************************************************************/

/* lib/hostap/src/drivers/driver_nrc_tx.c */
extern int atcmd_send_delba_req (int vif, int tid, uint8_t *addr);

static int _atcmd_test_delba_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_LOG_HELP("AT+DELBA=<tid>");
			break;

		case 1:
		{
			atcmd_wifi_macaddr_t macaddr;
			int tid = atoi(argv[0]);

			if (atcmd_send_delba_req(0, tid, (uint8_t *)macaddr) != 0)
				return ATCMD_ERROR_FAIL;

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_test_delba =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_TEST,

	.cmd = "DELBA",
	.id = ATCMD_TEST_DELBA,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = NULL,
	.handler[ATCMD_HANDLER_SET] = _atcmd_test_delba_set,
};




static atcmd_group_t g_atcmd_group_test =
{
	.list.next = NULL,
	.list.prev = NULL,

	.name = "TEST",
	.id = ATCMD_GROUP_TEST,

	.cmd_prefix = "",
	.cmd_prefix_size = 0,

	.cmd_list_head.next = NULL,
	.cmd_list_head.prev = NULL,
};

static atcmd_info_t *g_atcmd_test[] =
{
	&g_atcmd_test_iperf,
	&g_atcmd_test_addba,
	&g_atcmd_test_delba,
	NULL
};

int atcmd_test_enable (void)
{
	int i;

	if (atcmd_group_register(&g_atcmd_group_test) != 0)
		return -1;

	for (i = 0 ; g_atcmd_test[i] ; i++)
	{
		if (atcmd_info_register(ATCMD_GROUP_TEST, g_atcmd_test[i]) != 0)
			return -1;
	}

	return 0;
}

void atcmd_test_disable (void)
{
	int i;

	for (i = 0 ; g_atcmd_test[i] ; i++)
		atcmd_info_unregister(ATCMD_GROUP_TEST, g_atcmd_test[i]->id);

	atcmd_group_unregister(ATCMD_GROUP_TEST);
}
