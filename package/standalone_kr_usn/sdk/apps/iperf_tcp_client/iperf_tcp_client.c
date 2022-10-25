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
#include "lwip/api.h"

#include "wifi_config_setup.h"
#include "wifi_connect_common.h"

#define REMOTE_PORT 5001
#define RECV_BUF_SIZE (1024 * 2)
#define DEFAULT_DURATION 10

int sockfd;

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

static void tcp_client_task(void *pvParameters)
{
	char rx_buffer[RECV_BUF_SIZE];
	int ret = 0;
	struct sockaddr_in dest_addr;
	WIFI_CONFIG *param = pvParameters;
	int count = 0;
	void *txptr;

	uint32_t txlen_max =0;
	uint32_t txlen =0;

	uint32_t total_sent =0;
	uint32_t start_time =0;
	uint32_t diff_ms =0;
	uint32_t flags = lwip_htonl(0);
	uint32_t num_threads = lwip_htonl(1);
	uint32_t remote_port = lwip_htonl(REMOTE_PORT);
	uint32_t buffer_len = lwip_htonl(0);
	uint32_t win_band = lwip_htonl(0);
	uint32_t amount = (int)lwip_htonl((DEFAULT_DURATION * 100)*(-1));
	bool send_more = true;
	uint8_t apiflags;

	void* tcp_data = nrc_mem_malloc(TCP_MSS+1);
	memset(tcp_data, 0x0, TCP_MSS);

	/* Assign test is running */
	param->test_running = 1;

	/* build the destination's Internet address */
	bzero((char *) &dest_addr, sizeof(dest_addr));

	// Filling server information
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(REMOTE_PORT);
	dest_addr.sin_addr.s_addr = inet_addr((const char *)param->remote_addr);

	sockfd =  socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		error_val = -1;
		nrc_usr_print("Fail to create socket\n");
		goto exit;
	}
	nrc_usr_print("Socket created, connecting to %s:%d\n", (const char *)param->remote_addr, REMOTE_PORT);

	int err = connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
	if (err != 0) {
		error_val = -1;
		nrc_usr_print("Fail to connect\n");
		goto exit;
	}
	nrc_usr_print("Successfully connected\n");

	memcpy(tcp_data, &flags, 4);
	memcpy(tcp_data+4, &num_threads, 4);
	memcpy(tcp_data+8, &remote_port, 4);
	memcpy(tcp_data+12, &buffer_len, 4);
	memcpy(tcp_data+16, &win_band, 4);
	memcpy(tcp_data+20, &amount, 4);

	start_time = sys_now();
	send_more = true;


	nrc_usr_print("\n\n------------------------------------------------------------\n");
	nrc_usr_print("NRC iPerf Client connecting to %s, TCP port %d\n", (char *)param->remote_addr, REMOTE_PORT);
	nrc_usr_print("Duration: %d Sec\n", DEFAULT_DURATION);
	nrc_usr_print("------------------------------------------------------------\n");

	while (1) {
		uint32_t now = sys_now();
		diff_ms = now - start_time;

		if (diff_ms >= DEFAULT_DURATION * 1000) {
			send_more = false;
		}

		if (total_sent < 24) {
			/* transmit the settings a first time */
			txlen_max = (u16_t)(24 - total_sent);
			txptr = tcp_data+(total_sent);
		} else if (total_sent < 48) {
			/* transmit the settings a second time */
			txlen_max = (u16_t)(total_sent);
			txptr = tcp_data+(total_sent - 24);
		} else {
			/* transmit data */
			txlen_max = TCP_MSS;
			txptr = (void*)payload;

		}
		txlen = txlen_max;

		ret = send(sockfd, (const void *)txptr, txlen, 0);
		if(ret != txlen){
			nrc_usr_print("send: %d\n", ret);
		}

		total_sent += ret;

		if (!send_more)
			break;
	}

	nrc_usr_print("-------------------------------------------------------------------------------------\n");
	nrc_usr_print("[IPERF Report] TCP client\n");
	nrc_usr_print("[IPERF Report] Remote: %s:%d, %lu [Bytes], Duration:%d [msec], %d [kbits/s]\n", \
	param->remote_addr,REMOTE_PORT, total_sent,  diff_ms, (total_sent * 8 / diff_ms));
	nrc_usr_print("-------------------------------------------------------------------------------------\n");

exit:
	if (sockfd >= 0) {
		nrc_usr_print("Shutting down and close socket\n");
		shutdown(sockfd, SHUT_RDWR);
		close(sockfd);
	}

	if(tcp_data)
		nrc_mem_free(tcp_data);

	param->test_running = 0;
	vTaskDelete(NULL);
}


static void tcp_receive_task(void* arg)
{
    int ret = 0;
    void* tcp_buf_p = nrc_mem_malloc(RECV_BUF_SIZE);

    while (1) {
        ret = recv(sockfd, tcp_buf_p, RECV_BUF_SIZE, 0);

        if (ret > 0) {
            nrc_usr_print("[%s] received %d bytes.\n",__func__, ret);
        }
    }
}
/******************************************************************************
 * FunctionName : run_iperf_tcp_client
 * Description  : iperf test for tcp client
 * Parameters   : WIFI_CONFIG
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_iperf_tcp_client(WIFI_CONFIG *param)
{
	int network_index = 0;
	tWIFI_STATE_ID wifi_state = WIFI_STATE_INIT;
	TaskHandle_t tcp_receive_task_handle;
	SCAN_RESULTS results;
	int i = 0;
	int ssid_found =false;

	nrc_usr_print("[%s] Sample App for run_iperf_tcp_client \n",__func__);

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

	UBaseType_t p = uxTaskPriorityGet(NULL);
	xTaskCreate(tcp_receive_task, "tcp_receive_task",
		4096, NULL, p, &tcp_receive_task_handle);

	xTaskCreate(tcp_client_task, "tcp_client", 4096, (void*)param, 5, NULL);
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

	nrc_usr_print("[%s] End of run_iperf_tcp_client!! \n",__func__);

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

	set_wifi_config(param);
	ret = run_iperf_tcp_client(param);
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
	if(param){
		nrc_mem_free(param);
	}
}
