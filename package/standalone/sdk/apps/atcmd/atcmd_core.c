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


#include "atcmd.h"


extern void atcmd_firmware_write (char *buf, int len);
extern void atcmd_firmware_download_event_idle (uint32_t len, uint32_t cnt);
extern void atcmd_firmware_download_event_drop (uint32_t len, uint32_t cnt);
extern void atcmd_firmware_download_event_done (uint32_t len);

/**********************************************************************************************/

#if 0
static struct
{
	int val;
	const char *str;
} g_atcmd_errors[] =
{
	{ 0,			"OK"			},
	{ EPERM,		"EPERM"			},	// 1
	{ EBADF,		"EBADF"			},	// 9
	{ EAGAIN,		"EAGAIN"		},	// 11
	{ EBUSY,		"EBUSY"			},	// 16
	{ EINVAL,		"EINVAL"		},	// 22
	{ ENOSPC,		"ENOSPC"		}, 	// 28
	{ ENOTSOCK,		"ENOTSOCK"		}, 	// 88
	{ EPROTOTYPE,	"EPROTOTYPE"	},	// 91
	{ EADDRINUSE,	"EADDRINUSE"	}, 	// 98
	{ ECONNABORTED, "ECONNABORTED" 	},	// 103
	{ ECONNRESET, 	"ECONNRESET" 	},	// 104
	{ ENOTCONN, 	"ENOTCONN" 		},	// 107
	{ ETIMEDOUT, 	"ETIMEDOUT"		},	// 110
	{ ECONNREFUSED,	"ECONNREFUSED"	},	// 111
	{ EHOSTDOWN, 	"EHOSTDOWN" 	},	// 112
	{ EHOSTUNREACH,	"EHOSTUNREACH"	},	// 113
	{ EINPROGRESS, 	"EINPROGRESS" 	},	// 115

	{ -1, "UNKNOWN" }
};

const char *atcmd_strerror (int err)
{
	int i;

	if (err < 0)
		err *= -1;

	for (i = 0 ; g_atcmd_errors[i].val >= 0 ; i++)
	{
		if (g_atcmd_errors[i].val == err)
			break;
	}

	return g_atcmd_errors[i].str;
}
#endif

/*******************************************************************************************/

static uint32_t g_atcmd_config = 0;
static const char *str_atcmd_config[ATCMD_CFG_NUM] =
{
	"ATCMD_CFG_ECHO",
	"ATCMD_CFG_LINEFEED",
	"ATCMD_CFG_PROMPT",
#ifdef CONFIG_ATCMD_LOWERCASE
	"ATCMD_CFG_LOWERCASE",
#endif
#ifdef CONFIG_ATCMD_HISTORY
	"ATCMD_CFG_HISTORY",
#endif
};

static void atcmd_config_enable (enum ATCMD_CFG cfg)
{
	_atcmd_info("%s_ON\n", str_atcmd_config[cfg]);

	g_atcmd_config |= (1 << cfg);
}

static void atcmd_config_disable (enum ATCMD_CFG cfg)
{
	_atcmd_info("%s_OFF\n", str_atcmd_config[cfg]);

	g_atcmd_config &= ~(1 << cfg);
}

static bool atcmd_config_status (enum ATCMD_CFG cfg)
{
	return !!(g_atcmd_config & (1 << cfg));
}

static void atcmd_config_print (void)
{
	int cfg;

	_atcmd_info("[ ATCMD Configurations ]\n");

	for (cfg = ATCMD_CFG_MIN ; cfg <= ATCMD_CFG_MAX ; cfg++)
	{
		_atcmd_info(" - %s: %s\n", str_atcmd_config[cfg],
				atcmd_config_status(cfg) ? "ON" : "OFF");
	}
}

#define ATCMD_ECHO_ENABLE()				atcmd_config_enable(ATCMD_CFG_ECHO)
#define ATCMD_ECHO_DISABLE()			atcmd_config_disable(ATCMD_CFG_ECHO)
#define ATCMD_ECHO_IS_ENABLED()			atcmd_config_status(ATCMD_CFG_ECHO)

#define ATCMD_LINEFEED_ENABLE()			atcmd_config_enable(ATCMD_CFG_LINEFEED)
#define ATCMD_LINEFEED_DISABLE()		atcmd_config_disable(ATCMD_CFG_LINEFEED)
#define ATCMD_LINEFEED_IS_ENABLED()		atcmd_config_status(ATCMD_CFG_LINEFEED)

#define ATCMD_PROMPT_ENABLE()			atcmd_config_enable(ATCMD_CFG_PROMPT)
#define ATCMD_PROMPT_DISABLE()			atcmd_config_disable(ATCMD_CFG_PROMPT)
#define ATCMD_PROMPT_IS_ENABLED()		atcmd_config_status(ATCMD_CFG_PROMPT)

#ifdef CONFIG_ATCMD_LOWERCASE
#define ATCMD_LOWERCASE_ENABLE()		atcmd_config_enable(ATCMD_CFG_LOWERCASE)
#define ATCMD_LOWERCASE_DISABLE()		atcmd_config_disable(ATCMD_CFG_LOWERCASE)
#define ATCMD_LOWERCASE_IS_ENABLED()	atcmd_config_status(ATCMD_CFG_LOWERCASE)
#else
#define ATCMD_LOWERCASE_ENABLE()
#define ATCMD_LOWERCASE_DISABLE()
#define ATCMD_LOWERCASE_IS_ENABLED()	false
#endif

#ifdef CONFIG_ATCMD_HISTORY
#define ATCMD_HISTORY_ENABLE()			atcmd_config_enable(ATCMD_CFG_HISTORY)
#define ATCMD_HISTORY_DISABLE()			atcmd_config_disable(ATCMD_CFG_HISTORY)
#define ATCMD_HISTORY_IS_ENABLED()		atcmd_config_status(ATCMD_CFG_HISTORY)
#else
#define ATCMD_HISTORY_ENABLE()
#define ATCMD_HISTORY_DISABLE()
#define ATCMD_HISTORY_IS_ENABLED()		false
#endif

/*******************************************************************************************/

#define ATCMD_PROMPT		"\rNRC> "
#define ATCMD_PROMPT_LEN	5

static bool g_atcmd_prompt_enter = false;

static void atcmd_prompt_print (void)
{
	if (ATCMD_PROMPT_IS_ENABLED())
		atcmd_transmit(ATCMD_PROMPT, 1 + ATCMD_PROMPT_LEN);
}

