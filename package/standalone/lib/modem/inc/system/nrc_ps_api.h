#ifndef __NRC_PS_API_H__
#define __NRC_PS_API_H__

#include <stdbool.h>
#include "nrc_ps_type.h"

//26B
struct key_info {
	bool		key_enable;
	uint8_t		key_type:6;
	uint8_t		key_id:2;
	uint32_t	key[4]; //16B (128bit Key)
	uint64_t	tsc; //8B (64bit)
#if defined(INCLUDE_7393_7394_WORKAROUND)
	uint16_t	hw_index;
#endif
} __attribute__ ((packed));

//3B
struct ampdu_info {
	uint8_t		enable:1;
	uint8_t		manual:1;
	uint8_t		agg_maxnum:6;
	uint16_t	agg_size_limit;
} __attribute__ ((packed));

//8B
struct edca_info {
	uint8_t		aifsn;
	uint8_t		acm;
	uint16_t	cw_min;
	uint16_t	cw_max;
#if !defined(NRC7292) /* remove this to store rc info */
	uint16_t	txop_limit;
#endif
} __attribute__ ((packed));

//8B
struct seqnum_info {
	uint16_t	tx_sn;
	uint16_t	win_end;
	uint16_t	win_start;
	uint16_t	win_bitmap;
} __attribute__ ((packed));

//20B
struct ret_stainfo {
	uint8_t 	sta_type;
	uint8_t 	mac_addr[6];
	uint8_t 	country[2];
	uint16_t	aid;
	uint16_t	listen_interval;
	bool		bss_max_idle_opt;
	uint16_t	bss_max_idle_period;
	uint32_t	rts_threahold;
} __attribute__ ((packed));
#define RET_STAINFO_SIZE sizeof(struct ret_stainfo)

//42B
struct ucode_stats {
	uint32_t	deepsleep_cnt;			//#of Entering Deep Sleep in Ucode
	uint32_t	wake_cnt;			//#of Exiting Wake up in Ucode
	uint32_t	rx_bcn_cnt;			//#of RX Beacon in Ucode
	uint32_t	pmc_err_cnt:6;			//#of PMC Error before WFI
	uint32_t	rtc_err_cnt:6;			//#of RTC Error before WFI
	uint32_t	pmc_int_cnt:6;			//#of PMC INT before WFI
	uint32_t	fw_ps_fail_cnt:7;		//#of Deep sleep failed, because of no ack
	uint32_t	fw_assert_cnt:7;		//#of FW ASSERT right before Deep Sleep
	uint16_t	wake_rsn_cnt[PS_WAKE_RSN_MAX];	//Wake-up Reason (2*13(26B))
	uint32_t	wake_margin_us;			//wakeup margin time in us
} __attribute__ ((packed));

//47B
struct ret_apinfo {
	uint8_t		bssid[6];
	uint8_t		ssid[32];
	uint8_t		ssid_len;
	uint8_t		security;
	uint8_t		akm;
	uint8_t		dtim_period;
	uint16_t	bcn_interval;
	uint16_t	short_bcn_interval;
	uint8_t		ctrl_resp_preamble_1m: 1;
	uint8_t		ndp_1m_dup: 1;
	uint8_t		sae_pwe:2;
	uint8_t		reserved:4 ;
} __attribute__ ((packed));
#define RET_APINFO_SIZE sizeof(struct ret_apinfo)

//16B
struct ret_s1ginfo {
	uint8_t		s1g_long_support: 1;
	uint8_t		pv1_frame_support: 1;
	uint8_t		nontim_support: 1;
	uint8_t		twtoption_activated: 1;
	uint8_t		ampdu_support: 1;
	uint8_t		ndp_pspoll_support: 1;
	uint8_t		shortgi_1mhz_support: 1;
	uint8_t		shortgi_2mhz_support: 1;
	uint8_t		shortgi_4mhz_support: 1;
	uint8_t		color: 3;
	uint8_t		max_mpdu_length: 1;
	uint8_t		min_mpdu_start_spacing: 3;
	uint8_t		max_ampdu_len_exp: 2;
	uint8_t		traveling_pilot_support: 2;
	uint8_t		ndp_preq_support: 1;
	uint8_t 	legacy_ack_support: 1;
	uint8_t 	beacon_bypass_support: 1;
	uint8_t 	twtgrouping_support: 1;
	uint8_t		rx_s1gmcs_map: 8;
	int		cca_threshold;
} __attribute__ ((packed));
#define RET_S1GINFO_SIZE sizeof(struct ret_s1ginfo)

