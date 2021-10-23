#ifndef LMAC_CONFIG_H
#define LMAC_CONFIG_H

#include "system.h"
#include "protocol.h"
#include "lmac_drv.h"
#include "lmac_type.h"

#define MAX_DROP_ADDR		(10)

struct sn_manager {
	uint16_t	tx_sn			: 12;
	uint16_t	rx_sn			: 12;
	uint8_t		origin_addr[MAC_ADDR_LEN];
};

struct irq_data {
	uint32_t	winac_bitmap;
	uint32_t	txdone_bitmap;
	uint32_t	txdone_count;
};

struct tx_data {
	uint8_t			scrambler;
	uint8_t			n_sts;
	uint8_t			coding;
	uint8_t			smoothing;
	uint8_t			preamble_type;
	bool			stbc;
	bool			uplink_ind;
	uint8_t			format;
	uint8_t			rts;
	uint16_t		rts_threshold;
	uint8_t			retry_limit;
	uint8_t			short_gi;
};

struct irq_data * get_irq_data();
void set_irq_data(struct irq_data *src);

struct tx_data * get_tx_data(int vif_id);
void set_tx_data(int vif_id, struct tx_data *src);

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
uint8_t* get_country(uint8_t vif_id);
void set_country(uint8_t vif_id, uint8_t* v);
bool get_rate_ctrl_en(uint8_t vif_id);
void set_rate_ctrl_en(uint8_t vif_id, bool v);
uint8_t get_prim_ch_bw(uint8_t vif_id);
void set_prim_ch_bw(uint8_t vif_id, uint8_t v);
uint8_t get_prim_ch_loc(uint8_t vif_id);
void set_prim_ch_loc(uint8_t vif_id, uint8_t v);
uint32_t get_frequency(uint8_t vif_id);
void set_frequency(uint8_t vif_id, uint32_t v);
uint32_t get_mac80211_frequency(uint8_t vif_id);
void set_mac80211_frequency(uint8_t vif_id, uint32_t v);
uint8_t get_device_mode(uint8_t vif_id);
void set_device_mode(uint8_t vif_id, uint8_t v);
void set_aid(uint8_t vif_id, uint16_t aid);
uint16_t get_aid(uint8_t vif_id);
bool get_cipher_en(uint8_t vif_id);
void set_cipher_en(uint8_t vif_id, bool v);
uint8_t get_akm(uint8_t vif_id);
void set_akm(uint8_t vif_id, uint8_t  v);
uint8_t get_key_index(uint8_t vif_id);
void set_key_index(uint8_t vif_id, uint8_t v);
enum CipherType get_cipher_type(uint8_t vif_id, enum key_type k);
void set_cipher_type(uint8_t vif_id, enum key_type k, enum CipherType v);
uint8_t get_cipher_disable_hwmic(uint8_t vif_id);
void set_cipher_disable_hwmic(uint8_t vif_id, uint8_t v);
uint8_t get_promiscuous_mode(uint8_t vif_id);
void set_promiscuous_mode(uint8_t vif_id, uint8_t v);
uint8_t get_ch_bw(uint8_t vif_id);
void set_ch_bw(uint8_t vif_id, uint8_t v);
uint32_t get_basic_rate(uint8_t vif_id);
void set_basic_rate(uint8_t vif_id, uint32_t v);
bool get_scanning(uint8_t vif_id);
void set_scanning(uint8_t vif_id, bool v);
bool get_drop_addr_match(uint8_t vif_id, uint8_t* v);
bool set_drop_addr(uint8_t vif_id, uint8_t* v, bool on);
uint8_t get_drop_cnt(uint8_t vif_id);
void set_max_agg_sched_num(uint8_t max_num);
uint8_t get_max_agg_sched_num();

