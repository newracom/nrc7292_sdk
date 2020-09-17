#include "system_common.h"
#include "utils/includes.h"
#include "utils/common.h"
#include "driver.h"
#include "../common/ieee802_11_common.h"
#include "driver_nrc_scan.h"
#if defined(NRC_USER_APP)
#include "driver_nrc.h"
#endif

#ifdef TAG
#undef TAG
#endif
#define TAG "wpa: "

struct nrc_scan_info * scan_init()
{
	struct nrc_scan_info *scan;
	scan = (struct nrc_scan_info *) os_zalloc(sizeof(*scan));

	if (!scan) {
		E(TT_WPAS, TAG "Failed to allocate scan\n");
		return NULL;
	}
	dl_list_init(&scan->scan_list);

	return scan;
}

static void scan_start_stop(struct nrc_scan_info *scan, bool start)
{
	I(TT_WPAS, TAG "Scan %s\n", start ? "start" : "stop");
	if (scan)
		scan->is_run = start;
}

void scan_start(struct nrc_scan_info *scan)
{
	if (!scan) {
		E(TT_WPAS, TAG "Unable to start scan\n");
		return;
	}

	scan_start_stop(scan, true);
	scan_flush(scan);
}

void scan_stop(struct nrc_scan_info *scan)
{
	scan_start_stop(scan, false);

	/* Determine scan is terminated before it scan last given channel */
	if (scan->curr_freq == scan->last_freq)
		scan->curr_freq = 0;

#if defined(NRC_USER_APP)
	wpa_driver_notify_event_to_app(EVENT_SCHED_SCAN_STOPPED);
#endif
}

static void dump_scan_entry(struct nrc_wpa_scan_res *res)
{
	V(TT_WPAS, TAG "scan entry bssid(" MACSTR "), freq(%d), bi(%d), "
		"caps(%d), q(%d), noise(%d), level(%d)\n",
		MAC2STR(res->res->bssid),
		res->res->freq,
		res->res->beacon_int,
		res->res->caps,
		res->res->qual,
		res->res->noise,
		res->res->level);

	wpa_hexdump(MSG_ERROR, "scan entry", res->res + 1,
		res->res->beacon_ie_len + res->res->ie_len);
}

static void cleanup_history(struct nrc_scan_info *scan)
{
	V(TT_WPAS, TAG "(hist) Clean up\n");
	scan->history_cnt = 0;
}

static bool add_history(struct nrc_scan_info *scan, struct ieee80211_mgmt *f)
{
	if (scan->history_cnt >= MAX_SCAN_HISTORY-1) {
		E(TT_WPAS, TAG "(hist) Failed to add an entry (full)\n");
		return false;
	}

	memcpy(scan->history[scan->history_cnt].bssid, f->bssid, ETH_ALEN);
	scan->history[scan->history_cnt].time = NOW;

	V(TT_WPAS, TAG "(hist) Add an entry (idx: %d, bssid: " MACSTR ")\n",
		scan->history_cnt, MAC2STR(scan->history[scan->history_cnt].bssid));
	scan->history_cnt++;

	return true;
}

static bool scan_add_entry(struct nrc_scan_info *scan, uint16_t freq,
				int8_t rssi, uint8_t* frame, uint16_t len)
{
	struct ieee80211_mgmt *mgmt = (struct ieee80211_mgmt *) frame;
	const int ie_offset = offsetof(struct ieee80211_mgmt, u.beacon.variable);
	uint16_t ie_len = len - ie_offset;
	struct nrc_wpa_scan_res *res;

	res = (struct nrc_wpa_scan_res *) os_malloc(sizeof(*res));

	if (!res) {
		E(TT_WPAS, TAG "Failed to allocate scan_res\n");
		return false;
	}

	res->res = os_zalloc(sizeof(*res->res) + ie_len);

	if (!res->res) {
		E(TT_WPAS, TAG "Failed to allocate scan entry (ie len: %d)\n", ie_len);
		return false;
	}

	os_memcpy(res->res->bssid, mgmt->sa, ETH_ALEN);
	res->res->freq = freq;
	res->res->beacon_int = mgmt->u.beacon.beacon_int;
	res->res->caps = mgmt->u.beacon.capab_info;
	res->res->qual = 0;
	res->res->noise = 0;
	os_memcpy(&res->res->tsf, mgmt->u.beacon.timestamp, 8);
	res->res->level = (rssi>-10)?-10:rssi;
	res->res->ie_len = ie_len;
	os_memcpy(res->res + 1, frame + ie_offset ,ie_len);
	dump_scan_entry(res);

	dl_list_add(&scan->scan_list, &res->list);

	return true;
}

/**
  * 1. If the number of scan list exceeds MAX_SCAN_SSID_LIST or,
  * 2. If the channel of DS PARAM IE is not same as that of rxinfo or,
  * 3. If SSID is not filtered,
  * consider valid and return true.
*/
static bool scan_is_valid(struct nrc_scan_info *scan, uint8_t ch, int8_t rssi,
				struct ieee802_11_elems* elems)
{
	if (dl_list_len(&scan->scan_list) >= MAX_SCAN_SSID_LIST) {
		E(TT_WPAS, TAG "Scan list is full (max: %d) \n",
			MAX_SCAN_SSID_LIST);
		return false;
	}
	return true;
}

static bool scan_check_hist(struct nrc_scan_info *scan, uint16_t freq,
		struct ieee80211_mgmt *mgmt)
{
	int i;