#define	KCK_MAX_LEN	(16)
#define	KEK_MAX_LEN	(16)
//137B
struct ret_keyinfo {
	uint8_t		security;
	struct		key_info	cipher_info[4];
	/* for GTK rekey offload */
	uint8_t		kck[KCK_MAX_LEN];
	uint8_t		kek[KEK_MAX_LEN];
} __attribute__ ((packed));
#define RET_KEYINFO_SIZE sizeof(struct ret_keyinfo)

//4B
struct ret_usrinfo {
	uint32_t	txpwr:8;		//tx power (dBm)
	uint32_t	rxgain:8;		//rx gain (dBm)
	uint32_t	gi:8;			//guard interval (0:long, 1:short, 2:capa(auto))
	uint32_t	ucode_wake_src:4;	//ucode wake interval source (0:DTIM/1:TBTT/2:LI/3:USR)
	uint32_t	ucode_wake_by_group:1;	//ucode wake by DTIM BC/MC (0:disable, 1:enable)
	uint32_t	txpwr_type:2;		//tx power type (0:auto, 1:limit, 2:fixed)
	uint32_t	reserved:1;
} __attribute__ ((packed));
#define RET_USRINFO_SIZE sizeof(struct ret_usrinfo)

//5B
struct ret_chinfo {
	uint16_t	ch_freq;
	uint8_t		ch_bw;
	int8_t		ch_offset;
	uint8_t		primary_loc:7;
	uint8_t		center_lo:1;
#if defined(INCLUDE_NEW_CHANNEL_CTX)
	uint8_t		ch_index;
	uint8_t		oper;
	uint8_t		cca;
#endif
#if defined(INCLUDE_UCODE_TX)
	uint8_t		mcs10_tx_pwr; /* Tx in uCode use this */
#endif
} __attribute__ ((packed));
#define RET_CHINFO_SIZE sizeof(struct ret_chinfo)

// 32B + 12B = 44B
struct ret_acinfo {
	struct edca_info	edca[4];
	struct ampdu_info	agg_info[4];
} __attribute__ ((packed));
#define RET_ACINFO_SIZE sizeof(struct ret_acinfo)

//4B+ 64B + 8B-> 76B
struct ret_tidinfo {
	uint16_t tx_sn;
	uint16_t rx_sn;
	struct seqnum_info	seqnum[8];
	uint8_t ba_state[8];
} __attribute__ ((packed));
#define RET_TIDNFO_SIZE sizeof(struct ret_tidinfo)

//12B
struct ret_ipinfo {
	uint32_t	ip_addr;
	uint32_t	net_mask:31;
	uint32_t	ip_mode:1;
	uint32_t	gw_addr;
} __attribute__ ((packed));
#define RET_IPINFO_SIZE sizeof(struct ret_ipinfo)

//10B
struct ret_lbtinfo {
	uint16_t	cs_time;
	uint32_t	pause_time;
	uint32_t	resume_time;
} __attribute__ ((packed));
#define RET_LBTINFO_SIZE sizeof(struct ret_lbtinfo)

//375B
struct ret_rfinfo {
	uint8_t 	rf_efuse_revision;
#if defined(NRC7292)
	uint32_t	rf_reg_dump[92]; //RF register dump (368B)
#else
	uint32_t	rf_reg_dump[127]; //RF register dump (508B)
#endif
	int8_t		nrf_poc_param[3][2]; // (6B)
} __attribute__ ((packed));
#define RET_RFINFO_SIZE sizeof(struct ret_rfinfo)

#define WKUP_PIN_DEBOUNCE 		(6)
#define WKUP_PIN_NUMBER_MASK 	(BIT4 | BIT3 | BIT2 | BIT1 | BIT0)
#define PWR_PIN_ENABLE			(6)
#define PWR_PIN_NUMBER_MASK 	(BIT4 | BIT3 | BIT2 | BIT1 | BIT0)

