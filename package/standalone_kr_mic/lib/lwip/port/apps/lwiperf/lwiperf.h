/**
 * @file
 * lwIP iPerf server implementation
 */

/*
 * Copyright (c) 2014 Simon Goldschmidt
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Simon Goldschmidt
 *
 */
#ifndef LWIP_HDR_APPS_LWIPERF_H
#define LWIP_HDR_APPS_LWIPERF_H

#include "lwip/opt.h"
#include "lwip/ip_addr.h"
#include "arch/sys_arch.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LWIPERF_TCP_PORT_DEFAULT  5001
#ifdef NRC_LWIP
#define LWIPERF_UDP_PORT_DEFAULT  5001
#define LWIPERF_DEFAULT_DURATION     10 // (unit is seconds)
#define LWIPERF_UDP_DATA_SIZE 1470
#define LWIPERF_UDP_CLIENT_DATA_OFFSET 36

#define KILO 1000
#define MEGA ( KILO * KILO )
#define MILI_PER_SECOND 1000
#define BYTES_TO_BITS  8

enum lwiperf_udp_client_status
{
	LWIPERF_UDP_CLIENT_IDLE,
	LWIPERF_UDP_CLIENT_START,
	LWIPERF_UDP_CLIENT_RUNNING,
	LWIPERF_UDP_CLIENT_STOPPING,
	LWIPERF_UDP_CLIENT_STOP,
	LWIPERF_UDP_CLIENT_MAX
};

enum lwiperf_client_command_status {
	LWIPERF_CLIENT_COMMAND_STATUS_STOP,
	LWIPERF_CLIENT_COMMAND_STATUS_START,
	LWIPERF_CLIENT_COMMAND_STATUS_MAX,
};
#endif /* NRC_LWIP */

/** lwIPerf test results */
enum lwiperf_report_type
{
	/** The server side test is done */
	LWIPERF_TCP_DONE_SERVER,
	/** The client side test is done */
	LWIPERF_TCP_DONE_CLIENT,
	/** Local error lead to test abort */
	LWIPERF_TCP_ABORTED_LOCAL,
	/** Data check error lead to test abort */
	LWIPERF_TCP_ABORTED_LOCAL_DATAERROR,
	/** Transmit error lead to test abort */
	LWIPERF_TCP_ABORTED_LOCAL_TXERROR,
	/** Remote side aborted the test */
	LWIPERF_TCP_ABORTED_REMOTE
#ifdef NRC_LWIP
	/** The udp server side test is done */
	,LWIPERF_UDP_DONE_SERVER,
	/** The udp client side test is done */
	LWIPERF_UDP_DONE_CLIENT,
	/** The server side test is stopped */
	LWIPERF_TCP_STOP_SERVER,
	/** The udp server side test is stopped */
	LWIPERF_UDP_STOP_SERVER,
	/** Local error lead to test abort */
	LWIPERF_UDP_ABORTED_LOCAL,
	/** Data check error lead to test abort */
	LWIPERF_UDP_ABORTED_LOCAL_DATAERROR,
	/** Transmit error lead to test abort */
	LWIPERF_UDP_ABORTED_LOCAL_TXERROR,
	/** Remote side aborted the test */
	LWIPERF_UDP_ABORTED_REMOTE
#endif /* NRC_LWIP */
};

/** Control */
enum lwiperf_client_type
{
	/** Unidirectional tx only test */
	LWIPERF_CLIENT,
	/** Do a bidirectional test simultaneously */
	LWIPERF_DUAL,
	/** Do a bidirectional test individually */
	LWIPERF_TRADEOFF
};

/** Prototype of a report function that is called when a session is finished.
    This report function can show the test results.
    @param report_type contains the test result */
#ifdef NRC_LWIP
typedef void (*lwiperf_report_fn)(void *arg, enum lwiperf_report_type report_type,
	const ip_addr_t* local_addr, u16_t local_port, const ip_addr_t* remote_addr, u16_t remote_port,
	u32_t bytes_transferred, u32_t ms_duration, u32_t bandwidth_kbitpsec,
	u32_t total_packets, u32_t outoforder_packets, u32_t cnt_error);
#else
typedef void (*lwiperf_report_fn)(void *arg, enum lwiperf_report_type report_type,
	const ip_addr_t* local_addr, u16_t local_port, const ip_addr_t* remote_addr, u16_t remote_port,
	u32_t bytes_transferred, u32_t ms_duration, u32_t bandwidth_kbitpsec);
#endif

#ifdef NRC_LWIP
typedef struct _lwiperf_state_tcp lwiperf_state_tcp_t;
typedef struct _lwiperf_udp_server_conn lwiperf_udp_server_conn_t;

