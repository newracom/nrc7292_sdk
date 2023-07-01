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

#include "nrc_iperf.h"
#include "nrc_iperf_tcp.h"

#include "util_trace.h"

#include "api_wifi.h"

struct client_data {
	int sock;
	struct in_addr client_addr;
	iperf_opt_t *option;
	struct client_data *next;
};

static struct client_data *client_list = NULL;
static int maxdesc = 0;

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
		nrc_wifi_get_snr(&snr);
		nrc_wifi_get_rssi(&rssi);
	}

	nrc_iperf_spin_lock();
	if (mode == WIFI_MODE_STATION) {
		A("[iperf TCP Client Report for server %s, snr:%u, rssi:%d]\n", ipaddr_ntoa(&option->addr), snr, rssi);
	} else {
		A("[iperf TCP Client Report for server %s]\n", ipaddr_ntoa(&option->addr));
	}
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

	option->mBufLen = TCP_MSS;
	option->client_info.start_time = start_time;

	vif = wifi_get_vif_id(&option->addr);

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

	char client_str[INET_ADDRSTRLEN];
	uint8_t snr = 0;
	int8_t rssi = 0;
	tWIFI_DEVICE_MODE mode;

	nrc_wifi_get_device_mode(0, &mode);

	nrc_iperf_spin_lock();
	/* mode is 1 if softAP, 0 if station */
	/* For SoftAP, rssi is meaningless */
	A("\n[iperf TCP Server Report for client %s",
	  inet_ntop(AF_INET, &client->client_addr, client_str, INET_ADDRSTRLEN));
	if (mode) {
		A("]\n");
	} else {
		nrc_wifi_get_snr(&snr);
		nrc_wifi_get_rssi(&rssi);
		A(", snr:%u, rssi:%d]\n", snr, rssi);
	}
	A("[%3d]  %4.1f - %4.1f sec  %7sBytes  %7sbits/sec\n",
					client->sock,
					start_time, stop_time,
					byte_to_string(byte), bps_to_string(bps));
	nrc_iperf_spin_unlock();
}

static void iperf_tcp_server_recv (iperf_opt_t *option, char *buf, int len)
{
	iperf_time_t now;

	iperf_get_time(&now);
	option->server_info.stop_time = now;

	if (len > 0) {
		if (option->server_info.recv_byte == 0) {
			iperf_tcp_datagram_t *datagram = (iperf_tcp_datagram_t *)buf;
			int32_t amount = ntohl(datagram->client_header.mAmount);

			if (amount >= 0)
				option->server_info.send_byte = amount;
			else
				option->server_info.send_time = -amount / 100; // sec

			option->server_info.start_time = now;
			option->server_info.start = true;
		}
		option->server_info.recv_byte += len;
	}
}

static int tcp_process_input(struct client_data *client)
{
	iperf_time_t now;
	int received = 0;
	char buffer[IPERF_DEFAULT_DATA_BUF_LEN];

	iperf_get_time(&now);
	memset(buffer, 0, sizeof(buffer));
	if ((received = read(client->sock, buffer, sizeof(buffer))) > 0) {
		iperf_tcp_server_recv(client->option, buffer, received);
	} else {
		return -1;
	}
	return 0;
}

static int tcp_new_client(iperf_opt_t *option)
{
	struct sockaddr_in incoming;
	socklen_t len = sizeof(incoming);
	int client_sock = -1;
	struct client_data *c_data = NULL;
	char local_addr[INET_ADDRSTRLEN];
	char client_addr[INET_ADDRSTRLEN];
	if ((client_sock = accept(option->mSock, (struct sockaddr *) &incoming, &len)) < 0) {
		return -1;
	}

	if (fcntl(client_sock, F_SETFL, O_NDELAY) == -1) {
		return -1;
	}

	c_data = (struct client_data *) mem_malloc(sizeof(struct client_data));
	if (!c_data) {
		return -1;
	}
	memset(c_data, 0, sizeof(struct client_data));

	c_data->sock = client_sock;
	memcpy(&c_data->client_addr, &incoming.sin_addr, sizeof(struct in_addr));
	c_data->option = (iperf_opt_t *) mem_malloc(sizeof(iperf_opt_t));
	memcpy(c_data->option, option, sizeof(iperf_opt_t));
	c_data->next = client_list;
	client_list = c_data;

	if (client_sock > maxdesc) {
		maxdesc = client_sock;
	}

	nrc_iperf_spin_lock();
	A("\n[%3d] local %s port %d connected with %s port %d\n",
	  client_sock,
	  inet_ntop(AF_INET, &c_data->option->addr, local_addr, INET_ADDRSTRLEN),
	  option->mPort,
	  inet_ntop(AF_INET, &incoming.sin_addr, client_addr, INET_ADDRSTRLEN),
	  ntohs(incoming.sin_port));
	nrc_iperf_spin_unlock();

	return 0;
}

static void tcp_close_socket(struct client_data *client)
{
	struct client_data *tmp;

	if (!client) {
		return;
	}

	close(client->sock);

	if (client->sock == maxdesc) {
		maxdesc--;
	}

	if (client == client_list) {
		client_list = client_list ->next;
	} else {
		for (tmp = client_list; (tmp->next != client) && tmp; tmp = tmp->next);
		tmp->next = client->next;
	}

	iperf_tcp_server_report(client);
	mem_free(client->option);
	mem_free(client);
}

static void tcp_server_loop(iperf_opt_t *option)
{
	fd_set input_set, output_set, exec_set;
	struct client_data *client;
	struct client_data *next_client;

	maxdesc = option->mSock;

	while (!option->mForceStop) {
		FD_ZERO(&input_set);
		FD_ZERO(&output_set);
		FD_ZERO(&exec_set);
		FD_SET(option->mSock, &input_set);

		for (client = client_list; client; client = next_client) {
			next_client = client->next;
			FD_SET(client->sock, &input_set);
			FD_SET(client->sock, &output_set);
			FD_SET(client->sock, &output_set);
		}

		if (select(maxdesc + 1, &input_set, &output_set, &exec_set, NULL) < 0) {
			return;
		}

		if (FD_ISSET(option->mSock, &input_set)) {
			if (tcp_new_client(option) < 0) {
				system_printf("error new connection\n");
			}
		}

		for (client = client_list; client; client = next_client) {
			next_client = client->next;
			if (FD_ISSET(client->sock, &input_set)) {
				if (tcp_process_input(client) < 0) {
					FD_CLR(client->sock, &input_set);
					FD_CLR(client->sock, &output_set);
					tcp_close_socket(client);
				}
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
	client_list = NULL;
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