//3B
struct ret_wakeupinfo {
	/*
	 * wakeup_gpio_pin
	 *	- [7, Reserved]
	 *	- [6, Enable debounce function for switch]
	 *	- [5, 1: Active High, 0: Active Low]
	 *	- [4:0, GPIO Number]
	 */
	uint8_t 	wakeup_gpio_pin;

#if defined(NRC7394)
	/*
	 * wakeup_gpio_pin2
	 *	- [7, Reserved]
	 *	- [6, Enable debounce function for switch]
	 *	- [5, 1: Active High, 0: Active Low]
	 *	- [4:0, GPIO Number]
	 */
	uint8_t 	wakeup_gpio_pin2;

#endif
	/*
	 * wakeup_source
	 *	- [3:7, reservedn]
	 *	- [2, CSPI]
	 *	- [1, GPIO]
	 *	- [0, RTC]
	 */
	uint8_t 	wakeup_source;

	/*
	 * power_indication_pin
	 *	- [7, Reserved]
	 *	- [6, Enable power_indication_pin]
	 *	- [5, Reserved]
	 *	- [4:0, GPIO Number]
	 */
	uint8_t 	power_indication_pin;
} __attribute__ ((packed));
#define RET_WAKEUPINFO_SIZE sizeof(struct ret_wakeupinfo)

//58B(16+42)
struct ret_ucodeinfo {
	uint32_t	wake_interval;
	uint32_t	enter_first:1;	//(1: Enter Ucode first)
	uint32_t	pmc_err:1;	//(1: PMC Error before WFI)
	uint32_t	rtc_err:1;	//(1: INVALID RTC TICK before WFI)
	uint32_t	target_wake:1;	//(1: Exit Ucode)
	uint32_t	req_deauth:1;	//(1: Request for Deauth)
	uint32_t	wfi_success:1;	//(1: success 0:fail)
	uint32_t	wake_reason:4;	//Reason of UCode wake-up
	uint32_t	pmc_status0:8;	//PMC STATUS0
	uint32_t	pmc_wake_src:6;	//PMC wakeup source
	uint32_t	pmc_int:1;	//(1: PMC INT before WFI)
	uint32_t	send_null:1;	//(1: Sent Null Frame before Deep Sleep)
	uint32_t	assert:1;	//(1: Assert right before Deep Sleep)
	uint32_t	ucode_test_mode:1;
	uint32_t	execute_rf_init:1;
	uint32_t	qos_null_pm1_ack:2;		// ack for qos_null frame with pm1 bit. (0: wait, 1: success, 2: fail)
	uint32_t	reserved:1;
	/* To calculate the deepsleep interval */
	uint32_t	last_beacon_tsf_lower;	// last received beacon timestamp tsf lower
	uint32_t	last_beacon_tsf_upper;	// last received beacon timestamp tsf upper
#if defined(INCLUDE_UCODE_TX)
	uint32_t	qos_null_tx_time;		// [31~12] = tsf_upper[10:0], [11:0] = tsf_lower[31:20], 
#endif
	uint16_t	ucode_beacon_timer_expire_cont;
	uint16_t	ucode_beacon_timer_expire_accum;
	struct 		ucode_stats	 stats;	//Ucode Statistics
} __attribute__ ((packed));
#define RET_UCODE_INFO_SIZE sizeof(struct ret_ucodeinfo)

// 7B
struct ret_drvinfo {
	uint16_t fw_boot_mode:2;		//FW boot mode (1B) : 0(XIP), 1 or 2(ROM, it must be distinguished in the code based on the core type)
	uint16_t do_reset:1;			//FW reboot mode in uCode (1B) : 0(reboot with jump), 1(reboot with reset)
	uint16_t cqm_off:1;
	uint16_t kern_ver:12;			// 4bit for major. 8bit for minor.
	uint8_t sw_enc:2;
	uint8_t brd_rev:2;
	uint8_t bitmap_encoding:1;
	uint8_t reverse_scrambler:1;
	uint8_t supported_ch_width:1; //supported CH width (0:1/2MHz, 1:1/2/4Mhz)
	uint8_t ps_pretend_flag:1;
	uint32_t vendor_oui;
} __attribute__ ((packed));
#define RET_DRV_INFO_SIZE sizeof(struct ret_drvinfo)

