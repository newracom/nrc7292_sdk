#include "system_common.h"
#include "system_modem_api.h"

#include "utils/includes.h"
#include "utils/common.h"
#include "../common/wpa_common.h"
#include "utils/eloop.h"
#include "utils/wpa_debug.h"
#include "driver.h"
#include "driver_nrc_scan.h"
#include "nrc-wim-types.h"
#include "umac_wim_builder.h"
#include "../common/ieee802_11_common.h"
#include "../../wpa_supplicant/config_ssid.h"
#include "driver_nrc.h"
#include "driver_nrc_scan.h"
#include "driver_nrc_debug.h"
#include "standalone.h"
#include <ap/sta_info.h>
#include "../../lwip/port/lwip.h"

#define DEF_CFG_S1G_SHORT_BEACON_COUNT	(10)

#ifdef TAG
#undef TAG
#endif
#define TAG "wpa: "

#if defined(INCLUDE_BD_SUPPORT)
enum {
	BOARD_CC_US=1,
	BOARD_CC_JP,
	BOARD_CC_KR,
	BOARD_CC_TW,
	BOARD_CC_EU,
	BOARD_CC_CN,
	BOARD_CC_MAX,
}BOARD_CC_TYPE;
#endif

static struct nrc_wpa *nrc_global;

struct nrc_wpa_if *wpa_driver_get_interface(int vif)
{
	if (nrc_global)
		return nrc_global->intf[vif];
	return NULL;
}

static void wpa_clear_key(struct nrc_wpa_key *key)
{
	os_memset(key, 0, sizeof(struct nrc_wpa_key));
	key->cipher = WIM_CIPHER_TYPE_NONE;
}

enum wim_cipher_type nrc_to_wim_cipher_type(enum wpa_alg alg, int key_len)
{
	switch (alg) {
		case WPA_ALG_NONE:
		return WIM_CIPHER_TYPE_NONE;
		case WPA_ALG_WEP:
		return key_len == 5 ? WIM_CIPHER_TYPE_WEP40 : WIM_CIPHER_TYPE_WEP104;
		case WPA_ALG_TKIP:
		return WIM_CIPHER_TYPE_TKIP;
		case WPA_ALG_CCMP:
		return WIM_CIPHER_TYPE_CCMP;
		default:
		return WIM_CIPHER_TYPE_INVALID;
	}
}

static int wpa_driver_nrc_set_key_wim(struct nrc_wpa_if *intf,
			int cipher, const u8 *addr,
			int key_idx, const u8 *key, size_t key_len, bool is_set)
{
	struct wim_builder *wb = NULL;
	struct wim_key_param param;
	struct nrc_wpa_sta *wpa_sta = NULL;
	int key_cmd = WIM_CMD_SET_KEY;
	uint16_t aid = 0;

	I(TT_WPAS, TAG "%d %s cipher:%d, is_set:%s, key_len:%d\n",
			intf->vif_id, __func__, cipher, is_set?"SET":"DISABLE", key_len);

	if (!is_set)
		key_cmd = WIM_CMD_DISABLE_KEY;

	wb = wim_builder_create(intf->vif_id, key_cmd, WB_MIN_SIZE);

	param.cipher_type 	= cipher;
	param.key_index 	= key_idx;

	if (addr)
		os_memcpy(param.mac_addr, addr, ETH_ALEN);
	else
		os_memset(param.mac_addr, 0x0, ETH_ALEN);

	if (intf->is_ap && is_broadcast_ether_addr(addr)) {
		I(TT_WPAS, TAG "%d %s AP: change BR MAC into MAC: "MACSTR"\n",
				intf->vif_id, __func__, MAC2STR(intf->addr));
		os_memcpy(param.mac_addr, intf->addr, ETH_ALEN);
	}

	wpa_sta = nrc_wpa_find_sta(intf, addr);
	if(!wpa_sta)
		param.aid = 0;
	else
		param.aid = wpa_sta->aid;

	if(key_idx == 0)
		param.key_flags = WIM_KEY_FLAG_PAIRWISE;
	else
		param.key_flags = WIM_KEY_FLAG_GROUP;

	param.key_len 		= key_len;
	os_memcpy(param.key, key, key_len);

	wim_builder_append(wb, WIM_TLV_KEY_PARAM, &param, sizeof(param));
	wim_builder_run_wm_wq(wb, true);

	return 0;
}

static int wpa_driver_nrc_remove_key_wim(struct nrc_wpa_if *intf, struct nrc_wpa_key *k)
{
	I(TT_WPAS, TAG "%d %s\n", intf->vif_id, __func__);
	k->is_set[k->ix] = false;
	return wpa_driver_nrc_set_key_wim(intf, k->cipher, k->addr, k->ix,
			k->key, k->key_len, k->is_set[k->ix]);
}

static int wpa_driver_nrc_set_key_wim2(struct nrc_wpa_if *intf, struct nrc_wpa_key *k)
{
	I(TT_WPAS, TAG "%d %s\n", intf->vif_id, __func__);

	k->is_set[k->ix] = true;
	if (wpa_driver_nrc_set_key_wim(intf, k->cipher, k->addr, k->ix,
			k->key, k->key_len, k->is_set[k->ix]) == 0)
		return 0;
	else
		k->is_set[k->ix] = false;

	return -1;
}

static void wpa_driver_sta_type_wim(struct nrc_wpa_if *intf, uint32_t sta_type)
{
	struct wim_builder *wb = wim_builder_create(intf->vif_id, WIM_CMD_SET, WB_MIN_SIZE);
	intf->sta_type = sta_type;
	wim_builder_append_u32(wb, WIM_TLV_STA_TYPE, intf->sta_type);
	wim_builder_run_wm_wq(wb, true);
}

struct nrc_wpa_sta* nrc_wpa_find_sta(struct nrc_wpa_if *intf,
						 const uint8_t addr[ETH_ALEN])
{
	int i = 0;
	struct nrc_wpa_sta *sta = NULL;

	if (!intf->is_ap || is_multicast_ether_addr(addr))
		return &intf->sta;

	for (i = 0; i < NRC_WPA_SOFTAP_MAX_STA; i++) {
		if (!intf->ap_sta[i])
			continue;

		if (os_memcmp(intf->ap_sta[i]->addr, addr, ETH_ALEN) == 0)
			return intf->ap_sta[i];
	}

	return NULL;
}

static struct nrc_wpa_sta* nrc_wpa_add_sta(struct nrc_wpa_if* intf,
				const uint8_t *addr, const uint16_t aid)
{
	int i = 0;
	struct nrc_wpa_sta** sta;
	struct nrc_wpa_key* bkey = &intf->bss.broadcast_key;

	for (i = 0; i < NRC_WPA_SOFTAP_MAX_STA; i++) {
		if (intf->ap_sta[i])
			continue;
		intf->ap_sta[i] = os_zalloc(sizeof(struct nrc_wpa_sta));
		os_memcpy(intf->ap_sta[i]->addr, addr, ETH_ALEN);
		intf->ap_sta[i]->aid = aid;

		//if (p->flags & WPA_STA_WMM)
		intf->ap_sta[i]->qos = true;

		wpa_clear_key(&intf->ap_sta[i]->key);

		if (is_key_wep(bkey)) {
			bkey->is_set[bkey->ix] = true;
			wpa_driver_nrc_set_key_wim(intf, bkey->cipher,
				addr, bkey->ix, bkey->key, bkey->key_len, bkey->is_set[bkey->ix]);
		}

		intf->num_ap_sta++;
		return intf->ap_sta[i];
	}

	return NULL;
}

static void wpa_driver_scan_done_timeout(void *eloop_data, void *user_ctx)
{
	union wpa_event_data event;
	int vif = (int) user_ctx;
	struct nrc_wpa_if *intf = wpa_driver_get_interface(vif);
	I(TT_WPAS, TAG "%d scan finished \n", vif);
	scan_stop(intf->scan);

	os_memset(&event, 0, sizeof(event));
	event.scan_info.aborted = false;
	event.scan_info.nl_scan_event = 1; /* From normal scan */
	wpa_supplicant_event(intf->wpa_supp_ctx, EVENT_SCAN_RESULTS, &event);
}

void nrc_scan_done(int vif)
{
	eloop_register_timeout(0, 0, wpa_driver_scan_done_timeout, 0, (void *) vif);
}

/**
 * get_bssid - Get the current BSSID
 * @priv: private driver interface data
 * @bssid: buffer for BSSID (ETH_ALEN = 6 bytes)
 *
 * Returns: 0 on success, -1 on failure
 *
 * Query kernel driver for the current BSSID and copy it to bssid.
 * Setting bssid to 00:00:00:00:00:00 is recommended if the STA is not
 * associated.
 */
static int wpa_driver_nrc_get_bssid(void *priv, u8 *bssid)
{
	struct nrc_wpa_if *intf = (struct nrc_wpa_if *)(priv);

	if (!intf->associated) {
		os_memset(bssid, 0, ETH_ALEN);
		return 0;
	}
	os_memcpy(bssid, intf->bss.bssid, ETH_ALEN);

	return 0;
}

int nrc_get_sec_hdr_len(struct nrc_wpa_key *key)
{
	if (!key)
		return 0;

	switch (key->cipher) {
		case WIM_CIPHER_TYPE_WEP40:
		case WIM_CIPHER_TYPE_WEP104:
		return 4;
		case WIM_CIPHER_TYPE_TKIP:
		case WIM_CIPHER_TYPE_CCMP:
		return 8;
	}
	return 0;
}

int nrc_get_channel_list(struct nrc_wpa_if *intf, uint16_t chs[], const int MAX_CHS)
{
	int i = 0;

	uint16_t *sys_chs;
	int sys_n_chs = 0;

	system_api_get_supported_channels(&sys_chs, &sys_n_chs);

	for (i = 0; i < sys_n_chs; i++) {
		chs[i] = sys_chs[i];
	}
	return i;
}

/**
 * get_ssid - Get the current SSID
 * @priv: private driver interface data
 * @ssid: buffer for SSID (at least 32 bytes)
 *
 * Returns: Length of the SSID on success, -1 on failure
 *
 * Query kernel driver for the current SSID and copy it to ssid.
 * Returning zero is recommended if the STA is not associated.
 *
 * Note: SSID is an array of octets, i.e., it is not nul terminated and
 * can, at least in theory, contain control characters (including nul)
 * and as such, should be processed as binary data, not a printable
 * string.
 */
static int wpa_driver_nrc_get_ssid(void *priv, u8 *ssid)
{
	struct nrc_wpa_if *intf = (struct nrc_wpa_if *)(priv);

	if(!intf->associated)
		return -1;

	os_memcpy(ssid, intf->bss.ssid, intf->bss.ssid_len);

	return intf->bss.ssid_len;
}

static int wpa_driver_nrc_get_aid(void *priv, uint16_t *aid)
{
	struct nrc_wpa_if *intf = (struct nrc_wpa_if *)(priv);

	if(!intf->associated)
		return -1;

	*aid = intf->sta.aid;

	return 0;
}

/**
 * set_key - Configure encryption key
 * @ifname: Interface name (for multi-SSID/VLAN support)
 * @priv: private driver interface data
 * @alg: encryption algorithm (%WPA_ALG_NONE, %WPA_ALG_WEP,
 *	%WPA_ALG_TKIP, %WPA_ALG_CCMP, %WPA_ALG_IGTK, %WPA_ALG_PMK,
 *	%WPA_ALG_GCMP, %WPA_ALG_GCMP_256, %WPA_ALG_CCMP_256,
 *	%WPA_ALG_BIP_GMAC_128, %WPA_ALG_BIP_GMAC_256,
 *	%WPA_ALG_BIP_CMAC_256);
 *	%WPA_ALG_NONE clears the key.
 * @addr: Address of the peer STA (BSSID of the current AP when setting
 *	pairwise key in station mode), ff:ff:ff:ff:ff:ff for
 *	broadcast keys, %NULL for default keys that are used both for
 *	broadcast and unicast; when clearing keys, %NULL is used to
 *	indicate that both the broadcast-only and default key of the
 *	specified key index is to be cleared
 * @key_idx: key index (0..3), usually 0 for unicast keys; 0..4095 for
 *	IGTK
 * @set_tx: configure this key as the default Tx key (only used when
 *	driver does not support separate unicast/individual key
 * @seq: sequence number/packet number, seq_len octets, the next
 *	packet number to be used for in replay protection; configured
 *	for Rx keys (in most cases, this is only used with broadcast
 *	keys and set to zero for unicast keys); %NULL if not set
 * @seq_len: length of the seq, depends on the algorithm:
 *	TKIP: 6 octets, CCMP/GCMP: 6 octets, IGTK: 6 octets
 * @key: key buffer; TKIP: 16-byte temporal key, 8-byte Tx Mic key,
 *	8-byte Rx Mic Key
 * @key_len: length of the key buffer in octets (WEP: 5 or 13,
 *	TKIP: 32, CCMP/GCMP: 16, IGTK: 16)
 *
 * Returns: 0 on success, -1 on failure
 *
 * Configure the given key for the kernel driver. If the driver
 * supports separate individual keys (4 default keys + 1 individual),
 * addr can be used to determine whether the key is default or
 * individual. If only 4 keys are supported, the default key with key
 * index 0 is used as the individual key. STA must be configured to use
 * it as the default Tx key (set_tx is set) and accept Rx for all the
 * key indexes. In most cases, WPA uses only key indexes 1 and 2 for
 * broadcast keys, so key index 0 is available for this kind of
 * configuration.
 *
 * Please note that TKIP keys include separate TX and RX MIC keys and
 * some drivers may expect them in different order than wpa_supplicant
 * is using. If the TX/RX keys are swapped, all TKIP encrypted packets
 * will trigger Michael MIC errors. This can be fixed by changing the
 * order of MIC keys by swapping te bytes 16..23 and 24..31 of the key
 * in driver_*.c set_key() implementation, see driver_ndis.c for an
 * example on how this can be done.
 */

struct nrc_wpa_key *nrc_wpa_get_key(struct nrc_wpa_if *intf, const uint8_t *addr)
{
	struct nrc_wpa_sta *wpa_sta = NULL;
	struct nrc_wpa_key *b_key = &intf->bss.broadcast_key;

	if (!addr || is_multicast_ether_addr(addr))
		return b_key;

	if (intf->is_ap && is_key_wep(b_key))
		return b_key;

	if (!intf->is_ap)
		return &intf->sta.key;

	wpa_sta = nrc_wpa_find_sta(intf, addr);

