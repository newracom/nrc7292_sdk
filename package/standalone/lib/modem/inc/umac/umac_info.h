#ifndef UMAC_INFO_H
#define UMAC_INFO_H

#include "system.h"
#include "umac_ieee80211_types.h"
#include "lmac_common.h"
#include "lmac_rate_control.h"
#if defined(INCLUDE_DEFRAG)
#include "util_sysbuf_queue.h"
#endif
#if defined(INCLUDE_TWT_SUPPORT)
#include "lmac_twt_common.h"
#endif

#if !defined(MAX_STA)
#if defined(NRC7292) || defined(NRC7394)
#define MAX_STA	1000
#else
#define MAX_STA	4
#endif
#endif /* !defined(MAX_STA) */

//STA_INFO and AP_INFO keep only 2 KEYs to decrease total size of umacinfo
//AP_INFO : 2 GTKs(idx_1, idx_2),  STA_INFO : 2 PTKs (idx_0)
#define MAX_UINFO_KEY_ID 2

/* STA's State */
typedef enum _STA_STATE{
	INVALID = 0xFF,
	AUTH	= 0,
	ASSOC	= 1
} STA_STATE;

/*AP's basic Info */
typedef struct _AP_BASIC_INFO {
	uint8_t bssid[MAC_ADDR_LEN];
	uint8_t ssid[IEEE80211_MAX_SSID_LEN]; // key
	uint8_t ssid_len;	// key_length
	uint16_t beacon_interval;
	uint32_t comp_ssid;
	uint16_t short_beacon_interval;
	uint8_t assoc_s1g_channel;
	uint32_t change_seq_num;
} __attribute__((packed)) AP_BASIC_INFO;

/*STA's basic Info */
typedef struct _STA_BASIC_INFO {
	uint8_t vif_id;
	uint8_t maddr[MAC_ADDR_LEN];
	uint16_t aid; // key
	uint16_t listen_interval;
} __attribute__((packed)) STA_BASIC_INFO;

/* QoS Sequence Number Info*/
typedef struct _RX_QOS_SN {
	uint16_t win_end : 12;
	uint16_t win_start: 12;
	uint64_t win_bitmap: 16;
}__attribute__((packed)) RX_QOS_SN;

typedef struct _TX_QOS_SN {
	uint16_t tx_sn: 12;
}__attribute__((packed)) TX_QOS_SN;

/* Sequence Number Info*/
typedef struct _SN_INFO {
	RX_QOS_SN qos_rx_sn[MAX_TID];
	TX_QOS_SN qos_tx_sn[MAX_TID];
	uint16_t rx_sn: 12;
} __attribute__((packed)) SN_INFO;

/* AMPDU Info */
typedef struct _AMPDU_INFO {
	uint8_t aggregation : 1;
	uint8_t maxagg_num:6;
	uint8_t ba_session_tx:1;
} __attribute__((packed)) AMPDU_INFO;

/* BSS Max Idle Info */
// typedef struct _BSS_IDLE_INFO {
// 	bool idle_period_option;
// 	uint16_t max_idle_period;
// } __attribute__((packed)) BSS_IDLE_INFO;

/* S1G Capa Info */
typedef struct _S1G_CAPA {
	uint8_t s1g_long_support:1;
	uint8_t pv1_frame_support:1;
	uint8_t nontim_support:1;
	uint8_t twtoption_activated:1;
	uint8_t ampdu_support:1;
	uint8_t ndp_pspoll_support:1;
	uint8_t shortgi_1mhz_support:1;
	uint8_t shortgi_2mhz_support:1;
	uint8_t shortgi_4mhz_support:1;
	uint8_t s1g_1mctrlresppreamble_support:1;
	uint8_t traveling_pilot_support:1;
	uint8_t maximum_mpdu_length:1;
	uint8_t maximum_ampdu_length_exp: 2;
	uint8_t supported_ch_width: 2;
	uint8_t color:3;
	uint8_t minimum_mpdu_start_spacing: 3;
#if defined(INCLUDE_AUTH_CONTROL)
	uint8_t centralized_auth_control: 1;
	uint8_t distributed_auth_control: 1;
#else
	uint8_t reserved2: 2;
#endif
#if (dot11TWTGroupingSupport == 1)
	uint8_t twtgrouping_support:1;
	uint8_t reserved3: 7;
#endif
	uint8_t rx_s1gmcs_map:8;
} __attribute__((packed)) S1G_CAPA;

