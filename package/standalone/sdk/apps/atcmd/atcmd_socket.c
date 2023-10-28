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


/**********************************************************************************************/

#define _atcmd_socket_send(fmt, ...)	/* _atcmd_info(fmt, ##__VA_ARGS__) */
#define _atcmd_socket_recv(fmt, ...)	/* _atcmd_info(fmt, ##__VA_ARGS__) */

/**********************************************************************************************/

extern uint32_t _atcmd_timeout_value (const char *cmd);
static int _atcmd_socket_close (int id, int err);

/**********************************************************************************************/

static const char *str_proto[ATCMD_SOCKET_PROTO_NUM] = { "UDP", "TCP" };
static const char *str_proto_lwr[ATCMD_SOCKET_PROTO_NUM] = { "udp", "tcp" };

//#define CONFIG_ATCMD_SOCKET_STATIC
#ifdef CONFIG_ATCMD_SOCKET_STATIC
static atcmd_socket_t _atcmd_socket[ATCMD_SOCKET_NUM_MAX];
static atcmd_socket_t *g_atcmd_socket = _atcmd_socket;

static atcmd_socket_rxd_t _g_atcmd_socket_rxd;
static atcmd_socket_rxd_t *g_atcmd_socket_rxd = &_g_atcmd_socket_rxd;
#else
static atcmd_socket_t *g_atcmd_socket = NULL;
static atcmd_socket_rxd_t *g_atcmd_socket_rxd = NULL;
#endif

static char str_atcmd_socket_send_exit[32 + 1] = { 'A', 'T', '\r', '\n', '\0', };

static struct
{
	uint16_t event_send:1;
	uint16_t done_event:1;
	uint16_t error_verbose:1;
	uint16_t reserved:13;
} g_atcmd_socket_send_config =
{
	.event_send = 1,
	.done_event = 0,
	.error_verbose = 0,
};

static struct
{
	uint16_t verbose:1;
	uint16_t passive:1;
	uint16_t ready_event:1; /* 0:disable, 1/2: enable */
	uint16_t error_verbose:1;
	uint16_t reserved:12;

	uint16_t ready;
} g_atcmd_socket_recv_config =
{
	.verbose = 0,
	.passive = 0,
	.ready_event = 1,
	.error_verbose = 0,

	.ready = 0,
};

static struct
{
#define g_cmd_socket_log	g_cmd_socket.log
#define g_cmd_socket_send	g_cmd_socket.data[0]
#define g_cmd_socket_recv	g_cmd_socket.data[1]

	struct
	{
		uint16_t recv:1;
		uint16_t send:1;
		uint16_t send_done:1;
		uint16_t reserved:13;
	} log;

	struct
	{
		uint32_t cnt;
		uint32_t len;
	} data[2];
} g_cmd_socket =
{
	.log = { 0, 0, 0, 0 },
	.data = { { 0, 0 }, { 0 , 0} }
};

/**********************************************************************************************/

static void _atcmd_socket_print (atcmd_socket_t *socket)
{
	if (!socket || socket->id < 0)
		return;

	_atcmd_info("[ Socket Info ]");
	_atcmd_info(" - id : %d", socket->id);

	switch (socket->protocol)
	{
		case ATCMD_SOCKET_PROTO_UDP:
		case ATCMD_SOCKET_PROTO_TCP:
			_atcmd_info(" - protocol : %s", str_proto_lwr[socket->protocol]);
			break;

		default:
			_atcmd_info(" - protocol : unknown");
	}

	_atcmd_info(" - local_port : %d", socket->local_port);

	if (socket->remote_port > 0)
	{
		_atcmd_info(" - remote");
		_atcmd_info(" -- port : %d", socket->remote_port);
		_atcmd_info(" -- address : %s", ipaddr_ntoa(&socket->remote_addr));
	}
}

static void _atcmd_socket_reset (atcmd_socket_t *socket)
{
	memset(socket, 0, sizeof(atcmd_socket_t));

	socket->id = -1;
	socket->protocol = ATCMD_SOCKET_PROTO_NONE;
	socket->local_port = 0;
	socket->remote_port = 0;
	ip_addr_set_zero(&socket->remote_addr);
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

/*		_atcmd_debug("%s: id=%d proto=%d port=%d remote=%d,%s", __func__,
						socket->id, socket->protocol, socket->local_port,
						socket->remote_port, socket->remote_addr); */

		return socket;
	}

	return NULL;
}

#define _atcmd_socket_search_id(id)					_atcmd_socket_search(id, -1, 0)
#define _atcmd_socket_search_port(proto, port)		_atcmd_socket_search(-1, proto, port)

/**********************************************************************************************/

static int _atcmd_socket_event_handler (int type, int id, ...)
{
	va_list ap;
	int ret = 0;

	va_start(ap, id);

	switch (type)
	{
		case ATCMD_SOCKET_EVENT_CONNECT:
		{
			ip_addr_t *remote_addr = va_arg(ap, void *);
			uint32_t remote_port = va_arg(ap, uint32_t) & 0xffff;
			uint32_t local_port = va_arg(ap, uint32_t) & 0xffff;

			_atcmd_info("SEVENT: CONNECT, id=%d remote=%s,%u local=%u",
						id, ipaddr_ntoa(remote_addr), remote_port, local_port);
			ATCMD_MSG_SEVENT("\"CONNECT\",%d,\"%s\",%u,%u",
						id, ipaddr_ntoa(remote_addr), remote_port, local_port);
			break;
		}

		case ATCMD_SOCKET_EVENT_CLOSE:
		{
			int err = va_arg(ap, int);

			_atcmd_info("SEVENT: CLOSE, id=%d err=%d (%s)", id, -err, strerror(-err));
			ATCMD_MSG_SEVENT("\"CLOSE\",%d,%d,\"%s\"", id, -err, strerror(-err));
			break;
		}

		case ATCMD_SOCKET_EVENT_SEND_DONE:
		{
			uint32_t done = va_arg(ap, uint32_t);

			if (g_cmd_socket_log.send_done)
				_atcmd_info("SEVENT: SEND_DONE, id=%d done=%u", id, done);

			ATCMD_MSG_SEVENT("\"SEND_DONE\",%d,%u", id, done);
			break;
		}

		case ATCMD_SOCKET_EVENT_SEND_IDLE:
		{
			uint32_t done = va_arg(ap, uint32_t);
			uint32_t drop = va_arg(ap, uint32_t);
			uint32_t wait = va_arg(ap, uint32_t);
			uint32_t time = va_arg(ap, uint32_t);

			_atcmd_info("SEVENT: SEND_IDLE, id=%d done=%u drop=%u wait=%u time=%u", id, done, drop, wait, time);
			ATCMD_MSG_SEVENT("\"SEND_IDLE\",%d,%u,%u,%u", id, done, drop, wait);
			break;
		}

		case ATCMD_SOCKET_EVENT_SEND_DROP:
		{
			uint32_t drop = va_arg(ap, uint32_t);

			_atcmd_info("SEVENT: SEND_DROP, id=%d drop=%u", id, drop);
			ATCMD_MSG_SEVENT("\"SEND_DROP\",%d,%u", id, drop);
			break;
		}

		case ATCMD_SOCKET_EVENT_SEND_EXIT:
		{
			uint32_t done = va_arg(ap, uint32_t);
			uint32_t drop = va_arg(ap, uint32_t);

			_atcmd_info("SEVENT: SEND_EXIT, id=%d done=%u drop=%u", id, done, drop);
			ATCMD_MSG_SEVENT("\"SEND_EXIT\",%d,%u,%u", id, done, drop);
			break;
		}

		case ATCMD_SOCKET_EVENT_SEND_ERROR:
		{
			int err = va_arg(ap, int);

			_atcmd_info("SEVENT: SEND_ERROR, id=%d err=%d (%s)", id, -err, strerror(-err));
			ATCMD_MSG_SEVENT("\"SEND_ERROR\",%d,%d,\"%s\"", id, -err, strerror(-err));
			break;
		}

		case ATCMD_SOCKET_EVENT_RECV_READY:
		{
			int len = va_arg(ap, int);

			_atcmd_info("SEVENT: RECV_READY, id=%d len=%d", id, len);
			ATCMD_MSG_SEVENT("\"RECV_READY\",%d,%d", id, len);
			break;
		}

		case ATCMD_SOCKET_EVENT_RECV_ERROR:
		{
			int err = va_arg(ap, int);

			_atcmd_info("SEVENT: RECV_ERROR, id=%d err=%d (%s)", id, -err, strerror(-err));
			ATCMD_MSG_SEVENT("\"RECV_ERROR\",%d,%d,\"%s\"", id, -err, strerror(-err));
			break;
		}

		default:
			_atcmd_info("SEVENT: invalid type (%d)", type);
			ret = -1;
	}

	va_end(ap);

	return ret;
}

