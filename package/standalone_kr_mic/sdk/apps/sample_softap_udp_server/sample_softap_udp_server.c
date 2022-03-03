/*
 * MIT License
 *
 * Copyright (c) 2020 Newracom, Inc.
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

#define RECV_BUF_SIZE 256

static int error_val = 0;
static void softap_udp_server_task(void *pvParameters)
{
	int opt = 1;
	WIFI_CONFIG *param = pvParameters;
	char rx_buffer[RECV_BUF_SIZE];
	int clientlen;
	int sockfd; /* socket */
	struct sockaddr_in serveraddr, clientaddr;
	int n = 0;
	int recv_count = 0;
	char *MSGEND= "#*stop_server";

	param->test_running = 1;

	//create a socket
	if( (sockfd = socket(AF_INET , SOCK_DGRAM , 0)) < 0){
		error_val = -1;
		nrc_usr_print("ERROR opening socket");
		goto exit;
	}

	if( setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt)) < 0 ){
		error_val = -1;
		nrc_usr_print("setsockopt");
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
	nrc_usr_print("Listener on port %d \n", PORT);

	clientlen = sizeof(clientaddr);

	while (1) {
		bzero(rx_buffer, RECV_BUF_SIZE);
		n = recvfrom(sockfd, rx_buffer, RECV_BUF_SIZE, 0,
			 (struct sockaddr *) &clientaddr, (socklen_t *)&clientlen);
		if (n < 0){
			error_val = -1;
			nrc_usr_print("ERROR in recvfrom\n");
			goto exit;
		}
		nrc_usr_print("server received datagram from %s, port : %d\n",\
				inet_ntoa(clientaddr.sin_addr) , ntohs(clientaddr.sin_port));
		nrc_usr_print("server received %d/%d bytes: %s\n", n, sizeof(rx_buffer), rx_buffer);

		n = sendto(sockfd, rx_buffer, strlen(rx_buffer), 0,
			   (struct sockaddr *) &clientaddr, clientlen);
		if (n < 0){
			error_val = -1;
			nrc_usr_print("ERROR in sendto");
			goto exit;
		}

		nrc_usr_print("Data recvd count :  %d\n", ++recv_count);
		if(strncmp(rx_buffer, MSGEND, strlen(MSGEND)) == 0) {
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
 * FunctionName : run_sample_softap_udp_server
 * Description  : sample test for softap
 * Parameters   : count(test count), interval(test interval)
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_softap_udp_server(WIFI_CONFIG* param)
{
	int i = 0;
	int count =0;
	int network_index = 0;
	int dhcp_server =0;
	tWIFI_STATE_ID wifi_state = WIFI_STATE_INIT;

	count = param->count;
	dhcp_server = param->dhcp_server;

	/* set initial wifi configuration */
	while(1){
		if (wifi_init(param)== WIFI_SUCCESS) {
			nrc_usr_print ("[%s] wifi_init Success !! \n", __func__);
			break;
		} else {
			nrc_usr_print ("[%s] wifi_init Failed !! \n", __func__);
			_delay_ms(1000);
		}
	}

	if (wifi_start_softap(param)!= WIFI_SUCCESS) {
		nrc_usr_print ("[%s] ASSERT! Fail to start softap\n", __func__);
		return NRC_FAIL;
	}

	if (nrc_wifi_softap_set_ip((char *)&param->ap_ip) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail set AP's IP\n", __func__);
		return NRC_FAIL;
	}

	if (dhcp_server == 1) {
		nrc_usr_print("[%s] Trying to start DHCP Server\n",	__func__);
		if(nrc_wifi_softap_start_dhcp_server() != WIFI_SUCCESS) {
			nrc_usr_print("[%s] Fail to start dhcp server\n", __func__);
			return NRC_FAIL;
		}
	}

	xTaskCreate(softap_udp_server_task, "softap_udp_server_task", 4096, (void*)param, 5, NULL);
	while(param->test_running){
		_delay_ms(1);
	}

	nrc_wifi_get_state(&wifi_state);
	if (wifi_state == WIFI_STATE_GET_IP || wifi_state == WIFI_STATE_CONNECTED) {
		nrc_usr_print("[%s] Trying to DISCONNECT... for exit\n",__func__);
		if (nrc_wifi_disconnect(network_index) != WIFI_SUCCESS) {
			nrc_usr_print ("[%s] Fail for Wi-Fi disconnection (results:%d)\n", __func__);
			return NRC_FAIL;
		}
	}

	if (error_val < 0)
		return NRC_FAIL;

	nrc_usr_print("[%s] End of user_init!! \n",__func__);

	return NRC_SUCCESS;
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

	nrc_uart_console_enable(true);

	param = nrc_mem_malloc(WIFI_CONFIG_SIZE);
	memset(param, 0x0, WIFI_CONFIG_SIZE);

	set_wifi_softap_config(param);
	ret = run_sample_softap_udp_server(param);
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
	if(param){
		nrc_mem_free(param);
	}
}
