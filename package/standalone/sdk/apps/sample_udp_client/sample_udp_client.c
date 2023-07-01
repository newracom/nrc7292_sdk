/*
 * MIT License
 *
 * Copyright (c) 2022 Newracom, Inc.
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

#include "nrc_sdk.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/errno.h"
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"


#define MAX_RETRY 5
#define RECV_BUF_SIZE (1024 * 2)
#define UDP_DGRAM_SIZE 1470
#ifdef CONFIG_IPV6
#define UDP6_DGRAM_SIZE 1440
#endif

#define RECVFROM_WAITING_TIME 0 /* seconds */
#define UDP_CLIENT_TESTING_TIME 10000 /* msec */


//#define TICK_TOK_TEST_ENABLE 1
#if defined(TICK_TOK_TEST_ENABLE)
static const uint8_t message[2][128] = {"tick", "tok"};
#endif

static int error_val = 0;
static const uint8_t payload[1600] = {
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
};

static int recv_from_timeout(int sockfd, long sec, long usec)
{
	/* Setup timeval variable */
	struct timeval timeout;
	timeout.tv_sec = sec;
	timeout.tv_usec = usec;

	/* Setup fd_set structure */
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(sockfd, &fds);

	/* Return value: -1(error occured), 0(timed out), >0(data ready to be read) */
	return select(sockfd+1 , &fds, NULL, NULL, &timeout);
}

static void udp_client_task(void *pvParameters)
{
	WIFI_CONFIG *param = pvParameters;
	char rx_buffer[RECV_BUF_SIZE];
	int ret = 0;
	int destlen;
	int sockfd;
#ifdef CONFIG_IPV6
	struct sockaddr_in6 dest_addr;
#else
	struct sockaddr_in dest_addr;
#endif
	struct sockaddr_in *addr_in = (struct sockaddr_in *) &dest_addr;
	ip_addr_t remote_addr;

	uint32_t data_size = 0;
	uint32_t total_sent =0;
	uint32_t start_time =0;
	uint32_t end_time =0;
	const int duration = UDP_CLIENT_TESTING_TIME;
	start_time = sys_now();

#ifdef CONFIG_IPV6
	sockfd =  socket(AF_INET6, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		error_val = -1;
		nrc_usr_print("Fail to create socket\n");
		goto exit;
	}
#else
	sockfd =  socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		error_val = -1;
		nrc_usr_print("Fail to create socket\n");
		goto exit;
	}
#endif

	/* build the destination's Internet address */
	memset((char *) &dest_addr, 0, sizeof(dest_addr));

	// Filling server information
	if (ipaddr_aton((char *) param->remote_addr, &remote_addr)) {
		if (IP_IS_V4(&remote_addr)) {
			nrc_usr_print("[%s] IPv4...\n", __func__);
			addr_in->sin_family = AF_INET;
			addr_in->sin_port = htons(param->remote_port);
			addr_in->sin_addr.s_addr = inet_addr((char *)param->remote_addr);

			/* data size to be sent */
			data_size = UDP_DGRAM_SIZE;
		} else {
#ifdef CONFIG_IPV6
			nrc_usr_print("[%s] IPv6...\n", __func__);
			dest_addr.sin6_family = AF_INET6;
			dest_addr.sin6_len = sizeof(struct sockaddr_in6);
			dest_addr.sin6_port = htons(param->remote_port);
			dest_addr.sin6_flowinfo = 0;
			dest_addr.sin6_scope_id = ip6_addr_zone(ip_2_ip6(&remote_addr));
			inet6_addr_from_ip6addr(&dest_addr.sin6_addr, ip_2_ip6(&remote_addr));

			/* data size to be sent */
			data_size = UDP6_DGRAM_SIZE;
#else
			nrc_usr_print("[%s] Address %s not valid or enable IPv6 support.\n", __func__, param->remote_addr);
			nrc_usr_print("[%s] use nvs set remote_address <ip address>\n", __func__);
			goto exit;
#endif
		}
	}

	destlen = sizeof(dest_addr);
	nrc_usr_print("[UDP Client]\n");
#ifdef CONFIG_IPV6
	if (addr_in->sin_family == AF_INET6) {
		ip6_addr_t ip6_addr;
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *) &dest_addr;
		inet6_addr_to_ip6addr(&ip6_addr, &addr6->sin6_addr);
		nrc_usr_print("Connection to %s port %d, Duration:%d[sec]\n",
			ip6addr_ntoa(&ip6_addr),
			ntohs(addr6->sin6_port), duration/1000);
	} else
