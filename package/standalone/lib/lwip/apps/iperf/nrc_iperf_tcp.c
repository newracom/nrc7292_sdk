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
#include "lwip/if_api.h"
#include "lwip/etharp.h"

#include "nrc_iperf.h"
#include "nrc_iperf_tcp.h"

#include "util_trace.h"

#include "api_wifi.h"

#if LWIP_BRIDGE
extern struct netif br_netif;
#endif

struct client_data {
	iperf_opt_t *option;
};

#if defined(LWIP_IPERF) && (LWIP_IPERF == 1)

extern void sys_arch_msleep(u32_t delay_ms);
extern bool wpa_driver_get_associate_status(uint8_t vif);

static void iperf_tcp_client_report (iperf_opt_t * option )
{
	iperf_time_t start_time = 0.0;
	iperf_time_t end_time = option->client_info.end_time - option->client_info.start_time;
	iperf_time_t interval = end_time;

	uint32_t byte = option->client_info.datagram_cnt * option->mBufLen;
	uint32_t bps = byte_to_bps(interval, byte);

	uint8_t snr = 0;
	int8_t rssi = 0;
	tWIFI_DEVICE_MODE mode;

	nrc_wifi_get_device_mode(0, &mode);

	if (mode == WIFI_MODE_STATION) {
		nrc_wifi_get_snr(0, &snr);
		nrc_wifi_get_rssi(0, &rssi);
	} else {
		const ip4_addr_t *ip_addr;
		struct eth_addr *mac_addr;
		STA_INFO info;
		
		if (etharp_find_addr(nrc_netif[0], (const ip4_addr_t *) &option->addr, &mac_addr, &ip_addr) >= 0) {
			if (nrc_wifi_softap_get_sta_by_addr(0, mac_addr->addr, &info) == WIFI_SUCCESS) {
				snr = info.snr;
				rssi = info.rssi;
			}
		} else if (etharp_find_addr(nrc_netif[1], (const ip4_addr_t *) &option->addr, &mac_addr, &ip_addr) >= 0) {
			if (nrc_wifi_softap_get_sta_by_addr(1, mac_addr->addr, &info) == WIFI_SUCCESS) {
				snr = info.snr;
				rssi = info.rssi;
			}
#if LWIP_BRIDGE
		} else if (etharp_find_addr(&br_netif, (const ip4_addr_t *) &option->addr, &mac_addr, &ip_addr) >= 0) {
			if (nrc_wifi_softap_get_sta_by_addr(0, mac_addr->addr, &info) == WIFI_SUCCESS) {
				snr = info.snr;
				rssi = info.rssi;
			} else if (nrc_wifi_softap_get_sta_by_addr(1, mac_addr->addr, &info) == WIFI_SUCCESS) {
				snr = info.snr;
				rssi = info.rssi;
			}
#endif
		} else {
			A("##### [%s] etharp_find_addr failed\n", __func__);
		}
	}

	nrc_iperf_spin_lock();
	A("[iperf TCP Client Report for server %s, snr:%u, rssi:%d]\n", ipaddr_ntoa(&option->addr), snr, rssi);
	A("     Interval        Transfer      Bandwidth\n");
	A("  %4.1f - %4.1f sec  %7sBytes  %7sbits/sec\n",
					start_time, end_time,
					byte_to_string(byte), bps_to_string(bps));
	nrc_iperf_spin_unlock();
}

static void iperf_tcp_client_init_payload (iperf_tcp_datagram_t *datagram, int32_t datagram_size)
{
	char *payload;
	int payload_size;
	int i, j;

	payload = (char *)datagram + IPERF_CLIENT_HDR0_SIZE;
	payload_size = datagram_size - IPERF_CLIENT_HDR0_SIZE;

	for (i = 0, j = 0 ; i < payload_size ; i++) {
		payload[i] = '0' + j;
		if (++j == 10)
			j = 0;
	}
}

static void iperf_tcp_client_init_datagram (iperf_opt_t * option, iperf_tcp_datagram_t *datagram)
{
	datagram->client_header.flags = htonl(0);
	datagram->client_header.numThreads = htonl(1);
	datagram->client_header.mPort = htonl(option->mPort);
	datagram->client_header.bufferlen = htonl(0);
	datagram->client_header.mWindowSize = htonl(0);
	datagram->client_header.mAmount = htonl(-(option->mAmount ));
	iperf_tcp_client_init_payload(datagram, option->mBufLen);
}

