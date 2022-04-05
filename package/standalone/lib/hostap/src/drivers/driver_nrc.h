/*
 * Copyright (c) 2016-2021 Newracom, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _DRIVER_NRC_H_
#define _DRIVER_NRC_H_

#include <stdbool.h>
#include "system_common.h"
#include "driver.h"
#include "driver_nrc_tx.h"
#include "nrc-wim-types.h"

struct wpa_supplicant;
struct nrc_wpa;

//#define NRC_MAC_TEST_SELF_SCAN

#define NRC_WPA_NUM_INTERFACES			(2)
#define NRC_WPA_INTERFACE_NAME_0		("wlan0")
#define NRC_WPA_INTERFACE_NAME_1		("wlan1")

#if defined(MAX_STA)
#define NRC_WPA_SOFTAP_MAX_STA			MAX_STA
#else
#define NRC_WPA_SOFTAP_MAX_STA			(1024)
#endif /* !defined(MAX_STA) */
#define NRC_WPA_NUM_BEACON_MISS_ALLOWED	(20)
#define NRC_WPA_MIN_DURATION_BEACON_MISS_ALLOWED 	(3)	// In second
#define NRC_WPA_MAX_KEY_LEN 			(32)
#define NRC_WPA_MAX_KEY_INDEX 			(6)

#define _FC(T,ST)	IEEE80211_FC( WLAN_FC_TYPE_##T , WLAN_FC_STYPE_##ST )

#define ETH_P_AARP			(0x80F3)
#define ETH_P_IPX			(0x8137)
#define ETH_P_802_3_MIN		(0x0600)
#define HLEN_8023	(sizeof(struct ieee8023_hdr))

#define WLAN_BA_ADDBA_REQUEST		0	/* ADDBA request */
#define WLAN_BA_ADDBA_RESPONSE		1	/* ADDBA response */
#define WLAN_BA_DELBA				2	/* DELBA request */

#define WLAN_BA_NO_AMSDU			0
#define WLAN_BA_AMSDU				1
#define WLAN_BA_POLICY_IMM_BA		1
#define WLAN_BA_MAX_BUF				8
#define WLAN_REASON_END_BA 			37

#define WLAN_ACTION_S1G_TWT_SETUP			6
#define WLAN_ACTION_S1G_TWT_TEARDOWN		7
#define WLAN_ACTION_S1G_TWT_INFO			11

#if defined(INCLUDE_BD_SUPPORT)
#define NRC_WPA_BD_HEADER_LENGTH	16
#define NRC_WPA_BD_MAX_DATA_LENGTH	546
#endif /* defined(INCLUDE_BD_SUPPORT) */


extern uint8_t g_standalone_addr[6];

#define WLAN_CCMP_MIC_LEN	8
#define WLAN_BIP_MIC_LEN	8
#define WLAN_CCMP_KEY_LEN_BIT	128
#define WLAN_BIP_KEY_LEN_BIT	128

#define NRC_WPA_BUFFER_ALLOC_TRY (30)

enum ccmp_key_index {
	WLAN_KEY_PTK =0,
	WLAN_KEY_GTK_1 =1,
	WLAN_KEY_GTK_2 =2,
	WLAN_KEY_GTK_3 =3,
	WLAN_KEY_IGTK_4 =4,
	WLAN_KEY_IGTK_5 =5,
};

enum block_ack_state {
	BLOCK_ACK_INVALID	= 0, //0
	BLOCK_ACK_TX		= BIT(0), //1
	BLOCK_ACK_RX		= BIT(1), //2
};

struct nrc_wpa_key {
	uint64_t tsc;	/* key transmit sequence counter */
	uint16_t ix;	/* Key Index */
	uint8_t cipher; /* WIM Cipher */
	uint8_t addr[ETH_ALEN];
	uint8_t key[NRC_WPA_MAX_KEY_LEN];
	uint16_t key_len;
	bool is_set[NRC_WPA_MAX_KEY_INDEX];
};

#define MAX_TID 8
struct nrc_wpa_sta {
	struct nrc_wpa_key		key;
	uint8_t 				addr[6];
	uint16_t				aid;
	uint8_t 				state;
	bool					qos;
	uint16_t 				last_mgmt_stype;
	enum block_ack_state 	block_ack[MAX_TID];
};

struct nrc_wpa_bss {
	uint8_t					bssid[6];
	uint8_t					ssid[32];
	uint8_t					ssid_len;
	struct nrc_wpa_key		broadcast_key[2];
	uint16_t 				beacon_int;
	struct os_time 			last_beacon_update;
	bool 					authorized_1x;
	uint16_t				max_idle;
	struct os_time 			last_tx_time;
	bool					qos;
};

struct nrc_wpa_if {
	int	vif_id;
	int sta_type;
	char if_name[6];
	uint8_t addr[6];
	void *wpa_supp_ctx;
	bool associated;
	bool is_ap;
	bool pending_rx_mgmt;
	struct nrc_wpa_sta sta;
	struct nrc_wpa_bss bss;
	struct nrc_scan_info* 	scan;
	struct hostapd_hw_modes* hw_modes;
	struct nrc_wpa	*global;
	struct nrc_wpa_sta *ap_sta[NRC_WPA_SOFTAP_MAX_STA];
	int num_ap_sta;
	int key_mgmt ;
};