static void atcmd_prompt_reset (int cmd_len)
{
	if (g_atcmd_prompt_enter)
	{
		char buf[1 + ATCMD_PROMPT_LEN + ATCMD_MSG_LEN_MAX];
		int prompt_len = ATCMD_PROMPT_LEN;

		if (!ATCMD_PROMPT_IS_ENABLED())
			prompt_len = 0;

		buf[0] = '\r';
		memset(&buf[1], ' ', prompt_len + cmd_len);
		buf[1 + prompt_len + cmd_len] = '\r';

		atcmd_transmit(buf, 1 + prompt_len + cmd_len + 1);

		atcmd_prompt_print();
	}
}

static void atcmd_prompt_enter (void)
{
	if (!g_atcmd_prompt_enter)
	{
		g_atcmd_prompt_enter = true;

		atcmd_prompt_print();
	}
}

static void atcmd_prompt_exit (void)
{
	if (g_atcmd_prompt_enter)
	{
		g_atcmd_prompt_enter = false;

		if (ATCMD_ECHO_IS_ENABLED())
			atcmd_transmit("\r\n", 2);
	}
}

/*******************************************************************************************/

#ifdef CONFIG_ATCMD_HISTORY
/************************************************************/

#define ATCMD_HISTORY_MAX		16

#define ATCMD_HISTORY_PUSH		0
#define ATCMD_HISTORY_POP		1

struct atcmd_history
{
	int push_idx;
	int pop_idx;

	int push_cnt;
	int pop_cnt;

	atcmd_buf_t buf[2][ATCMD_HISTORY_MAX];
} *g_atcmd_history = NULL;

static int atcmd_history_enable (void)
{
	if (g_atcmd_history)
		return 0;

	g_atcmd_history = _atcmd_malloc(sizeof(struct atcmd_history));

	if (g_atcmd_history)
	{
		int i, j;

		memset(g_atcmd_history, 0, sizeof(struct atcmd_history));

		for (i = 0 ; i < 2 ; i++)
			for (j = 0 ; j < ATCMD_HISTORY_MAX ; j++)
				g_atcmd_history->buf[i][j].cnt = -1;

		return 0;
	}
	else
	{
		_atcmd_error("malloc() failed\n");

		return -1;
	}
}

static void atcmd_history_disable (void)
{
	if (g_atcmd_history)
	{
		_atcmd_free(g_atcmd_history);

		g_atcmd_history = NULL;
	}
}

static void atcmd_history_print (void)
{
	if (g_atcmd_history)
	{
		atcmd_buf_t *push_buf = g_atcmd_history->buf[ATCMD_HISTORY_PUSH];
		int push_idx = g_atcmd_history->push_idx;
		int push_cnt = g_atcmd_history->push_cnt;
		int i;

		_atcmd_printf("[ ATCMD_HISTORY : %d ]\n", push_cnt);

		for (i = 0 ; i < push_cnt ; i++)
		{
			push_idx--;
			push_idx &= ATCMD_HISTORY_MAX - 1;

			_atcmd_printf("  - %d. idx=%d, cnt=%d, cmd=%s\n", i,
					push_idx, push_buf[push_idx].cnt, push_buf[push_idx].cmd);
		}
	}
}

static int atcmd_history_push (char *cmd, int cnt)
{
	if (g_atcmd_history)
	{
		int *push_idx = &g_atcmd_history->push_idx;
		int *push_cnt = &g_atcmd_history->push_cnt;
		int *pop_idx = &g_atcmd_history->pop_idx;
		int *pop_cnt = &g_atcmd_history->pop_cnt;
		atcmd_buf_t *push_buf = g_atcmd_history->buf[ATCMD_HISTORY_PUSH];
		atcmd_buf_t *pop_buf = g_atcmd_history->buf[ATCMD_HISTORY_POP];
		atcmd_buf_t *buf = NULL;
		int i;

		if (cmd && cnt)
		{
/*			_atcmd_debug("push: %d/%d, idx=%d, cnt=%d, cmd=%s\n",
					 *push_cnt, ATCMD_HISTORY_MAX, *push_idx, cnt, cmd); */

			buf = &push_buf[*push_idx];

			buf->cnt = cnt;
			memcpy(buf->cmd, cmd, cnt);
			buf->cmd[cnt] = '\0';

			++(*push_idx);
			*push_idx &= (ATCMD_HISTORY_MAX - 1);

			if (*push_cnt < ATCMD_HISTORY_MAX)
				++(*push_cnt);
		}

		*pop_idx = *push_idx;
		*pop_cnt = 0;

		for (i = 0 ; i < ATCMD_HISTORY_MAX ; i++)
			pop_buf[i].cnt = -1;

/*		atcmd_history_print(); */
	}

	return 0;
}

static int atcmd_history_pop (char **cmd, int **cnt, int key)
{
	if (g_atcmd_history)
	{
		int *push_cnt = &g_atcmd_history->push_cnt;
		int *pop_idx = &g_atcmd_history->pop_idx;
		int *pop_cnt = &g_atcmd_history->pop_cnt;
		atcmd_buf_t *push_buf = g_atcmd_history->buf[ATCMD_HISTORY_PUSH];
		atcmd_buf_t *pop_buf = g_atcmd_history->buf[ATCMD_HISTORY_POP];
		atcmd_buf_t *buf = NULL;

		if (!cmd || !cnt)
			return -1;

		switch (key)
		{
			case ATCMD_KEY_UP:
				if (*pop_cnt >= *push_cnt)
					return -1;

				(*pop_idx)--;
				(*pop_cnt)++;
				break;

			case ATCMD_KEY_DOWN:
				if (*pop_cnt == 0)
					return -1;

				(*pop_idx)++;
				(*pop_cnt)--;

				if (*pop_cnt == 0)
				{
					*cmd = NULL;
					*cnt = NULL;

/*				_atcmd_debug("pop: %d/%d, idx=%d\n",
							 *pop_cnt, *push_cnt, *pop_idx); */
					return 0;
				}

				break;

			default:
				return -1;
		}

		*pop_idx &= (ATCMD_HISTORY_MAX - 1);
/*		*pop_cnt &= (ATCMD_HISTORY_MAX - 1); */

		if (pop_buf[*pop_idx].cnt < 0)
		{
			pop_buf[*pop_idx].cnt = push_buf[*pop_idx].cnt;
			strcpy(pop_buf[*pop_idx].cmd, push_buf[*pop_idx].cmd);
		}

		*cnt = &pop_buf[*pop_idx].cnt;
		*cmd = pop_buf[*pop_idx].cmd;

/*		_atcmd_debug("pop: %d/%d, idx=%d, cnt=%d, cmd=%s\n",
					*pop_cnt, *push_cnt, *pop_idx, **cnt, *cmd); */
	}

	return 0;
}

#else

static int atcmd_history_enable (void) { return 0; }
static void atcmd_history_disable (void) {}
static void atcmd_history_print (void) {}
static int atcmd_history_push (char *cmd, int cnt) { return 0; }
static int atcmd_history_pop (char **cmd, int **cnt, int key) { return 0; }