// 12B
struct ret_dutyinfo {
	uint32_t duty_window;
	uint32_t max_token;
	uint16_t duty_margin;
	uint16_t duty_beacon_margin;
} __attribute__ ((packed));
#define RET_DUTY_INFO_SIZE sizeof(struct ret_dutyinfo)

// 5B
struct ret_rcinfo {
		uint16_t maxtp:4;		/* 4bits for 0 ~ 15 */
		uint16_t tp2:4;
		uint16_t maxp:4;
		uint16_t probe:4;
		uint8_t ewma:3;			/* ewma                       : 1~5 => 10% 20% ... 50%    */
		uint8_t intval:3;		/* statistics update interval : 1~7 => 100ms 200ms...700ms */
		uint8_t probe_intval;	/* probe inerval              : 1~255 => 10ms 20ms ... 2550ms  */
//#if defined(INCLUDE_DUTYCYCLE)
		uint8_t org_intval:3;		/* original statistics update interval : 1~7 => 100ms 200ms...700ms */
		uint8_t org_probe_intval:5;	/* original probe inerval			  : 1~31 => 10ms 20ms ... 310ms  */
//#endif
} __attribute__ ((packed));
#define RET_RC_INFO_SIZE sizeof(struct ret_rcinfo)

#if defined(NRC7394)
// 1024B
#define RET_USER_DATA_SIZE 1024
struct ret_userData {
	uint8_t data[RET_USER_DATA_SIZE];
} __attribute__ ((packed));
#else
#define RET_USER_DATA_SIZE 0
#endif

#if defined (INCLUDE_WOWLAN_PATTERN)
// 56B
/* This must be matched with wim_pm_param */
#define WOWLAN_PATTER_SIZE      48
struct ret_wowlanptns {
	uint16_t offset:6;
	uint16_t mask_len:4;
	uint16_t pattern_len:6;
	uint8_t mask[WOWLAN_PATTER_SIZE/8];
	uint8_t pattern[WOWLAN_PATTER_SIZE];	// array size = array size of mask * 8
} __attribute__ ((packed));
#define RET_WOWLAN_PATTERNS_SIZE sizeof(struct ret_wowlanptns)

// 2B
struct ret_wowlaninfo {
	uint8_t n_patterns:2;
	uint8_t any:1;
	uint8_t magicpacket:1;	// not used currently. for future use.
	uint8_t disconnect:1;	// not used currently. for future use.
	uint8_t reserved:3;
	uint8_t wakeup_host_gpio_pin;
} __attribute__ ((packed));
#define RET_WOWLAN_INFO_SIZE sizeof(struct ret_wowlaninfo)
#endif /* INCLUDE_WOWLAN_PATTERN */

#if defined (INCLUDE_TCP_KEEPALIVE)
struct ret_keepaliveinfo {
	uint32_t seqnum;
	uint32_t acknum;
	uint8_t src_ip[4];
	uint8_t dest_ip[4];
	uint16_t src_port;
	uint16_t dest_port;
	uint16_t winsize;  /* need ?, CKLEE_TODO */
	uint32_t last_tsf;
	uint32_t period_sec;
} __attribute__ ((packed));
#endif

#define MAX_ETHERTYPE_FILTER	10
#define MAX_MAC_FILTER			10

struct ethertype_filter {
	uint8_t num;
	uint16_t entry[MAX_ETHERTYPE_FILTER];
};

struct mac_filter {
	uint8_t num;
	uint8_t entry[MAX_MAC_FILTER][6]; /* MAC_ADDR_LEN */
};

struct ret_bcmc_filter_info
{
	struct ethertype_filter ef;
	struct mac_filter mf;
};

#define MAX_BCMC_SIZE			2048
struct ret_bcmc_info
{
	uint8_t frame_num;
	uint8_t buffer[MAX_BCMC_SIZE];
};

#define RET_RECOVERED_SIZE 1

