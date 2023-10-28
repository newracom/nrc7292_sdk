#ifndef LMAC_DUTY_CYCLE_H
#define LMAC_DUTY_CYCLE_H

#if defined(INCLUDE_DUTYCYCLE)
bool lmac_duty_is_on(void);
void lmac_duty_cycle_init();
bool lmac_duty_cycle_schedule(uint16_t sched_duration, int ac, bool is_mgmt);
void lmac_check_duty_window();
void lmac_check_pause_time(uint8_t ac, bool mgmt);
bool lmac_enable_duty_cycle(uint32_t window, uint32_t duration, uint16_t margin, uint16_t beacon_margin);
bool lmac_disable_duty_cycle(bool init_ps);
bool lmac_tx_available_duty_cycle();
uint32_t lmac_duty_get_token(void);
uint32_t lmac_get_duty_window (void);
uint32_t lmac_get_tx_duration (void);
uint16_t lmac_get_duty_margin (void);
uint16_t lmac_get_duty_beacon_margin (void);
bool lmac_duty_is_on( void );
uint32_t lmac_duty_get_token( void );
#else
static inline void lmac_duty_cycle_init() {return;};
static inline bool lmac_duty_cycle_schedule(uint16_t s, int a, bool i) {return true;};
static inline void lmac_check_duty_window() {return;};
static inline void lmac_check_pause_time(uint8_t ac, bool mgmt) {};
static inline bool lmac_enable_duty_cycle(uint32_t window, uint32_t duration, uint16_t margin, uint16_t beacon_margin) {return false;};
static inline bool lmac_disable_duty_cycle(bool init_ps) {return false;};
static inline bool lmac_tx_available_duty_cycle(void) {return false;};
static inline uint32_t lmac_get_duty_window (void) {return 0;};
static inline uint32_t lmac_get_tx_duration (void) {return 0;};
static inline uint16_t lmac_get_duty_margin (void) {return 0;};
static inline uint16_t lmac_get_duty_beacon_margin (void) {return 0;};
#endif /* defined(INCLUDE_DUTYCYCLE) */
#endif /* LMAC_DUTY_CYCLE_H */
