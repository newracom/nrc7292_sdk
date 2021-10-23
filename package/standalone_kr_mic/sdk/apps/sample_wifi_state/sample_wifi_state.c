/*
 * MIT License
 *
 * Copyright (c) 2021 Newracom, Inc.
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
nrc_err_t run_sample_wifi_state(WIFI_CONFIG *param)
{
	SCAN_RESULTS results;
	tWIFI_STATE_ID wifi_state = WIFI_STATE_INIT;
	int count = 0;
	int interval = 0;
	int network_index = 0;
	int retry_count = 0;
	int ssid_found =false;
	int i = 0;

	nrc_usr_print("[%s] Sample App for Wi-Fi State \n",__func__);
	nrc_usr_print(" - count : %d\n", param->count);
	nrc_usr_print(" - interval : %d\n", param->interval);
	nrc_usr_print(" - ssid : %s\n", param->ssid);
	nrc_usr_print(" - security : %s\n", param->security_mode == WIFI_SEC_WPA2 ? "wpa2" : "open");
	nrc_usr_print(" - password : %s\n", param->security_mode == WIFI_SEC_WPA2 ? (char *)param->password : "");

	count = param->count;
	interval = param->interval;

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
		} else{
			nrc_usr_print ("[%s] Fail for connection %s\n", __func__, param->ssid);
			_delay_ms(1000);
		}
	}

	nrc_wifi_get_network_index(&network_index);

	/* check the IP is ready */
	while(1){
		nrc_wifi_get_state(&wifi_state);
		if(wifi_state < 0)
			return -1;

		if (wifi_state == WIFI_STATE_GET_IP) {
			nrc_usr_print("[%s] IP ...\n",__func__);
			break;
		} else{
			nrc_usr_print("[%s] Current State : %d...\n",__func__, wifi_state);
		}
		_delay_ms(1000);
	}

	for(i = 0 ; i < count ; i++){
		/* Repete connection and disconnection every 3 seconds */
		_delay_ms(interval);
		nrc_wifi_get_state(&wifi_state);
		if(wifi_state < 0)
			return -1;

		if (wifi_state == WIFI_STATE_GET_IP) {
			nrc_usr_print("[%s] Trying to DISCONNECT...\n",__func__);
			if (nrc_wifi_disconnect(network_index) != WIFI_SUCCESS) {
				nrc_usr_print ("[%s] Fail for Wi-Fi disconnection\n", __func__);
				return -1;
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
				return -1;
		}
	}

	nrc_wifi_get_state(&wifi_state);
	if(wifi_state < 0)
		return -1;

	if (wifi_state == WIFI_STATE_GET_IP || wifi_state == WIFI_STATE_CONNECTED) {
		nrc_usr_print("[%s] Trying to DISCONNECT... for exit\n",__func__);
		if (nrc_wifi_disconnect(network_index) != WIFI_SUCCESS) {
			nrc_usr_print ("[%s] Fail for Wi-Fi disconnection\n", __func__);
			return -1;
		}
	}

	nrc_usr_print("[%s] End of run_sample_wifi_state!! \n",__func__);

	return 0;
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

	param = malloc(WIFI_CONFIG_SIZE);
	memset(param, 0x0, WIFI_CONFIG_SIZE);
	nrc_usr_print("[%s] \n", __func__);

	set_wifi_config(param);
	ret = run_sample_wifi_state(param);
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
	if(param){
		free(param);
	}
}
