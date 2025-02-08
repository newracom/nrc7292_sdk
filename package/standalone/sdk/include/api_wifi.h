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

#ifndef __NRC_API_WIFI_H__
#define __NRC_API_WIFI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "nrc_types.h"
#include "nrc_sdk.h"

#define WIFI_TX_POWER_MIN            1		/* minimum value of tx power input range */
#define WIFI_TX_POWER_MAX            30		/* maximum value of tx power input range */

/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_get_device_mode (int vif_id, tWIFI_DEVICE_MODE *mode)
 *
 * @brief Get the current device mode
 *
 * @param vif_id: Network interface index
 *
 * @param mode: device mode (WIFI_MODE_STATION:0, WIFI_MODE_AP:1)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_device_mode (int vif_id, tWIFI_DEVICE_MODE *mode);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_get_mac_address (int vif_id, char *addr)
 *
 * @brief Get MAC address
 *
 * @param vif_id: Network interface index
 *
 * @param addr: MAC address output
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_mac_address (int vif_id, char *addr);


/**********************************************
 * @fn int nrc_wifi_get_tx_power (int vif_id, uint8_t *txpower)
 *
 * @brief Get the transmit power level (in dBm) after a successful connection.
 *
 * @param vif_id: Network interface index
 *
 * @param txpower: TX Power (in dBm)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_tx_power (int vif_id, uint8_t *txpower);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_tx_power (uint8_t txpower, uint8_t type)
 *
 * @brief Set the transmit power level (in dBm) and type
 *        It should be invoked following the configuration of the country code.
 *
 * @param txpower: TX Power (in dBm) (1~30)
 *
 * @param type: Auto(0), Limit(1), Fixed(2)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_tx_power (uint8_t txpower, uint8_t type);


/**********************************************
 * @fn  tWIFI_STATUS nrc_wifi_get_rssi(int vif_id, int8_t *rssi)
 *
 * @brief Get the RSSI (Received Signal Strength Indication) from the latest packet
 *
 * @param vif_id: Network interface index
 *
 * @param rssi: Pointer to a variable where the RSSI value will be stored.(in dB)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_rssi(int vif_id, int8_t *rssi);


/**********************************************
 * @fn  tWIFI_STATUS nrc_wifi_get_average_rssi(int vif_id, int8_t *average_rssi)
 *
 * @brief Get the average RSSI based on the latest 4 packets
 *
 * @param vif_id: Network interface index
 *
 * @param average_rssi: Pointer to a variable where the average RSSI value will be stored.
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_average_rssi(int vif_id, int8_t *average_rssi);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_get_snr(int vif_id, uint8_t* snr)
 *
 * @brief Get SNR
 *
 * @param vif_id: Network interface index
 *
 * @param snr: SNR value (in dB)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_snr(int vif_id, uint8_t *snr);


/**********************************************
 * @fn bool nrc_wifi_get_rate_control (int vif_id)
 *
 * @brief Get the MCS rate control option
 *
 * @param vif_id: Network interface index
 *
 * @return status - enable: 1(enable) or 0(disable)
 ***********************************************/
bool nrc_wifi_get_rate_control(int vif_id);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_rate_control (int vif_id, bool enable)
 *
 * @brief Configure the MCS rate control option
 *
 * @param vif_id: Network interface index
 *
 * @param enable: 1(enable) or 0(disable)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_rate_control (int vif_id, bool enable);


/**********************************************
 * @fn  tWIFI_STATUS nrc_wifi_get_mcs_info(int vif_id, uint8_t *tx_mcs, uint8_t *rx_mcs)
 *
 * @brief Retrieve the Modulation and Coding Scheme(MCS) information
 *
 * @param vif_id: Network interface index.
 *
 * @param tx_mcs: Pointer to store the MCS for transmission(TX).
 *                RC_OFF : manually configured MCS for transmission (TX)
 *                RC_ON  : the latest MCS set by Rate Control(RC) for transmission
 *                         and updated by tx data frames
 *
 * @param rx_mcs: Pointer to store the MCS for reception(RX).
 *                the latest MCS for reception(RX) and updated by rx data frames
 *
 * @return If successful, it returns WIFI_SUCCESS. Otherwise, an error code (tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_mcs_info(int vif_id, uint8_t *tx_mcs, uint8_t *rx_mcs);


/**********************************************
 * @fn  tWIFI_STATUS nrc_wifi_get_mcs(int vif_id, uint8_t *mcs)
 *
 * @brief Get MCS (RC_OFF : get manual MCS set by nrc_wifi_set_mcs, RC_ON : get latest MCS set by RC)
 *
 * @param vif_id: Network interface index
 *
 * @param mcs: Pointer to store the MCS for transmission (TX).
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_mcs(int vif_id, uint8_t *mcs);


/**********************************************
 * @fn  tWIFI_STATUS nrc_wifi_set_mcs(uint8_t mcs)
 *
 * @brief Set MCS, which is applied when rate control is disabled
 *
 * @param mcs: Modulation Coding Scheme (0~7, 10)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_mcs(uint8_t mcs);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_get_cca_threshold (int vif_id, int *cca_threshold)
 *
 * @brief Get CCA threshold
 *
 * @param vif_id: Network interface index
 *
 * @param cca_threshold: CCA threshold output.(time unit: dBm) (-100 ~ -35)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_cca_threshold (int vif_id, int *cca_threshold);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_cca_threshold (int vif_id, int cca_threshold)
 *
 * @brief Set CCA threshold
 *
 * @param vif_id: Network interface index
 *
 * @param cca_threshold: CCA threshold.(time unit: dBm) (-100 ~ -35)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_cca_threshold (int vif_id, int cca_threshold);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_tx_time (uint16_t cs_time, uint32_t pause_time)
 *
 * @brief Set carrier sense time and pause time
 *
 * @param cs_time: Carrier sensing time. Listen before talk (time unit: us) (0~12480)
 *
 * @param pause_time: Tx pause time (time unit : us)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_tx_time (uint16_t cs_time, uint32_t pause_time);


/**********************************************
 * @fn  tWIFI_STATUS nrc_wifi_enable_duty_cycle (uint32_t window, uint32_t duration, uint32_t margin)
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
tWIFI_STATUS nrc_wifi_enable_duty_cycle (uint32_t window, uint32_t duration, uint32_t margin);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_disable_duty_cycle (void)
 *
 * @brief Disable duty cycle
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_disable_duty_cycle (void);


/**********************************************
 * @fn bool nrc_wifi_tx_avaliable_duty_cycle(void)
 *
 * @brief check the tx is availabe in duty cycle
 *
 * @return If enabled, then true. Otherwise, false is returned.
 ***********************************************/
