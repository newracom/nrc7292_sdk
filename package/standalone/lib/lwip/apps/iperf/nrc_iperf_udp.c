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
#include "lwip/etharp.h"

#include "util_trace.h"

#include "nrc_iperf.h"
#include "nrc_iperf_tcp.h"

#include "api_wifi.h"

#if LWIP_BRIDGE
extern struct netif br_netif;
#endif

struct udp_client_data {
	iperf_opt_t *option;
	struct udp_client_data *next;
};

static struct udp_client_data *udp_client_list = NULL;

extern void sys_arch_msleep(u32_t delay_ms);
extern bool wpa_driver_get_associate_status(uint8_t vif);

#if !defined(DISABLE_IPERF_APP)
#if defined(LWIP_IPERF) && (LWIP_IPERF == 1)

static void iperf_udp_client_init_payload (iperf_udp_datagram_t *datagram, int32_t datagram_size)
{
	char *payload;
	int payload_size;
	int i, j;

	payload = (char *)&datagram->client_header;

	if (datagram->client_header.flags & IPERF_FLAG_HDR_VER1)
		payload += IPERF_CLIENT_HDR1_SIZE;
	else
		payload += IPERF_CLIENT_HDR0_SIZE;

	payload_size = datagram_size - (payload - (char *)datagram);

	for (i = 0, j = 0 ; i < payload_size ; i++)
	{
		payload[i] = '0' + j;

		if (++j == 10)
			j = 0;
	}
}

static void iperf_udp_client_init_datagram (iperf_opt_t * option,
											iperf_udp_datagram_t *datagram)
{
	bool client_header_ver1 = false;

	datagram->id = htonl(0);
	datagram->tv_sec = htonl(0);
	datagram->tv_usec = htonl(0);

	datagram->client_header.flags = htonl(client_header_ver1 ? IPERF_FLAG_HDR_VER1 : 0);
	datagram->client_header.numThreads = htonl(1);
	datagram->client_header.mPort = htonl(option->mPort);
	datagram->client_header.bufferlen = htonl(0);
	datagram->client_header.mWindowSize = htonl(0);
	datagram->client_header.mAmount = htonl(-(option->mAmount));
	if (client_header_ver1)
	{
		datagram->client_header.mRate = htonl(IPERF_DEFAULT_UDP_RATE);
		datagram->client_header.mUDPRateUnits = htonl(0);
		datagram->client_header.mRealTime = htonl(0);
	}

	iperf_udp_client_init_payload(datagram, option->mBufLen);
}

static void iperf_udp_client_report (iperf_opt_t * option)
{
	iperf_time_t start_time = 0;
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
			CPA("##### [%s] etharp_find_addr failed\n", __func__);
		}
	}

	nrc_iperf_spin_lock();
	CPA("[iperf UDP Client Report for server %s, snr:%u, rssi:%d]\n", ipaddr_ntoa(&option->addr), snr, rssi);
	CPA("  Interval          Transfer      Bandwidth\n");
	CPA("  %4.1f - %4.1f sec  %7sBytes  %7sbits/sec\n",
					start_time, end_time,
					byte_to_string(byte), bps_to_string(bps));
	CPA("  sent : %llu datagrams\n", option->client_info.datagram_cnt);
	nrc_iperf_spin_unlock();
}

static void iperf_udp_client_report_from_server (iperf_server_header_t *header)
{
	iperf_time_t time = ntohl(header->stop_sec) + (ntohl(header->stop_usec) / 1000000.);
	int32_t byte = ntohl(header->total_len2);
	int32_t datagrams = ntohl(header->datagrams);
	int32_t errors = ntohl(header->error_cnt);
	iperf_time_t jitter = ntohl(header->jitter1) + (ntohl(header->jitter2) / 1000000.);

	nrc_iperf_spin_lock();
	CPA("  Server Report:\n");
	CPA("  %4.1f - %4.1f sec  %7sBytes  %7sbits/sec  %.3f ms  %ld/%ld (%.2g%%)\n",
					0., time,
					byte_to_string(byte),
					bps_to_string(byte_to_bps(time, byte)),
					jitter * 1000,
					errors, datagrams,
					(double)((errors * 100.) / datagrams));
	nrc_iperf_spin_unlock();
}

