#ifndef	__SYSTEM_MODEM_API_H__
#define __SYSTEM_MODEM_API_H__

#include <stdbool.h>
#include "nrc-wim-types.h"
#include "system_type.h"
#include "lmac_type.h"
#include "lmac_ps_common.h"
#include "lmac_common.h"
#include "util_byte_stream.h"
#if defined (BOOT_LOADER) || (INCLUDE_RF_NRC7292RFE) || (LMAC_TEST)
#if defined (NRC7393) || defined (NRC7394)
#include "hal_sflash_legacy.h"
#else
#include "hal_sflash.h"
#endif
#else
#include "hal_sflash.h"
#endif

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
	uint8_t 		addr[6];		//< MAC Address
	uint16_t		aid;
	uint8_t 		disable_hwmic;
#if defined(INCLUDE_7393_7394_WORKAROUND)
	uint16_t		hw_index;
#endif
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

#if defined(INCLUDE_SCAN_MODE)
enum {
	SCAN_NORMAL	= 0,
	SCAN_PASSIVE	= 1,
	SCAN_FAST	= 2,
	SCAN_FAST_PASSIVE	= 3,
	SCAN_MAX,
};
#endif

#if defined (INCLUDE_AMPDU_AUTO_TX)
enum {
	LMAC_AMPDU_TX_OFF	= 0,
	LMAC_AMPDU_TX_ON	= 1,
};
#endif

uint32_t system_modem_api_get_dl_hif_length(struct _SYS_BUF *packet);
uint32_t system_modem_api_get_tx_space();
uint32_t system_modem_api_get_rx_space();
void 	 system_modem_api_get_capabilities(struct wim_cap_param* param);

#define system_modem_api_set_mac_address(x, y)		lmac_set_mac_address(x, y)
#define system_modem_api_get_mac_address(x)		lmac_get_mac_address(x)
#define system_modem_api_get_bssid(x)			lmac_get_bssid(x)
#define system_modem_api_enable_bssid(x, y)		lmac_set_enable_bssid(x, y)
#define system_modem_api_get_enable_bssid(x)	lmac_get_enable_bssid(x)
#define system_modem_api_mode_bssid(x, y)		lmac_mode_bssid(x, y)
#define system_modem_api_set_aid(x, y)			lmac_set_aid(x, y)
#define system_modem_api_get_aid(x)			lmac_get_aid(x)
#define system_modem_api_set_mode(x, y)			lmac_set_mode(x, y)
#define system_modem_api_is_ap(x)			lmac_is_ap(x)
#define system_modem_api_is_sta(x)			lmac_is_sta(x)
#define system_modem_api_is_mesh(x)			lmac_is_mesh(x)
#define system_modem_api_is_mesh_ap(x)			lmac_is_mesh_ap(x)
#define system_modem_api_is_relay()			lmac_is_relay()
#define system_modem_api_is_concurrent()		lmac_is_concurrent()
#define system_modem_api_get_mesh_vif_id()		lmac_get_mesh_vif_id()
#define system_modem_api_set_cca_ignore(x)		lmac_set_cca_ignore(x)
#define system_modem_api_set_cipher_ignore(x, y)	lmac_set_cipher_ignore(x, y)
#define system_modem_api_set_promiscuous_mode(x, y)	lmac_set_promiscuous_mode(x, y)
#define system_modem_api_ps_set_bss_max_idle(x, y)	lmac_ps_set_bss_maxidle(x, y)
#define system_modem_api_set_ba_state(x, y, z)		lmac_ps_set_ba_state(x, y, z)
#define system_modem_api_ps_get_ret_recovered()		lmac_ps_get_ret_recovered()
#define system_modem_api_ps_get_ret_valid()		lmac_ps_get_ret_validity()
#define system_modem_api_ps_set_pmk(x, y)		lmac_ps_set_pmk(x, y)
#define system_modem_api_ps_set_security(x)		lmac_ps_set_security(x)
#define system_modem_api_ps_set_akm(x)			lmac_ps_set_akm(x)
#define system_modem_api_ps_set_key_tsc(x, y)		lmac_ps_set_key_tsc(x, y)
#define system_api_set_promiscuous_mode(x)		lmac_set_promiscuous_mode(0, x)
#define system_api_get_bypass_beacon(x)			lmac_get_bypass_beacon_mode(x)
#define system_api_set_bypass_beacon(x)			lmac_set_bypass_beacon_mode(0, x)
#define system_modem_api_get_rate_control(x)		lmac_get_rate_control(x)
#if defined(INCLUDE_TWT_SUPPORT)
#define system_modem_api_set_twt_service_period(x, y)	lmac_set_twt_service_period(y)
#define system_modem_api_set_twt_wake_interval(x, y)	lmac_set_twt_wake_interval(y)
#define system_modem_api_get_twt_service_period()	lmac_get_twt_service_period()
#define system_modem_api_get_twt_wake_interval()	lmac_get_twt_wake_interval()
#endif /* defined(INCLUDE_TWT_SUPPORT) */
#if defined(NRC7393) || defined(NRC7394)
#define system_modem_api_enable_mac_address(x, y)	lmac_set_enable_mac_addr(x, y)
#else
#define system_modem_api_enable_mac_address(x, y)	0 // TODO
#endif /* defined(NRC7393) */
#define system_modem_api_add_key(x, y, z)		lmac_sec_add_key(x, y, z)
#define system_modem_api_del_key(x, y)			lmac_sec_del_key(x, y)
#define system_modem_api_del_key_all(x)			lmac_sec_del_key_all(x)
#define system_modem_api_set_basic_rate(x, y)		lmac_set_basic_rate(x, y)
#define system_modem_api_set_tsf_timer(x, y, z)		lmac_set_tsf_timer(x, y, z)
#define system_modem_api_set_response_ind(x, y)		lmac_set_response_ind(x, y)
#define system_modem_api_get_response_ind(x)		lmac_get_response_ind(x)
#define system_modem_api_set_aggregation(x, y)		lmac_set_aggregation(x, y)
#define system_modem_api_set_aggregation_ap_by_macaddr(x, y, z, a)	lmac_set_aggregation_ap(x, y, z, a, 0)
#define system_modem_api_set_aggregation_ap_by_aid(x, y, z, a)		lmac_set_aggregation_ap(x, y, z, NULL, a)
#define system_modem_api_get_aggregation(x)				lmac_get_aggregation(x)
#define system_modem_api_get_aggregation_ap_by_macaddr(x, y, z)		lmac_get_aggregation_ap(x, y, z, 0)
#define system_modem_api_get_aggregation_ap_by_aid(x, y, z)		lmac_get_aggregation_ap(x, y, NULL, z)
#define system_modem_api_support_lbt()		lmac_support_lbt()