bool nrc_wifi_tx_avaliable_duty_cycle(void);


/**********************************************
 * @fn tWIFI_STATE_ID nrc_wifi_get_state (int vif_id)
 *
 * @brief Get state of Wi-Fi connection
 *
 * @param vif_id: Network interface index
 *
 * @return wifi status.
 ***********************************************/
 tWIFI_STATE_ID nrc_wifi_get_state (int vif_id);


/**********************************************
 * @fn void nrc_wifi_set_state (int vif_id, tWIFI_STATE_ID state)
 *
 * @brief Set state of Wi-Fi connection
 *
 * @param vif_id: Network interface index
 *
 * @param state: State of Wi-Fi
 *
 * @return N/A
 ***********************************************/
 void nrc_wifi_set_state (int vif_id, tWIFI_STATE_ID state);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_add_network (int vif_id)
 *
 * @brief Add network index for Wi-Fi connection
 *
 * @param vif_id: Network interface index
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_add_network (int vif_id);


/**********************************************
 * @fn  tWIFI_STATUS nrc_wifi_remove_network (int vif_id)
 *
 * @brief Remove network index for Wi-Fi connection
 *
 * @param vif_id: Network interface index
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_remove_network (int vif_id);


/**********************************************
 * @fn tWIFI_COUNTRY_CODE nrc_wifi_country_from_string (const char *str_cc)
 *
 * @brief Get country code index from string
 *
 * @param str_cc: country code {"US","JP","K1","TW","EU","CN","NZ","AU","K2"}
 *
 * @return tWIFI_COUNTRY_CODE, see nrc_types.h
 ***********************************************/
tWIFI_COUNTRY_CODE nrc_wifi_country_from_string (const char *str_cc);


/**********************************************
 * @fn const char *nrc_wifi_country_to_string (tWIFI_COUNTRY_CODE cc)
 *
 * @brief Get string from country code index
 *
 * @param cc: country code index (tWIFI_COUNTRY_CODE)
 *
 * @return If success, NULL terminated country code. Otherwise null.
 ***********************************************/
const char *nrc_wifi_country_to_string (tWIFI_COUNTRY_CODE cc);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_get_country (tWIFI_COUNTRY_CODE *cc)
 *
 * @brief Get country code
 *
 * @param cc: country code index
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_country (tWIFI_COUNTRY_CODE *cc);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_country (int vif_id, tWIFI_COUNTRY_CODE cc)
 *
 * @brief Set country code
 *
 * @param vif_id: Network interface index
 *
 * @param cc: country code index
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_country (int vif_id, tWIFI_COUNTRY_CODE cc);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_get_channel_bandwidth (int vif_id, uint8_t bandwidth)
 *
 * @brief Get channel bandwidth
 *
 * @param vif_id: Network interface index
 *
 * @param bandwidth: 1(1M BW) or 2(2M BW) or 4(4M BW)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_channel_bandwidth (int vif_id, uint8_t *bandwidth);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_get_channel_freq (int vif_id, uint16_t *s1g_freq)
 *
 * @brief Get frequency (Sub-1GHz)
 *
 * @param vif_id: Network interface index
 *
 * @param s1g_freq: S1G channel frequency (MHz/10)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_channel_freq (int vif_id, uint16_t *s1g_freq);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_channel_freq (int vif_id, uint16_t s1g_freq)
 *
 * @brief Set S1G channel
 *
 * @param vif_id: Network interface index
 *
 * @param s1g_freq: S1G channel frequency
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_channel_freq (int vif_id, uint16_t s1g_freq);

