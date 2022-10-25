#ifndef	__SYSTEM_MODEM_API_H__
#define __SYSTEM_MODEM_API_H__

#include <stdbool.h>
#include "nrc-wim-types.h"
#include "system_type.h"
#include "lmac_common.h"
#include "util_byte_stream.h"

#if defined(STANDARD_11AH)
#if defined(NRC_ROMLIB)
#include "hal_sflash_lib.h"
#else
#include "hal_sflash.h"
#endif /* defined(NRC_ROMLIB) */
#endif /* defined(STANDARD_11AH) */

struct edca_param {
	uint8_t aifsn;
	uint8_t acm;
	uint8_t aci;
	uint16_t cw_min;
	uint16_t cw_max;
	uint16_t txop_limit;
} __attribute__((packed));

struct cipher_def {
	//enum CipherType type;
	int				type;
	enum key_type	key_type;
	uint8_t 		key_id;
	uint32_t		key[MAX_KEY_ID][MAX_KEY_LEN];
	uint32_t		mic_key[MAX_KEY_LEN];
	uint8_t 		addr[6];		//< MAC Address
	uint16_t		aid;
	uint8_t 		disable_hwmic;
};
struct channel_def {
	uint16_t freq;
	uint8_t ch_index;
	uint8_t cca_type;
	enum ch_width band;
	int offset;
	int primary_loc;
	bool _1m_dup;
	bool en_dup_1mhz;
	bool en_1mhz_c_resp;
};

enum efuse_lot_ver {
	lot_TT = 0, /* "000" */
	lot_SS = 3, /* "011" */
	lot_FF = 4, /* "100" */
	lot_NA = 1 /* N/A */
};

