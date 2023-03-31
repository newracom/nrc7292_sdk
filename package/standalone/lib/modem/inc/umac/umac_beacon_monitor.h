#ifndef UMAC_BEACON_MONITOR_H
#define UMAC_BEACON_MONITOR_H
#include <stdbool.h>

#if defined(UMAC_BEACON_MONITOR)
bool umac_beacon_monitor_init();
void umac_beacon_monitor_enable(int vif_id, bool enable);
bool umac_beacon_monitor_start(int vif_id);
void umac_beacon_monitor_stop(int vif_id);
void umac_beacon_monitor_update(int vif_id);
void umac_beacon_monitor_update_threshold(int vif_id);
void umac_beacon_monitor_set_threshold(int vif_id, uint32_t count);
uint32_t umac_beacon_monitor_get_threshold(int vif_id);
#else
static inline bool umac_beacon_monitor_init(int vif_id) {return false;};
static inline void umac_beacon_monitor_enable(int vif_id, bool enable) {};
static inline bool umac_beacon_monitor_start(int vif_id) {return true;};
static inline void umac_beacon_monitor_stop(int vif_id) {};
static inline void umac_beacon_monitor_update(int vif_id) {};
static inline void umac_beacon_monitor_update_threshold(int vif_id) {};
static inline void umac_beacon_monitor_set_threshold(int vif_id, uint32_t count) {};
static inline uint32_t umac_beacon_monitor_get_threshold(int vif_id) {return 0;};
#endif //UMAC_BEACON_MONITOR

#endif //UMAC_BEACON_MONITOR_H
