/**
 * @file
 * lwIP iPerf server implementation
 */

/**
 * @defgroup iperf Iperf server
 * @ingroup apps
 *
 * This is a simple performance measuring client/server to check your bandwith using
 * iPerf2 on a PC as server/client.
 * It is currently a minimal implementation providing a TCP client/server only.
 *
 * @todo:
 * - implement UDP mode
 * - protect combined sessions handling (via 'related_master_state') against reallocation
 *   (this is a pointer address, currently, so if the same memory is allocated again,
 *    session pairs (tx/rx) can be confused on reallocation)
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
 */

#include "apps/lwiperf/lwiperf.h"
#include "nrc_wifi.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "lwip/sys.h"
#include "driver_nrc.h"
#include <string.h>

#if LWIP_IPERF

/** Specify the idle timeout (in seconds) after that the test fails */
#ifndef LWIPERF_TCP_MAX_IDLE_SEC
#define LWIPERF_TCP_MAX_IDLE_SEC    10U
#endif
#if LWIPERF_TCP_MAX_IDLE_SEC > 255
#error LWIPERF_TCP_MAX_IDLE_SEC must fit into an u8_t
#endif

/** Change this if you don't want to lwiperf to listen to any IP version */
#ifndef LWIPERF_SERVER_IP_TYPE
#define LWIPERF_SERVER_IP_TYPE      IPADDR_TYPE_ANY
#endif

/* File internal memory allocation (struct lwiperf_*): this defaults to
   the heap */
#ifndef LWIPERF_ALLOC
#ifdef NRC_LWIP
#define LWIPERF_ALLOC(type)         pvPortMalloc(sizeof(type))
#define LWIPERF_FREE(type, item)    vPortFree(item)
#else
#define LWIPERF_ALLOC(type)         mem_malloc(sizeof(type))
#define LWIPERF_FREE(type, item)    mem_free(item)
#endif
#endif

/** If this is 1, check that received data has the correct format */
#ifndef LWIPERF_CHECK_RX_DATA
#define LWIPERF_CHECK_RX_DATA       0
#endif

#if !defined(NRC_LWIP)    // Move to lwiperf.h
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
};

/** Connection handle for a TCP iperf session */
typedef struct _lwiperf_state_tcp {
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
	} lwiperf_state_tcp_t;
#else
static xSemaphoreHandle xMutex_iperf = NULL;
/** List of connected with udp server */
static lwiperf_udp_server_conn_t* lwiperf_all_udp_server_connections;
void lwiperf_display_server_connect(void *arg);
#endif

/** List of active iperf sessions */
static lwiperf_state_base_t *lwiperf_all_connections;
/** A const buffer to send from: we want to measure sending, not copying! */
static const u8_t lwiperf_txbuf_const[1600] = {
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
};

static err_t lwiperf_tcp_poll(void *arg, struct tcp_pcb *tpcb);
static void lwiperf_tcp_err(void *arg, err_t err);
static err_t lwiperf_start_tcp_server_impl(const ip_addr_t *local_addr, u16_t local_port,
                                           lwiperf_report_fn report_fn, void *report_arg,
                                           lwiperf_state_base_t *related_master_state, lwiperf_state_tcp_t **state);


/** Add an iperf session to the 'active' list */
static void
lwiperf_list_add(lwiperf_state_base_t *item)
{
	item->next = lwiperf_all_connections;
	lwiperf_all_connections = item;
}

/** Remove an iperf session from the 'active' list */
static void
lwiperf_list_remove(lwiperf_state_base_t *item)
{
	lwiperf_state_base_t *prev = NULL;
	lwiperf_state_base_t *iter;
	for (iter = lwiperf_all_connections; iter != NULL; prev = iter, iter = iter->next) {
		if (iter == item) {
			if (prev == NULL) {
				lwiperf_all_connections = iter->next;
			} else {
				prev->next = iter->next;
			}
			/* @debug: ensure this item is listed only once */
			for (iter = iter->next; iter != NULL; iter = iter->next) {
				LWIP_ASSERT("duplicate entry", iter != item);
			}
			break;
		}
	}
}

static lwiperf_state_base_t *
lwiperf_list_find(lwiperf_state_base_t *item)
{
	lwiperf_state_base_t *iter;
	for (iter = lwiperf_all_connections; iter != NULL; iter = iter->next) {
		if (iter == item) {
			return item;
		}
	}
	return NULL;
}

/** Call the report function of an iperf tcp session */
static void
lwip_tcp_conn_report(lwiperf_state_tcp_t *conn, enum lwiperf_report_type report_type)
{
	if ((conn != NULL) && (conn->report_fn != NULL)) {
		u32_t now, duration_ms, bandwidth_kbitpsec;
		now = sys_now();
#if defined(NRC_LWIP)
		if(conn->time_ended)
			now = conn->time_ended;
#endif
		duration_ms = now - conn->time_started;
		if (duration_ms == 0) {
			bandwidth_kbitpsec = 0;
		} else {
#if defined(NRC_LWIP)
			bandwidth_kbitpsec = (8U * (conn->bytes_transferred) / duration_ms) ;
#else
			bandwidth_kbitpsec = (conn->bytes_transferred / duration_ms) * 8U;
#endif
		}
		conn->report_fn(conn->report_arg, report_type,
#if defined(NRC_LWIP)
				&conn->base.local_addr, conn->base.local_port,
				&conn->base.remote_addr, conn->base.remote_port,
				conn->bytes_transferred, duration_ms, bandwidth_kbitpsec,
				conn->total_packets, conn->outoforder_packets, conn->cnt_error);
#else
				&conn->conn_pcb->local_ip, conn->conn_pcb->local_port,
				&conn->conn_pcb->remote_ip, conn->conn_pcb->remote_port,
				conn->bytes_transferred, duration_ms, bandwidth_kbitpsec);
#endif
	}
}

/** Close an iperf tcp session */
static void
lwiperf_tcp_close(lwiperf_state_tcp_t *conn, enum lwiperf_report_type report_type)
{
	err_t err;

	lwiperf_list_remove(&conn->base);
	lwip_tcp_conn_report(conn, report_type);
	if (conn->conn_pcb != NULL) {
		tcp_arg(conn->conn_pcb, NULL);
		tcp_poll(conn->conn_pcb, NULL, 0);
		tcp_sent(conn->conn_pcb, NULL);
		tcp_recv(conn->conn_pcb, NULL);
		tcp_err(conn->conn_pcb, NULL);
		err = tcp_close(conn->conn_pcb);
		if (err != ERR_OK) {
			/* don't want to wait for free memory here... */
			tcp_abort(conn->conn_pcb);
		}
	} else {
		/* no conn pcb, this is the listener pcb */
		err = tcp_close(conn->server_pcb);
		LWIP_ASSERT("error", err == ERR_OK);
	}
	LWIPERF_FREE(lwiperf_state_tcp_t, conn);
}

