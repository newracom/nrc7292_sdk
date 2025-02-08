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

#include "lwip/dhcp.h"

enum IF_MODE
{
	IF_MODE_APSTA = 0,
	IF_MODE_RELAY
};

enum TX_POWER_TYPE
{
	TX_POWER_NONE = -1,

	TX_POWER_AUTO = 0,
	TX_POWER_LIMIT,
	TX_POWER_FIXED,

	TX_POWER_TYPE_MAX
};

enum DHCP_RESULT
{
	DHCP_TIMEOUT = -4,
	DHCP_STOP = -3,
	DHCP_BUSY = -2,
	DHCP_FAIL = -1,
	DHCP_SUCCESS = 0,
	DHCP_RECOVERY = 1
};

enum WPS_STATUS
{
	WPS_TIMEOUT = -ETIMEDOUT,
	WPS_FAIL = -1,
	WPS_SUCCESS = 0,
	WPS_ACTIVE,
	WPS_DISABLE,
};

#define UMAC_COUNTRY_CODES	1

#if UMAC_COUNTRY_CODES == 0
typedef struct
{
	int cc_index;
	const char *alpha2_cc;
} wifi_country_t;
#else
typedef country_codes	wifi_country_t;
#endif

typedef struct
{
	uint32_t s1g_freq:14;
	uint32_t nons1g_freq:14;
	uint32_t bw:3;
	uint32_t scan:1;
} wifi_channel_t;

typedef struct
{
#define WIFI_CHANNEL_NUM_MAX	50

	int n_channel;
	wifi_channel_t channel[WIFI_CHANNEL_NUM_MAX];
} wifi_channels_t;

typedef struct
{
	uint8_t tx;
	uint8_t rx;
} wifi_mcs_t;

typedef struct
{
#define STR_WIFI_BSSID_LEN				17
#define STR_WIFI_SSID_LEN_MAX			32
#define STR_WIFI_SECURITY_LEN_MAX		8
#define STR_WIFI_PASSWORD_LEN_MAX		64

	struct
	{
		uint16_t bw;
		uint16_t freq;
	} channel;

	char bssid[STR_WIFI_BSSID_LEN + 1];
	char ssid[STR_WIFI_SSID_LEN_MAX + 1];
	char security[STR_WIFI_SECURITY_LEN_MAX + 1];
/*	char password[STR_WIFI_PASSWORD_LEN_MAX + 1]; */
} wifi_ap_info_t;

typedef void (*wifi_event_cb_t) (int, void *, int);
typedef void (*wifi_wps_cb_t) (enum WPS_STATUS, int, ...);

typedef void (*dhcpc_event_cb_t) (void);

/**********************************************************************************************/

extern int vif_id_ap;
extern int vif_id_sta;
extern int vif_id_max;

extern const char *str_txpwr_type[TX_POWER_TYPE_MAX];

extern const wifi_country_t *g_wifi_country_list;

extern int wifi_api_register_event_callback (wifi_event_cb_t event_cb[]);

extern int wifi_api_get_if_mode (void);
extern int wifi_api_set_if_mode (int mode);

extern struct netif *wifi_api_get_netif (void);

extern int16_t wifi_api_get_s1g_freq (uint16_t non_s1g_freq);
extern int wifi_api_get_supported_channels (const char *country, wifi_channels_t *channels);

#if defined(NRC7292)
extern int wifi_api_get_rf_cal (bool *cal_use, char *country);
#else
extern int wifi_api_get_rf_cal (bool *cal_use, char *country, int *id);
#endif

extern int wifi_api_get_macaddr (char *macaddr0, char *macaddr1);

extern bool wifi_api_valid_country (char *country);
extern int wifi_api_get_country (char *country);
extern int wifi_api_set_country (char *country);

extern int wifi_api_get_channel (uint16_t *freq, uint8_t *bw, bool ap);

extern int wifi_api_get_tx_power (uint8_t *power0, uint8_t *power1);
extern int wifi_api_set_tx_power (enum TX_POWER_TYPE type, uint8_t power);

extern int wifi_api_get_rssi (int8_t *rssi, bool average);
extern int wifi_api_get_snr (uint8_t *snr);

extern int wifi_api_get_rate_control (bool *rate_ctrl);
extern void wifi_api_set_rate_control (bool rate_ctrl);

extern int wifi_api_get_mcs (wifi_mcs_t *index0, wifi_mcs_t *index1);
extern int wifi_api_set_mcs (uint8_t index);

