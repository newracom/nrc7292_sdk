#ifndef HAL_LMAC_QUEUE_MANAGER_H_
#define HAL_LMAC_QUEUE_MANAGER_H_
#if defined(STANDARD_11N)
#include "lmac_queue_manager_11n.h"
#endif
#if defined(STANDARD_11AH)
#include "lmac_queue_manager_11ah.h"
#endif
#include "system_common.h"

#define	QM(x)   	                m_qm[x]
#define	EDCA(x)                     m_qm[x].m_edcaparam
#define QUEUE(x, y)                 m_qm[x].m_queue[y]

#define DEC_EDCA_GET_SET(t, x) \
	t get_##x(uint8_t);\
	void set_##x(uint8_t, t);

#define DEF_EDCA_GET_SET(t, x) \
	t get_##x(uint8_t aci ) { \
		return EDCA(aci).m_##x; \
	}; \
	void set_##x(uint8_t aci, t value) { \
        EDCA(aci).m_##x = value; \
	}

DEC_EDCA_GET_SET(uint8_t, acid);
DEC_EDCA_GET_SET(uint8_t, acm);
DEC_EDCA_GET_SET(uint8_t, aifsn);
DEC_EDCA_GET_SET(uint32_t, cw_min);
DEC_EDCA_GET_SET(uint32_t, cw_max);
DEC_EDCA_GET_SET(uint32_t, txop_max);

#define NORMAL_ACK  0
#define NO_ACK      1

typedef enum txmode {
	SIFS,
	BACKOFF,
	MAX_MODE
} TXMODE;

typedef enum qmstate {
	WAIT_FRAME,
	WAIT_CHANNEL,
	WAIT_ACK,
	CHANNEL_READY,
	STATE_MAX,
} QMSTATE;

typedef enum qtype {
	    RE_TX,          /// Retransmission Queue
	    NEW_TX,         /// MPDU queue from UMAC
	    PENDING,        /// Pending Queue
		MAX_Q,
} QTYPE;

/// statistics for monitoring QM
typedef enum qm_statistic {
		Q_RE_TX,
		Q_NEW_TX,
		Q_PENDING,
		Q_RE_TX_CHUNK,
		Q_NEW_TX_CHUNK,
		Q_PENDING_CHUNK,
		LIST_SCHED_CHUNK,
		LIST_FREE_CHUNK,
	    TOTAL_QUEUED,
	    TOTAL_BYTE,
	    REMAIN_BYTE,
	    ISSUED_TX,
	    RETX_QUEUED,
	    SENT_MPDU,
	    SENT_AMPDU,
	    SENT_PSDU,
	    TX_SUCCESS,
	    TX_FAIL,
	    N_BACKOFF_ISSUED,
	    N_SIFS_ISSUED,
	    N_MPDU_FREE,
	    N_DISCARD,
	    N_UL_REQUEST,
	    N_ACK_RECEIVED,
	    MAX_STATS,
} STATISTIC;


struct Queue {
    	LMAC_TXBUF *m_tail;
    	uint16_t    m_depth;
    	uint16_t    m_max_depth;

		uint8_t     n_sched;
		uint32_t    n_bytes;
};

struct q_edcaparam {
	/// EDCA QoS parameters for uplink tx
	uint8_t  m_acid;
	uint8_t  m_acm;
	uint8_t  m_aifsn;                       /// AIFSN
	uint16_t m_cw_min;                      /// Minimum CW
	uint16_t m_cw_max;                      /// Maximum CW
	uint16_t m_cw;                          /// current CW
	uint32_t m_txop_max;                    /// Maximum TXOP limit
	uint32_t m_txop_limit;
};