//116B
struct ret_airtimeinfo {
	/*
	 * Airtime Measurment
	 */
	uint32_t	ps_ucode_first_bootup;
	uint32_t	ps_ucode_first_idle_rfoff;
	uint32_t	ps_ucode_first_ds;
	uint64_t	ps_ucode_tsf_before_first_ds;

	uint32_t	ps_accum_ucode_bootup;
	uint32_t	ps_accum_ucode_init;
	uint32_t	ps_accum_ucode_idle;
	uint32_t	ps_accum_ucode_idle_rfoff;
	uint32_t	ps_accum_deepsleep;
	uint32_t	ps_accum_count_deepsleep;
	uint32_t	ps_accum_expired_deepsleep;

	uint16_t	ps_ucode_max_offset;
	uint16_t	ps_ucode_max_backoff;
	uint32_t	max_backoff_tsf_upper;
	uint32_t	max_backoff_tsf_lower;

	uint32_t 	ps_ucode_last_tsf;
	uint32_t	ps_ucode_last_fw_tsf;

	uint16_t	airtime_conversion_ucode_to_fw;
	uint16_t	airtime_conversion_ucode_to_fw_nextcyc;
	uint16_t	airtime_fw_idle_rfoff;
	uint16_t	airtime_fw_idle_rfoff_2nd;
	uint16_t	airtime_fw_idle_rfoff_2nd_nextcyc;
	uint16_t	esl_tx_sw;
	uint16_t	esl_tx_hw;
	uint16_t	esl_total;
	uint32_t	esl_ps_processing_time;
	uint32_t	esl_period_buf[3];

	uint16_t	ucode_idle_under_7;
	uint16_t	ucode_idle_under_30;
	uint16_t	ucode_idle_under_100;
	uint16_t	ucode_idle_over_100;

	uint16_t	max_ucode_to_main_ps;
	uint16_t	max_ucode_to_init_ps;
	uint32_t	max_ucode_to_bcn_ps;
} __attribute__ ((packed));
#define RET_AIRTIME_INFO_SIZE sizeof(struct ret_airtimeinfo)
#define RET_PMK_SIZE 32
#if defined(NRC7292)
#define WOWLAN_MAX_PATTERNS 1
#elif defined(NRC7394) || defined(NRC5292) || defined(NRC5293)
#define WOWLAN_MAX_PATTERNS 2
#else
#error "define wowlan pattern size, if ret size is enough, set 2 or set 1"
#endif

/* Retention Memory Total_1KB(1024B)
 * from 0x200B_BC00 to 0x200C_0000)
 * Last 16 Bytes for uCode Header
 */

#if defined(NRC7394)
#if !(defined(BOOT_LOADER) || (MASKROM) || (INCLUDE_RF_NRC7292RFE) || (LMAC_TEST))
#include "drv_port.h"
#include "hal_modem.h"
#include "nrc_retention.h"
#endif
#endif

#if defined(INCLUDE_TWT_SUPPORT)
#include "lmac_twt_common.h"
struct ret_twtinfo {
	ret_twt agm[MAX_TWT_AGREEMENT];
} __attribute__ ((packed));
#define RET_TWT_INFO_SIZE sizeof(struct ret_twtinfo)
#endif

#if defined(INCLUDE_AUTH_CONTROL)
//4Byte
struct ret_authctrlinfo {
	uint32_t	enable:1;
	uint32_t	slot:7;
	uint32_t	min:8;
	uint32_t	max:8;
	uint32_t	scale:8;
} __attribute__ ((packed));
#endif

