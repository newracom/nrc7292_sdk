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
#include "wifi_config_setup.h"
#include "wifi_config.h"

#ifdef SUPPORT_NVS_FLASH
#include <nvs.h>
#include "nvs_config.h"
#endif
#include "crypto/sha1.h"

#define DISPLAY_WIFI_CONFIG_SETTING 0

static WIFI_CONFIG* g_wifi_config;

 /*********************************************************************
 * @brief set default configuration
  *
 * Set default wifi configuration in memory
  *
 * @param wifi configuration ptr
 * @returns nrc_err_t
 **********************************************************************/
 static nrc_err_t set_wifi_defaults(WIFI_CONFIG* wifi_config)
{
	nrc_usr_print("[%s]\n", __func__);

	if (!wifi_config)
		return NRC_FAIL;

	memcpy(wifi_config->ssid,  STR_SSID, sizeof(STR_SSID));
	memcpy(wifi_config->bssid,	STR_BSSID,	(MAX_BSSID_LENGTH+1));
	memcpy(wifi_config->country,  COUNTRY_CODE, sizeof(COUNTRY_CODE));
	wifi_config->security_mode = NRC_WIFI_SECURE;
	memcpy(wifi_config->password, NRC_WIFI_PASSWORD, sizeof(NRC_WIFI_PASSWORD));
	memset(wifi_config->pmk,  0x0,	(MAX_PMK_LENGTH+1));
	memset(wifi_config->pmk_ssid,  0x0,  (MAX_SSID_LENGTH+1));
	memset(wifi_config->pmk_pw,  0x0,  (MAX_PW_LENGTH+1));
	nrc_set_default_scan_channel(wifi_config);
	wifi_config->channel = NRC_WIFI_CHANNEL;
	wifi_config->bw = NRC_AP_SET_CHANNEL_BW;
	wifi_config->bcn_interval =  NRC_WIFI_BCN_INTERVAL;
	wifi_config->ip_mode = NRC_WIFI_IP_MODE;
	memcpy(wifi_config->static_ip,  NRC_STATIC_IP, sizeof(NRC_STATIC_IP));
	memcpy(wifi_config->netmask,  NRC_NETMASK, sizeof(NRC_NETMASK));
	memcpy(wifi_config->gateway,  NRC_GATEWAY, sizeof(NRC_GATEWAY));
	memcpy(wifi_config->remote_addr,  NRC_REMOTE_ADDRESS, sizeof(NRC_REMOTE_ADDRESS));
#ifdef CONFIG_IPV6
	memset(wifi_config->static_ip6,  0x0,  (MAX_STATIC_IP_LENGTH+1));
#endif
	wifi_config->remote_port = NRC_REMOTE_PORT;
	wifi_config->tx_power = TX_POWER;
	wifi_config->tx_power_type = TX_POWER_TYPE;
	wifi_config->dhcp_server = NRC_WIFI_SOFTAP_DHCP_SERVER;
	wifi_config->conn_timeout = WIFI_CONN_TIMEOUT;
	wifi_config->disconn_timeout = WIFI_DISCONN_TIMEOUT;
	wifi_config->bss_max_idle = WIFI_BSS_MAX_IDLE;
	wifi_config->bss_retry_cnt = WIFI_BSS_RETRY_CNT;
	wifi_config->device_mode = WIFI_DEVICE_MODE;
	wifi_config->network_mode = WIFI_NETWORK_MODE;
	wifi_config->rc = NRC_WIFI_RATE_CONTROL ;
	wifi_config->mcs = NRC_WIFI_MCS_DEFAULT ;
	wifi_config->gi = NRC_WIFI_GUARD_INTERVAL_DEFAULT;
	wifi_config->cca_thres = NRC_WIFI_CCA_THRES_DEFAULT;
	wifi_config->ignore_broadcast_ssid = NRC_WIFI_IGNORE_BROADCAST_SSID_DEFAULT;
	wifi_config->max_num_sta = NRC_WIFI_SOFTAP_MAX_NUM_STA_DEFAULT;
	wifi_config->listen_interval = NRC_WIFI_LISTEN_INTERVAL_DEFAULT;

	return NRC_SUCCESS;
}


