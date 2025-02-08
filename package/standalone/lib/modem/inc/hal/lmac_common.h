#ifndef HAL_LMAC_COMMON_H
#define HAL_LMAC_COMMON_H

#include "system_freertos.h"
#include "nrc-wim-types.h"
#include "protocol.h"
#include "lmac_type.h"
#include "lmac_config.h"
#include "lmac_ps_common.h"
#include "protocol_11ah.h"
#include "lmac_11ah.h"

#if defined(INCLUDE_BD_SUPPORT_TARGET_VERSION)
#define	MAX_AVAILABLE_HW_VERSION	2047
#define	DEFAULT_HW_VERSION	65535
#endif

/* Semaphore Definition */
enum {
    MODEM_SEMAPHORE_TASK        = 0,
    MODEM_SEMAPHORE_ISR         = 1,
    MODEM_SEMAPHORE_REC         = 2,
    MODEM_SEMAPHORE_REC_ISR     = 3,
};

typedef struct _CCA_RESULT_4M
{
    double cca_1m_prim;
    double cca_2m_prim;
    double cca_2m_sec;
} CCA_RESULT_4M;

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
#if !defined(CUNIT)
#define lmac_get_bssid(x)			get_bssid(x)
#define lmac_get_aid(x)				get_aid(x)
#define lmac_get_device_mode(x)			get_device_mode(x)
#define lmac_sec_set_enable_key(x, y)		set_cipher_en(x, y)
#define lmac_sec_get_enable_key(x)		get_cipher_en(x)
#define lmac_sec_set_akm(x, y)			set_akm(x, y)
#define lmac_sec_get_akm(x)			get_akm(x)
#define lmac_set_scanning(x, y)			set_scanning(x, y)
#define lmac_get_scanning(x)			get_scanning(x)
#if defined(INCLUDE_PV1)
#define lmac_set_pn(x, y)			set_pn(x, y)
#define lmac_get_pn(x)				get_pn(x)
#define lmac_set_pv1(x, y)			set_pv1_en(x, y)
#define lmac_get_pv1(x)				get_pv1_en(x)
#endif
#define lmac_common_get_cfo_cal_en()		get_common_cfo_cal_en()
#define lmac_get_1m_center_lo(x)		get_1m_center_lo(x)
#define lmac_set_1m_center_lo(x, y)		set_1m_center_lo(x, y)
#define lmac_set_mcs(x, y)			set_mcs(y)
#define lmac_get_mcs(x)				get_mcs()
#define lmac_set_prim_ch_bw(x, y)		set_prim_ch_bw(y)
#define lmac_get_prim_ch_bw(x)			get_prim_ch_bw()
#define lmac_set_prim_ch_loc(x, y)		set_prim_ch_loc(y)
#define lmac_get_prim_ch_loc(x)			get_prim_ch_loc()
#define lmac_set_ch_bw(x, y)			set_ch_bw(y)
#define lmac_get_ch_bw(x)			get_ch_bw()
#define lmac_set_frequency(x, y)		set_frequency(y)
#define lmac_get_frequency(x)			get_frequency()
#define lmac_set_mac80211_frequency(x, y)	set_mac80211_frequency(y)
#define lmac_get_mac80211_frequency(x)		get_mac80211_frequency()
#define lmac_set_phy_txgain(x, y)		set_phy_txgain(y)
#define lmac_get_phy_txgain(x)			get_phy_txgain()
#define lmac_set_phy_rxgain(x, y)		set_phy_rxgain(y)
#define lmac_get_phy_rxgain(x)			get_phy_rxgain()
#define lmac_set_country(x, y)			set_country(y)
#define lmac_get_country(x)			get_country()
#define lmac_set_rate_control(x, y)		set_rate_ctrl_en(x, y)
#define lmac_get_rate_control(x)		get_rate_ctrl_en(x)
#define lmac_set_rc_mode(x, y)			set_rate_ctrl_mode(x, y)
#define lmac_get_rc_mode(x)				get_rate_ctrl_mode(x)
#define lmac_set_format(x, y)			set_format(x, y)
#define lmac_set_preamble_type(x, y)		set_preamble_type(x, y)
#define lmac_get_short_gi(x)			get_short_gi(x)
#define lmac_get_key_index(x)			get_key_index(x)
#define lmac_set_cipher_type(x, y, z)		set_cipher_type(x, y, z)
#define lmac_get_cipher_type(x, y)		get_cipher_type(x, y)
#define lmac_set_pmc(x)				drv_pmc_sleep(PMC_DEEPSLEEP0, x)
#define lmac_get_tsf_h(x)				drv_lmac_get_tsf_hi(x)
#define lmac_get_tsf_l(x)				drv_lmac_get_tsf_lo(x)
#define lmac_mode_bssid(x, y)			drv_mode_bssid(x, y)
#define lmac_set_max_agg_sched_num(x)	set_max_agg_sched_num(x)
#define lmac_get_max_agg_sched_num(x)	get_max_agg_sched_num(x)
#define lmac_get_rts(x)                 get_rts(x)
#if defined(INCLUDE_DYNAMIC_FRAG)
#define lmac_set_dynamic_frag(x)		set_dynamic_frag(x)
#define lmac_get_dynamic_frag()		get_dynamic_frag()
#endif
#if defined(INCLUDE_DEFRAG)
#define lmac_set_dynamic_defrag(x)		set_dynamic_defrag(x)
#define lmac_get_dynamic_defrag()		get_dynamic_defrag()
#endif
#if defined(INCLUDE_AUTH_CONTROL)
#define lmac_set_enable_auth_ctrl(x)		set_enable_auth_ctrl(x)
#define lmac_get_enable_auth_ctrl()		get_enable_auth_ctrl()
#endif

