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
static int _atcmd_wifi_disconnect_run (int argc, char *argv[]);

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

/*	_atcmd_debug("%s: %u -> %u\n", non_s1g_freq, s1g_freq); */

	return s1g_freq / 10.;
}

static NRC_WIFI_SECURITY _atcmd_wifi_security_mode (const char *str)
{
	if (str)
	{
		if (strcmp(str, "open") == 0)
			return MODE_OPEN;
		else if (strcmp(str, "wpa2") == 0)
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
	_atcmd_wifi_init_info(info);

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

static void _atcmd_wifi_event_scan_done (void)
{
/*	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect; */
	atcmd_wifi_scan_t *scan = &g_atcmd_wifi_info->scan;

/*	_atcmd_debug("%s: connected=%d connecting=%d disconnecting=%d\n", __func__,
				connect->connected, connect->connecting, connect->disconnecting); */

	if (scan->scanning)
	{
		scan->scanning = false;

		if (_atcmd_wifi_event_polling(WLAN_EVT_SCAN_DONE))
		{
			_atcmd_wifi_event_polled(WLAN_EVT_SCAN_DONE);

			ATCMD_LOG_EVENT("WEVENT", "%s", "%s", "\"SCAN_DONE\"");
		}
	}
}

static void _atcmd_wifi_event_connect_success (void)
{
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;

/*	_atcmd_debug("%s: connected=%d connecting=%d disconnecting=%d\n", __func__,
				connect->connected, connect->connecting, connect->disconnecting); */

	if (connect->connecting)
	{
		connect->connected = true;
		connect->connecting = false;
		connect->disconnecting = false;

		if (_atcmd_wifi_event_polling(WLAN_EVT_CONNECT_SUCCESS))
		{
			_atcmd_wifi_event_polled(WLAN_EVT_CONNECT_SUCCESS);
			_atcmd_wifi_event_polled(WLAN_EVT_CONNECT_FAIL);

			ATCMD_LOG_EVENT("WEVENT", "%s", "%s", "\"CONNECT_SUCCESS\"");
		}
	}
	else if (!connect->connected)
	{
		connect->connected = true;
/*		connect->connecting = false;
		connect->disconnecting = false; */

		ATCMD_LOG_EVENT("WEVENT", "%s", "%s", "\"CONNECT_SUCCESS\"");
	}
}

static void _atcmd_wifi_event_connect_fail (void)
{
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;

/*	_atcmd_debug("%s: connected=%d connecting=%d disconnecting=%d\n", __func__,
				connect->connected, connect->connecting, connect->disconnecting); */

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

			ATCMD_LOG_EVENT("WEVENT", "%s", "%s", "\"CONNECT_FAIL\"");
		}
	}
	else if (!connect->connected)
	{
		g_atcmd_wifi_info->net_id = -1;

/*		connect->connected = false;
		connect->connecting = false;
		connect->disconnecting = false; */

		ATCMD_LOG_EVENT("WEVENT", "%s", "%s", "\"CONNECT_FAIL\"");
	}
}

static int _atcmd_wifi_connect_run (int argc, char *argv[]);

static void _atcmd_wifi_event_disconnect (void)
{
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;

/*	_atcmd_debug("%s: connected=%d connecting=%d disconnecting=%d\n", __func__,
				connect->connected, connect->connecting, connect->disconnecting); */

	if (connect->disconnecting)
	{
		if (connect->connecting)
		{
			_atcmd_wifi_event_polled(WLAN_EVT_CONNECT_SUCCESS);
			_atcmd_wifi_event_polled(WLAN_EVT_CONNECT_FAIL);
		}

		g_atcmd_wifi_info->net_id = -1;

		connect->connected = false;
		connect->connecting = false;
		connect->disconnecting = false;

		if (_atcmd_wifi_event_polling(WLAN_EVT_DISCONNECT))
		{
			_atcmd_wifi_event_polled(WLAN_EVT_DISCONNECT);

			ATCMD_LOG_EVENT("WEVENT", "%s", "%s", "\"DISCONNECT\"");
		}
	}
	else if (connect->connecting || connect->connected)
	{
		g_atcmd_wifi_info->net_id = -1;

		connect->connected = false;
		connect->connecting = true;
		connect->disconnecting = false;

		_atcmd_wifi_event_poll(WLAN_EVT_CONNECT_SUCCESS);
		_atcmd_wifi_event_poll(WLAN_EVT_CONNECT_FAIL);

		ATCMD_LOG_EVENT("WEVENT", "%s", "%s", "\"DISCONNECT\"");
	}
}

