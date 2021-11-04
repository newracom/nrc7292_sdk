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
 * THE SODTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SODTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SODTWARE.
 *
 */


#include "atcmd.h"

#define _atcmd_host_info(fmt, ...)			_atcmd_info(fmt, ##__VA_ARGS__)
#define _atcmd_host_debug(fmt, ...)			//_atcmd_debug(fmt, ##__VA_ARGS__)

/**********************************************************************************************/

//#define CONFIG_ATCMD_HOST_STATIC
#ifdef CONFIG_ATCMD_HOST_STATIC
static atcmd_rxd_t _g_atcmd_host_rxd;
static atcmd_rxd_t *g_atcmd_host_rxd = &_g_atcmd_host_rxd;
#else
static atcmd_rxd_t *g_atcmd_host_rxd = NULL;
#endif

/**********************************************************************************************/

static int _atcmd_host_event_handler (int type, ...)
{
	va_list ap;
	int len = 0;
	int err = 0;
	int ret = 0;

	va_start(ap, type);

	switch (type)
	{
		case ATCMD_HOST_EVENT_SEND_IDLE:
			len = va_arg(ap, int);

			_atcmd_info("HEVENT: SEND_IDLE, len=%u\n", len);
			ATCMD_MSG_HEVENT("\"SEND_IDLE\",%u", len);
			break;

		case ATCMD_HOST_EVENT_SEND_DROP:
			len = va_arg(ap, int);

			_atcmd_info("HEVENT: SEND_DROP, len=%u\n", len);
			ATCMD_MSG_HEVENT("\"SEND_DROP\",%u", len);
			break;

		case ATCMD_HOST_EVENT_SEND_EXIT:
			len = va_arg(ap, int);

			_atcmd_info("HEVENT: SEND_EXIT, len=%u\n", len);
			ATCMD_MSG_HEVENT("\"SEND_EXIT\",%u", len);
			break;

		case ATCMD_HOST_EVENT_SEND_ERROR:
			len = va_arg(ap, int);
			err = va_arg(ap, int);

			_atcmd_info("HEVENT: SEND_ERROR, len=%u err=%d,%s\n", len, err, atcmd_strerror(err));
			ATCMD_MSG_HEVENT("\"SEND_ERROR\",%u,%d", len, err);
			break;

		default:
			_atcmd_info("HEVENT: invalid type (%d)\n", type);
			ret = -1;
	}

	va_end(ap);

	return ret;
}

#if 0
/*
 * lib/hostap/src/drivers/driver_nrc_tx.c
 */
extern int nrc_transmit_from_8023_mb(uint8_t vif_id, uint8_t **frames, const uint16_t len[], int n_frames);

#define __atcmd_host_send_data(addr, len, num)	nrc_transmit_from_8023_mb(0, addr, len, num)
#else
static int __atcmd_host_send_data (uint8_t **addr, const uint16_t *len, int num)
{
	static uint32_t cnt = 0;
	static uint32_t size = 0;
	int i;

	for (i = 0 ; i < num ; i++)
	{
		cnt++;
		size += len[i];

		_atcmd_host_info("HSEND: %d:%d - %u:%u\n", i, len[i], cnt, size);

		atcmd_host_recv_data((char *)addr[i], (int)len[i]);
	}

	return 0;
}
#endif