static int iperf_await_server_fin_packet (iperf_opt_t * option, 	iperf_udp_datagram_t *datagram)
{
	int rc;
	fd_set readSet;
	const int CLIENTHDRACK =  0x2;
	struct timeval timeout;
	int ack_success = 0;
	int count = 20 ;
	int mySocket = option->mSock;
	iperf_udp_datagram_t *buf = NULL;
	buf = (iperf_udp_datagram_t *)mem_malloc(IPERF_DEFAULT_DATA_BUF_LEN);
	if (!buf){
		return 0;
	}
	memset(buf, 0, IPERF_DEFAULT_DATA_BUF_LEN);

	while (--count >= 0) {
		FD_ZERO(&readSet);
		FD_SET(mySocket, &readSet);
		timeout.tv_sec  = 0;
		timeout.tv_usec = 100000; // 100 milliseconds
		rc = select(mySocket+1, &readSet, NULL, NULL, &timeout);
		if (rc == 0) {
			rc = write(mySocket, datagram, option->mBufLen);
		} else {
			rc = read(mySocket, (char*)buf, IPERF_DEFAULT_DATA_BUF_LEN);
			if (rc == sizeof(struct client_hdr_ack)) {
				struct client_hdr_ack *ack =  (struct client_hdr_ack *)(buf);
				if (ntohl(ack->typelen.type) == CLIENTHDRACK) {
					continue;
				}
			}

			if (rc > 0) {
				ack_success = 1;
				break;
			}
		}
	}
	if ((!ack_success))
		CPA("Not received server fin packet\n");
	else
		iperf_udp_client_report_from_server((iperf_server_header_t *)&buf->server_header);

	if(buf) mem_free(buf);
	return ack_success;
}

uint16_t iperf_udp_client_send_delay(u32_t bandwidth, u32_t data_size)
{
	u16_t time_delay = ((8*data_size* 1000) / bandwidth);

	if(!time_delay)
		time_delay = 1;
	//CPA("packet delay = %d ms, bandwidth = %d  data_size = %d\n" , time_delay ,bandwidth, data_size);
	return time_delay;
}

ATTR_NC __attribute__((optimize("O3"))) bool iperf_check_throttle(iperf_opt_t * option)
{
	iperf_time_t now;
	iperf_time_t interval;
	uint32_t byte;
	uint32_t bps;

	iperf_get_time(&now);

	byte = option->client_info.datagram_cnt * option->mBufLen;
	interval = now - option->client_info.start_time;
	bps = byte_to_bps(interval, byte);

	if (bps < option->mAppRate) {
		return true;
	} else {
		return false;
	}
}