#endif
	{
		nrc_usr_print("Connection to %s port %d, Duration:%d[sec]\n",
			inet_ntoa(addr_in->sin_addr),
			ntohs(addr_in->sin_port), duration/1000);
	}
	while (1) {
		if (nrc_wifi_get_state(0) != WIFI_STATE_CONNECTED) {
				nrc_usr_print("[%s] Disconnected from AP!!\n", __func__);
				goto exit;
		}

#if defined(TICK_TOK_TEST_ENABLE)
		int flag = count%2;
		ret = sendto(sockfd, (const char *)message[flag], sizeof(message[flag])+1, 0,
				(const struct sockaddr *) &dest_addr, sizeof(dest_addr));
		nrc_usr_print("[%s] sending data with size %d... (tick tok)\n", __func__, sizeof(message[flag]) + 1);
#else
		ret = sendto(sockfd, (const char *)payload, data_size, 0,
			(const struct sockaddr *) &dest_addr, sizeof(dest_addr));
		nrc_usr_print("[%s] sending data with size %d...\n", __func__, data_size);
#endif
		if (ret < 0) {
			error_val = -1;
			nrc_usr_print("Error occurred during sending\n");
			goto exit;
		}
		total_sent += ret;

		ret = recv_from_timeout(sockfd, RECVFROM_WAITING_TIME, 0);
		switch (ret)
		{
			case 0: // Timeout
			case -1: // Error
				break;
			default:
				ret = recvfrom(sockfd, (char *)rx_buffer, sizeof(rx_buffer)-1, 0,\
					(struct sockaddr *) &dest_addr, (socklen_t *)&destlen);
		}

		end_time = sys_now();
		if((end_time - start_time) >= duration){
			nrc_usr_print("-------------------------------------------------------------------------------------\n");
			nrc_usr_print("[UDP Client]\n");
			nrc_usr_print("%lu [Bytes], Duration:%lu [msec], %ld [kbits/s] Interval:%d [msec]\n", \
				 total_sent, (end_time - start_time), (total_sent * 8 / (end_time - start_time)), duration);
			nrc_usr_print("-------------------------------------------------------------------------------------\n");
			break;
		}

		_delay_ms(1000);
	}

	error_val = 0;

exit:
	if (sockfd >= 0) {
		nrc_usr_print("Shutting down and close socket\n");
		shutdown(sockfd, SHUT_RDWR);
		close(sockfd);
	}
	vTaskDelete(NULL);
}

/******************************************************************************
 * FunctionName : run_sample_udp_client
 * Description  : sample test for udp client
 * Parameters   : WIFI_CONFIG
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_udp_client(WIFI_CONFIG *param)
{
	int count;

	nrc_usr_print("[%s] Sample App for run_sample_udp_client \n",__func__);

	/* set initial wifi configuration */
	if (wifi_init(param)!= WIFI_SUCCESS) {
		nrc_usr_print ("[%s] ASSERT! Fail for init\n", __func__);
		return -1;
	}

	/* connect to AP */
	for(count = 0 ; count < MAX_RETRY ; ){
		if (wifi_connect(param)== WIFI_SUCCESS){
			nrc_usr_print ("[%s] connect to %s successfully !! \n", __func__, param->ssid);
			break;
		}

		if (++count == MAX_RETRY){
			nrc_usr_print ("[%s] Fail for connection %s\n", __func__, param->ssid);
			return -1;
		}

		_delay_ms(1000);
	}

	/* check if IP is ready */
	while(1){
		if (nrc_addr_get_state(0) == NET_ADDR_SET) {
			nrc_usr_print("[%s] IP ...\n",__func__);
			break;
		} else {
			nrc_usr_print("[%s] IP Address setting State : %d != NET_ADDR_SET(%d) yet...\n",
						  __func__, nrc_addr_get_state(0), NET_ADDR_SET);
		}
		_delay_ms(1000);
	}

	xTaskCreate(udp_client_task, "udp_client", 1024,
					(void*)param, uxTaskPriorityGet(NULL), NULL) ;

	return 0;
}


/******************************************************************************
 * FunctionName : user_init
 * Description  : Start Code for User Application, Initialize User function
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
WIFI_CONFIG wifi_config;
WIFI_CONFIG* param = &wifi_config;

void user_init(void)
{
	param = nrc_mem_malloc(WIFI_CONFIG_SIZE);
	memset(param, 0x0, WIFI_CONFIG_SIZE);

	nrc_wifi_set_config(param);
	run_sample_udp_client(param);
}

