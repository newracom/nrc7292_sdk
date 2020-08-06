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


#define _atcmd_perf_info(fmt, ...)		_atcmd_info("SPERF_" fmt, ##__VA_ARGS__)

/**********************************************************************************************/

typedef double atcmd_perf_time_t;

typedef struct
{
	bool recv;
	bool event;

	atcmd_socket_t *socket;

	struct
	{
		uint16_t len;	/* byte */
	   	uint16_t delay;	/* msec */
		uint16_t time;	/* sec  */
	} send;
} atcmd_perf_params_t;

typedef struct
{
	bool active;
	bool event;
	bool recv;

	int interval;
	atcmd_perf_time_t start_time;
	atcmd_perf_time_t stop_time;

	uint32_t byte;
	uint32_t last_byte;
	atcmd_perf_time_t last_time;
} atcmd_perf_report_t;

typedef struct
{
#define ATCMD_PERF_DATA_ID_VAL		"TEST"
#define ATCMD_PERF_DATA_ID_LEN		sizeof(uint32_t)

#define ATCMD_PERF_DATA_LEN_MIN		(3 * sizeof(uint32_t))
#define ATCMD_PERF_DATA_LEN_MAX		ATCMD_DATA_LEN_MAX

#define ATCMD_PERF_DATA_SEND_RETRY	8

	uint32_t id;
	uint32_t len; /* byte */
	uint32_t seq;

	char payload[ATCMD_PERF_DATA_LEN_MAX];
} atcmd_perf_data_t;

typedef struct
{
	atcmd_perf_params_t params;
	atcmd_perf_report_t report;
	atcmd_perf_data_t data;

	struct
	{
		TaskHandle_t handle;
		bool done;
		bool exit;
	} task;
} atcmd_perf_info_t;

static atcmd_perf_info_t *g_atcmd_perf_info = NULL;

/**********************************************************************************************/

static uint32_t byte_to_bps (atcmd_perf_time_t time, uint32_t byte)
{
	return 8 * (byte / time);
}

static char *byte_to_string (uint32_t byte)
{
	static char buf[20];

	if (byte < 1024)
		snprintf(buf, sizeof(buf), "%lu ", byte);
	else if (byte < (1024 * 1024))
		snprintf(buf, sizeof(buf), "%4.2f K", byte / 1024.);
	else
		snprintf(buf, sizeof(buf), "%4.2f M", (byte / 1024.) / 1024.);

	return buf;
}

static char *bps_to_string (uint32_t bps)
{
	static char buf[20];

	if (bps < 1000)
		snprintf(buf, sizeof(buf), "%lu ", bps);
	else if (bps < (1000 * 1000))
		snprintf(buf, sizeof(buf), "%4.2f K", bps / 1000.);
	else
		snprintf(buf, sizeof(buf), "%4.2f M", (bps / 1000.) / 1000.);

	return buf;
}

/**********************************************************************************************/

static atcmd_perf_time_t _atcmd_perf_time_msec (void)
{
	return sys_now() / 1000.;
}

#define _atcmd_perf_delay_msec(time)		_delay_ms(time)

/**********************************************************************************************/

static int _atcmd_perf_data_init (atcmd_perf_data_t *data, uint32_t len, uint32_t seq)
{
	if (!data)
		return -EINVAL;

	memcpy(&data->id, ATCMD_PERF_DATA_ID_VAL, ATCMD_PERF_DATA_ID_LEN);

	data->len = len;
	data->seq = seq;

	if (data->len > ATCMD_PERF_DATA_LEN_MIN)
	{
		int i;

		for (i = 0 ; i < (data->len - ATCMD_PERF_DATA_LEN_MIN) ; i++)
			data->payload[i] = '0' + (i % 10);
	}

	return 0;
}

static int _atcmd_perf_data_set_payload (atcmd_perf_data_t *data, char *payload, int len)
{
	if (!data || !payload|| !len)
		return -EINVAL;

	memset(data->payload, 0, data->len - ATCMD_PERF_DATA_LEN_MIN);

	data->len = len + ATCMD_PERF_DATA_LEN_MIN;
	memcpy(data->payload, payload, len);

	return 0;
}