/************************************************************/
#endif /* #ifdef CONFIG_ATCMD_HISTORY */

/*******************************************************************************************/

#define _atcmd_data_mode_debug(fmt, ...)	/* _atcmd_info("data_mode: " fmt, ##__VA_ARGS__) */

typedef struct
{
	TaskHandle_t task;

	bool enable;
	bool idle;
	bool passthrough;
	bool done_event;
	bool binary;
	char *exit_cmd;

	uint32_t cnt;
	uint32_t send_len;
	uint32_t send_done;
	uint32_t send_drop;
	atcmd_socket_t socket;

	uint32_t send_timeout; // msec
	uint32_t last_send_time; // msec
} atcmd_data_mode_t;

static atcmd_data_mode_t g_atcmd_data_mode =
{
	.task = NULL,

	.enable = false,
	.idle = true,
	.passthrough = false,
	.done_event = false,
	.binary = false,
	.exit_cmd = NULL,

	.cnt = 0,
	.send_len = 0,
	.send_done = 0,
	.send_drop = 0,

	.socket =
	{
		.id = -1,
	},

	.send_timeout = 0,
	.last_send_time = 0,
};

static bool atcmd_data_mode_is_enabled (void)
{
	return g_atcmd_data_mode.enable;
}

static int _atcmd_data_mode_enable (atcmd_socket_t *socket, int32_t len, uint32_t timeout,
									char *exit_cmd, bool done_event, bool binary)
{
	if (g_atcmd_data_mode.enable)
	{
		_atcmd_error("enable=%d\n", g_atcmd_data_mode.enable);
		return -1;
	}

	if (socket && socket->id < 0)
		return -1;

	if (timeout == 0)
		return -1;

	if (binary)
	{
		_atcmd_data_mode_debug("enable, id=%d len=%d timeout=%u exit_cmd=%s binary=%d\n",
							socket ? socket->id : -1, len, timeout, exit_cmd ? exit_cmd : "", binary);
	}
	else
	{
		_atcmd_data_mode_debug("enable, id=%d len=%d timeout=%u\n", socket ? socket->id : -1, len, timeout);
	}

	g_atcmd_data_mode.enable = true;
	g_atcmd_data_mode.idle = false;
	g_atcmd_data_mode.passthrough = (len <= 0) ? true : false;
	g_atcmd_data_mode.done_event = done_event;
	g_atcmd_data_mode.binary = binary;
	g_atcmd_data_mode.exit_cmd = exit_cmd ? exit_cmd : "AT\r\n";

	g_atcmd_data_mode.cnt = 0;
	g_atcmd_data_mode.send_len = abs(len);
	g_atcmd_data_mode.send_done = 0;
	g_atcmd_data_mode.send_drop = 0;

	if (socket)
		memcpy(&g_atcmd_data_mode.socket, socket, sizeof(atcmd_socket_t));
	else
		atcmd_socket_reset(&g_atcmd_data_mode.socket);

	g_atcmd_data_mode.send_timeout = timeout + 100;
	g_atcmd_data_mode.last_send_time = 0;

	return 0;
}

int atcmd_data_mode_enable (atcmd_socket_t *socket, int32_t len, bool done_event, uint32_t timeout, char *exit_cmd)
{
	return _atcmd_data_mode_enable(socket, len, timeout, exit_cmd, done_event, false);
}

int atcmd_firmware_download_enable (int32_t len, uint32_t timeout)
{
	return _atcmd_data_mode_enable(NULL, len, timeout, NULL, false, true);
}

static int atcmd_data_mode_disable (void)
{
	if (!g_atcmd_data_mode.enable)
	{
		_atcmd_error("enable=%d\n", g_atcmd_data_mode.enable);
		return -1;
	}

	if (g_atcmd_data_mode.binary)
	{
		_atcmd_data_mode_debug("disable, id=%d binary=%d\n",
								g_atcmd_data_mode.socket.id, g_atcmd_data_mode.binary);
	}
	else
	{
		_atcmd_data_mode_debug("disable, id=%d\n", g_atcmd_data_mode.socket.id);
	}

	g_atcmd_data_mode.enable = false;
	g_atcmd_data_mode.idle = true;

	g_atcmd_data_mode.passthrough = false;
	g_atcmd_data_mode.done_event = false;
	g_atcmd_data_mode.binary = false;
	g_atcmd_data_mode.exit_cmd = NULL;

	g_atcmd_data_mode.cnt = 0;
	g_atcmd_data_mode.send_len = 0;
	g_atcmd_data_mode.send_done = 0;
	g_atcmd_data_mode.send_drop = 0;

	atcmd_socket_reset(&g_atcmd_data_mode.socket);

/*	g_atcmd_data_mode.send_timeout = 0;
	g_atcmd_data_mode.last_send_time = 0; */

	return 0;
}

static void atcmd_data_mode_task_suspend (uint32_t time)
{
	if (g_atcmd_data_mode.task)
	{
		TickType_t ticks = (time == 0) ? portMAX_DELAY : pdMS_TO_TICKS(time);
		uint32_t _time;

		_atcmd_data_mode_debug("suspend %u\n", time);

		_time = atcmd_sys_now();

		ulTaskNotifyTake(pdTRUE, ticks);
	}
}

static void atcmd_data_mode_task_resume (void)
{
	if (g_atcmd_data_mode.task)
	{
		_atcmd_data_mode_debug("resume\n");

		xTaskNotifyGive(g_atcmd_data_mode.task);
	}
}

static void atcmd_data_mode_sending (void)
{
	_atcmd_data_mode_debug("sending\n");

	if (g_atcmd_data_mode.enable)
	{
		g_atcmd_data_mode.idle = false;
		g_atcmd_data_mode.last_send_time = atcmd_sys_now();

		atcmd_data_mode_task_resume();
	}
}

