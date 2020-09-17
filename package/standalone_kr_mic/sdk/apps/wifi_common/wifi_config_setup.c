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
#include "wifi_config.h"

/******************************************************************************
 * FunctionName : set_wifi_config
 * Description  : set configuration for sample application
 * Parameters   : WIFI_CONFIG
 * Returns      : void
 *******************************************************************************/
void set_wifi_config(WIFI_CONFIG *param)
{
	memset(param, 0x0, WIFI_CONFIG_SIZE);
	memcpy(param->ssid, STR_SSID, MAX_SSID_LENGTH);
	memcpy(param->country, COUNTRY_CODE, MAX_CC_LENGTH);
	memcpy(param->remote_addr, NRC_REMOTE_ADDRESS, MAX_STATIC_IP_LENGTH);
	param->security_mode = NRC_WIFI_SECURE;
#if defined(NRC_WIFI_SECURE) && (NRC_WIFI_SECURE == MODE_WPA2)
	memcpy(param->password, NRC_WIFI_PASSWORD, MAX_PW_LENGTH);
#endif
	param->tx_power = TX_POWER;
	param->count = NRC_WIFI_TEST_COUNT;
	param->interval = NRC_WIFI_TEST_INTERVAL;
	param->dhcp_enable = NRC_WIFI_DHCP_ENABLE;

#if (NRC_WIFI_DHCP_ENABLE != 1)
	memcpy(param->static_ip, NRC_STATIC_IP, MAX_STATIC_IP_LENGTH);
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
	memcpy(param->ap_ip, NRC_AP_IP, MAX_STATIC_IP_LENGTH);
	memcpy(param->ssid, STR_SSID, MAX_SSID_LENGTH);
	memcpy(param->country, COUNTRY_CODE, MAX_CC_LENGTH);
	param->security_mode = NRC_WIFI_SECURE;
#if defined(NRC_WIFI_SECURE) && (NRC_WIFI_SECURE == MODE_WPA2)
	memcpy(param->password, NRC_WIFI_PASSWORD, MAX_PW_LENGTH);
#endif
	param->tx_power = TX_POWER;
	param->dhcp_server = NRC_WIFI_SOFTAP_DHCP_SERVER;
	param->count = NRC_WIFI_TEST_COUNT;
	param->interval = NRC_WIFI_TEST_INTERVAL;
}