struct retention_info {
#if defined(NRC7394)
#if !(defined(BOOT_LOADER) || (MASKROM) || (INCLUDE_RF_NRC7292RFE) || (LMAC_TEST))
	nrc_ret_info_t			nrc_ret_info;
	struct ret_userData		user_data;		//User data (1KB)
#endif
#endif
	struct ret_stainfo		sta_info;		//station info (20B)
	struct ret_apinfo		ap_info;		//ap info (47B)
	struct ret_s1ginfo		s1g_info;		//s1g info (16B)
	struct ret_keyinfo		key_info;		//key info (105B)
	struct ret_usrinfo		usr_conf_info;		//user config info (4B)
	struct ret_chinfo		ch_info;		//channel info (5B)
	struct ret_wakeupinfo	wakeup_info;		//wakeup source (3B)
	struct ret_acinfo		ac_info;		//info per AC (40B)
	struct ret_tidinfo		tid_info;		//info per TID ( 76B)
	struct ret_ipinfo		ip_info;		//ip info (12B)
	struct ret_lbtinfo		lbt_info;		//lbt info (10B)
	struct ret_rfinfo		rf_info;		//rf info (375B)
	struct ret_ucodeinfo	ucode_info;		//ucode info (58B)
	struct ret_drvinfo		drv_info;		//driver info (3B)
	struct ret_dutyinfo		duty_info;		//duty cycle info (12B)
	struct ret_rcinfo		rc_info;		//rc info (5B)
#if defined (INCLUDE_WOWLAN_PATTERN)
	struct ret_wowlanptns	wowlan_patterns[WOWLAN_MAX_PATTERNS];	//wowlan patterns (56B * WOWLAN_MAX_PATTERNS)
	struct ret_wowlaninfo	wowlan_info;		//wowlan info (2B)
#endif
#if defined (INCLUDE_TCP_KEEPALIVE)
	struct ret_keepaliveinfo keepalive_info;	//tcp keepalive info (28B)
#endif
#if defined(INCLUDE_MEASURE_AIRTIME) || defined(INCLUDE_MEASURE_AIRTIME_NONESL)
	struct ret_airtimeinfo	airtime_info;			//airtime info (116B)
#endif
#if defined (INCLUDE_PS_SCHEDULE)
	struct ret_schedule_info schedule_info;			//callback info (65B)
#endif
#if defined(INCLUDE_TWT_SUPPORT)
	struct ret_twtinfo		twt_info;
#endif
#if defined(INCLUDE_BCMC_RX)
	struct ret_bcmc_info bcmc_info;
#endif
#if defined(INCLUDE_BCMC_RX_FILTER)
	struct ret_bcmc_filter_info bcmc_filter_info;
#endif
#if defined(INCLUDE_AUTH_CONTROL)
	struct ret_authctrlinfo auth_ctrl_info;
#endif
	/* GPIO setting while in deep sleep */
	uint32_t sleep_gpio_dir_mask;
	uint32_t sleep_gpio_out_mask;
	uint32_t sleep_gpio_pullup_mask;
#if defined (INCLUDE_AVOID_FRAG_ATTACK_TEST)
	uint8_t 			prev_ptk[RET_PMK_SIZE];	//Previous PTK (32B)
#endif
#if !defined(INCLUDE_RTC_ALWAYS_ON)
	uint32_t			sync_time_ms;		//time for recovery (4B)
#endif
#if defined(INCLUDE_NRC7392_UCODE_TEST)
	uint32_t			ucode_test_deepsleep_interval_ms;
	uint32_t			ucode_test_idle_interval_ms;
	uint32_t			ucode_test_count;
	uint32_t			ucode_count;
	uint32_t			ucode_interval_ms;
	uint32_t			ucode_deepsleep_interval_ms;
#endif
	uint8_t				pmk[RET_PMK_SIZE];	//PMK(PSK) (32B)
	uint32_t			sig_a;			//sig a (4B)
	uint32_t			sig_b;			//sig b (4B)
	uint8_t				ps_mode;		//Power saving mode (1B) (none/modem/tim/nontim)
	uint64_t			ps_duration;		//Power save duration (8B) (in ms unit)
	uint64_t			usr_timeout;		//TIM-mode user timer (8B) (in ms unit)
	uint32_t			wdt_flag:8;		//WDT Reset Flag (1B)
	uint32_t			wdt_cnt: 24;		//WDT Reset Count (3B)
	uint8_t				fota_in_progress:1;	//fota in progress flag(1bit) (1: in progress, 0: not in progress)
	uint8_t				fota_done:1;		//fota done flag(1bit) (1: done, 0: not compeleted)
	uint8_t				wake_by_usr:1;		//wakeup in ucode for user timer(1bit) (1: wakeup, 0: none)
	uint8_t				xtal_status:2;		// 0(not checked), 1(working), 2(not working)
	uint8_t				ps_null_pm0:1;		//0(nothing) 1(need to send null with PM0 after wakeup)
	uint8_t				fast_connect:1;		//0(disable) 1(enable)
	uint8_t				reserved:1;		//reserved 3 bits
	bool				recovered;		//recovery status(1B) (true:recovered, false:not recovered)
	bool				sleep_alone;		//sleep without connection(1B) (true:alone false:with AP)
	uint8_t				rc_mode:2;		// rate control(1B) for STA
	uint8_t				rc_default_mcs:4;
	uint8_t				rc_reserved:2;

} __attribute__ ((packed));
#define RET_TOTAL_SIZE 				sizeof(struct retention_info)