uint32_t system_modem_api_get_dl_hif_length(struct _SYS_BUF *packet);
uint32_t system_modem_api_get_tx_space();
uint32_t system_modem_api_get_rx_space();
void 	 system_modem_api_get_capabilities(struct wim_cap_param* param);
void     system_modem_api_set_mac_address(int vif_id, uint8_t *mac_addr);
uint8_t* system_modem_api_get_mac_address(int vif_id);
void 	 system_modem_api_enable_mac_address(int vif_id, bool enable);
void     system_modem_api_set_ssid(int vif_id , uint8_t *ssid , uint8_t ssid_len);
void     system_modem_api_enable_beacon(int vif_id, bool enable);
void     system_modem_api_update_beacon(int vif_id, uint8_t* ,uint16_t );
void     system_modem_api_set_beacon_interval(int vif_id, uint16_t beacon_interval);
void     system_modem_api_ap_set_short_beacon_interval(int vif_id, uint16_t short_beacon_interval);
void     system_modem_api_sta_set_short_beacon_interval(int vif_id, uint16_t short_beacon_interval);
void     system_modem_api_set_tim_flag(uint8_t vif_id);
void 	 system_modem_api_set_bssid(int vif_id, uint8_t *bssid);
uint8_t *system_modem_api_get_bssid(int vif_id);
void 	 system_modem_api_enable_bssid(int vif_id, bool enable);
void     system_mode_api_mode_bssid(int vif_id, bool ap_mode_en);
void     system_modem_api_set_edca_param(struct edca_param* edca);
bool     system_modem_api_set_aid(int vif_id, uint16_t aid);
int      system_modem_api_get_aid(int vif_id);
void 	 system_modem_api_set_mode(int vif_id, uint8_t mode);
bool     system_modem_api_is_ap(int vif_id);
bool     system_modem_api_is_sta(int vif_id);
bool     system_modem_api_is_mesh(int vif_id);
bool     system_modem_api_is_mesh_ap(void);
void     system_modem_api_set_cca_ignore(bool ignore);
void     system_modem_api_set_cipher_ignore(bool ignore, uint8_t dir);
void     system_modem_api_set_promiscuous_mode(int vif_id, bool enable);
bool     system_modem_api_set_erp_param(int vif_id, struct wim_erp_param* p);
int      system_modem_api_get_cipher_icv_length(int type);
bool     system_modem_api_sec_set_enable_key(int vif_id, bool enable);
bool     system_modem_api_sec_get_enable_key   (int vif_id);
bool     system_modem_api_add_key(int vif_id, struct cipher_def *lmc, bool dummy);
bool     system_modem_api_del_key(int vif_id, struct cipher_def *lmc);
void     system_modem_api_del_key_all(int vif_id);
void system_modem_api_set_basic_rate(int vif_id, uint32_t basic_rate_set);
void     system_modem_api_set_tsf_timer(int vif_id, uint32_t ts_hi, uint32_t ts_lo);
void     system_modem_api_set_response_ind(int ac, uint8_t response_ind);
uint8_t  system_modem_api_get_response_ind(int ac);
void     system_modem_api_set_aggregation(int ac, bool aggregation);
bool     system_modem_api_get_aggregation(int ac);
bool     system_modem_api_set_ht_operation_info(int vif_id, uint16_t ht_info);
bool 	 system_modem_api_set_ht_capability(int vif_id, uint16_t ht_cap);
bool 	 system_modem_api_set_channel(int vif_id, uint32_t ch_freq);
bool 	 system_modem_api_set_channel2(struct channel_def *def);
bool 	 system_modem_api_set_channel_width(int vif_id, uint8_t chan_width, uint8_t prim_loc);
void 	 system_modem_api_set_short_gi(int vif_id, uint8_t short_gi, bool gi_auto_flag);
void     system_modem_api_set_rate_control(int vif_id, bool enable);
bool     system_modem_api_get_rate_control(int vif_id);
void     system_api_get_supported_channels(uint16_t **chs, int *n_ch);
void     system_modem_api_set_txgain(uint32_t txgain);
void     system_modem_api_set_rxgain(uint32_t rxgain);
uint16_t system_modem_api_get_frequency(struct _SYS_BUF *packet);
int      system_modem_api_get_rssi(struct _SYS_BUF *packet);
uint16_t system_modem_api_get_current_channel_number();
uint8_t  system_modem_api_mac80211_frequency_to_channel(uint32_t frequency);
uint32_t system_modem_api_channel_to_mac80211_frequency(uint8_t channel);
void     system_modem_api_set_cs_time(uint16_t value);
uint32_t system_modem_api_get_cs_time(void);
void     system_modem_api_set_tx_pause_time(uint32_t value);
uint32_t system_modem_api_get_tx_pause_time(void);
bool     system_modem_api_set_tx_time(uint16_t cs_time, uint32_t pause_time);
bool     system_modem_api_get_duty_cycle(uint32_t *window, uint32_t *duration, uint32_t *margin);
bool     system_modem_api_enable_duty_cycle(uint32_t window, uint32_t duration, uint32_t margin);
bool     system_modem_api_disable_duty_cycle(void);
bool     system_modem_api_set_cca_threshold(int vif_id, int cca_threshold);
int      system_modem_api_get_cca_threshold(int vif_id);
void     system_modem_api_set_tx_suppress_dur(uint32_t value);
void     system_modem_api_set_tx_suppress_cmd(uint32_t value);
uint32_t system_api_get_version(void);
uint32_t system_api_get_align(void);
uint32_t system_api_get_buffer_length(void);

void system_api_set_promiscuous_mode(bool enable);
void system_api_set_bypass_beacon(bool enable);

bool system_modem_api_set_mcs(int vif_id, uint8_t mcs);
uint8_t system_modem_api_get_mcs(int vif_id);