#if defined(STANDARD_11N)
uint8_t get_erp_protection(uint8_t vif_id);
void set_erp_protection(uint8_t vif_id, uint8_t v);
uint8_t get_short_slot(uint8_t vif_id);
void set_short_slot(uint8_t vif_id, uint8_t v);
uint8_t get_short_preamble(uint8_t vif_id);
void set_short_preamble(uint8_t vif_id, uint8_t v);
uint16_t get_ht_info(uint8_t vif_id);
void set_ht_info(uint8_t vif_id, uint16_t v);
uint16_t get_ht_cap(uint8_t vif_id);
void set_ht_cap(uint8_t vif_id, uint16_t v);
#endif /* defined(STANDARD_11N) */
#if defined(STANDARD_11AH)
uint8_t get_preamble_type(uint8_t vif_id);
void set_preamble_type(uint8_t vif_id, uint8_t v);
bool get_bypass_mgmt(uint8_t vif_id);
void set_bypass_mgmt(uint8_t vif_id, bool v);
uint8_t get_phy_txgain(uint8_t vif_id);
void set_phy_txgain(uint8_t vif_id, uint8_t v);
uint8_t get_phy_rxgain(uint8_t vif_id);
void set_phy_rxgain(uint8_t vif_id, uint8_t v);
bool get_pv1_en(uint8_t vif_id);
void set_pv1_en(uint8_t vif_id, bool v);
uint64_t get_pn(uint8_t vif_id);
void set_pn(uint8_t vif_id, uint64_t v);
#if defined(INCLUDE_FRAG_FRAME)
uint8_t* get_tx_pn(uint8_t vif_id, uint8_t type);
void set_tx_pn(uint8_t vif_id, uint8_t type, uint64_t value);
void increase_tx_pn(uint8_t vif_id, uint8_t type);
#endif /* INCLUDE_FRAG_FRAME */
void set_1m_center_lo(uint8_t cc_id, bool v);
bool get_1m_center_lo(uint8_t cc_id);
#endif /* defined(STANDARD_11AH) */
uint8_t get_format(uint8_t vif_id);
void set_format(uint8_t vif_id, uint8_t v);
uint8_t get_mcs(uint8_t vif_id);
void set_mcs(uint8_t vif_id, uint8_t v);
void set_short_gi(uint8_t vif_id, uint8_t v);
uint8_t get_short_gi(uint8_t vif_id);
uint16_t get_rts_threshold(uint8_t vif_id);
void set_rts_threshold(uint8_t vif_id, uint16_t v);
uint8_t get_retry_limit(uint8_t vif_id);
void set_retry_limit(uint8_t vif_id, uint8_t v);
void set_rts(uint8_t vif_id, uint8_t v);
uint8_t get_rts(uint8_t vif_id);
struct sn_manager * get_sn_manager(uint8_t vif_id);
bool get_common_cfo_cal_en();
void set_common_cfo_cal_en(bool v);
bool get_common_cfo_apply_en();
void set_common_cfo_apply_en(bool v);
double get_common_cfo_avg();
void set_common_cfo_avg(double v);
uint8_t get_common_cfg_count();
void set_common_cfo_count(uint8_t v);
void set_mgmt_mcs10_permit(bool enable);
bool get_mgmt_mcs10_permit();
void set_cs_duration(uint16_t dur);
uint16_t get_cs_duration();
#if defined(INCLUDE_BD_SUPPORT)
void set_txpower_per_mcs(int index, uint8_t power);
uint8_t get_txpower_per_mcs(int index);
#else
void set_txpower_per_mcs(uint8_t bw, uint8_t freq_range, uint8_t mcs_range, uint8_t tx_gain);
uint8_t get_txpower_per_mcs(uint8_t bw, uint8_t freq_range, uint8_t mcs_range);
#endif /* defined(INCLUDE_BD_SUPPORT) */
void set_ndp_blockack(bool v);
bool get_ndp_blockack();
#if defined (INCLUDE_RF_KILL)
void set_rf_kill(bool v);
bool get_rf_kill();
#endif
void set_ignore_sec(bool v);
bool get_ignore_sec();
void set_doppler(bool v);
bool get_doppler();
void set_txgain_per_mcs(int8_t mcs , uint8_t txgain);
uint8_t get_txgain_per_mcs(int8_t mcs);
enum CipherType get_ptk_type(uint8_t vif_id);
enum CipherType get_gtk_type(uint8_t vif_id);

#endif /* LMAC_CONFIG_H */