	if (!wpa_sta) {
		E(TT_WPAS, TAG "Failed to find sta " MACSTR " (Key)\n", MAC2STR(addr));
		return NULL;
	}
	return &wpa_sta->key;
}

static void run_wim_set_bssid(int vif_id, uint8_t* bssid)
{
	struct wim_builder *wb = NULL;
	wb = wim_builder_create(vif_id, WIM_CMD_SET, WB_MIN_SIZE);
	wim_builder_append(wb, WIM_TLV_BSSID, bssid, ETH_ALEN);
	wim_builder_run_wm_wq(wb, true);
}

static int wpa_driver_nrc_set_key(const char *ifname, void *priv, enum wpa_alg alg,
	const u8 *addr, int key_idx, int set_tx,
	const u8 *seq, size_t seq_len,
	const u8 *key, size_t key_len)
{
	int ret = 0;
	uint16_t aid = 0;
	struct nrc_wpa_sta *wpa_sta = NULL;
	struct nrc_wpa_if *intf = (struct nrc_wpa_if *)(priv);

	wpa_sta = nrc_wpa_find_sta(intf, addr);
	if(!wpa_sta)
		aid = 0;
	else
		aid = wpa_sta->aid;

	bool is_remove = (alg == WPA_ALG_NONE || (!is_wep(alg) && addr == NULL));
	struct nrc_wpa_key *wk = nrc_wpa_get_key(intf, addr);

	I(TT_WPAS, TAG "%d Set key (", intf->vif_id);
	if (addr)
		I(TT_WPAS, "addr: " MACSTR ", ", MAC2STR(addr));
	else
		I(TT_WPAS, "addr: NULL, ");

	I(TT_WPAS, "alg:%s, idx:%d, set_tx:%d, seq_len:%d, key_len: %d, aid: %d, is_set: %d)\n",
				wpa_driver_alg_str(alg), key_idx, set_tx,
				(int)seq_len, (int)key_len, aid, wk->is_set[key_idx]);
	if (!wk) {
		I(TT_WPAS, "Return! Fail to get key\n");
		return 0;
	}

	if (is_remove && !wk->is_set[key_idx]) {
		I(TT_WPAS, "Return! Remove key but not set before \n");
		return 0;
	}

	/* Skip remove key. key_idx=0 with Null addr */
	if ((key_idx == 0) && (!addr)) {
		I(TT_WPAS, "Return! key_idx(0 => PTK) but no addr \n");
		return 0;
	}

	wk->cipher = nrc_to_wim_cipher_type(alg, key_len);
	wk->ix = key_idx;
	wk->key_len = key_len;
	if (addr)
		os_memcpy(wk->addr, addr, ETH_ALEN);
	else
		os_memset(wk->addr, 0x0, ETH_ALEN);

	os_memcpy(wk->key, key, wk->key_len);

	if (alg == WPA_ALG_WEP) {
		uint8_t bssid[ETH_ALEN];
		intf->sta.key.cipher = intf->bss.broadcast_key.cipher;
		intf->sta.key.ix = intf->bss.broadcast_key.ix;
		wpa_driver_nrc_get_bssid(intf, bssid);
		/* BSSID should be set before set_key */
		run_wim_set_bssid(intf->vif_id, bssid);
		os_memcpy(wk->addr, bssid, ETH_ALEN);
	}

	/* Remove Key */
#if 1
	if (wk->is_set[key_idx]) {
		if (intf->is_ap && is_broadcast_ether_addr(addr)) {
			I(TT_WPAS, "Soft_AP! Remove and Set GTK again!\n");
			wpa_driver_nrc_remove_key_wim(intf, wk);
			return wpa_driver_nrc_set_key_wim2(intf, wk);
		}
		return wpa_driver_nrc_remove_key_wim(intf, wk);
	}
#else
	if (wk->is_set[key_idx]) {
		if(!addr)
			return wpa_driver_nrc_remove_key_wim(intf, wk);
		else
			wpa_driver_nrc_remove_key_wim(intf, wk);
	}
#endif
	/* Set Key */
	return wpa_driver_nrc_set_key_wim2(intf, wk);
}

/**
 * deinit - Deinitialize driver interface
 * @priv: private driver interface data from init()
 *
 * Shut down driver interface and processing of driver events. Free
 * private data buffer if one was allocated in init() handler.
 */
 static void wpa_driver_nrc_deinit(void *priv)
 {
	struct nrc_wpa_if *intf = (struct nrc_wpa_if *)(priv);
	scan_deinit(intf->scan);
 }

/**
 * set_param - Set driver configuration parameters
 * @priv: private driver interface data from init()
 * @param: driver specific configuration parameters
 *
 * Returns: 0 on success, -1 on failure
 *
 * Optional handler for notifying driver interface about configuration
 * parameters (driver_param).
 */
 static int wpa_driver_nrc_set_param(void *priv, const char *param)
 {
	return 0;
 }

/**
* set_countermeasures - Enable/disable TKIP countermeasures
* @priv: private driver interface data
* @enabled: 1 = countermeasures enabled, 0 = disabled
*
 * Returns: 0 on success, -1 on failure
*
* Configure TKIP countermeasures. When these are enabled, the driver
* should drop all received and queued frames that are using TKIP.
*/
static int wpa_driver_nrc_set_countermeasures(void *priv, int enabled)
{
	return -1;
}

 static int send_deauthenticate(struct nrc_wpa_if *intf, const u8 *a1,
 				const u8 *a2, const u8 *a3, u16 reason_code)
 {
 	struct ieee80211_mgmt mgmt = {0,};
	const int len = offsetof(struct ieee80211_mgmt, u.deauth.variable);

	mgmt.frame_control = IEEE80211_FC(WLAN_FC_TYPE_MGMT, WLAN_FC_STYPE_DEAUTH);
	mgmt.u.deauth.reason_code = reason_code;

	os_memcpy(mgmt.da, a1, ETH_ALEN);
	os_memcpy(mgmt.sa, a2, ETH_ALEN);
	os_memcpy(mgmt.bssid, a3, ETH_ALEN);

	return nrc_transmit(intf, (uint8_t *) &mgmt, len);
 }

/**
 * deauthenticate - Request driver to deauthenticate
 * @priv: private driver interface data
 * @addr: peer address (BSSID of the AP)
 * @reason_code: 16-bit reason code to be sent in the deauthentication
 *	frame
 *
 * Returns: 0 on success, -1 on failure
 */
 static int wpa_driver_nrc_deauthenticate(void *priv, const u8 *addr, u16 reason_code)
{
	struct nrc_wpa_if *intf = (struct nrc_wpa_if *)(priv);
	int i = 0;

	I(TT_WPAS, TAG "Deauth requested (addr=" MACSTR ",reason=%d)\n",
				MAC2STR(addr), reason_code);

	if(!intf)
		return -1;

	if (!intf->associated)
		return 0;

	if (!intf->is_ap) {
		send_deauthenticate(intf, addr, intf->addr, addr, reason_code);
		wpa_driver_sta_sta_remove(intf);
		reset_ip_address(intf->vif_id);
		for (i = 0; i < MAX_TID; i++)
			intf->sta.block_ack[i] = BLOCK_ACK_INVALID;
	} else {
		// Deauthenticate all connected STAs, AP entitiy send deauthuntication
		// frame though send_mgmt() indivisually otherwise.
		for (i = 0; i < NRC_WPA_SOFTAP_MAX_STA; i++) {
			if (intf->ap_sta[i])
				send_deauthenticate(intf, intf->ap_sta[i]->addr,
					intf->addr, intf->addr, reason_code);
		}
	}
#if defined(NRC_USER_APP)
	wpa_driver_notify_event_to_app(EVENT_DEAUTH);
#endif
	return 0;
}

static bool wpa_driver_is_privacy(struct wpa_driver_associate_params *p) {
	return !(!p->wpa_ie
		&& p->pairwise_suite == WPA_CIPHER_NONE
		&& p->group_suite == WPA_CIPHER_NONE
		&& p->key_mgmt_suite == WPA_KEY_MGMT_NONE);
}

static int wpa_driver_eid_ssid(uint8_t* eid, const uint8_t* ssid, uint8_t ssid_len)
{
	*(eid++) = WLAN_EID_SSID;
	*(eid++) = ssid_len;
	if (ssid)
		os_memcpy(eid, ssid, ssid_len);
	return 2 + ssid_len;
}

static int to_rateie(int rate, bool basic) {
	return (rate / 5) | (basic ? 0x80 : 0x0);
}

static int wpa_driver_eid_supp_rates(struct nrc_wpa_if *intf, uint8_t *eid)
{
	uint8_t* pos = eid;
	// TODO:
	*pos++ = WLAN_EID_SUPP_RATES;
	*pos++ = 8;
	*pos++ = to_rateie(10, true);
	*pos++ = to_rateie(20, true);
	*pos++ = to_rateie(55, true);
	*pos++ = to_rateie(110, true);
	*pos++ = to_rateie(180, false);
	*pos++ = to_rateie(240, false);
	*pos++ = to_rateie(360, false);
	*pos++ = to_rateie(540, false);
	*pos++ = WLAN_EID_EXT_SUPP_RATES;
	*pos++ = 4;
	*pos++ = to_rateie(60, false);
	*pos++ = to_rateie(90, false);
	*pos++ = to_rateie(120, false);
	*pos++ = to_rateie(480, false);

	return (pos - eid);
}

static int wpa_driver_wmm_elem(struct nrc_wpa_if *intf, uint8_t *eid)
{
	uint8_t* pos = eid;
	*pos++ = WLAN_EID_VENDOR_SPECIFIC;
	*pos++ = 7;
	*pos++ = 0x00; *pos++ = 0x50; *pos++ = 0xf2; // OUI_MICROSOFT
	*pos++ = WMM_OUI_TYPE;
	*pos++ = WMM_OUI_SUBTYPE_INFORMATION_ELEMENT;
	*pos++ = WMM_VERSION;
	*pos++ = 0;

	return (pos - eid);
}

static int wpa_driver_nrc_sta_associate(struct nrc_wpa_if *intf,
							struct wpa_driver_associate_params *p)
{
	uint8_t *frame = os_malloc(512);
	struct ieee80211_ht_capabilities *htcap = (struct ieee80211_ht_capabilities *)
							p->htcaps;
	struct ieee80211_mgmt *mgmt = (struct ieee80211_mgmt *) frame;
	const int ie_offset = offsetof(struct ieee80211_mgmt, u.assoc_req.variable);
	uint8_t* ie = frame + ie_offset;
	uint8_t* ie_pos = ie;

	if (!frame) {
		E(TT_WPAS, TAG "Failed to allocate assoc req frame\n");
		return -1;
	}

	I(TT_WPAS, TAG "%d associated (STA)\n", intf->vif_id);

	mgmt->frame_control = _FC(MGMT,ASSOC_REQ);
	mgmt->u.assoc_req.capab_info = WLAN_CAPABILITY_ESS;
	mgmt->u.assoc_req.listen_interval = 100; /* TODO */

	os_memcpy(mgmt->da,		p->bssid, ETH_ALEN);
	os_memcpy(mgmt->sa,		intf->addr, ETH_ALEN);
	os_memcpy(mgmt->bssid,	p->bssid, ETH_ALEN);

	if (wpa_driver_is_privacy(p))
		mgmt->u.assoc_req.capab_info |= WLAN_CAPABILITY_PRIVACY;

	ie_pos += wpa_driver_eid_ssid(ie_pos, p->ssid, p->ssid_len);
	ie_pos += wpa_driver_eid_supp_rates(intf, ie_pos);

	if (p->wpa_ie) {
		os_memcpy(ie_pos, p->wpa_ie, p->wpa_ie_len);
		ie_pos += p->wpa_ie_len;
	}

	if (htcap) {
		const int htcap_len = sizeof(struct ieee80211_ht_capabilities);
		htcap->a_mpdu_params = 0x17; // TODO:
		htcap->supported_mcs_set[0] = 0xFF; // TODO
		*ie_pos++ = WLAN_EID_HT_CAP;
		*ie_pos++ = htcap_len;
		os_memcpy(ie_pos, (uint8_t *) htcap, htcap_len);
		ie_pos += htcap_len;
	}

	ie_pos += wpa_driver_eid_supp_rates(intf, ie_pos);
	ie_pos += wpa_driver_wmm_elem(intf, ie_pos);

	nrc_transmit(intf, frame, ie_offset + (ie_pos - ie));
	os_free(frame);

	return 0;
}

/**
 * associate - Request driver to associate
 * @priv: private driver interface data
 * @params: association parameters
 *
 * Returns: 0 on success, -1 on failure
 */
 static int wpa_driver_nrc_associate(void *priv, struct wpa_driver_associate_params *p)
 {
	struct nrc_wpa_if* intf = (struct nrc_wpa_if *)(priv);

	wpa_driver_debug_assoc_params(p);

	if (!(p->mode == IEEE80211_MODE_INFRA ||
		  p->mode == IEEE80211_MODE_AP))
		return -1; // Only STA and AP modes are supported.

	intf->is_ap = (p->mode == IEEE80211_MODE_AP);

	intf->bss.ssid_len = p->ssid_len;
	intf->bss.beacon_int = p->beacon_int;
	I(TT_WPAS, TAG "%d Update beacon interval(STA) : %d(%d)\n",
		intf->vif_id, p->beacon_int, intf->bss.beacon_int);
	os_memcpy(intf->bss.ssid, p->ssid, p->ssid_len);

	if (intf->is_ap)
		// wpa_driver_associate_params::BSSID is null,
		os_memcpy(intf->bss.bssid, intf->addr, ETH_ALEN);
	else
		os_memcpy(intf->bss.bssid, p->bssid, ETH_ALEN);

	if (intf->is_ap) {
		intf->associated = true;

		if (intf->sta_type != WIM_STA_TYPE_AP) {
			intf->sta_type = WIM_STA_TYPE_AP;
			wpa_driver_sta_type_wim(intf, intf->sta_type);
		}
	} else {
		wpa_driver_nrc_sta_associate(intf, p);
	}

	//if (intf->is_ap)
	//	hostapd_logger_register_cb(nrc_wpa_hostapd_logger_cb);

	return 0;
}

