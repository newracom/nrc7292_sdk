#include "system_common.h"
#include "nrc_ps_api.h"
#include "nrc_wifi.h"

#include "driver_nrc.h"
#include "driver_nrc_scan.h"
#include "driver_nrc_debug.h"
#include "driver_nrc_ps.h"

#include "umac_info.h"
#include "umac_s1g_channel.h"

#include "lmac_util.h"
#include "lmac_ps_common.h"

#include "lwip/etharp.h"
#include "config_ssid.h"

#include "nrc_user_app.h"
#include "api_wifi.h"

#ifdef TAG
#undef TAG
#endif
#define TAG "wpa_ps: "

static int wpa_ps_check_hook(DRV_PS_HOOK_TYPE type, struct retention_info *ret_info)
{
	uint8_t empty_bssid[6] = {0,};

	/* No need to check HOOK_TYPE_PORT. Always hooked */
	if (type == WPA_PS_HOOK_TYPE_PORT) {
		return WPA_PS_HOOK_RET_SUCCESS;
	}

	if (lmac_is_ap(0)) {
		V(TT_WPAS, TAG "STA TYPE is AP! \n");
		return WPA_PS_HOOK_RET_FAIL_NOT_RECOVERED;
	}

	if (ret_info == NULL) {
		E(TT_WPAS, TAG "Error. retention is null! \n");
		return WPA_PS_HOOK_RET_FAIL_NO_RETENT;
	}

	if (!ret_info->recovered) {
		I(TT_WPAS, TAG "no hook. recovery is not started yet \n");
		return WPA_PS_HOOK_RET_FAIL_NOT_RECOVERED;
	}

	if (!ret_info->ap_info.ssid_len ||(memcmp(ret_info->ap_info.bssid, empty_bssid, 6) == 0)) {
		E(TT_WPAS, TAG "Error. No AP info in retention! \n");
		return WPA_PS_HOOK_RET_FAIL_NO_AP_INFO;
	}

#if 0 
	// scan and dhcp can be hooked even if wpa2/wpa3
	if (type == WPA_PS_HOOK_TYPE_SCAN || type == WPA_PS_HOOK_TYPE_DHCP) {
		return WPA_PS_HOOK_RET_SUCCESS;
	}

	if (ret_info->ap_info.security != WPA_KEY_MGMT_NONE) {
		I(TT_WPAS, TAG "WPA2/WPA3 recovery does not support yet\n");
		return WPA_PS_HOOK_RET_FAIL_NO_AP_INFO;
	}
#endif
	return WPA_PS_HOOK_RET_SUCCESS;
}

static int _build_internal_element(struct retention_info *ret_info , uint8_t *ie_chain)
{
	int pos =0;
	uint8_t non_s1g_channel;
	uint8_t support_rate[8] ={0x8c, 0x12, 0x98, 0x24, 0xb0, 0x48, 0x60, 0x6c};
	uint8_t vendor_ie[24]={0x00,0x50,0xf2,0x02,0x01,0x01,0x01,0x00,0x03,0xa4,0x00,0x00,0x27,0xa4,0x00,0x00,0x42,0x43,0x5e,0x00,0x62,0x32,0x2f,0x00};

	ASSERT(ie_chain);
	non_s1g_channel = STAGetNonS1GChNum(GetS1GChIDFromS1GFreq(ret_info->ch_info.ch_freq));

	// [0x0] SSID (2+SSID_LEN)
	*(ie_chain + pos) = 0x00;
	*(ie_chain+pos + 1) = ret_info->ap_info.ssid_len;
	pos += 2;
	os_memcpy((ie_chain+pos), ret_info->ap_info.ssid, ret_info->ap_info.ssid_len);
	pos += ret_info->ap_info.ssid_len;

	// [0x1] Supported rates ie (10B)
	*(ie_chain + pos) = 0x01;
	*(ie_chain +pos+1) = 0x08;
	pos += 2;
	memcpy (ie_chain+pos, support_rate, IE_LENGTH_SUPPORTED_RATES);
	pos += IE_LENGTH_SUPPORTED_RATES;

	// [0x3] DSSS parameters ie (3B)
	*(ie_chain+pos) = 0x03;
	*(ie_chain+pos+1) = 0x01;
	*(ie_chain+pos+2) = non_s1g_channel;
	pos += 3;

	// [0x2D] HT capa ie (28B)
	*(ie_chain+pos) = 0x2D;
	*(ie_chain+pos+1) = 0x1A;
	pos += 2 + IE_LENGTH_HT_CAPABILITIES;

	// [0x30] RSN info ie (for PSK/SAE security)
	if (ret_info->ap_info.security == WPA_KEY_MGMT_PSK || ret_info->ap_info.security == 8 ||
		ret_info->ap_info.security == 16) {
		uint8_t rsn_ie[22] = {0x30, 0x14, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x04, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x04, 0x01, 
		0x00, 0x00, 0x0f, 0xac, 0x00, 0x00, 0x00};
		os_memcpy (ie_chain+pos, rsn_ie, 22);
		pos += 19;
		if (ret_info->ap_info.security == WPA_KEY_MGMT_PSK) {
			*(ie_chain+pos) = 0x02; //akm type 2
			*(ie_chain+pos+1) = 0x0c;
		} else if (ret_info->ap_info.security == 8) {
			*(ie_chain+pos) = 0x08; //aky type 8
			*(ie_chain+pos+1) = 0xcc; //RSN Capa (11w)
		} else if (ret_info->ap_info.security == 16) {
			*(ie_chain+pos) = 0x12; //akm type 18 (11w)
			*(ie_chain+pos+1) = 0xcc;
		}
		pos += 3;
	}

	// [0x3D] HT operation ie (24B)
	*(ie_chain+pos) = 0x3d;
	*(ie_chain+pos+1) = 0x16;
	*(ie_chain+pos+2) = non_s1g_channel;
	pos += 2 + IE_LENGTH_HT_OPERATION;

	// [0x7F] extended capa ie (10B)
	*(ie_chain+pos) = 0x7f;
	*(ie_chain+pos+1) = 0x08;
	pos += 1+ IE_LENGTH_EXTENDED_CAPABILITIES;

	// [0xDD] Vendor specific ie (26B)
	*(ie_chain+pos) = 0xdd;
	*(ie_chain+pos+1) = 0x18;
	pos+=2;
	memcpy(ie_chain+pos, vendor_ie, 24);
	pos += 24;

	return pos;
}

