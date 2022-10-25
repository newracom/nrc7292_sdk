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
#define LOCAL_PORT 8099
#define MAX_CLIENTS 128
#define BUFFER_SIZE (1024*12)

#define ECHO_SERVER_ENABLE 1
#define SERVER_DATA_DEBUG 0
#define SERVER_DATA_HEX_PRINT 0

static int error_val = 0;

static void softap_tcp_server_task(void *pvParameters)
{
	int opt = TRUE;
	int listen_socket , addrlen , new_socket , conn_socket[MAX_CLIENTS];
	int activity, i , sd;
	int max_sd;
	int PORT = LOCAL_PORT;
	struct sockaddr_in address;
	WIFI_CONFIG *param = pvParameters;
	char *MSGEND = "#*stop_server";
	char buffer[BUFFER_SIZE];
	int len,ret = 0;

	param->test_running = 1;

	fd_set rfds, wfds;

	for (i = 0; i < MAX_CLIENTS; i++) {
		conn_socket[i] = 0;
	}

	if( (listen_socket = socket(AF_INET , SOCK_STREAM , 0)) < 0)  {
		nrc_usr_print("socket failed\n");
		goto exit;
	}

	if( setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ){
		nrc_usr_print("setsockopt failed\n");
		goto exit;
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( PORT );

	if (bind(listen_socket, (struct sockaddr *)&address, sizeof(address))<0) {
		nrc_usr_print("bind failed\n");
		goto exit;
	}
	nrc_usr_print("Listener on port %d \n", PORT);

	if (listen(listen_socket, 3) < 0) {
		nrc_usr_print("listen failed\n");
		goto exit;
	}

	addrlen = sizeof(address);
	nrc_usr_print("Waiting for connections ...\n");

	while(TRUE)
	{
		FD_ZERO(&rfds);
		FD_SET(listen_socket, &rfds);
		max_sd = listen_socket;

		for ( i = 0 ; i < MAX_CLIENTS ; i++)
		{
			sd = conn_socket[i];
			if(sd > 0)
				FD_SET( sd , &rfds);

			if(sd > max_sd)
				max_sd = sd;
		}

		activity = select( max_sd + 1 , &rfds , NULL , NULL , NULL);
		if ((activity < 0) && (errno!=EINTR))
			nrc_usr_print("select error\n");

		if (activity == 0)
			continue;

		if (FD_ISSET(listen_socket, &rfds)) {
			FD_CLR(listen_socket, &rfds);
			if ((new_socket = accept(listen_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0){
				nrc_usr_print("accept failed\n");
				goto exit;
			}
		        fcntl(new_socket, F_SETFL, O_NONBLOCK);
			nrc_usr_print("New connection , socket fd is %d , ip is : %s , port : %d \n" ,\
				 new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

			for (i = 0; i < MAX_CLIENTS; i++)
			{
				if( conn_socket[i] == 0 ){
					conn_socket[i] = new_socket;
					nrc_usr_print("Adding to list of sockets as %d\n" , i);
					break;
				}
			}
		}

		for (i = 0; i < MAX_CLIENTS; i++)
		{
			sd = conn_socket[i];

			if (FD_ISSET( sd , &rfds)) 	{
				len = recv( sd , buffer, sizeof(buffer), 0);
				if(len < 0){
					nrc_usr_print("recv failed: errno %d\n", errno);
					shutdown(sd, 0);
					close(sd);
					conn_socket[i] = 0;
				} else if(len == 0) {
					getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
					nrc_usr_print("Host disconnected , ip %s , port %d \n" ,\
						 inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

					close( sd );
					conn_socket[i] = 0;
				} else {
					#if SERVER_DATA_DEBUG
					nrc_usr_print("[%d] Recv(%d)\n", sd, len);
					#if SERVER_DATA_HEX_PRINT
					print_hex((uint8_t*)buffer, len);
					#endif /* SERVER_DATA_HEX_PRINT */
					#endif /* SERVER_DATA_DEBUG */

					if (strncmp(buffer, MSGEND, strlen(MSGEND)) == 0)
						goto exit;

					#if ECHO_SERVER_ENABLE
					ret = send(sd, buffer, len, 0);
					if (ret == -1) {
						if (errno == EWOULDBLOCK) 	{
							FD_ZERO(&wfds);
							FD_SET(sd, &wfds);

							struct timeval timeout;
							timeout.tv_sec = 2;
							timeout.tv_usec = 0;

							ret = select(max_sd + 1, NULL, &wfds, NULL, &timeout);

							if (ret > 0)
								continue;

							if (ret == 0) 	{
								nrc_usr_print("Send Timeout\n");
							}
						}
						nrc_usr_print("send failed: errno %d\n", errno);
					} else {
					#if SERVER_DATA_DEBUG
						nrc_usr_print("[%d] Send(%d)\n", sd, ret);
					#endif /* SERVER_DATA_DEBUG */
					}
					#endif /* ECHO_SERVER_ENABLE */
				}
			}
		}
	}

exit:
	if (listen_socket >= 0) {
		nrc_usr_print("Shutting down and close socket\n");
		shutdown(listen_socket, 0);
		close(listen_socket);
	}
	param->test_running = 0;
	vTaskDelete(NULL);
}

/******************************************************************************
 * FunctionName : run_sample_softap_tcp_server
 * Description  : sample test for softap
 * Parameters   : count(test count), interval(test interval)
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_softap_tcp_server(WIFI_CONFIG* param)
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

	xTaskCreate(softap_tcp_server_task, "softap_tcp_server_task", 4096, (void*)param, 5, NULL);
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
	ret = run_sample_softap_tcp_server(param);
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
	if(param){
		nrc_mem_free(param);
	}
}
