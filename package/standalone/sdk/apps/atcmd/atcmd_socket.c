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
#include "lwip_socket.h"

#ifndef CONFIG_ATCMD_XPUT_IMPROVEMENT
#define CONFIG_ATCMD_SOCKET_EVENT_SEND
#endif

/**********************************************************************************************/

#ifdef CONFIG_ATCMD_DEBUG
#define _atcmd_socket_send(fmt, ...)	_atcmd_info(fmt, ##__VA_ARGS__)
#define _atcmd_socket_recv(fmt, ...)	_atcmd_info(fmt, ##__VA_ARGS__)
#else
#define _atcmd_socket_send(fmt, ...)
#define _atcmd_socket_recv(fmt, ...)
#endif

/**********************************************************************************************/

extern uint32_t _atcmd_timeout_value (const char *cmd);
static int _atcmd_socket_close (int id, bool event);

/**********************************************************************************************/

static const char *str_proto[ATCMD_SOCKET_PROTO_NUM] = { "UDP", "TCP" };
static const char *str_proto_lwr[ATCMD_SOCKET_PROTO_NUM] = { "udp", "tcp" };
static const char *str_addr_any = "0.0.0.0";

//#define CONFIG_ATCMD_SOCKET_STATIC
#ifdef CONFIG_ATCMD_SOCKET_STATIC
static atcmd_socket_t _atcmd_socket[ATCMD_SOCKET_NUM_MAX];
static atcmd_socket_t *g_atcmd_socket = _atcmd_socket;

static atcmd_rxd_t _g_atcmd_socket_rxd;
static atcmd_rxd_t *g_atcmd_socket_rxd = &_g_atcmd_socket_rxd;
#else
static atcmd_socket_t *g_atcmd_socket = NULL;
static atcmd_rxd_t *g_atcmd_socket_rxd = NULL;
#endif

static struct
{
	int id;
	int error;
	bool connected;
} g_atcmd_socket_tcp_client = { -1, 0, false };

static struct
{
	struct
	{
		bool passive;
		bool verbose;
	} rx;
} g_atcmd_socket_trx_mode = { .rx = { false, false } };

/**********************************************************************************************/

static void _atcmd_socket_print (atcmd_socket_t *socket)
{
	if (!socket || socket->id < 0)
		return;

	_atcmd_info("[ Socket Info ]\n");
	_atcmd_info(" - id : %d\n", socket->id);

	switch (socket->protocol)
	{
		case ATCMD_SOCKET_PROTO_UDP:
		case ATCMD_SOCKET_PROTO_TCP:
			_atcmd_info(" - protocol : %s\n", str_proto_lwr[socket->protocol]);
			break;

		default:
			_atcmd_info(" - protocol : unknown\n");
	}

	_atcmd_info(" - local_port : %d\n", socket->local_port);

	if (socket->protocol == ATCMD_SOCKET_PROTO_TCP && socket->remote_port < 0)
	{
/*
	   	_atcmd_info(" - keepalive\n");
		_atcmd_info(" -- idle : %u\n", ATCMD_SOCKET_TCP_KA_IDLE);
		_atcmd_info(" -- interval : %u\n", ATCMD_SOCKET_TCP_KA_INTERVAL);
		_atcmd_info(" -- count : %u\n", ATCMD_SOCKET_TCP_KA_COUNT);
 */
	}
	else
	{
		_atcmd_info(" - remote\n");
		_atcmd_info(" -- port : %d\n", socket->remote_port);
		_atcmd_info(" -- address : %s\n", socket->remote_addr);
	}
}

static void _atcmd_socket_reset (atcmd_socket_t *socket)
{
	memset(socket, 0, sizeof(atcmd_socket_t));

	socket->id = -1;
	socket->protocol = ATCMD_SOCKET_PROTO_NONE;
	socket->local_port = 0;
	socket->remote_port = 0;
	strcpy(socket->remote_addr, str_addr_any);
}

static bool _atcmd_socket_valid_port (int32_t port)
{
	if (port > 0 && port <= 0xffff)
		return true;

	return false;
}

static atcmd_socket_t *_atcmd_socket_search (int id,
											enum ATCMD_SOCKET_PROTO protocol,
											uint16_t local_port)
{
	atcmd_socket_t *socket;
	int i;

	for (i = 0 ; i < ATCMD_SOCKET_NUM_MAX ; i++)
	{
		socket = &g_atcmd_socket[i];
		if (socket->id < 0)
			continue;

		if (id >= 0 && id != socket->id)
			continue;

		if (protocol == ATCMD_SOCKET_PROTO_UDP || protocol == ATCMD_SOCKET_PROTO_TCP)
			if (protocol != socket->protocol)
				continue;

		if (local_port > 0 && local_port != socket->local_port)
			continue;

/*		_atcmd_debug("%s: id=%d proto=%d port=%d remote=%d,%s\n", __func__,
						socket->id, socket->protocol, socket->local_port,
						socket->remote_port, socket->remote_addr); */

		return socket;
	}

	return NULL;
}

#define _atcmd_socket_search_id(id)					_atcmd_socket_search(id, -1, 0)
#define _atcmd_socket_search_port(proto, port)		_atcmd_socket_search(-1, proto, port)

/**********************************************************************************************/

#ifdef CONFIG_ATCMD_SOCKET_EVENT_SEND
static struct
{
	atcmd_socket_t *socket;