extern void nrc_scan_done(int param);

static int wpa_ps_hook_handle_init(struct retention_info *ret_info, void *param1, void *param2)
{
	I(TT_WPAS, TAG "%s\n", __func__);
	return WPA_PS_HOOK_RET_SUCCESS;
}

static int wpa_ps_hook_handle_pmk(struct retention_info *ret_info, void *param1, void *param2)
{
	ASSERT(param1);
	ASSERT(ret_info->ap_info.security ==WPA_KEY_MGMT_PSK);
	uint8_t empty_pmk[32] = {0x00,};

	struct wpa_ssid *ssid = (struct wpa_ssid *)(param1);
	I(TT_WPAS, TAG "%s ssid_len:%d, ssid:%s\n", __func__, ssid->ssid_len, ssid->ssid);

	if (!ssid->ssid_len || (memcmp (ssid->ssid, ret_info->ap_info.ssid, ssid->ssid_len) != 0)) {
		E(TT_WPAS, TAG "%s ssid(%s) is different from ssid in retention. Initialize retention. \n",
			__func__, ssid->ssid);
		lmac_ps_set_init_retention();
		return WPA_PS_HOOK_RET_FAIL;
	}

	if (memcmp (ret_info->pmk, empty_pmk, 32) == 0) {
		E(TT_WPAS, TAG "%s empty pmk! \n",__func__);
		return WPA_PS_HOOK_RET_FAIL_NO_RETENT;
	}

	if (ret_info->pmk) {
		memcpy (ssid->psk, ret_info->pmk, 32);
		ssid->psk_set = 1;
	}

	return WPA_PS_HOOK_RET_SUCCESS;
}

static int wpa_ps_hook_handle_set_key(struct retention_info *ret_info, void *param1, void *param2)
{
	V(TT_WPAS, TAG "%s\n", __func__);
	return WPA_PS_HOOK_RET_SUCCESS;
}

