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

#include "nrc_sdk.h"
#include "wifi_config.h"
#include "wifi_connect_common.h"

#include "driver_nrc.h"
#ifdef SUPPORT_ETHERNET_ACCESSPOINT
#include "nrc_eth_if.h"
#endif

#include "nrc_lwip.h"

#if LWIP_BRIDGE
#include "netif/bridgeif.h"
extern struct netif br_netif;
#endif /* LWIP_BRIDGE */

#ifdef CONFIG_IPV6
#include "lwip/ip_addr.h"
static char *static_ip6 = NULL;
#endif

static char *static_ip4 = NULL;
static char *static_netmask = NULL;
static char *static_gateway = NULL;

/* Time to wait for Wifi Pairing */
static uint32_t conn_wait_ms = 60 * 1000;

#define MAX_CNT 100
//#define MAX_CNT 9999

static void wifi_event_handler(int vif, tWIFI_EVENT_ID event, int data_len, void *data)
{
	char* ip_addr = NULL;
	tWIFI_STATUS ret = WIFI_FAIL;
	int cnt = 0;
	tWIFI_IP_MODE ip_mode;
	WIFI_CONFIG* wifi_config = nrc_get_global_wifi_config();

	switch(event) {
		case WIFI_EVT_CONNECT_SUCCESS:
#if defined(INCLUDE_TRACE_WAKEUP)
			nrc_usr_print("[%s] Receive Connection Success Event for Interface %d\n", __func__, vif);
#endif

			if (!netif_is_link_up(nrc_netif[vif])) {
				netif_set_link_up(nrc_netif[vif]);
			}

#if LWIP_BRIDGE
			if (wifi_config->network_mode == WIFI_NETWORK_MODE_BRIDGE) {
				nrc_usr_print("[%s] Adding nrc_netif[%d] to bridge...\n", __func__, vif);
				bridgeif_add_port(&br_netif, nrc_netif[vif]);
				if (wifi_config->ip_mode !=  WIFI_STATIC_IP) {
					wifi_bridge_dhcpc_start();
				}
				break;
			}
#endif /* LWIP_BRIDGE */

			nrc_wifi_get_ip_mode(vif, &ip_mode);
			if (ip_mode == WIFI_DYNAMIC_IP) {
				ret = nrc_wifi_set_ip_address(vif, ip_mode, wifi_config->dhcp_timeout, NULL, NULL, NULL);
			} else {
				ret = nrc_wifi_set_ip_address(vif, ip_mode, 0, static_ip4, static_netmask, static_gateway);
				if(ret != WIFI_SUCCESS) {
					if(nrc_wifi_get_state(vif) == WIFI_STATE_CONNECTED)
						nrc_wifi_set_state(vif,WIFI_STATE_DISCONNECTED);
					nrc_usr_print("[%s] Fail to set IP addr(cnt %d)\n", __func__,cnt);
					return;
				}
			}

#if defined(INCLUDE_ADD_ETHARP)
			if(ret == WIFI_SUCCESS) {
				ret = nrc_wifi_add_etharp(vif, NRC_REMOTE_ADDRESS, STR_WLAN_BSSID);
				if(ret != WIFI_SUCCESS)
					nrc_usr_print("[%s] Fail to add ARP info of AP\n", __func__);
#if defined(INCLUDE_EXTERNAL_BROKER)
				else {
					ret = nrc_wifi_add_etharp(vif, NRC_BROKER_ADDRESS, BROKER_BSSID);
					if(ret != WIFI_SUCCESS)
						nrc_usr_print("[%s] Fail to add ARP info of Broker\n", __func__);
				}
#endif
			}
#endif
#ifdef CONFIG_IPV6
			ip_addr_t addr;
			if (ipaddr_aton(static_ip6, &addr)) {
				int8_t index = 0;
				/* Set static ipv6 address to available slot in netif */
				if (netif_add_ip6_address(nrc_netif[vif], &addr.u_addr.ip6, &index) != ERR_OK) {
					nrc_usr_print("[%s] adding %s failed\n", __func__, static_ip6);
					return;
				}
				netif_ip6_addr_set_state(nrc_netif[vif], index, IP6_ADDR_PREFERRED);
			}
#endif
			break;
		case WIFI_EVT_DISCONNECT:
#if !defined(INCLUDE_MEASURE_AIRTIME)
			nrc_usr_print("[%s] Receive Disconnection Event\n", __func__);
#endif /* !defined(INCLUDE_MEASURE_AIRTIME) */
#if defined(INCLUDE_FAST_CONNECT)
			reset_ip_address(vif);
#endif
			nrc_wifi_stop_dhcp_client(vif);
			break;
		case WIFI_EVT_SCAN_DONE:
#if defined(INCLUDE_TRACE_WAKEUP)
			nrc_usr_print("[%s] Receive Scan Done Event\n", __func__);
#endif
			break;
		case WIFI_EVT_AP_STARTED:
#if !defined(INCLUDE_MEASURE_AIRTIME)
			nrc_usr_print("[%s] Receive Start Soft AP Event\n", __func__);
#endif /* !defined(INCLUDE_MEASURE_AIRTIME) */

#if LWIP_BRIDGE
			if (wifi_config->network_mode == WIFI_NETWORK_MODE_BRIDGE) {
				bridgeif_add_port(&br_netif, nrc_netif[vif]);
			}
#endif /* LWIP_BRIDGE */

			break;
		case WIFI_EVT_AP_STA_CONNECTED:
#if !defined(INCLUDE_MEASURE_AIRTIME)
			nrc_usr_print("[%s] Receive STA CONNECT Event ("MACSTR")\n",
				__func__, MAC2STR((uint8_t *)data));
#endif /* !defined(INCLUDE_MEASURE_AIRTIME) */
			/* Check STA Info */
			STA_INFO *sta = nrc_mem_malloc(sizeof(STA_INFO));
			if (sta) {
				nrc_wifi_softap_get_sta_by_addr(vif, (uint8_t *)data, sta);
				nrc_usr_print("STA  - state:%d aid:%d addr:"MACSTR" rssi:%d snr:%u\n",
					sta->state, sta->aid, MAC2STR(sta->addr), sta->rssi, sta->snr);
				nrc_mem_free(sta);
			}
			break;
		case WIFI_EVT_AP_STA_DISCONNECTED:
#if !defined(INCLUDE_MEASURE_AIRTIME)
			nrc_usr_print("[%s] Receive STA DISCONNECT Event ("MACSTR")\n",
				__func__, MAC2STR((uint8_t *)data));
#endif /* !defined(INCLUDE_MEASURE_AIRTIME) */
#if 0 //for api test. need to remove
			/* Check remained STA info */
			STA_LIST *sta_list = nrc_mem_malloc(sizeof(STA_LIST));
			tWIFI_STATUS ret;

			if (sta_list) {
				ret = nrc_wifi_softap_get_sta_list(0, sta_list, sizeof(STA_LIST));
				if (ret != WIFI_SUCCESS) {
					if(ret == WIFI_FAIL_SOFTAP_NOSTA)
						nrc_usr_print("[%s] No station is connected\n", __func__);
					else
						nrc_usr_print("[%s] Fail to get sta info\n", __func__);
					nrc_mem_free(sta_list);
					return;
				}
				nrc_usr_print("[%s] number of sta:%d (max_sta:%d size:%d)\n", __func__,
					sta_list->total_num, MAX_STA_CONN_NUM, sizeof(STA_LIST));
				for (int i=0; i < sta_list->total_num; i++) {
					if (sta_list->sta[i].aid && sta_list->sta[i].state == WIFI_STA_ASSOC) {
						nrc_usr_print("STA[%d] - state:%d aid:%d addr:"MACSTR" rssi:%d snr:%u\n",
							i, sta_list->sta[i].state, sta_list->sta[i].aid, MAC2STR(sta_list->sta[i].addr),
							sta_list->sta[i].rssi,sta_list->sta[i].snr);
					}
				}
			} else {
				nrc_usr_print("[%s] Fail to alloc mem\n", __func__);
				return;
			}
			nrc_mem_free(sta_list);
#endif

			break;
		default:
			break;
	}
}

