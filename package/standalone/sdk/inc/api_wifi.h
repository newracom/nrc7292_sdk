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

#ifndef __NRC_API_WIFI_H__
#define __NRC_API_WIFI_H__

/**********************************************
 * @struct NRC_UART_CONFIG
 * @brief UART configuration parameters
 ***********************************************/

/** @brief security mode */
typedef enum {
	MODE_OPEN	= 0,
	MODE_WPA2	= 1,
	MODE_SEC_MAX,
} NRC_WIFI_SECURITY;

/** @brief connection results	*/
typedef enum {
	WIFI_INIT_FAIL 			= -1,
	WIFI_GET_ND			= -2,
	WIFI_SET_FAIL 			= -3,
	WIFI_CONNECTION_FAIL	= -4,
	WIFI_DHCP_FAIL			= -5,
	WIFI_SET_IP_FAIL		= -6,
	WIFI_SOFTAP_FAIL		= -7,
	WIFI_SUCCESS			= 0,
}NRC_WIFI_CONN_RET;

/** @brief scan item */
typedef union
{
	char *items[5];
	struct
	{
		char *bssid;
		char *freq;
		char *sig_level;
		char *flags;
		char *ssid;
	};
} SCAN_RESULT;

/** @brief scan list */
typedef struct
{
#define SCAN_RESULT_BUFFER_SIZE		512
#define SCAN_RESULT_NUM				10

	int n_result;
	SCAN_RESULT result[SCAN_RESULT_NUM];
} SCAN_RESULTS;

/**********************************************
 * @fn bool nrc_wifi_get_dhcp(void)
 *
 * @brief Get DHCP state
 *
 * @return true(enabled) or false(disabled)
 ***********************************************/
bool nrc_wifi_get_dhcp(void);


/**********************************************
 * @fn bool nrc_wifi_set_dhcp(bool dhcp, char *ip_addr)
 *
 * @brief Set DHCP or static IP
 *
 * @param dhcp: true(DHCP) or false(static IP)
 *
 * @return true(success) or false(fail)
 ***********************************************/
bool nrc_wifi_set_dhcp(bool dhcp, char *ip_addr);


/**********************************************
 * @fn char* nrc_wifi_get_ip_address(void)
 *
 * @brief Get IP address
 *
 * @return IP address
 ***********************************************/
char* nrc_wifi_get_ip_address(void);


/**********************************************
 * @fn int nrc_wifi_get_nd(void)
 *
 * @brief Get network index for Wi-Fi connection
 *
 * @return If it's bigger than 0, then it means a network index. Otherwise, it's fail.
 ***********************************************/
int nrc_wifi_get_nd(void);


/**********************************************
 * @fn int nrc_wifi_set_country(char *country_code)
 *
 * @brief Set country code
 *
 * @param country_code: country code (JP, KR, US, TW, EU)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(NRC_WIFI_CONN_RET) is returned.
 ***********************************************/
int nrc_wifi_set_country(char *country_code);


/**********************************************
 * @fn int nrc_wifi_set_ssid(int index, char *ssid)
 *
 * @brief Set SSID of AP that STA wants to connect
 *
 * @param index: network index
 *
 * @param ssid: SSID
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(NRC_WIFI_CONN_RET) is returned.
 ***********************************************/
int nrc_wifi_set_ssid(int index, char *ssid);


/**********************************************
 * @fn int nrc_wifi_set_security(int index, int mode, char *password)
 *
 * @brief Set security parameters for Wi-Fi connection
 *
 * @param index: network index
 *
 * @param mode: security mode (Open, WPA1, WPA2)
 *
 * @param password: PASSWORD
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(NRC_WIFI_CONN_RET) is returned.
 ***********************************************/
int nrc_wifi_set_security(int index, int mode, char *password);


/**********************************************
 * @fn int nrc_wifi_scan(void)
 *
 * @brief Start scan procedure (sync: block until scan is done)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(NRC_WIFI_CONN_RET) is returned.
 ***********************************************/
int nrc_wifi_scan(void);


/**********************************************
 * @fn int nrc_wifi_scan_results(SCAN_RESULTS *results)
 *
 * @brief Get scan results
 *
 * @param results: scan list
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(NRC_WIFI_CONN_RET) is returned.
 ***********************************************/
int nrc_wifi_scan_results(SCAN_RESULTS *results);


/**********************************************
 * @fn int nrc_wifi_connect(int index)
 *
 * @brief Start connection (sync: block until association is connected)
 *
 * @param index: network index
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(NRC_WIFI_CONN_RET) is returned.
 ***********************************************/
int nrc_wifi_connect(int index);


/**********************************************
 * @fn int nrc_wifi_connect_async(int index)
 *
 * @brief Start connection (async: unblock regardless of getting connected)
 *
 * @param index: network index
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(NRC_WIFI_CONN_RET) is returned.
 ***********************************************/
int nrc_wifi_connect_async(int index);


/**********************************************
 * @fn int nrc_wifi_disconnect(int index)
 *
 * @brief Start disconnection (async: unblock regardless of getting disconnected)
 *
 * @param index: network index
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(NRC_WIFI_CONN_RET) is returned.
 ***********************************************/
int nrc_wifi_disconnect(int index);


/**********************************************
 * @fn int nrc_wifi_disconnect_sync(int index)
 *
 * @brief Start disconnection (sync: block until getting disconnected)
 *
 * @param index: network index
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(NRC_WIFI_CONN_RET) is returned.
 ***********************************************/
int nrc_wifi_disconnect_sync(int index);