/**********************************************************************************************/

static void __atcmd_perf_report_print (atcmd_perf_time_t start_time,
										atcmd_perf_time_t stop_time,
										uint32_t byte, bool event)
{
/* #define fmt1 "%4.1f,%4.1f,%7s,%7s" */
#define fmt1 "  %4.1f ~ %4.1f sec  %7sBytes %7sbits/sec"
#define fmt2 "  %4.1f ~ %4.1f sec  %7sBytes %7sbits/sec"

	atcmd_perf_time_t interval = stop_time - start_time;
	uint32_t bps = byte_to_bps(interval, byte);

	if (event)
	{
		ATCMD_LOG_EVENT("SPERF", fmt1, fmt2, start_time, stop_time,
						byte_to_string(byte), bps_to_string(bps));
	}
	else
	{
		_atcmd_info(fmt2 "\n", start_time, stop_time,
						byte_to_string(byte), bps_to_string(bps));
	}
}

static void _atcmd_perf_report_print (atcmd_perf_report_t *report, atcmd_perf_time_t time)
{
	if (!report->active)
		return;

	if (time >= (report->last_time + report->interval))
	{
		__atcmd_perf_report_print(report->last_time - report->start_time,
									time - report->start_time,
									report->byte - report->last_byte, report->event);

		report->last_time = time;
		report->last_byte = report->byte;
	}
}

static void _atcmd_perf_report_update (atcmd_perf_report_t *report, uint32_t byte)
{
	if (report->active && byte > 0)
		report->byte += byte;
}

static bool _atcmd_perf_report_done (atcmd_perf_report_t *report, atcmd_perf_time_t time)
{
	if (!report->active || time >= report->stop_time)
		return true;

	return false;
}

static int _atcmd_perf_report_start (atcmd_perf_report_t *report,
										atcmd_perf_time_t start_time,
										atcmd_perf_time_t stop_time,
										bool event, bool recv)
{
	if (start_time <= 0)
	{
		_atcmd_error("%s\n", atcmd_strerror(EINVAL));
		return -EINVAL;
	}

/*	_atcmd_debug("start_time=%.3f stop_time=%.3f\n", start_time, stop_time); */

	memset(report, 0, sizeof(atcmd_perf_report_t));

	report->active = true;
	report->event = event;
	report->recv = recv;
	report->interval = 1;
	report->start_time = start_time;
	report->stop_time = stop_time;
	report->last_time = start_time;

	return 0;
}

static void _atcmd_perf_report_stop (atcmd_perf_report_t *report, atcmd_perf_time_t time)
{
	if (!report->active)
		return;

	_atcmd_perf_report_print(report, time);

	__atcmd_perf_report_print(0, time - report->start_time,	report->last_byte, report->event);

	report->active = false;
}

/**********************************************************************************************/