typedef void (*nrc_ps_cb_t)(enum ps_event event,enum sys_operation operation);
struct nrc_ps_ops {
	nrc_ps_cb_t cb;
};

/* ============================================================================== */
/* =================== Internal INTERFACE ======================================= */
/* ============================================================================== */
struct retention_info* nrc_ps_get_retention_info();
struct nrc_ps_ops *nrc_ps_get_user(void);
struct nrc_ps_ops *nrc_ps_get_vendor(void);
struct nrc_ps_ops *nrc_ps_get(void);

#define PS_EVENT_CB(e,o) do { \
	struct nrc_ps_ops *ops = nrc_ps_get();\
	nrc_ps_cb_t cb = ops->cb;\
	if (ops && cb) \
		cb(e, o);\
} while(0)

#define PS_FILL_SIG() do { \
    struct retention_info* ret_info;\
    ret_info = nrc_ps_get_retention_info();\
	ret_info->sig_a 	= PS_SIG_A; \
	ret_info->sig_b 	= PS_SIG_B; \
} while(0)

#define PS_CLEAR_SIG() do { \
    struct retention_info* ret_info;\
    ret_info = nrc_ps_get_retention_info();\
    ret_info->sig_a = 0;\
    ret_info->sig_b = 0; \
} while(0)

static inline bool PS_CHECK_SIG() {
	struct retention_info* ret_info;
	ret_info = nrc_ps_get_retention_info();
	if (ret_info->sig_a == PS_SIG_A
		&& ret_info->sig_b == PS_SIG_B)
		return true;
	return false;
};

#if defined (INCLUDE_STANDALONE) && (INCLUDE_FAST_CONNECT)
#define PS_FILL_SIG_2() do { \
    struct retention_info* ret_info;\
    ret_info = nrc_ps_get_retention_info();\
	ret_info->sig_a 	= PS_SIG_C; \
	ret_info->sig_b 	= PS_SIG_D; \
} while(0)

static inline bool PS_CHECK_SIG_2() {
	struct retention_info* ret_info;
	ret_info = nrc_ps_get_retention_info();
	if (ret_info->sig_a == PS_SIG_C
		&& ret_info->sig_b == PS_SIG_D)
		return true;
	return false;
};
#endif /* defined (INCLUDE_STANDALONE) && (INCLUDE_FAST_CONNECT) */

#if !defined(UCODE)
void nrc_ps_init();
void nrc_ps_init_retention_info(bool cold_boot);
void nrc_ps_recovery();
int nrc_ps_config_wakeup_pin(bool check_debounce, int pin_number, bool active_high);
#if defined(NRC7394)
int nrc_ps_config_wakeup_pin2(bool check_debounce, int pin_number, bool active_high);
#endif
int nrc_ps_config_wakeup_source(uint8_t wakeup_source);
int nrc_ps_get_wakeup_pin(bool *check_debounce, int *pin_number, bool *active_high);
int nrc_ps_get_wakeup_source(uint8_t *wakeup_source);
int nrc_ps_get_wakeup_reason(uint8_t *wakeup_reason);
int nrc_ps_get_wakeup_count(uint32_t *wakeup_count);
int nrc_ps_reset_wakeup_count();
void nrc_ps_show_ucode_stats();
void nrc_ps_set_ucode();
void nrc_ps_set_last_beacon_tsf(uint32_t upper, uint32_t lower, uint32_t diff);

