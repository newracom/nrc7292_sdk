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


#include "nrc-atcmd.h"
#include "nrc-echo.h"

#define echo_info(fmt, ...)				log_info(fmt, ##__VA_ARGS__)
#define echo_error(fmt, ...)			log_error(fmt, ##__VA_ARGS__)
#define echo_debug(fmt, ...)			//log_debug(fmt, ##__VA_ARGS__)

#define echo_log_push(fmt, ...)			//echo_debug("echo_push: " fmt, ##__VA_ARGS__)
#define echo_log_pop(fmt, ...)			//echo_debug("echo_pop : " fmt, ##__VA_ARGS__)

#define echo_log_send(log, fmt, ...)	if (log) echo_info("echo_send: " fmt, ##__VA_ARGS__)
#define echo_log_recv(log, fmt, ...)	if (log) echo_info("echo_recv: " fmt, ##__VA_ARGS__)

static echo_info_t g_nrc_echo_info =
{
	.enable = false,
	.queue =
	{
		.cnt = 0,
		.head = NULL,
		.tail = NULL,
	},
};

static void nrc_echo_queue_print (void)
{

}

static bool nrc_echo_queue_empty (void)
{
	echo_info_t *info = &g_nrc_echo_info;

	return !info->queue.cnt;
}

static int nrc_echo_queue_push (echo_data_t *data)
{
	echo_info_t *info = &g_nrc_echo_info;

	echo_log_push("%u %d %d %s %u\n", info->queue.cnt,
					data->id, data->len, data->remote.addr, data->remote.port);

	if (!info->queue.head)
		info->queue.head = data;

	if (info->queue.tail)
		info->queue.tail->next = data;

	info->queue.tail = data;

	info->queue.cnt++;

	return 0;
}

static echo_data_t *nrc_echo_queue_pop (void)
{
	echo_info_t *info = &g_nrc_echo_info;
	echo_data_t *data = NULL;

	if (!info->queue.head)
	{
		echo_debug("%s: empty\n", __func__);
	}
	else
	{
		data = info->queue.head;

		echo_log_pop("%u %d %d %s %u\n", info->queue.cnt,
					data->id, data->len, data->remote.addr, data->remote.port);

		if (data->next)
			info->queue.head = data->next;
		else
		{
			info->queue.head = NULL;
			info->queue.tail = NULL;
		}

		info->queue.cnt--;
	}

	return data;
}

static int nrc_echo_recv_event (enum ATCMD_EVENT event, int argc, char *argv[])
{

	return 0;
}

static void nrc_echo_recv_data (atcmd_rxd_t *rxd, char *buf)
{
	echo_info_t *info = &g_nrc_echo_info;
	echo_data_t *data;

	if (!rxd || !buf)
	{
		echo_error("%s\n", strerror(EINVAL));
		return;
	}

	echo_log_recv(info->log, "%d %d %s %u (%u)\n", rxd->id, rxd->len,
					rxd->remote_addr, rxd->remote_port, info->queue.cnt);

	data = (echo_data_t *)malloc(sizeof(echo_data_t));
	if (!data)
	{
		echo_error("%s\n", strerror(ENOMEM));
		return;
	}

	data->buf = (char *)malloc(rxd->len);
	if (!data->buf)
	{
		echo_error("%s\n", strerror(ENOMEM));
		free(data);
		return;
	}

	data->id = rxd->id;
	data->len = rxd->len;
	memcpy(data->buf, buf, rxd->len);
	strcpy(data->remote.addr, rxd->remote_addr);
	data->remote.port = rxd->remote_port;
	data->next = NULL;

	if (pthread_mutex_lock(&info->mutex) != 0)
		echo_error("pthread_mutex_lock(), %s\n", strerror(errno));

	nrc_echo_queue_push(data);

	if (pthread_cond_signal(&info->cond) != 0)
		echo_error("pthread_cond_signal(), %s\n", strerror(errno));

	if (pthread_mutex_unlock(&info->mutex) != 0)
		echo_error("pthread_mutex_unlock(), %s\n", strerror(errno));
}