ATTR_NC __attribute__((optimize("O3"))) void iperf_udp_client(void *pvParameters)
{
	int sock = -1;
	int ret;
	iperf_time_t start_time, now, duration;
	struct sockaddr_storage to;
	struct sockaddr_storage bind;
	int count = 0;
	int tosval;
	uint8_t vif = 0;
	uint16_t delay;
	uint32_t packet_count = 0;
	iperf_udp_datagram_t *datagram = NULL;
	iperf_opt_t * option  = pvParameters;
	TaskHandle_t task_handle = option->task_handle;

	int alloc_size = (option->mBufLen < IPERF_DEFAULT_DATA_BUF_LEN) ? IPERF_DEFAULT_DATA_BUF_LEN : option->mBufLen;
	const int multisend_num = 3;
	int multisend;

#if DEBUG_IPERF_TASK_HANDLE
	nrc_iperf_spin_lock();
	CPA("%s Start [handle:%d]\n", __func__, task_handle);
	nrc_iperf_spin_unlock();
#endif

#if LWIP_IPV6
	if(IP_IS_V6(&option->addr) && !ip6_addr_isipv4mappedipv6(ip_2_ip6(&option->addr))) {
		CPA("[%s] Unsupported IPV6\n", __func__);
		goto task_exit;
	}
#endif /* LWIP_IPV6 */

	if(nrc_iperf_task_list_add(option) < 0){
		goto task_exit;
	}

#if LWIP_IPV4
	if(IP_IS_V4(&option->addr)) {
		struct sockaddr_in *to4 = (struct sockaddr_in*)&to;
		struct sockaddr_in *bind4 = (struct sockaddr_in*)&bind;

		sock = socket(AF_INET, SOCK_DGRAM, 0);
		if (sock < 0) {
			CPA("create socket failed!\n");
			goto exit;
		}

		bind4->sin_len	= sizeof(bind4);
		bind4->sin_family = AF_INET;
		bind4->sin_port = htons(0);
		bind4->sin_addr.s_addr = htonl(INADDR_ANY);

		if (bind(sock, (struct sockaddr*)&bind, sizeof(bind)) < 0){
			int err = errno;
			CPA("[%s] bind(%d) returned error: %d.\n", __func__, sock, -err);
			goto exit;
		}

		to4->sin_len	= sizeof(to4);
		to4->sin_family = AF_INET;
		to4->sin_port = htons(option->mPort);
		inet_addr_from_ip4addr(&to4->sin_addr, ip_2_ip4(&option->addr));
	}
#endif /* LWIP_IPV4 */

	tosval = (int)option->mTOS;
	ret = setsockopt(sock, IPPROTO_IP, IP_TOS, (void *) &tosval, sizeof(tosval));
	if (ret == -1) {
		CPA("Set socket IP_TOS option failed!\n");
		goto exit;
	}

	datagram = (iperf_udp_datagram_t *)mem_malloc(alloc_size);
	if (!datagram){
		goto exit;
	}
	memset(datagram, 0, alloc_size);

	iperf_udp_client_init_datagram(option, datagram);

	nrc_iperf_spin_lock();
	CPA("------------------------------------------------------------\n");
	CPA("Client connecting to %s, UDP port %d\n",
		ipaddr_ntoa(&option->addr), option->mPort);
	CPA("Sending %ld byte datagrams, Duration %4.1f sec\n",
		option->mBufLen, option->mAmount / 100.0);
	CPA("------------------------------------------------------------\n");
	nrc_iperf_spin_unlock();

	iperf_get_time(&start_time);
	duration = (option->mAmount / 100.);

	option->mSock = sock;
	option->client_info.start_time = start_time;
	option->client_info.duration= duration;

	delay = iperf_udp_client_send_delay(option->mAppRate,option->mBufLen);
	vif = wifi_get_vif_id(&option->addr);

	if (option->mInterval > 0) {
		xTaskCreate(nrc_iperf_periodic_report, "iperf_udp_client_report", 1024,
			(void *) option, LWIP_IPERF_TASK_PRIORITY, &option->client_info.periodic_report_task);
	}

	while (1)
	{
		iperf_get_time(&now);

		datagram->tv_sec = htonl((uint32_t)now);
		datagram->tv_usec = htonl((uint32_t)((now - ntohl(datagram->tv_sec)) * 1000000));

		if (wpa_driver_get_associate_status(vif)== false){
			option->client_info.end_time = now;
			CPA("Wi-Fi connection lost\n");
			break;
		}

		if (option->mForceStop || iperf_time_expried(start_time, duration) ){
			option->client_info.end_time = now;
			datagram->id = htonl(~(packet_count-1));
			if(iperf_send(sock, (const char *)datagram, option->mBufLen,
				 (struct sockaddr*)&to, sizeof(to)) == -1)
				CPA("[%s] iperf udp server report fail\n", __func__);

			break;
		}else{
			for (multisend = 0; multisend < multisend_num; multisend++) {
				if(iperf_check_throttle(option)){
					datagram->id = htonl(packet_count);
					if(iperf_send(sock, (const char *)datagram, option->mBufLen,
						(struct sockaddr*)&to, sizeof(to)) != -1) {
						option->client_info.datagram_cnt++;
						packet_count++;
					}
				} else {
					break;
				}
			}

			/* put a actual delay to calculate bandwidth exactly */
			{
				iperf_time_t dummy;
				iperf_get_time(&dummy);
				if ((dummy - now) < delay)
					sys_arch_msleep(delay - (dummy-now));
			}
		}
	}

	if(option->client_info.periodic_report_task) {
		sys_arch_msleep(100);
		xTaskNotifyGive(option->client_info.periodic_report_task);
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	}

	iperf_udp_client_report(option);

	if (wpa_driver_get_associate_status(vif)== true)
		iperf_await_server_fin_packet(option, datagram);

exit:
	nrc_iperf_task_list_del(option);
	if(datagram) mem_free(datagram);
	if(sock!=-1) {
		shutdown(sock, SHUT_RDWR);
		closesocket(sock);
	}

task_exit:
	iperf_option_free(option);
#if DEBUG_IPERF_TASK_HANDLE
	nrc_iperf_spin_lock();
	CPA("%s END [handle:%d]\n", __func__, task_handle);
	nrc_iperf_spin_unlock();
#endif
	vTaskDelete(NULL);
}