/*********************************************************************
 * @brief save wifi configration data
 *
 * Save wifi configration to key+value storage
 *
 * @param wifi configuration ptr
 *
 * @param rewrite : whether to forcefully rewrite configurations
 * @returns nrc_err_t
 **********************************************************************/
nrc_err_t nrc_save_wifi_config(WIFI_CONFIG* wifi_config, int rewrite)
{
	nrc_usr_print("[%s]\n", __func__);

#ifdef SUPPORT_NVS_FLASH
	nvs_err_t err = NVS_OK;
	nvs_handle_t nvs_handle = 0;
	size_t length = 0;
	int32_t nvs_signed_int = 0;
	uint8_t written = 0;

	if (!wifi_config)
		goto failed;

	err = nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle);
	if (NVS_OK != err)
		goto failed;

	if (!rewrite) {
		if (nvs_get_u8(nvs_handle, NVS_CONFIG_WRITTEN, &written) == NVS_OK) {
			if (written) {
				if (nvs_handle)
					nvs_close(nvs_handle);
				return NRC_FAIL;
			}
		}
	}

	nvs_set_u8(nvs_handle, NVS_CONFIG_WRITTEN, 1);

	nvs_set_str(nvs_handle, NVS_SSID, (char*)wifi_config->ssid);
	nvs_set_str(nvs_handle, NVS_BSSID, (char*)wifi_config->bssid);
	nvs_set_str(nvs_handle, NVS_COUNTRY, (char*)wifi_config->country);
	nvs_set_u8(nvs_handle, NVS_WIFI_SECURITY, (uint8_t)wifi_config->security_mode);
	nvs_set_str(nvs_handle, NVS_WIFI_PASSWORD, (char*)wifi_config->password);
	nvs_set_str(nvs_handle, NVS_WIFI_PMK, (char*)wifi_config->pmk);
	nvs_set_str(nvs_handle, NVS_WIFI_PMK_SSID, (char*)wifi_config->pmk_ssid);
	nvs_set_str(nvs_handle, NVS_WIFI_PMK_PASSWORD, (char*)wifi_config->pmk_pw);
	nvs_set_u16(nvs_handle, NVS_WIFI_CHANNEL, (uint16_t)wifi_config->channel);
	nvs_set_u8(nvs_handle, NVS_WIFI_CHANNEL_BW, (uint8_t)wifi_config->bw);
	nvs_set_u16(nvs_handle, NVS_WIFI_BCN_INTERVAL, (uint16_t)wifi_config->bcn_interval);
	nvs_set_u8(nvs_handle, NVS_IP_MODE, (uint8_t)wifi_config->ip_mode);
	nvs_set_str(nvs_handle, NVS_STATIC_IP, (char*)wifi_config->static_ip);
	nvs_set_str(nvs_handle, NVS_NETMASK, (char*)wifi_config->netmask);
	nvs_set_str(nvs_handle, NVS_GATEWAY, (char*)wifi_config->gateway);
	nvs_set_str(nvs_handle, NVS_REMOTE_ADDRESS, (char*)wifi_config->remote_addr);
#ifdef CONFIG_IPV6
	nvs_set_str(nvs_handle, NVS_STATIC_IP6, (char*)wifi_config->static_ip6);
