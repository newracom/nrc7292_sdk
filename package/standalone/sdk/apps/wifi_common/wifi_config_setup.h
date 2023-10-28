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

#ifndef __WIFI_CONFIG_SETUP_H__
#define __WIFI_CONFIG_SETUP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "nrc_types.h"

#define MAX_FREQ_NUM	50

typedef struct  {
	uint8_t ssid[MAX_SSID_LENGTH + 1];
	uint8_t bssid[MAX_BSSID_LENGTH + 1];
	uint8_t country[MAX_CC_LENGTH + 1];
	uint8_t security_mode;
	uint8_t password[MAX_PW_LENGTH + 1];
	uint8_t pmk[MAX_PMK_LENGTH+1];
	uint8_t pmk_ssid[MAX_SSID_LENGTH+1];
	uint8_t pmk_pw[MAX_PW_LENGTH+1];
	uint16_t scan_freq_list[MAX_FREQ_NUM];
	uint8_t scan_freq_num;
	uint16_t channel;
	uint8_t bw;
	uint16_t bcn_interval;
	uint8_t ip_mode;
	char static_ip[MAX_STATIC_IP_LENGTH+1];
	char netmask[MAX_STATIC_IP_LENGTH+1];
	char gateway[MAX_STATIC_IP_LENGTH+1];
	char remote_addr[MAX_STATIC_IP_LENGTH+1];
#ifdef CONFIG_IPV6
	char static_ip6[MAX_STATIC_IP_LENGTH+1];
#endif
	uint16_t remote_port;
	uint8_t tx_power;
	uint8_t tx_power_type;
	uint8_t dhcp_server;
	uint32_t conn_timeout;
	uint32_t disconn_timeout;
	uint32_t bss_max_idle;
	uint32_t bss_retry_cnt;
	uint8_t device_mode;
	uint8_t network_mode;
	uint8_t rc;
	uint8_t mcs;
	uint8_t gi;
	int8_t cca_thres;
	uint8_t ignore_broadcast_ssid;
	uint8_t max_num_sta;
	uint16_t listen_interval;
}WIFI_CONFIG;
#define WIFI_CONFIG_SIZE	sizeof (WIFI_CONFIG)

/*********************************************************************
 * @fn set default configuration
 *
 * @brief Set default wifi configuration in memory
 *
 * @param vif
 *
 * @param wifi configuration ptr
 *
 * @return nrc_err_t
 **********************************************************************/
 static nrc_err_t set_wifi_defaults(WIFI_CONFIG* wifi_config);


/*********************************************************************
 * @fn nrc_save_wifi_config
 *
 * @brief Save wifi configration to key+value storage
 *
 * @param wifi configuration ptr
 *
 * @param rewrite whether to forcefully rewrite configuration
 *
 * @return nrc_err_t
 **********************************************************************/
nrc_err_t nrc_save_wifi_config(WIFI_CONFIG* wifi_config, int rewrite);


/*********************************************************************
 * @fn print_settings
 *
 * @brief Print settings to debug serial port
 *
 * @param wifi configuration ptr
 *
 * @return none
 **********************************************************************/
void print_settings(WIFI_CONFIG* wifi_config);


/*********************************************************************
 * @fn nrc_set_default_scan_channel
 *
 * @brief set frequency channels to scan on
 *
 * @param wifi configuration ptr
 *
 * @return true or false
 **********************************************************************/
bool nrc_set_default_scan_channel(WIFI_CONFIG *wifi_config);


/*********************************************************************
 * @fn nrc_wifi_set_config
 *
 * @brief set wifi configration data
 *
 * @param wifi configuration ptr
 * @return nrc_err_t
 **********************************************************************/
nrc_err_t nrc_wifi_set_config(WIFI_CONFIG* wifi_config);


/*********************************************************************
 * @fn nrc_erase_all_wifi_nvs
 *
 * @brief Erase all wifi configuration in NVS
 *
 * @param void
 *
 * @return nrc_err_t
 **********************************************************************/
nrc_err_t nrc_erase_all_wifi_nvs(void);


/*********************************************************************
 * @fn nrc_get_global_wifi_config
 *
 * @brief Get globally accessible configuration parameter
 *
 * @param void
 *
 * @return wifi configuration ptr
 *********************************************************************/
WIFI_CONFIG* nrc_get_global_wifi_config(void);

#ifdef __cplusplus
}
#endif

#endif /* __WIFI_CONFIG_SETUP_H__ */