static void atcmd_data_mode_task (void *pvParameters)
{
	uint32_t send_time;
	uint32_t send_timeout;
	uint32_t current_time;
	uint32_t elapsed_time;
	uint32_t suspend_time;

	while (1)
	{
		if (atcmd_data_mode_is_enabled())
		{
			send_time = g_atcmd_data_mode.last_send_time;
			send_timeout = g_atcmd_data_mode.send_timeout;

			if (send_time > 0 && send_timeout > 0)
			{
				current_time = atcmd_sys_now();

				if (current_time >= send_time)
					elapsed_time = (current_time - send_time);
				else
					elapsed_time = (0xffffffff - send_time) + current_time + 1;

/*				_atcmd_data_mode_debug("current=%u send=%u elapsed=%u\n",
										current_time, send_time, elapsed_time); */

				if (elapsed_time < send_timeout)
				{
					suspend_time = send_timeout - elapsed_time;

/*					_atcmd_data_mode_debug("suspend_time=%u\n", suspend_time); */

					atcmd_data_mode_task_suspend(suspend_time);
					continue;
				}

				_atcmd_data_mode_debug("timeout, %ums\n", elapsed_time);

				if (!g_atcmd_data_mode.idle)
				{
					if (g_atcmd_data_mode.enable && send_timeout > 0)
					{
/*						_atcmd_data_mode_debug("%d %u %u %u %u\n", g_atcmd_data_mode.socket.id,
												g_atcmd_data_mode.send_done,
												g_atcmd_data_mode.send_drop,
												g_atcmd_data_mode.cnt,
												elapsed_time); */

						if (g_atcmd_data_mode.binary)
							atcmd_firmware_download_event_idle(g_atcmd_data_mode.send_len, g_atcmd_data_mode.cnt);
						else
						{
							atcmd_socket_event_send_idle(g_atcmd_data_mode.socket.id,
														g_atcmd_data_mode.send_done,
														g_atcmd_data_mode.send_drop,
														g_atcmd_data_mode.cnt,
														elapsed_time - 100);
						}

						g_atcmd_data_mode.idle = true;
					}
				}
			}
		}

		atcmd_data_mode_task_suspend(0);
	}
}

static int atcmd_data_mode_task_create (void)
{
#define ATCMD_DATA_MODE_TASK_PRIORITY		(ATCMD_TASK_PRIORITY + 1)
#define ATCMD_DATA_MODE_TASK_STACK_SIZE		((2 * 1024) / sizeof(StackType_t))

	BaseType_t ret;

	ret = xTaskCreate(atcmd_data_mode_task, "atcmd_data_mode",
						ATCMD_DATA_MODE_TASK_STACK_SIZE,
						NULL,
						ATCMD_DATA_MODE_TASK_PRIORITY,
						&g_atcmd_data_mode.task);

	if (ret == pdPASS && g_atcmd_data_mode.task)
		return 0;

	return -1;
}

static void atcmd_data_mode_task_delete (void)
{
	if (g_atcmd_data_mode.task)
	{
		vTaskDelete(g_atcmd_data_mode.task);
		g_atcmd_data_mode.task = NULL;
	}
}

/*******************************************************************************************/

int atcmd_msg_vsnprint (int type, char *buf, int len, const char *fmt, va_list ap)
{
	const char *prefix = "+";
	int ret = 0;

	switch (type)
	{
		case ATCMD_MSG_TYPE_RETURN:
		case ATCMD_MSG_TYPE_INFO:
		case ATCMD_MSG_TYPE_EVENT:
		case ATCMD_MSG_TYPE_DEBUG:
		case ATCMD_MSG_TYPE_HELP:
			break;

		default:
			return 0;
	}

	ret += snprintf(buf + ret, len - ret, prefix);
	ret += vsnprintf(buf + ret, len - ret, fmt, ap);
	ret += snprintf(buf + ret, len - ret, "\r\n");

	return ret;
}

int atcmd_msg_snprint (int type, char *buf, int len, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	len = atcmd_msg_vsnprint(type, buf, len, fmt, ap);
	va_end(ap);

	return len;
}

int atcmd_msg_print (int type, const char *fmt, ...)
{
	char buf[ATCMD_MSG_LEN_MAX + 1];
	int len;
	va_list ap;

	va_start(ap, fmt);
	len = atcmd_msg_vsnprint(type, buf, sizeof(buf) - 1, fmt, ap);
	va_end(ap);

	len = atcmd_transmit(buf, len);

	return len;
}

/**********************************************************************************************/

static void atcmd_list_init (atcmd_list_t *list)
{
	list->next = NULL;
	list->prev = NULL;
}

static void atcmd_list_add (atcmd_list_t *head, atcmd_list_t *list)
{
	list->next = head;
	list->prev = head->prev;

	head->prev->next = list;
	head->prev = list;
}

static void atcmd_list_del (atcmd_list_t *list)
{
	list->next->prev = list->prev;
	list->prev->next = list->next;

	list->next = NULL;
	list->prev = NULL;
}

/*******************************************************************************************/

static atcmd_list_t g_atcmd_group_head =
{
	.next = NULL,
	.prev = NULL
};

#if 0
void atcmd_group_print (void)
{
	atcmd_group_t *group;
	atcmd_list_t *list;

	_atcmd_info("Group List\n");
	_atcmd_info(" - Head (%p): next=%p prev=%p\n", &g_atcmd_group_head,
							g_atcmd_group_head.next, g_atcmd_group_head.prev);

	for (list = g_atcmd_group_head.prev ; list ; list = list->prev)
	{
		group = (atcmd_group_t *)list;

		_atcmd_info(" - %s (%p): id=%d prefix=%s prefix_size=%d next=%p prev=%p\n",
						group->name, group, group->id,
						group->cmd_prefix, group->cmd_prefix_size,
						group->list.next, group->list.prev);
	}
}
#else
void atcmd_group_print (void) {}
#endif

atcmd_group_t *atcmd_group_search (enum ATCMD_GROUP_ID id)
{
	atcmd_group_t *group;
	atcmd_list_t *list;

	for (list = g_atcmd_group_head.prev ; list ; list = list->prev)
	{
		group = (atcmd_group_t *)list;

		if (group->id == id)
			return group;
	}

	return NULL;
}

int atcmd_group_register (atcmd_group_t *group)
{
	if (!group || atcmd_group_search(group->id))
		return -1;

	atcmd_list_add(&g_atcmd_group_head, &group->list);

	return 0;
}

int atcmd_group_unregister (enum ATCMD_GROUP_ID id)
{
	atcmd_list_t *list = (atcmd_list_t *)atcmd_group_search(id);

	if (!list)
		return -1;

	atcmd_list_del(list);

	return 0;
}

/*******************************************************************************************/

#if 0
void atcmd_info_print (atcmd_group_t *group)
{
	if (group)
	{
		atcmd_info_t *info;
		atcmd_list_t *list;

		_atcmd_info("Command List: %s (%p)\n", group->name, group);
		_atcmd_info(" - Head (%p): next=%p prev=%p\n",
						&group->cmd_list_head,
						group->cmd_list_head.next, group->cmd_list_head.prev);

		for (list = group->cmd_list_head.prev ; list ; list = list->prev)
		{
			info = (atcmd_info_t *)list;

			_atcmd_info(" - %s (%p): id=%d next=%p prev=%p\n",
							info->cmd, info, info->id,
							info->list.next, info->list.prev);
		}
	}
}
#else
void atcmd_info_print (atcmd_group_t *group) {}
#endif

atcmd_info_t *atcmd_info_search (atcmd_group_t *group, enum ATCMD_ID id)
{
	if (group)
	{
		atcmd_info_t *info;
		atcmd_list_t *list;

		for (list = group->cmd_list_head.prev ; list ; list = list->prev)
		{
			info = (atcmd_info_t *)list;

			if (info->id == id)
				return info;
		}
	}

	return NULL;
}

