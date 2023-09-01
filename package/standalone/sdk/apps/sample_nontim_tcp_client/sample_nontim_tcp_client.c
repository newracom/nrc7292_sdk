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

#include "wifi_network.h"

/* Report to server duration set as 1 min */
#define REPORT_DURATION (60 * 1000)

#define REMOTE_SERVER "192.168.200.1"
#define REMOTE_TCP_PORT 8099

#define SEND_DATA_SIZE 128

static int connect_to_server()
{
	int sockfd;
	struct sockaddr_in dest_addr;

	/* build the destination's Internet address */
	memset(&dest_addr, 0, sizeof(dest_addr));

	// Filling server information
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(REMOTE_TCP_PORT);
	dest_addr.sin_addr.s_addr = inet_addr((const char *) REMOTE_SERVER);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		nrc_usr_print("Fail to create socket\n");
		return sockfd;
	}
	nrc_usr_print("Connecting to server %s:%d\n", REMOTE_SERVER, REMOTE_TCP_PORT);

	if (connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) != 0) {
		nrc_usr_print("Connecttion to server failed.\n");
		close(sockfd);
		sockfd = -1;
	} else {
		nrc_usr_print("Successfully connected\n");
	}

	return sockfd;
}

static void send_data_to_server()
{
	int sockfd = -1;
	char buf[SEND_DATA_SIZE] = {0, };
	size_t length = SEND_DATA_SIZE;
	int i = 0;
	int data_index = 0;

	for (i = 0; i < (SEND_DATA_SIZE-1); i++) {
		buf[i] = 'A' + data_index;
		data_index++;
		if (data_index > ('Z' - 'A')) {
			data_index = 0;
		}
	}

	buf[SEND_DATA_SIZE-1] = '\n';

	if ((sockfd = connect_to_server()) >= 0) {
		nrc_usr_print ("Sending data to server...\n");
		if (send(sockfd, buf, length, 0) < 0) {
			nrc_usr_print("Error occurred during sending\n");
		}
	}

	/*
	 * The TCP server is a Linux implementation utilizing the "nc" (netcat) command.
	 * (ex) nc -k -l 8099
	 */

	if (sockfd >= 0) {
		nrc_usr_print("Shutting down and close socket\n");
		close(sockfd);

		/* In order to gracefully terminate connection to the server,
		   it is necessary to give some delay to wait for FIN ACK from server and send ACK for it. */
		/* One can remove below delay and the server will eventually clean up the server socket
		   when the TCP FIN timeout reached. */
		_delay_ms(100);
	}
}


/******************************************************************************
 * FunctionName : run_scheduled_client
 * Description  : callback function for scheduled wake up
 * Parameters   : NONE
 * Returns      : NONE
 *******************************************************************************/
static void run_scheduled_client()
{
	uint64_t interval = 0;
	int32_t report_count = 0;
	int32_t collect_count = 0;
	uint64_t current_time = 0;

	WIFI_CONFIG param;
	uint8_t boot;

	nrc_uart_console_enable(true);
	memset(&param, 0, WIFI_CONFIG_SIZE);
	nrc_wifi_set_config(&param);

	nrc_usr_print("**** [%s] started. \n",__func__);

	if (connect_to_ap(&param) == NRC_SUCCESS) {
		nrc_usr_print("[%s] Sending data to server...\n", __func__);
		send_data_to_server();
	}
}

/******************************************************************************
 * FunctionName : schedule_deep_sleep
 * Description  : Configure deep sleep schedule callbacks and timeouts
 * Parameters   : NONE
 * Returns      : NRC_SUCCESS or NRC_FAIL
 *******************************************************************************/
nrc_err_t schedule_deep_sleep()
{
	nrc_ps_set_wakeup_source(WAKEUP_SOURCE_RTC);

	/* Set GPIO pullup/output/direction mask */
	/* The GPIO configuration should be customized based on the target board layout */
	/* If values not set correctly, the board may consume more power during deep sleep */
#ifdef NRC7292
	/* Below configuration is for NRC7292 EVK Revision B board */
	nrc_ps_set_gpio_direction(0x07FFFF7F);
	nrc_ps_set_gpio_out(0x0);
	nrc_ps_set_gpio_pullup(0x0);
#elif defined(NRC7394)
	/* Below configuration is for NRC7394 EVK Revision board */
	nrc_ps_set_gpio_direction(0xFFFFFDFF);
	nrc_ps_set_gpio_out(0x00000100);
	nrc_ps_set_gpio_pullup(0xFFFFFFFF);
#endif

	while (nrc_ps_deep_sleep(REPORT_DURATION) != NRC_SUCCESS) {
		nrc_usr_print("[%s] FAILED TO SLEEP!\n", __func__);
		_delay_ms(1000);
	}
	/* NOT REACHED */
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

	nrc_uart_console_enable(true);

	run_scheduled_client();

	if (schedule_deep_sleep() == NRC_FAIL) {
		nrc_usr_print("[%s] ** schedule_deep_sleep failed... **\n", __func__);
	}
}
