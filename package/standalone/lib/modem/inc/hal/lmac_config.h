#ifndef LMAC_CONFIG_H
#define LMAC_CONFIG_H

#include "system.h"
#include "protocol.h"
#include "lmac_drv.h"
#include "lmac_type.h"

void set_winac_bitmap(uint32_t v);
uint32_t get_winac_bitmap(void);
void set_txdone_bitmap(uint32_t v);
uint32_t get_txdone_bitmap(void);
void set_txdone_count(uint32_t v);

void set_rx_sn(int vif_id, uint16_t v);
uint16_t get_rx_sn(int vif_id);
void set_tx_sn(int vif_id, uint16_t v);
uint16_t get_tx_sn(int vif_id);
void set_origin_addr(int vif_id, uint8_t *v);
uint8_t get_n_sts(int vif_id);
uint8_t get_coding(int vif_id);
bool get_stbc(int vif_id);
void set_uplink_ind(int vif_id, bool v);
bool get_uplink_ind(int vif_id);
uint8_t get_smoothing(int vif_id);
void set_scrambler(int vif_id, uint8_t v);
uint8_t get_scrambler(int vif_id);
 
void set_sniffer_mode(bool v);
bool get_sniffer_mode();
void set_sniffer_mode_beacon_display(bool v);
bool get_sniffer_mode_beacon_display();

