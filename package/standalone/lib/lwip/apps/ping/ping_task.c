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

#include "FreeRTOS.h"
#include "semphr.h"

#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/stats.h"

#include "arch/sys_arch.h"
#include "ping.h"

static xSemaphoreHandle xMutex = NULL;
static ping_parm_t* ping_all_connections;

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

sys_thread_t
ping_init(void *arg)
{
	char taskName[16] = {0,};
	ping_parm_t* ping_info = (ping_parm_t*)arg;
	sprintf(taskName, "ping_thread");
	if(IP_IS_V4(&(ping_info->addr))) {
		return sys_thread_new(taskName, ping_thread, ping_info, LWIP_PING_TASK_STACK_SIZE, LWIP_PING_TASK_PRIORITY);
	} else {
		return sys_thread_new(taskName, ping6_thread, ping_info, LWIP_PING_TASK_STACK_SIZE, LWIP_PING_TASK_PRIORITY);
	}
}

void
ping_deinit(void *arg)
{
	ping_parm_t* ping_info = (ping_parm_t*)arg;
	xTaskHandle destroy_handle = ping_info->ping_thread.thread_handle;
	ping_info->ping_thread.thread_handle = NULL;
	mem_free(ping_info);
	vTaskDelete(destroy_handle);
}

/** Add an ping session to the 'active' list */
void ping_list_add(ping_parm_t* item)
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
void ping_list_remove(ping_parm_t* item)
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

ping_parm_t *ping_get_session(ip_addr_t* addr)
{
	ping_parm_t* iter;

	for (iter = ping_all_connections; iter != NULL;iter = iter->next) {
		if(ip_addr_cmp(&(iter->addr),addr)){
			return iter;
		}
	}
	return NULL;
}

void ping_list_display(void)
{
	ping_parm_t *ptr = ping_all_connections;
	ip_addr_t fromaddr;

	if(ptr == NULL){
		ping_mutex_lock();
		LWIP_PLATFORM_DIAG(  ("No ping application is running\n"));
		ping_mutex_unlock();
		return;
	}

	ping_mutex_lock();
	LWIP_PLATFORM_DIAG(  ("\n-------------------------- Ping running Status ------------------------------\n"));
	while(ptr != NULL) {
		fromaddr = ptr->addr;
		ip_addr_debug_print_val(PING_DEBUG, fromaddr);
		LWIP_PLATFORM_DIAG(  ("\n"));
		ptr = ptr->next;
	}
	LWIP_PLATFORM_DIAG(  ("-----------------------------------------------------------------------------\n"));
	ping_mutex_unlock();
}
