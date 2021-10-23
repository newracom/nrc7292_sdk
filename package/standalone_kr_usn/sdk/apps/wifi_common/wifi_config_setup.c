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
#include "wifi_config.h"
#include "wifi_config_setup.h"
#include "wlan_manager.h"

/******************************************************************************
 * FunctionName : set_wifi_config
 * Description  : set configuration for sample application
 * Parameters   : WIFI_CONFIG
 * Returns      : void
 *******************************************************************************/
void set_wifi_config(WIFI_CONFIG *param)
{
	memset(param, 0x0, WIFI_CONFIG_SIZE);
	memcpy(param->ssid, STR_SSID, sizeof(STR_SSID));
	memcpy(param->country, COUNTRY_CODE, sizeof(COUNTRY_CODE));
	memcpy(param->remote_addr, NRC_REMOTE_ADDRESS, sizeof(NRC_REMOTE_ADDRESS));
	param->remote_port = NRC_REMOTE_PORT;
	param->security_mode = NRC_WIFI_SECURE;
#if defined(NRC_WIFI_SECURE) && (NRC_WIFI_SECURE == WIFI_SEC_WPA2)
	memcpy(param->password, NRC_WIFI_PASSWORD, sizeof(NRC_WIFI_PASSWORD));
#endif
	param->tx_power = TX_POWER;
	param->count = NRC_WIFI_TEST_COUNT;
	param->interval = NRC_WIFI_TEST_INTERVAL;
	param->duration = NRC_WIFI_TEST_DURATION;
	param->ip_mode = NRC_WIFI_IP_MODE;
#if NRC_WIFI_SCAN_LIST
	param->scan_freq_num = NRC_WIFI_SCAN_FREQ_NUM;
	for (int i=0; i < param->scan_freq_num; i++)
		param->scan_freq_list[i] = nrc_scan_freq_list[i];
#endif

#if (NRC_WIFI_IP_MODE == WIFI_STATIC_IP)
	memcpy(param->static_ip, NRC_STATIC_IP, sizeof(NRC_STATIC_IP));
#endif
}

/******************************************************************************
 * FunctionName : set_wifi_softap_config
 * Description  : set configuration for softap application
 * Parameters   : WIFI_CONFIG
 * Returns      : void
 *******************************************************************************/
void set_wifi_softap_config(WIFI_CONFIG *param)
{
	memset(param, 0x0, WIFI_CONFIG_SIZE);
	memcpy(param->ap_ip, NRC_AP_IP, sizeof(NRC_AP_IP));
	memcpy(param->ssid, STR_SSID, sizeof(STR_SSID));
	memcpy(param->country, COUNTRY_CODE, sizeof(COUNTRY_CODE));
	param->security_mode = NRC_WIFI_SECURE;
#if defined(NRC_WIFI_SECURE) && (NRC_WIFI_SECURE == WIFI_SEC_WPA2)
	memcpy(param->password, NRC_WIFI_PASSWORD, sizeof(NRC_WIFI_PASSWORD));
#endif
	param->tx_power = TX_POWER;
	param->dhcp_server = NRC_WIFI_SOFTAP_DHCP_SERVER;
	param->count = NRC_WIFI_TEST_COUNT;
	param->interval = NRC_WIFI_TEST_INTERVAL;
	param->channel = NRC_AP_SET_CHANNEL;
}
