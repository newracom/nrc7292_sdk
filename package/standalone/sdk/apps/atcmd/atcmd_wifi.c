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


#include "atcmd.h"

#ifndef CONFIG_ATCMD_WITHOUT_LWIP
#include "nrc_wifi.h" /* /lib/lwip/contrib/port */
#endif

/**********************************************************************************************/

extern struct netif *nrc_netif[];

extern uint32_t _atcmd_timeout_value (const char *cmd);

/**********************************************************************************************/

//#define ATCMD_WIFI_INFO_STATIC
#ifdef ATCMD_WIFI_INFO_STATIC
static atcmd_wifi_info_t _atcmd_wifi_info;
static atcmd_wifi_info_t *g_atcmd_wifi_info = &_atcmd_wifi_info;
#else
static atcmd_wifi_info_t *g_atcmd_wifi_info = NULL;
#endif

static void _atcmd_wifi_event_handler (tWIFI_EVENT_ID event, int data_len, char* data);
static int _atcmd_wifi_scan_freq_set (uint16_t freq[], int n_freq);
#ifndef CONFIG_ATCMD_WITHOUT_LWIP
static void _atcmd_wifi_ping_report_callback (_lwip_ping_report_t *report);
#endif

/**********************************************************************************************/

static uint16_t _atcmd_wifi_s1g_freq (uint16_t nons1g_freq)
{
	uint16_t s1g_freq = 0;

	if (CheckSupportNonS1GFreq(nons1g_freq))
		s1g_freq = STAGetS1GFreq(nons1g_freq);

/*	if (s1g_freq == 0)
		_atcmd_info("wifi_s1g: not supported, %u\n", nons1g_freq); */

	return s1g_freq;
}

static uint32_t _atcmd_wifi_snr_read (void)
{
	uint8_t snr;

	if (nrc_wifi_get_snr(&snr) != WIFI_SUCCESS)
		return -1;

	return snr;
}

static int8_t _atcmd_wifi_rssi_read (void)
{
	int8_t rssi;

	if (nrc_wifi_get_rssi(&rssi) != WIFI_SUCCESS)
		return ATCMD_WIFI_RSSI_MIN;

	return rssi;
}

static bool _atcmd_wifi_rssi_valid (const int rssi)
{
	if (rssi >= ATCMD_WIFI_RSSI_MIN && rssi <= ATCMD_WIFI_RSSI_MAX)
		return true;

	return false;
}

static bool _atcmd_wifi_bssid_valid (const char *bssid)
{
	int i;

	if (!bssid || strlen(bssid) != ATCMD_WIFI_BSSID_LEN)
		return false;

	for (i = 0 ; i < ATCMD_WIFI_BSSID_LEN ; i++)
	{
		if ((i % 3) < 2)
		{
			if (bssid[i] >= '0' && bssid[i] <= '9')
				continue;
			else if (bssid[i] >= 'a' && bssid[i] <= 'f')
				continue;
			else if (bssid[i] >= 'A' && bssid[i] <= 'F')
				continue;

			return false;
		}
		else if (bssid[i] != ':')
			return false;
	}

	return true;
}

static bool _atcmd_wifi_country_valid (atcmd_wifi_country_t country)
{
	if (strlen(country) == 2)
	{
		const char *support_country[] = ATCMD_WIFI_COUNTRY_ARRAY;
		const int n_support_country = ATCMD_WIFI_COUNTRY_NUMBER;
		int i;

		for (i = 0 ; i < n_support_country ; i++)
		{
			if (strcmp(country, support_country[i]) == 0)
				return true;
		}
	}

	return false;
}

static tWIFI_SECURITY _atcmd_wifi_security_string_to_index (atcmd_wifi_security_t str)
{
	if (str)
	{
		if (strcmp(str, "open") == 0)
			return WIFI_SEC_OPEN;
		else if (strcmp(str, "wpa2-psk") == 0)
			return WIFI_SEC_WPA2;
#if defined(CONFIG_ATCMD_WPA3)
		else if (strcmp(str, "wpa3-owe") == 0)
			return WIFI_SEC_WPA3_OWE;
		else if (strcmp(str, "wpa3-sae") == 0)
			return WIFI_SEC_WPA3_SAE;
#endif
	}

	return WIFI_SEC_MAX;
}

static int _atcmd_wifi_rf_cal_info (bool *cal_use, atcmd_wifi_country_t country)
{
	if (!cal_use || !country)
		return -1;

	*cal_use = !!hal_get_rf_cal_use();
	memset(country, 0, sizeof(atcmd_wifi_country_t));

	if (*cal_use)
	{
		TX_PWR_CAL_PARAM tx_pwr_cal;

		system_api_get_rf_cal(SF_RF_CAL, (uint8_t*)&tx_pwr_cal, sizeof(tx_pwr_cal));

		sprintf(country, "%c%c", tx_pwr_cal.country[0], tx_pwr_cal.country[1]);

		if (!_atcmd_wifi_country_valid(country))
			memset(country, 0, sizeof(atcmd_wifi_country_t));
	}

	return 0;
}

/**********************************************************************************************/

static void _atcmd_wifi_channels_print (uint16_t freq[], int n_ch, const char *func)
{
	_atcmd_info("%s: %d\n", func, n_ch);

	if (freq && n_ch > 0)
	{
		int i;

		for (i = 0 ; i < n_ch ; i++)
		{
			if ((i % 10) == 0)
				_atcmd_info("");

			_atcmd_printf(" %4d", freq[i]);

			if ((i % 10) == 9)
				_atcmd_printf("\n");
		}

		if ((i % 10) != 0)
			_atcmd_printf("\n");
	}
}

static int _atcmd_wifi_channels_init (atcmd_wifi_channels_t *channels)
{
	uint16_t *chs = NULL;
	int n_ch = 0;
	int n_ch_max;
	uint16_t freq;
	int i, j;

	system_api_get_supported_channels(&chs, &n_ch);

	if (!chs)
	{
		_atcmd_error("no supported channel\n");
		return -1;
	}

	n_ch_max = sizeof(channels->freq) / sizeof(uint16_t);

	if (n_ch > n_ch_max)
	{
		_atcmd_error("n_ch(%d) > %d\n", n_ch, n_ch_max);
		return -1;
	}

	for (i = j = 0 ; i < n_ch ; i++)
	{
		freq = _atcmd_wifi_s1g_freq(chs[i]);
		if(freq > 0)
			channels->freq[j++] = freq;
	}

	channels->n_freq = j;

	for ( ; j < ATCMD_WIFI_CHANNELS_MAX ; j++)
		channels->freq[j] = 0;

	_atcmd_wifi_channels_print(channels->freq, channels->n_freq, "supported_channels");

	return 0;
}

/**********************************************************************************************/

static void _atcmd_wifi_init_info (atcmd_wifi_info_t *info)
{
	bool cal_use;
	atcmd_wifi_country_t country;

	if (_atcmd_wifi_rf_cal_info(&cal_use, country) != 0)
	{
		cal_use = false;
		memset(country, 0, sizeof(atcmd_wifi_country_t));
	}

	_atcmd_info("RF_CAL_INFO: cal_use=%d country=%s\n", cal_use, country);

	memset(info, 0, sizeof(atcmd_wifi_info_t));

	info->event = 0;

	if (cal_use && strlen(country) == 2)
		strcpy(info->country, country);
	else
	{
		memcpy(info->country, lmac_get_country(0), 2);

		if (!_atcmd_wifi_country_valid(info->country))
			strcpy(info->country, ATCMD_WIFI_INIT_COUNTRY);
	}

	info->txpower = ATCMD_WIFI_INIT_TXPOWER;

	if (1)
	{
		atcmd_wifi_scan_t *scan = &info->scan;

		scan->scanning = false;
		memset(&scan->results, 0, sizeof(SCAN_RESULTS));
	}

	if (1)
	{
		atcmd_wifi_connect_t *connect = &info->connect;

		connect->connected = false;
		connect->connecting = false;
		connect->disconnecting = false;
		strcpy(connect->ssid, ATCMD_WIFI_INIT_SSID);
		strcpy(connect->bssid, ATCMD_WIFI_INIT_BSSID);
		strcpy(connect->security, ATCMD_WIFI_INIT_SECURITY);
		strcpy(connect->password, ATCMD_WIFI_INIT_PASSWORD);
	}

	if (1)
	{
		atcmd_wifi_dhcp_t *dhcp = &info->dhcp;

		dhcp->requesting = false;
		dhcp->responsed = false;
	}

	if (1)
	{
		atcmd_wifi_roaming_t *roaming = &info->roaming;

		roaming->task = NULL;
		roaming->enable = false;
		memset(&roaming->params, 0, sizeof(atcmd_wifi_roaming_params_t));
	}

#ifndef CONFIG_ATCMD_WITHOUT_LWIP
	if (1)
	{
		atcmd_wifi_ping_t *ping = &info->ping;

		ip4_addr_set_any(&ping->remote_ip);
		ping->interval = ATCMD_WIFI_INIT_PING_INTERVAL;
		ping->count = ATCMD_WIFI_INIT_PING_COUNT;
		ping->data_size = ATCMD_WIFI_INIT_PING_SIZE;
		ping->report_cb = _atcmd_wifi_ping_report_callback;
	}
#endif

	if (1)
	{
		atcmd_wifi_softap_t *softap = &info->softap;

		softap->active = false;
		softap->dhcp_server = false;

		softap->net_id = -1;

		softap->channel.number = -1;
		softap->channel.freq = 0;

		softap->bss_max_idle.period = ATCMD_WIFI_BSS_MAX_IDLE_PERIOD;
		softap->bss_max_idle.retry = ATCMD_WIFI_BSS_MAX_IDLE_RETRY;

		strcpy(softap->ssid, ATCMD_WIFI_INIT_SSID);
		strcpy(softap->security, ATCMD_WIFI_INIT_SECURITY);
		strcpy(softap->password, ATCMD_WIFI_INIT_PASSWORD);

		softap->ip.addr = "192.168.50.1";
		softap->ip.netmask = "255.255.255.0";
		softap->ip.gateway = "192.168.50.1";
	}
}

static int _atcmd_wifi_init (atcmd_wifi_info_t *info)
{
	if (nrc_wifi_set_country(nrc_wifi_country_from_string(info->country)) != WIFI_SUCCESS)
		return -1;

	if (nrc_wifi_set_tx_power(info->txpower) != WIFI_SUCCESS)
		return -1;

	if (_atcmd_wifi_channels_init(&info->channels) != 0)
		return -1;

	nrc_wifi_register_event_handler(_atcmd_wifi_event_handler);

	return 0;
}

/**********************************************************************************************/

static void _atcmd_wifi_event_poll (int event)
{
	g_atcmd_wifi_info->event |= (1 << event);
}

static bool _atcmd_wifi_event_polling (int event)
{
	return !!(g_atcmd_wifi_info->event & (1 << event));
}

static void _atcmd_wifi_event_polled (int event)
{
	g_atcmd_wifi_info->event &= ~(1 << event);
}

static int _atcmd_wifi_scan_result_report (SCAN_RESULTS *results, bool to_host);

static void _atcmd_wifi_event_scan_done (void)
{
	atcmd_wifi_scan_t *scan = &g_atcmd_wifi_info->scan;

	if (scan->scanning)
	{
		scan->scanning = false;

		if (_atcmd_wifi_event_polling(WIFI_EVT_SCAN_DONE))
		{
			_atcmd_wifi_event_polled(WIFI_EVT_SCAN_DONE);

			_atcmd_wifi_scan_result_report(&scan->results, false);

			ATCMD_MSG_WEVENT("\"SCAN_DONE\"");
		}
	}
}

static void _atcmd_wifi_event_connect_success (void)
{
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
	bool event = false;

	if (connect->connecting)
	{
		connect->connected = true;
		connect->connecting = false;
		connect->disconnecting = false;

		if (_atcmd_wifi_event_polling(WIFI_EVT_CONNECT_SUCCESS))
		{
			_atcmd_wifi_event_polled(WIFI_EVT_CONNECT_SUCCESS);
			_atcmd_wifi_event_polled(WIFI_EVT_CONNECT_FAIL);

			event = true;
		}
	}
	else if (!connect->connected)
	{
		connect->connected = true;
		connect->connecting = false;
		connect->disconnecting = false;

		event = true;
	}

	if (event)
	{
		ATCMD_MSG_WEVENT("\"CONNECT_SUCCESS\",\"%s\",\"%s\",\"%s\"",
					connect->bssid, connect->ssid, connect->security);
	}
}

static void _atcmd_wifi_event_connect_fail (void)
{
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
	bool event = false;

	if (connect->connecting)
	{
		connect->connected = false;
		connect->connecting = false;
		connect->disconnecting = false;

		if (_atcmd_wifi_event_polling(WIFI_EVT_CONNECT_FAIL))
		{
			_atcmd_wifi_event_polled(WIFI_EVT_CONNECT_SUCCESS);
			_atcmd_wifi_event_polled(WIFI_EVT_CONNECT_FAIL);

			event = true;
		}
	}
	else if (!connect->connected)
	{
		connect->connected = false;
		connect->connecting = false;
		connect->disconnecting = false;

		event = true;
	}

	if (event)
	{
		ATCMD_MSG_WEVENT("\"CONNECT_FAIL\",\"%s\",\"%s\",\"%s\"",
					connect->bssid, connect->ssid, connect->security);
	}
}

static void _atcmd_wifi_event_disconnect (void)
{
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
	bool event = false;

	if (connect->disconnecting)
	{
		connect->connected = false;
		connect->connecting = false;
		connect->disconnecting = false;

		if (_atcmd_wifi_event_polling(WIFI_EVT_DISCONNECT))
		{
			_atcmd_wifi_event_polled(WIFI_EVT_DISCONNECT);

			event = true;
		}
	}
	else if (connect->connecting || connect->connected)
	{
		connect->connected = false;
		connect->connecting = false;
		connect->disconnecting = false;

		event = true;
	}

	if (event)
	{
		ATCMD_MSG_WEVENT("\"DISCONNECT\",\"%s\",\"%s\",\"%s\"",
					connect->bssid, connect->ssid, connect->security);
	}
}

static void _atcmd_wifi_event_get_ip_success (void)
{
	atcmd_wifi_dhcp_t *dhcp = &g_atcmd_wifi_info->dhcp;

	if (dhcp->requesting)
	{
		dhcp->requesting = false;
		dhcp->responsed = true;

		if (_atcmd_wifi_event_polling(WIFI_EVT_GET_IP))
		{
			_atcmd_wifi_event_polled(WIFI_EVT_GET_IP);

/*			ATCMD_MSG_WEVENT("\"DHCP_SUCCESS\""); */
		}
	}
}