	char *data;
	int len;
	int ret;
	int err;

	TaskHandle_t task;
} g_atcmd_socket_send_info =
{
	.socket = NULL,

	.data = NULL,
	.len = 0,
	.ret = 0,
	.err = 0,

	.task = NULL
};

static int _atcmd_socket_send_start (atcmd_socket_t *socket, char *data, int len, int *err)
{
	int ret = -EINPROGRESS;

	if (!g_atcmd_socket_send_info.socket)
	{
		g_atcmd_socket_send_info.socket = socket;
		g_atcmd_socket_send_info.data = data;
		g_atcmd_socket_send_info.len = len;
		g_atcmd_socket_send_info.ret = 0;
		g_atcmd_socket_send_info.task = xTaskGetCurrentTaskHandle();

		ret = _lwip_socket_send_start(socket->id);
		if (ret == 0)
		{
			ret = _lwip_socket_send_wait(socket->id);
			if (ret == 0)
			{
				ret = g_atcmd_socket_send_info.ret;
				*err = g_atcmd_socket_send_info.err;
			}
		}

		memset(&g_atcmd_socket_send_info, 0, sizeof(g_atcmd_socket_send_info));
	}

	if (ret < 0)
	{
		*err = ret;
		ret = 0;
	}

	return ret;
}
#endif

static int __atcmd_socket_send_data (atcmd_socket_t *socket, char *data, int len, int *err)
{
	const int retry_max = 10;
	int retry;
	int ret;
	int i;

	atcmd_wifi_lock();

	for (ret = 0, retry = 0, i = 0 ; i < len && ret >= 0 ; )
	{
		if (socket->protocol == ATCMD_SOCKET_PROTO_TCP)
			ret = _lwip_socket_send_tcp(socket->id, data + i, len - i);
		else
		{
			ret = _lwip_socket_send_udp(socket->id,
						socket->remote_addr, socket->remote_port, data + i, len - i);
		}

		if (ret > 0)
		{
			i += ret;

			_atcmd_socket_send("%s_send: id=%d len=%d/%d\n",
							str_proto_lwr[socket->protocol], socket->id, i, len);
		}
		else if (ret == 0 || ret == -EAGAIN || ret == -ENOBUFS)
		{
			retry++;

			if (retry > retry_max)
			{
				_atcmd_info("%s_send: id=%d len=%d/%d retry=%d\n",
							str_proto_lwr[socket->protocol], socket->id, i, len, retry);
				break;
			}

			_delay_ms(10);

			ret = 0;
		}
	}

	*err = ret < 0 ? ret : 0;

	atcmd_wifi_unlock();

	return i;
}

static int _atcmd_socket_send_data (atcmd_socket_t *socket, char *data, int len, int *err)
{
#ifdef _atcmd_socket_send
	static uint32_t cnt[ATCMD_SOCKET_PROTO_NUM] = { 0, 0 };
#endif
	atcmd_socket_t *_socket;
	const char *_str_proto;
	const char *_str_proto_lwr = NULL;
	int ret = 0;

	if (!socket || !data || !len)
	{
		*err = -EINVAL;
		goto socket_send_data_fail;
	}

	switch (socket->protocol)
	{
		case ATCMD_SOCKET_PROTO_UDP:
		case ATCMD_SOCKET_PROTO_TCP:
			_str_proto = str_proto[socket->protocol];
			_str_proto_lwr = str_proto_lwr[socket->protocol];
			break;

		default:
			*err = -EPROTOTYPE;
			goto socket_send_data_fail;
	}

	_socket = _atcmd_socket_search_id(socket->id);
	if (!_socket)
	{
		*err = -ENOTSOCK;
		goto socket_send_data_fail;
	}

	socket->protocol = _socket->protocol;
	socket->local_port = _socket->local_port;
	if (socket->remote_port == 0 && _socket->remote_port > 0)
	{
		socket->remote_port = _socket->remote_port;
		strcpy(socket->remote_addr, _socket->remote_addr);
	}

	if (socket->remote_port == 0)
	{
		_atcmd_socket_send("%s_send: id=%d len=%d cnt=%u\n",
						_str_proto_lwr, socket->id, len,
						cnt[socket->protocol]++);
	}
	else
	{
		_atcmd_socket_send("%s_send: id=%d len=%d remote=%u,%s cnt=%u\n",
						_str_proto_lwr, socket->id, len,
						socket->remote_port, socket->remote_addr,
						cnt[socket->protocol]++);
	}

#ifdef CONFIG_ATCMD_SOCKET_EVENT_SEND
	ret = _atcmd_socket_send_start(socket, data, len, err);
#else
	{
		fd_set fds_write;
		struct timeval timeout;
		int count = 3;

		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		do
		{
			FD_ZERO(&fds_write);
			FD_SET(socket->id, &fds_write);
			ret = select(socket->id+1, NULL, &fds_write, NULL, &timeout);
			if (ret > 0 && FD_ISSET(socket->id, &fds_write)){
				ret = __atcmd_socket_send_data(socket, data, len, err);
			} else if (ret < 0) {
				_atcmd_error("%s(%d) select error! (%d)\n", ret);
				return ret;
			}
		} while (--count > 0 && ret <= 0);
	}
#endif

	if (ret == len)
		_atcmd_socket_send("%s_send: id=%d len=%d done\n", _str_proto_lwr, socket->id, len);

socket_send_data_fail:

	if (*err)
	{
		if (_str_proto_lwr)
		{
			_atcmd_info("%s_send: id=%d len=%d/%d err=%d,%s\n",
							_str_proto_lwr, socket->id, ret, len,
							*err, atcmd_strerror(*err));
		}
		else
		{
			_atcmd_info("socket_send: id=%d err=%d,%s\n",
							socket->id, *err, atcmd_strerror(*err));
		}

		switch (*err)
		{
			case -ENOTCONN:
			case -ECONNRESET:
				_atcmd_socket_close(socket->id, true);
		}
	}

	return ret;
}