/** Try to send more data on an iperf tcp session */
static err_t
lwiperf_tcp_client_send_more(lwiperf_state_tcp_t *conn)
{
	int send_more;
	err_t err;
	u16_t txlen;
	u16_t txlen_max;
	void *txptr;
	u8_t apiflags;

	LWIP_ASSERT("conn invalid", (conn != NULL) && conn->base.tcp && (conn->base.server == 0));

	do {
		send_more = 0;

		if (conn->force_stop == true) {
			lwiperf_tcp_close(conn, LWIPERF_TCP_DONE_CLIENT);
			return ERR_OK;
		}

		if (conn->settings.amount & PP_HTONL(0x80000000)) {
			/* this session is time-limited */
			u32_t now = sys_now();
			u32_t diff_ms = now - conn->time_started;
			u32_t time = (u32_t) - (s32_t)lwip_htonl(conn->settings.amount);
			u32_t time_ms = time * 10;
			if (diff_ms >= time_ms) {
				/* time specified by the client is over -> close the connection */
				lwiperf_tcp_close(conn, LWIPERF_TCP_DONE_CLIENT);
				return ERR_OK;
			}
		} else {
			/* this session is byte-limited */
			u32_t amount_bytes = lwip_htonl(conn->settings.amount);
			/* @todo: this can send up to 1*MSS more than requested... */
			if (amount_bytes >= conn->bytes_transferred) {
				/* all requested bytes transferred -> close the connection */
				lwiperf_tcp_close(conn, LWIPERF_TCP_DONE_CLIENT);
				return ERR_OK;
			}
		}

		if (conn->bytes_transferred < 24) {
			/* transmit the settings a first time */
			txptr = &((u8_t *)&conn->settings)[conn->bytes_transferred];
			txlen_max = (u16_t)(24 - conn->bytes_transferred);
			apiflags = TCP_WRITE_FLAG_COPY;
		} else if (conn->bytes_transferred < 48) {
			/* transmit the settings a second time */
			txptr = &((u8_t *)&conn->settings)[conn->bytes_transferred - 24];
			txlen_max = (u16_t)(48 - conn->bytes_transferred);
			apiflags = TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE;
			send_more = 1;
		} else {
			/* transmit data */
			/* @todo: every x bytes, transmit the settings again */
			txptr = LWIP_CONST_CAST(void *, &lwiperf_txbuf_const[conn->bytes_transferred % 10]);
			txlen_max = TCP_MSS;
			if (conn->bytes_transferred == 48) { /* @todo: fix this for intermediate settings, too */
				txlen_max = TCP_MSS - 24;
			}
			apiflags = 0; /* no copying needed */
			send_more = 1;
		}
		txlen = txlen_max;
		do {
			err = tcp_write(conn->conn_pcb, txptr, txlen, apiflags);
			if (err ==  ERR_MEM) {
				txlen /= 2;
			}
		} while ((err == ERR_MEM) && (txlen >= (TCP_MSS / 2)));

		if (err == ERR_OK) {
			conn->bytes_transferred += txlen;
		} else {
			send_more = 0;
		}
	} while (send_more);

	tcp_output(conn->conn_pcb);
	return ERR_OK;
}

/** TCP sent callback, try to send more data */
static err_t
lwiperf_tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
	lwiperf_state_tcp_t *conn = (lwiperf_state_tcp_t *)arg;
	/* @todo: check 'len' (e.g. to time ACK of all data)? for now, we just send more... */
	LWIP_ASSERT("invalid conn", conn->conn_pcb == tpcb);
	LWIP_UNUSED_ARG(tpcb);
	LWIP_UNUSED_ARG(len);

	conn->poll_count = 0;

	return lwiperf_tcp_client_send_more(conn);
}

/** TCP connected callback (active connection), send data now */
static err_t
lwiperf_tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
	lwiperf_state_tcp_t *conn = (lwiperf_state_tcp_t *)arg;
	LWIP_ASSERT("invalid conn", conn->conn_pcb == tpcb);
	LWIP_UNUSED_ARG(tpcb);
	if (err != ERR_OK) {
		lwiperf_tcp_close(conn, LWIPERF_TCP_ABORTED_REMOTE);
		return ERR_OK;
	}
	conn->poll_count = 0;
	conn->time_started = sys_now();
	return lwiperf_tcp_client_send_more(conn);
}

/** Start TCP connection back to the client (either parallel or after the
 * receive test has finished.
 */
static err_t
lwiperf_tx_start_impl(const ip_addr_t *remote_ip, u16_t remote_port, lwiperf_settings_t *settings, lwiperf_report_fn report_fn,
                      void *report_arg, lwiperf_state_base_t *related_master_state, lwiperf_state_tcp_t **new_conn
#if defined(NRC_LWIP)
                      , u32_t duration, u8_t tos
#endif
)
{
	err_t err;
	lwiperf_state_tcp_t *client_conn;
	struct tcp_pcb *newpcb;
	ip_addr_t remote_addr;

	LWIP_ASSERT("remote_ip != NULL", remote_ip != NULL);
	LWIP_ASSERT("remote_ip != NULL", settings != NULL);
	LWIP_ASSERT("new_conn != NULL", new_conn != NULL);
	*new_conn = NULL;

	client_conn = (lwiperf_state_tcp_t *)LWIPERF_ALLOC(lwiperf_state_tcp_t);
	if (client_conn == NULL) {
		return ERR_MEM;
	}
	newpcb = tcp_new_ip_type(IP_GET_TYPE(remote_ip));
	if (newpcb == NULL) {
		LWIPERF_FREE(lwiperf_state_tcp_t, client_conn);
		return ERR_MEM;
	}
	memset(client_conn, 0, sizeof(lwiperf_state_tcp_t));
#ifdef NRC_LWIP
	newpcb->tos = tos;
	client_conn->tos = tos;
	client_conn->duration = duration ;
#endif /* NRC_LWIP */
	client_conn->base.tcp = 1;
	client_conn->base.related_master_state = related_master_state;
	client_conn->conn_pcb = newpcb;
	client_conn->time_started = sys_now(); /* @todo: set this again on 'connected' */
	client_conn->report_fn = report_fn;
	client_conn->report_arg = report_arg;
	client_conn->next_num = 4; /* initial nr is '4' since the header has 24 byte */
	client_conn->bytes_transferred = 0;
	memcpy(&client_conn->settings, settings, sizeof(*settings));
	client_conn->have_settings_buf = 1;

	tcp_arg(newpcb, client_conn);
	tcp_sent(newpcb, lwiperf_tcp_client_sent);
	tcp_poll(newpcb, lwiperf_tcp_poll, 2U);
	tcp_err(newpcb, lwiperf_tcp_err);

	ip_addr_copy(remote_addr, *remote_ip);

	err = tcp_connect(newpcb, &remote_addr, remote_port, lwiperf_tcp_client_connected);
	if (err != ERR_OK) {
		lwiperf_tcp_close(client_conn, LWIPERF_TCP_ABORTED_LOCAL);
		return err;
	}
#if defined(NRC_LWIP)
	client_conn->base.local_port = newpcb->local_port;
	client_conn->base.remote_port = remote_port;
	ip4_addr_copy(client_conn->base.local_addr, newpcb->local_ip);
	ip4_addr_copy(client_conn->base.remote_addr, *remote_ip);
	client_conn->base.conn = client_conn;
#endif
	lwiperf_list_add(&client_conn->base);
	*new_conn = client_conn;
	return ERR_OK;
}