/**
 * remove_pmkid - Remove PMKSA cache entry to the driver
 * @priv: private driver interface data
 * @bssid: BSSID for the PMKSA cache entry
 * @pmkid: PMKID for the PMKSA cache entry
 *
 * Returns: 0 on success, -1 on failure
 *
 * This function is called when the supplicant drops a PMKSA cache
 * entry for any reason.
 *
 * If the driver generates RSN IE, i.e., it does not use wpa_ie in
 * associate(), remove_pmkid() can be used to synchronize PMKSA caches
 * between the driver and wpa_supplicant. If the driver uses wpa_ie
 * from wpa_supplicant, this driver_ops function does not need to be
 * implemented. Likewise, if the driver does not support WPA, this
 * function is not needed.
 */
 static int wpa_driver_nrc_remove_pmkid(void *priv,
 				struct wpa_pmkid_params *params)
 {
	return -1;
 }

/**
 * get_capa - Get driver capabilities
 * @priv: private driver interface data
 *
 * Returns: 0 on success, -1 on failure
 *
 * Get driver/firmware/hardware capabilities.
 */
 static int wpa_driver_nrc_get_capa(void *priv, struct wpa_driver_capa *capa)
 {
 	struct nrc_wpa_if *intf = (struct nrc_wpa_if *)(priv);

	os_memset(capa, 0, sizeof(*capa));

	capa->key_mgmt = (WPA_DRIVER_CAPA_KEY_MGMT_WPA |
						WPA_DRIVER_CAPA_KEY_MGMT_WPA2 |
						WPA_DRIVER_CAPA_KEY_MGMT_WPA_PSK |
						WPA_DRIVER_CAPA_KEY_MGMT_WPA2_PSK);

	capa->enc 		= (WPA_DRIVER_CAPA_ENC_WEP40 |
						WPA_DRIVER_CAPA_ENC_WEP104 |
						WPA_DRIVER_CAPA_ENC_TKIP |
						WPA_DRIVER_CAPA_ENC_CCMP);

	capa->auth = WPA_DRIVER_AUTH_OPEN;

	capa->flags = (WPA_DRIVER_FLAGS_AP
					| WPA_DRIVER_FLAGS_FULL_AP_CLIENT_STATE
					| WPA_DRIVER_FLAGS_PROBE_RESP_OFFLOAD
					| WPA_DRIVER_FLAGS_SME
					| WPA_DRIVER_FLAGS_SET_KEYS_AFTER_ASSOC_DONE /* for WEP */ );

	capa->max_scan_ssids  = 2;

	return 0;
 }

/**
 * get_ifname - Get interface name
 * @priv: private driver interface data
 *
 * Returns: Pointer to the interface name. This can differ from the
 * interface name used in init() call. Init() is called first.
 *
 * This optional function can be used to allow the driver interface to
 * replace the interface name with something else, e.g., based on an
 * interface mapping from a more descriptive name.
 */
 static const char* wpa_driver_nrc_get_ifname(void *priv)
 {
 	struct nrc_wpa_if *intf = (struct nrc_wpa_if *)(priv);
	return intf->if_name;
 }

/**
 * get_mac_addr - Get own MAC address
 * @priv: private driver interface data
 *
 * Returns: Pointer to own MAC address or %NULL on failure
 *
 * This optional function can be used to get the own MAC address of the
 * device from the driver interface code. This is only needed if the
 * l2_packet implementation for the OS does not provide easy access to
 * a MAC address.
 */
 static const u8* wpa_driver_nrc_get_mac_addr(void *priv)
 {
 	struct nrc_wpa_if* intf = (struct nrc_wpa_if *)(priv);
	return intf->addr;
 }

/**
 * set_operstate - Sets device operating state to DORMANT or UP
 * @priv: private driver interface data
 * @state: 0 = dormant, 1 = up
 * Returns: 0 on success, -1 on failure
 *
 * This is an optional function that can be used on operating systems
 * that support a concept of controlling network device state from user
 * space applications. This function, if set, gets called with
 * state = 1 when authentication has been completed and with state = 0
 * when connection is lost.
 */
 static int wpa_driver_nrc_set_operstate(void *priv, int state)
 {
	struct nrc_wpa_if *intf = (struct nrc_wpa_if *)(priv);

	I(TT_WPAS, TAG "%d Sets device operating state (state=%d)\n", intf->vif_id,
		state);

	return 0;
 }

/**
 * mlme_setprotection - MLME-SETPROTECTION.request primitive
 * @priv: Private driver interface data
 * @addr: Address of the station for which to set protection (may be
 * %NULL for group keys)
 * @protect_type: MLME_SETPROTECTION_PROTECT_TYPE_*
 * @key_type: MLME_SETPROTECTION_KEY_TYPE_*
 * Returns: 0 on success, -1 on failure
 *
 * This is an optional function that can be used to set the driver to
 * require protection for Tx and/or Rx frames. This uses the layer
 * interface defined in IEEE 802.11i-2004 clause 10.3.22.1
 * (MLME-SETPROTECTION.request). Many drivers do not use explicit
 * set protection operation; instead, they set protection implicitly
 * based on configured keys.
 */
 static int wpa_driver_nrc_mlme_setprotection(void *priv, const u8 *addr,
 				int protect_type, int wpa_driver_nrc_key_type)
 {
	return 0;
 }

#define DECLARE_CHANNEL(p,i,c,f,l,m,n)  (p+i)->chan = c; \
			(p+i)->freq = f; \
			(p+i)->flag = l; \
			(p+i)->max_tx_power = m; \
			(p+i)->allowed_bw = HOSTAPD_CHAN_WIDTH_20; \
			(p+i)->min_nf = n;
/**
 * get_hw_feature_data - Get hardware support data (channels and rates)
 * @priv: Private driver interface data
 * @num_modes: Variable for returning the number of returned modes
 * flags: Variable for returning hardware feature flags
 * @dfs: Variable for returning DFS region (HOSTAPD_DFS_REGION_*)
 * Returns: Pointer to allocated hardware data on success or %NULL on
 * failure. Caller is responsible for freeing this.
 */
struct hostapd_hw_modes * wpa_driver_nrc_get_hw_feature_data(void *priv,
							u16 *num_modes, u16 *flags, u8 *dfs)
{
	struct nrc_wpa_if* intf = (struct nrc_wpa_if *)(priv);
	int i = 0, j = 0, k = 0, size = 0, n_chs = 0, max_tx_power = 20, min_nf = 48;
	struct hostapd_hw_modes* hw_modes = NULL;
	uint16_t *chs = NULL;

	/* TODO: The below should be retrieved from F/W */
	int rates_a[] = { 60, 120, 240, -1 };
	int rates_b[] = { 10, 20, 55, 110, -1 };
	//int rates_g[] = {10, 20, 55, 110, 60, 90, 120, 180, 240, 360, 480, 540, -1};
	int rates_g[] = {10, 20, 55, 110, 60, 90, 120, 180, 240, 360, 480, 540, -1};
	int *rates = NULL;

	I(TT_WPAS, TAG "%d hw feature (mode: %d, flags: 0x%2X)\n", intf->vif_id,
		*num_modes, *flags);
	system_api_get_supported_channels(&chs, &n_chs);

	if (chs[0] > 4000) {
		*num_modes = 1;
		hw_modes = (struct hostapd_hw_modes *) os_zalloc(sizeof(*hw_modes) *
									(*num_modes));
		hw_modes[0].mode = HOSTAPD_MODE_IEEE80211A;
		hw_modes[0].num_channels = n_chs;
	} else {
		*num_modes = 2;
		hw_modes = (struct hostapd_hw_modes *) os_zalloc(sizeof(*hw_modes) *
									(*num_modes));
		hw_modes[0].mode = HOSTAPD_MODE_IEEE80211B;
		hw_modes[1].mode = HOSTAPD_MODE_IEEE80211G;
		hw_modes[0].num_channels = hw_modes[1].num_channels = n_chs;
	}
	for (i = 0; i < *num_modes; i++) {
		hw_modes[i].channels = os_zalloc(sizeof(*hw_modes[i].channels)
								* hw_modes[i].num_channels);

		for (j = 0; j < n_chs; j++) {
			uint8_t ieee;
			if (ieee80211_freq_to_chan(chs[j], &ieee) == NUM_HOSTAPD_MODES)
				return NULL;
			DECLARE_CHANNEL(hw_modes[i].channels, j, ieee, chs[j], 80,
						max_tx_power, min_nf);
		}

		switch (hw_modes[i].mode) {
			case HOSTAPD_MODE_IEEE80211B:
			rates = rates_b;
			break;
			case HOSTAPD_MODE_IEEE80211A:
			rates = rates_a;
			break;
			case HOSTAPD_MODE_IEEE80211G:
			rates = rates_g;
			break;
			default:
			E(TT_WPAS, TAG "Failed to set rate due to wrong mode (%d)\n",
				hw_modes[i].mode);
			return NULL;
		}

		k = 0;
		while (rates[k++] >= 0)
			hw_modes[i].num_rates++;
		size = sizeof(int) * hw_modes[i].num_rates;
		hw_modes[i].rates = (int *) os_zalloc(size);
		os_memcpy(hw_modes[i].rates, rates, size);

		if (hw_modes[i].mode == HOSTAPD_MODE_IEEE80211A ||
			hw_modes[i].mode == HOSTAPD_MODE_IEEE80211G) {
			hw_modes[i].ht_capab		= HT_CAP_INFO_SHORT_GI20MHZ;
			hw_modes[i].mcs_set[0]		= 0xff;
			hw_modes[i].mcs_set[1]		= 0x0;
			hw_modes[i].a_mpdu_params	= 0;
			hw_modes[i].vht_capab		= 0;
		}
	}
	intf->hw_modes = hw_modes;

	return hw_modes;
 }


void wpa_driver_debug_event(enum wpa_event_type event, union wpa_event_data *data)
{
	switch (event) {
		case EVENT_TX_STATUS:
		I(TT_WPAS, TAG "New event type(%d), stype(%d), dst(" MACSTR "), ack(%d)\n",
					data->tx_status.type, data->tx_status.stype
					,MAC2STR(data->tx_status.dst), data->tx_status.ack);
		break;
		default:
		break;
	}
}

struct wpa_driver_frame_rx_event {
	uint16_t len;
	uint16_t freq;
	uint8_t data[0];
};

static void wpa_driver_nrc_tx_status(struct nrc_wpa_if* intf, const u8* data,
				size_t data_len, int ack)
{
	union wpa_event_data event;
	const struct ieee80211_mgmt *mgmt = (const struct ieee80211_mgmt *) data;

	os_memset(&event, 0, sizeof(event));
	event.tx_status.type = WLAN_FC_GET_TYPE(mgmt->frame_control);
	event.tx_status.stype = WLAN_FC_GET_STYPE(mgmt->frame_control);
	event.tx_status.dst = mgmt->da;
	event.tx_status.data = data;
	event.tx_status.data_len = data_len;
	event.tx_status.ack = ack;
	wpa_driver_debug_event(EVENT_TX_STATUS, &event);

	wpa_supplicant_event(intf->wpa_supp_ctx, EVENT_TX_STATUS, &event);
}

/**
 * send_mlme - Send management frame from MLME
 * @priv: Private driver interface data
 * @data: IEEE 802.11 management frame with IEEE 802.11 header
 * @data_len: Size of the management frame
 * @noack: Do not wait for this frame to be acked (disable retries)
 * @freq: Frequency (in MHz) to send the frame on, or 0 to let the
 * driver decide
 * Returns: 0 on success, -1 on failure
 */
static int wpa_driver_nrc_send_frame(struct nrc_wpa_if* intf, uint32_t freq,
			const u8 *data, size_t data_len)
{
	return nrc_transmit(intf, (uint8_t *) data, data_len);
}

static int wpa_driver_nrc_send_mgmt(void *priv, const u8 *data, size_t data_len,
			 int noack, unsigned int freq, const u16 *csa_offs,
			 size_t csa_offs_len)
{
	struct nrc_wpa_if* intf = (struct nrc_wpa_if *)(priv);

	if (wpa_driver_nrc_send_frame(intf, freq, data, data_len) < 0)
		return -1;

	wpa_driver_nrc_tx_status(intf, data, data_len, noack ? 0 : 1);

	return 0;
}

/**
 * update_ft_ies - Update FT (IEEE 802.11r) IEs
 * @priv: Private driver interface data
 * @md: Mobility domain (2 octets) (also included inside ies)
 * @ies: FT IEs (MDIE, FTIE, ...) or %NULL to remove IEs
 * @ies_len: Length of FT IEs in bytes
 * Returns: 0 on success, -1 on failure
 *
 * The supplicant uses this callback to let the driver know that keying
 * material for FT is available and that the driver can use the
 * provided IEs in the next message in FT authentication sequence.
 *
 * This function is only needed for driver that support IEEE 802.11r
 * (Fast BSS Transition).
 */
 static int wpa_driver_nrc_update_ft_ies(void *priv, const u8 *md, const u8 *ies,
	size_t ies_len)
{
	return -1;
}

/**
 * get_scan_results2 - Fetch the latest scan results
 * @priv: private driver interface data
 *
 * Returns: Allocated buffer of scan results (caller is responsible for
 * freeing the data structure) on success, NULL on failure
 */
 static struct wpa_scan_results* wpa_driver_nrc_get_scan_results2(void *priv)
 {
	struct nrc_wpa_if* intf = (struct nrc_wpa_if *)(priv);
	struct wpa_scan_results *res;

	res = get_scan_results(intf->scan);

	I(TT_WPAS, TAG "%d Received scan results (%lu BSSes)\n", intf->vif_id,
		res ? (unsigned long) res->num : 0);

	return res;
 }

#if defined(INCLUDE_BD_SUPPORT)
static uint16_t wpa_driver_nrc_checksum_16(uint16_t len, uint8_t* buf)
{
	uint32_t checksum = 0;
	int i = 0;

	//len = Total num of bytes
	while(len > 0)
	{
		//get two bytes at a time and  add previous calculated checsum value
		checksum = ((buf[i]) + (buf[i+1]<<8)) + checksum;

		//decrease by 2 for 2 byte boundaries
		len -= 2;
		i += 2;
	}
	//Add the carryout
	checksum = (checksum>>16) + checksum;
	// if 1's complement
	//checksum = (unsigned int)~checksum;

	return checksum;
}

