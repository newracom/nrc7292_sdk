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
#include "wifi_config.h"
#include "api_pbc.h"

#define MAX_LOOP	10

uint8_t wps_failed  = false;
uint8_t btn_pressed = false;
uint8_t disconnected = false;
uint8_t loop_cnt = 0;

static void wps_pbc_btn_pressed(int vector)
{
	int input_high;

	if (nrc_gpio_inputb(wps_pbc_ops->GPIO_PushButton, &input_high))
		return;

    //This condition assume that GPIO is configured as pull-up
   	if (input_high) {
		nrc_usr_print("[%s] DOWN \n", __func__);
	} else {
		nrc_usr_print("[%s] UP \n", __func__);
		/* Try to connect using wps pushbutton */
		if(nrc_wifi_wps_pbc() < 0) {
			nrc_usr_print("[%s] Fail to wps_pbc \n", __func__);
			return;
		}
		btn_pressed = true;
		wps_failed = false;
	}
}

static void wps_pbc_failed()
{
	wps_failed = true;
}

static struct pbc_ops custom_ops = {
    .GPIO_PushButton       = GPIO_10,
    .nrc_wifi_wps_pbc_fail = wps_pbc_failed,
    .nrc_wifi_wps_pbc_timeout = wps_pbc_failed,
    .nrc_wifi_wps_pbc_success = NULL,
    .nrc_wifi_wps_pbc_pressed = wps_pbc_btn_pressed,
};


/******************************************************************************
 * FunctionName : run_sample_wps_pbc
 * Description  : sample test for wps pbc
 * Parameters   : WIFI_CONFIG
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_wps_pbc(WIFI_CONFIG *param)
{
	tWIFI_STATE_ID wifi_state = WIFI_STATE_INIT;
	int count, index = -1;
	int txpower;

	nrc_usr_print("[%s] Sample App for WPS_PBC, ipmode : %d \n",__func__, param->ip_mode);

	/* Set IP mode config (Dynamic IP(DHCP client) or STATIC IP) */
	if (nrc_wifi_set_ip_mode(param->ip_mode, (char *)param->static_ip) < 0) {
		nrc_usr_print("[%s] Fail to set static IP \n", __func__);
		return WIFI_SET_IP_FAIL;
	}

	/* Set TX Power */
	txpower = param->tx_power;

	if(nrc_wifi_set_tx_power(txpower) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail set TX Power\n", __func__);
		return WIFI_FAIL;
	}
	txpower = 0;
	nrc_wifi_get_tx_power(&txpower);
	nrc_usr_print("[%s] TX Power (%d dBm)\n", __func__, txpower);

	nrc_usr_print("[%s] Trying to add network...\n",__func__);
	if (nrc_wifi_add_network(&index) < 0) {
		nrc_usr_print("[%s] Fail to init network \n", __func__);
		return WIFI_SET_IP_FAIL;
	}

	/* Init wps pbc */
	init_wps_pbc(&custom_ops);

	/* Loop for wps pbc test */
	while(loop_cnt++ < MAX_LOOP)
	{
		while(!btn_pressed) {
			nrc_usr_print("Press the WPS Pushbutton...!!!\n");
			_delay_ms(1500);
		}

		/* check the IP is ready */
		while(!wps_failed) {
			if(nrc_wifi_get_state(&wifi_state) != WIFI_SUCCESS) {
				nrc_usr_print("[%s] Fail to get state\n", __func__);
				return -1;
			}

			if (disconnected && (wifi_state == WIFI_STATE_CONNECTED || wifi_state == WIFI_STATE_SCAN_DONE)) {
				nrc_usr_print("[%s] Get IP Address ...\n", __func__);

				if (nrc_wifi_set_ip_address() != WIFI_SUCCESS)
					nrc_usr_print("[%s] Fail to set IP Address\n", __func__);
			}
			else if (wifi_state == WIFI_STATE_GET_IP){
				char* ip_addr = NULL;

				if (nrc_wifi_get_ip_address(&ip_addr) != WIFI_SUCCESS){
					nrc_usr_print("[%s] Fail to get IP address\n", __func__);
					return -1;
				}

				nrc_usr_print("[%s] IP Address : %s\n", __func__, ip_addr);
				break;
			} else if (wifi_state == WIFI_STATE_SCAN) {
				nrc_usr_print("[%s] Scanning..\n", __func__);
			} else if (wifi_state == WIFI_STATE_DISCONNECTED) {
				nrc_usr_print("[%s] Disconnected..\n", __func__);
				disconnected = true;
			}

			_delay_ms(1000);
		}

		disconnected = false;
		btn_pressed  = false;
		if(!wps_failed)
			nrc_usr_print("[%s] connection success!!\n", __func__);
		else
			nrc_usr_print("[%s] connection fail!!\n", __func__);
	}

	/* EXIT */
	if(nrc_wifi_get_state(&wifi_state) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] End of wps_pbc sample application \n",__func__);
		return -1;
	}

	if (wifi_state == WIFI_STATE_GET_IP || wifi_state == WIFI_STATE_CONNECTED) {
		int network_index;
		nrc_usr_print("[%s] Trying to DISCONNECT... for exit\n",__func__);
		if (nrc_wifi_get_network_index(&network_index) != WIFI_SUCCESS){
			nrc_usr_print("[%s] Fail to get network index\n", __func__);
			return -1;
		}
		if (nrc_wifi_disconnect(network_index) != WIFI_SUCCESS) {
			nrc_usr_print ("[%s] Fail for Wi-Fi disconnection\n", __func__);
			return -1;
		}
	}

	nrc_usr_print("[%s] End of wps_pbc sample application \n",__func__);

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
	WIFI_CONFIG* param;
	nrc_err_t ret;

	//Enable Console for Debugging
	nrc_uart_console_enable(true);

	//Wifi config
	param = nrc_mem_malloc(WIFI_CONFIG_SIZE);
	memset(param, 0x0, WIFI_CONFIG_SIZE);

	param->tx_power = TX_POWER;
	param->ip_mode = NRC_WIFI_IP_MODE;

#if (NRC_WIFI_IP_MODE == WIFI_STATIC_IP)
	memcpy(param->static_ip, NRC_STATIC_IP, sizeof(NRC_STATIC_IP));
#endif

	run_sample_wps_pbc(param);
}
