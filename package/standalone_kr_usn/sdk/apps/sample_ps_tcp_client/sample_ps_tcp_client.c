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

#include "nvs.h"
#include "ps_config.h"

/* Data collect interval set as 2 min. */
#define COLLECT_DURATION (2 * 60 * 1000)

/* Report to server duration set as 5 min. */
#define REPORT_DURATION (5 * 60 * 1000)

#define READ_I2C_SENSOR_ENABLE 0

#define REMOTE_TCP_PORT 8099
#define MAX_RETRY 10
#ifndef WAKEUP_GPIO_PIN
#define WAKEUP_GPIO_PIN 15
#endif /* WAKEUP_GPIO_PIN */

#if READ_I2C_SENSOR_ENABLE
#define LIS331HH	(0x30) /* VDD:3.3v, VIL(<0.2*VDD), VIH(>0.8*VDD), VOL(<0.1*VDD), VOH(>0.9*VDD) */

#define SENSOR_I2C_CHANNEL I2C_MASTER_0
#define SENSOR_I2C_SCL 16
#define SENSOR_I2C_SDA 17
#define SENSOR_I2C_CLOCK 200000
#define SENSOR_I2C_CLOCK_SOURCE 0 /* 0:Clock controller 1:PCLK */
#define SENSOR_I2C_DATA_WIDTH I2C_8BIT
#define SENSOR_I2C_ADDR LIS331HH

i2c_device_t tempsensor_i2c;
#endif

static nvs_handle_t nvs_handle;
static uint64_t time_woken = 0;

static nrc_err_t connect_to_ap(WIFI_CONFIG *param)
{
	int network_index = 0;
	tWIFI_STATE_ID wifi_state = WIFI_STATE_INIT;

	if (wifi_init(param) != WIFI_SUCCESS) {
		nrc_usr_print ("[%s] wifi initialization failed.\n", __func__);
		return NRC_FAIL;
	}

	nrc_usr_print ("[%s] Connecting to AP - %s...\n", __func__, param->ssid);
	do {
		if (wifi_connect(param) == WIFI_SUCCESS) {
			nrc_usr_print ("[%s] Wi-Fi connection successful...\n", __func__);
			break;
		} else {
			nrc_usr_print ("[%s] Fail for connection %s\n", __func__, param->ssid);
			_delay_ms(1000);
		}
	} while (1);


	nrc_wifi_get_network_index(&network_index);

	/* check if IP is ready */
	while(1) {
		nrc_wifi_get_state(&wifi_state);
		if (wifi_state == WIFI_STATE_GET_IP) {
			nrc_usr_print("[%s] IP ...\n",__func__);
			break;
		} else{
			nrc_usr_print("[%s] Current State : %d...\n",__func__, wifi_state);
		}
		_delay_ms(1000);
	}

	nrc_usr_print("[%s] Device is online connected to %s\n",__func__, param->ssid);
	return NRC_SUCCESS;
}

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

#if READ_I2C_SENSOR_ENABLE
static void i2c_read_reg(i2c_device_t* i2c, uint8_t sad, uint8_t reg, uint8_t *value)
{
	nrc_i2c_start(i2c);
	nrc_i2c_writebyte(i2c, sad);
	nrc_i2c_writebyte(i2c, reg);

	nrc_i2c_start(i2c);
	nrc_i2c_writebyte(i2c, sad|0x1);
	nrc_i2c_readbyte(i2c, value, false);
	nrc_i2c_stop(i2c);
}

void sensor_i2c_config(i2c_device_t* i2c)
{
	i2c->pin_sda = SENSOR_I2C_SDA;
	i2c->pin_scl = SENSOR_I2C_SCL;
	i2c->clock_source =SENSOR_I2C_CLOCK_SOURCE;
	i2c->controller = SENSOR_I2C_CHANNEL;
	i2c->clock = SENSOR_I2C_CLOCK;
	i2c->width = SENSOR_I2C_DATA_WIDTH;
	i2c->address = SENSOR_I2C_ADDR;
}
#endif