static struct udp_client_data *udp_new_client(iperf_opt_t *option)
{
	struct udp_client_data *udp_c_data = NULL;

	udp_c_data = (struct udp_client_data *) mem_malloc(sizeof(struct udp_client_data));
	if (!udp_c_data) {
		CPA("[%s] new client malloc failed\n", __func__);
		return NULL;
	}

	memset(udp_c_data, 0, sizeof(struct udp_client_data));

	udp_c_data->option = (iperf_opt_t *) mem_malloc(sizeof(iperf_opt_t));
	if(!udp_c_data->option) {
		CPA("[%s] option malloc failed\n", __func__);
		mem_free(udp_c_data);
		return NULL;
	}
	memcpy(udp_c_data->option, option, sizeof(iperf_opt_t));
	udp_c_data->next = udp_client_list;
	udp_client_list = udp_c_data;

	if (option->mInterval > 0) {
		xTaskCreate(nrc_iperf_periodic_report, "iperf_udp_server_report", 1024,
			(void *) udp_c_data->option, LWIP_IPERF_TASK_PRIORITY, &udp_c_data->option->server_info.periodic_report_task);
	}
	return udp_c_data;
}

ATTR_NC __attribute__((optimize("O3"))) static struct udp_client_data *find_matching_client(iperf_opt_t *option)
{
	struct udp_client_data *client;
	struct udp_client_data *next_client;

	if (!udp_client_list) {
		return NULL;
	}

	for (client = udp_client_list; client; client = next_client) {
		next_client = client->next;
		struct sockaddr_in *from_in_client = (struct sockaddr_in *) &client->option->server_info.clientaddr;
		struct sockaddr_in *from = (struct sockaddr_in *) &option->server_info.clientaddr;
		if ((from_in_client->sin_addr.s_addr == from->sin_addr.s_addr)
			&& (from_in_client->sin_port == from->sin_port)) {
			return client;
		}
	}
	return NULL;
}

static void iperf_udp_server_report (struct udp_client_data *client)
{
	iperf_time_t start_time = 0;
	iperf_time_t stop_time = client->option->server_info.stop_time - client->option->server_info.start_time;
	iperf_time_t interval = stop_time;
	uint64_t byte = client->option->server_info.recv_byte;
	uint32_t bps = (interval)? (byte*8)/interval : 0;
	struct sockaddr_in *from = (struct sockaddr_in*)&client->option->server_info.clientaddr;
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

		client_ip.addr = from->sin_addr.s_addr;
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
			CPA("##### [%s] etharp_find_addr failed\n", __func__);
		}
	}

	nrc_iperf_spin_lock();
	CPA("[iperf UDP Server Report for %s:%d, snr:%u, rssi:%d]\n", inet_ntoa(from->sin_addr), ntohs(from->sin_port), snr, rssi);
	CPA("  %4.1f - %4.1f sec  %7sBytes  %7sbits/sec  %6.3f ms  %4ld/%5ld (%.2g%%)",
	  start_time, stop_time,
	  byte_to_string(byte), bps_to_string(bps),
	  client->option->server_info.jitter * 1000,
	  client->option->server_info.error_cnt,
	  client->option->server_info.datagram_seq,
	  (double)(client->option->server_info.error_cnt * 100.) / client->option->server_info.datagram_seq);

	if (client->option->server_info.outoforder_cnt > 0)
		CPA("\t OUT_OF_ORDER(%ld)\n", client->option->server_info.outoforder_cnt);
	else
		CPA("\n");
	nrc_iperf_spin_unlock();
}

