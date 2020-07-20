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
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"

#define MAX_RETRY 10
#define SLEEP_MODE 1 /* 0: POWER_SAVE_MODEM_SLEEP_MODE | 1 : POWER_SAVE_DEEP_SLEEP_MODE */
#define INTERVAL 0 /* 0: Tim | non-zero : Non - Tim (ms) */

#ifndef WAKEUP_GPIO_PIN
#define WAKEUP_GPIO_PIN 15
#endif /* WAKEUP_GPIO_PIN */


/******************************************************************************
 * FunctionName : run_sample_wifi_power_save
 * Description  : sample test for wifi connect & power_save on Standalone mode
 * Parameters   : WIFI_CONFIG
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
int  run_sample_wifi_power_save(WIFI_CONFIG *param)
{
	int i = 0;
	int network_index = 0;
	int wifi_state = WLAN_STATE_INIT;
	int retry_count = 0;
	int current_wifi_state = WLAN_STATE_INIT;

	nrc_usr_print("[%s] Sample App for Wi-Fi  \n",__func__);

	if (wifi_init(param)!= WIFI_SUCCESS) {
		nrc_usr_print ("[%s] ASSERT! Fail for init\n", __func__);
		return RUN_FAIL;
	}

	for(retry_count=0; retry_count<MAX_RETRY; retry_count++){
		if (wifi_connect(param)!= WIFI_SUCCESS) {
			nrc_usr_print ("[%s] Fail for Wi-Fi connection, retry:%d\n", __func__, retry_count);
			_delay_ms(1000);
		}else{
			nrc_usr_print ("[%s] Success for Wi-Fi connection\n", __func__);
			retry_count = 0;
			break;
		}
	}

	if (retry_count == MAX_RETRY) {
		return RUN_FAIL;
	}

	/* Check the IP is ready */
	while(1){
		current_wifi_state = nrc_wifi_get_state();
		if (current_wifi_state == WLAN_STATE_GET_IP) {
			nrc_usr_print("[%s] IP ...\n",__func__);
			break;
		}else {
			nrc_usr_print("[%s] Current State : %d...\n",__func__, current_wifi_state);
		}
		_delay_ms(1000);
	}

	_delay_ms(10000);


#if defined(WAKEUP_GPIO_PIN)
	nrc_ps_set_gpio_wakeup_pin(WAKEUP_GPIO_PIN);
	nrc_ps_set_wakeup_source(WAKEUP_SOURCE_RTC|WAKEUP_SOURCE_GPIO);
#endif /* defined(WAKEUP_GPIO_PIN) */

	/* 0(Tim), non-zero(Non-Tim)(ms) */
	nrc_ps_set_sleep(SLEEP_MODE, INTERVAL);

	nrc_usr_print("[%s] End of run_sample_wifi_power_save!! \n",__func__);

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
	ret = run_sample_wifi_power_save(param);
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
	if(param){
		nrc_mem_free(param);
	}
}