char g_bdf_data[SF_SLOT_MAX_LEN] = {0,};
static void * wpa_driver_nrc_dump_load(int len)
{
	int i;
	uint32_t ret;
	size_t length = len;

	if (!system_modem_api_is_bdf_use()) {
		E(TT_WPAS, TAG "Board Data is Disabled.\n");
		return NULL;
	}
	ret = system_modem_api_get_bdf_data(SF_USER_CONFIG_4, (uint8_t *)g_bdf_data, length);

	if(!ret) {
		E(TT_WPAS, TAG "No board data is found\n");
		return NULL;
	}

	for(i=0; i < ret;) {
		V(TT_WPAS, TAG "%02X %02X %02X %02X %02X %02X %02X %02X\n",
			g_bdf_data[i],
			g_bdf_data[i+1],
			g_bdf_data[i+2],
			g_bdf_data[i+3],
			g_bdf_data[i+4],
			g_bdf_data[i+5],
			g_bdf_data[i+6],
			g_bdf_data[i+7]);
		i += 8;
	}

#if 0 // for single country case
	if(ret >= NRC_WPA_BD_HEADER_LENGTH)
		ret -= NRC_WPA_BD_HEADER_LENGTH;
	if(ret != (uint32_t)(g_bdf_data[2] + (g_bdf_data[3]<<8))) {
		E(TT_WPAS, TAG "Invalid length %u %u buf addr %p %p\n",
			(unsigned int)ret,
			(unsigned int)(g_bdf_data[2] + (g_bdf_data[3]<<8)),
			g_bdf_data, &g_bdf_data[0]);	
	}
#endif

	return &g_bdf_data[0];
}

struct wim_bd_param * wpa_driver_nrc_read_bd_tx_pwr(uint8_t *country_code)
{
	uint8_t cc_index = BOARD_CC_MAX;
	uint16_t ret = 0;
	uint16_t len = 0;
	uint8_t type = 0;
	int i, j;
	struct nrc_wpa_bdf *bd;
	uint8_t cc[2] = {0,0};
	struct wim_bd_param *bd_sel;

	bd_sel = (struct wim_bd_param *)os_zalloc(sizeof(*bd_sel));
	if (!bd_sel) {
		E(TT_WPAS, TAG "bd_sel is NULL\n");
		return NULL;
	}
	memset(bd_sel, 0, sizeof(*bd_sel));
	
	cc[0] = *country_code;
	cc[1] = *(country_code + 1);
	
	if(cc[0] == 'J' && cc[1] == 'P')
		cc_index = BOARD_CC_JP;
	else if(cc[0] == 'K' && cc[1] == 'R')
		cc_index = BOARD_CC_KR;
	else if(cc[0] == 'T' && cc[1] == 'W')
		cc_index = BOARD_CC_TW;
	else if(cc[0] == 'U' && cc[1] == 'S')
		cc_index = BOARD_CC_US;
	else if(cc[0] == 'D' && cc[1] == 'E')
		cc_index = BOARD_CC_EU;
	else if(cc[0] == 'C' && cc[1] == 'N')
		cc_index = BOARD_CC_CN;
	else {
		E(TT_WPAS, TAG "Invalid country code(%c%c). Set default value(%d)\n",
			cc[0], cc[1], cc_index);
		os_free(bd_sel);
		return NULL;
	}

	bd = (struct nrc_wpa_bdf*)
		wpa_driver_nrc_dump_load(NRC_WPA_BD_MAX_DATA_LENGTH +
			NRC_WPA_BD_HEADER_LENGTH);

	if(!bd) {
		E(TT_WPAS, TAG "bd is NULL\n");
		os_free(bd_sel);
		return NULL;
	}

	V(TT_WPAS, TAG "Major %02X Minor %02X Total len %04X Num_country %04X Checksum %04X\n",
		bd->ver_major, bd->ver_minor, bd->total_len, bd->num_country, bd->checksum_data);

	for(i=0; i < bd->total_len;) {
		V(TT_WPAS, TAG "%02X %02X %02X %02X %02X %02X %02X %02X\n",
			bd->data[i],
			bd->data[i+1],
			bd->data[i+2],
			bd->data[i+3],
			bd->data[i+4],
			bd->data[i+5],
			bd->data[i+6],
			bd->data[i+7]);
		i += 8;
	}

	// checksum for all country code's data
	ret = wpa_driver_nrc_checksum_16(bd->total_len, (uint8_t *)&bd->data[0]);

	if(ret != bd->checksum_data) {
		E(TT_WPAS, TAG "Invalid checksum(%04X : %04X)\n",
			bd->checksum_data, ret);
		os_free(bd_sel);
		return NULL;
	}

	for(i = 0; i < bd->num_country; i++) {
		type = g_bdf_data[NRC_WPA_BD_HEADER_LENGTH + len + 4*i];
		if(type == cc_index) {
			// copy data for specific country code
			I(TT_WPAS, TAG "cc_index is matched(%u : %u)\n",
				type, cc_index);
			bd_sel->type = (uint16_t)type;
			bd_sel->length = (uint16_t)(g_bdf_data[NRC_WPA_BD_HEADER_LENGTH + 2 + len + 4*i] +
					(g_bdf_data[NRC_WPA_BD_HEADER_LENGTH + 3 + len + 4*i]<<8));
			bd_sel->checksum = (uint16_t)(g_bdf_data[NRC_WPA_BD_HEADER_LENGTH + 4 + len + 4*i] +
					(g_bdf_data[NRC_WPA_BD_HEADER_LENGTH + 5 + len + 4*i]<<8));

			for(j=0; j < bd_sel->length - 2; j++)
				bd_sel->value[j] = g_bdf_data[NRC_WPA_BD_HEADER_LENGTH + 6 + len + 4*i + j];

			I(TT_WPAS, TAG "Type %04X, Len %04X, Checksum %04X\n",
				bd_sel->type, bd_sel->length, bd_sel->checksum);
			break;
		}
		len += (uint16_t)(g_bdf_data[NRC_WPA_BD_HEADER_LENGTH + 2 + len + 4*i] +
					(g_bdf_data[NRC_WPA_BD_HEADER_LENGTH + 3 + len + 4*i]<<8));
	}

	return bd_sel;
}
#endif /* defined(INCLUDE_BD_SUPPORT) */

/**
 * set_country - Set country
 * @priv: Private driver interface data
 * @alpha2: country to which to switch to
 * Returns: 0 on success, -1 on failure
 *
 * This function is for drivers which support some form
 * of setting a regulatory domain.
 */
 static int wpa_driver_nrc_set_country(void *priv, const char *alpha2)
 {
	struct nrc_wpa_if *intf = (struct nrc_wpa_if *)(priv);
 	struct wim_builder *wb = NULL;
 	char* p = (char *) alpha2;
	char country_name[2] = {0,};
#if defined(INCLUDE_BD_SUPPORT)
	int i;
	struct wim_bd_param *bd_param;
#endif /* defined(INCLUDE_BD_SUPPORT) */

	I(TT_WPAS, TAG "%d Set country code (%c%c)\n", intf->vif_id,
		*p, *(p+1));

	country_name[0] = *p;
	country_name[1] = *(p+1);

#if defined(INCLUDE_BD_SUPPORT)
	wb = wim_builder_create(intf->vif_id, WIM_CMD_SET, WM_MAX_SIZE);
#else
	wb = wim_builder_create(intf->vif_id, WIM_CMD_SET, WB_MIN_SIZE);
#endif /* defined(INCLUDE_BD_SUPPORT) */
	wim_builder_append(wb, WIM_TLV_COUNTRY_CODE,country_name, 2);

#if defined(INCLUDE_BD_SUPPORT)
	//Read board data and save buffer
	bd_param = wpa_driver_nrc_read_bd_tx_pwr((uint8_t *)p);
	if(bd_param) {
		V(TT_WPAS, TAG "type %04X length %04X checksum %04X\n",
			bd_param->type, bd_param->length, bd_param->checksum);
		for(i=0; i < bd_param->length - 2;) {
			V(TT_WPAS, TAG "%02X %02X %02X %02X %02X %02X %02X %02X\n",
				bd_param->value[i],
				bd_param->value[i+1],
				bd_param->value[i+2],
				bd_param->value[i+3],
				bd_param->value[i+4],
				bd_param->value[i+5],
				bd_param->value[i+6],
				bd_param->value[i+7]);
			i += 8;
		}
		wim_builder_append(wb, WIM_TLV_BD, bd_param, sizeof(*bd_param));
	}
#endif /* defined(INCLUDE_BD_SUPPORT) */

	wim_builder_run_wm_wq(wb, true);

	return 0;
}

/**
 * get_country - Get country
 * @priv: Private driver interface data
 * @alpha2: Buffer for returning country code (at least 3 octets)
 * Returns: 0 on success, -1 on failure
 */
 static int wpa_driver_nrc_get_country(void *priv, char *country)
{
	struct nrc_wpa_if* intf = (struct nrc_wpa_if *)(priv);

	return 0;
}

/**
* global_init - Global driver initialization
* @ctx: wpa_global pointer
* Returns: Pointer to private data (global), %NULL on failure
*
* This optional function is called to initialize the driver wrapper
* for global data, i.e., data that applies to all interfaces. If this
* function is implemented, global_deinit() will also need to be
* implemented to free the private data. The driver will also likely
* use init2() function instead of init() to get the pointer to global
* data available to per-interface initializer.
*/
 #define memset os_memset
#if defined(_UMAC_S1G)
 #include "s1g_config.h"
#endif
 #undef memset

 static void* wpa_driver_nrc_global_init(void *ctx) {
	struct wim_builder *wb = NULL;

	nrc_global = (struct nrc_wpa *) os_zalloc (sizeof(struct nrc_wpa));
	/* Start WIM */
	wb = wim_builder_create(0, WIM_CMD_START, WB_MIN_SIZE);
	wim_builder_run_wm(wb, true);

	return nrc_global;
 }

/**
 * global_deinit - Global driver deinitialization
 * @priv: private driver global data from global_init()
 *
 * Terminate any global driver related functionality and free the
 * global data structure.
 */
 static void wpa_driver_nrc_global_deinit(void *priv)
 {
	int i = 0;
	struct nrc_wpa *global = (struct nrc_wpa*) priv;
	struct nrc_wpa_if* intf = (struct nrc_wpa_if *)(priv);

	I(TT_WPAS, TAG "%d deinitialize\n", intf->vif_id);


	if(global) {
		for (i = 0; i < NRC_WPA_NUM_INTERFACES; i++) {
			if (global->intf[i])
				os_free(global->intf[i]);
		}
		os_free(global);
	}
}

static void wpa_driver_clear_key(struct nrc_wpa_key *key)
{
	os_memset(key, 0, sizeof(*key));
	key->cipher = WIM_CIPHER_TYPE_NONE;
}

static void wpa_driver_nrc_init_nrc(struct nrc_wpa_if* intf)
{
	int i = 0;
	struct wim_addr_param addr = {false, false, };
	struct wim_builder *wb = NULL;

	intf->sta_type = WIM_STA_TYPE_STA;
	os_memcpy(addr.addr, intf->addr, ETH_ALEN);

	wb = wim_builder_create(intf->vif_id, WIM_CMD_SET, WB_MIN_SIZE);
	wim_builder_append_u32(wb, WIM_TLV_STA_TYPE, intf->sta_type);
	wim_builder_append(wb, WIM_TLV_MACADDR_PARAM, &addr, sizeof(addr));
	wim_builder_run_wm(wb, true);

#if defined(_UMAC_S1G)
	s1g_config *s1g_cfg = s1g_config::getInstance();
	s1g_cfg->setS1GStaTypeSupport(NON_SENSOR);
#endif

	for (i = ACI_BK; i <= ACI_VO; i++)
		system_modem_api_set_aggregation(i, false);
}

/**
 * init2 - Initialize driver interface (with global data)
 * @ctx: context to be used when calling wpa_supplicant functions,
 * e.g., wpa_supplicant_event()
 * @ifname: interface name, e.g., wlan0
 * @global_priv: private driver global data from global_init()
 * Returns: Pointer to private data, %NULL on failure
 *
 * This function can be used instead of init() if the driver wrapper
 * uses global data.
 */
 static void* wpa_driver_nrc_init(void *ctx, const char *ifname, void *global_priv)
 {
	struct nrc_wpa *global	= (struct nrc_wpa *) global_priv;
	struct nrc_wpa_if* intf	= (struct nrc_wpa_if *) os_zalloc(sizeof(*intf));

	if (!intf) {
		E(TT_WPAS, TAG "Failed to allocate if_info\n");
		return NULL;
	}

	if (os_strcmp(NRC_WPA_INTERFACE_NAME_0, ifname) == 0) {
		intf->vif_id = 0;
	} else if (os_strcmp(NRC_WPA_INTERFACE_NAME_1, ifname) == 0) {
		intf->vif_id = 1;
	} else {
		ASSERT(0);
	}
	I(TT_WPAS, TAG "%d initialize (ifname: %s) \n", intf->vif_id, ifname);

	global->intf[intf->vif_id]		= intf;
	strcpy(intf->if_name, ifname);
	//TODO:
	get_standalone_macaddr(intf->vif_id, intf->addr);
	//os_memcpy(intf->addr, get_standalone_macaddr(), ETH_ALEN);
	intf->global		= global;
	intf->wpa_supp_ctx	= ctx;
	wpa_driver_clear_key(&intf->sta.key);
	wpa_driver_clear_key(&intf->bss.broadcast_key);
	intf->scan = scan_init();
	os_memcpy(intf->sta.addr, intf->addr, ETH_ALEN);
	wpa_driver_nrc_init_nrc(intf);

	return (void *)(intf);
 }

/**
 * get_interfaces - Get information about available interfaces
 * @global_priv: private driver global data from global_init()
 * Returns: Allocated buffer of interface information (caller is
 * responsible for freeing the data structure) on success, NULL on
 * failure
 */
static struct wpa_interface_info* wpa_driver_nrc_get_interfaces(void *global_priv)
{
	return NULL;
}

void wpa_driver_nrc_scan_timeout(void *eloop_ctx, void *timeout_ctx)
{
	union wpa_event_data event;
	struct nrc_wpa_if* intf = (struct nrc_wpa_if *)(eloop_ctx);

	os_memset(&event, 0, sizeof(event));
	event.scan_info.aborted = false;
	event.scan_info.nl_scan_event = 1; /* From normal scan */

	wpa_supplicant_event(intf->wpa_supp_ctx, EVENT_SCAN_RESULTS, &event);
}

#define SCAN_PF(x) (WIM_SCAN_PARAM_FLAG_##x)

