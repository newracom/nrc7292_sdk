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


#ifndef __LWIP_PING_H__
#define __LWIP_PING_H__
/**********************************************************************************************/

#include "lwip_common.h"


#define _lwip_ping_malloc					pvPortMalloc
#define _lwip_ping_free						vPortFree

#define _lwip_ping_delay					_delay_ms

#define _lwip_ping_printf(fmt, ...)			hal_uart_printf("[PING] " fmt, ##__VA_ARGS__)

#define _lwip_ping_error(fmt, ...)			_lwip_ping_printf(fmt, ##__VA_ARGS__)
#define _lwip_ping_debug(fmt, ...)			/* _lwip_ping_printf(fmt, ##__VA_ARGS__) */
#define _lwip_ping_log(fmt, ...)			/* _lwip_ping_printf(fmt, ##__VA_ARGS__) */

/**********************************************************************************************/

typedef enum
{
	_LWIP_PING_SUCCESS = 0,
	_LWIP_PING_SEND_FAIL,
	_LWIP_PING_RECV_TIMEOUT
} _lwip_ping_status_t;

typedef struct
{
	_lwip_ping_status_t status;

	ip_addr_t remote_ip;
	uint16_t data_size;
	uint16_t icmp_seq;
	uint8_t ttl;
	uint32_t resp_time; /* msec */
} _lwip_ping_report_t;

typedef void (*_lwip_ping_report_cb_t)(_lwip_ping_report_t *report);

typedef struct _lwip_ping_params
{
	ip_addr_t remote_ip;
	uint32_t count;
	uint16_t interval; /* msec */
	uint16_t data_size;

	_lwip_ping_report_cb_t report_cb;
} _lwip_ping_params_t;

typedef struct
{
	char *buf;
	uint16_t buf_size;
	uint16_t data_size;

	ip_addr_t remote_ip;
	uint16_t id;
	uint16_t icmp_seq;
	uint32_t time; /* msec */
} _lwip_ping_request_t;

typedef struct
{
	char *buf;
	uint16_t buf_size;

	ip_addr_t remote_ip;
	uint16_t data_size;
	uint16_t icmp_seq;
	uint8_t ttl;
	uint32_t time; /* msec */
} _lwip_ping_reply_t;

typedef struct
{
#ifdef CONFIG_ATCMD_IPV6
	bool ipv6;
#endif
	int socket;

	_lwip_ping_params_t params;
	_lwip_ping_request_t request;
	_lwip_ping_reply_t reply;
	_lwip_ping_status_t status;

	TaskHandle_t task;
} _lwip_ping_info_t;


extern int _lwip_ping_start (_lwip_ping_params_t *params);
/* extern void _lwip_ping_stop (_lwip_ping_info_t *info); */

extern int _lwip_ping_params_init (_lwip_ping_params_t *params, bool ipv6);

/**********************************************************************************************/
#endif /* #ifndef __LWIP_PING_H__ */

