#ifndef HAL_LMAC_RATE_CONTROL_H
#define HAL_LMAC_RATE_CONTROL_H
#include "system.h"
#include "lmac_common.h"

/* Events for rate control update */
enum {
	RC_NO_OP,
	RC_ACK_RX_SUCCESS,
	RC_ACK_RX_FAIL,
};

/* The result of update */
enum {
	RC_NONE,
	RC_NEXT,
	RC_PREV,
};

enum {
	RC_ATTR_20M = (1 << 0),
	RC_ATTR_40M = (1 << 1),
	RC_ATTR_RSV = (1 << 2),
	RC_ATTR_SGI = (1 << 3),
};

enum {
	RC_ATTR_1M    = 0,
	RC_ATTR_2M    = 1,
	RC_ATTR_4M    = 2,
};

enum {
	RC_1M_ENTRY_NUM = 9,
	RC_2M_ENTRY_NUM = 8,
	RC_4M_ENTRY_NUM = 8,
	RC_MAX_ENTRY_SIZE = RC_1M_ENTRY_NUM + RC_2M_ENTRY_NUM + RC_4M_ENTRY_NUM,

	RC_1M_BASE = 0,
	RC_2M_BASE = 9,
	RC_4M_BASE = 17,
};

enum {
	RC_MODE_NONE,	/* not set by user, system default */
	RC_MODE_0,		/* disable */
	RC_MODE_1,		/* mode1 */
	RC_MODE_2,		/* mode2 */
	RC_MODE_MAX,
};


#define RC_USE_RAND_PROBE_RSSI_CAP    /* use random probe using rssi cap */
//#undef RC_USE_RAND_PROBE_RSSI_CAP    /* use random probe using rssi cap */

#if defined (RC_USE_RAND_PROBE_RSSI_CAP)
#define RC_USE_UPPER_CAP_PROBE_WEAK_RF_F_ONLY    /* use upper cap in case of weak rf field only */ 
#undef RC_USE_LOWER_CAP_PROBE    				/* use downward cap when getting random probing rate index */
#endif

/* MEM Tuning
 * MEM9: remove total_success, total_attempt from PER_RATE							==> remain for debug purpose
 */

//#define MEM9    /* debug info    */
#undef MEM9     /* no debug info */

#define DYN_NODE_ALLOC /* alloc node using pvPortMallocfunction */

#define RC_BW_1         0
#define RC_BW_2         1
#define RC_BW_4         2

#define RC_FORMAT_S1G   0

#if defined (INCLUDE_ADAPTIVE_GI)
#define NUM_RATES       12
#else
#define NUM_RATES       11
#endif

#define RC_MODE_NEW		(0)
#define RC_MODE			(1)

#define RC_STATE_IDLE	(0)
#define RC_STATE_FIX	(1)
#define RC_STATE_RC		(2)




typedef struct _PER_RATE {
    uint8_t success;
    uint8_t attempt;
    uint8_t last_attempt;
    float    success_prob;
#ifdef MEM9 /* CKLEE_TODO, there is one byte space (4byte align), so percentage is possible */
    uint32_t total_success;
    uint32_t total_attempt;
	uint8_t last_success;
#endif

} __attribute__((packed)) PER_RATE;

enum {
	RC_MAXTP	= 0,
	RC_TP2,
	RC_MAXP,
	RC_LOWEST,
	RC_PROBE,
	RC_MAX_TRY
};

typedef struct _PER_NODE {
	uint8_t try_index[RC_MAX_TRY];
    uint32_t last_update_tsf_us;
    uint32_t last_probe_us;
    uint8_t get_mcs_count;
    int8_t   rssi_avg;
    uint32_t tx_total_cnt;     /* tx total count */
    uint8_t  c_retry_cnt;		/* continuous retry count */
    uint8_t retry[9];
    PER_RATE rate[NUM_RATES];
} __attribute__((packed)) PER_NODE;

typedef struct _RC_RATE {
    uint8_t bw          : 4;
    uint8_t format      : 4;
    uint8_t mcs;
    uint16_t effective_tp;
    int8_t min_rssi; /* Last PER 0% position */
	bool sgi;
} RC_RATE;


typedef struct _RC_INFO {
    PER_NODE node[VIF_MAX];
	bw_t bw[VIF_MAX];
	uint8_t entry_num;
    double ewma;
    uint16_t update_interval_ms; // time
    uint16_t probe_interval_ms;  // per packet
} RC_INFO;

struct track_index {
	bool flag;
	uint8_t index : 7;
};

