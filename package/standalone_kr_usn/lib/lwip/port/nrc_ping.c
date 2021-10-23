/*
 * MIT License
 *
 * Copyright (c) 2021 Newracom, Inc.
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

#include <string.h>
#include <stdbool.h>
#include "lwipopts.h"
#include "apps/ping/ping.h"
#include "util_trace.h"

#include "includes.h"
#include "nrc_wifi.h"
#include "lwip/netif.h"

#if LWIP_PING
static const char *module_name()
{
	return "ping : ";
}

extern struct netif* nrc_netif[MAX_IF];

const char ping_usage_str[] = "Usage: ping destination [-c count] [-i interval] [-s packetsize]\n"
                           "      `ping destination stop' for stopping\n"
                           "      `ping -st' for checking current ping applicatino status\n";

void ping_usage(void)
{
	A("%s\r\n", ping_usage_str);
}

ping_parm_t * ping_init_parameters(void)
{
	ping_parm_t* parm = NULL;

	parm = (ping_parm_t *)malloc(sizeof(ping_parm_t));
	if (parm == NULL) {
		E(TT_NET, "%s memory allocation fail!\n", module_name());
		return NULL;
	}

	parm->packet_size = PING_DATA_SIZE;
	parm->interval = PING_DELAY_DEFAULT;
	parm->target_count = PING_COUNT_DEFAULT;
	parm->count = 0;
	parm->time = 0 ;
	parm->success = 0;
	parm->total_delay = 0 ;
	parm->time_delay = 0 ;
	parm->seq_num = 0;
	parm->ping_thread.thread_handle = NULL;
	parm->force_stop = 0;
	ip4_addr_set_zero(&parm->addr);
	parm->next = NULL;

	return parm;
}

void ping_deinit_parameters(ping_parm_t* parm)
{
	if (parm != NULL) {
		free(parm);
	}
}

int  ping_run(int argc, char *argv[])
{
	ping_parm_t* ping_conn = NULL;
	ping_parm_t* ping_stop_conn = NULL;
	char *str = NULL;
	int ret = true;
	int i;

	if(argc ==  1 || strcmp(argv[1] , "-h") == 0) {
		ping_usage();
		return true;
	}

	if(strcmp(argv[1] , "-st") == 0) {
		ping_list_display();
		return true;
	}

	ping_conn = ping_init_parameters();
	if(ping_conn == NULL){
		E(TT_NET, "ping parameter init fail!!\n");
		return false;
	}

	if(!ip4addr_aton(argv[1], &ping_conn->addr)){
		E(TT_NET, "%s error!! unknown address value\n",
			module_name());
		ret = false;
		goto free_and_return;
	}

	if(strcmp(argv[2],  "stop") == 0){
		I(TT_NET, "%s ping stop : %s\n", module_name(), argv[1]);
		ret = true;
		goto ping_stop;
	}

#ifndef SUPPORT_ETHERNET_ACCESSPOINT
	for(i=0; i<MAX_IF; i++){
		if(ip4_addr_cmp(&nrc_netif[i]->ip_addr,&ping_conn->addr)){
			I(TT_NET, "%s ping address is own address\n",
			module_name());
			ret = false;
			goto free_and_return;
		}
	}
#endif

	if(ping_get_session(&ping_conn->addr) != NULL){
		I(TT_NET, "%s Ping application is already running\n",
			module_name());
		ret = false;
		goto free_and_return;
	}

	ping_mutex_init();

	for(i = 2 ; i < argc ; i++)
	{
		str = argv[i];

		if(str == NULL || str[0] == '\0'){
				break;
		}else if(strcmp(str, "-i") == 0){
			str = argv[++i];
			ping_conn->interval = atoi(str);
		}else if(strcmp(str, "-c") == 0){
			str = argv[++i];
			ping_conn->target_count = atoi(str);
		}else if(strcmp(str, "-s") == 0){
			str = argv[++i];
			ping_conn->packet_size = atoi(str);
		}else{
			E(TT_NET, "%s unknown options : %s\n",
				module_name(), str);
			ret =false;
			goto free_and_return;
		}
	}

	ping_conn->ping_thread = ping_init((void*)ping_conn);
	if(ping_conn->ping_thread.thread_handle == NULL){
		ret = false;
		goto free_and_return;
	}
	return true;

ping_stop:
	ping_stop_conn = ping_get_session(&ping_conn->addr);
	if(ping_stop_conn!= NULL){
		ping_stop_conn->force_stop = 1;
	}else{
		ret = false;
		I(TT_NET, "%s Nothing to stop\n", module_name());
	}

free_and_return:
	ping_deinit_parameters(ping_conn);
	return ret;
}
#endif /* LWIP_PING */
