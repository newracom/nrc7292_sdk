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

//#define NVS_USE 1

#if defined(NVS_USE) && (NVS_USE == 1)
#include "nrc_sdk.h"
#include "nvs.h"
#include "nvs_flash.h"
nvs_handle_t nvs_handle;
#endif

#define MAX_RETRY 10

/* TIM or NonTIM deep sleep (select one depending on services)*/
#define TIM_DEEPSLEEP 1 //TIM (1) NonTIM(0)
/* in ms. STA can enter deep sleep if there is no traffic during timeout */
#define IDLE_TIMEOUT 100 // ms
/* in ms. STA wakes up if sleep time is expired during deep sleep*/
#define SLEEP_TIME_MS 0 // ms

//#define WAKEUP_GPIO_PIN 15

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

#if defined(NVS_USE) && (NVS_USE == 1)
#define NVS_PS_DEEPSLEEP_MODE "ps_mode"
#define NVS_PS_IDLE_TIMEOUT "ps_idle"
#define NVS_PS_SLEEP_TIME "ps_sleep"

int set_nvs_ps_setting(uint8_t ps_mode, uint32_t idle_time, uint32_t sleep_time)
{
	int retry_cnt = 0;
	while(1){
		if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
			break;
		} else {
			_delay_ms(1000);
			if(retry_cnt == 10)
				return -1;
		}
	}
	nvs_set_u8(nvs_handle, NVS_PS_DEEPSLEEP_MODE, ps_mode);
	nvs_set_u32(nvs_handle, NVS_PS_IDLE_TIMEOUT, idle_time);
	nvs_set_u32(nvs_handle, NVS_PS_SLEEP_TIME, sleep_time);
	nvs_close(nvs_handle);

	return 0;
}

int get_nvs_ps_setting(uint8_t *ps_mode, uint32_t *idle_time, uint32_t *sleep_time)
{
	nvs_err_t err = NVS_OK;
	int retry_cnt = 0;
	while(1){
		if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
			break;
		} else {
			_delay_ms(1000);
			if(retry_cnt == 10)
				return -1;
		}
	}

	err = nvs_get_u8(nvs_handle, NVS_PS_DEEPSLEEP_MODE, ps_mode);
	if (NVS_ERR_NVS_NOT_FOUND == err) { /* no configuration set */
		return -1;
	}

	err = nvs_get_u32(nvs_handle, NVS_PS_IDLE_TIMEOUT, idle_time);
	if (NVS_ERR_NVS_NOT_FOUND == err) { /* no configuration set */
		return -1;
	}

	err = nvs_get_u32(nvs_handle, NVS_PS_SLEEP_TIME, sleep_time);
	if (NVS_ERR_NVS_NOT_FOUND == err) { /* no configuration set */
		return -1;
	}

	nvs_close(nvs_handle);

	return 0;
}
#endif

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

	uint8_t ps_mode = TIM_DEEPSLEEP;
	uint32_t ps_idle_timeout_ms = IDLE_TIMEOUT;
	uint32_t ps_sleep_time_ms = SLEEP_TIME_MS;

	nrc_usr_print("[%s] Sample App for Wi-Fi  \n",__func__);

#if defined(NVS_USE) && (NVS_USE == 1)
	if(get_nvs_ps_setting(&ps_mode, &ps_idle_timeout_ms, &ps_sleep_time_ms) < 0){
		set_nvs_ps_setting(TIM_DEEPSLEEP,IDLE_TIMEOUT,  SLEEP_TIME_MS);
	}
#endif

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

#if defined(WAKEUP_GPIO_PIN)
	nrc_ps_set_gpio_wakeup_pin(false, WAKEUP_GPIO_PIN);
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
	nrc_ps_set_gpio_direction(0xFFFFFDFF);
	nrc_ps_set_gpio_out(0x00000100);
	nrc_ps_set_gpio_pullup(0xFFFFFFFF);
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
