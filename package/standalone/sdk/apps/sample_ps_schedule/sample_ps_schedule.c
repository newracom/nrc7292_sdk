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
#include "sample_ps_schedule_version.h"

#include "nvs.h"
#include "ps_config.h"

#include "wifi_network.h"
#include "fota_callback.h"

/* The duration should be at least one minute apart */

/* Data collect interval set as 2 min. */
#define COLLECT_DURATION (2 * 60 * 1000)

/* Report to server duration set as 5 min. */
#define REPORT_DURATION (5 * 60 * 1000)

/* example tests to exercise all 4 schedules */
#define SCHEDULE_3_DURATION (3 * 60 * 1000)

/* FOTA check duration */
#define SCHEDULE_4_DURATION (10 * 60 * 1000)
//#define SCHEDULE_4_DURATION (24 *60 * 60 * 1000)

#define READ_I2C_SENSOR_ENABLE 0

#define REMOTE_TCP_PORT 8099
#define MAX_RETRY 10

//#define WAKEUP_GPIO_PIN 15

static nvs_handle_t nvs_handle;
static uint64_t time_woken = 0;

static int connect_to_server(WIFI_CONFIG *param)
{
	int sockfd;
	struct sockaddr_in dest_addr;

	/* build the destination's Internet address */
	memset(&dest_addr, 0, sizeof(dest_addr));

	// Filling server information
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(REMOTE_TCP_PORT);
	dest_addr.sin_addr.s_addr = inet_addr((const char *)param->remote_addr);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		nrc_usr_print("Fail to create socket\n");
		return sockfd;
	}
	nrc_usr_print("Connecting to server %s:%d\n", param->remote_addr, REMOTE_TCP_PORT);

	if (connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) != 0) {
		nrc_usr_print("Connecttion to server failed.\n");
		close(sockfd);
		sockfd = -1;
	} else {
		nrc_usr_print("Successfully connected\n");
	}

	return sockfd;
}

/* Collect the sensor data if one is configured, otherwise save collection time. */
static void collect_sensor_data()
{
	char send_data[1024];
	size_t length = 1024;
	uint64_t rtc_time = 0;

	nrc_get_rtc(&rtc_time);
	nrc_usr_print("**** [%s] called at %llu\n", __func__, rtc_time);
	/* Open nvram */
	/* Note that nvs_init should have already called, and it is done in system start up. */
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		nrc_usr_print("[%s] nvs_open failed.\n", __func__);
		return;
	}

	if (nvs_get_str(nvs_handle, NVS_SEND_DATA, (char *)send_data, &length) != NVS_OK) {
		memset(send_data, 0, sizeof(send_data));
	}

	/* subtract sizeof (uint64_t), " <value> sec" (5), and null terminate from send_data size. */
	/* If the size exceeds available buffer, then just skip */
	if (strlen(send_data) < (sizeof(send_data) - sizeof(uint64_t) - 6)) {
		nrc_get_rtc(&rtc_time);
		sprintf(send_data + strlen(send_data), " %llu sec", rtc_time/ 1000);
	}

	if (nvs_set_str(nvs_handle, NVS_SEND_DATA, (char *)send_data) != NVS_OK) {
		nrc_usr_print("[%s] ERROR saving \"%s\" data.\n", __func__, NVS_SEND_DATA);
	}
}

static void send_data_to_server(WIFI_CONFIG *param)
{
	int sockfd = -1;
	char buf[1024] = {0, };
	size_t length = 1024;
	size_t offset = 0;
	uint64_t rtc_time = 0;

	nrc_get_rtc(&rtc_time);

	/* Report the time transmittion processed. */
	sprintf(buf, "time stamp %llu sec: ", rtc_time/ 1000);
	offset = strlen(buf);
	length -= offset;

	if (nvs_get_str(nvs_handle, NVS_SEND_DATA, buf + offset, &length) == NVS_OK) {
		if ((sockfd = connect_to_server(param)) >= 0) {
			nrc_usr_print ("Sending data to server : %s\n", buf);

			buf[strlen(buf)] = '\n';

			length = length + offset + 1;
			if (send(sockfd, buf, length, 0) > 0) {
				/* erase nvs data after sending data */
				nvs_erase_key(nvs_handle, NVS_SEND_DATA);
			} else {
				nrc_usr_print("Error occurred during sending\n");
			}
		}

		/* Sample server is simply "nc -k -l 8099" on another linux machine */
		/* For real use case, implement something to make sure the sent data actually */
		/* reached the server */
		/* For our sample, we will simply put 1 sec delay hoping the data is received */
		/* by server */
		_delay_ms(1000);

		if (sockfd >= 0) {
			nrc_usr_print("Shutting down and close socket\n");
//			shutdown(sockfd, SHUT_WR);
			close(sockfd);
		}
	}
}


static void send_alert_to_server(WIFI_CONFIG *param)
{
	int sockfd = -1;
	char buf[1024] = {0, };
	uint64_t rtc_time = 0;

	nrc_get_rtc(&rtc_time);

	/* Report the time transmittion processed. */
	sprintf(buf, "time stamp %llu sec: reporting GPIO Alert.\n", rtc_time/ 1000);

	if ((sockfd = connect_to_server(param)) >= 0) {
		nrc_usr_print ("Sending data to server : %s\n", buf);

		if (send(sockfd, buf, strlen(buf) + 1, 0) <= 0) {
			nrc_usr_print("Error occurred during sending\n");
		}
	}

	/* Sample server is simply "nc -k -l 8099" on another linux machine */
	/* For real use case, implement something to make sure the sent data actually */
	/* reached the server */
	/* For our sample, we will simply put 1 sec delay hoping the data is received */
	/* by server */
	_delay_ms(1000);

	if (sockfd >= 0) {
		nrc_usr_print("Shutting down and close socket\n");
//		shutdown(sockfd, SHUT_WR);
		close(sockfd);
	}
}

