#ifndef LMAC_DOWNLINK_H
#define LMAC_DOWNLINK_H

#include "system_type.h"

enum lmac_rx_policy {
	DROP_BEFORE_DOWNLINK_CALLBACK = 1,
};

struct lmac_rx_h_data {
	int8_t vif_id;
	struct _SYS_BUF *buffer;
	struct _LMAC_RXBUF *vector;
	GenericMacHeader *mh;
	enum lmac_rx_policy flags;
};

typedef	enum dl_statistic{
	    DL_GOOD,
	    DL_ERR_CRC,
	    DL_ERR_MATCH,
	    DL_ERR_LEN,
	    DL_ERR_KEY,
	    DL_ERR_MIC,
	    DL_ERR_DESC,
	    DL_ERR_NOIND,
	    DL_UNDER_RUN,
	    DL_OVER_RUN,                    // this is for statistics only
	    DL_BUFFER_FULL,
	    MAX_DL_STATS                    // this is just a marker
} DL_STATISTIC;

typedef struct desc_ring {
	uint32_t    m_start;            ///< address of first descriptor
	uint32_t    m_end;              ///< address of last descriptor
	uint32_t    m_current;          ///< address of current descriptor
	uint32_t    m_current_desc;		///< address of current descriptor for including further information
	uint32_t    m_size;             ///< Register size
	uint32_t    m_base;             ///< Base address of buffer

	struct      _SYS_BUF *m_tail;   ///< Tail of unfinished dl/rx list
	uint32_t    m_head;             ///< First fragment of tx list
	uint32_t	m_fill;				///< Fill the Buffer to tail
	uint32_t	m_fill_desc;		///< Fill the Buffer to tail for including further information

	uint8_t     m_type;                             ///< Type of Descriptor ring
	uint8_t 	m_stats[MAX_DL_STATS];              ///< Descriptor error counts
	bool        m_valid[LMAC_CONFIG_DL_DESCRIPTOR]; ///< Prevent wrap over of ring
} DESC_RING;

SYS_BUF* rx_read_dl_desc();
void nrc7291_rxvect_rotate_war(struct lmac_rx_h_data *rx);

#if defined(STANDARD_11AH)
void hal_dl_recovery_post_process();
void hal_dl_cleanup_dl_ring();
bool is_ndp_preq(struct lmac_rx_h_data *rx);
#if defined(INCLUDE_STA_SIG_INFO)
void hal_dl_process_signal_info();
#endif
#endif

#endif /* LMAC_DOWNLINK_H */
