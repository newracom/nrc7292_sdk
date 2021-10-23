#ifndef __LMAC_TWT_COMMON_H__
#define __LMAC_TWT_COMMON_H__

typedef enum _twt_mode {
	TWT_MODE_NONE = 0,
	TWT_MODE_NULL,
	TWT_MODE_PSPOLL,
	TWT_MODE_MAX,
} twt_mode;

typedef enum _twt_state {
	TWT_STATE_NULL = 0,
	TWT_STATE_REQUEST,
	TWT_STATE_ACCEPT,
	TWT_STATE_REJECT,
	TWT_STATE_SERVICE,
	TWT_STATE_QUIET,
	TWT_STATE_SERVICE_TO_QUIET,
	TWT_STATE_QUIET_TO_SERVICE,
	TWT_STATE_MAX,
} twt_state;

typedef void (*twt_quiet_cb)(void);
typedef void (*twt_service_cb)(void);

#define MAX_TWT_CB 1

typedef struct _lmac_twt_info {
	bool		enable;
	bool		ps_poll;
	twt_mode     mode;
	twt_state    state;
	twt_state	null_state;
	twt_state	beacon_tim;
	twt_state	state_pm;

	twt_quiet_cb  quiet_cb[MAX_TWT_CB];
	uint8_t     quiet_cb_num;

	twt_service_cb  service_cb[MAX_TWT_CB];
	uint8_t     service_cb_num;

	uint32_t	service_period;
	uint32_t	wake_interval;
	uint32_t	start_time;
	uint32_t	end_time;

	uint32_t	init_start_margin;
	uint32_t	start_margin;
	uint32_t	end_margin;
	uint32_t	init_start_time;
	uint8_t		request_type;
	int8_t		vif_id;
} lmac_twt_info;

void lmac_process_twt(void);

bool lmac_twt_quiet_cb(void);
bool lmac_twt_service_cb(void);

void lmac_twt_init(void);
bool lmac_twt_add_quiet_cb( twt_quiet_cb );
bool lmac_twt_add_service_cb( twt_service_cb );

void lmac_twt_service(void);
void lmac_twt_service_to_quiet(uint32_t now);
void lmac_twt_service_to_quiet_ap(void);
void lmac_twt_quiet_to_service_ap(void);
void lmac_twt_quiet(void);
void lmac_twt_quiet_to_service(void);
bool lmac_get_twt_enable();
void lmac_set_twt_enable(bool flag);
twt_mode lmac_get_twt_mode(void);
twt_state lmac_get_twt_state(void);
void lmac_set_twt_mode(twt_mode mode);
void lmac_set_twt_state(twt_state state);
void lmac_set_twt_null_state(twt_state state);
twt_state lmac_get_twt_null_state();
void lmac_set_twt_service_period(uint32_t service_period_us);
void lmac_set_twt_wake_interval(uint32_t wake_interval_us);
uint32_t lmac_get_twt_service_period(void);
uint32_t lmac_get_twt_wake_interval(void);
void lmac_set_twt_start_end_time(uint32_t wake_interval_us, uint32_t service_period_us);
uint32_t lmac_get_twt_start_time();
void lmac_set_twt_start_time(uint32_t time);
uint32_t lmac_get_twt_end_time();
void lmac_set_twt_end_time(uint32_t time);
void lmac_set_twt_request_type(uint8_t req_type);
uint8_t lmac_get_twt_request_type();
uint32_t lmac_get_twt_init_start_margin();
void lmac_set_twt_init_start_margin(uint32_t time);
uint32_t lmac_get_twt_start_margin();
void lmac_set_twt_start_margin(uint32_t time);
uint32_t lmac_get_twt_end_margin();
void lmac_set_twt_end_margin(uint32_t time);
twt_state lmac_get_twt_beacon_tim();
void lmac_set_twt_beacon_tim(twt_state state);
twt_state lmac_get_twt_state_pm();
void lmac_set_twt_state_pm(twt_state state);
uint32_t lmac_get_twt_init_start_time();
void lmac_set_twt_init_start_time(uint32_t time);

extern lmac_twt_info g_lmac_twt_info;
#endif // __LMAC_TWT_COMMON_H__
