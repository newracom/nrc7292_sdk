#ifndef LMAC_DRV_H
#define LMAC_DRV_H

void drv_tx_init();
bool drv_lmac_reset();
void drv_lmac_set_ccm_monitor_interval(int interval);
void drv_lmac_set_base_address(uint32_t address);
void drv_lmac_set_alarm(uint32_t tsf);
void drv_lmac_set_basic_rate(int vif_id, uint32_t value);
uint32_t drv_lmac_get_tsf_lo(int vif_id);
uint32_t drv_lmac_get_tsf_hi(int vif_id);
void drv_set_mode(int vif_id, uint8_t mode);
void drv_mode_bssid(int vif_id, bool ap_mode_en);
void drv_set_pause(uint32_t mode);
#endif /* LMAC_DRV_H */