static bool init_flag[NRC_WPA_NUM_INTERFACES] = {false, false};
tWIFI_STATUS wifi_init_with_vif(int vif, WIFI_CONFIG *param)
{
	uint8_t txpower = 0, txpower_type = 0;

	if (init_flag[vif]) {
		return WIFI_SUCCESS;
	}

	/* Register Wi-Fi Event Handler */
	if (nrc_wifi_register_event_handler(vif, wifi_event_handler) != WIFI_SUCCESS) {
		return WIFI_FAIL;
	}

	/* Set IP mode config (Dynamic IP(DHCP client) or STATIC IP) */
	if (nrc_wifi_set_ip_mode(vif, (bool)param->ip_mode, (char *)param->static_ip)<0) {
		nrc_usr_print("[%s] Fail to set static IP \n", __func__);
		return WIFI_FAIL_SET_IP;
	}

	static_ip4 = (char *) param->static_ip;
	static_netmask = (char *) param->netmask;
	static_gateway = (char *) param->gateway;

#ifdef CONFIG_IPV6
	ip_addr_t addr;
	static_ip6 = (char *) param->static_ip6;
	if (ipaddr_aton(static_ip6, &addr)) {
		int8_t index = 0;
		/* Set static ipv6 address to available slot in netif */
		if (netif_add_ip6_address(nrc_netif[vif], &addr.u_addr.ip6, &index) != ERR_OK) {
			nrc_usr_print("[%s] adding %s failed\n", __func__, param->static_ip6);
			return WIFI_FAIL;
		}
		netif_ip6_addr_set_state(nrc_netif[vif], index, IP6_ADDR_PREFERRED);
	}
#endif

	if (nrc_wifi_get_state(vif) == WIFI_STATE_CONNECTED) {
		A("[%s] Wi-Fi already connected\n", __func__);
		init_flag[vif] = true;
		return WIFI_SUCCESS;
	}

	/* Set Country Code */
	if(nrc_wifi_set_country(vif, nrc_wifi_country_from_string((char *)param->country)) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to set Country\n", __func__);
		return WIFI_FAIL;
	}

	/* Set TX Power and types */
	txpower = param->tx_power;
	txpower_type = param->tx_power_type;

	if(nrc_wifi_set_tx_power(txpower,txpower_type) != WIFI_SUCCESS)
	{
		nrc_usr_print("[%s] Fail set TX Power\n", __func__);
		return WIFI_FAIL;
	}

	if (nrc_wifi_add_network(vif) < 0) {
		nrc_usr_print("[%s] Fail to init \n", __func__);
		return WIFI_FAIL_INIT;
	}

	if(param->scan_freq_num > 0){
		if (nrc_wifi_set_scan_freq(vif, param->scan_freq_list, param->scan_freq_num) != WIFI_SUCCESS) {
			nrc_usr_print("[%s] Fail to set Scan Freq\n", __func__);
			return WIFI_FAIL;
		}
	}

	if(nrc_wifi_set_rate_control(vif, (bool)param->rc) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to set rate control\n", __func__);
		return WIFI_FAIL;
	}

	if(nrc_wifi_set_mcs (param->mcs) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to set mcs\n", __func__);
		return WIFI_FAIL;
	}

	if(nrc_wifi_set_gi(param->gi) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to set guard interval\n", __func__);
		return WIFI_FAIL;
	}

	if(nrc_wifi_set_cca_threshold(vif, param->cca_thres) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to set cca threshold\n", __func__);
		return WIFI_FAIL;
	}

	init_flag[vif] = true;

	return WIFI_SUCCESS;
}

