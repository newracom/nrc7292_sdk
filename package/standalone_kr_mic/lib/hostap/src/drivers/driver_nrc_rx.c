#include "system_common.h"
#include "system_memory_manager.h"
#include "system_modem_api.h"
#include "umac_s1g_agent.h"
#include "lmac_common.h"
#include "utils/includes.h"
#include "utils/common.h"
#include "../common/wpa_common.h"
#include "../common/ieee802_11_common.h"
#include "../common/eapol_common.h"
#include "utils/eloop.h"
#include "utils/wpa_debug.h"
#include "driver_nrc.h"
#include "driver_nrc_scan.h"
#include "driver_nrc_debug.h"
#include "nrc-wim-types.h"
#include "nrc-vendor.h"
#include "nrc-wim-types.h"
#include "umac_wim_builder.h"
#include "../../lwip/port/lwip.h"
#ifdef TAG
#undef TAG
#endif
#define TAG "wpa_rx: "

static bool linearize(SYS_BUF *src, uint8_t *dst, int start_src_offset,
					int start_dst_offset, int length)
{
	ASSERT(dst);
	ASSERT(src);

	bool res = false;

	int src_offset = start_src_offset;
	int dst_offset = start_dst_offset;

	int src_buffer_length = LMAC_CONFIG_BUFFER_SIZE - src_offset;

	SYS_BUF *src_buffer = src;
	uint8_t *dst_buffer = dst + dst_offset;
	int remain = length;

	while(src_buffer && remain) {
		int copy_len = MIN(src_buffer_length, remain);

		memcpy(dst_buffer + dst_offset,
			(uint8_t*)src_buffer + src_offset, copy_len);
		dst_offset = 0;
		src_offset = SYS_HDR_SIZE;
		dst_buffer += copy_len;
		src_buffer = SYS_BUF_LINK(src_buffer);
		src_buffer_length = LMAC_CONFIG_BUFFER_SIZE - src_offset;
		remain -= copy_len;
	}
	if (!remain) res = true;

	return res;
}

#define P_OFFSET (sizeof(SYS_HDR) + sizeof(LMAC_RXBUF))

static uint16_t wpa_rx_to_freq(struct nrc_wpa_rx_data *rx)
{
#if defined(NRC7291_SDK_DUAL_CM3)
	return nrc_mbx_channel_get();
#endif
#if defined(STANDARD_11N) // TODO : Fix the below
	return ieee80211_chan_to_freq(NULL, 81, rx->rxi->center_freq);
#endif
#if defined(STANDARD_11AH)
	return lmac_get_mac80211_frequency(rx->intf->vif_id);
#endif
}

static void nrc_wpa_scan_sta_rx(struct nrc_wpa_rx_data *rx)
{
	int res = scan_add(rx->intf->scan, wpa_rx_to_freq(rx), rx->rxh->rssi,
		rx->u.frame, rx->len);

	if (res == -ENOMEM) {
		struct wim_builder *wb = wim_builder_create(rx->intf->vif_id,
			WIM_CMD_SCAN_STOP, WB_MIN_SIZE);
		wim_builder_run_wm(wb, true);
	}
}

static void nrc_mgmt_auth_sta_rx(struct nrc_wpa_if* intf,
				struct ieee80211_mgmt *mgmt,
				uint16_t len)
{
	union wpa_event_data event;
	os_memset(&event, 0, sizeof(event));
	int ie_offset = offsetof(struct ieee80211_mgmt, u.auth.variable);

	os_memcpy(event.auth.peer, mgmt->sa, ETH_ALEN);
	os_memcpy(event.auth.bssid, mgmt->sa, ETH_ALEN);

	event.auth.auth_type 		= mgmt->u.auth.auth_alg;
	event.auth.auth_transaction = mgmt->u.auth.auth_transaction;
	event.auth.status_code 		= mgmt->u.auth.status_code;

	event.auth.ies_len = len - ie_offset;
	event.auth.ies = (uint8_t *) mgmt + ie_offset;

	wpa_supplicant_event(intf->wpa_supp_ctx, EVENT_AUTH, &event);
}