static err_t
lwiperf_tx_start_passive(lwiperf_state_tcp_t *conn)
{
	err_t ret;
	lwiperf_state_tcp_t *new_conn = NULL;
	u16_t remote_port = (u16_t)lwip_htonl(conn->settings.remote_port);

	ret = lwiperf_tx_start_impl(&conn->conn_pcb->remote_ip, remote_port, &conn->settings, conn->report_fn, conn->report_arg,
	conn->base.related_master_state, &new_conn
#if defined(NRC_LWIP)
	, conn->duration, conn->tos
#endif
	);
	if (ret == ERR_OK) {
		LWIP_ASSERT("new_conn != NULL", new_conn != NULL);
		new_conn->settings.flags = 0; /* prevent the remote side starting back as client again */
	}
	return ret;
}

/** Receive data on an iperf tcp session */
static err_t
lwiperf_tcp_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
	u8_t tmp;
	u16_t tot_len;
	u32_t packet_idx;
	struct pbuf *q;
	lwiperf_state_tcp_t *conn = (lwiperf_state_tcp_t *)arg;

	LWIP_ASSERT("pcb mismatch", conn->conn_pcb == tpcb);
	LWIP_UNUSED_ARG(tpcb);

	if (err != ERR_OK) {
		lwiperf_tcp_close(conn, LWIPERF_TCP_ABORTED_REMOTE);
		return ERR_OK;
	}
	if (p == NULL) {
		/* connection closed -> test done */
		if (conn->settings.flags & PP_HTONL(LWIPERF_FLAGS_ANSWER_TEST)) {
			if ((conn->settings.flags & PP_HTONL(LWIPERF_FLAGS_ANSWER_NOW)) == 0) {
				/* client requested transmission after end of test */
				lwiperf_tx_start_passive(conn);
			}
		}
		lwiperf_tcp_close(conn, LWIPERF_TCP_DONE_SERVER);
		return ERR_OK;
	}
	tot_len = p->tot_len;

	conn->poll_count = 0;

	if ((!conn->have_settings_buf) || ((conn->bytes_transferred - 24) % (1024 * 128) == 0)) {
		/* wait for 24-byte header */
		if (p->tot_len < sizeof(lwiperf_settings_t)) {
			lwiperf_tcp_close(conn, LWIPERF_TCP_ABORTED_LOCAL_DATAERROR);
			pbuf_free(p);
			return ERR_OK;
		}
		if (!conn->have_settings_buf) {
			if (pbuf_copy_partial(p, &conn->settings, sizeof(lwiperf_settings_t), 0) != sizeof(lwiperf_settings_t)) {
				lwiperf_tcp_close(conn, LWIPERF_TCP_ABORTED_LOCAL);
				pbuf_free(p);
				return ERR_OK;
			}
			conn->have_settings_buf = 1;
			if (conn->settings.flags & PP_HTONL(LWIPERF_FLAGS_ANSWER_TEST)) {
				if (conn->settings.flags & PP_HTONL(LWIPERF_FLAGS_ANSWER_NOW)) {
					/* client requested parallel transmission test */
					err_t err2 = lwiperf_tx_start_passive(conn);
					if (err2 != ERR_OK) {
						lwiperf_tcp_close(conn, LWIPERF_TCP_ABORTED_LOCAL_TXERROR);
						pbuf_free(p);
						return ERR_OK;
					}
				}
			}
		} else {
			if (conn->settings.flags & PP_HTONL(LWIPERF_FLAGS_ANSWER_TEST)) {
				if (pbuf_memcmp(p, 0, &conn->settings, sizeof(lwiperf_settings_t)) != 0) {
					lwiperf_tcp_close(conn, LWIPERF_TCP_ABORTED_LOCAL_DATAERROR);
					pbuf_free(p);
					return ERR_OK;
				}
			}
		}
		conn->bytes_transferred += sizeof(lwiperf_settings_t);
		if (conn->bytes_transferred <= 24) {
			conn->time_started = sys_now();
			tcp_recved(tpcb, p->tot_len);
			pbuf_free(p);
			return ERR_OK;
		}
		conn->next_num = 4; /* 24 bytes received... */
		tmp = pbuf_remove_header(p, 24);
		LWIP_ASSERT("pbuf_remove_header failed", tmp == 0);
		LWIP_UNUSED_ARG(tmp); /* for LWIP_NOASSERT */
	}

	packet_idx = 0;
	for (q = p; q != NULL; q = q->next) {
#if LWIPERF_CHECK_RX_DATA
		const u8_t *payload = (const u8_t *)q->payload;
		u16_t i;
		for (i = 0; i < q->len; i++) {
			u8_t val = payload[i];
			u8_t num = val - '0';
			if (num == conn->next_num) {
				conn->next_num++;
				if (conn->next_num == 10) {
					conn->next_num = 0;
				}
			} else {
				lwiperf_tcp_close(conn, LWIPERF_TCP_ABORTED_LOCAL_DATAERROR);
				pbuf_free(p);
				return ERR_OK;
			}
		}
#endif
		packet_idx += q->len;
	}
	LWIP_ASSERT("count mismatch", packet_idx == p->tot_len);
	conn->bytes_transferred += packet_idx;
	tcp_recved(tpcb, tot_len);
	pbuf_free(p);
	return ERR_OK;
}

/** Error callback, iperf tcp session aborted */
static void
lwiperf_tcp_err(void *arg, err_t err)
{
	lwiperf_state_tcp_t *conn = (lwiperf_state_tcp_t *)arg;
	LWIP_UNUSED_ARG(err);
	lwiperf_tcp_close(conn, LWIPERF_TCP_ABORTED_REMOTE);
}

/** TCP poll callback, try to send more data */
static err_t
lwiperf_tcp_poll(void *arg, struct tcp_pcb *tpcb)
{
	lwiperf_state_tcp_t *conn = (lwiperf_state_tcp_t *)arg;
	LWIP_ASSERT("pcb mismatch", conn->conn_pcb == tpcb);
	LWIP_UNUSED_ARG(tpcb);
	if (++conn->poll_count >= LWIPERF_TCP_MAX_IDLE_SEC) {
		lwiperf_tcp_close(conn, LWIPERF_TCP_ABORTED_LOCAL);
		return ERR_OK; /* lwiperf_tcp_close frees conn */
	}

	if (!conn->base.server) {
		lwiperf_tcp_client_send_more(conn);
	}

	return ERR_OK;
}