/**********************************************************************************************/

static struct
{
	atcmd_socket_t *socket;

	char *data;
	int len;
	int ret;
	int done;

	TaskHandle_t task;
} g_atcmd_socket_event_send =
{
	.socket = NULL,

	.data = NULL,
	.len = 0,
	.ret = 0,
	.done = 0,

	.task = NULL
};

static int _atcmd_socket_send_request (atcmd_socket_t *socket, char *data, int len, int *done)
{
	int ret = -EBUSY;

	if (g_atcmd_socket_event_send.socket)
		_atcmd_error("busy, id=%d", g_atcmd_socket_event_send.socket->id);
	else
	{
		g_atcmd_socket_event_send.socket = socket;
		g_atcmd_socket_event_send.data = data;
		g_atcmd_socket_event_send.len = len;
		g_atcmd_socket_event_send.ret = 0;
		g_atcmd_socket_event_send.done = 0;
		g_atcmd_socket_event_send.task = xTaskGetCurrentTaskHandle();

		ret = _lwip_socket_send_request(socket->id);
		if (ret == 0)
		{
			ret = g_atcmd_socket_event_send.ret;

			if (done)
				*done = g_atcmd_socket_event_send.done;
		}

		memset(&g_atcmd_socket_event_send, 0, sizeof(g_atcmd_socket_event_send));
	}

	return ret;
}

static int __atcmd_socket_send_data (atcmd_socket_t *socket, char *data, int len, int *done)
{
	const int retry_max = (60 * (1000 / 10));
	int retry = 0;
	int ret = 0;
	int i;

	for (i = 0 ; i < len ; )
	{
		if (socket->protocol == ATCMD_SOCKET_PROTO_TCP)
			ret = _lwip_socket_send_tcp(socket->id, data + i, len - i);
		else
		{
			ret = _lwip_socket_send_udp(socket->id,
						&socket->remote_addr, socket->remote_port, data + i, len - i);
		}

		if (ret > 0)
		{
			i += ret;
			ret = 0;

			_atcmd_socket_send("%s_send: id=%d len=%d/%d\n",
							str_proto_lwr[socket->protocol], socket->id, i, len);
			continue;
		}

		switch (ret)
		{
			case 0:
				ret = -ENOTCONN;

			case -EBADF:
			case -ENOTCONN:
			case -ECONNRESET:
			case -ECONNABORTED:
			case -EHOSTUNREACH:
				_atcmd_socket_close(socket->id, ret);
				break;

			case -ENOBUFS:
			case -EWOULDBLOCK:
				if (++retry < retry_max)
				{
					_delay_ms(10);
					continue;
				}

				_atcmd_info("%s_send: id=%d len=%d/%d retry=%d",
						str_proto_lwr[socket->protocol], socket->id, i, len, retry);

			default:
				_atcmd_socket_event_handler(ATCMD_SOCKET_EVENT_SEND_ERROR, socket->id, ret);
		}

		break;		
	}
		
/*	if (retry > 0 && retry < retry_max)
	{
		_atcmd_debug("%s_send: %d %d %d",
					str_proto_lwr[socket->protocol], socket->id, i, retry);
	} */

	if (i > 0)
	{
		g_cmd_socket_send.cnt++;
		g_cmd_socket_send.len += i;

		if (g_cmd_socket_log.send)
		{
			_atcmd_info("%s_send: id=%d len=%d (%u, %u)",
						str_proto_lwr[socket->protocol], socket->id, 
						len, g_cmd_socket_send.cnt, g_cmd_socket_send.len);
		}
		else
		{
			_atcmd_socket_send("%s_send: id=%d len=%d done\n", 
							str_proto_lwr[socket->protocol], socket->id, len);
		}
	}

	if (done)
		*done = i;

	return ret;
}

static int _atcmd_socket_send_data (atcmd_socket_t *socket, char *data, int len, int *done)
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
		ret = -EINVAL;
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
			ret = -EPROTOTYPE;
			goto socket_send_data_fail;
	}

	_socket = _atcmd_socket_search_id(socket->id);
	if (!_socket)
	{
		ret = -ENOTSOCK;
		goto socket_send_data_fail;
	}

	socket->protocol = _socket->protocol;
	socket->local_port = _socket->local_port;
	if (socket->remote_port == 0 && _socket->remote_port > 0)
	{
		socket->remote_port = _socket->remote_port;
		ip_addr_copy(socket->remote_addr, _socket->remote_addr);
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
						socket->remote_port, ipaddr_ntoa(&socket->remote_addr),
						cnt[socket->protocol]++);
	}

	if (g_atcmd_socket_send_config.event_send)
		ret = _atcmd_socket_send_request(socket, data, len, done);
	else
		ret = __atcmd_socket_send_data (socket, data, len, done);

socket_send_data_fail:

	if (ret < 0)
	{
		int err = -ret;

		if (_str_proto_lwr)
		{
			_atcmd_info("%s_send: id=%d len=%d/%d err=%d,%s",
					_str_proto_lwr, socket->id, ret, len, err, strerror(err));
		}
		else
		{
			_atcmd_info("socket_send: id=%d err=%d,%s", socket->id, err, strerror(err));
		}
	}

	return ret;
}

static void _atcmd_socket_send_handler (int id)
{
	if (!g_atcmd_socket_event_send.socket)
		_atcmd_error("no request, id=%d", id);
	else
	{
		atcmd_socket_t *socket = g_atcmd_socket_event_send.socket;

		if (id != socket->id)
			_atcmd_error("mismatch id, %d -> %d", socket->id, id);
		else
		{
			char *data = g_atcmd_socket_event_send.data;
			int len = g_atcmd_socket_event_send.len;
			int done = 0;
			int ret;

			ret = __atcmd_socket_send_data(socket, data, len, &done);

			g_atcmd_socket_event_send.socket = NULL;
			g_atcmd_socket_event_send.data = NULL;
			g_atcmd_socket_event_send.len = 0;
			g_atcmd_socket_event_send.ret = ret;
			g_atcmd_socket_event_send.done = done;

			_lwip_socket_send_done(id);
		}
	}
}