static void nrc_mgmt_deauth_event_sta_rx(struct nrc_wpa_if* intf,
				struct ieee80211_mgmt *mgmt,
				uint16_t len)
{
	int ie_offset;
	union wpa_event_data event;

	os_memset(&event, 0, sizeof(event));
	ie_offset = offsetof(struct ieee80211_mgmt, u.deauth.variable);

	event.deauth_info.addr = mgmt->sa;
	event.deauth_info.reason_code = mgmt->u.deauth.reason_code;
	event.deauth_info.ie_len = len - ie_offset;
	event.deauth_info.ie = (uint8_t *) mgmt + ie_offset;
	event.deauth_info.locally_generated = 0;

	wpa_supplicant_event(intf->wpa_supp_ctx, EVENT_DEAUTH, &event);

#if defined(NRC_USER_APP)
	I(TT_WPAS, TAG "RX Deauth frame from AP"
			"(addr="MACSTR", reason=%d)\n",
			MAC2STR(mgmt->sa), mgmt->u.deauth.reason_code);
	reset_ip_address(intf->vif_id);
	wpa_driver_notify_event_to_app(EVENT_DEAUTH);
#endif
}

static void nrc_mgmt_deauth_sta_rx(struct nrc_wpa_if* intf,
					struct ieee80211_mgmt *mgmt,
					uint16_t len)
{
	if(!intf || !intf->associated) {
		I(TT_WPAS, TAG "Deauth frame from other BSS, ignored "
				"(da= "MACSTR", sa="MACSTR", reason=%d)\n",
				MAC2STR(mgmt->da), MAC2STR(mgmt->sa),
				mgmt->u.deauth.reason_code);
		return;
	}

	nrc_mgmt_deauth_event_sta_rx(intf, mgmt, len);
	wpa_driver_sta_sta_remove(intf);
}

static void nrc_mgmt_disassoc_sta_rx(struct nrc_wpa_if* intf,
				struct ieee80211_mgmt *mgmt,
				uint16_t len)
{
	union wpa_event_data event;

	os_memset(&event, 0, sizeof(event));
	int ie_offset = offsetof(struct ieee80211_mgmt, u.disassoc.variable);

	event.disassoc_info.addr = mgmt->sa;
	event.disassoc_info.reason_code = mgmt->u.deauth.reason_code;
	event.disassoc_info.ie_len = len - ie_offset;
	event.disassoc_info.ie = (uint8_t *) mgmt + ie_offset;

	wpa_supplicant_event(intf->wpa_supp_ctx, EVENT_DISASSOC, &event);
}

static void nrc_mgmt_assoc_sta_rx(struct nrc_wpa_if* intf,
				RXINFO *rxi, struct ieee80211_mgmt *mgmt,
				uint16_t len)
{
	union wpa_event_data event;
	int ie_offset = offsetof(struct ieee80211_mgmt, u.assoc_resp.variable);
	uint8_t *ie = (uint8_t *) mgmt + ie_offset;
	struct ieee802_11_elems elems;
	uint16_t ies_len = len - ie_offset;
	int status = mgmt->u.assoc_resp.status_code;

	os_memset(&event, 0, sizeof(event));

	if (status == WLAN_STATUS_SUCCESS) {
		intf->associated = true;
		event.assoc_info.reassoc = (WLAN_FC_GET_STYPE(mgmt->frame_control) ==
					WLAN_FC_STYPE_REASSOC_RESP);
		event.assoc_info.resp_ies_len = ies_len;
		event.assoc_info.resp_ies = ie;
		/* TODO: Country code is required? */
		event.assoc_info.freq = ieee80211_chan_to_freq(/* COUNTRY CODE */ NULL,
				81, rxi->center_freq);
		event.assoc_info.authorized = 0;
		/* 802.11-2016 9.4.1.8 */
		intf->sta.aid = (mgmt->u.assoc_resp.aid & 0x3FFF);
		wpa_supplicant_event(intf->wpa_supp_ctx, EVENT_ASSOC, &event);
		os_memcpy(intf->sta.addr, mgmt->sa, ETH_ALEN);
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
		wpa_driver_sta_sta_add(intf);
#if defined(NRC_USER_APP)
		wpa_driver_notify_event_to_app(EVENT_ASSOC);
#endif
	} else {
		intf->associated = false;
		os_memset(&intf->bss, 0, sizeof(intf->bss));
		event.assoc_reject.bssid = mgmt->sa;
		event.assoc_reject.resp_ies_len = ies_len;
		event.assoc_reject.resp_ies = ie;
		event.assoc_reject.status_code = status;
		event.assoc_reject.timed_out = 0;
		wpa_supplicant_event(intf->wpa_supp_ctx, EVENT_ASSOC_REJECT, &event);
#if defined(NRC_USER_APP)
		wpa_driver_notify_event_to_app(EVENT_ASSOC_REJECT);
#endif
	}
}

