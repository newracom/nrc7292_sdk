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
#include "nrc_lwip.h"
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"
#include "wifi_config.h"
#include "api_pbc.h"
#include <nvs.h>
#include "nvs_config.h"
#include "nvs_flash.h"

#define MAX_LOOP	10

uint8_t wps_failed  = false;
uint8_t btn_pressed = false;
uint8_t loop_cnt = 0;
TaskHandle_t wps_pbc_sta_task_handle;
static int pre_gpio_value = 0;      /*!< previous gpio value */

nvs_handle_t nvs_handle;
WIFI_CONFIG* param;
uint8_t nvs_pbc_set = 0;

nrc_err_t run_sample_wps_pbc_sta(WIFI_CONFIG *param);

static void wps_pbc_sta_task(void *pvParameters)
{
	while (1) {
		if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10000)) == 0) {
			continue;
		}

		/* Try to connect using wps pushbutton */
		if (nrc_wifi_wps_pbc(0) < 0) {
			nrc_usr_print("[%s] Fail to wps_pbc \n", __func__);
		}
		if (btn_pressed) {
			nvs_err_t err = NVS_OK;
			nrc_usr_print("WPS button Pressed !!\n");
			err = nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle);
			if (err != NVS_OK) {
				nrc_usr_print("[%s] nvs_open failed (0x%x).\n", __func__, err);
			}
			if (nvs_pbc_set == 1)
				nvs_set_u8(nvs_handle, "nvs_pbc_set", 3);
			else
				nvs_set_u8(nvs_handle, "nvs_pbc_set", 0);
			nvs_close(nvs_handle);
			_delay_ms(1000);
			if(nrc_wifi_get_state(0) == WIFI_STATE_CONNECTED) {
				nrc_usr_print("[%s] Trying to DISCONNECT... for exit\n",__func__);
				if (nrc_wifi_disconnect(0, 5000) != WIFI_SUCCESS) {
					nrc_usr_print ("[%s] Fail for Wi-Fi disconnection\n", __func__);
				}
				_delay_ms(2000);
				run_sample_wps_pbc_sta(param);
			}
			else {
				if (nvs_pbc_set == 1){
					nrc_sw_reset();
				}				 	
			}
		}
	}
}

static void wps_pbc_sta_btn_pressed(int vector)
{
	int input_high;

	if (nrc_gpio_inputb(wps_pbc_ops->GPIO_PushButton[0], &input_high))
		return;

    // This condition assume that GPIO input is valid by only edge-trigger method
    if (pre_gpio_value != input_high) {
		if (input_high) {
			nrc_usr_print("[%s] UP \n", __func__);
			BaseType_t xHigherPriorityTaskWoken = pdFALSE;
			vTaskNotifyGiveFromISR(wps_pbc_sta_task_handle, &xHigherPriorityTaskWoken);
			if (xHigherPriorityTaskWoken != pdFALSE) {
				portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
			}
			btn_pressed = true;
			wps_failed = false;
		} else {
			nrc_usr_print("[%s] DOWN \n", __func__);
		}
	}
	pre_gpio_value = input_high;
}

static void wps_pbc_sta_success(void *priv, int net_id, uint8_t *ssid, uint8_t ssid_len, uint8_t security_mode, char *passphrase)
{
	nvs_err_t err = NVS_OK;

	err = nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle);
	if (err != NVS_OK) {
		nrc_usr_print("[%s] nvs_open failed (0x%x).\n", __func__, err);
	}
	memset(param->ssid, 0, sizeof(param->ssid));
	memset(param->password, 0, sizeof(param->password));
	memcpy(param->ssid, ssid, ssid_len);
	param->security_mode = security_mode;
	memcpy(param->password, passphrase, strlen(passphrase));

	nrc_usr_print(" - ssid : %s, ssid_len:%d \n", param->ssid, ssid_len);
	nrc_usr_print(" - security : %s\n", param->security_mode == WIFI_SEC_WPA2 ? "wpa2" : "open");
	nrc_usr_print(" - password : %s, len:%d \n", param->security_mode == WIFI_SEC_WPA2 ? (char *)param->password : "", strlen(passphrase));

	nvs_set_str(nvs_handle, NVS_SSID, (char *) param->ssid);
	nvs_set_u8(nvs_handle, NVS_WIFI_SECURITY, param->security_mode);
	nvs_set_str(nvs_handle, NVS_WIFI_PASSWORD, (char *) param->password);
	nvs_set_u8(nvs_handle, "nvs_pbc_set", 1);
	nvs_close(nvs_handle);
}

