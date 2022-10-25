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


static int _lwip_ping_request_init (_lwip_ping_info_t *info)
{
	int buf_size = ICMP_ECHO_HLEN + info->params.data_size;
	char *buf;

	buf = _lwip_ping_malloc(buf_size);
	if (buf)
	{
		_lwip_ping_request_t *request = &info->request;
		struct icmp_echo_hdr *header = (struct icmp_echo_hdr *)buf;
		char *data = &buf[ICMP_ECHO_HLEN];
		int i;

		ICMPH_TYPE_SET(header, ICMP_ECHO);
		ICMPH_CODE_SET(header, 0);
		header->id     = 0xECEC;
		header->seqno  = 0;
		header->chksum = 0;

		for(i = 0 ; i < info->params.data_size ; i++)
			data[i] = i;

		request->buf = buf;
		request->buf_size = buf_size;

		ip4_addr_copy(request->remote_ip, info->params.remote_ip);
		request->id = header->id;
		request->icmp_seq = 0;
		request->icmp_seq_end = info->params.count;
		request->time = 0;
	}

	return buf ? 0 : -1;
}

static int _lwip_ping_reply_init (_lwip_ping_info_t *info)
{
	int buf_size = info->request.buf_size + IP_HLEN_MAX;
	char *buf;

	buf = _lwip_ping_malloc(buf_size);
	if (buf)
	{
		_lwip_ping_reply_t *reply = &info->reply;

		reply->buf = buf;
		reply->buf_size = buf_size;
	}

	return buf ? 0 : -1;
}

