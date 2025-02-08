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
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"
#include "lwip/sockets.h"

#define MAX_RETRY 10

/* in ms. the send delay */
#define SEND_DELAY_MS 10 // ms

#define UDP_DATA_SIZE 100

#ifdef NRC7292
//#define WAKEUP_GPIO_PIN 15
#elif defined(NRC7394)
//#define WAKEUP_GPIO_PIN 17
#endif

static void prepare_test_data(uint8_t *test_data, size_t length)
{
	char j = 'A';

	for (int i = 0; i < length - 2; i++, j++) {
		if (j > 'Z') {
			j = 'A';
		}
		test_data[i] = j;
	}

	test_data[length - 2] = '\r';
	test_data[length - 1] = '\n';
}

static int udp_send_data(WIFI_CONFIG *param, uint8_t *data, size_t length)
{
	int sockfd;
	struct sockaddr_in server_addr;
	int ret = -1;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		nrc_usr_print("Failed to create DGRAM socket.\n");
		return -1;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(param->remote_port);
	server_addr.sin_addr.s_addr = inet_addr(param->remote_addr);

	ret = (int)sendto(sockfd, data, length, 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
	close(sockfd);
	return ret;
}

static bool _ready_ip_address(void)
{
	if (nrc_addr_get_state(0) == NET_ADDR_SET &&
		nrc_wifi_get_state(0) == WIFI_STATE_CONNECTED) {
		return true;
	} else {
		return false;
	}
}

/******************************************************************************
 * FunctionName : run_sample_wifi_power_save
 * Description  : sample test for wifi connect & power_save on Standalone mode
 * Parameters   : WIFI_CONFIG
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_wifi_power_save(WIFI_CONFIG *param)
{
	int retry_count = 0;
	SCAN_RESULTS results;
	char* ip_addr = NULL;
	uint32_t wakeup_source = 0;

	uint8_t ps_mode = param->ps_mode;
	uint32_t ps_idle_timeout_ms = param->ps_idle;
	uint32_t ps_sleep_time_ms = param->ps_sleep;
	uint32_t send_delay_ms = SEND_DELAY_MS;

	nrc_usr_print("[%s] Sample App for Wi-Fi  \n",__func__);
	nrc_usr_print("[%s] ps_mode(%s)\n", __func__, (ps_mode == 1) ? "TIM" : "NON-TIM");
	nrc_usr_print("[%s] idle_timeout : %d ms\n", __func__, ps_idle_timeout_ms);
	nrc_usr_print("[%s] sleep_time : %d ms\n", __func__, ps_sleep_time_ms);
	nrc_usr_print("[%s] send_delay : %d ms\n", __func__, send_delay_ms);

	int i = 0;
	int ssid_found =false;
	uint8_t retry_cnt = 0;

	/* set initial wifi configuration */
	while(1){
		if (wifi_init(param)== WIFI_SUCCESS) {
			nrc_usr_print ("[%s] wifi_init Success !! \n", __func__);
			break;
		} else {
			nrc_usr_print ("[%s] wifi_init Failed !! \n", __func__);
			_delay_ms(10);
		}
	}

	/* connect to AP */
	while(1) {
		if (wifi_connect(param)== WIFI_SUCCESS) {
			nrc_usr_print ("\n[%s] connect to %s successfully !! \n", __func__, param->ssid);
			break;
		} else {
			_delay_ms(100);
			if (++retry_cnt > 2) {
				nrc_usr_print ("[%s] Fail for connection %s\n", __func__, param->ssid);
				nrc_usr_print("(connect) Exceeded retry limit (2), going into RTC deep sleep for 5000ms.");
				nrc_ps_sleep_alone(5000);
			}
		}
	}

check_again:
	retry_cnt = 0;
	const int wifi_retry_max = 4000;
	const int wifi_retry_delay = 10;

	/* check if wifi-connected and IP is ready */
	nrc_usr_print("[%s] Check IP address ", __func__);
	while (1) {
		if (_ready_ip_address()) {
			nrc_usr_print("\n[%s] IP is ready\n",__func__);
			break;
		}

		if (++retry_cnt > wifi_retry_max) {
			//waiting for wifi connection
			nrc_usr_print("[%s] not ready (wifi_state:%d, ip_state:%d)\n",
				__func__, nrc_wifi_get_state(0), nrc_addr_get_state(0));
			nrc_usr_print("Exceeded retry limit (%d)ms, going into RTC deep sleep for 5000ms.",
				wifi_retry_delay*wifi_retry_max);
			nrc_ps_sleep_alone(5000);
		} else {
			_delay_ms(wifi_retry_delay);
		}
	}

	/* Pairing with AP should be made.*/
	/* send udp data to remote server.*/

	uint8_t udp_data[UDP_DATA_SIZE];
	int sent_size = 0;
	nrc_usr_print("[%s] Preparing data to be sent\n", __func__);
	prepare_test_data(udp_data, UDP_DATA_SIZE);
	nrc_usr_print("[%s] sending data to %s:%d\n", __func__, param->remote_addr, param->remote_port);
	sent_size = udp_send_data(param, udp_data, UDP_DATA_SIZE);
	if(sent_size > 0)
		nrc_usr_print("[%s] Message of %d bytes sent\n", __func__, UDP_DATA_SIZE, sent_size);
	else
		nrc_usr_print("[%s] Message of %d bytes sent failed\n", __func__, UDP_DATA_SIZE, sent_size);

	if(send_delay_ms > 0)
		_delay_ms(send_delay_ms);

#if defined(WAKEUP_GPIO_PIN)
	nrc_ps_set_gpio_wakeup_pin(false, WAKEUP_GPIO_PIN, true);
	wakeup_source |= WAKEUP_SOURCE_GPIO;
#endif /* defined(WAKEUP_GPIO_PIN) */

	wakeup_source |= WAKEUP_SOURCE_RTC;
	nrc_ps_set_wakeup_source(wakeup_source);

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
	nrc_ps_set_gpio_direction(0xFFF7FDC7);
	nrc_ps_set_gpio_out(0x0);
	nrc_ps_set_gpio_pullup(0x0);
#endif

	while (1) {
		if(ps_mode){
			/* TIM Mode sleep */
			/* STA can wake up if buffered unit is indicated by AP or timeout is expired.
				(if gpio is set for wakeup source, STA also can wake up by gpio input) */
			if (nrc_ps_wifi_tim_deep_sleep(ps_idle_timeout_ms, ps_sleep_time_ms) == NRC_SUCCESS) {
				if (_ready_ip_address()) {
					nrc_usr_print("[%s] IP is ready\n",__func__);
					break;
				}
			} else {
				nrc_usr_print("[%s] fail to set sleep (wifi_state:%d, ip_state:%d)\n",
					__func__, nrc_wifi_get_state(0), nrc_addr_get_state(0));
				goto check_again;
			}
		} else {
			/* NON-TIM Mode sleep */
			/* STA can wake up if timeout is expired.
				(if gpio is set for wakeup source, STA also can wake up by gpio input) */
			if (nrc_ps_deep_sleep(ps_sleep_time_ms) == NRC_SUCCESS) {
				if (_ready_ip_address()) {
					nrc_usr_print("[%s] IP is ready\n",__func__);
					break;
				}
			} else {
				nrc_usr_print("[%s] fail to set sleep (wifi_state:%d, ip_state:%d)\n",
					__func__, nrc_wifi_get_state(0), nrc_addr_get_state(0));
				goto check_again;
			}
		}
	}

	while(1) {
		if (_ready_ip_address()) {
			nrc_usr_print("Done.\n");
		} else {
			nrc_usr_print("[%s] disconnected (wifi_state:%d, ip_state:%d)\n",
				__func__, nrc_wifi_get_state(0), nrc_addr_get_state(0));
			goto check_again;
		}
	}

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
	run_sample_wifi_power_save(param);
}
