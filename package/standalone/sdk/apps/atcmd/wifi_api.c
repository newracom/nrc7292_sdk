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


#include "atcmd.h"
#include "umac_info.h"
#include "nrc_lwip.h"
#include "nrc_ps_type.h"

#include "lwip/dhcp.h"
#include "netif/bridgeif.h"
#include "driver_nrc.h"


int vif_id_ap = 0;
int vif_id_sta = 0;
int vif_id_br = 0;
int vif_id_max = 1;

#if UMAC_COUNTRY_CODES == 0
static const wifi_country_t _wifi_country_list[] =
{
	{ COUNTRY_CODE_AU, "AU" },
	{ COUNTRY_CODE_CN, "CN" },
	{ COUNTRY_CODE_EU, "EU" },
	{ COUNTRY_CODE_JP, "JP" },
	{ COUNTRY_CODE_NZ, "NZ" },
	{ COUNTRY_CODE_TW, "TW" },
	{ COUNTRY_CODE_US, "US" },
	{ COUNTRY_CODE_K1, "K1" },
	{ COUNTRY_CODE_K2, "K2" },

	{ COUNTRY_CODE_MAX, "00" }
};
const wifi_country_t *g_wifi_country_list = _wifi_country_list;
#else
const wifi_country_t *g_wifi_country_list = _country_codes;
#endif

const char *str_txpwr_type[TX_POWER_TYPE_MAX] =
{
	[TX_POWER_AUTO] = "auto",
	[TX_POWER_LIMIT] = "limit",
	[TX_POWER_FIXED] = "fixed",
};

const char *str_sec_mode[WIFI_SEC_MAX + 1] = { "open", "wpa2-psk", "wpa3-owe", "wpa3-sae", "unkown" };

/**********************************************************************************************/

extern bool lmac_send_qos_null_frame (bool pm);
extern int set_standalone_hook_dhcp(int vif_id);

extern tWIFI_STATUS nrc_wifi_get_scan_freq_nons1g (int vif_id, uint16_t *freq_list, uint8_t *num_freq);
extern tWIFI_STATUS nrc_wifi_set_scan_freq_nons1g (int vif_id, uint16_t *freq_list, uint8_t num_freq);

/**********************************************************************************************/

static wifi_event_cb_t g_wifi_event_cb[WIFI_EVT_MAX] =
{
	[WIFI_EVT_SCAN] = NULL,
	[WIFI_EVT_SCAN_DONE] = NULL,
	[WIFI_EVT_CONNECT_SUCCESS] = NULL,
	[WIFI_EVT_DISCONNECT] = NULL,
	[WIFI_EVT_AP_STARTED] = NULL,
	[WIFI_EVT_VENDOR_IE] = NULL,
	[WIFI_EVT_AP_STA_CONNECTED] = NULL,
	[WIFI_EVT_AP_STA_DISCONNECTED] = NULL,
};

static void _wifi_api_event_handler (int vif_id, tWIFI_EVENT_ID id, int data_len, void *data)
{
	const char *str_event[] =
	{
		[WIFI_EVT_SCAN] = "scan",
		[WIFI_EVT_SCAN_DONE] = "scan_done",
		[WIFI_EVT_CONNECT_SUCCESS] = "connect_success",
		[WIFI_EVT_DISCONNECT] = "disconnect",
		[WIFI_EVT_AP_STARTED] = "ap_started",
		[WIFI_EVT_VENDOR_IE] = "vendor_ie",
		[WIFI_EVT_AP_STA_CONNECTED] = "sta_connected",
		[WIFI_EVT_AP_STA_DISCONNECTED] = "sta_disconnected",
	};

	if (id < 0 || id >= WIFI_EVT_MAX)
		_atcmd_info("[%d] wifi_event: %d (invalid)", vif_id, id);
	else if (!g_wifi_event_cb[id])
		_atcmd_info("[%d] wifi_event: %d, %s (unused)", vif_id, id, str_event[id]);
	else
	{
		_atcmd_info("[%d] wifi_event: %s, data=%p,%d", vif_id, str_event[id], data, data_len);

		g_wifi_event_cb[id](vif_id, data, data_len);
	}
}

int wifi_api_register_event_callback (wifi_event_cb_t event_cb[])
{
	int i;

	if (!event_cb)
		return -EINVAL;

	for (i = 0 ; i < ATCMD_WIFI_EVT_MAX ; i++)
	{
		if (!event_cb[i])
			continue;

		switch (i)
		{
			case ATCMD_WIFI_EVT_SCAN_DONE:
				g_wifi_event_cb[WIFI_EVT_SCAN_DONE] = event_cb[i];
				break;

			case ATCMD_WIFI_EVT_CONNECT_SUCCESS:
				g_wifi_event_cb[WIFI_EVT_CONNECT_SUCCESS] = event_cb[i];
				break;

			case ATCMD_WIFI_EVT_DISCONNECT:
				g_wifi_event_cb[WIFI_EVT_DISCONNECT] = event_cb[i];
				break;

			case ATCMD_WIFI_EVT_STA_CONNECT:
				g_wifi_event_cb[WIFI_EVT_AP_STA_CONNECTED] = event_cb[i];
				break;

			case ATCMD_WIFI_EVT_STA_DISCONNECT:
				g_wifi_event_cb[WIFI_EVT_AP_STA_DISCONNECTED] = event_cb[i];
				break;
		}
	}

	if (nrc_wifi_register_event_handler(0, _wifi_api_event_handler) != WIFI_SUCCESS)
		return -1;

	if (nrc_wifi_register_event_handler(1, _wifi_api_event_handler) != WIFI_SUCCESS)
		return -1;

	return 0;
}

/**********************************************************************************************/

