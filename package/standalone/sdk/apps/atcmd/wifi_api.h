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

#ifndef __ATCMD_WIFI_API_H__
#define __ATCMD_WIFI_API_H__
/**********************************************************************************************/

enum TX_POWER_TYPE
{
	TX_POWER_AUTO = 0,
	TX_POWER_LIMIT,
	TX_POWER_FIXED
};

enum
{
	DHCP_SUCCESS = 0,
	DHCP_FAIL = -1,
	DHCP_BUSY = -2,
	DHCP_STOP = -3,
	DHCP_TIMEOUT = -4
};

typedef void (*wifi_event_cb_t) (int, void *, int);

extern int wifi_api_register_event_callback (wifi_event_cb_t event_cb[]);

extern int wifi_api_get_rf_cal (bool *cal_use, char *country);
extern int wifi_api_set_rf_cal (bool cal_use);

extern uint16_t wifi_api_get_s1g_freq (uint16_t non_s1g_freq);
extern int wifi_api_get_supported_channels (uint16_t s1g_freq[], int n_s1g_freq_max);

extern int wifi_api_get_macaddr (char *macaddr);

extern int wifi_api_get_country (char *country);
extern int wifi_api_set_country (char *country);

extern int wifi_api_get_tx_power (uint8_t *power);
extern int wifi_api_set_tx_power (uint8_t power, enum TX_POWER_TYPE type);

extern int wifi_api_get_rate_control (bool *enable);
extern void wifi_api_set_rate_control (bool enable);

extern int wifi_api_get_mcs (uint8_t *index);
extern int wifi_api_set_mcs (uint8_t index);

extern int wifi_api_get_snr (uint8_t *snr);
extern int wifi_api_get_rssi (int8_t *rssi);

extern int wifi_api_get_tsf (uint64_t *tsf);

extern int wifi_api_get_duty_cycle (uint32_t *window, uint32_t *duration, uint32_t *margin);
extern int wifi_api_set_duty_cycle (uint32_t window, uint32_t duration, uint32_t margin);

extern int wifi_api_get_cca_threshold (int *threshold);
extern int wifi_api_set_cca_threshold (int threshold);

extern int wifi_api_get_tx_time (uint16_t *cs_time, uint32_t *pause_time);
extern int wifi_api_set_tx_time (uint16_t cs_time, uint32_t pause_time);

extern int wifi_api_get_lbt (uint16_t *cs_duration, uint32_t *pause_time, uint32_t *tx_resume_time);
extern int wifi_api_set_lbt (uint16_t cs_duration, uint32_t pause_time, uint32_t tx_resume_time);

extern int wifi_api_get_mic_scan (bool *enable, bool *channel_move, uint32_t *cnt_detected);
extern int wifi_api_set_mic_scan (bool enable, bool channel_move);

extern int wifi_api_get_bmt (uint32_t *threshold);
extern int wifi_api_set_bmt (uint32_t threshold);

extern int wifi_api_set_ssid (char *ssid);
extern int wifi_api_set_bssid (char *bssid);
extern int wifi_api_set_security (char *security, char *password);

extern int wifi_api_add_network (void);
extern int wifi_api_remove_network (void);

extern int wifi_api_start_scan (uint32_t timeout);
extern int wifi_api_get_scan_results (SCAN_RESULTS *results);
extern int wifi_api_get_scan_freq (uint16_t freq[], uint8_t *n_freq);
extern int wifi_api_set_scan_freq (uint16_t freq[], uint8_t n_freq);

extern bool wifi_api_connecting (void);
extern bool wifi_api_connected (void);
extern bool wifi_api_disconnected (void);

extern int wifi_api_connect (uint32_t timeout);
extern int wifi_api_disconnect (uint32_t timeout);

extern int wifi_api_get_ap_ssid (char *str_ssid, int len);
extern int wifi_api_get_ap_bssid (char *str_bssid);

extern int wifi_api_get_ip_address (char *address, char *netmask, char *gateway);
extern int wifi_api_set_ip_address (char *address, char *netmask, char *gateway);

extern int wifi_api_start_dhcp_client (uint32_t timeout_msec);

extern int wifi_api_start_deep_sleep (uint32_t timeout, uint8_t gpio);
extern bool wifi_api_wakeup_done (void);

#ifdef CONFIG_ATCMD_SOFTAP
extern int wifi_api_start_softap (int freq, int bw, char *ssid, char *security, char *password, uint32_t timeout);
extern int wifi_api_stop_softap (void);

extern int wifi_api_get_max_sta_aid (void);
extern int wifi_api_get_sta_info (int aid, char *maddr, int8_t *rssi, uint8_t *snr, uint8_t *mcs);

extern int wifi_api_set_bss_max_idle (int period, int retry_cnt);

extern int wifi_api_start_dhcp_server (void);
extern int wifi_api_stop_dhcp_server (void);
#endif

/**********************************************************************************************/
#endif /* #ifndef __ATCMD_WIFI_API_H__ */

