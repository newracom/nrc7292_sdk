#ifndef LMAC_DUTY_CYCLE_H
#define LMAC_DUTY_CYCLE_H

#if defined(INCLUDE_DUTYCYCLE)
uint32_t duty_cycle_get_window();
bool duty_cycle_schedule(uint16_t sched_duration, int ac, bool is_mgmt);
bool duty_cycle_tick(int ac, uint16_t sched_duration, bool is_mgmt);
uint32_t duty_cycle_aifsn_offset();
void lmac_check_pause_time(uint8_t ac);
bool duty_cycle_filter();
uint32_t lmac_get_duty_window (void);
uint32_t lmac_get_tx_duration (void);
uint32_t lmac_get_duty_margin (void);
bool lmac_enable_duty_cycle(uint32_t window, uint32_t duration, uint32_t margin);
bool lmac_disable_duty_cycle(void);
#else
static inline uint32_t duty_cycle_get_window() {return 0;};
static inline bool duty_cycle_schedule(uint16_t s, int a, bool i) {return true;};
static inline bool duty_cycle_tick(int ac, uint16_t sched_duration, bool is_mgmt) {return true;};
static inline uint32_t duty_cycle_aifsn_offset() {return 0;};
static inline void lmac_check_pause_time(uint8_t ac) {};
static inline bool duty_cycle_filter() {return false;};
static inline uint32_t lmac_get_duty_window (void) {return 0;};
static inline uint32_t lmac_get_tx_duration (void) {return 0;};
static inline uint32_t lmac_get_duty_margin (void) {return 0;};
static inline bool lmac_enable_duty_cycle(uint32_t window, uint32_t duration, uint32_t margin) {return false;};
static inline bool lmac_disable_duty_cycle(void) {return false;};
#endif /* defined(INCLUDE_DUTYCYCLE) */
#endif /* LMAC_DUTY_CYCLE_H */