#endif
	nvs_set_u16(nvs_handle, NVS_REMOTE_PORT, (uint16_t)wifi_config->remote_port);
	nvs_set_u8(nvs_handle, NVS_WIFI_TX_POWER, (uint8_t)wifi_config->tx_power);
	nvs_set_u8(nvs_handle, NVS_WIFI_TX_POWER_TYPE, (uint8_t)wifi_config->tx_power_type);
	nvs_set_u8(nvs_handle, NVS_DHCP_SERVER_ON_WLAN, (uint8_t)wifi_config->dhcp_server);
	nvs_set_i32(nvs_handle, NVS_WIFI_CONN_TIMEOUT, (int32_t)wifi_config->conn_timeout);
	nvs_set_i32(nvs_handle, NVS_WIFI_DISCONN_TIMEOUT, (int32_t)wifi_config->disconn_timeout);
	nvs_set_i32(nvs_handle, NVS_BSS_MAX_IDLE, (int32_t)wifi_config->bss_max_idle);
	nvs_set_u8(nvs_handle, NVS_DEVICE_MODE, (uint8_t)wifi_config->device_mode);
	nvs_set_u8(nvs_handle, NVS_NETWORK_MODE, (uint8_t)wifi_config->network_mode);
	nvs_set_u8(nvs_handle, NVS_WIFI_RATE_CONTROL, (uint8_t)wifi_config->rc);
	nvs_set_u8(nvs_handle, NVS_WIFI_MCS, (uint8_t)wifi_config->mcs);
	nvs_set_u8(nvs_handle, NVS_WIFI_GI, (uint8_t)wifi_config->gi);
	nvs_set_i8(nvs_handle, NVS_WIFI_CCA_THRES, (int8_t)wifi_config->cca_thres);
	nvs_set_u8(nvs_handle, NVS_WIFI_IGNORE_BROADCAST_SSID , (uint8_t)wifi_config->ignore_broadcast_ssid);
	nvs_set_u8(nvs_handle, NVS_WIFI_SOFTAP_MAX_NUM_STA, (uint8_t)wifi_config->max_num_sta);
	nvs_set_u16(nvs_handle, NVS_WIFI_LISTEN_INTERVAL, (uint16_t)wifi_config->listen_interval);

	err = nvs_commit(nvs_handle);
	if (NVS_OK != err)
		goto failed;

	if (nvs_handle)
		nvs_close(nvs_handle);

	return NRC_SUCCESS;

failed:
	nrc_usr_print("[%s] Failed to save wifi settings\n", __func__);

	if (nvs_handle)
		nvs_close(nvs_handle);

	return NRC_FAIL;
#else
	nrc_usr_print("[%s] NVS is not enabled\n", __func__);
	return NRC_SUCCESS;
#endif /* SUPPORT_NVS_FLASH */
}


/*********************************************************************
 * @brief print out settings
 *
 * Print settings to debug serial port
 *
 * @param wifi configuration ptr
 * @returns none
 **********************************************************************/
void print_settings(WIFI_CONFIG* wifi_config)
{
#if defined(DISPLAY_WIFI_CONFIG_SETTING) && (DISPLAY_WIFI_CONFIG_SETTING == 1)
	nrc_usr_print("\n-----------------------------------------------\n");
	nrc_usr_print("[%s] wifi settings:\n\n", __func__);
	nrc_usr_print("ssid %s\n", wifi_config->ssid);
	nrc_usr_print("bssid %s\n", wifi_config->bssid);
	nrc_usr_print("country %s\n", wifi_config->country);
	nrc_usr_print("security %d\n", wifi_config->security_mode);
	nrc_usr_print("pasword %s\n", wifi_config->password);
	nrc_usr_print("pmk %s\n", wifi_config->pmk);
	nrc_usr_print("pmk_ssid %s\n", wifi_config->pmk_ssid);
	nrc_usr_print("pmk_pw %s\n", wifi_config->pmk_pw);
	nrc_usr_print("scan_freq_num %d\n", wifi_config->scan_freq_num);
	for(int i=0; i<wifi_config->scan_freq_num; i++) {
		nrc_usr_print("%d \n", wifi_config->scan_freq_list[i]);
		if((i%10)==9) nrc_usr_print("\n");
	}
	nrc_usr_print("channel %d\n", wifi_config->channel);
	nrc_usr_print("bw %d\n", wifi_config->bw);
	nrc_usr_print("bcn interval %d\n", wifi_config->bcn_interval);
	nrc_usr_print("short bcn interval %d\n", wifi_config->short_bcn_interval);
	nrc_usr_print("ip_mode %d [%s]\n", wifi_config->ip_mode,
		( wifi_config->ip_mode == 0) ? "Static IP" : "Dynamic IP");
	nrc_usr_print("static ip %s\n", wifi_config->static_ip);
	nrc_usr_print("netmask %s\n", wifi_config->netmask);
	nrc_usr_print("gateway %s\n", wifi_config->gateway);
	nrc_usr_print("remote_addr %s\n", wifi_config->remote_addr);
#ifdef CONFIG_IPV6
	nrc_usr_print("static_ip6 %s\n", wifi_config->static_ip6);
#endif
	nrc_usr_print("remote_port %d\n", wifi_config->remote_port);
	nrc_usr_print("txpower %d\n", wifi_config->tx_power);
	nrc_usr_print("tx_power_type %d\n", wifi_config->tx_power_type);
	nrc_usr_print("dhcp_server %d\n", wifi_config->dhcp_server);
	nrc_usr_print("conn_timeout %d\n", wifi_config->conn_timeout);
	nrc_usr_print("disconn_timeout %d\n", wifi_config->disconn_timeout);
	nrc_usr_print("bss_max_idle %d\n", wifi_config->bss_max_idle);
	nrc_usr_print("bss_retry_cnt %d\n", wifi_config->bss_retry_cnt);
	nrc_usr_print("device_mode %d [%s]\n",wifi_config->device_mode,
		( wifi_config->device_mode == 0) ? "STA" : "AP");
	nrc_usr_print("network_mode %d [%s]\n",wifi_config->network_mode,
		( wifi_config->network_mode == 0) ? "Bridge" : "NAT");
	nrc_usr_print("rate control %d [%s]\n",wifi_config->rc,
		( wifi_config->rc == 0) ? "OFF" : "ON");
	nrc_usr_print("mcs %d\n", wifi_config->mcs);
	nrc_usr_print("cca threshol %d\n", wifi_config->cca_thres);
	nrc_usr_print("ssid_type %d\n", wifi_config->ignore_broadcast_ssid);
	nrc_usr_print("max_num_sta %d\n", wifi_config->max_num_sta);
	nrc_usr_print("beacon_cnt %d\n", wifi_config->beacon_cnt);
	nrc_usr_print("-----------------------------------------------\n\n");
#endif
}


