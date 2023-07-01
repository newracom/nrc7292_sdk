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

static const int vif_id = ATCMD_NETIF_INDEX;

/**********************************************************************************************/

extern bool lmac_support_lbt (void);
extern int lmac_set_lbt(uint16_t cs_duration, uint32_t pause_time, uint32_t tx_resume_time);
extern int lmac_get_lbt(uint16_t *cs_duration, uint32_t *pause_time, uint32_t *tx_resume_time);
extern bool lmac_send_qos_null_frame (bool pm);

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

static void _wifi_api_event_handler (int vif, tWIFI_EVENT_ID id, int data_len, void *data)
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
		_atcmd_info("wifi_event: %d (invalid)\n", id);
	else if (!g_wifi_event_cb[id])
		_atcmd_info("wifi_event: %d, %s (unused)\n", id, str_event[id]);
	else
	{
		_atcmd_info("wifi_event: %s, data=%p,%d\n", str_event[id], data, data_len);

		g_wifi_event_cb[id](vif, data, data_len);
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
				if (wifi_station_dhcpc_status(vif_id))
					wifi_station_dhcpc_stop(vif_id);

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

	if (nrc_wifi_register_event_handler(vif_id, _wifi_api_event_handler) != WIFI_SUCCESS)
		return -1;

	return 0;
}

/**********************************************************************************************/

int wifi_api_get_rf_cal (bool *cal_use, char *country)
{
	TX_PWR_CAL_PARAM tx_pwr_cal;

	if (!cal_use || !country)
		return -1;

#if !defined(NRC7292)
	return -1;
#else
	*cal_use = !!hal_get_rf_cal_use();

	system_api_get_rf_cal(SF_RF_CAL, (uint8_t*)&tx_pwr_cal, sizeof(tx_pwr_cal));
	sprintf(country, "%c%c", tx_pwr_cal.country[0], tx_pwr_cal.country[1]);
#endif

	return 0;
}

int wifi_api_set_rf_cal (bool cal_use)
{
#if !defined(NRC7292)
	return -1;
#else
	system_api_set_sys_config_cal_use(cal_use ? 1 : 0);
#endif

	return 0;
}

uint16_t wifi_api_get_s1g_freq (uint16_t non_s1g_freq)
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

int wifi_api_get_supported_channels (uint16_t s1g_freq[], int n_s1g_freq_max)
{
	uint16_t *nons1g_freq = NULL;
	int n_nons1g_freq = 0;
	int n_s1g_freq = 0;
	uint16_t freq;
	int i;

	if (!s1g_freq || !n_s1g_freq_max)
		return -EINVAL;

	system_api_get_supported_channels(&nons1g_freq, &n_nons1g_freq);

	if (!nons1g_freq || !n_nons1g_freq)
	{
		_atcmd_error("failed to get supported channels\n");
		return -1;
	}

	for (i = 0 ; i < n_nons1g_freq ; i++)
	{
		freq = wifi_api_get_s1g_freq(nons1g_freq[i]);
		if(freq == 0)
			continue;

		s1g_freq[n_s1g_freq] = freq;
		if (++n_s1g_freq >= n_s1g_freq_max)
			break;
	}

	return n_s1g_freq;
}

