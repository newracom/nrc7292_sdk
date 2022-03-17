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

#ifndef __NRC_ATCMD_SOCKET_H__
#define __NRC_ATCMD_SOCKET_H__
/**********************************************************************************************/

#define ATCMD_SOCKET_NUM_MAX			LWIP_SOCKET_NUM_MAX

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
	ATCMD_SOCKET_EVENT_SEND_IDLE,
	ATCMD_SOCKET_EVENT_SEND_DROP,
	ATCMD_SOCKET_EVENT_SEND_EXIT,
	ATCMD_SOCKET_EVENT_SEND_ERROR,
	ATCMD_SOCKET_EVENT_RECV_ERROR,
};

typedef struct
{
	int16_t id;
	int16_t protocol;
	uint16_t local_port;
	uint16_t remote_port;
	char remote_addr[ATCMD_IP4_ADDR_LEN_MAX + 1];
} atcmd_socket_t;

/**********************************************************************************************/

#define ATCMD_MSG_SEVENT(fmt, ...)	\
		ATCMD_MSG_EVENT("SEVENT", fmt, ##__VA_ARGS__)

#define ATCMD_MSG_SRXD(buf, len, fmt, ...)	\
		atcmd_msg_snprint(ATCMD_MSG_TYPE_EVENT, buf, len, "RXD:" fmt, ##__VA_ARGS__)

extern void atcmd_socket_reset (atcmd_socket_t *socket);
extern int atcmd_socket_enable (void);
extern void atcmd_socket_disable (void);
extern int atcmd_socket_send_data (atcmd_socket_t *socket, char *data, int len, int *err);

extern int atcmd_socket_event_send_idle (int id, uint32_t done, uint32_t drop, uint32_t wait);
extern int atcmd_socket_event_send_drop (int id, uint32_t drop);
extern int atcmd_socket_event_send_exit (int id, uint32_t done, uint32_t drop);
extern int atcmd_socket_event_send_error (int id, int err);

/**********************************************************************************************/
#endif /* #ifndef __NRC_ATCMD_SOCKET_H__ */

