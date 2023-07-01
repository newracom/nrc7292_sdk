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
#define LOCAL_PORT 8099
#define MAX_CLIENTS 128
#define BUFFER_SIZE (1024*4)

#define ECHO_SERVER_ENABLE 1
#define SERVER_DATA_DEBUG 0
#define SERVER_DATA_HEX_PRINT 0

static int error_val = 0;

static void softap_tcp_server_task(void *pvParameters)
{
	WIFI_CONFIG *param = pvParameters;
	int opt = TRUE;
	int listen_socket = -1, addrlen , new_socket , conn_socket[MAX_CLIENTS];
	int activity, i , sd;
	int max_sd;
	int PORT = LOCAL_PORT;

#ifdef CONFIG_IPV6
	struct sockaddr_in6 addr;
	struct sockaddr_in6 peer;
#else
	struct sockaddr_in addr;
	struct sockaddr_in peer;
#endif
	char *MSGEND = "#*stop_server";
	char buffer[BUFFER_SIZE];
	int len,ret = 0;

	fd_set rfds, wfds;

	for (i = 0; i < MAX_CLIENTS; i++) {
		conn_socket[i] = -1;
	}

#ifdef CONFIG_IPV6
	if( (listen_socket = socket(AF_INET6 , SOCK_STREAM , 0)) < 0)  {
		nrc_usr_print("socket failed\n");
		goto exit;
	}
	addrlen = sizeof(struct sockaddr_in6);
	addr.sin6_len = addrlen;
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(PORT);
	addr.sin6_flowinfo = 0;
	addr.sin6_addr = in6addr_any;
	addr.sin6_scope_id = 1;
#else
	if( (listen_socket = socket(AF_INET , SOCK_STREAM , 0)) < 0)  {
		nrc_usr_print("socket failed\n");
		goto exit;
	}
	addrlen = sizeof(struct sockaddr_in);
	addr.sin_len = addrlen;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
#endif

	if (bind(listen_socket, (struct sockaddr *)&addr, addrlen) < 0) {
		nrc_usr_print("bind failed\n");
		goto exit;
	}

	nrc_usr_print("[TCP Server]\n");
	nrc_usr_print("Echo : %s\n", ECHO_SERVER_ENABLE ? "Enable" : "Disable");
	nrc_usr_print("Receive buffer size :%d \n", BUFFER_SIZE);
	nrc_usr_print("Listener on port %d \n", PORT);

	if (listen(listen_socket, 3) < 0) {
		nrc_usr_print("listen failed\n");
		goto exit;
	}

	nrc_usr_print("Waiting for connections ...\n");

	while(TRUE)
	{
		FD_ZERO(&rfds);
		FD_SET(listen_socket, &rfds);
		max_sd = listen_socket;

		for ( i = 0 ; i < MAX_CLIENTS ; i++) {
			sd = conn_socket[i];
			if(sd >= 0)
				FD_SET(sd , &rfds);

			max_sd = MAX(max_sd, sd);
		}

		activity = select(max_sd + 1 , &rfds , NULL , NULL , NULL);
		if ((activity < 0) && (errno!=EINTR))
			nrc_usr_print("select error\n");

		if (activity == 0)
			continue;

		if (FD_ISSET(listen_socket, &rfds)) {
			FD_CLR(listen_socket, &rfds);
			if ((new_socket = accept(listen_socket, (struct sockaddr *)&addr, (socklen_t*)&addrlen)) < 0){
				nrc_usr_print("accept failed\n");
				goto exit;
			}

#ifdef CONFIG_IPV6
			struct sockaddr_in *addr_in = (struct sockaddr_in *) &addr;
			if (addr_in->sin_family == AF_INET6) {
				ip6_addr_t ip6_addr;
				inet6_addr_to_ip6addr(&ip6_addr, &addr.sin6_addr);
				nrc_usr_print("New connection , socket fd is %d , ip is : %s , port : %d \n",
							  new_socket , ip6addr_ntoa(&ip6_addr) , ntohs(addr.sin6_port));
			} else
#endif
			{
				nrc_usr_print("New connection , socket fd is %d , ip is : %s , port : %d \n",
							  new_socket , inet_ntoa(addr.sin_addr) , ntohs(addr.sin_port));
			}

			fcntl(new_socket, F_SETFL, O_NONBLOCK);

			for (i = 0; i < MAX_CLIENTS; i++) {
				if( conn_socket[i] < 0 ) {
					conn_socket[i] = new_socket;
					nrc_usr_print("Adding socket %d to list of sockets as %d\n" , new_socket, i);
					break;
				}
			}
			continue;
		}

		for (i = 0; i < MAX_CLIENTS; i++) {
			sd = conn_socket[i];
			if (sd < 0)
				continue;

			if (FD_ISSET(sd , &rfds)) {
				FD_CLR(sd, &rfds);
				len = recv(sd , buffer, sizeof(buffer), 0);
				if(len < 0) {
					nrc_usr_print("recv failed: errno %d\n", errno);
					shutdown(sd, 0);
					close(sd);
					conn_socket[i] = -1;
				} else if (len == 0) {
					struct sockaddr_in *peer_in = (struct sockaddr_in *) &peer;
					getpeername(sd , (struct sockaddr*) &peer , (socklen_t*)&addrlen);
#ifdef CONFIG_IPV6
					if (peer_in->sin_family == AF_INET6) {
						ip6_addr_t ip6_addr;
						inet6_addr_to_ip6addr(&ip6_addr, &peer.sin6_addr);
						nrc_usr_print("Host disconnected , ip %s , port %d \n" ,
									  ip6addr_ntoa(&ip6_addr) , ntohs(peer.sin6_port));
					} else
#endif
					{
						nrc_usr_print("Host disconnected , ip %s , port %d \n" ,
									  inet_ntoa(peer_in->sin_addr) , ntohs(peer_in->sin_port));
					}

					close( sd );
					conn_socket[i] = -1;
				} else {
#if SERVER_DATA_DEBUG
					nrc_usr_print("socket %d, %d bytes received...\n", sd, len);
#if SERVER_DATA_HEX_PRINT
					print_hex((uint8_t*)buffer, len);
#endif /* SERVER_DATA_HEX_PRINT */
#endif /* SERVER_DATA_DEBUG */

					if (strncmp(buffer, MSGEND, strlen(MSGEND)) == 0)
						goto exit;

#if ECHO_SERVER_ENABLE
					FD_ZERO(&wfds);
					FD_SET(sd, &wfds);

					struct timeval timeout;
					timeout.tv_sec = 2;
					timeout.tv_usec = 0;

					ret = select(sd + 1, NULL, &wfds, NULL, &timeout);

					if (ret > 0) {
						if (FD_ISSET(sd, &wfds)) {
							FD_CLR(sd, &wfds);
							ret = send(sd, buffer, len, 0);
							if (ret == -1) {
								if (errno == EWOULDBLOCK) {
									nrc_usr_print("error send, EWOULDBLOCK\n");
								}
								nrc_usr_print("send failed: errno %d\n", errno);
							} else {
								nrc_usr_print("socket %d, %d bytes sent...\n", sd, ret);
							}
						}
					}
					else {
						nrc_usr_print("Send Timeout\n");
					}
#endif /* ECHO_SERVER_ENABLE */
				}
			}
		}
	}

exit:
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		sd = conn_socket[i];
		if (sd >= 0)
		{
			nrc_usr_print("close socket: i=%d sd=%d\n", i, sd);
			shutdown(sd, 0);
			close(sd);
			conn_socket[i] = -1;
		}
	}

	if (listen_socket >= 0) {
		nrc_usr_print("Shutting down and close socket\n");
		shutdown(listen_socket, 0);
		close(listen_socket);
	}
	vTaskDelete(NULL);
}