static int _atcmd_host_send_data (char *buf, int len, int *err)
{
	static struct
	{
		const int max;
		int cnt;

		uint16_t len[16];
		uint8_t *addr[16];
	} payload = { .max = 16, .cnt = 0, };

	static struct
	{
		int len;
		char buf[ATCMD_PACKET_LEN_MAX];
	} temp = { .len = 0, };

	atcmd_packet_t *packet = NULL;
	int packet_len = 0;
	int i = 0;

	_atcmd_host_debug("HSEND: %p %d\n", buf, len);

	*err = 0;
	payload.cnt = 0;

	if (temp.len > 0)
	{
		packet = (atcmd_packet_t *)temp.buf;

		for (i = 0 ; i < len ; )
		{
			_atcmd_host_debug("HSEND1: i=%d temp=%d\n", i, temp.len);

			if (temp.len < ATCMD_PACKET_START_SIZE)
			{
				if ((len - i) < (ATCMD_PACKET_START_SIZE - temp.len))
				{
					memcpy(temp.buf + temp.len, buf + i, len - i);
					temp.len += len - i;

					_atcmd_host_debug("HSEND1: ret1, temp=%d\n", temp.len);

					return len;
				}

				memcpy(temp.buf + temp.len, buf + i, ATCMD_PACKET_START_SIZE - temp.len);
				i += ATCMD_PACKET_START_SIZE - temp.len;
				temp.len = ATCMD_PACKET_START_SIZE;
			}

			if (memcmp(packet->start, ATCMD_PACKET_START, ATCMD_PACKET_START_SIZE) != 0)
			{
				_atcmd_host_info("HSEND1: invalid SOP\n");

				memcpy(temp.buf, temp.buf + 1, --temp.len);
				continue;
			}

			_atcmd_host_debug("HSEND1: start, %d %d\n", i, temp.len);

			if (temp.len < (ATCMD_PACKET_START_SIZE + ATCMD_PACKET_HEADER_SIZE))
			{
				if ((len - i) < ((ATCMD_PACKET_START_SIZE + ATCMD_PACKET_HEADER_SIZE) - temp.len))
				{
					memcpy(temp.buf + temp.len, buf + i, len - i);
					temp.len += len - i;

					_atcmd_host_debug("HSEND1: ret2, temp=%d\n", temp.len);

					return len;
				}

				memcpy(temp.buf + temp.len, buf + i, (ATCMD_PACKET_START_SIZE + ATCMD_PACKET_HEADER_SIZE) - temp.len);
				i += (ATCMD_PACKET_START_SIZE + ATCMD_PACKET_HEADER_SIZE) - temp.len;
				temp.len = ATCMD_PACKET_START_SIZE + ATCMD_PACKET_HEADER_SIZE;
			}

			_atcmd_host_debug("HSEND1: seq=%u len=%u\n", packet->seq, packet->len);

			if (packet->len > ATCMD_PACKET_PAYLOAD_LEN_MAX)
			{
				_atcmd_host_info("HSEND1: invalid length, %u\n", packet->len);

				continue;
			}

			packet_len = ATCMD_PACKET_LEN_MIN + packet->len;

			if ((len - i) < (packet_len - temp.len))
			{
				memcpy(temp.buf + temp.len, buf + i, len - i);
				temp.len += len - i;

				_atcmd_host_debug("HSEND1: ret3, temp=%d\n", temp.len);

				return len;
			}

			memcpy(temp.buf + temp.len, buf + i, packet_len - temp.len);
			i += packet_len - temp.len;

			if (memcmp(packet->payload + packet->len, ATCMD_PACKET_END, ATCMD_PACKET_END_SIZE) == 0)
			{
				_atcmd_host_debug(" - frame %d: seq=%u len=%u\n", payload.cnt, data->seq, data->len);

				payload.len[payload.cnt] = packet->len;
				payload.addr[payload.cnt] = (uint8_t *)packet->payload;
				payload.cnt++;
				break;
			}

			i -= packet->len + ATCMD_PACKET_END_SIZE;
			temp.len = 0;

			_atcmd_host_info("HSEND1: invalid EOP\n");
		}
	}

	for ( ; (len - i) > (ATCMD_PACKET_START_SIZE + ATCMD_PACKET_HEADER_SIZE) ; )
	{
		_atcmd_host_debug("HSEND2: i=%d cnt=%d\n", i, payload.cnt);

		packet = (atcmd_packet_t *)(buf + i);

		if (memcmp(packet->start, ATCMD_PACKET_START, ATCMD_PACKET_START_SIZE) != 0)
		{
			_atcmd_host_info("HSEND2: invalid SOP\n");

			i += ATCMD_PACKET_START_SIZE;
			continue;
		}

		_atcmd_host_debug("HSEND2: seq=%u len=%u\n", packet->seq, packet->len);

		if (packet->len > ATCMD_PACKET_PAYLOAD_LEN_MAX)
		{
			_atcmd_host_info("HSEND2: invalid length, %u\n", packet->len);

			i += ATCMD_PACKET_START_SIZE + ATCMD_PACKET_HEADER_SIZE;
			continue;
		}

		packet_len = ATCMD_PACKET_LEN_MIN + packet->len;

		if ((len - i) < packet_len)
		{
			_atcmd_host_debug("HSEND2: break\n");
			break;
		}

		if (memcmp(packet->payload + packet->len, ATCMD_PACKET_END, ATCMD_PACKET_END_SIZE) != 0)
		{
			_atcmd_host_info("HSEND2: invalid EOP\n");

			i -= packet->len + ATCMD_PACKET_END_SIZE;
			continue;
		}

		i += packet_len;

		_atcmd_host_debug(" - frame %d: seq=%u len=%u\n", payload.cnt, packet->seq, packet->len);

		payload.len[payload.cnt] = packet->len;
		payload.addr[payload.cnt] = (uint8_t *)packet->payload;

		if (++payload.cnt >= payload.max)
		{
			if (__atcmd_host_send_data(payload.addr, payload.len, payload.cnt) != 0)
				_atcmd_host_info("HSEND2: failed\n");

			payload.cnt = 0;

			if (temp.len > 0)
				temp.len = 0;
		}
	}

	_atcmd_host_debug("HSEND2: done, i=%d\n", i);

	if (payload.cnt > 0)
	{
		if (__atcmd_host_send_data(payload.addr, payload.len, payload.cnt) != 0)
			_atcmd_host_info("HSEND: failed\n");

		if (temp.len > 0)
			temp.len = 0;
	}

	if (i < len)
	{
		_atcmd_host_debug("HSEND: temp=(%d + %d)\n", temp.len, len - i);

		memcpy(&temp.buf, buf + i, len - i);
		temp.len += len - i;
		i = len;
	}

	_atcmd_host_debug("HSEND: exit, %d/%d\n", i, len);

	return i;
}

