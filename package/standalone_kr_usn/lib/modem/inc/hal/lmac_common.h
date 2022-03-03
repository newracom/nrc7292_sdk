#ifndef HAL_LMAC_COMMON_H
#define HAL_LMAC_COMMON_H

#include "system_freertos.h"
#include "nrc-wim-types.h"
#include "protocol.h"
#include "lmac_type.h"
#if	defined(STANDARD_11N)
#include "lmac_11n.h"
#endif /* defined(STANDARD_11N) */

#if defined(STANDARD_11AH)
#include "protocol_11ah.h"
#include "lmac_11ah.h"
#endif /* defined(STANDARD_11AH) */

/* Semaphore Definition */
enum {
    MODEM_SEMAPHORE_TASK        = 0,
    MODEM_SEMAPHORE_ISR         = 1,
    MODEM_SEMAPHORE_REC         = 2,
    MODEM_SEMAPHORE_REC_ISR     = 3,
};

#if !defined (INCLUDE_NO_USE_LMAC_SEM)
#define LMAC_PS_SEM_TAKE(a) do{lmac_take_modem_semaphore(a);}while(0)
#define LMAC_PS_SEM_TAKE_RET(a) lmac_take_modem_semaphore(a)
#define LMAC_PS_SEM_GIVE(a) do{lmac_give_modem_semaphore(a);}while(0)
#else
//no need to use semaphore
#define LMAC_PS_SEM_TAKE(a) do{}while(0)
#define LMAC_PS_SEM_TAKE_RET(a) true
#define LMAC_PS_SEM_GIVE(a) do{}while(0)
#endif

#ifndef LMAC_MODEM_RECOVERY_TIME_MS
#define LMAC_MODEM_RECOVERY_TIME_MS 2000
#endif

// HAL Function
struct      cipher_def; // declare

void        lmac_init(int vector);
void        lmac_start(void);
void        lmac_idle_hook(void);
void        lmac_set_dl_callback(void (*dl_callback)(int vif_id, struct _SYS_BUF*) );
void        lmac_set_tbtt_callback(void (*tbtt_callback)(int8_t) );
void        lmac_mask_tbtt_irq(void);
void        lmac_unmask_tbtt_irq(void);
void        lmac_set_beacon_interval(uint8_t vif_id, uint16_t beacon_interval);
uint16_t    lmac_get_beacon_interval();

void        lmac_set_credit_callback(void (*credit_callback)(uint8_t ac, uint8_t value, bool inc) );
void        lmac_isr(int vector);
void        lmac_uplink_request_sysbuf(struct _SYS_BUF* buf);
int8_t      lmac_get_vif_id(GenericMacHeader* gmh);

