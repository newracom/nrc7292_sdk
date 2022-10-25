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
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"


#define MAX_RETRY 5
#define RECV_BUF_SIZE (1024 * 2)
#define UDP_DGRAM_SIZE 1470
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
	tWIFI_STATE_ID wifi_state;
	char rx_buffer[RECV_BUF_SIZE];
	int ret = 0;
	int destlen;
	int sockfd;
	struct sockaddr_in dest_addr;

	uint32_t total_sent =0;
	uint32_t start_time =0;
	uint32_t end_time =0;

	/* Assign test is running */
	param->test_running = 1;

	start_time = sys_now();

	sockfd =  socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		error_val = -1;
		nrc_usr_print("Fail to create socket\n");
		goto exit;
	}

	/* build the destination's Internet address */
	bzero((char *) &dest_addr, sizeof(dest_addr));

	// Filling server information
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(param->remote_port);
	dest_addr.sin_addr.s_addr = inet_addr((const char*)param->remote_addr);

	destlen = sizeof(dest_addr);
	nrc_usr_print("[UDP Client]\n");
	nrc_usr_print("Connection to %s:%d, Duration:%d[sec]\n",  inet_ntoa(dest_addr.sin_addr),
		ntohs(dest_addr.sin_port), UDP_CLIENT_TESTING_TIME/1000);

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

#if defined(TICK_TOK_TEST_ENABLE)
		int flag = count%2;
		ret = sendto(sockfd, (const char *)message[flag], sizeof(message[flag])+1, 0,
				(const struct sockaddr *) &dest_addr, sizeof(dest_addr));
#else
		ret = sendto(sockfd, (const char *)payload, UDP_DGRAM_SIZE, 0,
			(const struct sockaddr *) &dest_addr, sizeof(dest_addr));
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
		if((end_time - start_time) >=  param->duration){
			nrc_usr_print("-------------------------------------------------------------------------------------\n");
			nrc_usr_print("[UDP Client]\n");
			nrc_usr_print("%lu [Bytes], Duration:%lu [msec], %ld [kbits/s] Interval:%d [msec]\n", \
				 total_sent, (end_time - start_time), (total_sent * 8 / (end_time - start_time)), param->interval);
			nrc_usr_print("-------------------------------------------------------------------------------------\n");
			break;
		}

		_delay_ms(param->interval);
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
 * FunctionName : run_sample_udp_client
 * Description  : sample test for udp client
 * Parameters   : WIFI_CONFIG
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_udp_client(WIFI_CONFIG *param)
{
	tWIFI_STATE_ID wifi_state = WIFI_STATE_INIT;
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

	if (xTaskCreate(udp_client_task, "udp_client", 1024,
				(void*)param, uxTaskPriorityGet(NULL), NULL) == pdPASS)
		param->test_running = 1;

	while(param->test_running){
		_delay_ms(500);
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

	if (error_val < 0)
		return -1;

	nrc_usr_print("[%s] End of run_sample_udp_client!! \n",__func__);

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
	param->duration = UDP_CLIENT_TESTING_TIME;

	ret = run_sample_udp_client(param);
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
	if(param){
		free(param);
	}
}

