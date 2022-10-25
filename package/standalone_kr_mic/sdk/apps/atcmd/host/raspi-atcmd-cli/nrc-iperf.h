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

#ifndef __NRC_IPERF_UDP_H__
#define __NRC_IPERF_UDP_H__
/**********************************************************************************************/

#include <getopt.h>
#include <arpa/inet.h>

#include "common.h"

/**********************************************************************************************/

#define IPERF_IPADDR_LEN_MIN	STR_IP4ADDR_LEN_MIN
#define IPERF_IPADDR_LEN_MAX	STR_IP4ADDR_LEN_MAX

#define IPERF_IPADDR_ANY		"0.0.0.0"

enum IPERF_PROTO
{
	IPERF_PROTO_NONE = -1,
	IPERF_PROTO_UDP = 0,
	IPERF_PROTO_TCP,

	IPERF_PROTO_NUM
};

typedef struct
{
	int16_t id;
	int16_t protocol;
	uint16_t local_port;
	uint16_t remote_port;
	char remote_addr[IPERF_IPADDR_LEN_MAX + 1];
} iperf_socket_t;

/**********************************************************************************************/

#define IPERF_DEFAULT_SERVER_PORT			5001
#define IPERF_DEFAULT_CLIENT_PORT			50000
#define IPERF_DEFAULT_SEND_TIME				10
#define IPERF_DEFAULT_REPORT_INTERVAL		1

#define IPERF_DEFAULT_UDP_RATE				(1024 * 1024) /* bps */
#define IPERF_DEFAULT_UDP_DATAGRAM_SIZE		1470 /* byte */

#define IPERF_DEFAULT_TCP_DATA_SIZE			1440 /* byte, TCP_MSS in NRC_MACSW/lib/lwip/port/lwipopts.h */

#define IPERF_FLAG_HDR_VER1				0x80000000

typedef double	iperf_time_t; /* usec */

typedef struct
{
#define IPERF_SERVER_HDR0_SIZE		(4 * 10)
#define IPERF_SERVER_HDR1_SIZE		(4 * (10 + 15))

	int32_t flags;
	int32_t total_len1;
	int32_t total_len2;
	int32_t stop_sec;
	int32_t stop_usec;
	int32_t error_cnt;
	int32_t outorder_cnt;
	int32_t datagrams;
	int32_t jitter1;
	int32_t jitter2;

	/* header version 1 */
	int32_t minTransit1;
	int32_t minTransit2;
	int32_t maxTransit1;
	int32_t maxTransit2;
	int32_t sumTransit1;
	int32_t sumTransit2;
	int32_t meanTransit1;
	int32_t meanTransit2;
	int32_t m2Transit1;
	int32_t m2Transit2;
	int32_t vdTransit1;
	int32_t vdTransit2;
	int32_t cntTransit;
	int32_t IPGcnt;
	int32_t IPGsum;
} iperf_server_header_t;

typedef struct
{
#define IPERF_CLIENT_HDR0_SIZE		(4 * 6)
#define IPERF_CLIENT_HDR1_SIZE		(4 * (6 + 3))

	int32_t flags;
	int32_t numThreads;
	int32_t mPort;
	int32_t bufferlen;
	int32_t mWindowSize;
	int32_t mAmount; 		/* -: -t option (sec), +: -n option (byte) */

	/* header version 1 */
	int32_t mRate; 			/* -b option (bits/sec) */
	int32_t mUDPRateUnits; 	/* 0:BW, 1:PPS (Don't use) */
	int32_t mRealTime; 		/* Don't care */
} iperf_client_header_t;

typedef struct
{
	int32_t id;
	uint32_t tv_sec;
	uint32_t tv_usec;

	union
	{
		iperf_server_header_t server_header;
		iperf_client_header_t client_header;
	};
} iperf_udp_datagram_t;

typedef struct
{
	union
	{
		iperf_server_header_t server_header;
		iperf_client_header_t client_header;
	};
} iperf_tcp_datagram_t;

typedef struct
{
	iperf_socket_t client;

	bool start;
	bool done;

	iperf_time_t send_time;
	int32_t send_byte;
	uint32_t recv_byte;
	int32_t datagram_cnt;
	int32_t datagram_seq;
	int32_t error_cnt;
	int32_t outoforder_cnt;

	iperf_time_t jitter;
	iperf_time_t transit;

	iperf_time_t start_time;
	iperf_time_t stop_time;

	iperf_udp_datagram_t last_datagram;

	/* options */
	uint16_t server_port;
	int32_t report_interval;
} iperf_server_info_t;

typedef struct
{
	bool done;
	int32_t datagram_cnt;

	/* options */
	uint16_t server_port;
	int32_t report_interval;
	int32_t datagram_size;
	int32_t send_time;
} iperf_client_info_t;

typedef iperf_server_info_t iperf_udp_server_info_t;
typedef iperf_client_info_t iperf_udp_client_info_t;

typedef iperf_server_info_t iperf_tcp_server_info_t;
typedef iperf_client_info_t iperf_tcp_client_info_t;

typedef struct
{
	/* Client/Server */
	int report_interval; /* -i, sec */
	int server_port; /* -p */
	bool udp; /* -u */

	/* Server */
	bool server; /* -s */

	/* Client */
	char server_ip[IPERF_IPADDR_LEN_MAX + 1]; /* -c */
	int send_time; /* -t , sec */
	bool passthrough; /* -P */
	bool negative; /* -N */

	int datagram_size;
} iperf_opt_t;

extern int iperf_main (char *cmd);

/**********************************************************************************************/
#endif /* #ifndef __NRC_IPERF_UDP_H__ */

