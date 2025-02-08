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

uint8_t btn_pressed = false;
TaskHandle_t wps_pbc_softap_task_handle;
static int pre_gpio_value = 0;      /*!< previous gpio value */

nvs_handle_t nvs_handle;
WIFI_CONFIG* param;
uint8_t nvs_pbc_set = 0;

nrc_err_t run_sample_wps_pbc_softap(WIFI_CONFIG *param);

static int str_to_argc_argv(const char* str, int* argc, char*** argv) {
	int i, j, len, n;
	char** res, * p;

	/* count the number of arguments */
	len = strlen(str);
	for (i = n = 0; i < len; ) {
		while (i < len && str[i] == ' ')
			i++;
		if (i == len)
			break;
		n++;
		while (i < len && str[i] != ' ')
			i++;
	}

	/* allocate space for argv */
	res = (char**)nrc_mem_malloc((n + 1) * sizeof(char*));
	if (!res)
		return -1;

	/* fill argv with pointers to arguments */
	p = (char*)nrc_mem_malloc(len + 1);
	if (!p) {
		nrc_mem_free(res);
		return -1;
	}
	strcpy(p, str);
	for (i = j = 0; i < len; ) {
		while (i < len && p[i] == ' ')
			p[i++] = '\0';
		if (i == len)
			break;
		res[j++] = p + i;
		while (i < len && p[i] != ' ')
			i++;
	}
	res[j] = NULL;

	/* set argc and argv */
	*argc = n;
	*argv = res;

	return 0;
}

static void wps_pbc_softap_task(void *pvParameters)
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
			nrc_usr_print("WPS button Pressed !!\n");
		}
		_delay_ms(1000);
	}
}

static void wps_pbc_softap_btn_pressed(int vector)
{
	int input_high;

	if (nrc_gpio_inputb(wps_pbc_ops->GPIO_PushButton[0], &input_high))
		return;

    // This condition assume that GPIO input is valid by only edge-trigger method
    if (pre_gpio_value != input_high) {
		if (input_high) {
			nrc_usr_print("[%s] UP \n", __func__);
			BaseType_t xHigherPriorityTaskWoken = pdFALSE;
			vTaskNotifyGiveFromISR(wps_pbc_softap_task_handle, &xHigherPriorityTaskWoken);
			if (xHigherPriorityTaskWoken != pdFALSE) {
				portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
			}
			btn_pressed = true;
		} else {
			nrc_usr_print("[%s] DOWN \n", __func__);
		}
	}
	pre_gpio_value = input_high;
}

static void wps_pbc_softap_success(void *priv, int net_id, uint8_t *ssid, uint8_t ssid_len, uint8_t security_mode, char *passphrase)
{
    nrc_usr_print("[%s] WPS SUCCESS\n", __func__);
	btn_pressed = false;
}

static void wps_pbc_softap_failed(void *priv)
{
    nrc_usr_print("[%s] WPS FAIL\n", __func__);
	btn_pressed = false;
}

static struct pbc_ops custom_ops = {
    .GPIO_PushButton[0]       = GPIO_10,
	.GPIO_PushButton[1]       = -1,
    .nrc_wifi_wps_pbc_fail = wps_pbc_softap_failed,
    .nrc_wifi_wps_pbc_timeout = wps_pbc_softap_failed,
    .nrc_wifi_wps_pbc_success = wps_pbc_softap_success,
    .nrc_wifi_wps_pbc_pressed = wps_pbc_softap_btn_pressed,
};


/******************************************************************************
 * FunctionName : run_sample_wps_pbc_softap
 * Description  : sample test for wps pbc softap
 * Parameters   : WIFI_CONFIG
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_wps_pbc_softap(WIFI_CONFIG *param)
{
	char str[200];
	int argc;
	char** argv;

	nrc_usr_print("[%s] Sample App for wps_pbc_softap \n",__func__);
	nrc_usr_print(" - ssid : %s\n", param->ssid);
	nrc_usr_print(" - security : %s\n", param->security_mode == WIFI_SEC_WPA2 ? "wpa2" : "open");
	nrc_usr_print(" - password : %s\n", param->security_mode == WIFI_SEC_WPA2 ? (char *)param->password : "");

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

	/* run softap */
	nrc_usr_print("[%s] Starting AP with ssid \"%s\"...\n", __func__, param->ssid);
	if (wifi_start_softap(param) == WIFI_SUCCESS) {
		nrc_usr_print("Soft AP Started...\n");
		nrc_usr_print("[%s] ssid %s security %d channel %d\n", __func__, param->ssid, param->security_mode, param->channel);
	} else {
		nrc_usr_print("Error Starting Soft AP...\n");
	}

	/* set 4 address */
	nrc_wifi_set_use_4address(true);

	/* set DHCP server */
	if (param->dhcp_server == 1 && param->device_mode == WIFI_MODE_AP) {
		nrc_usr_print("The DHCP server on wlan0 interface\n");
		memset(str, 0x0, sizeof(str));
		sprintf(str, "-i wlan0");
		nrc_usr_print("dhcps : %s\n", str);
		if (str_to_argc_argv(str, &argc, &argv) == -1) {
			nrc_usr_print("Failed to convert string to argc and argv\n");
			return NRC_FAIL;
		}
		wifi_dhcps(argc, argv);
		nrc_mem_free(argv[0]);
		nrc_mem_free(argv);
	}

	/* Loop for wps pbc softap test */
	while(1)
	{
		while(!btn_pressed) {
			nrc_usr_print("Press the WPS Pushbutton...!!!\n");
			_delay_ms(1500);
		}
		while(btn_pressed){
			nrc_usr_print("softAP WPS PBC in progress...\n");
			_delay_ms(10000);
		}
	}

	nrc_usr_print("[%s] End of wps_pbc_softap sample application \n",__func__);

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
	// nvs_get_u8(nvs_handle, "nvs_pbc_set", &nvs_pbc_set);
	nvs_close(nvs_handle);

	xTaskCreate(wps_pbc_softap_task, "wps_pbc_softap_task", 2048, NULL, uxTaskPriorityGet(NULL), &wps_pbc_softap_task_handle);

	run_sample_wps_pbc_softap(param);

	if(param){
		nrc_mem_free(param);
	}
}