static int wpa_driver_nrc_scan2_nrc(struct nrc_wpa_if* intf, struct wpa_driver_scan_params *p)
{
	struct wim_builder *wb = wim_builder_create(intf->vif_id, WIM_CMD_SCAN_START, WM_MAX_SIZE);
	struct wim_scan_param wimp = {{},{}, 0, 0, 0, 0, 0, {},{},{}};
	int i = 0, nchs = 0;
	int *freqs = p->freqs;
	uint16_t chs[WIM_MAX_SCAN_CHANNEL] = {0,}, last_ch = 0;

	if (freqs) {
		while (*freqs)
			wimp.channel[wimp.n_channels++] = *(freqs++);
	} else {
		I(TT_WPAS, TAG "Wrong scan list, "
			"replaced with system supported channels\n");
		wimp.n_channels = nrc_get_channel_list(intf, chs, WIM_MAX_SCAN_CHANNEL);
		os_memcpy(wimp.channel, chs, sizeof(uint16_t) * WIM_MAX_SCAN_CHANNEL);
	}

	if (wimp.n_channels > 0)
		last_ch = wimp.channel[wimp.n_channels - 1];

	wimp.n_ssids = p->num_ssids;
	for (i = 0; i < p->num_ssids; i++) {
		wimp.ssid[i].ssid_len = p->ssids[i].ssid_len;
		os_memcpy(wimp.ssid[i].ssid, p->ssids[i].ssid, p->ssids[i].ssid_len);

		I(TT_WPAS, TAG "SSID(%d) : %s %s\n", i,
			wpa_ssid_txt(p->ssids[i].ssid, p->ssids[i].ssid_len),
			p->ssids[i].ssid_len ? "[Selected SSID]" : "");
	}

	if (p->bssid) {
		wimp.n_bssids++;
		memcpy(wimp.bssid, p->bssid, ETH_ALEN);
	}
#if defined(STANDARD_11N)
	wimp.scan_flag = SCAN_PF(HT) | SCAN_PF(WMM) | SCAN_PF(DS);
#endif
	scan_start(intf->scan);
	scan_config(intf->scan, p, last_ch);

	/* Start Scan */
	wim_builder_append(wb, WIM_TLV_SCAN_PARAM, &wimp, sizeof(wimp));
	wim_builder_run_wm_wq(wb, true);

	return 0;
}
#undef SCAN_PF

static int wpa_driver_nrc_scan2(void *priv, struct wpa_driver_scan_params *p)
{
	struct nrc_wpa_if* intf = (struct nrc_wpa_if *)(priv);

	if (intf->is_ap) {
		E(TT_WPAS, TAG "scan is triggered on AP Mode\n");
		if (eloop_register_timeout(0, 20, wpa_driver_scan_done_timeout, 0,
			(void *) intf->vif_id) < 0)
			return -1;
		return 0;
	}

#if !defined(NRC_USER_APP) && !defined(CONFIG_BG_SCAN)
	if (intf->associated) {
		// Temp code: prevent scan request when associated
		E(TT_WPAS, TAG "scan is not allowed when associated.\n");
		return -1;
	}
#endif /* !defined(NRC_USER_APP) && !defined(CONFIG_BG_SCAN) */

	wpa_driver_nrc_scan2_nrc(intf, p);

	return 0;
}

/**
 * set_acl - Set ACL in AP mode
 * @priv: Private driver interface data
 * @params: Parameters to configure ACL
 * Returns: 0 on success, -1 on failure
 *
 * This is used only for the drivers which support MAC address ACL.
 */
 static int wpa_driver_nrc_set_acl(void *priv, struct hostapd_acl_params *params)
{
	return -1;
}

/**
 * read_sta_data - Fetch station data
 * @priv: Private driver interface data
 * @data: Buffer for returning station information
 * @addr: MAC address of the station
 * Returns: 0 on success, -1 on failure
 */
 static int wpa_driver_nrc_read_sta_data(void *priv, struct hostap_sta_driver_data *data,
	const u8 *addr)
{
	return -1;
}

static void wpa_driver_nrc_sta_cmd_wim(struct nrc_wpa_if* intf, int cmd,
				const uint8_t *addr, int sleep, int aid, int flags)
{
 	struct wim_sta_param p = {cmd, {} /*addr*/, sleep, aid, flags};
 	os_memcpy(p.addr, addr, ETH_ALEN);
 	struct wim_builder * wb = wim_builder_create(intf->vif_id, WIM_CMD_STA_CMD, WB_MIN_SIZE);
	wim_builder_append(wb, WIM_TLV_STA_PARAM, &p, sizeof(p));
	wim_builder_run_wm_wq(wb, true);
}

static void wpa_remove_sta(struct nrc_wpa_if * intf, const uint8_t* addr)
{
	int i;
	struct nrc_wpa_key *wpa_key = NULL;
	uint16_t aid = 0;

	for (i = 0; i < NRC_WPA_SOFTAP_MAX_STA; i++) {
		if (!intf->ap_sta[i])
			continue;

		if (os_memcmp(intf->ap_sta[i]->addr, addr, ETH_ALEN) != 0)
			continue;

		wpa_key = &intf->ap_sta[i]->key;

		if (is_key_wep(wpa_key)) {
			wpa_key->is_set[wpa_key->ix] = false;
			wpa_driver_nrc_set_key_wim(intf, wpa_key->cipher,
				addr, wpa_key->ix, wpa_key->key, wpa_key->key_len,
				wpa_key->is_set[wpa_key->ix]);
		}

		if (wpa_key->is_set[wpa_key->ix])
			wpa_driver_nrc_remove_key_wim(intf, wpa_key);

		os_free(intf->ap_sta[i]);
		intf->ap_sta[i] = NULL;
		intf->num_ap_sta--;
	}
}

#define W2ST(x) case WIM_STA_CMD_STATE_##x: return #x
static const char* convert_sta_state(int wim_state)
{
	switch (wim_state) {
		W2ST(NOTEXIST);
		W2ST(NONE);
		W2ST(AUTH);
		W2ST(ASSOC);
		W2ST(AUTHORIZED);
	}
	return "Unknown";
}

static void send_local_deauth_event(struct nrc_wpa_if *intf, const uint8_t *addr)
{
	union wpa_event_data event;
	event.deauth_info.addr = addr;
	event.deauth_info.ie = NULL;
	event.deauth_info.ie_len = 0;
	event.deauth_info.locally_generated = 1;
	wpa_supplicant_event(intf->wpa_supp_ctx, EVENT_DEAUTH, &event);
}
/*
 * AP Only - STATE is changed
 */
static void on_sta_state_changed(struct nrc_wpa_if* intf, const uint8_t* addr,
				const uint16_t new_aid, int new_state)
{
	struct nrc_wpa_sta* sta = nrc_wpa_find_sta(intf, addr);

	I(TT_WPA, TAG "STA state changed (mac:" MACSTR ", %s -> %s, aid %u) \n",
		MAC2STR(addr),
		convert_sta_state(sta ? sta->state : WIM_STA_CMD_STATE_NOTEXIST),
			convert_sta_state(new_state),
		new_aid);

	if (sta && sta->state == new_state)
		return;

	switch (new_state) {
		case WIM_STA_CMD_STATE_NOTEXIST:
		if (sta) {
			wpa_driver_nrc_sta_cmd_wim(intf, WIM_STA_CMD_REMOVE,
				addr, 0, new_aid, 0);
			wpa_remove_sta(intf, addr);
		}
		break;
		case WIM_STA_CMD_STATE_NONE:
		/*
		 * TODO: Define this.
		 */
		break;
		case WIM_STA_CMD_STATE_AUTH:
		if (sta)
			send_local_deauth_event(intf, addr);

		sta = nrc_wpa_add_sta(intf, addr, new_aid);
		break;
		case WIM_STA_CMD_STATE_ASSOC:
		if (!sta)
			/*
			 * This happens when Standalone STA resets without deauth/disasssoc
			 * frame sent and the STA tries to authenticate within BSS MAX
			 * IDLE period. For this, first change this state to NONE
			 * so that Drv and F/W clean up and starts over.
			 */
			on_sta_state_changed(intf, addr, 0, WIM_STA_CMD_STATE_AUTH);

		sta->aid = new_aid;
		wpa_driver_nrc_sta_cmd_wim(intf, WIM_STA_CMD_ADD,
						addr, 0, new_aid, 0);
		break;
		case WIM_STA_CMD_STATE_AUTHORIZED:
		/*
		 * TODO: Fill this,
		 */
		break;
	}

	sta->state = new_state;

	if((sta) && (new_state == WIM_STA_CMD_STATE_NOTEXIST))
		return;
	else
		wpa_driver_nrc_sta_cmd_wim(intf, new_state, addr, 0 /*not used*/,
				new_aid, 0/*not used*/);
}

/**
 * sta_add - Add a station entry
 * @priv: Private driver interface data
 * @params: Station parameters
 * Returns: 0 on success, -1 on failure
 *
 * This function is used to add a station entry to the driver once the
 * station has completed association. This is only used if the driver
 * does not take care of association processing.
 *
 * With TDLS, this function is also used to add or set (params->set 1)
 * TDLS peer entries.
 */
static int wpa_driver_nrc_sta_add(void *priv, struct hostapd_sta_add_params *p)
{
	struct nrc_wpa_if* intf = (struct nrc_wpa_if *)(priv);

	I(TT_WPAS, TAG "[SoftAP] " MACSTR " flags:0x%x\n", MAC2STR(p->addr), p->flags);

	if (p->flags & WLAN_STA_ASSOC) {
		I(TT_WPAS, TAG "SoftAP associated " MACSTR "\n", MAC2STR(p->addr));
		on_sta_state_changed(intf, p->addr, p->aid, WIM_STA_CMD_STATE_ASSOC);
		return 0;
	}

	 if (!p->flags || p->flags & WLAN_STA_AUTH){
		I(TT_WPAS, TAG "SoftAP authenticated " MACSTR "\n", MAC2STR(p->addr));
		on_sta_state_changed(intf, p->addr, 0, WIM_STA_CMD_STATE_AUTH);
		 return 0;
	}

	 I(TT_WPAS, TAG "[SoftAP] " MACSTR " not supported flags:0x%x\n",MAC2STR(p->addr), p->flags);

	return 0;
}
/**
 * sta_remove - Remove a station entry (AP only)
 * @priv: Private driver interface data
 * @addr: MAC address of the station to be removed
 * Returns: 0
 */
static int wpa_driver_nrc_sta_remove(void *priv, const u8 *addr)
{
	struct nrc_wpa_if* intf = (struct nrc_wpa_if *)(priv);
	struct nrc_wpa_sta* sta = nrc_wpa_find_sta(intf, addr);
	uint16_t aid = 0;

	I(TT_WPAS, TAG "Remove STA (" MACSTR ")\n", MAC2STR(addr));

	if (!sta || intf->num_ap_sta <= 0) {
		E(TT_WPAS, TAG "Attempt to remove unknown STA\n");
		return 0;
	}
	aid = sta->aid;
	on_sta_state_changed(intf, addr, aid, WIM_STA_CMD_STATE_NOTEXIST);
	return 0;
}

static int wpa_driver_nrc_sta_deauth(void *priv, const u8 *own_addr,
				const u8 *addr, u16 reason)
{
	struct nrc_wpa_if* intf = (struct nrc_wpa_if *)(priv);
	struct nrc_wpa_sta* sta = nrc_wpa_find_sta(intf, addr);

	if (!sta) {
		E(TT_WPAS, TAG "Attempt to deauthenticate unknown STA ("MACSTR")\n",
			MAC2STR(addr));
		return 0;
	}

	on_sta_state_changed(intf, addr, 0, WIM_STA_CMD_STATE_NONE);

	return 0;
}

static int wpa_driver_nrc_sta_disassoc(void *priv, const u8 *own_addr,
					const u8 *addr, u16 reason)
{
	struct nrc_wpa_if* intf = (struct nrc_wpa_if *)(priv);
	on_sta_state_changed(intf, addr, 0, WIM_STA_CMD_STATE_NONE);
	return 0;
}

/**
 * set_rts - Set RTS threshold
 * @priv: Private driver interface data
 * @rts: RTS threshold in octets
 * Returns: 0 on success, -1 on failure
 */
static int wpa_driver_nrc_set_rts(void *priv, int rts)
{
	return -1;
}

/**
 * set_frag - Set fragmentation threshold
 * @priv: Private driver interface data
 * @frag: Fragmentation threshold in octets
 * Returns: 0 on success, -1 on failure
 */
 static int wpa_driver_nrc_set_frag(void *priv, int frag)
{
	return -1;
}

/**
 * set_tx_queue_params - Set TX queue parameters
 * @priv: Private driver interface data
 * @queue: Queue number (0 = VO, 1 = VI, 2 = BE, 3 = BK)
 * @aifs: AIFS
 * @cw_min: cwMin
 * @cw_max: cwMax
 * @burst_time: Maximum length for bursting in 0.1 msec units
 */
 static int wpa_driver_nrc_set_tx_queue_params(void *priv, int queue, int aifs, int cw_min,
	int cw_max, int burst_time)
{
	struct nrc_wpa_if *intf = (struct nrc_wpa_if *)(priv);
	struct wim_tx_queue_param txq_p	= {queue, 0, cw_min, cw_max, 0, };

	struct wim_builder *wb = NULL;
	int q_to_ac[WMM_AC_NUM] = {WMM_AC_VO, WMM_AC_VI, WMM_AC_BE, WMM_AC_BK};

	if (queue >= WMM_AC_NUM)
		return -1;

	txq_p.ac = q_to_ac[queue];
	txq_p.txop = (burst_time * 100 + 16) / 32; /*0.1 msec to TXOP param */
	txq_p.sta_type = 0;
	txq_p.aifsn = aifs;

#if 0 // untested
	wb = wim_builder_create(intf->vif_id, WIM_CMD_SET, WB_MIN_SIZE);
	wim_builder_append(wb, WIM_TLV_TXQ_PARAM, &txq_p, sizeof(txq_p));
	wim_builder_run_wm(wb, true);
#endif

	return 0;
}

/**
 * if_add - Add a virtual interface
 * @priv: Private driver interface data
 * @type: Interface type
 * @ifname: Interface name for the new virtual interface
 * @addr: Local address to use for the interface or %NULL to use the
 *	parent interface address
 * @bss_ctx: BSS context for %WPA_IF_AP_BSS interfaces
 * @drv_priv: Pointer for overwriting the driver context or %NULL if
 *	not allowed (applies only to %WPA_IF_AP_BSS type)
 * @force_ifname: Buffer for returning an interface name that the
 *	driver ended up using if it differs from the requested ifname
 * @if_addr: Buffer for returning the allocated interface address
 *	(this may differ from the requested addr if the driver cannot
 *	change interface address)
 * @bridge: Bridge interface to use or %NULL if no bridge configured
 * @use_existing: Whether to allow existing interface to be used
 * Returns: 0 on success, -1 on failure
 */
 static int wpa_driver_nrc_if_add(void *priv, enum wpa_driver_if_type type,
	const char *ifname, const u8 *addr, void *bss_ctx,
	void **drv_priv, char *force_ifname, u8 *if_addr,
	const char *bridge, int use_existing,int setup_ap)
{
	return 0;
}