int atcmd_info_register (enum ATCMD_GROUP_ID gid, atcmd_info_t *info)
{
	atcmd_group_t *group = atcmd_group_search(gid);

	if (!group || !info)
		return -1;

	atcmd_list_add(&group->cmd_list_head, &info->list);

	return 0;
}

void atcmd_info_unregister (enum ATCMD_GROUP_ID gid, enum ATCMD_ID id)
{
	atcmd_group_t *group = atcmd_group_search(gid);

	if (group)
	{
		atcmd_info_t *info = atcmd_info_search(group, id);

		if (info)
			atcmd_list_del(&info->list);
	}
}

/*******************************************************************************************/

static void atcmd_parse_print (enum ATCMD_TYPE type, int argc, char **argv)
{
	char *str_type[ATCMD_TYPE_NUM] = { "RUN", "GET", "GET_PARAM", "SET", "SET_HELP" };
	int i;

	if (type == ATCMD_TYPE_NONE || !argc || !argv)
		return;

	_atcmd_info("%s_%s: %d\r\n", argv[0], str_type[type], argc);

	for (i = 1 ; i < argc ; i++)
		_atcmd_info(" - opt%d : %s\r\n", i - 1, argv[i]);
}

static enum ATCMD_HANDLER atcmd_parse (char *cmd, int *argc, char **argv)
{
	enum ATCMD_TYPE type = ATCMD_TYPE_NONE;
	int max_argc = ATCMD_MSG_PARAM_MAX + 1;
	bool set_help = false;
	int i;

	for (i = 0 ; i < max_argc ; i++)
	{
		argv[i] = cmd;

		do
		{
/*			_atcmd_debug("%d.%d: %c\r\n", i, cmd - argv[0], *cmd); */

			switch (*cmd)
			{
				case '=':
					if (i == 0 && type == ATCMD_TYPE_NONE)
						type = ATCMD_TYPE_SET;
					else if (i == 1 && type == ATCMD_TYPE_GET)
					{
						i = 0;
						type = ATCMD_TYPE_GET_PARAM;
					}
					else
						return ATCMD_HANDLER_NONE;

					*cmd = '\0';
					break;

				case ',':
					*cmd = '\0';
					break;

				case '?':
					if (i == 0 && type == ATCMD_TYPE_NONE)
						type = ATCMD_TYPE_GET;
					else if (i == 1 && type == ATCMD_TYPE_SET)
					{
						i = 0;
						type = ATCMD_TYPE_SET_HELP;
					}
					else
						return ATCMD_HANDLER_NONE;

					*cmd = '\0';
					break;

				case '\r':
				case '\n':
					*cmd = '\0';

					*argc = 1;

					if (i == 0)
					{
						if (type == ATCMD_TYPE_NONE)
							type = ATCMD_TYPE_RUN;
					}
					else if (type == ATCMD_TYPE_GET_PARAM || type == ATCMD_TYPE_SET)
					{
						if (strlen(argv[1]) > 0)
							*argc += i;
					}

					strupr(argv[0]);

/*					atcmd_parse_print(type, *argc, argv); */

					switch (type)
					{
						case ATCMD_TYPE_RUN:
							return ATCMD_HANDLER_RUN;

						case ATCMD_TYPE_GET_PARAM:
							if (*argc == 1)
								break;

						case ATCMD_TYPE_GET:
							return ATCMD_HANDLER_GET;

						case ATCMD_TYPE_SET:
							if (*argc == 1)
								break;

						case ATCMD_TYPE_SET_HELP:
							return ATCMD_HANDLER_SET;

						default:
							break;
					}

					return ATCMD_HANDLER_NONE;
			}

		} while (*cmd++ != '\0');
	}

	return ATCMD_HANDLER_NONE;
}

static int atcmd_handler (char *cmd)
{
	enum ATCMD_HANDLER type;
	char *argv[ATCMD_MSG_PARAM_MAX + 1];
	int argc;

	type = atcmd_parse(&cmd[3], &argc, argv);

/*	_atcmd_debug("handler: %d\n", type); */

	if (type != ATCMD_HANDLER_NONE && argc > 0)
	{
		atcmd_group_t *group;
		atcmd_list_t *list;

		for (list = g_atcmd_group_head.prev ; list ; list = list->prev)
		{
			group = (atcmd_group_t *)list;

/*			_atcmd_debug("Group %s: id=%d prefix=%s prefix_size=%d next=%p prev=%p\n",
							group->name, group->id,
							group->cmd_prefix, group->cmd_prefix_size,
							group->list.next, group->list.prev); */

			if (group->cmd_prefix_size == 0 ||
					strncmp(argv[0], group->cmd_prefix, group->cmd_prefix_size) == 0)
			{
				atcmd_info_t *atcmd;

				for (list = group->cmd_list_head.prev ; list ; list = list->prev)
				{
					atcmd = (atcmd_info_t *)list;

/*					_atcmd_debug("Command %s: id=%d next=%p prev=%p\n",
									atcmd->cmd, atcmd->id,
									atcmd->list.next, atcmd->list.prev); */

					if (strcmp(argv[0] + group->cmd_prefix_size, atcmd->cmd) == 0)
					{
						int ret = ATCMD_ERROR_NOTSUPP;

						if (atcmd->handler[type])
							ret = atcmd->handler[type](argc - 1, argv + 1);

						if (ret == ATCMD_NO_RETURN)
							ret = ATCMD_SUCCESS;
						else
						{
							if (ret == ATCMD_ERROR_NOTSUPP)
								ATCMD_MSG_RETURN(NULL, ret);
							else
								ATCMD_MSG_RETURN(argv[0], ret);
						}

						if (ret != ATCMD_SUCCESS)
							_atcmd_info("cmd=%s ret=%d\n", argv[0], ret);

						return ret;
					}
				}

				if (group->cmd_prefix_size > 0)
					break;
			}

			list = &group->list;
		}
	}

	_atcmd_info("invalid, %s\n", cmd);

	ATCMD_MSG_RETURN(NULL, ATCMD_ERROR_INVAL);

	return ATCMD_ERROR_INVAL;
}

