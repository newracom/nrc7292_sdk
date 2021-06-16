#ifndef UMAC_INFO_H
#define UMAC_INFO_H

#include "system.h"
#include "umac_ieee80211_types.h"
#include "lmac_common.h"

#if defined(INCLUDE_UMAC)
#define APINFO				g_umac_apinfo
#define DEC_ARR_APINFO_GET_SET(t, x) \
	t get_umac_apinfo_##x(int8_t);\
	void set_umac_apinfo_##x(int8_t, t, int8_t);

#define DEF_ARR_APINFO_GET_SET(t, x) \
	t get_umac_apinfo_##x(int8_t vif_id) { \
		return APINFO[vif_id].x; \
	}; \
	void set_umac_apinfo_##x(int8_t vif_id, t value, int8_t len) { \
        memcpy( APINFO[vif_id].x, value , len );\
	}

#define DEC_VAL_APINFO_GET_SET(t, x) \
	t get_umac_apinfo_##x(int8_t);\
	void set_umac_apinfo_##x(int8_t, t);

#define DEF_VAL_APINFO_GET_SET(t, x) \
	t get_umac_apinfo_##x(int8_t vif_id) { \
		return APINFO[vif_id].x; \
	}; \
	void set_umac_apinfo_##x(int8_t vif_id, t value) { \
        APINFO[vif_id].x = value; \
	}
#endif /* defined(INCLUDE_UMAC) */

struct lmac_cipher {
	bool		key_enable;
	enum key_type key_type;
	uint8_t		key_id;
	uint32_t	key[MAX_KEY_LEN];
};

typedef struct {
	// General
	uint8_t		bssid[6];
	uint8_t		ssid[IEEE80211_MAX_SSID_LEN];	// key
	uint8_t		ssid_len;	// key_length

	uint8_t		security;
	uint16_t	beacon_interval;

#if defined(STANDARD_11AH)
	uint16_t	short_beacon_interval;
	uint8_t		assoc_s1g_channel;
	// S1G Info
	uint32_t	comp_ssid;
	uint32_t	change_seq_num;
	// Peer Info
	bool		s1g_long_support;
	bool		pv1_frame_support;
	bool		nontim_support;
	bool		twtoption_activated;
	bool		ampdu_support;
	bool		ndp_pspoll_support;
	bool		shortgi_1mhz_support;
	bool		shortgi_2mhz_support;
	bool		shortgi_4mhz_support;
	bool		maximum_mpdu_length;
	uint8_t		maximum_ampdu_length_exp: 3;
	uint8_t		minimum_mpdu_start_spacing: 5;
	uint8_t		rx_s1gmcs_map;
	uint8_t		color;
	uint8_t		traveling_pilot_support;
	uint32_t	base_pn_bk;
	uint32_t	base_pn_be;
	uint32_t	base_pn_vi;
	uint32_t	base_pn_vo;
#if defined(INCLUDE_TWT_SUPPORT)
	uint32_t	twt_service_period;
	uint32_t	twt_wake_interval;
#endif /* defined(INCLUDE_TWT_SUPPORT) */
#endif /* defined(STANDARD_11AH) */
	uint8_t     key_addr[MAC_ADDR_LEN];
	/* README: yj.kim 12/6/2019
	 * - cipher_info has to be located at the end
	 */
	struct lmac_cipher	cipher_info[MAX_KEY_ID];
} __attribute__((packed)) umac_apinfo;

#define DEC_ARR_STAINFO_GET_SET(t, x) \
	t get_umac_stainfo_##x(umac_stainfo*);\
	void set_umac_stainfo_##x(umac_stainfo*, t);

#define DEF_ARR_STAINFO_GET_SET(t, x) \
	t get_umac_stainfo_##x(umac_stainfo* sta_info) { \
		if (!sta_info) \
			return (t)0; \
		return sta_info->x; \
	}; \
	void set_umac_stainfo_##x(umac_stainfo* sta_info, t value) { \
		if (sta_info) \
        	memcpy( sta_info->x , value , sizeof( sta_info->x ) );\
	}