static void *nrc_echo_send_data (void *arg)
{
#define SEND_DATA_MAX	10000

	bool passthrough = false;
	bool send_start = false;

	echo_info_t *info = &g_nrc_echo_info;
	echo_data_t *data[SEND_DATA_MAX];
	echo_data_t *pdata;
	int cmd_ret;
	int ret;
	int i;

	while (1)
	{
		ret = pthread_mutex_lock(&info->mutex);
	   	if (ret	!= 0)
			echo_error("pthread_mutex_lock(), %s\n", strerror(ret));

		ret = pthread_cond_wait(&info->cond, &info->mutex);
	   	if (ret	!= 0)
			echo_error("pthread_cond_wait(), %s\n", strerror(ret));

		for (i = 0 ; i < SEND_DATA_MAX ; i++)
		{
			data[i] = nrc_echo_queue_pop();
			if (!data[i])
				break;
		}

		for (i = 0 ; i < SEND_DATA_MAX && data[i] ; i++)
		{
			pdata = data[i];

			echo_log_send(info->log, "%d %d %s %u (%u)\n", pdata->id, pdata->len,
								pdata->remote.addr, pdata->remote.port, info->queue.cnt);

			ret = pthread_mutex_unlock(&info->mutex);
		   	if (ret != 0)
				echo_error("pthread_mutex_unlock(), %s\n", strerror(ret));

			if (!passthrough)
			{
				if (strlen(pdata->remote.addr) >= 7 && pdata->remote.port > 0)
				{
					cmd_ret = nrc_atcmd_send_cmd("AT+SSEND=%d,\"%s\",%u,%d",
							pdata->id, pdata->remote.addr, pdata->remote.port, pdata->len);
				}
				else
				{
					cmd_ret = nrc_atcmd_send_cmd("AT+SSEND=%d,%d", pdata->id, pdata->len);
				}
			}
			else if (!send_start)
			{
				if (strlen(pdata->remote.addr) >= 7 && pdata->remote.port > 0)
				{
					cmd_ret = nrc_atcmd_send_cmd("AT+SSEND=%d,\"%s\",%u",
							pdata->id, pdata->remote.addr, pdata->remote.port);
				}
				else
				{
					cmd_ret = nrc_atcmd_send_cmd("AT+SSEND=%d", pdata->id);
				}

				send_start = true;
			}

			ret = pthread_mutex_lock(&info->mutex);
		   	if (ret != 0)
				echo_error("pthread_mutex_lock(), %s\n", strerror(ret));

			if (cmd_ret	== 0)
				nrc_atcmd_send_data(pdata->buf, pdata->len);

			free(pdata->buf);
			free(pdata);

			data[i] = NULL;
		}

		ret = pthread_mutex_unlock(&info->mutex);
	   	if (ret != 0)
			echo_error("pthread_mutex_unlock(), %s\n", strerror(ret));
	}

	pthread_exit(0);
}

int nrc_echo_enable (void)
{
	echo_info_t *info = &g_nrc_echo_info;
	int ret;

	if (info->enable)
		return -1;

	info->log = false;
	info->enable = true;

	ret = pthread_cond_init(&info->cond, NULL);
	if (ret != 0)
	{
		echo_error("pthread_cond_init(), %s\n", strerror(ret));
		goto nrc_echo_enable_fail;
	}

	ret = pthread_mutex_init(&info->mutex, NULL);
	if (ret != 0)
	{
		echo_error("pthread_mutex_init(), %s\n", strerror(ret));
		goto nrc_echo_enable_fail;
	}

	ret = pthread_create(&info->thread, NULL, nrc_echo_send_data, NULL);
	if (ret != 0)
	{
		echo_error("pthread_create(), %s\n", strerror(ret));
		goto nrc_echo_enable_fail;
	}

	if (nrc_atcmd_register_callback(ATCMD_CB_EVENT, nrc_echo_recv_event) != 0)
	{
		goto nrc_echo_enable_fail;
	}

	if (nrc_atcmd_register_callback(ATCMD_CB_RXD, nrc_echo_recv_data) != 0)
	{
		goto nrc_echo_enable_fail;
	}

	return 0;

nrc_echo_enable_fail:

	nrc_echo_disable();

	return -1;

}

void nrc_echo_disable (void)
{
	echo_info_t *info = &g_nrc_echo_info;
	int ret;
	int i;

	if (!info->enable)
		return;

	nrc_atcmd_unregister_callback(ATCMD_CB_RXD);
	nrc_atcmd_unregister_callback(ATCMD_CB_EVENT);

	ret = pthread_cancel(info->thread);
	if (ret != 0)
		echo_error("pthread_cancel(), %s\n", strerror(ret));

	ret = pthread_join(info->thread, NULL);
	if (ret != 0)
		echo_error("pthread_join(), %s\n", strerror(ret));

	ret = pthread_cond_destroy(&info->cond);
   	if (ret != 0)
		echo_error("pthread_cond_destroy(), %s\n", strerror(ret));

	ret = pthread_mutex_unlock(&info->mutex);
	if (ret != 0)
		echo_error("pthread_mutex_unlock(), %s\n", strerror(ret));

	ret = pthread_mutex_destroy(&info->mutex);
	if (ret != 0)
		echo_error("pthread_mutex_destroy(), %s\n", strerror(ret));

	for (i = 0 ; i < info->queue.cnt ; i++)
	{
		if (!info->queue.head)
		{
			echo_error("queue empty\n");
			continue;
		}

		free(info->queue.head->buf);
		free(info->queue.head);

		info->queue.head = info->queue.head->next;
	}

	info->queue.tail = NULL;

	if (info->queue.head)
		echo_error("no queue empty\n");

	info->log = false;
	info->enable = false;
}

void nrc_echo_log_on (void)
{
	echo_info_t *info = &g_nrc_echo_info;

	info->log = true;
}

void nrc_echo_log_off (void)
{
	echo_info_t *info = &g_nrc_echo_info;

	info->log = false;
}

bool nrc_echo_log_is_on (void)
{
	echo_info_t *info = &g_nrc_echo_info;

	return info->log;
}