#define MAX_IPERF_TCP_CLIENT_RETRY 10

void iperf_tcp_client(void *pvParameters)
{
	int sock = -1;
	int ret;
	int flag;
	int tosval;
	uint8_t vif = 0;
	iperf_time_t start_time, stop_time, now;
	struct sockaddr_storage to;
	int count = 0;

	char buffer[IPERF_DEFAULT_DATA_BUF_LEN];
	iperf_tcp_datagram_t *datagram = (iperf_tcp_datagram_t *) buffer;
	iperf_opt_t * option  = pvParameters;
	TaskHandle_t periodic_report_task = NULL;

#if LWIP_IPV6
	if(IP_IS_V6(&option->addr) && !ip6_addr_isipv4mappedipv6(ip_2_ip6(&option->addr))) {
		A("[%s] Unsupported IPV6\n", __func__);
		goto task_exit;
	}
#endif /* LWIP_IPV6 */

	if(nrc_iperf_task_list_add(option) < 0){
		goto task_exit;
	}

#if LWIP_IPV4
	if(IP_IS_V4(&option->addr)) {
		struct sockaddr_in *to4 = (struct sockaddr_in*)&to;

		sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock < 0) {
			A("create socket failed!\n");
			goto exit;
		}

		to4->sin_len	= sizeof(to4);
		to4->sin_family = AF_INET;
		to4->sin_port = htons(option->mPort);
		inet_addr_from_ip4addr(&to4->sin_addr, ip_2_ip4(&option->addr));
	}
#endif /* LWIP_IPV4 */

	nrc_iperf_spin_lock();
	A("Client connecting to %s, TCP port %d\n",
		ipaddr_ntoa(&option->addr), option->mPort);
	nrc_iperf_spin_unlock();

	/* connect to TCP Server */
	while(1){
		ret = connect(sock, (const struct sockaddr*)&to, sizeof(to));
		if (ret == -1) {
			if(count == MAX_IPERF_TCP_CLIENT_RETRY){
				goto exit;
				A("Connect to iperf server failed!\n");
			}
			count++;
			sys_arch_msleep(1000);
		} else {
			count = 0;
			break;
		}
	}

	ret = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (void *) &flag, sizeof(int));
	if (ret == -1) {
		A("Set socket TCP_NODELAY option failed!\n");
		goto exit;
	}

	tosval = (int)option->mTOS;
	ret = setsockopt(sock, IPPROTO_IP, IP_TOS, (void *) &tosval, sizeof(tosval));
	if (ret == -1) {
		A("Set socket IP_TOS option failed!\n");
		goto exit;
	}

	memset(datagram, 0, IPERF_DEFAULT_DATA_BUF_LEN);
	iperf_tcp_client_init_datagram(option, datagram);

	iperf_get_time(&start_time);

	stop_time = start_time + (option->mAmount / 100.);

	option->mSock = sock;
	option->mBufLen = TCP_MSS;
	option->client_info.start_time = start_time;

	vif = wifi_get_vif_id(&option->addr);

	if (option->mInterval > 0) {
		xTaskCreate(nrc_iperf_periodic_report, "iperf periodic report",
					1024, (void *) option, LWIP_IPERF_TASK_PRIORITY, &periodic_report_task);
	}

	while (1) {
		if(wpa_driver_get_associate_status(vif)== false){
			iperf_get_time(&now);
			option->client_info.end_time = now;
			break;
		}
		nrc_iperf_spin_lock();
		ret = send(sock, datagram, option->mBufLen, 0);
		nrc_iperf_spin_unlock();
		if (ret > 0) {
			option->client_info.datagram_cnt++;
		}

		if (ret < 0) {	//disconnected
			option->mForceStop = 1;
		}

		iperf_get_time(&now);

		if (option->mForceStop || (now >= stop_time)){
			option->client_info.end_time = now;
			break;
		}
	}
	iperf_tcp_client_report(option);

exit:
	nrc_iperf_task_list_del(option);

	if(sock!=-1) {
		shutdown(sock, SHUT_RDWR);
		closesocket(sock);
	}

task_exit:

	if (periodic_report_task)
		vTaskDelete(periodic_report_task);
	iperf_option_free(option);
	vTaskDelete(NULL);
}