/**
 * if_remove - Remove a virtual interface
 * @priv: Private driver interface data
 * @type: Interface type
 * @ifname: Interface name of the virtual interface to be removed
 * Returns: 0 on success, -1 on failure
 */
 static int wpa_driver_nrc_if_remove(void *priv, enum wpa_driver_if_type type,
	const char *ifname)
{
	return -1;
}

/**
 * set_radius_acl_auth - Notification of RADIUS ACL change
 * @priv: Private driver interface data
 * @mac: MAC address of the station
 * @accepted: Whether the station was accepted
 * @session_timeout: Session timeout for the station
 * Returns: 0 on success, -1 on failure
 */
 static int wpa_driver_nrc_set_radius_acl_auth(void *priv, const u8 *mac, int accepted,
	u32 session_timeout)
{
	return -1;
}

/**
 * set_radius_acl_expire - Notification of RADIUS ACL expiration
 * @priv: Private driver interface data
 * @mac: MAC address of the station
 * Returns: 0 on success, -1 on failure
 */
 static int wpa_driver_nrc_set_radius_acl_expire(void *priv, const u8 *mac)
{
	return -1;
}

/**
 * set_supp_port - Set IEEE 802.1X Supplicant Port status
 * @priv: Private driver interface data
 * @authorized: Whether the port is authorized
 * Returns: 0 on success, -1 on failure
 */
 static int wpa_driver_nrc_set_supp_port(void *priv, int authorized)
{
	struct nrc_wpa_if *intf = (struct nrc_wpa_if *)(priv);
	intf->bss.authorized_1x = !!(authorized);
	I(TT_WPAS, TAG "Port %s\n", authorized ? "authorized" : "unauthorized");
	return 0;
}

/**
 * send_action - Transmit an Action frame
 * @priv: Private driver interface data
 * @freq: Frequency (in MHz) of the channel
 * @wait: Time to wait off-channel for a response (in ms), or zero
 * @dst: Destination MAC address (Address 1)
 * @src: Source MAC address (Address 2)
 * @bssid: BSSID (Address 3)
 * @data: Frame body
 * @data_len: data length in octets
 @ @no_cck: Whether CCK rates must not be used to transmit this frame
 * Returns: 0 on success, -1 on failure
 *
 * This command can be used to request the driver to transmit an action
 * frame to the specified destination.
 *
 * If the %WPA_DRIVER_FLAGS_OFFCHANNEL_TX flag is set, the frame will
 * be transmitted on the given channel and the device will wait for a
 * response on that channel for the given wait time.
 *
 * If the flag is not set, the wait time will be ignored. In this case,
 * if a remain-on-channel duration is in progress, the frame must be
 * transmitted on that channel; alternatively the frame may be sent on
 * the current operational channel (if in associated state in station
 * mode or while operating as an AP.)
 */
 static int wpa_driver_nrc_send_action(void *priv, unsigned int freq,
 		unsigned int wait, const u8 *dst, const u8 *src, const u8 *bssid,
		const u8 *data, size_t data_len, int no_cck)
{
	return 0;
}

/**
 * send_action_cancel_wait - Cancel action frame TX wait
 * @priv: Private driver interface data
 *
 * This command cancels the wait time associated with sending an action
 * frame. It is only available when %WPA_DRIVER_FLAGS_OFFCHANNEL_TX is
 * set in the driver flags.
 */
 static void wpa_driver_nrc_send_action_cancel_wait(void *priv)
{
}

/**
 * remain_on_channel - Remain awake on a channel
 * @priv: Private driver interface data
 * @freq: Frequency (in MHz) of the channel
 * @duration: Duration in milliseconds
 * Returns: 0 on success, -1 on failure
 *
 * This command is used to request the driver to remain awake on the
 * specified channel for the specified duration and report received
 * Action frames with EVENT_RX_MGMT events. Optionally, received
 * Probe Request frames may also be requested to be reported by calling
 * probe_req_report(). These will be reported with EVENT_RX_PROBE_REQ.
 *
 * The driver may not be at the requested channel when this function
 * returns, i.e., the return code is only indicating whether the
 * request was accepted. The caller will need to wait until the
 * EVENT_REMAIN_ON_CHANNEL event indicates that the driver has
 * completed the channel change. This may take some time due to other
 * need for the radio and the caller should be prepared to timing out
 * its wait since there are no guarantees on when this request can be
 * executed.
 */
 static int wpa_driver_nrc_remain_on_channel(void *priv, unsigned int freq,
	unsigned int duration)
{
	return 0;
}

/**
 * cancel_remain_on_channel - Cancel remain-on-channel operation
 * @priv: Private driver interface data
 *
 * This command can be used to cancel a remain-on-channel operation
 * before its originally requested duration has passed. This could be
 * used, e.g., when remain_on_channel() is used to request extra time
 * to receive a response to an Action frame and the response is
 * received when there is still unneeded time remaining on the
 * remain-on-channel operation.
 */
static int wpa_driver_nrc_cancel_remain_on_channel(void *priv)
{
	return 0;
}

/**
 * probe_req_report - Request Probe Request frames to be indicated
 * @priv: Private driver interface data
 * @report: Whether to report received Probe Request frames
 * Returns: 0 on success, -1 on failure (or if not supported)
 *
 * This command can be used to request the driver to indicate when
 * Probe Request frames are received with EVENT_RX_PROBE_REQ events.
 * Since this operation may require extra resources, e.g., due to less
 * optimal hardware/firmware RX filtering, many drivers may disable
 * Probe Request reporting at least in station mode. This command is
 * used to notify the driver when the Probe Request frames need to be
 * reported, e.g., during remain-on-channel operations.
 */
 static int wpa_driver_nrc_probe_req_report(void *priv, int report)
{
	return 0;
}

/**
 * deinit_ap - Deinitialize AP mode
 * @priv: Private driver interface data
 * Returns: 0 on success, -1 on failure (or if not supported)
 *
 * This optional function can be used to disable AP mode related
 * configuration. If the interface was not dynamically added,
 * change the driver mode to station mode to allow normal station
 * operations like scanning to be completed.
 */
 static int wpa_driver_nrc_deinit_ap(void *priv)
{
	uint8_t null_bssid[ETH_ALEN] = {0,};
	struct nrc_wpa_if *intf = (struct nrc_wpa_if *)(priv);
	struct wim_builder *wb = wim_builder_create(intf->vif_id, WIM_CMD_SET, WB_MIN_SIZE);
	int i;

	I(TT_WPAS, TAG "deinitialize AP\n");

	intf->is_ap = false;

	if (intf->sta_type != WIM_STA_TYPE_STA) {
		intf->sta_type = WIM_STA_TYPE_STA;
		wim_builder_append_u32(wb, WIM_TLV_STA_TYPE, intf->sta_type);
	}

	wim_builder_append(wb, WIM_TLV_BSSID, null_bssid, ETH_ALEN);
	wim_builder_run_wm_wq(wb, true);
	reset_ip_address(intf->vif_id);
	intf->associated = false;

	wb = wim_builder_create(intf->vif_id, WIM_CMD_STOP, WB_MIN_SIZE);
	wim_builder_run_wm_wq(wb, true);

	if (intf->vif_id == 0) {
		for (i = ACI_BK; i <= ACI_VO; i++)
			system_modem_api_set_aggregation(i, false);
	}

	return 0;
}

/**
 * deinit_p2p_cli - Deinitialize P2P client mode
 * @priv: Private driver interface data
 * Returns: 0 on success, -1 on failure (or if not supported)
 *
 * This optional function can be used to disable P2P client mode. If the
 * interface was not dynamically added, change the interface type back
 * to station mode.
 */
 static int wpa_driver_nrc_deinit_p2p_cli(void *priv)
{
	return 0;
}

/**
 * suspend - Notification on system suspend/hibernate event
 * @priv: Private driver interface data
 */
static void wpa_driver_nrc_suspend(void *priv)
{
}

/**
 * resume - Notification on system resume/thaw event
 * @priv: Private driver interface data
 */
static void wpa_driver_nrc_resume(void *priv)
{
}

/**
 * signal_monitor - Set signal monitoring parameters
 * @priv: Private driver interface data
 * @threshold: Threshold value for signal change events; 0 = disabled
 * @hysteresis: Minimum change in signal strength before indicating a
 *	new event
 * Returns: 0 on success, -1 on failure (or if not supported)
 *
 * This function can be used to configure monitoring of signal strength
 * with the current AP. Whenever signal strength drops below the
 * %threshold value or increases above it, EVENT_SIGNAL_CHANGE event
 * should be generated assuming the signal strength has changed at
 * least %hysteresis from the previously indicated signal change event.
 */
 static int wpa_driver_nrc_signal_monitor(void *priv, int threshold, int hysteresis)
{
	return -1;
}

/**
 * get_noa - Get current Notice of Absence attribute payload
 * @priv: Private driver interface data
 * @buf: Buffer for returning NoA
 * @buf_len: Buffer length in octets
 * Returns: Number of octets used in buf, 0 to indicate no NoA is being
 * advertized, or -1 on failure
 *
 * This function is used to fetch the current Notice of Absence
 * attribute value from GO.
 */
static int wpa_driver_nrc_get_noa(void *priv, u8 *buf, size_t buf_len)
{
	return -1;
}

/**
 * set_noa - Set Notice of Absence parameters for GO (testing)
 * @priv: Private driver interface data
 * @count: Count
 * @start: Start time in ms from next TBTT
 * @duration: Duration in ms
 * Returns: 0 on success or -1 on failure
 *
 * This function is used to set Notice of Absence parameters for GO. It
 * is used only for testing. To disable NoA, all parameters are set to
 * 0.
 */
static int wpa_driver_nrc_set_noa(void *priv, u8 count, int start, int duration)
{
	return -1;
}

/**
 * set_p2p_powersave - Set P2P power save options
 * @priv: Private driver interface data
 * @legacy_ps: 0 = disable, 1 = enable, 2 = maximum PS, -1 = no change
 * @opp_ps: 0 = disable, 1 = enable, -1 = no change
 * @ctwindow: 0.. = change (msec), -1 = no change
 * Returns: 0 on success or -1 on failure
 */
 static int wpa_driver_nrc_set_p2p_powersave(void *priv, int legacy_ps, int opp_ps,
	int ctwindow)
{

	return -1;
}

/**
 * ampdu - Enable/disable aggregation
 * @priv: Private driver interface data
 * @ampdu: 1/0 = enable/disable A-MPDU aggregation
 * Returns: 0 on success or -1 on failure
 */
 static int wpa_driver_nrc_ampdu(void *priv, int ampdu)
{
	return -1;
}

/**
 * get_radio_name - Get physical radio name for the device
 * @priv: Private driver interface data
 * Returns: Radio name or %NULL if not known
 *
 * The returned data must not be modified by the caller. It is assumed
 * that any interface that has the same radio name as another is
 * sharing the same physical radio. This information can be used to
 * share scan results etc. information between the virtual interfaces
 * to speed up various operations.
 */
 static const char * wpa_driver_nrc_get_radio_name(void *priv)
 {
	return "nrc";
 }

/**
 * set_wowlan - Set wake-on-wireless triggers
 * @priv: Private driver interface data
 * @triggers: wowlan triggers
 */
static int wpa_driver_nrc_set_wowlan(void *priv, const struct wowlan_triggers *triggers)
{
	return -1;
}

/**
 * signal_poll - Get current connection information
 * @priv: Private driver interface data
 * @signal_info: Connection info structure
 */
static int wpa_driver_nrc_signal_poll(void *priv, struct wpa_signal_info *signal_info)
{
	return -1;
}

/**
 * vendor_cmd - Execute vendor specific command
 * @priv: Private driver interface data
 * @vendor_id: Vendor id
 * @subcmd: Vendor command id
 * @data: Vendor command parameters (%NULL if no parameters)
 * @data_len: Data length
 * @buf: Return buffer (%NULL to ignore reply)
 * Returns: 0 on success, negative (<0) on failure
 *
 * This function handles vendor specific commands that are passed to
 * the driver/device. The command is identified by vendor id and
 * command id. Parameters can be passed as argument to the command
 * in the data buffer. Reply (if any) will be filled in the supplied
 * return buffer.
 *
 * The exact driver behavior is driver interface and vendor specific. As
 * an example, this will be converted to a vendor specific cfg80211
 * command in case of the nl80211 driver interface.
 */
 static int wpa_driver_nrc_vendor_cmd(void *priv, unsigned int vendor_id,
	unsigned int subcmd, const u8 *data, size_t data_len,
	struct wpabuf *buf)
{
	return -1;
}

/**
 * sched_scan - Request the driver to initiate scheduled scan
 * @priv: Private driver interface data
 * @params: Scan parameters
 * @interval: Interval between scan cycles in milliseconds
 * Returns: 0 on success, -1 on failure
 *
 * This operation should be used for scheduled scan offload to
 * the hardware. Every time scan results are available, the
 * driver should report scan results event for wpa_supplicant
 * which will eventually request the results with
 * wpa_driver_get_scan_results2(). This operation is optional
 * and if not provided or if it returns -1, we fall back to
 * normal host-scheduled scans.
 */
static int wpa_driver_nrc_sched_scan(void *priv, struct wpa_driver_scan_params *params)
{
	return -1;
}

/**
 * stop_sched_scan - Request the driver to stop a scheduled scan
 * @priv: Private driver interface data
 * Returns: 0 on success, -1 on failure
 *
 * This should cause the scheduled scan to be stopped and
 * results should stop being sent. Must be supported if
 * sched_scan is supported.
 */
 static int wpa_driver_nrc_stop_sched_scan(void *priv)
{
	return -1;
}

/**
 * poll_client - Probe (null data or such) the given station
 * @priv: Private driver interface data
 * @own_addr: MAC address of sending interface
 * @addr: MAC address of the station to probe
 * @qos: Indicates whether station is QoS station
 *
 * This function is used to verify whether an associated station is
 * still present. This function does not need to be implemented if the
 * driver provides such inactivity polling mechanism.
 */
