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
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"

#define SLEEP_MODE 1 /* 0: POWER_SAVE_MODEM_SLEEP_MODE | 1 : POWER_SAVE_DEEP_SLEEP_MODE */
#define INTERVAL 0 /* 0: Tim | non-zero : Non - Tim (ms) */

#define LOCAL_PORT 0
#define REMOTE_TCP_PORT 8099
#define MAX_RETRY 10
#ifndef WAKEUP_GPIO_PIN
#define WAKEUP_GPIO_PIN 15
#endif /* WAKEUP_GPIO_PIN */

static int error_val = 0;
static char tcp_buf[100] = {0, };

static void tcp_client_task(void *pvParameters)
{
	int n = 0;
	int sockfd;
	struct sockaddr_in dest_addr;
	WIFI_CONFIG *param = pvParameters;
	int count = 0;
	char send_data[] = "This is a power save test application using tcp client";

	/* Assign test is running */
	param->test_running = 1;

	/* build the destination's Internet address */
	bzero((char *) &dest_addr, sizeof(dest_addr));

	// Filling server information
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(REMOTE_TCP_PORT);
	dest_addr.sin_addr.s_addr = inet_addr((const char *)param->remote_addr);

	sockfd =  socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		error_val = -1;
		nrc_usr_print("Fail to create socket\n");
		goto exit;
	}
	nrc_usr_print("Socket created, connecting to %s:%d\n", param->remote_addr, REMOTE_TCP_PORT);

	int err = connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
	if (err != 0) {
		error_val = -1;
		nrc_usr_print("Fail to connect\n");
		goto exit;
	} else {
		nrc_usr_print("Successfully connected\n");
	}

	sprintf(tcp_buf, "%s", send_data);
	nrc_usr_print ("Send tcp_buf(%s) to the TCP server!!\n", tcp_buf);

	n = send(sockfd, tcp_buf, sizeof(tcp_buf), 0);
	if (n < 0) {
		nrc_usr_print("Error occurred during sending\n");
	}

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
 * FunctionName : run_sample_tcp_client
 * Description  : sample test for tcp client
 * Parameters   : WIFI_CONFIG
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
int  run_sample_ps_tcp_client(WIFI_CONFIG *param)
{
	int network_index = 0;
	int wifi_state = WLAN_STATE_INIT;
	int retry_count = 0;

	nrc_usr_print("[%s] Sample App for run_sample_tcp_client \n",__func__);

	if (wifi_init(param)!= WIFI_SUCCESS) {
		nrc_usr_print ("[%s] ASSERT! Fail for init\n", __func__);
		return RUN_FAIL;
	}

	for(retry_count=0; retry_count<MAX_RETRY; retry_count++){
		if (wifi_connect(param)!= WIFI_SUCCESS) {
			nrc_usr_print ("[%s] Fail for Wi-Fi connection, retry:%d\n", __func__, retry_count);
			if (retry_count == MAX_RETRY) {
				return RUN_FAIL;
			}
			_delay_ms(1000);
		}else{
			nrc_usr_print ("[%s] Success for Wi-Fi connection\n", __func__);
			retry_count = 0;
			break;
		}
	}

	/* Check the IP is ready */
	while(1){
		wifi_state = nrc_wifi_get_state();
		if (wifi_state == WLAN_STATE_GET_IP) {
			nrc_usr_print("[%s] IP ...\n",__func__);
			break;
		} else{
			nrc_usr_print("[%s] Current State : %d...\n",__func__, wifi_state);
		}
		_delay_ms(1000);
	}

	xTaskCreate(tcp_client_task, "tcp_client", 4096, (void*)param, 5, NULL);
	while(param->test_running){
		_delay_ms(1);
	}

	_delay_ms(10000);

#if defined(WAKEUP_GPIO_PIN)
	nrc_ps_set_gpio_wakeup_pin(WAKEUP_GPIO_PIN);
	nrc_ps_set_wakeup_source(WAKEUP_SOURCE_RTC|WAKEUP_SOURCE_GPIO);
#endif /* defined(WAKEUP_GPIO_PIN) */

	/* 0(Tim), non-zero(Non-Tim)(ms) */
	nrc_ps_set_sleep(SLEEP_MODE, INTERVAL);

	nrc_usr_print("[%s] End of run_sample_ps_tcp_client!! \n",__func__);

	while(1) {
		_delay_ms(1000);
	}

	return RUN_SUCCESS;
}


/******************************************************************************
 * FunctionName : user_init
 * Description  : Start Code for User Application, Initialize User function
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void user_init(void)
{
	int ret = 0;
	WIFI_CONFIG* param;

	nrc_uart_console_enable();

	param = nrc_mem_malloc(WIFI_CONFIG_SIZE);
	memset(param, 0x0, WIFI_CONFIG_SIZE);

	set_wifi_config(param);
	ret = run_sample_ps_tcp_client(param);
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
	if(param){
		nrc_mem_free(param);
	}
}