void        lmac_key_counter_reset();
void        lmac_set_mac_address(int vif_id , uint8_t* addr);
void        lmac_set_dev_mac(int hw_index, uint8_t *mac_addr);
uint8_t*    lmac_get_mac_address(int vif_id);
void        lmac_set_enable_mac_addr(int vif_id, bool enable);
bool        lmac_get_enable_mac_addr(int vif_id);
void        lmac_set_bssid(int vif_id, uint8_t *bssid);
uint8_t*    lmac_get_bssid(int vif_id);
void        lmac_set_enable_bssid(int vif_id, bool enable);
bool        lmac_get_enable_bssid(int vif_id);
void        lmac_mode_bssid(int vif_id, bool ap_mode_en);
bool        lmac_set_aid(int vif_id, uint16_t aid);
int         lmac_get_aid(int vif_id);
void        lmac_set_mode(int vif_id, uint8_t mode);
uint8_t     lmac_get_device_mode(uint8_t vif_id);
#if defined (INCLUDE_RF_KILL)
void        lmac_set_rf_kill(bool value);
bool        lmac_get_rf_kill();
#endif
bool        lmac_is_ap(int vif_id);
bool        lmac_is_sta(int vif_id);
bool        lmac_is_mesh(int vif_id);
bool        lmac_is_mesh_ap(void);
void        lmac_set_cca_ignore(bool ignore);
void        lmac_set_cipher_ignore(bool ignore, uint8_t dir);
void        lmac_set_promiscuous_mode(int vif_id, uint8_t);
bool        lmac_get_promiscuous_mode(int vif_id);
bool        lmac_get_bypass_mgmt_mode(int vif_id);
void        lmac_set_bypass_mgmt_mode(int vif_id, bool);
void        lmac_set_bypass_beacon_mode(int vif_id, bool enable);
bool        lmac_set_erp_param(int vif_id, uint8_t, uint8_t, uint8_t);
int         lmac_get_cipher_icv_length(int type);
bool        lmac_sec_set_enable_key(int vif_id, bool enable);
bool        lmac_sec_get_enable_key(int vif_id);
bool        lmac_sec_set_akm(int vif_id, uint8_t akm);
uint8_t     lmac_sec_get_akm(int vif_id);
bool        lmac_sec_add_key(int vif_id, struct cipher_def *lmc, bool dummy);
bool        lmac_sec_del_key(int vif_id, struct cipher_def *lmc);
bool        lmac_sec_del_key_all(int vif_id);
void lmac_set_basic_rate(int vif_id, uint32_t basic_rate_set);
uint32_t    lmac_get_tsf_timer_high(int vif_id);
uint32_t    lmac_get_tsf_timer_low(int vif_id);
void        lmac_set_tsf_timer(int vif_id, uint32_t ts_hi, uint32_t ts_lo);
void        lmac_set_response_ind(int ac, uint8_t response_ind);
uint8_t     lmac_get_response_ind(int ac);
void        lmac_set_max_agg_num(int ac, int num);
// void        lmac_set_max_agg_num_ap_by_macaddr(int ac, int num, int8_t vif_id, uint8_t* macaddr);
// void        lmac_set_max_agg_num_ap_by_aid(int ac, int num, int8_t vif_id, int aid);
uint8_t     lmac_get_max_agg_num(int ac);
// uint8_t     lmac_get_max_agg_num_ap_by_macaddr(int ac, int8_t vif_id, uint8_t* macaddr);
// uint8_t     lmac_get_max_agg_num_ap_by_aid(int ac, int8_t vif_id, int aid);
void        lmac_set_aggregation(int ac, bool aggregation);
// void        lmac_set_aggregation_ap_by_macaddr(int ac, bool aggregation, int8_t vif_id, uint8_t* macaddr);
// void        lmac_set_aggregation_ap_by_aid(int ac, bool aggregation, int8_t vif_id, int aid);
bool        lmac_get_aggregation(int ac);
// bool        lmac_get_aggregation_ap_by_macaddr(int ac, int8_t vif_id, uint8_t* macaddr);
// bool        lmac_get_aggregation_ap_by_aid(int ac, int8_t vif_id, int aid);
void		lmac_set_agg_size(int ac, uint16_t mpdu_length);
uint16_t	lmac_get_agg_size(int ac);
bool        lmac_set_ht_operation_info(int vif_id, uint16_t ht_info);
bool		lmac_set_ht_capability(int vif_id, uint16_t ht_cap);
bool 		lmac_set_short_gi(int vif_id, uint8_t short_gi, bool gi_auto_flag);
void        lmac_enable_1m(bool enable, bool enable_dup);
bool        lmac_get_scanning(int vif_id);
void        lmac_set_scanning(int vif_id, bool en);
void 		lmac_set_rts(int vif_id, uint8_t mode, uint16_t threshold, int ndp);
int8_t 		lmac_get_ap_vif_id();
int8_t 		lmac_get_sta_vif_id();
int8_t 		lmac_get_mesh_vif_id();
void        lmac_set_pv1(int vif_id, bool en);
bool        lmac_get_pv1(int vif_id);
void        lmac_set_pn(int vif_id, uint64_t pn);
uint64_t    lmac_get_pn(int vif_id);

void        lmac_common_set_cfo_cal_en(bool en);
bool        lmac_common_get_cfo_cal_en();

void        lmac_uplink_request(LMAC_TXBUF *buf);
void 		lmac_uplink_request2 (LMAC_TXBUF *buf, int txque);
void        lmac_uplink_request_non_filtered (LMAC_TXBUF *buf);

void        lmac_contention_result_isr();
void        lmac_tx_done_isr();
void        lmac_rx_done_isr();
void        lmac_tsf_isr(uint8_t );
void        lmac_tbtt_isr();
void        lmac_ps_qosnull_txdone_isr();

bool        lmac_qm_transit(uint8_t, uint16_t);
bool        lmac_process_tx_report(uint8_t);	/// Processing tx result from hardware module

void        lmac_ampdu_init_info();
void        lmac_ampdu_set_info(uint8_t vif_id, bool ctrl_1m_prem_sup);

void        lmac_free_txbuffer(LMAC_TXBUF* buffer, uint8_t ac);

/// internal utility
void        lmac_check_legacy_tim(uint8_t *ie, int8_t vif_id);
void        lmac_qm_show_statistics();
void        lmac_dl_show_statistics();
void        lmac_qm_configure();
void        lmac_qm_schedule(uint8_t, uint8_t);
void        downlink_init();
bool        lmac_rx_post_process(struct _SYS_BUF*, void (*dl_cb)(int vif_id, struct _SYS_BUF *));

void        discard(struct _SYS_BUF* buffer);
uint16_t    bd_set_datalen(LMAC_TXBUF *, uint16_t, bool);
int         set_buffer_descriptor(LMAC_TXBUF *);
int         set_tx_vector(LMAC_TXBUF *, uint8_t, bool, struct frame_tx_info*);
uint32_t    get_valid_dl_desc_num();
void        fill_descriptor_all(struct _SYS_BUF*);
void        fill_descriptor(struct _SYS_BUF*);

void 		lmac_set_sniffer_mode(bool v);
void		lmac_set_sniffer_mode_beacon_display(bool v);
void		lmac_set_pmc(uint32_t after_ms);

void        lmac_print_semaphore();
bool        lmac_take_modem_semaphore(uint8_t type);
void        lmac_give_modem_semaphore(uint8_t type);
void        lmac_sys_background_task();

