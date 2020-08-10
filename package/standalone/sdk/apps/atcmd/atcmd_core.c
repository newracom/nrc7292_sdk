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


#include "hif.h"
#include "atcmd.h"

/*******************************************************************************************/

static uint32_t g_atcmd_config = 0;
static const char *str_atcmd_config[ATCMD_CFG_NUM] =
{
	"ATCMD_CFG_PROMPT",
	"ATCMD_CFG_ECHO",
	"ATCMD_CFG_HISTORY",
	"ATCMD_CFG_LOWERCASE",
	"ATCMD_CFG_LINEFEED",
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

#define ATCMD_PROMPT_ENABLE()			atcmd_config_enable(ATCMD_CFG_PROMPT)
#define ATCMD_PROMPT_DISABLE()			atcmd_config_disable(ATCMD_CFG_PROMPT)
#define ATCMD_PROMPT_IS_ENABLED()		atcmd_config_status(ATCMD_CFG_PROMPT)

#define ATCMD_ECHO_ENABLE()				atcmd_config_enable(ATCMD_CFG_ECHO)
#define ATCMD_ECHO_DISABLE()			atcmd_config_disable(ATCMD_CFG_ECHO)
#define ATCMD_ECHO_IS_ENABLED()			atcmd_config_status(ATCMD_CFG_ECHO)

#define ATCMD_HISTORY_ENABLE()			atcmd_config_enable(ATCMD_CFG_HISTORY)
#define ATCMD_HISTORY_DISABLE()			atcmd_config_disable(ATCMD_CFG_HISTORY)
#define ATCMD_HISTORY_IS_ENABLED()		atcmd_config_status(ATCMD_CFG_HISTORY)

#define ATCMD_LOWERCASE_ENABLE()		atcmd_config_enable(ATCMD_CFG_LOWERCASE)
#define ATCMD_LOWERCASE_DISABLE()		atcmd_config_disable(ATCMD_CFG_LOWERCASE)
#define ATCMD_LOWERCASE_IS_ENABLED()	atcmd_config_status(ATCMD_CFG_LOWERCASE)

#define ATCMD_LINEFEED_ENABLE()			atcmd_config_enable(ATCMD_CFG_LINEFEED)
#define ATCMD_LINEFEED_DISABLE()		atcmd_config_disable(ATCMD_CFG_LINEFEED)
#define ATCMD_LINEFEED_IS_ENABLED()		atcmd_config_status(ATCMD_CFG_LINEFEED)

/*******************************************************************************************/

#define ATCMD_PROMPT		"\rNRC> "
#define ATCMD_PROMPT_LEN	5

static bool g_atcmd_prompt_enter = false;

#define ATCMD_IS_PROMPT()	g_atcmd_prompt_enter

static void atcmd_prompt_print (void)
{
	if (ATCMD_PROMPT_IS_ENABLED())
		atcmd_transmit(ATCMD_PROMPT, 1 + ATCMD_PROMPT_LEN);
}

static void atcmd_prompt_reset (int cmd_len)
{
	if (g_atcmd_prompt_enter)
	{
		char buf[1 + ATCMD_PROMPT_LEN + ATCMD_LEN_MAX];
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

#define ATCMD_DATA_MODE_STATIC

typedef struct
{
#ifdef ATCMD_DATA_MODE_STATIC
	bool enable;
#endif

	bool idle;
	bool error;

	bool sync;
	bool passthrough;

	uint32_t cnt;
	uint32_t len;
	atcmd_socket_t socket;

	uint32_t send_timeout; // msec
	uint32_t last_send_time; // msec

} atcmd_data_mode_t;

#ifdef ATCMD_DATA_MODE_STATIC
static atcmd_data_mode_t _atcmd_data_mode =
{
	.enable = false,

	.idle = true,
	.error= false,

	.sync = false,
	.passthrough = false,

	.cnt = 0,
	.len = 0,
	.socket =
	{
		.id = -1,
	},

	.send_timeout = 0,
	.last_send_time = 0,
};
static atcmd_data_mode_t *g_atcmd_data_mode = &_atcmd_data_mode;
#else
static atcmd_data_mode_t *g_atcmd_data_mode = NULL;
#endif

static bool ATCMD_IS_DATA_MODE (void)
{
#ifdef ATCMD_DATA_MODE_STATIC
	return g_atcmd_data_mode->enable;
#else
	return g_atcmd_data_mode ? true : false;
#endif
}

#define _atcmd_data_mode_trace(fmt, ...)	/* _atcmd_info(fmt, ##__VA_ARGS__) */

void ATCMD_DATA_MODE_ENABLE (atcmd_socket_t *socket, uint32_t len, uint32_t timeout, bool sync)
{
	_atcmd_data_mode_trace("DATA_MODE_ON: id=%d len=%d timeout=%u\n", socket->id, len, timeout);

#ifdef ATCMD_DATA_MODE_STATIC
	g_atcmd_data_mode->enable = true;
#else
	if (g_atcmd_data_mode)
		return;

	g_atcmd_data_mode = _atcmd_malloc(sizeof(struct atcmd_data_mode));
	if (!g_atcmd_data_mode)
	{
		_atcmd_error("malloc() failed\n");
		return;
	}
#endif

	g_atcmd_data_mode->idle = false;
	g_atcmd_data_mode->error = false;

	g_atcmd_data_mode->sync = sync;
	g_atcmd_data_mode->passthrough = (len == 0) ? true : false;

	g_atcmd_data_mode->cnt = 0;
	g_atcmd_data_mode->len = len;
	memcpy(&g_atcmd_data_mode->socket, socket, sizeof(atcmd_socket_t));

	g_atcmd_data_mode->send_timeout = timeout;
	g_atcmd_data_mode->last_send_time = sys_now();
}

void ATCMD_DATA_MODE_DISABLE (void)
{
	_atcmd_data_mode_trace("DATA_MODE_OFF: id=%d\n", g_atcmd_data_mode->socket.id);

#ifdef ATCMD_DATA_MODE_STATIC
	g_atcmd_data_mode->enable = false;

	g_atcmd_data_mode->idle = true;
	g_atcmd_data_mode->error = false;

	g_atcmd_data_mode->sync = false;
	g_atcmd_data_mode->passthrough = false;

	g_atcmd_data_mode->cnt = 0;
	g_atcmd_data_mode->len = 0;
	atcmd_socket_reset(&g_atcmd_data_mode->socket);

	g_atcmd_data_mode->send_timeout = 0;
	g_atcmd_data_mode->last_send_time = 0;
#else
	if (g_atcmd_data_mode)
	{
		_atcmd_free(g_atcmd_data_mode);

		g_atcmd_data_mode = NULL;
	}
#endif
}

static TaskHandle_t g_atcmd_data_mode_task = NULL;

static void atcmd_data_mode_task_suspend (TickType_t ticks)
{
	if (g_atcmd_data_mode_task)
	{
/*		_atcmd_info("data_mode: suspend %u\n", sys_now()); */

		atcmd_trace_task_suspend(ATCMD_TRACE_TASK_DATA_MODE);

		ulTaskNotifyTake(pdTRUE, ticks);

/*		_atcmd_info("data_mode: run %u\n", sys_now()); */
	}
}

static void atcmd_data_mode_task_resume (void)
{
	if (g_atcmd_data_mode_task)
	{
/*		_atcmd_info("data_mode: resume %u\n", sys_now()); */

		atcmd_trace_task_resume(ATCMD_TRACE_TASK_DATA_MODE);

		xTaskNotifyGive(g_atcmd_data_mode_task);
	}
}

static void atcmd_data_mode_task (void *pvParameters)
{
	uint32_t send_time;
	uint32_t current_time;
	uint32_t elapsed_time;
	uint32_t suspend_time;

	while (1)
	{
		atcmd_trace_task_loop(ATCMD_TRACE_TASK_DATA_MODE);

		if (ATCMD_IS_DATA_MODE())
		{
			send_time = g_atcmd_data_mode->last_send_time;
			current_time = sys_now();

			if (current_time >= send_time)
				elapsed_time = (current_time - send_time);
			else
				elapsed_time = (0xffffffff - send_time) + current_time + 1;

/*			_atcmd_info("data_mode: current=%u send=%u elapsed=%u\n",
								current_time, send_time, elapsed_time); */

			if (elapsed_time < g_atcmd_data_mode->send_timeout)
			{
				suspend_time = g_atcmd_data_mode->send_timeout - elapsed_time;

/*				_atcmd_info("data_mode: suspend_time=%u\n", suspend_time); */

				atcmd_data_mode_task_suspend(pdMS_TO_TICKS(suspend_time));
				continue;
			}

			if (!g_atcmd_data_mode->idle)
			{
				if (g_atcmd_data_mode->socket.id >= 0 && g_atcmd_data_mode->send_timeout > 0)
				{
					_atcmd_info("SEVENT: SEND_IDLE, id=%d len=%u time=%u,%u,%u\n",
							g_atcmd_data_mode->socket.id, g_atcmd_data_mode->cnt,
							send_time, current_time, elapsed_time);

					ATCMD_LOG_EVENT("SEVENT", "%s,%d,%u", "%s id=%d len=%u", "\"SEND_IDLE\"",
							g_atcmd_data_mode->socket.id, g_atcmd_data_mode->cnt);

					g_atcmd_data_mode->idle = true;
				}
			}
		}

		atcmd_data_mode_task_suspend(portMAX_DELAY);
	}
}

static int atcmd_data_mode_task_create (void)
{
	BaseType_t ret;

	ret = xTaskCreate(atcmd_data_mode_task, "atcmd_data_mode",
						1024, /* stack size */
						NULL,
						CONFIG_HIF_RX_TASK_PRIORITY,
						&g_atcmd_data_mode_task);

	if (ret == pdPASS && g_atcmd_data_mode_task)
		return 0;

	return -1;
}

static void atcmd_data_mode_task_delete (void)
{
	if (g_atcmd_data_mode_task)
	{
		vTaskDelete(g_atcmd_data_mode_task);
		g_atcmd_data_mode_task = NULL;
	}
}

/*******************************************************************************************/

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
			return -EINVAL;

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

/*******************************************************************************************/

static SemaphoreHandle_t g_atcmd_log_mutex = NULL;

static int atcmd_log_mutex_create (void)
{
	static StaticSemaphore_t buffer;

	g_atcmd_log_mutex = xSemaphoreCreateMutexStatic(&buffer);

	return !g_atcmd_log_mutex ? -1 : 0;
}

static void atcmd_log_mutex_delete (void)
{
	if (g_atcmd_log_mutex)
		vSemaphoreDelete(g_atcmd_log_mutex);
}

static bool atcmd_log_mutex_take (void)
{
	TickType_t timeout = pdMS_TO_TICKS(60000);
	bool take = false;

	if (g_atcmd_log_mutex)
	{
		atcmd_trace_mutex_take(ATCMD_TRACE_MUTEX_LOG);

		take = !!xSemaphoreTake(g_atcmd_log_mutex, timeout);
	}

	if (!take)
	{
		_atcmd_error("timeout\n");
	}

	return take;
}

static bool atcmd_log_mutex_give (void)
{
	bool give = false;

	if (g_atcmd_log_mutex)
	{
		atcmd_trace_mutex_give(ATCMD_TRACE_MUTEX_LOG);

		give = !!xSemaphoreGive(g_atcmd_log_mutex);
	}

	if (!give)
	{
		_atcmd_error("error\n");
	}

	return give;
}

int ATCMD_LOG_VSNPRINT (int type, char *buf, int len, const char *fmt, va_list ap)
{
	const char *prefix = "+";
	int ret = 0;

	switch (type)
	{
		case ATCMD_LOG_TYPE_RETURN:
		case ATCMD_LOG_TYPE_INFO:
		case ATCMD_LOG_TYPE_EVENT:
		case ATCMD_LOG_TYPE_HELP:
			break;

		default:
			return 0;
	}

	if (ATCMD_IS_PROMPT())
	{
		atcmd_prompt_exit();

		ret += snprintf(buf + ret, len - ret, "\r\n");
	}

	ret += snprintf(buf + ret, len - ret, prefix);
	ret += vsnprintf(buf + ret, len - ret, fmt, ap);
	ret += snprintf(buf + ret, len - ret, "\r\n");

	return ret;
}

int ATCMD_LOG_SNPRINT (int type, char *buf, int len, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	len = ATCMD_LOG_VSNPRINT(type, buf, len, fmt, ap);
	va_end(ap);

	return len;
}

int ATCMD_LOG_PRINT (int type, const char *fmt, ...)
{
	int len = 0;

	if (atcmd_log_mutex_take())
	{
		char buf[ATCMD_LEN_MAX + 1];
		va_list ap;

		va_start(ap, fmt);
		len = ATCMD_LOG_VSNPRINT(type, buf, sizeof(buf) - 1, fmt, ap);
		va_end(ap);

		len = atcmd_transmit(buf, len);

		atcmd_log_mutex_give();
	}

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
	int max_argc = ATCMD_PARAMS_MAX + 1;
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

#ifdef CONFIG_ATCMD_DEBUG
					atcmd_parse_print(type, *argc, argv);
#endif

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
	char *argv[ATCMD_PARAMS_MAX + 1];
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

						if (ret == ATCMD_ERROR_NOTSUPP)
							ATCMD_LOG_RETURN(NULL, ret);
						else
							ATCMD_LOG_RETURN(argv[0], ret);

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

	ATCMD_LOG_RETURN(NULL, ATCMD_ERROR_INVAL);

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
				uint32_t reset_cm0 = SW_RESET_CM0_F | SW_RESET_CM0_H;
				uint32_t reset_peri = 0;

				/* reset_peri |= SW_RESET_AHB; */
				reset_peri |= (1 << 25); // EPUSE, LPO_CAL, RTC_EST, PMS_SPI, AUXADC
				/* reset_peri |= SW_RESET_IMC3; */
				/* reset_peri |= SW_RESET_SCFG; */
				/* reset_peri |= SW_RESET_CKC; */
				reset_peri |= SW_RESET_PMC;
				reset_peri |= SW_RESET_GPIO;
				reset_peri |= SW_RESET_WDOG;
				reset_peri |= SW_RESET_RTC;
				reset_peri |= SW_RESET_HOSTIF;
				reset_peri |= SW_RESET_WIFI;
				reset_peri |= SW_RESET_GDMA;
				/* reset_peri |= SW_RESET_XIP; */
				reset_peri |= SW_RESET_UART3;
				reset_peri |= SW_RESET_UART2;
				reset_peri |= SW_RESET_UART1;
				reset_peri |= SW_RESET_UART0;
				reset_peri |= SW_RESET_SPI3;
				reset_peri |= SW_RESET_SPI2;
				reset_peri |= SW_RESET_SPI1;
				reset_peri |= SW_RESET_SPI0;
				reset_peri |= SW_RESET_I2C;
				reset_peri |= SW_RESET_PWM;
				reset_peri |= SW_RESET_TIMER1;
				reset_peri |= SW_RESET_TIMER0;

				_atcmd_info("System Reset: peri=0x%08X\n", reset_peri);

				RegSW_RESET = 0;
				while (RegSW_RESET != 0);

				if (reset_peri)
				{
					RegSW_RESET = reset_peri;
					RegSW_RESET = 0;
					while (RegSW_RESET != 0);
					/* _delay_ms(1000); */
				}

				RegSW_RESET = reset_cm0;

				while (RegSW_RESET != 0);

				ret = ATCMD_SUCCESS;
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
			if (len < 2 || len > ATCMD_LEN_MAX)
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

	ATCMD_LOG_RETURN(NULL, ret); /* success or invalid */
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
					ATCMD_LOG_INFO("LINEFEED", "%d", "%d", 0);
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

				if (*cnt < ATCMD_LEN_MAX)
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

				if (ATCMD_IS_DATA_MODE())
					return ++i;

				break;

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

static int atcmd_receive_data (char *buf, int len)
{
	uint32_t *rx_cnt;
	int id;
	int err = 0;
	int ret = 0;

	if (!g_atcmd_data_mode || !len)
	   return 0;

	id = g_atcmd_data_mode->socket.id;

	if (id < 0)
	{
		ATCMD_DATA_MODE_DISABLE();
		return 0;
	}

	rx_cnt = &g_atcmd_data_mode->cnt;

	if (g_atcmd_data_mode->idle)
	{
		const char *str_exit_cmd = "AT\r\n";
		const int len_exit_cmd = 4;
		static int cnt_exit_cmd = 0;
		int i;

		for (i = 0 ; i < len ; i++)
		{
			if (buf[i] != str_exit_cmd[cnt_exit_cmd])
			{
				cnt_exit_cmd = 0;
				break;
			}

			if (++cnt_exit_cmd == len_exit_cmd)
			{
				ATCMD_LOG_RETURN(NULL, ATCMD_SUCCESS);

				_atcmd_info("SEVENT: SEND_EXIT, id=%d len=%u\n", id, *rx_cnt);

				ATCMD_LOG_EVENT("SEVENT", "%s,%d,%u", "%s id=%d len=%u",
											"\"SEND_EXIT\"", id, *rx_cnt);

				cnt_exit_cmd = 0;
				ATCMD_DATA_MODE_DISABLE();

				return i;
			}
		}

		if (i == len)
			return len;

		g_atcmd_data_mode->idle = false;

		_atcmd_info("SSEND: continue\n");
	}

	g_atcmd_data_mode->last_send_time = sys_now();

	atcmd_data_mode_task_resume();

	if (g_atcmd_data_mode->error)
	{
		_atcmd_info("SEVENT: SEND_DROP, id=%d len=%u\n", id, len);

		ATCMD_LOG_EVENT("SEVENT", "%s,%d,%u", "%s id=%d len=%u",
										"\"SEND_DROP\"", id, len);

		return len;
	}

	if (!g_atcmd_data_mode->passthrough)
	{
		if (g_atcmd_data_mode->len == 0)
			return 0;

		if ((*rx_cnt + len) > g_atcmd_data_mode->len)
			len = g_atcmd_data_mode->len - *rx_cnt;
	}

	ret = atcmd_socket_send_data(&g_atcmd_data_mode->socket, buf, len, &err);

	*rx_cnt += ret;

	if (err)
	{
		_atcmd_info("SEVENT: SEND_ERROR, id=%d len=%u err=%d,%s\n",
								id, *rx_cnt, err, atcmd_strerror(err));

		ATCMD_LOG_EVENT("SEVENT", "%s,%d,%u,%d", "%s len=%u err=%d",
									"\"SEND_ERROR\"", id, *rx_cnt, err);

		g_atcmd_data_mode->error = true;

		return len;
	}

	if (ret != len)
		_atcmd_info("SSEND: id=%d len=%u/%u\n", id, ret, len);

/*	if (g_atcmd_data_mode->sync)
	{
		if (g_atcmd_data_mode->passthrough)
		{
			_atcmd_info("SEVENT: SEND_DONE, id=%d len=%u\n", id, ret);

			ATCMD_LOG_EVENT("SEVENT", "%s,%d,%u", "%s id=%d len=%u",
											"\"SEND_DONE\"", id, ret);
		}
		else if (*rx_cnt == g_atcmd_data_mode->len)
		{
			_atcmd_info("SEVENT: SEND_DONE, id=%d len=%u\n", id, *rx_cnt);

			ATCMD_LOG_EVENT("SEVENT", "%s,%d,%u", "%s id=%d len=%u",
											"\"SEND_DONE\"", id, *rx_cnt);
		}
	} */

	if (!g_atcmd_data_mode->passthrough && *rx_cnt == g_atcmd_data_mode->len)
	{
/*		_atcmd_info("SEVENT: SEND_DONE, id=%d len=%u\n",
								g_atcmd_data_mode->socket.id, g_atcmd_data_mode->cnt);

		ATCMD_LOG_EVENT("SEVENT", "%s,%d,%u", "%s id=%d len=%u", "\"SEND_DONE\"",
								g_atcmd_data_mode->socket.id, g_atcmd_data_mode->cnt); */

		ATCMD_DATA_MODE_DISABLE();
	}

	return ret;
}

int atcmd_transmit_data (atcmd_socket_rxd_t *rxd)
{
	int ret = -1;

	if (rxd)
	{
		char *msg_data = rxd->buf.msg + sizeof(rxd->buf.msg) - rxd->len.msg;
		int len = rxd->len.msg + rxd->len.data;

		ret = atcmd_transmit(msg_data, len);
	}

	return ret;
}

int atcmd_transmit_return (char *cmd, int ret)
{
/* #define CONFIG_ATCMD_RET_ERR_SIMPLE */

	if (ret == ATCMD_SUCCESS)
		atcmd_transmit("OK\r\n", 2 + 2);
	else if (ret != ATCMD_SEND_DATA)
	{
#ifdef CONFIG_ATCMD_RET_ERR_SIMPLE
		char buf[12];
		int len;

		len = snprintf(buf, sizeof(buf), "ERROR:%d\r\n", ret);
		atcmd_transmit(buf, len);
#else
		if (cmd)
			ATCMD_LOG_PRINT(ATCMD_LOG_TYPE_RETURN, "%s:%d", "%s: %d", cmd, ret);

		atcmd_transmit("ERROR\r\n", 5 + 2);
#endif
	}

	return 0;
}

/*******************************************************************************************/

static SemaphoreHandle_t g_atcmd_tx_mutex = NULL;

static int atcmd_tx_mutex_create (void)
{
	static StaticSemaphore_t buffer;

	g_atcmd_tx_mutex = xSemaphoreCreateMutexStatic(&buffer);

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
	{
		atcmd_trace_mutex_take(ATCMD_TRACE_MUTEX_TX);

		take = !!xSemaphoreTake(g_atcmd_tx_mutex, pdMS_TO_TICKS(time_ms));
	}

	if (!take)
	{
		_atcmd_error("timeout=%d\n", time_ms);
	}

	return take;
}

static bool atcmd_tx_mutex_give (void)
{
	bool give = false;

	if (g_atcmd_tx_mutex)
	{
		atcmd_trace_mutex_give(ATCMD_TRACE_MUTEX_TX);

		give = !!xSemaphoreGive(g_atcmd_tx_mutex);
	}

	if (!give)
	{
		_atcmd_error("error\n");
	}

	return give;
}

void atcmd_receive (char *buf, int len)
{
	int ret = 0;
	int i;

	if (!buf || !len)
		return;

	for (i = 0 ; i < len ; i += ret)
	{
		if (ATCMD_IS_DATA_MODE())
			ret = atcmd_receive_data(buf + i, len - i);
		else
			ret = atcmd_receive_command(buf + i, len - i);
	}
}

int atcmd_transmit (char *buf, int len)
{
	int i;

	if (!atcmd_tx_mutex_take())
		return 0;

	for (i = 0 ; i < len ; )
		i += _hif_write(&buf[i], len - i);

	atcmd_tx_mutex_give();

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

	if (atcmd_trace_init() != 0)
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

	if (atcmd_log_mutex_create() != 0)
		return -1;

	if (atcmd_data_mode_task_create() != 0)
		return -1;

	_atcmd_info("ATCMD_TXBUF_SIZE: %d\n", ATCMD_TXBUF_SIZE);
	_atcmd_info("ATCMD_RXBUF_SIZE: %d\n", ATCMD_RXBUF_SIZE);
	_atcmd_info("ATCMD_DATA_LEN_MAX: %d\n", ATCMD_DATA_LEN_MAX);

	atcmd_config_disable(ATCMD_CFG_PROMPT);
	atcmd_config_disable(ATCMD_CFG_ECHO);
	atcmd_config_disable(ATCMD_CFG_HISTORY);
	atcmd_config_disable(ATCMD_CFG_LOWERCASE);
	atcmd_config_enable(ATCMD_CFG_LINEFEED);

	if (ATCMD_HISTORY_IS_ENABLED())
		atcmd_history_enable();
	else
		atcmd_history_disable();

	atcmd_user_enable();
	atcmd_test_enable();
	atcmd_socket_enable();
	atcmd_wifi_enable();
	atcmd_basic_enable();

#ifdef CONFIG_ATCMD_DEBUG
	atcmd_group_print();
#endif

	info->rx_params.buf.addr = hif_rx_buf;
	info->rx_params.buf.size = ATCMD_RXBUF_SIZE;
	info->rx_params.cb = atcmd_receive;

	ret = _hif_open(info);

	if (ret == 0)
	{
#ifdef CONFIG_ATCMD_WELCOME_MESSAGE
		if (atcmd_mode_get() == ATCMD_MODE_TERMINAL)
		{
			char *title = "Newracom AT Commands\r\n";

			atcmd_transmit(title, strlen(title));
		}
#endif

		atcmd_prompt_enter();
	}

	return ret;
}

void atcmd_disable (void)
{
	_hif_close();

	atcmd_basic_disable();
	atcmd_wifi_disable();
	atcmd_socket_disable();
	atcmd_test_disable();
	atcmd_user_disable();

	atcmd_data_mode_task_delete();

	atcmd_log_mutex_delete();
	atcmd_tx_mutex_delete();

	atcmd_trace_exit();
}

/**********************************************************************************************/

static struct
{
	int num;
	const char *str;
} g_atcmd_errors[] =
{
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

	{ 0, NULL }
};

const char *atcmd_strerror (int err)
{
	int i;

	if (err == 0)
		return "OK";

	for (i = 0 ; g_atcmd_errors[i].num ; i++)
	{
		if (-err == g_atcmd_errors[i].num)
			return g_atcmd_errors[i].str;
	}

	return "UNKNOWN";
}

/**********************************************************************************************/

int atcmd_param_to_long (char *param, long *val)
{
	*val = strtol(param, NULL, 10);

	return -errno;
}

int atcmd_param_to_ulong (char *param, unsigned long *val)
{
	*val = strtoul(param, NULL, 10);

	return -errno;
}

char *atcmd_param_to_str (const char *param, char *str, int len)
{
	int param_len;

	if (!param || !str || !len)
		return NULL;

	param_len = strlen(param);

	if (param_len <= 2 || (param_len - 2) > (len - 1))
		return NULL;

	if (param[0] != '"' || param[param_len - 1] != '"')
		return NULL;

	memcpy(str, &param[1], param_len - 2);
	str[param_len - 2] = '\0';

	return str;
}

char *atcmd_str_to_param (const char *str, char *param, int len)
{
	int str_len;

	if (!str || !param || !len)
		return NULL;

	str_len = strlen(str);
	if (len < (str_len + 2 + 1))
		return NULL;

	snprintf(param, len, "\"%s\"", str);
	param[str_len + 2] = '\0';

	return param;
}