#if defined (INCLUDE_PS_SCHEDULE)
/**********************************************
 * @fn  int nrc_ps_schedule_add(uint32_t timeout, bool net_init, scheduled_callback func)
 *
 * @brief Add schedule information.
 *        timeout, whether to enable wifi, and callback funtion to execute when
 *        the scheduled time is reached.
 *        This information will be added into retention memory and processed in
 *        main() and standalone_main().
 *        If net_init is set to true, then wifi and network will be initialized.
 *        Current implementation can accept up to 4 individual schedules.
 *        Each individual schedule should have at least one minute apart in timeout.
 *        When adding schedule the callback should be able to finish in the time window.
 *
 * @param timeout: value in msec for this schedule.
 * @param net_init: whether callback will require wifi connection
 * @param func: scheduled_callback funtion pointer defined as void func()
 *
 * @return 0 for success and -1 for failure.
 ***********************************************/
int nrc_ps_schedule_add(uint32_t timeout, bool net_init, scheduled_callback func);

/**********************************************
 * @fn  int nrc_ps_schedule_gpio_add(bool net_init, scheduled_callback func)
 *
 * @brief Add gpio exception callback to handle gpio interrupted wake up.
 *        This information will be added into retention memory and
 *        processed if gpio interrupt occurs.
 *        If net_init is set to true, then wifi and network will be initialized.
 *
 * @param net_init: whether callback will require wifi connection
 * @param func: scheduled_callback funtion pointer defined as void func()
 *
 * @return 0 for success and -1 for failure.
 ***********************************************/
int nrc_ps_schedule_gpio_add(bool net_init, scheduled_callback func);

/**********************************************
 * @fn  int nrc_ps_schedule_start()
 *
 * @brief Starts scheduled deep sleep configured with nrc_ps_schedule_add.
 *
 * @param None
 *
 * @return 0 for success and -1 for failure.
 ***********************************************/
int nrc_ps_schedule_start();

/**********************************************
 * @fn  int nrc_ps_schedule_resume(uint64_t timeout)
 *
 * @brief Resumes scheduler after wakeup with timeout value given.
 *        timeout value can be found using "nrc_ps_get_next_sleep_time()"
 *
 * @param timeout : timeout for next wakeup
 *
 * @return 0 for success and -1 for failure.
 ***********************************************/
int nrc_ps_schedule_resume(uint64_t timeout);

/**********************************************
 * @fn  uint64_t nrc_ps_get_next_sleep_time()
 *
 * @brief Used to calculate next timeout based on schedule information
 *        saved in retention memory (schedule_info)
 *
 * @param None
 *
 * @return next sleep duration
 ***********************************************/
uint64_t nrc_ps_get_next_sleep_time();

/**********************************************
 * @fn  bool nrc_ps_callback_run(uint32_t index)
 *
 * @brief Iterate schedule_info in retention memory to find
 *        whether the given scheduled entry should be run at
 *        current RTC time.
 *
 * @param index: schedule index in schedule_info
 *
 * @return true if the given schedule should run
 *         false otherwise.
 ***********************************************/
bool nrc_ps_callback_run(uint32_t index);
#endif /* INCLUDE_PS_SCHEDULE */

#endif /* !defined(UCODE) */

int nrc_ps_config_power_indication_pin(bool enable, int pin_number);
int nrc_ps_get_power_indication_pin(bool *enable, int *pin_number);
void ps_gpio_config_wakeup_pin();
void ps_gpio_set_deepsleep(uint32_t pullup_mask, uint32_t gpio_out_mask, uint32_t gpio_dir_mask);
void ps_set_gpio_direction(uint32_t mask);
uint32_t ps_get_gpio_direction(void);
void ps_set_gpio_out(uint32_t mask);
uint32_t ps_get_gpio_out(void);
void ps_set_gpio_pullup(uint32_t mask);
uint32_t ps_get_gpio_pullup(void);
int nrc_ps_event_user_get(enum ps_event event);
int nrc_ps_event_user_clear(enum ps_event event);
bool nrc_ps_check_fast_connect(void);
bool nrc_ps_set_user_data(void* data, uint16_t size);
bool nrc_ps_get_user_data(void* data, uint16_t size);
uint16_t nrc_ps_get_user_data_size(void);

#endif /*__NRC_PS_API_H__*/