#if defined(INCLUDE_TWT_SUPPORT)
static void nrc_twt_setup_rx(struct nrc_wpa_if* intf,
				struct ieee80211_mgmt* mgmt)
{
	extern SYS_BUF * alloc_sys_buf_try(int hif_len, int nTry);

	uint16_t hif_len;
	SYS_BUF* packet;
	const int nTry = 10;

	/* disregarding qos/payload field in GenericMacHeader */
	hif_len = sizeof(FRAME_HDR) + sizeof(GenericMacHeader)
		- 2 + sizeof(TWT_Setup);
	packet = alloc_sys_buf_try(hif_len, nTry);
	if(!packet){
		return;
	}

	HIF_HDR(packet).vifindex = intf->vif_id;
	HIF_HDR(packet).len = hif_len;
	HIF_HDR(packet).tlv_len = 0;

	FRAME_HDR(packet).flags.tx.ac = ACI_VO;

	memset(&TX_MAC_HDR(packet), 0, sizeof(GenericMacHeader));

	TX_MAC_HDR(packet).version = FC_PV0;
	TX_MAC_HDR(packet).type = FC_PV0_TYPE_MGMT;
	TX_MAC_HDR(packet).subtype = FC_PV0_TYPE_MGMT_ACTION_NOACK;

	memcpy(TX_MAC_HDR(packet).address1, mgmt->sa, MAC_ADDR_LEN);
	memcpy(TX_MAC_HDR(packet).address2, mgmt->da, MAC_ADDR_LEN);
	memcpy(TX_MAC_HDR(packet).address3, mgmt->bssid, MAC_ADDR_LEN);

	/* ToDo
	* Extract fields info from TWT setup action frame
	*/

	memset(&TX_MAC_HDR(packet).payload, 0, sizeof(TWT_Info));

	TWT_Info *twt_info 	= (TWT_Info*) TX_MAC_HDR(packet).payload;
	twt_info->category		= WLAN_ACTION_S1G;
	twt_info->action		= WLAN_ACTION_S1G_TWT_INFO;

	/* ToDo
	* Insert fields info from TWT setup action frame
	*/

	lmac_uplink_request_sysbuf(packet);

}

static void nrc_twt_teardown_rx(struct nrc_wpa_if* intf,
				struct ieee80211_mgmt* mgmt)
{
	extern SYS_BUF * alloc_sys_buf_try(int hif_len, int nTry);

	uint16_t hif_len;
	SYS_BUF* packet;
	const int nTry = 10;

	/* disregarding qos/payload field in GenericMacHeader */
	hif_len = sizeof(FRAME_HDR) + sizeof(GenericMacHeader)
		- 2 + sizeof(TWT_Teardown);
	packet = alloc_sys_buf_try(hif_len, nTry);
	if(!packet){
		return;
	}

	HIF_HDR(packet).vifindex = intf->vif_id;
	HIF_HDR(packet).len = hif_len;
	HIF_HDR(packet).tlv_len = 0;

	FRAME_HDR(packet).flags.tx.ac = ACI_VO;

	memset(&TX_MAC_HDR(packet), 0, sizeof(GenericMacHeader));

	TX_MAC_HDR(packet).version = FC_PV0;
	TX_MAC_HDR(packet).type = FC_PV0_TYPE_MGMT;
	TX_MAC_HDR(packet).subtype = FC_PV0_TYPE_MGMT_ACTION_NOACK;

	memcpy(TX_MAC_HDR(packet).address1, mgmt->sa, MAC_ADDR_LEN);
	memcpy(TX_MAC_HDR(packet).address2, mgmt->da, MAC_ADDR_LEN);
	memcpy(TX_MAC_HDR(packet).address3, mgmt->bssid, MAC_ADDR_LEN);

	/* ToDo
	* Extract fields info from TWT teardown action frame
	*/

	memset(&TX_MAC_HDR(packet).payload, 0, sizeof(TWT_Info));

	TWT_Info *twt_info 	= (TWT_Info*) TX_MAC_HDR(packet).payload;
	twt_info->category		= WLAN_ACTION_S1G;
	twt_info->action		= WLAN_ACTION_S1G_TWT_INFO;

	/* ToDo
	* Insert fields info from TWT teardown action frame
	*/

	lmac_uplink_request_sysbuf(packet);
}

static void nrc_twt_info_rx(struct nrc_wpa_if* intf,
				struct ieee80211_mgmt* mgmt)
{
	/* ToDo
	* Extract fields info from TWT Informaiotn action frame
	*/
	//TWT_Info *twt_info 	= (TWT_Info*) mgmt->u.action.u.s1g_action.u.twt_info;

}
#endif /* defined(INCLUDE_TWT_SUPPORT) */

