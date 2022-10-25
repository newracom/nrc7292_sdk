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
#define ROAMING_SCAN_THRESHOLD (-60)
#define ROAMING_RSSI_THRESHOLD (1)

typedef struct ap_info {
	uint8_t ssid[MAX_SSID_LENGTH];
	uint8_t bssid[MAX_BSSID_LENGTH];
	int freq;
	int rssi;
	struct ap_info* next;
}ap_info_t;

typedef struct roaming_info {
	int roaming_rssi_level;
	int roaming_scan_threshold;
	ap_info_t* current_ap;
	ap_info_t* ap_list;
}roaming_info_t;

ap_info_t * good_ap;
roaming_info_t roaming_info;
SCAN_RESULTS results;

static int network_index = 0;
static int error_val = 0;

int init_roaming_info(roaming_info_t *info)
{
	if(info == NULL){
		nrc_usr_print("[%s] Roaming info is NULL  \n",__func__);
		return false;
	}

	info->roaming_rssi_level = ROAMING_RSSI_THRESHOLD;
	info->roaming_scan_threshold = ROAMING_SCAN_THRESHOLD;
	info->current_ap = NULL;
	info->ap_list = NULL;
	return true;
}

int display_scan_results(SCAN_RESULTS *results){
	int i = 0;

	if(results == NULL){
		nrc_usr_print("[%s] scan results is NULL  \n",__func__);
		return false;
	}

	nrc_usr_print("\n[%s] scan results(num_ap:%d)\n", __func__, results->n_result);

	for (i=0; i <results->n_result ;i++) {
		nrc_usr_print("[%s] bssid:%s, freq:%s, sig_level:%s, flags:%s, ssid:%s\n", __func__,
				results->result[i].bssid,
				results->result[i].freq,
				results->result[i].sig_level,
				results->result[i].flags,
				results->result[i].ssid);
	}
	return true;
}

int ap_list_display(roaming_info_t *info)
{
	ap_info_t* ap_info;
	int i;

	if(info == NULL){
		nrc_usr_print("[%s] Roaming info is NULL  \n",__func__);
		return false;
	}

	ap_info = info->ap_list;

	while(ap_info != NULL){
		nrc_usr_print("ap_list_display-> ssid:%s, bssid:%s, freq:%d, rssi:%d\n",\
					ap_info->ssid, ap_info->bssid, ap_info->freq, ap_info->rssi);
		ap_info = ap_info->next;
	}
	return true;
}

int add_current_ap(roaming_info_t *info, uint8_t *ssid, uint8_t *bssid, int rssi, int freq)
{
	ap_info_t* ap_info;

	if(info == NULL){
		nrc_usr_print("[%s] Roaming info is NULL  \n",__func__);
		return false;
	}

	if(info->current_ap == NULL){
		ap_info =  nrc_mem_malloc(sizeof(ap_info_t));
		if(ap_info == NULL){
			nrc_usr_print("[%s] %d Bytes memory allocation fail  \n",__func__, sizeof(ap_info_t));
			return false;
		}
	} else {
		ap_info = info->current_ap;
	}
	memcpy(ap_info->ssid, ssid, MAX_SSID_LENGTH);
	memcpy(ap_info->bssid, bssid, MAX_BSSID_LENGTH+1);
	ap_info->freq = freq;
	ap_info->rssi = rssi;
	ap_info->next = NULL;
	info->current_ap = ap_info;

	return true;
}

int ap_add(roaming_info_t *info, uint8_t *ssid, uint8_t *bssid, int rssi, int freq)
{
	ap_info_t* ap_info;
	ap_info_t* ap_info_cur;

	if(info == NULL){
		nrc_usr_print("[%s] Roaming info is NULL  \n",__func__);
		return false;
	}

	ap_info =  nrc_mem_malloc(sizeof(ap_info_t));
	if(ap_info == NULL){
		nrc_usr_print("[%s] %d Bytes memory allocation fail  \n",__func__, sizeof(ap_info_t));
		return false;
	}
	memset(ap_info, 0x0, sizeof(ap_info_t));

	memcpy(ap_info->ssid, ssid, MAX_SSID_LENGTH);
	memcpy(ap_info->bssid, bssid, MAX_BSSID_LENGTH);
	ap_info->freq = freq;
	ap_info->rssi = rssi;
	ap_info->next = NULL;

	if(info->ap_list == NULL){
		info->ap_list = ap_info;
		return true;
	}

	ap_info_cur = info->ap_list;
	while(1){
		if (strcmp((char *)ap_info_cur->bssid, (char *)bssid) == 0){
			break;
		}
		if(ap_info_cur->next == NULL){
			ap_info_cur->next = ap_info;
			break;
		}
		ap_info_cur = ap_info_cur->next;
	}
	return true;
}

