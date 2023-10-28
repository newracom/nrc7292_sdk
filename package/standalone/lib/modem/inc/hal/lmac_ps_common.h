#ifndef __HAL_LMAC_PS_COMMON_H__
#define __HAL_LMAC_PS_COMMON_H__

#define SET_PS_SLEEP_MODE(variable, value)	\
	do {									\
		variable &= ~(0x03); 				\
		variable |= value & 0x03; 			\
	} while(0);

#define GET_PS_SLEEP_MODE(variable) \
	(variable & 0x03)

#define SET_PS_TIM_MODE(variable, value)	\
	do {									\
		variable &= ~((0x01) << 2);			\
		variable |= (value & 0x01) << 2;	\
	} while(0);

#define GET_PS_TIM_MODE(variable)	\
	(variable & 0x04) >> 2

typedef enum _ps_mode {
    PS_MODE_NO,
    PS_MODE_PSNONPOLL,
    PS_MODE_PSPOLL,
    PS_MODE_WMMPS,
    PS_MODE_NONTIM
} ps_mode;

typedef enum _ps_sleep_mode {
	PS_SLEEP_MODE_MODEM,
	PS_SLEEP_MODE_DEEP
} ps_sleep_mode;

typedef enum _ps_tim_mode {
	PS_NON_TIM_MODE,
	PS_TIM_MODE
} ps_tim_mode;

typedef enum _ps_state {
    PS_STATE_IDLE,
    PS_STATE_ACTIVE,
    PS_STATE_ACTIVE_TO_DOZE,
	PS_STATE_DOZE,
    PS_STATE_DOZE_TO_ACTIVE
} ps_state;

typedef enum _ps_callback_cmd {
	PS_BEFORE_SLEEP,
	PS_AFTER_SYSTEM_WAKEUP,
	PS_AFTER_WAKEUP
} ps_callback_cmd;

typedef enum _ps_pm_type {
	PS_PM_TYPE_NONE,
	PS_PM_MODEMSLEEP,
	PS_PM_DEEPSLEEP_TIM,
	PS_PM_DEEPSLEEP_NONTIM
} ps_pm_type;

typedef enum _ps_pm_send_state {
	PS_PM_SEND_PROCESSING,
	PS_PM_SEND_SUCCESS,
	PS_PM_SEND_FAIL
} ps_pm_send_state;

typedef void (*ps_doze_cb)(void);
typedef void (*ps_wake_cb)(void);

#define MAX_CB 5

typedef struct _lmac_dynamic_ps {
	uint32_t	dyn_ps_timeout_us; //timeout for dynamic ps in us unit
	uint32_t	last_check_tsf; //tsf when dynamic ps is expired
	uint32_t	last_tx_us; //lower tsf for last tx frame
	uint32_t	last_rx_us; //lower tsf for last rx frame
	uint32_t	last_bcmc_rx_us; //lower tsf for last rx bc/mc frame

	bool	expired; //whether dynamic ps is expired
} lmac_dynamic_ps;

typedef struct _lmac_ps_info {
	ps_mode     	mode;
	ps_sleep_mode	sleep_mode;

	ps_state	    state;
	ps_state	    tx_state;
	ps_state	    rx_state;
	ps_state	    rx_state_bcmc;
	ps_state		tx_state_pm;
	ps_state	    beacon_tim;

	bool			is_wakeup;
	bool			traffic_indicator;

	ps_doze_cb		doze_cb[ MAX_CB ];
	uint8_t			doze_cb_num;

	ps_wake_cb		wake_cb[ MAX_CB ];
	uint8_t 	    wake_cb_num;

	uint64_t		target_interval_ms;
	uint32_t	    wait_rx_before_deepsleep_us;
	uint64_t	    wakeup_after_deepsleep_us;
	uint32_t		minimum_sleep_interval_us;
	uint32_t	    deepsleep_start_us;
	uint32_t		beacon_rx_us;
	//uint32_t		beacon_interval_us;
	uint32_t		ready_for_wakeup_us;
	bool    	    non_tim;
	bool    	    keep_alive;
	int8_t			vif_id;
	uint8_t 		dtim_count;
	uint32_t	    next_dtim_us;
	bool			sent_pm0;
	bool			activate;
	bool			associated;
	uint32_t		last_ps_tsf;
	uint32_t		total_elapsed_time_us;
	uint32_t		total_elapsed_time_assoc_us;
	uint32_t		total_elapsed_time_modem_sleep_ms;

	uint32_t		last_req_time_null_frame_us;
	uint32_t		null_frame_isr_threshold;
	uint32_t 		wakeup_margin;

	uint32_t		total_count_qosdata_fail;
	bool			first_ps;

} lmac_ps_info;