static int atcmd_key_code (uint8_t c)
{
	static bool esc = false;
	static char buf[3] = { 0, };
	static int cnt = 0;

	if (c < 0x20 || c > 0x7E)
	{
		switch (c)
		{
			case 0x08: // backspace
				return ATCMD_KEY_BS;

			case 0x0A: // line feed
				return ATCMD_KEY_LF;

			case 0x0D: // carriage return
				return ATCMD_KEY_CR;

			case 0x1B: // escape
				esc = true;
				cnt = 0;
				return ATCMD_KEY_ESC;

			default:
				return ATCMD_KEY_IGNORE;
		}
	}

	if (esc)
	{
		buf[cnt] = c;

		switch (cnt++)
		{
			case 0:
		   		if (c == '[')
					return ATCMD_KEY_IGNORE;

				esc = false;
				return ATCMD_KEY_RESET;

			case 1:
				if (c >= 'A' && c <= 'D') /* UP, DOWN, RIGHT, LEFT */
				{
					esc = false;
					return ATCMD_KEY_UP + c - 'A';
				}

				return ATCMD_KEY_IGNORE;

			case 2:
				esc = false;

				if (c == '~' && buf[1] >= '1' && buf[1] <= '6')
					return ATCMD_KEY_HOME + buf[1] - '1';

				return ATCMD_KEY_RESET;
		}
	}

	return ATCMD_KEY_PRINT;
}

/*******************************************************************************************/

static int atcmd_compare_command (const char *cmd, const char *cmd_upr, int size)
{
	if (memcmp(cmd, cmd_upr, size) == 0)
		return 0;

	if (ATCMD_LOWERCASE_IS_ENABLED())
	{
		char cmd_lwr[30];

		if (size <= sizeof(cmd_lwr))
		{
			strcpy(cmd_lwr, cmd_upr);
			strlwr(cmd_lwr);

			if (memcmp(cmd, cmd_lwr, size) == 0)
				return 0;
		}
	}

	return -1;
}

static void atcmd_process_command (char *cmd, int len)
{
	int ret = ATCMD_ERROR_INVAL;

	if (len >= 2)
	{
		cmd[len] = '\0';
		cmd = strstr(cmd, "AT");
		if (!cmd)
			len = 0;
		else
		{
			len = strlen(cmd);
			cmd[len] = '\n';
		}
	}

	switch (len)
	{
		case 2:
			if (atcmd_compare_command(cmd, "AT", 2) == 0)
				ret = ATCMD_SUCCESS;
			break;

		case 3:
			if (atcmd_compare_command(cmd, "ATE", 3) == 0)
			{
				if (ATCMD_ECHO_IS_ENABLED())
					ATCMD_ECHO_DISABLE();
				else
					ATCMD_ECHO_ENABLE();

				ret = ATCMD_SUCCESS;
			}
			else if (atcmd_compare_command(cmd, "ATH", 3) == 0)
			{
				if (ATCMD_HISTORY_IS_ENABLED())
					ATCMD_HISTORY_DISABLE();
				else
					ATCMD_HISTORY_ENABLE();

				ret = ATCMD_SUCCESS;
			}
			else if (atcmd_compare_command(cmd, "ATZ", 3) == 0)
			{
				_atcmd_info("System Reset\n");
				_delay_ms(1);
				util_fota_reboot_firmware();
			}

			break;

		case 4:
			if (atcmd_compare_command(cmd, "ATE0", 4) == 0)
			{
				if (ATCMD_ECHO_IS_ENABLED())
					ATCMD_ECHO_DISABLE();

				ret = ATCMD_SUCCESS;
			}
			else if (atcmd_compare_command(cmd, "ATE1", 4) == 0)
			{
				if (!ATCMD_ECHO_IS_ENABLED())
					ATCMD_ECHO_ENABLE();

				ret = ATCMD_SUCCESS;
			}
			else if (atcmd_compare_command(cmd, "ATH0", 4) == 0)
			{
				if (ATCMD_HISTORY_IS_ENABLED())
					ATCMD_HISTORY_DISABLE();

				ret = ATCMD_SUCCESS;
			}
			else if (atcmd_compare_command(cmd, "ATH1", 4) == 0)
			{
				if (!ATCMD_HISTORY_IS_ENABLED())
					ATCMD_HISTORY_ENABLE();

				ret = ATCMD_SUCCESS;
			}

			break;

		default:
			if (len < 2 || len > ATCMD_MSG_LEN_MAX)
			{
				_atcmd_info("no command\n");
				return;
			}

			if (atcmd_compare_command(cmd, "AT+", 3) == 0)
			{
				cmd[len + 1] = '\0';

				atcmd_handler(cmd);
				return;
			}
	}

	if (ret)
	{
		cmd[len] = '\0';
		_atcmd_info("cmd=%s len=%d ret=%d\n", cmd, len, ret);
	}

	ATCMD_MSG_RETURN(NULL, ret); /* success or invalid */
}

int atcmd_receive_command (char *buf, int len)
{
//#define ATCMD_RXBUF_STATIC

#ifdef ATCMD_RXBUF_STATIC
	static atcmd_buf_t _atcmd_buf = { 0, };
	static atcmd_buf_t *atcmd_buf = &_atcmd_buf;
	static char *cmd = atcmd_buf->cmd;
	static int *cnt = &atcmd_buf->cnt;
#else
	static atcmd_buf_t *atcmd_buf = NULL;
	static char *cmd = NULL;
	static int *cnt = NULL;
#endif
	static int cnt_CR = 0;
	int key_code;
	int i;

#ifndef ATCMD_RXBUF_STATIC
	if (!atcmd_buf)
	{
		atcmd_buf = _atcmd_malloc(sizeof(atcmd_buf_t));
		if (!atcmd_buf)
		{
			_atcmd_error("malloc() failed\n");
			return 0;
		}

		memset(atcmd_buf, 0, sizeof(atcmd_buf_t));

		cmd = atcmd_buf->cmd;
		cnt = &atcmd_buf->cnt;
	}
#endif

	for (i = 0 ; i < len ; i++)
	{
		cmd[*cnt] = buf[i];

/*		_atcmd_debug("rx.%d: %c (0x%02X)\n", *cnt, cmd[*cnt], cmd[*cnt]); */

		key_code = atcmd_key_code(cmd[*cnt]);

		if (ATCMD_LINEFEED_IS_ENABLED())
		{
			if (key_code == ATCMD_KEY_CR)
			{
				if (++cnt_CR == 10)
				{
					ATCMD_LINEFEED_DISABLE();
					ATCMD_MSG_INFO("LINEFEED", "%d", 0);
				}

				continue;
			}

			cnt_CR = 0;
		}

		switch (key_code)
		{
			case ATCMD_KEY_UP:
			case ATCMD_KEY_DOWN:
				if (ATCMD_HISTORY_IS_ENABLED())
				{
					cmd[*cnt] = '\0';

					atcmd_prompt_reset(*cnt);

					if (atcmd_history_pop(&cmd, &cnt, key_code) == 0)
					{
						if (!cmd && !cnt)
						{
							cmd = atcmd_buf->cmd;
							cnt = &atcmd_buf->cnt;
						}
					}

					atcmd_transmit(cmd, *cnt);
				}
				break;

			case ATCMD_KEY_BS:
				if (*cnt > 0)
				{
					(*cnt)--;

					if (ATCMD_ECHO_IS_ENABLED())
						atcmd_transmit("\b \b", 3);
				}

				break;

			case ATCMD_KEY_PRINT:
			{
#ifdef CONFIG_ATCMD_PREFIX_CHECK
				const char *prefix_upper = "AT";
				const char *prefix_lower = "at";

				if (*cnt < 2)
				{
				 	if (cmd[*cnt] != prefix_upper[*cnt])
					{
						if (!ATCMD_LOWERCASE_IS_ENABLED() ||
							cmd[*cnt] != prefix_lower[*cnt])
						{
							atcmd_prompt_reset(*cnt);
							*cnt = 0;
							break;
						}
					}
				}
#endif
				if (ATCMD_ECHO_IS_ENABLED())
					atcmd_transmit(&cmd[*cnt], 1);

				if (*cnt < ATCMD_MSG_LEN_MAX)
					(*cnt)++;

				break;
			}

			case ATCMD_KEY_CR:
				if (ATCMD_LINEFEED_IS_ENABLED())
					break;

			case ATCMD_KEY_LF:
				if (*cnt == 0)
					break;

				atcmd_prompt_exit();

				if (cmd != atcmd_buf->cmd)
				{
					atcmd_buf->cnt = *cnt;
					memcpy(atcmd_buf->cmd, cmd, *cnt + 1);

					cnt = &atcmd_buf->cnt;
					cmd = atcmd_buf->cmd;
				}

				if (ATCMD_HISTORY_IS_ENABLED())
					atcmd_history_push(cmd, *cnt);

				atcmd_process_command(cmd, *cnt);

				*cnt = 0;
				atcmd_prompt_enter();

				return (i + 1);

			case ATCMD_KEY_RESET:
				if (ATCMD_HISTORY_IS_ENABLED())
					atcmd_history_push(NULL, 0);

				atcmd_prompt_enter();

				cmd = atcmd_buf->cmd;
				cnt = &atcmd_buf->cnt;;
				*cnt = 0;
				break;
		}
	}

	return i;
}