tWIFI_STATUS wifi_deinit_with_vif(int vif)
{
	if (nrc_wifi_remove_network(vif) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Failed to remove network\n", __func__);
	}

	/* Unregister Wi-Fi Event Handler */
	nrc_wifi_unregister_event_handler(vif, wifi_event_handler);

	init_flag[vif] = false;

	return WIFI_SUCCESS;
}

tWIFI_STATUS wifi_connect_with_vif(int vif, WIFI_CONFIG *param)
{
	tWIFI_STATUS status = WIFI_SUCCESS;

	if (nrc_wifi_get_state(vif) == WIFI_STATE_CONNECTED) {
		A("[%s] Wi-Fi already connected\n", __func__);
		return WIFI_SUCCESS;
	}

	/* Try to connect with ssid and security */
#if !defined(INCLUDE_MEASURE_AIRTIME)
	nrc_usr_print("[%s] Trying Wi-Fi connection to '%s'...\n",__func__, param->ssid);

#endif /* !defined(INCLUDE_MEASURE_AIRTIME) */

	status = nrc_wifi_set_ssid(vif, (char *)param->ssid);

	if (status != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to set SSID %d\n", __func__, status);
		return WIFI_FAIL;
	}

	if (strlen((char *)param->bssid) != 0 && strcmp((char *)param->bssid, "00:00:00:00:00:00")!= 0){
		status = nrc_wifi_set_bssid(vif, (char *)param->bssid);

		if (status != WIFI_SUCCESS) {
			nrc_usr_print("[%s] Fail to set BSSID %d\n", __func__, status);
			return WIFI_FAIL;
		}
	}

	/* Set Non-S1G channel */
	if(param->channel != 0) {
		status = nrc_wifi_set_channel_freq(vif, param->channel);
		if(status != WIFI_SUCCESS) {
			nrc_usr_print("[%s] Fail to set S1G channel %d\n", __func__, status);
			return WIFI_FAIL;
		}
	}

	if (param->security_mode == WIFI_SEC_WPA2 ) {
		if((strlen((char *)param->pmk)>0) && (param->eap_type == 0)) {
			status = nrc_wifi_set_pmk(vif, (char *)param->pmk);
			if (status != WIFI_SUCCESS) {
				nrc_usr_print("[%s] Fail to set PMK %d\n", __func__, status);
				return WIFI_FAIL;
			}
		} else {
			if(param->eap_type){
				status = nrc_wifi_set_eap_security(vif, (int)param->security_mode, (int)param->eap_type,
							(char *)param->identity, (char *)param->password, 	param->eap_ca_cert,
							param->eap_client_cert, param->eap_private_key, (char *)param->private_key_password);
				if (status != WIFI_SUCCESS) {
					nrc_usr_print("[%s] Fail to set EAP Security %d\n", __func__, status);
					return WIFI_FAIL;
				}
			} else {
				status = nrc_wifi_set_security(vif, (int)param->security_mode, (char *)param->password);
				if (status != WIFI_SUCCESS) {
					nrc_usr_print("[%s] Fail to set Security %d\n", __func__, status);
					return WIFI_FAIL;
				}
			}
		}
	} else if (param->security_mode == WIFI_SEC_WPA3_SAE ) {
		status = nrc_wifi_set_security(vif, (int)param->security_mode, (char *)param->password);
		if (status != WIFI_SUCCESS) {
			nrc_usr_print("[%s] Fail to set Security %d\n", __func__, status);
			return WIFI_FAIL;
		}
	} else {
		status = nrc_wifi_set_security(vif, (int)param->security_mode, NULL);
		if (status != WIFI_SUCCESS) {
			nrc_usr_print("[%s] Fail to set Security %d\n", __func__, status);
			return WIFI_FAIL;
		}
	}

	if (param->security_mode == WIFI_SEC_WPA3_SAE ) {
		status = nrc_wifi_set_sae_pwe(vif, (int)param->sae_pwe);
		if (status != WIFI_SUCCESS) {
			nrc_usr_print("[%s] Fail to set sae_pwe %d\n", __func__, status);
			return WIFI_FAIL;
		}
	}

	if (param->listen_interval > 0) {
		status = nrc_wifi_set_listen_interval(vif, param->listen_interval);
		if (status != WIFI_SUCCESS) {
			nrc_usr_print("[%s] Fail to set listen interval %d\n", __func__, status);
			return WIFI_FAIL_CONNECT;
		}
	}

	if (param->scan_mode == WIFI_SCAN_MODE_PASSIVE) {
		status = nrc_wifi_set_passive_scan(true);
		if(status != WIFI_SUCCESS) {
			nrc_usr_print("[%s] Fail to set passive scan %d\n", __func__, status);
			return WIFI_FAIL;
		}
	}

	nrc_usr_print("[%s] bgsscan %s\n", __func__, param->bgscan_enable ? "Enable" : "Disable");
	if (param->bgscan_enable == true) {
		status = nrc_wifi_set_simple_bgscan(vif, param->bgscan_short, param->bgscan_thresh, param->bgscan_long);
		nrc_usr_print("[%s] short %d, thres %d, long %d\n", __func__, param->bgscan_short, param->bgscan_thresh, param->bgscan_long);
		if(status != WIFI_SUCCESS) {
			nrc_usr_print("[%s] Fail to set bgsscan %d\n", __func__, status);
			return WIFI_FAIL;
		}
	}


	if (param->auth_control == WIFI_ENABLE_AUTH_CONTROL) {
		status = nrc_wifi_set_enable_auth_control(vif, true);
		if(status != WIFI_SUCCESS) {
			nrc_usr_print("[%s] Fail to set auth. control %d\n", __func__, status);
			return WIFI_FAIL;
		}
	}

	status = nrc_wifi_connect(vif, param->conn_timeout);
	if (status != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to Connect %d\n", __func__, status);
		return WIFI_FAIL_CONNECT;
	}

 	return WIFI_SUCCESS;
}

