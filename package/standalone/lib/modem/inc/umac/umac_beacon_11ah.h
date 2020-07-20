#ifndef UMAC_BEACON_11AH_H
#define UMAC_BEACON_11AH_H

void umac_beacon_init(int8_t vif_id);
void umac_beacon_deinit();
void umac_beacon_start(int8_t vif_id);
void umac_beacon_stop();
void umac_beacon_update(int8_t vif_id, uint8_t* b, uint16_t len);
void umac_beacon_tbtt_cb(int8_t vif_id);
void umac_beacon_update_tim(uint16_t aid, bool val);
void mac_beacon_update_tim_flag(void);
void umac_beacon_update_dtim_period(int8_t vif_id, uint8_t period );
void umac_beacon_set_ssid(int8_t vif_id, uint8_t *ssid , uint8_t ssid_len);
void umac_beacon_set_beacon_interval(int8_t vif_id, uint16_t interval);
void umac_beacon_set_short_beacon_interval(int8_t vif_id, uint16_t interval);
void umac_beacon_set_bss_bw(uint8_t ch_bw, uint8_t prim_ch_bw);
void umac_beacon_update_tim_flag();

#endif /* UMAC_BEACON_11AH_H */
