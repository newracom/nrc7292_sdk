#ifndef UMAC_BEACON_11AH_H
#define UMAC_BEACON_11AH_H

#include "umac_ieee80211_types.h"


#if defined(INCLUDE_BEACON_AP_BEACON) || defined(CUNIT)
void umac_beacon_init(int8_t vif_id);
void umac_beacon_deinit(uint8_t vif_id);
void umac_beacon_start(int8_t vif_id);
void umac_beacon_stop(uint8_t vif_id);
void umac_beacon_update(int8_t vif_id, uint8_t* b, uint16_t len);
void umac_beacon_tbtt_cb(int8_t vif_id);
void umac_beacon_update_tim(uint8_t vif_id, uint16_t aid, bool val);
void mac_beacon_update_tim_flag(uint8_t vif_id);
void umac_beacon_update_dtim_period(int8_t vif_id, uint8_t period );
void umac_beacon_set_ssid(int8_t vif_id, uint8_t *ssid , uint8_t ssid_len);
void umac_beacon_set_beacon_interval(int8_t vif_id, uint16_t interval);
void umac_beacon_ap_set_short_beacon_interval(int8_t vif_id, uint16_t interval);
void umac_beacon_sta_set_short_beacon_interval(int8_t vif_id, uint16_t interval);
void umac_beacon_set_bss_bw(uint8_t vif_id, uint8_t ch_bw, uint8_t prim_ch_bw);
void umac_beacon_update_tim_flag(uint8_t vif_id);
bool umac_beacon_is_dtim(uint8_t dtim_count);
bool umac_beacon_is_hidden(uint8_t vif_id);
ie_csa* umac_beacon_get_csa_info(uint8_t vif_id);
#else
static inline void umac_beacon_init(int8_t vif_id) {}
static inline void umac_beacon_deinit(int8_t vif_id) {}
static inline void umac_beacon_start(int8_t vif_id) {}
static inline void umac_beacon_stop(int8_t vif_id) {}
static inline void umac_beacon_update(int8_t vif_id, uint8_t* b, uint16_t len) {}
static inline void umac_beacon_tbtt_cb(int8_t vif_id) {}
static inline void umac_beacon_update_tim(uint8_t vif_id, uint16_t aid, bool val) {}
static inline void mac_beacon_update_tim_flag(uint8_t vif_id) {}
static inline void umac_beacon_update_dtim_period(int8_t vif_id, uint8_t period ) {}
static inline void umac_beacon_set_ssid(int8_t vif_id, uint8_t *ssid , uint8_t ssid_len) {}
static inline void umac_beacon_set_beacon_interval(int8_t vif_id, uint16_t interval) {}
static inline void umac_beacon_ap_set_short_beacon_interval(int8_t vif_id, uint16_t interval) {}
static inline void umac_beacon_sta_set_short_beacon_interval(int8_t vif_id, uint16_t interval) {}
static inline void umac_beacon_set_bss_bw(uint8_t vif_id, uint8_t ch_bw, uint8_t prim_ch_bw) {}
static inline void umac_beacon_update_tim_flag(uint8_t vif_id) {}
static inline bool umac_beacon_is_dtim(uint8_t dtim_count) {return true;}
static inline bool umac_beacon_is_hidden(uint8_t vif_id) {return false;}
static inline ie_csa* umac_beacon_get_csa_info(uint8_t vif_id){return NULL;}
#endif //#if defined(INCLUDE_BEACON_AP_BEACON) || defined(CUNIT)

#endif /* UMAC_BEACON_11AH_H */
