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

#include "nrc_iperf.h"
#include "nrc_iperf_tcp.h"

#include "util_trace.h"


#if defined(LWIP_IPERF) && (LWIP_IPERF == 1)

extern int wpa_driver_get_associate_status(void);

static void iperf_tcp_client_report (iperf_opt_t * option )
{
	iperf_time_t start_time = 0.0;
	iperf_time_t end_time = option->client_info.end_time - option->client_info.start_time;
	iperf_time_t interval = end_time;

	uint32_t byte = option->client_info.datagram_cnt * option->mBufLen;
	uint32_t bps = byte_to_bps(interval, byte);

	A("[iperf TCP Client Report]\n");
	A("     Interval        Transfer      Bandwidth\n");
	A("  %4.1f - %4.1f sec  %7sBytes  %7sbits/sec\n",
					start_time, end_time,
					byte_to_string(byte), bps_to_string(bps));
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

void iperf_tcp_client(void *pvParameters)
{
	int sock = -1;
	int ret;
	int flag;
	int tosval;
	iperf_time_t start_time, stop_time, now;
	struct sockaddr_in addr;

	iperf_tcp_datagram_t *datagram = NULL;
	iperf_opt_t * option  = pvParameters;

	A("%s Start\n", __func__);

	if(nrc_iperf_task_list_add(option) < 0){
		goto task_exit;
	}

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		A("create socket failed!\n");
		goto exit;
	}

	addr.sin_family = PF_INET;
	addr.sin_port = htons(option->mPort);
	addr.sin_addr.s_addr = option->addr.addr;

	A("Client connecting to %s, TCP port %d\n",
		ip4addr_ntoa((const ip4_addr_t*)ip_2_ip4(&option->addr.addr)),
		option->mPort);

	ret = connect(sock, (const struct sockaddr *)&addr, sizeof(addr));
	if (ret == -1) {
		A("Connect to iperf server failed!\n");
		goto exit;
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

	datagram = (iperf_tcp_datagram_t *)malloc(IPERF_DEFAULT_DATA_BUF_LEN);
	if (!datagram){
		A("memory allocation failed!\n");
		goto exit;
	}
	memset(datagram, 0, IPERF_DEFAULT_DATA_BUF_LEN);
	iperf_tcp_client_init_datagram(option, datagram);

	iperf_get_time(&start_time);

	stop_time = start_time + (option->mAmount / 100.);

	option->mBufLen = TCP_MSS;
	option->client_info.start_time = start_time;

	while (1) {
		if(wpa_driver_get_associate_status()== false){
			iperf_get_time(&now);
			option->client_info.end_time = now;
			break;
		}

		ret = send(sock, datagram, option->mBufLen, 0);
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
	if(datagram) free(datagram);
	if(sock!=-1) {
		shutdown(sock, SHUT_RDWR);
		closesocket(sock);
	}

task_exit:
	A("%s End\n", __func__);
	iperf_option_free(option);
	vTaskDelete(NULL);
}

static void iperf_tcp_server_report (iperf_opt_t * option)
{
	iperf_time_t start_time = 0;
	iperf_time_t stop_time = option->server_info.stop_time - option->server_info.start_time;
	iperf_time_t interval = stop_time;
	uint64_t byte = option->server_info.recv_byte;
	uint32_t bps = (interval)? (byte*8)/interval:0;

	A("[iperf TCP Server Report]\n");
	A("  %4.1f - %4.1f sec  %7sBytes  %7sbits/sec\n",
					start_time, stop_time,
					byte_to_string(byte), bps_to_string(bps));
}

static void iperf_tcp_server_recv (iperf_opt_t * option, char *buf, int len)
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

void iperf_tcp_server(void *pvParameters)
{
	iperf_opt_t * option  = pvParameters;
	int sock = -1;
	socklen_t sin_size;
	uint32_t *buf = NULL;
	int connected, bytes_received;
	iperf_time_t elapse_time;
	struct sockaddr_in server;
	fd_set readset;
	struct timeval timeout;
	int ret;
	int flag = 1;

	A("%s Start\n", __func__);

	if(nrc_iperf_task_list_add(option) < 0){
		goto task_exit;
	}

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		A("can't create socket!! exit\n");
		goto exit;
	}
	option->mSock = sock;

	server.sin_family = PF_INET;
	server.sin_port = htons(option->mPort);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(&(server.sin_zero), 0x0, sizeof(server.sin_zero));

	if (bind(sock, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) {
		A("iperf server bind failed!! exit\n");
		goto exit;
	}

	if (listen(sock, 5) == -1) {
		A("iperf server listen failed!! exit\n");
		goto exit;
	}

	timeout.tv_sec = 3;
	timeout.tv_usec = 0;

	buf = malloc(IPERF_DEFAULT_DATA_BUF_LEN);
	if (buf == NULL) {
		A("No memory\n");
		goto exit;
	}
	memset(buf, 0, IPERF_DEFAULT_DATA_BUF_LEN);

	while (!option->mForceStop) {
		FD_ZERO(&readset);
		FD_SET(sock, &readset);

		if (select(sock + 1, &readset, NULL, NULL, &timeout) == 0)
			continue;

		sin_size = sizeof(struct sockaddr_in);
		connected = accept(sock, (struct sockaddr *)&option->server_info.clientaddr, &sin_size);

		A("TCP client connected from (%s, %d)\n",
			inet_ntoa(option->server_info.clientaddr.sin_addr),
			 ntohs(option->server_info.clientaddr.sin_port));

		ret = setsockopt(connected, IPPROTO_TCP, TCP_NODELAY, (void *) &flag, sizeof(int));
		if (ret == -1) {
			A("Set socket option failed!\n");
			closesocket(connected);
			goto exit;
		}

		while (!option->server_info.done) {
			bytes_received = 0;
			bytes_received = recv(connected, buf, IPERF_DEFAULT_DATA_BUF_LEN, 0);

			if(bytes_received < 0 ){	// disconnected
				option->server_info.done = 1;
			}else {
				iperf_tcp_server_recv(option, (char*)buf, bytes_received);
			}

			elapse_time = option->server_info.stop_time - option->server_info.start_time;
			if ((option->server_info.send_time > 0) && (elapse_time >= option->server_info.send_time)){
				option->server_info.done = 1;
			}

			if(option->mForceStop){
				option->server_info.done = 1;
			}
		}
		A("TCP client disconnected from (%s, %d)\n",
			inet_ntoa(option->server_info.clientaddr.sin_addr),
			 ntohs(option->server_info.clientaddr.sin_port));
		iperf_tcp_server_report(option);

		if (connected >= 0)
			closesocket(connected);
		connected = -1;
		option->server_info.done = 0;
		option->server_info.start = 0;
		option->server_info.recv_byte = 0;
	}

exit:
	nrc_iperf_task_list_del(option);
	if(buf) free(buf);
	if(sock!=-1) {
		shutdown(sock, SHUT_RDWR);
		closesocket(sock);
	}

task_exit:
	A("%s End\n", __func__);
	iperf_option_free(option);
	vTaskDelete(NULL);
}
#endif /* LWIP_IPERF */