/** This is called when a new client connects for an iperf tcp session */
static err_t
lwiperf_tcp_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
	lwiperf_state_tcp_t *s, *conn;
	if ((err != ERR_OK) || (newpcb == NULL) || (arg == NULL)) {
		return ERR_VAL;
	}

	s = (lwiperf_state_tcp_t *)arg;
	LWIP_ASSERT("invalid session", s->base.server);
	LWIP_ASSERT("invalid listen pcb", s->server_pcb != NULL);
	LWIP_ASSERT("invalid conn pcb", s->conn_pcb == NULL);
	if (s->specific_remote) {
		LWIP_ASSERT("s->base.related_master_state != NULL", s->base.related_master_state != NULL);
		if (!ip_addr_cmp(&newpcb->remote_ip, &s->remote_addr)) {
			/* this listener belongs to a client session, and this is not the correct remote */
			return ERR_VAL;
		}
	} else {
		LWIP_ASSERT("s->base.related_master_state == NULL", s->base.related_master_state == NULL);
	}

	conn = (lwiperf_state_tcp_t *)LWIPERF_ALLOC(lwiperf_state_tcp_t);
	if (conn == NULL) {
		return ERR_MEM;
	}
	memset(conn, 0, sizeof(lwiperf_state_tcp_t));
	conn->base.tcp = 1;
	conn->base.server = 1;
	conn->base.related_master_state = &s->base;
	conn->conn_pcb = newpcb;
	conn->time_started = sys_now();
	conn->report_fn = s->report_fn;
	conn->report_arg = s->report_arg;

	/* setup the tcp rx connection */
	tcp_arg(newpcb, conn);
	tcp_recv(newpcb, lwiperf_tcp_recv);
	tcp_poll(newpcb, lwiperf_tcp_poll, 2U);
	tcp_err(conn->conn_pcb, lwiperf_tcp_err);

	if (s->specific_remote) {
		/* this listener belongs to a client, so make the client the master of the newly created connection */
		conn->base.related_master_state = s->base.related_master_state;
		/* if dual mode or (tradeoff mode AND client is done): close the listener */
		if (!s->client_tradeoff_mode || !lwiperf_list_find(s->base.related_master_state)) {
			/* prevent report when closing: this is expected */
			s->report_fn = NULL;
			lwiperf_tcp_close(s, LWIPERF_TCP_ABORTED_LOCAL);
		}
	}
#if defined(NRC_LWIP)
	lwiperf_display_server_connect(conn);
#endif
	lwiperf_list_add(&conn->base);
	return ERR_OK;
}

/**
 * @ingroup iperf
 * Start a TCP iperf server on the default TCP port (5001) and listen for
 * incoming connections from iperf clients.
 *
 * @returns a connection handle that can be used to abort the server
 *          by calling @ref lwiperf_abort()
 */
void *
lwiperf_start_tcp_server_default(lwiperf_report_fn report_fn, void *report_arg)
{
  return lwiperf_start_tcp_server(IP_ADDR_ANY, LWIPERF_TCP_PORT_DEFAULT,
                                  report_fn, report_arg);
}

/**
 * @ingroup iperf
 * Start a TCP iperf server on a specific IP address and port and listen for
 * incoming connections from iperf clients.
 *
 * @returns a connection handle that can be used to abort the server
 *          by calling @ref lwiperf_abort()
 */
void *
lwiperf_start_tcp_server(const ip_addr_t *local_addr, u16_t local_port,
                         lwiperf_report_fn report_fn, void *report_arg)
{
	err_t err;
	lwiperf_state_tcp_t *state = NULL;

	err = lwiperf_start_tcp_server_impl(local_addr, local_port, report_fn, report_arg,
	NULL, &state);
	if (err == ERR_OK) {
		return state;
	}
	return NULL;
}

static err_t lwiperf_start_tcp_server_impl(const ip_addr_t *local_addr, u16_t local_port,
                                           lwiperf_report_fn report_fn, void *report_arg,
                                           lwiperf_state_base_t *related_master_state, lwiperf_state_tcp_t **state)
{
	err_t err;
	struct tcp_pcb *pcb;
	lwiperf_state_tcp_t *s;

	LWIP_ASSERT_CORE_LOCKED();

	LWIP_ASSERT("state != NULL", state != NULL);

	if (local_addr == NULL) {
		return ERR_ARG;
	}

	s = (lwiperf_state_tcp_t *)LWIPERF_ALLOC(lwiperf_state_tcp_t);
	if (s == NULL) {
		return ERR_MEM;
	}
	memset(s, 0, sizeof(lwiperf_state_tcp_t));
	s->base.tcp = 1;
	s->base.server = 1;
	s->base.related_master_state = related_master_state;
	s->report_fn = report_fn;
	s->report_arg = report_arg;

	pcb = tcp_new_ip_type(LWIPERF_SERVER_IP_TYPE);
	if (pcb == NULL) {
		return ERR_MEM;
	}
	err = tcp_bind(pcb, local_addr, local_port);
	if (err != ERR_OK) {
		return err;
	}
	s->server_pcb = tcp_listen_with_backlog(pcb, 1);
	if (s->server_pcb == NULL) {
		if (pcb != NULL) {
			tcp_close(pcb);
		}
		LWIPERF_FREE(lwiperf_state_tcp_t, s);
		return ERR_MEM;
	}
	pcb = NULL;

#if defined(NRC_LWIP)
	s->base.local_port =local_port;
	ip4_addr_copy(s->base.local_addr, *local_addr);
	s->base.conn = s;
#endif
	tcp_arg(s->server_pcb, s);
	tcp_accept(s->server_pcb, lwiperf_tcp_accept);

	lwiperf_list_add(&s->base);
	*state = s;
	return ERR_OK;
}

/**
 * @ingroup iperf
 * Start a TCP iperf client to the default TCP port (5001).
 *
 * @returns a connection handle that can be used to abort the client
 *          by calling @ref lwiperf_abort()
 */
void* lwiperf_start_tcp_client_default(const ip_addr_t* remote_addr,
                               lwiperf_report_fn report_fn, void* report_arg)
{
	return lwiperf_start_tcp_client(remote_addr, LWIPERF_TCP_PORT_DEFAULT, LWIPERF_CLIENT,
                                  report_fn, report_arg
#if defined(NRC_LWIP)
                                  , LWIPERF_DEFAULT_DURATION, 0
#endif
);
}

/**
 * @ingroup iperf
 * Start a TCP iperf client to a specific IP address and port.
 *
 * @returns a connection handle that can be used to abort the client
 *          by calling @ref lwiperf_abort()
 */