int ap_del(roaming_info_t *info, uint8_t *bssid)
{
	ap_info_t* ap_info;
	ap_info_t* ap_info_prev;

	if(info == NULL){
		nrc_usr_print("[%s] Roaming info is NULL  \n",__func__);
		return false;
	}

	if(info->ap_list == NULL){
		nrc_usr_print("[%s] AP is not in the list!\n", __func__);
		return false;
	}

	ap_info_prev =NULL;
	ap_info = info->ap_list;

	while(ap_info != NULL){
		if (strcmp((char *)ap_info->bssid, (char *)bssid) == 0){
			if (ap_info_prev == NULL) {
				info->ap_list = ap_info->next;
			} else {
				ap_info_prev->next = ap_info->next;
			}
			vPortFree(ap_info);
			return true;
		}
		ap_info_prev = ap_info;
		ap_info = ap_info->next;
	}

	return false;
}

int add_scan_ap_list(roaming_info_t *info, SCAN_RESULTS *results, uint8_t *ssid)
{
	int i = 0;

	if(info == NULL){
		nrc_usr_print("[%s] Roaming info is NULL  \n",__func__);
		return false;
	}

	if(results == NULL){
		nrc_usr_print("[%s] scan results is NULL  \n",__func__);
		return false;
	}

	if(ssid == NULL){
		nrc_usr_print("[%s] ssid is NULL  \n",__func__);
		return false;
	}

	for (i = 0; i<results->n_result; i++) {
		if (strcmp((char *)ssid, (char *)results->result[i].ssid) == 0) {
			ap_add(info, (uint8_t *)results->result[i].ssid, (uint8_t *)results->result[i].bssid,\
					 atoi(results->result[i].sig_level),  atoi(results->result[i].freq));

			if(info->current_ap ==  NULL || strcmp((char *)info->current_ap->bssid , (char *)results->result[i].bssid) == 0){
				add_current_ap(info, (uint8_t *)results->result[i].ssid, (uint8_t *)results->result[i].bssid,\
					 atoi(results->result[i].sig_level),  atoi(results->result[i].freq));
			}
		}
	}
	return true;
}

int remove_scan_ap_list(roaming_info_t *info)
{
	ap_info_t* ap_info;
	ap_info_t* ap_info_temp;

	if(info == NULL){
		nrc_usr_print("[%s] Roaming info is NULL  \n",__func__);
		return false;
	}

	ap_info = info->ap_list;

	while(ap_info != NULL){
		ap_info_temp = ap_info->next;
		vPortFree(ap_info);
		ap_info = ap_info_temp;
	}
	info->ap_list =  NULL;

	return true;
}

ap_info_t* get_max_rssi_ap(roaming_info_t *info)
{
	ap_info_t* ap_info;
	ap_info_t* ap_info_max;

	int max_rssi = -100;

	if(info == NULL){
		nrc_usr_print("[%s] Roaming info is NULL  \n",__func__);
		return NULL;
	}

	ap_info = info->ap_list;
	ap_info_max = info->ap_list;

	if(info->ap_list == NULL){
		nrc_usr_print("[%s] AP is not in the list!\n", __func__);
		return NULL;
	}

	while (ap_info != NULL){
		if (ap_info->rssi > max_rssi){
		       max_rssi = ap_info->rssi;
		       ap_info_max = ap_info;
		}
		ap_info = ap_info->next;
	}

	return ap_info_max;
}

int check_roaming_rssi_condition(roaming_info_t * info, ap_info_t* rssi_max_ap)
{
	if(info == NULL){
		nrc_usr_print("[%s] Roaming info is NULL  \n",__func__);
		return false;
	}

	if(info->current_ap == NULL){
		nrc_usr_print("[%s] Current AP is empty!\n", __func__);
		return true;
	}

	if(rssi_max_ap == NULL){
		nrc_usr_print("[%s] There is no ap [ssid:%s]!\n", __func__, rssi_max_ap);
		return false;
	}

	nrc_usr_print("[%s] RSSI Compare -> cur: %s %d | max: %s %d\n",\
		 __func__, info->current_ap->bssid, info->current_ap->rssi,rssi_max_ap->bssid,rssi_max_ap->rssi);

	if (strcmp((char *)info->current_ap->bssid , (char *)rssi_max_ap->bssid) == 0) {
		return false;
	}

	if(info->current_ap->rssi < rssi_max_ap->rssi){
		if(rssi_max_ap->rssi - info->current_ap->rssi >  ROAMING_RSSI_THRESHOLD) {
			nrc_usr_print("[%s] Roaming condition to [ssid:%s, bssid:%s, freq:%d, rssi:%d]\n",\
					__func__,(info->current_ap)->ssid, (info->current_ap)->bssid,\
					(info->current_ap)->freq, (info->current_ap)->rssi);
			return true;
		}
	}
	return false;
}