/**********************************************************************************************/

static void _atcmd_socket_recv_data (atcmd_socket_rxd_t *rxd)
{
	char msg[ATCMD_MSG_LEN_MAX + 1];
	char *buf;
	int len;

	_atcmd_socket_recv("%s_recv: id=%d remote=%s,%u len=%d\n",
						str_proto_lwr[rxd->socket.protocol], rxd->socket.id,
						ipaddr_ntoa(&rxd->socket.remote_addr), rxd->socket.remote_port,
						rxd->len.data);

	if (g_atcmd_socket_recv_config.verbose)
	{
		rxd->len.msg = ATCMD_MSG_SRXD(msg, sizeof(msg), "%d,%d,\"%s\",%u",
						rxd->socket.id, rxd->len.data,
						ipaddr_ntoa(&rxd->socket.remote_addr), rxd->socket.remote_port);
	}
	else
	{
		rxd->len.msg = ATCMD_MSG_SRXD(msg, sizeof(msg) - 1, "%d,%d", rxd->socket.id, rxd->len.data);
	}

	buf = rxd->buf.msg + sizeof(rxd->buf.msg) - rxd->len.msg;
	len = rxd->len.msg + rxd->len.data;

	memcpy(buf, msg, rxd->len.msg);

	atcmd_transmit(buf, len);
}

static int __atcmd_socket_recv_handler (int id, int len, bool passive)
{
	atcmd_socket_rxd_t *rxd = g_atcmd_socket_rxd;
	atcmd_socket_t *socket;
	int ret = 0;

	ASSERT(rxd);

	socket = _atcmd_socket_search_id(id);
	if (!socket)
	{
		_atcmd_error("invalid, id=%d", id);
		return -EINVAL;
	}

	if (len < 0)
	{
		_atcmd_error("invalild, len=%d", len);
		return -EINVAL;
	}

	if (len > sizeof(rxd->buf.data))
		len = sizeof(rxd->buf.data);

	memcpy(&rxd->socket, socket, sizeof(atcmd_socket_t));
	socket = &rxd->socket;

	switch (socket->protocol)
	{
		case ATCMD_SOCKET_PROTO_UDP:
			ret = _lwip_socket_recv(id, &socket->remote_addr, &socket->remote_port,
										rxd->buf.data, len);
			break;

		case ATCMD_SOCKET_PROTO_TCP:
			ret = _lwip_socket_recv(id, NULL, NULL, rxd->buf.data, len);
			break;

		default:
			_atcmd_error("invalid, protocol=%d", socket->protocol);
			ret = -EPROTOTYPE;
	}

	if (ret > 0)
	{
		g_cmd_socket_recv.cnt++;
		g_cmd_socket_recv.len += ret;

		if (g_cmd_socket_log.recv)
			_atcmd_info("%s_recv: id=%d len=%d (%u, %u)",
					str_proto_lwr[socket->protocol], id, ret, g_cmd_socket_recv.cnt, g_cmd_socket_recv.len);

		rxd->len.data = ret;

		if (passive)
			ATCMD_MSG_RETURN(NULL, ATCMD_SUCCESS);

		_atcmd_socket_recv_data(rxd);
	}
	else if (ret < 0 && _atcmd_socket_search_id(id))
	{
		if (ret == -EAGAIN)
			ret = 0;
		else
		{
			switch (ret)
			{
				case -EBADF:
				case -ENOTCONN:
				case -ECONNRESET:
				case -ECONNABORTED:
				case -EHOSTUNREACH:
					_atcmd_socket_close(id, ret);
					break;

				default:
					_atcmd_socket_event_handler(ATCMD_SOCKET_EVENT_RECV_ERROR, id, ret);
			}
		}
	}

	return ret;
}

static void _atcmd_socket_recv_handler (int id)
{
/*	_atcmd_debug("recv_handler: %d, passive=%d event=%d ready=0x%02X", id,
					g_atcmd_socket_recv_config.passive,
					g_atcmd_socket_recv_config.ready_event,
					g_atcmd_socket_recv_config.ready); */

	if (id < 0 || id >= ATCMD_SOCKET_NUM_MAX)
		_atcmd_error("invalid, id=%d", id);
	else
	{
		int len = _lwip_socket_recv_len(id);
		int ret = 0;
		int i;

		if (len == 0)
			ret = __atcmd_socket_recv_handler(id, 0, false);
		else if (len > 0)
		{
			if (g_atcmd_socket_recv_config.passive)
			{
/*				_atcmd_debug("recv_ready: %d, %d", id, len); */

				if (len > 0)
				{
					_lwip_socket_recv_done(id);

					if (!(g_atcmd_socket_recv_config.ready & (1 << id)))
					{
						g_atcmd_socket_recv_config.ready |= (1 << id);

						if (g_atcmd_socket_recv_config.ready_event)
							_atcmd_socket_event_handler(ATCMD_SOCKET_EVENT_RECV_READY, id, len);
					}

					return;
				}
			}

			for (i = 0 ; i < len ; i += ret)
			{
				if ((len - i) >= ATCMD_DATA_LEN_MAX)
					ret = __atcmd_socket_recv_handler(id, ATCMD_DATA_LEN_MAX, false);
				else
					ret = __atcmd_socket_recv_handler(id, len - i, false);

				if (ret < 0)
					break;
			}
		}
	}
}

static void _atcmd_socket_tcp_connect_handler (int id, ip_addr_t *remote_addr, uint16_t remote_port)
{
	atcmd_socket_t *socket;
	uint16_t local_port;
	int i;

	_atcmd_info("tcp_connect: id=%d remote=%s,%u",
				id, remote_addr ? ipaddr_ntoa(remote_addr) : "0.0.0.0", remote_port);

	if (id < 0 || !remote_addr || !remote_port)
		return;

	if (_lwip_socket_get_local(id, NULL, &local_port) != 0)
		local_port = 0;

	for (i = 0 ; i < ATCMD_SOCKET_NUM_MAX ; i++)
	{
		socket = &g_atcmd_socket[i];

		if (socket->id < 0)
		{
			socket->id = id;
			socket->protocol = ATCMD_SOCKET_PROTO_TCP;
			socket->local_port = local_port;
			socket->remote_port = remote_port;
			ip_addr_copy(socket->remote_addr, *remote_addr);

			_atcmd_socket_print(socket);
			_atcmd_socket_event_handler(ATCMD_SOCKET_EVENT_CONNECT, id, remote_addr, remote_port, local_port);
			return;
		}
	}

	_atcmd_info("tcp_connect: id=%d socket_full", id);
}

static int _atcmd_socket_init (void)
{
	lwip_socket_cb_t cb;
	int i;

	memset(&cb, 0, sizeof(lwip_socket_cb_t));

	_atcmd_info("%s_SOCKET_SEND", g_atcmd_socket_send_config.event_send ? "EVENT" : "DIRECT");

	cb.send_ready = _atcmd_socket_send_handler;
	cb.recv_ready = _atcmd_socket_recv_handler;
	cb.tcp_connect = _atcmd_socket_tcp_connect_handler;

	for (i = 0 ; i < ATCMD_SOCKET_NUM_MAX ; i++)
		_atcmd_socket_reset(&g_atcmd_socket[i]);

	return _lwip_socket_init(&cb);
}

