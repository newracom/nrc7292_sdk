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

#ifndef __LWIP_SOCKET_H__
#define __LWIP_SOCKET_H__
/**********************************************************************************************/

#include "lwip_common.h"


#define _lwip_socket_malloc				_atcmd_malloc
#define _lwip_socket_mfree				_atcmd_free
#define _lwip_socket_log(fmt, ...)		_atcmd_info(fmt, ##__VA_ARGS__)

/**********************************************************************************************/

/*
 * lib/FreeRTOS/Source/include/event_groups.h
 *
 * The type that holds event bits always matches TickType_t - therefore the
 * number of bits it holds is set by configUSE_16_BIT_TICKS (16 bits if set to 1,
 * 32 bits if set to 0.
 *
 * Although event groups are not related to ticks, for internal implementation
 * reasons the number of bits available for use in an event group is dependent
 * on the configUSE_16_BIT_TICKS setting in FreeRTOSConfig.h.  If
 * configUSE_16_BIT_TICKS is 1 then each event group contains 8 usable bits (bit
 * 0 to bit 7).  If configUSE_16_BIT_TICKS is set to 0 then each event group has
 * 24 usable bits (bit 0 to bit 23).  The EventBits_t type is used to store
 * event bits within an event group.
 */
#if configUSE_16_BIT_TICKS
#define LWIP_SOCKET_EVENT_BIT_MAX		(16 - 8)
#else
#define LWIP_SOCKET_EVENT_BIT_MAX		(32 - 8)
#endif

/*
 * lib/lwip/contrib/port/lwipopts.h 
 */
#define LWIP_SOCKET_NUM_MAX				MEMP_NUM_NETCONN 

#define LWIP_SOCKET_TASK_PRIORITY		ATCMD_TASK_PRIORITY
#define LWIP_SOCKET_TASK_STACK_SIZE		((4 * 1024) / sizeof(StackType_t))

/**********************************************************************************************/

typedef union
{
	struct sockaddr_in ip4;
#ifdef CONFIG_ATCMD_IPV6
	struct sockaddr_in6 ip6;
#endif
} lwip_sockaddr_in_t;

typedef struct
{
	void (*send_ready) (int fd);
	void (*recv_ready) (int fd);
	void (*tcp_connect) (int fd, ip_addr_t *remote_addr, uint16_t remote_port);
} lwip_socket_cb_t;

typedef struct
{
	TaskHandle_t task;

	lwip_socket_cb_t cb;

	EventGroupHandle_t send_done_event;

	struct
	{
		SemaphoreHandle_t mutex;

		fd_set read;
		fd_set write;
		fd_set listen;
	} fds;

	struct
	{
		uint32_t task;
	} timeout;

	struct
	{
		uint16_t task:1;
		uint16_t send:1;
		uint16_t recv:1;
		uint16_t reserved:13;
	} log;

	struct
	{
		char name[32];
		char addr[16];
	} addrinfo;
} lwip_socket_info_t;

/**********************************************************************************************/

extern int _lwip_socket_tcp_get_keepalive (int fd, int *keepalive, int *keepidle, int *keepcnt, int *keepintvl);
extern int _lwip_socket_tcp_set_keepalive (int fd, int keepalive, int keepidle, int keepcnt, int keepintvl);

extern int _lwip_socket_tcp_get_nodelay (int fd, bool *enabled);
extern int _lwip_socket_tcp_set_nodelay (int fd, bool enable);
extern int _lwip_socket_udp_get_broadcast (int fd, bool *enabled);
extern int _lwip_socket_udp_set_broadcast (int fd, bool enable);

extern int _lwip_socket_init (lwip_socket_cb_t *cb);
extern int _lwip_socket_deinit (void);

extern int _lwip_socket_open_udp (int *fd, uint16_t local_port, bool ipv6, bool reuse_addr);
extern int _lwip_socket_open_tcp_server (int *fd, uint16_t local_port, bool ipv6, bool reuse_addr);
extern int _lwip_socket_open_tcp_client (int *fd, ip_addr_t *remote_addr, uint16_t remote_port,
								int timeout_msec, bool ipv6, bool reuse_addr);
extern int _lwip_socket_close (int fd);

extern int _lwip_socket_get_peer (int fd, ip_addr_t *ipaddr, uint16_t *port);
extern int _lwip_socket_get_local (int fd, ip_addr_t *ipaddr, uint16_t *port);

extern int _lwip_socket_send_request (int fd, uint32_t timeout_ms);
extern int _lwip_socket_send_done (int fd);
extern int _lwip_socket_send_udp(int fd, ip_addr_t *remote_addr, uint16_t remote_port,
								char *data, int len, struct netif *netif);
extern int _lwip_socket_send_tcp(int fd, char *data, int len);

extern int _lwip_socket_recv_request (int fd);
extern int _lwip_socket_recv_done (int fd);
extern int _lwip_socket_recv_len (int fd);
extern int _lwip_socket_recv_udp(int fd, ip_addr_t *remote_addr, uint16_t *remote_port,
								char *data, int len);
extern int _lwip_socket_recv_tcp(int fd, char *data, int len);

extern int _lwip_socket_addr_info_1 (const char *host, char *addr, int addrlen);
extern int _lwip_socket_addr_info_2 (const char *host, const char *port, char *addr, int addrlen);

/**********************************************************************************************/
#endif /* #ifndef __LWIP_SOCKET_H__ */