int roaming_condition(roaming_info_t *info, SCAN_RESULTS *results, uint8_t *ssid)
{
	int ret = false;

	ap_info_t* ap_info_max = NULL;

	if(info == NULL){
		nrc_usr_print("[%s] Roaming info is NULL  \n",__func__);
		return false;
	}

	if(!add_scan_ap_list(info, results, (uint8_t *)ssid)){
		nrc_usr_print ("[%s] add_scan_ap_list failed\n", __func__);
		return false;
	}
	ap_list_display(info);

	ap_info_max = get_max_rssi_ap(info);

	if(check_roaming_rssi_condition(info, ap_info_max)){
	        add_current_ap(info, ap_info_max->ssid, ap_info_max->bssid, ap_info_max->rssi, ap_info_max->freq);
		nrc_usr_print("-------------------------------------------------------------------------------\n");
		nrc_usr_print("[%s] Update Current AP!(bssid: %s, rssi: %d)\n",\
					 __func__, (info->current_ap)->bssid, (info->current_ap)->rssi);
		nrc_usr_print("-------------------------------------------------------------------------------\n");
		ret = true;
	}

	if(!remove_scan_ap_list(info)){
		nrc_usr_print ("[%s] remove_scan_ap_list failed\n", __func__);
	}

	return ret;
}


static void roaming_task(void *pvParameters)
{
	WIFI_CONFIG *param =pvParameters;
	param->test_running = 1;

	while(1)
	{
		if (nrc_wifi_scan() != WIFI_SUCCESS){
			nrc_usr_print( "%s nrc_wifi_scan fail\n", __func__);
			goto exit;
		}

		if (nrc_wifi_scan_results(&results)!= WIFI_SUCCESS) {
			nrc_usr_print ("[%s] Fail to scan\n", __func__);
			goto exit;
		}

		display_scan_results(&results);
		if(roaming_condition(&roaming_info, &results, (uint8_t *)param->ssid)){
			if (nrc_wifi_disconnect(network_index) != WIFI_SUCCESS) {
				nrc_usr_print ("[%s] Fail for Wi-Fi disconnection (results:%d)\n", __func__);
				error_val = -1;
				goto exit;
			}
			strcpy((char *)param->bssid, (char *) roaming_info.current_ap->bssid);
			wifi_connect(param);
		}
		_delay_ms(10000); //10 seconds Scan cycle
	}

exit:
	param->test_running = 0;
	vTaskDelete(NULL);
}

/******************************************************************************
 * FunctionName : run_sample_wifi_roaming
 * Description  : sample test for wifi roaming
 * Parameters   : WIFI_CONFIG
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_wifi_roaming(WIFI_CONFIG *param)
{
	init_roaming_info(&roaming_info);

	nrc_usr_print("[%s] Sample App for Wifi Roaming  \n",__func__);
	tWIFI_STATE_ID wifi_state = WIFI_STATE_INIT;

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
			if (nrc_wifi_scan_results(&results)== WIFI_SUCCESS)
				break;
		} else {
			nrc_usr_print ("[%s] Scan fail !! \n", __func__);
		}
		_delay_ms(1000);
	}
	display_scan_results(&results);

	if(!add_scan_ap_list(&roaming_info, &results, (uint8_t *)(char *)param->ssid)){
		nrc_usr_print ("[%s] add_scan_ap_list failed\n", __func__);
		return false;
	}
	ap_list_display(&roaming_info);

	strcpy((char *)param->bssid, (char *) roaming_info.current_ap->bssid);
	wifi_connect(param);

	if(!remove_scan_ap_list(&roaming_info)){
		nrc_usr_print ("[%s] remove_scan_ap_list failed\n", __func__);
	}
	_delay_ms(1000);

	//Create Roaming Task
	xTaskCreate(roaming_task, "roaming_task", 4096, (void*)param, 5, NULL);

	while(param->test_running) {
		_delay_ms(10);
	}

	if (error_val < 0)
		return NRC_FAIL;

	nrc_usr_print("[%s] End of user_init!! \n",__func__);

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
	ret = run_sample_wifi_roaming(param);
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
	if(param){
		nrc_mem_free(param);
	}
}