static int _atcmd_socket_deinit (void)
{
	return _lwip_socket_deinit();
}

static int _atcmd_socket_open (atcmd_socket_t *socket, bool ipv6, bool reuse_addr)
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
			_atcmd_info("%s_%s: existing socket", str_proto_lwr[socket->protocol], (ipv6 ? "open6" : "open"));
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
		_atcmd_error("socket full");

		return -EPERM;
	}
	else
	{
		int id = -1;
		int ret = -EINVAL;

		if (socket->protocol == ATCMD_SOCKET_PROTO_UDP)
			ret = _lwip_socket_open_udp(&id, socket->local_port, ipv6, reuse_addr);
		else if (socket->local_port > 0 && socket->remote_port == 0)
			ret = _lwip_socket_open_tcp_server(&id, socket->local_port, ipv6, reuse_addr);
		else if (socket->local_port == 0 && socket->remote_port > 0)
		{
			uint32_t timeout_msec;

			if (ipv6)
				timeout_msec = _atcmd_timeout_value("SOPEN6");
		   	else
				timeout_msec = _atcmd_timeout_value("SOPEN");

			if (timeout_msec == 0)
				timeout_msec = 30 * 1000;

			ret = _lwip_socket_open_tcp_client(&id, &socket->remote_addr, socket->remote_port,
												timeout_msec, ipv6, reuse_addr);
			if (ret == 0)
			{
				if (_lwip_socket_get_local(id, NULL, &socket->local_port) != 0)
					socket->local_port = 0;
			}
		}

		socket->id = id;

		_atcmd_info("%s_%s: id=%d port=%u remote=%u,%s err=%d,%s",
					str_proto_lwr[socket->protocol], (ipv6 ? "open6" : "open"),
					socket->id, socket->local_port,
					socket->remote_port, ipaddr_ntoa(&socket->remote_addr),
					ret, strerror(ret));

		if (socket->id < 0)
			return ret;

		memcpy(&g_atcmd_socket[i], socket, sizeof(atcmd_socket_t));

		_atcmd_socket_print(&g_atcmd_socket[i]);

		ATCMD_MSG_INFO("%s", "%d", (ipv6 ? "SOPEN6" : "SOPEN"), socket->id);

		return 0;
	}
}

static int _atcmd_socket_close (int id, int err)
{
	atcmd_socket_t *socket;
	int ret = 0;
	int i;

	for (i = 0 ; i < ATCMD_SOCKET_NUM_MAX ; i++)
	{
		socket = &g_atcmd_socket[i];

		if (socket->id < 0)
		   continue;

		if (id < 0 || id == socket->id)
		{
			ret = _lwip_socket_close(socket->id);

			_atcmd_info("%s_close: id=%d port=%d err=%d,%s",
					str_proto_lwr[socket->protocol], socket->id, socket->local_port,
					ret, strerror(ret));

			if (err < 0)
				_atcmd_socket_event_handler(ATCMD_SOCKET_EVENT_CLOSE, socket->id, err);
			else
				ATCMD_MSG_INFO("SCLOSE", "%d", socket->id);

			if (g_atcmd_socket_recv_config.passive)
			{
				g_atcmd_socket_recv_config.ready &= ~(1 << socket->id);

				_atcmd_debug("passive_recv: %d, event=%d ready=0x%02X", socket->id,
									g_atcmd_socket_recv_config.ready_event,
									g_atcmd_socket_recv_config.ready);
			}

			_atcmd_socket_reset(socket);
		}
	}

	return 0;
}

/**********************************************************************************************/

static int __atcmd_socket_open_set (int argc, char *argv[], enum ATCMD_SOCKET_PROTO proto, bool ipv6)
{
	char *param_reuse_addr = NULL;
	bool reuse_addr = false;
	atcmd_socket_t socket;
	int ret;

	_atcmd_socket_reset(&socket);

	socket.protocol = proto;

	if (ipv6)
		IP_SET_TYPE(&socket.remote_addr, IPADDR_TYPE_V6);
	else
		IP_SET_TYPE(&socket.remote_addr, IPADDR_TYPE_V4);

	switch (argc)
	{
		case 3:
			param_reuse_addr = argv[2];

		case 2: /* tcp client */
		{
			char str_server_addr[ATCMD_MSG_LEN_MAX];
			char *param_server_addr = argv[0];
			char *param_server_port = argv[1];

			if (atcmd_param_to_str(param_server_addr, str_server_addr, sizeof(str_server_addr)))
			{
				if (!ipaddr_aton(str_server_addr, &socket.remote_addr))
				{
					char addr[ATCMD_STR_SIZE(ATCMD_IPADDR_LEN_MAX)];
					int ret;

					ret = _lwip_socket_addr_info_2(str_server_addr, param_server_port, addr, sizeof(addr));
					if (ret != 0)
						return ret;

					if (!ipaddr_aton(addr, &socket.remote_addr))
						return -EINVAL;
				}

				if (!ipv6 && IP_GET_TYPE(&socket.remote_addr) != IPADDR_TYPE_V4)
				{
					_atcmd_info("invalid address: ipv4");
					return -EINVAL;
				}
				else if (ipv6 && IP_GET_TYPE(&socket.remote_addr) != IPADDR_TYPE_V6)
				{
					_atcmd_info("invalid address: ipv6");
					return -EINVAL;
				}

				socket.remote_port = atoi(param_server_port);
				if (!_atcmd_socket_valid_port(socket.remote_port))
					return -EINVAL;

				break;
			}
			else if (param_reuse_addr)
				return -EINVAL;
			else
				param_reuse_addr = argv[1];
		}

		case 1: /* tcp server, udp server/client */
		{
			char *param_local_port = argv[0];

			socket.local_port = atoi(param_local_port);
			if (!_atcmd_socket_valid_port(socket.local_port))
				return -EINVAL;

			break;
		}

		default:
			return -EINVAL;
	}

	if (param_reuse_addr)
	{
		switch (atoi(param_reuse_addr))
		{
			case 0:
				reuse_addr = false;
				break;

			case 1:
				reuse_addr = true;
				break;

			default:
				return -EINVAL;
		}
	}

	ret = _atcmd_socket_open(&socket, ipv6, reuse_addr);
	if (ret < 0)
		_atcmd_info("socket_open: ipv6=%d reuse_addr=%d ret=%d", ipv6, reuse_addr, ret);

	return ret;
}

static int _atcmd_socket_open_set (int argc, char *argv[])
{
	bool ipv6 = false;
	char *param_protocol = NULL;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+SOPEN=\"udp\",<local_port>[,<reuse_addr>]");
			ATCMD_MSG_HELP("AT+SOPEN=\"tcp\",<local_port>[,<reuse_addr>]");
			ATCMD_MSG_HELP("AT+SOPEN=\"tcp\",\"<server_address>\",<server_port>[,<reuse_addr>]");
			break;

#ifdef CONFIG_ATCMD_IPV6
		case 7:
		case 6:
		case 5:
			argc -= 3;
			ipv6 = true;