static void wpa_driver_nrc_poll_client(void *priv, const u8 *own_addr,
	const u8 *addr, int qos)
{
}

/**
 * radio_disable - Disable/enable radio
 * @priv: Private driver interface data
 * @disabled: 1=disable 0=enable radio
 * Returns: 0 on success, -1 on failure
 *
 * This optional command is for testing purposes. It can be used to
 * disable the radio on a testbed device to simulate out-of-radio-range
 * conditions.
 */
static int wpa_driver_nrc_radio_disable(void *priv, int disabled)
{
	return -1;
}

/**
 * switch_channel - Announce channel switch and migrate the GO to the
 * given frequency
 * @priv: Private driver interface data
 * @settings: Settings for CSA period and new channel
 * Returns: 0 on success, -1 on failure
 *
 * This function is used to move the GO to the legacy STA channel to
 * avoid frequency conflict in single channel concurrency.
 */
static int wpa_driver_nrc_switch_channel(void *priv, struct csa_settings *settings)
{
	return -1;
}

/**
 * add_tx_ts - Add traffic stream
 * @priv: Private driver interface data
 * @tsid: Traffic stream ID
 * @addr: Receiver address
 * @user_prio: User priority of the traffic stream
 * @admitted_time: Admitted time for this TS in units of
 *	32 microsecond periods (per second).
 * Returns: 0 on success, -1 on failure
 */
static int wpa_driver_nrc_add_tx_ts(void *priv, u8 tsid, const u8 *addr, u8 user_prio,
	u16 admitted_time)
{
	return -1;
}

/**
 * del_tx_ts - Delete traffic stream
 * @priv: Private driver interface data
 * @tsid: Traffic stream ID
 * @addr: Receiver address
 * Returns: 0 on success, -1 on failure
 */
static int wpa_driver_nrc_del_tx_ts(void *priv, u8 tsid, const u8 *addr)
{
	return -1;
}

/**
 * get_survey - Retrieve survey data
 * @priv: Private driver interface data
 * @freq: If set, survey data for the specified frequency is only
 *	being requested. If not set, all survey data is requested.
 * Returns: 0 on success, -1 on failure
 *
 * Use this to retrieve:
 *
 * - the observed channel noise floor
 * - the amount of time we have spent on the channel
 * - the amount of time during which we have spent on the channel that
 *   the radio has determined the medium is busy and we cannot
 *   transmit
 * - the amount of time we have spent receiving data
 * - the amount of time we have spent transmitting data
 *
 * This data can be used for spectrum heuristics. One example is
 * Automatic Channel Selection (ACS). The channel survey data is
 * kept on a linked list on the channel data, one entry is added
 * for each survey. The min_nf of the channel is updated for each
 * survey.
 */
static int wpa_driver_nrc_get_survey(void *priv, unsigned int freq)
{
	return -1;
}

/**
 * status - Get driver interface status information
 * @priv: Private driver interface data
 * @buf: Buffer for printing tou the status information
 * @buflen: Maximum length of the buffer
 * Returns: Length of written status information or -1 on failure
 */
static int wpa_driver_nrc_status(void *priv, char *buf, size_t buflen)
{
	return -1;
}

/**
 * set_mac_addr - Set MAC address
 * @priv: Private driver interface data
 * @addr: MAC address to use or %NULL for setting back to permanent
 * Returns: 0 on success, -1 on failure
 */
static int wpa_driver_nrc_set_mac_addr(void *priv, const u8 *addr)
{
	return -1;
}

/**
 * set_param - Enable/disable NDP Probe Request
 * @priv: private driver interface data from init()
 * @enabled: 1 = NDP Preq enabled, 0 = disabled
 *
 * Returns: 0 on success, -1 on failure
 *
 * Only For 802.11ah. Enable/Disable NDP Probe Req
 */
static int wpa_driver_nrc_set_ndp_request(void *priv,  int enabled)
{
	struct wim_builder *wb = NULL;
	struct nrc_wpa_if *intf = (struct nrc_wpa_if *)(priv);

	wb = wim_builder_create(intf->vif_id, WIM_CMD_SET, WB_MIN_SIZE);
	wim_builder_append_u32(wb, WIM_TLV_NDP_PREQ, enabled);
	wim_builder_run_wm_wq(wb, true);
	return 0;
}

static int wpa_driver_nrc_hapd_send_eapol(void *priv, const u8 *addr,
			const u8 *data, size_t data_len, int encrypt, const u8 *own_addr,
			u32 flags)
{
	const uint8_t rfc1042[6] = { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00 };
	struct nrc_wpa_if *intf = (struct nrc_wpa_if *)(priv);
	int qos = flags & WPA_STA_WMM;
	struct ieee80211_hdr *hdr = NULL;
	uint16_t len = 0, i = 0;
	union wpa_event_data event;

	event.eapol_tx_status.dst = addr;
	event.eapol_tx_status.data = data;
	event.eapol_tx_status.data_len = data_len;
	event.eapol_tx_status.ack = 1;

	len = sizeof(*hdr) + (qos ? 2 : 0) + sizeof(rfc1042)
					+ 2 /* QoS Control Field */ + data_len;
	hdr = os_zalloc(len);

	if (!hdr) {
		E(TT_WPAS, TAG "Failed to allocate EAPOL frame\n");
		return -1;
	}
	uint8_t *pos = (uint8_t *) hdr;
	hdr->frame_control = IEEE80211_FC(WLAN_FC_TYPE_DATA, WLAN_FC_STYPE_DATA)
			| host_to_le16(WLAN_FC_FROMDS)
			| (encrypt ? host_to_le16(WLAN_FC_ISWEP << 4) : 0)
			| (qos ? host_to_le16(WLAN_FC_STYPE_QOS_DATA << 4) : 0);

	memcpy(hdr->IEEE80211_DA_FROMDS, 	addr, ETH_ALEN);
	memcpy(hdr->IEEE80211_BSSID_FROMDS,	own_addr, ETH_ALEN);
	memcpy(hdr->IEEE80211_SA_FROMDS,	own_addr, ETH_ALEN);
	pos += sizeof(*hdr);

	if (qos) {
		/* QoS Control Field */
		*pos++ = 7;
		*pos++ = 0;
	}
	os_memcpy(pos, rfc1042, sizeof(rfc1042));
	pos += sizeof(rfc1042);
	WPA_PUT_BE16(pos, ETH_P_PAE);
	pos += 2;
	os_memcpy(pos, data, data_len);
	pos += data_len;

	if (wpa_driver_nrc_send_frame(intf, 0, (void *) hdr, len) < 0) {
		E(TT_WPAS, TAG "Failed to send EAPOL frame\n");
		os_free(hdr);
		return -1;
	}
	os_free(hdr);
	wpa_supplicant_event(intf->wpa_supp_ctx, EVENT_EAPOL_TX_STATUS, &event);
	return 0;
}

static int wpa_driver_nrc_set_freq(void *priv, struct hostapd_freq_params *freq)
{
	struct nrc_wpa_if *intf = (struct nrc_wpa_if *)(priv);
	E(TT_WPAS, TAG "Change freq (freq=%d, channel=%d)\n", freq->freq,
		freq->channel);
	return 0;
}

void wpa_driver_wim_run(void *eloop_ctx, void *timeout_ctx)
{
	struct wim_builder *wb = (struct wim_builder *) timeout_ctx;
	wim_builder_run_wm_wq(wb, true);
}

#if defined(STANDARD_11AH)
bool g_start_ap = false;
#endif
static int wpa_driver_nrc_set_ap(void *priv, struct wpa_driver_ap_params *p)
{
	struct nrc_wpa_if *intf = (struct nrc_wpa_if *)(priv);
	struct wim_builder *wb = NULL;
	uint8_t *frame = NULL, *pos = NULL;
	struct wim_erp_param erp = {0, };
	int i = 0, size = 0;
	const int ADDITIONAL_IE_LEN = 100;
	uint8_t bssid[ETH_ALEN];
	uint16_t bi = p->beacon_int;
	int frame_len = p->head_len + p->tail_len;

#if defined(STANDARD_11AH)
	if (g_start_ap) {
		//11ah doesn't care ERP. So no need to update param whenever STA connection
		I(TT_WPAS, TAG "%d Already Set for SoftAP\n", intf->vif_id);
		return 0;
	}
#endif

	I(TT_WPAS, TAG "%d Set AP\n", intf->vif_id);

	if (frame_len > WM_MAX_SIZE ||
		frame_len < IEEE80211_HDRLEN) {
		E(TT_WPAS, TAG "Invalid frame (len: %d)\n", frame_len);
		return -1;
	}

	wb = wim_builder_create(intf->vif_id, WIM_CMD_SET, WM_MAX_SIZE);

	if (!wb)
		return -1;

	wpa_driver_nrc_get_bssid(intf, bssid);
	wim_builder_append(wb, WIM_TLV_BSSID, bssid, ETH_ALEN);
#if defined(STANDARD_11AH)
	wim_builder_append_u16(wb, WIM_TLV_BCN_INTV,
				bi * DEF_CFG_S1G_SHORT_BEACON_COUNT);
	wim_builder_append_u16(wb, WIM_TLV_SHORT_BCN_INTV, p->beacon_int);
#else
	wim_builder_append_u16(wb, WIM_TLV_BCN_INTV, bi);
#endif
	wim_builder_append_u32(wb, WIM_TLV_DTIM_PERIOD, p->dtim_period);
	wim_builder_append(wb, WIM_TLV_SSID, (void *) p->ssid, p->ssid_len);

	if (p->ht_opmode >= 0) // HT in use
		wim_builder_append_u16(wb, WIM_TLV_HT_MODE, p->ht_opmode);

	erp.use_11b_protection 	= p->cts_protect;
	erp.use_short_preamble 	= p->preamble;
	erp.use_short_slot 		= p->short_slot_time;
	wim_builder_append(wb, WIM_TLV_ERP_PARAM, &erp, sizeof(erp));

	if (p->basic_rates) {
		//TODO: Base Rate Set
	}

	if (p->proberesp)
		wim_builder_append(wb, WIM_TLV_PROBE_RESP, p->proberesp, p->proberesp_len);

	if (p->freq) {
		struct wim_channel_param wp = {p->freq->freq, };
		wim_builder_append(wb, WIM_TLV_CHANNEL, &wp, sizeof(wp));
	}

	pos = frame = (uint8_t *) os_malloc(p->head_len + p->tail_len
						+ ADDITIONAL_IE_LEN);

	if (!frame) {
		E(TT_WPAS, TAG "Failed to allocate beacon\n");
		return -1;
	}
	os_memcpy(pos, p->head, p->head_len);
	pos += p->head_len;
	os_memcpy(pos, p->tail, p->tail_len);
	pos += p->tail_len;
	if (p->privacy) {
	}

	wim_builder_append(wb, WIM_TLV_BEACON, frame, pos - frame);
	os_free(frame);
	wim_builder_append_u8(wb, WIM_TLV_BEACON_ENABLE, 1);

#if defined(STANDARD_11AH)
#if defined(NRC_USER_APP)
		wpa_driver_notify_event_to_app(EVENT_INTERFACE_ENABLED);
#endif
		g_start_ap = true;
#endif

	//eloop_register_timeout(0, 0, wpa_driver_wim_run, (void *) intf, (void *) wb);
	wim_builder_run_wm_wq(wb, true);

	return 0;
}

struct wpa_driver_event_item {
	int id;
	size_t len;
	uint8_t data[0];
};

#if defined(NRC_USER_APP)
#include "nrc_user_app.h"
extern QueueHandle_t    x_wlan_mgr_queue_handle;

void wpa_driver_notify_event_to_app(int event_id)
{
	WLAN_MESSAGE *message = NULL;
	MSG_ID_FORMAT *message_id = NULL;

	I(TT_WPAS, TAG "New event sent to APP (id :%d)\n", event_id);

	message = (WLAN_MESSAGE *) pvPortMalloc(WLAN_MESSAGE_SIZE);
	if (message == NULL) {
		return;
	}

	message_id = (MSG_ID_FORMAT *) pvPortMalloc(WLAN_CONNECT_FM_SIZE);
	if (message_id == NULL) {
		if (message != NULL) {
			vPortFree(message);
		}
		return;
	}

	/* Clear message */
	memset (message, 0, sizeof(WLAN_MESSAGE));
	memset (message_id, 0, sizeof(MSG_ID_FORMAT));

	switch (event_id) {
		case EVENT_ASSOC:
			message_id->id = WLAN_EVT_CONNECT_SUCCESS;
			break;
		case EVENT_DISASSOC:
		case EVENT_DEAUTH:
			message_id->id = WLAN_EVT_DISCONNECT;
			break;
		case EVENT_INTERFACE_ENABLED:
			message_id->id = WLAN_EVT_START_SOFT_AP;
			break;
		case EVENT_SCHED_SCAN_STOPPED:
			message_id->id = WLAN_EVT_SCAN_DONE;
			break;
		case EVENT_ASSOC_REJECT:
		default:
			E(TT_WPAS, TAG "Unknown event id for APP (%d)\n", event_id);
			return;
	}

	message->type = WLAN_EVENT;
	message->len = WLAN_ID_FM_SIZE;
	message->value = (void *)message_id;

	BaseType_t ret = xQueueSend( x_wlan_mgr_queue_handle, (void *)message, (TickType_t) portMAX_DELAY);

	if (message != NULL) {
		vPortFree(message);
	}

	if(ret != pdTRUE){
		E(TT_WPAS, TAG "[%s] Enqueue Fail (Event:%d)!!\n", __func__, event_id);
	}
}
#endif //#if defined(NRC_USER_APP)
void wpa_driver_notify_vevent_to_app(int cmd, uint8_t* data, uint16_t len)
{
	size_t hex_len = 2 * len + 1;
	char *hex = os_malloc(hex_len);
	wpa_snprintf_hex(hex, hex_len, data, len);
	I(TT_WPAS, TAG "^%s [TBD] data_len: %d \n", __func__, len);
	I(TT_WPAS, TAG "%s", hex);
	os_free(hex);
}

static int wpa_driver_notify_txq_to_wim(struct nrc_wpa_if *intf)
{
	return 0;
}

