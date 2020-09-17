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
#include "nrc_wifi.h" /* /lib/lwip/contrib/port */

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

static void _atcmd_wifi_event_callback (WLAN_EVENT_ID event);
static void _atcmd_wifi_ping_report_callback (_lwip_ping_report_t *report);

/**********************************************************************************************/

static float _atcmd_wifi_s1g_freq (const char *freq)
{
	const CHANNEL_MAPPING_TABLE *table;
	uint16_t non_s1g_freq;
	uint16_t s1g_freq;

	non_s1g_freq = atoi(freq);

	if (!CheckSupportNonS1GFreq(non_s1g_freq))
		return 0;

	table = get_s1g_channel_item_by_nons1g_freq(non_s1g_freq);
	if (!table)
		return 0;

	s1g_freq = table->s1g_freq;

	return s1g_freq / 10.;
}

static uint32_t _atcmd_wifi_snr_read (void)
{
	uint32_t snr = nrc_wifi_get_snr();

	return snr;
}

static int8_t _atcmd_wifi_rssi_read (void)
{
	int8_t rssi = nrc_wifi_get_rssi();

	return rssi;
}

static bool _atcmd_wifi_rssi_valid (const int rssi)
{
	if (rssi >= -128 && rssi <= 0)
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

static NRC_WIFI_SECURITY _atcmd_wifi_security_mode (const char *sec_mode)
{
	if (sec_mode)
	{
		if (strcmp(sec_mode, "open") == 0)
			return MODE_OPEN;
		else if (strcmp(sec_mode, "wpa2") == 0)
			return MODE_WPA2;
	}

	return MODE_SEC_MAX;
}

/**********************************************************************************************/

static void _atcmd_wifi_init_info (atcmd_wifi_info_t *info)
{
	memset(info, 0, sizeof(atcmd_wifi_info_t));

	info->net_id = -1;

	info->event = 0;

	strcpy(info->country, ATCMD_WIFI_INIT_COUNTRY);
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

	if (1)
	{
		atcmd_wifi_ping_t *ping = &info->ping;

		ip4_addr_set_any(&ping->remote_ip);
		ping->interval = ATCMD_WIFI_INIT_PING_INTERVAL;
		ping->count = ATCMD_WIFI_INIT_PING_COUNT;
		ping->data_size = ATCMD_WIFI_INIT_PING_SIZE;
		ping->report_cb = _atcmd_wifi_ping_report_callback;
	}

	if (1)
	{
		atcmd_wifi_softap_t *softap = &info->softap;

		softap->active = false;
		softap->dhcp_server = false;

		softap->channel.number = -1;
		softap->channel.freq = 0;

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
	if (nrc_wifi_set_country(info->country) != WIFI_SUCCESS)
		return -1;

	if (nrc_wifi_set_tx_power(info->txpower) != WIFI_SUCCESS)
		return -1;

	nrc_wifi_register_event_handler(_atcmd_wifi_event_callback);

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

		if (_atcmd_wifi_event_polling(WLAN_EVT_SCAN_DONE))
		{
			_atcmd_wifi_event_polled(WLAN_EVT_SCAN_DONE);

			_atcmd_wifi_scan_result_report(&scan->results, false);

			ATCMD_LOG_EVENT("WEVENT", "%s", "%s", "\"SCAN_DONE\"");
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

		if (_atcmd_wifi_event_polling(WLAN_EVT_CONNECT_SUCCESS))
		{
			_atcmd_wifi_event_polled(WLAN_EVT_CONNECT_SUCCESS);
			_atcmd_wifi_event_polled(WLAN_EVT_CONNECT_FAIL);

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
/*		char param_ssid[ATCMD_STR_PARAM_SIZE(sizeof(atcmd_wifi_ssid_t))];
		char param_bssid[ATCMD_STR_PARAM_SIZE(sizeof(atcmd_wifi_bssid_t))];

		if (atcmd_str_to_param(connect->ssid, param_ssid, sizeof(param_ssid)) &&
				atcmd_str_to_param(connect->bssid, param_bssid, sizeof(param_bssid)))
		{
			ATCMD_LOG_EVENT("WEVENT", "%s,%s,%s", "%s ssid=%s bssid=%s",
									"\"CONNECT_SUCCESS\"", param_ssid, param_bssid);
		}
		else */
		{
			ATCMD_LOG_EVENT("WEVENT", "%s", "%s", "\"CONNECT_SUCCESS\"");
		}
	}
}

static void _atcmd_wifi_event_connect_fail (void)
{
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
	bool event = false;

	if (connect->connecting)
	{
		g_atcmd_wifi_info->net_id = -1;

		connect->connected = false;
		connect->connecting = false;
		connect->disconnecting = false;

		if (_atcmd_wifi_event_polling(WLAN_EVT_CONNECT_FAIL))
		{
			_atcmd_wifi_event_polled(WLAN_EVT_CONNECT_SUCCESS);
			_atcmd_wifi_event_polled(WLAN_EVT_CONNECT_FAIL);

			event = true;
		}
	}
	else if (!connect->connected)
	{
		g_atcmd_wifi_info->net_id = -1;

		connect->connected = false;
		connect->connecting = false;
		connect->disconnecting = false;

		event = true;
	}

	if (event)
	{
/*		char param_ssid[ATCMD_STR_PARAM_SIZE(sizeof(atcmd_wifi_ssid_t))];
		char param_bssid[ATCMD_STR_PARAM_SIZE(sizeof(atcmd_wifi_bssid_t))];

		if (atcmd_str_to_param(connect->ssid, param_ssid, sizeof(param_ssid)) &&
				atcmd_str_to_param(connect->bssid, param_bssid, sizeof(param_bssid)))
		{
			ATCMD_LOG_EVENT("WEVENT", "%s,%s,%s", "%s ssid=%s bssid=%s",
									"\"CONNECT_FAIL\"", param_ssid, param_bssid);
		}
		else */
		{
			ATCMD_LOG_EVENT("WEVENT", "%s", "%s", "\"CONNECT_FAIL\"");
		}
	}
}

static void _atcmd_wifi_event_disconnect (void)
{
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
	bool event = false;

	if (connect->disconnecting)
	{
		g_atcmd_wifi_info->net_id = -1;

		connect->connected = false;
		connect->connecting = false;
		connect->disconnecting = false;

		if (_atcmd_wifi_event_polling(WLAN_EVT_DISCONNECT))
		{
			_atcmd_wifi_event_polled(WLAN_EVT_DISCONNECT);

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
/*		char param_ssid[ATCMD_STR_PARAM_SIZE(sizeof(atcmd_wifi_ssid_t))];
		char param_bssid[ATCMD_STR_PARAM_SIZE(sizeof(atcmd_wifi_bssid_t))];

		if (atcmd_str_to_param(connect->ssid, param_ssid, sizeof(param_ssid)) &&
				atcmd_str_to_param(connect->bssid, param_bssid, sizeof(param_bssid)))
		{
			ATCMD_LOG_EVENT("WEVENT", "%s,%s,%s", "%s ssid=%s bssid=%s",
									"\"DISCONNECT\"", param_ssid, param_bssid);
		}
		else */
		{
			ATCMD_LOG_EVENT("WEVENT", "%s", "%s", "\"DISCONNECT\"");
		}
	}
}

static void _atcmd_wifi_event_get_ip_success (void)
{
	atcmd_wifi_dhcp_t *dhcp = &g_atcmd_wifi_info->dhcp;

	if (dhcp->requesting)
	{
		dhcp->requesting = false;
		dhcp->responsed = true;

		if (_atcmd_wifi_event_polling(WLAN_EVT_GET_IP))
		{
			_atcmd_wifi_event_polled(WLAN_EVT_GET_IP);

/*			ATCMD_LOG_EVENT("WEVENT", "%s", "%s", "\"DHCP_SUCCESS\""); */
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

		if (_atcmd_wifi_event_polling(WLAN_EVT_GET_IP_FAIL))
		{
			_atcmd_wifi_event_polled(WLAN_EVT_GET_IP_FAIL);

/*			ATCMD_LOG_EVENT("WEVENT", "%s", "%s", "\"DHCP_FAIL\""); */
		}
	}
}

static void (*_atcmd_wifi_event_handler[WLAN_EVT_MAX])(void) =
{
	[WLAN_EVT_CONNECT_SUCCESS] = _atcmd_wifi_event_connect_success,
	[WLAN_EVT_CONNECT_FAIL] = _atcmd_wifi_event_connect_fail,
	[WLAN_EVT_GET_IP] = _atcmd_wifi_event_get_ip_success,
	[WLAN_EVT_GET_IP_FAIL] = _atcmd_wifi_event_get_ip_fail,
	[WLAN_EVT_DISCONNECT] = _atcmd_wifi_event_disconnect,
	[WLAN_EVT_SCAN_DONE] = _atcmd_wifi_event_scan_done,
};

static void _atcmd_wifi_event_callback (WLAN_EVENT_ID event)
{
	const char *str_event[] =
	{
		[WLAN_EVT_SCAN_DONE] = "scan_done",
		[WLAN_EVT_CONNECT_SUCCESS] = "connect_success",
		[WLAN_EVT_CONNECT_FAIL] = "connect_fail",
		[WLAN_EVT_DISCONNECT] = "disconnect",
		[WLAN_EVT_GET_IP] = "dhcp_success",
		[WLAN_EVT_GET_IP_FAIL] = "dhcp_fail",
		[WLAN_EVT_START_SOFT_AP] = "soft_ap_start",
		[WLAN_EVT_SET_SOFT_AP_IP] = "soft_ap_ip_set",
		[WLAN_EVT_START_DHCP_SERVER] = "dhcp_server_start"
	};

	switch(event)
	{
		case WLAN_EVT_SCAN_DONE:
		case WLAN_EVT_CONNECT_SUCCESS:
		case WLAN_EVT_CONNECT_FAIL:
		case WLAN_EVT_DISCONNECT:
		case WLAN_EVT_GET_IP:
		case WLAN_EVT_GET_IP_FAIL:
			_atcmd_info("wifi_event: %s\n", str_event[event]);
			_atcmd_wifi_event_handler[event]();
			break;

		case WLAN_EVT_START_SOFT_AP:
		case WLAN_EVT_SET_SOFT_AP_IP:
		case WLAN_EVT_START_DHCP_SERVER:
			_atcmd_info("wifi_event: %s (unused)\n", str_event[event]);
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
			char *macaddr = nrc_wifi_get_mac_address();

			if (!atcmd_str_to_param(macaddr, param_macaddr, sizeof(param_macaddr)))
				return ATCMD_ERROR_FAIL;

			ATCMD_LOG_INFO("WMACADDR", "%s", "macaddr=%s", param_macaddr);
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
			char *str_country = g_atcmd_wifi_info->country;
			char param_country[ATCMD_STR_PARAM_SIZE(sizeof(atcmd_wifi_country_t))];

			if (!atcmd_str_to_param(str_country, param_country, sizeof(param_country)))
				return ATCMD_ERROR_FAIL;

			ATCMD_LOG_INFO("WCOUNTRY", "%s", "country=%s", param_country);
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
			ATCMD_LOG_HELP("AT+WCOUNTRY=\"<country code>\"");
/*			ATCMD_LOG_HELP("AT+WCOUNTRY=\"{%s}\"", ATCMD_WIFI_COUNTRY_STRING); */
			break;

		case 1:
		{
			const char *support_country[] = ATCMD_WIFI_COUNTRY_ARRAY;
			int n_support_country = ATCMD_WIFI_COUNTRY_NUMBER;
			atcmd_wifi_country_t str_country;
			int i;

			param_country = argv[0];

			if (!atcmd_param_to_str(param_country, str_country, sizeof(str_country)))
				return ATCMD_ERROR_FAIL;

			for (i = 0 ; i < n_support_country ; i++)
			{
				if (strcmp(str_country, support_country[i]) == 0)
					break;
			}

			if (i == n_support_country)
				return ATCMD_ERROR_INVAL;

			if (nrc_wifi_set_country(str_country) != WIFI_SUCCESS)
				return ATCMD_ERROR_FAIL;

			strcpy(g_atcmd_wifi_info->country, str_country);

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
			int txpower = nrc_wifi_get_tx_power();

			if (txpower < TX_POWER_MIN || txpower > TX_POWER_MAX)
				return ATCMD_ERROR_FAIL;

			ATCMD_LOG_INFO("WTXPOWER", "%d", "txpower=%d", txpower);

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
			ATCMD_LOG_HELP("AT+WTXPOWER=<power in dBm>");
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
			int8_t rssi = nrc_wifi_get_rssi();
			uint32_t snr = nrc_wifi_get_snr();

			ATCMD_LOG_INFO("WRXSIG", "%d,%u", "rssi=%d snr=%u", (int)rssi, snr);
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
			ATCMD_LOG_HELP("AT+WRXSIG=<time>");
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
			int mode = nrc_wifi_get_rate_control() ? 1 : 0;

			ATCMD_LOG_INFO("WRATECTRL", "%d", "mode=%d", mode);

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
			ATCMD_LOG_HELP("AT+WRATECTRL=<mode>");
			break;

		case 1:
		{
			switch (atoi(argv[0]))
			{
				case 0:
					nrc_wifi_set_rate_control(false);
					break;

				case 1:
					nrc_wifi_set_rate_control(true);
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
	switch (argc)
	{
		case 0:
		{
			uint8_t index = nrc_wifi_get_mcs();

			ATCMD_LOG_INFO("WMCS", "%u", "index=%d", index);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_mcs_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_LOG_HELP("AT+WMCS=<MCS_index>");
			break;

		case 1:
		{
			if (nrc_wifi_get_rate_control())
			{
				int index = atoi(argv[0]);

				if ((index >= 0 && index <= 7) || index == 10)
				{
					nrc_wifi_set_mcs(index);
					break;
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

			ATCMD_LOG_INFO("WTSF", "%llu", "tsf=%llu", tsf.val);
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
					ATCMD_LOG_INFO("WDHCP", "%s,%s,%s", "ip=%s netmask=%s gateway=%s",
								param_ipaddr[0], param_ipaddr[1], param_ipaddr[2]);
					break;

				case 2:
					ATCMD_LOG_INFO("WDHCPS", "%s,%s,%s", "ip=%s netmask=%s gateway=%s",
								param_ipaddr[0], param_ipaddr[1], param_ipaddr[2]);
					break;

				default:
					ATCMD_LOG_INFO("WIPADDR", "%s,%s,%s", "ip=%s netmask=%s gateway=%s",
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
			ATCMD_LOG_HELP("AT+WIPADDR=\"<IP>\",\"<netmask>\",\"<gateway>\"");
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

/**********************************************************************************************/

extern int nrc_wifi_dhcp_start(void);

static int _atcmd_wifi_dhcp_run (int argc, char *argv[])
{
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
	atcmd_wifi_dhcp_t *dhcp = &g_atcmd_wifi_info->dhcp;

	if (!connect->connected)
	{
		_atcmd_info("wifi_dhcp: not connected\n");

		return ATCMD_ERROR_BUSY;
	}

	if (dhcp->requesting)
	{
		_atcmd_info("wifi_dhcp: busy\n");

		return ATCMD_ERROR_BUSY;
	}

	_atcmd_info("wifi_dhcp: run\n");

	dhcp->requesting = true;

	if (nrc_wifi_dhcp_start() == WIFI_SUCCESS)
	{
		uint32_t timeout_msec = 0;
		uint32_t time;

		for (time = 0 ; dhcp->requesting ; time += 100)
		{
			if (time > 0 && (time % 1000) == 0)
				_atcmd_info("wifi_dhcp: %u sec\n", time / 1000);

			if (timeout_msec > 0 && time >= timeout_msec)
			{
				_atcmd_wifi_event_poll(WLAN_EVT_GET_IP);
				_atcmd_wifi_event_poll(WLAN_EVT_GET_IP_FAIL);

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
	}

	dhcp->requesting = false;

	_atcmd_info("wifi_dhcp: fail\n");

	return ATCMD_ERROR_FAIL;
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

/**********************************************************************************************/

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

/**********************************************************************************************/

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
			ATCMD_LOG_INFO("WSCAN", "%s,%.1f,%d,%s,%s",
							"bssid=%s freq=%.1f sig_level=%d flags=%s ssid=%s",
							param_bssid, _atcmd_wifi_s1g_freq(result->freq),
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
				_atcmd_wifi_event_poll(WLAN_EVT_SCAN_DONE);

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
			_atcmd_wifi_scan_result_report(&g_atcmd_wifi_info->scan.results, true);
			break;

		default:
			return ATCMD_ERROR_INVAL;
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
	.handler[ATCMD_HANDLER_SET] = NULL,
};

/**********************************************************************************************/

static bool _atcmd_wifi_connect_valid_ap (char *ssid, char *bssid, NRC_WIFI_SECURITY sec_mode)
{
	atcmd_wifi_scan_t *scan = &g_atcmd_wifi_info->scan;
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

	if (sec_mode != MODE_OPEN && sec_mode != MODE_WPA2)
		return false;

	for (i = 0 ; i < scan->results.n_result ; i++)
	{
		if (ssid_len > 0 && strcmp(ssid, scan->results.result[i].ssid) != 0)
			continue;

		if (bssid_len > 0 && strcmp(bssid, scan->results.result[i].bssid) != 0)
			continue;

		if (strstr(scan->results.result[i].flags, "WPA2-PSK"))
		{
			if (sec_mode == MODE_WPA2)
				break;
		}
		else if (sec_mode == MODE_OPEN)
			break;
	}

	if (i == scan->results.n_result)
		return false;

	if (ssid_len == 0)
		strcpy(ssid, scan->results.result[i].ssid);

	if (bssid_len == 0)
		strcpy(bssid, scan->results.result[i].bssid);

	return true;
}

static int _atcmd_wifi_connect_run (int argc, char *argv[])
{
	bool wifi_roaming = (argc == -1) ? true : false;
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
	int net_id;

	atcmd_wifi_lock();

	if (connect->connecting || connect->connected)
	{
		_atcmd_info("wifi_connect: busy\n");

		atcmd_wifi_unlock();

		return ATCMD_ERROR_BUSY;
	}

	if (g_atcmd_wifi_info->net_id >= 0)
		net_id = g_atcmd_wifi_info->net_id;
	else
		net_id = nrc_wifi_add_network();

	if (net_id >= 0)
	{
		NRC_WIFI_SECURITY security = _atcmd_wifi_security_mode(connect->security);
		uint32_t timeout_msec = _atcmd_timeout_value("WCONN");
		uint32_t time;

		_atcmd_info("wifi_connect: ssid=%s\n", connect->ssid);

		if(strlen(connect->ssid) > 0 && nrc_wifi_set_ssid(net_id, connect->ssid) != WIFI_SUCCESS)
			goto wifi_connect_fail;

		_atcmd_info("wifi_connect: bssid=%s\n", connect->bssid);

		if(strlen(connect->bssid) > 0 && nrc_wifi_set_bssid(net_id, connect->bssid) != WIFI_SUCCESS)
			goto wifi_connect_fail;

		_atcmd_info("wifi_connect: security=%s\n", connect->security);

		if(nrc_wifi_set_security(net_id, security, connect->password) != WIFI_SUCCESS)
			goto wifi_connect_fail;

		g_atcmd_wifi_info->net_id = net_id;

		connect->connected = false;
		connect->connecting = true;
		connect->disconnecting = false;

		if (wifi_roaming)
		{
			_atcmd_wifi_event_poll(WLAN_EVT_CONNECT_SUCCESS);
			_atcmd_wifi_event_poll(WLAN_EVT_CONNECT_FAIL);
		}

		if(nrc_wifi_connect(net_id) != WIFI_SUCCESS)
		{
			connect->connecting = false;

			goto wifi_connect_fail;
		}

		for (time = 0 ; connect->connecting ; time += 100)
		{
			if (time > 0 && (time % 1000) == 0)
				_atcmd_info("wifi_connect: %u sec\n", time / 1000);

			if (timeout_msec > 0 && time >= timeout_msec)
			{
				if (!wifi_roaming)
				{
					_atcmd_wifi_event_poll(WLAN_EVT_CONNECT_SUCCESS);
					_atcmd_wifi_event_poll(WLAN_EVT_CONNECT_FAIL);
				}

				_atcmd_info("wifi_connect: timeout\n");

				atcmd_wifi_unlock();

				return ATCMD_ERROR_TIMEOUT;
			}

			_delay_ms(100);
		}

		if (!connect->connected)
			goto wifi_connect_fail;

		_atcmd_info("wifi_connect: done\n");

		atcmd_wifi_unlock();

		return ATCMD_SUCCESS;
	}

	_atcmd_info("wifi_connect: invalid net_id (%d)\n", net_id);

wifi_connect_fail:

	_atcmd_info("wifi_connect: fail\n");

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
				ATCMD_LOG_INFO("WCONN", "%s,%s,%s,%s,%s",
								"ssid=%s bssid=%s security=%s password=%s status=%s",
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
			ATCMD_LOG_HELP("AT+WCONN=\"<ssid|bssid>\"[,\"<security>\"[,\"<password>\"]]");
			break;

		case 3:
			param_security = argv[1];
			param_password = argv[2];

		case 2:
			if (!param_security)
				param_security = argv[1];

		case 1:
		{
			NRC_WIFI_SECURITY sec_mode = MODE_OPEN;

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

				sec_mode = _atcmd_wifi_security_mode(str_security);

				switch (sec_mode)
				{
					case MODE_OPEN:
						break;

					case MODE_WPA2:
						if (param_password)
							if (atcmd_param_to_str(param_password, str_password, sizeof(str_password)))
								if (strlen(str_password) <= ATCMD_WIFI_PASSWORD_LEN_MAX)
									break;

					default:
						return ATCMD_ERROR_INVAL;
				}
			}
			else
			{
				strcpy(str_security, "open");
				strcpy(str_password, "");
			}

			if (_atcmd_wifi_connect_valid_ap(str_ssid, str_bssid, sec_mode))
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
	bool wifi_roaming = (argc == -1) ? true : false;
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;

	atcmd_wifi_lock();

	if (!connect->connecting && !connect->connected)
	{
		_atcmd_info("wifi_disconnect: not connected\n");

		atcmd_wifi_unlock();

		return ATCMD_SUCCESS;
	}

	if (!connect->disconnecting)
	{
		_atcmd_info("wifi_disconnect: run\n");

		connect->disconnecting = true;

		if (connect->connecting)
		{
			connect->connecting = false;

			_atcmd_wifi_event_polled(WLAN_EVT_CONNECT_SUCCESS);
			_atcmd_wifi_event_polled(WLAN_EVT_CONNECT_FAIL);
		}

		if (wifi_roaming)
			_atcmd_wifi_event_poll(WLAN_EVT_DISCONNECT);

		if (nrc_wifi_disconnect(g_atcmd_wifi_info->net_id) != WIFI_SUCCESS)
		{
			connect->disconnecting = false;

			_atcmd_info("wifi_disconnect: fail\n");

			atcmd_wifi_unlock();

			return ATCMD_ERROR_FAIL;
		}

		if (1)
		{
			uint32_t timeout_msec = _atcmd_timeout_value("WDISCONN");
			uint32_t time;

			for (time = 0 ; connect->disconnecting ; time += 100)
			{
				if (time > 0 && (time % 1000) == 0)
					_atcmd_info("wifi_disconnect: %u sec\n", time / 1000);

				if (timeout_msec > 0 && time >= timeout_msec)
				{
					if (!wifi_roaming)
						_atcmd_wifi_event_poll(WLAN_EVT_DISCONNECT);

					_atcmd_info("wifi_disconnect: timeout\n");

					atcmd_wifi_unlock();

					return ATCMD_ERROR_TIMEOUT;
				}

				_delay_ms(100);
			}
		}

		_atcmd_info("wifi_disconnect: done\n");

		atcmd_wifi_unlock();

		return ATCMD_SUCCESS;
	}

	_atcmd_info("wifi_disconnect: busy\n");

	atcmd_wifi_unlock();

	return ATCMD_ERROR_BUSY;
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

#define _wifi_roaming_debug(fmt, ...)	_atcmd_debug("wifi_roaming: " fmt "\n", ##__VA_ARGS__)

#if 1
#define _wifi_roaming_log(fmt, ...)		_atcmd_info("wifi_roaming: " fmt "\n", ##__VA_ARGS__)
#else
#define _wifi_roaming_log(fmt, ...) \
{\
	_atcmd_info("wifi_roaming: " fmt "\n", ##__VA_ARGS__);\
	ATCMD_LOG_DEBUG("ROAMING: " fmt, ##__VA_ARGS__);\
}
#endif

static int _atcmd_wifi_roaming_scan (void)
{
	atcmd_wifi_scan_t *scan = &g_atcmd_wifi_info->scan;

	_wifi_roaming_log("Scanning ...");

	switch (_atcmd_wifi_scan_run(-1, NULL))
	{
		case ATCMD_ERROR_FAIL:
			_wifi_roaming_log("scan fail");
			break;

		case ATCMD_ERROR_BUSY:
		case ATCMD_ERROR_TIMEOUT:
		{
			const int timeout_msec = 100 * 100;
			int i;

			for (i = 0 ; i < timeout_msec ; i += 100)
			{
				vTaskDelay(pdMS_TO_TICKS(100));

				if (!scan->scanning)
					break;
			}

			if (scan->scanning)
			{
				_wifi_roaming_log("scan timeout, %d-msec", timeout_msec);
				break;
			}
		}

		case ATCMD_SUCCESS:
			_wifi_roaming_log("scan success, APs=%d", scan->results.n_result);

			if (scan->results.n_result > 0)
				return 0;
	}

	return -1;
}

static int _atcmd_wifi_roaming_connect (const char *bssid)
{
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;

	_wifi_roaming_log("Connecting to %s ...", bssid);

	strcpy(connect->bssid, bssid);

	switch (_atcmd_wifi_connect_run(-1, NULL))
	{
		case ATCMD_ERROR_BUSY:
			_wifi_roaming_log("already connecting(%d) or connected(%d)",
								connect->connecting, connect->connected);
			break;

		case ATCMD_ERROR_FAIL:
			_wifi_roaming_log("connect fail");
			break;

		case ATCMD_ERROR_TIMEOUT:
		{
			const int timeout_msec = 100 * 100;
			int i;

			for (i = 0 ; i < timeout_msec ; i += 100)
			{
				vTaskDelay(pdMS_TO_TICKS(100));

				if (connect->connected)
					break;
			}

			if (connect->connecting)
			{
				_wifi_roaming_log("connect timeout, %d-msec", timeout_msec);
				break;
			}
		}

		case ATCMD_SUCCESS:
			_wifi_roaming_log("connect success");
			return 0;
	}

	return -1;
}

static int _atcmd_wifi_roaming_disconnect (void)
{
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;

	_wifi_roaming_log("Disconnecting from %s ...", connect->bssid);

	switch (_atcmd_wifi_disconnect_run(-1, NULL))
	{
		case ATCMD_ERROR_FAIL:
			_wifi_roaming_log("disconnect fail");
			break;

		case ATCMD_ERROR_BUSY:
		case ATCMD_ERROR_TIMEOUT:
		{
			int timeout_msec = 100 *100;
			int i;

			for (i = 0 ; i < timeout_msec ; i += 100)
			{
				vTaskDelay(pdMS_TO_TICKS(100));

				if (!connect->disconnecting)
					break;
			}

			if (connect->disconnecting)
			{
				_wifi_roaming_log("disconnect timeout, %d-msec", timeout_msec);
				break;
			}
		}

		case ATCMD_SUCCESS:
			_wifi_roaming_log("disconnect success");
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

static int _atcmd_wifi_roaming_check_status (void)
{
	const bool rssi_check = false;
	atcmd_wifi_scan_t *scan = &g_atcmd_wifi_info->scan;
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
	atcmd_wifi_roaming_params_t *params = &g_atcmd_wifi_info->roaming.params;

	_wifi_roaming_log("Checking ...");

	if (scan->scanning)
	{
		_wifi_roaming_log("scanning");
		return -1;
	}
	if (connect->connecting)
	{
		_wifi_roaming_log("connecting");
		return -1;
	}
	if (connect->disconnecting)
	{
		_wifi_roaming_log("disconnecting");
		return -1;
	}
	else if (!connect->connected)
	{
		_wifi_roaming_log("disconnected");

		if (g_atcmd_wifi_info->net_id >= 0)
		{
			if (nrc_wifi_remove_network(g_atcmd_wifi_info->net_id) != WIFI_SUCCESS)
			{
				_wifi_roaming_log("nrc_wifi_remove_network() failed");
				return -1;
			}

			g_atcmd_wifi_info->net_id = -1;
		}
	}
	else if (rssi_check)
	{
		int8_t rssi = nrc_wifi_get_rssi();

		_wifi_roaming_log("connected, rssi %s (%d/%d)",
							rssi > params->rssi_threshold ? "high" : "low",
							rssi, params->rssi_threshold);

		if (rssi > params->rssi_threshold)
		   return 0; /* good */
	}

	return 1; /* scan */
}

static bool _atcmd_wifi_roaming_valid_ap (SCAN_RESULT *ap)
{
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;

	if (strcmp(ap->ssid, connect->ssid) != 0)
		return false;

	if (!_atcmd_wifi_bssid_valid(ap->bssid))
		return false;

	if (!_atcmd_wifi_rssi_valid(atoi(ap->sig_level)))
		return false;

	if (strstr(ap->flags, "WPA2-PSK"))
	{
		if (strcmp(connect->security, "wpa2") != 0)
			return false;
	}
	else if (strcmp(connect->security, "open") != 0)
		return false;

	return true;
}

static int _atcmd_wifi_roaming_compare_ap (SCAN_RESULT *ap1, SCAN_RESULT *ap2)
{
	int rssi1 = atoi(ap1->sig_level);
	int rssi2 = atoi(ap2->sig_level);

	if ((rssi1 < 0 && rssi1 > 255) || (rssi2 < 0 && rssi2 > 255))
		return -1;

	rssi1 = (int8_t)rssi1;
	rssi2 = (int8_t)rssi2;

	if (!_atcmd_wifi_rssi_valid(rssi1) || !_atcmd_wifi_rssi_valid(rssi2))
		return -1;

	if (rssi1 >= rssi2)
		return 0;

	return 1;
}

static int _atcmd_wifi_roaming_select_ap (SCAN_RESULT **good_ap)
{
	atcmd_wifi_scan_t *scan = &g_atcmd_wifi_info->scan;
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
	atcmd_wifi_roaming_t *roaming = &g_atcmd_wifi_info->roaming;

	SCAN_RESULT *scan_result, *cur_ap, *new_ap;
	int i;

	_wifi_roaming_log("Selecting ...");

	cur_ap = new_ap = NULL;

	for (i = 0 ; i < scan->results.n_result ; i++)
	{
		scan_result = &scan->results.result[i];

/*		_wifi_roaming_log("scn_ap: %s %s %s %d %s",
							scan_result->ssid, scan_result->bssid,
							scan_result->freq, (int8_t)atoi(scan_result->sig_level),
							scan_result->flags); */

		if (!_atcmd_wifi_roaming_valid_ap(scan_result))
			continue;

/*		_wifi_roaming_log("val_ap: %s %s %s %d %s",
							scan_result->ssid, scan_result->bssid,
							scan_result->freq, (int8_t)atoi(scan_result->sig_level),
							scan_result->flags); */

		if (!cur_ap && strcmp(scan_result->bssid, connect->bssid) == 0)
			cur_ap = scan_result;

		if (new_ap && _atcmd_wifi_roaming_compare_ap(new_ap, scan_result) == 0)
			continue;

		new_ap = scan_result;
	}

	if (cur_ap)
	{
		_wifi_roaming_log("cur_ap: %s %s %s %d %s",
							cur_ap->ssid, cur_ap->bssid,
							cur_ap->freq, (int8_t)atoi(cur_ap->sig_level),
							cur_ap->flags);
	}

	if (new_ap)
	{
/*		_wifi_roaming_log("new_ap: %s %s %s %d %s",
							new_ap->ssid, new_ap->bssid,
							new_ap->freq, (int8_t)atoi(new_ap->sig_level),
							new_ap->flags); */
	}

	*good_ap = cur_ap;

	if (!cur_ap)
		*good_ap = new_ap;
	else if (new_ap && cur_ap != new_ap)
	{
		int cur_rssi = atoi(cur_ap->sig_level);
		int new_rssi = atoi(new_ap->sig_level);

		if (cur_rssi <= roaming->params.rssi_threshold)
		{
			if ((new_rssi - cur_rssi) >= roaming->params.rssi_level)
				*good_ap = new_ap;
		}
	}

	if (!*good_ap)
	{
		_wifi_roaming_log("no APs");
	}
	else
	{
		_wifi_roaming_log("good_ap: %s %s %s %d %s",
							(*good_ap)->ssid, (*good_ap)->bssid,
							(*good_ap)->freq, (int8_t)atoi((*good_ap)->sig_level),
							(*good_ap)->flags);
	}

	if (!cur_ap)
		return -1;
	else if (*good_ap == cur_ap)
	{
/*		_wifi_roaming_log("current good"); */

		*good_ap = NULL;
	}

	return 0;
}

static void _atcmd_wifi_roaming_send_event (void)
{
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
	char param_ssid[ATCMD_STR_PARAM_SIZE(sizeof(atcmd_wifi_ssid_t))];
	char param_bssid[ATCMD_STR_PARAM_SIZE(sizeof(atcmd_wifi_bssid_t))];

	if (atcmd_str_to_param(connect->ssid, param_ssid, sizeof(param_ssid)) &&
			atcmd_str_to_param(connect->bssid, param_bssid, sizeof(param_bssid)))
	{
		ATCMD_LOG_EVENT("WEVENT", "%s,%s,%s", "%s ssid=%s bssid=%s",
								"\"ROAMING\"", param_ssid, param_bssid);
	}
	else
	{
		ATCMD_LOG_EVENT("WEVENT", "%s", "%s", "\"ROAMING\"");
	}
}

static void _atcmd_wifi_roaming_task (void *pvParameters)
{
	const int retry_max = 2;
	atcmd_wifi_roaming_t *roaming = &g_atcmd_wifi_info->roaming;
	SCAN_RESULT *good_ap = NULL;
	int i;

	_wifi_roaming_log("task run");

	while (roaming->enable)
	{
		vTaskDelay(pdMS_TO_TICKS(roaming->params.scan_delay));

		if (!roaming->enable)
			break;

		if (_atcmd_wifi_roaming_check_status() <= 0)
			continue;

		for (good_ap = NULL, i = 0 ; i <= retry_max ; i++)
		{
			if (_atcmd_wifi_roaming_scan() != 0)
				break;

			if (_atcmd_wifi_roaming_select_ap(&good_ap) == 0)
				break;

			_wifi_roaming_log("no current AP, retry=%d/%d", i, retry_max);
		}

		if (!good_ap)
			continue;

		if (_atcmd_wifi_roaming_disconnect() == 0)
		{
			if (_atcmd_wifi_roaming_connect(good_ap->bssid) == 0)
				_atcmd_wifi_roaming_send_event();
		}
	}

	_wifi_roaming_log("task exit");

	roaming->task = NULL;
	memset(&roaming->params, 0, sizeof(atcmd_wifi_roaming_params_t));

	vTaskDelete(NULL);
}

static int _atcmd_wifi_roaming_enable (atcmd_wifi_roaming_params_t *params)
{
#define ATCMD_WIFI_ROAMING_TASK_PRIORITY		2
#define ATCMD_WIFI_ROAMING_TASK_STACK_SIZE		1024

	atcmd_wifi_roaming_t *roaming = &g_atcmd_wifi_info->roaming;

	if (roaming->enable)
		return ATCMD_ERROR_BUSY;

	_wifi_roaming_log("enable, scan_delay=%d rssi_threshold=%d rssi_level=%d",
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

	_wifi_roaming_log("disable");

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
			ATCMD_LOG_INFO("WROAM", "%d,%d,%d",
					"scan_delay=%d rssi_threshold=%d rssi_level=%d",
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
			ATCMD_LOG_HELP("AT+WROAM=<scan_delay>,<rssi_threshold>,<rssi_level>"); /* enable */
			ATCMD_LOG_HELP("AT+WROAM=0"); /* disable */
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
					_wifi_roaming_log("disconnected");

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

static void _atcmd_wifi_ping_report_callback (_lwip_ping_report_t *report)
{
	char param_remote_ip[2 + ATCMD_IP4_ADDR_LEN_MAX];

	if (!atcmd_str_to_param(ip4addr_ntoa(&report->remote_ip),
							param_remote_ip, sizeof(param_remote_ip)))
		return;

	switch (report->status)
	{
		case _LWIP_PING_SUCCESS:
			ATCMD_LOG_EVENT("WPING", "%u,%s,%u,%u,%u",
							"%u bytes from %s, icmp_seq=%u ttl=%u time=%ums",
							report->data_size, param_remote_ip,
							report->icmp_seq, report->ttl, report->resp_time);
							break;

		case _LWIP_PING_SEND_FAIL:
		case _LWIP_PING_RECV_TIMEOUT:
			ATCMD_LOG_EVENT("WPING", "%u,%s,%u,%s",
							"%u bytes from %s, icmp_seq=%u %s",
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

			ATCMD_LOG_INFO("WPING", "%s,%u,%u,%u",
							"remote_ip=%s interval=%u count=%u size=%u",
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
			ATCMD_LOG_HELP("AT+WPING=\"<remote_IP>\"[,<count>]");
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

/**********************************************************************************************/

static int _atcmd_wifi_softap_run (int argc, char *argv[])
{
	atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
	int net_id;

	if (softap->active)
	{
		_atcmd_info("wifi_softap: busy\n");
		return ATCMD_ERROR_BUSY;
	}

	if (connect->connecting || connect->connected)
	{
		_atcmd_info("wifi_softap: connecting or connected to AP\n");

		return ATCMD_ERROR_BUSY;
	}
	else if (connect->disconnecting)
	{
		_atcmd_info("wifi_softap: disconnecting from AP\n");

		return ATCMD_ERROR_BUSY;
	}

	if (g_atcmd_wifi_info->net_id >= 0)
	{
		net_id = g_atcmd_wifi_info->net_id;

		_atcmd_info("wifi_softap: net_id=%d\n", net_id);
	}
	else
		net_id = nrc_wifi_add_network();

	if (net_id >= 0)
	{
		_atcmd_info("wifi_softap_set: channel=%d,%d ssid=%s security=%s,%s\n",
							softap->channel.number, softap->channel.freq,
							softap->ssid, softap->security, softap->password);

		if (nrc_wifi_softap_set_conf(net_id,
									softap->ssid, softap->channel.number,
									_atcmd_wifi_security_mode(softap->security),
									softap->password) == WIFI_SUCCESS)
		{

			if (nrc_wifi_softap_start(net_id) == WIFI_SUCCESS)
			{
				struct netif *netif = nrc_netif[0];
				ip4_addr_t ip[3];

				ip[0].addr = (netif_ip4_addr(netif))->addr;
				ip[1].addr = (netif_ip4_netmask(netif))->addr;
				ip[2].addr = (netif_ip4_gw(netif))->addr;

				if (!ip[0].addr && !ip[1].addr && !ip[2].addr)
				{
					int i;

					for (i = 0 ; i < 3 ; i++)
					{
						if (!ip4addr_aton(softap->ipaddr[i], &ip[i]))
							goto wifi_softap_fail;
					}

					netif_set_ipaddr(netif, &ip[0]);
					netif_set_netmask(netif, &ip[1]);
					netif_set_gw(netif, &ip[2]);
				}

				_atcmd_info("wifi_softap: done\n");

				softap->active = true;

				return ATCMD_SUCCESS;
			}
		}

		goto wifi_softap_fail;
	}

	_atcmd_info("wifi_softap: invalid net_id (%d)\n", net_id);

wifi_softap_fail:

	_atcmd_info("wifi_softap: fail\n");

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
					ATCMD_LOG_INFO("WSOFTAP", "%.1f,%s,%s,%s,\"dhcp\"",
								"freq=%.1f, ssid=%s security=%s password=%s dhcp",
								softap->channel.freq / 10.,
								param_ssid, param_security, param_password);
				}
				else
				{
					ATCMD_LOG_INFO("WSOFTAP", "%.1f,%s,%s,%s",
								"freq=%.1f, ssid=%s security=%s password=%s",
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
			ATCMD_LOG_HELP("AT+WSOFTAP=<frequency>,\"<ssid>\"[,\"<security>\"[,\"<password>\"]]");
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

				switch (_atcmd_wifi_security_mode(str_security))
				{
					case MODE_OPEN:
						break;

					case MODE_WPA2:
						if (param_password)
							if (atcmd_param_to_str(param_password, str_password, sizeof(str_password)))
								if (strlen(str_password) <= ATCMD_WIFI_PASSWORD_LEN_MAX)
									break;

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

#include "atcmd_fota.h"

static void _atcmd_wifi_fota_check_done (const char *sdk_ver, const char *atcmd_ver)
{
	ATCMD_LOG_EVENT("WEVENT", "\"%s\",\"%s\",\"%s\"", "%s sdk=%s atcmd=%s",
							"FOTA", sdk_ver, atcmd_ver);
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

			ATCMD_LOG_INFO("WFOTA", "%d,\"%s\"", "check_time=%d server_url=\"%s\"",
									params.check_time, params.server_url);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_fota_set (int argc, char *argv[])
{
	char *param_server_url = NULL;

	switch (argc)
	{
		case 0:
			ATCMD_LOG_HELP("AT+WFOTA=<check_time>[,\"<server_url>\"]");
			break;

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

			params.fw_bin_type = fw_bin_type[_hif_get_type()];
			params.check_time = atoi(argv[0]);
			params.check_done_cb = _atcmd_wifi_fota_check_done;
			strcpy(params.server_url, "");

			if (param_server_url)
				atcmd_param_to_str(param_server_url, params.server_url, sizeof(params.server_url));

			if (atcmd_fota_valid_params(&params))
			{
				if (atcmd_fota_set_params(&params) == 0)
					break;

				return ATCMD_ERROR_FAIL;
			}
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
			ATCMD_LOG_HELP("AT+WTIMEOUT=\"<command>\",<time>");
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
	&g_atcmd_wifi_tsf,
	&g_atcmd_wifi_ipaddr,
	&g_atcmd_wifi_dhcp,
	&g_atcmd_wifi_dhcps,
	&g_atcmd_wifi_scan,
	&g_atcmd_wifi_connect,
	&g_atcmd_wifi_disconnect,
	&g_atcmd_wifi_roaming,
	&g_atcmd_wifi_ping,
	&g_atcmd_wifi_softap,
	&g_atcmd_wifi_fota,
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