#else
void        lmac_set_pv1(int vif_id, bool en);
bool        lmac_sec_set_akm(int vif_id, uint8_t akm);
uint32_t    lmac_get_tsf_h(int vif_id);
uint32_t    lmac_get_tsf_l(int vif_id);
uint8_t     lmac_get_ch_bw(uint8_t vif_id);
#endif

struct      cipher_def; // declare

#define CCA_THRESHOLD_MIN	(-100)
#define CCA_THRESHOLD_MAX	(-35)

#if defined(NRC5291)
void        sbr_init(int vector);
#endif
void        lmac_init(int vector);
void        lmac_default_gain(void);
void        lmac_start(void);
void        lmac_idle_hook(void);
void        lmac_set_dl_callback(void (*dl_callback)(int vif_id, struct _SYS_BUF*) );
void        lmac_set_tbtt_callback(void (*tbtt_callback)(int8_t) );
void        lmac_set_beacon_interval(uint8_t vif_id, uint16_t beacon_interval);
uint16_t    lmac_get_beacon_interval();
uint32_t	lmac_get_next_tbtt(int vif_id, uint32_t sbi_us);
void        lmac_set_credit_callback(void (*credit_callback)(uint8_t ac, uint8_t value, bool inc) );
void        lmac_isr(int vector);
void        lmac_uplink_request_sysbuf(struct _SYS_BUF* buf);
int8_t      lmac_get_vif_id(GenericMacHeader* gmh);
void        lmac_key_counter_reset();
void        lmac_set_mac_address(int vif_id , uint8_t* addr);
void        lmac_set_dev_mac(int hw_index, uint8_t *mac_addr);
uint8_t*    lmac_get_mac_address(int vif_id);
#if defined(NRC7394)
void        lmac_set_enable_mac_addr(int vif_id, bool enable);
bool        lmac_get_enable_mac_addr(int vif_id);
#endif /* defined(NRC7394) */
void        lmac_set_bssid(int vif_id, uint8_t *bssid);
void        lmac_set_enable_bssid(int vif_id, bool enable);
bool        lmac_get_enable_bssid(int vif_id);
bool        lmac_set_aid(int vif_id, uint16_t aid);
void        lmac_set_mode(int vif_id, uint8_t mode);
bool        lmac_is_ap(int vif_id);
bool        lmac_is_sta(int vif_id);
bool        lmac_is_mesh(int vif_id);
#if defined (INCLUDE_IBSS)
bool        lmac_is_ibss(int vif_id);
#endif
bool        lmac_is_mesh_ap(void);
bool        lmac_is_relay(void);
bool        lmac_is_concurrent(void);
bool        lmac_is_rev(void);
void        lmac_set_cca_ignore(bool ignore);
uint8_t     lmac_get_cca_ignore();
bool        lmac_scan_set_channel(int vif_id, double freq, bw_t bw);
double      lmac_scan_read_cca(int vif_id, double freq, bw_t bw, uint32_t scan_duration);
bool        lmac_scan_read_cca_fast(int vif_id, double freq, bw_t bw, uint32_t scan_duration, CCA_RESULT_4M *cca_results);
void        lmac_set_cipher_ignore(bool ignore, uint8_t dir);
void        lmac_set_promiscuous_mode(int vif_id, uint8_t);
bool        lmac_get_promiscuous_mode(int vif_id);
bool        lmac_get_bypass_mgmt_mode(int vif_id);
void        lmac_set_bypass_mgmt_mode(int vif_id, bool);
bool        lmac_get_bypass_beacon_mode(int vif_id);
void        lmac_set_bypass_beacon_mode(int vif_id, bool enable);
int         lmac_get_cipher_icv_length(int type);
bool        lmac_sec_add_key(int vif_id, struct cipher_def *lmc, bool dummy);
bool        lmac_sec_del_key(int vif_id, struct cipher_def *lmc);
bool        lmac_sec_del_key_all(int vif_id);
void        lmac_set_basic_rate(int vif_id, uint32_t basic_rate_set);
uint32_t    lmac_get_tsf_timer_high(int vif_id);
uint32_t    lmac_get_tsf_timer_low(int vif_id);
void        lmac_set_tsf_timer(int vif_id, uint32_t ts_hi, uint32_t ts_lo);
void        lmac_set_response_ind(int ac, uint8_t response_ind);
uint8_t     lmac_get_response_ind(int ac);
void        lmac_set_max_agg_num(int ac, int num);
uint8_t     lmac_get_max_agg_num(int ac);
void        lmac_set_aggregation(int ac, bool aggregation);
bool        lmac_get_aggregation(int ac);
void        lmac_set_agg_size(int ac, uint16_t mpdu_length);
uint16_t    lmac_get_agg_size(int ac);
#if defined (INCLUDE_MANUAL_AGG_NUM)
bool        lmac_get_agg_manual(int ac);
void        lmac_set_agg_manual(int ac, bool manaul);
#endif
bool        lmac_set_short_gi(int vif_id, uint8_t short_gi, bool gi_auto_flag);
void        lmac_enable_1m(bool enable, bool enable_dup);
bool        lmac_set_rts_rid(int vif_id, int rid);
int         lmac_get_rts_rid(void);
bool        lmac_set_rts(int vif_id, uint8_t mode, uint16_t threshold, int rid);
int         lmac_get_cts(int vif_id);
bool        lmac_set_cts(int ndp);
int8_t      lmac_get_ap_vif_id();
int8_t      lmac_get_sta_vif_id();
int8_t      lmac_get_mesh_vif_id();
#if defined (INCLUDE_IBSS)
int8_t      lmac_get_ibss_vif_id();
#endif
void        lmac_set_cfo_cal_en(bool en);
void		lmac_inc_bcmc_cnt(uint8_t bcmc_cnt);
uint8_t		lmac_get_bcmc_cnt(void);
void        lmac_uplink_request(LMAC_TXBUF *buf);
void        lmac_uplink_request2(LMAC_TXBUF *buf, int txque, bool reschedule);
void        lmac_uplink_request_non_filtered (LMAC_TXBUF *buf);
void        lmac_downlink_request(int vif_id, SYS_BUF *packet);
void        lmac_contention_result_isr();
void        lmac_tx_done_isr();
void        lmac_rx_done_isr();
void        lmac_tsf_isr(uint8_t );
void        lmac_tbtt_isr(int vif_id);
void        lmac_ps_qosnull_txdone_isr();
uint8_t		lmac_qm_state();
bool        lmac_qm_transit(uint8_t, uint16_t);
bool        lmac_process_tx_report(uint8_t);	/// Processing tx result from hardware module
void        lmac_ampdu_init_info(uint8_t vif_id);
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
void        lmac_set_sniffer_mode(bool v);
void        lmac_set_sniffer_mode_beacon_display(bool v);
void        lmac_print_semaphore();
bool        lmac_take_modem_semaphore(uint8_t type);
void        lmac_give_modem_semaphore(uint8_t type);
void        lmac_sys_background_task();
int         lmac_get_country_code_index(void);
void        lmac_uplink_request_ndp_sysbuf(struct _SYS_BUF* buf);
uint8_t     s1g_get_bw(TXVECTOR *vector);
bool        s1g_check_bw(uint8_t bw, uint8_t format);
bool        s1g_check_preamble(uint8_t preamble_type);
bool        lmac_check_s1g_ndp_frame(LMAC_TXBUF *buf);
uint16_t    get_sig_length(TXVECTOR* vector);
bool        get_sig_aggregation(TXVECTOR* vector);
uint8_t     get_sig_short_gi(TXVECTOR* vector);
uint8_t     get_sig_mcs(TXVECTOR* vector);
void        set_sig_length(TXVECTOR* vector, uint16_t len);
void        set_sig_aggregation(TXVECTOR* vector, bool agg);
void        set_sig_short_gi(TXVECTOR* vector, uint8_t gi);
void        set_sig_mcs(TXVECTOR* vector, uint8_t mcs);
void        set_sig_doppler(TXVECTOR* vector , uint8_t doppler);
bool        get_sig_rx_aggregation(LMAC_RXHDR *rxhdr);
uint8_t		get_sig_rx_mcs (LMAC_RXHDR *rxhdr);
uint8_t 	get_sig_rx_response_ind (LMAC_RXHDR *rxhdr);
bool 		get_sig_rx_ndp_ind (LMAC_RXHDR *rxhdr);
void        lmac_set_bpn_register(uint8_t bpn);
void        lmac_show_test_config_modem_recovery();
#if defined(NRC7292)
uint8_t     lmac_set_txpwr_resp_frame(uint8_t bandwidth);
#else
void        lmac_set_txpwr_resp_frame(void);
#endif
uint32_t    lmac_get_idle_hook_count();
#if defined (DEBUG_MAC_STATS)
void        lmac_init_mac_stats();
void        lmac_update_stats(uint8_t dir, void *hdr, uint8_t mcs, uint8_t status);
#endif /* DEBUG_MAC_STATS */
uint8_t     get_run_rx_gain_auto_test();
int         lmac_get_cca_device_type(void);
bool        lmac_set_cca_device_type(int device_type);
bool        lmac_set_cca_threshold(int8_t cca_threshold);
int8_t      lmac_get_cca_threshold(void);
void        lmac_set_mcs(int vif_id, uint8_t mcs);
uint8_t     lmac_get_mcs(int vif_id);
bool        lmac_get_rf_kill();
void        lmac_set_rf_kill(bool kill);
bool        lmac_get_bd_use();
void        lmac_set_bd_use(bool bd_on);
void lmac_auto_tx_gain_enable(bool enable);
#if defined(INCLUDE_BD_SUPPORT)
void lmac_tx_power_init(uint8_t ch_index, uint8_t* data);
#else
void lmac_tx_power_init();
#endif /* defined(INCLUDE_BD_SUPPORT) */
#if !defined(NRC7292)
void lmac_set_rx_buffer_lookup(bool v);
#endif /* !defined(NRC7292) */
/* To test lookup interrupt, define LOOK_UP_INT_TEST */
//#define LOOK_UP_INT_TEST
void lmac_task_handle_data(void *param);
void lmac_task_send_queue(LMacEvent evt, uint8_t sub_evt);
/* LBT-related function */
bool lmac_support_lbt (void);
void lmac_lbt_init(uint16_t cs_duration, uint32_t pause_time, uint32_t tx_resume_time);
void lmac_lbt_deinit(void);
void lmac_lbt_set_cs_time(uint16_t cs_time);
uint16_t lmac_lbt_get_cs_time();
void lmac_lbt_set_cs_time_upper(uint16_t quotient);
uint16_t lmac_lbt_get_cs_time_upper();
void lmac_lbt_set_pause_time(uint32_t pause_time);
uint32_t lmac_lbt_get_pause_time();
void lmac_lbt_set_resume_time(uint32_t resume_time);
uint32_t lmac_lbt_get_resume_time();
void lmac_lbt_set_backup_aifsn(uint8_t ac, uint8_t aifsn);
uint8_t lmac_lbt_get_backup_aifsn(uint8_t ac);
bool lmac_check_tx_pause_flag();
#if defined(INCLUDE_LEGACY_ACK)
void lmac_set_ack_configure(uint8_t type);
#endif
void lmac_set_sw_reset();
void udelay(uint32_t delay);
void mdelay(uint32_t delay);
#endif /* HAL_LMAC_COMMON_H */