#if defined(INCLUDE_MODEM_SLEEP)
void lmac_ps_init_base();
void lmac_ps_init(uint32_t wait_tx , uint32_t wait_rx , uint64_t wakeup_after_deepsleep_us );
bool lmac_ps_get_modem_sleep_support();
bool hal_lmac_ps_doze();
bool hal_lmac_ps_wake();
bool lmac_can_start_ps();
void lmac_process_ps();
bool lmac_ps_add_doze_cb( ps_doze_cb cb );
bool lmac_ps_add_wake_cb( ps_wake_cb cb );
void lmac_ps_set_wakeup();
bool lmac_ps_is_wakeup();
int lmac_ps_stop();
void lmac_ps_modemsleep_stop();
ps_state lmac_ps_get_tx_state();
ps_state lmac_ps_get_tx_state_pm();
ps_state lmac_ps_get_beacon_tim();
int8_t lmac_ps_get_vifid();
bool lmac_ps_is_modem_sleep_mode();
bool lmac_ps_is_modem_sleep(int8_t vif_id);
bool lmac_ps_set_halt_modem_sleep(int8_t vif_id, bool stop);
void lmac_ps_set_beacon_tim(ps_state state);
void lmac_ps_set_rx_state(ps_state state);
ps_state lmac_ps_get_rx_state();
void lmac_ps_set_tx_state_pm(ps_state state);
void lmac_ps_set_bcmc_rx_state(ps_state tsf);
ps_state lmac_ps_get_bcmc_rx_state();

/* in lmac_ps_psnonpoll.c */
void lmac_ps_active();
void lmac_ps_doze();
void lmac_ps_active_to_doze();
void lmac_ps_doze_to_active();
void lmac_ps_check_qm();

#else /* INCLUDE_MODEM_SLEEP */

static inline void lmac_ps_init_base() {}
static inline void lmac_ps_init(uint32_t wait_tx , uint32_t wait_rx , uint64_t wakeup_after_deepsleep_us ) {}
static inline bool lmac_ps_get_modem_sleep_support() {return false;}
static inline bool hal_lmac_ps_doze() {return false;}
static inline bool hal_lmac_ps_wake() {return false;}
static inline bool lmac_can_start_ps() {return false;};
static inline void lmac_process_ps() {}
static inline bool lmac_ps_add_doze_cb( ps_doze_cb cb ) {return false;}
static inline bool lmac_ps_add_wake_cb( ps_wake_cb cb ) {return false;}
static inline void lmac_ps_set_wakeup() {}
static inline bool lmac_ps_is_wakeup() {return false;}
static inline int lmac_ps_stop() {return -1;}
static inline void lmac_ps_modemsleep_stop() {}
static inline ps_state lmac_ps_get_tx_state() {return 0;}
static inline ps_state lmac_ps_get_tx_state_pm() {return 0;}
static inline ps_state lmac_ps_get_beacon_tim() {return 0;}
static inline ps_state lmac_ps_get_rx_state() {return 0;}
static inline ps_state lmac_ps_get_bcmc_rx_state() {return 0;}
static inline int8_t lmac_ps_get_vifid() {return 0;}
static inline bool lmac_ps_is_modem_sleep_mode() {return false;}
static inline bool lmac_ps_is_modem_sleep(int8_t vif_id) {return false;}
static inline bool lmac_ps_set_halt_modem_sleep(int8_t vif_id, bool stop) {return false;}
static inline void lmac_ps_set_beacon_tim(ps_state state) {}
static inline void lmac_ps_set_rx_state(ps_state state) {}
static inline void lmac_ps_set_tx_state_pm(ps_state state) {}
static inline void lmac_ps_set_bcmc_rx_state(ps_state tsf) {}
#endif /* INCLUDE_MODEM_SLEEP */

