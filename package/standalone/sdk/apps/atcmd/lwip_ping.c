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


static int _lwip_ping_request_init (_lwip_ping_info_t *info)
{
	char *buf;
	int buf_size;

#ifdef CONFIG_ATCMD_IPV6
	if (info->ipv6)
		buf_size = ICMP6_ECHO_HLEN + info->params.data_size;
	else
#endif
		buf_size = ICMP_ECHO_HLEN + info->params.data_size;

	_lwip_ping_log("INIT: request_buf_size=%d\n", buf_size);

	buf = _lwip_ping_malloc(buf_size);
	if (buf)
	{
		const uint16_t PING_ID = 0xECEC;
		_lwip_ping_request_t *request = &info->request;
		char *data;
		int i;

#ifdef CONFIG_ATCMD_IPV6
		if (info->ipv6)
		{
			struct icmp6_echo_hdr *header = (struct icmp6_echo_hdr *)buf;
			header->type = ICMP6_TYPE_EREQ;
			header->code = 0;
			header->chksum = 0;
			header->id = PING_ID;
			header->seqno = 0;

			data = buf + ICMP6_ECHO_HLEN;
		}
		else
#endif
		{
			struct icmp_echo_hdr *header = (struct icmp_echo_hdr *)buf;

			header->type = ICMP_ECHO;
			header->code = 0;
			header->chksum = 0;
			header->id = PING_ID;
			header->seqno = 0;

			data = buf + ICMP_ECHO_HLEN;
		}

		for(i = 0 ; i < info->params.data_size ; i++)
			data[i] = i & 0xff;

		request->buf = buf;
		request->buf_size = buf_size;
		request->data_size = info->params.data_size;

		ip_addr_copy(request->remote_ip, info->params.remote_ip);
		request->id = PING_ID;
		request->icmp_seq = 0;
		request->time = 0;

		return 0;
	}

	return -1;
}

static int _lwip_ping_reply_init (_lwip_ping_info_t *info)
{
	char *buf;
	int buf_size;

#ifdef CONFIG_ATCMD_IPV6
	if (info->ipv6)
		buf_size = info->request.buf_size + IP6_HLEN;
	else
#endif
		buf_size = info->request.buf_size + IP_HLEN_MAX;

	_lwip_ping_log("INIT: reply_buf_size=%d\n", buf_size);

	buf = _lwip_ping_malloc(buf_size);
	if (buf)
	{
		_lwip_ping_reply_t *reply = &info->reply;

		reply->buf = buf;
		reply->buf_size = buf_size;

		reply->data_size = 0;
		reply->icmp_seq = 0;
		reply->ttl = 0;
		reply->time = 0;
		ip_addr_set_zero(&reply->remote_ip);

		return 0;
	}

	return -1;
}

static int _lwip_ping_init (_lwip_ping_info_t *info)
{
	struct timeval timeout =
	{
		.tv_sec = info->params.interval / 1000,
		.tv_usec = (info->params.interval % 1000) * 1000
	};

/*	_lwip_ping_debug("INIT: timeout=%ds,%dus\n", timeout.tv_sec, timeout.tv_usec); */

#ifdef CONFIG_ATCMD_IPV6
	info->ipv6 = IP_IS_V6(&info->params.remote_ip) ? true : false;

	_lwip_ping_log("INIT: %s\n", info->ipv6 ? "IPv6" : "IPv4");

	if (info->ipv6)
		info->socket = lwip_socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
	else
#endif
		info->socket = lwip_socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

	if (info->socket < 0)
		return -1;

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
	char *data;
	int i;

	request->icmp_seq++;
	request->time = 0;

	reply->data_size = 0;
	reply->icmp_seq = 0;
	reply->ttl = 0;
	reply->time = 0;
	ip_addr_set_zero(&reply->remote_ip);

#ifdef CONFIG_ATCMD_IPV6
	if (info->ipv6)
	{
		struct icmp6_echo_hdr *icmp6_echo_hdr = (struct icmp6_echo_hdr *)request->buf;

		icmp6_echo_hdr->seqno = htons(request->icmp_seq);
		icmp6_echo_hdr->chksum = 0;
/*		icmp6_echo_hdr->chksum = inet_chksum(request->buf, request->buf_size); */

/*		_lwip_ping_debug("UPDATE: type=%u code=%u chksum=0x%04X id=0x%04X seq=%u len=%u\n",
				icmp6_echo_hdr->type, icmp6_echo_hdr->code, icmp6_echo_hdr->chksum,
				icmp6_echo_hdr->id, icmp6_echo_hdr->seqno, request->buf_size); */

		data = request->buf + ICMP6_ECHO_HLEN;
	}
	else
#endif
	{
		struct icmp_echo_hdr *icmp_echo_hdr = (struct icmp_echo_hdr *)request->buf;

		icmp_echo_hdr->seqno = htons(request->icmp_seq);
		icmp_echo_hdr->chksum = 0;
		icmp_echo_hdr->chksum = inet_chksum(request->buf, request->buf_size);

/*		_lwip_ping_debug("UPDATE: type=%u code=%u chksum=0x%04X id=0x%04X seq=%u len=%u\n",
				icmp_echo_hdr->type, icmp_echo_hdr->code, icmp_echo_hdr->chksum,
				icmp_echo_hdr->id, icmp_echo_hdr->seqno, request->buf_size); */

		data = request->buf + ICMP_ECHO_HLEN;
	}
}