typedef struct _rate_entry {
	uint8_t order;
	uint8_t format : 4;
	uint8_t mcs    : 4;
	uint8_t attr;
} __attribute__((packed)) RateEntry;

typedef struct _rate_controller {
	uint8_t m_start_idx;
	uint8_t m_end_idx;
	struct track_index m_entry_track[RC_MAX_ENTRY_SIZE];

	uint8_t m_success_count : 4;
	uint8_t m_fail_count    : 4;
	uint8_t m_cursor        : 7;
    uint8_t m_probe         : 1;

} __attribute__((packed)) RC_CONTROL;

void lmac_rc_initialize_arf(); /// Initialize values
void lmac_rc_configure_arf(uint8_t format, uint8_t attr, uint32_t mcs_bitmap, int vif_id);

/// Called by internal HAL
uint8_t lmac_rc_seq_success(uint8_t count, int vif_id); /// one sequence completed without any error
uint8_t lmac_rc_seq_fail(uint8_t count, int vif_id); /// sequence has flaw point
uint8_t lmac_rc_pre_update(uint8_t event, uint8_t count, int vif_id);
void    lmac_rc_post_update(uint8_t event, int vif_id);
void 	lmac_rc_reset_start_index(int vif_id);

//const struct _rate_entry* lmac_rc_get_base(int vif_id);
const struct _rate_entry* lmac_rc_get_entry(int vif_id);
const struct _rate_entry* lmac_rc_get_max_entry(int vif_id);

void lmac_bcn_mcs_init(uint8_t vif_id);
void lmac_bcn_mcs_set(uint8_t vif_id, uint8_t val);
uint8_t lmac_bcn_mcs_get(uint8_t vif_id);

void lmac_rc_initialize();
void lmac_rc_node_initialize (int i, PER_NODE *n);
void lmac_rc_update_stats( uint8_t vif , uint16_t aid , uint8_t index , uint8_t attempt ,
									uint8_t success, int8_t rssi_of_sta_ack);
void lmac_rc_configure(uint8_t vif , uint8_t bw, bool keep_param);
void lmac_rc_node_configure (PER_NODE *n, bw_t bw, bool keep_param);
uint8_t lmac_rc_get_rate_index(uint8_t vif , uint16_t aid ,  uint8_t retry, uint8_t *n_max_mpdu);
const RC_RATE *lmac_rc_get_rate(uint8_t index);
RC_INFO *lmac_rc_get_rcinfo();
uint8_t lmac_rc_get_state();
void lmac_rc_set_state(uint8_t state);
uint32_t lmac_rc_get_last_tsf();
void lmac_rc_set_last_tsf(uint32_t state);
void lmac_rc_show_by_vif (uint8_t vif_id);
void lmac_rc_show_by_aid (uint8_t vif_id, uint16_t aid);
void lmac_rc_show();
uint8_t lmac_rc_get_retry_order(uint8_t vif, uint8_t * retry_chain, uint16_t aid);



void lmac_rc_set_param(uint8_t ewma, uint8_t uinterval, uint8_t pinterval);
void lmac_rc_set_update_interval(uint8_t uinterval);
#if defined(INCLUDE_DUTYCYCLE)
void lmac_rc_save_org_param(void);
void lmac_rc_restore_org_param(void);
#endif

void lmac_rc_show_rc_info(void);
void lmac_rc_get_rc_info(uint32_t* ewma, uint16_t* update_interval_ms, uint16_t* probe_interval_ms);


uint8_t lmac_get_max_rc_bandwidth(int vif_id);
void lmac_config_rate_control(uint8_t format, uint8_t attr, uint32_t mcs_bitmap, int vif_id);
void lmac_reconfig_rc_entry(uint8_t cur_width, uint8_t new_width, int vif_id);
void lmac_rc_restore_status (uint8_t vif_id, uint8_t maxtp, uint8_t tp2,
									uint8_t maxp, uint8_t lowest, uint8_t probe);
void lmac_rc_save_status (uint8_t vif_id, uint8_t maxtp, uint8_t tp2,
									uint8_t maxp, uint8_t lowest, uint8_t probe);
uint8_t lmac_rc_get_maxtp(uint8_t vif, uint16_t aid);
char *lmac_rc_get_mode_str (uint8_t vif_id);
uint8_t lmac_rc_get_mode2_mcs (uint8_t vif_id, uint16_t aid);
double lmac_rc_mcs_to_rate (uint8_t vif_id, uint8_t bw, uint8_t mcs);

void lmac_rc_channel_updated (/* uint8_t vif_id, */ bw_t bw);

#endif /* HAL_LMAC_RATE_CONTROL_H */