/*********************************************************************
 * @brief save default configuration
 *
 * Save default configuration to key+value storage
 *
 * @param wifi configuration ptr
 * @returns nrc_err_t
 **********************************************************************/
static nrc_err_t save_wifi_defaults(WIFI_CONFIG* wifi_config)
{
	nrc_usr_print("[%s]\n", __func__);

	if (!wifi_config)
		return NRC_FAIL;

	set_wifi_defaults(wifi_config);
	print_settings(wifi_config);
	return NRC_SUCCESS;
}


/******************************************************************************
 * @fn parse_channel_list
 *
 * @brief Parses a comma-separated list of Wi-Fi channels and returns
 *           an array containing the list of channels as uint16_t values.
 *
 * @param channel_list - A string containing a comma-separated list of
 *                           Wi-Fi channels in MHz (e.g. "2412,2437,2462").
 *           list_size    - A pointer to a the number of channels in the list.
 *
 * @return  A pointer to an array of uint16_t values representing the list
 *            of Wi-Fi channels. The array must be freed by the caller when
 *            it is no longer needed.
 *******************************************************************************/
static uint16_t* parse_channel_list(const char* channel_list, uint8_t * list_size) {
	// Check for NULL or empty channel_list
	if (channel_list == NULL || *channel_list == '\0') {
		*list_size = 0;
		return NULL;
	}

	// Count number of comma-separated values in channel_list
	size_t num_channels = 1;
	for (const char* p = channel_list; *p; p++) {
		if (*p == ',')
			num_channels++;
	}

	// Allocate memory for nrc_scan_freq_list
	uint16_t* nrc_scan_freq_list = (uint16_t*)nrc_mem_malloc(sizeof(uint16_t) * num_channels);

	// Parse channel list
	int i = 0;
	char* token = strtok((char*)channel_list, ",");
	while (token != NULL) {
		nrc_scan_freq_list[i++] = atoi(token);
		token = strtok(NULL, ",");
	}

	// Update list_size to reflect actual size of nrc_scan_freq_list
	*list_size = i;

	// Return pointer to nrc_scan_freq_list
	return nrc_scan_freq_list;
}


/******************************************************************************
 * @fn nrc_set_default_scan_channel
 *
 * @brief set frequency channels to scan on
 *
 * @param wifi configuration ptr
 *
 * @return true or false
 *******************************************************************************/