static int _lwip_ping_init (_lwip_ping_info_t *info)
{
	struct timeval timeout =
	{
		.tv_sec = info->params.interval / 1000,
		.tv_usec = (info->params.interval % 1000) * 1000
	};

	info->socket = lwip_socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (info->socket < 0)
		return -1;

	_lwip_ping_debug("INIT: timeout=%ds,%dus\n", timeout.tv_sec, timeout.tv_usec);

	if (lwip_setsockopt(info->socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) != 0)
		_lwip_ping_error("SO_SNDTIMEO: errno=%d\n", errno);

	if (lwip_setsockopt(info->socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0)
		_lwip_ping_error("SO_RCVTIMEO: errno=%d\n", errno);

	if (_lwip_ping_request_init(info) != 0)
		return -1;

	if (_lwip_ping_reply_init(info) != 0)
		return -1;

	return 0;
}

static void _lwip_ping_exit (_lwip_ping_info_t *info)
{
	if (info->socket >= 0)
		lwip_close(info->socket);

	if (info->request.buf)
		_lwip_ping_free(info->request.buf);

	if (info->reply.buf)
		_lwip_ping_free(info->reply.buf);
}

static void _lwip_ping_update (_lwip_ping_info_t *info)
{
	_lwip_ping_request_t *request = &info->request;
	_lwip_ping_reply_t *reply = &info->reply;

	struct icmp_echo_hdr *header = (struct icmp_echo_hdr *)request->buf;

	request->icmp_seq++;
	request->time = 0;

	reply->data_size = 0;
	reply->icmp_seq = 0;
	reply->ttl = 0;
	reply->time = 0;
	ip_addr_set_zero(&reply->remote_ip);

	header->seqno = htons(request->icmp_seq);
  	header->chksum = 0;
  	header->chksum = inet_chksum(request->buf, request->buf_size);
}

static int _lwip_ping_send (_lwip_ping_info_t *info)
{
	_lwip_ping_request_t *request = &info->request;

	int len = 0;
	struct sockaddr_in to;
	uint32_t send_time;

	to.sin_len = sizeof(to);
	to.sin_family = AF_INET;
	inet_addr_from_ip4addr(&to.sin_addr, &request->remote_ip);

	len = lwip_sendto(info->socket, request->buf, request->buf_size,
							0, (struct sockaddr*)&to, sizeof(to));

	send_time = atcmd_sys_now();

	if (len <= 0)
	{
		_lwip_ping_error("SEND: errno=%d\n", errno);

		info->status = _LWIP_PING_SEND_FAIL;
		return -1;
	}

	request->time = send_time;

	_lwip_ping_log("SEND: id=0x%04X data=%u seq=%u time=%u addr=%s\n",
			request->id, request->buf_size, request->icmp_seq, request->time,
			ip4addr_ntoa(&request->remote_ip));

	return 0;
}

static int _lwip_ping_receive (_lwip_ping_info_t *info)
{
	_lwip_ping_params_t *params = &info->params;
	_lwip_ping_request_t *request = &info->request;
	_lwip_ping_reply_t *reply = &info->reply;

	const int min_header_size = IP_HLEN + ICMP_ECHO_HLEN;
	uint32_t recv_time;
	struct sockaddr_in from;
	int fromlen;
	struct ip_hdr *ip_hdr;
	int packet_size;
	ip4_addr_t remote_ip;
	int ip_hdr_size;
	int total_hdr_size;
	struct icmp_echo_hdr *echo_hdr;
	int len;

/*	while (atcmd_sys_now() < (info->send_time + params->interval)) */
	while (atcmd_sys_now() < (info->request.time + params->interval))
	{
		len = lwip_recvfrom(info->socket, reply->buf, reply->buf_size, 0,
							(struct sockaddr*)&from, (socklen_t*)&fromlen);

		recv_time = atcmd_sys_now();

		if (len < 0)
		{
			switch (errno)
			{
				case EAGAIN:
/*					_lwip_ping_debug("RECV: again, time=%u\n", recv_time); */
					break;

				default:
					_lwip_ping_error("RECV: errno=%d\n", errno);
			}

			_lwip_ping_delay(1); /* 1msec */
			continue;
		}

		ip_hdr = (struct ip_hdr *)reply->buf;
		packet_size = ntohs(IPH_LEN(ip_hdr));
		inet_addr_to_ip4addr(&remote_ip, &from.sin_addr);

		if (len != packet_size)
			goto _lwip_ping_rx_drop;

		if (!ip4_addr_cmp(&remote_ip, &request->remote_ip))
			goto _lwip_ping_rx_drop;

		ip_hdr_size = IPH_HL(ip_hdr) << 2;
		total_hdr_size = ip_hdr_size + ICMP_ECHO_HLEN;
		echo_hdr = (struct icmp_echo_hdr *)&reply->buf[ip_hdr_size];

		if (echo_hdr->id == request->id &&
				echo_hdr->seqno == htons(request->icmp_seq))
		{
			reply->time = recv_time;
			reply->data_size = packet_size - total_hdr_size;
			reply->icmp_seq = ntohs(echo_hdr->seqno);
			reply->ttl = IPH_TTL(ip_hdr);
			ip4_addr_copy(reply->remote_ip, remote_ip);

			_lwip_ping_log("RECV: data=%u seq=%u ttl=%u time=%u addr=%s\n",
						reply->data_size, reply->icmp_seq, reply->ttl, reply->time,
						ip4addr_ntoa(&reply->remote_ip));

			return 0;
		}

_lwip_ping_rx_drop:
		_lwip_ping_log("RECV: drop, len=%d, addr=%s\n", len, ip4addr_ntoa(&remote_ip));
	}

	info->status = _LWIP_PING_RECV_TIMEOUT;

	return -1;
}

static void _lwip_ping_report (_lwip_ping_info_t *info)
{
	uint32_t response_time = info->reply.time - info->request.time;
	_lwip_ping_report_t report;

	report.status = info->status;

	switch (info->status)
	{
		case _LWIP_PING_SUCCESS:
		{
			_lwip_ping_reply_t *reply = &info->reply;

			_lwip_ping_log("%u bytes from %s, icmp_seq=%u ttl=%u time=%ums\n",
					reply->data_size, ip4addr_ntoa(&reply->remote_ip),
					reply->icmp_seq, reply->ttl, response_time);

			ip4_addr_copy(report.remote_ip, reply->remote_ip);
			report.data_size = reply->data_size;
			report.icmp_seq = reply->icmp_seq;
			report.ttl = reply->ttl;
			report.resp_time = response_time;
			break;
		}

		case _LWIP_PING_SEND_FAIL:
		case _LWIP_PING_RECV_TIMEOUT:
		{
			_lwip_ping_params_t *params = &info->params;
			_lwip_ping_request_t *request = &info->request;

			ip4_addr_copy(report.remote_ip, request->remote_ip);
			report.data_size = params->data_size;
			report.icmp_seq = request->icmp_seq;
			report.ttl = 0;
			report.resp_time = 0;
			break;
		}
	}

	if (info->params.report_cb)
		info->params.report_cb(&report);
}

static int _lwip_ping_run (_lwip_ping_info_t *info)
{
	int wait_time;
	int err = 0;

	if (!info || _lwip_ping_init(info) != 0)
	{
		err = -1;
		goto _lwip_ping_done;
	}

	while (info->request.icmp_seq < info->request.icmp_seq_end)
	{
		_lwip_ping_update(info);

		info->send_time = atcmd_sys_now();
		info->status = _LWIP_PING_SUCCESS;

		if (_lwip_ping_send(info) == 0)
			_lwip_ping_receive(info);

		_lwip_ping_report(info);

/*		wait_time = info->params.interval - (atcmd_sys_now() - info->send_time); */
		wait_time = info->params.interval - (atcmd_sys_now() - info->request.time);

/*		_lwip_ping_debug("wait_time=%d\n", wait_time); */

		if (wait_time > 0)
			_lwip_ping_delay(wait_time);
	}

_lwip_ping_done:

	_lwip_ping_exit(info);

	if (err)
		_lwip_ping_log("Abort\n");
	else
		_lwip_ping_log("Complete\n");

	return err;
}

/**********************************************************************************************/

/* #define _LWIP_PING_TASK_RUN */

#ifdef _LWIP_PING_TASK_RUN

static void _lwip_ping_task (void *pvParameters)
{
	_lwip_ping_info_t *info = pvParameters;

	if (info)
		_lwip_ping_run(info);
}

static int _lwip_ping_task_create (_lwip_ping_info_t *info)
{
	TaskHandle_t *pxCreatedTask = &info->task;
	UBaseType_t uxPriority = uxTaskPriorityGet(NULL) - 1;
	unsigned short usStackDepth = 512 / sizeof(StackType_t);
	void *pvParameters = info;
	BaseType_t status;

	status = xTaskCreate(_lwip_ping_task, "LWIP_PING",
					usStackDepth, pvParameters, uxPriority, pxCreatedTask);
	if (status != pdPASS)
		return -1;

	return 0;
}

static void _lwip_ping_task_delete (_lwip_ping_info_t *info)
{
	if (info->task)
		vTaskDelete(info->task);
}

#endif /* #ifdef _LWIP_PING_TASK_RUN */

/**********************************************************************************************/

#define _LWIP_PING_INFO_STATIC

#ifdef _LWIP_PING_INFO_STATIC
static _lwip_ping_info_t g_lwip_ping_info;
#endif

void _lwip_ping_stop (_lwip_ping_info_t *info)
{
	if (!info)
		return;

#ifdef _LWIP_PING_TASK_RUN
	_lwip_ping_task_delete(info);
#else

#endif

#ifdef _LWIP_PING_INFO_STATIC
	memset(info, 0, sizeof(_lwip_ping_info_t));
#else
	_lwip_ping_free(info);
#endif
}

int _lwip_ping_start (_lwip_ping_params_t *params)
{
	_lwip_ping_info_t *info;
	int ret;

	if (!params)
		return -1;

#ifdef _LWIP_PING_INFO_STATIC
	info = &g_lwip_ping_info;
#else
	info = _lwip_ping_malloc(sizeof(_lwip_ping_info_t));
	if (!info)
		return -1;
#endif

	memcpy(&info->params, params, sizeof(_lwip_ping_params_t));

	_lwip_ping_log("START: remote_ip=%s interval=%u count=%u, size=%u\n",
				ip4addr_ntoa(&info->params.remote_ip),
				info->params.interval, info->params.count, info->params.data_size);

#ifdef _LWIP_PING_TASK_RUN
	ret = _lwip_ping_task_create(info);
#else
	ret = _lwip_ping_run(info);

	_lwip_ping_stop(info);
#endif

	return ret;
}

