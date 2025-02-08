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

#ifndef __NRC_ATCMD_SOCKET_H__
#define __NRC_ATCMD_SOCKET_H__
/**********************************************************************************************/

#include "lwip_socket.h"


#if LWIP_SOCKET_NUM_MAX <= LWIP_SOCKET_EVENT_BIT_MAX
#define ATCMD_SOCKET_NUM_MAX	LWIP_SOCKET_NUM_MAX
#else
#define ATCMD_SOCKET_NUM_MAX	LWIP_SOCKET_EVENT_BIT_MAX
#endif

#define ATCMD_SOCKET_IPADDR_LEN_MIN		ATCMD_IPADDR_LEN_MIN
#define ATCMD_SOCKET_IPADDR_LEN_MAX		ATCMD_IPADDR_LEN_MAX

enum ATCMD_SOCKET_PROTO
{
	ATCMD_SOCKET_PROTO_NONE = -1,

	ATCMD_SOCKET_PROTO_UDP = 0,
	ATCMD_SOCKET_PROTO_TCP,

	ATCMD_SOCKET_PROTO_NUM,

	ATCMD_SOCKET_PROTO_ALL = ~0
};

enum ATCMD_SOCKET_EVENT
{
	ATCMD_SOCKET_EVENT_CONNECT = 0,
	ATCMD_SOCKET_EVENT_CLOSE,
	ATCMD_SOCKET_EVENT_SEND_DONE,
	ATCMD_SOCKET_EVENT_SEND_IDLE,
	ATCMD_SOCKET_EVENT_SEND_DROP,
	ATCMD_SOCKET_EVENT_SEND_EXIT,
	ATCMD_SOCKET_EVENT_SEND_ERROR,
	ATCMD_SOCKET_EVENT_RECV_READY,
	ATCMD_SOCKET_EVENT_RECV_ERROR,
};

typedef char atcmd_socket_ipaddr_t[ATCMD_SOCKET_IPADDR_LEN_MAX + 1];

typedef struct
{
	int16_t id;
	int16_t protocol;
	uint16_t local_port;
	uint16_t remote_port;
	ip_addr_t remote_addr;
} atcmd_socket_t;

typedef struct
{
	atcmd_socket_t socket;

	struct
	{
		int msg;
		int data;
	} len;

	struct
	{
		char msg[ATCMD_MSG_LEN_MAX];
		char data[ATCMD_TXBUF_SIZE];
	} buf;
} atcmd_socket_rxd_t;

/**********************************************************************************************/

#define ATCMD_MSG_SEVENT(fmt, ...)	\
		ATCMD_MSG_EVENT("SEVENT", fmt, ##__VA_ARGS__)

#define ATCMD_MSG_RXD(buf, len, fmt, ...)	\
		atcmd_msg_snprint(ATCMD_MSG_TYPE_EVENT, buf, len, "RXD:" fmt, ##__VA_ARGS__)

extern void atcmd_socket_reset (atcmd_socket_t *socket);
extern int atcmd_socket_enable (void);
extern void atcmd_socket_disable (void);
extern int atcmd_socket_send_data (int id, char *data, int len);

extern void atcmd_socket_event_send_done (int id, uint32_t done);
extern void atcmd_socket_event_send_idle (int id, uint32_t done, uint32_t drop, uint32_t wait, uint32_t time);
extern void atcmd_socket_event_send_drop (int id, uint32_t drop);
extern void atcmd_socket_event_send_exit (int id, uint32_t done, uint32_t drop);

/**********************************************************************************************/
#endif /* #ifndef __NRC_ATCMD_SOCKET_H__ */