#define DEC_VAL_STAINFO_GET_SET(t, x) \
	t get_umac_stainfo_##x(umac_stainfo*);\
	void set_umac_stainfo_##x(umac_stainfo*, t);

#define DEF_VAL_STAINFO_GET_SET(t, x) \
	t get_umac_stainfo_##x(umac_stainfo* sta_info) { \
		if (!sta_info) \
			return (t)0; \
		return sta_info->x; \
	}; \
	void set_umac_stainfo_##x(umac_stainfo* sta_info, t value) { \
		if (sta_info) \
        	sta_info->x = value; \
	}

typedef enum {
	INVALID = 0xFF,
	AUTH	= 0,
	ASSOC	= 1
} STAINFO_STATE;

typedef struct _umac_stainfo {
	uint8_t maddr[6];
	uint16_t aid;			// key
	uint32_t listen_interval;
	bool bss_max_idle_period_idle_option;
	uint16_t bss_max_idle_period;
#if defined(STANDARD_11AH)
   	bool s1g_long_support;
	bool pv1_frame_support;
	bool nontim_support;
	bool twtoption_activated;
	bool ampdu_support;
	bool ndp_pspoll_support;
	bool shortgi_1mhz_support;
	bool shortgi_2mhz_support;
	bool shortgi_4mhz_support;
	bool maximum_mpdu_length;
	uint8_t maximum_ampdu_length_exp: 3;
	uint8_t minimum_mpdu_start_spacing: 5;
	uint8_t rx_s1gmcs_map;
	uint8_t	traveling_pilot_support;
	uint8_t	ssid[IEEE80211_MAX_SSID_LEN];
	uint8_t	ssid_len;
	uint32_t base_pn_bk;
	uint32_t base_pn_be;
	uint32_t base_pn_vi;
	uint32_t base_pn_vo;
#endif /* defined(STANDARD_11AH) */
	STAINFO_STATE				state;
	struct {
		struct _qos_sn {
			uint16_t tx_sn		: 12;
			uint16_t win_end	: 12;
			uint16_t win_start	: 12;
			uint64_t win_bitmap	: MAX_AGG_SCHED_NUMBER;
		} qos_sn[MAX_TID];
		uint16_t rx_sn		: 12;
	} snmanager;
#if defined(INCLUDE_TWT_SUPPORT)
	uint32_t	twt_service_period;
	uint32_t	twt_wake_interval;
#endif /* defined(INCLUDE_TWT_SUPPORT) */
#if defined(STANDARD_11AH)
	uint16_t    key_aid;
	uint8_t     key_addr[MAC_ADDR_LEN];
	struct lmac_cipher cipher_info[MAX_KEY_ID];
#endif /* defined(STANDARD_11AH) */
#if defined(INCLUDE_STA_SIG_INFO)
	int8_t		rssi;
	uint8_t		snr;
	uint8_t 	mcs;
#endif /* defined(INCLUDE_SIGNAL_INFO_SOFTAP) */
} __attribute__((packed)) umac_stainfo;

#if !defined(MAX_STA)
#define MAX_STA			1024
#endif /* !defined(MAX_STA) */

//////////////////////
// Public Functions //
//////////////////////
#if defined(INCLUDE_UMAC)
void umac_stainfo_del(void);
void umac_apinfo_clean(int8_t);
void umac_apinfo_deauth(int8_t);
void umac_info_init(int8_t, MAC_STA_TYPE);
void add_umac_stainfo_aid(int8_t vif_id, umac_stainfo* sta, uint16_t aid);
void del_umac_stainfo_peer_sta(int8_t vif_id, uint8_t* addr);
umac_stainfo * get_umac_stainfo_by_macaddr(int8_t vif_id, uint8_t *addr);
umac_stainfo * get_umac_stainfo_by_aid(int8_t vif_id, uint16_t aid);
umac_stainfo * get_umac_stainfo_by_vifid(int8_t vif_id);
bool umac_check_remain_allow_sta(int8_t vif_id, uint8_t* addr);
bool umac_check_stainfo_by_macaddr_ap(int8_t vif_id, uint8_t *addr);
#if defined(STANDARD_11AH)
uint8_t* get_umac_stainfo_ssid(int8_t vif_id);
uint8_t	get_umac_stainfo_ssid_len(int8_t vif_id);
void set_umac_stainfo_ssid(int8_t vif_id, uint8_t *ssid, uint8_t ssid_len);
void set_umac_stainfo_key_info(int8_t vif_id, uint16_t aid, enum key_type key_type,
						uint8_t key_id, uint32_t *key, uint8_t *addr);
