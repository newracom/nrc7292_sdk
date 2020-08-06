#ifndef HAL_LMAC_STATS_H
#define HAL_LMAC_STATS_H

struct lmac_wa_tx {
	uint16_t	m_wa_tx_result_case1;
	uint16_t	m_wa_tx_result_case2;
	uint16_t	m_wa_tx_result_case3;
	uint16_t	m_wa_tx_result_case4;
	uint16_t	m_wa_tx_isr_clear_winac;
	uint16_t	m_wa_tx_isr_clear_dl;
	uint16_t	m_wa_tx_isr_clear_etc;
	uint32_t	m_wa_remain_tx_isr;
};

struct lmac_stats {
	uint32_t	m_total_rx_byte;
	int32_t		m_total_beacon_rssi;
	int32_t		m_total_beacon_cnt;
	uint32_t	m_total_length;
	uint16_t	m_total_bw;
	int16_t		m_total_mcs;
	uint16_t	m_total_gi;
	uint16_t	m_total_doppler;
	uint32_t	m_total_symbol;
	uint16_t	m_rssi_zero_cnt;
	uint16_t	m_info_error_cnt;
	uint16_t	m_vector_cross_cnt;
	uint16_t	m_vector_agg_cnt;
	uint16_t	m_vector_fix_cnt;
	uint32_t	m_discard_count;
	uint16_t	m_ndpba_send_fail_cnt;
	uint16_t	m_ndpba_send_fail_max_delay;
	struct lmac_wa_tx wa_tx;
};

void lmac_stats_init();
struct lmac_stats * stats();

#endif /* HAL_LMAC_STATS_H */
