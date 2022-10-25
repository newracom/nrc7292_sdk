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
#define LOCAL_PORT 5001

#define RX_BUF_SIZE (1024*2)
#define RECVFROM_WAITING_TIME 0 /* seconds */

#define SERVER_DATA_DEBUG 0
#define SERVER_DATA_HEX_PRINT 0
#define CLIENT_SETTING_DEBUG 0

static int error_val = 0;

/** This is the Iperf settings struct sent from the client */
typedef struct _iperf_settings {
	#define LWIPERF_FLAGS_ANSWER_TEST 0x80000000
	#define LWIPERF_FLAGS_ANSWER_NOW  0x00000001
	u32_t flags;
	u32_t num_threads; /* unused for now */
	u32_t remote_port;
	u32_t buffer_len; /* unused for now */
	u32_t win_band; /* TCP window / UDP rate: unused for now */
	u32_t amount; /* pos. value: bytes?; neg. values: time (unit is 10ms: 1/100 second) */
} iperf_settings_t;

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

static void iperf_client_setting_display(char* buffer, iperf_settings_t *settings )
{
	int i;
	uint32_t setting_temp[6];
#if CLIENT_SETTING_DEBUG
	for(i=0; i<6; i++)
		memcpy(&setting_temp[i], buffer+4*i, 4);

	settings->flags = ntohl(setting_temp[0]);
	settings->num_threads = ntohl(setting_temp[1]);
	settings->remote_port = ntohl(setting_temp[2]);
	settings->buffer_len = ntohl(setting_temp[3]);
	settings->win_band = ntohl(setting_temp[4]);
	settings->amount = ntohl(setting_temp[5]);

	nrc_usr_print("\n[Client Setting]\n");
	nrc_usr_print("--------------------------------------------------------------------------\n");
	nrc_usr_print("flags:%x\n", settings->flags);
	nrc_usr_print("num_threads:%d\n", settings->num_threads);
	nrc_usr_print("port:%d\n", settings->remote_port);
	nrc_usr_print("buffer_len:%d\n", settings->buffer_len);
	nrc_usr_print("win_band:%d\n", settings->win_band);
	if((int)settings->amount < 0){
		nrc_usr_print("time:%d[s]\n", (int)(-1*(settings->amount))/100);
	}else{
		nrc_usr_print("bytes:%d\n", settings->amount);
	}
	nrc_usr_print("--------------------------------------------------------------------------\n");
#endif /* CLIENT_SETTING_DEBUG */
}


static void iperf_udp_server_task(void *pvParameters)
{
	int opt = 1;
	WIFI_CONFIG *param = pvParameters;
	char rx_buffer[RX_BUF_SIZE];
	int clientlen;
	int sockfd; /* socket */
	struct sockaddr_in serveraddr, clientaddr;
	int ret = 0;
	int iperf_started = FALSE;
	int start_time=0, end_time=0;
	int pcount = 0;
	int total_received = 0, packet_count = -1;
	int data_size = 0;
	int report_done = FALSE;
	int outoforder_packets = 0;
	int cnt_error = 0;
	iperf_settings_t settings;

	param->test_running = 1;
	nrc_usr_print("--------------------------------------------------------------------------\n");
	nrc_usr_print("Start iperf_udp_server_task !\n",__func__);
	nrc_usr_print("--------------------------------------------------------------------------\n");

	//create a socket
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
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
	serveraddr.sin_port = htons( LOCAL_PORT );

	//bind the socket to localhost port 5001
	if (bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))<0){
		error_val = -1;
		nrc_usr_print("ERROR on binding");
		goto exit;
	}
	nrc_usr_print("Listener on port %d \n", LOCAL_PORT);

	clientlen = sizeof(clientaddr);

	bzero(rx_buffer, RX_BUF_SIZE);

	while (1) {
		ret = recv_from_timeout(sockfd, RECVFROM_WAITING_TIME, 0);
		switch (ret)
		{
			case 0: // Timeout
			case -1: // Error
				break;
			default:
				ret = recvfrom(sockfd, rx_buffer, RX_BUF_SIZE, 0,
					 (struct sockaddr *) &clientaddr, (socklen_t *)&clientlen);
		}

		if (ret > 0){
			uint32_t pc;
			memcpy(&pc, rx_buffer, sizeof(pc));
			pcount = ntohl(pc);

			if(pcount >= 0){
				if (iperf_started == FALSE) {
					start_time = sys_now();
					iperf_client_setting_display(rx_buffer+12, &settings);
					nrc_usr_print("--------------------------------------------------------------------------\n");
					nrc_usr_print("local %s port %d connected with %s port %d\n",\
					 param->static_ip, LOCAL_PORT, inet_ntoa(clientaddr.sin_addr) , ntohs(clientaddr.sin_port));
					data_size = ret;
					iperf_started = TRUE;
					report_done = FALSE;
				}

				if (pcount >= packet_count + 1) {
					if (pcount > packet_count + 1) {
						cnt_error += (pcount - 1) - packet_count;
					}
					packet_count = pcount;
				} else {
					outoforder_packets++;
					if (cnt_error > 0)
						cnt_error--;
					//nrc_usr_print(" * OUT OF ORDER - incoming packet sequence %d but expected sequence %d\n",
					//	 pcount, packet_count + 1);
				}
				total_received += ret;
				end_time = sys_now();

				#if SERVER_DATA_DEBUG
				nrc_usr_print("[%d] Recv(%d)\n", sock, len);
				#if SERVER_DATA_HEX_PRINT
				print_hex((uint8_t*)buffer, len);
				#endif /* SERVER_DATA_HEX_PRINT */
				#endif /* SERVER_DATA_DEBUG */
			} else {
				ret = sendto(sockfd, rx_buffer, data_size, 0,(struct sockaddr *) &clientaddr, clientlen);

				iperf_started = FALSE;
				if (!report_done){
					pcount =(-1*pcount);
					uint32_t duration = end_time - start_time;
					double kbits_per_second = (8U*total_received) / duration;
					double lost_percent = 100.0 * cnt_error / pcount;

					nrc_usr_print("--------------------------------------------------------------------------\n");
					nrc_usr_print("[IPERF Report] UDP Server\n");
					nrc_usr_print("[IPERF Report] %d msec  %d bytes  %.1f kbps\n", duration, total_received, kbits_per_second);
					nrc_usr_print("[IPERF Report] lost/packets : %d/%d (%.1f %%) out_of_order: %d\n",\
			  				cnt_error, pcount, (double)lost_percent, outoforder_packets);
					nrc_usr_print("--------------------------------------------------------------------------\n");
					total_received =start_time = end_time =  pcount = 0 ;
					outoforder_packets = cnt_error = 0;
					packet_count = -1;
					report_done = TRUE;
				}
			}
		}
	}

	error_val = 0;