bool nrc_set_default_scan_channel(WIFI_CONFIG *wifi_config)
{
#if NRC_WIFI_SCAN_LIST
	uint8_t list_size = 0;
	uint16_t* nrc_scan_freq_list = parse_channel_list(SCAN_CHANNEL_LIST, &list_size);

	if (nrc_scan_freq_list == NULL)
		return false;

	wifi_config->scan_freq_num = list_size;
	for (int i=0; i < list_size; i++){
		wifi_config->scan_freq_list[i] = nrc_scan_freq_list[i];
		A("%d ", wifi_config->scan_freq_list[i]);
		if((i)%10 == 9)
			A("\n");
	}
	A("\n");
	nrc_mem_free(nrc_scan_freq_list);
	return true;
#else
	return false;
#endif
}


/******************************************************************************
 * @fn nrc_generate_pmk
 *
 * @brief generate PMK
 *
 * @param ssid
 *
 * @param passphrase
 *
 * @param pmk (Pairwise Master Key)
 *
 * @return true or false
 *******************************************************************************/
static int nrc_generate_pmk(char* ssid, const char *passphrase, char *pmk)
{
	int ssid_len;
	const int iterations = 4096;
	const int pmk_len = 32;
	char pmk_hash[32];
	char buf[64];

	if(ssid == NULL || passphrase == NULL || pmk == NULL)
		return false;

	ssid_len = strlen(ssid);

	if (pbkdf2_sha1(passphrase, (const u8 *)ssid, ssid_len, iterations, (u8 *)pmk_hash, pmk_len) < 0) {
		E(TT_SDK_WIFI, "PMK generation is failed\n");
		return false;
	}

	for (int i =0; i<32; i++){
		sprintf(buf, "%02x",pmk_hash[i]);
		strcat(pmk, buf);
	}

	V(TT_SDK_WIFI, "PMK for %s based on passphrase '%s'\n", ssid , passphrase);
	V(TT_SDK_WIFI, "psk_value : %s\n", pmk);

	return true;
}


/*********************************************************************
 * @FunctionName : nrc_wifi_set_config
 * @brief           : set wifi configration data
 * @param wifi configuration ptr
 * @returns nrc_err_t
 **********************************************************************/
nrc_err_t nrc_wifi_set_config(WIFI_CONFIG* wifi_config)
{
	nrc_usr_print("[%s]\n", __func__);

	if (!wifi_config){
		nrc_usr_print("[%s] wifi_config in NULL\n", __func__);
	}

	memset(wifi_config, 0x0, sizeof(WIFI_CONFIG));
	set_wifi_defaults(wifi_config);

#ifdef SUPPORT_NVS_FLASH
	nvs_err_t err = NVS_OK;
	nvs_handle_t nvs_handle = 0;
	int32_t nvs_signed_int = 0;
	size_t length = 0;

	err = nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle);
	if (NVS_OK != err) {
		nrc_usr_print("[%s] Failed to open NVS\n", __func__);
		return NRC_FAIL;
	}

	nvs_get_u8(nvs_handle, NVS_IP_MODE, &wifi_config->ip_mode);
	length = sizeof(wifi_config->ssid);
	nvs_get_str(nvs_handle, NVS_SSID, (char *) wifi_config->ssid, &length);

	length = sizeof(wifi_config->bssid);
	nvs_get_str(nvs_handle, NVS_BSSID, (char *) wifi_config->bssid, &length);

	length = sizeof(wifi_config->static_ip);
	nvs_get_str(nvs_handle, NVS_STATIC_IP, (char *) wifi_config->static_ip, &length);

	length = sizeof(wifi_config->country);
	nvs_get_str(nvs_handle, NVS_COUNTRY, (char *) wifi_config->country, &length);

	nvs_get_u8(nvs_handle, NVS_WIFI_SECURITY, (uint8_t*)&wifi_config->security_mode);

	length = sizeof(wifi_config->password);
	nvs_get_str(nvs_handle, NVS_WIFI_PASSWORD, (char *) wifi_config->password, &length);

	length = sizeof(wifi_config->pmk);
	nvs_get_str(nvs_handle, NVS_WIFI_PMK, (char *) wifi_config->pmk, &length);

	length = sizeof(wifi_config->pmk_ssid);
	nvs_get_str(nvs_handle, NVS_WIFI_PMK_SSID, (char *) wifi_config->pmk_ssid, &length);

	length = sizeof(wifi_config->pmk_pw);
	nvs_get_str(nvs_handle, NVS_WIFI_PMK_PASSWORD, (char *) wifi_config->pmk_pw, &length);

	nvs_get_u16(nvs_handle,NVS_WIFI_CHANNEL, (uint16_t*)&wifi_config->channel);
	nvs_get_u8(nvs_handle, NVS_WIFI_CHANNEL_BW, (uint8_t*)&wifi_config->bw);
	nvs_get_u16(nvs_handle, NVS_WIFI_BCN_INTERVAL, (uint16_t*)&wifi_config->bcn_interval);
	nvs_get_u8(nvs_handle, NVS_IP_MODE, (uint8_t*)&wifi_config->ip_mode);

	length = sizeof(wifi_config->netmask);
	nvs_get_str(nvs_handle, NVS_NETMASK, (char*)wifi_config->netmask, &length);

	length = sizeof(wifi_config->gateway);
	nvs_get_str(nvs_handle, NVS_GATEWAY, (char*)wifi_config->gateway, &length);

	length = sizeof(wifi_config->remote_addr);
	nvs_get_str(nvs_handle, NVS_REMOTE_ADDRESS, (char*)wifi_config->remote_addr, &length);