#if defined(STANDARD_11AH)
bool lmac_get_1m_center_lo(uint8_t cc_id);
void lmac_set_1m_center_lo(uint8_t cc_id, bool v);
int lmac_get_country_code_index(uint8_t *cc);
void lmac_uplink_request_ndp_sysbuf(struct _SYS_BUF* buf);
uint8_t s1g_get_bw(TXVECTOR *vector);
bool s1g_check_bw(uint8_t bw, uint8_t format);
bool s1g_check_preamble(uint8_t preamble_type);
bool lmac_check_s1g_ndp_frame(LMAC_TXBUF *buf);
uint16_t get_sig_length(TXVECTOR* vector);
bool get_sig_aggregation(TXVECTOR* vector);
uint8_t get_sig_short_gi(TXVECTOR* vector);
uint8_t get_sig_mcs(TXVECTOR* vector);
void set_sig_length(TXVECTOR* vector, uint16_t len);
void set_sig_aggregation(TXVECTOR* vector, bool agg);
void set_sig_short_gi(TXVECTOR* vector, uint8_t gi);
void set_sig_mcs(TXVECTOR* vector, uint8_t mcs);
void set_sig_doppler(TXVECTOR* vector , uint8_t doppler);
void lmac_set_bpn_register(uint8_t bpn);
void lmac_show_test_config_modem_recovery();
#endif /* defined(STANDARD_11AH) */

uint32_t get_idle_hook_count();
void lmac_update_stats(uint8_t dir, void *hdr, uint8_t mcs, uint8_t status);
uint8_t get_run_rx_gain_auto_test();
uint32_t lmac_get_tsf_h(int vif_id);
uint32_t lmac_get_tsf_l(int vif_id);
void lmac_set_mcs(int vif_id, uint8_t mcs);
uint8_t lmac_get_mcs(int vif_id);
void lmac_set_prim_ch_bw(uint8_t vif_id, uint8_t v);
uint8_t lmac_get_prim_ch_loc(uint8_t vif_id);
void lmac_set_prim_ch_loc(uint8_t vif_id, uint8_t v);
void lmac_set_ch_bw(uint8_t vif_id, uint8_t v);
uint8_t lmac_get_ch_bw(uint8_t vif_id);
void lmac_set_frequency(uint8_t vif_id, uint32_t v);
uint32_t lmac_get_frequency(uint8_t vif_id);
uint32_t lmac_get_mac80211_frequency(uint8_t vif_id);
void lmac_set_mac80211_frequency(uint8_t vif_id, uint32_t v);
uint8_t lmac_get_phy_txgain(uint8_t vif_id);
void lmac_set_phy_txgain(uint8_t vif_id, uint8_t v);
uint8_t lmac_get_phy_rxgain(uint8_t vif_id);
void lmac_set_phy_rxgain(uint8_t vif_id, uint8_t v);
uint8_t* lmac_get_country(uint8_t vif_id);
void lmac_set_country(uint8_t vif_id, uint8_t* v);
void lmac_set_format(uint8_t vif_id, uint8_t v);
void lmac_set_preamble_type(uint8_t vif_id, uint8_t v);
uint8_t lmac_get_short_gi(uint8_t vif_id);
uint8_t lmac_get_key_index(uint8_t vif_id);
void lmac_set_cipher_type(uint8_t vif_id, enum key_type k, enum CipherType v);
enum CipherType lmac_get_cipher_type(uint8_t vif_id, enum key_type k);
void lmac_set_max_agg_sched_num(uint8_t v);
uint8_t lmac_get_max_agg_sched_num();

#if defined(INCLUDE_DUTYCYCLE)
void duty_cycle_init();
void duty_cycle_set_pause(uint32_t time);
uint32_t duty_cycle_get_pause();
void lmac_check_duty_window();
#else
static inline void duty_cycle_init() {};
static inline uint32_t duty_cycle_get_pause() {return 0;};
static inline void duty_cycle_set_pause(uint32_t time) {};
static inline void lmac_check_duty_window() {};
#endif /* defined(INCLUDE_DUTYCYCLE) */

#if defined(INCLUDE_BD_SUPPORT)
void lmac_tx_power_init(uint8_t* index);
void lmac_tx_power_init_default(void);
#else
void lmac_tx_power_init();
#endif /* defined(INCLUDE_BD_SUPPORT) */

#if !defined(NRC7292)
void lmac_set_rx_buffer_lookup(bool v);
#endif /* !defined(NRC7292) */

void lmac_task_handle_data(void *param);
void lmac_task_send_queue(LMacEvent evt, uint8_t sub_evt);

#if defined(INCLUDE_TEST_LBT_KR)
void lmac_init_lbt(uint16_t cs_duration, uint32_t pause_time, uint32_t tx_resume_time);
void lmac_update_last_tsf(uint32_t time);
bool lmac_check_tx_pause_flag();
void lmac_check_lbt_pause_time(uint8_t ac);
#endif
uint32_t lmac_get_cs_time(void);
void lmac_set_cs_time(uint16_t cs_time);
#endif /* HAL_LMAC_COMMON_H */