static void _atcmd_perf_send_task (void *pvParameters)
{
	atcmd_perf_info_t *info = (atcmd_perf_info_t *)pvParameters;
	atcmd_perf_params_t *params = (atcmd_perf_params_t *)&info->params;
	atcmd_perf_report_t *report = (atcmd_perf_report_t *)&info->report;
	atcmd_perf_data_t *data = &info->data;
	atcmd_perf_time_t time;
	int err;
	int ret;

	_atcmd_perf_info("SEND: task_enter\n");

	if (params->send.len > ATCMD_PERF_DATA_LEN_MAX)
		params->send.len = ATCMD_PERF_DATA_LEN_MAX;

	_atcmd_perf_data_init(data, params->send.len, 0);

	time = _atcmd_perf_time_msec();

	if (_atcmd_perf_report_start(report, time, time + params->send.time, params->event, false) != 0)
		_atcmd_perf_info("SEND: fail\n");
	else
	{
		char *msg_status = "stop\0";

		_atcmd_perf_info("SEND: start\n");

		while (!_atcmd_perf_report_done(report, _atcmd_perf_time_msec()) && !info->task.exit)
		{
			ret = atcmd_socket_send_data(params->socket, (char *)data, data->len, &err);

/*			_atcmd_debug("SEND: seq=%u len=%u ret=%d err=%d\n", data->seq, data->len, ret, err); */

			if (ret >= ATCMD_PERF_DATA_LEN_MIN)
			{
				_atcmd_perf_report_update(report, ret);
				_atcmd_perf_report_print(report, _atcmd_perf_time_msec());

				data->seq++;

				if (params->send.delay > 0)
					_atcmd_perf_delay_msec(params->send.delay);
			}
			else if (err)
			{
				msg_status = "cancel\0";
				break;
			}
		}

		_atcmd_perf_report_stop(report, _atcmd_perf_time_msec());

		_atcmd_perf_delay_msec(100);

		data->seq = ~0;
		_atcmd_perf_data_set_payload(data, msg_status, strlen(msg_status) + 1);
		atcmd_socket_send_data(params->socket, (char *)data, data->len, &err);

		_atcmd_perf_info("SEND: %s\n", msg_status);
	}

	info->task.done = true;

	_atcmd_perf_info("SEND: task exit\n");

	info->task.handle = NULL;

	vTaskDelete(NULL);
}

static void _atcmd_perf_recv_data (atcmd_perf_info_t *info, atcmd_socket_t *socket, char *buf, int len)
{
	atcmd_perf_data_t *data = &info->data;
	static uint32_t data_seq = 0;
	static uint32_t recv_len = 0;
	int _len;
	int ret;
	int i;

	for (i = 0 ; i < len ; )
	{
		if (recv_len < ATCMD_PERF_DATA_LEN_MIN)
		{
			if ((len - i) < (ATCMD_PERF_DATA_LEN_MIN - recv_len))
				_len = len - i;
			else
				_len = ATCMD_PERF_DATA_LEN_MIN - recv_len;

			memcpy((char *)data + recv_len, buf + i, _len);

			recv_len += _len;
			i += _len;

			while (recv_len >= ATCMD_PERF_DATA_ID_LEN &&
				memcmp(&data->id, ATCMD_PERF_DATA_ID_VAL, ATCMD_PERF_DATA_ID_LEN) != 0)
			{
				_atcmd_perf_info("RECV: invalid data id (0x%08X)\n", data->id);

				recv_len -= ATCMD_PERF_DATA_ID_LEN;
				memcpy((char *)data, (char *)data + ATCMD_PERF_DATA_ID_LEN, recv_len);
			}

			if (recv_len == ATCMD_PERF_DATA_LEN_MIN && data->len > ATCMD_PERF_DATA_LEN_MAX)
			{
				_atcmd_info("RECV: invalid data length (%u)\n", data->len);

				recv_len -= ATCMD_PERF_DATA_LEN_MIN;
				memset((char *)data, 0, ATCMD_PERF_DATA_LEN_MIN);
			}

			continue;
		}

		if ((len - i) < (data->len - recv_len))
		{
			recv_len += len - i;
			i += len - i;
		}
		else
		{
			i += data->len - recv_len;
			recv_len = data->len;
		}

		if (recv_len < data->len)
			continue;
		else if (recv_len > data->len)
		{
			_atcmd_perf_info("RECV: recv_len (%u) > data.len (%u)\n", recv_len, data->len);
			continue;
		}

		switch (data->seq)
		{
			case ~0:
				_atcmd_perf_report_stop(&info->report, _atcmd_perf_time_msec());

				info->params.recv = false;

				_atcmd_perf_info("RECV: stop\n");
				break;

			case 0:
				data_seq = 0;
				_atcmd_perf_report_start(&info->report, _atcmd_perf_time_msec(), 0, info->params.event, true);

				_atcmd_perf_info("RECV: start\n");

			default:
/*				_atcmd_perf_info("RECV: data, seq=%u len=%u\n", data->seq, data->len); */

				_atcmd_perf_report_update(&info->report, data->len);

				if (data->seq == data_seq)
					data_seq++;
				else
				{
					_atcmd_perf_info("RECV: sequence error, %u -> %u\n", data_seq, data->seq);

					data_seq = data->seq;
				}
		}

		recv_len = 0;
		memset((char *)data, 0, ATCMD_PERF_DATA_LEN_MIN);
	}
}

