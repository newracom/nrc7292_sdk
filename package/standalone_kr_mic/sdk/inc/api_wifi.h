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

#ifndef __NRC_API_WIFI_H__
#define __NRC_API_WIFI_H__

#include "nrc_types.h"

#define DHCP_CLIENT_TIMEOUT_DEFAULT		(10000) /* msec */

#define WIFI_TX_POWER_MIN            1		/* minimum value of tx power input range */
#define WIFI_TX_POWER_MAX            30		/* maximum value of tx power input range */

/**********************************************
 * @fn  tWIFI_STATUS nrc_wifi_get_ip_mode(tWIFI_IP_MODE* mode)
 *
 * @brief Get IP address mode
 *
 * @param mode: WIFI_STATIC_IP or WIFI_DYNAMIC_IP
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_ip_mode(tWIFI_IP_MODE* mode);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_ip_mode(tWIFI_IP_MODE mode, char* ip_addr)
 *
 * @brief Enable DHCP cient or set static IP
 *
 * @param mode: WIFI_STATIC_IP or WIFI_DYNAMIC_IP
 *
 * @param ip_addr: IP address for static IP
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_ip_mode(tWIFI_IP_MODE mode, char* ip_addr);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_ip_address(void)
 *
 * @brief Set IP address
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_ip_address(void);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_add_network(int *index)
 *
 * @brief Add network index for Wi-Fi connection
 *
 * @param index: network index (>= 0)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_add_network(int *index);

/**********************************************
 * @fn  tWIFI_STATUS nrc_wifi_remove_network(int index)
 *
 * @brief Remove network index for Wi-Fi connection
 *
 * @param index: network index
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_remove_network(int index);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_get_country(tWIFI_COUNTRY_CODE *country_code)
 *
 * @brief Get country code
 *
 * @param country_code: country code value
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_country(tWIFI_COUNTRY_CODE *country_code);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_country(char *country_code)
 *
 * @brief Set country code
 *
 * @param country_code: country code value
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_country(tWIFI_COUNTRY_CODE country_code);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_ssid(int index, char *ssid)
 *
 * @brief Set SSID of AP that STA wants to connect
 *
 * @param index: network index
 *
 * @param ssid: SSID
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_ssid(int index, char *ssid);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_bssid(int index, char *bssid)
 *
 * @brief Set BSSID of AP that STA wants to connect
 *
 * @param index: network index
 *
 * @param bssid: BSSID
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_bssid(int index, char *bssid);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_scan_freq(int index, uint16_t *freq, uint8_t num_freq)
 *
 * @brief Set scan list for scan AP
 *
 * @param index: network index
 *
 * @param freq: freq list to scan
 *
 * @param num_freq: num of freq

 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_scan_freq(int index, uint16_t *freq, uint8_t num_freq);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_get_scan_freq(int index, uint16_t *freq, uint8_t *num_freq)
 *
 * @brief Set scan list for scan AP
 *
 * @param index: network index
 *
 * @param freq: freq list to scan
 *
 * @param num_freq: num of freq

 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_scan_freq(int index, uint16_t *freq, uint8_t *num_freq);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_get_bssid(char *bssid)
 *
 * @brief Get the BSSID of the AP to establish connection
 *
 * @param bssid: BSSID
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_bssid(char *bssid);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_security(int index, int mode, char *password)
 *
 * @brief Set security parameters for Wi-Fi connection
 *
 * @param index: network index
 *
 * @param mode: security mode
 *
 * @param password: PASSWORD
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_security(int index, int mode, char *password);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_get_enable_key(bool *enable)
 *
 * @brief Get information on whether security is active.
 *
 * @param enable: enable
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 * ***********************************************/
tWIFI_STATUS nrc_wifi_get_enable_key(bool *enable);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_scan(void)
 *
 * @brief Start scan procedure (sync: block until scan is done)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_scan(void);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_scan_results(SCAN_RESULTS *results)
 *
 * @brief Get scan results
 *
 * @param results: scan list
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_scan_results(SCAN_RESULTS *results);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_abort_scan(void)
 *
 * @brief Stop scan procedure (async: unblock regardless of getting scan done)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_abort_scan(void);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_disassociate(char* mac_addr)
 *
 * @brief Disassociate all stations or a specific station equal to mac address.
 *
 * @param results: broadcast(ff:ff:ff:ff:ff:ff) or single sta's  MAC Address
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_disassociate(char* mac_addr);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_connect(int index)
 *
 * @brief Start connection (sync: block until association is connected)
 *
 * @param index: network index
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_connect(int index);


