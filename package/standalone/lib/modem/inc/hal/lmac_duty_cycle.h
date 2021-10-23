#ifndef LMAC_DUTY_CYCLE_H
#define LMAC_DUTY_CYCLE_H

#if defined(INCLUDE_DUTYCYCLE)
uint32_t duty_cycle_get_window();
bool duty_cycle_schedule(uint16_t sched_duration, int ac, bool is_mgmt);
bool duty_cycle_tick(int ac, uint16_t sched_duration, bool is_mgmt);
uint32_t duty_cycle_aifsn_offset();
void lmac_check_pause_time(uint8_t ac);
bool duty_cycle_filter();
#else
static inline uint32_t duty_cycle_get_window() {return 0;};
static inline bool duty_cycle_schedule(uint16_t s, int a, bool i) {return true;};
static inline bool duty_cycle_tick(int ac, uint16_t sched_duration, bool is_mgmt) {return true;};
static inline uint32_t duty_cycle_aifsn_offset() {return 0;};
static inline void lmac_check_pause_time(uint8_t ac) {};
static inline bool duty_cycle_filter() {return false;};
#endif /* defined(INCLUDE_DUTYCYCLE) */
#endif /* LMAC_DUTY_CYCLE_H */