int wifi_api_get_macaddr (char *macaddr)
{
	if (!macaddr)
		return -EINVAL;

	if (nrc_wifi_get_mac_address(vif_id, macaddr) != WIFI_SUCCESS)
		return -1;

	if (strlen(macaddr) != ATCMD_WIFI_MACADDR_LEN)
		return -1;

	return 0;
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

	if (nrc_wifi_set_country(vif_id, cc) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_get_tx_power (uint8_t *power)
{
	if (!power)
		return -EINVAL;

	if (nrc_wifi_get_tx_power(power) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_set_tx_power (uint8_t power, enum TX_POWER_TYPE type)
{
	if (power == 0)
		return -EINVAL;

	switch (type)
	{
		case TX_POWER_AUTO:
		case TX_POWER_LIMIT:
		case TX_POWER_FIXED:
			break;

		default:
			return -EINVAL;
	}

	if (nrc_wifi_set_tx_power(power, type) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_get_rate_control (bool *enable)
{
	if (!enable)
		return -EINVAL;

	*enable = system_modem_api_get_rate_control(vif_id);

	return 0;
}

void wifi_api_set_rate_control (bool enable)
{
	system_modem_api_set_rate_control(vif_id, enable);
}

int wifi_api_get_mcs (uint8_t *index)
{
	if (!index)
		return -EINVAL;

	*index = system_modem_api_get_mcs();

	return 0;
}

int wifi_api_set_mcs (uint8_t index)
{
	if (system_modem_api_set_mcs(index))
		return 0;

	return -1;
}

int wifi_api_get_snr (uint8_t *snr)
{
	if (!snr)
		return -EINVAL;

	*snr = 0;

	if (nrc_wifi_get_snr(snr) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_get_rssi (int8_t *rssi)
{
	if (!rssi)
		return -EINVAL;

	*rssi = ATCMD_WIFI_RSSI_MIN;

	if (nrc_wifi_get_rssi(rssi) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_get_tsf (uint64_t *tsf)
{
	uint32_t *low;
	uint32_t *high;

	if (!tsf)
		return -EINVAL;

	low = (uint32_t *)tsf;
	high = (uint32_t *)tsf + 1;

	*high = lmac_get_tsf_h(vif_id);
	*low = lmac_get_tsf_l(vif_id);

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
	if (!threshold)
		return -EINVAL;

	*threshold = system_modem_api_get_cca_threshold(vif_id);

	return 0;
}

int wifi_api_set_cca_threshold (int threshold)
{
	if (system_modem_api_set_cca_threshold(vif_id, threshold))
		return 0;

	return -1;
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

int wifi_api_get_lbt (uint16_t *cs_duration, uint32_t *pause_time, uint32_t *tx_resume_time)
{
	if (!lmac_support_lbt())
		return -ENOTSUP;

	if (!cs_duration || !pause_time || !tx_resume_time)
		return -EINVAL;

	if (lmac_get_lbt(cs_duration, pause_time, tx_resume_time) != 0)
		return -1;

	return 0;
}

int wifi_api_set_lbt (uint16_t cs_duration, uint32_t pause_time, uint32_t tx_resume_time)
{
#if 0
	if (!lmac_support_lbt())
		return -ENOTSUP;
#endif

	if (lmac_set_lbt(cs_duration, pause_time, tx_resume_time) != 0)
		return -1;

	return 0;
}

int wifi_api_get_mic_scan (bool *enable, bool *channel_move, uint32_t *cnt_detected)
{
	if (!enable || !channel_move)
		return -EINVAL;

	system_modem_api_get_mic_scan(enable);
	system_modem_api_get_mic_scan_move(channel_move);

	if (cnt_detected)
		*cnt_detected = system_modem_api_get_mic_detect_count();

	return 0;
}

int wifi_api_set_mic_scan (bool enable, bool channel_move)
{
	system_modem_api_set_mic_scan(enable);
	system_modem_api_set_mic_scan_move(channel_move);

	return 0;
}

int wifi_api_get_bmt (uint32_t *threshold)
{
	if (!threshold)
		return -EINVAL;

	*threshold = system_modem_api_get_bmt_threshold(vif_id);

	return 0;
}

int wifi_api_set_bmt (uint32_t threshold)
{
	if (threshold == 0)
	{
		system_modem_api_enable_bmt(vif_id, false);
		system_modem_api_set_bmt_threshold(vif_id, 0);
	}
	else
	{
		system_modem_api_set_bmt_threshold(vif_id, threshold);
		system_modem_api_enable_bmt(vif_id, true);
	}

	return 0;
}

int wifi_api_set_ssid (char *ssid)
{
	if (!ssid)
		return -EINVAL;

	if (nrc_wifi_set_ssid(vif_id, ssid) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_set_bssid (char *bssid)
{
	if (!bssid)
		return -EINVAL;

	if (nrc_wifi_set_bssid(vif_id, bssid) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_set_security (char *security, char *password)
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
		sec_mode = WIFI_SEC_WPA3_SAE;
#endif
	else
		return -1;

	if (nrc_wifi_set_security(vif_id, sec_mode, password) != WIFI_SUCCESS)
		return -1;

	return 0;
}

/**********************************************************************************************/

int wifi_api_add_network (void)
{
	if (nrc_wifi_add_network(vif_id) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_remove_network (void)
{
	if (nrc_wifi_remove_network(vif_id) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_start_scan (uint32_t timeout)
{
	if (nrc_wifi_scan_timeout(vif_id, timeout, NULL) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_get_scan_results (SCAN_RESULTS *results)
{
	if (nrc_wifi_scan_results(vif_id, results) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_get_scan_freq (uint16_t freq[], uint8_t *n_freq)
{
	if (!freq || !n_freq)
		return -EINVAL;

	if (nrc_wifi_get_scan_freq(vif_id, freq, n_freq) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_set_scan_freq (uint16_t freq[], uint8_t n_freq)
{
	if ((!freq && n_freq > 0) || (freq && n_freq == 0))
		return -EINVAL;

	if (nrc_wifi_set_scan_freq(vif_id, freq, n_freq) != WIFI_SUCCESS)
		return -1;

	return 0;
}

bool wifi_api_connecting (void)
{
	if (nrc_wifi_get_state(vif_id) == WIFI_STATE_TRY_CONNECT)
		return true;

	return false;
}

bool wifi_api_connected (void)
{
	if (nrc_wifi_get_state(vif_id) == WIFI_STATE_CONNECTED)
		return true;

	return false;
}

bool wifi_api_disconnected (void)
{
	if (nrc_wifi_get_state(vif_id) == WIFI_STATE_DISCONNECTED)
		return true;

	return false;
}

int wifi_api_connect (uint32_t timeout)
{
	switch (nrc_wifi_connect(vif_id, timeout))
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
	switch (nrc_wifi_disconnect(vif_id, timeout))
	{
		case WIFI_SUCCESS:
			return 0;

		case WIFI_TIMEOUT:
			return 1;

		default:
			return -1;
	}
}

int wifi_api_get_ap_ssid (char *str_ssid, int len)
{
	APINFO *apinfo = get_apinfo_by_vifid(vif_id);
	uint8_t *ssid;
	uint8_t ssid_len;

	if (!str_ssid || !len)
		return -EINVAL;

	if (!apinfo)
		return -1;

	ssid = apinfo->m_binfo.ssid;
	ssid_len = apinfo->m_binfo.ssid_len;

	if (ssid_len > len)
		return -1;

	strcpy(str_ssid, (char *)ssid);

	return 0;
}

int wifi_api_get_ap_bssid (char *str_bssid)
{
	APINFO *apinfo = get_apinfo_by_vifid(vif_id);
	uint8_t *bssid;

	if (!str_bssid)
		return -EINVAL;

	if (!apinfo)
		return -1;

	bssid = apinfo->m_binfo.bssid;

	sprintf(str_bssid, "%02x:%02x:%02x:%02x:%02x:%02x",
				bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);

	return 0;
}

int wifi_api_get_ip_address (char *address, char *netmask, char *gateway)
{
	struct netif *netif = nrc_netif[ATCMD_NETIF_INDEX];

	if (!address || !netmask || !gateway)
		return -EINVAL;

	strcpy(address, ipaddr_ntoa(netif_ip_addr4(netif)));
	strcpy(netmask, ipaddr_ntoa(netif_ip_netmask4(netif)));
	strcpy(gateway, ipaddr_ntoa(netif_ip_gw4(netif)));

	return 0;
}

int wifi_api_set_ip_address (char *address, char *netmask, char *gateway)
{
	struct netif *netif = nrc_netif[ATCMD_NETIF_INDEX];
	ip_addr_t ipaddr[3];

	if (!address || !netmask || !gateway)
		return -EINVAL;

	if (!ipaddr_aton(address, &ipaddr[0]))
		return -1;

	if (!ipaddr_aton(netmask, &ipaddr[1]))
		return -1;

	if (!ipaddr_aton(gateway, &ipaddr[2]))
		return -1;

	netif_set_ipaddr(netif, ip_2_ip4(&ipaddr[0]));
	netif_set_netmask(netif, ip_2_ip4(&ipaddr[1]));
	netif_set_gw(netif, ip_2_ip4(&ipaddr[2]));

	return 0;
}

int wifi_api_start_dhcp_client (uint32_t timeout_msec)
{
	int i;

	wifi_station_dhcpc_stop(vif_id);
	set_dhcp_status(false);

	if (dhcp_run(vif_id) < 0)
		return DHCP_FAIL;

	for (i = 0 ; i < timeout_msec ; i += 10)
	{
		if (get_dhcp_status())
			return DHCP_SUCCESS;

		if (!wifi_station_dhcpc_status(vif_id))
			return DHCP_STOP;

		_delay_ms(10);
	}

	wifi_station_dhcpc_stop(vif_id);

	return DHCP_TIMEOUT;
}

/**********************************************************************************************/

int wifi_api_start_deep_sleep (uint32_t timeout, uint8_t gpio)
{
	uint8_t wakeup_source = WKUP_SRC_RTC;
	nrc_err_t err;

	if (gpio > 0)
	{
		bool debounce = false;

		if (nrc_ps_set_gpio_wakeup_pin(debounce, gpio) != NRC_SUCCESS)
		{
			_atcmd_error("failed to set wakeup pin\n");
			return -1;
		}

		wakeup_source |= WKUP_SRC_GPIO;
	}

	if (nrc_ps_set_wakeup_source(wakeup_source) != NRC_SUCCESS)
	{
		_atcmd_error("failed to set wakeup source\n");
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
		_atcmd_error("failed to start deep sleep\n");
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

#ifdef CONFIG_ATCMD_SOFTAP
/**********************************************************************************************/

#include "umac_info.h"

int wifi_api_start_softap (int bw, int freq, char *ssid,
							char *security, char *password,
							uint32_t timeout)
{
	tWIFI_SECURITY sec_mode = WIFI_SEC_MAX;

	if (freq <= 0 || !ssid || !security)
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

		_atcmd_info("%s: bandwidth=%d s1g_freq=%d\n", __func__, bw, freq);
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
			sec_mode = WIFI_SEC_WPA3_SAE;
#endif

		if (sec_mode == WIFI_SEC_MAX || !password)
			return -1;
	}

	if (nrc_wifi_softap_set_conf(vif_id, ssid, freq, bw, sec_mode, password) != WIFI_SUCCESS)
		return -1;

	if (nrc_wifi_softap_start_timeout(vif_id, timeout) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_stop_softap (void)
{
	if (nrc_wifi_softap_stop(vif_id) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_get_max_sta_aid (void)
{
	return MAX_STA;
}

int wifi_api_get_sta_info (int aid, char *maddr, int8_t *rssi, uint8_t *snr, uint8_t *mcs)
{
	STAINFO *sta_info;

	if (!aid || !maddr || !rssi || !snr || !mcs)
		return -EINVAL;

	sta_info = get_stainfo_by_aid(vif_id, aid);
	if (!sta_info || sta_info->m_state != ASSOC || sta_info->m_binfo.aid != aid)
		return -1;

	sprintf(maddr, MACSTR, MAC2STR(sta_info->m_binfo.maddr));

#if defined(INCLUDE_STA_SIG_INFO)
	*rssi = sta_info->m_signal.rssi;
	*snr = sta_info->m_signal.snr;
	*mcs = sta_info->m_signal.mcs;
#endif

	return 0;
}

int wifi_api_set_bss_max_idle (int period, int retry_cnt)
{
	if (period < 0 || retry_cnt < 0)
		return -EINVAL;

	if (nrc_wifi_softap_set_bss_max_idle(vif_id, period, retry_cnt) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_start_dhcp_server (void)
{
	if (nrc_wifi_softap_start_dhcp_server(vif_id) != WIFI_SUCCESS)
		return -1;

	return 0;
}

int wifi_api_stop_dhcp_server (void)
{
	if (nrc_wifi_softap_stop_dhcp_server(vif_id) != WIFI_SUCCESS)
		return -1;

	return 0;
}

/**********************************************************************************************/
#endif /* #ifdef CONFIG_ATCMD_SOFTAP */