static void _atcmd_socket_recv_data (atcmd_socket_t *socket, atcmd_rxd_t *rxd)
{
#ifndef CONFIG_ATCMD_XPUT_IMPROVEMENT
	char param_remote_addr[ATCMD_STR_PARAM_SIZE(ATCMD_IP4_ADDR_LEN_MAX)];

	switch (socket->protocol)
	{
		case ATCMD_SOCKET_PROTO_UDP:
		case ATCMD_SOCKET_PROTO_TCP:
			break;

		default:
			return;
	}

	if (atcmd_str_to_param(socket->remote_addr, param_remote_addr, sizeof(param_remote_addr)))
#endif
	{
		char msg[ATCMD_MSG_LEN_MAX + 1];
		char *buf;
		int len;
#ifdef CONFIG_ATCMD_XPUT_IMPROVEMENT
		static atcmd_socket_t s_prev_socket = {0, 0, 0, 0, {0, }};
		static int s_prev_len = 0;

		if (memcmp(&s_prev_socket, socket, sizeof(atcmd_socket_t)) == 0
			&& s_prev_len == rxd->len.data) {
			buf = rxd->buf.msg + sizeof(rxd->buf.msg) - rxd->len.msg;
			len = rxd->len.msg + rxd->len.data;
		} else {
			if (g_atcmd_socket_trx_mode.rx.verbose) {
				rxd->len.msg = snprintf(msg, sizeof(msg) - 1, "+RXD:%d,%d,\"%s\",%u\r\n",
											socket->id, rxd->len.data,
											socket->remote_addr, socket->remote_port);
			} else {
				rxd->len.msg = snprintf(msg, sizeof(msg) - 1, "+RXD:%d,%d\r\n",
											socket->id, rxd->len.data);
			}

			buf = rxd->buf.msg + sizeof(rxd->buf.msg) - rxd->len.msg;
			len = rxd->len.msg + rxd->len.data;

			memcpy(buf, msg, rxd->len.msg);

			memcpy(&s_prev_socket, socket, sizeof(atcmd_socket_t));
			s_prev_len = rxd->len.data;
		}
#else
		if (g_atcmd_socket_trx_mode.rx.verbose)
		{
			rxd->len.msg = ATCMD_MSG_SRXD(msg, sizeof(msg) - 1, "%d,%d,%s,%u",
										socket->id, rxd->len.data,
										socket->remote_addr, socket->remote_port);
		}
		else
		{
			rxd->len.msg = ATCMD_MSG_SRXD(msg, sizeof(msg) - 1, "%d,%d",
										socket->id, rxd->len.data);
		}

		buf = rxd->buf.msg + sizeof(rxd->buf.msg) - rxd->len.msg;
		len = rxd->len.msg + rxd->len.data;

		memcpy(buf, msg, rxd->len.msg);
#endif

		atcmd_transmit(buf, len);
	}
}

/**********************************************************************************************/

static int _atcmd_socket_event_handler (int type, int id, ...)
{
	va_list ap;
	int ret = 0;

	va_start(ap, id);

	switch (type)
	{
		case ATCMD_SOCKET_EVENT_CONNECT:
			_atcmd_info("SEVENT: CONNECT, id=%d\n", id);
			ATCMD_MSG_SEVENT("\"CONNECT\",%d", id);
			break;

		case ATCMD_SOCKET_EVENT_CLOSE:
			_atcmd_info("SEVENT: CLOSE, id=%d\n", id);
			ATCMD_MSG_SEVENT("\"CLOSE\",%d", id);
			break;

		case ATCMD_SOCKET_EVENT_SEND_IDLE:
		{
			uint32_t done = va_arg(ap, uint32_t);
			uint32_t drop = va_arg(ap, uint32_t);
			uint32_t wait = va_arg(ap, uint32_t);

			_atcmd_info("SEVENT: SEND_IDLE, id=%d done=%u drop=%u wait=%u\n", id, done, drop, wait);
			ATCMD_MSG_SEVENT("\"SEND_IDLE\",%d,%u,%u,%u", id, done, drop, wait);
			break;
		}

		case ATCMD_SOCKET_EVENT_SEND_DROP:
		{
			uint32_t drop = va_arg(ap, uint32_t);

			_atcmd_info("SEVENT: SEND_DROP, id=%d drop=%u\n", id, drop);
			ATCMD_MSG_SEVENT("\"SEND_DROP\",%d,%u", id, drop);
			break;
		}

		case ATCMD_SOCKET_EVENT_SEND_EXIT:
		{
			uint32_t done = va_arg(ap, uint32_t);
			uint32_t drop = va_arg(ap, uint32_t);

			_atcmd_info("SEVENT: SEND_EXIT, id=%d done=%u drop=%u\n", id, done, drop);
			ATCMD_MSG_SEVENT("\"SEND_EXIT\",%d,%u,%u", id, done, drop);
			break;
		}

		case ATCMD_SOCKET_EVENT_SEND_ERROR:
		{
			int err = va_arg(ap, int);

			_atcmd_info("SEVENT: SEND_ERROR, id=%d err=%d,%s\n", id, err, atcmd_strerror(err));
			ATCMD_MSG_SEVENT("\"SEND_ERROR\",%d,%d", id, err);
			break;
		}

		case ATCMD_SOCKET_EVENT_RECV_ERROR:
		{
			int err = va_arg(ap, int);

			_atcmd_info("SEVENT: RECV_ERROR, id=%d err=%d,%s\n", id, err, atcmd_strerror(err));
			ATCMD_MSG_SEVENT("\"RECV_ERROR\",%d,%d", id, err);
			break;
		}

		default:
			_atcmd_info("SEVENT: invalid type (%d)\n", type);
			ret = -1;
	}

	va_end(ap);

	return ret;
}