static int wpa_ps_hook_handle_scan(struct retention_info *ret_info, void *param1, void *param2)
{
	ASSERT(param1);

	struct nrc_wpa_if *intf = (struct nrc_wpa_if *)(param1);
	struct nrc_wpa_scan_res *scan_res = NULL;
	int vif_id = intf->vif_id;
	uint16_t ie_chain_len = 0;
	uint16_t ie_copy_len = 0;
	u64 tsf = NOW;

	E(TT_WPAS, TAG "%d scan start\n", vif_id);

	scan_flush(intf->scan);

	/* Using retention info, make scan item and add it to scan list for next scan_results */
	scan_res = (struct nrc_wpa_scan_res *) os_malloc(sizeof(*scan_res));
	if (!scan_res) {
		E(TT_WPAS, TAG "Failed to allocate scan_res\n");
		return WPA_PS_HOOK_RET_FAIL_MEM_ALLOC;
	}
	/* ie chain => SSID (2+SSID_LEN) + Supported RATE (10B) + DSSS Pram (3B)  + HT Capa(28B)
		+ HT Oper (24B) + Ext Capa(10B) + Vendor (26B) + RSNIE (22B) only for wpa2/wpa3) */
	ie_chain_len = 2 + ret_info->ap_info.ssid_len + 101;
	if (ret_info->ap_info.security == WPA_KEY_MGMT_PSK || ret_info->ap_info.security == 8 ||
		ret_info->ap_info.security == 16) {
		ie_chain_len += 22;
	}
	scan_res->res = os_zalloc(sizeof(*scan_res->res) + ie_chain_len);
	if (!scan_res->res) {
		E(TT_WPAS, TAG "Failed to allocate scan entry (ie_chain_len: %d)\n", ie_chain_len);
		return WPA_PS_HOOK_RET_FAIL_MEM_ALLOC;
	}
	os_memcpy(scan_res->res->bssid, ret_info->ap_info.bssid, ETH_ALEN);
	scan_res->res->freq = STAGetNoneS1GFreq(ret_info->ch_info.ch_freq);
	scan_res->res->beacon_int = ret_info->ap_info.bcn_interval;
	scan_res->res->caps = 0x1;
	if (ret_info->ap_info.security == WPA_KEY_MGMT_PSK || ret_info->ap_info.security == 8 ||
		ret_info->ap_info.security == 16) {
		scan_res->res->caps |= 0x10;
	}
	scan_res->res->qual = 0;
	scan_res->res->noise = 0;
	os_memcpy(&(scan_res->res->tsf), &tsf, 8);
	scan_res->res->level = -10;
	scan_res->res->ie_len = ie_chain_len;
	ie_copy_len = _build_internal_element(ret_info, (uint8_t *) (scan_res->res+1));
	V(TT_WPAS, TAG "ie_copy_len(%d) vs alloc_chain_len(%d)\n", ie_copy_len, ie_chain_len);
	ASSERT(ie_copy_len <= ie_chain_len);
	dl_list_add(&intf->scan->scan_list, &scan_res->list);

	E(TT_WPAS, TAG "%d scan done\n",vif_id);

#if 0
	I(TT_WPAS, TAG "%d scan entry bssid(" MACSTR "), freq(%d), bi(%d), "
		"caps(%d), q(%d), noise(%d), level(%d), ie_len(%d)\n",
		vif_id,
		MAC2STR(scan_res->res->bssid),
		scan_res->res->freq,
		scan_res->res->beacon_int,
		scan_res->res->caps,
		scan_res->res->qual,
		scan_res->res->noise,
		scan_res->res->level,
		scan_res->res->ie_len);
#else
	E(TT_WPAS, TAG "%d bssid(" MACSTR "), freq(%d), bi(%d), caps(%d), level(%d)\n",
		vif_id, MAC2STR(scan_res->res->bssid),
		scan_res->res->freq,
		scan_res->res->beacon_int,
		scan_res->res->caps,
		scan_res->res->level);
#endif
	//print_hex(scan_res->res+1, ie_chain_len);

	nrc_scan_done(intf->vif_id);
	return WPA_PS_HOOK_RET_SUCCESS;
}

static int wpa_ps_hook_handle_auth(struct retention_info *ret_info, void *param1, void *param2)
{
	ASSERT(param1);

	if (ret_info->ap_info.security == 8) {
		/* Auth process must be skipped while connecting AP after deep sleep */
		ASSERT(0);
		//E(TT_WPAS, TAG "%d skip auth\n", 0);
		//return WPA_PS_HOOK_RET_SUCCESS;
	}

	E(TT_WPAS, TAG "%d auth\n", 0);

	struct nrc_wpa_if *intf = (struct nrc_wpa_if *)(param1);
	union wpa_event_data event;
	os_memset(&event, 0, sizeof(event));
	os_memcpy(event.auth.peer, ret_info->ap_info.bssid, ETH_ALEN);
	os_memcpy(event.auth.bssid, ret_info->ap_info.bssid, ETH_ALEN);
	event.auth.auth_type		= 0;
	event.auth.auth_transaction	= 2;
	event.auth.status_code		= 0;
	event.auth.ies_len = 0;
	event.auth.ies = NULL;

	//print_hex(&(event.auth), sizeof(event.auth));

	wpa_supplicant_event(intf->wpa_supp_ctx, EVENT_AUTH, &event);

	return WPA_PS_HOOK_RET_SUCCESS;
}