/* Security Info */
typedef struct _SECURITY_INFO {
	uint8_t security:1;
	uint8_t akm_type:7; //1:wpa2-ent, 2:wpa2-psk, 8:wpa3-sae, 18:wpa3-owe
} SECURITY_INFO;

/*Cipher Info */
typedef struct _CIPHER_INFO {
	bool key_enable;
	enum key_type key_type;
	uint8_t key_id;
	uint32_t key[MAX_KEY_LEN];
#if defined(INCLUDE_DYNAMIC_FRAG)
	uint64_t tx_pn;
#endif
#if defined(INCLUDE_DEFRAG)
	uint64_t rx_pn;
#endif /* INCLUDE_DEFRAG */
#if defined(INCLUDE_7393_7394_WORKAROUND)
	uint16_t hw_index;
#endif
} __attribute__((packed)) CIPHER_INFO;

/* Key Info */
typedef struct _KEY_INFO {
	uint16_t key_aid;
	uint8_t key_addr[MAC_ADDR_LEN];
#if defined (INCLUDE_IBSS) && defined(NRC7394)
    /* Each IBSS STA has own GTK. Array 0, 1 for PTK0/1 and 2,3 for GTK1/2 */
	CIPHER_INFO cipher_info[4];
#else
	CIPHER_INFO cipher_info[MAX_UINFO_KEY_ID];
#endif
} __attribute__((packed)) KEY_INFO;

#if defined(INCLUDE_TWT_SUPPORT)
/* TWT Info */
typedef struct _TWT_INFO {
	struct dl_list list;
	uint8_t bitmap;
	uint8_t dial_token;
} __attribute__((packed)) TWT_INFO;
#endif /* INCLUDE_TWT_SUPPORT */

#if defined(INCLUDE_PV1) ||defined(INCLUDE_AVOID_FRAG_ATTACK)
/* Packet Number in CCMP Header Info */
typedef struct _PN_INFO {
#if defined(INCLUDE_PV1)
	uint32_t base_pn_bk;
	uint32_t base_pn_be;
	uint32_t base_pn_vi;
	uint32_t base_pn_vo;
#endif
#if defined(INCLUDE_AVOID_FRAG_ATTACK)
	uint32_t PN_H;
	uint32_t PN_L;
#endif
} __attribute__((packed)) PN_INFO;
#endif

#if defined(INCLUDE_STA_SIG_INFO)
/* Signal Info */
typedef struct _SIGNAL_INFO {
	int8_t rssi;
	int8_t rssi_avg;
	uint8_t snr;
} __attribute__((packed)) SIGNAL_INFO;
#endif /* INCLUDE_STA_SIG_INFO */

#if defined(INCLUDE_DEFRAG)
 /* An entry is for 1 frame that is defragmented. */
 #define DEFRAG_ENTRY_MAX 1

typedef struct _DEFRAG_ENTRY {
	struct sysbuf_queue dfque;
	unsigned long first_frag_time;
	uint8_t tid;
	uint8_t num_buf;
	uint8_t last_frag;
	uint16_t sn;
#if defined (INCLUDE_DEFRAG_CHECK_PN)
	uint8_t last_pn[6]; /* PN of the last fragment */
#endif
}__attribute__((packed)) DEFRAG_ENTRY;

typedef struct _DEFRAG_INFO {
	DEFRAG_ENTRY entries[DEFRAG_ENTRY_MAX];
	unsigned int next;
}__attribute__((packed)) DEFRAG_INFO;
#endif/* INCLUDE_DEFRAG */


typedef struct _MCS_INFO {
	uint8_t last_tx_mcs:4;
	uint8_t last_rx_mcs:4;
}__attribute__((packed)) MCS_INFO;

typedef struct _M_SIG_INFO{
	int8_t rssi_avg;
	int8_t rssi_last;
	int8_t noise_last;
	//int8_t snr_avg;
	//int8_t snr_last;
}__attribute__((packed)) M_SIG_INFO;