#ifdef CONFIG_ATCMD_SOCKET_EVENT_SEND
static void _atcmd_socket_send_handler (int id)
{
	if (g_atcmd_socket_send_info.socket)
	{
		atcmd_socket_t *socket = g_atcmd_socket_send_info.socket;

		if (socket->id == id)
		{
			char *data = g_atcmd_socket_send_info.data;
			int len = g_atcmd_socket_send_info.len;
			int ret;
			int err;

			ret = __atcmd_socket_send_data(socket, data, len, &err);

			g_atcmd_socket_send_info.socket = NULL;
			g_atcmd_socket_send_info.data = NULL;
			g_atcmd_socket_send_info.len = 0;
			g_atcmd_socket_send_info.ret = ret;
			g_atcmd_socket_send_info.err = err;
		}
	}
}
#endif

static void _atcmd_socket_recv_handler (int id)
{
	atcmd_socket_t *socket = _atcmd_socket_search_id(id);

	if (!socket)
	{
		_atcmd_error("no socket\n");
		return;
	}

	if (!g_atcmd_socket_rxd)
	{
		_atcmd_error("no rx buffer\n");
		return;
	}

	atcmd_wifi_lock();

	switch (socket->protocol)
	{
		case ATCMD_SOCKET_PROTO_UDP:
		case ATCMD_SOCKET_PROTO_TCP:
		{
			atcmd_rxd_t *rxd = g_atcmd_socket_rxd;
#ifndef CONFIG_ATCMD_XPUT_IMPROVEMENT
			atcmd_socket_t rxd_socket;
			char remote_addr[ATCMD_IP4_ADDR_LEN_MAX + 1];
			uint16_t remote_port;
#endif
			int ret;

#ifdef CONFIG_ATCMD_XPUT_IMPROVEMENT
			ret = _lwip_socket_recv(id, socket->remote_addr, &socket->remote_port,
									rxd->buf.data, sizeof(rxd->buf.data));
#else
			ret = _lwip_socket_recv(id, remote_addr, &remote_port,
									rxd->buf.data, sizeof(rxd->buf.data));
#endif

			if (ret >= 0)
			{
#ifndef CONFIG_ATCMD_XPUT_IMPROVEMENT
				memcpy(&rxd_socket, socket, sizeof(atcmd_socket_t));

				rxd_socket.remote_port = remote_port;
				strcpy(rxd_socket.remote_addr, remote_addr);

				_atcmd_socket_recv("%s_recv: id=%d port=%u remote=%u,%s len=%d\n",
							str_proto_lwr[rxd_socket.protocol],
							rxd_socket.id, rxd_socket.local_port,
							rxd_socket.remote_port, rxd_socket.remote_addr, ret);
#endif

				if (ret > 0)
				{
					rxd->len.data = ret;

#ifdef CONFIG_ATCMD_XPUT_IMPROVEMENT
					_atcmd_socket_recv_data(socket, rxd);
#else
					_atcmd_socket_recv_data(&rxd_socket, rxd);
#endif
				}

				break;
			}

			if (!_atcmd_socket_search_id(id))
				break;

			_atcmd_socket_event_handler(ATCMD_SOCKET_EVENT_RECV_ERROR, id, ret);

			switch (ret)
			{
				case -ENOTCONN:
				case -ECONNREFUSED:
					_atcmd_socket_close(id, true);
			}

			break;
		}

		default:
			_atcmd_error("invalid socket protocol\n");
	}

	atcmd_wifi_unlock();

}

static void _atcmd_socket_tcp_connect_handler (int id, const char *remote_addr, uint16_t remote_port)
{
	atcmd_socket_t *socket;
	int i;

	_atcmd_info("tcp_connect: id=%d remote=%u,%s\n",
							id, remote_port, remote_addr ? remote_addr : "");

	for (i = 0 ; i < ATCMD_SOCKET_NUM_MAX ; i++)
	{
		socket = &g_atcmd_socket[i];

		if (socket->id < 0)
		{
			socket->id = id;
			socket->protocol = ATCMD_SOCKET_PROTO_TCP;
			socket->local_port = 0;
			socket->remote_port = remote_port;
			strcpy(socket->remote_addr, remote_addr ? remote_addr : str_addr_any );

			_atcmd_socket_print(socket);
			_atcmd_socket_event_handler(ATCMD_SOCKET_EVENT_CONNECT, id);
			return;
		}
	}

	_atcmd_info("tcp_connect: id=%d socket_full\n", id);
}