extern int wifi_api_get_duty_cycle (uint32_t *window, uint32_t *duration, uint32_t *margin);
extern int wifi_api_set_duty_cycle (uint32_t window, uint32_t duration, uint32_t margin);

extern int wifi_api_get_cca_threshold (int *threshold);
extern int wifi_api_set_cca_threshold (int threshold);

extern int wifi_api_get_tx_time (uint16_t *cs_time, uint32_t *pause_time);
extern int wifi_api_set_tx_time (uint16_t cs_time, uint32_t pause_time);

extern int wifi_api_get_tsf (uint64_t *tsf0, uint64_t *tsf1);

extern int wifi_api_add_network (bool ap);
extern int wifi_api_remove_network (bool ap);

extern bool wifi_api_connecting (void);
extern bool wifi_api_connected (void);
extern bool wifi_api_disconnected (void);

extern int wifi_api_start_scan (char *ssid, uint32_t timeout);
extern int wifi_api_get_scan_results (SCAN_RESULTS *results);
extern int wifi_api_get_scan_freq (uint16_t freq[], uint8_t *n_freq);
extern int wifi_api_set_scan_freq (uint16_t freq[], uint8_t n_freq);
extern int wifi_api_set_scan_background (int short_interval, int signal_threshold, int long_interval);

extern int wifi_api_get_beacon_interval (uint16_t *beacon_interval);

extern int wifi_api_get_listen_interval (uint16_t *listen_interval, uint32_t *listen_interval_tu);
extern int wifi_api_set_listen_interval (uint16_t listen_interval);

extern int wifi_api_set_ssid (char *ssid);
extern int wifi_api_set_bssid (char *bssid);
extern int wifi_api_set_security (char *security, char *password, int sae_pwe);

#if defined(CONFIG_ATCMD_SAEPWE) && defined(CONFIG_ATCMD_WPA3)
extern int wifi_api_get_sae_pwe (int *sae_pwe, bool ap);
extern int wifi_api_set_sae_pwe (int sae_pwe, bool ap);
#endif

extern int wifi_api_connect (uint32_t timeout);
extern int wifi_api_disconnect (uint32_t timeout);

extern int wifi_api_get_ap_info (wifi_ap_info_t *info);

extern int wifi_api_get_ip4_address (char *address, char *netmask, char *gateway);
extern int wifi_api_set_ip4_address (char *address, char *netmask, char *gateway);

extern int wifi_api_start_dhcp_client (uint32_t timeout_msec, dhcpc_event_cb_t event_cb[]);
extern void wifi_api_stop_dhcp_client (void);
extern int wifi_api_get_dhcp_lease_time (void);

extern int wifi_api_start_deep_sleep (uint32_t timeout, uint8_t gpio);
extern bool wifi_api_wakeup_done (void);

#ifdef CONFIG_ATCMD_SOFTAP
extern int wifi_api_start_softap (int bw, int freq,
								char *ssid, int ssid_type,
								char *security, char *password, int sae_pwe,
								uint32_t timeout);
extern int wifi_api_stop_softap (void);

extern int wifi_api_get_max_num_sta (uint8_t *max_num_sta);
extern int wifi_api_set_max_num_sta (uint8_t max_num_sta);

extern int wifi_api_get_sta_info (int aid, char *maddr, int8_t *rssi, uint8_t *snr,
								uint8_t *tx_mcs, uint8_t *rx_mcs);

extern int wifi_api_set_bss_max_idle (uint16_t period, uint8_t retry_cnt);

extern int wifi_api_start_dhcp_server (void);
extern int wifi_api_stop_dhcp_server (void);

extern bool wifi_api_is_relay_mode (void);
extern void wifi_api_get_relay_mode (bool *enable);
extern void wifi_api_set_relay_mode (bool enable);
#define wifi_api_enable_relay_mode()	wifi_api_set_relay_mode(true)
#define wifi_api_disable_relay_mode()	wifi_api_set_relay_mode(false)

extern void wifi_api_add_bridge (void);
extern void wifi_api_delete_bridge (void);

extern void wifi_api_get_4address (bool *enable);
extern void wifi_api_set_4address (bool enable);
#endif

extern int wifi_api_enable_wps (const char *bssid, wifi_wps_cb_t cb);
extern int wifi_api_disable_wps (void);

/**********************************************************************************************/
#endif /* #ifndef __ATCMD_WIFI_API_H__ */

