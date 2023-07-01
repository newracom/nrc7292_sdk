/**
 * @file
 * Ping sender module
 *
 */

/*
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
 */

/**
 * This is an example of a "ping" sender (with raw API and socket API).
 * It can be used as a start point to maintain opened a network connection, or
 * like a network "watchdog" for your device.
 *
 */
#include "system.h"

#include "lwip/opt.h"

#if LWIP_RAW /* don't build if not configured for use in lwipopts.h */

#include "apps/ping/ping.h"
#include "lwip/sockets.h"

#include "lwip/mem.h"
#include "lwip/raw.h"
#include "lwip/icmp.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "lwip/inet_chksum.h"
#include "lwip/prot/ip4.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "nrc_lwip.h"
#include "os.h"
#include "util_string.h"

#include "ping_task.h"

#if LWIP_PING
/**
 * PING_DEBUG: Enable debugging for PING.
 */
#ifndef PING_DEBUG
#define PING_DEBUG     LWIP_DBG_ON
#endif

/** ping receive timeout - in milliseconds */
#ifndef PING_RCV_TIMEO
#define PING_RCV_TIMEO 1000
#endif

/** ping result action - no default action */
#ifndef PING_RESULT
#define PING_RESULT(ping_ok)
#endif

static ping_parm_t* ping_all_connections;

/** Report a ping test result */
static void
ping_statistics_report(void *arg)
{
	ping_parm_t* ping_info = (ping_parm_t*)arg;
	char avg_delay_str[8];
	float avg_delay =((double)ping_info->total_delay/(double)ping_info->success);
	ftoa(avg_delay,avg_delay_str, 2);

	ping_mutex_lock();
	LWIP_DEBUGF( PING_DEBUG, ("\n---------------------------- Ping Statistics --------------------------------\n"));
	LWIP_DEBUGF( PING_DEBUG, (" Target Address : "));
	ip_addr_debug_print(PING_DEBUG, &(ping_info->addr));
	LWIP_DEBUGF( PING_DEBUG, ("\n"));
	LWIP_DEBUGF( PING_DEBUG, (" %d packets transmitted, %d received, %d%% packet loss, %s ms average delay",\
		 (int)ping_info->count, (int)ping_info->success, (int)((ping_info->count-ping_info->success)*100/ping_info->count), \
		((ping_info->success == 0) ?  "0":avg_delay_str)));
	LWIP_DEBUGF( PING_DEBUG, ("\n-----------------------------------------------------------------------------\n"));
	ping_mutex_unlock();
}

/** Initialize ping test parameters */

static void
ping_parameters_init(void *arg)
{
	ping_parm_t* ping_info = (ping_parm_t*)arg;

	ping_info->seq_num = 0;
	ping_info->time = 0;
	ping_info->count = 0;
	ping_info->success = 0;
	ping_info->total_delay = 0;
	/* unique ping ID for each session by taking thread_handle pointer */
	ping_info->id = (uintptr_t) ping_info->ping_thread.thread_handle & 0xFFFF;
}

/** Prepare a echo ICMP request */
static void
ping_prepare_echo(void *arg, struct icmp_echo_hdr *iecho, u16_t len)
{
	ping_parm_t* ping_info = (ping_parm_t*)arg;
	size_t i;
	size_t data_len = len - sizeof(struct icmp_echo_hdr);

	ICMPH_TYPE_SET(iecho, ICMP_ECHO);
	ICMPH_CODE_SET(iecho, 0);
	iecho->chksum = 0;
	iecho->id     = ping_info->id;
	iecho->seqno  = htons(++ping_info->seq_num);

	/* fill the additional data buffer with some data */
	for(i = 0; i < data_len; i++) {
		((char*)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)i;
	}

	iecho->chksum = inet_chksum(iecho, len);
}

