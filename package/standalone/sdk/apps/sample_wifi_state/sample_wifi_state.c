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

/******************************************************************************
 * FunctionName : run_sample_wifi_state
 * Description  : sample test for wifi connect & disconnect
 * Parameters   : WIFI_CONFIG
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
int  run_sample_wifi_state(WIFI_CONFIG *param)
{
	int i = 0;
	int count = 0;
	int interval = 0;
	int network_index = 0;
	int wifi_state = WLAN_STATE_INIT;
	int retry_count = 0;

	nrc_usr_print("[%s] Sample App for Wi-Fi State \n",__func__);

	count = param->count;
	interval = param->interval;

	if (wifi_init(param)!= WIFI_SUCCESS) {
		nrc_usr_print ("[%s] ASSERT! Fail for init\n", __func__);
		return RUN_FAIL;
	}

	/* 1st trial to connect */
	if (wifi_connect(param)!= WIFI_SUCCESS) {
		nrc_usr_print ("[%s] Fail for Wi-Fi connection (results:%d)\n", __func__);
		return RUN_FAIL;
	}

	network_index = nrc_wifi_get_network_index();

	for(i = 0 ; i < count ; i++){
		/* Repete connection and disconnection every 3 seconds */
		_delay_ms(interval);
		if (nrc_wifi_get_state() == WLAN_STATE_GET_IP) {
			nrc_usr_print("[%s] Trying to DISCONNECT...\n",__func__);
			if (nrc_wifi_disconnect(network_index) != WIFI_SUCCESS) {
				nrc_usr_print ("[%s] Fail for Wi-Fi disconnection (results:%d)\n", __func__);
				return RUN_FAIL;
			}
			_delay_ms(2000);
		} else {
			nrc_usr_print("[%s] Trying to CONNECT...\n",__func__);
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
			if(retry_count == MAX_RETRY)
				return RUN_FAIL;
		}
	}

	wifi_state = nrc_wifi_get_state();
	if (wifi_state == WLAN_STATE_GET_IP || wifi_state == WLAN_STATE_CONNECTED) {
		nrc_usr_print("[%s] Trying to DISCONNECT... for exit\n",__func__);
		if (nrc_wifi_disconnect(network_index) != WIFI_SUCCESS) {
			nrc_usr_print ("[%s] Fail for Wi-Fi disconnection (results:%d)\n", __func__);
			return RUN_FAIL;
		}
	}

	nrc_usr_print("[%s] End of run_sample_wifi_state!! \n",__func__);

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
	ret = run_sample_wifi_state(param);
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
	if(param){
		nrc_mem_free(param);
	}
}
