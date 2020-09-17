#include "system.h"
#include "system_common.h"
#include "driver_nrc_debug.h"
#include "common.h"

#ifdef TAG
#undef TAG
#endif
#define TAG "wpa: "

void wpa_driver_debug_frame(uint8_t* frame, uint16_t len)
{
	struct ieee80211_mgmt *mgmt = (void *) frame;

	if (len < IEEE80211_HDRLEN)
		return;

	I(TT_WPAS, TAG "Frame (fc:%d, dur:%d,"
		"DA:" MACSTR ",SA:" MACSTR ",BSSID:" MACSTR
		"seq:%d) \n", mgmt->frame_control, mgmt->duration,
		MAC2STR(mgmt->da), MAC2STR(mgmt->sa), MAC2STR(mgmt->bssid),
		mgmt->seq_ctrl);
}

#define W2A(x) case WPA_ALG_##x: return #x
const char* wpa_driver_alg_str(enum wpa_alg alg)
{
	switch (alg) {
		W2A(NONE); W2A(WEP); W2A(TKIP);
		W2A(CCMP); W2A(IGTK); W2A(PMK);
		W2A(GCMP); W2A(GCMP_256); W2A(CCMP_256);
		default: return "Unknown";
	}
}
#undef W2A

void wpa_driver_debug_assoc_params(struct wpa_driver_associate_params *p)
{
	if (!p)
		return;

	I(TT_WPAS, TAG "Assoc param (auth:%d, disableHT:%d, drop_unenc:%d,"
		"bint:%d, bg scan:%d, fixed bssid:%d, fixed freq:%d, \n"
		"        pairwise suite:%d, group suite:%d, key_mgmt suite:%d)\n",
		p->auth_alg, p->disable_ht, p->drop_unencrypted, p->beacon_int,
		p->bg_scan_period, p->fixed_bssid, p->fixed_freq, p->pairwise_suite,
		p->group_suite, p->key_mgmt_suite);
}

void wpa_driver_debug_key(struct nrc_wpa_key *key)
{
	I(TT_WPAS, TAG "Key (tsc:%lld, ix:%d, len:%d, cipher:%d)\n"
		,key->tsc, key->ix, key->key_len, key->cipher);
}
