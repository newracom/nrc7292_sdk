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

#include "nrc_lwip.h"
#include "nrc_iperf.h"
#include "nrc_iperf_tcp.h"
#include "nrc_iperf_udp.h"

#ifdef SUPPORT_ETHERNET_ACCESSPOINT
#include "nrc_eth_if.h"
#endif /* SUPPORT_ETHERNET_ACCESSPOINT */

#define CONFIG_TASK_PRIORITY_IPERF		LWIP_IPERF_TASK_PRIORITY
#define CONFIG_TASK_STACKSIZE_IPERF		LWIP_IPERF_TASK_STACK_SIZE

#define IPERF_SUPPORT_MULTIPLE_PORT 0

extern void sys_arch_msleep(u32_t delay_ms);

/********************************************************************************/

#define WARN_ON(condition) \
({ \
	 int warn_on = !!(condition); \
	 if(warn_on) \
		 CPA("[WARN] %s at %s:%d\n", __FUNCTION__, __FILE__, __LINE__); \
	 warn_on; \
 })

typedef struct mutex
{
	SemaphoreHandle_t handle;
} spinlock_t;

static void mutex_init (struct mutex *m)
{
	SemaphoreHandle_t handle = xSemaphoreCreateMutex();

	if (!WARN_ON(!m || !handle))
		m->handle = handle;
}

static void mutex_lock (struct mutex *m)
{
	if (!WARN_ON(!m || !m->handle))
		WARN_ON(xSemaphoreTake(m->handle, portMAX_DELAY) !=pdTRUE);
}

static void mutex_unlock (struct mutex *m)
{
	if (!WARN_ON(!m || !m->handle))
		WARN_ON(xSemaphoreGive(m->handle) !=pdTRUE);
}

/********************************************************************************/

struct list_head
{
        struct list_head *next, *prev;
};