void clear_umac_stainfo_key_info(int8_t vif_id, uint16_t aid, enum key_type key_type,
						uint8_t key_id);
bool get_umac_apinfo_key_info(int8_t vif_id, uint8_t key_id, struct cipher_def *lmc);
bool get_umac_stainfo_key_info(int8_t vif_id, uint16_t sta_idx, uint8_t key_id, struct cipher_def *lmc);

void test_cmd_show_info();
#endif /* defined(STANDARD_11AH) */

DEC_ARR_APINFO_GET_SET(uint8_t*,			bssid);
DEC_ARR_APINFO_GET_SET(uint8_t*,			ssid);

DEC_VAL_APINFO_GET_SET(uint8_t, 			ssid_len);
DEC_VAL_APINFO_GET_SET(uint8_t, 			security);
DEC_VAL_APINFO_GET_SET(uint16_t,			beacon_interval);

DEC_ARR_STAINFO_GET_SET(uint8_t*,			maddr);
DEC_VAL_STAINFO_GET_SET(uint16_t,			aid);
DEC_VAL_STAINFO_GET_SET(uint32_t,			listen_interval);

#if defined(STANDARD_11AH)
DEC_VAL_APINFO_GET_SET(uint16_t,			short_beacon_interval);
DEC_VAL_APINFO_GET_SET(uint8_t,				assoc_s1g_channel);
DEC_VAL_APINFO_GET_SET(uint32_t, 			comp_ssid);
DEC_VAL_APINFO_GET_SET(uint32_t, 			change_seq_num);
DEC_VAL_APINFO_GET_SET(bool,				s1g_long_support);
DEC_VAL_APINFO_GET_SET(bool,				pv1_frame_support);
DEC_VAL_APINFO_GET_SET(bool,        		nontim_support);
DEC_VAL_APINFO_GET_SET(bool,        		twtoption_activated);
DEC_VAL_APINFO_GET_SET(bool,        		ampdu_support);
DEC_VAL_APINFO_GET_SET(bool,        		ndp_pspoll_support);
DEC_VAL_APINFO_GET_SET(bool,        		shortgi_1mhz_support);
DEC_VAL_APINFO_GET_SET(bool,        		shortgi_2mhz_support);
DEC_VAL_APINFO_GET_SET(bool,        		shortgi_4mhz_support);
DEC_VAL_APINFO_GET_SET(bool,        		maximum_mpdu_length);
DEC_VAL_APINFO_GET_SET(uint8_t,     		maximum_ampdu_length_exp);
DEC_VAL_APINFO_GET_SET(uint8_t,     		minimum_mpdu_start_spacing);
DEC_VAL_APINFO_GET_SET(uint8_t,     		rx_s1gmcs_map);
DEC_VAL_APINFO_GET_SET(uint8_t,     		color);
DEC_VAL_APINFO_GET_SET(uint8_t,        		traveling_pilot_support);
DEC_VAL_APINFO_GET_SET(uint32_t, 			base_pn_bk);
DEC_VAL_APINFO_GET_SET(uint32_t, 			base_pn_be);
DEC_VAL_APINFO_GET_SET(uint32_t, 			base_pn_vi);
DEC_VAL_APINFO_GET_SET(uint32_t, 			base_pn_vo);
DEC_ARR_APINFO_GET_SET(uint8_t*,			key_addr);
DEC_ARR_APINFO_GET_SET(struct lmac_cipher*, cipher_info);
#if defined(INCLUDE_TWT_SUPPORT)
DEC_VAL_APINFO_GET_SET(uint32_t,			twt_service_period);
DEC_VAL_APINFO_GET_SET(uint32_t,			twt_wake_interval);
#endif /* defined(INCLUDE_TWT_SUPPORT) */