tWIFI_STATUS wifi_start_softap_with_vif(int vif, WIFI_CONFIG *param)
{
	nrc_usr_print("[%s] Trying to start Soft AP (SSID:%s, S1G_CH:%d , BW:%d)\n", \
			 __func__, (char *)param->ssid,  (int)param->channel,  (int)param->bw);

	if(nrc_wifi_softap_set_conf(vif, (char *)param->ssid, (int)param->channel, (int)param->bw,
			(int)param->security_mode, (char *)param->password, (int)param->sae_pwe) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to set sotftap config\n", __func__);
		return WIFI_FAIL;
	}

	if(param->bss_max_idle > 0) {
		uint32_t disassociate_timeout = 0;
		disassociate_timeout = (param->bss_max_idle*param->bss_retry_cnt);
		nrc_usr_print("[%s] bss max idle : %d [sec]\n", __func__, param->bss_max_idle);
		nrc_usr_print("[%s] bss max idle count : %d\n", __func__, param->bss_retry_cnt);
		nrc_usr_print("[%s] Disassociate Timeout : %d [sec]\n", __func__, disassociate_timeout);
		nrc_wifi_softap_set_bss_max_idle(vif, param->bss_max_idle, param->bss_retry_cnt);
	}

	if(nrc_wifi_set_rate_control(vif, (bool)param->rc) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to set rate control\n", __func__);
		return WIFI_FAIL;
	}

	if(nrc_wifi_set_mcs (param->mcs) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to set mcs\n", __func__);
		return WIFI_FAIL;
	}

	if(nrc_wifi_set_cca_threshold(vif, param->cca_thres) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to set cca threshold\n", __func__);
		return WIFI_FAIL;
	}

	if(nrc_wifi_softap_set_ignore_broadcast_ssid(vif, param->ignore_broadcast_ssid) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to set hidden ssid\n", __func__);
		return WIFI_FAIL;
	}

	if(nrc_wifi_softap_set_max_num_sta(vif,  param->max_num_sta) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to set max_num_sta\n", __func__);
		return WIFI_FAIL;
	}

	if(nrc_wifi_softap_set_beacon_interval(vif,  param->bcn_interval) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to set bcn_interval %d\n", __func__, param->bcn_interval);
		return WIFI_FAIL;
	}

	if (param->auth_control == WIFI_ENABLE_AUTH_CONTROL) {
		if(nrc_wifi_set_enable_auth_control(vif, true) != WIFI_SUCCESS) {
			nrc_usr_print("[%s] Fail to set auth. control\n", __func__);
			return WIFI_FAIL;
		}
		nrc_wifi_set_auth_control_param(100, 8, 16); //slot:100, ti_min:8 bi, ti_max:16 bi
		nrc_wifi_set_auth_control_scale(10); // bi* 10
	}

	if(nrc_wifi_softap_start(vif) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail to start sotftap\n", __func__);
		return WIFI_FAIL_SOFTAP;
	}
	return WIFI_SUCCESS;
}