struct nrc_wpa {
	struct wpa_supplicant	*wpa_s;
	struct nrc_wpa_if 	*intf[NRC_WPA_NUM_INTERFACES];
};

struct nrc_wpa_rx_data {
	struct nrc_wpa_if* intf;
	struct nrc_wpa_sta* sta;
	LMAC_RXHDR *rxh;
	RXINFO	*rxi;
	uint16_t len;
	union {
		uint8_t *frame;
		struct ieee80211_mgmt *mgmt;
		struct ieee80211_hdr *hdr;
	} u;
	bool is_8023;
	uint16_t offset_8023;
	SYS_BUF* buf;
	uint32_t ref_time;
};

struct nrc_wpa_rx_event {
	uint8_t *frame;
	uint16_t len;
	uint16_t freq;
	void* pv;
	uint16_t subtype;
	uint32_t ref_time;
};

struct nrc_wpa_log_event {
	char *msg;
	uint16_t len;
	int level;
};

#if defined(INCLUDE_BD_SUPPORT_TARGET_VERSION)
#define NRC_DRIVER_BD_MAX_CH_LIST		45
struct wpa_bd_ch_table {
	uint16_t    s1g_freq;
	uint16_t    nons1g_freq;
	uint8_t     s1g_freq_index;
	uint16_t    nons1g_freq_index;
};

struct wpa_bd_supp_param {
	uint8_t num_ch;
	uint8_t s1g_ch_index[NRC_DRIVER_BD_MAX_CH_LIST];
	uint16_t nons1g_ch_freq[NRC_DRIVER_BD_MAX_CH_LIST];
};
#endif /* defined(INCLUDE_BD_SUPPORT_TARGET_VERSION) */

#if defined(INCLUDE_BD_SUPPORT)
struct nrc_wpa_bdf {
	uint8_t	ver_major;
	uint8_t	ver_minor;
	uint16_t total_len;

	uint16_t num_data_groups;
	uint16_t reserved[4];
	uint16_t checksum_data;

	uint8_t data[NRC_WPA_BD_MAX_DATA_LENGTH];
};
#endif /* defined(INCLUDE_BD_SUPPORT) */

SYS_BUF * alloc_sys_buf_try(int hif_len, int nTry);

struct nrc_wpa_if *wpa_driver_get_interface(int vif);
struct nrc_wpa_sta* nrc_wpa_find_sta(struct nrc_wpa_if *intf,
						const uint8_t addr[ETH_ALEN]);
struct nrc_wpa_key *nrc_wpa_get_key_by_key_idx(struct nrc_wpa_if *intf,
						int key_idx);
struct nrc_wpa_key *nrc_wpa_get_key_by_addr(struct nrc_wpa_if *intf,
						const uint8_t *addr);
void wpa_driver_sta_sta_add(struct nrc_wpa_if* intf);
void wpa_driver_set_associate_status(int status);
int wpa_driver_get_associate_status(void);
void wpa_driver_notify_event_to_app(enum wpa_event_type event_id, uint32_t data_len, uint8_t* data);
void wpa_driver_notify_vevent_to_app(int event_id, uint32_t data_len, uint8_t* data);

void wpa_driver_sta_sta_remove(struct nrc_wpa_if* intf);
int wpas_l2_packet_filter(uint8_t *buffer, int len);
void wpa_driver_clear_key_all(struct nrc_wpa_if *intf);
int nrc_transmit(struct nrc_wpa_if* intf, uint8_t *frm, const uint16_t len);
int nrc_transmit_pmf(struct nrc_wpa_if* intf, uint8_t *frm, const uint16_t len,
		const uint16_t data_len, struct nrc_wpa_key *wpa_key);
int nrc_raw_transmit(struct nrc_wpa_if* intf, uint8_t *frm, const uint16_t len,
				const int ac);
int nrc_get_sec_hdr_len(struct nrc_wpa_key *key);
void nrc_start_keep_alive(struct nrc_wpa_if* intf);
#if defined(SOFT_AP_BSS_MAX_IDLE)
void softap_bss_timer_stop(uint16_t aid);
void softap_bss_timer_start(uint16_t aid);
void softap_bss_max_idle_period_update(int period);
int softap_get_bss_max_idle_period(void);
void softap_bss_max_idle_retry_update(int retry);
void refresh_bss_max_idle_by_maddr(uint8_t vif_id, uint8_t* addr);
#endif /* defined(SOFT_AP_BSS_MAX_IDLE) */
// Helper functions
static inline bool is_wep(uint8_t cipher)
{
	return (cipher == WIM_CIPHER_TYPE_WEP40 || cipher == WIM_CIPHER_TYPE_WEP104);
}

static inline bool is_key_wep(struct nrc_wpa_key *key)
{
	return (key && is_wep(key->cipher));
}

static inline bool is_ccmp(uint8_t cipher)
{
	return (cipher == WIM_CIPHER_TYPE_CCMP || cipher == WIM_CIPHER_TYPE_CCMP_256);
}

static inline bool is_key_ccmp(struct nrc_wpa_key *key)
{
	return (key && is_ccmp(key->cipher));
}
#endif // _DRIVER_NRC_H_