static int wpa_ps_hook_handle_assoc(struct retention_info *ret_info, void *param1, void *param2)
{
	ASSERT(param1);
	ASSERT(param2);

	struct nrc_wpa_if *intf = (struct nrc_wpa_if *)(param1);
	struct wpa_driver_associate_params *assoc_param= (struct wpa_driver_associate_params *)(param2);
	union wpa_event_data event;
	os_memset(&event, 0, sizeof(event));

	E(TT_WPAS, TAG "%d assoc (kms:%d)\n",
		intf->vif_id, assoc_param->key_mgmt_suite);
	intf->key_mgmt = assoc_param->key_mgmt_suite;
	intf->associated = true;
	intf->sta.aid = ret_info->sta_info.aid;
	intf->sta.qos = intf->bss.qos = true;
	intf->bss.max_idle = ret_info->sta_info.bss_max_idle_period;
	os_memcpy(intf->sta.addr, ret_info->ap_info.bssid, ETH_ALEN);
	for (int tid=0; tid < MAX_TID; tid++) {
		intf->sta.block_ack[tid] = ret_info->tid_info.ba_state[tid];
	}
#if 0
		if (ieee802_11_parse_elems(ie, ies_len, &elems, 1) == ParseOK) {
			/* In 11ah, QoS bit needs to be set also when EDCA Param IE is visible.
			* But, hook system underlying this translates EDCA IE to WMM IE.
			*/
			intf->bss.qos = intf->sta.qos = !!(elems.wmm);
			if (elems.bss_max_idle_period) {
				intf->bss.max_idle = WPA_GET_LE16(elems.bss_max_idle_period);
				os_get_time(&intf->bss.last_tx_time);
				/* TODO: Protected Keep Alive */
			}
		}
#endif
	event.assoc_info.reassoc = false;
	event.assoc_info.freq = STAGetNoneS1GFreq(ret_info->ch_info.ch_freq);
	if (intf->key_mgmt == WPA_KEY_MGMT_NONE)
		event.assoc_info.authorized = 0;
	else
		event.assoc_info.authorized = 1;
	event.assoc_info.resp_ies_len = 0;
	event.assoc_info.resp_ies = NULL;
	E(TT_WPAS, TAG "%d bss max idle (%d)\n", intf->vif_id, intf->bss.max_idle);
	if (intf->bss.max_idle) {
		nrc_start_keep_alive(intf);
	}
	wpa_supplicant_event(intf->wpa_supp_ctx, EVENT_ASSOC, &event);
	if (event.assoc_info.authorized) {
#if 1
		struct nrc_wpa_key *key = NULL;
		uint8_t br_addr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
		for (int i = 0; i < 4; i++) {
			if (!ret_info->key_info.cipher_info[i].key_enable) continue;
			ASSERT(i==ret_info->key_info.cipher_info[i].key_id);
			key = nrc_wpa_get_key_by_key_idx(intf, i);
			if (ret_info->key_info.cipher_info[i].key_type == KEY_PTK)
				memcpy (key->addr, ret_info->ap_info.bssid, 6);
			else
				memcpy (key->addr, br_addr,6);

			key->cipher = WPA_ALG_CCMP;
			key->ix = ret_info->key_info.cipher_info[i].key_id;
			key->tsc = ret_info->key_info.cipher_info[i].tsc;
			key->key_len = 16;
			memcpy (key->key, (uint8_t *) ret_info->key_info.cipher_info[i].key, 16);
			key->is_set[i] = true;
		}
		wpa_driver_debug_key_all(intf);
#endif
		wpa_supplicant_event(intf->wpa_supp_ctx, EVENT_PORT_AUTHORIZED, NULL);
	}
	wpa_driver_set_associate_status(true);
	wpa_driver_notify_event_to_app(EVENT_ASSOC, 0, NULL);

	return WPA_PS_HOOK_RET_SUCCESS;
}

static int wpa_ps_hook_handle_port(struct retention_info *ret_info, void *param1, void *param2)
{
	ASSERT(param1);
	ASSERT(param2);

	struct nrc_wpa_if *intf = (struct nrc_wpa_if *)(param1);
	int authorized = *((int*)(param2));

	E(TT_WPAS, TAG "%d set key (mgmt:%d, authorized:%d)\n", intf->vif_id, intf->key_mgmt, authorized);

	if (!intf->key_mgmt || intf->key_mgmt == WPA_KEY_MGMT_NONE ||authorized)
		return WPA_PS_HOOK_RET_FAIL;

	/* (PSK/SAE) When deauhorized, clear all keys instead of WPA Supplicant */
	wpa_driver_clear_key_all(intf);

	wpa_driver_debug_key_all(intf);

	return WPA_PS_HOOK_RET_SUCCESS;
}

