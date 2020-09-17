#ifndef HAL_LMAC_QUEUE_MANAGER_11AH_H_
#define HAL_LMAC_QUEUE_MANAGER_11AH_H_

typedef struct _Sched_Info {
		uint8_t  valid      : 1;
		uint8_t  aggregated : 1;
		uint8_t  mpdu_cnt   : 6;
		uint16_t ssn        : 16;
} Sched_Info;

void lmac_qm_configure();
void lmac_qm_schedule(uint8_t ac, uint8_t mode);
bool lmac_process_tx_report(uint8_t ac);
void lmac_uplink_request_ndp (LMAC_TXBUF *buf);
void lmac_qm_recovery();
void lmac_qm_recovery_post_process();

#endif /* HAL_LMAC_QUEUE_MANAGER_11AH_H_ */