/**********************************************
 * @fn int nrc_wifi_get_ip(void)
 *
 * @brief Get IP address. Start DHCP or set Static IP (Sync: block until getting IP)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(NRC_WIFI_CONN_RET) is returned.
 ***********************************************/
int nrc_wifi_get_ip(void);


/**********************************************
 * @fn WLAN_STATE_ID nrc_wifi_get_state(void)
 *
 * @brief Get state of Wi-Fi connection
 *
 * @return WLAN_STATE_ID
 ***********************************************/
WLAN_STATE_ID nrc_wifi_get_state(void);


/**********************************************
 * @fn WLAN_STATE_ID nrc_wifi_set_state(WLAN_STATE_ID state)
 *
 * @brief Set state of Wi-Fi connection
 *
 * @param state: wifi state
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(NRC_WIFI_CONN_RET) is returned.
 ***********************************************/
void nrc_wifi_set_state(WLAN_STATE_ID state);


/**********************************************
 * @fn int nrc_wifi_softap_set_ip(char *ip_addr)
 *
 * @brief Set IP for softap
 *
 * @param ip_addr: IP address
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(NRC_WIFI_CONN_RET) is returned.
 ***********************************************/
int nrc_wifi_softap_set_ip(char *ip_addr);


/**********************************************
 * @fn int nrc_wifi_softap_set_conf(int index, char *ssid, int channel, int sec_mode, char *password)
 *
 * @brief Set configuration for softap
 *
 * @param index: network index
 *
 * @param ssid: SSID
 *
 * @param channel: 11ah channel
 *
 * @param sec_mode: security mode (Open or WPA2)
 *
 * @param password: PASSWORD
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(NRC_WIFI_CONN_RET) is returned.
 ***********************************************/
int nrc_wifi_softap_set_conf(int index, char *ssid, int channel, int sec_mode, char *password);


/**********************************************
 * @fn int nrc_wifi_softap_start(int index)
 *
 * @brief Start softap
 *
 * @param index: network index
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(NRC_WIFI_CONN_RET) is returned.
 ***********************************************/
int nrc_wifi_softap_start(int index);


/**********************************************
 * @fn int nrc_wifi_softap_start_dhcp_server(void)
 *
 * @brief Start DHCP Server
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(NRC_WIFI_CONN_RET) is returned.
 ***********************************************/
int nrc_wifi_softap_start_dhcp_server(void);


/**********************************************
 * @fn int nrc_wifi_set_tx_power(int tx_power)
 *
 * @brief Set TX Power
 *
 * @param tx_power: TX Power (in dBm) (8~18)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(NRC_WIFI_CONN_RET) is returned.
 ***********************************************/
int nrc_wifi_set_tx_power(int tx_power);


/**********************************************
 * @fn int nrc_wifi_get_tx_power(void)
 *
 * @brief Get TX Power
 *
 * @return TX Power (in dBm) (8~18)
 ***********************************************/
int nrc_wifi_get_tx_power(void);


/**********************************************
 * @fn void nrc_wifi_register_event_handler(event_callback_fn fn)
 *
 * @brief Regiest event handler for connection and dhcp
 *
 * @param fn: event handler for wifi connection and dhcp
 *
 * @return N/A
 ***********************************************/
void nrc_wifi_register_event_handler(event_callback_fn fn);


/**********************************************
 * @fn int8_t nrc_wifi_get_rssi(void)
 *
 * @brief Get RSSI
 *
 * @return RSSI (in dB)
 ***********************************************/
int8_t nrc_wifi_get_rssi(void);


/**********************************************
 * @fn int nrc_wifi_get_snr(void)
 *
 * @brief Get SNR
 *
 * @return SNR (in dB)
 ***********************************************/
uint32_t nrc_wifi_get_snr(void);


/**********************************************
 * @fn char* nrc_wifi_get_mac_address(void)
 *
 * @brief Get MAC address
 *
 * @return MAC address string("%02x:%02x:%02x:%02x:%02x:%02x")
 ***********************************************/
char* nrc_wifi_get_mac_address(void);


/**********************************************
 * @fn void nrc_wifi_set_rate_control(bool enable)
 *
 * @brief Configure the MCS rate control option
 *
 * @param enable: true or false
 *
 * @return N/A
 ***********************************************/
void nrc_wifi_set_rate_control(bool enable);


/**********************************************
 * @fn bool nrc_wifi_get_rate_control(void)
 *
 * @brief Get the MCS rate control option
 *
 * @return true or false
 ***********************************************/
bool nrc_wifi_get_rate_control(void);


/**********************************************
 * @fn void nrc_wifi_set_mcs(uint8_t mcs)
 *
 * @brief Configure the MCS index
 *
 * @param mcs: MCS index(0~7 or 10)
 *
 * @return N/A
 ***********************************************/
void nrc_wifi_set_mcs(uint8_t mcs);


/**********************************************
 * @fn uint8_t nrc_wifi_get_mcs(void)
 *
 * @brief Get the current MCS index
 *
 * @return MCS index(0~7 or 10)
 ***********************************************/
uint8_t nrc_wifi_get_mcs(void);


/**********************************************
 * @fn void nrc_wifi_set_network_index(int)
 *
 * @brief Set the current Network index
 *
 * @return N/A
 ***********************************************/
void nrc_wifi_set_network_index(int index);


/**********************************************
 * @fn int nrc_wifi_get_network_index(void)
 *
 * @brief Get the current Network index
 *
 * @return Current Network index
 ***********************************************/
int nrc_wifi_get_network_index(void);
#endif /* __NRC_API_WIFI_H__ */
