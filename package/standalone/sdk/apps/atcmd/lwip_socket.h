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

#ifndef __LWIP_SOCKET_H__
#define __LWIP_SOCKET_H__
/**********************************************************************************************/

#include "FreeRTOS.h"
#include "event_groups.h" /* FreeRTOS */

#define _lwip_socket_malloc				_atcmd_malloc
#define _lwip_socket_mfree				_atcmd_free
#define _lwip_socket_log(fmt, ...)		_atcmd_log(fmt, ##__VA_ARGS__)

/**********************************************************************************************/

#define LWIP_SOCKET_NUM_MAX				MEMP_NUM_NETCONN /* 12, lib/lwip/contrib/port/lwipopts.h */

#if configUSE_16_BIT_TICKS
#define LWIP_SOCKET_EVENT_BIT_MAX		8
#else
#define LWIP_SOCKET_EVENT_BIT_MAX		24
#endif

#if LWIP_SOCKET_NUM_MAX > LWIP_SOCKET_EVENT_BIT_MAX
#error "LWIP_SOCKET_NUM_MAX > LWIP_SOCKET_EVENT_BIT_MAX"
#endif

#define LWIP_SOCKET_TASK_PRIORITY		ATCMD_TASK_PRIORITY
#define LWIP_SOCKET_TASK_STACK_SIZE		((4 * 1024) / sizeof(StackType_t))
#define LWIP_SOCKET_TASK_TIMEOUT		1000000 /* usec */

#define LWIP_SOCKET_FDS_MUTEX_TIMEOUT	portMAX_DELAY

/**********************************************************************************************/

typedef struct
{
	void (*send_ready) (int fd);
	void (*recv_ready) (int fd);
	void (*tcp_connect) (int fd, const char *remote_addr, uint16_t remote_port);
} lwip_socket_cb_t;

typedef struct
{
	TaskHandle_t task;
	EventGroupHandle_t event;

	struct
	{
		SemaphoreHandle_t mutex;
		StaticSemaphore_t mutex_buffer;

		fd_set read;
		fd_set write;
		fd_set listen;
		fd_set send;
	} fds;

	lwip_socket_cb_t cb;
} lwip_socket_info_t;

/**********************************************************************************************/

extern int _lwip_socket_init (lwip_socket_cb_t *cb);
extern int _lwip_socket_exit (void);

extern int _lwip_socket_open_udp (int *fd, uint16_t local_port);
extern int _lwip_socket_open_tcp_server (int *fd, uint16_t local_port);
extern int _lwip_socket_open_tcp_client (int *fd, const char *remote_addr, uint16_t remote_port,
										int timeout_msec);
extern int _lwip_socket_close (int fd);

extern int _lwip_socket_send_start (int fd);
extern int _lwip_socket_send_wait (int fd);
extern int _lwip_socket_send_stop (int fd);

extern int _lwip_socket_send (int fd, const char *remote_addr, uint16_t remote_port,
								char *data, int len);
extern int _lwip_socket_recv (int fd, char *remote_addr, uint16_t *remote_port,
								char *data, int len);

#define _lwip_socket_send_udp(fd, remote_addr, remote_port, data, len) \
								_lwip_socket_send(fd, remote_addr, remote_port, data, len)
#define _lwip_socket_send_tcp(fd, data, len) \
								_lwip_socket_send(fd, NULL, 0, data, len)

#define _lwip_socket_recv_udp(fd, remote_addr, remote_port, data, len) \
								_lwip_socket_recv(fd, remote_addr, remote_port, data, len)
#define _lwip_socket_recv_tcp(fd, data, len) \
								_lwip_socket_recv(fd, NULL, 0, data, len)

/**********************************************************************************************/
#endif /* #ifndef __LWIP_SOCKET_H__ */