static void wps_pbc_sta_failed(void *priv)
{
	wps_failed = true;
}

static struct pbc_ops custom_ops = {
   	.GPIO_PushButton[0]       = GPIO_10,
	.GPIO_PushButton[1]       = -1,
    .nrc_wifi_wps_pbc_fail = wps_pbc_sta_failed,
    .nrc_wifi_wps_pbc_timeout = wps_pbc_sta_failed,
    .nrc_wifi_wps_pbc_success = wps_pbc_sta_success,
    .nrc_wifi_wps_pbc_pressed = wps_pbc_sta_btn_pressed,
};


/******************************************************************************
 * FunctionName : run_sample_wps_pbc_sta
 * Description  : sample test for wps pbc sta
 * Parameters   : WIFI_CONFIG
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_wps_pbc_sta(WIFI_CONFIG *param)
{
	int txpower;
	int network_id = 0;

	nrc_usr_print("[%s] Sample App for WPS_PBC_STA, ipmode : %d \n",__func__, param->ip_mode);

	/* Set IP mode config (Dynamic IP(DHCP client) or STATIC IP) */
	if (nrc_wifi_set_ip_mode(0, param->ip_mode, (char *)param->static_ip) < 0) {
		nrc_usr_print("[%s] Fail to set static IP \n", __func__);
		return WIFI_FAIL_SET_IP;
	}

	nrc_usr_print("[%s] Trying to add network...\n",__func__);
	if (nrc_wifi_add_network(0) < 0) {
		nrc_usr_print("[%s] Fail to init network \n", __func__);
		return WIFI_FAIL_SET_IP;
	}

	if(nrc_wifi_set_country(0, nrc_wifi_country_from_string(COUNTRY_CODE)) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to set Country\n", __func__);
		return WIFI_FAIL;
	}

	/* Set TX Power */
	txpower = param->tx_power;
	if(nrc_wifi_set_tx_power(txpower, 1) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail set TX Power\n", __func__);
		return WIFI_FAIL;
	}

	/* Loop for wps pbc sta test */
	while(loop_cnt++ < MAX_LOOP)
	{
		while(!btn_pressed) {
			nrc_usr_print("Press the WPS Pushbutton...!!!\n");
			_delay_ms(1500);
		}

		/* check the IP is ready */
		while(!wps_failed) {
			if (nrc_wifi_get_state(0) == WIFI_STATE_CONNECTED) {
				nrc_usr_print("[%s] Get IP Address ...\n", __func__);
				if (param->ip_mode == WIFI_DYNAMIC_IP) {
					if (nrc_wifi_set_ip_address(0, param->ip_mode, param->dhcp_timeout, NULL, NULL, NULL) != WIFI_SUCCESS) {
						nrc_usr_print("[%s] Fail to set IP Address(DHCP)\n", __func__);
					}
					else
						break;
				} else {
					if (nrc_wifi_set_ip_address(0, param->ip_mode, 0, param->static_ip, param->netmask, param->gateway) != WIFI_SUCCESS) {
						nrc_usr_print("[%s] Fail to set Static IP Address\n", __func__);
					}
					else
						break;
				}
			} else if (nrc_wifi_get_state(0) == WIFI_STATE_DISCONNECTED) {
				nrc_usr_print("[%s] Disconnected..\n", __func__);
			}
			_delay_ms(1000);
		}

		/* check if IP is ready */
		if (nrc_wait_for_ip(0, param->dhcp_timeout) == NRC_FAIL) {
			return NRC_FAIL;
		}

		btn_pressed  = false;
		if (!wps_failed) {
			nrc_usr_print("[%s] connection success!!\n", __func__);
			break;
		}
		else
			nrc_usr_print("[%s] connection fail!!\n", __func__);
	}

	nrc_usr_print("[%s] End of wps_pbc_sta sample application \n",__func__);

	return NRC_SUCCESS;
}

