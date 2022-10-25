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
nrc_err_t run_sample_wifi_power_save(WIFI_CONFIG *param)
{
	int network_index = 0;
	tWIFI_STATE_ID wifi_state = WIFI_STATE_INIT;
	int retry_count = 0;
	int current_wifi_state = WIFI_STATE_INIT;
	SCAN_RESULTS results;
	char* ip_addr = NULL;

	nrc_usr_print("[%s] Sample App for Wi-Fi  \n",__func__);

	int i = 0;
	int ssid_found =false;

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
		}else{
			nrc_usr_print ("[%s] Fail for connection %s\n", __func__, param->ssid);
			_delay_ms(1000);
		}
	}

	nrc_wifi_get_network_index(&network_index );

	/* check the IP is ready */
	while(1){
		nrc_wifi_get_state(&wifi_state);
		if (wifi_state == WIFI_STATE_GET_IP) {
			if (nrc_wifi_get_ip_address(&ip_addr) != WIFI_SUCCESS){
				nrc_usr_print("[%s] IP Address : %s\n", __func__, ip_addr);
			}
			break;
		} else{
			if (wifi_state == WIFI_STATE_CONNECTED) nrc_wifi_set_ip_address();
			nrc_usr_print("[%s] Current State : %d...\n",__func__, wifi_state);
		}
		_delay_ms(1000);
	}

	uint32_t delay_ms = 3000;
	nrc_usr_print("[%s] waiting for %u ms before entering sleep... \n",__func__, delay_ms);
	_delay_ms(delay_ms);

sleep:
#if defined(WAKEUP_GPIO_PIN)
	nrc_ps_set_gpio_wakeup_pin(false, WAKEUP_GPIO_PIN);
	nrc_ps_set_wakeup_source(WAKEUP_SOURCE_RTC|WAKEUP_SOURCE_GPIO);
#endif /* defined(WAKEUP_GPIO_PIN) */

	if (nrc_ps_set_sleep(SLEEP_MODE, INTERVAL, 0) < 0) {
		/* Something is wrong. maybe disconnected. wait for reconnection */
		while(1){
			nrc_wifi_get_state(&wifi_state);
			if (wifi_state == WIFI_STATE_CONNECTED) {
				if (nrc_wifi_set_ip_address() != WIFI_SUCCESS) {
					nrc_usr_print("[%s] Fail to Get IP Address\n", __func__);
				}
				nrc_wifi_get_state(&wifi_state);
				if (wifi_state == WIFI_STATE_GET_IP) {
					goto sleep;
				}
			}
			nrc_usr_print("[%s] Current State : %d...\n",__func__, wifi_state);
			_delay_ms(1000);
		}
	}

	nrc_usr_print("[%s] End of run_sample_wifi_power_save!! \n",__func__);

	while(1) {
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
void user_init(void)
{
	nrc_err_t ret;
	WIFI_CONFIG* param;

	nrc_uart_console_enable(true);

	param = nrc_mem_malloc(WIFI_CONFIG_SIZE);
	memset(param, 0x0, WIFI_CONFIG_SIZE);

	set_wifi_config(param);
	ret = run_sample_wifi_power_save(param);
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
	if(param){
		nrc_mem_free(param);
	}
}
