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

/* If the server echos back the data sent, enable HANDLE_SERVER_ECHO */
#define HANDLE_SERVER_ECHO 1
#define CLIENT_DATA_DEBUG 0

#define MAX_RETRY 10

/* tcp send operation */
#define TCP_SEND 1 // Send TCP data : 1 or 0 (1:enable, 0:disable)
#define TCP_SEND_SIZE 128 // Bytes

#ifdef NRC7292
//#define WAKEUP_GPIO_PIN 15
#else
//#define WAKEUP_GPIO_PIN 25
#endif

#define TCP_SND_RECV_TIMEOUT 15 /* in sec */
#define TCP_SND_RECV_RETRY_MAX 4 /* Timeout retry max */

static void user_operation(uint32_t delay_ms)
{
	_delay_ms(delay_ms);
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

static int connect_to_server(WIFI_CONFIG *param)
{
	int sockfd;
	struct sockaddr_in dest_addr;
	int ret = 0;
	int flag;
	struct timeval tv;

	/* build the destination's Internet address */
	memset(&dest_addr, 0, sizeof(dest_addr));

	// Filling server information
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(param->remote_port);
	dest_addr.sin_addr.s_addr = inet_addr((const char *) param->remote_addr);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		nrc_usr_print("Fail to create socket\n");
		return sockfd;
	}
	nrc_usr_print("Connecting to server %s:%d\n", param->remote_addr, param->remote_port);

	flag = 1;
	setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (void *) &flag, sizeof(int));

	tv.tv_sec = TCP_SND_RECV_TIMEOUT;
	tv.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tv, sizeof(tv));
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));

	if (connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) != 0) {
		nrc_usr_print("Connecttion to server failed.\n");
		close(sockfd);
		sockfd = -1;
	} else {
		nrc_usr_print("Successfully connected\n");
	}

	return sockfd;
}

struct client_data {
	int sockfd;
	size_t send_size;
};

#if HANDLE_SERVER_ECHO
static void echo_client_task(void *pvParameters)
{
	struct client_data *client = (struct client_data *) pvParameters;
	char* buf = NULL;
	int received = 0;
	int total = 0;

	buf = (char*)nrc_mem_malloc(client->send_size);
	if(!buf) {
		nrc_usr_print("%s: buffer allocation failed! size:%d\n", __func__, client->send_size);
		return;
	}

#if CLIENT_DATA_DEBUG
	nrc_usr_print("Waiting for Echo.\n");
#endif
	do {
		if ((received = recv(client->sockfd , buf + received, client->send_size, 0)) > 0) {
#if CLIENT_DATA_DEBUG
			nrc_usr_print("Echo received : %d\n", received);
#endif
			total += received;
		}
		if (total >= client->send_size) {
			break;
		}
	} while (1);
#if CLIENT_DATA_DEBUG
	nrc_usr_print("Finished Echo handling.\n");
#endif
	nrc_mem_free(buf);
	vTaskDelete(NULL);
}
#endif

static void send_data_to_server(WIFI_CONFIG *param, uint32_t send_data_size)
{
	int sockfd = -1;
	char* buf = NULL;
	size_t length = send_data_size;
	int i = 0;
	int data_index = 0;
	int ret;
	int err_retry = 0;

	buf = (char*)nrc_mem_malloc(send_data_size);
	if(!buf)
		nrc_usr_print("%s: buffer allocation failed! size:%d\n", __func__, send_data_size);

	for (i = 0; i < (send_data_size-1); i++) {
		buf[i] = 'A' + data_index;
		data_index++;
		if (data_index > ('Z' - 'A')) {
			data_index = 0;
		}
	}

	buf[send_data_size-1] = '\n';

	if ((sockfd = connect_to_server(param)) >= 0) {
		nrc_usr_print ("Sending data to server...\n");

#if HANDLE_SERVER_ECHO
		struct client_data client = {.sockfd = sockfd, .send_size = send_data_size};
		xTaskCreate(echo_client_task, "echo_client_task", 4096,
						(void*)&client, uxTaskPriorityGet(NULL), NULL);
#endif

		while (1) {
			ret = send(sockfd, buf, length, 0);
			if(ret > 0) {
				nrc_usr_print("%s sent %d bytes\n", __func__, length);
				break;
			} else if(ret == 0) {
				nrc_usr_print("%s Connection closed by the peer.\n", __func__);
				break;
			} else {
				int err = errno;
				if (err == EAGAIN || err == EWOULDBLOCK) {
					if(err_retry++ < TCP_SND_RECV_RETRY_MAX) {
						_delay_ms(100);
						nrc_usr_print("%s sent retry %d\n", __func__, err_retry);
					} else {
						nrc_usr_print("%s stopped : %d(%s)\n", __func__, ret, lwip_strerr(ret));
						break;
					}
				} else {
					nrc_usr_print("%s stopped : %d(%s)\n", __func__, ret, lwip_strerr(ret));
					break;
				}
			}
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
		_delay_ms(300);
		if(buf)
			nrc_mem_free(buf);
	}
}

void enter_gpio_wakeup_mode(int wakeup_gpio)
{
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

	nrc_ps_set_gpio_wakeup_pin(false, wakeup_gpio, true);
	nrc_ps_set_wakeup_source(WAKEUP_SOURCE_GPIO);

	nrc_ps_sleep_forever();
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
	uint32_t ps_idle_timeout_ms =   param->ps_idle;
	uint32_t ps_sleep_time_ms =   param->ps_sleep;
	uint8_t tcp_send = TCP_SEND;
	uint32_t tcp_send_size = TCP_SEND_SIZE;

	nrc_usr_print("[%s] Sample App for Wi-Fi  \n",__func__);
	nrc_usr_print("[%s] ps_mode(%s) idle_timeout(%d) sleep_time(%d)\n",
		__func__, (ps_mode == 1) ? "TIM" : "NON-TIM", ps_idle_timeout_ms, ps_sleep_time_ms);

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
			_delay_ms(1000);
		}
	}

	/* connect to AP */
	while(1) {
		if (wifi_connect(param)== WIFI_SUCCESS) {
			nrc_usr_print ("[%s] connect to %s successfully !! \n", __func__, param->ssid);
			break;
		} else {
			nrc_usr_print ("[%s] Fail for connection %s\n", __func__, param->ssid);
			_delay_ms(1000);
			if (++retry_cnt > 2) {
				nrc_usr_print("(connect) Exceeded retry limit (2), going into RTC deep sleep for 5000ms.");
				nrc_ps_sleep_alone(5000);
			}
		}
	}

check_again:
	retry_cnt = 0;
	/* check if wifi-connected and IP is ready */
	while (1) {
		if (_ready_ip_address()) {
			nrc_usr_print("[%s] IP is ready\n",__func__);
			break;
		}
		nrc_usr_print("[%s] not ready (wifi_state:%d, ip_state:%d)\n",
			__func__, nrc_wifi_get_state(0), nrc_addr_get_state(0));
		_delay_ms(200);
		if (++retry_cnt > 200) {
			//waiting for connection for (total 200ms * 200 try = 40000 ms)
			nrc_usr_print("Exceeded retry limit (200), going into RTC deep sleep for 5000ms.");
			nrc_ps_sleep_alone(5000);
		}
	}

	/* send data to tcp server */
	if(tcp_send == 1 && tcp_send_size > 0){
		send_data_to_server(param, tcp_send_size);
		nrc_usr_print("[%s] tcp_send (%d) tcp_send_size(%d)\n", __func__,  tcp_send, tcp_send_size);
	}

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
		/* just wait for entering deep sleep via dynamic PS */
		_delay_ms(1000);
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