static void nrc_addba_req_rx(struct nrc_wpa_if* intf,
				struct ieee80211_mgmt* mgmt)
{
	extern SYS_BUF * alloc_sys_buf_try(int hif_len, int nTry);

	int i, tid, ac;
	uint16_t hif_len, mpdu_len, max_buf, timeout, dial_token;
	SYS_BUF* packet;
	const int nTry = 10;

	/* disregarding qos/payload field in GenericMacHeader */
	hif_len = sizeof(FRAME_HDR) + sizeof(GenericMacHeader) - 2 + sizeof(ADDBA_Resp);
	packet = alloc_sys_buf_try(hif_len, nTry);
	if(!packet){
		return;
	}

	HIF_HDR(packet).vifindex				= intf->vif_id;
	HIF_HDR(packet).len						= hif_len;
	HIF_HDR(packet).tlv_len					= 0;

	FRAME_HDR(packet).flags.tx.ac			= ACI_VO;

	memset( &TX_MAC_HDR(packet) , 0 , sizeof(GenericMacHeader) );

	TX_MAC_HDR(packet).version 				= FC_PV0;
	TX_MAC_HDR(packet).type					= FC_PV0_TYPE_MGMT;
	TX_MAC_HDR(packet).subtype 				= FC_PV0_TYPE_MGMT_ACTION;

	memcpy( TX_MAC_HDR(packet).address1 , mgmt->sa , MAC_ADDR_LEN);
	memcpy( TX_MAC_HDR(packet).address2 , mgmt->da , MAC_ADDR_LEN);
	memcpy( TX_MAC_HDR(packet).address3 , mgmt->bssid , MAC_ADDR_LEN);

	/* fields info from addba request action frame */
	tid = mgmt->u.action.u.ba_action.u.addba_req_action.tid;
	ac = TID_TO_AC(tid);
	max_buf = mgmt->u.action.u.ba_action.u.addba_req_action.max_buf;
	timeout = mgmt->u.action.u.ba_action.u.addba_req_action.timeout;
	dial_token = mgmt->u.action.u.ba_action.u.addba_req_action.dial_token;

	memset( &TX_MAC_HDR(packet).payload , 0 , sizeof(ADDBA_Resp) );

	ADDBA_Resp *addba_resp 	= (ADDBA_Resp*) TX_MAC_HDR(packet).payload;
	addba_resp->category 	= WLAN_ACTION_BLOCK_ACK;
	addba_resp->ba_action 	= WLAN_BA_ADDBA_RESPONSE;
	addba_resp->dial_token 	= dial_token;
	addba_resp->policy 		= WLAN_BA_POLICY_IMM_BA;
	addba_resp->max_buf 	= (max_buf <= WLAN_BA_MAX_BUF) ? max_buf : WLAN_BA_MAX_BUF;
	addba_resp->tid 		= tid;

	struct nrc_wpa_sta* sta = nrc_wpa_find_sta(intf, mgmt->sa);
	if (sta->block_ack[tid] != BLOCK_ACK_RX){
		sta->block_ack[tid] = BLOCK_ACK_RX;
		addba_resp->status = WLAN_STATUS_SUCCESS;
		I(TT_WPAS, TAG "BA_RX_START:"MACSTR",TID:%d,AC:%d",MAC2STR(mgmt->sa),tid,ac);
	} else {
		addba_resp->status = WLAN_STATUS_REQUEST_DECLINED;
		I(TT_WPAS, TAG "BA_RX_DECLINED:"MACSTR",TID:%d,AC:%d",MAC2STR(mgmt->sa),tid,ac);
	}

	lmac_uplink_request_sysbuf(packet);
}

static void nrc_addba_resp_rx(struct nrc_wpa_if* intf,
					struct ieee80211_mgmt* mgmt)
{
	uint16_t status = mgmt->u.action.u.ba_action.u.addba_resp_action.status;
	int tid = mgmt->u.action.u.ba_action.u.addba_resp_action.tid;
	int ac = TID_TO_AC(tid);

	if (status != WLAN_STATUS_SUCCESS) {
		I(TT_WPAS, TAG "BA_TX_DECLINED:"MACSTR",TID:%d,AC:%d",MAC2STR(mgmt->sa),tid,ac);
		return;
	}

	struct nrc_wpa_sta* sta = nrc_wpa_find_sta(intf, mgmt->sa);
	if (!sta)
		return;
	sta->block_ack[tid] = BLOCK_ACK_TX;

