#ifndef UMAC_BEACON_MONITOR_H
#define UMAC_BEACON_MONITOR_H
#include <stdbool.h>

#if defined(UMAC_BEACON_MONITOR)
bool umac_beacon_monitor_init();
bool umac_beacon_monitor_start(int vif_id);
void umac_beacon_monitor_stop(int vif_id);
void umac_beacon_monitor_update(int vif_id);
void umac_beacon_monitor_update_threshold(int vif_id);
#else
static inline bool umac_beacon_monitor_init(int vif_id) {return false;};
static inline bool umac_beacon_monitor_start(int vif_id) {return true;};
static inline void umac_beacon_monitor_stop(int vif_id) {};
static inline void umac_beacon_monitor_update(int vif_id) {};
static inline void umac_beacon_monitor_update_threshold(int vif_id) {};
#endif //UMAC_BEACON_MONITOR

#endif //UMAC_BEACON_MONITOR_H