int wifi_api_get_if_mode (void)
{
	int mode;

	if (vif_id_ap == 0 && vif_id_sta == 0)
		mode = IF_MODE_APSTA;
	else if (vif_id_ap == 0 && vif_id_sta == 1)
		mode = IF_MODE_RELAY;
	else
	{
		_atcmd_error("invalid if_mode, ap=%d sta=%d", vif_id_ap, vif_id_sta);
		return -1;
	}

	return mode;
}

int wifi_api_set_if_mode (int mode)
{
	switch (mode)
	{
		case IF_MODE_APSTA:
			vif_id_max = 1;
			vif_id_ap = WLAN0_INTERFACE;
			vif_id_sta = WLAN0_INTERFACE;
			vif_id_br = WLAN0_INTERFACE;

			system_modem_api_set_mode(WLAN0_INTERFACE, MAC_STA_TYPE_STA);
			system_modem_api_set_mode(WLAN1_INTERFACE, MAC_STA_TYPE_MAX);
			break;

		case IF_MODE_RELAY:
			vif_id_max = 2;
			vif_id_ap = WLAN0_INTERFACE;
			vif_id_sta = WLAN1_INTERFACE;
			vif_id_br = BRIDGE_INTERFACE;

			system_modem_api_set_mode(vif_id_ap, MAC_STA_TYPE_AP);
			system_modem_api_set_mode(vif_id_sta, MAC_STA_TYPE_STA);
			break;

		default:
			_atcmd_error("invalid if_mode, mode=%d", mode);
			return -EINVAL;
	}

	return 0;
}

struct netif *wifi_api_get_netif (void)
{
	if (wifi_api_is_relay_mode())
		return nrc_netif_get_by_idx(BRIDGE_INTERFACE);
	else
		return nrc_netif_get_by_idx(WLAN0_INTERFACE);
}

int16_t wifi_api_get_s1g_freq (uint16_t non_s1g_freq)
{
	if (CheckSupportNonS1GFreq(non_s1g_freq))
	{
		const CHANNEL_MAPPING_TABLE *table;

		table = get_s1g_channel_item_by_nons1g_freq(non_s1g_freq);
		if (table)
			return table->s1g_freq;
	}

	return 0;
}

int wifi_api_get_supported_channels (const char *country, wifi_channels_t *channels)
{
	const CHANNEL_MAPPING_TABLE *channel_table;
	int bw;
	uint16_t s1g_freq;
	uint16_t nons1g_freq;
	int i, j;

	if (strlen(country) != 2 || !channels)
		return -1;

/*	_atcmd_debug("%s: %s", __func__, channels->country_code); */

	for (i = 0 ; g_wifi_country_list[i].cc_index < COUNTRY_CODE_MAX ; i++)
	{
		if (strcmp(country, g_wifi_country_list[i].alpha2_cc) == 0)
		{
			channel_table = get_channel_mapping_tables(g_wifi_country_list[i].cc_index);

			memset(channels, 0, sizeof(wifi_channels_t));

			for (j = 0 ; channel_table[j].s1g_freq != 0 ; j++)
			{
/*				_atcmd_debug("%s: %d %u %u %u", __func__, j,
							channel_table[j].chan_spacing,
							channel_table[j].s1g_freq,
							channel_table[j].nons1g_freq); */

				bw = channel_table[j].chan_spacing;
				s1g_freq = channel_table[j].s1g_freq;
				nons1g_freq = channel_table[j].nons1g_freq;

				switch (bw)
				{
					case BW_1M:
					case BW_2M:
					case BW_4M:
						if (strcmp(country, "K2") == 0)
						{
							switch (s1g_freq)
							{
								case 9245:
								case 9315:
									continue;

								case 9280:
								case 9300:
									bw = BW_2M;
									s1g_freq -= 10;
							}
						}

						channels->channel[channels->n_channel].bw = (1 << bw);
						channels->channel[channels->n_channel].scan = 1;
						channels->channel[channels->n_channel].s1g_freq = s1g_freq;
						channels->channel[channels->n_channel].nons1g_freq = nons1g_freq;
						channels->n_channel++;

/*						_atcmd_debug("%s: %d %u %u %u", __func__,
									channels->n_channel - 1,
									channels->channel[channels->n_channel - 1].bw,
									channels->channel[channels->n_channel - 1].scan,
									channels->channel[channels->n_channel - 1].s1g_freq,
									channels->channel[channels->n_channel - 1].nons1g_freq); */
				}
			}

			return 0;
		}
	}

	return -1;
}

#if defined(NRC7292)
int wifi_api_get_rf_cal (bool *cal_use, char *country)
{
	uint8_t cal_cc[2];
	uint8_t* rf_country = NULL;

	if (!cal_use || !country)
		return -1;

	rf_country = hal_rf_get_country();
	if (!rf_country)
		return -1;

	*cal_use = !!hal_get_rf_cal_use();
	memcpy(cal_cc, rf_country, 2);
	if(!memcmp (cal_cc, "KR", 2)){
		cal_cc[1] = hal_get_channel_type_kr() +'0';
	}
	sprintf(country, "%c%c", cal_cc[0], cal_cc[1]);
	return 0;
}
#else
#include "nrc_config.h"

int wifi_api_get_rf_cal (bool *cal_use, char *country, int *id)
{
	if (!cal_use || !country || !id)
		return -1;

	*cal_use = (RF_CAL_DATA.header.segment_valid) == 1 ? true : false;

	if (!*cal_use)
		*id = -1;
	else
	{
		*id = RF_CAL_DATA.header.id;

		memcpy(country, RF_CAL_DATA.header.country_code, 2);
		country[2] = '\0';
	}

	return 0;
}
#endif

int wifi_api_get_macaddr (char *macaddr0, char *macaddr1)
{
	int vif_id;
	char *macaddr;

/*	if (!macaddr0 || !macaddr1)
		return -EINVAL; */

	for (vif_id = 0 ; vif_id < 2 ; vif_id++)
	{
		macaddr = (vif_id == 0) ? macaddr0 : macaddr1;
		if (!macaddr)
			continue;

		strcpy(macaddr, "");

		if (nrc_wifi_get_mac_address(vif_id, macaddr) != WIFI_SUCCESS)
			return -1;

		if (strlen(macaddr) != ATCMD_WIFI_MACADDR_LEN)
			return -1;
	}

	return 0;
}