static void _atcmd_host_recv_data (atcmd_rxd_t *rxd)
{
	char msg[ATCMD_MSG_LEN_MAX + 1];
	char *buf;
	int len;

	rxd->len.msg = ATCMD_MSG_HRXD(msg, sizeof(msg) - 1, "%d", rxd->len.data);

	buf = rxd->buf.msg + sizeof(rxd->buf.msg) - rxd->len.msg;
	len = rxd->len.msg + rxd->len.data;

	memcpy(buf, msg, rxd->len.msg);

	atcmd_transmit(buf, len);
}

/**********************************************************************************************/

extern uint32_t _atcmd_timeout_value (const char *cmd);

static int _atcmd_host_send_run (int argc, char *argv[]) /* passthrough mode */
{
	if (_hif_get_type() == _HIF_TYPE_UART)
		return ATCMD_ERROR_NOTSUPP;

	atcmd_data_mode_enable(0, _atcmd_timeout_value("HSEND"));

	return ATCMD_SUCCESS;
}

static int _atcmd_host_send_set (int argc, char *argv[]) /* normal mode */
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+HSEND=<length>");
			break;

		case 1:
		{
			uint32_t timeout_msec;
			int len = atoi(argv[0]);

			if (len <= 0 || len > ATCMD_PACKET_LEN_MAX)
				return ATCMD_ERROR_INVAL;

			timeout_msec = _atcmd_timeout_value("HSEND");
			if (timeout_msec == 0)
				timeout_msec = 1000;

/*			_atcmd_debug("ssend: len=%d timeout_msec=%u\n", len, timeout_msec); */

			atcmd_data_mode_enable(ATCMD_PACKET_LEN_MIN + len, timeout_msec);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_host_send =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_HOST,

	.cmd = "SEND",
	.id = ATCMD_HOST_SEND,

	.handler[ATCMD_HANDLER_RUN] = _atcmd_host_send_run,
	.handler[ATCMD_HANDLER_GET] = NULL,
	.handler[ATCMD_HANDLER_SET] = _atcmd_host_send_set,
};