/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_channel_freq_bw (int vif_id, uint16_t s1g_freq, uint8_t bw)
 *
 * @brief Set S1G channel
 *
 * @param vif_id: Network interface index
 *
 * @param s1g_freq: S1G channel frequency
 *
 * @param bw: bandwidth (1,2,4)M
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_channel_freq_bw (int vif_id, uint16_t s1g_freq, uint8_t bw);

/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_ssid (int vif_id, char *ssid)
 *
 * @brief Set SSID of AP that STA wants to connect
 *
 * @param vif_id: Network interface index
 *
 * @param ssid: SSID
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_ssid (int vif_id, char *ssid);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_get_bssid (int vif_id, char *bssid)
 *
 * @brief Get the BSSID of the AP to establish connection
 *
 * @param vif_id: Network interface index
 *
 * @param bssid: BSSID
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_bssid (int vif_id, char **bssid);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_bssid (int vif_id, char *bssid)
 *
 * @brief Set BSSID of AP that STA wants to connect
 *
 * @param vif_id: Network interface index
 *
 * @param bssid: BSSID
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_bssid (int vif_id, char *bssid);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_softap_get_ignore_broadcast_ssid(int vif_id, int *ignore_broadcast_ssid)
 *
 * @brief Get ignore_broadcast_ssid - Hide SSID in AP mode
 *
 * @param vif_id: Network interface index
 *
 * @param ignore_broadcast_ssid: ignore broadcast SSID type
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_softap_get_ignore_broadcast_ssid(int vif_id, int *ignore_broadcast_ssid);

/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_softap_set_ignore_broadcast_ssid(int vif_id, int ignore_broadcast_ssid)
 *
 * @brief Set ignore_broadcast_ssid - Hide SSID in AP mode
 *
 * @param vif_id: Network interface index
 *
 * @param ignore_broadcast_ssid: ignore broadcast SSID type
 *   0: send full SSID in beacon
 *   1: send empty SSID (length=0) in beacon and ignore probe request for broadcast SSID
 *   2: send clear SSID (ASCII 0), but keep the original length and ignore probe reqeust for broadcast SSID
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_softap_set_ignore_broadcast_ssid(int vif_id, int ignore_broadcast_ssid);

/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_security (int vif_id, int mode, char *password)
 *
 * @brief Set security parameters for Wi-Fi connection
 *
 * @param vif_id: Network interface index
 *
 * @param mode: security mode
 *
 * @param password: PASSWORD
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_security (int vif_id, int mode, char *password);

/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_eap_security(int vif_id, int mode, int eap, char *identity, char *password, const char *ca_cert, const char *client_cert, const char *private_key, char *private_key_password)
 *
 * @brief Set EAP security parameters for Wi-Fi connection
 *
 * @param vif_id: Network interface index
 *
 * @param mode: security mode
 *
 * @param eap: eap type
 *
 * @param identity: identity
 *
 * @param password: password
 *
 * @param ca_cert: ca certificate(only TLS type)
 *
 * @param client_cert: client certificate(only TLS type)
 *
 * @param private_key: private key(only TLS type)
 *
 * @param private_key_password: private key password(only TLS type)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_eap_security(int vif_id, int mode, int eap, char *identity, char *password, const char *ca_cert, const char *client_cert, const char *private_key, char *private_key_password);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_pmk (int vif_id, char *pmk)
 *
 * @brief Set PMK(Pairwise Master Key) parameters for Wi-Fi connection
 *
 * @param vif_id: Network interface index
 *
 * @param pmk: Pairwise master key
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_pmk(int vif_id, char *pmk);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_sae_pwe (int vif_id, int sae_pwe)
 *
 * @brief Set SAE mechanism for PWE derivation
 *
 * @param vif_id: Network interface index
 *
 * @param sae_pwe: SAE mechanism for PWE derivation
 *                 0 = hunting-and-pecking loop only (default without password identifier)
 *                 1 = hash-to-element only (default with password identifier)
 *                 2 = both hunting-and-pecking loop and hash-to-element enabled
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_sae_pwe (int vif_id, int sae_pwe);

/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_get_sae_pwe (int vif_id, int *sae_pwe)
 *
 * @brief Get current SAE mechanism for PWE derivation
 *
 * @param vif_id: Network interface index
 *
 * @param sae_pwe: SAE mechanism for PWE derivation
 *                 0 = hunting-and-pecking loop only (default without password identifier)
 *                 1 = hash-to-element only (default with password identifier)
 *                 2 = both hunting-and-pecking loop and hash-to-element enabled
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_sae_pwe (int vif_id, int *sae_pwe);