static int _lwip_ping_send (_lwip_ping_info_t *info)
{
	_lwip_ping_request_t *request = &info->request;

	struct sockaddr *to = NULL;
	socklen_t tolen = 0;

#ifdef CONFIG_ATCMD_IPV6
	if (info->ipv6)
	{
		struct sockaddr_in6 *_to;

		tolen = sizeof(struct sockaddr_in6);

		_to = _lwip_ping_malloc(tolen);
		if (_to)
		{
			memset(_to, 0, tolen);

			_to->sin6_len = tolen;
			_to->sin6_family = AF_INET6;
			inet6_addr_from_ip6addr(&_to->sin6_addr, ip_2_ip6(&request->remote_ip));
			_to->sin6_scope_id = SIN6_SCOPE_ID;

			to = (struct sockaddr *)_to;
		}
	}
	else
#endif
	{
		struct sockaddr_in *_to;

		tolen = sizeof(struct sockaddr_in);

		_to = _lwip_ping_malloc(tolen);
		if (_to)
		{
			memset(_to, 0, tolen);

			_to->sin_len = tolen;
			_to->sin_family = AF_INET;
			inet_addr_from_ip4addr(&_to->sin_addr, ip_2_ip4(&request->remote_ip));

			to = (struct sockaddr *)_to;
		}
	}

	if (to)
	{
		int len = lwip_sendto(info->socket, request->buf, request->buf_size, 0, to, tolen);
		uint32_t send_time = atcmd_sys_now();

		if (len <= 0 || len != request->buf_size)
		{
			_lwip_ping_error("SEND: errno=%d len=%d/%u\n", errno, len, request->buf_size);

			info->status = _LWIP_PING_SEND_FAIL;
		}
		else
		{
			request->time = send_time;

			_lwip_ping_log("SEND: time=%u ipaddr=%s icmp_seq=%u size=%u\n",
					request->time, ipaddr_ntoa(&request->remote_ip),
					request->icmp_seq, request->buf_size);
		}

		_lwip_ping_free(to);

		if (len > 0)
			return 0;
	}

	return -1;
}