ATTR_NC __attribute__((optimize("O3"))) static void udp_free_client(struct udp_client_data *client)
{
	struct udp_client_data *tmp;

	if (!client) {
		return;
	}

	if (client == udp_client_list) {
		udp_client_list = udp_client_list->next;
	} else {
		for (tmp = udp_client_list; (tmp->next != client) && tmp; tmp = tmp->next);
		tmp->next = client->next;
	}

	if(client->option->server_info.periodic_report_task) {
		sys_arch_msleep(100);
		xTaskNotifyGive(client->option->server_info.periodic_report_task);
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	}
	iperf_udp_server_report(client);
	mem_free(client->option);
	mem_free(client);
}


static void iperf_udp_server_init_payload (iperf_udp_datagram_t *datagram)
{
	char *payload;
	int payload_size;
	int i, j;

	payload = (char *)&datagram->server_header;

	if (datagram->server_header.flags & htonl(IPERF_FLAG_HDR_VER1))
		payload += IPERF_SERVER_HDR1_SIZE;
	else
		payload += IPERF_SERVER_HDR0_SIZE;

	payload_size = payload - (char *)datagram;

	for (i = 0, j = 0 ; i < payload_size ; i++)
	{
		payload[i] = '0' + j;

		if (++j == 10)
			j = 0;
	}
}

static void iperf_udp_server_report_to_client (iperf_opt_t * option)
{
	const bool server_header_ver1 = true;
	iperf_time_t total_time;
	uint64_t total_len;
	iperf_udp_datagram_t *datagram = NULL;
	struct sockaddr client_addr;

	memcpy (&client_addr, &(option->server_info.clientaddr), sizeof(struct sockaddr));

	if (client_addr.sa_family != AF_INET) {
		CPA("invalid client addr\n");
		return;
	}

	datagram = (iperf_udp_datagram_t *)mem_malloc(IPERF_DEFAULT_DATA_BUF_LEN);
	if (!datagram){
		CPA("datagram malloc failed\n");
		return;
	}
	memset(datagram, 0, IPERF_DEFAULT_DATA_BUF_LEN);

	total_time = option->server_info.stop_time - option->server_info.start_time;
	total_len = option->server_info.recv_byte;

	memset(datagram, 0x0, sizeof(iperf_udp_datagram_t));

	datagram->id = htonl(option->server_info.last_id);
	datagram->server_header.flags = server_header_ver1 ? htonl(IPERF_FLAG_HDR_VER1) : 0;
	datagram->server_header.total_len1 = htonl((total_len >> 32) & 0xffffffff);
	datagram->server_header.total_len2 = htonl(total_len & 0xffffffff);
	datagram->server_header.stop_sec = htonl((int32_t)total_time);
	datagram->server_header.stop_usec = htonl((int32_t)((total_time - ntohl(datagram->server_header.stop_sec)) * 1000000));
	datagram->server_header.error_cnt = htonl(option->server_info.error_cnt) ;
	datagram->server_header.outoforder_cnt = htonl(option->server_info.outoforder_cnt);
	datagram->server_header.datagrams = htonl(option->server_info.datagram_seq);
	datagram->server_header.jitter1 = htonl((int32_t)option->server_info.jitter);
	datagram->server_header.jitter2 = htonl((int32_t)((option->server_info.jitter - ntohl(datagram->server_header.jitter1)) * 1000000));

	if (server_header_ver1)
	{
		/* -e, --enhancedreports
		 *  use enhanced reporting giving more tcp/udp and traffic information
		 */
		datagram->server_header.minTransit1 = htonl(0);
		datagram->server_header.minTransit2 = htonl(0);
		datagram->server_header.maxTransit1 = htonl(0);
		datagram->server_header.maxTransit2 = htonl(0);
		datagram->server_header.sumTransit1 = htonl(0);
		datagram->server_header.sumTransit2 = htonl(0);
		datagram->server_header.meanTransit1 = htonl(0);
		datagram->server_header.meanTransit2 = htonl(0);
		datagram->server_header.m2Transit1 = htonl(0);
		datagram->server_header.m2Transit2 = htonl(0);
		datagram->server_header.vdTransit1 = htonl(0);
		datagram->server_header.vdTransit2 = htonl(0);
		datagram->server_header.cntTransit = htonl(0);

		datagram->server_header.IPGcnt = htonl(option->server_info.datagram_cnt / total_time);
		datagram->server_header.IPGsum = htonl(1);
	}

	iperf_udp_server_init_payload(datagram);
	sendto(option->mSock, datagram, IPERF_DEFAULT_UDP_DATAGRAM_SIZE, 0,
		&client_addr, sizeof(struct sockaddr));
	if(datagram) mem_free(datagram);
}