void* lwiperf_start_tcp_client(const ip_addr_t* remote_addr, u16_t remote_port,
	enum lwiperf_client_type type, lwiperf_report_fn report_fn, void* report_arg
#if defined(NRC_LWIP)
	, u32_t duration, u8_t tos
#endif
)
{
	err_t ret;
	lwiperf_settings_t settings;
	lwiperf_state_tcp_t *state = NULL;

	memset(&settings, 0, sizeof(settings));
	switch (type) {
		case LWIPERF_CLIENT:
			/* Unidirectional tx only test */
			settings.flags = 0;
			break;
		case LWIPERF_DUAL:
			/* Do a bidirectional test simultaneously */
			settings.flags = htonl(LWIPERF_FLAGS_ANSWER_TEST | LWIPERF_FLAGS_ANSWER_NOW);
			break;
		case LWIPERF_TRADEOFF:
			/* Do a bidirectional test individually */
			settings.flags = htonl(LWIPERF_FLAGS_ANSWER_TEST);
			break;
		default:
			/* invalid argument */
			return NULL;
	}
	settings.num_threads = htonl(1);
#if defined(NRC_LWIP)
	settings.remote_port = htonl(remote_port);
	settings.amount = htonl((u32_t)(100*duration)*(-1));
#else
	settings.remote_port = htonl(LWIPERF_TCP_PORT_DEFAULT);
	/* TODO: implement passing duration/amount of bytes to transfer */
	settings.amount = htonl((u32_t)-1000);
#endif

	ret = lwiperf_tx_start_impl(remote_addr, remote_port, &settings, report_fn, report_arg, NULL, &state
#if defined(NRC_LWIP)
	, duration, tos
#endif
	);
	if (ret == ERR_OK) {
		LWIP_ASSERT("state != NULL", state != NULL);
		if (type != LWIPERF_CLIENT) {
			/* start corresponding server now */
			lwiperf_state_tcp_t *server = NULL;
			ret = lwiperf_start_tcp_server_impl(&state->conn_pcb->local_ip, LWIPERF_TCP_PORT_DEFAULT,
			report_fn, report_arg, (lwiperf_state_base_t *)state, &server);
			if (ret != ERR_OK) {
				/* starting server failed, abort client */
				lwiperf_abort(state);
				return NULL;
			}
			/* make this server accept one connection only */
			server->specific_remote = 1;
			server->remote_addr = state->conn_pcb->remote_ip;
			if (type == LWIPERF_TRADEOFF) {
				/* tradeoff means that the remote host connects only after the client is done,
				so keep the listen pcb open until the client is done */
				server->client_tradeoff_mode = 1;
			}
		}
		return state;
	}
	return NULL;
}

/**
 * @ingroup iperf
 * Abort an iperf session (handle returned by lwiperf_start_tcp_server*())
 */
void
lwiperf_abort(void *lwiperf_session)
{
	lwiperf_state_base_t *i, *dealloc, *last = NULL;

	LWIP_ASSERT_CORE_LOCKED();

	for (i = lwiperf_all_connections; i != NULL; ) {
		if ((i == lwiperf_session) || (i->related_master_state == lwiperf_session)) {
			dealloc = i;
			i = i->next;
			if (last != NULL) {
				last->next = i;
			}
			LWIPERF_FREE(lwiperf_state_tcp_t, dealloc); /* @todo: type? */
		} else {
			last = i;
			i = i->next;
		}
	}
}

#ifdef NRC_LWIP
lwiperf_state_tcp_t* lwiperf_get_session(iperf_parm_t* parm)
{
	lwiperf_state_base_t* iter;

	for (iter = lwiperf_all_connections; iter != NULL; iter = iter->next) {
		if(parm->role == 1){
			if (ip4_addr_cmp(ip_2_ip4(&iter->local_addr),&(parm->addr)) && (iter->tcp == parm->mode)&&
				(iter->local_port == parm->port)&& (iter->server == parm->role)) {
				return iter->conn;
			}
		}else{
			if (ip4_addr_cmp(ip_2_ip4(&iter->remote_addr),&(parm->addr)) && (iter->tcp == parm->mode)&&
				(iter->remote_port == parm->port)&& (iter->server == parm->role)) {
				return iter->conn;
			}
		}
	}
	return NULL;
}

void lwiperf_tcp_server_close(lwiperf_state_tcp_t* conn)
{
	lwiperf_list_remove(&conn->base);
	if(conn->server_pcb != NULL)
		tcp_close(conn->server_pcb);

	if(conn != NULL)
		LWIPERF_FREE(lwiperf_state_tcp_t, conn);
}

static u8_t payload_buffer[LWIP_MEM_ALIGN_BUFFER(LWIPERF_UDP_DATA_SIZE+1)];
static u8_t* payload = &payload_buffer[0];

lwiperf_udp_server_conn_t* lwiperf_get_session_udp_server(struct udp_pcb* upcb)
{
	lwiperf_udp_server_conn_t* iter = NULL;
	for (iter = lwiperf_all_udp_server_connections; iter != NULL; iter = iter->next) {
		if (ip4_addr_cmp(&(iter->remote_ip),&(upcb->remote_ip)) && (iter->remote_port == upcb->remote_port)&&
			ip4_addr_cmp(&(iter->local_ip),&(upcb->local_ip)) && (iter->local_port == upcb->local_port)) {
			return (lwiperf_udp_server_conn_t*)iter;
		}
	}
	return (lwiperf_udp_server_conn_t*)NULL;
}

static void
lwiperf_list_add_udp_server(lwiperf_udp_server_conn_t* item)
{
	lwiperf_udp_server_conn_t* temp;
	if (lwiperf_all_udp_server_connections == NULL) {
		lwiperf_all_udp_server_connections = item;
		item->next = NULL;
	} else {
		temp = lwiperf_all_udp_server_connections;
		lwiperf_all_udp_server_connections = item;
		item->next = temp;
	}
}

/** Remove an iperf session from the 'active' list */
static void
lwiperf_list_remove_udp_server(lwiperf_udp_server_conn_t* item)
{
	lwiperf_udp_server_conn_t* prev = NULL;
	lwiperf_udp_server_conn_t* iter;
	for (iter = lwiperf_all_udp_server_connections; iter != NULL; prev = iter, iter = iter->next) {
		if (iter == item) {
			if (prev == NULL) {
				lwiperf_all_udp_server_connections = iter->next;
			} else {
				prev->next = iter->next;
			}
			break;
		}
	}
}

void lwiperf_list_display(void) {
	lwiperf_state_base_t *ptr = lwiperf_all_connections;
	char local_address[30];
	char remote_address[30];

	memset(local_address, 0x0, sizeof(local_address));
	memset(remote_address, 0x0, sizeof(remote_address));

	if(ptr == NULL){
		lwiperf_mutex_lock();
		A("No iperf application is running\n");
		lwiperf_mutex_unlock();
		return;
	}

	lwiperf_mutex_lock();
	A("\n-------------------------- iperf running Status -----------------------------\n");
	A(" Operation\tLocal address  \t\tRemote address  \tInterface\n");

	while(ptr != NULL) {
		A(" %s_%s\t",
			ptr->tcp == 1 ? "tcp":"udp",ptr->server == 1 ? "server":"client");

		sprintf(local_address,"%"U16_F".%"U16_F".%"U16_F".%"U16_F":%d\t",
			ip4_addr1_16(&ptr->local_addr), ip4_addr2_16(&ptr->local_addr),
			ip4_addr3_16(&ptr->local_addr), ip4_addr4_16(&ptr->local_addr), (int)ptr->local_port);
		A("%s", local_address);

		if(strlen(local_address)<17){
			A("\t");
		}

		sprintf(remote_address, "%"U16_F".%"U16_F".%"U16_F".%"U16_F":%d\t",
			ip4_addr1_16(&ptr->remote_addr), ip4_addr2_16(&ptr->remote_addr),
			ip4_addr3_16(&ptr->remote_addr), ip4_addr4_16(&ptr->remote_addr), (int)ptr->remote_port);
		A("%s", remote_address);

		if(strlen(remote_address)<17){
			A("\t");
		}
		A("wlan%d\n",wifi_get_vif_id(&ptr->local_addr));
		ptr = ptr->next;
	}
	A("-----------------------------------------------------------------------------\n");
	lwiperf_mutex_unlock();
}

