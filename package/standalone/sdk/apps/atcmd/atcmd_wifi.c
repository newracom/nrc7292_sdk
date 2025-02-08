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

#include "dhcpserver.h"
#include "lwip/etharp.h"

/* #define CONFIG_SHOW_WIFI_PASSWORD */

/**********************************************************************************************/

extern uint32_t _atcmd_timeout_value (const char *cmd);
extern int _atcmd_basic_timeout_get (int argc, char *argv[]);
extern int _atcmd_basic_timeout_set (int argc, char *argv[]);

static void _atcmd_wifi_connect_update_ap_info (void);
static int _atcmd_wifi_dhcp_run (int argc, char *argv[]);
static void _atcmd_wifi_dhcp_task_resume (void);

/**********************************************************************************************/

//#define ATCMD_WIFI_INFO_STATIC
#ifdef ATCMD_WIFI_INFO_STATIC
static atcmd_wifi_info_t _atcmd_wifi_info;
static atcmd_wifi_info_t *g_atcmd_wifi_info = &_atcmd_wifi_info;
#else
static atcmd_wifi_info_t *g_atcmd_wifi_info = NULL;
#endif

/**********************************************************************************************/

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

static bool _atcmd_wifi_security_valid (atcmd_wifi_security_t security, bool wpa3)
{
	if (strcmp(security, "open") == 0)
		return true;
	else if (strcmp(security, "wpa2") == 0 || strcmp(security, "psk") == 0)
	{
		strcpy(security, "wpa2-psk");
		return true;
	}
	else if (strcmp(security, "wpa2-psk") == 0)
		return true;

#if defined(CONFIG_ATCMD_WPA3)
	if (!wpa3)
		return false;

	if (strcmp(security, "owe") == 0)
	{
		strcpy(security, "wpa3-owe");
		return true;
	}
	else if (strcmp(security, "wpa3-owe") == 0)
		return true;
	else if (strcmp(security, "sae") == 0)
	{
		strcpy(security, "wpa3-sae");
		return true;
	}
	else if (strcmp(security, "wpa3-sae") == 0)
		return true;
#endif

	return false;
}

static bool _atcmd_wifi_password_valid (atcmd_wifi_password_t password)
{
	int len = strlen(password);

	if (len < ATCMD_WIFI_PASSWORD_LEN_MIN || len > ATCMD_WIFI_PASSWORD_LEN_MAX)
		return false;

	return true;
}

/**********************************************************************************************/

static void _atcmd_wifi_channels_print (atcmd_wifi_channels_t *channels, bool nons1g, bool scan_only, const char *fmt, ...)
{
	int bw;
	int i, j;

	if (fmt)
	{
		char buf[128];
		va_list ap;

		va_start(ap, fmt);
		vsnprintf(buf, sizeof(buf), fmt, ap);
		_atcmd_info("%s", buf);
		va_end(ap);
	}

	for (bw = 1 ; bw <= 4; bw <<= 1)
	{
		for (i = j = 0 ; i < channels->n_channel ; i++)
		{
			if (channels->channel[i].bw == bw)
			{
				if (!scan_only || channels->channel[i].scan)
				{
					if ((j % 10) == 0)
						_atcmd_printf(" %dM_BW:", bw);

					_atcmd_printf(" %4d", nons1g ? channels->channel[i].nons1g_freq : channels->channel[i].s1g_freq);

					if ((j % 10) == 9)
						_atcmd_printf("\n");

					j++;
				}
			}
		}

		if ((j % 10) != 0)
			_atcmd_printf("\n");
	}
}

/**********************************************************************************************/

static bool _atcmd_wifi_connecting (void)
{
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;

	return (connect->connecting && !connect->connected && !connect->disconnecting);
}

static bool _atcmd_wifi_connected (void)
{
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;

	return (!connect->connecting && connect->connected && !connect->disconnecting);
}

static bool _atcmd_wifi_disconnecting (void)
{
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;

	return (!connect->connecting && connect->connected && connect->disconnecting);
}

static bool _atcmd_wifi_disconnected (void)
{
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;

	return (!connect->connecting && !connect->connected && !connect->disconnecting);
}

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

static int _atcmd_wifi_scan_result_report (SCAN_RESULTS *results, const char *ssid, bool to_host);

static void _atcmd_wifi_event_scan_done (int vif_id, void *data, int len)
{
	atcmd_wifi_scan_t *scan = &g_atcmd_wifi_info->scan;

	if (scan->scanning)
	{
		scan->scanning = false;

		if (_atcmd_wifi_event_polling(ATCMD_WIFI_EVT_SCAN_DONE))
		{
			_atcmd_wifi_event_polled(ATCMD_WIFI_EVT_SCAN_DONE);

			_atcmd_wifi_scan_result_report(&scan->results, NULL, false);
		}
	}
}

static void _atcmd_wifi_event_connect_success (int vif_id, void *data, int len)
{
	atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;

/*	if (!softap->active) */
	{
		atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
		bool event = true;

		if (_atcmd_wifi_connecting())
		{
			if (_atcmd_wifi_event_polling(ATCMD_WIFI_EVT_CONNECT_SUCCESS))
				_atcmd_wifi_event_polled(ATCMD_WIFI_EVT_CONNECT_SUCCESS);
			else
				event = false;
		}

		connect->recovery = false;
		connect->connected = true;
		connect->connecting = false;
		connect->disconnecting = false;

		_atcmd_wifi_connect_update_ap_info();

		if (event)
		{
			ATCMD_MSG_WEVENT("\"CONNECT_SUCCESS\",\"%s\",\"%s\",\"%s\"",
					connect->bssid, connect->ssid, connect->security);
		}

		_atcmd_wifi_dhcp_task_resume();
	}
}

static void _atcmd_wifi_event_disconnect (int vif_id, void *data, int len)
{
	atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;

/*	if (!softap->active) */
	{
		atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
		bool event = true;

/*		_atcmd_debug("%s: %d %d %d", __func__,
				connect->connected, connect->connecting, connect->disconnecting); */

		if (_atcmd_wifi_disconnected() || _atcmd_wifi_connecting())
			event = false;
		else if (_atcmd_wifi_disconnecting())
		{
			if (_atcmd_wifi_event_polling(ATCMD_WIFI_EVT_DISCONNECT))
				_atcmd_wifi_event_polled(ATCMD_WIFI_EVT_DISCONNECT);
			else
				event = false;
		}

		if (connect->connected || connect->disconnecting)
		{
			connect->connected = false;
			connect->connecting = false;
			connect->disconnecting = false;
		}

		if (event)
		{
			ATCMD_MSG_WEVENT("\"DISCONNECT\",\"%s\",\"%s\",\"%s\"",
					connect->bssid, connect->ssid, connect->security);
		}

		if (connect->recovery)
		{
			connect->recovery = false;

			strcpy(connect->ssid, ATCMD_WIFI_INIT_SSID);
			strcpy(connect->bssid, ATCMD_WIFI_INIT_BSSID);
			strcpy(connect->security, ATCMD_WIFI_INIT_SECURITY);
			strcpy(connect->password, ATCMD_WIFI_INIT_PASSWORD);
		}
	}
}

static void _atcmd_wifi_event_sta_connect (int vif_id, void *data, int len)
{
	atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;

	if (softap->active)
	{
		char addr[ATCMD_WIFI_MACADDR_LEN + 1];
		char *_addr;

		if (!data || len != ETH_ALEN)
		{
			_atcmd_error("[%d] data: %p %d", vif_id, data, len);
			return;
		}

		_addr = (char *)data;
		snprintf(addr, sizeof(addr), "%02X:%02X:%02X:%02X:%02X:%02X",
				_addr[0], _addr[1], _addr[2], _addr[3], _addr[4], _addr[5]);

		if (strlen(addr) != ATCMD_WIFI_MACADDR_LEN)
		{
			_atcmd_error("[%d] snprint()", vif_id);
			return;
		}

		_atcmd_info("[%d] sta_connect: %s", vif_id, addr);

		ATCMD_MSG_WEVENT("\"STA_CONNECT\",\"%s\"", addr);
	}
}

static bool ip_from_arp_cache(char *addr, ip4_addr_t *ip_addr)
{
	if (!addr || !ip_addr) {
		return false;
	}

	for (int i = 0; i < ARP_TABLE_SIZE; i++) {
		ip4_addr_t *ip;
		struct netif *netif;
		struct eth_addr *ethaddr;

		if (etharp_get_entry(i, &ip, &netif, &ethaddr)) {
			if (memcmp(addr, ethaddr->addr, ETH_HWADDR_LEN) == 0) {
				memcpy(ip_addr, ip, sizeof(ip4_addr_t));
				return true;
			}
		}
	}
	return false;
}

static void _atcmd_wifi_event_sta_disconnect (int vif_id, void *data, int len)
{
	atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;

	if (softap->active)
	{
		char addr[ATCMD_WIFI_MACADDR_LEN + 1];
		char *_addr;

		if (!data || len != ETH_ALEN)
		{
			_atcmd_error("[%d] data: %p %d", vif_id, data, len);
			return;
		}

		_addr = (char *)data;
		snprintf(addr, sizeof(addr), "%02X:%02X:%02X:%02X:%02X:%02X",
				_addr[0], _addr[1], _addr[2], _addr[3], _addr[4], _addr[5]);

		if (strlen(addr) != ATCMD_WIFI_MACADDR_LEN)
		{
			_atcmd_error("[%d] snprint()", vif_id);
			return;
		}

#ifdef CONFIG_ATCMD_STA_DISCONNECT_IP_ADDR
		ip4_addr_t ip;
		memset(&ip, 0, sizeof(ip4_addr_t));
		if (!ip_from_arp_cache(_addr, &ip)) {
			dhcps_get_ip((u8_t *)_addr, &ip);
		}
		_atcmd_info("[%d] sta_disconnect: %s %s", vif_id, addr, ip4addr_ntoa(&ip));
		ATCMD_MSG_WEVENT("\"STA_DISCONNECT\",\"%s\", \"%s\"", addr, ip4addr_ntoa(&ip));
#else
		_atcmd_info("[%d] sta_disconnect: %s", vif_id, addr);
		ATCMD_MSG_WEVENT("\"STA_DISCONNECT\",\"%s\"", addr);
#endif
	}
}

/**********************************************************************************************/