typedef struct ndpba_info_t {
	uint32_t ssn			: 12;
	uint32_t bitmap 		: 16;
	uint32_t bitmap_encoded : 1; // valid only if A-MPDU is sent,
	uint32_t reserved0      : 3;
} NDPBA_INFO_T;
typedef struct queuemanager {

	/// Internal queues
	struct Queue m_queue[MAX_Q];
	/// State
	uint8_t m_state;
	/// Schedule& Pending Info
	Sched_Info sched_info, pend_info;
	/// Flags
	bool m_pend_info_copied;
	bool m_sched_failed;
	bool m_delayed_clear;

	/// Schedule list
	LMAC_TXBUF *m_sched_head;
	LMAC_TXBUF *m_sched_list;
	uint16_t m_sched_list_depth;
	/// Free list
	LMAC_TXBUF *m_free_head;
	LMAC_TXBUF *m_free_list;
	uint16_t m_free_list_depth;

	uint16_t m_to_be_clear_depth;
	uint16_t m_current_pending_depth;

	uint16_t m_fail_mpdu_cnt;

	uint32_t m_addr_bo_command;
	uint32_t m_addr_txop_command;

	uint16_t m_sn;                          /// Next sequence number
	uint8_t m_max_aggregated_num;
	uint8_t m_max_buffer;

	struct q_edcaparam m_edcaparam;

	uint8_t  m_response_ind;
	bool	 m_aggregation;
	uint16_t m_agg_size;
	bool	 m_ba_session_tx;

	uint8_t  m_priority;                    /// priority
	uint8_t  m_vif_id;

	uint8_t  m_max_tx_count;
	/// Statistics
    uint32_t m_ac_transmitted;
    uint16_t m_n_state[STATE_MAX];
	uint16_t m_stats[MAX_STATS];
	uint32_t m_transmitted_data;
    uint32_t m_hw_tx_bytes;
    uint16_t m_n_aggregation[STAT_AGG_MAX];

	uint32_t m_n_update_credit;
	uint32_t m_n_buf_magic_code;
	uint32_t m_n_tlv;
	int32_t m_n_buf_s1ghook;
	int32_t m_n_diff_buf;
	uint32_t m_n_alloc_buffer;
	uint32_t m_n_txop_wa;

#if defined(INCLUDE_MODEM_RECOVERY)
	uint16_t m_last_sn;
#endif
#if defined(CONFIG_BITMAP_ENCODING)
	NDPBA_INFO_T m_ndpba_info;
#endif /* defined(CONFIG_BITMAP_ENCODING) */

} QUEUEMANAGER;

extern QUEUEMANAGER	m_qm[MAX_AC];

void enqueue_lifo(LMAC_TXBUF* item, uint8_t ac, uint8_t qtype);
LMAC_TXBUF* queue_peek(uint8_t ac, uint8_t qtype);
bool queue_find(LMAC_TXBUF *p, uint8_t ac, uint8_t qtype);
void enqueue_fifo(LMAC_TXBUF* item, uint8_t ac, uint8_t qtype);
LMAC_TXBUF* queue_dequeue(uint8_t ac, uint8_t qtype);
LMAC_TXBUF* queue_bulk_dequeue(uint16_t depth, uint8_t ac, uint8_t qtype);
bool queue_remove(LMAC_TXBUF *item, uint8_t ac, uint8_t qtype);
void queue_mqfree(uint8_t ac, uint8_t qtype);
void queue_show(uint8_t ac, uint8_t qtype);
bool queue_append(LMAC_TXBUF *append_tail, uint16_t depth, bool pos, uint8_t ac, uint8_t qtype);
bool queue_merge_front( uint8_t dest_ac , uint8_t dest_qtype , uint8_t src_ac , uint8_t src_qtype );

void lmac_qm_clear_sched(uint8_t ac);

void lmac_qm_flush_pending_queue(uint8_t ac, bool mode);
void lmac_qm_staggered_free(uint8_t ac);
bool lmac_qm_append_pend_queue(uint8_t ac);
void lmac_qm_restore_queue(uint8_t ac);
void lmac_qm_buffer_to_free(uint8_t ac, LMAC_TXBUF *buffer);
void lmac_qm_buffer_to_free_all(uint8_t ac, uint8_t qtype);
void qm_init();
void lmac_qm_show_all();
void lmac_qm_check();
void lmac_qm_show_statistics();

void lmac_qm_set_segment_duration(uint8_t ac, uint16_t duration);

bool lmac_qm_transit(uint8_t ac, uint16_t state);

void lmac_set_max_agg_num(int ac, int num);
uint8_t lmac_get_max_agg_num(int ac);
void lmac_set_aggregation(int ac, bool aggregation);
bool lmac_get_aggregation(int ac);
void hal_qm_init();
bool lmac_get_ba_session_tx(int ac);
void lmac_set_ba_session_tx(int ac, bool session);
void lmac_reset_ba_session_tx();

