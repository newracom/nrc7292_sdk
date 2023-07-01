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
#include "lwip/netdb.h"
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"

#define TRUE   1
#define FALSE  0
#define PORT 8099

#define MAX_RETRY 5
#define RECV_BUF_SIZE 1600
#define ECHO_SERVER_ENABLE 1

static int error_val = 0;
static void udp_server_task(void *pvParameters)
{
	WIFI_CONFIG *param = pvParameters;
	char rx_buffer[RECV_BUF_SIZE];
	int clientlen;
	int sockfd; /* socket */

#ifdef CONFIG_IPV6
	struct sockaddr_in6 serveraddr, clientaddr;
#else
	struct sockaddr_in serveraddr, clientaddr;
#endif

	int ret = 0;
	char *MSGEND = "#*stop_server";

	fd_set read_set;

	//create a socket
#ifdef CONFIG_IPV6
	if( (sockfd = socket(AF_INET6 , SOCK_DGRAM , 0)) < 0) {
		error_val = -1;
		nrc_usr_print("ERROR opening socket");
		goto exit;
	}

	serveraddr.sin6_len = sizeof(struct sockaddr_in6);
	serveraddr.sin6_family = AF_INET6;
	serveraddr.sin6_port = htons(PORT);
	serveraddr.sin6_flowinfo = 0;
	serveraddr.sin6_addr = in6addr_any;
	serveraddr.sin6_scope_id = 1;

	clientlen = sizeof(struct sockaddr_in6);
#else
	if( (sockfd = socket(AF_INET , SOCK_DGRAM , 0)) < 0)  {
		error_val = -1;
		nrc_usr_print("ERROR opening socket");
		goto exit;
	}

	serveraddr.sin_len = sizeof(struct sockaddr_in);
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(PORT);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	clientlen = sizeof(struct sockaddr_in);
#endif


	//bind the socket to localhost port 8888
	if (bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0){
		error_val = -1;
		nrc_usr_print("ERROR on binding");
		goto exit;
	}
	nrc_usr_print("[UDP Server]\n");
	nrc_usr_print("Echo : %s\n", ECHO_SERVER_ENABLE ? "Enable" : "Disable");
	nrc_usr_print("Receive buffer size :%d \n", RECV_BUF_SIZE);
	nrc_usr_print("Listener on port %d \n", PORT);

	while (1) {
		if (nrc_wifi_get_state(0) != WIFI_STATE_CONNECTED) {
				nrc_usr_print("[%s] Disconnected from AP!!\n", __func__);
				goto exit;
		}

		memset(rx_buffer, 0, RECV_BUF_SIZE);

		FD_ZERO(&read_set);
		FD_SET(sockfd, &read_set);
		if (select(sockfd + 1, &read_set, NULL, NULL, NULL) >= 0) {
			memset(&clientaddr, 0, sizeof(clientaddr));
			ret = recvfrom(sockfd, rx_buffer, RECV_BUF_SIZE, MSG_DONTWAIT,
						   (struct sockaddr *) &clientaddr, (socklen_t *)&clientlen);
			nrc_usr_print("[%s] received data with size %d...\n", __func__, ret);
			if (ret < 0){
				if (errno == EAGAIN)
					continue;

				error_val = -1;
				nrc_usr_print("ERROR in recvfrom, errno=%d\n", errno);
				goto exit;
			}
			struct sockaddr_in *client_in = (struct sockaddr_in *) &clientaddr;
#ifdef CONFIG_IPV6
			if (client_in->sin_family == AF_INET6) {
				ip6_addr_t ip6_addr;
				inet6_addr_to_ip6addr(&ip6_addr, &clientaddr.sin6_addr);

				nrc_usr_print("Server received datagram from %s, port : %d\n",
							  ip6addr_ntoa(&ip6_addr), ntohs(clientaddr.sin6_port));
			} else
#endif
			{
				nrc_usr_print("Server received datagram from %s, port : %d\n",
							  inet_ntoa(client_in->sin_addr), ntohs(client_in->sin_port));
			}
//		nrc_usr_print("server received %d bytes: %s\n", ret, rx_buffer);
#if ECHO_SERVER_ENABLE
			ret = sendto(sockfd, rx_buffer, strlen(rx_buffer), 0,
						 (struct sockaddr *) &clientaddr, clientlen);
			if (ret < 0){
				error_val = -1;
				nrc_usr_print("ERROR in sendto");
				goto exit;
			}
#endif
			if (strncmp(rx_buffer, MSGEND, strlen(MSGEND)) == 0)  {
				break;
			}

		}
	}
	error_val = 0;

exit:
	if (sockfd  >= 0) {
		nrc_usr_print("Shutting down and close socket\n");
		shutdown(sockfd, SHUT_RDWR);
		close(sockfd);
	}
	vTaskDelete(NULL);
}

/******************************************************************************
 * FunctionName : run_sample_udp_server
 * Description  : sample test for udp server
 * Parameters   : WIFI_CONFIG
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_udp_server(WIFI_CONFIG *param)
{
	tWIFI_STATE_ID wifi_state = WIFI_STATE_INIT;
	int count;

	nrc_usr_print("[%s] Sample App for run_sample_udp_server \n",__func__);

	/* set initial wifi configuration */
	if (wifi_init(param) != WIFI_SUCCESS) {
		nrc_usr_print ("[%s] ASSERT! Fail for init\n", __func__);
		return -1;
	}

	/* connect to AP */
	for(count = 0 ; count < MAX_RETRY ; ){
		if (wifi_connect(param) == WIFI_SUCCESS){
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

	xTaskCreate(udp_server_task, "udp_server", 1024,
					(void*)param, uxTaskPriorityGet(NULL), NULL);

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
	memset(param, 0x0, WIFI_CONFIG_SIZE);

	nrc_wifi_set_config(param);
	run_sample_udp_server(param);
}