/**********************************************
 * @fn int nrc_wifi_disconnect(int index)
 *
 * @brief Start disconnection (async: unblock regardless of getting disconnected)
 *
 * @param index: network index
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_disconnect(int index);


/**********************************************
 * @fn  tWIFI_STATUS nrc_wifi_get_ip_address(char** ip_addr)
 *
 * @brief Get IP address. Start DHCP or set static IP (Sync: block until getting IP)
 *
 * @param ip_addr: ip address
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_ip_address(char **ip_addr);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_state(tWIFI_STATE_ID state)
 *
 * @brief Set state of Wi-Fi connection
 *
 * @param state: Wi-Fi State
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_state(tWIFI_STATE_ID state);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_get_state(tWIFI_STATE_ID* state)
 *
 * @brief Get state of Wi-Fi connection
 *
 * @param state: Wi-Fi State
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_state(tWIFI_STATE_ID* state);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_tx_power(int txpower)
 *
 * @brief Set TX Power
 *
 * @param txpower: TX Power (in dBm) (8~18)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_tx_power(int txpower);


/**********************************************
 * @fn int nrc_wifi_get_tx_power(int *txpower)
 *
 * @brief Get TX Power
 *
 * @param txpower: TX Power (in dBm)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_tx_power(int *txpower);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_register_event_handler(event_callback_fn fn)
 *
 * @brief Regiest event handler for connection and DHCP
 *
 * @param fn: Callback function for wifi connection and DHCP
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_register_event_handler(event_callback_fn fn);


/**********************************************
 * @fn  tWIFI_STATUS nrc_wifi_get_rssi(int8_t* rssi)
 *
 * @brief Get RSSI
 *
 * @param rssi: RSSI value (in dB)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_rssi(int8_t *rssi);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_get_snr(uint8_t* snr)
 *
 * @brief Get SNR
 *
 * @param snr: SNR value (in dB)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_snr(uint8_t *snr);

/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_get_mac_address(char **addr)
 *
 * @brief Get MAC address
 *
 * @param addr: MAC address
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_mac_address(char **addr);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_get_device_mode(tWIFI_DEVICE_MODE *mode)
 *
 * @brief Get the current device mode
 *
 * @param mode: device mode (WIFI_MODE_STATION:0, WIFI_MODE_AP:1, WIFI_MODE_MESH_POINT:2)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_device_mode(tWIFI_DEVICE_MODE *mode);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_rate_control(bool enable)
 *
 * @brief Configure the MCS rate control option
 *
 * @param enable: 1(enable) or 0(disable)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_rate_control(bool enable);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_get_rate_control(bool *enable)
 *
 * @brief Get the MCS rate control option
 *
 * @param enable: 1(enable) or 0(disable)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_rate_control(bool *enable);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_get_aid(int *aid)
 *
 * @brief Get association ID, allocated by AP
 *
 * @param aid: association id
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_aid(int *aid);


/**********************************************
 * @fn int nrc_wifi_get_network_index(int *index)
 *
 * @brief Get the current network index
 *
 * @param index: network index
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_network_index(int *index);


/**********************************************
 * @fn tWIFI_COUNTRY_CODE nrc_wifi_country_from_string (const char *str)
 *
 * @brief Get country code index from string
 *
 * @param str: country code ("KR", "US", "JP", "TW", "EU", "CN", "NZ", "AU")
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_COUNTRY_CODE nrc_wifi_country_from_string(const char *str);


/**********************************************
 * @fn const char *nrc_wifi_country_to_string(tWIFI_COUNTRY_CODE cc)
 *
 * @brief Get string from country code index
 *
 * @param cc: country code index (tWIFI_COUNTRY_CODE)
 *
 * @return If success, then string. Otherwise null.
 ***********************************************/
const char *nrc_wifi_country_to_string(tWIFI_COUNTRY_CODE cc);


/**********************************************
 * @fn tWIFI_STATUS register_vendor_ie_handler(int cmd, vendor_ie_event_callback_fn func)
 *
 * @brief Register vendor ie handler
 *
 * @param cmd: command value (0xF0 ~ 0xF4)
 *
 * @param func: event callback function
 *
 * @return If success, then string. Otherwise null.
 ***********************************************/