/**********************************************************************************************/

extern int _atcmd_basic_timeout_get (int argc, char *argv[]);
extern int _atcmd_basic_timeout_set (int argc, char *argv[]);

static int _atcmd_host_timeout_get (int argc, char *argv[])
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

static int _atcmd_host_timeout_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+HTIMEOUT=\"<command>\",<time>");
			break;

		case 2:
			return _atcmd_basic_timeout_set(argc, argv);

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_host_timeout =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_HOST,

	.cmd = "TIMEOUT",
	.id = ATCMD_HOST_TIMEOUT,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_host_timeout_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_host_timeout_set,
};

/**********************************************************************************************/

static atcmd_group_t g_atcmd_group_host =
{
	.list.next = NULL,
	.list.prev = NULL,

	.name ="HOST",
	.id = ATCMD_GROUP_HOST,

	.cmd_prefix = "H",
	.cmd_prefix_size = 1,

	.cmd_list_head.next = NULL,
	.cmd_list_head.prev = NULL,
};

static atcmd_info_t *g_atcmd_info_host[] =
{
	&g_atcmd_host_send,
	&g_atcmd_host_timeout,

	NULL
};

int atcmd_host_enable (void)
{
	int i;

#ifndef CONFIG_ATCMD_HOST_STATIC
	g_atcmd_host_rxd = _atcmd_malloc(sizeof(atcmd_rxd_t));

	if (!g_atcmd_host_rxd)
	{
		_atcmd_error("malloc() failed\n");
		return -1;
	}
#endif

	if (atcmd_group_register(&g_atcmd_group_host) != 0)
		return -1;

	for (i = 0 ; g_atcmd_info_host[i] ; i++)
	{
		if (atcmd_info_register(ATCMD_GROUP_HOST, g_atcmd_info_host[i]) != 0)
			return -1;
	}

#ifdef CONFIG_ATCMD_DEBUG
	atcmd_info_print(&g_atcmd_group_host);
#endif

	return 0;
}

void atcmd_host_disable (void)
{
	int i;

	for (i = 0 ; g_atcmd_info_host[i] ; i++)
		atcmd_info_unregister(ATCMD_GROUP_HOST, g_atcmd_info_host[i]->id);

	atcmd_group_unregister(ATCMD_GROUP_HOST);

	_atcmd_free(g_atcmd_host_rxd);
}

int atcmd_host_send_data (char *buf, int len, int *err)
{
	return _atcmd_host_send_data(buf, len, err);
}

void atcmd_host_recv_data (char *buf, int len)
{
	atcmd_rxd_t *rxd = g_atcmd_host_rxd;
	const int rxbuf_size = sizeof(rxd->buf.data);
	int i;

	for (i = 0 ; i < len ; i += rxbuf_size)
	{
		if ((len - i) < rxbuf_size)
		{
			memcpy(rxd->buf.data, buf + i, len - i);
			rxd->len.data = len - i;
		}
		else
		{
			memcpy(rxd->buf.data, buf + i, rxbuf_size);
			rxd->len.data = rxbuf_size;
		}

		_atcmd_host_recv_data(rxd);
	}
}

int atcmd_host_event_send_idle (int len)
{
	return _atcmd_host_event_handler(ATCMD_HOST_EVENT_SEND_IDLE, len);
}

int atcmd_host_event_send_drop (int len)
{
	return _atcmd_host_event_handler(ATCMD_HOST_EVENT_SEND_DROP, len);
}

int atcmd_host_event_send_exit (int len)
{
	return _atcmd_host_event_handler(ATCMD_HOST_EVENT_SEND_EXIT, len);
}

int atcmd_host_event_send_error (int len, int err)
{
	return _atcmd_host_event_handler(ATCMD_HOST_EVENT_SEND_ERROR, len, err);
}