/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_get_scan_freq (int vif_id, uint16_t *freq_list, uint8_t *num_freq)
 *
 * @brief Get scan list for scan AP
 *
 * @param vif_id: Network interface index
 *
 * @param freq_list: freq list to scan
 *
 * @param num_freq: num of freq

 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_scan_freq (int vif_id, uint16_t *freq, uint8_t *num_freq);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_scan_freq (int vif_id, uint16_t *freq_list, uint8_t num_freq)
 *
 * @brief Set scan list for scan AP
 *
 * @param vif_id: Network interface index
 *
 * @param freq_list: freq list to scan
 *
 * @param num_freq: num of freq

 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_scan_freq (int vif_id, uint16_t *freq_list, uint8_t num_freq);


/**********************************************
 * @fn  tWIFI_STATUS nrc_wifi_get_scan_freq_nons1g(int vif_id, uint16_t *freq_list, uint8_t *num_freq)
 *
 * @brief Get scan list for scan AP. It specifically focuses on setting frequencies for non-1g channels.
 *
 * @param vif_id: Network interface index
 *
 * @param freq_list: freq list to scan
 *
 * @param num_freq: num of freq

 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_scan_freq_nons1g(int vif_id, uint16_t *freq_list, uint8_t *num_freq);


/**********************************************
 * @fn  tWIFI_STATUS nrc_wifi_set_scan_freq_nons1g(int vif_id, uint16_t *freq_list, uint8_t num_freq)
 *
 * @brief Set scan list for scan AP. It specifically focuses on setting frequencies for non-1g channels.
 *
 * @param vif_id: Network interface index
 *
 * @param freq_list: freq list to scan
 *
 * @param num_freq: num of freq

 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_scan_freq_nons1g(int vif_id, uint16_t *freq_list, uint8_t num_freq);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_get_aid (int vif_id, int *aid)
 *
 * @brief Get association ID, allocated by AP
 *
 * @param vif_id: Network interface index
 *
 * @param aid: association id
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_aid (int vif_id, int *aid);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_scan (int vif_id)
 *
 * @brief Start scan procedure (sync: block until scan is done)
 *
 * @param vif_id: Network interface index
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_scan (int vif_id);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_scan_ssid (int vif_id, char *ssid)
 *
 * @brief Start scan procedure (sync: block until scan is done)
 *         It assigns one scan result slot specifically for the specified SSID
 *
 * @param vif_id: Network interface index
 *
 * @param ssid : SSID to scan for
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_scan_ssid (int vif_id, char *ssid);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_scan_timeout (int vif_id, uint32_t timeout, char *ssid)
 *
 * @brief Start scan procedure (async)
 *
 * @param vif_id: Network interface index
 *
 * @param timeout: blocking time in milliseconds
 *                 If timeout is 0, the caller will be blocked until the scan is done
 *
 * @param ssid: SSID to scan for. If NULL, scan for all SSID's.
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_scan_timeout (int vif_id, uint32_t timeout, char *ssid);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_scan_results (int vif_id, SCAN_RESULTS *results)
 *
 * @brief Get scan results
 *
 * @param vif_id: Network interface index
 *
 * @param results: scan list
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_scan_results (int vif_id, SCAN_RESULTS *results);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_abort_scan (int vif_id)
 *
 * @brief Stop scan procedure (async: unblock regardless of getting scan done)
 *
 * @param vif_id: Network interface index
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_abort_scan (int vif_id);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_connect (int vif_id, uint32_t timeout)
 *
 * @brief Start connection
 *
 * @param vif_id: Network interface index
 *
 * @param timeout: blocking time in milliseconds
 *                 If timeout is 0, the caller will be blocked until the connection is established.
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_connect(int vif_id, uint32_t timeout);


/**********************************************
 * @fn int nrc_wifi_disconnect (int vif_id, uint32_t timeout)
 *
 * @brief Start disconnection
 *
 * @param vif_id: Network interface index
 *
 * @param timeout: blocking time in milliseconds
 *                 If timeout is 0, the caller will be blocked until the disconnection is completed.
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_disconnect (int vif_id, uint32_t timeout);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_wps_pbc (int vif_id)
 *
 * @brief Set WPS Push button
 *
 * @param vif_id: Network interface index
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_wps_pbc(int vif_id);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_wps_cancel (int vif_id)
 *
 * @brief Cancels the pending WPS operaion
 *
 * @param vif_id: Network interface index
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_wps_cancel (int vif_id);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_softap_set_conf (int vif_id, char *ssid, uint16_t s1g_freq, uint8_t bw, tWIFI_SECURITY sec_mode, char *password, uint8_t sae_pwe )
 *
 * @brief Set configuration for softap
 *
 * @param vif_id: Network interface index
 *
 * @param ssid: SSID
 *
 * @param s1g_freq: 11ah channel frequency
 *
 * @param bw: specify the bandwidth for a wireless connection
 *              (0(BW is selected Automatically), 1(WIFI_1M), 2(WIFI_2M), 4(WIFI_4M))
 *
 * @param mode: security mode (tWIFI_SECURITY)
 *
 * @param password: PASSWORD
 *
 * @param sae_pwe:  sae pwe settings (0: hunting-and-pecking loop, 1: hash-to-element, 2:Both)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_softap_set_conf (int vif_id, char *ssid, uint16_t s1g_freq, uint8_t bw, tWIFI_SECURITY sec_mode, char *password, uint8_t sae_pwe);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_softap_set_bss_max_idle (int vif_id, int period, int retry_cnt);
 *
 * @brief Set BSS MAX IDLE period and retry count
 *
 * @param vif_id: Network interface index
 *
 * @param period: bss max idle period. (period value should be between 0 and 2,147,483,647)
 *
 * @param retry_cnt: retry count for receiving keep alive packet from STA. (retry_cnt value should be between 1 and 100)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_softap_set_bss_max_idle (int vif_id, int period, int retry_cnt);


/**********************************************
 * @fn  tWIFI_STATUS nrc_wifi_softap_get_max_num_sta(int vif_id, uint8_t *max_sta_num)
 *
 * @brief Get max station number to be connected
 *
 * @param vif_id: Network interface index
 *
 * @param max_sta_num: max station number
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_softap_get_max_num_sta(int vif_id, uint8_t *max_sta_num);


/**********************************************
 * @fn  tWIFI_STATUS nrc_wifi_softap_set_max_num_sta(int vif_id, uint8_t max_sta_num)
 *
 * @brief Set max station number to be connected
 *
 * @param vif_id: Network interface index
 *
 * @param max_sta_num: max station number. (Max 10)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_softap_set_max_num_sta(int vif_id, uint8_t max_sta_num);


/**********************************************
 * @fn  tWIFI_STATUS nrc_wifi_softap_set_ip (int vif_id, char* ipaddr, char* netmask, char* gateway)
 *
 * @brief Set IP for softap
 *
 * @param vif_id: Network interface index
 *
 * @param ip_addr: IP address
 *
 * @param netmask: netmask for static IP
 *
 * @param gateway: gateway address for static IP
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_softap_set_ip (int vif_id, char* ipaddr, char* netmask, char* gateway);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_softap_start (int vif_id)
 *
 * @brief Start softap (sync : block until the softap is started)
 *
 * @param vif_id: Network interface index
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_softap_start (int vif_id);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_softap_start_timeout (int vif_id, uint32_t timeout)
 *
 * @brief Start softap (async)
 *
 * @param vif_id: Network interface index
 *
 * @param timeout: blocking time in milliseconds
 *                 If timeout is 0, the caller will be blocked until the softap is started.
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_softap_start_timeout (int vif_id, uint32_t timeout);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_softap_stop (int vif_id)
 *
 * @brief Stop softap
 *
 * @param vif_id: Network interface index
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_softap_stop (int vif_id);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_softap_disassociate (int vif_id, char* mac_addr)
 *
 * @brief Disassociate all stations or a specific station equal to mac address.
 *
 * @param vif_id: Network interface index
 *
 * @param mac_addr: broadcast(ff:ff:ff:ff:ff:ff) or single sta's  MAC Address
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_softap_disassociate (int vif_id, char* mac_addr);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_softap_deauthenticate (int vif_id, char* mac_addr)
 *
 * @brief Deauthenticate all stations or a specific station equal to mac address.
 *
 * @param vif_id: Network interface index
 *
 * @param mac_addr: broadcast(ff:ff:ff:ff:ff:ff) or single sta's  MAC Address
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_softap_deauthenticate (int vif_id, char* mac_addr);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_softap_start_dhcp_server (int vif_id)
 *
 * @brief Start DHCP Server
 *
 * @param vif_id: Network interface index
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_softap_start_dhcp_server (int vif_id);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_softap_stop_dhcp_server (int vif_id)
 *
 * @brief Stop DHCP Server
 *
 * @param vif_id: Network interface index
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_softap_stop_dhcp_server (int vif_id);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_softap_get_sta_list(int vif_id, STA_LIST *info, uint16_t len)
 *
 * @brief get STAs' information (only for AP)
 *
 * @param vif_id: Network interface index
 *
 * @param info: STAs' information (state/addr/aid/SNR/RSSI)
 *
 * @param len: STA_LIST size
 *
 * @return If enabled, then WIFI_SUCCESS. Otherwise, WIFI_FAIL is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_softap_get_sta_list(int vif_id, STA_LIST *info, uint16_t len);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_softap_get_sta_by_addr(int vif_id, uint8_t *addr, STA_INFO *sta)
 *
 * @brief get STA's information using MAC addr (only for AP)
 *
 * @param vif_id: Network interface index
 *
 * @param addr: STA's MAC addr

 * @param sta: STAs' information (state/addr/aid/SNR/RSSI)
 *
 * @return If enabled, then WIFI_SUCCESS. Otherwise, WIFI_FAIL is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_softap_get_sta_by_addr(int vif_id, uint8_t *addr, STA_INFO *sta);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_softap_get_sta_num(int vif_id)
 *
 * @brief get number of STA associated (only for AP)
 *
 * @param vif_id: Network interface index
 *
 * @return number of STA associated
 ***********************************************/