#ifdef CONFIG_IPV6
	length = sizeof(wifi_config->static_ip6);
	nvs_get_str(nvs_handle, NVS_STATIC_IP6, (char*)wifi_config->static_ip6, &length);
#endif

	nvs_get_u16(nvs_handle, NVS_REMOTE_PORT, (uint16_t*)&wifi_config->remote_port);
	nvs_get_u8(nvs_handle, NVS_WIFI_TX_POWER, (uint8_t*)&wifi_config->tx_power);
	nvs_get_u8(nvs_handle, NVS_WIFI_TX_POWER_TYPE, (uint8_t*)&wifi_config->tx_power_type);
	nvs_get_u8(nvs_handle, NVS_DHCP_SERVER_ON_WLAN, (uint8_t*)&wifi_config->dhcp_server);
	nvs_get_i32(nvs_handle, NVS_WIFI_CONN_TIMEOUT, (int32_t*)&wifi_config->conn_timeout);
	nvs_get_i32(nvs_handle, NVS_WIFI_DISCONN_TIMEOUT, (int32_t*)&wifi_config->disconn_timeout);
	nvs_get_i32(nvs_handle, NVS_BSS_MAX_IDLE, (int32_t*)&wifi_config->bss_max_idle);
	nvs_get_i32(nvs_handle, NVS_BSS_RETRY_CNT, (int32_t*)&wifi_config->bss_retry_cnt);
	nvs_get_u8(nvs_handle, NVS_DEVICE_MODE, (uint8_t*)&wifi_config->device_mode);
	nvs_get_u8(nvs_handle, NVS_NETWORK_MODE, (uint8_t*)&wifi_config->network_mode);
	nvs_get_u8(nvs_handle, NVS_WIFI_RATE_CONTROL, (uint8_t*)&wifi_config->rc);
	nvs_get_u8(nvs_handle, NVS_WIFI_MCS, (uint8_t*)&wifi_config->mcs);
	nvs_get_u8(nvs_handle, NVS_WIFI_GI, (uint8_t*)&wifi_config->gi);
	nvs_get_i8(nvs_handle, NVS_WIFI_CCA_THRES, (int8_t*)&wifi_config->cca_thres);
	nvs_get_u8(nvs_handle, NVS_WIFI_IGNORE_BROADCAST_SSID, (uint8_t*)&wifi_config->ignore_broadcast_ssid);
	nvs_get_u8(nvs_handle, NVS_WIFI_SOFTAP_MAX_NUM_STA, (uint8_t*)&wifi_config->max_num_sta);
	nvs_get_u16(nvs_handle, NVS_WIFI_LISTEN_INTERVAL, (uint16_t*)&wifi_config->listen_interval);

	if (nvs_handle)
		nvs_close(nvs_handle);
