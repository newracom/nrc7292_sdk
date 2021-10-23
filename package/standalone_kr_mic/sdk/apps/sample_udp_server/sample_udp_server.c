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

#include "nrc_sdk.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/errno.h"
#include "lwip/netdb.h"
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"


#define TRUE   1
#define FALSE  0
#define PORT 8088

#define MAX_RETRY 5
#define RECV_BUF_SIZE 1600
#define ECHO_SERVER_ENABLE 1

static int error_val = 0;
static void udp_server_task(void *pvParameters)
{
	WIFI_CONFIG *param = pvParameters;
	tWIFI_STATE_ID wifi_state;
	char rx_buffer[RECV_BUF_SIZE];
	int clientlen;
	int sockfd; /* socket */
	struct sockaddr_in serveraddr, clientaddr;
	int ret = 0;
	char *MSGEND = "#*stop_server";

	param->test_running = 1;

	//create a socket
	if( (sockfd = socket(AF_INET , SOCK_DGRAM , 0)) < 0){
		error_val = -1;
		nrc_usr_print("ERROR opening socket");
		goto exit;
	}

	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr =  htonl(INADDR_ANY);
	serveraddr.sin_port = htons( PORT );

	//bind the socket to localhost port 8888
	if (bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))<0){
		error_val = -1;
		nrc_usr_print("ERROR on binding");
		goto exit;
	}
	nrc_usr_print("[UDP Server]\n");
	nrc_usr_print("Echo : %s\n", ECHO_SERVER_ENABLE ? "Enable" : "Disable");
	nrc_usr_print("Receive buffer size :%d \n", RECV_BUF_SIZE);
	nrc_usr_print("Listener on port %d \n", PORT);

	clientlen = sizeof(clientaddr);

	while (1) {
		if(nrc_wifi_get_state(&wifi_state) != WIFI_SUCCESS) {
			nrc_usr_print("[%s] Fail to get state\n", __func__);
			goto exit;
		}

		switch (wifi_state){
			case WIFI_STATE_CONNECTED:
			case WIFI_STATE_GET_IP:
				break;
			default:
				nrc_usr_print("[%s] Disconnected from AP!!\n", __func__);
				goto exit;
		}

		bzero(rx_buffer, RECV_BUF_SIZE);
		ret = recvfrom(sockfd, rx_buffer, RECV_BUF_SIZE, MSG_DONTWAIT,
			 (struct sockaddr *) &clientaddr, (socklen_t *)&clientlen);
		if (ret < 0){
			if (errno == EAGAIN)
				continue;

			error_val = -1;
			nrc_usr_print("ERROR in recvfrom, errno=%d\n", errno);
			goto exit;
		}
		nrc_usr_print("server received datagram from %s, port : %d\n",\
				inet_ntoa(clientaddr.sin_addr) , ntohs(clientaddr.sin_port));
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
	error_val = 0;

exit:
	if (sockfd  >= 0) {
		nrc_usr_print("Shutting down and close socket\n");
		shutdown(sockfd, SHUT_RDWR);
		close(sockfd);
	}
	param->test_running = 0;
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

	/* check the IP is ready */
	while(1){
		if(nrc_wifi_get_state(&wifi_state) != WIFI_SUCCESS) {
			nrc_usr_print("[%s] Fail to get state\n", __func__);
			return -1;
		}

		if (wifi_state == WIFI_STATE_CONNECTED)
			nrc_usr_print("[%s] IP Address ...\n", __func__);
		else if (wifi_state == WIFI_STATE_GET_IP){
			char* ip_addr = NULL;

			if (nrc_wifi_get_ip_address(&ip_addr) != WIFI_SUCCESS){
				nrc_usr_print("[%s] Fail to get IP address\n", __func__);
				return -1;
			}

			nrc_usr_print("[%s] IP Address : %s\n", __func__, ip_addr);
			break;
		}
		else {
			nrc_usr_print("[%s] Disconnected from AP!!\n", __func__);
			return 0;
		}

		_delay_ms(1000);
	}

	if (xTaskCreate(udp_server_task, "udp_server", 1024,
				(void*)param, uxTaskPriorityGet(NULL), NULL) == pdPASS)
		param->test_running = 1;

	while(param->test_running){
		_delay_ms(1);
	}

	if(nrc_wifi_get_state(&wifi_state) != WIFI_SUCCESS)
		return -1;

	if (wifi_state == WIFI_STATE_GET_IP || wifi_state == WIFI_STATE_CONNECTED) {
		int network_index;
		nrc_usr_print("[%s] Trying to DISCONNECT... for exit\n",__func__);
		if (nrc_wifi_get_network_index(&network_index) != WIFI_SUCCESS){
			nrc_usr_print("[%s] Fail to get network index\n", __func__);
			return -1;
		}
		if (nrc_wifi_disconnect(network_index) != WIFI_SUCCESS) {
			nrc_usr_print ("[%s] Fail for Wi-Fi disconnection\n", __func__);
			return -1;
		}
	}

	if (error_val <0)
		return -1;

	nrc_usr_print("[%s] End of run_sample_udp_server!! \n",__func__);

	return 0;
}


/******************************************************************************
 * FunctionName : user_init
 * Description  : Start Code for User Application, Initialize User function
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void user_init(void)
{
	nrc_err_t ret;
	WIFI_CONFIG* param;

	param = malloc(WIFI_CONFIG_SIZE);
	memset(param, 0x0, WIFI_CONFIG_SIZE);

	set_wifi_config(param);
	ret = run_sample_udp_server(param);
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
	if(param){
		free(param);
	}
}

