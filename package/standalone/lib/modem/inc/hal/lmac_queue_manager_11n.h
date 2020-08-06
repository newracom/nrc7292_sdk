#ifndef HAL_LMAC_QUEUE_MANAGER_11N_H_
#define HAL_LMAC_QUEUE_MANAGER_11N_H_

typedef struct _Sched_Info {
		uint8_t  valid      : 1;
		uint8_t  aggregated : 1;
		uint8_t  mpdu_cnt   : 6;
		uint16_t ssn        : 16;
		uint8_t mode;
		uint32_t queue;
		uint8_t n_max_mpdu;
		uint8_t n_mpdu;
		uint32_t* ul_max_table;

		uint32_t psdu_len;
		uint32_t tot_duration;

        LMAC_TXBD* bd;
        TXVECTOR*  vector;
} Sched_Info;

LMAC_TXBUF* peek_tx_queue(uint32_t* queue, uint8_t ac);
bool schedule_one_mpdu(LMAC_TXBUF *buffer, uint8_t ac, uint8_t mode);
LMAC_TXBUF* hw_build_ppdu(uint8_t mode, uint8_t ac);
static uint32_t mk_tx_command(uint32_t oft, uint32_t aifsn, uint32_t cw);
static void hw_set_NAN( LMAC_TXBUF *bd, uint32_t addr);
void lmac_bd_print(LMAC_TXBUF* buf,uint8_t type);
void lmac_vector_print(LMAC_TXBUF* buf , uint8_t type);
void lmac_qm_configure();
void lmac_qm_schedule(uint8_t ac, uint8_t mode);
bool lmac_process_tx_report(uint8_t ac);

#endif /* HAL_LMAC_QUEUE_MANAGER_11N_H_ */