tWIFI_STATUS wifi_init(WIFI_CONFIG *param)
{
	return wifi_init_with_vif(0, param);
}

tWIFI_STATUS wifi_deinit(void)
{
	return wifi_deinit_with_vif(0);
}

tWIFI_STATUS wifi_connect(WIFI_CONFIG *param)
{
	return wifi_connect_with_vif(0, param);
}

tWIFI_STATUS wifi_start_softap(WIFI_CONFIG *param)
{
	return wifi_start_softap_with_vif(0, param);
}

nrc_err_t nrc_wait_for_ip(int vif, uint32_t timeout)
{
	int ip_retry_check = 0;
	while (1) {
		ip_retry_check++;
		if (nrc_wifi_get_state(vif) == WIFI_STATE_DISCONNECTED) {
			return NRC_FAIL;
		}

		if (nrc_addr_get_state(vif) == NET_ADDR_SET) {
			struct netif *interface = nrc_netif_get_by_idx(vif);
			nrc_usr_print("[%s] IP ...\n",__func__);
			nrc_usr_print("IP4 : %s\n", ipaddr_ntoa(&interface->ip_addr));
			nrc_usr_print("Netmask : %s\n", ipaddr_ntoa(&interface->netmask));
			nrc_usr_print("Gateway : %s\n", ipaddr_ntoa(&interface->gw));
			break;
		} else {
			nrc_usr_print("[%s] Waiting for IP to be assigned (%d)...\n", __func__, ip_retry_check);
		}

		if (timeout > 0) {
			if (ip_retry_check >= timeout) {
				nrc_usr_print("[%s] Failed to receive IP.\n", __func__);
				return NRC_FAIL;
			}
		}
		_delay_ms(1000);
	}

	return NRC_SUCCESS;
}