/******************************************************************************
 * FunctionName : run_sample_softap_tcp_server
 * Description  : sample test for softap
 * Parameters   : WIFI_CONFIG
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_softap_tcp_server(WIFI_CONFIG* param)
{
	int i = 0;
	int count =0;
	int network_index = 0;
	int dhcp_server =0;

	count = 10;
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

	if (nrc_wifi_softap_set_ip(0, (char *)&param->static_ip, (char *)&param->netmask, (char *)&param->gateway) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail set AP's IP\n", __func__);
		return NRC_FAIL;
	}

	if (dhcp_server == 1) {
		nrc_usr_print("[%s] Trying to start DHCP Server\n",	__func__);
		if(nrc_wifi_softap_start_dhcp_server(0) != WIFI_SUCCESS) {
			nrc_usr_print("[%s] Fail to start dhcp server\n", __func__);
			return NRC_FAIL;
		}
	}

	xTaskCreate(softap_tcp_server_task, "softap_tcp_server_task", 2048,
						(void*)param, uxTaskPriorityGet(NULL), NULL);

	return NRC_SUCCESS;
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
	nrc_uart_console_enable(true);

	memset(param, 0x0, WIFI_CONFIG_SIZE);

	nrc_wifi_set_config(param);
	run_sample_softap_tcp_server(param);
}