static int _atcmd_wifi_macaddr_get (int argc, char *argv[])
{
	atcmd_wifi_macaddr_t macaddr[2];

	switch (argc)
	{
		case 0:
			if (wifi_api_get_macaddr(macaddr[0], macaddr[1]) != 0)
				return ATCMD_ERROR_FAIL;

			_atcmd_info("wifi_macaddr_get: %s %s", macaddr[0], macaddr[1]);
			ATCMD_MSG_INFO("WMACADDR", "\"%s\",\"%s\"", macaddr[0], macaddr[1]);
			break;
/*
		case 1:
			if (wifi_api_get_macaddr(macaddr[0], NULL) != 0)
				return ATCMD_ERROR_FAIL;

			if (argc == 0)
			{
				_atcmd_info("wifi_macaddr_get: %s", macaddr[0]);
				ATCMD_MSG_INFO("WMACADDR", "\"%s\"", macaddr[0]);
			}
			else
			{
				_atcmd_info("wifi_macaddr0_get: %s", macaddr[0]);
				ATCMD_MSG_INFO("WMACADDR0", "\"%s\"", macaddr[0]);
			}
			break;

		case 2:
			if (wifi_api_get_macaddr(NULL, macaddr[1]) != 0)
				return ATCMD_ERROR_FAIL;

			_atcmd_info("wifi_macaddr1_get: %s", macaddr[1]);
			ATCMD_MSG_INFO("WMACADDR1", "\"%s\"", macaddr[1]);
			break;
*/
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

/*
static int _atcmd_wifi_macaddr0_get (int argc, char *argv[])
{
	return _atcmd_wifi_macaddr_get(1, NULL);
}

static atcmd_info_t g_atcmd_wifi_macaddr0 =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "MACADDR0",
	.id = ATCMD_WIFI_MACADDR0,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_macaddr0_get,
	.handler[ATCMD_HANDLER_SET] = NULL,
};
*/

/**********************************************************************************************/

/*
static int _atcmd_wifi_macaddr1_get (int argc, char *argv[])
{
	return _atcmd_wifi_macaddr_get(2, NULL);
}

static atcmd_info_t g_atcmd_wifi_macaddr1 =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "MACADDR1",
	.id = ATCMD_WIFI_MACADDR1,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_macaddr1_get,
	.handler[ATCMD_HANDLER_SET] = NULL,
};
*/

/**********************************************************************************************/

static int _atcmd_wifi_country_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			atcmd_wifi_country_t country;

			if (wifi_api_get_country(country) != 0)
				return ATCMD_ERROR_FAIL;

			_atcmd_info("wifi_country_get: %s", country);

			if (strcmp(country, g_atcmd_wifi_info->country) != 0)
			{
				_atcmd_error("mismatch, %s -> %s",
								g_atcmd_wifi_info->country, country);

				strcpy(g_atcmd_wifi_info->country, country);
			}

			ATCMD_MSG_INFO("WCOUNTRY", "\"%s\"", country);
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
		{
			char buf[3 * COUNTRY_CODE_MAX];
			int len;
			int i;

			for (len = i = 0 ; len < sizeof(buf) && g_wifi_country_list[i].cc_index < COUNTRY_CODE_MAX ; i++)
				len += snprintf(buf + len, sizeof(buf) - len, "%s|", g_wifi_country_list[i].alpha2_cc);

			buf[--len] = '\0';

			ATCMD_MSG_HELP("AT+WCOUNTRY=\"{%s}\"", buf);
			break;
		}

		case 1:
		{
			atcmd_wifi_country_t country;

			param_country = argv[0];

			if (!atcmd_param_to_str(param_country, country, sizeof(country)))
				return ATCMD_ERROR_FAIL;

			if (!wifi_api_valid_country(country))
				return ATCMD_ERROR_INVAL;

			if (strcmp(country, "KR") == 0)
				strcpy(country, "K2");

/*			_atcmd_debug("wifi_country_set: %s", country); */

			if (wifi_api_set_country(country) != 0)
				return ATCMD_ERROR_FAIL;
			else
			{
				atcmd_wifi_channels_t *channels = &g_atcmd_wifi_info->supported_channels;

				strcpy(g_atcmd_wifi_info->country, country);

				if (wifi_api_get_supported_channels(country, channels) == 0)
				{
					_atcmd_wifi_channels_print(channels, false, false,
								"wifi_country_set: %s (%d)", country, channels->n_channel);
				}

				break;
			}
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
	uint32_t count = 1;
	uint32_t interval = 0;

	switch (argc)
	{
		case 2: /* AT+WTXPOWER?=<count>,<interval> */
			if (atcmd_param_to_uint32(argv[1], &interval) != 0)
				return ATCMD_ERROR_INVAL;

		case 1: /* AT+WTXPOWER?=<count> */
			if (atcmd_param_to_uint32(argv[0], &count) != 0)
				return ATCMD_ERROR_INVAL;

			if (interval == 0)
				interval = 1;

		case 0:
		{
			uint8_t power[2] = { 0, 0 };
			uint32_t i;

			for (i = 0 ; i < count ; i++)
			{
				power[0] = power[1] = 0;

				if (wifi_api_get_tx_power(&power[0], &power[1]) != 0)
					return ATCMD_ERROR_FAIL;

				_atcmd_info("tx_power_get: type=%s,%u power=%u,%u",
						str_txpwr_type[g_atcmd_wifi_info->txpower.type],
						g_atcmd_wifi_info->txpower.val, power[0], power[1]);

				if (!wifi_api_is_relay_mode())
					power[1] = 0;

				ATCMD_MSG_INFO("WTXPOWER", "%u,%u,\"%s\",%u", power[0], power[1],
						str_txpwr_type[g_atcmd_wifi_info->txpower.type],
						g_atcmd_wifi_info->txpower.val);
				
				if (interval > 0)
					_delay_ms(interval * 1000);
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
	enum TX_POWER_TYPE type = TX_POWER_NONE;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WTXPOWER=<power:%d..%d>[,\"{auto|fixed|limit}\"]",
					ATCMD_WIFI_TXPOWER_MIN, ATCMD_WIFI_TXPOWER_MAX);
			break;

		case 2:
		{
			char str_type[6];

			if (!atcmd_param_to_str(argv[1], str_type, sizeof(str_type)))
				return ATCMD_ERROR_INVAL;

			if (strcmp(str_type, "auto") == 0)
				type = TX_POWER_AUTO;
			else if (strcmp(str_type, "fixed") == 0)
				type = TX_POWER_FIXED;
			else if (strcmp(str_type, "limit") == 0)
				type = TX_POWER_LIMIT;
			else
				return ATCMD_ERROR_INVAL;
		}

		case 1:
		{
			uint8_t power;			

			if (atcmd_param_to_uint8(argv[0], &power) == 0)
			{
				switch (type)
				{
					case TX_POWER_AUTO:
						power = ATCMD_WIFI_TXPOWER_MAX;
						break;

					case TX_POWER_FIXED:
					case TX_POWER_LIMIT:
						break;

					default:
						if (power > 0)
							type = TX_POWER_FIXED;
						else
						{
							type = TX_POWER_AUTO;
							power = ATCMD_WIFI_TXPOWER_MAX;
						}
				}

				if (power >= ATCMD_WIFI_TXPOWER_MIN && power <= ATCMD_WIFI_TXPOWER_MAX)
				{
					_atcmd_info("tx_power_set: type=%s power=%d", str_txpwr_type[type], power);

					if (wifi_api_set_tx_power(type, power) != 0)
						return ATCMD_ERROR_FAIL;

					g_atcmd_wifi_info->txpower.type = type;
					g_atcmd_wifi_info->txpower.val = power;
					break;
				}

				_atcmd_info("tx_power_set: invalid power %d", type, power);
			}
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
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
	atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;
	bool rssi_average = false;

	if (!wifi_api_is_relay_mode() && softap->active)
	{
		_atcmd_info("wifi_rxsig_get: softap mode");
		return ATCMD_ERROR_NOTSUPP;
	}

	if (!connect->connected)
	{
		_atcmd_info("wifi_rxsig_get: not connected");
		return ATCMD_ERROR_NOTSUPP;
	}

	switch (argc)
	{
		case 1:
			rssi_average = true;

		case 0:
		{
			int8_t rssi;
			uint8_t snr;

			if (wifi_api_get_rssi(&rssi, rssi_average) != 0)
				return ATCMD_ERROR_FAIL;

			if (wifi_api_get_snr(&snr) != 0)
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
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
	atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;
	int average = 0;

	if (!wifi_api_is_relay_mode() && softap->active)
	{
		_atcmd_info("wifi_rxsig_set: softap mode");
		return ATCMD_ERROR_NOTSUPP;
	}

	if (!connect->connected)
	{
		_atcmd_info("wifi_rxsig_set: not connected");
		return ATCMD_ERROR_NOTSUPP;
	}

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WRXSIG=<time>[,<average>]");
			break;

		case 2:
			average = atoi(argv[1]);
			if (average != 0 && average != 1)
				return ATCMD_ERROR_INVAL;

		case 1:
		{
			int time_sec = atoi(argv[0]);
			int ret;
			int i;

			_atcmd_info("wifi_rxsig_set: time=%d average=%d", time_sec, average);

			for (i = 0 ; i < time_sec ; i++)
			{
				ret = _atcmd_wifi_rx_signal_get(average, NULL);
				if (ret != ATCMD_SUCCESS)
					return ret;

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
			bool rate_ctrl;

			if (wifi_api_get_rate_control(&rate_ctrl) != 0)
				return ATCMD_ERROR_FAIL;

			_atcmd_info("wifi_rate_ctrl_get: %s", rate_ctrl ? "enabled" : "disabled");

			ATCMD_MSG_INFO("WRATECTRL", "%d", rate_ctrl ? 1 : 0);
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
			ATCMD_MSG_HELP("AT+WRATECTRL={0|1}");
			break;

		case 1:
		{
			int param = atoi(argv[0]);

			if (param == 0 || param == 1)
			{
				bool rate_ctrl = !!param;

				_atcmd_info("wifi_rate_ctrl_set: %s", rate_ctrl ? "enable" : "disable");

				wifi_api_set_rate_control(rate_ctrl);
				break;
			}
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
	uint32_t count = 1;
	uint32_t interval = 0;

	switch (argc)
	{
		case 2: /* AT+WMCS?=<count>,<interval> */
			if (atcmd_param_to_uint32(argv[1], &interval) != 0)
				return ATCMD_ERROR_INVAL;

		case 1: /* AT+WMCS?=<count> */
			if (atcmd_param_to_uint32(argv[0], &count) != 0)
				return ATCMD_ERROR_INVAL;

			if (interval == 0)
				interval = 1;
		case 0:
		{
			wifi_mcs_t index0, index1;
			uint32_t i;

			for (i = 0 ; i < count ; i++)
			{
				memset(&index0, 0, sizeof(wifi_mcs_t));
				memset(&index1, 0, sizeof(wifi_mcs_t));

				if (wifi_api_get_mcs(&index0, &index1) != 0)
					return ATCMD_ERROR_FAIL;

				if (!wifi_api_is_relay_mode())
					index1.tx = index1.rx = 0;

				_atcmd_info("wifi_mcs_get: tx=%d,%d rx=%d,%d",
						index0.tx, index1.tx, index0.rx, index1.rx);

				ATCMD_MSG_INFO("WMCS", "%u,%u,%u,%u", index0.tx, index1.tx, index0.rx, index1.rx);

				if (interval > 0)
					_delay_ms(interval * 1000);
			}

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_mcs_set (int argc, char *argv[])
{
	bool rate_ctrl;

	wifi_api_get_rate_control(&rate_ctrl);
	if (rate_ctrl)
		return ATCMD_ERROR_NOTSUPP;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WMCS={0..7|10}");
			break;

		case 1:
		{
			int index = atoi(argv[0]);

			if ((index >= 0 && index <= 7) || index == 10)
			{
				_atcmd_info("wifi_mcs_set: %d", index);

				if (wifi_api_set_mcs(index) != 0)
					return ATCMD_ERROR_FAIL;

				break;
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

			if (wifi_api_get_duty_cycle(&window, &duration, &margin) != 0)
				return ATCMD_ERROR_FAIL;

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

			if (wifi_api_get_duty_cycle(&params.window, &params.duration, &params.margin) != 0)
				return ATCMD_ERROR_FAIL;

			for (i = 0 ; i < argc ; i++)
			{
				if (atcmd_param_to_uint32(argv[i], &params.val[i]) != 0)
					return ATCMD_ERROR_INVAL;
			}

			_atcmd_info("wifi_duty_cycle: window=%u duration=%u margin=%u",
							params.window, params.duration, params.margin);

			if (wifi_api_set_duty_cycle(params.window, params.duration, params.margin) != 0)
				return ATCMD_ERROR_FAIL;

			break;
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
			int threshold;

			if (wifi_api_get_cca_threshold(&threshold) != 0)
				return ATCMD_ERROR_FAIL;

			_atcmd_info("wifi_cca_threshold_get: %d", threshold);

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
			ATCMD_MSG_HELP("AT+WCCATHRESHOLD={%d..%d}", CCA_THRESHOLD_MIN, CCA_THRESHOLD_MAX);
			break;

		case 1:
		{
			int32_t threshold;

			if (atcmd_param_to_int32(argv[0], &threshold) != 0)
				return ATCMD_ERROR_INVAL;

			if (threshold < CCA_THRESHOLD_MIN || threshold > CCA_THRESHOLD_MAX)
				return ATCMD_ERROR_INVAL;

			_atcmd_info("wifi_cca_threshold_set: %d", threshold);

			if (wifi_api_set_cca_threshold(threshold) != 0)
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
			uint16_t cs_time = 0;
			uint32_t pause_time = 0;

			if (wifi_api_get_tx_time(&cs_time, &pause_time) != 0)
				return ATCMD_ERROR_FAIL;

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

	if (wifi_api_get_tx_time(&cs_time, &pause_time) != 0)
	{
		cs_time = 0;
		pause_time = 0;
	}

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WTXTIME=<cs_time>,<pause_time>");
			break;

		case 2:
			if (atcmd_param_to_uint32(argv[1], &pause_time) != 0)
				return ATCMD_ERROR_INVAL;

			if (atcmd_param_to_uint16(argv[0], &cs_time) != 0)
				return ATCMD_ERROR_INVAL;

			if (wifi_api_set_tx_time(cs_time, pause_time) != 0)
				return ATCMD_ERROR_FAIL;

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
			uint64_t tsf[2];

			if (wifi_api_get_tsf(&tsf[0], &tsf[1]) != 0)
				return ATCMD_ERROR_FAIL;

			if (wifi_api_is_relay_mode())
			{
				_atcmd_info("wifi_tsf_get: %llu %llu (0x%llX 0x%llX)", tsf[0], tsf[1], tsf[0], tsf[1]);

				ATCMD_MSG_INFO("WTSF", "%llu,%llu", tsf[0], tsf[1]);
			}
			else
			{
				_atcmd_info("wifi_tsf_get: %llu (0x%llX)", tsf[0], tsf[0]);

				ATCMD_MSG_INFO("WTSF", "%llu", tsf[0]);
			}

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

static int _atcmd_wifi_beacon_interval_get (int argc, char *argv[])
{
	if (!_atcmd_wifi_connected())
	{
		_atcmd_info("beacon_interval_get: not connected");

		return ATCMD_ERROR_NOTSUPP;
	}

	switch (argc)
	{
		case 0:
		{
			uint16_t beacon_interval = 0;

			if (wifi_api_get_beacon_interval(&beacon_interval) != 0)
				return ATCMD_ERROR_FAIL;

			_atcmd_info("beacon_interval_get: %uTU", beacon_interval);

			ATCMD_MSG_INFO("WBI", "%u", beacon_interval);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_beacon_interval =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "BI",
	.id = ATCMD_WIFI_BEACON_INTERVAL,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_beacon_interval_get,
	.handler[ATCMD_HANDLER_SET] = NULL,
};

/**********************************************************************************************/

static int _atcmd_wifi_listen_interval_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			uint16_t listen_interval = 0;
			uint32_t listen_interval_tu = 0;

			if (wifi_api_get_listen_interval(&listen_interval, &listen_interval_tu) != 0)
				return ATCMD_ERROR_FAIL;

			_atcmd_info("listen_interval_get: %uBI, %uTU", listen_interval, listen_interval_tu);

			ATCMD_MSG_INFO("WLI", "%u", listen_interval);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_listen_interval_set (int argc, char *argv[])
{
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;

	if (!_atcmd_wifi_disconnected())
	{
		_atcmd_info("listen_interval_get: not disconnected");

		return ATCMD_ERROR_NOTSUPP;
	}

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WLI=<listen_interval>");
			break;

		case 1:
		{
			uint16_t listen_interval;

			if (atcmd_param_to_uint16(argv[0], &listen_interval) == 0)
			{
				_atcmd_info("listen_interval_set: %u", listen_interval);

				if (wifi_api_set_listen_interval(listen_interval) == 0)
					break;

				return ATCMD_ERROR_FAIL;
			}
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_listen_interval =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "LI",
	.id = ATCMD_WIFI_LISTEN_INTERVAL,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_listen_interval_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_listen_interval_set,
};

/**********************************************************************************************/

static bool _atcmd_wifi_scan_freq_valid (uint16_t s1g_freq, int bw)
{
	switch (bw)
	{
		case 0:
		case 1:
		case 2:
		case 4:
		{
			atcmd_wifi_channels_t *supported_channels = &g_atcmd_wifi_info->supported_channels;
			int i;

			for (i = 0 ; i < supported_channels->n_channel ; i++)
			{
				if (s1g_freq == supported_channels->channel[i].s1g_freq)
				{
					if (bw == 0 || bw == supported_channels->channel[i].bw)
					{
/*						_atcmd_debug("scan_freq_valid: %u@%d", s1g_freq, bw); */
						return true;
					}
				}
			}
		}
	}

	return false;
}

static int _atcmd_wifi_scan_freq_get (atcmd_wifi_channels_t *channels, bool print)
{
	uint16_t nons1g_freq[WIFI_CHANNEL_NUM_MAX];
	uint8_t n_freq;
	int i, j;

	ATCMD_WIFI_LOCK();

	if (wifi_api_add_network(false) != 0)
	{
		ATCMD_WIFI_UNLOCK();
		_atcmd_info("scan_freq_get: failed add_network()");
		return -1;
	}

	ATCMD_WIFI_UNLOCK();

	if (wifi_api_get_scan_freq(nons1g_freq, &n_freq) != 0)
	{
/*		_atcmd_info("scan_freq_get: full channels"); */

		for (i = 0 ; i < channels->n_channel ; i++)
			channels->channel[i].scan = 1;

		return channels->n_channel;
	}

	if (n_freq > WIFI_CHANNEL_NUM_MAX)
	{
		_atcmd_info("scan_freq_get: n_freq(%d) > %d", n_freq, WIFI_CHANNEL_NUM_MAX);
		return -1;
	}

	for (i = 0 ; i < channels->n_channel ; i++)
	{
		for (j = 0 ; j < n_freq ; j++)
		{
			if (channels->channel[i].nons1g_freq == nons1g_freq[j])
			{
				channels->channel[i].scan = 1;
				nons1g_freq[j] = 0;
				break;
			}
		}

		if (j == n_freq)
			channels->channel[i].scan = 0;
	}

	for (i = 0 ; i < n_freq ; i++)
	{
		if (nons1g_freq[i] != 0)
			_atcmd_info("scan_freq_get: ignored nons1g_freq %u", nons1g_freq[i]);
	}

	if (print)
		_atcmd_wifi_channels_print(channels, false, true, "scan_freq_get");

	return n_freq;
}

static int __atcmd_wifi_scan_freq_set (atcmd_wifi_channels_t *channels)
{
	uint16_t nons1g_freq[WIFI_CHANNEL_NUM_MAX];
	uint8_t n_freq;
	int i;

	for (n_freq = i = 0 ; channels && i < channels->n_channel ; i++)
	{
		if (channels->channel[i].scan)
		{
/*			_atcmd_debug("scan_freq_set: %u %u %u", n_freq,
					channels->channel[i].s1g_freq, channels->channel[i].nons1g_freq); */

			nons1g_freq[n_freq++] = channels->channel[i].nons1g_freq;
		}
	}

	ATCMD_WIFI_LOCK();

	if (wifi_api_add_network(false) != 0)
	{
		ATCMD_WIFI_UNLOCK();
		_atcmd_info("scan_freq_set: failed add_network()");
		return -1;
	}

	ATCMD_WIFI_UNLOCK();

	if (wifi_api_set_scan_freq(n_freq ? nons1g_freq : NULL, n_freq) != 0)
	{
		_atcmd_info("scan_freq_set: failed");
		return -1;
	}

	return 0;
}

static int _atcmd_wifi_scan_freq_set (int n_scan_channel, atcmd_wifi_channel_t scan_channel[])
{
	atcmd_wifi_channels_t *supported_channels = &g_atcmd_wifi_info->supported_channels;
	char buf[ATCMD_WIFI_SCAN_SET_PARAM_MAX * 7];
	char *pbuf = buf;
	int i, j;

	if (_atcmd_wifi_scan_freq_get(supported_channels, false) <= 0)
		return -1;

	for (i = 0 ; i < supported_channels->n_channel; i++)
	{
		for (j = 0 ; j < n_scan_channel ; j++)
		{
			if (scan_channel[j].s1g_freq == supported_channels->channel[i].s1g_freq)
			{
				if (scan_channel[j].bw == 0 || scan_channel[j].bw == supported_channels->channel[i].bw)
				{
					pbuf += sprintf(pbuf, " %u@%u", supported_channels->channel[i].s1g_freq,
										supported_channels->channel[i].bw);

					supported_channels->channel[i].scan = 1;
					scan_channel[j].s1g_freq = 0;
					break;
				}
			}
		}

		if (j == n_scan_channel)
			supported_channels->channel[i].scan = 0;
	}

	_atcmd_wifi_channels_print(supported_channels, false, true, "scan_freq_set:%s", buf);

	for (i = 0 ; i < n_scan_channel ; i++)
	{
		if (scan_channel[i].s1g_freq != 0)
			_atcmd_info("scan_freq_set: ignored channel %u@%u", scan_channel[i].s1g_freq, scan_channel[i].bw);
	}

	return __atcmd_wifi_scan_freq_set(supported_channels);
}

static int _atcmd_wifi_scan_freq_add (int n_scan_channel, atcmd_wifi_channel_t scan_channel[])
{
	atcmd_wifi_channels_t *supported_channels = &g_atcmd_wifi_info->supported_channels;
	char buf[ATCMD_WIFI_SCAN_SET_PARAM_MAX * 7];
	char *pbuf = buf;
	int i, j;

	if (_atcmd_wifi_scan_freq_get(supported_channels, false) <= 0)
		return -1;

	for (i = 0 ; i < supported_channels->n_channel; i++)
	{
		for (j = 0 ; j < n_scan_channel ; j++)
		{
			if (scan_channel[j].s1g_freq == supported_channels->channel[i].s1g_freq)
			{
				if (scan_channel[j].bw == 0 || scan_channel[j].bw == supported_channels->channel[i].bw)
				{
					pbuf += sprintf(pbuf, " %u@%u", supported_channels->channel[i].s1g_freq,
										supported_channels->channel[i].bw);

					supported_channels->channel[i].scan = 1;
					scan_channel[j].s1g_freq = 0;
					break;
				}
			}
		}
	}

	_atcmd_wifi_channels_print(supported_channels, false, true, "scan_freq_add:%s", buf);

	for (i = 0 ; i < n_scan_channel ; i++)
	{
		if (scan_channel[i].s1g_freq != 0)
			_atcmd_info("scan_freq_add: ignored channel %u@%u", scan_channel[i].s1g_freq, scan_channel[i].bw);
	}

	return __atcmd_wifi_scan_freq_set(supported_channels);
}

static int _atcmd_wifi_scan_freq_delete (int n_scan_channel, atcmd_wifi_channel_t scan_channel[])
{
	atcmd_wifi_channels_t *supported_channels =  &g_atcmd_wifi_info->supported_channels;
	char buf[ATCMD_WIFI_SCAN_SET_PARAM_MAX * 7];
	char *pbuf = buf;
	int i, j;

	if (_atcmd_wifi_scan_freq_get(supported_channels, false) <= 0)
		return -1;

	for (i = 0 ; i < supported_channels->n_channel; i++)
	{
		for (j = 0 ; j < n_scan_channel ; j++)
		{
			if (scan_channel[j].s1g_freq == supported_channels->channel[i].s1g_freq)
			{
				if (scan_channel[j].bw == 0 || scan_channel[j].bw == supported_channels->channel[i].bw)
				{
					pbuf += sprintf(pbuf, " %u@%u", supported_channels->channel[i].s1g_freq,
										supported_channels->channel[i].bw);

					supported_channels->channel[i].scan = 0;
					scan_channel[j].s1g_freq = 0;
					break;
				}
			}
		}
	}

	_atcmd_wifi_channels_print(supported_channels, false, true, "scan_freq_del:%s", buf);

	for (i = 0 ; i < n_scan_channel ; i++)
	{
		if (scan_channel[i].s1g_freq != 0)
			_atcmd_info("scan_freq_del: ignored channel %u@%u", scan_channel[i].s1g_freq, scan_channel[i].bw);
	}

	return __atcmd_wifi_scan_freq_set(supported_channels);
}

static int _atcmd_wifi_scan_result_report (SCAN_RESULTS *results, const char *ssid, bool to_host)
{
	uint16_t s1g_freq;
	SCAN_RESULT *result;
	int i;

/*	memset(results, 0, sizeof(SCAN_RESULTS)); */

	if (wifi_api_get_scan_results(results) != 0)
		return ATCMD_ERROR_FAIL;

	for (i = 0 ; i < results->n_result ; i++)
	{
		result = &results->result[i];

		if (!to_host)
			continue;

		s1g_freq = atoi(result->freq);
		if (s1g_freq > 0)
		{
			if (ssid)
			{
				if (strcmp(ssid, result->ssid) == 0)
				{
					ATCMD_MSG_INFO("WSCANSSID", "\"%s\",%.1f@%d,%d,\"%s\",\"%s\"",
							result->bssid, s1g_freq / 10., result->bandwidth, (int8_t)atoi(result->sig_level),
							result->flags, result->ssid);
				}
			}
			else
			{
				ATCMD_MSG_INFO("WSCAN", "\"%s\",%.1f@%d,%d,\"%s\",\"%s\"",
						result->bssid, s1g_freq / 10., result->bandwidth, (int8_t)atoi(result->sig_level),
						result->flags, result->ssid);
			}
		}
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_scan_run (int argc, char *argv[])
{
	atcmd_wifi_scan_t *scan = &g_atcmd_wifi_info->scan;
	bool scan_retry = false;
	char *ssid = NULL;
	uint32_t timeout_msec = _atcmd_timeout_value("WSCAN");

	if (argc == 1)
		ssid = argv[0];

	ATCMD_WIFI_LOCK();

	if (scan->scanning)
	{
		_atcmd_info("wifi_scan: busy");

		ATCMD_WIFI_UNLOCK();

		return ATCMD_ERROR_BUSY;
	}

	if (wifi_api_add_network(false) != 0)
	{
		ATCMD_WIFI_UNLOCK();
		_atcmd_error("failed add_network()");
		return ATCMD_ERROR_FAIL;
	}

	_atcmd_info("wifi_scan: run");

_atcmd_wifi_scan_retry:

	scan->scanning = true;
	memset(&scan->results, 0, sizeof(SCAN_RESULTS));

	if (wifi_api_start_scan(ssid, timeout_msec) == 0)
	{
		uint32_t time;

		for (time = 0 ; scan->scanning ; time += 100)
		{
			if (time > 0 && (time % 1000) == 0)
				_atcmd_info("wifi_scan: %u sec", time / 1000);

			if (timeout_msec > 0 && time >= timeout_msec)
			{
				_atcmd_wifi_event_poll(ATCMD_WIFI_EVT_SCAN_DONE);

				_atcmd_info("wifi_scan: timeout");

				ATCMD_WIFI_UNLOCK();

				return ATCMD_ERROR_TIMEOUT;
			}

			_delay_ms(100);
		}

		_atcmd_wifi_scan_result_report(&scan->results, ssid, true);

		if (scan->results.n_result == 0 && !scan_retry)
		{
			scan_retry = true;

			_atcmd_info("wifi_scan: retry");

			goto _atcmd_wifi_scan_retry;
		}

		_atcmd_info("wifi_scan: done");

		ATCMD_WIFI_UNLOCK();

		return ATCMD_SUCCESS;
	}

	scan->scanning = false;

	_atcmd_info("wifi_scan: fail");

	ATCMD_WIFI_UNLOCK();

	return ATCMD_ERROR_FAIL;
}

static int _atcmd_wifi_scan_get (int argc, char *argv[])
{
	atcmd_wifi_channels_t *supported_channels = &g_atcmd_wifi_info->supported_channels;

	switch (argc)
	{
		case 0:
		{
			char buf[ATCMD_MSG_LEN_MAX + 1];
			char *pbuf = buf;
			int n_scan_channel;
			int bw;
			int i, j;

			n_scan_channel = _atcmd_wifi_scan_freq_get(supported_channels, true);
			if (n_scan_channel <= 0)
				return ATCMD_ERROR_FAIL;

			for (n_scan_channel = 0, bw = 1 ; bw <= 4; bw <<= 1)
			{
				for (i = j = 0 ; i < supported_channels->n_channel ; i++)
				{
					if (supported_channels->channel[i].bw == bw && supported_channels->channel[i].scan)
					{
						if ((j % 10) == 0)
						{
							pbuf = buf;
							pbuf += sprintf(pbuf, "%d", bw);
						}

						n_scan_channel++;
						pbuf += sprintf(pbuf, ",%3.1f", supported_channels->channel[i].s1g_freq / 10.);

						if ((j % 10) == 9)
							ATCMD_MSG_INFO("WSCAN", "%s", buf);

						j++;
					}
				}

				if (pbuf > buf && (j % 10) != 0)
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
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;

	if (connect->connecting || connect->disconnecting || connect->connected)
		return ATCMD_ERROR_NOTSUPP;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WSCAN=[{+|-}]<freq>[@<bandwidth>][,<freq>[@<bandwidth>] ...]");
			break;

		default:
		{
			int (*func[3]) (int, atcmd_wifi_channel_t *) =
			{
				_atcmd_wifi_scan_freq_set,
				_atcmd_wifi_scan_freq_add,
				_atcmd_wifi_scan_freq_delete,
			};
			char *param_bw;
			char *param_freq;
			atcmd_wifi_channel_t channel[ATCMD_WIFI_SCAN_SET_PARAM_MAX];
			float f_freq;
			uint16_t freq;
			int bw;
			int mode;
			int i;

			if (argc > ATCMD_WIFI_SCAN_SET_PARAM_MAX)
			{
				_atcmd_info("wifi_scan_set: argc > %d", ATCMD_WIFI_SCAN_SET_PARAM_MAX);
				return ATCMD_ERROR_INVAL;
			}

			if (argc == 1 && strcmp(argv[0], "0") == 0)
			{
				_atcmd_info("wifi_scan_set: init");

				if (__atcmd_wifi_scan_freq_set(NULL) != 0)
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
				param_freq = argv[i];
				param_bw = strchr(param_freq, '@');
				if (!param_bw)
					bw = 0;
				else
				{
					*param_bw = '\0';

					bw = atoi(++param_bw);
					switch (bw)
					{
						case 1:
						case 2:
						case 4:
							break;

						default:
							_atcmd_info("wifi_scan_set: invalid bandwidth %s (%d)", param_bw, bw);
							return ATCMD_ERROR_INVAL;
					}
				}

				if (atcmd_param_to_float(param_freq, &f_freq) != 0)
				{
					_atcmd_info("wifi_scan_set: invalid frequency %s", param_freq);
					return ATCMD_ERROR_INVAL;
				}

				freq = (uint16_t)(f_freq * 10);

				if (!_atcmd_wifi_scan_freq_valid(freq, bw))
				{
					_atcmd_info("wifi_scan_set: invalid channel %u@%d", freq, bw);
					return ATCMD_ERROR_INVAL;
				}

				channel[i].bw = bw;
				channel[i].s1g_freq = freq;
			}

			if (func[mode](i, channel) != 0)
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

static int _atcmd_wifi_scan_ssid_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WSCANSSID=\"<ssid>\"");
			break;

		case 1:
		{
			char ssid[ATCMD_STR_SIZE(ATCMD_WIFI_SSID_LEN_MAX)];

			if (atcmd_param_to_str(argv[0], ssid, sizeof(ssid)))
			{
				int ssid_len = strlen(ssid);

				if (ssid_len > 0 && ssid_len <= ATCMD_WIFI_SSID_LEN_MAX)
				{
					_atcmd_info("wifi_scan_ssid: %s", ssid);
					argv[0] = ssid;
					return _atcmd_wifi_scan_run(1, argv);
				}
			}
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_scan_ssid =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "SCANSSID",
	.id = ATCMD_WIFI_SCAN_SSID,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = NULL,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_scan_ssid_set,
};

/**********************************************************************************************/

#ifdef CONFIG_ATCMD_BGSCAN

static int _atcmd_wifi_scan_background_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			atcmd_wifi_bgscan_t *bgscan = &g_atcmd_wifi_info->bgscan;

			_atcmd_info("wifi_bgscan_get: scanning=%d interval=%d,%d threshold=%d",
						bgscan->scanning,
						bgscan->short_interval, bgscan->long_interval,
						bgscan->signal_threshold);

			ATCMD_MSG_INFO("WBGSCAN", "%d,%d,%d,%d",
						bgscan->scanning,
						bgscan->short_interval, bgscan->long_interval,
						bgscan->signal_threshold);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_scan_background_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			/*
			 * short_interval: short scan interval (sec)
			 * ong_interval: long scan interval (sec)
			 * signal_threshold: short/long interval choice signal threshold (ex : -45)
			 */
			ATCMD_MSG_HELP("AT+WBGSCAN=<short_interval>,<long_interval>,<signal_threshold>");
			break;

		case 3:
		{
			int short_interval = atoi(argv[0]);
			int long_interval = atoi(argv[1]);
			int signal_threshold = atoi(argv[2]);

			if (short_interval >= 0 && long_interval >= 0 && signal_threshold <= 0)
			{
				atcmd_wifi_bgscan_t *bgscan = &g_atcmd_wifi_info->bgscan;

				_atcmd_info("wifi_bgscan_set: interval=%d,%d threshold=%d",
							short_interval, long_interval, signal_threshold);

				if (short_interval == 0 && long_interval == 0 && signal_threshold == 0)
					bgscan->scanning = false;
				else
					bgscan->scanning = true;

				bgscan->short_interval = short_interval;
				bgscan->long_interval = long_interval;
				bgscan->signal_threshold = signal_threshold;
			}
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_scan_background =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "BGSCAN",
	.id = ATCMD_WIFI_SCAN_BACKGROUND,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_scan_background_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_scan_background_set,
};

#endif /* #ifdef CONFIG_ATCMD_BGSCAN */

/**********************************************************************************************/

#if defined(CONFIG_ATCMD_SAEPWE) && defined(CONFIG_ATCMD_WPA3)

#if 1
static int _atcmd_wifi_sae_pwe_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			_atcmd_info("wifi_sae_pwe_get: sta=%d ap=%d relay=%d,%d", 
					g_atcmd_wifi_info->sae_pwe_sta,
					g_atcmd_wifi_info->sae_pwe_ap,
					g_atcmd_wifi_info->sae_pwe_relay_sta,
					g_atcmd_wifi_info->sae_pwe_relay_ap);

			ATCMD_MSG_INFO("WSAEPWE", "%d", g_atcmd_wifi_info->sae_pwe[0]);
			break;

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_sae_pwe_set (int argc, char *argv[])
{
	if (!_atcmd_wifi_disconnected())
		return ATCMD_ERROR_NOTSUPP; 

	switch (argc)
	{
		case 0:
			/*
			 * <sae_pwe>
			 *   SAE mechanism for PWE derivation
			 *    - 0 = hunting-and-pecking loop only (default without password identifier)
			 *    - 1 = hash-to-element only (default with password identifier)
			 *    - 2 = both hunting-and-pecking loop and hash-to-element enabled
			 *    - Note: The default value is likely to change from 0 to 2 once the new
			 *            hash-to-element mechanism has received more interoperability testing.
			 *            When using SAE password identifier, the hash-to-element mechanism is used
			 *            regardless of the sae_pwe parameter value.
			 */
			ATCMD_MSG_HELP("AT+WSAEPWE=<sae_pwe>");
			break;

		case 1:
		{
			int sae_pwe = atoi(argv[0]);

			if (sae_pwe >= 0 && sae_pwe <= 2)
			{
				int i;

				_atcmd_info("wifi_sae_pwe_set: %d", sae_pwe);

				for (i = 0 ; i < 4 ; i++)
					g_atcmd_wifi_info->sae_pwe[i] = sae_pwe;

				break;
			}
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}
#else
static int _atcmd_wifi_sae_pwe_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			_atcmd_info("wifi_sae_pwe_get: sta=%d ap=%d relay=%d,%d", 
					g_atcmd_wifi_info->sae_pwe_sta,
					g_atcmd_wifi_info->sae_pwe_ap,
					g_atcmd_wifi_info->sae_pwe_relay_sta,
					g_atcmd_wifi_info->sae_pwe_relay_ap);

			ATCMD_MSG_INFO("WSAEPWE", "%d,%d,%d,%d",
					g_atcmd_wifi_info->sae_pwe_sta,
					g_atcmd_wifi_info->sae_pwe_ap,
					g_atcmd_wifi_info->sae_pwe_relay_sta,
					g_atcmd_wifi_info->sae_pwe_relay_ap);

			break;

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_sae_pwe_set (int argc, char *argv[])
{
	int mode;
	int sae_pwe[2] = { -1, -1 };

	if (!_atcmd_wifi_disconnected())
		return ATCMD_ERROR_NOTSUPP; 

	switch (argc)
	{
		case 0:
			/*
			 * <mode>
			 *    - 0 : STA
			 *    - 1 : AP
			 *    - 2 : RELAY
			 *
			 * <sae_pwe>
			 *   SAE mechanism for PWE derivation
			 *    - 0 = hunting-and-pecking loop only (default without password identifier)
			 *    - 1 = hash-to-element only (default with password identifier)
			 *    - 2 = both hunting-and-pecking loop and hash-to-element enabled
			 *    - Note: The default value is likely to change from 0 to 2 once the new
			 *            hash-to-element mechanism has received more interoperability testing.
			 *            When using SAE password identifier, the hash-to-element mechanism is used
			 *            regardless of the sae_pwe parameter value.
			 */
			ATCMD_MSG_HELP("AT+WSAEPWE=0,<sae_pwe_sta>");
#if defined(CONFIG_ATCMD_SOFTAP_WPA3)
			ATCMD_MSG_HELP("AT+WSAEPWE=1,<sae_pwe_ap>");
			ATCMD_MSG_HELP("AT+WSAEPWE=2,<sae_pwe_sta>[,<sae_pwe_ap>]");
#else
			ATCMD_MSG_HELP("AT+WSAEPWE=2,<sae_pwe_sta>");
#endif			
			break;

#if defined(CONFIG_ATCMD_SOFTAP_WPA3)
		case 3:
			sae_pwe[1] = atoi(argv[2]);
#endif			
		case 2:
			mode = atoi(argv[0]);
			sae_pwe[0] = atoi(argv[1]);

			if (sae_pwe[0] >= 0 && sae_pwe[0] <= 2)
			{
#if !defined(CONFIG_ATCMD_SOFTAP_WPA3)
				if (mode == 0)
#else					
				if (mode == 0 || mode == 1)
#endif
				{
					_atcmd_info("wifi_sae_pwe_set: %s=%d", (mode == 0) ? "sta" : "ap", sae_pwe[0]);
					g_atcmd_wifi_info->sae_pwe[mode] = sae_pwe[0];
					break;
				}
				else if (mode == 2)
				{
					if (argc == 2 || (sae_pwe[1] >= 0 && sae_pwe[1] <= 2))
					{
						_atcmd_info("wifi_sae_pwe_set: relay=%d,%d", sae_pwe[0], sae_pwe[1]);
						g_atcmd_wifi_info->sae_pwe[mode] = sae_pwe[0];
#if defined(CONFIG_ATCMD_SOFTAP_WPA3)
						if (argc == 3)
							g_atcmd_wifi_info->sae_pwe[mode + 1] = sae_pwe[1];
#endif						
						break;
					}
				}
			}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}
#endif

static atcmd_info_t g_atcmd_wifi_sae_pwe =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "SAEPWE",
	.id = ATCMD_WIFI_SAE_PWE,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_sae_pwe_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_sae_pwe_set,
};

#endif /* #if defined(CONFIG_ATCMD_SAEPWE) && defined(CONFIG_ATCMD_WPA3) */

/**********************************************************************************************/

static bool _atcmd_wifi_connect_valid_ap (char *ssid, char *bssid, char *security)
{
	atcmd_wifi_scan_t *scan = &g_atcmd_wifi_info->scan;
	bool scanned_ap = true;
	int ssid_len, bssid_len;
	int sec_mode;
	int i;

	if (!ssid || !bssid || !security)
		return false;

	ssid_len = strlen(ssid);
	bssid_len = strlen(bssid);

	if (ssid_len == 0 && bssid_len == 0)
		return false;

	if (ssid_len > ATCMD_WIFI_SSID_LEN_MAX || (bssid_len > 0 && bssid_len != ATCMD_WIFI_BSSID_LEN))
		return false;

	if (strcmp(security, "open") == 0)
		sec_mode = ATCMD_WIFI_SEC_OPEN;
 	else if (strcmp(security, "wpa2-psk") == 0)
		sec_mode = ATCMD_WIFI_SEC_PSK;
#if defined(CONFIG_ATCMD_WPA3)
	else if (strcmp(security, "wpa3-owe") == 0)
		sec_mode = ATCMD_WIFI_SEC_OWE;
	else if (strcmp(security, "wpa3-sae") == 0)
		sec_mode = ATCMD_WIFI_SEC_SAE;
#endif
	else
		return false;

	if (!scanned_ap)
	{
		for (i = 0 ; i < scan->results.n_result ; i++)
		{
			if (ssid_len > 0 && strcmp(ssid, scan->results.result[i].ssid) != 0)
				continue;

			if (bssid_len > 0 && strcmp(bssid, scan->results.result[i].bssid) != 0)
				continue;

			if (strstr(scan->results.result[i].flags, "WPA2-PSK"))
				scanned_ap = (sec_mode == ATCMD_WIFI_SEC_PSK) ? true : false;
#if defined(CONFIG_ATCMD_WPA3)
			else if (strstr(scan->results.result[i].flags, "WPA3-OWE"))
				scanned_ap = (sec_mode == ATCMD_WIFI_SEC_OWE) ? true : false;
			else if (strstr(scan->results.result[i].flags, "WPA3-SAE"))
				scanned_ap = (sec_mode == ATCMD_WIFI_SEC_SAE) ? true : false;
#endif
			else
				scanned_ap = (sec_mode == ATCMD_WIFI_SEC_OPEN) ? true : false;

			if (scanned_ap)
			{
				if (ssid_len == 0)
					strcpy(ssid, scan->results.result[i].ssid);

				if (bssid_len == 0)
					strcpy(bssid, scan->results.result[i].bssid);

				break;
			}
		}

		if (!scanned_ap)
			return false;
	}

	return true;
}

static void _atcmd_wifi_connect_update_ap_info (void)
{
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
	wifi_ap_info_t ap_info;

	if (wifi_api_get_ap_info(&ap_info) == 0)
	{
		_atcmd_info("wifi_connect: %.1f@%u %s %s %s", 
					ap_info.channel.freq/10., ap_info.channel.bw,
					ap_info.bssid, ap_info.ssid, ap_info.security);

		if (strcmp(connect->bssid, ap_info.bssid) != 0)
		{
			_atcmd_info(" - bssid: %s -> %s", connect->bssid, ap_info.bssid);
			strcpy(connect->bssid, ap_info.bssid);
		}

		if (strcmp(connect->ssid, ap_info.ssid) != 0)
		{
			_atcmd_info(" - ssid: %s -> %s", connect->ssid, ap_info.ssid);
			strcpy(connect->ssid, ap_info.ssid);
		}

		if (strcmp(connect->security, ap_info.security) != 0)
		{
			_atcmd_info(" - security: %s -> %s", connect->security, ap_info.security);
			strcpy(connect->security, ap_info.security);
		}
	}
}

static int _atcmd_wifi_connect (atcmd_wifi_bssid_t bssid, atcmd_wifi_ssid_t ssid, 
								atcmd_wifi_security_t security, atcmd_wifi_password_t password, 
								bool event_poll)
{
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
	atcmd_wifi_bgscan_t *bgscan = &g_atcmd_wifi_info->bgscan;
	uint32_t timeout_msec = _atcmd_timeout_value("WCONN");
	bool connection_timeout = false;
	int sae_pwe;

/*	_atcmd_debug("wifi_connect: event_poll=%d net_id=%d", event_poll, net_id); */

	if (connect->connecting || connect->connected)
	{
		_atcmd_info("wifi_connect: %s", connect->connected ? "connected" : "connecting");

		return ATCMD_ERROR_BUSY;
	}

	ATCMD_WIFI_LOCK();

	if (wifi_api_add_network(false) != 0)
		goto wifi_connect_fail;

	if (bgscan->scanning) {
		_atcmd_info("Start bgscan simple:%u:%d:%u\n",
					bgscan->short_interval,
					bgscan->signal_threshold,
					bgscan->long_interval);

		wifi_api_set_scan_background(bgscan->short_interval, bgscan->signal_threshold, bgscan->long_interval);
	}

	if (wifi_api_is_relay_mode())
		sae_pwe = g_atcmd_wifi_info->sae_pwe_relay_sta;
   	else
		sae_pwe = g_atcmd_wifi_info->sae_pwe_sta;

	_atcmd_info("wifi_connect: ssid=%s bssid=%s security=%s sae_pwe=%d timeout=%u", 
					ssid, bssid, security, sae_pwe, timeout_msec);

	if (strlen(ssid) > 0 && wifi_api_set_ssid(ssid) != 0)
		goto wifi_connect_fail;

	if (strlen(bssid) > 0 && wifi_api_set_bssid(bssid) != 0)
		goto wifi_connect_fail;

	if (wifi_api_set_security(security, password, sae_pwe) != 0)
		goto wifi_connect_fail;

	connect->connected = false;
	connect->connecting = true;
	connect->disconnecting = false;

	if (event_poll)
	{
		_atcmd_info("wifi_connect: event poll");

		_atcmd_wifi_event_poll(ATCMD_WIFI_EVT_CONNECT_SUCCESS);
	}
	else
	{
		_atcmd_wifi_event_polled(ATCMD_WIFI_EVT_CONNECT_SUCCESS);
	}

	switch (wifi_api_connect(timeout_msec))
	{
		case 0:
			break;

		case 1:
			connection_timeout = true;

		default:
			goto wifi_connect_fail;
	}

	if (connect->connecting)
	{
		uint32_t time;

		_atcmd_info("wifi_connect: connecting");

		for (time = 0 ; connect->connecting ; time += 100)
		{
			if (time > 0 && (time % 1000) == 0)
				_atcmd_info("wifi_connect: %u sec", time / 1000);

			if (timeout_msec > 0 && time >= timeout_msec)
			{
				if (!event_poll)
				{
					_atcmd_wifi_event_poll(ATCMD_WIFI_EVT_CONNECT_SUCCESS);
				}

				connection_timeout = true;

				goto wifi_connect_fail;
			}

			_delay_ms(100);
		}
	}

	if (!connect->connected)
	{
		_atcmd_info("wifi_connect: not connected");
		goto wifi_connect_fail;
	}

/*	_atcmd_wifi_connect_update_ap_info(); */

	strcpy(connect->password, password);

	_atcmd_info("wifi_connect: done");

	ATCMD_WIFI_UNLOCK();

	return ATCMD_SUCCESS;

wifi_connect_fail:

	connect->connected = false;
	connect->connecting = false;
	connect->disconnecting = false;

	_atcmd_info("wifi_connect: %s", connection_timeout ? "timeout" : "fail");

	if (wifi_api_remove_network(false) != 0)
		_atcmd_error("api_remove_network()");

	ATCMD_WIFI_UNLOCK();

	return connection_timeout ? ATCMD_ERROR_TIMEOUT : ATCMD_ERROR_FAIL;
}

static int _atcmd_wifi_connect_run (int argc, char *argv[])
{
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
	bool event_poll = false;

	switch (argc)
	{
		case 1:
			event_poll = true;

		case 0:
			break;

		default:
			_atcmd_info("wifi_connect: invalid argc");
			return ATCMD_ERROR_INVAL;
	}

	return _atcmd_wifi_connect(connect->bssid, connect->ssid, 
							connect->security, connect->password, 
							event_poll);
}

static int _atcmd_wifi_connect_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
			char *status = NULL;

			if (connect->disconnecting)
				status = "disconnecting";
			else if (connect->connecting)
				status = "connecting";
			else if (connect->connected)
				status = "connected";
			else
				status = "disconnected";

#if defined(CONFIG_SHOW_WIFI_PASSWORD)
			if (strlen(connect->password) <= 16)
			{
				ATCMD_MSG_INFO("WCONN", "\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"",
								connect->ssid, connect->bssid, 
								connect->security, connect->password, 
								status);
			}
			else
			{
				char password[16 + 1];

				memcpy(password, connect->password, 16);
				password[16] = '\0';

				ATCMD_MSG_INFO("WCONN", "\"%s\",\"%s\",\"%s\",\"%s...%d\",\"%s\"",
								connect->ssid, connect->bssid, 
								connect->security, password, strlen(connect->password), 
								status);
			}
#else		
			ATCMD_MSG_INFO("WCONN", "\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"",
							connect->ssid, connect->bssid, connect->security, 
							connect->recovery ? "*" : "", status);
#endif
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_connect_set (int argc, char *argv[])
{
	char *param_ssid = NULL;
	char *param_bssid = NULL;
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
			atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
			atcmd_wifi_ssid_t str_ssid;
			atcmd_wifi_bssid_t str_bssid;
			atcmd_wifi_security_t  str_security;
			atcmd_wifi_password_t str_password;

			strcpy(str_ssid, "");
			strcpy(str_bssid, "");
			strcpy(str_security, "open");
			strcpy(str_password, "");

			param_ssid = argv[0];

			if (!atcmd_param_to_str(param_ssid, str_ssid, sizeof(str_ssid)))
				return ATCMD_ERROR_INVAL;

			if (_atcmd_wifi_bssid_valid(str_ssid))
			{
				strcpy(str_bssid, str_ssid);
				strcpy(str_ssid, "");
			}

			if (param_security)
			{
				if (!atcmd_param_to_str(param_security, str_security, sizeof(str_security)))
					return ATCMD_ERROR_INVAL;

				if (!_atcmd_wifi_security_valid(str_security, true))
					return ATCMD_ERROR_INVAL;
			}

			if (param_password)
			{
				if (!atcmd_param_to_str(param_password, str_password, sizeof(str_password)))
					return ATCMD_ERROR_INVAL;

				if (!_atcmd_wifi_password_valid(str_password))
					return ATCMD_ERROR_INVAL;
			}

			if (_atcmd_wifi_connect_valid_ap(str_ssid, str_bssid, str_security))
				return _atcmd_wifi_connect(str_bssid, str_ssid, str_security, str_password, false);

			_atcmd_info("wifi_connect: invalid AP, ssid=%s bssid=%s security=%s",
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
	uint32_t timeout_msec = _atcmd_timeout_value("WDISCONN");

	switch (argc)
	{
		case 1:
			event_poll = true;

		case 0:
			break;

		default:
			_atcmd_info("wifi_disconnect: invalid argc");
			return ATCMD_ERROR_INVAL;
	}

/*	_atcmd_debug("wifi_disconnect: event_poll=%d", event_poll); */

	ATCMD_WIFI_LOCK();

	if (connect->disconnecting)
	{
		_atcmd_info("wifi_disconnect: busy");

		ATCMD_WIFI_UNLOCK();

		return ATCMD_ERROR_BUSY;
	}
	else if (!connect->connecting && !connect->connected)
	{
		_atcmd_info("wifi_disconnect: disconnected");

		connect->connected = false;
		connect->connecting = false;
		connect->disconnecting = false;

		ATCMD_WIFI_UNLOCK();

		return ATCMD_SUCCESS;
	}

	_atcmd_info("wifi_disconnect: run");

	connect->disconnecting = true;

	if (connect->connecting)
	{
		connect->connecting = false;

		_atcmd_wifi_event_polled(ATCMD_WIFI_EVT_CONNECT_SUCCESS);
	}

	if (event_poll)
	{
		_atcmd_info("wifi_disconnect: event poll");

		_atcmd_wifi_event_poll(ATCMD_WIFI_EVT_DISCONNECT);
	}
	else
	{
		_atcmd_wifi_event_polled(ATCMD_WIFI_EVT_DISCONNECT);
	}

	if (wifi_api_disconnect(timeout_msec) != 0)
	{
		connect->disconnecting = false;

		_atcmd_info("wifi_disconnect: fail");

		ATCMD_WIFI_UNLOCK();

		return ATCMD_ERROR_FAIL;
	}

	if (connect->connecting || connect->connected)
	{
		uint32_t time;

		_atcmd_info("wifi_disconnect: disconnecting");

		for (time = 0 ; connect->disconnecting ; time += 100)
		{
			if (time > 0 && (time % 1000) == 0)
				_atcmd_info("wifi_disconnect: %u sec", time / 1000);

			if (timeout_msec > 0 && time >= timeout_msec)
			{
				if (!event_poll)
					_atcmd_wifi_event_poll(ATCMD_WIFI_EVT_DISCONNECT);

				_atcmd_info("wifi_disconnect: timeout");

				ATCMD_WIFI_UNLOCK();

				return ATCMD_ERROR_TIMEOUT;
			}

			_delay_ms(100);
		}
	}

	if (connect->disconnecting)
	{
		connect->disconnecting = false;

		if (_atcmd_wifi_event_polling(ATCMD_WIFI_EVT_DISCONNECT))
			_atcmd_wifi_event_polled(ATCMD_WIFI_EVT_DISCONNECT);
	}

	wifi_api_stop_dhcp_client();

	_atcmd_info("wifi_disconnect: done");

	ATCMD_WIFI_UNLOCK();

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

static int _atcmd_wifi_ipaddr_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 4: /* DHCP client */
		case 3: /* DHCP server */
		case 0:
		{
			atcmd_wifi_ipaddr_t str_ip4addr[3];

			if (wifi_api_get_ip4_address(str_ip4addr[0], str_ip4addr[1], str_ip4addr[2]) != 0)
				return ATCMD_ERROR_FAIL;

			if (argc == 0)
			{
				ATCMD_MSG_INFO("WIPADDR", "\"%s\",\"%s\",\"%s\"",
								str_ip4addr[0], str_ip4addr[1], str_ip4addr[2]);
			}
			else /* DHCP */
			{
				strcpy(argv[0], str_ip4addr[0]);
				strcpy(argv[1], str_ip4addr[1]);
				strcpy(argv[2], str_ip4addr[2]);

				if (argc == 4)
					*(int *)argv[3] = wifi_api_get_dhcp_lease_time();
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
	char *param_ip4addr[3] = { NULL, NULL, NULL };

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WIPADDR=\"<address>\",\"<netmask>\",\"<gateway>\"");
			break;

		case 3:
		{
			atcmd_wifi_ipaddr_t str_ip4addr[3];
			ip_addr_t ip4addr[3];
			int i;

			for (i = 0 ; i < 3 ; i++)
			{
				param_ip4addr[i] = argv[i];

				if (!atcmd_param_to_str(param_ip4addr[i], str_ip4addr[i], sizeof(atcmd_wifi_ipaddr_t)))
					return ATCMD_ERROR_FAIL;
			}

			if (wifi_api_set_ip4_address(str_ip4addr[0], str_ip4addr[1], str_ip4addr[2]) != 0)
				return ATCMD_ERROR_FAIL;

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

#ifdef CONFIG_ATCMD_IPV6

static int _atcmd_wifi_ipaddr6_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0: /* AT+WIPADDR6? */
		{
			struct netif *netif = nrc_netif[0];
			atcmd_wifi_ipaddr_t str_ip6addr;
			char msg_info[LWIP_IPV6_NUM_ADDRESSES][ATCMD_STR_PARAM_SIZE(sizeof(atcmd_wifi_ipaddr_t)) + 5];
			int len;
			int i;

			for (len = i = 0 ; i < LWIP_IPV6_NUM_ADDRESSES ; i++)
			{
				strcpy(str_ip6addr, ipaddr_ntoa(netif_ip_addr6(netif, i)));

				len += snprintf((char *)msg_info + len, sizeof(msg_info) - len, "\"%s/0x%02X\",",
									str_ip6addr, netif_ip6_addr_state(netif, i));
			}

			*((char *)msg_info + len - 1) = '\0';

			ATCMD_MSG_INFO("WIPADDR6", "%s", msg_info);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_ipaddr6_set (int argc, char *argv[])
{
	char *param_index = NULL;
	char *param_state = NULL;
	char *param_ip6addr = NULL;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WIPADDR6=<index>,<state>[,\"<address>\"]");
			break;

		case 3:
			param_ip6addr = argv[2];

		case 2:
		{
			struct netif *netif = nrc_netif[0];
			struct
			{
				uint8_t val;
				const char *str;
			} ip6addr_state[3] =
			{
				[0] = { IP6_ADDR_INVALID, "invalid" },
				[1] = { IP6_ADDR_VALID, "valid" },
				[2] = { IP6_ADDR_PREFERRED, "preferred" }
			};
			int index;
			int state;

			param_index = argv[0];
			param_state = argv[1];

			index = atoi(param_index);
			if (index < 0 || index >= LWIP_IPV6_NUM_ADDRESSES)
				return ATCMD_ERROR_INVAL;

			state = atoi(param_state);
			if (index < 0 || index > 2)
				return ATCMD_ERROR_INVAL;

			if (param_ip6addr)
			{
				atcmd_wifi_ipaddr_t str_ip6addr;
				ip_addr_t ipaddr;
				int ret;

				if (!atcmd_param_to_str(param_ip6addr, str_ip6addr, sizeof(atcmd_wifi_ipaddr_t)))
					return ATCMD_ERROR_FAIL;

				if (!ipaddr_aton(str_ip6addr, &ipaddr))
				{
					_atcmd_info("wifi_ip6addr_set: invalid address");
					return ATCMD_ERROR_INVAL;
				}

				ret = netif_get_ip6_addr_match(netif, ip_2_ip6(&ipaddr));
			   	if (ret	>= 0)
				{
					_atcmd_info("wifi_ip6addr_set: existing address, index=%d", ret);
					return ATCMD_ERROR_BUSY;
				}

				_atcmd_info("wifi_ip6addr_set: index=%d state=%s ipaddr=%s",
							index, ip6addr_state[state].str, ipaddr_ntoa(&ipaddr));

				netif_ip6_addr_set(netif, index, ip_2_ip6(&ipaddr));
			}
			else
			{
				_atcmd_info("wifi_ip6addr_set: index=%d state=%s",
							index, ip6addr_state[state].str);
			}

			netif_ip6_addr_set_state(netif, index, ip6addr_state[state].val);

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_ipaddr6 =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "IPADDR6",
	.id = ATCMD_WIFI_IPADDR6,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_ipaddr6_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_ipaddr6_set,
};

#endif /* #ifdef CONFIG_ATCMD_IPV6 */

/**********************************************************************************************/

static TaskHandle_t g_atcmd_wifi_dhcp_task_handle = NULL;

static void _atcmd_wifi_dhcp_task_suspend (void)
{
	_atcmd_info("wifi_dhcp: task_suspend");

	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
}

static void _atcmd_wifi_dhcp_task_resume (void)
{
	if (g_atcmd_wifi_dhcp_task_handle)
	{
		_atcmd_info("wifi_dhcp: task_resume");

		xTaskNotifyGive(g_atcmd_wifi_dhcp_task_handle);
	}
}

static void _atcmd_wifi_dhcp_task (void *pvParameters)
{
	_atcmd_info("wifi_dhcp: task_run");

	while (1)
	{
		_atcmd_wifi_dhcp_task_suspend();
		_delay_ms(1000);
		_atcmd_wifi_dhcp_run(1, NULL);
	}
}

static int _atcmd_wifi_dhcp_task_start (void)
{
	if (!g_atcmd_wifi_dhcp_task_handle)
	{
		if (xTaskCreate(_atcmd_wifi_dhcp_task, "atcmd_wifi_dhcp",
						ATCMD_WIFI_DHCP_TASK_STACK_SIZE, NULL,
						ATCMD_WIFI_DHCP_TASK_PRIORITY,
						&g_atcmd_wifi_dhcp_task_handle) != pdPASS)
			return -1;
	}

	return 0;
}

static void _atcmd_wifi_dhcp_task_stop (void)
{
	if (g_atcmd_wifi_dhcp_task_handle)
	{
		vTaskDelete(g_atcmd_wifi_dhcp_task_handle);
		g_atcmd_wifi_dhcp_task_handle = NULL;
	}
}

static void _atcmd_wifi_dhcp_release_handler (void)
{
	_atcmd_info("wifi_dhcp: release");

	ATCMD_MSG_WEVENT("\"DHCP_RELEASE\"");
}

static void _atcmd_wifi_dhcp_renew_handler (void)
{
	atcmd_wifi_ipaddr_t ip4addr[3];

	if (wifi_api_get_ip4_address(ip4addr[0], ip4addr[1], ip4addr[2]) == 0)
	{
		int lease_time = wifi_api_get_dhcp_lease_time();

		_atcmd_info("wifi_dhcp: renew, %s %s %s %d",
					ip4addr[0], ip4addr[1], ip4addr[2], lease_time);

		ATCMD_MSG_WEVENT("\"DHCP_RENEW\",\"%s\",\"%s\",\"%s\",%d",
					ip4addr[0], ip4addr[1], ip4addr[2], lease_time);
	}
}

static int _atcmd_wifi_dhcp_run (int argc, char *argv[])
{
	dhcpc_event_cb_t event_cb[ATCMD_DHCPC_EVT_MAX] =
	{
		[ATCMD_DHCPC_EVT_RELEASE] = _atcmd_wifi_dhcp_release_handler,
		[ATCMD_DHCPC_EVT_RENEW] = _atcmd_wifi_dhcp_renew_handler
	};
	bool task_run = (argc == 0) ? false : true;
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
	uint32_t timeout_msec = _atcmd_timeout_value("WDHCP");
	int ret;

	if (!connect->connected)
	{
		_atcmd_info("wifi_dhcp: not connected");
		return ATCMD_ERROR_INVAL;
	}

	ATCMD_WIFI_LOCK();

	_atcmd_info("wifi_dhcp: run");

	if (task_run)
		ATCMD_MSG_WEVENT("\"DHCP_START\"");

	switch (wifi_api_start_dhcp_client(timeout_msec, event_cb))
	{
		case DHCP_RECOVERY:
			_atcmd_info("wifi_dhcp: recovery");
			ret = ATCMD_SUCCESS;
			break;

		case DHCP_SUCCESS:
			_atcmd_info("wifi_dhcp: success");
			ret = ATCMD_SUCCESS;
			break;

		case DHCP_BUSY:
			_atcmd_info("wifi_dhcp: busy");
			if (task_run)
				ATCMD_MSG_WEVENT("\"DHCP_BUSY\"");
			ret = ATCMD_ERROR_BUSY;
			break;

		case DHCP_TIMEOUT:
			_atcmd_info("wifi_dhcp: timeout, %umsec", timeout_msec);
			if (task_run)
				ATCMD_MSG_WEVENT("\"DHCP_TIMEOUT\",%u", timeout_msec / 1000);
			ret = ATCMD_ERROR_TIMEOUT;
			break;

		case DHCP_STOP:
			_atcmd_info("wifi_dhcp: stop");
			if (task_run)
				ATCMD_MSG_WEVENT("\"DHCP_STOP\"");
			ret = ATCMD_ERROR_FAIL;
			break;

		default:
			_atcmd_info("wifi_dhcp: fail");
			if (task_run)
				ATCMD_MSG_WEVENT("\"DHCP_FAIL\"");
			ret = ATCMD_ERROR_FAIL;
	}

	if (ret == ATCMD_SUCCESS)
	{
		atcmd_wifi_ipaddr_t ip4addr[3];

		if (wifi_api_get_ip4_address(ip4addr[0], ip4addr[1], ip4addr[2]) != 0)
			_atcmd_info("wifi_dhcp: failed to get ip address");
		else
		{
			int lease_time = wifi_api_get_dhcp_lease_time();

			if (task_run)
			{
				ATCMD_MSG_WEVENT("\"DHCP_SUCCESS\",\"%s\",\"%s\",\"%s\",%d",
						ip4addr[0], ip4addr[1], ip4addr[2], lease_time);
			}
			else
			{
				ATCMD_MSG_INFO("WDHCP", "\"%s\",\"%s\",\"%s\",%d",
						ip4addr[0], ip4addr[1], ip4addr[2], lease_time);
			}
		}
	}

	ATCMD_WIFI_UNLOCK();

	return ret;
}

static int _atcmd_wifi_dhcp_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_INFO("WDHCP", "%u", g_atcmd_wifi_dhcp_task_handle ? 1 : 0);
			break;

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_dhcp_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WDHCP={0|1}");
			break;

		case 1:
		{
			int param = atoi(argv[0]);

			if (param == 0)
			{
				_atcmd_wifi_dhcp_task_stop();
				break;
			}
			else if (param == 1)
			{
				if (_atcmd_wifi_dhcp_task_start() != 0)
					return ATCMD_ERROR_FAIL;
				break;
			}
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_dhcp =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "DHCP",
	.id = ATCMD_WIFI_DHCP,

	.handler[ATCMD_HANDLER_RUN] = _atcmd_wifi_dhcp_run,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_dhcp_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_dhcp_set,
};

/**********************************************************************************************/

#ifdef CONFIG_ATCMD_SOFTAP
static int _atcmd_wifi_dhcps_run (int argc, char *argv[])
{
	atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;

	if (wifi_api_is_relay_mode())
	{
		_atcmd_info("wifi_dhcps: relay mode");
		return ATCMD_ERROR_NOTSUPP;
	}

	if (!softap->active)
	{
		_atcmd_info("wifi_dhcps: sta mode");
		return ATCMD_ERROR_NOTSUPP;
	}

	if (softap->dhcp_server)
	{
		_atcmd_info("wifi_dhcps: busy");
		return ATCMD_ERROR_BUSY;
	}

	ATCMD_WIFI_LOCK();

	_atcmd_info("wifi_dhcps: run");

	if (wifi_api_start_dhcp_server() == 0)
	{
		char param_ip4addr[3][ATCMD_STR_PARAM_SIZE(sizeof(atcmd_wifi_ipaddr_t))];
		char *argv[3] = { param_ip4addr[0], param_ip4addr[1], param_ip4addr[2] };

		_atcmd_info("wifi_dhcps: done");

		softap->dhcp_server = true;

		if (_atcmd_wifi_ipaddr_get(3, argv) != ATCMD_SUCCESS)
			_atcmd_info("wifi_dhcps: failed to get ip address");

		ATCMD_MSG_INFO("WDHCPS", "%s,%s,%s",
						param_ip4addr[0], param_ip4addr[1], param_ip4addr[2]);

		ATCMD_WIFI_UNLOCK();

		return ATCMD_SUCCESS;
	}
	else
	{
		_atcmd_info("wifi_dhcps: fail");

		ATCMD_WIFI_UNLOCK();

		return ATCMD_ERROR_FAIL;
	}
}

static int _atcmd_wifi_dhcps_get (int argc, char *argv[])
{
	atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_INFO("WDHCPS", "%u", softap->dhcp_server ? 1 : 0);
			break;

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_dhcps_set (int argc, char *argv[])
{
	atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WDHCPS={0|1}");
			break;

		case 1:
		{
			int param = atoi(argv[0]);

			if (param == 0 || param == 1)
			{
				bool enable = !!param;

				if (enable)
					return _atcmd_wifi_dhcps_run(0, NULL);

				if (softap->dhcp_server)
				{
					if (wifi_api_stop_dhcp_server() != 0)
						return ATCMD_ERROR_FAIL;

					softap->dhcp_server = false;
				}

				break;
			}
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

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
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_dhcps_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_dhcps_set,
};
#endif

/**********************************************************************************************/

static void _atcmd_wifi_ping_report_callback (_lwip_ping_report_t *report)
{
	atcmd_wifi_ipaddr_t str_remote_ip;

	strcpy(str_remote_ip, ipaddr_ntoa(&report->remote_ip));

	switch (report->status)
	{
		case _LWIP_PING_SUCCESS:
			if (IP_IS_V6(&report->remote_ip))
			{
				ATCMD_MSG_INFO("WPING6", "%u,\"%s\",%u,%u,%u",
					report->data_size, str_remote_ip,
					report->icmp_seq, report->ttl, report->resp_time);
			}
			else
			{
				ATCMD_MSG_INFO("WPING", "%u,\"%s\",%u,%u,%u",
					report->data_size, str_remote_ip,
					report->icmp_seq, report->ttl, report->resp_time);
			}
			break;

		case _LWIP_PING_SEND_FAIL:
		case _LWIP_PING_RECV_TIMEOUT:
			if (IP_IS_V6(&report->remote_ip))
			{
				ATCMD_MSG_INFO("WPING6", "%u,\"%s\",%u,\"%s\"",
					report->data_size, str_remote_ip, report->icmp_seq,
					report->status == _LWIP_PING_SEND_FAIL ? "send_fail" : "recv_timeout");
			}
			else
			{
				ATCMD_MSG_INFO("WPING", "%u,\"%s\",%u,\"%s\"",
					report->data_size, str_remote_ip, report->icmp_seq,
					report->status == _LWIP_PING_SEND_FAIL ? "send_fail" : "recv_timeout");
			}
	}
}

static int _atcmd_wifi_ping_run (int argc, char *argv[])
{
#ifdef CONFIG_ATCMD_IPV6
	bool ipv6 = !!argc;
#else
	bool ipv6 = false;
#endif

	if (g_atcmd_wifi_info->connect.connected
#ifdef CONFIG_ATCMD_SOFTAP
	|| g_atcmd_wifi_info->softap.active
#endif
	)
	{
		_lwip_ping_params_t *params = g_atcmd_wifi_info->ping.params + (ipv6 ? 1 : 0);

		if (ip_addr_isany(&params->remote_ip))
		{
			_atcmd_info("wifi_ping_run: ip is any");
			return ATCMD_ERROR_INVAL;
		}

		_atcmd_info("wifi_ping_run: ip=%s interval=%u count=%u size=%u",
						ipaddr_ntoa(&params->remote_ip), params->interval,
						params->count, params->data_size);

		if (!params->report_cb)
			params->report_cb = _atcmd_wifi_ping_report_callback;

		if (_lwip_ping_start(params) == 0)
			return ATCMD_SUCCESS;
	}

	return ATCMD_ERROR_FAIL;
}

static int _atcmd_wifi_ping_get (int argc, char *argv[])
{
	bool ipv6 = false;

	switch (argc)
	{
#ifdef CONFIG_ATCMD_IPV6
		case 1:
			ipv6 = true;
#endif

		case 0:
		{
			_lwip_ping_params_t *params = g_atcmd_wifi_info->ping.params + (ipv6 ? 1 : 0);

			if (ipv6)
			{
				ATCMD_MSG_INFO("WPING6", "\"%s\",%u,%u,%u",
							ipaddr_ntoa(&params->remote_ip), params->interval,
							params->count, params->data_size);
			}
			else
			{
				ATCMD_MSG_INFO("WPING", "\"%s\",%u,%u,%u",
							ipaddr_ntoa(&params->remote_ip), params->interval,
							params->count, params->data_size);
			}
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_ping_set (int argc, char *argv[])
{
	bool ipv6 = false;
	char *param_remote_ip = NULL;
	char *param_count = NULL;
	char *param_interval = NULL;
	char *param_size = NULL;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WPING=\"<remote_IP>\"[,<count>[,<interval>[,<size>]]]");
			break;

#ifdef CONFIG_ATCMD_IPV6
		case 8:
		case 7:
		case 6:
		case 5:
			argc -= 4;
			ipv6 = true;
#endif

		case 4:
			if (argc == 4)
				param_size = argv[3];

		case 3:
			if (argc >= 3)
				param_interval = argv[2];

		case 2:
			if (argc >= 2)
				param_count = argv[1];

		case 1:
		{
			atcmd_wifi_ping_t *ping = &g_atcmd_wifi_info->ping;
			_lwip_ping_params_t params;
			char str_remote_ip[ATCMD_STR_SIZE(ATCMD_WIFI_IPADDR_LEN_MAX)];

			_lwip_ping_params_init(&params, ipv6);

			param_remote_ip = argv[0];

			if (!atcmd_param_to_str(param_remote_ip, str_remote_ip, sizeof(str_remote_ip)))
				return ATCMD_ERROR_FAIL;

			if (!ipaddr_aton(str_remote_ip, &params.remote_ip))
			{
				_atcmd_info("wifi_ping_set: invalid ip address");
				return ATCMD_ERROR_INVAL;
			}

			switch (IP_GET_TYPE(&params.remote_ip))
			{
				case IPADDR_TYPE_V4:
					if (ipv6)
					{
						_atcmd_info("wifi_ping_set: no ipv4 address");
						return ATCMD_ERROR_INVAL;
					}
					break;

				case IPADDR_TYPE_V6:
					if (!ipv6)
					{
						_atcmd_info("wifi_ping_set: no ipv6 address");
						return ATCMD_ERROR_INVAL;
					}
					break;

				default:
					_atcmd_info("wifi_ping_set: invalid protocol");
					return ATCMD_ERROR_INVAL;
			}

			if (param_count && atcmd_param_to_uint32(param_count, &params.count) != 0)
			{
				_atcmd_info("wifi_ping_set: invalid count");
				return ATCMD_ERROR_INVAL;
			}

			if (param_interval && atcmd_param_to_uint16(param_interval, &params.interval) != 0)
			{
				_atcmd_info("wifi_ping_set: invalid interval");
				return ATCMD_ERROR_INVAL;
			}

			if (param_size && atcmd_param_to_uint16(param_size, &params.data_size) != 0)
			{
				_atcmd_info("wifi_ping_set: invalid size");
				return ATCMD_ERROR_INVAL;
			}

			_atcmd_info("wifi_ping_set: ip=%s interval=%u count=%u size=%u",
						ipaddr_ntoa(&params.remote_ip), params.interval,
						params.count, params.data_size);

			memcpy(ping->params + (ipv6 ? 1 : 0), &params, sizeof(_lwip_ping_params_t));

			return _atcmd_wifi_ping_run(ipv6, NULL);
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

#ifdef CONFIG_ATCMD_IPV6

static int _atcmd_wifi_ping6_run (int argc, char *argv[])
{
	return _atcmd_wifi_ping_run(1, NULL);
}

static int _atcmd_wifi_ping6_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			return _atcmd_wifi_ping_get(1, NULL);

		default:
			return ATCMD_ERROR_INVAL;
	}
}

static int _atcmd_wifi_ping6_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WPING6=\"<remote_IP>\"[,<count>[,<interval>[,<size>]]]");
			break;

		case 4:
		case 3:
		case 2:
		case 1:
			return _atcmd_wifi_ping_set(argc + 4, argv);

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_ping6 =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "PING6",
	.id = ATCMD_WIFI_PING6,

	.handler[ATCMD_HANDLER_RUN] = _atcmd_wifi_ping6_run,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_ping6_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_ping6_set,
};
#endif /* #ifdef CONFIG_ATCMD_IPV6 */

/**********************************************************************************************/

#include "lwip/dns.h"

static int _atcmd_wifi_dns_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			atcmd_wifi_ipaddr_t str_ipaddr[2];
			ip_addr_t *ipaddr;
			int i;

			for (i = 0 ; i < 2 ; i++)
				strcpy(str_ipaddr[i], ipaddr_ntoa(dns_getserver(i)));

			ATCMD_MSG_INFO("WDNS", "\"%s\",\"%s\"", str_ipaddr[0], str_ipaddr[1]);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_dns_set (int argc, char *argv[])
{
	char *param_dns[2] = { NULL, NULL };

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WDNS=\"<dns1>\"[,\"<dns2>\"]");
			break;

		case 2:
			param_dns[1] = argv[1];

		case 1:
		{
			atcmd_wifi_ipaddr_t str_ipaddr;
			ip_addr_t ipaddr;
			int i;

			param_dns[0] = argv[0];

			for (i = 0 ; i < argc ; i++)
			{
				if (!atcmd_param_to_str(param_dns[i], str_ipaddr, sizeof(atcmd_wifi_ipaddr_t)))
					return ATCMD_ERROR_FAIL;

				if (!ipaddr_aton(str_ipaddr, &ipaddr))
					return ATCMD_ERROR_FAIL;

				dns_setserver(i, &ipaddr);
			}
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_dns =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "DNS",
	.id = ATCMD_WIFI_DNS,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_dns_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_dns_set,
};

/**********************************************************************************************/

#ifdef CONFIG_ATCMD_FOTA

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
				!atcmd_param_to_str(param_server_url, params.server_url, sizeof(params.server_url)))
				return ATCMD_ERROR_INVAL;

			if (param_bin_name &&
				!atcmd_param_to_str(param_bin_name, params.bin_name, sizeof(params.bin_name)))
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

#endif /* #ifdef CONFIG_ATCMD_FOTA */

/**********************************************************************************************/

#include "ctrl_iface_freeRTOS.h"
#include "nrc_ps_api.h"

static void _atcmd_wifi_deep_sleep_recovery (void)
{
	ctrl_iface_recovery_t rconf;

	nrc_ps_get_rconf(&rconf);

	if (0)
	{
		uint8_t *ipaddr;
		uint8_t *netmask;
		uint8_t *gateway;
		uint32_t tmp;

		ipaddr = (uint8_t *)&rconf.ip_addr;
		tmp = (rconf.net_mask << 1) | 1;
		netmask = (uint8_t *)&tmp;
		gateway = (uint8_t *)&rconf.gw_addr;;

		_atcmd_info("[ retention_info : total=%d ]", RET_TOTAL_SIZE);
		_atcmd_info(" - country: %c%c", rconf.country[0], rconf.country[1]);	
		_atcmd_info(" - channel: %u", rconf.channel);
		_atcmd_info(" - ssid: %s (%d)", rconf.ssid, strlen(rconf.ssid));
		_atcmd_info(" - bssid: %s (%d)", rconf.bssid, strlen(rconf.bssid));
		_atcmd_info(" - security: %u", rconf.security);
		_atcmd_info(" - akm: %u", rconf.akm);
		_atcmd_info(" - sae_pwe: %u", rconf.sae_pwe);
		_atcmd_info(" - password: %s", rconf.password ? rconf.password : ""); 
		_atcmd_info(" - pmk: %s", rconf.pmk ? rconf.pmk : ""); 
		_atcmd_info(" - ip_mode: %s (%d)", rconf.ip_mode ? "dhcp" : "static", rconf.ip_mode);
		_atcmd_info(" - ip_addr: %u.%u.%u.%u", ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3]);
		_atcmd_info(" - netmask: %u.%u.%u.%u", netmask[0], netmask[1], netmask[2], netmask[3]);
		_atcmd_info(" - gateway: %u.%u.%u.%u", gateway[0], gateway[1], gateway[2], gateway[3]);
	}

	if (memcmp(rconf.country, g_atcmd_wifi_info->country, 2) != 0)
	{
		int argc = 1;
		char *argv[1];
		char param_country[5];

		snprintf(param_country, 5, "\"%c%c\"", rconf.country[0], rconf.country[1]);

		if (_atcmd_wifi_country_set(argc, argv) != ATCMD_SUCCESS)
			_atcmd_info("wifi_recovery: failed to set the country %s", param_country);
	}

	if (wifi_api_connected())
	{
		atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;

		connect->recovery = true;
		connect->connected = true;
		connect->connecting = false;
		connect->disconnecting = false;

		strcpy(connect->ssid, rconf.ssid);
		strcpy(connect->bssid, rconf.bssid);

		strcpy(connect->password, "*"); 

		if (rconf.security == 4 && rconf.akm == 0)
			strcpy(connect->security, "open");
		else if (rconf.security == 2 && rconf.akm == 2)
			strcpy(connect->security, "wpa2-psk");
		else if (rconf.security == 16 && rconf.akm == 18)
			strcpy(connect->security, "wpa3-owe");
		else if (rconf.security == 8 && rconf.akm == 8)
			strcpy(connect->security, "wpa3-sae");
		else
			strcpy(connect->security, "unknown");

/*		if (rconf.password)
			strcpy(connect->password, rconf.password); */

		g_atcmd_wifi_info->sae_pwe_sta = rconf.sae_pwe;

		_atcmd_wifi_connect_update_ap_info();
	}
}

static void _atcmd_wifi_deep_sleep_send_event (void)
{
	if (wifi_api_wakeup_done())
	{
		_atcmd_info("wifi_event: deepsleep_wakeup");

		_atcmd_wifi_deep_sleep_recovery();

		ATCMD_MSG_WEVENT("\"DEEPSLEEP_WAKEUP\"");
	}
}

static int _atcmd_wifi_deep_sleep_set (int argc, char *argv[])
{
	atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;
	char *param_timeout = NULL;
	char *param_gpio = NULL;

	if (wifi_api_is_relay_mode() || softap->active)
		return ATCMD_ERROR_NOTSUPP;

	switch (argc)
	{
		case 0:
			/*
			 *  timeout (msec)
			 *    - sleep timeout for nonTIM mode
			 *    - 0 for TIM mode.
			 *
			 * 	gpio : GPIO pin number to wake up from deep sleep (8 ~ 17)
			 *
			 */
			ATCMD_MSG_HELP("AT+WDEEPSLEEP=<timeout>[,<gpio>]");
			break;

		case 2:
			param_gpio = argv[1];

		case 1:
		{
			uint32_t timeout = 0;
			uint8_t gpio = 0;
			bool tim_mode = true;
			int ret = -EINVAL;

			param_timeout = argv[0];

			if (atcmd_param_to_uint32(param_timeout, &timeout) != 0)
				return ATCMD_ERROR_INVAL;

			if (param_gpio)
			{
				if (atcmd_param_to_uint8(param_gpio, &gpio) != 0)
					return ATCMD_ERROR_INVAL;

				if (!atcmd_gpio_pin_valid(gpio))
				{
					_atcmd_info("wifi_deep_sleep: invalid_gpio=%u", gpio);
					return ATCMD_ERROR_INVAL;
				}
			}

			_atcmd_info("wifi_deep_sleep: timeout=%u gpio=%u", timeout, gpio);

			if (timeout == 0 && !_atcmd_wifi_connected())
			{
				_atcmd_info("wifi_deep_sleep: not connected");
				return ATCMD_ERROR_NOTSUPP;
			}

			ret = wifi_api_start_deep_sleep(timeout, gpio);
			if (ret == 0)
				return ATCMD_NO_RETURN;
			else if (ret != -EINVAL)
				return ATCMD_ERROR_FAIL;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_deep_sleep =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "DEEPSLEEP",
	.id = ATCMD_WIFI_DEEP_SLEEP,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = NULL,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_deep_sleep_set,
};

/**********************************************************************************************/

#ifdef CONFIG_ATCMD_SOFTAP

static void _atcmd_wifi_softap_init_info (void)
{
	atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;
	uint8_t max_num_sta;

	softap->active = false;
	softap->dhcp_server = false;

	softap->channel_bw = 0;
	softap->channel_freq = 0;

	ASSERT(wifi_api_get_max_num_sta(&max_num_sta) == 0);

	softap->max_num_sta.system = max_num_sta;
	softap->max_num_sta.current = max_num_sta;

	softap->bss_max_idle.period = 0;
	softap->bss_max_idle.retry = ATCMD_WIFI_BSS_MAX_IDLE_RETRY_MIN;

	softap->ssid_type = ATCMD_WIFI_SSID_FULL;
	strcpy(softap->ssid, ATCMD_WIFI_INIT_SSID);
	strcpy(softap->security, ATCMD_WIFI_INIT_SECURITY);
	strcpy(softap->password, ATCMD_WIFI_INIT_PASSWORD);
}

static int _atcmd_wifi_softap_run (int argc, char *argv[])
{
	atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;
	atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
	uint32_t timeout_msec = _atcmd_timeout_value("WSOFTAP");
	int sae_pwe;

	if (softap->active)
	{
		_atcmd_info("wifi_softap: busy");

		return ATCMD_ERROR_BUSY;
	}

	ATCMD_WIFI_LOCK();

	if (wifi_api_add_network(true) != 0)
		goto wifi_softap_fail;

	if (wifi_api_is_relay_mode())
		sae_pwe = g_atcmd_wifi_info->sae_pwe_relay_ap;
	else
	{
		if (connect->connecting || connect->connected)
		{
			_atcmd_info("wifi_softap: %s to AP", connect->connected ? "connected" : "connecting");

			goto wifi_softap_fail;
		}
		else if (connect->disconnecting)
		{
			_atcmd_info("wifi_softap: disconnecting from AP");

			goto wifi_softap_fail;
		}
		
		sae_pwe = g_atcmd_wifi_info->sae_pwe_ap;
	}

	_atcmd_info("wifi_softap: bandwidth=%u freq=%u ssid=%s ssid_type=%d security=%s password=%s sae_pwe=%d",
				softap->channel_bw, softap->channel_freq, 
				softap->ssid, softap->ssid_type,
				softap->security, softap->password, sae_pwe,
				timeout_msec); 

	if (wifi_api_start_softap(softap->channel_bw, softap->channel_freq, 
							softap->ssid, softap->ssid_type,
							softap->security, softap->password, sae_pwe, 
							timeout_msec) == 0)
	{
		uint16_t freq;
		uint8_t bw;

		if (wifi_api_get_channel(&freq, &bw, true) == WIFI_SUCCESS)
		{
			softap->channel_freq = freq;
			softap->channel_bw = bw;
		}

		softap->active = true;
		ATCMD_WIFI_UNLOCK();

		_atcmd_info("wifi_softap: done");

		return ATCMD_SUCCESS;
	}

wifi_softap_fail:

	_atcmd_info("wifi_softap: fail");

	if (wifi_api_remove_network(true) != 0)
		_atcmd_error("api_remove_network()");

	ATCMD_WIFI_UNLOCK();

	return ATCMD_ERROR_FAIL;
}

static int _atcmd_wifi_softap_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;

			if (softap->active)
			{
#if defined(CONFIG_SHOW_WIFI_PASSWORD)
				_atcmd_info("wifi_softap_get: channel=%u,%u ssid=%s security=%s password=%s",
						softap->channel_bw, softap->channel_freq,
						softap->ssid, softap->security, softap->password);

				if (softap->dhcp_server)
				{
					ATCMD_MSG_INFO("WSOFTAP", "%u,%.1f,\"%s\",\"%s\",\"%s\",\"dhcp\"",
							softap->channel_bw, softap->channel_freq / 10.,
							softap->ssid, softap->security, softap->password);
				}
				else
				{
					ATCMD_MSG_INFO("WSOFTAP", "%u,%.1f,\"%s\",\"%s\",\"%s\"",
							softap->channel_bw, softap->channel_freq / 10.,
							softap->ssid, softap->security, softap->password);
				}
#else
				_atcmd_info("wifi_softap_get: channel=%u,%u ssid=%s security=%s",
						softap->channel_bw, softap->channel_freq,
						softap->ssid, softap->security);

				if (softap->dhcp_server)
				{
					ATCMD_MSG_INFO("WSOFTAP", "%u,%.1f,\"%s\",\"%s\",\"\",\"dhcp\"",
							softap->channel_bw, softap->channel_freq / 10.,
							softap->ssid, softap->security, softap->password);
				}
				else
				{
					ATCMD_MSG_INFO("WSOFTAP", "%u,%.1f,\"%s\",\"%s\",\"\"",
							softap->channel_bw, softap->channel_freq / 10.,
							softap->ssid, softap->security, softap->password);
				}
#endif				
			}
			else
			{
				_atcmd_info("wifi_softap_get: no run");
			}

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_softap_set (int argc, char *argv[])
{
	char *param_bw = NULL;
	char *param_freq = NULL;
	char *param_ssid = NULL;
	char *param_security = NULL;
	char *param_password = NULL;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WSOFTAP=<frequency>[@<bandwidth][,\"<ssid>\"[,\"<security>\"[,\"<password>\"]]]");
			break;

		case 4:
			param_password = argv[3];

		case 3:
			param_security = argv[2];

		case 2:
			param_ssid = argv[1];

		case 1:
			param_freq = argv[0];

			param_bw = strchr(param_freq, '@');
			if (param_bw)
			{
				*param_bw = '\0';
				param_bw++;
			}
			else
				_atcmd_info("no bw");

			break;

		default:
			return ATCMD_ERROR_INVAL;
	}

	if (argc == 1)
	{
		switch (atoi(param_freq))
		{
			case 0:
				if (wifi_api_stop_softap() != 0)
					return ATCMD_ERROR_FAIL;

				_atcmd_wifi_softap_init_info();
				break;

			default:
				return ATCMD_ERROR_INVAL;
		}
	}
	else if (argc >= 2)
	{
		atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;

		int bw;
		float freq;
		atcmd_wifi_ssid_t str_ssid;
		atcmd_wifi_security_t  str_security;
		atcmd_wifi_password_t str_password;

		bw = 0;
		freq = 0;
		strcpy(str_ssid, "");
		strcpy(str_security, "open");
		strcpy(str_password, "");

		if (param_bw)
		{
			bw = atoi(param_bw);

			switch (bw)
			{
					case 1:
					case 2:
					case 4:
						break;

					default:
						return ATCMD_ERROR_INVAL;
			}
		}

/*		_atcmd_info("bw: %s", param_bw);
		_atcmd_info("freq: %s", param_freq);
		_atcmd_info("ssid: %s", param_ssid);
		_atcmd_info("security: %s", param_security);
		_atcmd_info("password: %s", param_password); */

		if (atoi(param_freq) == 0)
			freq = 0;
		else if (atcmd_param_to_float(param_freq, &freq) != 0)
			return ATCMD_ERROR_INVAL;

		if (param_ssid)
		{
			if (!atcmd_param_to_str(param_ssid, str_ssid, sizeof(str_ssid)))
				return ATCMD_ERROR_INVAL;
		}

		if (param_security)
		{
#if defined(CONFIG_ATCMD_SOFTAP_WPA3)
			bool wpa3 = true;
#else
			bool wpa3 = false;
#endif
			if (!atcmd_param_to_str(param_security, str_security, sizeof(str_security)))
				return ATCMD_ERROR_INVAL;

			if (!_atcmd_wifi_security_valid(str_security, wpa3))
				return ATCMD_ERROR_INVAL;
		}

		if (param_password)
		{
			if (!atcmd_param_to_str(param_password, str_password, sizeof(str_password)))
				return ATCMD_ERROR_INVAL;

			if (!_atcmd_wifi_password_valid(str_password))
				return ATCMD_ERROR_INVAL;
		}

		softap->channel_bw = bw;
		softap->channel_freq = (int)(freq * 10);

		strcpy(softap->ssid, str_ssid);
		strcpy(softap->security, str_security);
		strcpy(softap->password, str_password);

		_atcmd_debug("wifi_softap_set: bandwidth=%u freq=%u ssid=%s security=%s password=%s",
						softap->channel_bw, softap->channel_freq,
						softap->ssid, softap->security, softap->password);

		return _atcmd_wifi_softap_run(0, NULL);
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

#endif /* #ifdef CONFIG_ATCMD_SOFTAP */

/**********************************************************************************************/

#ifdef CONFIG_ATCMD_SOFTAP

static const char *str_ssid_type[] = { "full", "empty", "clear" };

static int _atcmd_wifi_softap_ssid_get (int argc, char *argv[])
{
	atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;

	switch (argc)
	{
		case 0:
		{
			int ssid_type = softap->ssid_type;

			_atcmd_info("wifi_softap_ssid_get: %s", str_ssid_type[ssid_type]);

			ATCMD_MSG_INFO("WSOFTAPSSID", "%d", ssid_type);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_softap_ssid_set (int argc, char *argv[])
{
	atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WSOFTAPSSID=<type>");
			break;

		case 1:
			if (softap->active)
			{
				_atcmd_info("wifi_softap_ssid_set: actived");
				return ATCMD_ERROR_NOTSUPP;
			}
			else
			{
				int ssid_type = atoi(argv[0]);

				switch (ssid_type)
				{
					case ATCMD_WIFI_SSID_FULL:
					case ATCMD_WIFI_SSID_EMPTY:
					case ATCMD_WIFI_SSID_CLEAR:
						_atcmd_info("wifi_softap_ssid_set: %s", str_ssid_type[ssid_type]);
						softap->ssid_type = ssid_type;
						return ATCMD_SUCCESS;
				}
			}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_softap_ssid =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "SOFTAPSSID",
	.id = ATCMD_WIFI_SOFTAP_SSID,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_softap_ssid_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_softap_ssid_set,
};

#endif /* #ifdef CONFIG_ATCMD_SOFTAP */

/**********************************************************************************************/

#ifdef CONFIG_ATCMD_SOFTAP

static int _atcmd_wifi_bss_max_idle_get (int argc, char *argv[])
{
	atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_INFO("WBSSMAXIDLE", "%u,%u",
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
	uint16_t period = 0;
	uint8_t retry = 0;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WBSSMAXIDLE=<period>[,<retry>]");
			break;

		case 2:
			if (atcmd_param_to_uint8(argv[1], &retry) != 0)
				return ATCMD_ERROR_INVAL;

			if (retry < ATCMD_WIFI_BSS_MAX_IDLE_RETRY_MIN || retry > ATCMD_WIFI_BSS_MAX_IDLE_RETRY_MAX)
				return ATCMD_ERROR_INVAL;

		case 1:
			if (atcmd_param_to_uint16(argv[0], &period) != 0)
				return ATCMD_ERROR_INVAL;

			if (retry == 0)
			{
				if (period == 0)
					retry = ATCMD_WIFI_BSS_MAX_IDLE_RETRY_MIN;
				else
					retry = softap->bss_max_idle.retry;
			}

			_atcmd_info("bss_max_idle: period=%u retry=%u", period, retry);

			if (wifi_api_set_bss_max_idle(period, retry) != 0)
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

#endif /* #ifdef CONFIG_ATCMD_SOFTAP */

/**********************************************************************************************/

#ifdef CONFIG_ATCMD_SOFTAP

static int _atcmd_wifi_stainfo_get (int argc, char *argv[])
{
	atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;
	int aid, aid_max;

	if (!softap->active)
		return ATCMD_ERROR_NOTSUPP;

	aid = 1;
	aid_max = softap->max_num_sta.current;

	switch (argc)
	{
		case 1:
			aid = atoi(argv[0]);
			if (aid <= 0)
				return ATCMD_ERROR_INVAL;

			if (aid > aid_max)
			   return ATCMD_ERROR_NOTSUPP;

			aid_max = aid;

		case 0:
		{
			atcmd_wifi_macaddr_t macaddr;
			int8_t rssi;
			uint8_t snr;
			uint8_t tx_mcs;
			uint8_t rx_mcs;
			int cnt;

			for (cnt = 0 ; aid <= aid_max ; aid++)
			{
				if (wifi_api_get_sta_info(aid, macaddr, &rssi, &snr, &tx_mcs, &rx_mcs) != 0)
					continue;

				cnt++;

#if defined(INCLUDE_STA_SIG_INFO)
				ATCMD_MSG_INFO("WSTAINFO", "%d,\"%s\",%d,%d,%d,%d", aid, macaddr, rssi, snr, tx_mcs, rx_mcs);
#else
				ATCMD_MSG_INFO("WSTAINFO", "%d,\"%s\"", aid, macaddr);
#endif
			}

			if (argc == 0 || cnt > 0)
				break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_stainfo_set (int argc, char *argv[])
{
	atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;
	int time_sec = 1;

	if (!softap->active)
		return ATCMD_ERROR_NOTSUPP;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WSTAINFO=<aid>[,<time>]");
			return ATCMD_SUCCESS;

		case 2:
			time_sec = atoi(argv[1]);
			if (time_sec <= 0)
				return ATCMD_ERROR_INVAL;

		case 1:
		{
			int ret;
			int i;

			for (i = 0 ; i < time_sec ; i++)
			{
				ret = _atcmd_wifi_stainfo_get(1, argv);
				if (ret != ATCMD_SUCCESS)
					return ret;

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

static atcmd_info_t g_atcmd_wifi_stainfo =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "STAINFO",
	.id = ATCMD_WIFI_STA_INFO,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_stainfo_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_stainfo_set,
};

#endif /* #ifdef CONFIG_ATCMD_SOFTAP */

/**********************************************************************************************/

#ifdef CONFIG_ATCMD_SOFTAP

static int _atcmd_wifi_max_sta_get (int argc, char *argv[])
{
	atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;

	switch (argc)
	{
		case 0:
		{
			uint8_t max_num_sta;

			if (wifi_api_get_max_num_sta(&max_num_sta) != 0)
				return ATCMD_ERROR_FAIL;

			if (max_num_sta != softap->max_num_sta.current)
			{
				_atcmd_info("max_sta_get: %u -> %u", softap->max_num_sta.current, max_num_sta);

				softap->max_num_sta.current = max_num_sta;
			}

			_atcmd_info("max_sta_get: %u", max_num_sta);

			ATCMD_MSG_INFO("WMAXSTA", "%d", max_num_sta);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_max_sta_set (int argc, char *argv[])
{
	atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WMAXSTA={1..%u}", softap->max_num_sta.system);
			break;

		case 1:
		{
			uint8_t max_num_sta;

			if (atcmd_param_to_uint8(argv[0], &max_num_sta) != 0)
				return ATCMD_ERROR_INVAL;

			if (max_num_sta < 0 || max_num_sta > softap->max_num_sta.system)
				return ATCMD_ERROR_INVAL;

			_atcmd_info("max_sta_set: %u/%u", max_num_sta, softap->max_num_sta.system);

			if (wifi_api_set_max_num_sta(max_num_sta) != 0)
				return ATCMD_ERROR_FAIL;

			softap->max_num_sta.current = max_num_sta;
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_max_sta =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "MAXSTA",
	.id = ATCMD_WIFI_MAX_STA,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_max_sta_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_max_sta_set,
};

#endif /* #ifdef CONFIG_ATCMD_SOFTAP */

/**********************************************************************************************/

#if !defined(NRC7292) && defined(CONFIG_ATCMD_SOFTAP) && defined(CONFIG_ATCMD_RELAY) 

static int _atcmd_wifi_relay_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;
			atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;

			if (connect->connected && softap->active)
			{
				if (strcmp(softap->security, connect->security) != 0)
				{
					_atcmd_error("security mismatch, ap=%s sta=%s",
							softap->security, connect->security);
				}
				else if (strcmp(softap->security, "open") != 0)
				{
					if (strcmp(softap->password, connect->password) != 0)
					{
#if defined(CONFIG_SHOW_WIFI_PASSWORD)
						_atcmd_error("%s password mismatch, ap=%s sta=%s",
								softap->security, softap->password, connect->password);
#else
						_atcmd_error("%s password mismatch", softap->security);
#endif						
					}
				}

#if defined(CONFIG_SHOW_WIFI_PASSWORD)
				_atcmd_info("wifi_relay_get: channel=%u,%u, ssid=%s,%s security=%s,%s",
						softap->channel_bw, softap->channel_freq,
						softap->ssid, connect->ssid,
						connect->security, connect->password);

				ATCMD_MSG_INFO("WRELAY", "%d,%.1f,\"%s\",\"%s\",\"%s\",\"%s\"",
						softap->channel_bw, softap->channel_freq / 10.,
						softap->ssid, connect->ssid,
						connect->security, connect->password);
#else
				_atcmd_info("wifi_relay_get: channel=%u,%u, ssid=%s,%s security=%s",
						softap->channel_bw, softap->channel_freq,
						softap->ssid, connect->ssid, connect->security);

				ATCMD_MSG_INFO("WRELAY", "%d,%.1f,\"%s\",\"%s\",\"%s\",\"\"",
						softap->channel_bw, softap->channel_freq / 10.,
						softap->ssid, connect->ssid, connect->security);
#endif				
			}
			else
			{
				_atcmd_info("wifi_relay_get: no run");
			}

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_relay_set (int argc, char *argv[])
{
	struct
	{
		int argc;

		union
		{
			char *argv[4];

			struct
			{
				char *freq;
				char *ssid;
				char *security;
				char *password;
			};
		};
	} params_ap;

	struct
	{
		int argc;

		union
		{
			char *argv[3];

			struct
			{
				char *ssid;
				char *security;
				char *password;
			};
		};
	} params_sta;

	char *param_security = NULL;
	char *param_password = NULL;

	memset(&params_ap, 0, sizeof(params_ap));
	memset(&params_sta, 0, sizeof(params_sta));

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WRELAY=\"<ap_ssid>\",\"<sta_ssid>\"[,\"<sta_security>\"[,\"<sta_password>\"]]");
			break;

		case 1:
			if (atoi(argv[0]) != 0)
				return ATCMD_ERROR_INVAL;

			if (!wifi_api_is_relay_mode())
				_atcmd_info("wifi_relay_set: already disabled");
			else
			{
				char *argv = "0";

				_atcmd_info("wifi_relay_set: disable");

				_atcmd_wifi_disconnect_run(0, NULL);
				_atcmd_wifi_softap_set(1, &argv);

				wifi_api_remove_network(false);
				wifi_api_remove_network(true);

				wifi_api_disable_relay_mode();
			}

			break;

		case 4:
			param_password = argv[3];

		case 3:
			param_security = argv[2];

		case 2:
			if (wifi_api_is_relay_mode())
			{
				_atcmd_info("wifi_relay_set: already enabled");
				return ATCMD_ERROR_BUSY;
			}

			params_ap.freq = "0";
			params_ap.ssid = argv[0];
			params_ap.argc = 2;

			params_sta.ssid = argv[1];
			params_sta.argc = 1;

			if (strlen(params_ap.ssid) == 0)
				params_ap.ssid = params_sta.ssid;

			if (param_security)
			{
#if defined(CONFIG_ATCMD_SOFTAP_WPA3)
				bool wpa3 = true;
#else
				bool wpa3 = false;
#endif
				atcmd_wifi_security_t str_security;

				if (!atcmd_param_to_str(param_security, str_security, sizeof(str_security)))
					return ATCMD_ERROR_INVAL;

				if (!_atcmd_wifi_security_valid(str_security, wpa3))
					return ATCMD_ERROR_INVAL;

				if (strcmp(str_security, "open") != 0)
				{
					if (!param_password)
						return ATCMD_ERROR_INVAL;

					params_ap.security = param_security;
					params_ap.password = param_password;
					params_ap.argc += 2;

					params_sta.security = param_security;
					params_sta.password = param_password;
					params_sta.argc += 2;
				}
			}

/*			_atcmd_debug("ap=%d sta=%d", params_ap.argc, params_sta.argc); */

			if (params_ap.argc >= 2 && params_sta.argc >= 1)
			{
				enum ATCMD_ERROR ret;

				_atcmd_info("wifi_relay_set: enable, ap=%s,%s,%s, sta=%s,%s,%s",
					params_ap.ssid, params_ap.security, params_ap.password,
					params_sta.ssid, params_sta.security, params_sta.password);

				wifi_api_enable_relay_mode();

				ret = _atcmd_wifi_connect_set(params_sta.argc, params_sta.argv);
			   	if (ret == ATCMD_SUCCESS)
				{
					ret = _atcmd_wifi_softap_set(params_ap.argc, params_ap.argv);
				   	if (ret != ATCMD_SUCCESS)
					{
						_atcmd_wifi_disconnect_run(0, NULL);
						wifi_api_remove_network(false);
					}
				}

				if (ret != ATCMD_SUCCESS)
					wifi_api_disable_relay_mode();

				return ret;
			}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}


static atcmd_info_t g_atcmd_wifi_relay =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "RELAY",
	.id = ATCMD_WIFI_RELAY,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_relay_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_relay_set,
};

#endif /* #if !defined(NRC7292) && defined(CONFIG_ATCMD_SOFTAP) && defined(CONFIG_ATCMD_RELAY) */

/**********************************************************************************************/

#ifdef CONFIG_ATCMD_WPS

static void _atcmd_wifi_wps_event_handler (enum WPS_STATUS status, int vif_id, ...)
{
	switch (status)
	{
		case WPS_SUCCESS:
		{
			atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;

			if (softap->active)
				_atcmd_info("[%d] wifi_wps_evt: success", vif_id);
			else
			{
				atcmd_wifi_connect_t *connect = &g_atcmd_wifi_info->connect;
				const char *ssid;
				const char *security;
				const char *password;
				va_list ap;

				va_start(ap, vif_id);

				ssid = va_arg(ap, const char *);
				security = va_arg(ap, const char *);
				password = va_arg(ap, const char *);

				va_end(ap);

				strcpy(connect->ssid, ssid);
				strcpy(connect->security, security);

				if (strcmp(security, "open") == 0)
					strcpy(connect->password, "");
				else
					strcpy(connect->password, password);					

				_atcmd_info("[%d] wifi_wps_evt: success, ssid=%s security=%s password=%d", 
							vif_id, ssid, security, strlen(password));
			}
				
			ATCMD_MSG_WEVENT("\"WPS_SUCCESS\"");
			break;
		}

		case WPS_TIMEOUT:
			_atcmd_info("[%d] wifi_wps_evt: timeout", vif_id);

			ATCMD_MSG_WEVENT("\"WPS_TIMEOUT\"");
			break;

		case WPS_FAIL:
			_atcmd_info("[%d] wifi_wps_evt: fail", vif_id);

			ATCMD_MSG_WEVENT("\"WPS_FAIL\"");
			break;

		default:
			_atcmd_info("[%d] wifi_wps_evt: unknown", vif_id);
	}
}

static int _atcmd_wifi_wps_enable (const char *bssid)
{
	wifi_wps_cb_t cb = _atcmd_wifi_wps_event_handler;

	_atcmd_info("wifi_wps: enable, bssid=%s", bssid);

	switch (wifi_api_enable_wps(bssid, cb))
	{
		case 0: 
			return ATCMD_SUCCESS;

		case -EBUSY: 
			return ATCMD_ERROR_BUSY;

		case -ETIMEDOUT: 
			return ATCMD_ERROR_TIMEOUT;
	}
			
	return ATCMD_ERROR_FAIL;
}

static int _atcmd_wifi_wps_disable (void)
{
	_atcmd_info("wifi_wps: disable");

	if (wifi_api_disable_wps() != 0)
		return ATCMD_ERROR_FAIL;

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_wps_run (int argc, char *argv[])
{
	if (wifi_api_is_relay_mode())
		return ATCMD_ERROR_NOTSUPP;

	return _atcmd_wifi_wps_enable(NULL); 
}

static int _atcmd_wifi_wps_set (int argc, char *argv[])
{
	atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;

	if (wifi_api_is_relay_mode() || softap->active)
		return ATCMD_ERROR_NOTSUPP;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WWPS=\"<bssid>\"");
			break;

		case 1:
			if (strcmp(argv[0], "0") == 0)
				return _atcmd_wifi_wps_disable();
			else
			{				
				atcmd_wifi_bssid_t bssid;

				if (atcmd_param_to_str(argv[0], bssid, sizeof(bssid)))
					if (_atcmd_wifi_bssid_valid(bssid))
						return _atcmd_wifi_wps_enable(bssid); 
			}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_wifi_wps =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "WPS",
	.id = ATCMD_WIFI_WPS,

	.handler[ATCMD_HANDLER_RUN] = _atcmd_wifi_wps_run,
	.handler[ATCMD_HANDLER_GET] = NULL,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_wps_set,
};

#endif /* #ifdef CONFIG_ATCMD_WPS */

/**********************************************************************************************/

#if defined(INCLUDE_MANUAL_CONT_TX_SUPPORT)

extern bool system_modem_api_set_cont_tx (bool enable, uint32_t freq_100k, const char* bw,
											uint8_t mcs, uint8_t txpwr, uint8_t type, uint32_t interval);

#define CTX_FREQ_MIN		7500
#define CTX_FREQ_MAX		9500

#define CTX_INTERVAL_MIN	10
#define CTX_INTERVAL_DEF	100

static struct
{
	bool enable;
	uint16_t freq;
	uint8_t bw;
	uint8_t mcs;
	uint8_t txpwr;
	uint8_t type;
	uint32_t interval;
} g_atcmd_wifi_ctx = { false, 0, 0, 0, 0, 0, 0 };

static int _atcmd_wifi_continuous_tx_start (void)
{
	_atcmd_info("cont_tx_start");

	if (g_atcmd_wifi_ctx.enable)
		return 1;
	else
	{
		uint32_t freq = g_atcmd_wifi_ctx.freq;
		uint8_t bw = g_atcmd_wifi_ctx.bw;
		uint8_t mcs = g_atcmd_wifi_ctx.mcs;
		uint8_t txpwr = g_atcmd_wifi_ctx.txpwr;
		uint8_t type = g_atcmd_wifi_ctx.type;
		uint32_t interval = g_atcmd_wifi_ctx.interval;
		char str_bw[2+1];

		snprintf(str_bw, sizeof(str_bw), "%cm", '0' + bw);

		if (!system_modem_api_set_cont_tx(true, freq, str_bw, mcs, txpwr, type, interval))
			return -1;

		g_atcmd_wifi_ctx.enable = 1;
	}

	return 0;
}

static void _atcmd_wifi_continuous_tx_stop (void)
{
	if (g_atcmd_wifi_ctx.enable)
	{
		_atcmd_info("cont_tx_stop");

		system_modem_api_set_cont_tx(false, 0, NULL, 0, 0, 0, 0);

		g_atcmd_wifi_ctx.enable = 0;
	}
}

static int _atcmd_wifi_continuous_tx_run (int argc, char *argv[])
{
	_atcmd_info("cont_tx_run: freq=%u bw=%u mcs=%u power=%u interval=%u",
				g_atcmd_wifi_ctx.freq, g_atcmd_wifi_ctx.bw, g_atcmd_wifi_ctx.mcs,
				g_atcmd_wifi_ctx.txpwr, g_atcmd_wifi_ctx.interval);

	switch (_atcmd_wifi_continuous_tx_start())
	{
		case 1:
			return ATCMD_ERROR_BUSY;

		case -1:
			return ATCMD_ERROR_FAIL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_continuous_tx_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			_atcmd_info("cont_tx_get: freq=%u bw=%u mcs=%u txpwr=%u interval=%u",
						g_atcmd_wifi_ctx.freq, g_atcmd_wifi_ctx.bw,
						g_atcmd_wifi_ctx.mcs, g_atcmd_wifi_ctx.txpwr,
						g_atcmd_wifi_ctx.interval);


			if (g_atcmd_wifi_ctx.interval == CTX_INTERVAL_DEF)
			{
				ATCMD_MSG_INFO("WCTX", "%u,%u,%u,%u",
							g_atcmd_wifi_ctx.freq, g_atcmd_wifi_ctx.bw,
							g_atcmd_wifi_ctx.mcs, g_atcmd_wifi_ctx.txpwr);
			}
			else
			{
				ATCMD_MSG_INFO("WCTX", "%u,%u,%u,%u,%u",
							g_atcmd_wifi_ctx.freq, g_atcmd_wifi_ctx.bw,
							g_atcmd_wifi_ctx.mcs, g_atcmd_wifi_ctx.txpwr,
							g_atcmd_wifi_ctx.interval);
			}

			break;

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_continuous_tx_set (int argc, char *argv[])
{
	uint16_t freq = 0;
	uint8_t bw = 0;
	uint8_t mcs = 0;
	uint8_t txpwr = 0;
	uint32_t interval = 0;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WCTX=<frequency>,<bandwidth>,<mcs>,<tx_power>");
			ATCMD_MSG_HELP("AT+WCTX=0");
			return ATCMD_SUCCESS;

		case 5:
			if (atcmd_param_to_uint32(argv[4], &interval) != 0 || interval < CTX_INTERVAL_MIN)
			{
				_atcmd_info("cont_tx_set: invalid interval (%u)", interval);
				break;
			}

		case 4:
			if (atcmd_param_to_uint8(argv[3], &txpwr) != 0 || (txpwr < TX_POWER_MIN || txpwr > TX_POWER_MAX))
			{
				_atcmd_info("cont_tx_set: invalid tx power (%u)", txpwr);
				break;
			}

			if (atcmd_param_to_uint8(argv[2], &mcs) != 0 || (mcs > 10 || mcs == 8 || mcs == 9))
			{
				_atcmd_info("cont_tx_set: invalid mcs (%u)", mcs);
				break;
			}

			if (atcmd_param_to_uint8(argv[1], &bw) != 0 || (bw != 1 && bw != 2 && bw != 4))
			{
				_atcmd_info("cont_tx_set: invalid bandwidth (%u)", bw);
				break;
			}

			if (atcmd_param_to_uint16(argv[0], &freq) != 0 || (freq < CTX_FREQ_MIN || freq > CTX_FREQ_MAX))
			{
				_atcmd_info("cont_tx_set: invalid frequency (%u)", freq);
				break;
			}

			if (g_atcmd_wifi_ctx.enable)
				return ATCMD_ERROR_BUSY;

			g_atcmd_wifi_ctx.freq = freq;
			g_atcmd_wifi_ctx.bw = bw;
			g_atcmd_wifi_ctx.mcs = mcs;
			g_atcmd_wifi_ctx.txpwr = txpwr;

			if (interval > 0)
				g_atcmd_wifi_ctx.interval = interval;
			else
				g_atcmd_wifi_ctx.interval = CTX_INTERVAL_DEF;

			_atcmd_info("cont_tx_set: freq=%u bw=%u mcs=%u txpwr=%u interval=%u",
						g_atcmd_wifi_ctx.freq, g_atcmd_wifi_ctx.bw,
						g_atcmd_wifi_ctx.mcs, g_atcmd_wifi_ctx.txpwr,
						g_atcmd_wifi_ctx.interval);

			return ATCMD_SUCCESS;

		case 1:
			if (atoi(argv[0]) != 0)
				break;

			_atcmd_wifi_continuous_tx_stop();

			return ATCMD_SUCCESS;
	}

	return ATCMD_ERROR_INVAL;
}

static atcmd_info_t g_atcmd_wifi_continuous_tx =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "CTX",
	.id = ATCMD_WIFI_CONTINUOUS_TX,

	.handler[ATCMD_HANDLER_RUN] = _atcmd_wifi_continuous_tx_run,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_continuous_tx_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_continuous_tx_set,
};

#endif /* #if defined(INCLUDE_MANUAL_CONT_TX_SUPPORT) */

/**********************************************************************************************/

#if defined(INCLUDE_MANUAL_CONT_TX_SUPPORT) && defined(NRC7394)

#define STX_FREQ_MIN		7500
#define STX_FREQ_MAX		9500

static struct
{
	bool enable;
	uint16_t freq;
	uint8_t bw;
	uint8_t txpwr;
} g_atcmd_wifi_stx = { false, 0, 0, 0 };

static int _atcmd_wifi_sine_tx_start (void)
{
	_atcmd_info("sine_tx_start");

	if (g_atcmd_wifi_stx.enable)
		return 1;
	else
	{
		uint32_t freq = g_atcmd_wifi_stx.freq;
		uint8_t bw = g_atcmd_wifi_stx.bw;
		uint8_t txpwr = g_atcmd_wifi_stx.txpwr;
		char str_bw[2+1];

		snprintf(str_bw, sizeof(str_bw), "%cm", '0' + bw);

		if (!system_modem_api_set_sine_tx(true, freq, str_bw, txpwr))
			return -1;

		g_atcmd_wifi_stx.enable = 1;
	}

	return 0;
}

static void _atcmd_wifi_sine_tx_stop (void)
{
	if (g_atcmd_wifi_stx.enable)
	{
		_atcmd_info("sine_tx_stop");

		system_modem_api_set_sine_tx(false, 0, NULL, 0);

		g_atcmd_wifi_stx.enable = 0;
	}
}

static int _atcmd_wifi_sine_tx_run (int argc, char *argv[])
{
	_atcmd_info("sine_tx_run: freq=%u bw=%u power=%u",
		g_atcmd_wifi_stx.freq, g_atcmd_wifi_stx.bw, g_atcmd_wifi_stx.txpwr);

	switch (_atcmd_wifi_sine_tx_start())
	{
		case 1:
			return ATCMD_ERROR_BUSY;

		case -1:
			return ATCMD_ERROR_FAIL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_sine_tx_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			_atcmd_info("sine_tx_get: freq=%u bw=%u txpwr=%u",
				g_atcmd_wifi_stx.freq, g_atcmd_wifi_stx.bw, g_atcmd_wifi_stx.txpwr);

			ATCMD_MSG_INFO("WSTX", "%u,%u,%u",
				g_atcmd_wifi_stx.freq, g_atcmd_wifi_stx.bw, g_atcmd_wifi_stx.txpwr);

			break;

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_wifi_sine_tx_set (int argc, char *argv[])
{
	uint16_t freq = 0;
	uint8_t bw = 0;
	uint8_t txpwr = 0;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+WSTX=<frequency>,<bandwidth>,<tx_power>");
			ATCMD_MSG_HELP("AT+WSTX=0");
			return ATCMD_SUCCESS;

		case 3:
			if (atcmd_param_to_uint8(argv[2], &txpwr) != 0 || (txpwr < TX_POWER_MIN || txpwr > TX_POWER_MAX))
			{
				_atcmd_info("sine_tx_set: invalid tx power (%u)", txpwr);
				break;
			}

			if (atcmd_param_to_uint8(argv[1], &bw) != 0 || (bw != 1 && bw != 2 && bw != 4))
			{
				_atcmd_info("sine_tx_set: invalid bandwidth (%u)", bw);
				break;
			}

			if (atcmd_param_to_uint16(argv[0], &freq) != 0 || (freq < STX_FREQ_MIN || freq > STX_FREQ_MAX))
			{
				_atcmd_info("sine_tx_set: invalid frequency (%u)", freq);
				break;
			}

			if (g_atcmd_wifi_stx.enable)
				return ATCMD_ERROR_BUSY;

			g_atcmd_wifi_stx.freq = freq;
			g_atcmd_wifi_stx.bw = bw;
			g_atcmd_wifi_stx.txpwr = txpwr;

			_atcmd_info("sine_tx_set: freq=%u bw=%u txpwr=%u",
					g_atcmd_wifi_stx.freq, g_atcmd_wifi_stx.bw, g_atcmd_wifi_stx.txpwr);

			return ATCMD_SUCCESS;

		case 1:
			if (atoi(argv[0]) != 0)
				break;

			_atcmd_wifi_sine_tx_stop();

			return ATCMD_SUCCESS;
	}

	return ATCMD_ERROR_INVAL;
}

static atcmd_info_t g_atcmd_wifi_sine_tx =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_WIFI,

	.cmd = "STX",
	.id = ATCMD_WIFI_SINE_TX,

	.handler[ATCMD_HANDLER_RUN] = _atcmd_wifi_sine_tx_run,
	.handler[ATCMD_HANDLER_GET] = _atcmd_wifi_sine_tx_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_wifi_sine_tx_set,
};

#endif /* #if defined(INCLUDE_MANUAL_CONT_TX_SUPPORT) && defined(NRC7394) */

/**********************************************************************************************/

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

static atcmd_info_t *g_atcmd_info_wifi[] =
{
	&g_atcmd_wifi_macaddr,
/*	&g_atcmd_wifi_macaddr0,
	&g_atcmd_wifi_macaddr1, */
	&g_atcmd_wifi_country,
	&g_atcmd_wifi_tx_power,
	&g_atcmd_wifi_rx_signal,
	&g_atcmd_wifi_rate_control,
	&g_atcmd_wifi_mcs,
	&g_atcmd_wifi_duty_cycle,
	&g_atcmd_wifi_cca_threshold,
	&g_atcmd_wifi_tx_time,
	&g_atcmd_wifi_tsf,
	&g_atcmd_wifi_beacon_interval,
	&g_atcmd_wifi_listen_interval,
	&g_atcmd_wifi_scan,
	&g_atcmd_wifi_scan_ssid,
#ifdef CONFIG_ATCMD_BGSCAN
	&g_atcmd_wifi_scan_background, 
#endif	
#if defined(CONFIG_ATCMD_WPA3) && defined(CONFIG_ATCMD_SAEPWE)
	&g_atcmd_wifi_sae_pwe,
#endif
	&g_atcmd_wifi_connect,
	&g_atcmd_wifi_disconnect,
	&g_atcmd_wifi_ipaddr,
#ifdef CONFIG_ATCMD_IPV6
	&g_atcmd_wifi_ipaddr6,
#endif
	&g_atcmd_wifi_dhcp,
#ifdef CONFIG_ATCMD_SOFTAP
	&g_atcmd_wifi_dhcps,
#endif
	&g_atcmd_wifi_ping,
#ifdef CONFIG_ATCMD_IPV6
	&g_atcmd_wifi_ping6,
#endif
	&g_atcmd_wifi_dns,
#ifdef CONFIG_ATCMD_FOTA
	&g_atcmd_wifi_fota,
#endif	
	&g_atcmd_wifi_deep_sleep,
#ifdef CONFIG_ATCMD_SOFTAP
	&g_atcmd_wifi_softap,
	&g_atcmd_wifi_softap_ssid,
	&g_atcmd_wifi_bss_max_idle,
	&g_atcmd_wifi_stainfo,
	&g_atcmd_wifi_max_sta,

#if !defined(NRC7292) && defined(CONFIG_ATCMD_RELAY)
	&g_atcmd_wifi_relay,
#endif
#endif
#ifdef CONFIG_ATCMD_WPS
	&g_atcmd_wifi_wps,
#endif
#ifdef INCLUDE_MANUAL_CONT_TX_SUPPORT
	&g_atcmd_wifi_continuous_tx,
#if defined(NRC7394)
	&g_atcmd_wifi_sine_tx,
#endif
#endif

	&g_atcmd_wifi_timeout,

	NULL
};

static int _atcmd_wifi_init_info (atcmd_wifi_info_t *info)
{
	atcmd_wifi_country_t country;
	bool cal_use = false;
	int id = -1;

	memset(info, 0, sizeof(atcmd_wifi_info_t));

	info->lock = xSemaphoreCreateMutex();
	if (!info->lock)
		return -1;

	info->event = 0;
	strcpy(info->country, "");

#if defined(NRC7292)
	if (wifi_api_get_rf_cal(&cal_use, country) != 0)
		cal_use = false;

	_atcmd_info("RF_CAL_INFO: cal_use=%d country=%s", cal_use, country);
#else
	if (wifi_api_get_rf_cal(&cal_use, country, &id) != 0)
		cal_use = false;

	_atcmd_info("RF_CAL_INFO: cal_use=%d country=%s id=%d", cal_use, country, id);

	if (strcmp(country, "KR") == 0)
	{
		switch (id)
		{
			case 1:
				_atcmd_info("RF_CAL_INFO: KR -> K1");
				strcpy(country, "K1");
				break;

			case 2:
				_atcmd_info("RF_CAL_INFO: KR -> K2");
				strcpy(country, "K2");
				break;
		}
	}
#endif

	if (cal_use && strlen(country) == 2)
		strcpy(info->country, country);
	else
	{
		int vif_id = 0;

		memcpy(info->country, lmac_get_country(vif_id), 2);
		info->country[2] = '\0';

/*		if (_atcmd_wifi_country_valid(info->country)) */
			_atcmd_info("LMAC_COUNTRY : %s", info->country);
	}

/*	if (!_atcmd_wifi_country_valid(info->country))
		strcpy(info->country, ATCMD_WIFI_INIT_COUNTRY); */

	info->txpower.type = ATCMD_WIFI_INIT_TXPOWER_TYPE;
	info->txpower.val = ATCMD_WIFI_TXPOWER_MAX;

	if (1)
	{
		atcmd_wifi_scan_t *scan = &info->scan;

		scan->scanning = false;
		memset(&scan->results, 0, sizeof(SCAN_RESULTS));
	}

	if (1)
	{
		atcmd_wifi_bgscan_t *bgscan = &info->bgscan;

		bgscan->scanning = false;
		bgscan->short_interval = 0;
		bgscan->long_interval = 0;
		bgscan->signal_threshold = 0;
	}

	if (1)
	{
		atcmd_wifi_connect_t *connect = &info->connect;

		connect->recovery = false;
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
		atcmd_wifi_ping_t *ping = &info->ping;

		_lwip_ping_params_init(&ping->params[0], false);
#ifdef CONFIG_ATCMD_IPV6
		_lwip_ping_params_init(&ping->params[1], true);
#endif
	}

#ifdef CONFIG_ATCMD_SOFTAP
	_atcmd_wifi_softap_init_info();
#endif

	if (1)
	{
		int i;

		for (i = 0 ; i < 4 ; i++)
			info->sae_pwe[i] = ATCMD_WIFI_INIT_SAE_PWE;
	}

	return 0;
}

static void _atcmd_wifi_deinit (void)
{
	if (g_atcmd_wifi_info)
	{
		if (g_atcmd_wifi_info->lock)
			vSemaphoreDelete(g_atcmd_wifi_info->lock);

#ifndef ATCMD_WIFI_INFO_STATIC
		_atcmd_free(g_atcmd_wifi_info);
		g_atcmd_wifi_info = NULL;
#endif
	}
}

static int _atcmd_wifi_init (void)
{
#ifndef ATCMD_WIFI_INFO_STATIC
	g_atcmd_wifi_info = _atcmd_malloc(sizeof(atcmd_wifi_info_t));
	if (!g_atcmd_wifi_info)
	{
		_atcmd_error("malloc()");
		return -1;
	}
#endif

	if (_atcmd_wifi_init_info(g_atcmd_wifi_info) == 0)
	{
		wifi_event_cb_t event_cb[ATCMD_WIFI_EVT_MAX] =
		{
			[ATCMD_WIFI_EVT_SCAN_DONE] = _atcmd_wifi_event_scan_done,
			[ATCMD_WIFI_EVT_CONNECT_SUCCESS] = _atcmd_wifi_event_connect_success,
			[ATCMD_WIFI_EVT_DISCONNECT] = _atcmd_wifi_event_disconnect,

			[ATCMD_WIFI_EVT_STA_CONNECT] = _atcmd_wifi_event_sta_connect,
			[ATCMD_WIFI_EVT_STA_DISCONNECT] = _atcmd_wifi_event_sta_disconnect
		};

		if (wifi_api_register_event_callback(event_cb) == 0)
		{
			if (!wifi_api_valid_country(g_atcmd_wifi_info->country))
			{
				_atcmd_info("wifi_init: no country");
				return 0;
			}

			if (wifi_api_set_country(g_atcmd_wifi_info->country) != 0)
			{
				_atcmd_info("wifi_init: failed to set country");
				return 0;
			}

			if (wifi_api_get_supported_channels(g_atcmd_wifi_info->country,
						&g_atcmd_wifi_info->supported_channels) != 0)
				_atcmd_info("wifi_init: failed to get supported channels");
			else
			{
				_atcmd_wifi_channels_print(&g_atcmd_wifi_info->supported_channels, false, false,
						"wifi_init: %s %d", g_atcmd_wifi_info->country,
						g_atcmd_wifi_info->supported_channels.n_channel);
			}

			if (wifi_api_set_tx_power(g_atcmd_wifi_info->txpower.type, g_atcmd_wifi_info->txpower.val) != 0)
				_atcmd_info("wifi_init: failed to set tx power");

			return 0;
		}
	}

	_atcmd_wifi_deinit();

	return -1;
}

bool atcmd_wifi_lock (void)
{
	SemaphoreHandle_t lock = g_atcmd_wifi_info->lock;
	const int timeout_msec = 60 * 1000;

	ASSERT(lock);

	if (!xSemaphoreTake(lock, pdMS_TO_TICKS(timeout_msec)))
	{
		_atcmd_error("timeout, %dms", timeout_msec);

		return false;
	}

	return true;
}

bool atcmd_wifi_unlock (void)
{
	SemaphoreHandle_t lock = g_atcmd_wifi_info->lock;

	ASSERT(lock);

	if (!xSemaphoreGive(lock))
	{
		_atcmd_error("fail");

		return false;
	}

	return true;
}

int atcmd_wifi_enable (void)
{
	int i;

	if (_atcmd_wifi_init() != 0)
		return -1;

	if (atcmd_group_register(&g_atcmd_group_wifi) != 0)
		return -1;

	for (i = 0 ; g_atcmd_info_wifi[i] ; i++)
	{
		if (atcmd_info_register(ATCMD_GROUP_WIFI, g_atcmd_info_wifi[i]) != 0)
			return -1;
	}

#if defined(CONFIG_ATCMD_INTERNAL)	
	for (i = 0 ; g_atcmd_info_wifi_internal[i] ; i++)
	{
		if (atcmd_info_register(ATCMD_GROUP_WIFI, g_atcmd_info_wifi_internal[i]) != 0)
			return -1;
	}
#endif	

	atcmd_info_print(&g_atcmd_group_wifi);

	return 0;
}

void atcmd_wifi_disable (void)
{
	int i;

#if defined(CONFIG_ATCMD_INTERNAL)	
	for (i = 0 ; g_atcmd_info_wifi_internal[i] ; i++)
		atcmd_info_unregister(ATCMD_GROUP_WIFI, g_atcmd_info_wifi_internal[i]->id);
#endif	

	for (i = 0 ; g_atcmd_info_wifi[i] ; i++)
		atcmd_info_unregister(ATCMD_GROUP_WIFI, g_atcmd_info_wifi[i]->id);

	atcmd_group_unregister(ATCMD_GROUP_WIFI);

	_atcmd_wifi_deinit();
}

void atcmd_wifi_deep_sleep_send_event (void)
{
	_atcmd_wifi_deep_sleep_send_event();
}

bool atcmd_wifi_softap_active (void)
{
	atcmd_wifi_softap_t *softap = &g_atcmd_wifi_info->softap;

	return softap->active;
}