/** This is the Iperf settings struct sent from the client */
typedef struct _lwiperf_settings {
	#define LWIPERF_FLAGS_ANSWER_TEST 0x80000000
	#define LWIPERF_FLAGS_ANSWER_NOW  0x00000001
	u32_t flags;
	u32_t num_threads; /* unused for now */
	u32_t remote_port;
	u32_t buffer_len; /* unused for now */
	u32_t win_band; /* TCP window / UDP rate: unused for now */
	u32_t amount; /* pos. value: bytes?; neg. values: time (unit is 10ms: 1/100 second) */
} lwiperf_settings_t;

/** Basic connection handle */
struct _lwiperf_state_base;
typedef struct _lwiperf_state_base lwiperf_state_base_t;
struct _lwiperf_state_base {
	/* linked list */
	lwiperf_state_base_t *next;
	/* 1=tcp, 0=udp */
	u8_t tcp;
	/* 1=server, 0=client */
	u8_t server;
	/* master state used to abort sessions (e.g. listener, main client) */
	lwiperf_state_base_t *related_master_state;
#ifdef NRC_LWIP
	ip4_addr_t remote_addr;
	u32_t remote_port;
	ip4_addr_t local_addr;
	u32_t local_port;
	lwiperf_state_tcp_t* conn;
#endif /* NRC_LWIP */
};

/** Connection handle for a TCP iperf session */
struct _lwiperf_state_tcp {
	lwiperf_state_base_t base;
	struct tcp_pcb *server_pcb;
	struct tcp_pcb *conn_pcb;
	u32_t time_started;
	lwiperf_report_fn report_fn;
	void *report_arg;
	u8_t poll_count;
	u8_t next_num;
	/* 1=start server when client is closed */
	u8_t client_tradeoff_mode;
	u32_t bytes_transferred;
	lwiperf_settings_t settings;
	u8_t have_settings_buf;
	u8_t specific_remote;
	ip_addr_t remote_addr;
#ifdef NRC_LWIP
	struct udp_pcb* udp_conn_pcb;
	int udp_client_status;
	u32_t total_packets;
	u32_t outoforder_packets;
	u32_t cnt_error;
	u32_t time_ended;
	u32_t bandwidth;
	u32_t duration ;
	u32_t udp_data_length ;
	u16_t time_delay ;
	u8_t tos;
	u8_t force_stop;
	sys_thread_t iperf_thread;
#endif /* NRC_LWIP */
};

struct _lwiperf_udp_server_conn {
	ip4_addr_t local_ip, remote_ip;
	u16_t local_port, remote_port;
	u32_t bytes_received;
	u32_t time_started;
	u32_t time_ended;
	u32_t outoforder_packets;
	u32_t cnt_error;
	s32_t packet_count;
	u8_t session_on;
	lwiperf_udp_server_conn_t* next;
};

/** Parameters for iperf session */
typedef struct _iperf_parm {
	u8_t role ;
	u8_t mode ;
	u8_t tos ;
	u8_t client_type;
	u32_t bandwidth;
	u16_t port ;
	u32_t duration ;
	u32_t udp_data_length ;
	u8_t trigger ;
	ip4_addr_t addr;
}iperf_parm_t;
#endif /* NRC_LWIP */

void* lwiperf_start_tcp_server(const ip_addr_t* local_addr, u16_t local_port,
                               lwiperf_report_fn report_fn, void* report_arg);
void* lwiperf_start_tcp_server_default(lwiperf_report_fn report_fn, void* report_arg);
void* lwiperf_start_tcp_client(const ip_addr_t* remote_addr, u16_t remote_port,
                               enum lwiperf_client_type type,
                               lwiperf_report_fn report_fn, void* report_arg
#ifdef NRC_LWIP
                               ,u32_t duration, u8_t tos
#endif
);
void* lwiperf_start_tcp_client_default(const ip_addr_t* remote_addr,
                               lwiperf_report_fn report_fn, void* report_arg);

void  lwiperf_abort(void* lwiperf_session);

#ifdef NRC_LWIP
void lwiperf_tcp_server_close(lwiperf_state_tcp_t* conn);

void* lwiperf_start_udp_server(const ip_addr_t* local_addr, u16_t local_port,
  lwiperf_report_fn report_fn, void* report_arg);
void* lwiperf_start_udp_client(const ip_addr_t* remote_addr, u16_t remote_port,u32_t duration,
	lwiperf_report_fn report_fn, void* report_arg, u8_t tos, u32_t bandwidth, u32_t udp_data_length, enum lwiperf_client_type type);
void lwiperf_udp_server_close(lwiperf_state_tcp_t* conn);

lwiperf_state_tcp_t* lwiperf_get_session(iperf_parm_t* parm);
void lwiperf_list_display(void);

void lwiperf_mutex_init(void);
void lwiperf_mutex_lock(void);
void lwiperf_mutex_unlock(void);
#endif /* NRC_LWIP */

#ifdef __cplusplus
}
#endif

#endif /* LWIP_HDR_APPS_LWIPERF_H */