static void INIT_LIST_HEAD (struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

static void __list_add (struct list_head *new, struct list_head *prev, struct list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static void list_add (struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}

static void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}

static void __list_del (struct list_head * prev, struct list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

static void __list_del_entry (struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
}

static void list_del (struct list_head *entry)
{
	__list_del_entry(entry);
	entry->next = entry;
	entry->prev = entry;
}

#define list_entry(ptr, type, member)		container_of(ptr, type, member)
#define list_first_entry(ptr, type, member) list_entry((ptr)->next, type, member)
#define list_next_entry(pos, member) 		list_entry((pos)->member.next, typeof(*(pos)), member)
#define list_for_each_entry_safe(pos, n, head, member)       \
	for (pos = list_first_entry(head, typeof(*pos), member), \
			n = list_next_entry(pos, member);                \
			&pos->member != (head);                          \
			pos = n, n = list_next_entry(n, member))

/********************************************************************************/
#if !defined(DISABLE_IPERF_APP)
#if defined(LWIP_IPERF) && (LWIP_IPERF == 1)

static spinlock_t iperf_lock = {
	.handle = NULL
};

static struct list_head iperf_head = {
	.next = NULL,
	.prev = NULL
};

struct iperf_task {
	iperf_opt_t* option;
	struct list_head list;
};

extern struct netif* nrc_netif[MAX_IF];
#if LWIP_BRIDGE
extern struct netif br_netif;
#endif /* LWIP_BRIDGE */
#ifdef SUPPORT_ETHERNET_ACCESSPOINT
extern struct netif eth_netif;
#endif /* SUPPORT_ETHERNET_ACCESSPOINT */

static const char *module_name()
{
	return "iperf : ";
}

static const struct {
	const char *name;
	int value;
} ipqos[] = {
	{ "af11", IPTOS_DSCP_AF11 },
	{ "af12", IPTOS_DSCP_AF12 },
	{ "af13", IPTOS_DSCP_AF13 },
	{ "af21", IPTOS_DSCP_AF21 },
	{ "af22", IPTOS_DSCP_AF22 },
	{ "af23", IPTOS_DSCP_AF23 },
	{ "af31", IPTOS_DSCP_AF31 },
	{ "af32", IPTOS_DSCP_AF32 },
	{ "af33", IPTOS_DSCP_AF33 },
	{ "af41", IPTOS_DSCP_AF41 },
	{ "af42", IPTOS_DSCP_AF42 },
	{ "af43", IPTOS_DSCP_AF43 },
	{ "cs0", IPTOS_DSCP_CS0 },
	{ "cs1", IPTOS_DSCP_CS1 },
	{ "cs2", IPTOS_DSCP_CS2 },
	{ "cs3", IPTOS_DSCP_CS3 },
	{ "cs4", IPTOS_DSCP_CS4 },
	{ "cs5", IPTOS_DSCP_CS5 },
	{ "cs6", IPTOS_DSCP_CS6 },
	{ "cs7", IPTOS_DSCP_CS7 },
	{ "ef", IPTOS_DSCP_EF },
	{ "lowdelay", IPTOS_LOWDELAY },
	{ "throughput", IPTOS_THROUGHPUT },
	{ "reliability", IPTOS_RELIABILITY },
	{ NULL, -1 }
};

static int parse_qos(const char *cp)
{
	unsigned int i;
	char *ep = NULL;
	long val;

	if (cp == NULL)
		return -1;
	for (i = 0; ipqos[i].name != NULL; i++) {
		if (strcasecmp(cp, ipqos[i].name) == 0)
			return ipqos[i].value;
	}
	/* Try parsing as an integer */
	val = strtol(cp, &ep, 0);
	if (*cp == '\0' || *ep != '\0' || val < 0 || val > 255)
		return -1;
	return val;
}

int iperf_get_time (iperf_time_t *time)
{
	*time = system_get_time() / 1000.0;

	return 0;
}

bool iperf_time_expried(iperf_time_t start_time, iperf_time_t duration)
{
	iperf_time_t now;
	iperf_get_time(&now);

	if(now >= start_time+duration)
		return true;
	else
		return false;
}

uint32_t byte_to_bps (iperf_time_t time, uint32_t byte)
{
	iperf_time_t divide_time = (time == 0.0) ? (1.0) : time;
	return (8 * byte) / divide_time;
}

char *byte_to_string (uint32_t byte)
{
	static char buf[20];

	if (byte < 1024)
		snprintf(buf, sizeof(buf), "%lu ", byte);
	else if (byte < (1024 * 1024))
		snprintf(buf, sizeof(buf), "%4.2f K", byte / 1024.);
	else
		snprintf(buf, sizeof(buf), "%4.2f M", (byte / 1024.) / 1024.);

	return buf;
}

char *bps_to_string (uint32_t bps)
{
	static char buf[20];

	if (bps < 1000)
		snprintf(buf, sizeof(buf), "%lu ", bps);
	else if (bps < (1000 * 1000))
		snprintf(buf, sizeof(buf), "%4.2f K", bps / 1000.);
	else
		snprintf(buf, sizeof(buf), "%4.2f M", (bps / 1000.) / 1000.);

	return buf;
}

static void iperf_option_help (char *cmd)
{
	CPA("Usage: %s <-s|-c host> [options]\n", cmd);
	CPA("\r\n");
	CPA("Client/Server:\n");
	CPA("  -b, --bandwidth #[kmKM]  bandwidth to send at in bits/sec or packets per second\n");
	CPA("  -p, --port #          server port to listen on/connect to (default: %d)\n", IPERF_DEFAULT_SERVER_PORT);
	CPA("  -u, --udp             use UDP rather than TCP\n");
	CPA("  -S, --tos #           set the socket's IP_TOS (byte) field\n");
	CPA("  -i, --interval  #     seconds between periodic bandwidth reports\n");
	CPA("  -g, --sendInterval #    use TCP data send interval(ms)\n");
	CPA("  -N, --nodelay         set TCP no delay, disabling Nagle's Algorithm\n");
	CPA("\r\n");

	CPA("Server specific:\n");
	CPA("  -s, --server          run in server mode\n");
	CPA("\r\n");

	CPA("Client specific:\n");
	CPA("  -c, --client <host>   run in client mode, connecting to <host>\n");
	CPA("  -t, --time #          time in seconds to transmit for (default: %d sec)\n", IPERF_DEFAULT_SEND_TIME);
	CPA("\r\n");

	CPA("Miscellaneous:\n");
	CPA("  -h, --help            print this message and quit\n");
	CPA("\r\n");
}

#if 0
static void iperf_option_print (iperf_opt_t *option)
{

	CPA("[ IPERF OPTION ]\n");
	CPA(" - Role: %s\n", (option->mThreadMode == kMode_Server)? "Server" :
		 (option->mThreadMode == kMode_Client)? "Client"  : "unknown");
	CPA(" - Protocol: %s\n", option->mUDP ? "UDP" : "TCP");
	CPA(" - mPort: %d\n", option->mPort);

	if (option->mThreadMode == kMode_Server){
		CPA(" - server_ip: %s\n", ip4addr_ntoa((const ip4_addr_t*)ip_2_ip4(&option->addr)));
	} else if (option->mThreadMode == kMode_Client){
		CPA(" - remote ip: %s\n", ip4addr_ntoa((const ip4_addr_t*)ip_2_ip4(&option->addr)));
	}

	if (option->mUDP) {
		CPA(" - Datagram_size: %ld\n", option->mBufLen);
		if (option->mThreadMode == kMode_Client)
			CPA(" - Data Rate[bps]: %ld\n", option->mAppRate);
	}
	CPA(" - Time: %ld [sec]\n",(option->mAmount)/100);
	CPA(" - TOS: %d\n", option->mTOS);
	CPA("\r\n\n");
}
#endif

iperf_opt_t * iperf_option_alloc(void)
{
	iperf_opt_t* option = NULL;
	int i = 0;

	while(1) {
		option = (iperf_opt_t *)mem_malloc(sizeof(iperf_opt_t));
		if(option)
			break;

		if(i == 50){
			CPA("%s memory allocation fail!\n", module_name());
			return NULL;
		} else {
			sys_arch_msleep(100);
			i++;
		}
	}
	memset(option, 0x0, sizeof(iperf_opt_t));
	return option;
}

void iperf_option_free(iperf_opt_t* option)
{
	if (option != NULL)
		mem_free(option);
}

void nrc_iperf_spin_lock_init(void)
{
	mutex_init(&iperf_lock);
}

void nrc_iperf_spin_lock(void)
{
	mutex_lock(&iperf_lock);
}

void nrc_iperf_spin_unlock(void)
{
	mutex_unlock(&iperf_lock);
}

int nrc_iperf_list_init(void)
{
	nrc_iperf_spin_lock_init();
	INIT_LIST_HEAD(&iperf_head);
	return 0;
}

void nrc_iperf_list_deinit(void)
{
	struct iperf_task *cur, *next;

	nrc_iperf_spin_lock();
	list_for_each_entry_safe(cur, next, &iperf_head, list) {
		list_del(&cur->list);
		if(cur) mem_free(cur);
	}
	nrc_iperf_spin_unlock();
}

static void nrc_iperf_list_print(void)
{
	struct iperf_task *cur, *next;

	list_for_each_entry_safe(cur, next, &iperf_head, list) {
		CPA("%s ", (cur->option->mUDP == 1) ?  "UDP" : "TCP");
		CPA("%s :", (cur->option->mThreadMode == kMode_Server) ?  "Server" : "Client");
		CPA("%s:%d\n", ip4addr_ntoa((const ip4_addr_t*)ip_2_ip4(&cur->option->addr)),cur->option->mPort);
	}
}

int nrc_iperf_task_list_add(iperf_opt_t* option)
{
	struct iperf_task *task;
	struct iperf_task *cur, *next;
	int i = 0;

	nrc_iperf_spin_lock();
	list_for_each_entry_safe(cur, next, &iperf_head, list) {
		if ((option->mThreadMode == cur->option->mThreadMode) &&
#if IPERF_SUPPORT_MULTIPLE_PORT
				(option->mPort == cur->option->mPort) &&
#endif
				(option->mUDP == cur->option->mUDP) &&
				(ip_addr_cmp(&cur->option->addr,&option->addr))) {
			nrc_iperf_spin_unlock();
			return -1;
		}
	}
	nrc_iperf_spin_unlock();

	while(1) {
		task = mem_malloc(sizeof(struct iperf_task));
		if(task)
			break;

		if(i == 50){
			CPA("%s memory allocation fail!\n", module_name());
			return -1;
		} else {
			sys_arch_msleep(100);
			i++;
		}
	}
	task->option = option;
	INIT_LIST_HEAD(&task->list);

	nrc_iperf_spin_lock();
	list_add_tail(&task->list, &iperf_head);
	nrc_iperf_spin_unlock();

	return 0;
}

int nrc_iperf_task_list_del(iperf_opt_t* option)
{
	struct iperf_task *cur, *next;
	int ret = -1;

	nrc_iperf_spin_lock();
	list_for_each_entry_safe(cur, next, &iperf_head, list) {
		if ((option->mThreadMode == cur->option->mThreadMode) &&
#if IPERF_SUPPORT_MULTIPLE_PORT
				(option->mPort == cur->option->mPort) &&
#endif
				(option->mUDP == cur->option->mUDP) &&
				(ip_addr_cmp(&cur->option->addr,&option->addr))) {
			list_del(&cur->list);
			if(cur) mem_free(cur);
			ret = 0;
			break;
		}
	}
	nrc_iperf_spin_unlock();

	return ret;
}

void nrc_iperf_periodic_report(void *pvParameters)
{
	iperf_opt_t *option = (iperf_opt_t *) pvParameters;
	iperf_time_t last_report = 0.0;
	uint64_t last_byte_count_reported = 0;
	uint64_t processing_start_time = 0;
	uint64_t processing_end_time = 0;
	uint64_t processing_time = 0;
	uint64_t processing_sleep_duration = 0;

	char peer_addr[INET_ADDRSTRLEN];

	while (1) {
		uint64_t byte = 0;
		uint32_t bps = 0;
		processing_start_time = system_get_time();

		if (last_report > 0.0) {
			iperf_time_t current_time = 0;
			if (option->mThreadMode == kMode_Server) {
				inet_ntop(AF_INET, &option->server_info.clientaddr.s2_data2, peer_addr, INET_ADDRSTRLEN);
				if (option->mUDP) {
					if (option->server_info.last_id < 0) {
						break;
					}
				}
				byte = option->server_info.recv_byte - last_byte_count_reported;
				last_byte_count_reported = option->server_info.recv_byte;
			} else if (option->mThreadMode == kMode_Client) {
				inet_ntop(AF_INET, (struct sockaddr_in *) &option->addr, peer_addr, INET_ADDRSTRLEN);
				if(option->mUDP == 1) {
					byte = (option->client_info.datagram_cnt * option->mBufLen) - last_byte_count_reported;
					last_byte_count_reported = option->client_info.datagram_cnt * option->mBufLen;
				} else {
					byte = (option->client_info.send_byte) - last_byte_count_reported;
					last_byte_count_reported = option->client_info.send_byte;
				}
			} else {
				break;
			}

			nrc_iperf_spin_lock();
			bps = (byte * 8)/(option->mInterval / 1000.0);
			CPA("[%15s][%c]  %4.1f - %4.1f sec  %7sBytes  %7sbits/sec\n",
			  peer_addr, (option->mThreadMode == kMode_Server) ? 'S' : 'C',
			  (last_report - option->mInterval) / 1000,
			  last_report / 1000,
			  byte_to_string(byte), bps_to_string(bps));
			nrc_iperf_spin_unlock();

			if (option->mThreadMode == kMode_Server) {
				if (option->server_info.send_time) {
					iperf_get_time(&current_time);
					if ((current_time - option->server_info.start_time) >= option->server_info.send_time) {
						break;
					}
				}

				if (option->server_info.send_byte) {
					if (option->server_info.recv_byte >= option->server_info.send_byte) {
						break;
					}
				}
			}
		}
		processing_end_time = system_get_time();
		processing_time = processing_end_time - processing_start_time;
		processing_sleep_duration = (option->mInterval > processing_time) ? (option->mInterval - processing_time) : 1;
		last_report += option->mInterval;

		if (ulTaskNotifyTake(pdTRUE, processing_sleep_duration) == pdTRUE) {
			break;
		}
	}
	xTaskNotifyGive(option->task_handle);
	vTaskDelete(NULL);
}

static iperf_opt_t* nrc_iperf_task_get(iperf_opt_t* option)
{
	struct iperf_task *cur, *next;
	iperf_opt_t* ret_opt = NULL;

	nrc_iperf_spin_lock();
	list_for_each_entry_safe(cur, next, &iperf_head, list) {
		if ((option->mThreadMode == cur->option->mThreadMode) &&
#if IPERF_SUPPORT_MULTIPLE_PORT
				(option->mPort == cur->option->mPort) &&
#endif
				(option->mUDP == cur->option->mUDP) &&
				(ip_addr_cmp(&cur->option->addr,&option->addr))) {
			ret_opt = cur->option;
			break;
		}
	}
	nrc_iperf_spin_unlock();
	return ret_opt;
}

static int check_destination_address(iperf_opt_t* option)
{
	int i=0;

	if(option->mThreadMode == kMode_Client){
#if LWIP_BRIDGE
		if(netif_is_up(&br_netif)) {
			if (ip_addr_cmp(&br_netif.ip_addr, &option->addr)) {
				return -1;
			}
		} else
#endif /* LWIP_BRIDGE */
#ifdef SUPPORT_ETHERNET_ACCESSPOINT
		if (ip4_addr_cmp(&eth_netif.ip_addr, &option->addr)) {
			return -1;
		}
#else
		for(i=0; i<MAX_IF; i++){
			if(ip_addr_cmp(&nrc_netif[i]->ip_addr,&option->addr)){
				return -1;
			}
		}
#endif /* SUPPORT_ETHERNET_ACCESSPOINT */
	}
	return 0;
}

static int iperf_start_session(iperf_opt_t* option, void *report_cb)
{
	BaseType_t task_ret;

	if(check_destination_address(option) < 0){
		CPA("%s Destination address is own address\n",	module_name());
		return -1;
	}

	if(nrc_iperf_task_get(option)){
		CPA("Failed : %s %s : %s\n", (option->mUDP == 1) ?  "UDP" : "TCP",
				(option->mThreadMode == kMode_Server) ?  "Server" : "Client",
				(ip4addr_ntoa((const ip4_addr_t*)ip_2_ip4(&option->addr))));
		return -1;
	}

	if(option->mThreadMode == kMode_Server){
		if(option->mUDP){
			task_ret = xTaskCreate(iperf_udp_server, "iperf_udp_server_task",
				CONFIG_TASK_STACKSIZE_IPERF, (void*)option, CONFIG_TASK_PRIORITY_IPERF, &option->task_handle);
		}else{
			task_ret = xTaskCreate(iperf_tcp_server, "iperf_tcp_server_task",
				CONFIG_TASK_STACKSIZE_IPERF, (void*)option, CONFIG_TASK_PRIORITY_IPERF, &option->task_handle);
		}
	}else{
		if(option->mUDP){
			task_ret = xTaskCreate(iperf_udp_client, "iperf_udp_client_task",
				CONFIG_TASK_STACKSIZE_IPERF, (void*)option, CONFIG_TASK_PRIORITY_IPERF, &option->task_handle);
		}else{
			task_ret = xTaskCreate(iperf_tcp_client, "iperf_tcp_client_task",
				CONFIG_TASK_STACKSIZE_IPERF, (void*)option, CONFIG_TASK_PRIORITY_IPERF, &option->task_handle);
		}
	}

	nrc_iperf_spin_lock();
	CPA("%s %s %s : %s", __func__,(option->mUDP) ? "udp":"tcp",
		(option->mThreadMode == kMode_Server)?"server":"client" ,
		(task_ret == pdFAIL) ? "Failed!\n" : "Started!\n" );
	nrc_iperf_spin_unlock();

	return (task_ret == pdFAIL) ? -1: 0;
}

static int iperf_stop_session(iperf_opt_t* option)
{
	iperf_opt_t* del_task = NULL;
	del_task = nrc_iperf_task_get(option);
	if(del_task){
		del_task->mForceStop = true;
	}
	return true;
}

static int iperf_option_parse (int argc, char *argv[], iperf_opt_t *option)
{
	int i;
	int val = 0;
	char *str = NULL;
#if LWIP_IPV4
	ip4_addr_t ip4;
#endif
#if LWIP_IPV6
	ip6_addr_t ip6;
#endif
	ip_addr_t ip;

	for(i = 1 ; i < argc ; i++) {
		str = argv[i];

		if(str == NULL || str[0] == '\0'){
			break;
		}else if(strcmp(str, "-u") == 0){
			option->mUDP = true;
		}else if(strcmp(str, "-s") == 0){
			option->mThreadMode = kMode_Server;
		}else if(strcmp(str, "-c") == 0){
			option->mThreadMode  = kMode_Client;
			str = argv[++i];
			if(!ipaddr_aton(str, &option->addr)){
				E(TT_NET, "%s error!! unknown address value\n",
					module_name());
				return -2;
			}
		}else if(strcmp(str, "-t") == 0){
			str = argv[++i];
			option->mAmount = atoi(str)*100;
		}else if (strcmp(str, "-i") == 0){
			str = argv[++i];
			option->mInterval = atoi(str)*1000; // interval in ms
		}else if(strcmp(str, "-b") == 0){
			double value = 0.0;
			int len = 0;
			char suffix = '\0';
			str = argv[++i];

			len = strlen(str);
			if(str[len-1] == 'm' || str[len-1] == 'M'||str[len-1] == 'k'|| str[len-1] == 'K') {
				if(len > 1) {
					suffix = str[len-1];
					str[len-1] = '\0';
					value = atof(str);
				}
			} else {
				value = atof(str);
			}

			switch(suffix) {
				case 'm': case 'M':
					option->mAppRate = (value == 0.0) ? MAX_IPERF_THROUGHPUT : (value*MEGA);
					break;
				case 'k': case 'K':
					option->mAppRate = (value == 0.0) ? MAX_IPERF_THROUGHPUT : (value*KILO);
					break;
				default:
					option->mAppRate = (value == 0.0) ? MAX_IPERF_THROUGHPUT : value;
					break;
			}
		}else if(strcmp(str, "-p") == 0){
			str = argv[++i];
			option->mPort = atoi(str);
		}else if(strcmp(str, "-S") == 0){
			str = argv[++i];
			val = parse_qos(str);
			option->mTOS = (val>0) ? val : 0 ;
		}else if(strcmp(str, "-l") == 0){
			str = argv[++i];
			option->mBufLen = atoi(str);
		}else if(strcmp(str, "-g") == 0){
			str = argv[++i];
			option->mSendInterval = atoi(str);
		}else if(strcmp(str, "-N") == 0){
			option->mNodelay = true;
		}else if(strcmp(str , "stop") == 0) {
			option->mForceStop = true;
			return 0;
		}else if(strcmp(str, "-st") == 0){
			nrc_iperf_list_print();
			return -2;
		}else if(strcmp(str, "-h") == 0 || strcmp(str, "-help") == 0){
			iperf_option_help(argv[0]);
			return -2;
		}else{
			CPA("%s unknown options : %s\n",
				module_name(), str);
			return -1;
		}
	}
	return 0;
}

static void iperf_init_parameters(iperf_opt_t * option)
{
	option->mSock = -1;
	option->mBufLen       = IPERF_DEFAULT_DATA_BUF_LEN;
	option->mPort         = 5001;          // -p,  ttcp port
	option->mThreadMode   = kMode_Unknown; // -s,  or -c, none
	option->mAmount       = IPERF_DEFAULT_SEND_TIME*100;          // -t,  10 seconds
	option->mTOS          = 0;           // -S,  ie. don't set type of service
	option->mAppRate       = 1000*1000;          // -b,  1 Mbps
	option->mSendInterval        = 0;          // -g
	option->mNodelay        = false;          // -N
	option->task_handle    = NULL;
	memset(&option->addr, 0, sizeof(ip_addr_t));
}

int iperf_stop(iperf_opt_t* option)
{
	int i;
	if(option->mThreadMode == kMode_Server){
#ifdef SUPPORT_ETHERNET_ACCESSPOINT
#if LWIP_BRIDGE
		if(netif_is_up(&br_netif)){
			if(!ip4_addr_isany_val(*netif_ip4_addr(&br_netif))){
				ip_addr_copy(option->addr, br_netif.ip_addr);
				iperf_stop_session(option);
			}
		} else
#endif /* LWIP_BRIDGE */
		if(!ip4_addr_isany_val(eth_netif.ip_addr)){
				ip4_addr_copy(option->addr, eth_netif.ip_addr);
				iperf_stop_session(option);
		}
#else
		for(i=0; i<MAX_IF; i++){
			if(!ip4_addr_isany_val(*netif_ip4_addr(nrc_netif[i]))) {
				ip_addr_copy(option->addr, nrc_netif[i]->ip_addr);
				iperf_stop_session(option);
			}
		}
#endif /* SUPPORT_ETHERNET_ACCESSPOINT */
	}else{
		iperf_stop_session(option);
	}
	return 0;
}

int  iperf_run(int argc, char *argv[], void *report_cb)
{
	iperf_opt_t* option = NULL;
	int server_started = false;
	int ret = true;
	int i;

	if (!iperf_lock.handle && !iperf_head.next && !iperf_head.prev)
		nrc_iperf_list_init();

	// Make thread info structure & set default value
	option = iperf_option_alloc();
	if(option == NULL){
		CPA("iperf parameter init fail!!\n");
		return false;
	}
	iperf_init_parameters(option);

	ret = iperf_option_parse(argc, argv, option);
	if(ret <0){
		iperf_option_free(option);
		if(ret == -2) // '-st', '-h'
			return true;
		else
			return false;
	}

	if(option->mForceStop == true){
		iperf_stop(option);
		iperf_option_free(option);
		return true;
	}

	if(option->mThreadMode == kMode_Client){
		ret = iperf_start_session(option, report_cb);
		if(ret <0){
			iperf_option_free(option);
			return false;
		}
	}else {
		struct netif *target_if;
		for(i=0; i<END_INTERFACE; i++){
			target_if = nrc_netif_get_by_idx(i);
			if (!ip4_addr_isany_val(*netif_ip4_addr(target_if))) {
				if(server_started == true){
					iperf_opt_t* option2 =  iperf_option_alloc();
					memcpy(option2, option, sizeof(iperf_opt_t));
					option = option2;
				}
				ip_addr_copy(option->addr, target_if->ip_addr);
				ret = iperf_start_session(option, report_cb);
				if(ret < 0){
					if(server_started == true){
						iperf_option_free(option);
						return false;
					}
				}else {
					server_started = true;
				}
			}
		}
	}
	return true;
}

int iperf_send(int fd, const char *buf, int count,
	const struct sockaddr *to, socklen_t tolen)
{
	int r;
	int nleft = count;
	int flags = 0;
	int err_retry = 0;

	while (nleft > 0) {
		r = sendto(fd, buf, nleft, flags, to, tolen);
		if(r < 0){
			if (r == ERR_TIMEOUT || r == ERR_WOULDBLOCK) {
				if(err_retry++ < 3) {
					sys_arch_msleep(100);
				} else {
					return -1;
				}
			}
		} else {
			err_retry = 0;
			nleft -= r;
			buf += r;
		}
	}
	return count;
}

#endif /* LWIP_IPERF */
#endif /*!defined(DISABLE_IPERF_APP) */
