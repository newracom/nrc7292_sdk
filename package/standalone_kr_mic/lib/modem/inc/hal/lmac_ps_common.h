#ifndef __HAL_LMAC_PS_COMMON_H__
#define __HAL_LMAC_PS_COMMON_H__

#include "nrc_ps_api.h"

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

typedef void (*ps_doze_cb)(void);
typedef void (*ps_wake_cb)(void);

#define MAX_CB 5

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
	uint32_t	    wait_tx_before_deepsleep_us;
	uint32_t	    wait_rx_before_deepsleep_us;
	uint64_t	    wakeup_after_deepsleep_us;
	uint32_t	    last_tx_us;
	uint32_t	    last_rx_us;
	uint32_t	    last_bcmc_rx_us;
	uint32_t		minimum_sleep_interval_us;
	uint32_t	    deepsleep_start_us;
	uint32_t		beacon_rx_us;
	uint32_t		beacon_interval_us;
	uint32_t		ready_for_wakeup_us;
	bool    	    non_tim;
	bool    	    keep_alive;
	int8_t			vif_id;
	uint8_t 		dtim_count;
	uint32_t	    next_dtim_us;

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

#if defined(NRC7292_STANDALONE_XIP)
	bool 			ms_mode;
	uint32_t 		timeout_ms;	//standalone only
#endif /* NRC7292_STANDALONE_XIP */
} lmac_ps_info;

void lmac_process_ps();

bool hal_lmac_ps_doze();
bool hal_lmac_ps_wake();


void lmac_ps_init(uint32_t wait_tx , uint32_t wait_rx , uint64_t wakeup_after_deepsleep_us );

bool lmac_ps_add_doze_cb( ps_doze_cb );
bool lmac_ps_add_wake_cb( ps_wake_cb );

//Set PS params including retention info
void lmac_ps_set_listen_interval(uint8_t vif_id, uint32_t listen_interval_us );
void lmac_ps_set_short_beacon_interval(uint32_t sbi);
void lmac_ps_set_dtim_period(uint8_t period);
void lmac_ps_set_long_beacon_interval(uint16_t bi);
void lmac_ps_set_statype(uint8_t sta_type);
void lmac_ps_set_ps_mode(uint8_t mode, uint64_t duration);
void lmac_ps_set_target_wake(uint8_t wake);
void lmac_ps_set_channel(uint16_t freq);
void lmac_ps_set_aid(uint16_t aid);
void lmac_ps_set_addr(uint8_t *addr, bool ap);
void lmac_ps_set_country(uint8_t *cc);
void lmac_ps_set_ssid(uint8_t *ssid, uint8_t  len);
void lmac_ps_set_ndp_preq(uint8_t enable);
void lmac_ps_set_guard_interval(uint8_t gi);
void lmac_ps_set_rts_threshold(uint32_t rtstr);
void lmac_ps_set_key(uint8_t key_id, uint8_t key_type, uint32_t *key);
void lmac_ps_set_key_tsc(uint8_t key_id, uint64_t tsc);
void lmac_ps_set_edca(uint8_t ac, uint8_t aifsn, uint16_t cwmin, uint16_t cwmax,  uint16_t txop);
void lmac_ps_set_ampdu(uint8_t ac, uint8_t max_agg);
void lmac_ps_set_ampdu_enable(uint8_t ac, uint8_t enable);
void lmac_ps_set_tx_seqnum(uint8_t tid, uint16_t sn);
void lmac_ps_set_rx_seqnum(uint8_t tid, uint16_t sn, uint16_t bitmap);
void lmac_ps_set_ba_state(uint8_t tid, bool add, uint8_t ba_state);
void lmac_ps_set_txpwr(uint8_t tx_pwr);
void lmac_ps_set_rxgain(uint8_t rx_gain);
void lmac_ps_set_sgi(uint8_t sgi);
void lmac_ps_set_bss_maxidle(uint8_t option, uint16_t period);
void lmac_ps_set_security(uint8_t security);
void lmac_ps_set_pmk(uint8_t *pmk, uint32_t pmk_len);
void lmac_ps_set_ip_addr(uint32_t ipaddr, uint32_t netmask, uint32_t gwaddr);
void lmac_ps_set_ucode_wake_src(uint8_t source);
void lmac_ps_set_init_retention();
void lmac_ps_set_1m_ctrl_resp_preamble(uint8_t support);

// ps_xxx common function
void lmac_ps_active();
void lmac_ps_doze();
void lmac_ps_active_to_doze();
void lmac_ps_doze_to_active();
void lmac_ps_check_qm();
void lmac_ps_set_wakeup();
bool lmac_ps_is_wakeup();
void lmac_ps_execute_sleepmode(uint64_t after_ms);
uint32_t lmac_ps_get_last_sleep_time();
void lmac_ps_go_doze();

ps_mode lmac_ps_get_mode();
ps_state lmac_ps_get_state();
ps_state lmac_ps_get_tx_state();
ps_state lmac_ps_get_rx_state();
ps_state lmac_ps_get_tx_state_pm();
ps_state lmac_ps_get_beacon_tim();
uint32_t lmac_ps_get_beacon_interval();
void lmac_ps_set_bcmc_rx_state(ps_state tsf);
ps_state lmac_ps_get_bcmc_rx_state();
int8_t lmac_ps_get_vifid();
bool lmac_ps_get_ret_recovered();
bool lmac_ps_get_ret_validity();
bool lmac_ps_get_target_wake();
bool lmac_ps_is_modem_sleep_mode();
bool lmac_ps_set_halt_modem_sleep(int8_t vif_id, bool stop);

void lmac_ps_set_beacon_tim(ps_state state);
void lmac_ps_set_rx_state(ps_state state);
void lmac_ps_set_tx_state_pm(ps_state state);
void lmac_ps_set_last_rx_tsf(uint32_t tsf);
uint32_t lmac_ps_get_last_rx_tsf();
void lmac_ps_set_last_bcmc_rx_tsf(uint32_t tsf);
uint32_t lmac_ps_get_last_bcmc_rx_tsf();
void lmac_ps_set_last_tx_tsf(uint32_t tsf);
uint32_t lmac_ps_get_last_tx_tsf();
void lmac_ps_detect_qosdata_fail();

// ps api function
int lmac_ps_set_sleep(uint8_t mode, uint64_t interval_ms);
#if defined(NRC7292_STANDALONE_XIP) || defined(NRC7392_STANDALONE_XIP)
void lmac_ps_modemsleep_stop();
int lmac_ps_set_iw_power_timeout(uint32_t timeout_ms);
uint32_t lmac_ps_get_iw_power_timeout(void);
#endif
int lmac_ps_stop();
void lmac_ps_register_callback();
ps_mode lmac_ps_get_psmode();
void lmac_ps_go_sleep_alone(uint8_t mode, uint64_t duration);
void lmac_ps_set_sync_time_ms(uint32_t time);
uint32_t lmac_ps_get_sync_time_ms(void);

bool lmac_ps_can_be_doze();

extern lmac_ps_info g_lmac_ps_info;



#endif // __HAL_LMAC_PS_COMMON_H__