uint16_t nrc_wifi_softap_get_sta_num(int vif_id);


/**********************************************
 * @fn    tWIFI_STATUS nrc_wifi_softap_get_beacon_interval(int vif_id, uint16_t *beacon_interval)
 *
 * @brief Get beacon interval for softAP(default: 100)
 *
 * @param vif_id: Network interface index
 *
 * @param beacon_interval: beacon interval(TU). (1TU=1024us) (range range 15..65535)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_softap_get_beacon_interval(int vif_id, uint16_t *beacon_interval);


/**********************************************
 * @fn     tWIFI_STATUS nrc_wifi_softap_set_beacon_interval(int vif_id, uint16_t beacon_interval)
 *
 * @brief Set beacon interval for softAP(default: 100)
 *
 * @param vif_id: Network interface index
 *
 * @param beacon_interval: beacon interval(TU). (1TU=1024us) (range range 15..65535)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_softap_set_beacon_interval(int vif_id, uint16_t beacon_interval);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_register_event_handler (int vif_id, event_callback_fn fn)
 *
 * @brief Register event handler for connection
 *
 * @param vif_id: Network interface index
 *
 * @param fn: Callback function for wifi connection
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_register_event_handler (int vif_id, event_callback_fn fn);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_unregister_event_handler (int vif_id, event_callback_fn fn)
 *
 * @brief Unregister event handler for connection
 *
 * @param vif_id: Network interface index
 *
 * @param fn: Callback function for wifi connection
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_unregister_event_handler (int vif_id, event_callback_fn fn);


/**********************************************
 * @fn tNET_ADDR_STATUStWIFI_STATUS nrc_addr_get_state (int vif_id)
 *
 * @brief Get IP address setting state
 *
 * @param vif_id: Network interface index
 *
 * @param state: Address setting state
 *
 * @return IP address setting state
 ***********************************************/