static int _atcmd_socket_init (void)
{
	lwip_socket_cb_t cb;
	int i;

	memset(&cb, 0, sizeof(lwip_socket_cb_t));

#ifndef CONFIG_ATCMD_SOCKET_EVENT_SEND
	_atcmd_info("ATCMD_SOCKET_DIRECT_SEND\n");
#else
	_atcmd_info("ATCMD_SOCKET_EVENT_SEND\n");
	cb.send_ready = _atcmd_socket_send_handler;
#endif
	cb.recv_ready = _atcmd_socket_recv_handler;
	cb.tcp_connect = _atcmd_socket_tcp_connect_handler;

	for (i = 0 ; i < ATCMD_SOCKET_NUM_MAX ; i++)
		_atcmd_socket_reset(&g_atcmd_socket[i]);

	return _lwip_socket_init(&cb);
}

static int _atcmd_socket_exit (void)
{
	return _lwip_socket_exit();
}

static int _atcmd_socket_open (atcmd_socket_t *socket)
{
	int i;

/*
	if (!socket)
		return -EINVAL;

	switch (socket->protocol)
	{
		case ATCMD_SOCKET_PROTO_UDP:
		case ATCMD_SOCKET_PROTO_TCP:
			break;

		default:
			return -EINVAL;
	}
*/

	for (i = 0 ; i < ATCMD_SOCKET_NUM_MAX ; i++)
	{
		if (g_atcmd_socket[i].id < 0)
			continue;

		if (socket->protocol != g_atcmd_socket[i].protocol)
			continue;

		if (socket->local_port == 0)
			continue;

		if (socket->local_port == g_atcmd_socket[i].local_port)
		{
			_atcmd_info("%s_open: existing socket\n", str_proto_lwr[socket->protocol]);
			_atcmd_socket_print(&g_atcmd_socket[i]);

			return -EINVAL;
		}
	}

	for (i = 0 ; i < ATCMD_SOCKET_NUM_MAX ; i++)
	{
		if (g_atcmd_socket[i].id < 0)
			break;
	}

	if (i >= ATCMD_SOCKET_NUM_MAX)
	{
		_atcmd_error("socket full\n");

		return -EPERM;
	}
	else
	{
		int id = -1;
		int ret = -EINVAL;

		if (socket->protocol == ATCMD_SOCKET_PROTO_UDP)
			ret = _lwip_socket_open_udp(&id, socket->local_port);
		else if (socket->local_port > 0 && socket->remote_port == 0)
			ret = _lwip_socket_open_tcp_server(&id, socket->local_port);
		else if (socket->local_port == 0 && socket->remote_port > 0)
		{
			uint32_t timeout_msec = _atcmd_timeout_value("SOPEN");

			if (timeout_msec == 0)
				timeout_msec = 30 * 1000;

			ret = _lwip_socket_open_tcp_client(&id, socket->remote_addr, socket->remote_port,
												timeout_msec);
		}

		socket->id = id;

		_atcmd_info("%s_open: id=%d port=%u remote=%u,%s err=%d,%s\n",
					str_proto_lwr[socket->protocol], socket->id,
					socket->local_port, socket->remote_port, socket->remote_addr,
					ret, atcmd_strerror(ret));

		if (socket->id < 0)
			return ret;

		memcpy(&g_atcmd_socket[i], socket, sizeof(atcmd_socket_t));

		_atcmd_socket_print(&g_atcmd_socket[i]);

		ATCMD_MSG_INFO("SOPEN", "%d", socket->id);

		return socket->id;
	}
}

static int _atcmd_socket_close (int id, bool event)
{
	atcmd_socket_t *socket;
	int ret = 0;
	int i;

	for (i = 0 ; i < ATCMD_SOCKET_NUM_MAX ; i++)
	{
		socket = &g_atcmd_socket[i];

		if (socket->id < 0)
			continue;

		if (id >= 0 && id != socket->id)
			continue;

		ret = _lwip_socket_close(socket->id);

		_atcmd_info("%s_close: id=%d port=%d err=%d,%s\n",
				str_proto_lwr[socket->protocol], socket->id, socket->local_port,
				ret, atcmd_strerror(ret));

		if (event)
			_atcmd_socket_event_handler(ATCMD_SOCKET_EVENT_CLOSE, id);
		else
			ATCMD_MSG_INFO("SCLOSE", "%d", socket->id);

		_atcmd_socket_reset(socket);

		if (id >= 0)
			break;
	}

	return ATCMD_SUCCESS;
}

/**********************************************************************************************/

