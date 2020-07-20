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

#define SOFTAP_COUNTRY_NUM 5
s1g_operation_channel_mapping channel_table[SOFTAP_COUNTRY_NUM]=
{
	{"KR", 2},	/* 2  : 2MBW_918.5MHz(KR) */
	{"JP", 3},	/* 3  : 1MBW_918MHz(JP) */
	{"US", 46},	/* 46 : 2MBW_925MHz(US) */
	{"TW", 2},	/* 2  : 2MBW_839.5MHz(TW) */
	{"EU", 2},	/* 2  : 2MBW_864MHz(EU) */
};


void wifi_event_handler(WLAN_EVENT_ID event)
{
	switch(event) {
		case WLAN_EVT_CONNECT_SUCCESS:
			nrc_usr_print("[%s] Receive Connection Success Event\n", __func__);
			break;;
		case WLAN_EVT_CONNECT_FAIL:
			nrc_usr_print("[%s] Receive Connection Fail Event\n", __func__);
			break;
		case WLAN_EVT_GET_IP:
			nrc_usr_print("[%s] Receive IP_GET Success Event\n", __func__);
			nrc_usr_print("[%s] IP Address : %s\n", __func__, nrc_wifi_get_ip_address());
			break;
		case WLAN_EVT_GET_IP_FAIL:
			nrc_usr_print("[%s] Receive IP_GET Fail Event\n", __func__);
			break;
		case WLAN_EVT_DISCONNECT:
			nrc_usr_print("[%s] Receive Disconnection Event\n", __func__);
			break;
		case WLAN_EVT_SCAN_DONE:
			nrc_usr_print("[%s] Receive Scan Done Event\n", __func__);
			break;
		case WLAN_EVT_START_SOFT_AP: //SoftAP
			nrc_usr_print("[%s] Receive Start Soft AP Event\n", __func__);
			break;
		case WLAN_EVT_SET_SOFT_AP_IP: //SoftAP
			nrc_usr_print("[%s] Receive SET IP Event\n", __func__);
			nrc_usr_print("[%s] IP Address : %s\n", __func__, nrc_wifi_get_ip_address());
			break;
		case WLAN_EVT_START_DHCP_SERVER: //SoftAP
			nrc_usr_print("[%s] Receive Start DHCPS Event\n", __func__);
			break;

		default:
			nrc_usr_print("[%s] Receive Unknown Event %d\n", __func__, event);
			break;
	}
}

int wifi_init(WIFI_CONFIG *param)
{
	/* Optional: Register Wi-Fi Event Handler */
	nrc_wifi_register_event_handler(wifi_event_handler);

	/* Set DHCP config (DHCP or STATIC IP) */
	if (!nrc_wifi_set_dhcp((bool)param->dhcp_enable, (char *)param->static_ip)) {
		nrc_usr_print("[%s] Fail to set static IP \n", __func__);
		return WIFI_SET_IP_FAIL;
	}

	/* Set TX Power */
	if(nrc_wifi_set_tx_power((int)param->tx_power) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail set TX Power\n", __func__);
		return WIFI_SET_FAIL;
	}
	nrc_usr_print("[%s] TX Power (%d dBm)\n", __func__, nrc_wifi_get_tx_power());

	/* Set Country Code */
	if(nrc_wifi_set_country((char *)param->country) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to set Country\n", __func__);
		return WIFI_SET_FAIL;
	}

	return WIFI_SUCCESS;
}

int wifi_connect(WIFI_CONFIG *param)
{
	int index = -1;

	/* Try to connect with ssid and security */
	nrc_usr_print("[%s] Trying to Wi-Fi Connection...\n",__func__);
	if ((index = nrc_wifi_get_nd()) < 0) {
		nrc_usr_print("[%s] Fail to init \n", __func__);
		return WIFI_INIT_FAIL;
	}

	if (nrc_wifi_set_ssid(index, (char *)param->ssid) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to set SSID\n", __func__);
		return WIFI_SET_FAIL;
	}

	if (nrc_wifi_set_security(index, (int)param->security_mode, (char *)param->password) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to set Security\n", __func__);
		return WIFI_SET_FAIL;
	}

	if (nrc_wifi_connect(index) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to Connect\n", __func__);
		return WIFI_CONNECTION_FAIL;
	}

	if (nrc_wifi_get_ip() != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to Get IP Address\n", __func__);
		return WIFI_SET_IP_FAIL;
	}

	return WIFI_SUCCESS;
}

int wifi_start_softap(WIFI_CONFIG *param)
{
	int index = -1;
	uint8_t s1g_channel = wifi_get_s1g_channel_number(channel_table,\
					SOFTAP_COUNTRY_NUM, (uint8_t *)param->country);

	nrc_usr_print("[%s] Trying to start Soft AP (SSID:%s, S1G_CH:%d)\n",\
			 __func__, (char *)param->ssid,  s1g_channel);

	if ((index = nrc_wifi_get_nd()) < 0) {
		nrc_usr_print("[%s] Fail to init \n", __func__);
		return WIFI_INIT_FAIL;
	}

	if(nrc_wifi_softap_set_conf(index, (char *)param->ssid, (int)s1g_channel,\
			(int)param->security_mode, (char *)param->password) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to set sotftap config\n", __func__);
		return WIFI_SET_FAIL;
	}

	if(nrc_wifi_softap_start(index) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to start sotftap\n", __func__);
		return WIFI_SOFTAP_FAIL;
	}

	return WIFI_SUCCESS;
}

/******************************************************************************
 * FunctionName : wifi_get_s1g_channel_number
 * Description  : get s1g channel number based on country
 * Parameters   : s1g_operation_channel_mapping *table
                  uint8_t *country
 * Returns	  : s1g channel number
 *******************************************************************************/
uint16_t wifi_get_s1g_channel_number(s1g_operation_channel_mapping *table,
				int table_len, uint8_t *country){
	int i = 0;
	int s1g_freq = 0;

	for(i=0; i<table_len; i++){
		if(strncmp((const char*)country, (const char*)&table[i].country, 2) == 0){
			s1g_freq =table[i].s1g_freq;
			break;
		}
	}
	return s1g_freq;
}
