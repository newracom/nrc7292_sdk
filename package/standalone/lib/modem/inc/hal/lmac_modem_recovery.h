#ifndef LMAC_MODEM_RECOVERY_H_
#define LMAC_MODEM_RECOVERY_H_


void lmac_detect_sn_missing(uint8_t ac, int16_t diff);
void lmac_detect_rxbuf_discard();

void lmac_init_recovery_test_timer();
void lmac_run_test_timer_modem_recovery(uint32_t interval, int32_t count);
void lmac_run_modem_recovery();
void lmac_show_stats_modem_recovery();
void lmac_show_keyinfo_modem_recovery();
uint32_t  lmac_get_modem_recovery_count();

void hal_lmac_run_presteps_modem_sleep();
void hal_lmac_run_poststeps_modem_sleep();

#endif /* LMAC_MODEM_RECOVERY_H_ */