ps_mode lmac_ps_get_psmode();
void lmac_ps_go_doze();
void lmac_ps_execute_sleepmode(uint64_t after_ms);
ps_mode lmac_ps_get_mode();
ps_state lmac_ps_get_state();
void lmac_ps_set_pm0_sent(bool sent);
bool lmac_ps_get_pm0_sent();

/* API for SDK */
void lmac_ps_go_sleep_alone(uint8_t mode, uint64_t duration);
int lmac_ps_set_sleep(uint8_t sleep_mode, uint64_t interval_ms);

/* Retention APIs */
bool lmac_ps_get_target_wake();
void lmac_ps_detect_qosdata_fail();
static void lmac_ps_go_deepsleep();
bool lmac_ps_get_ret_recovered();
bool lmac_ps_get_ret_validity();
void lmac_ps_set_first_ps(bool first);
void lmac_ps_set_cca_threshold(int cca_threshold);
void lmac_ps_set_sync_time_ms(uint32_t time);
uint32_t lmac_ps_get_sync_time_ms(void);
void lmac_ps_set_addr(uint8_t *addr, bool ap);
void lmac_ps_set_country(uint8_t *cc);
#if defined(NRC7394)
void lmac_ps_set_freq(uint32_t freq);
#endif
void lmac_ps_set_ssid(uint8_t *ssid, uint8_t  len);
void lmac_ps_set_ndp_preq(uint8_t enable);
void lmac_ps_set_guard_interval(uint8_t gi);
void lmac_ps_set_rts_threshold(uint32_t rtstr);
void lmac_ps_set_key(uint8_t key_id, uint8_t key_type, uint32_t *key);
void lmac_ps_set_key_tsc(uint8_t key_id, uint64_t tsc);
void lmac_ps_set_edca(uint8_t ac, uint8_t aifsn, uint16_t cwmin, uint16_t cwmax,  uint16_t txop);
void lmac_ps_set_ampdu(uint8_t ac, uint8_t max_agg);
void lmac_ps_set_ampdu_enable(uint8_t ac, bool enable);
void lmac_ps_set_ampdu_size(uint8_t ac, uint16_t limit_size);
void lmac_ps_set_ampdu_manual(uint8_t ac, uint8_t manual);
void lmac_ps_set_tx_seqnum(uint8_t tid, uint16_t sn);
void lmac_ps_set_rx_seqnum(uint8_t tid, uint16_t sn, uint16_t bitmap);
void lmac_ps_set_ba_state(uint8_t tid, bool add, uint8_t ba_state);
void lmac_ps_set_txpwr(uint8_t tx_pwr, uint8_t type);
void lmac_ps_set_rxgain(uint8_t rx_gain);
void lmac_ps_set_sgi(uint8_t sgi);
void lmac_ps_set_bss_maxidle(uint8_t option, uint16_t usf_period);
void lmac_ps_set_security(uint8_t security);
void lmac_ps_set_akm(uint8_t akm);
void lmac_ps_set_pmk(uint8_t *pmk, uint32_t pmk_len);
void lmac_ps_set_ip_addr(uint32_t ipaddr, uint32_t netmask, uint32_t gwaddr);
void lmac_ps_set_ucode_wake_src(uint8_t source);
void lmac_ps_set_init_retention();
void lmac_ps_set_rf_reg_dump(uint32_t *dump_val);
void lmac_ps_set_lbt_cs(uint16_t cs_time);
void lmac_ps_set_lbt_pause(uint32_t pause_time);
void lmac_ps_set_lbt_resume(uint32_t resume_time);
void lmac_ps_set_listen_interval(uint8_t vif_id,  uint32_t listen_interval);
void lmac_ps_set_long_beacon_interval(uint16_t bi);
void lmac_ps_set_short_beacon_interval(uint32_t sbi);
void lmac_ps_set_dtim_period(uint8_t period);
void lmac_ps_set_1m_ctrl_resp_preamble(uint8_t support);
void lmac_ps_set_ps_mode(uint8_t mode, uint64_t duration);
void lmac_ps_set_qos_null_pm1_ack_flag(int ack_flag);
int lmac_ps_get_qos_null_pm1_ack_flag();
void lmac_ps_set_usr_timer(uint8_t vif_id, uint32_t timeout_ms);
void lmac_ps_set_target_wake(uint8_t wake);
void lmac_ps_set_statype(uint8_t sta_type);
void lmac_ps_set_aid(uint16_t aid);
void lmac_ps_set_channel(uint16_t freq);
void lmac_ps_set_rc_mode (uint8_t mode);
void lmac_ps_set_rc_default_mcs (uint8_t mcs);
#if defined(INCLUDE_LEGACY_ACK)
void lmac_ps_set_legacy_ack(uint8_t enable);
#endif /* INCLUDE_LEGACY_ACK */
#if defined(INCLUDE_BEACON_BYPASS)
void lmac_ps_set_beacon_bypass(uint8_t enable);
#endif /* INCLUDE_BEACON_BYPASS */
#if defined (INCLUDE_AVOID_FRAG_ATTACK_TEST)
void lmac_ps_set_prev_ptk(uint8_t *ptk, uint32_t ptk_len);
#endif /* INCLUDE_AVOID_FRAG_ATTACK_TEST */
#if defined(INCLUDE_DUTYCYCLE)
void lmac_ps_set_duty_info(uint32_t window, uint32_t token, uint16_t margin, uint16_t beacon_margin);
#endif /* defined(INCLUDE_DUTYCYCLE) */
#if defined(INCLUDE_TCP_KEEPALIVE)
void lmac_ps_set_keepalive_data (uint32_t seqnum, uint32_t acknum, uint8_t *src_ip, uint16_t src_port,
									uint16_t winsize, uint32_t last_tsf);