	I(TT_WPAS, TAG "BA_TX_START:"MACSTR",TID:%d,AC:%d",MAC2STR(mgmt->sa),tid,ac);
}

static void nrc_delba_req_rx(struct nrc_wpa_if* intf,
				struct ieee80211_mgmt* mgmt)
{
	int tid  = mgmt->u.action.u.ba_action.u.delba_req_action.tid;
	int ac = TID_TO_AC(tid);

	struct nrc_wpa_sta* sta = nrc_wpa_find_sta(intf, mgmt->sa);
	if (!sta)
		return;
	sta->block_ack[tid] = BLOCK_ACK_INVALID;

	I(TT_WPAS, TAG "BA_RX_STOP:"MACSTR",TID:%d,AC:%d",MAC2STR(mgmt->sa),tid,ac);
}

static void nrc_mgmt_action_blockack_rx(struct nrc_wpa_if* intf,
				struct ieee80211_mgmt* mgmt)
{
	switch (mgmt->u.action.u.ba_action.action) {
		case WLAN_BA_ADDBA_REQUEST:
		nrc_addba_req_rx(intf, mgmt);
		break;
		case WLAN_BA_ADDBA_RESPONSE:
		nrc_addba_resp_rx(intf, mgmt);
		break;
		case WLAN_BA_DELBA:
		nrc_delba_req_rx(intf, mgmt);
		break;
	}
}

static void nrc_mgmt_action_rx(struct ieee80211_mgmt* mgmt,
				struct nrc_wpa_if* intf)
{
	switch (mgmt->u.action.category) {
		case WLAN_ACTION_BLOCK_ACK:
		nrc_mgmt_action_blockack_rx(intf, mgmt);
		break;
#if 0
#if defined(INCLUDE_TWT_SUPPORT)
		case WLAN_ACTION_BLOCK_ACK:
		nrc_mgmt_action_twt_rx(intf, mgmt);
		break;
#endif /* defined(INCLUDE_TWT_SUPPORT) */
#endif
	}
}

static void _nrc_mgmt_sta_rx(uint16_t subtype, struct nrc_wpa_rx_data *rx)
{
	switch (subtype) {
		case WLAN_FC_STYPE_AUTH:
		nrc_mgmt_auth_sta_rx(rx->intf, rx->u.mgmt, rx->len);
		break;
		case WLAN_FC_STYPE_REASSOC_RESP:
		case WLAN_FC_STYPE_ASSOC_RESP:
		nrc_mgmt_assoc_sta_rx(rx->intf, rx->rxi, rx->u.mgmt, rx->len);
		break;
		case WLAN_FC_STYPE_DEAUTH:
		nrc_mgmt_deauth_sta_rx(rx->intf, rx->u.mgmt, rx->len);
		break;
		case WLAN_FC_STYPE_DISASSOC:
		nrc_mgmt_disassoc_sta_rx(rx->intf, rx->u.mgmt, rx->len);
		break;
		case WLAN_FC_STYPE_ACTION:
		nrc_mgmt_action_rx(rx->u.mgmt, rx->intf);
		break;

	}
}

static void run_vendor_func(uint8_t cmd, uint8_t* data, int data_len)
{
	wpa_driver_notify_vevent_to_app(cmd, data, data_len);
}

static bool check_vendor_ie(uint8_t *ies, uint16_t ies_len)
{
	ie_general *ie = (void *) ies;
	const int OUI_LEN = 3;

	while(ie && ies_len >= ie->length + 2) {
		if (ie->eid == WLAN_EID_VENDOR_SPECIFIC
			&& ies_len > OUI_LEN
			&& WPA_GET_BE24(ies + 2) == OUI_NRC) {
			int offset = 2 + OUI_LEN;
			uint8_t cmd = *(ies + offset);
			offset++;
			run_vendor_func(cmd, ies + offset, ie->length - (OUI_LEN + 1));
		}

		ies += ie->length + 2;
		ies_len  -= ie->length + 2;
		ie = (void *) ies;
	}

	return NULL;
}

static void sta_rx_beacon(struct nrc_wpa_rx_data *rx)
{
	uint8_t *ies = rx->u.mgmt->u.beacon.variable;
	int ies_len = rx->len - (ies - rx->u.frame);

	if (ies_len < sizeof(struct ieee80211_mgmt))
		return;

	check_vendor_ie(ies, ies_len);
}