static void iperf_tcp_server_report (struct client_data *client)
{
	iperf_time_t start_time = 0;
	iperf_time_t stop_time = client->option->server_info.stop_time - client->option->server_info.start_time;
	iperf_time_t interval = stop_time;
	uint64_t byte = client->option->server_info.recv_byte;
	uint32_t bps = (interval)? (byte*8)/interval:0;

	char peer_addr[INET_ADDRSTRLEN];
	uint8_t snr = 0;
	int8_t rssi = 0;
	tWIFI_DEVICE_MODE mode;

	nrc_wifi_get_device_mode(0, &mode);

	if (mode == WIFI_MODE_STATION) {
		nrc_wifi_get_snr(0, &snr);
		nrc_wifi_get_rssi(0, &rssi);
	} else {
		ip4_addr_t client_ip;
		const ip4_addr_t *ip_addr;
		struct eth_addr *mac_addr;
		STA_INFO info;

		struct sockaddr_in *addr4 = (struct sockaddr_in *) &client->option->server_info.clientaddr;
		client_ip.addr = addr4->sin_addr.s_addr;
		if (etharp_find_addr(nrc_netif[0], &client_ip, &mac_addr, &ip_addr) >= 0) {
			if (nrc_wifi_softap_get_sta_by_addr(0, mac_addr->addr, &info) == WIFI_SUCCESS) {
				snr = info.snr;
				rssi = info.rssi;
			}
		} else if (etharp_find_addr(nrc_netif[1], &client_ip, &mac_addr, &ip_addr) >= 0) {
			if (nrc_wifi_softap_get_sta_by_addr(1, mac_addr->addr, &info) == WIFI_SUCCESS) {
				snr = info.snr;
				rssi = info.rssi;
			}
#if LWIP_BRIDGE
		} else if (etharp_find_addr(&br_netif, &client_ip, &mac_addr, &ip_addr) >= 0) {
			if (nrc_wifi_softap_get_sta_by_addr(0, mac_addr->addr, &info) == WIFI_SUCCESS) {
				snr = info.snr;
				rssi = info.rssi;
			} else if (nrc_wifi_softap_get_sta_by_addr(1, mac_addr->addr, &info) == WIFI_SUCCESS) {
				snr = info.snr;
				rssi = info.rssi;
			}
#endif
		} else {
			A("##### [%s] etharp_find_addr failed\n", __func__);
		}
	}

	nrc_iperf_spin_lock();
	/* mode is 1 if softAP, 0 if station */
	/* For SoftAP, rssi is meaningless */
	A("\n[iperf TCP Server Report for client %s, snr:%u, rssi:%d]\n",
	  inet_ntop(AF_INET, client->option->server_info.clientaddr.s2_data2, peer_addr, INET_ADDRSTRLEN),
	  snr, rssi);
	A("[%3d]  %4.1f - %4.1f sec  %7sBytes  %7sbits/sec\n",
					client->option->server_info.client_sock,
					start_time, stop_time,
					byte_to_string(byte), bps_to_string(bps));
	nrc_iperf_spin_unlock();
}

ATTR_NC __attribute__((optimize("O3"))) static void tcp_process_input(void *pvParameters)
{
	struct client_data *client = (struct client_data *) pvParameters;
	iperf_opt_t *option = client->option;
	int received = 0;
	char buffer[IPERF_DEFAULT_DATA_BUF_LEN];
	iperf_time_t now;

	while ((received = recv(option->server_info.client_sock, buffer, IPERF_DEFAULT_DATA_BUF_LEN, 0)) > 0) {
		if (option->server_info.recv_byte == 0) {
			iperf_tcp_datagram_t *datagram = (iperf_tcp_datagram_t *)buffer;
			int32_t amount = ntohl(datagram->client_header.mAmount);

			if (amount >= 0)
				option->server_info.send_byte = amount;
			else
				option->server_info.send_time = -amount / 100; // sec

			iperf_get_time(&now);
			option->server_info.start_time = now;
			option->server_info.start = true;
		}
		option->server_info.recv_byte += received;

		iperf_get_time(&now);
		option->server_info.stop_time = now;
		if (option->server_info.send_time) {
			if ((option->server_info.stop_time - option->server_info.start_time) >= option->server_info.send_time) {
				break;
			}
		}

		if (option->server_info.send_byte) {
			if (option->server_info.recv_byte >= option->server_info.send_byte) {
				break;
			}
		}
	}

	close(option->server_info.client_sock);

	iperf_tcp_server_report(client);

	if (client->option->server_info.periodic_report_task) {
		vTaskDelete(client->option->server_info.periodic_report_task);
	}

	mem_free(client->option);
	mem_free(client);

	vTaskDelete(NULL);
}