#if defined(INCLUDE_BD_SUPPORT)
bool system_modem_api_set_bd_data(int vif_id, struct wim_bd_param* p);
void system_modem_api_set_tx_power(int vif_id, uint16_t ch_freq);
unsigned char* system_modem_api_get_bdf_data(unsigned int* size);
bool system_modem_api_is_bdf_use(void);
#endif /* defined(INCLUDE_BD_SUPPORT) */
#if defined(STANDARD_11AH)
enum efuse_lot_ver system_api_get_lot_ver(void);
void system_modem_api_set_channel_width_s1goper(int vif_id, uint8_t prim_ch_width, uint8_t prim_loc, uint8_t prim_ch_number);
void system_modem_api_set_country_code(int vif_id, uint8_t *country_code);
void system_api_get_rf_cal(uint32_t address, uint8_t *buffer, size_t size);
uint32_t system_api_get_flash_size(void);
void system_api_set_sys_config_cal_use(uint8_t cal_use);
void system_api_set_sys_config_country_code(uint8_t *country_code);
void system_api_set_rf_cal(uint32_t address, uint8_t *buffer, size_t size);
void system_api_clear_rf_cal(uint32_t address, size_t size);
void system_api_set_user_config(uint32_t address, uint8_t *buffer, size_t size);
bool system_modem_api_get_1m_center_lo(int vif_id);
int  system_api_set_freq_bw_prim(uint32_t lo_freq_hz, int rx_bw_index, int tx_bw_index, int prim_1m_loc);
#if defined(SF_WRITABLE)
void system_api_sflash_write(uint32_t address, struct _SYS_BUF *packet, size_t size);
uint16_t system_api_sflash_read(uint32_t sf_address, struct byte_stream *bs, int size);
#endif /* defined(SF_WRITABLE) */
uint16_t system_modem_api_get_hw_version();
#endif /* defined(STANDARD_11AH) */

void system_modem_api_update_probe_resp(uint8_t* probe, uint16_t len);
void system_modem_api_read_signal_noise(int loc, uint32_t *signal, uint32_t *noise);
uint32_t system_modem_api_get_snr(struct _SYS_BUF *packet);
uint32_t system_modem_api_get_current_snr(int loc);
uint32_t system_modem_api_get_current_snr_i(int loc);
void system_modem_api_init_retention();
bool system_modem_api_ps_get_ret_recovered();
bool system_modem_api_ps_get_ret_valid();
void system_modem_api_ps_set_pmk(uint8_t *pmk, uint32_t pmk_len);
void system_modem_api_ps_set_security(uint8_t security);
void system_modem_api_ps_set_key_tsc(uint8_t key_id, uint64_t tsc);
int system_modem_api_ps_set_sleep(uint8_t sleep_mode, uint64_t interval_ms);
void system_modem_api_ps_set_bss_max_idle(uint8_t option, uint16_t period);
void system_modem_api_set_ba_state(uint8_t tid, bool add, uint8_t ba_state);
void system_modem_api_ps_cleanup_dl_ring();
#if defined(INCLUDE_TWT_SUPPORT)
void system_modem_api_set_twt_service_period(int vif_id, uint32_t period);
void system_modem_api_set_twt_wake_interval(int vif_id, uint32_t interval);
uint32_t system_modem_api_get_twt_service_period();
uint32_t system_modem_api_get_twt_wake_interval();
void system_modem_api_set_tx_suppress_start(uint32_t setting, uint32_t value);
void system_modem_api_set_tx_suppress_stop(uint32_t setting);
#endif /* defined(INCLUDE_TWT_SUPPORT) */
#if defined(INCLUDE_AH_JPPC)
void system_modem_api_set_cca_ed_threshold(int vif_id);
#endif //#if definded (INCLUDE_AH_JPPC)

#if defined(INCLUDE_BD_SUPPORT_TARGET_VERSION)
bool system_modem_api_get_bd_block_flag();
void system_modem_api_set_bd_block_flag(bool flag);
#endif /* defined(INCLUDE_BD_SUPPORT_TARGET_VERSION) */
#endif //__SYSTEM_MODEM_API_H__