static int __atcmd_socket_open_set (enum ATCMD_SOCKET_PROTO protocol, int argc, char *argv[])
{
	char *param_local_port = NULL;
	char *param_remote_port = NULL;
	char *param_remote_addr = NULL;
	atcmd_socket_t socket;
	int id;

	switch (argc)
	{
		case 2: /* tcp client */
		{
			char str_remote_addr[ATCMD_STR_SIZE(ATCMD_IP4_ADDR_LEN_MAX)];
			ip4_addr_t ip4addr;

			param_remote_addr = argv[0];
			param_remote_port = argv[1];

			if (!atcmd_param_to_str(param_remote_addr, str_remote_addr, sizeof(str_remote_addr)) ||
				!ip4addr_aton(str_remote_addr, &ip4addr))
				return -EINVAL;

			param_remote_addr = str_remote_addr;
			break;
		}

		case 1: /* tcp server, udp server/client */
			param_local_port = argv[0];
			break;

		default:
			return -EINVAL;
	}

	_atcmd_socket_reset(&socket);

	socket.protocol = protocol;

	if (param_local_port)
	{
		long local_port = atoi(param_local_port);

		if (!_atcmd_socket_valid_port(local_port))
			return -EINVAL;

		socket.local_port = local_port;
	}

	if (param_remote_port && param_remote_addr)
	{
		long remote_port = atoi(param_remote_port);

		if (!_atcmd_socket_valid_port(remote_port))
			return -EINVAL;

		if (strlen(param_remote_addr) >= IP4ADDR_STRLEN_MAX)
			return -EINVAL;

		socket.remote_port = remote_port;
		strcpy(socket.remote_addr, param_remote_addr);
	}

	id = _atcmd_socket_open(&socket);
	if (id < 0)
		_atcmd_info("socket_set: err=%d\n", id);

	return id;
}

static int _atcmd_socket_open_set (int argc, char *argv[])
{
	char *param_protocol = NULL;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+SOPEN=\"udp\",<local_port>");
			ATCMD_MSG_HELP("AT+SOPEN=\"tcp\",<local_port>");
			ATCMD_MSG_HELP("AT+SOPEN=\"tcp\",\"<remote_ip>\",<remote_port>");
			break;

		case 3:
		case 2:
		{
			char str_protocol[ATCMD_STR_SIZE(3)];

			param_protocol = argv[0];

			if (atcmd_param_to_str(param_protocol, str_protocol, sizeof(str_protocol)))
			{
				enum ATCMD_SOCKET_PROTO proto;
				int id;

				if (strcmp(str_protocol, "udp") == 0 || strcmp(str_protocol, "UDP") == 0)
					proto = ATCMD_SOCKET_PROTO_UDP;
				else if (strcmp(str_protocol, "tcp") == 0 || strcmp(str_protocol, "TCP") == 0)
					proto = ATCMD_SOCKET_PROTO_TCP;
				else
					return ATCMD_ERROR_INVAL;

				id = __atcmd_socket_open_set(proto, argc - 1, argv + 1);

				switch (id)
				{
					case -EINVAL:
						return ATCMD_ERROR_INVAL;

					case -EINPROGRESS:
						return ATCMD_ERROR_BUSY;

					default:
						if (id >= 0)
							return ATCMD_SUCCESS;
						else
							return ATCMD_ERROR_FAIL;
				}
			}
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_socket_open =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_SOCKET,

	.cmd = "OPEN",
	.id = ATCMD_SOCKET_OPEN,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = NULL,
	.handler[ATCMD_HANDLER_SET] = _atcmd_socket_open_set,
};

/**********************************************************************************************/

static int _atcmd_socket_close_run (int argc, char *argv[])
{
	return _atcmd_socket_close(-1, false);
}

static int _atcmd_socket_close_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+SCLOSE=<socket_ID>");
			break;

		case 1:
		{
			int id = atoi(argv[0]);

			if (id >= 0 && id < ATCMD_SOCKET_NUM_MAX)
				return _atcmd_socket_close(id, false);
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_socket_close =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_SOCKET,

	.cmd = "CLOSE",
	.id = ATCMD_SOCKET_CLOSE,

	.handler[ATCMD_HANDLER_RUN] = _atcmd_socket_close_run,
	.handler[ATCMD_HANDLER_GET] = NULL,
	.handler[ATCMD_HANDLER_SET] = _atcmd_socket_close_set,
};

/**********************************************************************************************/

static int __atcmd_socket_list_get (enum ATCMD_SOCKET_PROTO proto)
{
	char param_protocol[ATCMD_STR_PARAM_SIZE(3)];
	char param_remote_addr[ATCMD_STR_PARAM_SIZE(ATCMD_IP4_ADDR_LEN_MAX)];
	atcmd_socket_t *socket;
	int i;

	for (i = 0 ; i < ATCMD_SOCKET_NUM_MAX ; i++)
	{
		socket = &g_atcmd_socket[i];

		if (socket->id < 0)
			continue;

		if (proto != ATCMD_SOCKET_PROTO_ALL && socket->protocol != proto)
			continue;

		if (!atcmd_str_to_param(str_proto[socket->protocol], param_protocol, sizeof(param_protocol)) ||
			!atcmd_str_to_param(socket->remote_addr, param_remote_addr, sizeof(param_remote_addr)))
			continue;

		ATCMD_MSG_INFO("SLIST", "%d,%s,%s,%d,%d",
						socket->id, param_protocol,
						param_remote_addr, socket->remote_port,
						socket->local_port);
	}

	return 0;
}

static int _atcmd_socket_list_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			__atcmd_socket_list_get(ATCMD_SOCKET_PROTO_ALL);
			break;

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_socket_list =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_SOCKET,

	.cmd = "LIST",
	.id = ATCMD_SOCKET_LIST,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_socket_list_get,
	.handler[ATCMD_HANDLER_SET] = NULL,
};