/**************************************************************
	APINFO (Common)
		- STA : peer(AP)'s info (preallocated)
		- AP/MESH : own info (preallocated)
**************************************************************/
typedef struct _APINFO{
	// General
	AP_BASIC_INFO m_binfo;
	S1G_CAPA m_s1g;
#if defined(INCLUDE_PV1)
	PN_INFO m_pn;
#endif
#if defined(INCLUDE_TWT_SUPPORT)
	TWT_INFO m_twt;
#endif /* defined(INCLUDE_TWT_SUPPORT) */
	SECURITY_INFO m_security;
	KEY_INFO m_key;
	M_SIG_INFO msig;
#if defined(INCLUDE_MULTI_STA_RC)
	//PER_NODE m_rc_node;
	//PER_NODE *m_rc_node_p;
#endif
} __attribute__((packed)) APINFO;

/**************************************************************
	STAINFO (Common)
		- STA : own info (preallocated)
		- AP/MESH : peer(STA)'s info (allocated from heap whenever STA is connected)
**************************************************************/
typedef struct _STAINFO {
	STA_STATE m_state;
	STA_BASIC_INFO m_binfo;
	AMPDU_INFO m_ampdu[MAX_AC];
	// BSS_IDLE_INFO m_bssidle;
	S1G_CAPA m_s1g;
	SN_INFO m_sn;
	KEY_INFO m_key;
#if defined(INCLUDE_PV1)
	PN_INFO m_pn;
#endif
#if defined(INCLUDE_TWT_SUPPORT)
#if (TWT_AP_TARGET_SUPPORT != 0)
	TWT_INFO m_twt;
#endif
#endif
#if defined(INCLUDE_STA_SIG_INFO)
	SIGNAL_INFO m_signal;
#endif
#if defined(INCLUDE_DEFRAG)
	DEFRAG_INFO m_defrags;
#endif
	MCS_INFO m_mcs;
#if defined(INCLUDE_MULTI_STA_RC)
#ifndef DYN_NODE_ALLOC
	PER_NODE m_rc_node;
#endif
	PER_NODE *m_rc_node_p;
#endif
#if defined(INCLUDE_MANAGE_BLACKLIST)
	uint8_t tx_retry_limit_cnt;
#endif
} __attribute__((packed)) STAINFO;


//// functions 
#if defined(INCLUDE_UMAC)
/* (COMMON) Initialize sta/ap info */
void initialize_sta_ap_info(int8_t vif_id, MAC_STA_TYPE type);

/* (COMMON) Get stainfo by addr (if stainfo does not exit, allocate new one only when create is true) */
STAINFO * get_stainfo_by_addr(int8_t vif_id, uint8_t *addr, bool create);

/* (COMMON) Get stainfo by aid */
STAINFO * get_stainfo_by_aid(int8_t vif_id, uint16_t aid);

/* (COMMON) get apinfo by vif_id */
APINFO * get_apinfo_by_vifid(int8_t vif_id);

/* (COMMON) clear apinfo by vif_id */
void clear_apinfo(int8_t vif_id);

#if defined(INCLUDE_7393_7394_WORKAROUND)
uint16_t get_keyinfo_hw_index(int8_t vif_id, uint16_t aid, uint8_t key_id, enum key_type key_type, uint8_t *addr);
void set_keyinfo(int8_t vif_id, uint16_t aid, enum key_type key_type, uint8_t key_id, uint32_t *key, uint8_t *addr, uint16_t index);
#else
/* (COMMON) Set KEY(PTK/GTK) INFO in stainfo or apinfo */
void set_keyinfo(int8_t vif_id, uint16_t aid, enum key_type key_type, uint8_t key_id, uint32_t *key, uint8_t *addr);
#endif

/* (COMMON) Get KEY(PTK/GTK) INFO in stainfo or apinfo */
bool get_keyinfo(int8_t vif_id, uint16_t sta_idx, uint8_t key_id, struct cipher_def *lmc, enum key_type key_type, bool is_tx_gtk);

#if defined(NRC7394) && defined(INCLUDE_IBSS)
void clear_keyinfo(int8_t vif_id, uint16_t aid, enum key_type key_type, uint8_t key_id, uint8_t * addr);
#else
/* (COMMON) Clear KEY(PTK/GTK) INFO in stainfo or apinfo */
void clear_keyinfo(int8_t vif_id, uint16_t aid, enum key_type key_type, uint8_t key_id);
#endif