#endif
		case 4:
		case 3:
		case 2:
		{
			char str_protocol[ATCMD_STR_SIZE(3)];

			param_protocol = argv[0];

			if (atcmd_param_to_str(param_protocol, str_protocol, sizeof(str_protocol)))
			{
				enum ATCMD_SOCKET_PROTO proto;

				if (strcmp(str_protocol, "udp") == 0 || strcmp(str_protocol, "UDP") == 0)
					proto = ATCMD_SOCKET_PROTO_UDP;
				else if (strcmp(str_protocol, "tcp") == 0 || strcmp(str_protocol, "TCP") == 0)
					proto = ATCMD_SOCKET_PROTO_TCP;
				else
					return ATCMD_ERROR_INVAL;

				switch (__atcmd_socket_open_set(argc - 1, argv + 1, proto, ipv6))
				{
					case 0:
						return ATCMD_SUCCESS;

					case -EINVAL:
						return ATCMD_ERROR_INVAL;

					case -EINPROGRESS:
						return ATCMD_ERROR_BUSY;

					default:
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

#ifdef CONFIG_ATCMD_IPV6

static int _atcmd_socket_open6_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+SOPEN6=\"udp\",<local_port>[,<reuse_addr>]");
			ATCMD_MSG_HELP("AT+SOPEN6=\"tcp\",<local_port>[,<reuse_addr>]");
			ATCMD_MSG_HELP("AT+SOPEN6=\"tcp\",\"<server_address>\",server_port>[,<reuse_addr>]");
			break;

		case 4:
		case 3:
		case 2:
			return _atcmd_socket_open_set(argc + 3, argv);

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_socket_open6 =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_SOCKET,

	.cmd = "OPEN6",
	.id = ATCMD_SOCKET_OPEN6,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = NULL,
	.handler[ATCMD_HANDLER_SET] = _atcmd_socket_open6_set,
};

#endif /* #ifdef CONFIG_ATCMD_IPV6 */

/**********************************************************************************************/

static int _atcmd_socket_close_run (int argc, char *argv[])
{
	return _atcmd_socket_close(-1, 0);
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
				return _atcmd_socket_close(id, 0);
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
	char param_remote_addr[ATCMD_STR_PARAM_SIZE(ATCMD_SOCKET_IPADDR_LEN_MAX)];
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
			!atcmd_str_to_param(ipaddr_ntoa(&socket->remote_addr), param_remote_addr, sizeof(param_remote_addr)))
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

#if defined(CONFIG_ATCMD_UART) || defined(CONFIG_ATCMD_UART_HFC)
extern bool g_atcmd_uart_passthrough_support;
#endif

static int _atcmd_socket_send_set (int argc, char *argv[])
{
	char *param_len = NULL;
	char *param_remote_ip = NULL;
	char *param_remote_port = NULL;
	char *param_done_event = NULL;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+SSEND=<socket_ID>[,<length>[,<done_event>]]"); /* tcp */
			ATCMD_MSG_HELP("AT+SSEND=<socket_ID>,\"<remote_ip>\",<remote_port>[,<length>[,<done_event]]"); /* udp */
			break;

		case 5:
			param_done_event = argv[4];

		case 4:
			param_len = argv[3];

		case 3:
			param_remote_ip = argv[1];
			param_remote_port = argv[2];

			if (argc == 3)
				param_done_event = argv[2];

		case 2:
			if (argc <= 3)
				param_len = argv[1];

		case 1:
		{
			int id = atoi(argv[0]);
			atcmd_socket_t *socket = _atcmd_socket_search_id(id);

			if (socket)
			{
				int32_t len = 0;
				bool done_event = false;
				char *exit_cmd = NULL;
				uint32_t timeout_msec;

				if (argc >= 3)
				{
					atcmd_socket_ipaddr_t str_remote_ip;
					int str_remote_ip_len;
					ip_addr_t remote_ip;
					int32_t remote_port;

					if (atcmd_param_to_str(param_remote_ip, str_remote_ip, sizeof(str_remote_ip)))
					{
						str_remote_ip_len = strlen(str_remote_ip);
						if (str_remote_ip_len < ATCMD_SOCKET_IPADDR_LEN_MIN ||
							str_remote_ip_len > ATCMD_SOCKET_IPADDR_LEN_MAX)
							return ATCMD_ERROR_INVAL;

						ipaddr_aton(str_remote_ip, &remote_ip);
						if (IP_GET_TYPE(&socket->remote_addr) != IP_GET_TYPE(&remote_ip))
							return ATCMD_ERROR_INVAL;

						remote_port = atoi(param_remote_port);
						if (!_atcmd_socket_valid_port(remote_port))
							return ATCMD_ERROR_INVAL;

						ip_addr_copy(socket->remote_addr, remote_ip);
						socket->remote_port = remote_port;

						if (argc == 3)
						{
							param_len = NULL;
							param_done_event = NULL;
						}
					}
					else if (argc > 3)
						return ATCMD_ERROR_INVAL;
				}

				if (param_len)
				{
					len = atoi(param_len);
					if (abs(len) > ATCMD_DATA_LEN_MAX)
						return ATCMD_ERROR_INVAL;
				}

				if (!param_done_event)
					done_event = !!g_atcmd_socket_send_config.done_event;
				else
				{
					switch (atoi(param_done_event))
					{
						case 0: done_event = false; break;
						case 1: done_event = true; break;
						default:
							return ATCMD_ERROR_INVAL;
					}
				}

#if defined(CONFIG_ATCMD_UART) || defined(CONFIG_ATCMD_UART_HFC)
				if (len <= 0 && _hif_get_type() == _HIF_TYPE_UART)
				{
					if (!g_atcmd_uart_passthrough_support)
						return ATCMD_ERROR_NOTSUPP;
				}
#endif

				if (strlen(str_atcmd_socket_send_exit) > 0)
				   exit_cmd = str_atcmd_socket_send_exit;

				timeout_msec = _atcmd_timeout_value("SSEND");
				if (timeout_msec == 0)
					timeout_msec = 1000;

/*				_atcmd_debug("ssend: id=%d remote=%s,%d len=%d done_event=%d timeout_msec=%u",
								socket->id, ipaddr_ntoa(&socket->remote_addr), socket->remote_port,
								len, done_event, timeout_msec); */

				if (atcmd_data_mode_enable(socket, len, done_event, timeout_msec, exit_cmd)  != 0)
					return ATCMD_ERROR_FAIL;

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

static int _atcmd_socket_recv_get (int argc, char *argv[])
{
	if (!g_atcmd_socket_recv_config.passive)
		return ATCMD_ERROR_NOTSUPP;

	switch (argc)
	{
		case 1:
		{
			int id = atoi(argv[1]);
			int len;

			if (id < 0 || _atcmd_socket_search_id(id) == NULL)
				return ATCMD_ERROR_INVAL;

			len = _lwip_socket_recv_len(id);
			if (len < 0)
				return ATCMD_ERROR_FAIL;

			ATCMD_MSG_INFO("SRECV", "%d,%d", id, len);
			break;
		}

		case 0:
		{
			atcmd_socket_t *socket;
			int len[ATCMD_SOCKET_NUM_MAX];
			int ret;
			int i;

			for (i = 0 ; i < ATCMD_SOCKET_NUM_MAX ; i++)
			{
				len[i] = 0;

				socket = &g_atcmd_socket[i];
				if (socket->id < 0)
					continue;

				ret = _lwip_socket_recv_len(socket->id);
				if (ret < 0)
					return ATCMD_ERROR_FAIL;

				if (socket->id < ATCMD_SOCKET_NUM_MAX)
					len[socket->id] = ret;
			}

			for (i = 0 ; i < ATCMD_SOCKET_NUM_MAX ; i++)
			{
				if (len[i] > 0)
					ATCMD_MSG_INFO("SRECV", "%d,%d", i, len[i]);
			}
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;

}

static int _atcmd_socket_recv_set (int argc, char *argv[])
{
	char *param_length = NULL;

	if (!g_atcmd_socket_recv_config.passive)
		return ATCMD_ERROR_NOTSUPP;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+SRECV=<socket_ID>[,<length>]");
			break;

		case 2:
			param_length = argv[1];

		case 1:
		{
			int id = atoi(argv[0]);

			if (id >= 0 && id < ATCMD_SOCKET_NUM_MAX)
			{
				int len = param_length ? atoi(param_length) : ATCMD_DATA_LEN_MAX;

				if (len > 0)
				{
					int ret;

					ret = __atcmd_socket_recv_handler(id, len, true);

					_atcmd_info("SRECV: id=%d len=%d ret=%d", id, len, ret);

					if (g_atcmd_socket_recv_config.ready & (1 << id))
					{
						g_atcmd_socket_recv_config.ready &= ~(1 << id);
						_lwip_socket_recv_request(id);
					}

					if (ret >= 0)
						return ATCMD_NO_RETURN;

					return ATCMD_ERROR_FAIL;
				}
			}
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
	.handler[ATCMD_HANDLER_GET] = _atcmd_socket_recv_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_socket_recv_set,
};

/**********************************************************************************************/

static int _atcmd_socket_recv_mode_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_INFO("SRECVMODE", "%d,%d",
					g_atcmd_socket_recv_config.passive, g_atcmd_socket_recv_config.ready_event);
			break;

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_socket_recv_mode_set (int argc, char *argv[])
{
	char *param_event = NULL;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+SRECVMODE=<mode>[,<ready_event>]");
			/*
			 * mode : 0=active, 1=passive
			 * ready_event: 0=disable, 1=enable
			 */
			break;

		case 2:
			param_event = argv[1];

		case 1:
		{
			int mode = atoi(argv[0]);
			int event = 0;

			if (param_event)
				event = atoi(param_event);

			if (mode == 0 || mode == 1)
			{
				if (event == 0 || event == 1)
				{
					g_atcmd_socket_recv_config.passive = mode;
					g_atcmd_socket_recv_config.ready_event = event;
					g_atcmd_socket_recv_config.ready = 0;
					break;
				}
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

	.cmd = "RECVMODE",
	.id = ATCMD_SOCKET_RECV_MODE,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_socket_recv_mode_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_socket_recv_mode_set,
};

/**********************************************************************************************/

static int _atcmd_socket_recv_info_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_INFO("SRECVINFO", "%d", g_atcmd_socket_recv_config.verbose);
			break;

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_socket_recv_info_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			/*
			 * 0 : terse
			 * 1 : verbose
			 */
			ATCMD_MSG_HELP("AT+SRECVINFO={0|1}");
			break;

		case 1:
		{
			int mode = atoi(argv[0]);

			if (mode == 0 || mode == 1)
			{
				g_atcmd_socket_recv_config.verbose = mode;
				break;
			}
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_socket_recv_info =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_SOCKET,

	.cmd = "RECVINFO",
	.id = ATCMD_SOCKET_RECV_INFO,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_socket_recv_info_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_socket_recv_info_set,
};

/**********************************************************************************************/

static int _atcmd_socket_recv_log_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_INFO("SRXLOGLEVEL", "%d", g_atcmd_socket_recv_config.verbose);
			break;

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_socket_recv_log_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+SRXLOGLEVEL=<mode>"); /* 0:terse 1:verbose */
			break;

		case 1:
			return _atcmd_socket_recv_info_set(argc, argv);

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

static int _atcmd_socket_addr_info_set (int argc, char *argv[])
{
	char *param_name = NULL;
	char *param_port = NULL;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+SADDRINFO=\"<domain_name>\"");
/*			ATCMD_MSG_HELP("AT+SADDRINFO=\"<domain_name>\"[,<port>]"); */
			break;

		case 2:
			param_port = argv[1];

		case 1:
		{
			char str_name[ATCMD_MSG_LEN_MAX];
			char str_addr[ATCMD_STR_SIZE(ATCMD_IP4_ADDR_LEN_MAX)];

			param_name = argv[0];

			if (!atcmd_param_to_str(param_name, str_name, sizeof(str_name)))
				return ATCMD_ERROR_INVAL;

			if (!param_port)
			{
				if (_lwip_socket_addr_info_1(str_name, str_addr, sizeof(str_addr)) != 0)
					return ATCMD_ERROR_FAIL;
			}
			else
			{
				uint16_t port ;

				if (atcmd_param_to_uint16(param_port, &port) != 0 || !_atcmd_socket_valid_port(port))
					return ATCMD_ERROR_INVAL;

				if (_lwip_socket_addr_info_2(str_name, param_port, str_addr, sizeof(str_addr)) != 0)
					return ATCMD_ERROR_FAIL;
			}

			ATCMD_MSG_INFO("SADDRINFO", "\"%s\"", str_addr);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_socket_addr_info =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_SOCKET,

	.cmd = "ADDRINFO",
	.id = ATCMD_SOCKET_ADDR_INFO,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = NULL,
	.handler[ATCMD_HANDLER_SET] = _atcmd_socket_addr_info_set,
};

/**********************************************************************************************/

static int _atcmd_socket_tcp_keepalive_get (int argc, char *argv[])
{
	atcmd_socket_t *socket = NULL;
	int keepalive, keepidle, keepcnt, keepintvl;

	switch (argc)
	{
		case 0:
		{
			int i;

			for (i = 0 ; i < ATCMD_SOCKET_NUM_MAX ; i++)
			{
				socket = &g_atcmd_socket[i];

				if (socket->id < 0)
					continue;

				if (socket->protocol != ATCMD_SOCKET_PROTO_TCP || socket->remote_port == 0) /* UDP or TCP server */
					continue;

				_lwip_socket_tcp_get_keepalive(socket->id, &keepalive, &keepidle, &keepcnt, &keepintvl);

				_atcmd_info("tcp_keepalive_get: id=%d keepalive=%d keepidle=%d keepcnt=%d keepintvl=%d",
							socket->id, keepalive, keepidle, keepcnt, keepintvl);

				ATCMD_MSG_INFO("STCPKEEPALIVE", "%d,%d,%d,%d,%d", socket->id, keepalive, keepidle, keepcnt, keepintvl);
			}

			break;
		}

		case 1:
		{
			int id = atoi(argv[0]);

			if (id >= 0)
			{
				socket = _atcmd_socket_search_id(id);

				if (socket && socket->protocol == ATCMD_SOCKET_PROTO_TCP && socket->remote_port != 0) /* TCP client */
				{
					if (_lwip_socket_tcp_get_keepalive(id, &keepalive, &keepidle, &keepcnt, &keepintvl) != 0)
						return ATCMD_ERROR_FAIL;

					_atcmd_info("tcp_keepalive_get: id=%d keepalive=%d keepidle=%d keepcnt=%d keepintvl=%d",
								socket->id, keepalive, keepidle, keepcnt, keepintvl);

					ATCMD_MSG_INFO("STCPKEEPALIVE", "%d,%d,%d,%d,%d", socket->id, keepalive, keepidle, keepcnt, keepintvl);

					break;
				}
			}
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_socket_tcp_keepalive_set (int argc, char *argv[])
{
	int32_t param_keepidle = -1;
	int32_t param_keepcnt = -1;
   	int32_t param_keepintvl = -1;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+STCPKEEPALIVE=<socket_ID>,{0|1}[,<keepidle>,<keepcnt>,<keepintvl>]");
			break;

		case 5:
			if (atcmd_param_to_int32(argv[2], &param_keepidle) != 0 || param_keepidle < 0)
				return ATCMD_ERROR_INVAL;

			if (atcmd_param_to_int32(argv[3], &param_keepcnt) != 0 || param_keepcnt < 0)
				return ATCMD_ERROR_INVAL;

			if (atcmd_param_to_int32(argv[4], &param_keepintvl) != 0 || param_keepintvl < 0)
				return ATCMD_ERROR_INVAL;

		case 2:
		{
			int id = atoi(argv[0]);
			int keepalive = atoi(argv[1]);

			if (id >= 0 && (keepalive == 0 || keepalive == 1))
			{
				atcmd_socket_t *socket  = _atcmd_socket_search_id(id);
				int keepidle, keepcnt, keepintvl;

				if (socket && socket->protocol == ATCMD_SOCKET_PROTO_TCP && socket->remote_port != 0) /* TCP client */
				{
					if (_lwip_socket_tcp_get_keepalive(id, NULL, &keepidle, &keepcnt, &keepintvl) != 0)
						return ATCMD_ERROR_FAIL;

					if (param_keepidle >= 0)
						keepidle = param_keepidle;

					if (param_keepcnt >= 0)
						keepcnt = param_keepcnt;

					if (param_keepintvl >= 0)
						keepintvl = param_keepintvl;

					_atcmd_info("tcp_keepalive_set: id=%d keepalive=%d keepidle=%d keepcnt=%d keepintvl=%d",
								id, keepalive, keepidle, keepcnt, keepintvl);

					if (_lwip_socket_tcp_set_keepalive(id, keepalive, keepidle, keepcnt, keepintvl) != 0)
						return ATCMD_ERROR_FAIL;

					break;
				}
			}
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_socket_tcp_keepalive =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_SOCKET,

	.cmd = "TCPKEEPALIVE",
	.id = ATCMD_SOCKET_TCP_KEEPALIVE,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_socket_tcp_keepalive_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_socket_tcp_keepalive_set,
};

/**********************************************************************************************/

static int _atcmd_socket_tcp_nodelay_get (int argc, char *argv[])
{
	atcmd_socket_t *socket = NULL;
	bool enabled = false;

	switch (argc)
	{
		case 0:
		{
			int i;

			for (i = 0 ; i < ATCMD_SOCKET_NUM_MAX ; i++)
			{
				socket = &g_atcmd_socket[i];

				if (socket->id < 0)
					continue;

				if (socket->protocol != ATCMD_SOCKET_PROTO_TCP || socket->remote_port == 0) /* UDP or TCP server */
					continue;

				if (_lwip_socket_tcp_get_nodelay(socket->id, &enabled) == 0)
				{
					_atcmd_info("tcp_nodelay_get: id=%d nodelay=%d", socket->id, enabled);

					ATCMD_MSG_INFO("STCPNODELAY", "%d,%d", socket->id, enabled ? 1 : 0);
				}
				else
				{
					_atcmd_info("tcp_nodelay_get: id=%d ?", socket->id);

					ATCMD_MSG_INFO("STCPNODELAY", "%d,?", socket->id);
				}
			}

			break;
		}

		case 1:
		{
			int id = atoi(argv[0]);

			if (id >= 0)
			{
				socket = _atcmd_socket_search_id(id);

				if (socket && socket->protocol == ATCMD_SOCKET_PROTO_TCP && socket->remote_port != 0) /* TCP client */
				{
					if (_lwip_socket_tcp_get_nodelay(socket->id, &enabled) != 0)
						return ATCMD_ERROR_FAIL;

					_atcmd_info("tcp_nodelay_get: id=%d nodelay=%d", socket->id, enabled);

					ATCMD_MSG_INFO("STCPNODELAY", "%d,%d", id, enabled ? 1 : 0);
					break;
				}
			}
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_socket_tcp_nodelay_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+STCPNODELAY=<socket_ID>,{0|1}");
			break;

		case 2:
		{
			int id = atoi(argv[0]);

			if (id >= 0)
			{
				atcmd_socket_t *socket  = _atcmd_socket_search_id(id);

				if (socket && socket->protocol == ATCMD_SOCKET_PROTO_TCP && socket->remote_port != 0) /* TCP client */
				{
					int enable = atoi(argv[1]);

					if (enable == 0 || enable == 1)
					{
						_atcmd_info("tcp_nodelay_set: id=%d nodelay=%d", socket->id, enable);

						if (_lwip_socket_tcp_set_nodelay(socket->id, !!enable) != 0)
							return ATCMD_ERROR_FAIL;

						break;
					}
				}
			}
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_socket_tcp_nodelay =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_SOCKET,

	.cmd = "TCPNODELAY",
	.id = ATCMD_SOCKET_TCP_NODELAY,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_socket_tcp_nodelay_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_socket_tcp_nodelay_set,
};

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

#if defined(CONFIG_ATCMD_SOCKET_INTERNAL)

static int _atcmd_socket_send_mode_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_INFO("SSENDMODE", "%d", g_atcmd_socket_send_config.event_send ? 1 : 0);
			break;

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_socket_send_mode_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			/*
			 * 0 : direct send
			 * 1 : event send
			 */
			ATCMD_MSG_HELP("AT+SSENDMODE={0|1}");
			break;

		case 1:
		{
			int mode = atoi(argv[0]);

			if (mode == 0 || mode == 1)
			{
				_atcmd_info("send_mode : %s", mode ? "event" : "direct");

				g_atcmd_socket_send_config.event_send = mode;
				break;
			}
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_socket_send_mode =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_SOCKET,

	.cmd = "SENDMODE",
	.id = ATCMD_SOCKET_SEND_MODE,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_socket_send_mode_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_socket_send_mode_set,
};

#endif /* #if defined(CONFIG_ATCMD_SOCKET_INTERNAL) */

/**********************************************************************************************/

#if defined(CONFIG_ATCMD_SOCKET_INTERNAL)

static int _atcmd_socket_send_done_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_INFO("SSENDDONE", "%d", g_atcmd_socket_send_config.done_event ? 1 : 0);
			break;

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_socket_send_done_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			/*
			 * 0 : disable
			 * 1 : enable
			 */
			ATCMD_MSG_HELP("AT+SSENDDONE={0|1}");
			break;

		case 1:
		{
			int param = atoi(argv[0]);

			if (param == 0 || param == 1)
			{
				g_atcmd_socket_send_config.done_event = param;
				break;
			}
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_socket_send_done =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_SOCKET,

	.cmd = "SENDDONE",
	.id = ATCMD_SOCKET_SEND_DONE,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_socket_send_done_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_socket_send_done_set,
};

#endif /* #if defined(CONFIG_ATCMD_SOCKET_INTERNAL) */

/**********************************************************************************************/

#if defined(CONFIG_ATCMD_SOCKET_INTERNAL)

static int _atcmd_socket_send_exit_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			if (strcmp(str_atcmd_socket_send_exit, "AT\r\n") == 0)
			{
				ATCMD_MSG_INFO("SSENDEXIT", "\"\"");
				break;
			}
			else if (memcmp(str_atcmd_socket_send_exit, "AT+", 3) == 0)
			{
				char *exit_cmd = str_atcmd_socket_send_exit + 3;
				int len = strlen(str_atcmd_socket_send_exit) - 3;

				exit_cmd[len - 2] = '\0';

				ATCMD_MSG_INFO("SSENDEXIT", "\"%s\"", exit_cmd);

				exit_cmd[len - 2] = '\r';
				break;
			}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_socket_send_exit_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+SSENDEXIT=\"<cmd>\"");
			break;

		case 1:
		{
			char exit_cmd[sizeof(str_atcmd_socket_send_exit)];

			if (strcmp(argv[0], "\"\"") == 0)
			{
				strcpy(str_atcmd_socket_send_exit, "AT\r\n");
				break;
			}
			else if (atcmd_param_to_str(argv[0], exit_cmd, sizeof(exit_cmd)))
			{
				int len = strlen(exit_cmd);

				if (len > 0 && len <= 32)
				{
					strcpy(str_atcmd_socket_send_exit, "AT+");
					strcpy(str_atcmd_socket_send_exit + 3, exit_cmd);
					strcpy(str_atcmd_socket_send_exit + 3 + len, "\r\n");
					break;
				}
			}
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_socket_send_exit =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_SOCKET,

	.cmd = "SENDEXIT",
	.id = ATCMD_SOCKET_SEND_EXIT,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_socket_send_exit_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_socket_send_exit_set,
};

#endif /* #if defined(CONFIG_ATCMD_SOCKET_INTERNAL) */

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
#ifdef CONFIG_ATCMD_IPV6
	&g_atcmd_socket_open6,
#endif
	&g_atcmd_socket_close,
	&g_atcmd_socket_list,
	&g_atcmd_socket_send,
	&g_atcmd_socket_recv,
	&g_atcmd_socket_recv_mode,
	&g_atcmd_socket_recv_info,
	&g_atcmd_socket_recv_log,
	&g_atcmd_socket_addr_info,
	&g_atcmd_socket_tcp_keepalive,
	&g_atcmd_socket_tcp_nodelay,
	&g_atcmd_socket_timeout,

	/*
	 * Command for internal
	 */
#if defined(CONFIG_ATCMD_SOCKET_INTERNAL)
	&g_atcmd_socket_send_mode,
	&g_atcmd_socket_send_done,
	&g_atcmd_socket_send_exit,
#endif

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
	g_atcmd_socket_rxd = _atcmd_malloc(sizeof(atcmd_socket_rxd_t));

	if (!g_atcmd_socket || !g_atcmd_socket_rxd)
	{
		_atcmd_error("malloc()");

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

	atcmd_info_print(&g_atcmd_group_socket);

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

int atcmd_socket_send_data (atcmd_socket_t *socket, char *data, int len)
{
	int done;

	_atcmd_socket_send_data(socket, data, len, &done);

	return done;
}

int atcmd_socket_event_send_done (int id, uint32_t done)
{
	return _atcmd_socket_event_handler(ATCMD_SOCKET_EVENT_SEND_DONE, id, done);
}

int atcmd_socket_event_send_idle (int id, uint32_t done, uint32_t drop, uint32_t wait, uint32_t time)
{
	return _atcmd_socket_event_handler(ATCMD_SOCKET_EVENT_SEND_IDLE, id, done, drop, wait, time);
}

int atcmd_socket_event_send_drop (int id, uint32_t drop)
{
	return _atcmd_socket_event_handler(ATCMD_SOCKET_EVENT_SEND_DROP, id, drop);
}

int atcmd_socket_event_send_exit (int id, uint32_t done, uint32_t drop)
{
	return _atcmd_socket_event_handler(ATCMD_SOCKET_EVENT_SEND_EXIT, id, done, drop);
}

/**********************************************************************************************/

#if defined(CONFIG_ATCMD_CLI)

static int cmd_atcmd_socket_log (cmd_tbl_t *t, int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			_atcmd_printf("recv: %d\n", g_cmd_socket_log.recv);
			_atcmd_printf("send: %d\n", g_cmd_socket_log.send);
			_atcmd_printf("send_done: %d\n", g_cmd_socket_log.send_done);
			break;

		case 2:
		{
			int log = atoi(argv[1]);

			if (log == 0 || log == 1)
			{
				if (strcmp(argv[0], "all") == 0)
				{
					g_cmd_socket_log.send = log;
					g_cmd_socket_log.send_done = log;
					g_cmd_socket_log.recv = log;
					break;
				}
				else if (strcmp(argv[0], "recv") == 0)
				{
					g_cmd_socket_log.recv = log;
					break;
				}
				else if (strcmp(argv[0], "send") == 0)
				{
					g_cmd_socket_log.send = log;
					break;
				}
				else if (strcmp(argv[0], "send_done") == 0)
				{
					g_cmd_socket_log.send_done = log;
					break;
				}
			}
		}

		default:
			return CMD_RET_USAGE;
	}

	return CMD_RET_SUCCESS;
}

static int cmd_atcmd_socket_data (cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_SUCCESS;
	int i;

	switch (argc)
	{
		case 1:
			if (strcmp(argv[0], "clear") != 0)
			{
				ret = CMD_RET_USAGE;
				break;
			}

			for (i = 0 ; i < 2 ; i++)
			{
				g_cmd_socket.data[i].cnt = 0;
				g_cmd_socket.data[i].len = 0;
			}

		case 0:
			for (i = 0 ; i < 2 ; i++)
			{
				_atcmd_printf(" - %s: cnt=%u len=%u\n", (i == 0) ? "send" : "recv",
					g_cmd_socket.data[i].cnt, g_cmd_socket.data[i].len);
			}

			if (g_atcmd_socket_recv_config.passive)
			{
				_atcmd_printf(" -- passive: event=%d, ready=0x%02X\n",
					g_atcmd_socket_recv_config.ready_event, g_atcmd_socket_recv_config.ready);
			}
			break;

		default:
			ret = CMD_RET_USAGE;
	}

	return ret;
}

static int cmd_atcmd_socket (cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_USAGE;

	if (argc >= 2)
	{
		if (strcmp(argv[1], "log") == 0)
			ret = cmd_atcmd_socket_log(t, argc - 2, argv + 2);
		else if (strcmp(argv[1], "data") == 0)
			ret = cmd_atcmd_socket_data(t, argc - 2, argv + 2);
		else if (strcmp(argv[1], "help") == 0)
		{
			_atcmd_printf("atcmd socket log {all|recv|send|send_done} {0|1}\n");
			_atcmd_printf("atcmd socket data [{clear}]\n");

			return CMD_RET_SUCCESS;
		}
	}

	return ret;
}

SUBCMD_MAND(atcmd,
		socket,
		cmd_atcmd_socket,
		"atcmd_socket",
		"atcmd socket help");

#endif /* #if defined(CONFIG_ATCMD_CLI) */