static void nrc_wpa_mgmt_sta_rx(uint16_t subtype, struct nrc_wpa_rx_data *rx)
{
	//TODO: Ignore beacon it is too verbose.
	if (subtype == WLAN_FC_STYPE_BEACON) {
		sta_rx_beacon(rx);
	}
	else if (subtype == WLAN_FC_STYPE_PROBE_RESP) {
		if (rx->intf->associated) {
			struct ieee80211_hdr *hdr = rx->u.hdr;
			if (os_memcmp(hdr->addr2, rx->intf->bss.bssid, ETH_ALEN) == 0)
				os_get_time(&rx->intf->bss.last_beacon_update);
#if defined(NRC_USER_APP)||defined(CONFIG_BG_SCAN)
			if (subtype == WLAN_FC_STYPE_PROBE_RESP)
			   nrc_wpa_scan_sta_rx(rx);
#endif
		} else {
			nrc_wpa_scan_sta_rx(rx);
		}
	}
	else
		_nrc_mgmt_sta_rx(subtype, rx);
}

static void wpa_driver_event_rx(void *eloop_data, void *user_ctx)
{
	struct nrc_wpa_rx_event *rxe = (struct nrc_wpa_rx_event *) user_ctx;
	union wpa_event_data event;
	struct os_time time;
	os_memset(&event, 0, sizeof(event));

	event.rx_mgmt.frame = rxe->frame;
	event.rx_mgmt.frame_len = rxe->len;
	event.rx_mgmt.freq = rxe->freq;
	wpa_supplicant_event(rxe->pv, EVENT_RX_MGMT, &event);

	os_free(rxe->frame);
	os_free(rxe);
}

static void nrc_wpa_mgmt_ap_rx(uint16_t subtype, struct nrc_wpa_rx_data *rx)
{
	struct nrc_wpa_rx_event *rxe = NULL;
	struct nrc_wpa_sta* sta = NULL;

	switch (subtype) {
		case WLAN_FC_STYPE_BEACON:
		case WLAN_FC_STYPE_PROBE_RESP:
		case WLAN_FC_STYPE_PROBE_REQ:
		// Drop
		return;
		case WLAN_FC_STYPE_DEAUTH:
		sta = nrc_wpa_find_sta(rx->intf, rx->u.mgmt->sa);
		if (!sta)
			return;
		break;
		case WLAN_FC_STYPE_ACTION:
		nrc_mgmt_action_rx(rx->u.mgmt, rx->intf);
		return;
	}

	rxe = os_malloc(sizeof(*rxe));

	if (!rxe) {
		E(TT_WPAS, TAG  "failed to allocate rx_event");
		return;
	}
	rxe->frame = os_malloc(rx->len);

	if (!rxe->frame) {
		E(TT_WPAS, TAG  "failed to allocate rx_event frame");
		os_free(rxe);
		return;
	}
	os_memcpy(rxe->frame, rx->u.frame, rx->len);
	rxe->len = rx->len;
	rxe->pv = rx->intf->wpa_supp_ctx;
	rxe->freq = ieee80211_chan_to_freq(NULL, 81, rx->rxi->center_freq);
	rxe->pv = rx->intf->wpa_supp_ctx;
	rxe->ref_time = rx->ref_time;
	rxe->subtype = subtype;

	eloop_register_timeout(0, 0, wpa_driver_event_rx, 0, rxe);
}

static void nrc_wpa_mgmt_rx(uint16_t subtype, struct nrc_wpa_rx_data *rx)
{
	rx->ref_time = NOW;
	if (!rx->intf->is_ap)
		nrc_wpa_mgmt_sta_rx(subtype, rx);
	else
		nrc_wpa_mgmt_ap_rx(subtype, rx);
}

static int nrc_wpa_michael_mic_verify(struct nrc_wpa_rx_data *rx)
{
	union wpa_event_data event;
	struct nrc_wpa_key *key;

	if(!rx->rxi->error_mic)
		return 0;

	key = nrc_wpa_get_key(rx->intf, rx->u.hdr->addr2);

	if (!key)
		return 0;

	if (key->cipher != WIM_CIPHER_TYPE_TKIP)
		return 0;

	os_memset(&event, 0, sizeof(event));
	if (!is_broadcast_ether_addr(rx->u.hdr->addr3)) {
		event.michael_mic_failure.unicast = 1;
		event.michael_mic_failure.src = rx->u.hdr->addr2;
	}
	wpa_supplicant_event(rx->intf->wpa_supp_ctx, EVENT_MICHAEL_MIC_FAILURE,
					&event);

	return -1;
}