#if defined(INCLUDE_UMAC)
void lmac_set_max_agg_num_ap_by_macaddr(int ac, int num, int8_t vif_id, uint8_t* macaddr);
void lmac_set_max_agg_num_ap_by_aid(int ac, int num, int8_t vif_id, int aid);
void lmac_set_max_agg_num_ap_by_all(int ac, int num, int8_t vif_id);
uint8_t lmac_get_max_agg_num_ap_by_macaddr(int ac, int8_t vif_id, uint8_t* macaddr);
uint8_t lmac_get_max_agg_num_ap_by_aid(int ac, int8_t vif_id, int aid);
void lmac_set_aggregation_ap_by_macaddr(int ac, bool aggregation, int8_t vif_id, uint8_t* macaddr);
void lmac_set_aggregation_ap_by_aid(int ac, bool aggregation, int8_t vif_id, int aid);
void lmac_set_aggregation_ap_by_all(int ac, bool aggregation, int8_t vif_id);
bool lmac_get_aggregation_ap_by_macaddr(int ac, int8_t vif_id, uint8_t* macaddr);
bool lmac_get_aggregation_ap_by_aid(int ac, int8_t vif_id, int aid);
bool lmac_get_ba_session_tx_ap_by_macaddr(int ac, int8_t vif_id, uint8_t* macaddr);
bool lmac_get_ba_session_tx_ap_by_aid(int ac, int8_t vif_id, int aid);
void lmac_set_ba_session_tx_ap_by_macaddr(int ac, bool session, int8_t vif_id, uint8_t* macaddr);
void lmac_set_ba_session_tx_ap_by_aid(int ac, bool session, int8_t vif_id, int aid);
#else
inline void lmac_set_max_agg_num_ap_by_macaddr(int ac, int num, int8_t vif_id, uint8_t* macaddr) {lmac_set_max_agg_num(ac, num);};
inline void lmac_set_max_agg_num_ap_by_aid(int ac, int num, int8_t vif_id, int aid) {lmac_set_max_agg_num(ac, num);};
inline void lmac_set_max_agg_num_ap_by_all(int ac, int num, int8_t vif_id) {};
inline uint8_t lmac_get_max_agg_num_ap_by_macaddr(int ac, int8_t vif_id, uint8_t* macaddr) {return lmac_get_max_agg_num(ac);};
inline uint8_t lmac_get_max_agg_num_ap_by_aid(int ac, int8_t vif_id, int aid) {return lmac_get_max_agg_num(ac);};
inline void lmac_set_aggregation_ap_by_macaddr(int ac, bool aggregation, int8_t vif_id, uint8_t* macaddr) {lmac_set_aggregation(ac, aggregation);};
inline void lmac_set_aggregation_ap_by_aid(int ac, bool aggregation, int8_t vif_id, int aid) {lmac_set_aggregation(ac, aggregation);};
inline void lmac_set_aggregation_ap_by_all(int ac, bool aggregation, int8_t vif_id) {};
inline bool lmac_get_aggregation_ap_by_macaddr(int ac, int8_t vif_id, uint8_t* macaddr) {return lmac_get_aggregation(ac);};
inline bool lmac_get_aggregation_ap_by_aid(int ac, int8_t vif_id, int aid) {return lmac_get_aggregation(ac);};
inline bool lmac_get_ba_session_tx_ap_by_macaddr(int ac, int8_t vif_id, uint8_t* macaddr) {return lmac_get_ba_session_tx(ac);};
inline bool lmac_get_ba_session_tx_ap_by_aid(int ac, int8_t vif_id, int aid) {return lmac_get_ba_session_tx(ac);};
inline void lmac_set_ba_session_tx_ap_by_macaddr(int ac, bool session, int8_t vif_id, uint8_t* macaddr) {lmac_set_ba_session_tx(ac, session);};
inline void lmac_set_ba_session_tx_ap_by_aid(int ac, bool session, int8_t vif_id, int aid) {lmac_set_ba_session_tx(ac, session);};
#endif

static inline void set_mac_hdr_duration(LMAC_TXBUF *txbuf, uint16_t val)
{
	if (ieee80211_is_pv0(txbuf->machdr) && !ieee80211_is_pspoll(txbuf->machdr) && !txbuf->txi.inject)
		txbuf->machdr->duration_id = val;
}
#endif /* HAL_LMAC_QUEUE_MANAGER_H_ */