static void _atcmd_wifi_event_get_ip_fail (void)
{
	atcmd_wifi_dhcp_t *dhcp = &g_atcmd_wifi_info->dhcp;

	if (dhcp->requesting)
	{
		dhcp->requesting = false;
		dhcp->responsed = false;

		if (_atcmd_wifi_event_polling(WIFI_EVT_GET_IP_FAIL))
		{
			_atcmd_wifi_event_polled(WIFI_EVT_GET_IP_FAIL);

/*			ATCMD_MSG_WEVENT("\"DHCP_FAIL\""); */
		}
	}
}

static void (*g_atcmd_wifi_event_handler[WIFI_EVT_MAX])(void) =
{
	[WIFI_EVT_CONNECT_SUCCESS] = _atcmd_wifi_event_connect_success,
	[WIFI_EVT_CONNECT_FAIL] = _atcmd_wifi_event_connect_fail,
	[WIFI_EVT_GET_IP] = _atcmd_wifi_event_get_ip_success,
	[WIFI_EVT_GET_IP_FAIL] = _atcmd_wifi_event_get_ip_fail,
	[WIFI_EVT_DISCONNECT] = _atcmd_wifi_event_disconnect,
	[WIFI_EVT_SCAN_DONE] = _atcmd_wifi_event_scan_done,
};

static void _atcmd_wifi_event_handler (tWIFI_EVENT_ID event, int data_len, char* data)
{
	const char *str_event[] =
	{
		[WIFI_EVT_SCAN_DONE] = "scan_done",
		[WIFI_EVT_CONNECT_SUCCESS] = "connect_success",
		[WIFI_EVT_CONNECT_FAIL] = "connect_fail",
		[WIFI_EVT_DISCONNECT] = "disconnect",
		[WIFI_EVT_GET_IP] = "dhcp_success",
		[WIFI_EVT_GET_IP_FAIL] = "dhcp_fail",
		[WIFI_EVT_START_SOFT_AP] = "soft_ap_start",
		[WIFI_EVT_SET_SOFT_AP_IP] = "soft_ap_ip_set",
		[WIFI_EVT_START_DHCP_SERVER] = "dhcp_server_start",
		[WIFI_EVT_VENDOR_IE] = "vendor_ie"
	};

	switch(event)
	{
		case WIFI_EVT_SCAN_DONE:
		case WIFI_EVT_CONNECT_SUCCESS:
		case WIFI_EVT_CONNECT_FAIL:
		case WIFI_EVT_DISCONNECT:
		case WIFI_EVT_GET_IP:
		case WIFI_EVT_GET_IP_FAIL:
			_atcmd_info("wifi_event: %s\n", str_event[event]);
			g_atcmd_wifi_event_handler[event]();
			break;

		case WIFI_EVT_START_SOFT_AP:
		case WIFI_EVT_SET_SOFT_AP_IP:
		case WIFI_EVT_START_DHCP_SERVER:
			_atcmd_info("wifi_event: %s (unused)\n", str_event[event]);
			break;

		case WIFI_EVT_VENDOR_IE:
			/* _atcmd_info("wifi_event: %s (unused)\n", str_event[event]); */
			break;

		default:
			_atcmd_info("wifi_event: %d (unknown)\n", event);
	}
}

/**********************************************************************************************/

static int _atcmd_wifi_macaddr_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			char param_macaddr[ATCMD_STR_PARAM_SIZE(sizeof(atcmd_wifi_macaddr_t))];
			char *macaddr;

			if (nrc_wifi_get_mac_address(&macaddr) != WIFI_SUCCESS)
				return ATCMD_ERROR_FAIL;

			if (!atcmd_str_to_param(macaddr, param_macaddr, sizeof(param_macaddr)))
				return ATCMD_ERROR_FAIL;

			ATCMD_MSG_INFO("WMACADDR", "%s", param_macaddr);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_macaddr =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "MACADDR",
	.id = ATCMD_WIFI_MACADDR,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_macaddr_get,
	.handler[ATCMD_HANDLER_SET] = NULL,
};

/**********************************************************************************************/