void lwiperf_display_server_connect(void *arg){
	lwiperf_state_tcp_t*conn =(lwiperf_state_tcp_t*)arg;
	ip_addr_t local_ip, remote_ip;
	u16_t local_port, remote_port;

	if(conn->base.tcp){
		ip_addr_copy(local_ip, conn->conn_pcb->local_ip);
		ip_addr_copy(remote_ip, conn->conn_pcb->remote_ip);
		local_port = conn->conn_pcb->local_port;
		remote_port = conn->conn_pcb->remote_port;
	}else{
		ip_addr_copy(local_ip, conn->udp_conn_pcb->local_ip);
		ip_addr_copy(remote_ip, conn->udp_conn_pcb->remote_ip);
		local_port = conn->udp_conn_pcb->local_port;
		remote_port = conn->udp_conn_pcb->remote_port;
	}
	conn->base.local_port =local_port;
	conn->base.remote_port =remote_port;
	ip4_addr_copy(conn->base.remote_addr, remote_ip);
	ip4_addr_copy(conn->base.local_addr, local_ip);

	A("\n[IPERF] %s_%s", conn->base.tcp == 1 ? "tcp":"udp", conn->base.server == 1 ? "server":"client");

	A(" %"U16_F".%"U16_F".%"U16_F".%"U16_F":%d ",
		ip4_addr1_16(&local_ip), ip4_addr2_16(&local_ip),
		ip4_addr3_16(&local_ip), ip4_addr4_16(&local_ip), local_port);
	A("connected with");
	A(" %"U16_F".%"U16_F".%"U16_F".%"U16_F":%d\n",
		ip4_addr1_16(&remote_ip), ip4_addr2_16(&remote_ip),
		ip4_addr3_16(&remote_ip), ip4_addr4_16(&remote_ip), remote_port);
}


lwiperf_udp_server_conn_t* lwiperf_udp_server_conn_init(lwiperf_state_tcp_t *conn )
{
	lwiperf_udp_server_conn_t* udp_server_conn;
	udp_server_conn = lwiperf_get_session_udp_server(conn->udp_conn_pcb);
	if(udp_server_conn != NULL){
		lwiperf_list_remove_udp_server(udp_server_conn);
		LWIPERF_FREE(lwiperf_udp_server_conn_t, udp_server_conn);
		udp_server_conn = NULL;
	}
	udp_server_conn = (lwiperf_udp_server_conn_t*)LWIPERF_ALLOC(lwiperf_udp_server_conn_t);
	udp_server_conn->remote_port= conn->udp_conn_pcb->remote_port;
	udp_server_conn->local_port= conn->udp_conn_pcb->local_port;
	udp_server_conn->session_on= 0;
	udp_server_conn->bytes_received = 0;
	udp_server_conn->outoforder_packets= 0;
	udp_server_conn->cnt_error = 0;
	udp_server_conn->packet_count = -1;
	udp_server_conn->time_started = sys_now();
	udp_server_conn->time_ended = 0;

	ip4_addr_copy(udp_server_conn->remote_ip, *(&conn->udp_conn_pcb->remote_ip));
	ip4_addr_copy(udp_server_conn->local_ip, *(&conn->udp_conn_pcb->local_ip));

	lwiperf_list_add_udp_server(udp_server_conn);
	return udp_server_conn;
}

void lwiperf_udp_server_conn_deinit(lwiperf_udp_server_conn_t* udp_server_conn )
{
	lwiperf_list_remove_udp_server(udp_server_conn);
	LWIPERF_FREE(lwiperf_udp_server_conn_t, udp_server_conn);
}


/** Receive data on an iperf udp session in server mode*/
static void
lwiperf_udp_recv(void *arg, struct udp_pcb *upcb,
                               struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
	s32_t  pcount;
	uint32_t pc;
	int ret = 0;
	lwiperf_state_tcp_t *conn = (lwiperf_state_tcp_t*)arg;
	lwiperf_udp_server_conn_t* udp_server_conn = NULL;

	if (p != NULL) {
		 /* copy payload inside static buffer for processing */
		if (pbuf_copy_partial(p, payload, p->tot_len, 0) == p->tot_len) {
			memcpy(&pc, payload, sizeof(pc));
			pcount = ntohl(pc);
			conn->udp_conn_pcb->remote_port= port;
			ip4_addr_copy(conn->udp_conn_pcb->remote_ip, *addr);

			udp_server_conn = lwiperf_get_session_udp_server(conn->udp_conn_pcb);
			if(udp_server_conn == NULL){
				udp_server_conn = lwiperf_udp_server_conn_init(conn);
			}

			if(pcount >= 0){
				if(udp_server_conn->session_on == 0){
					lwiperf_display_server_connect(conn);
					udp_server_conn->session_on = 1;
				}

				if (pcount >= udp_server_conn->packet_count + 1) {
					if (pcount > udp_server_conn->packet_count + 1) {
						udp_server_conn->cnt_error += (pcount - 1) - udp_server_conn->packet_count;
					}
					udp_server_conn->packet_count = pcount;
				} else {
					udp_server_conn->outoforder_packets++;
					if (udp_server_conn->cnt_error > 0)
						udp_server_conn->cnt_error--;
					//A(" * OUT OF ORDER - incoming packet sequence %d but expected sequence %d\n",
					//	 pcount, udp_server_conn->packet_count + 1);
				}
				// update total received length
				udp_server_conn->bytes_received += p->tot_len;
				udp_server_conn->time_ended = sys_now();
			} else {
				udp_sendto(upcb,p, addr, port);
				if(udp_server_conn != NULL){
					conn->time_started = udp_server_conn->time_started;
					conn->time_ended = udp_server_conn->time_ended;
					conn->outoforder_packets = udp_server_conn->outoforder_packets;
					conn->cnt_error = udp_server_conn->cnt_error;
					conn->total_packets = -1*(pcount);
					conn->bytes_transferred = udp_server_conn->bytes_received;
					if(udp_server_conn->session_on == 1) {
						lwip_tcp_conn_report(conn, LWIPERF_UDP_DONE_SERVER);
						udp_server_conn->session_on = 0;
					}
					lwiperf_udp_server_conn_deinit(udp_server_conn);
				}
			}
		}
		/* free the pbuf */
		if (p->ref != 0)
			pbuf_free(p);
	}
}

/**
 * Start a UDP iperf server on a specific IP address and port.
 *
 * @returns a connection handle that can be used to abort the server
 *          by calling @ref lwiperf_abort()
 */