ATTR_NC __attribute__((optimize("O3"))) static struct client_data *tcp_new_client(iperf_opt_t *option)
{
	socklen_t len = sizeof(struct sockaddr);
	struct client_data *c_data = NULL;
	char local_addr[INET_ADDRSTRLEN];
	char client_addr[INET_ADDRSTRLEN];
	int flag = 1;

	if ((option->server_info.client_sock
		 = accept(option->mSock, (struct sockaddr *) &option->server_info.clientaddr, &len)) < 0) {
		return NULL;
	}

	if (setsockopt(option->server_info.client_sock, IPPROTO_TCP, TCP_NODELAY, (void *) &flag, sizeof(int)) < 0) {
		return NULL;
	}

	c_data = (struct client_data *) mem_malloc(sizeof(struct client_data));
	if (!c_data) {
		return NULL;
	}
	memset(c_data, 0, sizeof(struct client_data));

	c_data->option = (iperf_opt_t *) mem_malloc(sizeof(iperf_opt_t));
	memcpy(c_data->option, option, sizeof(iperf_opt_t));

	nrc_iperf_spin_lock();
	A("\n[%3d] local %s port %d connected with %s port %d\n",
	  c_data->option->server_info.client_sock,
	  inet_ntop(AF_INET, &c_data->option->addr, local_addr, INET_ADDRSTRLEN),
	  c_data->option->mPort,
	  inet_ntop(AF_INET, c_data->option->server_info.clientaddr.s2_data2, client_addr, INET_ADDRSTRLEN),
	  ntohs(((struct sockaddr_in *) &c_data->option->server_info.clientaddr)->sin_port));
	nrc_iperf_spin_unlock();

	if (c_data->option->mInterval > 0) {
		xTaskCreate(nrc_iperf_periodic_report, "iperf periodic report",
					1024, (void *) c_data->option, LWIP_IPERF_TASK_PRIORITY, &c_data->option->server_info.periodic_report_task);
	}
	return c_data;
}

ATTR_NC __attribute__((optimize("O3"))) static void tcp_server_loop(iperf_opt_t *option)
{
	fd_set input_set;
	struct client_data *client;

	while (!option->mForceStop) {
		FD_ZERO(&input_set);
		FD_SET(option->mSock, &input_set);

		if (select(option->mSock + 1, &input_set, NULL, NULL, NULL) < 0) {
			return;
		}

		if (FD_ISSET(option->mSock, &input_set)) {
			if ((client = tcp_new_client(option)) != NULL) {
				xTaskCreate(tcp_process_input, "iperf server client task",
							LWIP_IPERF_TASK_STACK_SIZE, (void *) client, LWIP_IPERF_TASK_PRIORITY + 1, NULL);
			} else {
				system_printf("error new connection\n");
			}
		}
	}
}

static int tcp_init_server(unsigned short port)
{
	int sock;
	int reuse = 1;
	struct sockaddr_in server_addr;

	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		return -1;
	}

	memset(&server_addr, 0, sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0) {
		return -1;
	}
	if (bind(sock, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_in)) < 0) {
		return -1;
	}

	if (listen(sock, 3) < 0) {
		return -1;
	}

	return sock;
}

void tcp_start_server(iperf_opt_t *option)
{
	int server_socket = 0;

	if ((option->mSock = tcp_init_server(option->mPort)) < 0) {
		return;
	}
	tcp_server_loop(option);
}

void iperf_tcp_server(void *pvParameters)
{
	iperf_opt_t * option  = pvParameters;

	nrc_iperf_spin_lock();
	A("%s Start\n", __func__);
	nrc_iperf_spin_unlock();
#if LWIP_IPV6
	if(IP_IS_V6(&option->addr) && !ip6_addr_isipv4mappedipv6(ip_2_ip6(&option->addr))) {
		A("[%s] Unsupported IPV6\n", __func__);
		goto task_exit;
	}
#endif /* LWIP_IPV6 */

	if(nrc_iperf_task_list_add(option) < 0){
		goto task_exit;
	}

#if LWIP_IPV4
	if(IP_IS_V4(&option->addr)) {
		tcp_start_server(option);
	}
#endif /* LWIP_IPV4 */

	nrc_iperf_task_list_del(option);

task_exit:
	nrc_iperf_spin_lock();
	A("%s End\n", __func__);
	nrc_iperf_spin_unlock();
	iperf_option_free(option);
	vTaskDelete(NULL);
}
#endif /* LWIP_IPERF */