bool get_keyinfo_by_addr(int8_t vif_id, uint8_t *addr, uint8_t key_id, struct cipher_def *lmc, enum key_type key_type, bool is_tx_gtk);

/* (AP/MESH ONLY) Remove stainfo (free  memory allocated for stainfo) */
void remove_stainfo(int8_t vif_id, uint8_t* addr);

/* (AP/MESH ONLY) update stainfo (as assoc state) with aid */
void update_stainfo_by_aid(int8_t vif_id, STAINFO* sta, uint16_t aid);

/*(AP/MESH ONLY) Get AID from stainfo using stainfo index */
uint16_t get_aid_by_stainfo_index(int8_t vif_id, uint16_t sta_idx);

/* (COMMON) Get AID from stainfo using mac address */
uint16_t get_aid_by_addr(int8_t vif_id, uint8_t* addr);

/*(AP/MESH ONLY) Get stainfo with sta index */
STAINFO * get_stainfo_by_index(int8_t vif_id, uint16_t sta_idx);

/* (AP/MESH ONLY) Get number of allocated STA*/
uint16_t get_num_sta(int8_t vif_id);

/* (AP/MESH ONLY) Get number of associated STA*/
uint16_t get_num_assoc_sta(int8_t vif_id);

/* (AP/MESH ONLY) Get number of stainfo of STA asociated with start/end idx */
uint16_t get_num_stainfo(int8_t vif_id, uint16_t *start_idx, uint16_t *end_idx);

/* (AP/MESH ONLY) Get remaining number of stainfo to allocate */
bool check_remaining_stainfo(int8_t vif_id, uint8_t* addr);

/* (AP/MESH ONLY) show stainfo */
void show_stainfo();

/* (STA ONLY) get stainfo by vif_id */
STAINFO * get_stainfo_by_vifid(int8_t vif_id);

/* (STA ONLY) clear stainfo by vif_id */
void clear_stainfo(int8_t vif_id);

/* (STA ONLY) clear tx sn by vif_id */
void clear_sta_txsn(int8_t vif_id);

/* (STA ONLY) clear rx sn by vif_id */
void clear_sta_rxsn(int8_t vif_id);

#else /* defined(INCLUDE_UMAC) */
static inline STAINFO* get_stainfo_by_addr(int8_t vif_id, uint8_t *addr, bool create) {return NULL;};
static inline STAINFO* get_stainfo_by_aid(int8_t vif_id, uint16_t aid) {return NULL;};
static inline STAINFO* get_stainfo_by_vifid(int8_t vif_id) {return NULL;};
static inline APINFO * get_apinfo_by_vifid(int8_t vif_id) {return NULL;};
static inline uint16_t get_aid_by_stainfo_index(int8_t vif_id, uint16_t sta_idx) {return 0;};
static inline STAINFO* get_stainfo_by_index(int8_t vif_id, uint16_t sta_idx){return NULL;};
static inline uint16_t get_num_sta(int8_t vif_id) {return 0;};
static inline uint16_t get_num_assoc_sta(int8_t vif_id) {return 0;};
static inline uint16_t get_num_stainfo(int8_t vif_id, uint16_t *start_idx, uint16_t *end_idx) {return 0;};
#endif /* defined(INCLUDE_UMAC) */

#if defined (INCLUDE_IBSS)
void init_bssid_sta_aid_db(void);
uint16_t alloc_ibss_sta_aid(uint8_t * addr);
void dealloc_ibss_sta_aid(uint16_t aid, uint8_t * addr);
#endif /* INCLUDE_IBSS */


#if defined(INCLUDE_MULTI_STA_RC)
PER_NODE *get_rc_node_from_stainfo_by_aid(uint8_t vif_id, uint16_t aid);
PER_NODE *get_rc_node_of_stainfo(uint8_t vif_id, uint16_t aid);
int umac_rc_peer_init (int8_t vif_id, STAINFO *sta);
void umac_rc_peer_deinit (STAINFO *sta);
#endif /* INCLUDE_MULTI_STA_RC */
uint8_t get_rc_mcs_from_stainfo_by_aid (uint8_t vif_id, uint16_t aid);

void umac_init (void);
void umac_info_lock_init (void);
void umac_info_lock (void);
void umac_info_unlock (void);


#endif