static void _atcmd_wifi_event_get_ip_success (void)
{
/*	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect; */
	atcmd_wifi_dhcp_t *dhcp = &g_atcmd_wifi_info->dhcp;

/*	_atcmd_debug("%s: connected=%d connecting=%d disconnecting=%d\n", __func__,
				connect->connected, connect->connecting, connect->disconnecting); */

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
/*	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect; */
	atcmd_wifi_dhcp_t *dhcp = &g_atcmd_wifi_info->dhcp;

/*	_atcmd_debug("%s: connected=%d connecting=%d disconnecting=%d\n", __func__,
				connect->connected, connect->connecting, connect->disconnecting); */

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

/*	_atcmd_debug("%s: connected=%d connecting=%d disconnecting=%d\n", __func__,
				connect->connected, connect->connecting, connect->disconnecting); */

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

static int _atcmd_wifi_scan_result_report (SCAN_RESULTS *results)
{
	char param_bssid[ATCMD_STR_PARAM_SIZE(sizeof(atcmd_wifi_bssid_t))];
	char param_ssid[ATCMD_STR_PARAM_SIZE(sizeof(atcmd_wifi_ssid_t))];
	char param_flags[ATCMD_STR_PARAM_SIZE(ATCMD_WIFI_SCAN_FLAGS_LEN_MAX)];
	SCAN_RESULT *result;
	int i;

	memset(results, 0, sizeof(SCAN_RESULTS));

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
/*	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect; */
	atcmd_wifi_scan_t *scan = &g_atcmd_wifi_info->scan;

/*	_atcmd_debug("%s: connected=%d connecting=%d disconnecting=%d\n", __func__,
				connect->connected, connect->connecting, connect->disconnecting); */

	if (scan->scanning)
	{
		_atcmd_info("wifi_scan: busy\n");

		return ATCMD_ERROR_BUSY;
	}

	_atcmd_info("wifi_scan: run\n");

	scan->scanning = true;

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

				return ATCMD_ERROR_TIMEOUT;
			}

			_delay_ms(100);
		}

		_atcmd_wifi_scan_result_report(&g_atcmd_wifi_info->scan.results);

		_atcmd_info("wifi_scan: done\n");

		return ATCMD_SUCCESS;
	}

	scan->scanning = false;

	_atcmd_info("wifi_scan: fail\n");

	return ATCMD_ERROR_FAIL;
}

static int _atcmd_wifi_scan_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			_atcmd_wifi_scan_result_report(&g_atcmd_wifi_info->scan.results);
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

static int _atcmd_wifi_connect_run (int argc, char *argv[])
{
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
	int net_id;

/*	_atcmd_debug("%s: connected=%d connecting=%d disconnecting=%d\n", __func__,
				connect->connected, connect->connecting, connect->disconnecting); */

	if (connect->connecting || connect->connected)
	{
		_atcmd_info("wifi_connect: busy\n");

		return ATCMD_ERROR_BUSY;
	}

	if (g_atcmd_wifi_info->net_id >= 0)
		net_id = g_atcmd_wifi_info->net_id;
	else
		net_id = nrc_wifi_get_nd();

	if (net_id >= 0)
	{
		NRC_WIFI_SECURITY security = _atcmd_wifi_security_mode(connect->security);
		uint32_t timeout_msec = _atcmd_timeout_value("WCONN");
		uint32_t time;

		_atcmd_info("wifi_connect: ssid=%s\n", connect->ssid);

		if(nrc_wifi_set_ssid(net_id, connect->ssid) != WIFI_SUCCESS)
			goto wifi_connect_fail;

		_atcmd_info("wifi_connect: security=%s\n", connect->security);

		if(nrc_wifi_set_security(net_id, security, connect->password) != WIFI_SUCCESS)
			goto wifi_connect_fail;

		g_atcmd_wifi_info->net_id = net_id;

		connect->connected = false;
		connect->connecting = true;
		connect->disconnecting = false;

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
				_atcmd_wifi_event_poll(WLAN_EVT_CONNECT_SUCCESS);
				_atcmd_wifi_event_poll(WLAN_EVT_CONNECT_FAIL);

				_atcmd_info("wifi_connect: timeout\n");

				return ATCMD_ERROR_TIMEOUT;
			}

			_delay_ms(100);
		}

		if (!connect->connected)
			goto wifi_connect_fail;

		_atcmd_info("wifi_connect: done\n");

		return ATCMD_SUCCESS;
	}

	_atcmd_info("wifi_connect: invalid net_id (%d)\n", net_id);