static int wpa_ps_hook_handle_dhcp(struct retention_info *ret_info, void *param1, void *param2)
{
	ASSERT(param1);

	int vif_id = *(int *)param1;
	s8_t err;
	ip4_addr_t adrs;
	struct eth_addr bssid;

	E(TT_WPAS, TAG "%d dhcp\n",  vif_id);

	/* Check IP validity */
	if (ret_info->ip_info.ip_addr == 0) {
		E(TT_WPAS, TAG "%s IP is invalid in Retention\n", __func__);
		ret_info->recovered = false;
		return WPA_PS_HOOK_RET_FAIL_NO_IP;
	}

	/* Set IP and DNS */
	struct ip_info ip_info;
	ip_info.ip.addr = ret_info->ip_info.ip_addr;
	ip_info.netmask.addr = ret_info->ip_info.net_mask;
	ip_info.gw.addr = ret_info->ip_info.gw_addr;
	wifi_set_ip_info(vif_id, &ip_info);
	wifi_set_dns_server();

	/* Set state of WLAN Manager as "GET_IP" */
	nrc_wifi_set_state(WIFI_STATE_GET_IP);

	/* Add AP's MAC into ARP Entery */
	adrs.addr = ret_info->ip_info.gw_addr;
	memcpy(bssid.addr, ret_info->ap_info.bssid, 6);
	err = etharp_add_static_entry(&adrs, &bssid);
	if(err != ERR_OK)
		E(TT_WPAS, TAG "%s Fail to add ethar (err:%d)\n", __func__, err);
	else
		E(TT_WPAS, TAG "%d add arp\n", vif_id);

	/* Send Null for informing AP of Awake */
	lmac_send_qos_null_frame(false);

	E(TT_WPAS, TAG "%d recovery is done\n", vif_id);
	ret_info->recovered = false;

	return WPA_PS_HOOK_RET_SUCCESS;
}

static int wpa_ps_hook_handle_static(struct retention_info *ret_info, void *param1, void *param2)
{
	ret_info->recovered = false;
	return WPA_PS_HOOK_RET_SUCCESS;
}

typedef int (*wpa_ps_hook_handler)(struct retention_info *ret_info, void *param1, void *param2);
static const wpa_ps_hook_handler handlers[WPA_PS_HOOK_TYPE_MAX] = {
	[WPA_PS_HOOK_TYPE_INIT]	= wpa_ps_hook_handle_init,
	[WPA_PS_HOOK_TYPE_PMK]		= wpa_ps_hook_handle_pmk,
	[WPA_PS_HOOK_TYPE_SET_KEY]	= wpa_ps_hook_handle_set_key,
	[WPA_PS_HOOK_TYPE_SCAN]		= wpa_ps_hook_handle_scan,
	[WPA_PS_HOOK_TYPE_AUTH] 	= wpa_ps_hook_handle_auth,
	[WPA_PS_HOOK_TYPE_ASSOC]	= wpa_ps_hook_handle_assoc,
	[WPA_PS_HOOK_TYPE_PORT]	= wpa_ps_hook_handle_port,
	[WPA_PS_HOOK_TYPE_DHCP]	= wpa_ps_hook_handle_dhcp,
	[WPA_PS_HOOK_TYPE_STATIC]	= wpa_ps_hook_handle_static,
};

int wpa_driver_ps_hook_handle(DRV_PS_HOOK_TYPE type, void *param1, void *param2)
{
	struct retention_info *ret_info = nrc_ps_get_retention_info();
	int results = WPA_PS_HOOK_RET_SUCCESS;

	if ((results = wpa_ps_check_hook(type, ret_info)) !=  WPA_PS_HOOK_RET_SUCCESS) {
		return results;
	}

	V(TT_WPAS, TAG "%s\n", __func__);

	if (handlers[type])
		return handlers[type](ret_info, param1, param2);
	else
		return WPA_PS_HOOK_RET_FAIL_NO_HANDLER;
}

int8_t wpa_driver_ps_get_recovered()
{
	uint8_t empty_bssid[6] = {0,};
	struct retention_info *ret_info = nrc_ps_get_retention_info();

	if (lmac_is_ap(0) || (ret_info == NULL) ||  !ret_info->recovered) {
		return -1;
	}

	if (!ret_info->ap_info.ssid_len ||(memcmp(ret_info->ap_info.bssid, empty_bssid, 6) == 0)) {
		return -1;
	}

	return 0;
}