#define system_modem_api_set_user_timeout(x,y)	lmac_ps_set_usr_timer(x, y)
#define system_modem_api_set_dyn_ps_timeout(x,y)	lmac_dyn_ps_set_timeout(x, y)
#define system_modem_api_support_modem_sleep()	lmac_ps_get_modem_sleep_support()
#define system_modem_api_stop_modemsleep()	lmac_ps_modemsleep_stop()
#define system_modem_api_go_sleep_alone(x, y)	lmac_ps_go_sleep_alone(x, y)

#define system_modem_api_ps_set_ret_short_beacon_interval(x)		lmac_ps_set_short_bcn_interval(x)
#define system_modem_api_ps_set_ret_beacon_interval(x)		lmac_ps_set_bcn_interval(x)

void     system_modem_api_set_ssid(int vif_id , uint8_t *ssid , uint8_t ssid_len);
void     system_modem_api_set_dtim_period(int vif_id, uint8_t period);
uint8_t  system_modem_api_get_dtim_period(int vif_id);
void     system_modem_api_enable_beacon(int vif_id, bool enable);
void     system_modem_api_update_beacon(int vif_id, uint8_t* ,uint16_t );
void     system_modem_api_set_beacon_interval(int vif_id, uint16_t beacon_interval);
uint16_t system_modem_api_get_beacon_interval(int vif_id);
void     system_modem_api_ap_enable_short_beacon(int vif_id, bool enable, bool fota);
void     system_modem_api_ap_set_short_beacon_interval(int vif_id, uint16_t short_beacon_interval);
uint16_t system_modem_api_ap_get_short_beacon_interval(int vif_id);
void     system_modem_api_set_tim_flag(uint8_t vif_id);
void     system_modem_api_set_bssid(int vif_id, uint8_t *bssid);
void     system_modem_api_set_edca_param(int vif_id, struct edca_param* edca);
#if defined (INCLUDE_IBSS)
bool     system_modem_api_is_ibss(int vif_id);
#endif
bool     system_modem_api_set_erp_param(int vif_id, struct wim_erp_param* p);
int      system_modem_api_get_cipher_icv_length(int type);
bool     system_modem_api_sec_set_enable_key(int vif_id, bool enable);
bool     system_modem_api_sec_get_enable_key   (int vif_id);
bool     system_modem_api_set_channel(int vif_id, uint32_t ch_freq);
#if defined(INCLUDE_NEW_CHANNEL_CTX)
bool     system_modem_api_set_channel_ps(int vif_id, struct ret_chinfo ch_info);
#endif /* defined(INCLUDE_NEW_CHANNEL_CTX) */
bool     system_modem_api_set_channel_width(int vif_id, uint8_t chan_width, uint8_t prim_loc);
int8_t   system_modem_api_get_gi(int vif_id);
bool     system_modem_api_set_gi(int vif_id, uint8_t type);
void     system_modem_api_set_short_gi(int vif_id, uint8_t short_gi, bool gi_auto_flag);
bool     system_modem_api_get_short_gi(int vif_id, GenericMacHeader *mqh);
void     system_modem_api_set_rate_control(int vif_id, bool enable);
void     system_modem_api_set_rc_mode (uint8_t vif_id, uint8_t mode);
void     system_api_get_supported_channels(uint16_t **chs, int *n_ch);
void     system_modem_api_set_txgain(uint32_t txgain);
void     system_modem_api_set_rxgain(uint32_t rxgain);
uint16_t system_modem_api_get_frequency(struct _SYS_BUF *packet);
int      system_modem_api_get_rssi(struct _SYS_BUF *packet);
uint16_t system_modem_api_get_current_channel_number();
uint8_t  system_modem_api_mac80211_frequency_to_channel(uint32_t frequency);
uint32_t system_modem_api_channel_to_mac80211_frequency(uint8_t channel);
bool     system_modem_api_set_cs_time(uint16_t value);
uint16_t system_modem_api_get_cs_time(void);
bool     system_modem_api_set_tx_pause_time(uint32_t value);
uint32_t system_modem_api_get_tx_pause_time(void);
bool     system_modem_api_set_tx_time(uint16_t cs_time, uint32_t pause_time);
bool     system_modem_api_get_duty_cycle(uint32_t *window, uint32_t *duration, uint32_t *margin);
bool     system_modem_api_enable_duty_cycle(uint32_t window, uint32_t duration, uint32_t margin);
bool     system_modem_api_disable_duty_cycle(void);
bool     system_modem_api_tx_avaliable_duty_cycle(void);
bool     system_modem_api_set_cca_threshold(int vif_id, int cca_threshold);
int      system_modem_api_get_cca_threshold(int vif_id);
void     system_modem_api_set_tx_suppress_dur(uint32_t value);
void     system_modem_api_set_tx_suppress_cmd(uint32_t value);
int8_t   system_modem_api_get_ignore_broadcast_ssid_type(void);
uint32_t system_api_get_version(void);
uint32_t system_api_get_align(void);
uint32_t system_api_get_buffer_length(void);
bool 	 system_api_get_rssi(int vif_id , int8_t* rssi_avg , int8_t* rssi_last);

