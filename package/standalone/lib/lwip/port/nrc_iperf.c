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

#include "nrc_wifi.h"
#include "nrc_iperf.h"
#include "nrc_iperf_tcp.h"
#include "nrc_iperf_udp.h"


#define CONFIG_TASK_PRIORITY_IPERF		LWIP_IPERF_TASK_PRIORITY
#define CONFIG_TASK_STACKSIZE_IPERF		LWIP_IPERF_TASK_STACK_SIZE

/********************************************************************************/

#define WARN_ON(condition) \
({ \
	 int warn_on = !!(condition); \
	 if(warn_on) \
		 A("[WARN] %s at %s:%d\n", __FUNCTION__, __FILE__, __LINE__); \
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

#define spin_lock_init(lock)    mutex_init(lock)
#define spin_lock(lock)         mutex_lock(lock)
#define spin_unlock(lock)       mutex_unlock(lock)

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

#ifdef SUPPORT_ETHERNET_ACCESSPOINT
extern struct netif eth_netif;
#endif

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
	*time = xTaskGetTickCount();
	*time /= configTICK_RATE_HZ;

	return 0;
}

uint32_t byte_to_bps (iperf_time_t time, uint32_t byte)
{
	return (8 * byte) / time;
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
	A("Usage: %s <-s|-c host> [options]\n", cmd);
	A("\r\n");
	A("Client/Server:\n");
	A("  -b, --bandwidth #[kmKM]  bandwidth to send at in bits/sec or packets per second\n");
	A("  -p, --port #          server port to listen on/connect to (default: %d)\n", IPERF_DEFAULT_SERVER_PORT);
	A("  -u, --udp             use UDP rather than TCP\n");
	A("  -S, --tos #           set the socket's IP_TOS (byte) field\n");
	A("\r\n");

	A("Server specific:\n");
	A("  -s, --server          run in server mode\n");
	A("\r\n");

	A("Client specific:\n");
	A("  -c, --client <host>   run in client mode, connecting to <host>\n");
	A("  -t, --time #          time in seconds to transmit for (default: %d sec)\n", IPERF_DEFAULT_SEND_TIME);
	A("\r\n");

	A("Miscellaneous:\n");
	A("  -h, --help            print this message and quit\n");
	A("\r\n");
}

#if 0
static void iperf_option_print (iperf_opt_t *option)
{

	A("[ IPERF OPTION ]\n");
	A(" - Role: %s\n", (option->mThreadMode == kMode_Server)? "Server" :
		 (option->mThreadMode == kMode_Client)? "Client"  : "unknown");
	A(" - Protocol: %s\n", option->mUDP ? "UDP" : "TCP");
	A(" - mPort: %d\n", option->mPort);

	if (option->mThreadMode == kMode_Server){
		A(" - server_ip: %s\n", ip4addr_ntoa((const ip4_addr_t*)ip_2_ip4(&option->addr)));
	} else if (option->mThreadMode == kMode_Client){
		A(" - remote ip: %s\n", ip4addr_ntoa((const ip4_addr_t*)ip_2_ip4(&option->addr)));
	}

	if (option->mUDP) {
		A(" - Datagram_size: %ld\n", option->mBufLen);
		if (option->mThreadMode == kMode_Client)
			A(" - Data Rate[bps]: %ld\n", option->mAppRate);
	}
	A(" - Time: %ld [sec]\n",(option->mAmount)/100);
	A(" - TOS: %d\n", option->mTOS);
	A("\r\n\n");
}
#endif

iperf_opt_t * iperf_option_alloc(void)
{
	iperf_opt_t* option = NULL;

	option = (iperf_opt_t *)malloc(sizeof(iperf_opt_t));
	if (option == NULL) {
		A("%s memory allocation fail!\n", module_name());
		return NULL;
	}
	memset(option, 0x0, sizeof(iperf_opt_t));
	return option;
}

void iperf_option_free(iperf_opt_t* option)
{
	if (option != NULL)
		free(option);
}

int nrc_iperf_list_init(void)
{
	spin_lock_init(&iperf_lock);
	INIT_LIST_HEAD(&iperf_head);
	return 0;
}

void nrc_iperf_list_deinit(void)
{
	struct iperf_task *cur, *next;

	spin_lock(&iperf_lock);
	list_for_each_entry_safe(cur, next, &iperf_head, list) {
		list_del(&cur->list);
		free(cur);
	}
	spin_unlock(&iperf_lock);
}

static void nrc_iperf_list_print(void)
{
	struct iperf_task *cur, *next;

	spin_lock(&iperf_lock);
	list_for_each_entry_safe(cur, next, &iperf_head, list) {
		A("%s ", (cur->option->mUDP == 1) ?  "UDP" : "TCP");
		A("%s :", (cur->option->mThreadMode == kMode_Server) ?  "Server" : "Client");
		A("%s\n", ip4addr_ntoa((const ip4_addr_t*)ip_2_ip4(&cur->option->addr)));
	}
	spin_unlock(&iperf_lock);
}

int nrc_iperf_task_list_add(iperf_opt_t* option)
{
	struct iperf_task *task;
	struct iperf_task *cur, *next;

	spin_lock(&iperf_lock);
	list_for_each_entry_safe(cur, next, &iperf_head, list) {
		if ((option->mThreadMode == cur->option->mThreadMode) &&
				(option->mUDP == cur->option->mUDP) &&
				(ip4_addr_cmp(&cur->option->addr,&option->addr))) {
			spin_unlock(&iperf_lock);
			return -1;
		}
	}
	spin_unlock(&iperf_lock);

	task = malloc(sizeof(struct iperf_task));
	if (!task)
		return -1;

	task->option = option;
	INIT_LIST_HEAD(&task->list);

	spin_lock(&iperf_lock);
	list_add_tail(&task->list, &iperf_head);
	spin_unlock(&iperf_lock);

	return 0;
}

int nrc_iperf_task_list_del(iperf_opt_t* option)
{
	struct iperf_task *cur, *next;
	int ret = -1;

	spin_lock(&iperf_lock);
	list_for_each_entry_safe(cur, next, &iperf_head, list) {
		if ((option->mThreadMode == cur->option->mThreadMode) &&
				(option->mUDP == cur->option->mUDP) &&
				(ip4_addr_cmp(&cur->option->addr,&option->addr))) {
			list_del(&cur->list);
			free(cur);
			ret = 0;
			break;
		}
	}
	spin_unlock(&iperf_lock);

	return ret;
}


static iperf_opt_t* nrc_iperf_task_get(iperf_opt_t* option)
{
	struct iperf_task *cur, *next;
	iperf_opt_t* ret_opt = NULL;

	spin_lock(&iperf_lock);
	list_for_each_entry_safe(cur, next, &iperf_head, list) {
		if ((option->mThreadMode == cur->option->mThreadMode) &&
				(option->mUDP == cur->option->mUDP) &&
				(ip4_addr_cmp(&cur->option->addr,&option->addr))) {
			ret_opt = cur->option;
			break;
		}
	}
	spin_unlock(&iperf_lock);
	return ret_opt;
}

static int check_destination_address(iperf_opt_t* option)
{
	int i=0;

	if(option->mThreadMode == kMode_Client){
#ifndef SUPPORT_ETHERNET_ACCESSPOINT
		for(i=0; i<MAX_IF; i++){
			if(ip4_addr_cmp(&nrc_netif[i]->ip_addr,&option->addr)){
				return -1;
			}
		}
#else
		if (ip4_addr_cmp(&eth_netif.ip_addr, &option->addr)) {
			return -1;
		}
#endif
	}
	return 0;
}

static int iperf_start_session(iperf_opt_t* option, void *report_cb)
{
	if(check_destination_address(option) < 0){
		A("%s Destination address is own address\n",	module_name());
		return -1;
	}

	if(nrc_iperf_task_get(option)){
		A("Failed : %s %s : %s\n", (option->mUDP == 1) ?  "UDP" : "TCP",
				(option->mThreadMode == kMode_Server) ?  "Server" : "Client",
				(ip4addr_ntoa((const ip4_addr_t*)ip_2_ip4(&option->addr))));
		return -1;
	}

	if(option->mThreadMode == kMode_Server){
		if(option->mUDP){
			A("%s udp server\n", __func__);
			xTaskCreate(iperf_udp_server, "udp_server_task",
				CONFIG_TASK_STACKSIZE_IPERF, (void*)option, CONFIG_TASK_PRIORITY_IPERF, NULL);
		}else{
			A("%s tcp server\n", __func__);
			xTaskCreate(iperf_tcp_server, "tcp_server_task",
				CONFIG_TASK_STACKSIZE_IPERF, (void*)option, CONFIG_TASK_PRIORITY_IPERF, NULL);
		}
	}else{
		if(option->mUDP){
			A("%s udp client\n", __func__);
			xTaskCreate(iperf_udp_client, "udp_client_task",
				CONFIG_TASK_STACKSIZE_IPERF, (void*)option, CONFIG_TASK_PRIORITY_IPERF, NULL);
		}else{
			A("%s tcp client\n", __func__);
			xTaskCreate(iperf_tcp_client, "tcp_client_task",
				CONFIG_TASK_STACKSIZE_IPERF, (void*)option, CONFIG_TASK_PRIORITY_IPERF, NULL);
		}
	}
	return 0;
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
	int len = 0;
	char suffix = '\0';
	char *str = NULL;
	int ip4addr_aton_is_valid = false;

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
			ip4addr_aton_is_valid = ip4addr_aton(str, &option->addr);
			if(!ip4addr_aton_is_valid){
				A("%s error!! invalid address value\n",
					module_name());
				return -2;
			}
		}else if(strcmp(str, "-t") == 0){
			str = argv[++i];
			option->mAmount = atoi(str)*100;
		}else if(strcmp(str, "-b") == 0){
			str = argv[++i];
			len = strlen(str);
			suffix = str[len-1];
			switch(suffix) {
				case 'm': case 'M':
					str[len-1]='\0';
					option->mAppRate = atof(str)*MEGA;
					break;
				case 'k': case 'K':
					str[len-1]='\0';
					option->mAppRate = atof(str)*KILO;
					break;
				default:
					option->mAppRate = atof(str);
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
			A("%s unknown options : %s\n",
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
	ip4_addr_set_zero(&option->addr);
}

int iperf_stop(iperf_opt_t* option)
{
	int i;
	if(option->mThreadMode == kMode_Server){
#ifndef SUPPORT_ETHERNET_ACCESSPOINT
		for(i=0; i<MAX_IF; i++){
			if(!ip4_addr_isany_val(nrc_netif[i]->ip_addr)){
				ip4_addr_copy(option->addr, nrc_netif[i]->ip_addr);
				iperf_stop_session(option);
			}
		}
#else
		if(!ip4_addr_isany_val(eth_netif.ip_addr)){
			ip4_addr_copy(option->addr, eth_netif.ip_addr);
			iperf_stop_session(option);
		}
#endif
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
	iperf_init_parameters(option);
	if(option == NULL){
		A("iperf parameter init fail!!\n");
		return false;
	}

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
#ifndef SUPPORT_ETHERNET_ACCESSPOINT
		for(i=0; i<MAX_IF; i++){
			if(!ip4_addr_isany_val(nrc_netif[i]->ip_addr)){
				if(server_started == true){
					iperf_opt_t* option2 =  iperf_option_alloc();
					memcpy(option2, option, sizeof(iperf_opt_t));
					option = option2;
				}
				ip4_addr_copy(option->addr, nrc_netif[i]->ip_addr);
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
#else
		if (!ip4_addr_isany_val(eth_netif.ip_addr)) {
			if(server_started == true) {
				iperf_opt_t* option2 =  iperf_option_alloc();
				memcpy(option2, option, sizeof(iperf_opt_t));
				option = option2;
			}
			ip4_addr_copy(option->addr, eth_netif.ip_addr);
			ret = iperf_start_session(option, report_cb);
			if(ret < 0) {
				if(server_started == true){
					iperf_option_free(option);
					return false;
				}
			} else {
				server_started = true;
			}
		}
#endif
	}
	return true;
}

#endif /* LWIP_IPERF */