static int _atcmd_receive_binary (char *buf, int len)
{
	atcmd_firmware_write(buf, len);
	atcmd_firmware_download_event_done(g_atcmd_data_mode.send_len);

	return len;
}

static int _atcmd_receive_data (char *buf, int len)
{
	int ret = 0;

	if (!buf || len <= 0)
		return 0;

	ret = atcmd_socket_send_data(&g_atcmd_data_mode.socket, buf, len);
	if (ret < len)
	{
		g_atcmd_data_mode.send_drop += len - ret;
		atcmd_socket_event_send_drop(g_atcmd_data_mode.socket.id, len - ret);
	}

	return ret;
}

static int atcmd_receive_data (char *buf, int len)
{
	static char _buf[ATCMD_DATA_LEN_MAX];
	int send_len = 0;
	int send_done = 0;

	if (!g_atcmd_data_mode.enable || !len)
	   return 0;

	if (g_atcmd_data_mode.idle)
	{
		static int cnt_exit_cmd = 0;
		char *str_exit_cmd = NULL;
		int len_exit_cmd = 0;
		int i;

		if (g_atcmd_data_mode.exit_cmd)
			str_exit_cmd = g_atcmd_data_mode.exit_cmd;
		else
		{
			_atcmd_error("no data mode exit command\n");

			str_exit_cmd = "AT\r\n";
		}

		len_exit_cmd = strlen(str_exit_cmd);

		for (i = 0 ; i < len ; i++)
		{
			if (buf[i] != str_exit_cmd[cnt_exit_cmd])
			{
				cnt_exit_cmd = 0;
				break;
			}

			if (++cnt_exit_cmd == len_exit_cmd)
			{
				ATCMD_MSG_RETURN(NULL, ATCMD_SUCCESS);

				if (g_atcmd_data_mode.send_len > 0)
				{
					if (g_atcmd_data_mode.cnt > 0)
					{
						if (g_atcmd_data_mode.binary)
							_atcmd_info("BINDL: timeout, %d\n", g_atcmd_data_mode.cnt);
						else
						{
							_atcmd_info("SEND: timeout, %d\n", g_atcmd_data_mode.cnt);

							g_atcmd_data_mode.send_done += _atcmd_receive_data(_buf, g_atcmd_data_mode.cnt);
						}
					}
				}

				cnt_exit_cmd = 0;

				if (g_atcmd_data_mode.binary)
				{
					uint32_t send_len = g_atcmd_data_mode.send_len;
					uint32_t cnt = g_atcmd_data_mode.cnt;

					atcmd_data_mode_disable();

					atcmd_firmware_download_event_drop(send_len, cnt);
				}
				else
				{
					int id = g_atcmd_data_mode.socket.id;
					uint32_t send_done = g_atcmd_data_mode.send_done;
					uint32_t send_drop = g_atcmd_data_mode.send_drop;

					atcmd_data_mode_disable();

					atcmd_socket_event_send_exit(id, send_done, send_drop);
				}

				return i;
			}
		}

		if (i == len)
			return len;

		_atcmd_info("SEND: continue\n");
	}

	if (g_atcmd_data_mode.send_len > 0)
	{
		send_len = g_atcmd_data_mode.send_len;

		if ((g_atcmd_data_mode.cnt + len) > send_len)
			len = send_len - g_atcmd_data_mode.cnt;
		else if ((g_atcmd_data_mode.cnt + len) < send_len)
		{
			memcpy(_buf + g_atcmd_data_mode.cnt, buf, len);
			g_atcmd_data_mode.cnt += len;

			_atcmd_data_mode_debug("data: %d/%d\n", g_atcmd_data_mode.cnt, send_len);

			goto recv_data_done;
		}

		if (g_atcmd_data_mode.cnt > 0)
		{
			memcpy(_buf + g_atcmd_data_mode.cnt, buf, len);
			buf = _buf;
		}
	}
	else if (g_atcmd_data_mode.passthrough)
		send_len = len;

	if (g_atcmd_data_mode.binary)
		send_done = _atcmd_receive_binary(buf, send_len);
	else
		send_done = _atcmd_receive_data(buf, send_len);

	g_atcmd_data_mode.send_done += send_done;

	if (g_atcmd_data_mode.passthrough)
	{
		g_atcmd_data_mode.cnt = 0;

		if (g_atcmd_data_mode.done_event)
			atcmd_socket_event_send_done(g_atcmd_data_mode.socket.id, send_done);
	}
	else
	{
		int id = g_atcmd_data_mode.socket.id;
		bool done_event = g_atcmd_data_mode.done_event;

		atcmd_data_mode_disable();

		if (done_event)
			atcmd_socket_event_send_done(id, send_done);
	}

recv_data_done:

	atcmd_data_mode_sending();

	return len;
}