#if defined (INCLUDE_AMPDU_AUTO_TX)
void system_modem_tx_ampdu_control(int vif_id, uint8_t control, uint8_t *addr, uint8_t tid);
#endif
bool system_modem_api_set_mcs(uint8_t mcs);
uint8_t system_modem_api_get_tx_mcs(int vif_id);
uint8_t system_modem_api_get_rx_mcs(int vif_id);

uint8_t system_modem_api_get_tx_power(int vif_id);
int system_modem_api_set_tx_power(int type, uint8_t txpwr);

#if defined(INCLUDE_BD_SUPPORT)
bool system_modem_api_set_bd_data(int vif_id, struct wim_bd_param* p);
void system_modem_api_set_tx_power_by_channel_index(int vif_id, uint8_t ch_id);
#endif /* defined(INCLUDE_BD_SUPPORT) */
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
int  system_api_set_freq_bw_prim(uint32_t lo_freq_hz, bw_t rx_bw_index, bw_t tx_bw_index, int prim_1m_loc);
void system_api_set_tx_lpf(uint8_t vif_id);
#if defined(SF_WRITABLE)
void system_api_sflash_write(uint32_t address, struct _SYS_BUF *packet, size_t size);
uint16_t system_api_sflash_read(uint32_t sf_address, struct byte_stream *bs, int size);
uint32_t system_api_sflash_get_user_data_area_size(void);
bool system_api_sflash_write_user_data(uint32_t user_data_offset, uint8_t* data, uint32_t size);
bool system_api_sflash_read_user_data(uint8_t* buffer, uint32_t user_data_offset, uint32_t size);
bool system_api_sflash_write_device_info(uint8_t* data, uint32_t size);
bool system_api_sflash_read_device_info(uint8_t* data, uint32_t size);
#endif /* defined(SF_WRITABLE) */
uint16_t system_modem_api_get_hw_version();
bool system_modem_api_set_hw_version(uint16_t version);