void lmac_ps_set_keepalive_config (uint8_t *dest_ip, uint16_t dest_port, uint32_t period_sec);
#endif
void lmac_ps_set_short_bcn_interval(uint16_t short_bcn_interval);
void lmac_ps_set_bcn_interval(uint16_t bcn_interval);
void lmac_ps_set_1m_prim_loc(uint8_t prim_loc);
void lmac_ps_set_rc_status(uint8_t maxtp, uint8_t tp2, uint8_t maxp, uint8_t lowest, uint8_t probe);
void lmac_ps_set_support_ch_width(uint8_t width);

/* Dynamic PS API */
void lmac_dyn_ps_init();
uint32_t lmac_dyn_ps_get_timeout(uint8_t vif_id);
int lmac_dyn_ps_set_timeout(uint8_t vif_id, uint32_t dps_timeout_ms);
void lmac_dyn_ps_check_timeout(uint8_t vif_id);
void lmac_dyn_ps_set_last_rx_tsf();
uint32_t lmac_dyn_ps_get_last_rx_tsf();
uint32_t lmac_dyn_ps_get_last_rx_diff();
void lmac_dyn_ps_set_last_bcmc_rx_tsf();
uint32_t lmac_dyn_ps_get_last_bcmc_rx_tsf();
uint32_t lmac_dyn_ps_get_last_bcmc_rx_diff();
void lmac_dyn_ps_set_last_tx_tsf();
uint32_t lmac_dyn_ps_get_last_tx_tsf();
uint32_t lmac_dyn_ps_get_last_tx_diff();

extern lmac_ps_info g_lmac_ps_info;

#endif // __HAL_LMAC_PS_COMMON_H__
