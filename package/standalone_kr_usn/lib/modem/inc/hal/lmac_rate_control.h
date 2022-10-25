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
#if defined(STANDARD_11N)
	RC_11B_ENTRY_NUM = 4,
	RC_11AG_ENTRY_NUM = 8,
	RC_HT_LGI_ENTRY_NUM = 8,
	RC_HT_SGI_ENTRY_NUM = 8,
	RC_MAX_ENTRY_SIZE	= RC_11B_ENTRY_NUM + RC_11AG_ENTRY_NUM + RC_HT_LGI_ENTRY_NUM + RC_HT_SGI_ENTRY_NUM,

	RC_11B_BASE = 0,
	RC_11AG_BASE = 4,
	RC_HT_LGI_BASE = 12,
	RC_HT_SGI_BASE = 20,

#endif
#if defined(STANDARD_11AH)
	RC_1M_ENTRY_NUM = 9,
	RC_2M_ENTRY_NUM = 8,
	RC_4M_ENTRY_NUM = 8,
	RC_MAX_ENTRY_SIZE = RC_1M_ENTRY_NUM + RC_2M_ENTRY_NUM + RC_4M_ENTRY_NUM,

	RC_1M_BASE = 0,
	RC_2M_BASE = 9,
	RC_4M_BASE = 17,
#endif /* defined(STANDARD_11AH) */
};

typedef struct _PER_RATE {
    uint16_t success;
    uint16_t attempt;
    uint16_t last_success;
    uint16_t last_attempt;
    uint16_t estimate_tp;
    double   success_prob;
    uint32_t total_success;
    uint32_t total_attempt;
} PER_RATE;

typedef struct _PER_NODE {
    uint8_t maxtp;
    uint8_t tp2;
    uint8_t maxp;
    uint8_t lowest;
    uint8_t probe;
    uint8_t n_rates;
    uint8_t rate_table_start_index;
    uint32_t last_update_tsf_us;
    uint32_t last_probe_us;
    uint16_t get_mcs_count;
    uint8_t* retry[9];
    PER_RATE rate[30];
} PER_NODE;

#if defined(STANDARD_11N)
    #define RC_BW_20        0
    #define RC_BW_40        1

    #define RC_FORMAT_11A   0
    #define RC_FORMAT_11B   1
    #define RC_FORMAT_11N   3
	#define NUM_RATES 		16
#endif
#if defined(STANDARD_11AH)
    #define RC_BW_1         0
    #define RC_BW_2         1
    #define RC_BW_4         2

    #define RC_FORMAT_S1G   0
	#define NUM_RATES 		25
#endif

#define RC_MODE_NEW		(0)
#define RC_MODE			(1)

#define RC_STATE_IDLE	(0)
#define RC_STATE_FIX	(1)
#define RC_STATE_RC		(2)

typedef struct _RC_RATE {
    uint8_t bw          : 4;
    uint8_t format      : 4;
    uint8_t mcs;
    uint16_t effective_tp;
} RC_RATE;


typedef struct _RC_INFO {
    PER_NODE node[VIF_MAX];
    double ewma;
    uint16_t update_interval_ms; // time
    uint16_t probe_interval_ms; // per packet
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

void lmac_rc_initialize();
void lmac_rc_update_stats( uint8_t vif , uint16_t aid , uint8_t rate_index , uint8_t attempt , uint8_t success);
void lmac_rc_configure(uint8_t vif , uint8_t bw);
uint8_t lmac_rc_get_rate_index(uint8_t vif , uint8_t aid ,  uint8_t retry);
const RC_RATE *lmac_rc_get_rate(uint8_t index);
RC_INFO *lmac_rc_get_rcinfo();
uint8_t lmac_rc_get_state();
void lmac_rc_set_state(uint8_t state);
uint32_t lmac_rc_get_last_tsf();
void lmac_rc_set_last_tsf(uint32_t state);
void lmac_rc_show();
void lmac_rc_show_all(int vif_id);
void lmac_rc_show_current_entry(int vif_id);
void lmac_set_rate_control(int vif_id, bool en);
bool lmac_get_rate_control(int vif_id);
uint8_t lmac_get_max_rc_bandwidth(int vif_id);
void lmac_config_rate_control(uint8_t format, uint8_t attr, uint32_t mcs_bitmap, int vif_id);
void lmac_reconfig_rc_entry(uint8_t cur_width, uint8_t new_width, int vif_id);

#endif /* HAL_LMAC_RATE_CONTROL_H */