static int nrc_wpa_qos_null(struct nrc_wpa_rx_data *rx)
{
	if (WLAN_FC_GET_STYPE(rx->u.hdr->frame_control)
					== WLAN_FC_STYPE_QOS_NULL) {
		V(TT_WPAS, TAG "Drop QoS Null frame\n");
		return -1;
	}
	return 0;
}

static int nrc_wpa_eapol(struct nrc_wpa_rx_data *rx)
{
	if (rx->is_8023) {
		extern int wpas_l2_packet_filter(uint8_t *buffer, int len);
		if (wpas_l2_packet_filter(rx->u.frame + rx->offset_8023, rx->len))
			return -1;
	}
	return 0;
}

static int nrc_wpa_lwip(struct nrc_wpa_rx_data *rx)
{
	uint8_t vif_id = rx->intf->vif_id;

	if (!rx->is_8023)
		return -1;

	lwif_input(vif_id, rx->u.frame + rx->offset_8023, rx->len);

	return 0; /* Continue RX Path */
}

static int nrc_wpa_ap_process(struct nrc_wpa_rx_data *rx)
{
	struct ieee80211_hdr *hdr = rx->u.hdr;
	struct ieee80211_hdr forward_hdr = {0, };
	uint16_t fc = hdr->frame_control;
	struct nrc_wpa_sta *sta = NULL;
	uint8_t saddr[ETH_ALEN];
	int ret = 0, i = 0;

	if (!rx->intf->is_ap)
		return 0;

	if (!(fc & WLAN_FC_TODS))
		return -1;

	if ((fc & WLAN_FC_TODS) && (fc & WLAN_FC_FROMDS)) {
		E(TT_WPAS, TAG "Drop WDS frame\n");
		return -1;
	}

	// If DA is Soft AP, the frame is consumed (return 0)
	// If DA is another STA connected to Soft AP, the frame is forwared
	// If DA is multicast, the frame is consumed and forwared. (return 0)
	if (os_memcmp(hdr->addr3, rx->intf->sta.addr, ETH_ALEN) == 0)
		return 0;

	if (is_multicast_ether_addr(hdr->addr3)) {
		ret = 0;
	} else {
		// TODO: If the number of connected STs is just one,
		//  there is no point in forwarding the frame.
		sta = nrc_wpa_find_sta(rx->intf, hdr->addr3);

		if (!sta) {
			E(TT_WPAS, TAG "Unable forward frames to STA(" MACSTR "), "
						"(Failed to find sta_info)",
						MAC2STR(hdr->addr3));

		 	ret = -1;
		}
	}
#if 1
	os_memcpy(&forward_hdr, hdr, sizeof(forward_hdr));
	hdr->frame_control &= ~WLAN_FC_TODS;
	hdr->frame_control |= WLAN_FC_FROMDS;
	os_memcpy(saddr, hdr->addr2, ETH_ALEN);
	os_memcpy(hdr->addr2, hdr->addr1, ETH_ALEN);
	os_memcpy(hdr->addr1, hdr->addr3, ETH_ALEN);
	os_memcpy(hdr->addr3, saddr, ETH_ALEN);
	nrc_raw_transmit(rx->intf, (void *) rx->u.frame, rx->len, ACI_BE);

	if (ret == 0)
		os_memcpy(hdr, &forward_hdr, sizeof(forward_hdr));
#endif
	return ret;
}