void system_modem_api_update_probe_resp(uint8_t* probe, uint16_t len);
void system_modem_api_read_signal_noise(int loc, uint32_t *signal, uint32_t *noise);
uint32_t system_modem_api_get_snr(struct _SYS_BUF *packet);
int system_modem_api_get_avg_snr(int vif_id);
uint32_t system_modem_api_get_current_snr(int loc);
uint32_t system_modem_api_get_current_snr_i(int loc);
void system_modem_api_init_retention();
int system_modem_api_ps_set_sleep(uint8_t sleep_mode, uint64_t interval_ms);
void system_modem_api_ps_cleanup_dl_ring();
bool system_modem_api_set_listen_interval(int vif_id, uint16_t interval);
bool system_modem_api_get_listen_interval(int vif_id, uint16_t *interval, uint32_t *interval_ms);
#if defined(INCLUDE_TWT_SUPPORT)
void system_modem_api_set_tx_suppress_start(uint32_t setting, uint32_t value);
void system_modem_api_set_tx_suppress_stop(uint32_t setting);
#endif /* defined(INCLUDE_TWT_SUPPORT) */
#if defined(INCLUDE_SCAN_MODE)
uint8_t system_modem_api_get_scan_mode(void);
bool system_modem_api_set_scan_mode(uint8_t mode);
void system_modem_api_set_fast_scan(bool fast_scan_on);
bool system_modem_api_get_fast_scan();
uint8_t *system_modem_api_get_fast_bssid();
bool system_modem_api_set_fast_bssid(char *bssid);
void system_modem_api_set_fast_scan_param(uint8_t* ssid, uint8_t ssid_len, uint8_t* bssid, uint16_t freq, uint8_t security);
void system_modem_api_change_scan_period(uint32_t multiply);
#endif
#if defined (INCLUDE_STANDALONE)
int system_modem_api_sta_get_ap_info(uint8_t vif_id, void *info, uint16_t len);
uint16_t system_modem_api_ap_get_num_sta(uint8_t vif_id);
int system_modem_api_ap_get_sta_list(uint8_t vif_id, void *info, uint16_t len);
int system_modem_api_ap_get_sta_by_addr(uint8_t vif_id, uint8_t *addr, void *info, uint16_t len);
void system_modem_api_sw_reset(void);
void system_modem_api_set_rf_power(bool power_on);
#endif
bool system_modem_api_powersave_set_sleep(uint8_t sleep_mode,\
		uint64_t nontim_sleep_ms,uint32_t idle_timout_ms, uint32_t tim_sleep_ms, bool sleep_alone);
bool system_modem_api_ps_resume_deep_sleep();
bool system_modem_api_ps_set_gpio_wakeup_pin(bool check_debounce, int pin_number);
bool system_modem_api_ps_set_wakeup_source(uint8_t wakeup_source);
void system_modem_api_ps_set_gpio_direction(uint32_t bitmask);
void system_modem_api_ps_set_gpio_out(uint32_t bitmask);
void system_modem_api_ps_set_gpio_pullup(uint32_t bitmask);
bool system_modem_api_ps_wakeup_reason(uint8_t *reason);
bool system_modem_api_ps_add_schedule(uint32_t timeout, bool net_init, void*  func);
bool system_modem_api_ps_add_gpio_callback(bool net_init, void*  func);
bool system_modem_api_ps_start_schedule(void);
void system_modem_api_run_retention_schedule();
void system_modem_api_ps_check_network_init(bool* net_init, bool* ps_callback);
void system_modem_api_ps_schedule_resume_for_timeout();
int system_modem_api_ps_event_user_get(int event);
int system_modem_api_ps_event_user_clear(int   event);

/* KR -MIC Scan */
void system_modem_api_get_mic_scan(bool *enable);
void system_modem_api_set_mic_scan(bool enable);
void system_modem_api_get_mic_scan_move(bool *enable);
void system_modem_api_set_mic_scan_move(bool enable);
void system_modem_api_clear_mic_detect_count(void);
uint32_t system_modem_api_get_mic_detect_count(void);

/* Beacon Monitor (aka CQM) */
void system_modem_api_enable_bmt(int vif_id, bool enable);
void system_modem_api_set_bmt_threshold(int vif_id, uint32_t loss_count);
uint32_t system_modem_api_get_bmt_threshold(int vif_id);

void system_modem_api_update_probe_req(int vif_id, uint8_t* ven_ie, uint16_t len);
void system_modem_api_update_probe_rsp(int vif_id, uint8_t* ven_ie, uint16_t len);
void system_modem_api_update_assoc_req(int vif_id, uint8_t* ven_ie, uint16_t len);

bool system_modem_api_set_support_ch_width(uint8_t sup_ch_width);

#if defined(INCLUDE_MANUAL_CONT_TX_SUPPORT)
bool system_modem_api_set_cont_tx(bool enable, uint32_t freq_100k, const char* bw, uint8_t mcs, uint8_t txpwr, uint32_t interval);
#endif

#endif //__SYSTEM_MODEM_API_H__
