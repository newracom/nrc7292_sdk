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
#include "util_trace.h"

#include "nrc_iperf.h"
#include "nrc_iperf_tcp.h"


extern void sys_arch_msleep(u32_t delay_ms);
extern int wpa_driver_get_associate_status(void);

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

	A("[iperf UDP Client Report]\n");
	A("  Interval          Transfer      Bandwidth\n");
	A("  %4.1f - %4.1f sec  %7sBytes  %7sbits/sec\n",
					start_time, end_time,
					byte_to_string(byte), bps_to_string(bps));
	A("  sent : %ld datagrams\n", option->client_info.datagram_cnt);
}

static void iperf_udp_client_report_from_server (iperf_server_header_t *header)
{
	iperf_time_t time = ntohl(header->stop_sec) + (ntohl(header->stop_usec) / 1000000.);
	int32_t byte = ntohl(header->total_len2);
	int32_t datagrams = ntohl(header->datagrams);
	int32_t errors = ntohl(header->error_cnt);
	iperf_time_t jitter = ntohl(header->jitter1) + (ntohl(header->jitter2) / 1000000.);

	A("  Server Report:\n");
	A("  %4.1f - %4.1f sec  %7sBytes  %7sbits/sec  %.3f ms  %ld/%ld (%.2g%%)\n",
					0., time,
					byte_to_string(byte),
					bps_to_string(byte_to_bps(time, byte)),
					jitter * 1000,
					errors, datagrams,
					(double)((errors * 100.) / datagrams));
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
	buf = (iperf_udp_datagram_t *)malloc(IPERF_DEFAULT_DATA_BUF_LEN);
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
		A("Not received server fin packet\n");
	else
		iperf_udp_client_report_from_server((iperf_server_header_t *)&buf->server_header);

	free(buf);
	return ack_success;
}

uint16_t iperf_udp_client_send_delay(u32_t bandwidth, u32_t data_size)
{
	u16_t time_delay = ((8*data_size* 1000) / bandwidth);

	if(!time_delay)
		time_delay = 1;
	//A("packet delay = %d ms, bandwidth = %d  data_size = %d\n" , time_delay ,bandwidth, data_size);
	return time_delay;
}

void iperf_udp_client(void *pvParameters)
{
	int sock = -1;
	int ret;
	iperf_time_t start_time, stop_time, now;
	struct sockaddr_in addr;
	struct sockaddr_in bind = {0,};
	int count = 0;
	int tosval;
	uint16_t delay;
	uint32_t packet_count = 0;
	iperf_udp_datagram_t *datagram = NULL;
	iperf_opt_t * option  = pvParameters;

	A("%s Start\n", __func__);

	if(nrc_iperf_task_list_add(option) < 0){
		goto task_exit;
	}

	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		A("create socket failed!\n");
		goto exit;
	}

	tosval = (int)option->mTOS;
	ret = setsockopt(sock, IPPROTO_IP, IP_TOS, (void *) &tosval, sizeof(tosval));
	if (ret == -1) {
		A("Set socket IP_TOS option failed!\n");
		goto exit;
	}

	option->mSock = sock;

	bind.sin_family = PF_INET;
	bind.sin_addr.s_addr = htonl(INADDR_ANY);
	bind.sin_len = sizeof(bind);
	bind.sin_port = htons(0);

	if (bind(sock, (struct sockaddr*)&bind, sizeof(bind)) < 0){
		int err = errno;
		A("[%s] bind(%d) returned error: %d.\n", __func__, sock, -err);
		goto exit;
	}

	// Filling server information
	addr.sin_family = PF_INET;
	addr.sin_addr.s_addr = option->addr.addr;
	addr.sin_len = sizeof(addr);
	addr.sin_port = htons(option->mPort);

	datagram = (iperf_udp_datagram_t *)malloc(IPERF_DEFAULT_DATA_BUF_LEN);
	if (!datagram){
		goto exit;
	}
	memset(datagram, 0, IPERF_DEFAULT_DATA_BUF_LEN);

	iperf_udp_client_init_datagram(option, datagram);

	A("------------------------------------------------------------\n");
	A("Client connecting to %s, UDP port %d\n",
		ip4addr_ntoa((const ip4_addr_t*)ip_2_ip4(&option->addr.addr)),
		option->mPort);
	A("Sending %ld byte datagrams, Duration %4.1f sec\n",
		option->mBufLen, option->mAmount / 100.0);
	A("------------------------------------------------------------\n");

	iperf_get_time(&start_time);

	option->client_info.start_time = start_time;
	stop_time = start_time + (option->mAmount / 100.);

	delay = iperf_udp_client_send_delay(option->mAppRate,option->mBufLen);

	while (1)
	{
		iperf_get_time(&now);

		datagram->tv_sec = htonl((uint32_t)now);
		datagram->tv_usec = htonl((uint32_t)((now - ntohl(datagram->tv_sec)) * 1000000));

		if (wpa_driver_get_associate_status()== false){
			option->client_info.end_time = now;
			break;
		}

		if (option->mForceStop || (now >= stop_time)){
			datagram->id = htonl(~(packet_count-1));
			sendto(sock, (const char *)datagram, option->mBufLen, 0,
				(const struct sockaddr *) &addr, sizeof(addr));

			option->client_info.end_time = now;
			break;
		}else{
			datagram->id = htonl(packet_count++);
			sendto(sock, (const char *)datagram, option->mBufLen, 0,
				(const struct sockaddr *) &addr, sizeof(addr));
			option->client_info.datagram_cnt++;

			/* put a actual delay to calculate bandwidth exactly */
			{
				iperf_time_t dummy;
				iperf_get_time(&dummy);
				if ((dummy - now) < delay)
					sys_arch_msleep(delay - (dummy-now));
			}
		}
	}

	iperf_udp_client_report(option);
	if (wpa_driver_get_associate_status()== true)
		iperf_await_server_fin_packet(option, datagram);

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