bool wifi_api_valid_country (char *country)
{
	if (strlen(country) == 2)
	{
		int i;

		for (i = 0 ; g_wifi_country_list[i].cc_index < COUNTRY_CODE_MAX ; i++)
		{
			if (strcmp(country, g_wifi_country_list[i].alpha2_cc) == 0)
				return true;
		}
	}

	return false;
}

int wifi_api_get_country (char *country)
{
	tWIFI_COUNTRY_CODE cc;
	const char *str_cc;

	if (!country)
		return -EINVAL;

	if (nrc_wifi_get_country(&cc) != WIFI_SUCCESS)
		return -1;

	str_cc = nrc_wifi_country_to_string(cc);
	if (!str_cc)
		return -1;

	strcpy(country, str_cc);

	return 0;
}

int wifi_api_set_country (char *country)
{
	tWIFI_COUNTRY_CODE cc;

	if (!country)
		return -EINVAL;

	cc = nrc_wifi_country_from_string(country);
	if (cc == WIFI_CC_UNKNOWN)
		return -1;

	if (nrc_wifi_set_country(0, cc) != WIFI_SUCCESS)
		return -1;

	if (nrc_wifi_set_country(1, cc) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_get_channel (uint16_t *freq, uint8_t *bw, bool ap)
{
	int vif_id = ap ? vif_id_ap : vif_id_sta;

	if (nrc_wifi_get_channel_freq(vif_id, freq) != WIFI_SUCCESS)
		return -1;

	if (nrc_wifi_get_channel_bandwidth(vif_id, bw) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_get_tx_power (uint8_t *power0, uint8_t *power1)
{
	int vif_id;
	uint8_t *power;

	if (!power0)
		return -EINVAL;

	if (!wifi_api_is_relay_mode())
		power1 = NULL;
	else if (!power1)
		return -EINVAL;

	for (vif_id = 0 ; vif_id < vif_id_max ; vif_id++)
	{
		power = (vif_id == 0) ? power0 : power1;
		if (!power)
			continue;

		if (nrc_wifi_get_tx_power(vif_id, power) != WIFI_SUCCESS)
			return -1;
	}

	return 0;
}

int wifi_api_set_tx_power (enum TX_POWER_TYPE type, uint8_t power)
{
	tWIFI_TXPOWER_TYPE _type;

	switch (type)
	{
		case TX_POWER_AUTO:
			_type = WIFI_TXPOWER_AUTO;
			break;

		case TX_POWER_LIMIT:
			_type = WIFI_TXPOWER_LIMIT;
			break;

		case TX_POWER_FIXED:
			_type = WIFI_TXPOWER_FIXED;
			break;

		default:
			return -EINVAL;
	}

	if (nrc_wifi_set_tx_power(power, _type) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_get_rssi (int8_t *rssi, bool average)
{
	tWIFI_STATUS (*get_rssi[2])(int, int8_t *) =
	{
		nrc_wifi_get_rssi,
		nrc_wifi_get_average_rssi
	};
	int vif_id;

	if (!rssi)
		return -EINVAL;

	*rssi = ATCMD_WIFI_RSSI_MIN;

	if (get_rssi[average ? 1 : 0](vif_id_sta, rssi) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_get_snr (uint8_t *snr)
{
	int vif_id;

	if (!snr)
		return -EINVAL;

	if (nrc_wifi_get_snr(vif_id_sta, snr) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_get_rate_control (bool *rate_ctrl)
{
	bool rate_ctrl_0;
	bool rate_ctrl_1;

	if (!rate_ctrl)
		return -EINVAL;

	rate_ctrl_0 = system_modem_api_get_rate_control(0);

	if (wifi_api_is_relay_mode())
	{
		rate_ctrl_1 = system_modem_api_get_rate_control(1);

		if (rate_ctrl_0 != rate_ctrl_1)
			wifi_api_set_rate_control(rate_ctrl_0);
	}

	*rate_ctrl = rate_ctrl_0;

	return 0;
}

void wifi_api_set_rate_control (bool rate_ctrl)
{
	system_modem_api_set_rate_control(0, rate_ctrl);
	system_modem_api_set_rate_control(1, rate_ctrl);
}

int wifi_api_get_mcs (wifi_mcs_t *index0, wifi_mcs_t *index1)
{
	int vif_id;
	wifi_mcs_t *index;

	if (!index0)
		return -EINVAL;

	if (!wifi_api_is_relay_mode())
	   index1 = NULL;
	else if (!index1)
		return -EINVAL;

	for (vif_id = 0 ; vif_id < vif_id_max ; vif_id++)
	{
		index = (vif_id == 0) ? index0 : index1;
		if (!index)
			continue;

		if (nrc_wifi_get_mcs_info(vif_id, &index->tx, &index->rx) != WIFI_SUCCESS)
			return -1;
	}

	return 0;
}

int wifi_api_set_mcs (uint8_t index)
{
	if (!system_modem_api_set_mcs(index))
		return -1;

	return 0;
}

int wifi_api_get_duty_cycle (uint32_t *window, uint32_t *duration, uint32_t *margin)
{
	if (!window || !duration || !margin)
		return -EINVAL;

	if (system_modem_api_get_duty_cycle(window, duration, margin))
		return 0;

	return -1;
}

int wifi_api_set_duty_cycle (uint32_t window, uint32_t duration, uint32_t margin)
{
	if (window > 0)
	{
		if (system_modem_api_enable_duty_cycle(window, duration, margin))
			return 0;
	}
	else if (system_modem_api_disable_duty_cycle())
		return 0;

	return -1;
}

int wifi_api_get_cca_threshold (int *threshold)
{
	int vif_id;
	int threshold0, threshold1;

	if (!threshold)
		return -EINVAL;

	threshold0 = system_modem_api_get_cca_threshold(0);

	if (wifi_api_is_relay_mode())
	{
		threshold1 = system_modem_api_get_cca_threshold(0);

		if (threshold0 != threshold1)
			system_modem_api_set_cca_threshold(1, threshold0);
	}

	*threshold = threshold0;

	return 0;
}

int wifi_api_set_cca_threshold (int threshold)
{
	if (!system_modem_api_set_cca_threshold(0, threshold))
		return -1;

	if (!system_modem_api_set_cca_threshold(1, threshold))
		return -1;

	return 0;
}

int wifi_api_get_tx_time (uint16_t *cs_time, uint32_t *pause_time)
{
	if (!cs_time || !pause_time)
		return -EINVAL;

	*cs_time = system_modem_api_get_cs_time();
	*pause_time = system_modem_api_get_tx_pause_time();

	return 0;
}

int wifi_api_set_tx_time (uint16_t cs_time, uint32_t pause_time)
{
	if (cs_time > 13260)
		return -EINVAL;

	system_modem_api_set_cs_time(cs_time);
	system_modem_api_set_tx_pause_time(pause_time);

	return 0;
}

int wifi_api_get_tsf (uint64_t *tsf0, uint64_t *tsf1)
{
	int vif_id;
	uint64_t *tsf;
	uint32_t *low;
	uint32_t *high;

	if (!tsf0)
		return -EINVAL;

	if (!wifi_api_is_relay_mode())
	   tsf1 = NULL;
	else if (!tsf1)
		return -EINVAL;

	for (vif_id = 0 ; vif_id < vif_id_max ; vif_id++)
	{
		tsf = (vif_id == 0) ? tsf0 : tsf1;
		if (!tsf)
			continue;

		low = (uint32_t *)tsf;
		high = (uint32_t *)tsf + 1;

		*high = lmac_get_tsf_h(vif_id);
		*low = lmac_get_tsf_l(vif_id);
	}

	return 0;
}

/**********************************************************************************************/

int wifi_api_add_network (bool ap)
{
	int vif_id = ap ? vif_id_ap : vif_id_sta;

	if (nrc_wifi_add_network(vif_id) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_remove_network (bool ap)
{
	int vif_id = ap ? vif_id_ap : vif_id_sta;

	if (nrc_wifi_remove_network(vif_id) != WIFI_SUCCESS)
		return -1;

	return 0;
}

bool wifi_api_connecting (void)
{
	if (nrc_wifi_get_state(vif_id_sta) == WIFI_STATE_TRY_CONNECT)
		return true;

	return false;
}

bool wifi_api_connected (void)
{
	if (nrc_wifi_get_state(vif_id_sta) == WIFI_STATE_CONNECTED)
		return true;

	return false;
}

bool wifi_api_disconnected (void)
{
	if (nrc_wifi_get_state(vif_id_sta) == WIFI_STATE_DISCONNECTED)
		return true;

	return false;
}

int wifi_api_get_beacon_interval (uint16_t *beacon_interval)
{
	uint16_t listen_interval;
	uint32_t listen_interval_tu;

	if (!beacon_interval)
		return -EINVAL;

	if (nrc_wifi_get_listen_interval(vif_id_sta, &listen_interval, &listen_interval_tu) != WIFI_SUCCESS)
		return -1;

	*beacon_interval = listen_interval_tu / listen_interval;

	return 0;
}

int wifi_api_get_listen_interval (uint16_t *listen_interval, uint32_t *listen_interval_tu)
{
	if (!listen_interval || !listen_interval_tu)
		return -EINVAL;

	if (nrc_wifi_get_listen_interval(vif_id_sta, listen_interval, listen_interval_tu) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_set_listen_interval (uint16_t listen_interval)
{
	if (nrc_wifi_set_listen_interval(vif_id_sta, listen_interval) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_start_scan (char *ssid, uint32_t timeout)
{
	if (nrc_wifi_scan_timeout(vif_id_sta, timeout, ssid) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_get_scan_results (SCAN_RESULTS *results)
{
	if (nrc_wifi_scan_results(vif_id_sta, results) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_get_scan_freq (uint16_t freq[], uint8_t *n_freq)
{
	if (!freq || !n_freq)
		return -EINVAL;

	if (nrc_wifi_get_scan_freq_nons1g(vif_id_sta, freq, n_freq) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_set_scan_freq (uint16_t freq[], uint8_t n_freq)
{
	if ((!freq && n_freq > 0) || (freq && n_freq == 0))
		return -EINVAL;

	if (nrc_wifi_set_scan_freq_nons1g(vif_id_sta, freq, n_freq) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_set_scan_background (int short_interval, int signal_threshold, int long_interval)
{
#if defined(CONFIG_ATCMD_BGSCAN)
	if (nrc_wifi_set_simple_bgscan(vif_id_sta, short_interval, signal_threshold, long_interval) != WIFI_SUCCESS)
		return -1;

	return 0;
#else
	return -1;
#endif
}

int wifi_api_set_ssid (char *ssid)
{
	if (!ssid)
		return -EINVAL;

	if (nrc_wifi_set_ssid(vif_id_sta, ssid) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_set_bssid (char *bssid)
{
	if (!bssid)
		return -EINVAL;

	if (nrc_wifi_set_bssid(vif_id_sta, bssid) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_set_security (char *security, char *password, int sae_pwe)
{
	tWIFI_SECURITY sec_mode;

	if (!security || !password)
		return -EINVAL;

	if (strcmp(security, "open") == 0)
		sec_mode = WIFI_SEC_OPEN;
	else if (strcmp(security, "wpa2-psk") == 0)
		sec_mode = WIFI_SEC_WPA2;
#if defined(CONFIG_ATCMD_WPA3)
	else if (strcmp(security, "wpa3-owe") == 0)
		sec_mode = WIFI_SEC_WPA3_OWE;
	else if (strcmp(security, "wpa3-sae") == 0)
	{
		sec_mode = WIFI_SEC_WPA3_SAE;

#if defined(CONFIG_ATCMD_SAEPWE)
		if (sae_pwe < 0 || sae_pwe > 2)
			return -EINVAL;

		if (nrc_wifi_set_sae_pwe(vif_id_sta, sae_pwe) != WIFI_SUCCESS)
			return -1;
#endif
	}
#endif
	else
		return -1;

	if (nrc_wifi_set_security(vif_id_sta, sec_mode, password) != WIFI_SUCCESS)
		return -1;

	return 0;
}

#if defined(CONFIG_ATCMD_SAEPWE) && defined(CONFIG_ATCMD_WPA3)
int wifi_api_get_sae_pwe (int *sae_pwe, bool ap)
{
	int vif_id = ap ? vif_id_ap : vif_id_sta;

	if (!sae_pwe)
		return -EINVAL;

	*sae_pwe = -1;

	if (nrc_wifi_get_sae_pwe(vif_id, sae_pwe) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_set_sae_pwe (int sae_pwe, bool ap)
{
	int vif_id = ap ? vif_id_ap : vif_id_sta;

	if (sae_pwe < 0 || sae_pwe > 2)
		return -EINVAL;

	if (nrc_wifi_set_sae_pwe(vif_id, sae_pwe) != WIFI_SUCCESS)
		return -1;

	return 0;
}
#endif

int wifi_api_connect (uint32_t timeout)
{
	switch (nrc_wifi_connect(vif_id_sta, timeout))
	{
		case WIFI_SUCCESS:
			return 0;

		case WIFI_TIMEOUT:
			return 1;

		default:
			return -1;
	}
}

int wifi_api_disconnect (uint32_t timeout)
{
	switch (nrc_wifi_disconnect(vif_id_sta, timeout))
	{
		case WIFI_SUCCESS:
			return 0;

		case WIFI_TIMEOUT:
			return 1;

		default:
			return -1;
	}
}

int wifi_api_get_ap_info (wifi_ap_info_t *info)
{
	AP_INFO ap;

	if (!info)
		return -EINVAL;

	memset(info, 0, sizeof(wifi_ap_info_t));

	if (nrc_wifi_get_ap_info(vif_id_sta, &ap) != WIFI_SUCCESS)
		return -1;

	info->channel.bw = ap.bw;
	info->channel.freq = ap.freq;

	sprintf(info->bssid, "%02X:%02X:%02X:%02X:%02X:%02X",
				ap.bssid[0], ap.bssid[1], ap.bssid[2],
				ap.bssid[3], ap.bssid[4],ap.bssid[5]);

	if (ap.ssid_len > 0)
		memcpy(info->ssid, ap.ssid, ap.ssid_len);

	strcpy(info->security, str_sec_mode[ap.security]);
/*	strcpy(info->password, ap.password); */

	return 0;
}

int wifi_api_get_ip4_address (char *address, char *netmask, char *gateway)
{
	struct netif *netif;

	if (!address || !netmask || !gateway)
		return -EINVAL;

	if (wifi_api_is_relay_mode())
		netif = &br_netif;
	else
		netif = nrc_netif[vif_id_sta];

	strcpy(address, ipaddr_ntoa(netif_ip_addr4(netif)));
	strcpy(netmask, ipaddr_ntoa(netif_ip_netmask4(netif)));
	strcpy(gateway, ipaddr_ntoa(netif_ip_gw4(netif)));

	return 0;
}

int wifi_api_set_ip4_address (char *address, char *netmask, char *gateway)
{
	struct netif *netif;
	ip_addr_t ipaddr[3];

	if (!address || !netmask || !gateway)
		return -EINVAL;

	if (!ipaddr_aton(address, &ipaddr[0]))
		return -1;

	if (!ipaddr_aton(netmask, &ipaddr[1]))
		return -1;

	if (!ipaddr_aton(gateway, &ipaddr[2]))
		return -1;

	if (wifi_api_is_relay_mode())
		netif = &br_netif;
	else
		netif = nrc_netif[vif_id_sta];

	netif_set_netmask(netif, ip_2_ip4(&ipaddr[1]));
	netif_set_gw(netif, ip_2_ip4(&ipaddr[2]));
	netif_set_ipaddr(netif, ip_2_ip4(&ipaddr[0]));

	system_modem_api_set_static_ip(vif_id_sta);

	return 0;
}

static dhcpc_event_cb_t g_dhcpc_event_cb[] =
{
	[DHCP_EVENT_RENEWING] = NULL,
	[DHCP_EVENT_RELEASED] = NULL,
	[DHCP_EVENT_BOUND] = NULL,
};
static bool g_dhcpc_renewing = false;

static void _wifi_api_dhcp_client_event_handler (struct netif *netif, int event)
{
	switch (event)
	{
		case DHCP_EVENT_RENEWING:
			_atcmd_info("DHCP_EVENT_RENEWING");
			g_dhcpc_renewing = true;
			break;

		case DHCP_EVENT_RELEASED:
			_atcmd_info("DHCP_EVENT_RELEASED");
			break;

		case DHCP_EVENT_BOUND:
			_atcmd_info("DHCP_EVENT_BOUND");
			if (!g_dhcpc_renewing)
				return;
			g_dhcpc_renewing = false;
			break;

		default:
			_atcmd_error("invalid dhcp event (%d)", event);
			return;
	}

#if 0
	{
		atcmd_wifi_ipaddr_t ipaddr, netmask, gateway;
		int lease_time;

		wifi_api_get_ip4_address(ipaddr, netmask, gateway);
		lease_time = wifi_api_get_dhcp_lease_time();

		_atcmd_info(" - ipaddr : %s", ipaddr);
		_atcmd_info(" - netmask : %s", netmask);
		_atcmd_info(" - gateway :%s", gateway);
		_atcmd_info(" - lease_time : %d", lease_time);
	}
#endif

	if (g_dhcpc_event_cb[event])
		g_dhcpc_event_cb[event]();
}

int wifi_api_start_dhcp_client (uint32_t timeout_msec, dhcpc_event_cb_t event_cb[])
{
	char *str_ip4addr_any = "0.0.0.0";
	int vif_id = wifi_api_is_relay_mode() ? vif_id_br : vif_id_sta;
	int ret;
	int i;

#ifndef CONFIG_ATCMD_FAST_RECOVERY
	if (set_standalone_hook_dhcp(vif_id) == 0)
		return DHCP_RECOVERY;
#endif

	wifi_dhcpc_stop(vif_id);

	if (wifi_api_set_ip4_address(str_ip4addr_any, str_ip4addr_any, str_ip4addr_any) != 0)
		_atcmd_error("failed to reset ip address");

	if (wifi_dhcpc_start_with_event(vif_id, _wifi_api_dhcp_client_event_handler) != 0)
		return DHCP_FAIL;

	for (i = 0 ; i < timeout_msec ; i += 10)
	{
		if (get_dhcp_status(vif_id))
		{
			if (event_cb)
			{
				g_dhcpc_renewing = false;

				for (i = 0 ; i < ATCMD_DHCPC_EVT_MAX ; i++)
				{
					if (!event_cb[i])
						continue;

					switch (i)
					{
						case ATCMD_DHCPC_EVT_RELEASE:
							g_dhcpc_event_cb[DHCP_EVENT_RELEASED] = event_cb[i];
							break;

						case ATCMD_DHCPC_EVT_RENEW:
							g_dhcpc_event_cb[DHCP_EVENT_BOUND] = event_cb[i];
							break;
					}
				}
			}

			return DHCP_SUCCESS;
		}

		if (!wifi_dhcpc_status(vif_id))
			return DHCP_STOP;

		_delay_ms(10);
	}

	wifi_dhcpc_stop(vif_id);

	return DHCP_TIMEOUT;
}

void wifi_api_stop_dhcp_client (void)
{
	int vif_id = wifi_api_is_relay_mode() ? vif_id_br : vif_id_sta;
	bool dhcpc_started = !!wifi_dhcpc_status(vif_id);

	if (dhcpc_started)
		wifi_dhcpc_stop(vif_id);

	_atcmd_debug("%s: size=%d", __func__, sizeof(g_dhcpc_event_cb), sizeof(*g_dhcpc_event_cb));

	memset(g_dhcpc_event_cb, 0, sizeof(g_dhcpc_event_cb));
	g_dhcpc_renewing = false;
}

int wifi_api_get_dhcp_lease_time (void)
{
	int vif_id = wifi_api_is_relay_mode() ? vif_id_br : vif_id_sta;
	int lease_time;

	lease_time = wifi_dhcpc_get_lease_time(vif_id);
	if (lease_time < 0)
	{
		_atcmd_error("failed to get DHCP lease time");
		return -1;
	}

	return lease_time;
}

/**********************************************************************************************/

int wifi_api_start_deep_sleep (uint32_t timeout, uint8_t gpio)
{
	uint8_t wakeup_source = WKUP_SRC_RTC;
	nrc_err_t err;

	if (gpio > 0)
	{
		bool debounce = false;

		if (nrc_ps_set_gpio_wakeup_pin(debounce, gpio, true) != NRC_SUCCESS)
		{
			_atcmd_error("failed to set wakeup pin");
			return -1;
		}

		wakeup_source |= WKUP_SRC_GPIO;
	}

	if (nrc_ps_set_wakeup_source(wakeup_source) != NRC_SUCCESS)
	{
		_atcmd_error("failed to set wakeup source");
		return -1;
	}

	if (timeout > 0)
		err = nrc_ps_deep_sleep(timeout);
	else
	{
		uint32_t idle_timeout = 100; /* msec */

		err = nrc_ps_wifi_tim_deep_sleep(idle_timeout, 0);
	}

	if (err != NRC_SUCCESS)
	{
		_atcmd_error("failed to start deep sleep");
		return -1;
	}

	return 0;
}

bool wifi_api_wakeup_done (void)
{
	if (system_modem_api_ps_event_user_get(PS_EVT_WAKEUP_DEEPSLEEP) != 1)
		return false;

	system_modem_api_ps_event_user_clear(PS_EVT_WAKEUP_DEEPSLEEP);

	return true;
}

/**********************************************************************************************/
#ifdef CONFIG_ATCMD_SOFTAP

#include "umac_info.h"

int wifi_api_start_softap (int bw, int freq,
							char *ssid, int ssid_type,
							char *security, char *password, int sae_pwe,
							uint32_t timeout)
{
	tWIFI_SECURITY sec_mode = WIFI_SEC_MAX;

	if (freq == 0 && wifi_api_is_relay_mode())
	{
		uint16_t s1g_freq;
		uint8_t s1g_bw;

		if (wifi_api_get_channel(&s1g_freq, &s1g_bw, true) !=  WIFI_SUCCESS)
			return -1;

		freq = s1g_freq;
		bw = s1g_bw;
	}

	if (freq <= 0 || !ssid || !security || (ssid_type < 0 || ssid_type > 2))
		return -EINVAL;

	if (bw != WIFI_1M && bw != WIFI_2M && bw != WIFI_4M)
	{
		const int wifi_bw[] =
		{
			[BW_1M] = WIFI_1M,
			[BW_2M] = WIFI_2M,
			[BW_4M] = WIFI_4M,
		};
		bw_t bw_index;

		for (bw_index = BW_1M ; bw_index < BW_MAX ; bw_index++)
		{
			if (STAGetNonS1GFreqWithBw(freq, bw_index) != 0)
				break;
		}

		if (bw_index >= BW_MAX)
			return -EINVAL;

		bw = wifi_bw[bw_index];

		_atcmd_info("%s: bandwidth=%d s1g_freq=%d", __func__, bw, freq);
	}

	if (strcmp(security, "open") == 0)
		sec_mode = WIFI_SEC_OPEN;
	else
	{
		if (strcmp(security, "wpa2-psk") == 0)
			sec_mode = WIFI_SEC_WPA2;
#if defined(CONFIG_ATCMD_WPA3)
		else if (strcmp(security, "wpa3-owe") == 0)
			sec_mode = WIFI_SEC_WPA3_OWE;
		else if (strcmp(security, "wpa3-sae") == 0)
		{
			sec_mode = WIFI_SEC_WPA3_SAE;
			if (sae_pwe < 0 || sae_pwe > 2)
				return -EINVAL;
		}
#endif

		if (sec_mode == WIFI_SEC_MAX || !password)
			return -1;
	}

	if (nrc_wifi_softap_set_ignore_broadcast_ssid(vif_id_ap, ssid_type) != WIFI_SUCCESS)
		return -1;

/*	ssid_type = -1;
	if (nrc_wifi_softap_get_ssid_type(vif_id_ap, &ssid_type) != WIFI_SUCCESS)
		return -1;
	_atcmd_debug("%s: ssid_type=%d", __func__, ssid_type); */

	if (nrc_wifi_softap_set_conf(vif_id_ap, ssid, freq, bw, sec_mode, password, sae_pwe) != WIFI_SUCCESS)
		return -1;

	if (nrc_wifi_softap_start_timeout(vif_id_ap, timeout) != WIFI_SUCCESS)
		return -1;

	wifi_api_set_4address(true);

	return 0;
}

int wifi_api_stop_softap (void)
{
	if (nrc_wifi_softap_stop(vif_id_ap) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_get_max_num_sta (uint8_t *max_num_sta)
{
	if (nrc_wifi_softap_get_max_num_sta(vif_id_ap, max_num_sta) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_set_max_num_sta (uint8_t max_num_sta)
{
	if (nrc_wifi_softap_set_max_num_sta(vif_id_ap, max_num_sta) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_get_sta_info (int aid, char *maddr, int8_t *rssi, uint8_t *snr, uint8_t *tx_mcs, uint8_t *rx_mcs)
{
	STAINFO *sta_info;

	if (!aid || !maddr || !rssi || !snr || !tx_mcs || !rx_mcs )
		return -EINVAL;

	sta_info = get_stainfo_by_aid(vif_id_ap, aid);
	if (!sta_info || sta_info->m_state != ASSOC || sta_info->m_binfo.aid != aid)
		return -1;

	sprintf(maddr, MACSTR, MAC2STR(sta_info->m_binfo.maddr));

#if defined(INCLUDE_STA_SIG_INFO)
	*rssi = sta_info->m_signal.rssi;
	*snr = sta_info->m_signal.snr;
#endif
	*tx_mcs = sta_info->m_mcs.last_tx_mcs;
	*rx_mcs = sta_info->m_mcs.last_rx_mcs;

	return 0;
}

int wifi_api_set_bss_max_idle (uint16_t period, uint8_t retry_cnt)
{
	if (retry_cnt == 0)
		return -EINVAL;

	if (nrc_wifi_softap_set_bss_max_idle(vif_id_ap, period, retry_cnt) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_start_dhcp_server (void)
{
	if (nrc_wifi_softap_start_dhcp_server(vif_id_ap) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_stop_dhcp_server (void)
{
	if (nrc_wifi_softap_stop_dhcp_server(vif_id_ap) != WIFI_SUCCESS)
		return -1;

	return 0;
}

void wifi_api_get_relay_mode (bool *enable)
{
	if (vif_id_ap == vif_id_sta)
	{
/*		ASSERT(!system_modem_api_is_relay()); */
		*enable = false;
	}
	else
	{
		ASSERT(system_modem_api_is_ap(vif_id_ap) && system_modem_api_is_sta(vif_id_sta));
		*enable = true;
	}
}

void wifi_api_set_relay_mode (bool enable)
{
	if (enable)
	{
		_atcmd_debug("relay_mode: enable");

		wifi_api_set_if_mode(IF_MODE_RELAY);
		wifi_api_add_bridge();
	}
	else
	{
		_atcmd_debug("relay_mode: disable");

		wifi_api_set_if_mode(IF_MODE_APSTA);
		wifi_api_delete_bridge();
	}
}

bool wifi_api_is_relay_mode (void)
{
	bool enable;

	wifi_api_get_relay_mode(&enable);

	return enable;
}

void wifi_api_add_bridge (void)
{
	if (netif_is_up(&br_netif))
		_atcmd_info("bridge inerface is already up.");
	else
	{
		int vif_id;

		setup_wifi_bridge_interface();

		for (vif_id = 0 ; vif_id <= 1 ; vif_id++)
		{
			if (!netif_is_link_up(nrc_netif[vif_id]))
			{
				_atcmd_debug("interface %d up", vif_id);

				netif_set_link_up(nrc_netif[vif_id]);
			}

			bridgeif_add_port(&br_netif, nrc_netif[vif_id]);
		}
	}
}

void wifi_api_delete_bridge (void)
{
	if (!netif_is_up(&br_netif))
		_atcmd_info("bridge inerface is already down.");
	else
		delete_wifi_bridge_interface();
}

void wifi_api_get_4address (bool *enable)
{
	if (nrc_get_use_4address())
		*enable = true;
	else
		*enable = false;
}

void wifi_api_set_4address (bool enable)
{
	nrc_set_use_4address(enable);
}

#endif /* #ifdef CONFIG_ATCMD_SOFTAP */
/**********************************************************************************************/

#include "wps_defs.h"

extern tWIFI_STATUS nrc_wifi_set_network_index (int vif_id, int net_id);
extern tWIFI_STATUS nrc_wifi_wps_pbc_bssid (int vif_id, const char *bssid);

static struct
{
	bool init;
	enum WPS_STATUS status[2];
	wifi_wps_cb_t callback[2];
} g_wifi_wps_info =
{
	.init = false,
	.status = { WPS_DISABLE, WPS_DISABLE },
	.callback = { NULL, NULL },
};

static void _wifi_api_wps_success (void *priv, int net_id, uint8_t *ssid, uint8_t ssid_len,
										uint8_t security, char *passphrase)
{
	struct nrc_wpa_if *intf = (struct nrc_wpa_if *)priv;
	int vif_id = intf->vif_id;

	if (security > 4)
		security = 4;

/*	_atcmd_debug("WPS_SUCCESS: vif_id=%d", vif_id); */

	if (g_wifi_wps_info.status[vif_id] == WPS_ACTIVE)
	{
		if (net_id >= 0)
		{
/*			_atcmd_debug(" - net_id     : %d", net_id);
			_atcmd_debug(" - ssid       : %s (%u)", ssid, ssid_len);
			_atcmd_debug(" - security   : %s", str_sec_mode[security]);
			_atcmd_debug(" - passphrase : %s (%u)", passphrase, strlen(passphrase)); */

			nrc_wifi_set_network_index(vif_id, net_id);
		}

		if (!g_wifi_wps_info.callback[vif_id])
			g_wifi_wps_info.status[vif_id] = WPS_SUCCESS;
		else
		{
			g_wifi_wps_info.callback[vif_id](WPS_SUCCESS, vif_id, ssid, str_sec_mode[security], passphrase);

			g_wifi_wps_info.status[vif_id] = WPS_DISABLE;
			g_wifi_wps_info.callback[vif_id] = NULL;
		}
	}
}

static void _wifi_api_wps_timeout (void *priv)
{
	struct nrc_wpa_if *intf = (struct nrc_wpa_if *)priv;
	int vif_id = intf->vif_id;

/*	_atcmd_debug("WPS_TIMEOUT: vif_id=%d", vif_id); */

	if (g_wifi_wps_info.status[vif_id] == WPS_ACTIVE)
	{
		if (!g_wifi_wps_info.callback[vif_id])
			g_wifi_wps_info.status[vif_id] = WPS_TIMEOUT;
		else
		{
			g_wifi_wps_info.callback[vif_id](WPS_TIMEOUT, vif_id);

			g_wifi_wps_info.status[vif_id] = WPS_DISABLE;
			g_wifi_wps_info.callback[vif_id] = NULL;
		}
	}
}

static void _wifi_api_wps_fail (void *priv)
{
	struct nrc_wpa_if *intf = (struct nrc_wpa_if *)priv;
	int vif_id = intf->vif_id;

/*	_atcmd_debug("WPS_FAIL: vif_id=%d", vif_id); */

	if (g_wifi_wps_info.status[vif_id] == WPS_ACTIVE)
	{
		if (!g_wifi_wps_info.callback[vif_id])
			g_wifi_wps_info.status[vif_id] = WPS_FAIL;
		else
		{
			g_wifi_wps_info.callback[vif_id](WPS_FAIL, vif_id);

			g_wifi_wps_info.status[vif_id] = WPS_DISABLE;
			g_wifi_wps_info.callback[vif_id] = NULL;
		}
	}
}

static void _wifi_api_wps_init (int vif_id, wifi_wps_cb_t cb)
{
	if (!g_wifi_wps_info.init)
	{
		static struct pbc_ops ops;

		memset(&ops, 0, sizeof(ops));

		ops.GPIO_PushButton[0] = -1;
		ops.GPIO_PushButton[1] = -1;
		ops.nrc_wifi_wps_pbc_pressed = NULL;

		ops.nrc_wifi_wps_pbc_success = _wifi_api_wps_success;
		ops.nrc_wifi_wps_pbc_timeout = _wifi_api_wps_timeout;
		ops.nrc_wifi_wps_pbc_fail = _wifi_api_wps_fail;

		init_wps_pbc(&ops);

		g_wifi_wps_info.init = true;
	}

	g_wifi_wps_info.status[vif_id] = WPS_DISABLE;
	g_wifi_wps_info.callback[vif_id] = cb;
}

int wifi_api_enable_wps (const char *bssid, wifi_wps_cb_t cb)
{
	int vif_id = 0;

	if (g_wifi_wps_info.status[vif_id] == WPS_ACTIVE)
		return -EBUSY;

	_wifi_api_wps_init(vif_id, cb);

	if (nrc_wifi_wps_pbc_bssid(vif_id, bssid) != WIFI_SUCCESS)
		return -1;

	g_wifi_wps_info.status[vif_id] = WPS_ACTIVE;

	if (!cb)
	{
		const uint32_t timeout = (WPS_PBC_WALK_TIME + 10) * 1000; /* msec */
		int ret = -ETIMEDOUT;
		int i;

		for (i = 0 ; i < timeout ; i += 100)
		{
			_delay_ms(100);

			switch (g_wifi_wps_info.status[vif_id])
			{
				case WPS_SUCCESS:
				case WPS_TIMEOUT:
				case WPS_FAIL:
					i = timeout;
					ret = g_wifi_wps_info.status[vif_id];

				default:
					break;
			}
		}

		wifi_api_disable_wps();

		return ret;
	}

	return 0;
}

int wifi_api_disable_wps (void)
{
	int vif_id = 0;

	g_wifi_wps_info.status[vif_id] = WPS_DISABLE;
	g_wifi_wps_info.callback[vif_id] = NULL;

	if (nrc_wifi_wps_cancel(vif_id) != WIFI_SUCCESS)
		return -1;

	return 0;
}