static int _atcmd_wifi_country_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			char lmac_country[3];
			char *str_country = g_atcmd_wifi_info->country;
			char param_country[ATCMD_STR_PARAM_SIZE(sizeof(atcmd_wifi_country_t))];

			memset(lmac_country, 0, sizeof(lmac_country));
			memcpy(lmac_country, lmac_get_country(0), 2);

			if (strcmp(str_country, lmac_country) != 0)
				_atcmd_error("country: lmac=%s atcmd=%s\n", lmac_country, str_country);

			if (!atcmd_str_to_param(str_country, param_country, sizeof(param_country)))
				return ATCMD_ERROR_FAIL;

			ATCMD_MSG_INFO("WCOUNTRY", "%s", param_country);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_country_set (int argc, char *argv[])
{
	char *param_country = NULL;

	switch (argc)
	{
		case 0:
/*			ATCMD_MSG_HELP("AT+WCOUNTRY=\"<country code>\""); */
			ATCMD_MSG_HELP("AT+WCOUNTRY=\"{%s}\"", ATCMD_WIFI_COUNTRY_STRING);
			break;

		case 1:
		{
			atcmd_wifi_country_t str_country;

			param_country = argv[0];

			if (!atcmd_param_to_str(param_country, str_country, sizeof(str_country)))
				return ATCMD_ERROR_FAIL;

			if (!_atcmd_wifi_country_valid(str_country))
				return ATCMD_ERROR_INVAL;

			if (nrc_wifi_set_country(nrc_wifi_country_from_string(str_country)) != WIFI_SUCCESS)
				return ATCMD_ERROR_FAIL;
			else
			{
				atcmd_wifi_channels_t *channels = &g_atcmd_wifi_info->channels;

				strcpy(g_atcmd_wifi_info->country, str_country);

				if (_atcmd_wifi_channels_init(channels) != 0)
					return ATCMD_ERROR_FAIL;
			}

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_country =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "COUNTRY",
	.id = ATCMD_WIFI_COUNTRY,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_country_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_country_set,
};

/**********************************************************************************************/

static int _atcmd_wifi_tx_power_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			int txpower;

			if (nrc_wifi_get_tx_power(&txpower) != WIFI_SUCCESS)
				return ATCMD_ERROR_FAIL;

			if (txpower < TX_POWER_MIN || txpower > TX_POWER_MAX)
				return ATCMD_ERROR_FAIL;

			ATCMD_MSG_INFO("WTXPOWER", "%d", txpower);

			if (txpower != g_atcmd_wifi_info->txpower)
			{
				_atcmd_info("change txpower: %d -> %d\n",
								g_atcmd_wifi_info->txpower, txpower);

				g_atcmd_wifi_info->txpower = txpower;
			}

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_tx_power_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WTXPOWER=<power in dBm>");
			break;

		case 1:
		{
			int txpower = atoi(argv[0]);

			if (txpower < TX_POWER_MIN || txpower > TX_POWER_MAX)
				return ATCMD_ERROR_INVAL;

			if (nrc_wifi_set_tx_power(txpower) != WIFI_SUCCESS)
				return ATCMD_ERROR_FAIL;

			g_atcmd_wifi_info->txpower = txpower;

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_tx_power =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "TXPOWER",
	.id = ATCMD_WIFI_TXPOWER,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_tx_power_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_tx_power_set,
};

/**********************************************************************************************/

static int _atcmd_wifi_rx_signal_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			int8_t rssi;
			uint8_t snr;

			if (nrc_wifi_get_rssi(&rssi) != WIFI_SUCCESS || nrc_wifi_get_snr(&snr) != WIFI_SUCCESS)
				return ATCMD_ERROR_FAIL;

			ATCMD_MSG_INFO("WRXSIG", "%d,%u", (int)rssi, snr);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_rx_signal_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WRXSIG=<time>");
			break;

		case 1:
		{
			int time_sec = atoi(argv[0]);
			int i;

			for (i = 0 ; i < time_sec ; i++)
			{
				_atcmd_wifi_rx_signal_get(0, NULL);

				if (time_sec > 1)
					_delay_ms(1000);
			}

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_rx_signal =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "RXSIG",
	.id = ATCMD_WIFI_RXSIGNAL,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_rx_signal_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_rx_signal_set,
};

/**********************************************************************************************/

static int _atcmd_wifi_rate_control_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			bool enable = system_modem_api_get_rate_control(0);

			ATCMD_MSG_INFO("WRATECTRL", "%d", enable ? 1 : 0);

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_rate_control_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WRATECTRL=<mode>");
			break;

		case 1:
		{
			switch (atoi(argv[0]))
			{
				case 0:
					system_modem_api_set_rate_control(0, false);
					break;

				case 1:
					system_modem_api_set_rate_control(0, true);
					break;

				default:
					return ATCMD_ERROR_INVAL;
			}

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_rate_control =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "RATECTRL",
	.id = ATCMD_WIFI_RATECTRL,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_rate_control_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_rate_control_set,
};

/**********************************************************************************************/

static int _atcmd_wifi_mcs_get (int argc, char *argv[])
{
	bool enable = system_modem_api_get_rate_control(0);

	if (enable)
		return ATCMD_ERROR_NOTSUPP;

	switch (argc)
	{
		case 0:
		{
			uint8_t mcs_index = system_modem_api_get_mcs(0);

			ATCMD_MSG_INFO("WMCS", "%u", mcs_index);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_mcs_set (int argc, char *argv[])
{
	bool enable = system_modem_api_get_rate_control(0);

	if (enable)
		return ATCMD_ERROR_NOTSUPP;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WMCS=<index>");
			break;

		case 1:
		{
			uint8_t mcs_index;

			if (atcmd_param_to_uint8(argv[0], &mcs_index) == 0)
			{
				if ((mcs_index >= 0 && mcs_index <= 7) || mcs_index == 10)
				{
					if (system_modem_api_set_mcs(0, mcs_index))
						break;

					return ATCMD_ERROR_FAIL;
				}
			}
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_mcs =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "MCS",
	.id = ATCMD_WIFI_MCS,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_mcs_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_mcs_set,
};

/**********************************************************************************************/

static int _atcmd_wifi_duty_cycle_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			uint32_t window, duration, margin;

			system_modem_api_get_duty_cycle(&window, &duration, &margin);

			ATCMD_MSG_INFO("WDUTYCYCLE", "%u,%u,%u", window, duration, margin);

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_duty_cycle_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WDUTYCYCLE=<window>[,<duration>[,<margin>]]");
			break;

		case 1:
		case 2:
		case 3:
		{
			union
			{
				uint32_t val[3];
				struct
				{
					uint32_t window;
					uint32_t duration;
					uint32_t margin;
				};
			} params;
			int i;

			system_modem_api_get_duty_cycle(&params.window, &params.duration, &params.margin);

			for (i = 0 ; i < argc ; i++)
			{
				if (atcmd_param_to_uint32(argv[i], &params.val[i]) != 0)
					return ATCMD_ERROR_INVAL;
			}

			if (params.window > 0)
			{
				if (system_modem_api_enable_duty_cycle(params.window, params.duration, params.margin))
					break;
			}
			else if (system_modem_api_disable_duty_cycle())
				break;

			return ATCMD_ERROR_FAIL;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_duty_cycle =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "DUTYCYCLE",
	.id = ATCMD_WIFI_DUTY_CYCLE,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_duty_cycle_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_duty_cycle_set,
};

/**********************************************************************************************/

static int _atcmd_wifi_cca_threshold_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			int threshold = system_modem_api_get_cca_threshold(0);

			ATCMD_MSG_INFO("WCCATHRESHOLD", "%d", threshold);

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_cca_threshold_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WCCATHRESHOLD=<threshold>");
			break;

		case 1:
		{
			int32_t threshold;

			if (atcmd_param_to_int32(argv[0], &threshold) != 0)
				return ATCMD_ERROR_INVAL;

			if (threshold < CCA_THRESHOLD_MIN || threshold > CCA_THRESHOLD_MAX)
				return ATCMD_ERROR_INVAL;

			if (!system_modem_api_set_cca_threshold(0, threshold))
				return ATCMD_ERROR_FAIL;

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_cca_threshold =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "CCATHRESHOLD",
	.id = ATCMD_WIFI_CCA_THRESHOLD,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_cca_threshold_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_cca_threshold_set,
};

/**********************************************************************************************/

static int _atcmd_wifi_tx_time_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			uint16_t cs_time = system_modem_api_get_cs_time();
			uint32_t pause_time = system_modem_api_get_tx_pause_time();

			ATCMD_MSG_INFO("WTXTIME", "%u,%u", cs_time, pause_time);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_tx_time_set (int argc, char *argv[])
{
	uint16_t cs_time;
	uint32_t pause_time;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WTXTIME=<cs_time>[,<pause_time>]");
			break;

		case 2:
			if (atcmd_param_to_uint32(argv[1], &pause_time) != 0)
				return ATCMD_ERROR_INVAL;

		case 1:
			if (atcmd_param_to_uint16(argv[0], &cs_time) != 0)
				return ATCMD_ERROR_INVAL;

			if (argc == 1)
				pause_time = system_modem_api_get_tx_pause_time();

			if (system_modem_api_set_tx_time(cs_time, pause_time))
				break;

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_tx_time =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "TXTIME",
	.id = ATCMD_WIFI_TX_TIME,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_tx_time_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_tx_time_set,
};

/**********************************************************************************************/

static int _atcmd_wifi_tsf_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			union
			{
				uint64_t val;

				struct
				{
					uint32_t low;
					uint32_t high;
				};
			} tsf;

			tsf.high = lmac_get_tsf_h(0);
			tsf.low = lmac_get_tsf_l(0);

/*			_atcmd_debug("TSF: high=%u(0x%08X) low=%u(0x%08X) val=%llu(0x%llX)\n",
							tsf.high, tsf.high, tsf.low, tsf.low, tsf.val, tsf.val); */

			ATCMD_MSG_INFO("WTSF", "%llu", tsf.val);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_tsf =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "TSF",
	.id = ATCMD_WIFI_TSF,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_tsf_get,
	.handler[ATCMD_HANDLER_SET] = NULL,
};

/**********************************************************************************************/

extern bool lmac_support_lbt (void);
extern int lmac_set_lbt(uint16_t cs_duration, uint32_t pause_time, uint32_t tx_resume_time);
extern int lmac_get_lbt(uint16_t *cs_duration, uint32_t *pause_time, uint32_t *tx_resume_time);

static int _atcmd_wifi_lbt_get (int argc, char *argv[])
{
	if (!lmac_support_lbt())
		return ATCMD_ERROR_NOTSUPP;

	switch (argc)
	{
		case 0:
		{
			uint16_t cs_duration;
			uint32_t pause_time;
			uint32_t tx_resume_time;

			if (lmac_get_lbt(&cs_duration, &pause_time, &tx_resume_time) != 0)
				return ATCMD_ERROR_FAIL;

			_atcmd_info("wifi_lbt_get: cs_duration=%u pause_time=%u tx_resume_time=%u\n",
					cs_duration, pause_time, tx_resume_time);

			ATCMD_MSG_INFO("WTSF", "%u,%u,%u", cs_duration, pause_time, tx_resume_time);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_lbt_set (int argc, char *argv[])
{
	if (!lmac_support_lbt())
		return ATCMD_ERROR_NOTSUPP;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WLBT=<cs_duration>,<pause_time>,<tx_resume_time>");
			break;

		case 3:
		{
			uint16_t cs_duration;
			uint32_t pause_time;
			uint32_t tx_resume_time;
			int i;

			for (i = 0 ; i < 3 ; i++)
			{
				if (atoi(argv[i]) < 0)
					return ATCMD_ERROR_INVAL;
			}

			cs_duration = atoi(argv[0]);
			pause_time = atoi(argv[1]);
			tx_resume_time = atoi(argv[2]);

			_atcmd_info("wifi_lbt_set: cs_duration=%u pause_time=%u tx_resume_time=%u\n",
								cs_duration, pause_time, tx_resume_time);

			if (lmac_set_lbt(cs_duration, pause_time, tx_resume_time) < 0)
				return ATCMD_ERROR_FAIL;
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}
static atcmd_info_t g_atcmd_wifi_lbt =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "LBT",
	.id = ATCMD_WIFI_LBT,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_lbt_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_lbt_set,
};

/**********************************************************************************************/

#ifndef CONFIG_ATCMD_WITHOUT_LWIP

static int _atcmd_wifi_ipaddr_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0: /* AT+WIPADDR? */
		case 1: /* AT+WDHCP */
		case 2: /* AT+WDHCPS */
		{
			struct netif *netif = nrc_netif[0];
			atcmd_wifi_ipaddr_t str_ipaddr[3];
			char param_ipaddr[3][ATCMD_STR_PARAM_SIZE(sizeof(atcmd_wifi_ipaddr_t))];
			int i;

			strcpy(str_ipaddr[0], ip4addr_ntoa(netif_ip4_addr(netif)));
			strcpy(str_ipaddr[1], ip4addr_ntoa(netif_ip4_netmask(netif)));
			strcpy(str_ipaddr[2], ip4addr_ntoa(netif_ip4_gw(netif)));

			for (i = 0 ; i < 3 ; i++)
			{
				if (!atcmd_str_to_param(str_ipaddr[i], param_ipaddr[i],
							ATCMD_STR_PARAM_SIZE(sizeof(atcmd_wifi_ipaddr_t))))
					return ATCMD_ERROR_FAIL;
			}

			switch (argc)
			{
				case 1:
					ATCMD_MSG_INFO("WDHCP", "%s,%s,%s",
								param_ipaddr[0], param_ipaddr[1], param_ipaddr[2]);
					break;

				case 2:
					ATCMD_MSG_INFO("WDHCPS", "%s,%s,%s",
								param_ipaddr[0], param_ipaddr[1], param_ipaddr[2]);
					break;

				default:
					ATCMD_MSG_INFO("WIPADDR", "%s,%s,%s",
								param_ipaddr[0], param_ipaddr[1], param_ipaddr[2]);
			}

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_ipaddr_set (int argc, char *argv[])
{
	char *param_ipaddr[3] = { NULL, NULL, NULL };

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WIPADDR=\"<IP>\",\"<netmask>\",\"<gateway>\"");
			break;

		case 3:
		{
			struct netif *netif = nrc_netif[0];
			atcmd_wifi_ipaddr_t str_ipaddr[3];
			ip4_addr_t ipaddr[3];
			int i;

			for (i = 0 ; i < 3 ; i++)
			{
				param_ipaddr[i] = argv[i];

				if (!atcmd_param_to_str(param_ipaddr[i], str_ipaddr[i], sizeof(atcmd_wifi_ipaddr_t)))
					return ATCMD_ERROR_FAIL;

				if (!ip4addr_aton(str_ipaddr[i], &ipaddr[i]))
					return ATCMD_ERROR_FAIL;
			}

			netif_set_ipaddr(netif, &ipaddr[0]);
			netif_set_netmask(netif, &ipaddr[1]);
			netif_set_gw(netif, &ipaddr[2]);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_ipaddr =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "IPADDR",
	.id = ATCMD_WIFI_IPADDR,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_ipaddr_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_ipaddr_set,
};

#endif /* #ifndef CONFIG_ATCMD_WITHOUT_LWIP */

/**********************************************************************************************/

#ifndef CONFIG_ATCMD_WITHOUT_LWIP

extern int nrc_wifi_dhcp_start(uint32_t timeout);

static int _atcmd_wifi_dhcp_run (int argc, char *argv[])
{
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
	atcmd_wifi_dhcp_t *dhcp = &g_atcmd_wifi_info->dhcp;
	uint32_t timeout_msec = _atcmd_timeout_value("WDHCP");
	int ret;

	if (!connect->connected)
	{
		_atcmd_info("wifi_dhcp: not connected\n");

		return ATCMD_ERROR_INVAL;
	}

	if (dhcp->requesting)
	{
		_atcmd_info("wifi_dhcp: busy\n");

		return ATCMD_ERROR_BUSY;
	}

	_atcmd_info("wifi_dhcp: run\n");

	dhcp->requesting = true;

	switch (nrc_wifi_dhcp_start(timeout_msec))
	{
		case WIFI_SUCCESS:
		{
			uint32_t time;

			for (time = 0 ; dhcp->requesting ; time += 100)
			{
				if (time > 0 && (time % 1000) == 0)
					_atcmd_info("wifi_dhcp: %u sec\n", time / 1000);

				if (timeout_msec > 0 && time >= timeout_msec)
				{
					_atcmd_wifi_event_poll(WIFI_EVT_GET_IP);
					_atcmd_wifi_event_poll(WIFI_EVT_GET_IP_FAIL);

					_atcmd_info("wifi_dhcp: timeout\n");

					return ATCMD_ERROR_TIMEOUT;
				}

				_delay_ms(100);
			}

			if (dhcp->responsed && _atcmd_wifi_ipaddr_get(1, NULL) == ATCMD_SUCCESS)
			{
				_atcmd_info("wifi_dhcp: done\n");

				return ATCMD_SUCCESS;
			}

			_atcmd_info("wifi_dhcp: fail\n");
			ret = ATCMD_ERROR_FAIL;
			break;
		}

		case WIFI_DHCP_FAIL:
			_atcmd_info("wifi_dhcp: disconnected\n");
			ret = ATCMD_ERROR_FAIL;
			break;

		case WIFI_DHCP_TIMEOUT:
			_atcmd_info("wifi_dhcp: timeout=%umsec\n", timeout_msec);
			ret = ATCMD_ERROR_TIMEOUT;
			break;

		default:
			_atcmd_info("wifi_dhcp: invalid\n");
			ret = ATCMD_ERROR_FAIL;
	}

	dhcp->requesting = false;

	return ret;
}

static atcmd_info_t g_atcmd_wifi_dhcp =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "DHCP",
	.id = ATCMD_WIFI_DHCP,

	.handler[ATCMD_HANDLER_RUN] = _atcmd_wifi_dhcp_run,
	.handler[ATCMD_HANDLER_GET] = NULL,
	.handler[ATCMD_HANDLER_SET] = NULL,
};

#endif /* #ifndef CONFIG_ATCMD_WITHOUT_LWIP */

/**********************************************************************************************/

static bool _atcmd_wifi_scan_freq_valid (uint16_t freq)
{
	atcmd_wifi_channels_t *channels = &g_atcmd_wifi_info->channels;
	int i;

	for (i = 0 ; i < channels->n_freq ; i++)
	{
		if (freq == channels->freq[i])
			return true;
	}

	return false;
}

static int _atcmd_wifi_scan_freq_get (uint16_t freq[], int n_freq_max)
{
	uint8_t n_freq;
	int net_id;
	int ret = -1;


	if (nrc_wifi_get_network_index(&net_id) != WIFI_SUCCESS)
	{
		_atcmd_info("scan_freq_get: failed get_network_index()\n");
		return -1;
	}

	if (net_id < 0)
	{
		atcmd_wifi_lock();

		if (nrc_wifi_add_network(&net_id) != WIFI_SUCCESS)
		{
			_atcmd_info("scan_freq_get: failed add_network()\n");
			atcmd_wifi_unlock();
			return -1;
		}

		atcmd_wifi_unlock();
	}

	if (nrc_wifi_get_scan_freq(net_id, freq, &n_freq) != WIFI_SUCCESS)
	{
		atcmd_wifi_channels_t *channels = &g_atcmd_wifi_info->channels;
		int i;

		_atcmd_info("scan_fre_get: full channels\n");

		for (i = 0 ; i < n_freq_max && i < channels->n_freq ; i++)
			freq[i] = channels->freq[i];

		n_freq = i;
	}

	if (n_freq > n_freq_max)
	{
		_atcmd_error("wifi_scan_get: n_freq(%d) > n_freq_max(%d)\n", n_freq, n_freq_max);
		return -1;
	}

	_atcmd_wifi_channels_print(freq, n_freq, "scan_freq_get");

	return n_freq;
}

static int _atcmd_wifi_scan_freq_compare (const void *f1, const void *f2)
{
	uint16_t _f1 = *(uint16_t *)f1;
	uint16_t _f2 = *(uint16_t *)f2;

	if (_f1 > _f2) return 1;
	else if (_f1 < _f2) return -1;
	else return 0;
}

static int _atcmd_wifi_scan_freq_set (uint16_t freq[], int n_freq)
{
	int net_id;
	int ret = -1;
	int i;

	if (freq && n_freq >= 2)
		qsort(freq, n_freq, sizeof(uint16_t), _atcmd_wifi_scan_freq_compare);

	for (i = 0 ; i < (n_freq - 1) ; i++)
	{
		if (freq[i] == freq[i + 1])
		{
			memcpy(&freq[i], &freq[i + 1], sizeof(uint16_t) * (n_freq - (i + 1)));
			n_freq--;
		}
	}

	_atcmd_wifi_channels_print(freq, n_freq, "scan_freq_set");

	if (nrc_wifi_get_network_index(&net_id) != WIFI_SUCCESS)
	{
		_atcmd_info("wifi_scan_set: failed get_network_index()\n");
		return -1;
	}

	if (net_id < 0)
	{
		atcmd_wifi_lock();

		if (nrc_wifi_add_network(&net_id) != WIFI_SUCCESS)
		{
			_atcmd_info("wifi_scan_set: failed add_network()\n");
			atcmd_wifi_unlock();
			return -1;
		}

		atcmd_wifi_unlock();
	}

	if (nrc_wifi_set_scan_freq(net_id, freq, n_freq) != WIFI_SUCCESS)
	{
		_atcmd_info("wifi_scan_set: failed\n");
		return -1;
	}

	return 0;
}

static int _atcmd_wifi_scan_freq_add (uint16_t freq[], int n_freq)
{
	uint16_t scan_freq[ATCMD_WIFI_CHANNELS_MAX];
	int n_scan_freq;
	int i, j;

	_atcmd_wifi_channels_print(freq, n_freq, "scan_freq_add");

	n_scan_freq = _atcmd_wifi_scan_freq_get(scan_freq, ATCMD_WIFI_CHANNELS_MAX);

	for (i = 0 ; i < n_freq ; i++)
	{
		for (j = 0 ; j < n_scan_freq ; j++)
		{
			if (freq[i] == scan_freq[j])
				break;
		}

		if (j < n_scan_freq)
			continue;

		_atcmd_info("scan_freq_add: %u\n", freq[i]);

		scan_freq[n_scan_freq] = freq[i];

		if (++n_scan_freq >= ATCMD_WIFI_CHANNELS_MAX)
		{
			_atcmd_info("scan_freq_add: full (%d)\n", n_scan_freq);
			break;
		}
	}

	return _atcmd_wifi_scan_freq_set(scan_freq, n_scan_freq);
}

static int _atcmd_wifi_scan_freq_delete (uint16_t freq[], int n_freq)
{
	uint16_t scan_freq[ATCMD_WIFI_CHANNELS_MAX];
	int n_scan_freq;
	int i, j;

	n_scan_freq = _atcmd_wifi_scan_freq_get(scan_freq, ATCMD_WIFI_CHANNELS_MAX);

	for (i = 0 ; i < n_freq ; i++)
	{
		for (j = 0 ; j < n_scan_freq ; j++)
		{
			if (freq[i] == scan_freq[j])
			{
				_atcmd_info("scan_freq_del: %u\n", freq[i]);

				for ( ; j < n_scan_freq ; j++)
					scan_freq[j] = scan_freq[j + 1];

				if (--n_scan_freq == 0)
				{
					_atcmd_info("scan_freq_del: empty\n");
					return -1;
				}

				break;
			}
		}
	}

	return _atcmd_wifi_scan_freq_set(scan_freq, n_scan_freq);
}

static int _atcmd_wifi_scan_result_report (SCAN_RESULTS *results, bool to_host)
{
	char param_bssid[ATCMD_STR_PARAM_SIZE(sizeof(atcmd_wifi_bssid_t))];
	char param_ssid[ATCMD_STR_PARAM_SIZE(sizeof(atcmd_wifi_ssid_t))];
	char param_flags[ATCMD_STR_PARAM_SIZE(ATCMD_WIFI_SCAN_FLAGS_LEN_MAX)];
	SCAN_RESULT *result;
	int i;

/*	memset(results, 0, sizeof(SCAN_RESULTS)); */

	if (nrc_wifi_scan_results(results) != WIFI_SUCCESS)
		return ATCMD_ERROR_FAIL;

	for (i = 0 ; i < results->n_result ; i++)
	{
		result = &results->result[i];

		if (strlen(result->flags) > 0 && result->flags[0] == '[')
		{
			int len = strlen(result->flags);

			memcpy(&result->flags[0], &result->flags[0], len);
			result->flags[len] = '\0';
		}

		if (!to_host)
			continue;

		if (atcmd_str_to_param(result->bssid, param_bssid, sizeof(param_bssid)) &&
				atcmd_str_to_param(result->flags, param_flags, sizeof(param_flags)) &&
				atcmd_str_to_param(result->ssid, param_ssid, sizeof(param_ssid)))
		{
			ATCMD_MSG_INFO("WSCAN", "%s,%.1f,%d,%s,%s",
							param_bssid, _atcmd_wifi_s1g_freq(atoi(result->freq)) / 10.,
							(int8_t)atoi(result->sig_level), param_flags, param_ssid);
		}
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_scan_run (int argc, char *argv[])
{
	bool wifi_roaming = (argc == -1) ? true : false;
	bool scan_retry = false;
	atcmd_wifi_scan_t *scan = &g_atcmd_wifi_info->scan;

	atcmd_wifi_lock();

	if (scan->scanning)
	{
		_atcmd_info("wifi_scan: busy\n");

		atcmd_wifi_unlock();

		return ATCMD_ERROR_BUSY;
	}

	_atcmd_info("wifi_scan: run\n");

_atcmd_wifi_scan_retry:

	scan->scanning = true;
	memset(&scan->results, 0, sizeof(SCAN_RESULTS));

	if (nrc_wifi_scan() == WIFI_SUCCESS)
	{
		uint32_t timeout_msec = _atcmd_timeout_value("WSCAN");
		uint32_t time;

		for (time = 0 ; scan->scanning ; time += 100)
		{
			if (time > 0 && (time % 1000) == 0)
				_atcmd_info("wifi_scan: %u sec\n", time / 1000);

			if (timeout_msec > 0 && time >= timeout_msec)
			{
				_atcmd_wifi_event_poll(WIFI_EVT_SCAN_DONE);

				_atcmd_info("wifi_scan: timeout\n");

				atcmd_wifi_unlock();

				return ATCMD_ERROR_TIMEOUT;
			}

			_delay_ms(100);
		}

		_atcmd_wifi_scan_result_report(&scan->results, wifi_roaming ? false : true);

		if (scan->results.n_result == 0 && !scan_retry)
		{
			scan_retry = true;

			_atcmd_info("wifi_scan: retry\n");

			goto _atcmd_wifi_scan_retry;
		}

		_atcmd_info("wifi_scan: done\n");

		atcmd_wifi_unlock();

		return ATCMD_SUCCESS;
	}

	scan->scanning = false;

	_atcmd_info("wifi_scan: fail\n");

	atcmd_wifi_unlock();

	return ATCMD_ERROR_FAIL;
}

static int _atcmd_wifi_scan_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			uint16_t freq[ATCMD_WIFI_CHANNELS_MAX];
			int n_freq;
			char buf[ATCMD_MSG_LEN_MAX + 1];
			int len = ATCMD_MSG_LEN_MAX;
			int cnt;
			int ret;
			int i;

			n_freq = _atcmd_wifi_scan_freq_get(freq, ATCMD_WIFI_CHANNELS_MAX);
			if (n_freq <= 0)
				return ATCMD_ERROR_FAIL;

			for (cnt = i = 0 ; i < n_freq ; i++)
			{
				ret = snprintf(&buf[ATCMD_MSG_LEN_MAX - len], len, "%3.1f,", freq[i] / 10.);
				if (ret <= 0)
					continue;

				len -= ret;

				if (++cnt == 10)
				{
					buf[ATCMD_MSG_LEN_MAX - len - 1] = '\0';
					ATCMD_MSG_INFO("WSCAN", "%s", buf);
					len = ATCMD_MSG_LEN_MAX;
					cnt = 0;
				}
			}

			if (cnt > 0)
			{
				buf[ATCMD_MSG_LEN_MAX - len -1] = '\0';
				ATCMD_MSG_INFO("WSCAN", "%s", buf);
			}

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_scan_set (int argc, char *argv[])
{
	atcmd_wifi_channels_t *channels = &g_atcmd_wifi_info->channels;
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;

	if (connect->connecting || connect->disconnecting || connect->connected)
		return ATCMD_ERROR_NOTSUPP;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WSCAN=[{+|-}]<s1g_freq>[,<s1g_freq> ...]");
			break;

		default:
		{
			int (*func[3]) (uint16_t *, int) =
			{
				_atcmd_wifi_scan_freq_set,
				_atcmd_wifi_scan_freq_add,
				_atcmd_wifi_scan_freq_delete,
			};
			uint16_t freq[ATCMD_WIFI_SCAN_SET_PARAM_MAX];
			float f_freq;
			int mode;
			int i, j;

			if (argc > ATCMD_WIFI_SCAN_SET_PARAM_MAX)
				return ATCMD_ERROR_INVAL;

			if (argc == 1 && strcmp(argv[0], "0") == 0)
			{
				_atcmd_info("wifi_scan_set: init\n");

				if (_atcmd_wifi_scan_freq_set(NULL, 0) != 0)
					return ATCMD_ERROR_FAIL;

				break;
			}

			switch (argv[0][0])
			{
				case '-': mode = 2; break;
				case '+': mode = 1; break;
				default:  mode = 0;
			}

			if (mode > 0)
				strcpy(&argv[0][0], &argv[0][1]);

			for (i = 0 ; i < argc ; i++)
			{
				if (atcmd_param_to_float(argv[i], &f_freq) != 0)
					return ATCMD_ERROR_INVAL;

				freq[i] = (uint16_t )(f_freq * 10);

				if (!_atcmd_wifi_scan_freq_valid(freq[i]))
					return ATCMD_ERROR_INVAL;
			}

			_atcmd_wifi_channels_print(freq, i, "wifi_scan_set");

			if (func[mode](freq, i) != 0)
				return ATCMD_ERROR_FAIL;
		}
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_scan =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "SCAN",
	.id = ATCMD_WIFI_SCAN,

	.handler[ATCMD_HANDLER_RUN] = _atcmd_wifi_scan_run,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_scan_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_scan_set,
};

/**********************************************************************************************/

static bool _atcmd_wifi_connect_valid_ap (char *ssid, char *bssid, tWIFI_SECURITY security)
{
	atcmd_wifi_scan_t *scan = &g_atcmd_wifi_info->scan;
	bool scanned_ap = false;
	int ssid_len, bssid_len;
	int i;

	if (!ssid || !bssid)
		return false;

	ssid_len = strlen(ssid);
	bssid_len = strlen(bssid);

	if (ssid_len == 0 && bssid_len == 0)
		return false;

	if (ssid_len > ATCMD_WIFI_SSID_LEN_MAX || (bssid_len > 0 && bssid_len != ATCMD_WIFI_BSSID_LEN))
		return false;

	switch (security)
	{
		case WIFI_SEC_OPEN:
		case WIFI_SEC_WPA2:
#if defined(CONFIG_ATCMD_WPA3)
		case WIFI_SEC_WPA3_OWE:
		case WIFI_SEC_WPA3_SAE:
#endif
			break;

		default:
			return false;
	}

	for (i = 0 ; i < scan->results.n_result && !scanned_ap ; i++)
	{
		if (ssid_len > 0 && strcmp(ssid, scan->results.result[i].ssid) != 0)
			continue;

		if (bssid_len > 0 && strcmp(bssid, scan->results.result[i].bssid) != 0)
			continue;

		if (strstr(scan->results.result[i].flags, "WPA2-PSK"))
			scanned_ap = (security == WIFI_SEC_WPA2) ? true : false;
#if defined(CONFIG_ATCMD_WPA3)
		else if (strstr(scan->results.result[i].flags, "WPA3-OWE"))
			scanned_ap = (security == WIFI_SEC_WPA3_OWE) ? true : false;
		else if (strstr(scan->results.result[i].flags, "WPA3-SAE"))
			scanned_ap = (security == WIFI_SEC_WPA3_SAE) ? true : false;
#endif
		else
			scanned_ap = (security == WIFI_SEC_OPEN) ? true : false;
	}

	if (!scanned_ap)
		return false;

	if (ssid_len == 0)
		strcpy(ssid, scan->results.result[i - 1].ssid);

	if (bssid_len == 0)
		strcpy(bssid, scan->results.result[i - 1].bssid);

	return true;
}

static int _atcmd_wifi_connect_run (int argc, char *argv[])
{
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
	bool event_poll = false;
	int net_id = -1;

	switch (argc)
	{
		case 2:
			net_id = *(int *)argv[1];

		case 1:
			event_poll = *(bool *)argv[0];

		case 0:
			break;

		default:
			_atcmd_info("wifi_connect: invalid argc\n");
			return ATCMD_ERROR_INVAL;
	}

/*	_atcmd_debug("wifi_connect: event_poll=%d net_id=%d\n", event_poll, net_id); */

	atcmd_wifi_lock();

	if (net_id < 0 && nrc_wifi_get_network_index(&net_id) != WIFI_SUCCESS)
		goto wifi_connect_fail;

	if (net_id < 0)
	{
		_atcmd_info("wifi_connect: add_network\n");

		if (nrc_wifi_add_network(&net_id) != WIFI_SUCCESS)
			goto wifi_connect_fail;
	}
	else if (connect->connecting || connect->connected)
	{
		_atcmd_info("wifi_connect: %s\n", connect->connected ? "connected" : "connecting");

		atcmd_wifi_unlock();

		return ATCMD_ERROR_BUSY;
	}

	if (net_id >= 0)
	{
		tWIFI_SECURITY security = _atcmd_wifi_security_string_to_index(connect->security);

		_atcmd_info("wifi_connect: %s %s %s\n", connect->ssid, connect->bssid, connect->security);

		if(strlen(connect->ssid) > 0 && nrc_wifi_set_ssid(net_id, connect->ssid) != WIFI_SUCCESS)
			goto wifi_connect_fail;

		if(strlen(connect->bssid) > 0 && nrc_wifi_set_bssid(net_id, connect->bssid) != WIFI_SUCCESS)
			goto wifi_connect_fail;

		if(nrc_wifi_set_security(net_id, security, connect->password) != WIFI_SUCCESS)
			goto wifi_connect_fail;

		connect->connected = false;
		connect->connecting = true;
		connect->disconnecting = false;

		if (event_poll)
		{
			_atcmd_info("wifi_connect: event poll\n");

			_atcmd_wifi_event_poll(WIFI_EVT_CONNECT_SUCCESS);
			_atcmd_wifi_event_poll(WIFI_EVT_CONNECT_FAIL);
		}
		else
		{
			_atcmd_wifi_event_polled(WIFI_EVT_CONNECT_SUCCESS);
			_atcmd_wifi_event_polled(WIFI_EVT_CONNECT_FAIL);
		}

		if(nrc_wifi_connect(net_id) != WIFI_SUCCESS)
		{
			connect->connecting = false;
			goto wifi_connect_fail;
		}

		if (connect->connecting)
		{
			uint32_t timeout_msec = _atcmd_timeout_value("WCONN");
			uint32_t time;

			_atcmd_info("wifi_connect: connecting\n");

			for (time = 0 ; connect->connecting ; time += 100)
			{
				if (time > 0 && (time % 1000) == 0)
					_atcmd_info("wifi_connect: %u sec\n", time / 1000);

				if (timeout_msec > 0 && time >= timeout_msec)
				{
					if (!event_poll)
					{
						_atcmd_wifi_event_poll(WIFI_EVT_CONNECT_SUCCESS);
						_atcmd_wifi_event_poll(WIFI_EVT_CONNECT_FAIL);
					}

					_atcmd_info("wifi_connect: timeout\n");

					atcmd_wifi_unlock();

					return ATCMD_ERROR_TIMEOUT;
				}

				_delay_ms(100);
			}
		}

		if (!connect->connected)
		{
			_atcmd_info("wifi_connect: not connected\n");
			goto wifi_connect_fail;
		}

		_atcmd_info("wifi_connect: done\n");

		atcmd_wifi_unlock();

		return ATCMD_SUCCESS;
	}

wifi_connect_fail:

	_atcmd_info("wifi_connect: fail\n");

	if (net_id >= 0)
	{
		if (nrc_wifi_remove_network(-1) != WIFI_SUCCESS)
			_atcmd_error("wifi_connect: failed nrc_wifi_remove_network()\n");
	}

	atcmd_wifi_unlock();

	return ATCMD_ERROR_FAIL;
}

static int _atcmd_wifi_connect_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			char param_ssid[ATCMD_STR_PARAM_SIZE(sizeof(atcmd_wifi_ssid_t))];
			char param_bssid[ATCMD_STR_PARAM_SIZE(sizeof(atcmd_wifi_bssid_t))];
			char param_security[ATCMD_STR_PARAM_SIZE(sizeof(atcmd_wifi_security_t))];
			char param_password[ATCMD_STR_PARAM_SIZE(sizeof(atcmd_wifi_password_t))];

			atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
			char *status = NULL;

			if (connect->disconnecting)
				status = "\"disconnecting\"";
			else if (connect->connecting)
				status = "\"connecting\"";
			else if (connect->connected)
				status = "\"connected\"";
			else
				status = "\"disconnected\"";

			if (atcmd_str_to_param(connect->ssid, param_ssid, sizeof(param_ssid)) &&
				atcmd_str_to_param(connect->bssid, param_bssid, sizeof(param_bssid)) &&
				atcmd_str_to_param(connect->security, param_security, sizeof(param_security)) &&
				atcmd_str_to_param(connect->password, param_password, sizeof(param_password)))
			{
				ATCMD_MSG_INFO("WCONN", "%s,%s,%s,%s,%s",
								param_ssid, param_bssid, param_security, param_password, status);

				break;
			}

			return ATCMD_ERROR_FAIL;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_connect_set (int argc, char *argv[])
{
	char *param_ssid = NULL;
	char *param_security = NULL;
	char *param_password = NULL;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WCONN=\"<ssid|bssid>\"[,\"<security>\"[,\"<password>\"]]");
			break;

		case 3:
			param_security = argv[1];
			param_password = argv[2];

		case 2:
			if (!param_security)
				param_security = argv[1];

		case 1:
		{
			tWIFI_SECURITY security = WIFI_SEC_OPEN;

			atcmd_wifi_ssid_t str_ssid;
			atcmd_wifi_bssid_t str_bssid;
			atcmd_wifi_security_t  str_security;
			atcmd_wifi_password_t str_password;

			atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;

			param_ssid = argv[0];

			if (!atcmd_param_to_str(param_ssid, str_ssid, sizeof(str_ssid)) ||
						strlen(str_ssid) > ATCMD_WIFI_SSID_LEN_MAX)
				return ATCMD_ERROR_INVAL;

			if (!_atcmd_wifi_bssid_valid(str_ssid))
				str_bssid[0] = '\0';
			else
			{
				strcpy(str_bssid, str_ssid);
				str_ssid[0] = '\0';
			}

			if (param_security)
			{
				if (!atcmd_param_to_str(param_security, str_security, sizeof(str_security)))
					return ATCMD_ERROR_INVAL;

				if (strcmp(str_security, "wpa2") == 0 || strcmp(str_security, "psk") == 0)
					strcpy(str_security, "wpa2-psk");
#if defined(CONFIG_ATCMD_WPA3)
				else if (strcmp(str_security, "owe") == 0)
					strcpy(str_security, "wpa3-owe");
				else if (strcmp(str_security, "sae") == 0)
					strcpy(str_security, "wpa3-sae");
#endif

				security = _atcmd_wifi_security_string_to_index(str_security);

				switch (security)
				{
					case WIFI_SEC_OPEN:
#if defined(CONFIG_ATCMD_WPA3)
					case WIFI_SEC_WPA3_OWE:
#endif
						break;

					case WIFI_SEC_WPA2:
#if defined(CONFIG_ATCMD_WPA3)
					case WIFI_SEC_WPA3_SAE:
#endif
						if (param_password)
							if (atcmd_param_to_str(param_password, str_password, sizeof(str_password)))
							{
								int len = strlen(str_password);

								if (len >= ATCMD_WIFI_PASSWORD_LEN_MIN && len <= ATCMD_WIFI_PASSWORD_LEN_MAX)
									break;
							}

					default:
						return ATCMD_ERROR_INVAL;
				}
			}
			else
			{
				strcpy(str_security, "open");
				strcpy(str_password, "");
			}

			if (_atcmd_wifi_connect_valid_ap(str_ssid, str_bssid, security))
			{
				strcpy(connect->ssid, str_ssid);
				strcpy(connect->bssid, str_bssid);
				strcpy(connect->security, str_security);
				strcpy(connect->password, str_password);

				return _atcmd_wifi_connect_run(0, NULL);
			}

			_atcmd_info("wifi_connect: no AP, ssid=%s bssid=%s security=%s\n",
										str_ssid, str_bssid, str_security);
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_connect =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "CONN",
	.id = ATCMD_WIFI_CONNECT,

	.handler[ATCMD_HANDLER_RUN] = _atcmd_wifi_connect_run,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_connect_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_connect_set,
};

/**********************************************************************************************/

static int _atcmd_wifi_disconnect_run (int argc, char *argv[])
{
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
	bool event_poll = false;
	int net_id = -1;

	switch (argc)
	{
		case 2:
			net_id = *(int *)argv[1];

		case 1:
			event_poll = *(bool *)argv[0];

		case 0:
			break;

		default:
			_atcmd_info("wifi_disconnect: invalid argc\n");
			return ATCMD_ERROR_INVAL;
	}

/*	_atcmd_debug("wifi_disconnect: event_poll=%d net_id=%d\n", event_poll, net_id); */

	atcmd_wifi_lock();

	if (net_id < 0 && nrc_wifi_get_network_index(&net_id) != WIFI_SUCCESS)
	{
		_atcmd_info("wifi_disconnect: fail\n");

		atcmd_wifi_unlock();

		return ATCMD_ERROR_FAIL;
	}

	if (net_id < 0)
	{
		_atcmd_info("wifi_disconnect: disconnected\n");

		connect->connected = false;
		connect->connecting = false;
		connect->disconnecting = false;

		atcmd_wifi_unlock();

		return ATCMD_SUCCESS;
	}

	if (connect->disconnecting)
	{
		_atcmd_info("wifi_disconnect: busy\n");

		atcmd_wifi_unlock();

		return ATCMD_ERROR_BUSY;
	}

	_atcmd_info("wifi_disconnect: run\n");

	connect->disconnecting = true;

	if (connect->connecting)
	{
		connect->connecting = false;

		_atcmd_wifi_event_polled(WIFI_EVT_CONNECT_SUCCESS);
		_atcmd_wifi_event_polled(WIFI_EVT_CONNECT_FAIL);
	}

	if (event_poll)
	{
		_atcmd_info("wifi_disconnect: event poll\n");

		_atcmd_wifi_event_poll(WIFI_EVT_DISCONNECT);
	}
	else
	{
		_atcmd_wifi_event_polled(WIFI_EVT_DISCONNECT);
	}

	if (nrc_wifi_disconnect(net_id) != WIFI_SUCCESS)
	{
		connect->disconnecting = false;

		_atcmd_info("wifi_disconnect: fail\n");

		atcmd_wifi_unlock();

		return ATCMD_ERROR_FAIL;
	}

	if (connect->connecting || connect->connected)
	{
		uint32_t timeout_msec = _atcmd_timeout_value("WDISCONN");
		uint32_t time;

		_atcmd_info("wifi_disconnect: disconnecting\n");

		for (time = 0 ; connect->disconnecting ; time += 100)
		{
			if (time > 0 && (time % 1000) == 0)
				_atcmd_info("wifi_disconnect: %u sec\n", time / 1000);

			if (timeout_msec > 0 && time >= timeout_msec)
			{
				if (!event_poll)
					_atcmd_wifi_event_poll(WIFI_EVT_DISCONNECT);

				_atcmd_info("wifi_disconnect: timeout\n");

				atcmd_wifi_unlock();

				return ATCMD_ERROR_TIMEOUT;
			}

			_delay_ms(100);
		}
	}

	if (connect->disconnecting)
	{
		connect->disconnecting = false;

		if (_atcmd_wifi_event_polling(WIFI_EVT_DISCONNECT))
			_atcmd_wifi_event_polled(WIFI_EVT_DISCONNECT);
	}

	_atcmd_info("wifi_disconnect: done\n");

	atcmd_wifi_unlock();

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_disconnect =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "DISCONN",
	.id = ATCMD_WIFI_DISCONNECT,

	.handler[ATCMD_HANDLER_RUN] = _atcmd_wifi_disconnect_run,
	.handler[ATCMD_HANDLER_GET] = NULL,
	.handler[ATCMD_HANDLER_SET] = NULL,
};

/**********************************************************************************************/

#define _wifi_roaming_log(fmt, ...)		_atcmd_info("ROAMING: " fmt "\n", ##__VA_ARGS__)
#define _wifi_roaming_debug(fmt, ...)	_atcmd_debug("ROAMING: " fmt "\n", ##__VA_ARGS__)

static int _atcmd_wifi_roaming_scan (void)
{
	atcmd_wifi_scan_t *scan = &g_atcmd_wifi_info->scan;

	_wifi_roaming_log("Scanning ...");

	switch (_atcmd_wifi_scan_run(-1, NULL))
	{
		case ATCMD_ERROR_FAIL:
			_wifi_roaming_log(" scan fail");
			break;

		case ATCMD_ERROR_BUSY:
		case ATCMD_ERROR_TIMEOUT:
		{
			const int timeout_msec = 100 * 100;
			int i;

			for (i = 0 ; i < timeout_msec ; i += 100)
			{
				_delay_ms(100);

				if (!scan->scanning)
					break;
			}

			if (scan->scanning)
			{
				_wifi_roaming_log(" scan timeout, %d-msec", timeout_msec);
				break;
			}
		}

		case ATCMD_SUCCESS:
			_wifi_roaming_log(" scan success, APs=%d", scan->results.n_result);

			if (scan->results.n_result > 0)
				return 0;
	}

	return -1;
}

static int _atcmd_wifi_roaming_connect (const char *bssid)
{
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
	bool event_poll = true;
	int net_id = -1;
	char *argv[2] = { (char *)&event_poll, (char *)&net_id };

	if (!bssid || strlen(bssid) != ATCMD_WIFI_BSSID_LEN)
	{
		_wifi_roaming_log("invalid bssid");
		return -1;
	}

	if (nrc_wifi_get_network_index(&net_id) != WIFI_SUCCESS)
		return -1;

	if (strcmp(bssid, connect->bssid) == 0)
		_wifi_roaming_log("Reconnecting to %s ...", connect->bssid);
	else
	{
		strcpy(connect->bssid, bssid);

		_wifi_roaming_log("Connecting to %s ...", connect->bssid);
	}

/*	_wifi_roaming_debug(" - ssid: %s", connect->ssid);
	_wifi_roaming_debug(" - security: %s", connect->security);
	_wifi_roaming_debug(" - status: connected=%d connecting=%d disconnecting=%d",
						connect->connected, connect->connecting, connect->disconnecting); */

	switch (_atcmd_wifi_connect_run(2, argv))
	{
		case ATCMD_ERROR_BUSY:
			_wifi_roaming_log(" already connecting(%d) or connected(%d)",
								connect->connecting, connect->connected);
			break;

		case ATCMD_ERROR_FAIL:
			_wifi_roaming_log(" connect fail");
			break;

		case ATCMD_ERROR_TIMEOUT:
		{
			const int timeout_msec = 100 * 100;
			int i;

			for (i = 0 ; i < timeout_msec ; i += 100)
			{
				_delay_ms(100);

				if (connect->connected)
					break;
			}

			if (connect->connecting)
			{
				_wifi_roaming_log(" connect timeout, %d-msec", timeout_msec);
				break;
			}
		}

		case ATCMD_SUCCESS:
			_wifi_roaming_log(" connect success");
			return 0;
	}

	return -1;
}

static int _atcmd_wifi_roaming_disconnect (void)
{
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
	bool event_poll = true;
	int net_id = -1;
	char *argv[2] = { (char *)&event_poll, (char *)&net_id };

	if (nrc_wifi_get_network_index(&net_id) != WIFI_SUCCESS)
		return -1;

	if (net_id < 0)
	{
		_wifi_roaming_debug(" network removed");
		return 0;
	}

	_wifi_roaming_log("Disconnecting from %s ...", connect->bssid);
/*	_wifi_roaming_debug(" - ssid: %s", connect->ssid);
	_wifi_roaming_debug(" - security: %s", connect->security);
	_wifi_roaming_debug(" - status: connected=%d connecting=%d disconnecting=%d",
						connect->connected, connect->connecting, connect->disconnecting); */

	switch (_atcmd_wifi_disconnect_run(2, argv))
	{
		case ATCMD_ERROR_FAIL:
			_wifi_roaming_log(" disconnect fail");
			break;

		case ATCMD_ERROR_BUSY:
		case ATCMD_ERROR_TIMEOUT:
		{
			int timeout_msec = 100 *100;
			int i;

			for (i = 0 ; i < timeout_msec ; i += 100)
			{
				_delay_ms(100);

				if (!connect->disconnecting)
					break;
			}

			if (connect->disconnecting)
			{
				_wifi_roaming_log(" disconnect timeout, %d-msec", timeout_msec);
				break;
			}
		}

		case ATCMD_SUCCESS:
			_wifi_roaming_log(" disconnect success");
			return 0;
	}

	return -1;
}

static bool _atcmd_wifi_roaming_valid_params (atcmd_wifi_roaming_params_t *params)
{
	if (!params)
		return false;
	else if (params->scan_delay == 0)
		return true;
	else if (params->scan_delay < 0)
		return false;
	else if (!(params->rssi_threshold >= -100 && params->rssi_threshold <= 0))
		return false;
	else if (!(params->rssi_level > 0 && params->rssi_level <= 100))
		return false;

	return true;
}

static int _atcmd_wifi_roaming_check_status (bool *connected)
{
	atcmd_wifi_scan_t *scan = &g_atcmd_wifi_info->scan;
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
	atcmd_wifi_roaming_params_t *params = &g_atcmd_wifi_info->roaming.params;

	_wifi_roaming_log("Checking ...");

	if (scan->scanning)
	{
		_wifi_roaming_log(" scanning");
		return -1;
	}
	if (connect->connecting)
	{
		_wifi_roaming_log(" connecting");
		return -1;
	}
	if (connect->disconnecting)
	{
		_wifi_roaming_log(" disconnecting");
		return -1;
	}
	else if (!connect->connected)
	{
		_wifi_roaming_log(" disconnected");

		_atcmd_wifi_roaming_disconnect();

		*connected = false;
	}
	else
	{
		const bool check_rssi = false;

		_wifi_roaming_log(" connected");

		if (check_rssi)
		{
			int8_t rssi;

			if (nrc_wifi_get_rssi(&rssi) == WIFI_SUCCESS)
			{
				_wifi_roaming_log(" rssi=%d threshold=%d", rssi, params->rssi_threshold);

				if (rssi > params->rssi_threshold)
					return -1;
			}
		}

		*connected = true;
	}

	return 0;
}

static bool _atcmd_wifi_roaming_valid_ap (SCAN_RESULT *ap,
										atcmd_wifi_ssid_t ssid,
										atcmd_wifi_security_t security)
{
	if (strcmp(ap->ssid, ssid) != 0)
		return false;

	if (!_atcmd_wifi_bssid_valid(ap->bssid))
		return false;

	if (!_atcmd_wifi_rssi_valid(atoi(ap->sig_level)))
		return false;

	if (strstr(ap->flags, "WPA2-PSK"))
	{
		if (strcmp(security, "wpa2-psk") != 0)
			return false;
	}
	else if (strstr(ap->flags, "WPA3-OWE"))
	{
		if (strcmp(security, "wpa3-owe") != 0)
			return false;
	}
	else if (strstr(ap->flags, "WPA3-SAE"))
	{
		if (strcmp(security, "wpa3-sae") != 0)
			return false;
	}
	else if (strcmp(security, "open") != 0)
		return false;

	return true;
}

static SCAN_RESULT *_atcmd_wifi_roaming_compare_ap (SCAN_RESULT *ap1, SCAN_RESULT *ap2)
{
	int rssi1 = ap1 ? atoi(ap1->sig_level) : ATCMD_WIFI_RSSI_MIN;
	int rssi2 = ap2 ? atoi(ap2->sig_level) : ATCMD_WIFI_RSSI_MIN;

/*	_wifi_roaming_debug(" compare_ap: rssi1=%d rssi2=%d", rssi1, rssi2); */

	return (rssi1 >= rssi2) ? ap1 : ap2;
}

static int _atcmd_wifi_roaming_select_ap (char *bssid)
{
	atcmd_wifi_scan_t *scan = &g_atcmd_wifi_info->scan;
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
	atcmd_wifi_roaming_t *roaming = &g_atcmd_wifi_info->roaming;

	SCAN_RESULT *scan_result, *cur_ap, *good_ap;
	int i;

	_wifi_roaming_log("Selecting ...");

	cur_ap = good_ap = NULL;

	for (i = 0 ; i < scan->results.n_result ; i++)
	{
		scan_result = &scan->results.result[i];

/*		_wifi_roaming_debug(" scn_ap: %s %s %s %d %s",
							scan_result->ssid, scan_result->bssid,
							scan_result->freq, (int8_t)atoi(scan_result->sig_level),
							scan_result->flags); */

		if (!_atcmd_wifi_roaming_valid_ap(scan_result, connect->ssid, connect->security))
			continue;

/*		_wifi_roaming_debug(" val_ap: %s %s %s %d %s",
							scan_result->ssid, scan_result->bssid,
							scan_result->freq, (int8_t)atoi(scan_result->sig_level),
							scan_result->flags); */

		if (strcmp(scan_result->bssid, connect->bssid) == 0)
		{
			if (!cur_ap)
				cur_ap = scan_result;
			else
			{
				_wifi_roaming_log(" cur_ap: duplicate, %s %s %s %d %s",
							scan_result->ssid, scan_result->bssid,
							scan_result->freq, (int8_t)atoi(scan_result->sig_level),
							scan_result->flags);
			}
		}

		good_ap = _atcmd_wifi_roaming_compare_ap(good_ap, scan_result);
	}

	if (cur_ap)
	{
		_wifi_roaming_log(" cur_ap: %s %s %s %d %s",
							cur_ap->ssid, cur_ap->bssid,
							cur_ap->freq, (int8_t)atoi(cur_ap->sig_level),
							cur_ap->flags);
	}

	if (cur_ap)
	{
		if (!good_ap)
			good_ap = cur_ap;
		else if (good_ap != cur_ap)
		{
			int rssi1 = atoi(cur_ap->sig_level);
			int rssi2 = atoi(good_ap->sig_level);

			if (rssi1 > roaming->params.rssi_threshold)
				good_ap = cur_ap;
			else if ((rssi2 - rssi1) < roaming->params.rssi_level)
				good_ap = cur_ap;
		}
	}

	if (!good_ap)
	{
		_wifi_roaming_log(" no APs");
		return -1;
	}
	else if (good_ap != cur_ap)
	{
		_wifi_roaming_log(" good_ap: %s %s %s %d %s",
							good_ap->ssid, good_ap->bssid,
							good_ap->freq, (int8_t)atoi(good_ap->sig_level),
							good_ap->flags);
	}

	strcpy(bssid, good_ap->bssid);

	return 0;
}

static void _atcmd_wifi_roaming_task (void *pvParameters)
{
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
	atcmd_wifi_roaming_t *roaming = &g_atcmd_wifi_info->roaming;
	atcmd_wifi_bssid_t bssid;
	bool connected = false;
	int i;

	_wifi_roaming_log("Task Run");

	memset(bssid, 0, sizeof(atcmd_wifi_bssid_t));

	while (roaming->enable)
	{
		_delay_ms(roaming->params.scan_delay);

		if (!roaming->enable)
			break;

		if (_atcmd_wifi_roaming_check_status(&connected) != 0)
			continue;

		if (_atcmd_wifi_roaming_scan() != 0)
			continue;

		if (_atcmd_wifi_roaming_select_ap(bssid) != 0)
			continue;

		if (connected)
		{
			if (strcmp(bssid, connect->bssid) == 0)
				continue;

			if (_atcmd_wifi_roaming_disconnect() != 0)
				continue;
		}

		_atcmd_wifi_roaming_connect(bssid);
	}

	_wifi_roaming_log("Task Exit");

	roaming->task = NULL;
	memset(&roaming->params, 0, sizeof(atcmd_wifi_roaming_params_t));

	vTaskDelete(NULL);
}

static int _atcmd_wifi_roaming_enable (atcmd_wifi_roaming_params_t *params)
{
	atcmd_wifi_roaming_t *roaming = &g_atcmd_wifi_info->roaming;

	if (roaming->enable)
		return ATCMD_ERROR_BUSY;

	_wifi_roaming_log("Enable, scan_delay=%d rssi_threshold=%d rssi_level=%d",
				params->scan_delay, params->rssi_threshold, params->rssi_level);

	roaming->enable = true;
	memcpy(&roaming->params, params, sizeof(atcmd_wifi_roaming_params_t));

	if (!roaming->task)
	{
		if (xTaskCreate(_atcmd_wifi_roaming_task, "atcmd_wifi_roaming",
						ATCMD_WIFI_ROAMING_TASK_STACK_SIZE, NULL,
						ATCMD_WIFI_ROAMING_TASK_PRIORITY,
						&roaming->task) != pdPASS)
			goto _atcmd_wifi_roaming_enable_fail;
	}

	return ATCMD_SUCCESS;

_atcmd_wifi_roaming_enable_fail:

	roaming->task = NULL;
	roaming->enable = false;
	memset(&roaming->params, 0, sizeof(atcmd_wifi_roaming_params_t));

	return ATCMD_ERROR_FAIL;
}

static int _atcmd_wifi_roaming_disable (void)
{
	atcmd_wifi_roaming_t *roaming = &g_atcmd_wifi_info->roaming;

	_wifi_roaming_log("Disable");

	roaming->enable = false;

	if (roaming->task)
		xTaskAbortDelay(roaming->task);

	return 0;
}

static int _atcmd_wifi_roaming_get (int argc, char *argv[])
{
	atcmd_wifi_roaming_params_t *params = &g_atcmd_wifi_info->roaming.params;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_INFO("WROAM", "%d,%d,%d",
					params->scan_delay, params->rssi_threshold, params->rssi_level);
			break;

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_roaming_set (int argc, char *argv[])
{
	atcmd_wifi_roaming_params_t params;

	memset(&params, 0, sizeof(atcmd_wifi_roaming_params_t));

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WROAM=<scan_delay>,<rssi_threshold>,<rssi_level>"); /* enable */
			ATCMD_MSG_HELP("AT+WROAM=0"); /* disable */
			break;

		case 3:
			params.rssi_threshold = atoi(argv[1]);
			params.rssi_level = atoi(argv[2]);

		case 1:
			params.scan_delay = atoi(argv[0]);

			if (_atcmd_wifi_roaming_valid_params(&params))
			{
				if (params.scan_delay == 0)
					return _atcmd_wifi_roaming_disable();
				else if (!g_atcmd_wifi_info->connect.connected)
				{
					_wifi_roaming_log("!!! Disconnected !!!");

					return ATCMD_ERROR_FAIL;
				}

				return _atcmd_wifi_roaming_enable(&params);
			}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_roaming =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "ROAM",
	.id = ATCMD_WIFI_ROAMING,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_roaming_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_roaming_set,
};

/**********************************************************************************************/

#ifndef CONFIG_ATCMD_WITHOUT_LWIP

static void _atcmd_wifi_ping_report_callback (_lwip_ping_report_t *report)
{
	char param_remote_ip[2 + ATCMD_IP4_ADDR_LEN_MAX];

	if (!atcmd_str_to_param(ip4addr_ntoa(&report->remote_ip),
							param_remote_ip, sizeof(param_remote_ip)))
		return;

	switch (report->status)
	{
		case _LWIP_PING_SUCCESS:
			ATCMD_MSG_WEVENT("\"PING\",%u,%s,%u,%u,%u",
							report->data_size, param_remote_ip,
							report->icmp_seq, report->ttl, report->resp_time);
							break;

		case _LWIP_PING_SEND_FAIL:
		case _LWIP_PING_RECV_TIMEOUT:
			ATCMD_MSG_WEVENT("\"PING\",%u,%s,%u,%s",
							report->data_size, param_remote_ip, report->icmp_seq,
							report->status == _LWIP_PING_SEND_FAIL ? "\"send_fail\"" : "\"recv_timeout\"");
	}
}

static int _atcmd_wifi_ping_run (int argc, char *argv[])
{
	if (g_atcmd_wifi_info->connect.connected || g_atcmd_wifi_info->softap.active)
	{
		atcmd_wifi_ping_t *ping = &g_atcmd_wifi_info->ping;

		if (ip4_addr_get_u32(&ping->remote_ip) == IPADDR_ANY)
		{
			struct netif *netif = nrc_netif[0];
			ip4_addr_t ip4addr_gw;

			memcpy(&ip4addr_gw, netif_ip4_gw(netif), sizeof(ip4_addr_t));

			if (ip4_addr_get_u32(&ip4addr_gw) == IPADDR_ANY)
				return ATCMD_ERROR_FAIL;

			ip4_addr_copy(ping->remote_ip, ip4addr_gw);
		}

		if (_lwip_ping_start(ping) == 0)
			return ATCMD_SUCCESS;
	}

	return ATCMD_ERROR_FAIL;
}

static int _atcmd_wifi_ping_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			atcmd_wifi_ping_t *ping = &g_atcmd_wifi_info->ping;
			char param_remote_ip[2 + ATCMD_IP4_ADDR_LEN_MAX];

			if (!atcmd_str_to_param(ip4addr_ntoa(&ping->remote_ip),
									param_remote_ip, sizeof(param_remote_ip)))
				return ATCMD_ERROR_FAIL;

			ATCMD_MSG_INFO("WPING", "%s,%u,%u,%u",
							param_remote_ip,
							ping->interval, ping->count,
							ping->data_size);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_ping_set (int argc, char *argv[])
{
	char *param_remote_ip = NULL;
	char *param_count = NULL;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WPING=\"<remote_IP>\"[,<count>]");
			break;

		case 2:
			param_count = argv[1];

		case 1:
		{
			char str_remote_ip[ATCMD_STR_SIZE(ATCMD_IP4_ADDR_LEN_MAX)];
			atcmd_wifi_ping_t *ping = &g_atcmd_wifi_info->ping;
			ip4_addr_t remote_ip;

			param_remote_ip = argv[0];

			if (!atcmd_param_to_str(param_remote_ip, str_remote_ip, sizeof(str_remote_ip)) ||
					!ip4addr_aton(str_remote_ip, &remote_ip))
				return ATCMD_ERROR_INVAL;

			ip4_addr_copy(ping->remote_ip, remote_ip);

			if (param_count)
				ping->count = atoi(param_count);
			else
				ping->count = ATCMD_WIFI_INIT_PING_COUNT;

			ping->data_size = ATCMD_WIFI_INIT_PING_SIZE;

/*			_atcmd_debug("remote_ip=%s interval=%u count=%u size=%u",
							ip4addr_ntoa(&ping->remote_ip),
							ping->interval,
							ping->count,
							ping->data_size); */

			return _atcmd_wifi_ping_run(0, NULL);
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_ping =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "PING",
	.id = ATCMD_WIFI_PING,

	.handler[ATCMD_HANDLER_RUN] = _atcmd_wifi_ping_run,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_ping_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_ping_set,
};

#endif /*#ifndef CONFIG_ATCMD_WITHOUT_LWIP */

/**********************************************************************************************/

#ifndef CONFIG_ATCMD_WITHOUT_LWIP

#include "atcmd_fota.h"

static void _atcmd_wifi_fota_event_callback (atcmd_fota_event_t *event)
{
	if (!event)
		return;

	switch (event->type)
	{
		case FOTA_EVT_VERSION:
			ATCMD_MSG_WEVENT("\"%s\",\"%s\",\"%s\"", "FOTA_VERSION",
								event->version.sdk, event->version.atcmd);
			break;

		case FOTA_EVT_BINARY:
			ATCMD_MSG_WEVENT("\"%s\",\"%s\"", "FOTA_BINARY",
								event->binary.name);
			break;

		case FOTA_EVT_DOWNLOAD:
			ATCMD_MSG_WEVENT("\"%s\",%u,%u", "FOTA_DOWNLOAD",
								event->download.total, event->download.len);
			break;

		case FOTA_EVT_UPDATE:
			ATCMD_MSG_WEVENT("\"%s\"", "FOTA_UPDATE");
			break;

		case FOTA_EVT_FAIL:
			ATCMD_MSG_WEVENT("\"%s\"", "FOTA_FAIL");
			break;

		default:
			break;
	}
}

static int _atcmd_wifi_fota_run (int argc, char *argv[])
{
	if (atcmd_fota_update_firmware() == 0)
		return ATCMD_SUCCESS;

	return ATCMD_ERROR_FAIL;
}

static int _atcmd_wifi_fota_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			atcmd_fota_params_t params;

			atcmd_fota_get_params(&params);

			ATCMD_MSG_INFO("WFOTA", "%d,\"%s\",\"%s\",0x%X",
				params.check_time, params.server_url, params.bin_name, params.bin_crc32);

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_fota_set (int argc, char *argv[])
{
	char *param_check_time = NULL;
	char *param_server_url = NULL;
	char *param_bin_name = NULL;
	char *param_bin_crc32 = NULL;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WFOTA=<check_time>[,\"<server_url>\"[,\"<bin_name>\",<bin_crc32>]]");
			break;

		case 4:
			param_bin_name = argv[2];
			param_bin_crc32 = argv[3];

		case 2:
			param_server_url = argv[1];

		case 1:
		{
			const enum FW_BIN fw_bin_type[_HIF_TYPE_NUM] =
			{
				[_HIF_TYPE_HSPI] = FW_BIN_HSPI,
				[_HIF_TYPE_UART] = FW_BIN_UART,
				[_HIF_TYPE_UART_HFC] = FW_BIN_UART_HFC,
			};

			atcmd_fota_params_t params;
			int ret;

			param_check_time = argv[0];

			params.fw_bin_type = fw_bin_type[_hif_get_type()];
			params.event_cb = _atcmd_wifi_fota_event_callback;

			params.check_time = 0;
			strcpy(params.server_url, "");
			strcpy(params.bin_name, "");
			params.bin_crc32 = 0;

			if (param_check_time && atcmd_param_to_int32(param_check_time, &params.check_time) != 0)
				return ATCMD_ERROR_INVAL;

			if (param_server_url &&
				atcmd_param_to_str(param_server_url, params.server_url, sizeof(params.server_url)) == NULL)
				return ATCMD_ERROR_INVAL;

			if (param_bin_name &&
				atcmd_param_to_str(param_bin_name, params.bin_name, sizeof(params.bin_name)) == NULL)
				return ATCMD_ERROR_INVAL;

			if (param_bin_crc32 && atcmd_param_to_hex(param_bin_crc32, &params.bin_crc32) != 0)
				return ATCMD_ERROR_INVAL;

			switch (atcmd_fota_set_params(&params))
			{
				case 0:
					break;

				case -EINVAL:
					return ATCMD_ERROR_INVAL;

				default:
					return ATCMD_ERROR_FAIL;
			}

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_fota =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "FOTA",
	.id = ATCMD_WIFI_FOTA,

	.handler[ATCMD_HANDLER_RUN] = _atcmd_wifi_fota_run,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_fota_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_fota_set,
};

#endif /* #ifndef CONFIG_ATCMD_WITHOUT_LWIP */

/**********************************************************************************************/

/* system/vendor/vendor_atcmd.c */
extern int nrc_ps_event_user_get (enum ps_event event);
extern int nrc_ps_event_user_set (enum ps_event event);
extern int nrc_ps_event_user_clear (enum ps_event event);

static void _atcmd_wifi_sleep_send_event (void)
{
	if (nrc_ps_event_user_get(PS_EVT_WAKEUP_DEEPSLEEP) == 1)
	{
		nrc_ps_event_user_clear(PS_EVT_WAKEUP_DEEPSLEEP);

		_atcmd_info("wifi_event: deepsleep_wakeup\n");

		ATCMD_MSG_WEVENT("\"DEEPSLEEP_WAKEUP\"");
	}
}

static int _atcmd_wifi_sleep_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			struct
			{
				bool enable;
				uint32_t timeout;
			} modem_sleep;

			modem_sleep.enable = lmac_ps_is_modem_sleep_mode();
			modem_sleep.timeout = lmac_ps_get_iw_power_timeout();

			_atcmd_info("wifi_sleep: modem_sleep=%s timeout=%u\n",
							modem_sleep.enable ? "on" : "off", modem_sleep.timeout);

			ATCMD_MSG_INFO("WSLEEP", "%d,%u", modem_sleep.enable, modem_sleep.timeout);

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

#if 1 /* Don't change */
/*
 * MAC Layer API
 */
extern bool lmac_send_qos_null_frame (bool pm);

static int _atcmd_wifi_sleep_set (int argc, char *argv[])
{
	char *param_timeout = NULL;
	char *param_gpio = NULL;

	if (!g_atcmd_wifi_info->connect.connected)
	{
		_atcmd_info("wifi_sleep: not connected\n");
		return ATCMD_ERROR_FAIL;
	}

	switch (argc)
	{
		case 0:
			/*
			 * 	mode
			 *    - 0: active
			 *    - 1: modem sleep
			 *    - 2: deep sleep (tim mode)
			 *    - 3: deep sleep (nontim mode)
			 *  timeout (msec)
			 *    - modem sleep : active timeout
			 *    - deep sleep : sleep timeout for nontim mode
			 * 	gpio : GPIO pin number to wake up from deep sleep (8 ~ 17)
			 */
			ATCMD_MSG_HELP("AT+WSLEEP=<mode>[,<timeout>|<gpio>|<timeout>[,<gpio>]]");
			break;

		case 3:
			param_gpio = argv[2];

		case 2:
			param_timeout = argv[1];
			if (!param_gpio)
				param_gpio = argv[1];

		case 1:
		{
			int wakeup_source = WKUP_SRC_RTC;
			int mode = atoi(argv[0]);
			int timeout = 0;
			int gpio = -1;

			switch (mode)
			{
				case 0:
					_atcmd_info("wifi_sleep: active\n");
					lmac_ps_modemsleep_stop();
					return ATCMD_SUCCESS;

				case 1:
					SET_PS_SLEEP_MODE(mode, PS_SLEEP_MODE_MODEM);
					SET_PS_TIM_MODE(mode, PS_TIM_MODE);

					param_gpio = NULL;
					break;

				case 2:
					SET_PS_SLEEP_MODE(mode, PS_SLEEP_MODE_DEEP);
					SET_PS_TIM_MODE(mode, PS_TIM_MODE);

					param_timeout = NULL;
					break;

				case 3:
					SET_PS_SLEEP_MODE(mode, PS_SLEEP_MODE_DEEP);
					SET_PS_TIM_MODE(mode, PS_NON_TIM_MODE);

					if (argc == 2)
						param_gpio = NULL;
					break;

				default:
					_atcmd_info("wifi_sleep: invalid mode\n");
					return ATCMD_ERROR_INVAL;
			}

			if (param_timeout)
			{
				timeout = atoi(param_timeout);
				if (timeout < 0 || (timeout == 0 && GET_PS_SLEEP_MODE(mode) == PS_SLEEP_MODE_MODEM))
				{
					_atcmd_info("wifi_sleep: invalid timeout\n");
					return ATCMD_ERROR_INVAL;
				}
			}
			else if (GET_PS_SLEEP_MODE(mode) == PS_SLEEP_MODE_MODEM)
			{
				_atcmd_info("wifi_sleep: no timeout\n");
				return ATCMD_ERROR_INVAL;
			}

			if (param_gpio)
			{
				gpio = atoi(param_gpio);

				if (gpio < GPIO_08 || gpio > GPIO_17)
				{
					_atcmd_info("wifi_sleep: invalid gpio\n");
					return ATCMD_ERROR_INVAL;
				}

				wakeup_source |= WKUP_SRC_GPIO;
			}

			_atcmd_info("wifi_sleep: mode=%d timeout=%d gpio=%d, wakeup_source=0x%X\n",
									mode, timeout, gpio, wakeup_source);

			if (nrc_ps_config_wakeup_source(wakeup_source) != 0)
			{
				_atcmd_error("wifi_sleep: failed to set wakeup source\n");
				return ATCMD_ERROR_FAIL;
			}

			switch (GET_PS_SLEEP_MODE(mode))
			{
				case PS_SLEEP_MODE_MODEM:
					if (timeout == 0)
						timeout = lmac_ps_get_iw_power_timeout();

					_atcmd_info("wifi_sleep: modem sleep, timeout=%d\n", timeout);

					if (lmac_ps_set_iw_power_timeout(timeout) != 0)
					{
						_atcmd_error("wifi_sleep: failed to set modem sleep timeout\n");
						return ATCMD_ERROR_FAIL;
					}

					if (lmac_ps_set_sleep(mode, 0) != 0)
					{
						_atcmd_error("wifi_sleep: failed to set mode sleep mode\n");
						return ATCMD_ERROR_FAIL;
					}
					break;

				case PS_SLEEP_MODE_DEEP:
				{
					bool debounce = false;

					_atcmd_info("wifi_sleep: deep sleep, timeout=%d gpio=%d debounce=%d\n",
											timeout, gpio, debounce);

					if (gpio >= 0 && nrc_ps_config_wakeup_pin(debounce, gpio) != 0)
					{
						_atcmd_error("wifi_sleep: failed to set wakeup pin\n");
						return ATCMD_ERROR_FAIL;
					}
#if 0
					if (lmac_ps_set_sleep(mode, timeout) != 0)
					{
						_atcmd_error("wifi_sleep: failed to set deep sleep mode\n");
						return ATCMD_ERROR_FAIL;
					}
#else
					if (GET_PS_TIM_MODE(mode) == PS_NON_TIM_MODE)
						lmac_ps_set_ps_mode(PS_PM_DEEPSLEEP_NONTIM, timeout);
					else
						lmac_ps_set_ps_mode(PS_PM_DEEPSLEEP_TIM, 0);

					lmac_send_qos_null_frame(true);
#endif
				}
			}

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

#else
/*
 * User Layer API
 */

static int _atcmd_wifi_sleep_set (int argc, char *argv[])
{
	char *param_timeout = NULL;
	char *param_gpio = NULL;

	if (!g_atcmd_wifi_info->connect.connected)
	{
		_atcmd_info("wifi_sleep: not connected\n");
		return ATCMD_ERROR_FAIL;
	}

	switch (argc)
	{
		case 0:
			/*
			 * 	mode
			 *    - 0: active
			 *    - 1: modem sleep
			 *    - 2: deep sleep (tim mode)
			 *    - 3: deep sleep (nontim mode)
			 *  timeout (msec)
			 *    - modem sleep : active timeout
			 *    - deep sleep : sleep timeout for nontim mode
			 * 	gpio : GPIO pin number to wake up from deep sleep (8 ~ 17)
			 */
			ATCMD_MSG_HELP("AT+WSLEEP=<mode>[,<timeout>|<gpio>|<timeout>[,<gpio>]]");
			break;

		case 3:
			param_gpio = argv[2];

		case 2:
			param_timeout = argv[1];
			if (!param_gpio)
				param_gpio = argv[1];

		case 1:
		{
			int wakeup_source = WAKEUP_SOURCE_RTC;
			int mode = atoi(argv[0]);
			int timeout = 0;
			int gpio = -1;

			switch (mode)
			{
				case 0:
					_atcmd_info("wifi_sleep: active\n");
					lmac_ps_modemsleep_stop();
					return ATCMD_SUCCESS;

				case 1:
					mode = POWER_SAVE_MODEM_SLEEP_MODE;
					param_gpio = NULL;
					break;

				case 2:
					mode = POWER_SAVE_DEEP_SLEEP_MODE;
					param_timeout = NULL;
					break;

				case 3:
					mode = POWER_SAVE_DEEP_SLEEP_MODE;
					if (argc == 2)
						param_gpio = NULL;
					break;

				default:
					_atcmd_info("wifi_sleep: invalid mode\n");
					return ATCMD_ERROR_INVAL;
			}

			if (param_timeout)
			{
				timeout = atoi(param_timeout);
				if (timeout < 0 || (timeout == 0 && mode == POWER_SAVE_MODEM_SLEEP_MODE))
				{
					_atcmd_info("wifi_sleep: invalid timeout\n");
					return ATCMD_ERROR_INVAL;
				}
			}
			else if (mode == POWER_SAVE_MODEM_SLEEP_MODE)
			{
				_atcmd_info("wifi_sleep: no timeout\n");
				return ATCMD_ERROR_INVAL;
			}

			if (param_gpio)
			{
				gpio = atoi(param_gpio);

				if (gpio < GPIO_08 || gpio > GPIO_17)
				{
					_atcmd_info("wifi_sleep: invalid gpio\n");
					return ATCMD_ERROR_INVAL;
				}

				wakeup_source |= WAKEUP_SOURCE_GPIO;
			}

			_atcmd_info("wifi_sleep: mode=%d timeout=%d gpio=%d, wakeup_source=0x%X\n",
									mode, timeout, gpio, wakeup_source);

			if (nrc_ps_set_wakeup_source(wakeup_source) != NRC_SUCCESS)
			{
				_atcmd_error("wifi_sleep: failed to set wakeup source\n");
				return ATCMD_ERROR_FAIL;
			}

			switch (mode)
			{
				case POWER_SAVE_MODEM_SLEEP_MODE:
					if (timeout == 0)
						timeout = lmac_ps_get_iw_power_timeout();

					_atcmd_info("wifi_sleep: modem sleep, timeout=%d\n", timeout);

					if (nrc_ps_set_sleep(mode, 0, timeout) != NRC_SUCCESS)
					{
						_atcmd_error("wifi_sleep: failed to set modem sleep mode\n");
						return ATCMD_ERROR_FAIL;
					}

					break;

				case POWER_SAVE_DEEP_SLEEP_MODE:
				{
					_atcmd_info("wifi_sleep: deep sleep, timeout=%d gpio=%d\n", timeout, gpio);

					if (gpio >= 0 && nrc_ps_set_gpio_wakeup_pin(gpio) != NRC_SUCCESS)
					{
						_atcmd_error("wifi_sleep: failed to set wakeup pin\n");
						return ATCMD_ERROR_FAIL;
					}

					if (nrc_ps_set_sleep(mode, timeout, 0) != NRC_SUCCESS)
					{
						_atcmd_error("wifi_sleep: failed to set deep sleep mode\n");
						return ATCMD_ERROR_FAIL;
					}
				}
			}

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}
#endif

static atcmd_info_t g_atcmd_wifi_sleep =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "SLEEP",
	.id = ATCMD_WIFI_SLEEP,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_sleep_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_sleep_set,
};

/**********************************************************************************************/

static int _atcmd_wifi_softap_run (int argc, char *argv[])
{
	atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
	int net_id = -1;

	if (softap->active)
	{
		_atcmd_info("wifi_softap: busy\n");

		return ATCMD_ERROR_BUSY;
	}

	atcmd_wifi_lock();

	if (connect->connected || connect->connecting || connect->disconnecting)
	{
		_atcmd_info("wifi_softap: sta busy\n");

		goto wifi_softap_fail;
	}

	if (nrc_wifi_get_network_index(&net_id) != WIFI_SUCCESS)
		goto wifi_softap_fail;

	if (net_id < 0)
	{
		_atcmd_info("wifi_softap: add_network\n");

		if (nrc_wifi_add_network(&net_id) != WIFI_SUCCESS)
			goto wifi_softap_fail;
	}

	_atcmd_info("wifi_softap: net_id=%d\n", net_id);

	if (net_id >= 0)
	{
		_atcmd_info("wifi_softap: channel=%d,%d ssid=%s security=%s,%s\n",
							softap->channel.number, softap->channel.freq,
							softap->ssid, softap->security, softap->password);

		if (nrc_wifi_softap_set_conf(net_id,
									softap->ssid, softap->channel.freq,
									_atcmd_wifi_security_string_to_index(softap->security),
									softap->password) == WIFI_SUCCESS)
		{

			if (nrc_wifi_softap_start(net_id) == WIFI_SUCCESS)
			{
				softap->net_id = net_id;

				_atcmd_info("wifi_softap: bss_max_idle, period=%d retry=%d\n",
							softap->bss_max_idle.period, softap->bss_max_idle.retry);

				if (nrc_wifi_set_bss_max_idle(net_id,
						softap->bss_max_idle.period, softap->bss_max_idle.retry) == WIFI_SUCCESS)
				{
#ifndef CONFIG_ATCMD_WITHOUT_LWIP
					_atcmd_info("wifi_softap: ipaddr=%s\n", softap->ip.addr);

					if (nrc_wifi_softap_set_ip(softap->ip.addr) == WIFI_SUCCESS)
#endif
					{
						_atcmd_info("wifi_softap: done\n");

						softap->active = true;

						return ATCMD_SUCCESS;
					}
				}
			}
		}
	}

wifi_softap_fail:

	_atcmd_info("wifi_softap: fail\n");

	atcmd_wifi_unlock();

	return ATCMD_ERROR_FAIL;
}

static int _atcmd_wifi_softap_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;

			char param_ssid[ATCMD_STR_PARAM_SIZE(sizeof(atcmd_wifi_ssid_t))];
			char param_security[ATCMD_STR_PARAM_SIZE(sizeof(atcmd_wifi_security_t))];
			char param_password[ATCMD_STR_PARAM_SIZE(sizeof(atcmd_wifi_password_t))];

			if (atcmd_str_to_param(softap->ssid, param_ssid, sizeof(param_ssid)) &&
				atcmd_str_to_param(softap->security, param_security, sizeof(param_security)) &&
				atcmd_str_to_param(softap->password, param_password, sizeof(param_password)))
			{

/*				_atcmd_debug("wifi_softap: channel=%d,%d ssid=%s security=%s,%s\n",
							softap->channel.number, softap->channel.freq,
							param_ssid, param_security, param_password); */

				if (softap->dhcp_server)
				{
					ATCMD_MSG_INFO("WSOFTAP", "%.1f,%s,%s,%s,\"dhcp\"",
								softap->channel.freq / 10.,
								param_ssid, param_security, param_password);
				}
				else
				{
					ATCMD_MSG_INFO("WSOFTAP", "%.1f,%s,%s,%s",
								softap->channel.freq / 10.,
								param_ssid, param_security, param_password);
				}

				break;
			}

			return ATCMD_ERROR_FAIL;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_softap_set (int argc, char *argv[])
{
	char *param_freq = NULL;
	char *param_ssid = NULL;
	char *param_security = NULL;
	char *param_password = NULL;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WSOFTAP=<frequency>,\"<ssid>\"[,\"<security>\"[,\"<password>\"]]");
			break;

		case 4:
			param_password = argv[3];

		case 3:
			param_security = argv[2];

		case 2:
		{
			atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;
			atcmd_wifi_ssid_t str_ssid;
			atcmd_wifi_security_t  str_security;
			atcmd_wifi_password_t str_password;

			param_freq = argv[0];
			param_ssid = argv[1];

			if (!atcmd_param_to_str(param_ssid, str_ssid, sizeof(str_ssid)) ||
						strlen(str_ssid) > ATCMD_WIFI_SSID_LEN_MAX)
				return ATCMD_ERROR_INVAL;

			if (param_security)
			{
				if (!atcmd_param_to_str(param_security, str_security, sizeof(str_security)))
					return ATCMD_ERROR_INVAL;

				if (strcmp(str_security, "wpa2") == 0 || strcmp(str_security, "psk") == 0)
					strcpy(str_security, "wpa2-psk");
				else if (strcmp(str_security, "owe") == 0)
					strcpy(str_security, "wpa3-owe");
				else if (strcmp(str_security, "sae") == 0)
					strcpy(str_security, "wpa3-sae");

				switch (_atcmd_wifi_security_string_to_index(str_security))
				{
					case WIFI_SEC_OPEN:
						break;

					case WIFI_SEC_WPA2:
						if (param_password)
							if (atcmd_param_to_str(param_password, str_password, sizeof(str_password)))
							{
								int len = strlen(str_password);

								if (len >= ATCMD_WIFI_PASSWORD_LEN_MIN && len <= ATCMD_WIFI_PASSWORD_LEN_MAX)
									break;
							}

					default:
						return ATCMD_ERROR_INVAL;
				}
			}
			else
			{
				strcpy(str_security, "open");
				strcpy(str_password, "");
			}

			softap->channel.freq = (int)(atof(param_freq) * 10);
			softap->channel.number = GetChannelIndexByS1GFreq(softap->channel.freq);

			strcpy(softap->ssid, str_ssid);
			strcpy(softap->security, str_security);
			strcpy(softap->password, str_password);

/*			_atcmd_debug("wifi_softap: channel=%d,%d ssid=%s security=%s,%s\n",
							softap->channel.number, softap->channel.freq,
							softap->ssid, softap->security, softap->password); */

			return _atcmd_wifi_softap_run(0, NULL);
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_softap =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "SOFTAP",
	.id = ATCMD_WIFI_SOFTAP,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_softap_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_softap_set,
};

/**********************************************************************************************/

#include "umac_info.h"

static int _atcmd_wifi_bss_max_idle_get (int argc, char *argv[])
{
	atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;

/*	if (!softap->active)
		return ATCMD_ERROR_NOTSUPP; */

	switch (argc)
	{
		case 0:
			ATCMD_MSG_INFO("WBSSMAXIDLE", "%d,%d",
					softap->bss_max_idle.period, softap->bss_max_idle.retry);
			break;

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_bss_max_idle_set (int argc, char *argv[])
{
	atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;
	int32_t period = -1;
	int32_t retry = -1;

/*	if (!softap->active)
		return ATCMD_ERROR_NOTSUPP; */

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WBSSMAXIDLE=<period>[,<retry>]");
			break;

		case 2:
			if (atcmd_param_to_int32(argv[1], &retry) != 0)
				return ATCMD_ERROR_INVAL;

			if (retry < 1 || retry > 100)
				return ATCMD_ERROR_INVAL;

		case 1:
			if (atcmd_param_to_int32(argv[0], &period) != 0)
				return ATCMD_ERROR_INVAL;

			if (period < 0)
				return ATCMD_ERROR_INVAL;

			if (retry < 0)
				retry = softap->bss_max_idle.retry;

			_atcmd_info("bss_max_idle: period=%d retry=%d\n", period, retry);

			if (nrc_wifi_set_bss_max_idle(softap->net_id, period, retry) != WIFI_SUCCESS)
				return ATCMD_ERROR_FAIL;

			softap->bss_max_idle.period = period;
			softap->bss_max_idle.retry = retry;

			break;

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_bss_max_idle =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "BSSMAXIDLE",
	.id = ATCMD_WIFI_BSS_MAX_IDLE,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_bss_max_idle_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_bss_max_idle_set,
};

/**********************************************************************************************/

#include "umac_info.h"

static int _atcmd_wifi_stainfo_get (int argc, char *argv[])
{
	atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;
	int aid, aid_max;

	if (!softap->active)
		return ATCMD_ERROR_NOTSUPP;

	aid = 1;
	aid_max = MAX_STA;

	switch (argc)
	{
		case 1:
			aid = atoi(argv[0]);
			aid_max = aid + 1;

		case 0:
			if (aid > 0 && aid < MAX_STA)
			{
				umac_stainfo *sta_info;
				int cnt;

				for (cnt = 0 ; aid < aid_max ; aid++)
				{
					sta_info = get_umac_stainfo_by_aid(0, aid);
					if (sta_info)
					{
						if (sta_info->aid != aid)
							_atcmd_info("sta_info: aid=%d, mismatch (%d)\n", aid, sta_info->aid);

						if (sta_info->state != ASSOC)
						{
							_atcmd_info("sta_info: aid=%d, invalid\n", aid);
							continue;
						}

						cnt++;

#if defined(INCLUDE_STA_SIG_INFO)
						ATCMD_MSG_INFO("WSTAINFO", "%d,\""MACSTR"\",%d,%d,%d",
								sta_info->aid,
								MAC2STR(sta_info->maddr),
								sta_info->rssi,
								sta_info->snr,
								sta_info->mcs);
#else
						ATCMD_MSG_INFO("WSTAINFO", "%d,\""MACSTR"\"",
								sta_info->aid, MAC2STR(sta_info->maddr));
#endif
					}
				}

				break;
			}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_stainfo_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WSTAINFO=<aid>");
			return ATCMD_SUCCESS;

		case 1:
			return _atcmd_wifi_stainfo_get(argc, argv);

		default:
			return ATCMD_ERROR_INVAL;
	}
}

static atcmd_info_t g_atcmd_wifi_stainfo =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "STAINFO",
	.id = ATCMD_WIFI_STAINFO,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_stainfo_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_stainfo_set,
};

/**********************************************************************************************/

#ifndef CONFIG_ATCMD_WITHOUT_LWIP

static int _atcmd_wifi_dhcps_run (int argc, char *argv[])
{
	atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;

	if (!softap->active)
	{
		_atcmd_info("wifi_dhcp_server: inactive\n");

		return ATCMD_ERROR_INVAL;
	}

	if (softap->dhcp_server)
	{
		_atcmd_info("wifi_dhcp_server: busy\n");
		return ATCMD_ERROR_BUSY;
	}

	_atcmd_info("wifi_dhcp_server: run\n");

	if (nrc_wifi_softap_start_dhcp_server() != WIFI_SUCCESS)
	{
		_atcmd_info("wifi_dhcp_server: fail\n");
		return ATCMD_ERROR_FAIL;
	}

	_atcmd_wifi_ipaddr_get(2, NULL);

	softap->dhcp_server = true;

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_dhcps =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "DHCPS",
	.id = ATCMD_WIFI_DHCPS,

	.handler[ATCMD_HANDLER_RUN] = _atcmd_wifi_dhcps_run,
	.handler[ATCMD_HANDLER_GET] = NULL,
	.handler[ATCMD_HANDLER_SET] = NULL,
};

#endif /* #ifndef CONFIG_ATCMD_WITHOUT_LWIP */

/**********************************************************************************************/

extern int _atcmd_basic_timeout_get (int argc, char *argv[]);
extern int _atcmd_basic_timeout_set (int argc, char *argv[]);

static int _atcmd_wifi_timeout_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			argc = 1;
			argv[0] = "0";

			return _atcmd_basic_timeout_get(argc, argv);
		}

		default:
			return ATCMD_ERROR_INVAL;
	}
}

static int _atcmd_wifi_timeout_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WTIMEOUT=\"<command>\",<time>");
			break;

		case 2:
			return _atcmd_basic_timeout_set(argc, argv);

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_timeout =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "TIMEOUT",
	.id = ATCMD_WIFI_TIMEOUT,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_timeout_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_timeout_set,
};

/**********************************************************************************************/

static atcmd_group_t g_atcmd_group_wifi =
{
	.list.next = NULL,
	.list.prev = NULL,

	.name = "WIFI",
	.id = ATCMD_GROUP_WIFI,

	.cmd_prefix = "W",
	.cmd_prefix_size = 1,

	.cmd_list_head.next = NULL,
	.cmd_list_head.prev = NULL,
};

static atcmd_info_t *g_atcmd_wifi[] =
{
	&g_atcmd_wifi_macaddr,
	&g_atcmd_wifi_country,
	&g_atcmd_wifi_tx_power,
	&g_atcmd_wifi_rx_signal,
	&g_atcmd_wifi_rate_control,
	&g_atcmd_wifi_mcs,
	&g_atcmd_wifi_duty_cycle,
	&g_atcmd_wifi_cca_threshold,
	&g_atcmd_wifi_tx_time,
	&g_atcmd_wifi_tsf,
	&g_atcmd_wifi_lbt,
#ifndef CONFIG_ATCMD_WITHOUT_LWIP
	&g_atcmd_wifi_ipaddr,
	&g_atcmd_wifi_dhcp,
#endif
	&g_atcmd_wifi_scan,
	&g_atcmd_wifi_connect,
	&g_atcmd_wifi_disconnect,
	&g_atcmd_wifi_roaming,
#ifndef CONFIG_ATCMD_WITHOUT_LWIP
	&g_atcmd_wifi_ping,
	&g_atcmd_wifi_fota,
#endif
	&g_atcmd_wifi_sleep,

	&g_atcmd_wifi_softap,
	&g_atcmd_wifi_bss_max_idle,
	&g_atcmd_wifi_stainfo,
#ifndef CONFIG_ATCMD_WITHOUT_LWIP
	&g_atcmd_wifi_dhcps,
#endif

	&g_atcmd_wifi_timeout,

	NULL
};

bool atcmd_wifi_lock (void)
{
	SemaphoreHandle_t mutex = g_atcmd_wifi_info->mutex;
	const int timeout_msec = 60 * 1000;

	if (!mutex)
		_atcmd_error("null\n");
	else
	{
		if (!xSemaphoreTake(mutex, pdMS_TO_TICKS(timeout_msec)))
		{
			_atcmd_error("timeout, %d-msec\n", timeout_msec);

			return false;
		}
	}

	return true;
}

bool atcmd_wifi_unlock (void)
{
	SemaphoreHandle_t mutex = g_atcmd_wifi_info->mutex;

	if (!mutex)
		_atcmd_error("null\n");
	else
	{
		if (!xSemaphoreGive(mutex))
		{
			_atcmd_error("no space\n");

			return false;
		}
	}

	return true;
}

int atcmd_wifi_enable (void)
{
	static StaticSemaphore_t xMutexBuffer;
	int i;

#ifndef ATCMD_WIFI_INFO_STATIC
	g_atcmd_wifi_info = _atcmd_malloc(sizeof(atcmd_wifi_info_t));
	if (!g_atcmd_wifi_info)
	{
		_atcmd_error("malloc() failed\n");
		return -1;
	}
#endif

	_atcmd_wifi_init_info(g_atcmd_wifi_info);

	g_atcmd_wifi_info->mutex = xSemaphoreCreateMutexStatic(&xMutexBuffer);
	if (!g_atcmd_wifi_info->mutex)
		return -1;

	if (_atcmd_wifi_init(g_atcmd_wifi_info) != 0)
		return -1;

	if (atcmd_group_register(&g_atcmd_group_wifi) != 0)
		return -1;

	for (i = 0 ; g_atcmd_wifi[i] ; i++)
	{
		if (atcmd_info_register(ATCMD_GROUP_WIFI, g_atcmd_wifi[i]) != 0)
			return -1;
	}

#ifdef CONFIG_ATCMD_DEBUG
	atcmd_info_print(&g_atcmd_group_wifi);
#endif
	return 0;
}

void atcmd_wifi_disable (void)
{
	int i;

	for (i = 0 ; g_atcmd_wifi[i] ; i++)
		atcmd_info_unregister(ATCMD_GROUP_WIFI, g_atcmd_wifi[i]->id);

	atcmd_group_unregister(ATCMD_GROUP_WIFI);

	vSemaphoreDelete(g_atcmd_wifi_info->mutex);

	_atcmd_free(g_atcmd_wifi_info);
}

void atcmd_wifi_sleep_send_event (void)
{
	_atcmd_wifi_sleep_send_event();
}