	/* scan history is cleared, hwne new freq. is given */
	/* if (scan->curr_freq && freq != scan->curr_freq) */ //FIXME: temporary Jun 27 2019
		cleanup_history(scan);

	scan->curr_freq = freq;

	for (i = 0; i < scan->history_cnt; i++) {
		/* Found this AP on history, no need to continue */
		if (memcmp(scan->history[i].bssid, mgmt->bssid, 6) == 0) {
			I(TT_WPAS, TAG "(hist) Ignore duplicated (bssid: " MACSTR ") \n",
				MAC2STR(scan->history[i].bssid));
			return false;
		}
	}

	return true;
}

static bool scan_check_ies(struct nrc_scan_info *scan, const uint8_t ch,
				struct ieee802_11_elems *e)
{
	if (!e)
		return false;

	if (e->ds_params && e->ds_params[0] != ch) {
		V(TT_WPAS, TAG "Incorrect DSPARAM IE (ie: %d/actual: %d)",
			e->ds_params[0], ch);
		return false;
	}

	if (!e->ssid || !e->supp_rates) {
		V(TT_WPAS, TAG "Essential IE is missing (ssid: %d, srates: %d)",
			!!(e->ssid), !!(e->supp_rates));
		return false;
	}

	return true;
}

static bool scan_check_dup(struct nrc_scan_info *scan, uint8_t bssid[ETH_ALEN])
{
	struct nrc_wpa_scan_res *res, *tmp;

	dl_list_for_each_safe(res, tmp, &scan->scan_list,
			struct nrc_wpa_scan_res, list) {
		if (res && res->res) {
			if (memcmp(res->res->bssid, bssid, ETH_ALEN) == 0)
				return false;
		}
	}

	return true;
}

uint16_t scan_resume_from(struct nrc_scan_info *scan)
{
	return scan->curr_freq;
}

int scan_add(struct nrc_scan_info *scan, uint16_t freq, int8_t rssi,
				uint8_t* frame, uint16_t len)
{
	int ie_offset = offsetof(struct ieee80211_mgmt, u.beacon.variable);
	struct ieee80211_mgmt *mgmt = (struct ieee80211_mgmt *) frame;
	uint8_t *ie = frame + ie_offset, ch = 0;
	uint16_t ie_len = len - ie_offset;
	struct ieee802_11_elems elems;

	ieee80211_freq_to_chan(freq, &ch);
	ieee802_11_parse_elems(ie, ie_len, &elems, 1 /* show error */);

	if (!scan || !scan->is_run) {
		V(TT_WPAS, TAG "Scan is not running");
		return -EINVAL;
	}

	if (!frame || len < ie_offset ||
		!scan_check_hist(scan, freq, mgmt) ||
		!scan_check_dup(scan, mgmt->sa) ||
		!scan_check_ies(scan, ch, &elems))
		return -EINVAL;

	/* When the channel seems like changed, clean up all history scan */
	if (!scan_add_entry(scan, freq, rssi, frame, len) ||
		!add_history(scan, mgmt))
		return -ENOMEM;

	I(TT_WPAS, TAG "new AP scanned (bssid: " MACSTR ", ssid: %s) \n",
		MAC2STR(mgmt->sa),
		!!(elems.ssid) ? wpa_ssid_txt(elems.ssid, elems.ssid_len) : "N/A");

	return 0;
}

void scan_config(struct nrc_scan_info *scan, struct wpa_driver_scan_params *p,
		uint16_t last_freq)
{
	I(TT_WPAS, TAG "scan config is updated (last freq: %d) \n", last_freq);

	if (p)
		scan->params = *p;
	scan->last_freq = last_freq;
}

struct wpa_scan_results* get_scan_results(struct nrc_scan_info *scan)
{
	struct nrc_wpa_scan_res *res, *tmp;
	int cnt = 0, i = 0;
	struct wpa_scan_results* results;

	results = (struct wpa_scan_results *) os_zalloc(sizeof(*results));
	cnt = dl_list_len(&scan->scan_list);

	if (!results) {
		E(TT_WPAS, TAG "Failed to allocate wpa_scan_results\n");
		return NULL;
	}

	if (!cnt) {
		results->num = 0;
		return results;
	}
	results->res = os_zalloc(sizeof(*results->res) * cnt);

	if (!results->res) {
		E(TT_WPAS, TAG "Failed to allocate scan item\n");
		os_free(results);
		return NULL;
	}

	dl_list_for_each_safe(res, tmp, &scan->scan_list, struct nrc_wpa_scan_res, list) {
		results->res[i++] = res->res;
		// the inst of struct wpa_scan_res is being freed on
		// _wpa_supplicant_event_scan_results().
		dl_list_del(&res->list);
		os_free(res);
	}
	results->num = cnt;

	os_get_reltime(&results->fetch_time);

	return results;
}

static void flush_scan_list(struct dl_list* l)
{
	struct nrc_wpa_scan_res *res = NULL, *tmp = NULL;

	if (dl_list_empty(l))
		return;

	dl_list_for_each_safe(res, tmp, l, struct nrc_wpa_scan_res, list) {
		if (res) {
			if (res->res)
				os_free(res->res);
			dl_list_del(&res->list);
			os_free(res);
		}
	}
}

void scan_flush(struct nrc_scan_info *scan)
{
	E(TT_WPAS, TAG "Flush scan\n");
	flush_scan_list(&scan->scan_list);
	os_memset(&scan->params, 0, sizeof(scan->params));
}

void scan_deinit(struct nrc_scan_info *scan)
{
	scan_stop(scan);
	os_free(scan);
	scan = NULL;
}

#undef TAG