/* Collect the sensor data if one is configured, otherwise save collection time. */
static nrc_err_t collect_sensor_data()
{
	char send_data[1024];
	size_t length = 1024;
	uint64_t rtc_time = 0;

	if (nvs_get_str(nvs_handle, NVS_SEND_DATA, (char *)send_data, &length) != NVS_OK) {
		memset(send_data, 0, sizeof(send_data));
	}
#if READ_I2C_SENSOR_ENABLE
	uint8_t value1;

	sensor_i2c_config(&tempsensor_i2c);
	nrc_i2c_init(&tempsensor_i2c);
	nrc_i2c_enable(&tempsensor_i2c, true);
	i2c_read_reg(&tempsensor_i2c, SENSOR_I2C_ADDR, 0x20, &value1);

	/* CTRL_REG1 default value : 0x07 */
	if (value1 == 0x07){
		nrc_usr_print("[%s] Slave Device: LIS331HH\n  - CTRL_REG1=0x%02x\n", __func__, value1);
	} else {
		nrc_usr_print("[%s] ERROR..........(0x%02x)\n", __func__, value1);
		return NRC_FAIL;
	}

	/* subtract 10 ( sec) (4)  + ( 0x%02x) (5) and null terminate) from send_data buffer size. */
	/* If the size exceeds available buffer, then just skip */
	if (strlen(send_data) < (sizeof(send_data) - sizeof(uint64_t) - 10)) {
		nrc_get_rtc(&rtc_time);
		sprintf(send_data + strlen(send_data), "%llu sec 0x%02x", rtc_time/ 1000, value1);
	}
#else
	/* subtract sizeof (uint64_t), " <value> sec" (5), and null terminate from send_data size. */
	/* If the size exceeds available buffer, then just skip */
	if (strlen(send_data) < (sizeof(send_data) - sizeof(uint64_t) - 6)) {
		nrc_get_rtc(&rtc_time);
		sprintf(send_data + strlen(send_data), " %llu sec", rtc_time/ 1000);
	}
#endif
	if (nvs_set_str(nvs_handle, NVS_SEND_DATA, (char *)send_data) != NVS_OK) {
		nrc_usr_print("[%s] ERROR saving \"%s\" data.\n", __func__, NVS_SEND_DATA);
		return NRC_FAIL;
	}

	return NRC_SUCCESS;
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

		if (sockfd >= 0) {
			nrc_usr_print("Shutting down and close socket\n");
			shutdown(sockfd, SHUT_RDWR);
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
	if (sockfd >= 0) {
		nrc_usr_print("Shutting down and close socket\n");
		shutdown(sockfd, SHUT_RDWR);
		close(sockfd);
	}
}

/******************************************************************************
 * FunctionName : run_scheduled_client
 * Description  : sample test for scheduled process
 * Parameters   : WIFI_CONFIG
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
static nrc_err_t run_scheduled_client(WIFI_CONFIG *param)
{
	uint64_t interval = 0;
	int32_t report_count = 0;
	int32_t collect_count = 0;
	uint64_t current_time = 0;

	nrc_get_rtc(&current_time);

	nrc_usr_print("[%s] Sample App for run_scheduled_client \n",__func__);

	/* Retrieve the collection and report count from saved values in nvram.*/
	/* If they don't exist, just set it to 0 to indicate free start. */
	if (nvs_get_i32(nvs_handle, NVS_COLLECT_COUNT, &collect_count) != NVS_OK) {
		collect_count = 0;
	}

	if (nvs_get_i32(nvs_handle, NVS_REPORT_COUNT, &report_count) != NVS_OK) {
		report_count = 0;
	}

	nrc_usr_print("[%s] collect_count = %u, report_count = %u\n", __func__, collect_count, report_count);

	/* Collect the data if the current time is multiple of COLLECT DURATION. */
	if ((collect_count == 0) ||
		(((int64_t) current_time - ((collect_count - 1) * COLLECT_DURATION)) >= COLLECT_DURATION)) {
		nrc_usr_print("[%s] Collecting Sensor data...\n", __func__);
		if (collect_sensor_data() != NRC_SUCCESS) {
			nrc_usr_print("[%s] collecting sensor data failed.\n", __func__);
		}
		collect_count++;
		if (nvs_set_i32(nvs_handle, NVS_COLLECT_COUNT, collect_count) != NVS_OK) {
			nrc_usr_print("[%s] error saving \"%s\" data.\n", __func__, NVS_COLLECT_COUNT);
		} else {
			nrc_usr_print("[%s] saved collect count %u\n", __func__, collect_count);
		}
	}

	/* Report the collected data to server if the current time is multiple of REPORT DURATION. */
	if ((report_count == 0) ||
		(((int64_t) current_time - ((report_count - 1) * REPORT_DURATION)) >= REPORT_DURATION)) {
		if (connect_to_ap(param) == NRC_SUCCESS) {
			nrc_usr_print("[%s] Sending data to server...\n", __func__);
			send_data_to_server(param);
			report_count++;
			if (nvs_set_i32(nvs_handle, NVS_REPORT_COUNT, report_count) != NVS_OK) {
				nrc_usr_print("[%s] ERROR saving %s data.\n", __func__, NVS_REPORT_COUNT);
			} else {
				nrc_usr_print("[%s] saved report count %u\n", __func__, report_count);
			}
		}
	}
#if defined(WAKEUP_GPIO_PIN)
	nrc_ps_set_gpio_wakeup_pin(false, WAKEUP_GPIO_PIN);
	nrc_ps_set_wakeup_source(WAKEUP_SOURCE_RTC|WAKEUP_SOURCE_GPIO);
#endif /* defined(WAKEUP_GPIO_PIN) */

	nrc_get_rtc(&current_time);

	/* Calculate next sleep interval by multiplying report/collect count by respective values. */
	/* Subtract current_time to adjust the interval. */
	if ((collect_count * COLLECT_DURATION) > (report_count * REPORT_DURATION)) {
		interval = (report_count * REPORT_DURATION) - current_time;
	} else {
		interval = (collect_count * COLLECT_DURATION) - current_time;
	}
	nrc_usr_print("[%s] sleep interval = %u\n", __func__, interval);

	/* Go to Deep Sleep. */
	nrc_ps_deep_sleep(interval);

	return NRC_SUCCESS;
}