exit:
	if (sockfd >= 0) {
		nrc_usr_print("Shutting down and close socket\n");
		shutdown(sockfd, SHUT_RDWR);
		close(sockfd);
	}
	param->test_running = 0;
	vTaskDelete(NULL);
}

/******************************************************************************
 * FunctionName : run_iperf_udp_server
 * Description  : iperf udp server
 * Parameters   : WIFI_CONFIG
 * Returns	    : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_iperf_udp_server(WIFI_CONFIG *param)
{
	int network_index = 0;
	tWIFI_STATE_ID wifi_state = WIFI_STATE_INIT;
	SCAN_RESULTS results;
	int i = 0;
	int ssid_found =false;

 	nrc_usr_print("[%s] Sample App for run_sample_udp_server \n",__func__);

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

	/* find AP */
	while(1){
		if (nrc_wifi_scan() == WIFI_SUCCESS){
			if (nrc_wifi_scan_results(&results)== WIFI_SUCCESS) {
				/* Find the ssid in scan results */
				for(i=0; i<results.n_result ; i++){
					if(strcmp((char*)param->ssid, (char*)results.result[i].ssid)== 0 ){
						ssid_found = true;
						break;
					}
				}

				if(ssid_found){
					nrc_usr_print ("[%s] %s is found \n", __func__, param->ssid);
					break;
				}
			}
		} else {
			nrc_usr_print ("[%s] Scan fail !! \n", __func__);
			_delay_ms(1000);
		}
	}

	/* connect to AP */
	while(1) {
		if (wifi_connect(param)== WIFI_SUCCESS) {
			nrc_usr_print ("[%s] connect to %s successfully !! \n", __func__, param->ssid);
			break;
		} else{
			nrc_usr_print ("[%s] Fail for connection %s\n", __func__, param->ssid);
			_delay_ms(1000);
		}
	}

	nrc_wifi_get_network_index(&network_index );

	/* check the IP is ready */
	while(1){
		nrc_wifi_get_state(&wifi_state);
		if (wifi_state == WIFI_STATE_GET_IP) {
			nrc_usr_print("[%s] IP ...\n",__func__);
			break;
		} else{
			nrc_usr_print("[%s] Current State : %d...\n",__func__, wifi_state);
	}
		   _delay_ms(1000);
	   }

	xTaskCreate(iperf_udp_server_task, "iperf_udp_server", 4096, (void*)param, 5, NULL);
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

	nrc_usr_print("[%s] End of run_sample_udp_server!! \n",__func__);

	return NRC_SUCCESS;
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : Start Code for User Application, Initialize User function
 * Parameters   : none
 * Returns	    : none
 *******************************************************************************/
void user_init(void)
{
	nrc_err_t ret;
	WIFI_CONFIG* param;

	nrc_uart_console_enable(true);

	param = nrc_mem_malloc(WIFI_CONFIG_SIZE);
	memset(param, 0x0, WIFI_CONFIG_SIZE);

	set_wifi_config(param);
	ret = run_iperf_udp_server(param);
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
	if(param){
		nrc_mem_free(param);
	}
}