uint8_t* get_gtk_addr(uint8_t vif_id, uint8_t key_index);
void set_gtk_addr(uint8_t vif_id, uint8_t key_index,  uint8_t *addr);
uint8_t* get_macaddr(uint8_t vif_id);
void set_macaddr(uint8_t vif_id, uint8_t* v);
uint8_t* get_destaddr(uint8_t vif_id);
void set_destaddr(uint8_t vif_id, uint8_t* v);
uint8_t* get_bssid(uint8_t vif_id);
void set_bssid(uint8_t vif_id, uint8_t* v);
uint8_t* get_country(void);
void set_country(uint8_t* v);
bool get_rate_ctrl_en(uint8_t vif_id);
void set_rate_ctrl_en(uint8_t vif_id, bool v);
uint8_t get_rate_ctrl_mode (uint8_t vif_id);
void set_rate_ctrl_mode(uint8_t vif_id, uint8_t mode);
uint8_t get_prim_ch_bw(void);
void set_prim_ch_bw(uint8_t v);
uint8_t get_prim_ch_loc(void);
void set_prim_ch_loc(uint8_t v);
uint16_t get_frequency(void);
void set_frequency(uint16_t v);
uint16_t get_mac80211_frequency(void);
void set_mac80211_frequency(uint16_t v);
uint8_t get_device_mode(uint8_t vif_id);
void set_device_mode(uint8_t vif_id, uint8_t v);
void set_aid(uint8_t vif_id, uint16_t aid);
uint16_t get_aid(uint8_t vif_id);
bool get_cipher_en(uint8_t vif_id);
void set_cipher_en(uint8_t vif_id, bool v);
uint8_t get_akm(uint8_t vif_id);
void set_akm(uint8_t vif_id, uint8_t v);
uint8_t get_key_index(uint8_t vif_id);
void set_key_index(uint8_t vif_id, uint8_t v);
enum CipherType get_cipher_type(uint8_t vif_id, enum key_type k);
void set_cipher_type(uint8_t vif_id, enum key_type k, enum CipherType v);
uint8_t get_cipher_disable_hwmic(uint8_t vif_id);
void set_cipher_disable_hwmic(uint8_t vif_id, uint8_t v);
uint8_t get_promiscuous_mode(uint8_t vif_id);
void set_promiscuous_mode(uint8_t vif_id, uint8_t v);
uint8_t get_ch_bw(void);
void set_ch_bw(uint8_t v);
uint32_t get_basic_rate(uint8_t vif_id);
void set_basic_rate(uint8_t vif_id, uint32_t v);
bool get_scanning(int vif_id);
void set_scanning(uint8_t vif_id, bool v);
bool get_drop_addr_match(uint8_t vif_id, uint8_t *v);
bool set_drop_addr(uint8_t vif_id, uint8_t *v, bool on);
uint8_t get_drop_cnt(uint8_t vif_id);
uint8_t* get_mpp_addr(void);
void set_mpp_addr(uint8_t* v);
uint8_t* get_nexthop_addr(void);
void set_nexthop_addr(uint8_t* v);
void set_max_agg_sched_num(uint8_t max_num);
uint8_t get_max_agg_sched_num();
uint8_t get_preamble_type(int vif_id);
void set_preamble_type(int vif_id, uint8_t v);
bool get_bypass_mgmt(uint8_t vif_id);
void set_bypass_mgmt(uint8_t vif_id, bool v);
bool get_bypass_beacon(uint8_t vif_id);
void set_bypass_beacon(uint8_t vif_id, bool v);
uint8_t get_phy_txgain(void);
void set_phy_txgain(uint8_t v);
uint8_t get_phy_rxgain(void);
void set_phy_rxgain(uint8_t v);
bool get_pv1_en(uint8_t vif_id);
void set_pv1_en(uint8_t vif_id, bool v);
uint64_t get_pn(uint8_t vif_id);
void set_pn(uint8_t vif_id, uint64_t v);
void set_1m_center_lo(uint8_t cc_id, bool v);
bool get_1m_center_lo(uint8_t cc_id);
uint8_t get_format(uint8_t vif_id);
void set_format(uint8_t vif_id, uint8_t v);
uint8_t get_mcs(void);
void set_mcs(uint8_t v);
void set_short_gi(uint8_t vif_id, uint8_t v);
uint8_t get_short_gi(uint8_t vif_id);
uint16_t get_rts_threshold(uint8_t vif_id);
void set_rts_threshold(uint8_t vif_id, uint16_t v);
uint8_t get_retry_limit(uint8_t vif_id);
void set_retry_limit(uint8_t vif_id, uint8_t v);
#if defined(INCLUDE_MANAGE_BLACKLIST)
uint8_t get_retry_block_limit(uint8_t vif_id);
void set_retry_block_limit(uint8_t vif_id, uint8_t v);
#endif
void set_rts(uint8_t vif_id, uint8_t v);
uint8_t get_rts(uint8_t vif_id);
bool get_common_cfo_cal_en();
void set_common_cfo_cal_en(bool v);
bool get_common_cfo_apply_en();
void set_common_cfo_apply_en(bool v);
double get_common_cfo_avg();
void set_common_cfo_avg(double v);
void set_mgmt_mcs10_permit(bool enable);
bool get_mgmt_mcs10_permit();
#if defined(INCLUDE_BD_SUPPORT)
void set_txpower_per_mcs(int index, uint8_t power);
uint8_t get_txpower_per_mcs(int index);
#if defined(NRC7394)
void set_lpf_gain_per_mcs(int index, uint16_t lpf_gain);
uint16_t get_lpf_gain_per_mcs(int index);
#endif
#else
void set_txpower_per_mcs(uint8_t bw, uint8_t freq_range, uint8_t mcs_range, uint8_t tx_gain);
uint8_t get_txpower_per_mcs(uint8_t bw, uint8_t freq_range, uint8_t mcs_range);
#endif /* defined(INCLUDE_BD_SUPPORT) */
void set_ndp_blockack(bool v);
bool get_ndp_blockack();
void set_ignore_sec(bool v);
bool get_ignore_sec();
void set_doppler(bool v);
bool get_doppler();
void set_txgain_per_mcs(int8_t mcs , uint8_t txgain);
uint8_t get_txgain_per_mcs(int8_t mcs);
void set_auto_gi_flag(bool v);
bool get_auto_gi_flag(void);
#if defined(INCLUDE_DYNAMIC_FRAG)
void set_dynamic_frag(bool v);
bool get_dynamic_frag(void);
#endif
#if defined(INCLUDE_DEFRAG)
void set_dynamic_defrag(bool v);
bool get_dynamic_defrag(void);
#endif
#if defined(INCLUDE_AUTH_CONTROL)
void set_enable_auth_ctrl(bool v);
bool get_enable_auth_ctrl(void);
#endif /* defined(INCLUDE_AUTH_CONTROL) */

void set_group_mcs(int8_t mcs);
int8_t get_group_mcs();

#endif /* LMAC_CONFIG_H */