tNET_ADDR_STATUS nrc_addr_get_state (int vif_id);


/**********************************************
 * @fn  tWIFI_STATUS nrc_wifi_get_ip_mode (int vif_id, tWIFI_IP_MODE* mode)
 *
 * @brief Get IP address mode
 *
 * @param vif_id: Network interface index
 *
 * @param mode: WIFI_STATIC_IP or WIFI_DYNAMIC_IP
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_ip_mode (int vif_id, tWIFI_IP_MODE* mode);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_ip_mode (int vif_id, tWIFI_IP_MODE mode, char* ip_addr)
 *
 * @brief Enable DHCP cient or set static IP
 *
 * @param vif_id: Network interface index
 *
 * @param mode: WIFI_STATIC_IP or WIFI_DYNAMIC_IP
 *
 * @param ip_addr: IP address for static IP
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_ip_mode (int vif_id, tWIFI_IP_MODE mode, char* ip_addr);


/**********************************************
 * @fn  tWIFI_STATUS nrc_wifi_get_ip_address (int vif_id, char **ip_addr)
 *
 * @brief Get IP address. Start DHCP or set static IP (Sync: block until getting IP)
 *
 * @param vif_id: Network interface index
 *
 * @param ip_addr: ip address
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_ip_address (int vif_id, char **ip_addr);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_ip_address (int vif_id, tWIFI_IP_MODE mode, uint32_t timeout, char* ipaddr, char *netmask, char *gateway)
 *
 * @brief Set IP address
 *
 * @param vif_id: Network interface index
 *
 * @param mode: WIFI_STATIC_IP or WIFI_DYNAMIC_IP
 *
 * @param timeout : Wait timeout if WIFI_DYNAMIC_IP is selected. This value ignored for WIFI_STATIC_IP.
 *
 * @param ipaddr: IP address for static IP
 *
 * @param netmask: netmask for static IP
 *
 * @param gateway: gateway address for static IP
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_ip_address (int vif_id, tWIFI_IP_MODE mode, uint32_t timeout, char* ipaddr, char *netmask, char *gateway);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_stop_dhcp_client (int vif_id)
 *
 * @brief Stop the DHCP client
 *
 * @param vif_id: Network interface index
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_stop_dhcp_client(int vif_id);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_dns_server (char* pri_dns, char *sec_dns)
 *
 * @brief Set DNS server
 *
 * @param pri_dns: Primary DNS server
 *
 * @param sec_dns: Secondary DNS server
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_dns (char* pri_dns, char *sec_dns);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_add_etharp (int vif_id, const char *addr, char *bssid)
 *
 * @brief Add AP's ARP cache manually
 *
 * @param vif_id: Network interface index
 *
 * @param addr: IP address
 *
 * @param mac_addr: mac address
 *
 * @return If enabled, then WIFI_SUCCESS. Otherwise, WIFI_FAIL is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_add_etharp (int vif_id, const char *addr, char *mac_addr);


/**********************************************
 * @fn  tWIFI_STATUS nrc_wifi_send_addba(int vif_id, tWIFI_TID tid, char * mac_addr)
 *
 * @brief Send ADDBA action frame
 *
 * @param vif_id: Network interface index
 *
 * @param tid: traffic identifier(WIFI_TID_BE, WIFI_TID_BK, WIFI_TID_VI, WIFI_TID_VO) in tWIFI_TID
 *
 * @param mac_addr: mac address
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, WIFI_FAIL is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_send_addba(int vif_id, tWIFI_TID tid, char * mac_addr);


/**********************************************
 * @fn  tWIFI_STATUS nrc_wifi_send_addba(int vif_id, tWIFI_TID tid, char * mac_addr)
 *
 * @brief Send DELBA action frame
 *
 * @param vif_id: Network interface index
 *
 * @param tid: traffic identifier(WIFI_TID_BE, WIFI_TID_BK, WIFI_TID_VI, WIFI_TID_VO) in tWIFI_TID
 *
 * @param mac_addr: mac address
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, WIFI_FAIL is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_send_delba(int vif_id, tWIFI_TID tid, char * mac_addr);


/**********************************************
 * @fn  tWIFI_STATUS nrc_wifi_set_tx_aggr_auto(int vif_id, tWIFI_TID tid, uint8_t max_agg_num)
 *
 * @brief Enable automatic tx aggreation
 *
 * @param vif_id: Network interface index
 *
 * @param tid: traffic identifier(WIFI_TID_BE, WIFI_TID_BK, WIFI_TID_VI, WIFI_TID_VO) in tWIFI_TID
 *
 * @param max_agg_num: maximum number to aggregate frames
                       max_agg_num is up to 8 when channel bandwith is 1MHz.
                       max_agg_num is up to 16 when channel bandwith is 2MHz or 4MHz.
                       Even though you set 16 on 1MHz channel, it is set to 8.
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, WIFI_FAIL is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_tx_aggr_auto(int vif_id, tWIFI_TID tid, uint8_t max_agg_num);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_passive_scan(bool passive_scan_on)
 *
 * @brief Enable/Disable passive scan
 *
 * @param passive_scan_on 1(enable), 0(disable)
 *
 * @return If enabled, then WIFI_SUCCESS. Otherwise, WIFI_FAIL is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_passive_scan(bool passive_scan_on);

/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_simple_bgscan(int vif_id, uint16_t short_interval, int signal_threshold, uint16_t long_interval)
 *
 * @brief Set simple back ground scan
 *
 * @param vif_id: Network interface index
 *
 * @param short_interval: short scan interval (sec)
 *
 * @param signal_threshold: short/long interval choice signal threshold (db) (ex : -45)
 *
 * @param long_interval: long scan interval (sec)
 *
 * @return If enabled, then WIFI_SUCCESS. Otherwise, WIFI_FAIL is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_simple_bgscan(int vif_id, uint16_t short_interval, int signal_threshold, uint16_t long_interval);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_get_ap_info(int vif_id, AP_INFO *info)
 *
 * @brief get AP information (only for STA)
 *
 * @param vif_id: Network interface index
 *
 * @param info: AP's information  (bssid/ssid/ssid_len/Country_code/CH/FREQ/BW/Security)
 *
 * @return If enabled, then WIFI_SUCCESS. Otherwise, WIFI_FAIL is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_ap_info(int vif_id, AP_INFO *info);

/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_rf_power(bool power_on)
 *
 * @brief set RF power on/off
 *
 * @param power_on: True: turn on RF, False: turn off RF
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_rf_power(bool power_on);

/**********************************************
 * @fn  void nrc_wifi_set_use_4address(bool value)
 *
 * @brief sets whether to use four-address support.
 *
 * @param value A boolean value indicating whether to use four-address frames.
 *              Set to true to enable, false to disable.
 *
 * @return void
***********************************************/
void nrc_wifi_set_use_4address(bool value);