tWIFI_STATUS register_vendor_ie_handler(int cmd, vendor_ie_event_callback_fn func);


/**********************************************
 * @fn tWIFI_STATUS unregister_vendor_ie_handler(int cmd)
 *
 * @brief Unregister vendor ie handler
 *
 * @param cmd: command value (0xF0 ~ 0xF4)
 *
 * @return If success, then string. Otherwise null.
 ***********************************************/
tWIFI_STATUS unregister_vendor_ie_handler(int cmd);

/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_s1g_config(uint8_t s1g_channel)
 *
 * @brief Set S1G channel
 *
 * @param s1g_channel: S1G channel
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_s1g_config(uint16_t s1g_channel);

/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_wps_pbc()
 *
 * @brief Set WPS Pushbutton
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_wps_pbc();


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_channel(uint32_t s1g_freq)
 *
 * @brief Set frequency (Sub-1GHz)
 *
 * @param s1g_freq: S1G channel frequency (MHz/10)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_channel(uint32_t s1g_freq);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_get_channel(uint32_t *s1g_freq)
 *
 * @brief Get frequency (Sub-1GHz)
 *
 * @param s1g_freq: S1G channel frequency (MHz/10)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_channel(uint32_t *s1g_freq);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_get_ch_bw(uint8_t bandwidth)
 *
 * @brief Get channel bandwidth
 *
 * @param bandwidth: 0(1M BW) or 1(2M BW) or 2(4M BW)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_ch_bw(uint8_t *bandwidth);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_softap_set_ip(char *ip_addr)
 *
 * @brief Set IP for softap
 *
 * @param ip_addr: IP address
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_softap_set_ip(char *ip_addr);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_softap_set_conf(int index, char *ssid, int channel, int sec_mode, char *password)
 *
 * @brief Set configuration for softap
 *
 * @param index: network index
 *
 * @param ssid: SSID
 *
 * @param channel: 11ah channel
 *
 * @param mode: security mode (tWIFI_SECURITY)
 *
 * @param password: PASSWORD
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_softap_set_conf(int index, char *ssid, int channel, int sec_mode, char *password);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_softap_start(int index)
 *
 * @brief Start softap
 *
 * @param index: network index
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_softap_start(int index);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_softap_start_dhcp_server(void)
 *
 * @brief Start DHCP Server
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_softap_start_dhcp_server(void);

/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_tx_time(uint16_t cs_time, uint32_t pause_time)
 *
 * @brief Set carrier sense time and pause time
 *
 * @param cs_time: Carrier sensing time. Listen before talk (time unit: us) (0~12480)
 *
 * @param pause_time: Tx pause time (time unit : us)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_tx_time(uint16_t cs_time, uint32_t pause_time);

/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_bss_max_idle(int index, int period, int retry_cnt);
 *
 * @brief Set BSS MAX IDLE period and retry count
 *
 * @param index: network index
 *
 * @param period: bss max idle period. (period value should be between 0 and 2,147,483,647)
 *
 * @param retry_cnt: retry count for receiving keep alive packet from STA. (retry_cnt value should be between 1 and 100)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_bss_max_idle(int index, int period, int retry_cnt);


/**********************************************
 * @fn  tWIFI_STATUS nrc_wifi_enable_duty_cycle(uint32_t window, uint32_t duration, uint32_t margin)
  *
 * @brief Enable duty cycle
 *
 * @param window: duty cycle window in usec.
 *
 * @param duration: specify allowed tx duration within duty cycle window in usec.
 *
 * @param margin: duty margin in usec.
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_enable_duty_cycle(uint32_t window, uint32_t duration, uint32_t margin);

/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_disable_duty_cycle(void)
 *
 * @brief Disable duty cycle
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_disable_duty_cycle(void);

/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_cca_threshold(int cca_threshold)
 *
 * @brief Set CCA threshold
 *
 * @param cca_threshold: CCA threshold.(unit: dBm) (-100 ~ -70)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_cca_threshold(int cca_threshold);

/**********************************************
 * @fn  tWIFI_STATUS nrc_wifi_set_mcs(uint8_t mcs)
 *
 * @brief Set MCS, which is applied when rate control is disabled
 *
 * @param MCS : Modulation Coding Scheme (0 ~ 10)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_mcs(uint8_t mcs);

#endif /* __NRC_API_WIFI_H__ */