void*
lwiperf_start_udp_server(const ip_addr_t* local_addr, u16_t local_port,
	lwiperf_report_fn report_fn, void* report_arg)
{
	err_t err;
	struct udp_pcb *pcb;
	lwiperf_state_tcp_t* s;

	if (local_addr == NULL) {
		return NULL;
	}

	s = (lwiperf_state_tcp_t*)LWIPERF_ALLOC(lwiperf_state_tcp_t);
	if (s == NULL) {
		return NULL;
	}
	memset(s, 0, sizeof(lwiperf_state_tcp_t));
	s->base.tcp = 0;
	s->base.server =1;
	s->base.local_port = local_port;
	s->base.conn = s;
	ip4_addr_copy(s->base.local_addr, *local_addr);
	s->base.conn = s;

	s->report_fn = report_fn;
	s->report_arg = report_arg;

	pcb = udp_new();
	if (pcb != NULL) {
		err = udp_bind(pcb, local_addr, local_port);
		if (err == ERR_OK) {
			s->udp_conn_pcb = pcb;
		}
	}

	if (s->udp_conn_pcb == NULL) {
		if (pcb != NULL) {
			udp_remove(pcb);
		}
		LWIPERF_FREE(lwiperf_state_tcp_t, s);
		return NULL;
	}
	udp_recv(pcb, lwiperf_udp_recv, s);
	lwiperf_list_add(&s->base);
	pcb = NULL;

	return s;
}

/**
 * Start a UDP iperf server on the default UDP port (5001) and listen for
 * incoming connections from iperf clients.
 *
 * @returns a connection handle that can be used to abort the server
 *          by calling @ref lwiperf_abort()
 */
void*
lwiperf_start_udp_server_default(lwiperf_report_fn report_fn, void* report_arg,u16_t port)
{
	return lwiperf_start_udp_server(IP_ADDR_ANY, port, report_fn, report_arg);
}

/** Receive data on an iperf udp session in client mode*/
static void
udp_receive_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
	lwiperf_state_tcp_t* conn = (lwiperf_state_tcp_t* )arg;

	if(conn->udp_client_status == LWIPERF_UDP_CLIENT_STOPPING){
		conn->udp_client_status = LWIPERF_UDP_CLIENT_STOP;
	}

	/* Free receive pbuf */
	if (p->ref != 0)
		pbuf_free(p);
}

/** Close an iperf udp session */
static void
lwiperf_udp_close(lwiperf_state_tcp_t* conn, enum lwiperf_report_type report_type)
{
	lwip_tcp_conn_report(conn, report_type);
	lwiperf_list_remove(&conn->base);
	if(conn->udp_conn_pcb != NULL)
		udp_remove(conn->udp_conn_pcb);

	if (conn != NULL)
		LWIPERF_FREE(lwiperf_state_tcp_t, conn);
}

/** Try to send more data on an iperf udp session */
err_t lwiperf_udp_client_send_more(lwiperf_state_tcp_t* conn, int send_more, u16_t port, u32_t duration, u32_t bandwidth, u32_t udp_data_length)
{
	struct pbuf *p;
	u32_t  pcount;
	u32_t tv_sec;
	u32_t tv_usec;
	u32_t test_mode;
	u32_t flags;
	u32_t numThreads;
	u32_t listen_port;
	u32_t bufferlen;
	u32_t mUDPRate;
	u32_t mAmount;
	u32_t sent_time = sys_now();
	err_t err = ERR_MEM;

	p = pbuf_alloc(PBUF_TRANSPORT,udp_data_length, PBUF_POOL);

	if (p)
	{
		tv_sec =  lwip_htonl(sent_time/MILI_PER_SECOND);
		tv_usec =  lwip_htonl((sent_time%MILI_PER_SECOND)*1000);
		flags =  lwip_htonl(0);
		numThreads =  lwip_htonl(1);
		listen_port =  lwip_htonl(port);
		bufferlen =  lwip_htonl(0);
		mUDPRate =  lwip_htonl(bandwidth);
		mAmount = (s32_t)lwip_htonl((100*duration)*(-1));

		if(send_more){
			pcount = lwip_htonl(conn->total_packets);
			conn->total_packets++;
			conn->udp_client_status = LWIPERF_UDP_CLIENT_RUNNING;
		}else{
			pcount = ~lwip_htonl(conn->total_packets-1);
			conn->udp_client_status = LWIPERF_UDP_CLIENT_STOPPING;
			conn->time_ended = sys_now();
			sys_arch_msleep(10);
		}

		/* copy data to pbuf */
		pbuf_take_at(p, (char*)(lwiperf_txbuf_const+LWIPERF_UDP_CLIENT_DATA_OFFSET),
		udp_data_length-LWIPERF_UDP_CLIENT_DATA_OFFSET, LWIPERF_UDP_CLIENT_DATA_OFFSET);

		/* update iperf header information */
		MEMCPY(p->payload, &pcount, 4);
		MEMCPY(p->payload+4, &tv_sec, 4);
		MEMCPY(p->payload+8, &tv_usec, 4);
		MEMCPY(p->payload+12, &flags, 4);
		MEMCPY(p->payload+16, &numThreads, 4);
		MEMCPY(p->payload+20, &listen_port, 4);
		MEMCPY(p->payload+24, &bufferlen, 4);
		MEMCPY(p->payload+28, &mUDPRate, 4);
		MEMCPY(p->payload+32, &mAmount, 4);

		/* send udp data */
		err = udp_send(conn->udp_conn_pcb , p);
		if (err != ERR_OK) {
			A("err : %d\n", err);
			lwiperf_udp_close(conn, LWIPERF_UDP_ABORTED_LOCAL_TXERROR);
		}
		/* release pbuf */
		if(p->ref != 0){
			pbuf_free(p);
		}
	}
 	return err;
}

static void
iperf_udp_client_send_thread(void *arg)
{
	err_t err;

	lwiperf_state_tcp_t* conn = (lwiperf_state_tcp_t*)arg;
	u16_t listen_port = LWIPERF_UDP_PORT_DEFAULT;
	int waiting_count = 0;
	int send_more = 1;

	while(1) {
		/* this session is time-limited */
		u32_t now = sys_now();
		u32_t diff_ms = now - conn->time_started;
		u32_t time = (u32_t)-(s32_t)lwip_htonl(conn->settings.amount);
		u32_t time_ms = time * 10;
		if (diff_ms >= time_ms || conn->force_stop == true  ||
		(wifi_softap_dhcps_status()== DHCP_STOPPED && wpa_driver_get_associate_status() == false) ){
			A("lwiperf_udp_client_send_more stop\n");
			send_more = 0;
		}
		err = lwiperf_udp_client_send_more(conn, send_more, listen_port, conn->duration, conn->bandwidth, conn->udp_data_length);
	        if (err != ERR_OK) {
	            E(TT_NET, "lwiperf_udp_client_send_more!! code[%d]\n", err);
	        }
		conn->bytes_transferred += conn->udp_data_length;

		sys_arch_msleep(conn->time_delay);

		if(send_more == 0)
			break;
	};

	while(conn->udp_client_status != LWIPERF_UDP_CLIENT_STOP){
		sys_arch_msleep(10);
		if(waiting_count++ > 10){
			A("Not recieve report packet from server!!\n");
			break;
		}
	}
	lwiperf_udp_close(conn, LWIPERF_UDP_DONE_CLIENT);
	vTaskDelete(conn->iperf_thread.thread_handle);
}