/******************************************************************************
 * FunctionName : run_sample_wifi_state
 * Description  : sample test for wifi connect & disconnect
 * Parameters   : WIFI_CONFIG
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_wifi_state(WIFI_CONFIG *param)
{
	SCAN_RESULTS results;
	int count = 0;
	int interval = 0;
	int retry_count = 0;
	int ssid_found =false;
	int i = 0;
	AP_INFO *ap_info = NULL;

	nrc_usr_print("[%s] Sample App for Wi-Fi State \n",__func__);
	nrc_usr_print(" - ssid : %s\n", param->ssid);
	nrc_usr_print(" - security : %s\n", param->security_mode == WIFI_SEC_WPA2 ? "wpa2" : "open");
	nrc_usr_print(" - password : %s\n", param->security_mode == WIFI_SEC_WPA2 ? (char *)param->password : "");

	count = 10;
	interval =3000;

	/* set initial wifi configuration */
	while(1) {
		if (wifi_init(param)== WIFI_SUCCESS) {
			nrc_usr_print ("[%s] wifi_init Success !! \n", __func__);
			break;
		} else {
			nrc_usr_print ("[%s] wifi_init Failed !! \n", __func__);
			_delay_ms(1000);
		}
	}

	/* find AP */
	while(1) {
		if (nrc_wifi_scan(0) == WIFI_SUCCESS){
			if (nrc_wifi_scan_results(0, &results)== WIFI_SUCCESS) {
				/* Find the ssid in scan results */
				for(i=0; i<results.n_result ; i++){
					if((strcmp((char*)param->ssid, (char*)results.result[i].ssid)== 0)
					   && (results.result[i].security == param->security_mode)){
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

	/* check if IP is ready */
	while(1) {
		_delay_ms(1000);

		if (nrc_wifi_get_state(0) == WIFI_STATE_DISCONNECTED) {
			break;
		}

		if (nrc_addr_get_state(0) == NET_ADDR_SET) {
			struct netif *interface = nrc_netif_get_by_idx(0);
			nrc_usr_print("[%s] IP ...\n",__func__);
			nrc_usr_print("IP4 : %s\n", ipaddr_ntoa(&interface->ip_addr));
			nrc_usr_print("Netmask : %s\n", ipaddr_ntoa(&interface->netmask));
			nrc_usr_print("Gateway : %s\n", ipaddr_ntoa(&interface->gw));
			break;
		} else {
			nrc_usr_print("[%s] IP Address setting State : %d != NET_ADDR_SET(%d) yet...\n",
						  __func__, nrc_addr_get_state(0), NET_ADDR_SET);
		}
	}

	ap_info = nrc_mem_malloc(sizeof(AP_INFO));
	if (ap_info) {
		if (nrc_wifi_get_ap_info(0, ap_info) == WIFI_SUCCESS) {
			nrc_usr_print("[%s] AP ("MACSTR" %s (len:%d) %c%c bw:%d ch:%d freq:%d security:%d)\n",
				__func__, MAC2STR(ap_info->bssid), ap_info->ssid, ap_info->ssid_len,
				ap_info->cc[0],ap_info->cc[1], ap_info->bw, ap_info->ch, ap_info->freq,
				ap_info->security);
		} else {
			nrc_usr_print("[%s] Fail to get AP INFO\n",__func__);
			nrc_mem_free(ap_info);
			return NRC_FAIL;
		}
	} else {
		nrc_usr_print("[%s] Fail to alloc mem\n", __func__);
		return NRC_FAIL;
	}

	if (ap_info) {
		nrc_mem_free(ap_info);
	}

	_delay_ms(2000);

	nrc_usr_print("[%s] End of run_sample_wifi_state!! \n",__func__);

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
	nvs_err_t err = NVS_OK;

	//Enable Console for Debugging
	nrc_uart_console_enable(true);

	/* Init wps pbc */
	init_wps_pbc(&custom_ops);

	//Wifi config
	param = nrc_mem_malloc(WIFI_CONFIG_SIZE);
	memset(param, 0x0, WIFI_CONFIG_SIZE);
	nrc_wifi_set_config(param);

	err = nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle);
	if (err != NVS_OK) {
		nrc_usr_print("[%s] nvs_open failed (0x%x).\n", __func__, err);
	}
	nvs_get_u8(nvs_handle, "nvs_pbc_set", &nvs_pbc_set);
	nvs_close(nvs_handle);

	xTaskCreate(wps_pbc_sta_task, "wps_pbc_sta_task", 2048, NULL, uxTaskPriorityGet(NULL), &wps_pbc_sta_task_handle);

	if (nvs_pbc_set == 1) {
		nrc_usr_print("[%s] start from nvs \n", __func__);
		ret = run_sample_wifi_state(param);
		nrc_usr_print("[%s] run_sample_wifi_state !! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
	}
	else {
		nrc_usr_print("[%s] start for wps_pbc_sta \n", __func__);
		ret = run_sample_wps_pbc_sta(param);
		nrc_usr_print("[%s] run_sample_wps_pbc_sta!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
		_delay_ms(1000);
		nrc_sw_reset();
	}


	if(param){
		nrc_mem_free(param);
	}
}