#else
	nrc_usr_print("[%s] NVS is not enabled\n", __func__);
#endif /* SUPPORT_NVS_FLASH */

	g_wifi_config = wifi_config;

	print_settings(wifi_config);
	return NRC_SUCCESS;
}


/*********************************************************************
 * @brief nrc_erase_all_wifi_nvs
 *
 * Erase all wifi configuration in NVS
 *
 * @param void
 * @returns nrc_err_t
 **********************************************************************/
nrc_err_t nrc_erase_all_wifi_nvs(void)
{
#ifdef SUPPORT_NVS_FLASH
	nvs_handle_t nvs_handle;

	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		A("nvs open failed.\n");
		return NRC_FAIL;
	}

	nvs_erase_key(nvs_handle, NVS_SSID);
	nvs_erase_key(nvs_handle, NVS_WIFI_SECURITY);
	nvs_erase_key(nvs_handle, NVS_WIFI_PASSWORD);
	nvs_erase_key(nvs_handle, NVS_WIFI_PMK);
	nvs_erase_key(nvs_handle, NVS_WIFI_PMK_SSID);
	nvs_erase_key(nvs_handle, NVS_WIFI_PMK_PASSWORD);
	nvs_erase_key(nvs_handle, NVS_WIFI_CHANNEL);
	nvs_erase_key(nvs_handle, NVS_WIFI_CHANNEL_BW);
	nvs_erase_key(nvs_handle, NVS_IP_MODE);
	nvs_erase_key(nvs_handle, NVS_REMOTE_ADDRESS);
	nvs_erase_key(nvs_handle, NVS_REMOTE_PORT);
	nvs_erase_key(nvs_handle, NVS_STATIC_IP);
	nvs_erase_key(nvs_handle, NVS_NETMASK);
	nvs_erase_key(nvs_handle, NVS_GATEWAY);
	nvs_erase_key(nvs_handle, NVS_STATIC_IP6);
	nvs_erase_key(nvs_handle, NVS_DHCP_SERVER_ON_WLAN);
	nvs_erase_key(nvs_handle, NVS_COUNTRY);
	nvs_erase_key(nvs_handle, NVS_BSS_MAX_IDLE);
	nvs_erase_key(nvs_handle, NVS_BSS_RETRY_CNT);
	nvs_erase_key(nvs_handle, NVS_WIFI_TX_POWER);
	nvs_erase_key(nvs_handle, NVS_WIFI_TX_POWER_TYPE);
	nvs_erase_key(nvs_handle, NVS_BSSID);
	nvs_erase_key(nvs_handle, NVS_WIFI_CONN_TIMEOUT);
	nvs_erase_key(nvs_handle, NVS_WIFI_DISCONN_TIMEOUT);
	nvs_erase_key(nvs_handle, NVS_WIFI_BCN_INTERVAL);
	nvs_erase_key(nvs_handle, NVS_DEVICE_MODE);
	nvs_erase_key(nvs_handle, NVS_NETWORK_MODE);
	nvs_erase_key(nvs_handle, NVS_WIFI_RATE_CONTROL);
	nvs_erase_key(nvs_handle, NVS_WIFI_MCS);
	nvs_erase_key(nvs_handle, NVS_WIFI_GI);
	nvs_erase_key(nvs_handle, NVS_WIFI_CCA_THRES);
	nvs_erase_key(nvs_handle, NVS_WIFI_IGNORE_BROADCAST_SSID);

	if (nvs_handle)
		nvs_close(nvs_handle);

	A("Erased all wifi related settings in nvs.\n");
	return NRC_SUCCESS;
#else
	nrc_usr_print("[%s] NVS is not enabled\n", __func__);
	return NRC_FAIL;
#endif
}


 /*********************************************************************
 * @brief get global config
 *
 * Get globally accessible configuration parameter
 *
 * @param void
 * @returns wifi configuration ptr
 *********************************************************************/
WIFI_CONFIG* nrc_get_global_wifi_config(void)
{
	return (WIFI_CONFIG*)g_wifi_config;
}