static int _lwip_ping_receive (_lwip_ping_info_t *info)
{
	_lwip_ping_params_t *params = &info->params;
	_lwip_ping_request_t *request = &info->request;
	_lwip_ping_reply_t *reply = &info->reply;

	struct sockaddr_storage from;
	socklen_t fromlen;
	ip_addr_t remote_ip;
	uint16_t id;
	uint16_t seqno;
	uint8_t ttl;
	uint32_t recv_time;
	int recv_len;
	int data_len;

	ip_addr_set_zero(&remote_ip);
	id = seqno = data_len = ttl = 0;

	while (atcmd_sys_now() < (info->request.time + params->interval))
	{
		fromlen = sizeof(struct sockaddr_storage);

		recv_len = lwip_recvfrom(info->socket, reply->buf, reply->buf_size, 0,
									(struct sockaddr *)&from, &fromlen);
		recv_time = atcmd_sys_now();
		if (recv_len < 0)
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

		if (from.ss_family == AF_INET && fromlen == sizeof(struct sockaddr_in))
		{
			struct sockaddr_in *_from = (struct sockaddr_in *)&from;
			struct ip_hdr *ip_hdr = (struct ip_hdr *)reply->buf;
			struct icmp_echo_hdr *icmp_echo_hdr = (struct icmp_echo_hdr *)&reply->buf[IP_HDR_LEN(ip_hdr)];

			ip_addr_set_any(false, &remote_ip);
			inet_addr_to_ip4addr(ip_2_ip4(&remote_ip), &_from->sin_addr);

			_lwip_ping_debug("RECV: addr=%s id=0x%04X seq=%u ttl=%u len=%u\n",
							ipaddr_ntoa(&remote_ip),
							icmp_echo_hdr->id, icmp_echo_hdr->seqno,
							IPH_TTL(ip_hdr), IPH_LEN(ip_hdr));

			id = ntohs(icmp_echo_hdr->id);
		   	seqno = ntohs(icmp_echo_hdr->seqno);
			ttl = IPH_TTL(ip_hdr);
			data_len = recv_len - IP_HDR_LEN(ip_hdr) - ICMP_ECHO_HLEN;
		}
#ifdef CONFIG_ATCMD_IPV6
		else if (from.ss_family == AF_INET6 && fromlen == sizeof(struct sockaddr_in6))
		{
			struct sockaddr_in6 *_from = (struct sockaddr_in6 *)&from;
			struct ip6_hdr *ip6_hdr = (struct ip6_hdr *)reply->buf;

/*			_lwip_ping_debug("RECV: V=%u TC=%u FL=%X PLEN=%u\n",
					IP6H_V(ip6_hdr), IP6H_TC(ip6_hdr), IP6H_FL(ip6_hdr), IP6H_PLEN(ip6_hdr)); */

			ip_addr_set_any(true, &remote_ip);
			inet6_addr_to_ip6addr(ip_2_ip6(&remote_ip), &_from->sin6_addr);

			if (IP6H_NEXTH(ip6_hdr) != IP6_NEXTH_ICMP6)
			{
				_lwip_ping_error("RECV: no icmp6\n");
				goto _lwip_ping_rx_drop;
			}
			else
			{
				struct icmp6_hdr *icmp6_hdr = (struct icmp6_hdr *)(reply->buf + IP6_HLEN);

				if (icmp6_hdr->type != ICMP6_TYPE_EREP)
				{
					_lwip_ping_error("RECV: no echo reply, type=%u\n", icmp6_hdr->type);
					goto _lwip_ping_rx_drop;
				}
				else
				{
					struct icmp6_echo_hdr *icmp6_echo_hdr = (struct icmp6_echo_hdr *)icmp6_hdr;

					_lwip_ping_debug("RECV: addr=%s id=0x%04X seq=%u ttl=%u len=%u\n",
									ipaddr_ntoa(&remote_ip),
									icmp6_echo_hdr->id, icmp6_echo_hdr->seqno,
									IP6H_HOPLIM(ip6_hdr), IP6H_PLEN(ip6_hdr));

					id = ntohs(icmp6_echo_hdr->id);
				   	seqno = ntohs(icmp6_echo_hdr->seqno);
					ttl = IP6H_HOPLIM(ip6_hdr);
					data_len = recv_len - IP6_HLEN - ICMP6_ECHO_HLEN;
				}
			}
		}
#endif
		else
		{
			_lwip_ping_error("RECV: invalid packet, family=%d fromlen=%d\n", from.ss_family, fromlen);
			goto _lwip_ping_rx_drop;
		}

		if (!ip_addr_cmp(&remote_ip, &request->remote_ip))
			goto _lwip_ping_rx_drop;

		if (id == request->id && seqno == request->icmp_seq && data_len == request->data_size)
		{
			reply->time = recv_time;
			reply->data_size = data_len;
			reply->icmp_seq = seqno;
			reply->ttl = ttl;
			ip_addr_copy(reply->remote_ip, remote_ip);

			_lwip_ping_log("RECV: time=%u ipaddr=%s icmp_seq=%u data_size=%u ttl=%u\n",
							reply->time, ipaddr_ntoa(&reply->remote_ip),
							reply->icmp_seq, reply->data_size, reply->ttl);

			return 0;
		}

_lwip_ping_rx_drop:
		_lwip_ping_log("RECV: drop, ipaddr=%s recv_len=%d data_len=%d\n",
						ipaddr_ntoa(&remote_ip), recv_len, data_len);
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

			_lwip_ping_log("REPORT: %u bytes from %s, icmp_seq=%u ttl=%u time=%ums\n",
					reply->data_size, ipaddr_ntoa(&reply->remote_ip),
					reply->icmp_seq, reply->ttl, response_time);

			ip_addr_copy(report.remote_ip, reply->remote_ip);
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

			ip_addr_copy(report.remote_ip, request->remote_ip);
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
	uint32_t i;

	if (!info || _lwip_ping_init(info) != 0)
	{
		err = -1;
		goto _lwip_ping_done;
	}

	for (i = 0 ; i < info->params.count ; i++)
	{
		_lwip_ping_update(info);

		info->status = _LWIP_PING_SUCCESS;

		if (_lwip_ping_send(info) == 0)
			_lwip_ping_receive(info);

		_lwip_ping_report(info);

		wait_time = info->params.interval - (atcmd_sys_now() - info->request.time);

/*		_lwip_ping_debug("wait_time=%d\n", wait_time); */

		if (wait_time > 0)
			_lwip_ping_delay(wait_time);
	}

_lwip_ping_done:

	_lwip_ping_exit(info);

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

	_lwip_ping_log("STOP\n");

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
				ipaddr_ntoa(&info->params.remote_ip),
				info->params.interval, info->params.count, info->params.data_size);

#ifdef _LWIP_PING_TASK_RUN
	ret = _lwip_ping_task_create(info);
#else
	ret = _lwip_ping_run(info);

	_lwip_ping_stop(info);
#endif

	return ret;
}

int _lwip_ping_params_init (_lwip_ping_params_t *params, bool ipv6)
{
	if (!params)
		return -1;

	if (ipv6)
#ifdef CONFIG_ATCMD_IPV6
		ip_addr_set_zero_ip6(&params->remote_ip);
#else
		return -1;
#endif
	else
		ip_addr_set_zero_ip4(&params->remote_ip);

	params->interval = 1000;
	params->count = 5;
	params->data_size = 64;
	params->report_cb = NULL;

	return 0;
}