wifi_connect_fail:

	_atcmd_info("wifi_connect: fail\n");

	return ATCMD_ERROR_FAIL;
}

static int _atcmd_wifi_connect_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			char param_ssid[ATCMD_STR_PARAM_SIZE(sizeof(atcmd_wifi_ssid_t))];
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
				atcmd_str_to_param(connect->security, param_security, sizeof(param_security)) &&
				atcmd_str_to_param(connect->password, param_password, sizeof(param_password)))
			{
				ATCMD_LOG_INFO("WCONN", "%s,%s,%s,%s",
								"ssid=%s security=%s password=%s status=%s",
								param_ssid, param_security, param_password, status);

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
			ATCMD_LOG_HELP("AT+WCONN=\"<ssid>\"[,\"<security>\"[,\"<password>\"]]");
			break;

		case 3:
			param_security = argv[1];
			param_password = argv[2];

		case 2:
			if (!param_security)
				param_security = argv[1];

		case 1:
		{
			atcmd_wifi_ssid_t str_ssid;
			atcmd_wifi_security_t  str_security;
			atcmd_wifi_password_t str_password;

			atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;

			param_ssid = argv[0];

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

			strcpy(connect->ssid, str_ssid);
			strcpy(connect->security, str_security);
			strcpy(connect->password, str_password);

			return _atcmd_wifi_connect_run(0, NULL);
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

/*	_atcmd_debug("%s: connected=%d connecting=%d disconnecting=%d\n", __func__,
				connect->connected, connect->connecting, connect->disconnecting); */

	if (!connect->connecting && !connect->connected)
	{
		_atcmd_info("wifi_disconnect: not connected\n");

		return ATCMD_SUCCESS;
	}

	if (!connect->disconnecting)
	{
		_atcmd_info("wifi_disconnect: run\n");

		connect->disconnecting = true;

		if (nrc_wifi_disconnect(g_atcmd_wifi_info->net_id) != WIFI_SUCCESS)
		{
			connect->disconnecting = false;

			_atcmd_info("wifi_disconnect: fail\n");

			return ATCMD_ERROR_FAIL;
		}

		if (connect->connecting)
		{
			_atcmd_wifi_event_polled(WLAN_EVT_CONNECT_SUCCESS);
			_atcmd_wifi_event_polled(WLAN_EVT_CONNECT_FAIL);

			g_atcmd_wifi_info->net_id = -1;

			connect->connected = false;
			connect->connecting = false;
			connect->disconnecting = false;
		}
		else
		{
			uint32_t timeout_msec = _atcmd_timeout_value("WDISCONN");
			uint32_t time;

			for (time = 0 ; connect->disconnecting ; time += 100)
			{
				if (time > 0 && (time % 1000) == 0)
					_atcmd_info("wifi_disconnect: %u sec\n", time / 1000);

				if (timeout_msec > 0 && time >= timeout_msec)
				{
					_atcmd_wifi_event_poll(WLAN_EVT_DISCONNECT);

					_atcmd_info("wifi_disconnect: timeout\n");

					return ATCMD_ERROR_TIMEOUT;
				}

				_delay_ms(100);
			}
		}

		_atcmd_info("wifi_disconnect: done\n");

		return ATCMD_SUCCESS;
	}

	_atcmd_info("wifi_disconnect: busy\n");

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

		_atcmd_debug("wifi_softap: net_id=%d\n", net_id);
	}
	else
		net_id = nrc_wifi_get_nd();

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
	&g_atcmd_wifi_ping,
	&g_atcmd_wifi_softap,
	&g_atcmd_wifi_timeout,

	NULL
};

int atcmd_wifi_enable (void)
{
	int i;

#ifndef ATCMD_WIFI_INFO_STATIC
	g_atcmd_wifi_info = _atcmd_malloc(sizeof(atcmd_wifi_info_t));
	if (!g_atcmd_wifi_info)
	{
		_atcmd_error("malloc() failed\n");
		return -1;
	}
#endif

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

	_atcmd_free(g_atcmd_wifi_info);
}