static void _atcmd_perf_recv_task (void *pvParameters)
{
	atcmd_perf_info_t *info = (atcmd_perf_info_t *)pvParameters;
	atcmd_perf_params_t *params = (atcmd_perf_params_t *)&info->params;
	atcmd_perf_report_t *report = (atcmd_perf_report_t *)&info->report;

	_atcmd_perf_info("RECV: task enter\n");

	while (info->params.recv && !info->task.exit)
	{
		_atcmd_perf_report_print(report, _atcmd_perf_time_msec());
	}

	info->task.done = true;

	_atcmd_perf_info("RECV: task exit\n");

	info->task.handle = NULL;

	vTaskDelete(NULL);
}

/**********************************************************************************************/

static int _atcmd_perf_task_start (atcmd_perf_info_t *info)
{
	struct
	{
		const char *name;
		void (*func) (void *);
	} task_info[2] =
	{
		[0] = { "atcmd_perf_send", _atcmd_perf_send_task },
		[1] = { "atcmd_perf_recv", _atcmd_perf_recv_task },
	};

	TaskHandle_t handle = NULL;
	BaseType_t priority = uxTaskPriorityGet(NULL);
	BaseType_t ret;

	ret = xTaskCreate(task_info[!!info->params.recv].func,
						task_info[!!info->params.recv].name,
						512, info, priority, &handle);

	if (ret != pdPASS)
		return -1;

	info->task.handle = handle;
	info->task.done = false;
	info->task.exit = false;

	return 0;
}

static void _atcmd_perf_task_stop (atcmd_perf_info_t *info)
{
	info->task.exit = true;

	while (!info->task.done)
	{
		_delay_ms(100);
		_atcmd_trace();
	}

	_atcmd_trace();

	if (info->task.handle)
	{
		_atcmd_trace();

		vTaskDelete(info->task.handle);

		info->task.handle = NULL;
	}

	_atcmd_trace();
}

/**********************************************************************************************/

int atcmd_perf_start (atcmd_socket_t *socket, bool recv, bool event, int time)
{
	atcmd_perf_params_t *params;

	if (!socket)
		return -1;

	if (g_atcmd_perf_info)
		_atcmd_perf_task_stop(g_atcmd_perf_info);
	else
	{
		g_atcmd_perf_info = _atcmd_malloc(sizeof(atcmd_perf_info_t));
		if (!g_atcmd_perf_info)
		{
			_atcmd_error("malloc() failed\n");
			return -1;
		}
	}

	memset(g_atcmd_perf_info, 0, sizeof(atcmd_perf_info_t));

	params = &g_atcmd_perf_info->params;

	params->recv = recv;
	params->event = event;
	params->socket = socket;

	if (recv)
	{
		_atcmd_perf_info("RECV: event=%d\n", params->event);
	}
	else
	{
		params->send.len = ATCMD_PERF_DATA_LEN_MAX;	/* byte */
		params->send.delay = 0; 					/* msec */
		params->send.time = time > 0 ? time : 10; 	/* sec  */

		_atcmd_perf_info("SEND: len=%u delay=%u time=%u event=%d\n",
			params->send.len, params->send.delay, params->send.time, params->event);
	}

	_atcmd_perf_task_start(g_atcmd_perf_info);

	return 0;
}

int atcmd_perf_stop (void)
{

	return 0;
}

int atcmd_perf_recv_data (atcmd_socket_t *socket , char *buf, int len)
{
	if (!g_atcmd_perf_info || !g_atcmd_perf_info->params.recv)
		return -1;

	_atcmd_perf_recv_data(g_atcmd_perf_info, socket, buf, len);

	return 0;
}