uint16_t iperf_udp_client_time_delay(u32_t bandwidth, u32_t data_size)
{
	const float  time_correction_ratio = 0.99;
	u16_t time_delay = ((BYTES_TO_BITS*data_size* MILI_PER_SECOND) / bandwidth) * time_correction_ratio;

	lwiperf_mutex_lock();
	A("iperf udp client start: packet delay = %d ms, bandwidth = %d  data_size = %d\n" , time_delay ,bandwidth, data_size);
	lwiperf_mutex_unlock();
	return time_delay;
}

/** Start UDP connection to the server */
void*
lwiperf_start_udp_client(const ip_addr_t* remote_addr, u16_t remote_port,u32_t duration,
	lwiperf_report_fn report_fn, void* report_arg, u8_t tos, u32_t bandwidth, u32_t udp_data_length, enum lwiperf_client_type type)
{
	err_t err;
	struct netif *netif = NULL;

	lwiperf_state_tcp_t* client_conn;
	struct udp_pcb* newpcb;
	u16_t listen_port = LWIPERF_UDP_PORT_DEFAULT;
	u16_t remote_port_temp = remote_port;
	lwiperf_settings_t settings;

	memset(&settings, 0, sizeof(settings));
	switch (type) {
		case LWIPERF_CLIENT:
			/* Unidirectional tx only test */
			settings.flags = 0;
			break;
		case LWIPERF_DUAL:
			/* Do a bidirectional test simultaneously */
			settings.flags = htonl(LWIPERF_FLAGS_ANSWER_TEST | LWIPERF_FLAGS_ANSWER_NOW);
			break;
		case LWIPERF_TRADEOFF:
			/* Do a bidirectional test individually */
			settings.flags = htonl(LWIPERF_FLAGS_ANSWER_TEST);
			break;
		default:
			/* invalid argument */
			return NULL;
	}
	settings.num_threads = htonl(1);
	settings.remote_port = htonl(remote_port);
	settings.amount = htonl((u32_t)(100*duration)*(-1));

	client_conn = (lwiperf_state_tcp_t*)LWIPERF_ALLOC(lwiperf_state_tcp_t);
	if (client_conn == NULL) {
		E(TT_NET, "Unable to allocate to client_conn\r\n");
		return NULL;
	}

	newpcb = udp_new();
	if (newpcb == NULL) {
		LWIPERF_FREE(lwiperf_state_tcp_t, client_conn);
		E(TT_NET, "Unable to create udp pcb\r\n");
		return NULL;
	}

	memset(client_conn, 0, sizeof(lwiperf_state_tcp_t));
	client_conn->udp_client_status = LWIPERF_UDP_CLIENT_IDLE;

	err = udp_bind(newpcb, IP_ADDR_ANY, 0);
	if (err != ERR_OK) {
		E(TT_NET, "Unable to bind to port %d: err = %d\r\n", remote_port, err);
		return NULL;
	}

	udp_recv(newpcb, udp_receive_callback, client_conn);

	err = udp_connect(newpcb, remote_addr, remote_port_temp);
	if (err != ERR_OK) {
		E(TT_NET, "Unable to connect to port %d: err = %d\r\n", remote_port_temp, err);
		return NULL;
	}
	sys_arch_msleep(500);

	if (newpcb->netif_idx != NETIF_NO_INDEX) {
		netif = netif_get_by_index(newpcb->netif_idx);
	} else {
		/* check if we have a route to the remote host */
		netif = ip_route(&newpcb->local_ip, &newpcb->remote_ip);
	}
	if (netif == NULL) {
		/* Don't even try to send a SYN packet if we have no route since that will fail. */
		return NULL;
	}

	/* check if local IP has been assigned to pcb, if not, get one */
	if (ip_addr_isany(&newpcb->local_ip)) {
		const ip_addr_t *local_ip = ip_netif_get_local_ip(netif, ipaddr);
		if (local_ip == NULL) {
			return NULL;
		}
		ip_addr_copy(newpcb->local_ip, *local_ip);
	}

	newpcb->tos = tos;
	client_conn->tos = tos;
	client_conn->base.tcp = 0;
	client_conn->base.server = 0;
	client_conn->base.local_port = newpcb->local_port;
	client_conn->base.remote_port = remote_port;
	ip4_addr_copy(client_conn->base.local_addr, newpcb->local_ip);
	ip4_addr_copy(client_conn->base.remote_addr, *remote_addr);
	client_conn->base.conn = client_conn;
	client_conn->bandwidth = bandwidth ;
	client_conn->udp_data_length = udp_data_length ;
	client_conn->duration = duration ;
	client_conn->udp_conn_pcb = newpcb;
	client_conn->report_fn = report_fn;
	client_conn->report_arg = report_arg;
	client_conn->bytes_transferred = 0;
	client_conn->udp_conn_pcb->remote_port = remote_port;
	client_conn->total_packets = 0;
	client_conn->outoforder_packets = 0;
	client_conn->cnt_error = 0;
	client_conn->udp_client_status = LWIPERF_UDP_CLIENT_START;
	client_conn->time_delay = iperf_udp_client_time_delay(bandwidth, udp_data_length);
	memcpy(&client_conn->settings, &settings, sizeof(settings));

	sys_arch_msleep(500);

	client_conn->time_started = sys_now();
	client_conn->time_ended = 0;
	client_conn->iperf_thread = sys_thread_new("iperf_thread", iperf_udp_client_send_thread, \
			client_conn, LWIP_IPERF_TASK_STACK_SIZE, LWIP_IPERF_TASK_PRIORITY);

	lwiperf_list_add(&client_conn->base);
	return client_conn;
}

/** Start UDP connection to the server on the default UDP port (5001) */
void*
lwiperf_start_udp_client_default(lwiperf_report_fn report_fn, void* report_arg, ip_addr_t *addr,
	u16_t port, u32_t duration, u8_t tos, u32_t bandwidth, u32_t udp_data_length,  enum lwiperf_client_type type)
{
	return lwiperf_start_udp_client(addr, port, duration, report_fn, report_arg, tos, bandwidth, udp_data_length, type);
}

void lwiperf_udp_server_close(lwiperf_state_tcp_t* conn)
{
	lwiperf_list_remove(&conn->base);
	if(conn->udp_conn_pcb != NULL)
		udp_remove(conn->udp_conn_pcb);

	if(conn != NULL)
		LWIPERF_FREE(lwiperf_state_tcp_t, conn);
}

void lwiperf_mutex_init(void)
{
	if(xMutex_iperf == NULL)
		xMutex_iperf = xSemaphoreCreateMutex();
}

void lwiperf_mutex_lock(void)
{
	xSemaphoreTake(xMutex_iperf, portMAX_DELAY);
}

void lwiperf_mutex_unlock(void)
{
	 xSemaphoreGive(xMutex_iperf);
}
#endif /* NRC_LWIP */

#endif /* LWIP_IPERF */