static int nrc_wpa_to_8023(struct nrc_wpa_rx_data *rx)
{
/* convert IEEE 802.11 header + possible LLC headers into Ethernet
* header
* IEEE 802.11 address fields:
* ToDS FromDS Addr1 Addr2 Addr3 Addr4
*   0     0   DA    SA    BSSID n/a
*   0     1   DA    BSSID SA    n/a
*   1     0   BSSID SA    DA    n/a
*   1     1   RA    TA    DA    SA
*/
	struct ieee8023_hdr ehdr;
	uint16_t fc = rx->u.hdr->frame_control;

	os_memset(&ehdr, 0, sizeof(struct ieee8023_hdr));
	uint8_t *payload = (uint8_t *) (rx->u.hdr + 1);

	const uint8_t rfc1042_header[] = { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00 };
	const uint8_t bridge_tunnel_header[] = { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0xf8 };

	int to_ds = (rx->u.hdr->frame_control & WLAN_FC_TODS) ? 1 : 0;
	int from_ds = (rx->u.hdr->frame_control & WLAN_FC_FROMDS) ? 1 : 0;

	if (rx->u.hdr->frame_control & (WLAN_FC_STYPE_QOS_DATA << 4))
		payload += 2;

	if (rx->u.hdr->frame_control & WLAN_FC_ISWEP) {
		struct nrc_wpa_key *wpa_key = nrc_wpa_get_key(rx->intf, rx->u.hdr->addr2);
		payload += nrc_get_sec_hdr_len(wpa_key);
	}

	if (to_ds && from_ds) {
		E(TT_WPAS, TAG "Unsupported conversion to 802.3");
		return -1;
	} else if (to_ds && !from_ds && !rx->intf->is_ap) {
		E(TT_WPAS, TAG "Unsupported conversion to 802.3 (not AP)");
		return -1;
	}

	if (to_ds) {
		os_memcpy(ehdr.dest, rx->u.hdr->addr3, ETH_ALEN);
		os_memcpy(ehdr.src, rx->u.hdr->addr2, ETH_ALEN);
	} else {
		os_memcpy(ehdr.dest, rx->u.hdr->addr1, ETH_ALEN);
		os_memcpy(ehdr.src, rx->u.hdr->addr3, ETH_ALEN);
	}

	//ehdr.ethertype = (payload[6] << 8) | payload[7];
	os_memcpy(&ehdr.ethertype, payload + sizeof(rfc1042_header), 2);
	rx->offset_8023 = payload - rx->u.frame - HLEN_8023;

	if ((os_memcmp(payload, rfc1042_header, 6) == 0 &&
		(htons(ehdr.ethertype) != ETH_P_AARP || htons(ehdr.ethertype) != ETH_P_IPX))
		|| os_memcmp(payload, bridge_tunnel_header, 6) == 0)
		rx->offset_8023 += 8; ///* remove RFC1042 or Bridge-Tunnel encapsulation
	else
		os_memset(&ehdr.ethertype, 0, 2);

	rx->len -= rx->offset_8023;
	os_memcpy(rx->u.frame + rx->offset_8023, &ehdr, HLEN_8023);
	rx->is_8023 = true;

	return 0;
}

static void nrc_wpa_data_rx(struct nrc_wpa_rx_data *rx)
{
	int res;
#define CALL_RXH(rxh)		\
	do {					\
		res = rxh(rx);		\
		if (res != 0)		\
		do {				\
			goto drop;		\
		} while(0);			\
	} while (0)
	CALL_RXH(nrc_wpa_qos_null);
	CALL_RXH(nrc_wpa_michael_mic_verify);
	CALL_RXH(nrc_wpa_ap_process);
	CALL_RXH(nrc_wpa_to_8023);
	CALL_RXH(nrc_wpa_eapol);
	CALL_RXH(nrc_wpa_lwip);
drop:
	return;
#undef CALL_RXH
}

#if defined (NRC7291_SDK_DUAL_CM0) || defined (NRC7291_SDK_DUAL_CM3)
//TDDO.
int system_memory_pool_number_of_link(struct _SYS_BUF* buf) {return 0;};
void discard(SYS_BUF* buffer) {};
#endif

void nrc_mac_rx(int vif_id, SYS_BUF *head)
{
	struct nrc_wpa_rx_data rx = {
		.rxh = &head->lmac_rxhdr,
		.rxi = &head->lmac_rxhdr.rxinfo,
		.len = rx.rxi->mpdu_length,
		.buf = head,
	};
	GenericMacHeader *gmh = &head->rx_mac_header;
	int i = 0;
	bool need_free = false;

	if (rx.len < IEEE80211_HDRLEN)
		goto drop;

	if (system_memory_pool_number_of_link(head) > 1) {
		need_free = true;
		rx.u.frame = os_malloc(rx.len);
		if (!rx.u.frame) {
			ASSERT(0);
			goto drop;
		}
		linearize(head, rx.u.frame, P_OFFSET, 0, rx.len);
	} else {
		rx.u.frame = (uint8_t *) head + P_OFFSET;
	}

	for (i = 0; i < VIF_MAX; i++) {
		if (vif_id != VIF_BROADCAST && i != vif_id)
			continue;

		rx.intf = wpa_driver_get_interface(i);
		if (gmh->type == WLAN_FC_TYPE_DATA)
			nrc_wpa_data_rx(&rx);
		else if (gmh->type == WLAN_FC_TYPE_MGMT)
			nrc_wpa_mgmt_rx(gmh->subtype, &rx);
	}

drop:
	if (need_free && rx.u.frame)
		os_free(rx.u.frame);

	discard(head);
}

#ifdef TAG
#undef TAG
#endif