DEC_VAL_STAINFO_GET_SET(bool,				bss_max_idle_period_idle_option);
DEC_VAL_STAINFO_GET_SET(uint16_t,			bss_max_idle_period);
DEC_VAL_STAINFO_GET_SET(bool,				s1g_long_support);
DEC_VAL_STAINFO_GET_SET(bool,				pv1_frame_support);
DEC_VAL_STAINFO_GET_SET(bool,        		nontim_support);
DEC_VAL_STAINFO_GET_SET(bool,        		twtoption_activated);
DEC_VAL_STAINFO_GET_SET(bool,        		ampdu_support);
DEC_VAL_STAINFO_GET_SET(bool,        		ndp_pspoll_support);
DEC_VAL_STAINFO_GET_SET(bool,        		shortgi_1mhz_support);
DEC_VAL_STAINFO_GET_SET(bool,        		shortgi_2mhz_support);
DEC_VAL_STAINFO_GET_SET(bool,        		shortgi_4mhz_support);
DEC_VAL_STAINFO_GET_SET(bool,        		maximum_mpdu_length);
DEC_VAL_STAINFO_GET_SET(uint8_t,     		maximum_ampdu_length_exp);
DEC_VAL_STAINFO_GET_SET(uint8_t,     		minimum_mpdu_start_spacing);
DEC_VAL_STAINFO_GET_SET(uint8_t,     		rx_s1gmcs_map);
DEC_VAL_STAINFO_GET_SET(uint8_t,       		traveling_pilot_support);
DEC_VAL_STAINFO_GET_SET(uint32_t,       	base_pn_bk);
DEC_VAL_STAINFO_GET_SET(uint32_t,       	base_pn_be);
DEC_VAL_STAINFO_GET_SET(uint32_t,       	base_pn_vi);
DEC_VAL_STAINFO_GET_SET(uint32_t,       	base_pn_vo);
DEC_VAL_STAINFO_GET_SET(uint16_t,			key_aid);
DEC_ARR_STAINFO_GET_SET(uint8_t*,			key_addr);
DEC_ARR_STAINFO_GET_SET(struct lmac_cipher*, cipher_info);
#if defined(INCLUDE_TWT_SUPPORT)
DEC_VAL_STAINFO_GET_SET(uint32_t,			twt_service_period);
DEC_VAL_STAINFO_GET_SET(uint32_t,			twt_wake_interval);
#endif /* defined(INCLUDE_TWT_SUPPORT) */
#if defined(INCLUDE_STA_SIG_INFO)
DEC_VAL_STAINFO_GET_SET(int8_t,			rssi);
DEC_VAL_STAINFO_GET_SET(uint8_t,			snr);
DEC_VAL_STAINFO_GET_SET(uint8_t,		mcs);
#endif /* defined(INCLUDE_SIGNAL_INFO_SOFTAP) */

#endif /* defined(LMAC_CONFIG_11AH) */
#else /* defined(INCLUDE_UMAC) */
static inline uint8_t *get_umac_apinfo_bssid(int8_t vif_id) {return NULL;};
static inline umac_stainfo*	get_umac_stainfo_by_aid(int8_t vif_id, uint16_t aid) {return NULL;};
static inline umac_stainfo* get_umac_stainfo_by_vifid(int8_t vif_id) {return NULL;};
static inline umac_stainfo* get_umac_stainfo_by_macaddr(int8_t vif_id, uint8_t *addr) {return NULL;};
static inline uint32_t get_umac_stainfo_listen_interval(umac_stainfo* u){return 0;}
static inline uint32_t set_umac_apinfo_assoc_s1g_channel(int8_t v, uint8_t k){return 0;}
static inline bool umac_check_stainfo_by_macaddr_ap(int8_t vif_id, uint8_t *addr){return 0;}
static inline void set_umac_apinfo_bssid(int8_t v, uint8_t *m, int a) {}
#endif /* defined(INCLUDE_UMAC) */
#endif
