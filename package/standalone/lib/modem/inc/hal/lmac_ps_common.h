#ifndef __HAL_LMAC_PS_COMMON_H__
#define __HAL_LMAC_PS_COMMON_H__

#include "nrc_ps_api.h"

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

typedef void (*ps_doze_cb)(void);
typedef void (*ps_wake_cb)(void);

#define MAX_CB 5

typedef struct _lmac_ps_info {
	ps_mode     	mode;
	ps_sleep_mode	sleep_mode;

	ps_state	    state;
	ps_state	    tx_state;
	ps_state	    beacon_tim;

	ps_doze_cb		doze_cb[ MAX_CB ];
	uint8_t			doze_cb_num;

	ps_wake_cb		wake_cb[ MAX_CB ];
	uint8_t 	    wake_cb_num;

	uint32_t	    wait_tx_before_deepsleep_us;
	uint32_t	    wait_rx_before_deepsleep_us;
	uint32_t	    wakeup_after_deepsleep_us;
	uint32_t	    last_tx_us;
	uint32_t	    last_rx_us;
	uint32_t	    deepsleep_start_us;
	uint32_t		beacon_rx_us;
	uint32_t		beacon_interval_us;
	uint32_t		ready_for_wakeup_us;
	bool    	    non_tim;
	bool    	    keep_alive;
	int8_t			vif_id;

} lmac_ps_info;

void lmac_process_ps();

bool hal_lmac_ps_doze();
bool hal_lmac_ps_wake();


void lmac_ps_init(uint32_t wait_tx , uint32_t wait_rx , uint32_t wakeup_after_deepsleep_us );

bool lmac_ps_add_doze_cb( ps_doze_cb );
bool lmac_ps_add_wake_cb( ps_wake_cb );
void lmac_ps_set_listen_interval( uint32_t listen_interval_us );
void lmac_ps_set_beacon_interval(uint32_t short_beacon_interval_us);

void lmac_ps_set_channel(uint16_t freq);
void lmac_ps_set_aid(uint16_t aid);
void lmac_ps_set_bssid(uint8_t *bssid);

// ps_xxx common function
void lmac_ps_active();
void lmac_ps_doze();
void lmac_ps_active_to_doze();
void lmac_ps_doze_to_active();
void lmac_ps_execute_sleepmode(uint32_t after_ms);

// ps api function
void lmac_ps_set_sleep(uint8_t mode, uint16_t interval_ms);
void lmac_ps_register_callback();
void lmac_ps_signal_wakeup(bool enable); // For modem sleep only


ps_mode lmac_ps_get_psmode();

extern lmac_ps_info g_lmac_ps_info;



#endif // __HAL_LMAC_PS_COMMON_H__