/******************************************************************************
 * FunctionName : run_gpio_exception
 * Description  : Handle GPIO exception for sleep awake.
 * Parameters   : WIFI_CONFIG
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
static nrc_err_t run_gpio_exception(WIFI_CONFIG *param)
{
	uint64_t interval = 0;
	int32_t report_count = 0;
	int32_t collect_count = 0;
	uint64_t current_time = 0;
	uint64_t gpio_alert_time = 0;

	nrc_get_rtc(&gpio_alert_time);

	nrc_usr_print("[%s] %llu: Processing GPIO exception...\n", __func__, gpio_alert_time);

	/* Retrieve the collection and report count from saved values in nvram.*/
	/* If they don't exist, just set it to 0 to indicate free start. */
	if (nvs_get_i32(nvs_handle, NVS_COLLECT_COUNT, &collect_count) != NVS_OK) {
		collect_count = 0;
	}

	if (nvs_get_i32(nvs_handle, NVS_REPORT_COUNT, &report_count) != NVS_OK) {
		report_count = 0;
	}

	/* GPIO exception code goes here */
	if (connect_to_ap(param) == NRC_SUCCESS) {
		nrc_usr_print("[%s] Sending Alert to server...\n", __func__);
		send_alert_to_server(param);
	}

#if defined(WAKEUP_GPIO_PIN)
	nrc_ps_set_gpio_wakeup_pin(false, WAKEUP_GPIO_PIN);
	nrc_ps_set_wakeup_source(WAKEUP_SOURCE_RTC|WAKEUP_SOURCE_GPIO);
#endif /* defined(WAKEUP_GPIO_PIN) */

	nrc_get_rtc(&current_time);

	/* Calculate next sleep interval by multiplying report/collect count by respective values. */
	/* Subtract current_time to adjust the interval. */
	if ((collect_count * COLLECT_DURATION) > (report_count * REPORT_DURATION)) {
		interval = (report_count * REPORT_DURATION) - current_time;
	} else {
		interval = (collect_count * COLLECT_DURATION) - current_time;
	}

	nrc_usr_print("[%s] sleep interval = %u\n", __func__, interval);

	/* Sleep interval must be greater than 1000 ms */
	if (((int64_t) interval) >= 1000) {
		/* Go to Deep Sleep. */
		nrc_ps_deep_sleep(interval);
	}
	/* If interval calculated is less than 1000 ms,
	   call run_scheduled_client to complete regularly scheduled task. */
	else {
		run_scheduled_client(param);
	}
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
	uint8_t boot;

	nrc_uart_console_enable(true);

	nrc_get_rtc(&time_woken);

	/* Open nvram */
	/* Note that nvs_init should have already called, and it is done in system start up. */
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		nrc_usr_print("[%s] nvs_open failed.\n", __func__);
		return;
	}

	param = (WIFI_CONFIG *) nrc_mem_malloc(WIFI_CONFIG_SIZE);
	memset(param, 0, WIFI_CONFIG_SIZE);
	set_wifi_config(param);

	if (nrc_ps_wakeup_reason(&boot) == NRC_FAIL) {
		nrc_usr_print("[%s] Boot reason retrieval failed.\n", __func__);
	}

	switch(boot) {
		case WAKEUP_SOURCE_RTC:
			nrc_usr_print("[%s] ** Woke up from power save... **\n", __func__);
			ret = run_scheduled_client(param);
			break;
		case WAKEUP_SOURCE_GPIO:
			nrc_usr_print("[%s] ** Awoken by GPIO indication... **\n", __func__);
			ret = run_gpio_exception(param);
			break;
		default:
			/* If the boot reason isn't power save, remove all nvram variables to clean start */
			nrc_usr_print("[%s] ** Clean up NVS for cold boot... **\n", __func__);
			nvs_erase_key(nvs_handle, NVS_SEND_DATA);
			nvs_erase_key(nvs_handle, NVS_REPORT_COUNT);
			nvs_erase_key(nvs_handle, NVS_COLLECT_COUNT);
			ret = run_scheduled_client(param);
			break;
	}

	if(param){
		nrc_mem_free(param);
	}
}