/**********************************************************************************************/

static int _atcmd_socket_send_set (int argc, char *argv[])
{
	char *param_len = NULL;
	char *param_remote_ip = NULL;
	char *param_remote_port = NULL;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+SSEND=<socket_ID>[,<length>]"); /* tcp */
			ATCMD_MSG_HELP("AT+SSEND=<socket_ID>,\"<remote_ip>\",<remote_port>[,<length>]"); /* udp */
			break;

		case 4:
			param_len = argv[3];

		case 3:
			param_remote_ip = argv[1];
			param_remote_port = argv[2];

		case 2:
			if (!param_len && argc == 2)
				param_len = argv[1];

		case 1:
		{
			int id = atoi(argv[0]);
			atcmd_socket_t *socket = _atcmd_socket_search_id(id);
			uint32_t len = 0;

			if (socket)
			{
				uint32_t timeout_msec;

				if (param_remote_ip && param_remote_port)
				{
					char remote_ip[ATCMD_STR_SIZE(ATCMD_IP4_ADDR_LEN_MAX)];
					int remote_ip_len;
					int32_t remote_port;

					if (!atcmd_param_to_str(param_remote_ip, remote_ip, sizeof(remote_ip)))
						return ATCMD_ERROR_INVAL;

					remote_ip_len = strlen(remote_ip);

					if (remote_ip_len < ATCMD_IP4_ADDR_LEN_MIN || remote_ip_len > ATCMD_IP4_ADDR_LEN_MAX)
						return ATCMD_ERROR_INVAL;

					remote_port = atoi(param_remote_port);
					if (!_atcmd_socket_valid_port(remote_port))
						return ATCMD_ERROR_INVAL;

					strcpy(socket->remote_addr, remote_ip);
					socket->remote_port = remote_port;
				}

				if (param_len)
				{
					len = atoi(param_len);
					if (abs(len) > ATCMD_DATA_LEN_MAX)
						return ATCMD_ERROR_INVAL;
				}
				else if (_hif_get_type() == _HIF_TYPE_UART)
					return ATCMD_ERROR_INVAL;

				timeout_msec = _atcmd_timeout_value("SSEND");
				if (timeout_msec == 0)
					timeout_msec = 1000;

/*				_atcmd_debug("ssend: id=%d remote=%s,%d len=%d timeout_msec=%u\n",
							socket->id, socket->remote_addr, socket->remote_port, len, timeout_msec); */

				atcmd_data_mode_enable(socket, len, timeout_msec);

				break;
			}
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_socket_send =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_SOCKET,

	.cmd = "SEND",
	.id = ATCMD_SOCKET_SEND,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = NULL,
	.handler[ATCMD_HANDLER_SET] = _atcmd_socket_send_set,
};

/**********************************************************************************************/
#if 0
static int _atcmd_socket_recv_mode_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_INFO("SRXMODE", "%d", g_atcmd_socket_trx_mode.rx.passive);
			break;

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_socket_recv_mode_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+SRXMODE=<mode>");
			/*
			 * 0: active
			 * 1: passive
			 */
			break;

		case 1:
		{
			int passive = atoi(argv[0]);

			if (passive == 0 || passive == 1)
			{
				g_atcmd_socket_trx_mode.rx.passive = !!passive;
				break;
			}
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_socket_recv_mode =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_SOCKET,

	.cmd = "RXMODE",
	.id = ATCMD_SOCKET_RECV_MODE,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_socket_recv_mode_get,
	.handler[ATCMD_HANDLER_SET] = NULL, /*_atcmd_socket_recv_mode_set, */
};
#endif
/**********************************************************************************************/

static int _atcmd_socket_recv_log_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_INFO("SRXLOGLEVEL", "%d", g_atcmd_socket_trx_mode.rx.verbose);
			break;

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_socket_recv_log_set (int argc, char *argv[])
{
	char *param_xxx = NULL;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+SRXLOGLEVEL=<mode>");
/*			ATCMD_MSG_HELP("AT+SRXLOGLEVEL={0|1}	# 0:terse 1:verbose"); */
			break;

		case 1:
		{
			int verbose = atoi(argv[0]);

			if (verbose == 0 || verbose == 1)
			{
				g_atcmd_socket_trx_mode.rx.verbose = !!verbose;
				break;
			}
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_socket_recv_log =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_SOCKET,

	.cmd = "RXLOGLEVEL",
	.id = ATCMD_SOCKET_RECV_LOG,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_socket_recv_log_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_socket_recv_log_set,
};

/**********************************************************************************************/
#if 0
static int _atcmd_socket_recv_avail_set (int argc, char *argv[])
{
	char *param_xxx = NULL;

	if (!g_atcmd_socket_trx_mode.rx.passive)
		return ATCMD_ERROR_NOTSUPP;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+SAVAIL=<socket_ID>");
			break;

		case 1:
		{
			int id = atoi(argv[0]);

			if (id >= 0 && id < ATCMD_SOCKET_NUM_MAX)
			{
				break;
			}
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_socket_recv_avail =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_SOCKET,

	.cmd = "AVAIL",
	.id = ATCMD_SOCKET_RECV_AVAIL,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = NULL,
	.handler[ATCMD_HANDLER_SET] = _atcmd_socket_recv_avail_set,
};
#endif
/**********************************************************************************************/
#if 0
static int _atcmd_socket_recv_set (int argc, char *argv[])
{
	if (!g_atcmd_socket_trx_mode.rx.passive)
		return ATCMD_ERROR_NOTSUPP;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+SREAD=<socket_ID>,<max_read_length>");
			break;

		case 2:
		{
			int id = atoi(argv[0]);
			int length = atoi(argv[1]);

			if (id < 0 || id >= ATCMD_SOCKET_NUM_MAX || length <= 0)
				return ATCMD_ERROR_INVAL;

			/* read from fifo and send to terminal/host. */

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_socket_recv =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_SOCKET,

	.cmd = "RECV",
	.id = ATCMD_SOCKET_RECV,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = NULL,
	.handler[ATCMD_HANDLER_SET] = _atcmd_socket_recv_set,
};
#endif
/**********************************************************************************************/

extern int _atcmd_basic_timeout_get (int argc, char *argv[]);
extern int _atcmd_basic_timeout_set (int argc, char *argv[]);

static int _atcmd_socket_timeout_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			argc = 1;
			argv[0] = "1";

			return _atcmd_basic_timeout_get(argc, argv);
		}

		default:
			return ATCMD_ERROR_INVAL;
	}
}

static int _atcmd_socket_timeout_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+STIMEOUT=\"<command>\",<time>");
			break;

		case 2:
			return _atcmd_basic_timeout_set(argc, argv);

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_socket_timeout =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_SOCKET,

	.cmd = "TIMEOUT",
	.id = ATCMD_SOCKET_TIMEOUT,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_socket_timeout_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_socket_timeout_set,
};