/**********************************************
 * @fn bool  nrc_wifi_get_use_4address(void)
 *
 * @brief Check whether four-address frames are enabled for Wi-Fi communication.
 *
 * This get the current setting for whether to use four-address frames support.
 *
 * @return A boolean value indicating whether four-address
 *         frames are enabled (true) or disabled (false).
 ***********************************************/
bool nrc_wifi_get_use_4address(void);

/**********************************************
 * @fn uint16_t nrc_get_hw_version
 *
 * @brief Get hw_version
 *
 * @param void
 *
 * @return hw_version: hw_version (uint16)
 ***********************************************/
uint16_t nrc_get_hw_version(void);

/**********************************************
 * @fn  tWIFI_STATUS nrc_wifi_get_gi(tWIFI_GI *gi)
 *
 * @brief Get guard interval type
 *
 * @param gi : Guard interval (Long GI:0, Short GI:1)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_gi(tWIFI_GI *gi);

/**********************************************
 * @fn  tWIFI_STATUS nrc_wifi_set_gi(tWIFI_GI gi)
 *
 * @brief Set guard interval type
 *
 * @param gi : Guard interval (Long GI:0, Short GI:1)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_gi(tWIFI_GI gi);


/**********************************************
 * @fn  tWIFI_STATUS nrc_wifi_set_beacon_loss_detection(int vif_id, bool enable,
 *                      uint8_t beacon_loss_thresh)
 *
 * @brief Sets the operation for beacon loss detection (only for STA).
 *          the default :  beacon loss detection(1), beacon loss thresh(30)
 *          30 * BI(100) * 1024us = about 3 sec
 *
 * @param vif_id: Network interface index
 *
 * @param enable: Specifies whether to enable (1) or disable (0) beacon loss detection.
 *
 * @param beacon_loss_thresh: disconnection threshold about beacon loss
 *
 * @return If enabled, then WIFI_SUCCESS. Otherwise, WIFI_FAIL is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_beacon_loss_detection(int vif_id, bool enable, uint8_t beacon_loss_thresh);


/**********************************************
 * @fn  tWIFI_STATUS nrc_wifi_get_listen_interval(int vif_id, uint16_t *listen_interval, uint32_t *interval_ms)
 *
 * @brief Get listen interval (only for STA)
 *
 * @param vif_id: Network interface index
 *
 * @param listen_interval: listen interval
 *                      Listen Interval Time (us) = listen_interval * beacon_interval * 1TU (1024 us)
 *
 * @param interval_ms : listen interval_ms
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_listen_interval(int vif_id, uint16_t *listen_interval, uint32_t *interval_ms);



/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_listen_interval(int vif_id, uint16_t listen_interval)
 *
 * @brief Set listen interval (only for STA)
 *
 * @param vif_id: Network interface index
 *
 * @param listen_interval: listen interval
 *                      Listen Interval Time (us) = listen_interval * beacon_interval * 1TU (1024 us)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_listen_interval(int vif_id, uint16_t listen_interval);

/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_get_mic_scan(bool *enable, bool *channel_move, uint32_t *cnt_detected)
 *
 * @brief Retrieve MIC scan settings and channel detection count (applicable only to K2(KR MIC))
 *
 * @param enable: Pointer to a variable to store the MIC scan enable setting.
 *
 * @param channel_move: Pointer to a variable to store the channel move setting when the current channel is invalid.
 *
 * @param cnt_detected: Pointer to a variable to store the channel detection count.
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_mic_scan(bool *enable, bool *channel_move, uint32_t *cnt_detected);


 /**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_mic_scan(bool enable, bool channel_move)
 *
 * @brief Configure MIC scan settings and channel detection count (applicable only to K2(KR MIC))
 *
 * @param enable:  MIC scan enable / disable (0|1)
 *
 * @param channel_move: channel move setting when the current channel is invalid.
 *                      This is used for access points (AP) mode.
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_mic_scan (bool enable, bool channel_move);


/**********************************************
 * @fn  tWIFI_STATUS nrc_wifi_set_enable_auth_control(int vif_id, bool enable)
 *
 * @brief enable DAC (Distributed Auth Control) (refer to 802.11ah spec (11.3.9.3))
 *
 * @param vif_id : vif_id
 *
 * @param enable : true(enable), false(disable)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_enable_auth_control(int vif_id, bool enable);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_get_enable_auth_control(uint8_t *enable)
 *
 * @brief get current DAC (Distributed Auth Control)
 *
 * @param vif_id : vif_id
 *
 * @param val : value (0(disable),1(enable))
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_enable_auth_control(int vif_id, uint8_t *val);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_auth_control_param(uint8_t slot, uint8_t ti_min, uint8_t ti_max)
 *
 * @brief set DAC (Distributed Auth Control) parameter
 *
 * @param slot : authentication slot (in units of TUs)
 *
 * @param ti_min : minimum transmission interval (in units of beacon intervals)
 *
 * @param ti_max : maximum transmission interval (in units of beacon intervals)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_auth_control_param(uint8_t slot, uint8_t ti_min, uint8_t ti_max);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_get_auth_control_param(uint8_t* slot, uint8_t* ti_min, uint8_t* ti_max)
 *
 * @brief get DAC (Distributed Auth Control) parameter
 *
 * @param slot : authentication slot (in units of TUs)
 *
 * @param ti_min : minimum transmission interval (in units of beacon intervals)
 *
 * @param ti_max : maximum transmission interval (in units of beacon intervals)
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_auth_control_param(uint8_t* slot, uint8_t* ti_min, uint8_t* ti_max);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_set_auth_control_scale(uint8_t scale_factor)
 *
 * @brief set scale factor (1: in units of BI for TI_MIN/TI_MAX or 10: in units of 10*BI for TI_MIN/TI_MAX)
 *
 * @param scale_factor value
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_set_auth_control_scale(uint8_t scale_factor);


/**********************************************
 * @fn STATUS nrc_wifi_get_auth_control_scale(uint8_t *scale_factor)
 *
 * @brief get scale factor (1: in units of BI for TI_MIN/TI_MAX or 10: in units of 10*BI for TI_MIN/TI_MAX)
 *
 * @param scale_factor value
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_auth_control_scale(uint8_t *scale_factor);


/**********************************************
 * @fn tWIFI_STATUS nrc_wifi_get_auth_current_ti(int *ti)
 *
 * @brief get current transmission interval(TI) value of DAC
 *
 * @param ti value
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 ***********************************************/
tWIFI_STATUS nrc_wifi_get_auth_current_ti(int *ti);

#ifdef __cplusplus
}
#endif

#endif /* __NRC_API_WIFI_H__ */
