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
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "nrc_wifi.h"
#include "os.h"
#include "util_string.h"

#if LWIP_PING
/**
 * PING_DEBUG: Enable debugging for PING.
 */
#ifndef PING_DEBUG
#define PING_DEBUG     LWIP_DBG_ON
#endif

/** ping target - should be a "ip_addr_t" */
#ifndef PING_TARGET
#define PING_TARGET   (netif_default?netif_default->gw:ip_addr_any)
#endif

/** ping receive timeout - in milliseconds */
#ifndef PING_RCV_TIMEO
#define PING_RCV_TIMEO 1000
#endif

/** ping identifier - must fit on a u16_t */
#ifndef PING_ID
#define PING_ID        0xAFAF
#endif

/** ping result action - no default action */
#ifndef PING_RESULT
#define PING_RESULT(ping_ok)
#endif

static ping_parm_t* ping_all_connections;
static xSemaphoreHandle xMutex = NULL;

void ping_mutex_init(void)
{
	if(xMutex == NULL)
		xMutex = xSemaphoreCreateMutex();
}

void ping_mutex_lock(void)
{
	xSemaphoreTake(xMutex, portMAX_DELAY);
}

void ping_mutex_unlock(void)
{
	 xSemaphoreGive(xMutex);
}

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
	iecho->id     = PING_ID;
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
	struct sockaddr_in to;
	size_t ping_size = sizeof(struct icmp_echo_hdr) + ping_info->packet_size;
	LWIP_ASSERT("ping_size is too big", ping_size <= 0xffff);

	iecho = (struct icmp_echo_hdr *)mem_malloc((mem_size_t)ping_size);
	if (!iecho) {
		return ERR_MEM;
	}

	ping_prepare_echo(ping_info, iecho, (u16_t)ping_size);

	to.sin_len = sizeof(to);
	to.sin_family = AF_INET;
	inet_addr_from_ip4addr(&to.sin_addr, addr);

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
	struct sockaddr_in from;
	socklen_t fromlen = sizeof(from);
	struct ip_hdr *iphdr;
	struct icmp_echo_hdr *iecho;

	ping_info->time_delay=0;

	while((len = lwip_recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr*)&from, (socklen_t*)&fromlen)) > 0) {
		if (len >= (int)(sizeof(struct ip_hdr)+sizeof(struct icmp_echo_hdr))) {
			iphdr = (struct ip_hdr *)buf;
			iecho = (struct icmp_echo_hdr *)(buf + (IPH_HL(iphdr) * 4));
			if ( ( iecho->id == PING_ID ) && ( iecho->seqno == htons(ping_info-> seq_num ) ) ){
				ping_info->time_delay = (sys_now() -ping_info-> time);
				ip_addr_t fromaddr;
				inet_addr_to_ip4addr(&fromaddr, &from.sin_addr);
				ping_mutex_lock();
				LWIP_DEBUGF( PING_DEBUG, ("ping: recv "));
				ip4_addr_debug_print_val(PING_DEBUG, fromaddr);
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

	ping_mutex_lock();
	LWIP_DEBUGF( PING_DEBUG, ("ping: recv "));
	ip4_addr_debug_print_val(PING_DEBUG, (ping_info->addr));
	LWIP_DEBUGF( PING_DEBUG, (" timeout\n"));
	ping_mutex_unlock();

	/* do some ping result processing */
	PING_RESULT(0);
}


/** Add an ping session to the 'active' list */
static void
ping_list_add(ping_parm_t* item)
{
	ping_parm_t* temp;
	if (ping_all_connections == NULL) {
		ping_all_connections = item;
		item->next = NULL;
	} else {
		temp = ping_all_connections;
		ping_all_connections = item;
		item->next = temp;
	}
}

/** Remove an ping session from the 'active' list */
static void
ping_list_remove(ping_parm_t* item)
{
	ping_parm_t* prev = NULL;
	ping_parm_t* iter;
	for (iter = ping_all_connections; iter != NULL; prev = iter, iter = iter->next) {
		if (iter == item) {
			if (prev == NULL) {
				ping_all_connections = iter->next;
			} else {
				prev->next = iter->next;
			}
			break;
		}
	}
}

ping_parm_t *
ping_get_session(ip4_addr_t* addr)
{
	ping_parm_t* iter;

	for (iter = ping_all_connections; iter != NULL;iter = iter->next) {
		if(ip4_addr_cmp(&(iter->addr),addr)){
			return iter;
		}
	}
	return NULL;
}

void ping_list_display(void) {
	ping_parm_t *ptr = ping_all_connections;

	if(ptr == NULL){
		ping_mutex_lock();
		system_printf("No ping application is running\n");
		ping_mutex_unlock();
		return;
	}

	ping_mutex_lock();
	system_printf("\n-------------------------- Ping running Status ------------------------------\n");
	while(ptr != NULL) {
		system_printf("%"U16_F".%"U16_F".%"U16_F".%"U16_F"\n",
			ip4_addr1_16(&ptr->addr), ip4_addr2_16(&ptr->addr), ip4_addr3_16(&ptr->addr), ip4_addr4_16(&ptr->addr));

		ptr = ptr->next;
	}
	system_printf("-----------------------------------------------------------------------------\n");
	ping_mutex_unlock();
}

static void
ping_thread(void *arg)
{
	int s;
	int timeout = PING_RCV_TIMEO;
	ping_parm_t* ping_info = (ping_parm_t*)arg;

	if ((s = lwip_socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP)) < 0) {
		ping_deinit(ping_info);
		if(!ping_info)
			vPortFree(ping_info);
		return;
	}

	lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
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

	if(!ping_info)
		vPortFree(ping_info);
}

sys_thread_t
ping_init(void *arg)
{
	char taskName[16] = {0,};
	ping_parm_t* ping_info = (ping_parm_t*)arg;
	sprintf(taskName, "ping_thread");
	return sys_thread_new(taskName, ping_thread, ping_info, \
	LWIP_PING_TASK_STACK_SIZE, LWIP_PING_TASK_PRIORITY);
}

void
ping_deinit(void *arg)
{
	ping_parm_t* ping_info = (ping_parm_t*)arg;
	xTaskHandle destroy_handle = ping_info->ping_thread.thread_handle;
	ping_info->ping_thread.thread_handle = NULL;
	vTaskDelete(destroy_handle);
}
#endif /* LWIP_RAW */

#endif /* LWIP_PING */