/**********************************************************************************************/

static atcmd_group_t g_atcmd_group_socket =
{
	.list.next = NULL,
	.list.prev = NULL,

	.name ="SOCKET",
	.id = ATCMD_GROUP_SOCKET,

	.cmd_prefix = "S",
	.cmd_prefix_size = 1,

	.cmd_list_head.next = NULL,
	.cmd_list_head.prev = NULL,
};

static atcmd_info_t *g_atcmd_info_socket[] =
{
	&g_atcmd_socket_open,
	&g_atcmd_socket_close,
	&g_atcmd_socket_list,
	&g_atcmd_socket_send,
/*	&g_atcmd_socket_recv_mode, */
	&g_atcmd_socket_recv_log,
/*	&g_atcmd_socket_recv_avail, */
/*	&g_atcmd_socket_recv, */
	&g_atcmd_socket_timeout,

	NULL
};

void atcmd_socket_reset (atcmd_socket_t *socket)
{
	_atcmd_socket_reset(socket);
}

int atcmd_socket_enable (void)
{
	int i;

#ifndef CONFIG_ATCMD_SOCKET_STATIC
	g_atcmd_socket = _atcmd_malloc(sizeof(atcmd_socket_t) * ATCMD_SOCKET_NUM_MAX);
	g_atcmd_socket_rxd = _atcmd_malloc(sizeof(atcmd_rxd_t));

	if (!g_atcmd_socket || !g_atcmd_socket_rxd)
	{
		_atcmd_error("malloc() failed\n");

		if (g_atcmd_socket)
			_atcmd_free(g_atcmd_socket);

		if (g_atcmd_socket_rxd)
			_atcmd_free(g_atcmd_socket_rxd);

		return -1;
	}
#endif

	_atcmd_socket_init();

	if (atcmd_group_register(&g_atcmd_group_socket) != 0)
		return -1;

	for (i = 0 ; g_atcmd_info_socket[i] ; i++)
	{
		if (atcmd_info_register(ATCMD_GROUP_SOCKET, g_atcmd_info_socket[i]) != 0)
			return -1;
	}

#ifdef CONFIG_ATCMD_DEBUG
	atcmd_info_print(&g_atcmd_group_socket);
#endif
	return 0;
}

void atcmd_socket_disable (void)
{
	int i;

	for (i = 0 ; g_atcmd_info_socket[i] ; i++)
		atcmd_info_unregister(ATCMD_GROUP_SOCKET, g_atcmd_info_socket[i]->id);

	atcmd_group_unregister(ATCMD_GROUP_SOCKET);

	_atcmd_free(g_atcmd_socket_rxd);
	_atcmd_free(g_atcmd_socket);
}

int atcmd_socket_send_data (atcmd_socket_t *socket, char *data, int len, int *err)
{
	return _atcmd_socket_send_data(socket, data, len, err);
}

int atcmd_socket_event_send_idle (int id, uint32_t done, uint32_t drop, uint32_t wait)
{
	return _atcmd_socket_event_handler(ATCMD_SOCKET_EVENT_SEND_IDLE, id, done, drop, wait);
}

int atcmd_socket_event_send_drop (int id, uint32_t drop)
{
	return _atcmd_socket_event_handler(ATCMD_SOCKET_EVENT_SEND_DROP, id, drop);
}

int atcmd_socket_event_send_exit (int id, uint32_t done, uint32_t drop)
{
	return _atcmd_socket_event_handler(ATCMD_SOCKET_EVENT_SEND_EXIT, id, done, drop);
}

int atcmd_socket_event_send_error (int id, int err)
{
	return _atcmd_socket_event_handler(ATCMD_SOCKET_EVENT_SEND_ERROR, id, err);
}