/* Ping using the socket ip */
static err_t
ping_send(void *arg, int s, ip_addr_t *addr)
{
	ping_parm_t* ping_info = (ping_parm_t*)arg;
	int err;
	struct icmp_echo_hdr *iecho;
	struct sockaddr_storage to;
	size_t ping_size = sizeof(struct icmp_echo_hdr) + ping_info->packet_size;
	LWIP_ASSERT("ping_size is too big", ping_size <= 0xffff);

#if LWIP_IPV6
	if(IP_IS_V6(addr) && !ip6_addr_isipv4mappedipv6(ip_2_ip6(addr))) {
		/* todo: support ICMP6 echo */
		return ERR_VAL;
	}
#endif /* LWIP_IPV6 */

	iecho = (struct icmp_echo_hdr *)mem_malloc((mem_size_t)ping_size);
	if (!iecho) {
		return ERR_MEM;
	}

	ping_prepare_echo(ping_info, iecho, (u16_t)ping_size);

#if LWIP_IPV4
	if(IP_IS_V4(addr)) {
		struct sockaddr_in *to4 = (struct sockaddr_in*)&to;
		to4->sin_len    = sizeof(to4);
		to4->sin_family = AF_INET;
		inet_addr_from_ip4addr(&to4->sin_addr, ip_2_ip4(addr));
	}
#endif /* LWIP_IPV4 */

	err = lwip_sendto(s, iecho, ping_size, 0, (struct sockaddr*)&to, sizeof(to));

	mem_free(iecho);

	return (err ? ERR_OK : ERR_VAL);
}

static void
ping_recv(void *arg, int s)
{
	ping_parm_t* ping_info = (ping_parm_t*)arg;
	char buf[64];
	int len;
	char from_str[INET_ADDRSTRLEN];

	ping_info->time_delay=0;

	while ((len = lwip_recv(s, buf, sizeof(buf), 0)) > 0) {
		if (len >= (int)(sizeof(struct ip_hdr)+sizeof(struct icmp_echo_hdr))) {
			struct ip_hdr *iphdr;
			struct icmp_echo_hdr *iecho;

			iphdr = (struct ip_hdr *)buf;
			iecho = (struct icmp_echo_hdr *)(buf + (IPH_HL(iphdr) * 4));
			if ((iecho->id == ping_info->id) && (iecho->seqno == lwip_htons(ping_info->seq_num))) {
				ping_info->time_delay = (sys_now() - ping_info->time);
				inet_ntop(AF_INET, &ping_info->addr, from_str, INET_ADDRSTRLEN);
				ping_mutex_lock();
				LWIP_DEBUGF( PING_DEBUG, ("ping: recv %s", from_str));
				LWIP_DEBUGF( PING_DEBUG, (" [%d] ", ping_info->seq_num));
				LWIP_DEBUGF( PING_DEBUG, (" %"U32_F" ms\n", ping_info->time_delay));
				ping_mutex_unlock();

				/* do some ping result processing */
				PING_RESULT((ICMPH_TYPE(iecho) == ICMP_ER));
				ping_info->total_delay += ping_info->time_delay;
				ping_info->success++;
				return;
			}
		}
	}

	if (len == 0) {
		ping_mutex_lock();
		LWIP_DEBUGF( PING_DEBUG, ("ping: recv - %"U32_F" ms - timeout\n", (sys_now()-ping_info->time)));
		ping_mutex_unlock();
	}

	/* do some ping result processing */
	PING_RESULT(0);
}

void ping_thread(void *arg)
{
	int s;
	int ret;
	int timeout = PING_RCV_TIMEO;
	ping_parm_t* ping_info = (ping_parm_t*)arg;

	s = lwip_socket(AF_INET,  SOCK_RAW, IP_PROTO_ICMP);

	if (s < 0) {
		ping_deinit(ping_info);
		return;
	}

	ret = lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	LWIP_ASSERT("setting receive timeout failed", ret == 0);
	LWIP_UNUSED_ARG(ret);

	ping_parameters_init(ping_info);
	ping_list_add(ping_info);

	while (1) {
		if (ping_send(ping_info, s, &(ping_info->addr)) == ERR_OK) {
			ping_mutex_lock();
			LWIP_DEBUGF( PING_DEBUG, ("ping: send "));
			ip_addr_debug_print(PING_DEBUG, &(ping_info->addr));
			LWIP_DEBUGF( PING_DEBUG, ("\n"));
			ping_mutex_unlock();
			ping_info->time = sys_now();
			ping_recv(ping_info, s);
		} else {
			ping_mutex_lock();
			LWIP_DEBUGF( PING_DEBUG, ("ping: send "));
			ip_addr_debug_print(PING_DEBUG, &(ping_info->addr));
			LWIP_DEBUGF( PING_DEBUG, (" - error\n"));
			ping_mutex_unlock();
		}

		if((++ping_info->count  ==  ping_info->target_count) || \
			(ping_info->force_stop == true)){
			break;
		}else{
			sys_msleep( ping_info->interval);
		}
	}
	lwip_close(s);
	ping_statistics_report(ping_info);
	ping_list_remove(ping_info);
	ping_deinit(ping_info);
}

#endif /* LWIP_RAW */

#endif /* LWIP_PING */