static void iperf_udp_server_measure_jitter (int32_t id,
											iperf_time_t tx_time, iperf_time_t rx_time,
											iperf_time_t *jitter, iperf_time_t *transit)
{
	iperf_time_t prev_jitter = *jitter;
	iperf_time_t prev_transit = *transit;
	iperf_time_t cur_transit, diff_transit;

	if (id == 0) {
		prev_jitter = 0;
		prev_transit = 0;
	}

	cur_transit = rx_time - tx_time;
	diff_transit = cur_transit - prev_transit;

	if (diff_transit < 0)
		diff_transit = -diff_transit;

	*transit = cur_transit;
	*jitter += (diff_transit - prev_jitter) / 16.0;
}

ATTR_NC __attribute__((optimize("O3"))) static void iperf_udp_server_recv (iperf_opt_t * option, char *buf, int len)
{
	iperf_time_t now;

	iperf_get_time(&now);

	if (len > 0) {
		iperf_udp_datagram_t *datagram = (iperf_udp_datagram_t *)buf;

		int32_t id = ntohl(datagram->id);

		if (id == 0) {
			int32_t amount = ntohl(datagram->client_header.mAmount);
			if (amount >= 0)
				 option->server_info.send_byte = amount;
			else
				 option->server_info.send_time = -amount / 100.; // sec

			option->server_info.start_time = now;

			option->server_info.datagram_cnt = 0;
			option->server_info.datagram_seq = 0;
			option->server_info.error_cnt = 0;

			option->server_info.start = 1;
			option->server_info.recv_byte = 0;

#if LWIP_IPV4
			if(IP_IS_V4(&option->addr)) {
				struct sockaddr_in *from4 = (struct sockaddr_in*)&option->server_info.clientaddr;
				nrc_iperf_spin_lock();
				CPA("UDP server connected with %s port %d\n",
					   inet_ntoa(from4->sin_addr), ntohs(from4->sin_port));
				nrc_iperf_spin_unlock();
			}
#endif /* LWIP_IPV4 */
		} else 	{
			if (id < 0) {
				option->server_info.last_id = id;
				option->server_info.done = 1;
			} else {
				option->server_info.datagram_cnt++;
				option->server_info.datagram_seq++;

				if (id < option->server_info.datagram_seq) {
					/* CPA("out of order: %d -> %d\n", option->server_info.datagram_seq, id); */
					option->server_info.outoforder_cnt++;
					option->server_info.datagram_seq = id;
					if(option->server_info.error_cnt>0)
						option->server_info.error_cnt--;
				} else if (id > option->server_info.datagram_seq) {
					/* CPA("lost datagrams: %d, %d -> %d\n", id - option->server_info.datagram_seq,
															option->server_info.datagram_seq, id); */
					option->server_info.error_cnt += id - option->server_info.datagram_seq;
					option->server_info.datagram_seq = id;
				}
			}
		}

		option->server_info.recv_byte += len;
		option->server_info.stop_time = now;
	}
	else {
		nrc_iperf_spin_lock();
		CPA("len=%d\n", len);
		nrc_iperf_spin_unlock();
	}
}

uint32_t g_delay = 0;