static void schedule_3_callback()
{
	uint64_t current_time;

	nrc_get_rtc(&current_time);
	nrc_usr_print("**** [%s] called at %lu\n", __func__, current_time);
}

static void schedule_4_callback()
{
	uint64_t current_time;

	nrc_get_rtc(&current_time);
	nrc_usr_print("**** [%s] called at %lu\n", __func__, current_time);
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
	/* Open nvram */
	/* Note that nvs_init should have already called, and it is done in system start up. */
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		nrc_usr_print("[%s] nvs_open failed.\n", __func__);
		return;
	}

	memset(&param, 0, WIFI_CONFIG_SIZE);
	nrc_wifi_set_config(&param);

	nrc_get_rtc(&current_time);

	nrc_usr_print("**** [%s] started. \n",__func__);

	if (connect_to_ap(&param) == NRC_SUCCESS) {
		nrc_usr_print("[%s] Sending data to server...\n", __func__);
		send_data_to_server(&param);
	}
}

/******************************************************************************
 * FunctionName : run_gpio_exception
 * Description  : Handle GPIO exception for sleep awake.
 * Parameters   : NONE
 * Returns      : NONE
 *******************************************************************************/
static void run_gpio_exception()
{
	uint64_t gpio_alert_time = 0;
	WIFI_CONFIG param;
	nrc_err_t ret;

	memset(&param, 0, WIFI_CONFIG_SIZE);
	nrc_wifi_set_config(&param);

	nrc_get_rtc(&gpio_alert_time);

	nrc_usr_print("**** [%s] %llu: Processing GPIO exception...\n", __func__, gpio_alert_time);

	/* GPIO exception code goes here */
	if (connect_to_ap(&param) == NRC_SUCCESS) {
		nrc_usr_print("[%s] Sending Alert to server...\n", __func__);
		send_alert_to_server(&param);
	}

	ret = nrc_ps_resume_deep_sleep();
	if (ret == NRC_FAIL)
		nrc_usr_print("[%s] Can not received ack of QoS NULL frame with pm1 \n", __func__);

}

/******************************************************************************
 * FunctionName : schedule_deep_sleep
 * Description  : Configure deep sleep schedule callbacks and timeouts
 * Parameters   : NONE
 * Returns      : NRC_SUCCESS or NRC_FAIL
 *******************************************************************************/
nrc_err_t schedule_deep_sleep()
{
#if defined(WAKEUP_GPIO_PIN)
	/* set check_bounce to true to use a switch to toggle GPIO interrupt */
	nrc_ps_set_gpio_wakeup_pin(true, WAKEUP_GPIO_PIN);
	nrc_ps_set_wakeup_source(WAKEUP_SOURCE_RTC|WAKEUP_SOURCE_GPIO);
#else
	nrc_ps_set_wakeup_source(WAKEUP_SOURCE_RTC);
#endif /* defined(WAKEUP_GPIO_PIN) */

	/* Set GPIO pullup/output/direction mask */
	/* The GPIO configuration should be customized based on the target board layout */
	/* If values not set correctly, the board may consume more power during deep sleep */
#if defined(SUPPORT_DEVICEWORX)
	nrc_ps_set_gpio_direction(0x3FFFFFCF);
	nrc_ps_set_gpio_out(0x20004200);
	nrc_ps_set_gpio_pullup(0x00000000);
#else
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
#endif

	/* set the callbacks for scheduled callbacks */
	if (nrc_ps_add_schedule(COLLECT_DURATION, false, collect_sensor_data) != NRC_SUCCESS) {
		return NRC_FAIL;
	}

	if (nrc_ps_add_schedule(REPORT_DURATION, true, run_scheduled_client) != NRC_SUCCESS) {
		return NRC_FAIL;
	}

	if (nrc_ps_add_schedule(SCHEDULE_3_DURATION, false, schedule_3_callback) != NRC_SUCCESS) {
		return NRC_FAIL;
	}
/*
	if (nrc_ps_add_schedule(SCHEDULE_4_DURATION, false, schedule_4_callback) != NRC_SUCCESS) {
		return NRC_FAIL;
	}
*/

	if (SAMPLE_FOTA_ENABLED) {
		if (nrc_ps_add_schedule(SCHEDULE_4_DURATION, true, fota_callback) != NRC_SUCCESS) {
			return NRC_FAIL;
		}
	}

	if (nrc_ps_add_gpio_callback(true, run_gpio_exception) != NRC_SUCCESS) {
		return NRC_FAIL;
	}

	return nrc_ps_start_schedule();
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
	VERSION_T app_version;

	nrc_uart_console_enable(true);

	app_version.major = SAMPLE_PS_SCHEDULE_MAJOR;
	app_version.minor = SAMPLE_PS_SCHEDULE_MINOR;
	app_version.patch = SAMPLE_PS_SCHEDULE_PATCH;
	nrc_set_app_version(&app_version);
	nrc_set_app_name(SAMPLE_PS_SCHEDULE_APP_NAME);

	/* Open nvram */
	/* Note that nvs_init should have already called, and it is done in system start up. */
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		nrc_usr_print("[%s] nvs_open failed.\n", __func__);
		return;
	}

	/* Remove all nvram variables to clean start */
	nrc_usr_print("[%s] ** Clean up NVS for cold boot... **\n", __func__);
	nvs_erase_key(nvs_handle, NVS_SEND_DATA);
	if (schedule_deep_sleep() == NRC_FAIL) {
		nrc_usr_print("[%s] ** schedule_deep_sleep failed... **\n", __func__);
	}
}
