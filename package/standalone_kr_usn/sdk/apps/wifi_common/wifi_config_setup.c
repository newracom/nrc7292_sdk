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

#ifdef SUPPORT_NVS_FLASH
#include <nvs.h>
#include "nvs_config.h"
#endif

/******************************************************************************
 * FunctionName : set_wifi_config
 * Description  : set configuration for sample application
 * Parameters   : WIFI_CONFIG
 * Returns      : void
 *******************************************************************************/
void set_wifi_config(WIFI_CONFIG *param)
{
#ifdef SUPPORT_NVS_FLASH
	nvs_handle_t nvs_handle;
	nvs_err_t err = NVS_OK;
	size_t length = 0;

	err = nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READONLY, &nvs_handle);
	if (err != NVS_OK) {
		nrc_usr_print("[%s] nvs_open failed (0x%x).\n", __func__, err);
		return;
	}
#endif
	memset(param, 0x0, WIFI_CONFIG_SIZE);

#ifdef SUPPORT_NVS_FLASH
	length = sizeof(param->ssid);
	if (nvs_get_str(nvs_handle, NVS_SSID, (char *) param->ssid, &length) != NVS_OK)
#endif
	{
		memcpy(param->ssid, STR_SSID, sizeof(STR_SSID));
	}

	memcpy(param->country, COUNTRY_CODE, sizeof(COUNTRY_CODE));

#ifdef SUPPORT_NVS_FLASH
	length = sizeof(param->remote_addr);
	if (nvs_get_str(nvs_handle, NVS_REMOTE_ADDRESS, (char *) param->remote_addr, &length) != NVS_OK)
#endif
	{
		memcpy(param->remote_addr, NRC_REMOTE_ADDRESS, sizeof(NRC_REMOTE_ADDRESS));
	}
#ifdef SUPPORT_NVS_FLASH
	if (nvs_get_u16(nvs_handle, NVS_REMOTE_PORT, &param->remote_port) != NVS_OK)
#endif
	{
		param->remote_port = NRC_REMOTE_PORT;
	}
#ifdef SUPPORT_NVS_FLASH
	if (nvs_get_u8(nvs_handle, NVS_WIFI_SECURITY, &param->security_mode) != NVS_OK)
#endif
	{
		param->security_mode = NRC_WIFI_SECURE;
	}
	if ((param->security_mode == WIFI_SEC_WPA2) || (param->security_mode == WIFI_SEC_WPA3_SAE)) {
#ifdef SUPPORT_NVS_FLASH
		length = sizeof(param->password);
		if (nvs_get_str(nvs_handle, NVS_WIFI_PASSWORD, (char *) param->password, &length) != NVS_OK)
#endif
		{
			memcpy(param->password, NRC_WIFI_PASSWORD, sizeof(NRC_WIFI_PASSWORD));
		}
	}

	param->tx_power = TX_POWER;
	param->count = NRC_WIFI_TEST_COUNT;
	param->interval = NRC_WIFI_TEST_INTERVAL;
	param->short_bcn_interval = NRC_WIFI_TEST_INTERVAL / 10;
	param->duration = NRC_WIFI_TEST_DURATION;
#ifdef SUPPORT_NVS_FLASH
	if (nvs_get_u8(nvs_handle, NVS_IP_MODE, &param->ip_mode) != NVS_OK)
#endif
	{
		param->ip_mode = NRC_WIFI_IP_MODE;
	}
#if NRC_WIFI_SCAN_LIST
	param->scan_freq_num = NRC_WIFI_SCAN_FREQ_NUM;
	for (int i=0; i < param->scan_freq_num; i++)
		param->scan_freq_list[i] = nrc_scan_freq_list[i];
#endif

#if (NRC_WIFI_IP_MODE == WIFI_STATIC_IP)
#ifdef SUPPORT_NVS_FLASH
	length = sizeof(param->static_ip);
	if (nvs_get_str(nvs_handle, NVS_STATIC_IP, (char *) param->static_ip, &length) != NVS_OK)
#endif
	{
		memcpy(param->static_ip, NRC_STATIC_IP, sizeof(NRC_STATIC_IP));
	}
#endif /* #if (NRC_WIFI_IP_MODE == WIFI_STATIC_IP) */

#ifdef SUPPORT_NVS_FLASH
	nvs_close(nvs_handle);
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
#ifdef SUPPORT_NVS_FLASH
	nvs_handle_t nvs_handle;
	nvs_err_t err = NVS_OK;
	size_t length = 0;

	err = nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READONLY, &nvs_handle);
	if (err != NVS_OK) {
		nrc_usr_print("[%s] nvs_open failed (0x%x).\n", __func__, err);
		return;
	}
#endif
	memset(param, 0x0, WIFI_CONFIG_SIZE);

#ifdef SUPPORT_NVS_FLASH
	length = sizeof(param->ap_ip);
	if (nvs_get_str(nvs_handle, NVS_STATIC_IP, (char *) param->ap_ip, &length) != NVS_OK)
#endif
	{
		memcpy(param->ap_ip, NRC_AP_IP, sizeof(NRC_AP_IP));
	}

#ifdef SUPPORT_NVS_FLASH
	length = sizeof(param->ssid);
	if (nvs_get_str(nvs_handle, NVS_SSID, (char *) param->ssid, &length) != NVS_OK)
#endif
	{
		memcpy(param->ssid, STR_SSID, sizeof(STR_SSID));
	}
	memcpy(param->country, COUNTRY_CODE, sizeof(COUNTRY_CODE));

#ifdef SUPPORT_NVS_FLASH
	if (nvs_get_u8(nvs_handle, NVS_WIFI_SECURITY, &param->security_mode) != NVS_OK)
#endif
	{
		param->security_mode = NRC_WIFI_SECURE;
	}

	if ((param->security_mode == WIFI_SEC_WPA2) || (param->security_mode == WIFI_SEC_WPA3_SAE)) {
#ifdef SUPPORT_NVS_FLASH
		length = sizeof(param->password);
		if (nvs_get_str(nvs_handle, NVS_WIFI_PASSWORD, (char *) param->password, &length) != NVS_OK)
#endif
		{
			memcpy(param->password, NRC_WIFI_PASSWORD, sizeof(NRC_WIFI_PASSWORD));
		}
	}

	param->tx_power = TX_POWER;

#ifdef SUPPORT_NVS_FLASH
	if (nvs_get_u8(nvs_handle, NVS_DHCP_SERVER_ON_WLAN, &param->dhcp_server) != NVS_OK)
#endif
	{
		param->dhcp_server = NRC_WIFI_SOFTAP_DHCP_SERVER;
	}
	param->count = NRC_WIFI_TEST_COUNT;

	param->interval = NRC_WIFI_TEST_INTERVAL;

#ifdef SUPPORT_NVS_FLASH
	if (nvs_get_u32(nvs_handle, NVS_WIFI_CHANNEL, &param->channel) != NVS_OK)
#endif
	{
		param->channel = NRC_AP_SET_CHANNEL;
	}

#ifdef SUPPORT_NVS_FLASH
	nvs_close(nvs_handle);
#endif
}
