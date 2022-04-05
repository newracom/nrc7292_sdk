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

#include "nrc_sdk.h"
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"
#include "wlan_manager.h"
#include "nrc_ps_api.h"

#include "lwip/netif.h"
#ifdef SUPPORT_ETHERNET_ACCESSPOINT
#include "nrc_eth_if.h"
#endif

extern struct netif* nrc_netif[];

#define MAX_CNT 3


static void wifi_event_handler(tWIFI_EVENT_ID event, int data_len, char* data)
{
	char* ip_addr = NULL;
	tWIFI_STATUS ret = WIFI_FAIL;
	int cnt = 0;

	switch(event) {
		case WIFI_EVT_CONNECT_SUCCESS:
			nrc_usr_print("[%s] Receive Connection Success Event\n", __func__);
			if (!netif_is_link_up(nrc_netif[0])) {
				netif_set_link_up(nrc_netif[0]);
			}
#ifdef SUPPORT_ETHERNET_ACCESSPOINT
			if (get_network_mode() == NRC_NETWORK_MODE_BRIDGE) {
				break;
			}
#endif
			do {
				ret = nrc_wifi_set_ip_address();
				if(ret != WIFI_SUCCESS) {
					cnt++;
					nrc_usr_print("[%s] Fail to set IP addr(cnt %d)\n", __func__,cnt);
				}
				if(cnt == MAX_CNT) {
					nrc_wifi_set_state(WIFI_STATE_TRY_DISASSOC);
					break;
				}
			} while(ret != WIFI_SUCCESS);
			break;
		case WIFI_EVT_CONNECT_FAIL:
			nrc_usr_print("[%s] Receive Connection Fail Event\n", __func__);
			break;
		case WIFI_EVT_GET_IP:
			nrc_usr_print("[%s] Receive IP_GET Success Event\n", __func__);
			if( nrc_wifi_get_ip_address(&ip_addr) == WIFI_SUCCESS) {
				nrc_usr_print("[%s] IP Address : %s\n", __func__, ip_addr ? ip_addr : "null");
			}
			break;
		case WIFI_EVT_GET_IP_FAIL:
			nrc_usr_print("[%s] Receive IP_GET Fail Event\n", __func__);
			break;
		case WIFI_EVT_DISCONNECT:
			nrc_usr_print("[%s] Receive Disconnection Event\n", __func__);
			/* send netif down indication to LWIP */
			if (netif_is_link_up(nrc_netif[0])) {
				netif_set_link_down(nrc_netif[0]);
			}
			break;
		case WIFI_EVT_SCAN_DONE:
			nrc_usr_print("[%s] Receive Scan Done Event\n", __func__);
			break;
		case WIFI_EVT_START_SOFT_AP:
			nrc_usr_print("[%s] Receive Start Soft AP Event\n", __func__);
			break;
		case WIFI_EVT_SET_SOFT_AP_IP:
			nrc_usr_print("[%s] Receive SET IP Event\n", __func__);
			if( nrc_wifi_get_ip_address(&ip_addr) == WIFI_SUCCESS) {
				nrc_usr_print("[%s] IP Address : %s\n", __func__, ip_addr);
			}
			break;
		case WIFI_EVT_START_DHCP_SERVER:
			nrc_usr_print("[%s] Receive Start DHCPS Event\n", __func__);
			break;
		default:
			nrc_usr_print("[%s] Receive Unknown Event %d\n", __func__, event);
			break;
	}
}

int wifi_init(WIFI_CONFIG *param)
{
	int txpower;

	/* Register Wi-Fi Event Handler */
	nrc_wifi_register_event_handler(wifi_event_handler);

	/* Set IP mode config (Dynamic IP(DHCP client) or STATIC IP) */
	if (nrc_wifi_set_ip_mode((bool)param->ip_mode, (char *)param->static_ip) < 0) {
		nrc_usr_print("[%s] Fail to set static IP \n", __func__);
		return WIFI_SET_IP_FAIL;
	}

	/* Set TX Power */
	txpower = param->tx_power;

	if(nrc_wifi_set_tx_power(txpower) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail set TX Power\n", __func__);
		return WIFI_FAIL;
	}
	txpower = 0;
	nrc_wifi_get_tx_power(&txpower);
	nrc_usr_print("[%s] TX Power (%d dBm)\n", __func__, txpower);

	/* Set Country Code */
	if(nrc_wifi_set_country(nrc_wifi_country_from_string((char *)param->country)) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to set Country\n", __func__);
		return WIFI_FAIL;
	}

	return WIFI_SUCCESS;
}

int wifi_connect(WIFI_CONFIG *param)
{
	int index = -1;

	/* Try to connect with ssid and security */
	nrc_usr_print("[%s] Trying to Wi-Fi Connection...\n",__func__);

	if (nrc_wifi_add_network(&index) < 0) {
		nrc_usr_print("[%s] Fail to init \n", __func__);
		return WIFI_INIT_FAIL;
	}

	if (nrc_wifi_set_ssid(index, (char *)param->ssid) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to set SSID\n", __func__);
		return WIFI_FAIL;
	}

	if (strlen((char *)param->bssid) != 0){
		if (nrc_wifi_set_bssid(index, (char *)param->bssid) != WIFI_SUCCESS) {
			nrc_usr_print("[%s] Fail to set BSSID\n", __func__);
			return WIFI_FAIL;
		}
	}

	/* Set Non-S1G channel */
	if(param->s1g_channel != 0) {
		if(nrc_wifi_set_s1g_config(param->s1g_channel) != WIFI_SUCCESS) {
			nrc_usr_print("[%s] Fail to set S1G channel\n", __func__);
			return WIFI_FAIL;
		}
	}

	if (nrc_wifi_set_security(index, (int)param->security_mode, (char *)param->password) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to set Security\n", __func__);
		return WIFI_FAIL;
	}

	if (nrc_wifi_set_scan_freq(index, param->scan_freq_list, param->scan_freq_num) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to set Scan Freq\n", __func__);
		return WIFI_FAIL;
	}

	if (nrc_wifi_connect(index) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to Connect\n", __func__);
		return WIFI_CONNECTION_FAIL;
	}

	if (nrc_wifi_set_ip_address() != WIFI_SUCCESS) {
#ifdef SUPPORT_ETHERNET_ACCESSPOINT
	/* Do not set ip on bridge interface */
	/* TODO: incomplete. Should be fixed if bridge needs IP */
	if (get_network_mode() == NRC_NETWORK_MODE_BRIDGE) {
		return WIFI_SUCCESS;
	}
#endif
		nrc_usr_print("[%s] Fail to set IP Address\n", __func__);
		return WIFI_SET_IP_FAIL;
	}
 	return WIFI_SUCCESS;
}

int wifi_start_softap(WIFI_CONFIG *param)
{
	int index = -1;

	nrc_usr_print("[%s] Trying to start Soft AP (SSID:%s, S1G_CH:%d)\n",\
			 __func__, (char *)param->ssid,  (int)param->channel);

	if ((nrc_wifi_add_network(&index)) < 0) {
		nrc_usr_print("[%s] Fail to init \n", __func__);
		return WIFI_INIT_FAIL;
	}

	if(nrc_wifi_softap_set_conf(index, (char *)param->ssid, (int)param->channel,\
			(int)param->security_mode, (char *)param->password) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to set sotftap config\n", __func__);
		return WIFI_FAIL;
	}

	if(nrc_wifi_softap_start(index) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to start sotftap\n", __func__);
		return WIFI_SOFTAP_FAIL;
	}

	return WIFI_SUCCESS;
}