static void iperf_udp_server_report (iperf_opt_t * option,
										iperf_time_t jitter, int32_t total, int32_t datagrams,
										int32_t errors, int32_t outoforder)
{
	iperf_time_t start_time = 0;
	iperf_time_t stop_time = option->server_info.stop_time - option->server_info.start_time;
	iperf_time_t interval = stop_time;
	uint64_t byte = option->server_info.recv_byte;
	uint32_t bps = (interval)? (byte*8)/interval : 0;

	A("[iperf UDP Server Report]\n");
	A("  %4.1f - %4.1f sec  %7sBytes  %7sbits/sec  %6.3f ms  %4ld/%5ld (%.2g%%)",
					start_time, stop_time,
					byte_to_string(byte), bps_to_string(bps),
					jitter * 1000,
					errors, total,
					(double)(errors * 100.) / total);

	if (outoforder > 0)
		A("\t OUT_OF_ORDER(%ld)\n", outoforder);
	else
		A("\n");
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

	datagram = (iperf_udp_datagram_t *)malloc(IPERF_DEFAULT_DATA_BUF_LEN);
	if (!datagram){
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
		(struct sockaddr *) &option->server_info.clientaddr, sizeof(struct sockaddr_in));
	free(datagram);
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

static void iperf_udp_server_recv (iperf_opt_t * option, char *buf, int len)
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

			A("UDP server connected with %s port %d\n",\
				inet_ntoa(option->server_info.clientaddr.sin_addr),
				ntohs(option->server_info.clientaddr.sin_port));
		} else 	{
			if (id < 0) {
				option->server_info.last_id = id;
				option->server_info.done = 1;
				id = -id;
			} else {
				option->server_info.datagram_cnt++;
				option->server_info.datagram_seq++;

				if (id < option->server_info.datagram_seq) {
					/* A("out of order: %d -> %d\n", option->server_info.datagram_seq, id); */
					option->server_info.outoforder_cnt++;
					option->server_info.datagram_seq = id;
					if(option->server_info.error_cnt>0)
						option->server_info.error_cnt--;
				} else if (id > option->server_info.datagram_seq) {
					/* A("lost datagrams: %d, %d -> %d\n", id - option->server_info.datagram_seq,
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
		A("len=%d\n", len);
	}
}

uint32_t g_delay = 0;

void iperf_udp_server(void *pvParameters)
{
	iperf_opt_t * option  = pvParameters;
	int sock = -1;
	int ret = -1;
	int bytes_received;
	socklen_t sin_size;
	char *buf = NULL;
	struct sockaddr_in server;
	struct timeval timeout, timeout_recv;
	iperf_time_t elapse_time;

	/* Setup fd_set structure */
	fd_set fds;

	A("%s Start\n", __func__);

	if(nrc_iperf_task_list_add(option) < 0){
		goto task_exit;
	}

	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if(sock < 0) {
		A("can't create socket!! exit\n");
		return;
	}
	option->mSock = sock;

	server.sin_family = PF_INET;
	server.sin_port = htons(option->mPort);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	timeout.tv_sec = 2;
	timeout.tv_usec = 0;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
		A("setsockopt failed!!");
		goto exit;
	}
	if (bind(sock, (struct sockaddr *)&server, sizeof(struct sockaddr_in)) < 0) {
		A("iperf server bind failed!! exit\n");
		goto exit;
	}

	buf = malloc(IPERF_UDP_MAX_RECV_SIZE);
	if (buf == NULL) {
		A("No memory\n");
		goto exit;
	}
	memset(buf, 0, IPERF_UDP_MAX_RECV_SIZE);

	sin_size = sizeof(struct sockaddr_in);
	timeout_recv.tv_sec = 0;
	timeout_recv.tv_usec = 0;

	while(!option->mForceStop){
		while (!option->server_info.done){
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

			if(bytes_received < 0 ){	// disconnected
				option->server_info.done = 1;
			} else {
				iperf_udp_server_recv(option, buf, bytes_received);
				elapse_time = option->server_info.stop_time - option->server_info.start_time;
				if (option->server_info.start &&
					(option->server_info.send_time > 0 && elapse_time >= option->server_info.send_time)) {
					option->server_info.done = 1;
				}
			}
		}
		if(option->server_info.start == 1){
			iperf_udp_server_report(option,
					option->server_info.jitter,
					option->server_info.datagram_seq,
					option->server_info.datagram_cnt,
					option->server_info.error_cnt,
					option->server_info.outoforder_cnt);

			iperf_udp_server_report_to_client(option);

			option->server_info.start = 0;
			option->server_info.send_byte = 0;
			option->server_info.recv_byte = 0;
			option->server_info.datagram_cnt = 0;
			option->server_info.datagram_seq = 0;
			option->server_info.error_cnt = 0;
			option->server_info.outoforder_cnt = 0;
		}
		option->server_info.done = 0;
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