void wpa_driver_sta_sta_cmd(struct nrc_wpa_if* intf, int cmd)
{
	struct wim_builder *wb = NULL;
	uint8_t bssid[ETH_ALEN] = {0,};
	uint16_t aid = 0;
	struct wim_sta_param p = {cmd, {}, 0, 0, 0};

	// On AP mode wpa_driver_nrc_sta_add() will handle WIM_STA_CMD_ADD
	if (intf->is_ap)
		return;

	wpa_driver_nrc_get_bssid(intf, bssid);
	os_memcpy(p.addr, bssid, ETH_ALEN);
	p.aid = 0; // AP

	if (cmd == WIM_STA_CMD_REMOVE) {
		intf->associated = false;
		wpa_driver_nrc_get_bssid(intf, bssid);
	}

	wb = wim_builder_create(intf->vif_id, WIM_CMD_SET, WB_MIN_SIZE);
	wim_builder_append(wb, WIM_TLV_BSSID, bssid, ETH_ALEN);
	wim_builder_append_u16(wb, WIM_TLV_AID, intf->sta.aid);

	/* Add beacon interval for quick setting of beacon monitor */
	wim_builder_append_u16(wb, WIM_TLV_SHORT_BCN_INTV, intf->bss.beacon_int);

	wim_builder_run_wm_wq(wb, true);

	wb = wim_builder_create(intf->vif_id, WIM_CMD_STA_CMD, WB_MIN_SIZE);
	wim_builder_append(wb, WIM_TLV_STA_PARAM, &p, sizeof(p));
	wim_builder_run_wm_wq(wb, true);
}

void send_keep_alive(struct nrc_wpa_if* intf)
{
	uint16_t len = sizeof(struct ieee80211_hdr);
	uint8_t *frame;
	struct ieee80211_hdr *hdr;

	if (intf->bss.qos)
		len += 2; /* for QoS Ctrl */

	frame = os_zalloc(len);
	if (!frame) {
		E(TT_WPAS, TAG "Failed to allocate Keep Alive frame\n");
		return;
	}

	hdr = (struct ieee80211_hdr *) frame;

	if (intf->bss.qos)
		hdr->frame_control = IEEE80211_FC(WLAN_FC_TYPE_DATA,
			WLAN_FC_STYPE_QOS_NULL);
	else
		hdr->frame_control = IEEE80211_FC(WLAN_FC_TYPE_DATA,
			WLAN_FC_STYPE_NULLFUNC);

	hdr->frame_control |= host_to_le16(WLAN_FC_TODS);

	os_memcpy(hdr->addr1, intf->bss.bssid, 6);
	os_memcpy(hdr->addr2, intf->addr, 6);
	os_memcpy(hdr->addr3, intf->bss.bssid, 6);

	nrc_transmit(intf, frame, len);
	os_free(frame);
}

static void start_keep_alive(struct nrc_wpa_if* intf);

static void keep_alive_timeout(void *eloop_data, void *user_ctx)
{
	struct nrc_wpa_if* intf = (void *) user_ctx;

	if (!intf->associated)
		return;

	start_keep_alive(intf);
}

static uint64_t max_idle_to_ms(uint16_t v)
{
	uint64_t res = v;
#if defined(STANDARD_11AH)
	/* Convert the value in USF (Unified scaling factor)
	 * : 3 MSB is used for scaling factor (1->10, 2->100, 3->1000)
	 */
	res = (v & ~0xC000) * pow(10, (v >> 14) & 0x3);
#endif
	/* Max BSS Idle Period is in unit of 1000 TU */
	return (res * 1024);
}

static void start_keep_alive(struct nrc_wpa_if* intf)
{
	struct os_time now, past, next;
	uint64_t max_idle, past_ms;
	const uint32_t off_ms = 1000;

	max_idle = max_idle_to_ms(intf->bss.max_idle);

	/* MAX BSS Idle is a unit of 1000 TU, should be over 1024 ms */
	if (max_idle < 1024) {
		I(TT_WPAS, TAG "Wrong max_idle is received (%d)",
			(int) max_idle);
		return;
	}
	/* STA should transmit NULL frame before AP expires STA's */
	max_idle -= off_ms;

	os_get_time(&now);
	os_time_sub(&now, &intf->bss.last_tx_time, &past);
	past_ms = (past.sec * 1000) + (past.usec / 1000);

	next.sec = max_idle / 1000;
	next.usec = (max_idle % 1000) * 1000;

	if (past_ms > max_idle) {
		send_keep_alive(intf);
	} else {
		next.sec = intf->bss.last_tx_time.sec + next.sec - now.sec;
		next.usec = intf->bss.last_tx_time.usec + next.sec - now.usec;
		/* Prevent being 0 */
		next.usec = max(next.usec, 10000);
	}

	V(TT_WPAS, TAG "Next Keep Alive timer will trigger after %ds %dms \n",
			next.sec, next.usec / 1000);

	eloop_register_timeout(next.sec, next.usec,
			keep_alive_timeout, 0, intf);
}

static void stop_keep_alive(struct nrc_wpa_if* intf)
{
	intf->bss.max_idle = 0;
	eloop_cancel_timeout(keep_alive_timeout, ELOOP_ALL_CTX, ELOOP_ALL_CTX);
}

void wpa_driver_sta_sta_add(struct nrc_wpa_if* intf)
{
	wpa_driver_sta_sta_cmd(intf, WIM_STA_CMD_ADD);

	if (intf->bss.max_idle > 0)
		start_keep_alive(intf);
}

void wpa_driver_sta_sta_remove(struct nrc_wpa_if* intf)
{
	stop_keep_alive(intf);
	wpa_driver_sta_sta_cmd(intf, WIM_STA_CMD_REMOVE);
}

static void wpa_driver_set_channel(void *priv, uint16_t freq)
{
	struct nrc_wpa_if *intf = (struct nrc_wpa_if *)(priv);
	struct wim_builder *wb = wim_builder_create(intf->vif_id, WIM_CMD_SET, WB_MIN_SIZE);
	struct wim_channel_param wp = {freq, };
	wim_builder_append(wb, WIM_TLV_CHANNEL, &wp, sizeof(wp));
	wim_builder_run_wm_wq(wb, true);
}

static int wpa_driver_nrc_authenticate(void *priv, struct wpa_driver_auth_params *p)
{
	struct nrc_wpa_if *intf = (struct nrc_wpa_if *)(priv);
	struct ieee80211_mgmt mgmt = {0,};
	const int len = offsetof(struct ieee80211_mgmt, u.auth.variable);

	I(TT_WPAS, TAG "Authenticate to " MACSTR "(freq: %d)\n", MAC2STR(p->bssid),
				p->freq);
	intf->sta_type = WIM_STA_TYPE_STA;
	mgmt.frame_control =  IEEE80211_FC(WLAN_FC_TYPE_MGMT, WLAN_FC_STYPE_AUTH);

	os_memcpy(mgmt.da, p->bssid, ETH_ALEN);
	os_memcpy(mgmt.sa, intf->addr, ETH_ALEN);
	os_memcpy(mgmt.bssid, p->bssid, ETH_ALEN);

	mgmt.u.auth.auth_alg = (p->auth_alg == WPA_AUTH_ALG_SHARED) ? 1 : 0;
	mgmt.u.auth.auth_transaction = 1;
	mgmt.u.auth.status_code = 0;

	wpa_driver_set_channel(priv, p->freq);

	nrc_transmit(intf, (uint8_t *) &mgmt, len);

	return 0;
}

static int wpa_driver_stop_ap(void* priv)
{
	I(TT_WPAS, TAG "Stop AP \n");
	return 0;
}

void nrc_wpas_disconnect(int vif_id)
{
	struct nrc_wpa_if* intf = wpa_driver_get_interface(vif_id);
	wpa_supplicant_event(intf->wpa_supp_ctx, EVENT_DISASSOC, NULL);
}

const struct wpa_driver_ops wpa_driver_freertos_ops = {
	.name = "freeRTOS",
	.desc = "freeRTOS driver",
	.get_bssid = wpa_driver_nrc_get_bssid,
	.get_ssid = wpa_driver_nrc_get_ssid,
	.set_key = wpa_driver_nrc_set_key,
	.init = NULL,
	.deinit = wpa_driver_nrc_deinit,
	.set_param = wpa_driver_nrc_set_param,
	.set_countermeasures = wpa_driver_nrc_set_countermeasures,
	.deauthenticate = wpa_driver_nrc_deauthenticate,
	.associate = wpa_driver_nrc_associate,
	.add_pmkid = NULL,
	.remove_pmkid = wpa_driver_nrc_remove_pmkid,
	.flush_pmkid = NULL,
	.get_capa = wpa_driver_nrc_get_capa,
	.poll = NULL, //wpa_driver_nrc_poll,
	.get_ifindex = NULL,
	.get_ifname = wpa_driver_nrc_get_ifname,
	.get_mac_addr = wpa_driver_nrc_get_mac_addr,
	.set_operstate = wpa_driver_nrc_set_operstate,
	.mlme_setprotection = wpa_driver_nrc_mlme_setprotection,
	.get_hw_feature_data = wpa_driver_nrc_get_hw_feature_data,
	.send_mlme = wpa_driver_nrc_send_mgmt,
	.update_ft_ies = wpa_driver_nrc_update_ft_ies,
	.get_scan_results2 = wpa_driver_nrc_get_scan_results2,
	.set_country = wpa_driver_nrc_set_country,
	.get_country = wpa_driver_nrc_get_country,
	.global_init = wpa_driver_nrc_global_init,
	.global_deinit = wpa_driver_nrc_global_deinit,
	.init2 = wpa_driver_nrc_init,
	.get_interfaces = wpa_driver_nrc_get_interfaces,
	.scan2 = wpa_driver_nrc_scan2,
	.authenticate = wpa_driver_nrc_authenticate,
	.set_ap = wpa_driver_nrc_set_ap,
	.set_acl = wpa_driver_nrc_set_acl,
	.hapd_init = NULL,
	.hapd_deinit = NULL,
	.set_ieee8021x = NULL, // wpa_driver_nrc_set_ieee8021x,
	.set_privacy = NULL, // wpa_driver_nrc_set_privacy,
	.get_seqnum = NULL,
	.flush = NULL,
	.set_generic_elem = NULL, // wpa_driver_nrc_set_generic_elem,
	.read_sta_data = wpa_driver_nrc_read_sta_data,
	.hapd_send_eapol = wpa_driver_nrc_hapd_send_eapol,
	.sta_deauth = wpa_driver_nrc_sta_deauth,
	.sta_disassoc = wpa_driver_nrc_sta_disassoc,
	.sta_remove = wpa_driver_nrc_sta_remove,
	.hapd_get_ssid = NULL,
	.hapd_set_ssid = NULL,
	.hapd_set_countermeasures = NULL,
	.sta_add = wpa_driver_nrc_sta_add,
	.get_inact_sec = NULL,
	.sta_clear_stats = NULL,
	.set_freq = NULL,
	.set_rts = wpa_driver_nrc_set_rts,
	.set_frag = wpa_driver_nrc_set_frag,
	.sta_set_flags = NULL,
	.set_tx_queue_params = wpa_driver_nrc_set_tx_queue_params,
	.if_add = wpa_driver_nrc_if_add,
	.if_remove = wpa_driver_nrc_if_remove,
	.set_sta_vlan = NULL,
	.commit = NULL,
	.send_ether = NULL,
	.set_radius_acl_auth = wpa_driver_nrc_set_radius_acl_auth,
	.set_radius_acl_expire = wpa_driver_nrc_set_radius_acl_expire,
	.set_ap_wps_ie = NULL,
	.set_supp_port = wpa_driver_nrc_set_supp_port,
	.set_wds_sta = NULL,
	.send_action = wpa_driver_nrc_send_action,
	.send_action_cancel_wait = wpa_driver_nrc_send_action_cancel_wait,
	.remain_on_channel = wpa_driver_nrc_remain_on_channel,
	.cancel_remain_on_channel = wpa_driver_nrc_cancel_remain_on_channel,
	.probe_req_report = wpa_driver_nrc_probe_req_report,
	.deinit_ap = wpa_driver_nrc_deinit_ap,
	.deinit_p2p_cli = wpa_driver_nrc_deinit_p2p_cli,
	.suspend = wpa_driver_nrc_suspend,
	.resume = wpa_driver_nrc_resume,
	.signal_monitor = wpa_driver_nrc_signal_monitor,
	.send_frame = NULL,
	.get_noa = wpa_driver_nrc_get_noa,
	.set_noa = wpa_driver_nrc_set_noa,
	.set_p2p_powersave = wpa_driver_nrc_set_p2p_powersave,
	.ampdu = wpa_driver_nrc_ampdu,
	.get_radio_name = wpa_driver_nrc_get_radio_name,
	.send_tdls_mgmt = NULL,
	.tdls_oper = NULL,
	.wnm_oper = NULL,
	.set_qos_map = NULL,
	.br_add_ip_neigh = NULL,
	.br_delete_ip_neigh = NULL,
	.br_port_set_attr= NULL,
	.br_set_net_param= NULL,
	.set_wowlan = wpa_driver_nrc_set_wowlan,
	.signal_poll = wpa_driver_nrc_signal_poll,
	.set_authmode = NULL,
	.vendor_cmd = wpa_driver_nrc_vendor_cmd,
	.set_rekey_info = NULL,//wpa_driver_nrc_set_rekey_info,
	.sta_assoc = NULL,
	.sta_auth = NULL,
	.add_tspec = NULL,
	.add_sta_node = NULL,
	.sched_scan = wpa_driver_nrc_sched_scan,
	.stop_sched_scan = wpa_driver_nrc_stop_sched_scan,
	.poll_client = wpa_driver_nrc_poll_client,
	.radio_disable = wpa_driver_nrc_radio_disable,
	.switch_channel = wpa_driver_nrc_switch_channel,
	.add_tx_ts = wpa_driver_nrc_add_tx_ts,
	.del_tx_ts = wpa_driver_nrc_del_tx_ts,
	.tdls_enable_channel_switch = NULL,
	.tdls_disable_channel_switch = NULL,
	.start_dfs_cac = NULL,
	.stop_ap = wpa_driver_stop_ap,
	.get_survey = wpa_driver_nrc_get_survey,
	.status = wpa_driver_nrc_status,
	.roaming = NULL,
	.set_mac_addr = wpa_driver_nrc_set_mac_addr,
	.init_mesh = NULL,
	.join_mesh = NULL,
	.leave_mesh = NULL,
	.do_acs = NULL,
	.set_band = NULL,
	.get_pref_freq_list = NULL,
	.set_prob_oper_freq = NULL,
	.set_ndp_preq = wpa_driver_nrc_set_ndp_request,
};

#ifdef TAG
#undef TAG
#endif