void atcmd_receive (char *buf, int len)
{
	int ret = 0;
	int i;

	if (!buf || !len)
		return;

	for (i = 0 ; i < len ; i += ret)
	{
		if (atcmd_data_mode_is_enabled())
			ret = atcmd_receive_data(buf + i, len - i);
		else
			ret = atcmd_receive_command(buf + i, len - i);
	}
}

/*******************************************************************************************/

static SemaphoreHandle_t g_atcmd_tx_mutex = NULL;

static int atcmd_tx_mutex_create (void)
{
	g_atcmd_tx_mutex = xSemaphoreCreateMutex();

	return !g_atcmd_tx_mutex ? -1 : 0;
}

static void atcmd_tx_mutex_delete (void)
{
	if (g_atcmd_tx_mutex)
		vSemaphoreDelete(g_atcmd_tx_mutex);
}

static bool atcmd_tx_mutex_take (void)
{
	bool take = false;
	int time_ms = 60000;

	if (g_atcmd_tx_mutex)
		take = !!xSemaphoreTake(g_atcmd_tx_mutex, pdMS_TO_TICKS(time_ms));

	if (!take)
		_atcmd_error("timeout=%d\n", time_ms);

	return take;
}
#define ATCMD_TX_LOCK()		ASSERT(atcmd_tx_mutex_take())

static bool atcmd_tx_mutex_give (void)
{
	bool give = false;

	if (g_atcmd_tx_mutex)
		give = !!xSemaphoreGive(g_atcmd_tx_mutex);

	if (!give)
		_atcmd_error("error\n");

	return give;
}
#define ATCMD_TX_UNLOCK()	ASSERT(atcmd_tx_mutex_give())

int atcmd_transmit_return (char *cmd, int ret)
{
	int len = 0;

/*	_atcmd_debug("%s: ret=%d\r\n", cmd, ret); */

	if (ret == ATCMD_SUCCESS)
	{
		len = atcmd_transmit("OK\r\n", 2 + 2);

		if (atcmd_data_mode_is_enabled())
			atcmd_data_mode_sending();
	}
	else
	{
		if (cmd)
			atcmd_msg_print(ATCMD_MSG_TYPE_RETURN, "%s:%d", cmd, ret);

		len = atcmd_transmit("ERROR\r\n", 5 + 2);
	}

	return len;
}

int atcmd_transmit (char *buf, int len)
{
	int i;

	ATCMD_TX_LOCK();

	for (i = 0 ; i < len ; )
		i += _hif_write(&buf[i], len - i);

	ATCMD_TX_UNLOCK();

	return len;
}

/*******************************************************************************************/

int atcmd_enable (_hif_info_t *info)
{
#ifdef CONFIG_ATCMD_TRXBUF_STATIC
	static char hif_rx_buf[ATCMD_RXBUF_SIZE];
#else
	char *hif_rx_buf = NULL;
#endif
	int ret;

	if (!info)
		return -1;

#ifndef CONFIG_ATCMD_TRXBUF_STATIC
	hif_rx_buf = _atcmd_malloc(ATCMD_RXBUF_SIZE);
	if (!hif_rx_buf)
	{
		_atcmd_error("malloc() failed\n");
		return -1;
	}
#endif

	if (atcmd_tx_mutex_create() != 0)
		return -1;

	if (atcmd_data_mode_task_create() != 0)
		return -1;

	_atcmd_info("ATCMD_TASK_PRIORITY: %d (%d)\n", ATCMD_TASK_PRIORITY, ATCMD_DATA_MODE_TASK_PRIORITY);
	_atcmd_info("ATCMD_MSG_LEN_MIN: %d\n", ATCMD_MSG_LEN_MIN);
	_atcmd_info("ATCMD_MSG_LEN_MAX: %d\n", ATCMD_MSG_LEN_MAX);
	_atcmd_info("ATCMD_DATA_LEN_MAX: %d\n", ATCMD_DATA_LEN_MAX);
	_atcmd_info("ATCMD_TXBUF_SIZE: %d\n", ATCMD_TXBUF_SIZE);
	_atcmd_info("ATCMD_RXBUF_SIZE: %d\n", ATCMD_RXBUF_SIZE);

	ATCMD_PROMPT_DISABLE();
	ATCMD_ECHO_DISABLE();
	ATCMD_HISTORY_DISABLE();
	ATCMD_LOWERCASE_DISABLE();
	ATCMD_LINEFEED_ENABLE();

	if (ATCMD_HISTORY_IS_ENABLED())
		atcmd_history_enable();
	else
		atcmd_history_disable();

	atcmd_user_enable();
	atcmd_socket_enable();
	atcmd_wifi_enable();
	atcmd_basic_enable();

	atcmd_group_print();

	info->rx_params.buf.addr = hif_rx_buf;
	info->rx_params.buf.size = ATCMD_RXBUF_SIZE;
	info->rx_params.cb = atcmd_receive;

	ret = _hif_open(info);
	if (ret == 0)
		atcmd_prompt_enter();

	atcmd_wifi_deep_sleep_send_event();

	return ret;
}

void atcmd_disable (void)
{
	_hif_close();

	atcmd_basic_disable();
	atcmd_wifi_disable();
	atcmd_socket_disable();
	atcmd_user_disable();

	atcmd_data_mode_task_delete();

	atcmd_tx_mutex_delete();
}

/**********************************************************************************************/

static int cmd_atcmd_list (cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_SUCCESS;

	switch (--argc)
	{
		case 0:
		{
			atcmd_group_t *group;
			atcmd_info_t *info;
			atcmd_list_t *list_group;
			atcmd_list_t *list_info;

			for (list_group = g_atcmd_group_head.prev ; list_group ; list_group = list_group->prev)
			{
				group = (atcmd_group_t *)list_group;

				_atcmd_info("[ %s (%d, %s) ]\n", group->name, group->id, group->cmd_prefix);

				for (list_info = group->cmd_list_head.prev ; list_info->prev ; list_info = list_info->prev);

				for ( ; list_info != &group->cmd_list_head ; list_info = list_info->next)
				{
					info = (atcmd_info_t *)list_info;

					_atcmd_info("  - %-12s : %3d, %c%c%c\n",
									info->cmd, info->id,
									info->handler[ATCMD_HANDLER_RUN] ? 'R' : ' ',
									info->handler[ATCMD_HANDLER_GET] ? 'G' : ' ',
									info->handler[ATCMD_HANDLER_SET] ? 'S' : ' ');
				}

				_atcmd_info("\n");
			}

			break;
		}

		default:
			ret = CMD_RET_USAGE;
	}

	return ret;
}

SUBCMD_MAND(atcmd,
		list,
		cmd_atcmd_list,
		"command list info",
		"atcmd list");