ATTR_NC __attribute__((optimize("O3"))) void iperf_udp_server(void *pvParameters)
{
	iperf_opt_t * option  = pvParameters;
	int sock = -1;
	int ret = -1;
	int bytes_received;
	socklen_t sin_size;
	char *buf = NULL;
	struct sockaddr_storage server;
	struct timeval timeout, timeout_recv;
	TaskHandle_t task_handle = option->task_handle;

	/* Setup fd_set structure */
	fd_set fds;

	struct udp_client_data *client = NULL;

#if DEBUG_IPERF_TASK_HANDLE
	nrc_iperf_spin_lock();
	CPA("%s Start [handle:%d]\n", __func__, task_handle);
	nrc_iperf_spin_unlock();
#endif

#if LWIP_IPV6
		if(IP_IS_V6(&option->addr) && !ip6_addr_isipv4mappedipv6(ip_2_ip6(&option->addr))) {
			CPA("[%s] Unsupported IPV6\n", __func__);
			goto task_exit;
		}
#endif /* LWIP_IPV6 */

	if(nrc_iperf_task_list_add(option) < 0){
		goto task_exit;
	}

#if LWIP_IPV4
	if(IP_IS_V4(&option->addr)) {
		struct sockaddr_in *bind4 = (struct sockaddr_in*)&server;

		sock = socket(AF_INET, SOCK_DGRAM, 0);
		if (sock < 0) {
			CPA("create socket failed!\n");
			goto exit;
		}

		bind4->sin_len	= sizeof(bind4);
		bind4->sin_family = AF_INET;
		bind4->sin_port = htons(option->mPort);
		bind4->sin_addr.s_addr = htonl(INADDR_ANY);

		if (bind(sock, (struct sockaddr*)&server, sizeof(server)) < 0){
			int err = errno;
			CPA("[%s] bind(%d) returned error: %d.\n", __func__, sock, -err);
			goto exit;
		}
	}
#endif /* LWIP_IPV4 */

	option->mSock = sock;

	timeout.tv_sec = 2;
	timeout.tv_usec = 0;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
		CPA("setsockopt failed!!");
		goto exit;
	}
	if (bind(sock, (struct sockaddr *)&server, sizeof(struct sockaddr_storage)) < 0) {
		CPA("iperf server bind failed!! exit\n");
		goto exit;
	}

	buf = mem_malloc(IPERF_UDP_MAX_RECV_SIZE);
	if (buf == NULL) {
		CPA("No memory\n");
		goto exit;
	}
	memset(buf, 0, IPERF_UDP_MAX_RECV_SIZE);

	sin_size = sizeof(struct sockaddr_storage);
	timeout_recv.tv_sec = 0;
	timeout_recv.tv_usec = 0;

	while(!option->mForceStop){
		int client_done = 0;
		while (!client_done) {
			if(option->mForceStop){
				option->server_info.done = 1;
				continue;
			}

			FD_ZERO(&fds);
			FD_SET(sock, &fds);

			if (select(sock + 1, &fds, NULL, NULL, &timeout_recv) == 0) {
				sys_arch_msleep(5);
				continue;
			}

			bytes_received = 0;
			bytes_received = recvfrom(sock, buf, IPERF_UDP_MAX_RECV_SIZE, 0,
				(struct sockaddr *) &option->server_info.clientaddr, &sin_size);

			iperf_udp_datagram_t *datagram = (iperf_udp_datagram_t *)buf;
			int32_t id = ntohl(datagram->id);

			client = find_matching_client(option);

			if (!client) {
				if (id == 0) {
					if ((client = udp_new_client(option)) == NULL) {
						CPA("[%s] failed to create new client\n", __func__);
						break;
					}
				} else {
					break;
				}
			}

			if(bytes_received < 0 ){	// disconnected
				client->option->server_info.done = 1;
			} else {
				iperf_udp_server_recv(client->option, buf, bytes_received);
			}

			if (client->option->server_info.done) {
				client_done = 1;
			}
		}

		if (client) {
			if(client->option->server_info.start == 1) {
				iperf_udp_server_report_to_client(client->option);
				udp_free_client(client);
			}
		}
	}

exit:
	nrc_iperf_task_list_del(option);
	if(buf) mem_free(buf);
	if(sock!=-1) {
		shutdown(sock, SHUT_RDWR);
		closesocket(sock);
	}

task_exit:
	iperf_option_free(option);
#if DEBUG_IPERF_TASK_HANDLE
	nrc_iperf_spin_lock();
	CPA("%s END [handle:%d]\n", __func__, task_handle);
	nrc_iperf_spin_unlock();
#endif
	vTaskDelete(NULL);
}
#endif /* LWIP_IPERF */
#endif /*!defined(DISABLE_IPERF_APP) */
