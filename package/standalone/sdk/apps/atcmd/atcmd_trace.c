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


#ifdef CONFIG_ATCMD_TRACE
/**********************************************************************************************/

#define atcmd_trace_log(id) 	if (g_atcmd_trace_log_on) _atcmd_info("%s: %d\n", __func__, id)

static atcmd_trace_t *g_atcmd_trace = NULL;
static bool g_atcmd_trace_log_on = false;


int atcmd_trace_init (void)
{
	g_atcmd_trace = _atcmd_malloc(sizeof(atcmd_trace_t));

	if (!g_atcmd_trace)
	{
		_atcmd_error("malloc() failed\n");
		return -1;
	}

	memset(g_atcmd_trace, 0, sizeof(atcmd_trace_t));

	return 0;
}

void atcmd_trace_exit (void)
{
	if (g_atcmd_trace)
		_atcmd_free(g_atcmd_trace);
}

void atcmd_trace_task_loop (int id)
{
	atcmd_trace_log(id);

	if (g_atcmd_trace)
		g_atcmd_trace->task[id].loop++;
}

void atcmd_trace_task_suspend (int id)
{
	atcmd_trace_log(id);

	if (g_atcmd_trace)
		g_atcmd_trace->task[id].suspend++;
}

void atcmd_trace_task_resume (int id)
{
	atcmd_trace_log(id);

	if (g_atcmd_trace)
		g_atcmd_trace->task[id].resume++;
}

void atcmd_trace_mutex_take (int id)
{
	atcmd_trace_log(id);

	if (g_atcmd_trace)
		g_atcmd_trace->mutex[id].take++;
}

void atcmd_trace_mutex_give (int id)
{
	atcmd_trace_log(id);

	if (g_atcmd_trace)
		g_atcmd_trace->mutex[id].give++;
}

void atcmd_trace_func_call (int id)
{
	atcmd_trace_log(id);

	if (g_atcmd_trace)
	{
		g_atcmd_trace->func[id].call = true;
		g_atcmd_trace->func[id].cnt++;
	}
}

void atcmd_trace_func_return (int id)
{
	atcmd_trace_log(id);

	if (g_atcmd_trace)
		g_atcmd_trace->func[id].call = false;
}

/**********************************************************************************************/

static int atcmd_trace_info_handler(void)
{
	const char *str_task[ATCMD_TRACE_TASK_NUM] =
	{
		[ATCMD_TRACE_TASK_HIF_RX] = "hif_rx",
		[ATCMD_TRACE_TASK_DATA_MODE] = "data_mode",
	};
	const char *str_mutex[ATCMD_TRACE_MUTEX_NUM] =
	{
		[ATCMD_TRACE_MUTEX_LOG] = "log",
		[ATCMD_TRACE_MUTEX_TX] = "tx",
		[ATCMD_TRACE_MUTEX_FDS] = "fds",
	};
	const char *str_func[ATCMD_TRACE_FUNC_NUM] =
	{
		[ATCMD_TRACE_FUNC_SELECT] = "select",
		[ATCMD_TRACE_FUNC_SEND] = "send",
		[ATCMD_TRACE_FUNC_SENDTO] = "sendto",
		[ATCMD_TRACE_FUNC_RECV] = "recv",
		[ATCMD_TRACE_FUNC_RECVFROM] = "recvfrom",
		[ATCMD_TRACE_FUNC_EVTSET] = "evtset",
		[ATCMD_TRACE_FUNC_EVTWAIT] = "evtwait",
	};

	if (g_atcmd_trace)
	{
		int i;

		_atcmd_info("\r\n");

		_atcmd_info(" Task\r\n");

		for (i = 0 ; i < ATCMD_TRACE_TASK_NUM ; i++)
		{
			_atcmd_info("  - %s: loop=%u suspend=%u resume=%u\n",
					str_task[i],
					g_atcmd_trace->task[i].loop,
					g_atcmd_trace->task[i].suspend,
					g_atcmd_trace->task[i].resume);
		}

		_atcmd_info(" Mutex\r\n");

		for (i = 0 ; i < ATCMD_TRACE_MUTEX_NUM ; i++)
		{
			_atcmd_info("  - %s: take=%u give=%u\n",
					str_mutex[i],
					g_atcmd_trace->mutex[i].take,
					g_atcmd_trace->mutex[i].give);
		}

		_atcmd_info(" Function\r\n");

		for (i = 0 ; i < ATCMD_TRACE_FUNC_NUM ; i++)
		{
			_atcmd_info("  - %s: cnt=%u call=%u\n",
					str_func[i],
					g_atcmd_trace->func[i].cnt,
					g_atcmd_trace->func[i].call);
		}

		_atcmd_info(" Log: %s\n", g_atcmd_trace_log_on ? "on" : "off");
	
		return CMD_RET_SUCCESS;
	}

	return CMD_RET_FAILURE;
}

static int atcmd_trace_cli_handler(cmd_tbl_t *t, int argc, char *argv[])
{
	if (argc < 3 || strcmp(argv[0], "atcmd") != 0 || strcmp(argv[1], "trace") != 0)
		return CMD_RET_FAILURE;

	if (strcmp(argv[2], "info") == 0)
		atcmd_trace_info_handler();
	else if (strcmp(argv[2], "log") == 0)
	{
		if (strcmp(argv[3], "on") == 0)
		{
			_atcmd_info("log on\n");
			g_atcmd_trace_log_on = true;
		}
		else if (strcmp(argv[3], "off") == 0)
		{
			_atcmd_info("log off\n");
			g_atcmd_trace_log_on = false;
		}
		else
			return CMD_RET_FAILURE;
	}
	else
		return CMD_RET_FAILURE;

	return CMD_RET_SUCCESS;
}

CMD(atcmd,
    atcmd_trace_cli_handler,
    "atcmd trace",
    "atcmd trace {info|log {on|off}}");

/**********************************************************************************************/
#endif /* #ifdef CONFIG_ATCMD_TRACE */

