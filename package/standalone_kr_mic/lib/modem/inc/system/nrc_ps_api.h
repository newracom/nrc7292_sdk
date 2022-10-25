#ifndef __NRC_PS_API_H__
#define __NRC_PS_API_H__

#include <stdbool.h>

#define PS_SIG_A    0x13579bdf
#define PS_SIG_B    0x2468ace0

enum ps_event {
	PS_EVT_COLDBOOT,
	PS_EVT_BEFORE_DEEPSLEEP,
	PS_EVT_WAKEUP_DEEPSLEEP,
	PS_EVT_BEFORE_WAKEUP,       /* UCODE only */
	PS_EVT_BEFORE_MODEMSLEEP,
	PS_EVT_WAKEUP_MODEMSLEEP,
	PS_EVT_MAX
};

enum ps_ucode_wake_reason {
	PS_WAKE_NONTIM,
	PS_WAKE_RX_UNICAST,
	PS_WAKE_RX_BROADCAST,
	PS_WAKE_EXT_INT_1,
	PS_WAKE_EXT_INT_2,
	PS_WAKE_BEACON_LOSS,		//5 abnormal
	PS_WAKE_INVALID_RET_INFO,	//6 abnormal
	PS_WAKE_RTC_TIMEOUT,
#if defined(NRC7292)
	PS_WAKE_HSPI,
#endif
	PS_WAKE_RSN_MAX,
};

enum ps_ucode_wake_source {
	PS_WAKE_SRC_DTIM,
	PS_WAKE_SRC_TBTT,
	PS_WAKE_SRC_LI,
	PS_WAKE_SRC_USER_VAL,
};

enum sys_operation {
	SYS_OPER_FW,
	SYS_OPER_UCODE,
	SYS_OPER_MAX
};

#define WKUP_SRC_RTC BIT0
#define WKUP_SRC_GPIO BIT1
#define WKUP_SRC_HSPI BIT2
#define WKUP_SRC_MAX (3)

//26B
struct key_info {
	bool		key_enable;
	uint8_t		key_type:6;
	uint8_t		key_id:2;
	uint32_t	key[4]; //16B (128bit Key)
	uint64_t	tsc; //8B (64bit)
} __attribute__ ((packed));

//2B
struct ampdu_info {
	uint8_t		enable;
	uint8_t		agg_maxnum;
} __attribute__ ((packed));

//8B
struct edca_info {
	uint8_t		aifsn;
	uint8_t		acm;
	uint16_t	cw_min;
	uint16_t	cw_max;
	uint16_t	txop_limit;
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

//48B(NRC7292), 46B(Others)
struct ucode_stats {
	uint32_t	deepsleep_cnt;		//#of Entering Deep Sleep in Ucode
	uint32_t	wake_cnt;		//#of Exiting Wake up in Ucode
	uint32_t	rx_bcn_cnt;		//#of RX Beacon in Ucode
	uint32_t	pmc_err_cnt;		//#of PMC Error before WFI
	uint32_t	rtc_err_cnt;		//#of RTC Error before WFI
	uint32_t	pmc_int_cnt;		//#of PMC INT before WFI
	uint32_t	fw_force_rst_cnt;	//#of FW Deep sleep by force (without CB of Null Frame)
	uint32_t	fw_assert_cnt;	//#of FW ASSERT right before Deep Sleep
	uint16_t	wake_rsn_cnt[PS_WAKE_RSN_MAX]; //8*2(16B for NRC7292), 7*2(14B for Others) //Wake-up Reason
} __attribute__ ((packed));

//46B
struct ret_apinfo {
	uint8_t		bssid[6];
	uint8_t		ssid[32];
	uint8_t		ssid_len;
	uint8_t		security;
	uint8_t		dtim_period;
	uint16_t	bcn_interval;
	uint16_t	short_bcn_interval;
	uint8_t		ctrl_resp_preamble_1m: 1;
	uint8_t		ndp_1m_dup: 1;
	uint8_t		reserved:6 ;
} __attribute__ ((packed));
#define RET_APINFO_SIZE sizeof(struct ret_apinfo)

//12B
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
	uint8_t		reserved: 3;
	uint8_t		rx_s1gmcs_map: 8;
	uint32_t	twt_service_period;
	uint32_t	twt_wake_interval;
} __attribute__ ((packed));
#define RET_S1GINFO_SIZE sizeof(struct ret_s1ginfo)

//105B
struct ret_keyinfo {
	uint8_t		security;
	struct key_info	cipher_info[4];
} __attribute__ ((packed));
#define RET_KEYINFO_SIZE sizeof(struct ret_keyinfo)

//6B
struct ret_usrinfo {
	uint8_t		txpwr;			//tx power (dBm)
	uint8_t		gi;			//guard interval (0:long, 1:short)
	uint32_t	rxgain:8;		//rx gain (dBm)
	uint32_t	ucode_wake_src:4;	//ucode wake interval source (0:DTIM/1:TBTT/2:LI/3:USR)
	uint32_t	ucode_wake_by_group:1; //ucode wake by DTIM BC/MC (0:disable, 1:enable)
	uint32_t	reserved:19;
} __attribute__ ((packed));
#define RET_USRINFO_SIZE sizeof(struct ret_usrinfo)

//5B
struct ret_chinfo {
	uint16_t	ch_freq;
	uint8_t		ch_bw;
	int8_t		ch_offset;
	uint8_t		primary_loc;
} __attribute__ ((packed));
#define RET_CHINFO_SIZE sizeof(struct ret_chinfo)

// 32B + 8B = 40B
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
	uint32_t	net_mask;
	uint32_t	gw_addr;
} __attribute__ ((packed));
#define RET_IPINFO_SIZE sizeof(struct ret_ipinfo)

//369B
struct ret_rfinfo {
	uint8_t 	rf_efuse_revision;
	uint32_t	rf_reg_dump[92]; //RF register dump (368B)
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
	 *	- [5, Reserved]
	 *	- [4:0, GPIO Number]
	 */
	uint8_t 	wakeup_gpio_pin;

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

//56B(8+48) for NRC7292, 54B(8+46) for Others
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
	uint32_t	pmc_wake_src:4;	//PMC wakeup source
	uint32_t	pmc_int:1;	//(1: PMC INT before WFI)
	uint32_t	send_null:1;	//(1: Sent Null Frame before Deep Sleep)
	uint32_t	assert:1;	//(1: Assert right before Deep Sleep)
	uint32_t	reserved:7;
	struct ucode_stats	 stats;	//Ucode Statistics
} __attribute__ ((packed));
#define RET_UCODE_INFO_SIZE sizeof(struct ret_ucodeinfo)

// 1B
struct ret_drvinfo {
	uint8_t		fw_boot_mode:2; //FW boot mode (1B) : 0(XIP), 1 or 2(ROM, it must be distinguished in the code based on the core type)
	uint8_t		do_reset:1;		//FW reboot mode in uCode (1B) : 0(reboot with jump), 1(reboot with reset)
	uint8_t		cqm_off:1;
	uint8_t		brd_rev:4;
} __attribute__ ((packed));
#define RET_DRV_INFO_SIZE sizeof(struct ret_drvinfo)

#if defined (INCLUDE_WOWLAN_PATTERN)
// 65B
struct ret_wowlanptns {
	uint16_t offset:6;
	uint16_t mask_len:4;
	uint16_t pattern_len:6;
	uint8_t mask[7];
	uint8_t pattern[56];
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

#define RET_USERSPACE_SIZE 64
#define RET_RECOVERED_SIZE 1
#if defined (INCLUDE_WOWLAN_PATTERN)
//WoWLAN uses extra 132B for patten match in Ucode
#if defined(NRC7292)
#define RET_RESERVED_SIZE 4
#else
#define RET_RESERVED_SIZE 6
#endif
#else //INCLUDE_WOWLAN_PATTERN
#if defined(NRC7292)
#define RET_RESERVED_SIZE 136
#else
#define RET_RESERVED_SIZE 138
#endif
#endif // //INCLUDE_WOWLAN_PATTERN
#define RET_UCODE_HDR_SIZE 16
#define RET_PMK_SIZE 32
#define WOWLAN_MAX_PATTERNS 2

//Retention Memory (Total_1KB(1024B) - from 0x200B_BC00 to 0x200C_0000)
struct retention_info {
	struct ret_stainfo		sta_info;			//station info (19B)
	struct ret_apinfo		ap_info;			//ap info (46B)
	struct ret_s1ginfo		s1g_info;			//s1g info (12B)
	struct ret_keyinfo		key_info;			//key info (105B)
	struct ret_usrinfo		usr_conf_info;		//user config info (6B)
	struct ret_chinfo		ch_info;			//channel info (5B)
	struct ret_wakeupinfo	wakeup_info;		//wakeup source (3B)
	struct ret_acinfo		ac_info;			//info per AC (40B)
	struct ret_tidinfo		tid_info;			//info per TID ( 68B)
	struct ret_ipinfo		ip_info;			//ip info (12B)
	struct ret_rfinfo		rf_info;			//rf info (369B)
	struct ret_ucodeinfo	ucode_info;			//ucode info (68B)
	struct ret_drvinfo		drv_info;			//driver info (1B)
#if defined (INCLUDE_WOWLAN_PATTERN)
	struct ret_wowlanptns	wowlan_patterns[WOWLAN_MAX_PATTERNS]; //wowlan patterns (65B * 2)
	struct ret_wowlaninfo	wowlan_info;		//wowlan info (2B)
#endif
#if !defined(INCLUDE_RTC_ALWAYS_ON)
	uint32_t				sync_time_ms;		//time for recovery (4B)
#endif
	uint8_t					pmk[RET_PMK_SIZE];	//PMK(PSK) (32B)
	uint32_t				sig_a;				//sig a (4B)
	uint32_t				sig_b;				//sig b (4B)
	uint8_t 				userspace_area[RET_USERSPACE_SIZE];	//user space (64B)
	uint8_t					ps_mode;			//Power saving mode (1B)
	uint64_t				ps_duration;		//Power save duration (8B) (in ms unit)
	uint32_t				wdt_cnt;			//WDT Reset Count (4B)
	bool					recovered;			//recovery status(1B) (true:recovered, false:not recovered)
	bool					sleep_alone;		//sleep without connection(1B) (true:alone false:with AP)
	uint8_t					reserved[RET_RESERVED_SIZE];		//avaiable
	uint8_t     			ucode_hdr[RET_UCODE_HDR_SIZE];		//ucode header (don't touch. should be located at the end)
} __attribute__ ((packed));
#define RET_TOTAL_SIZE sizeof(struct retention_info)

#define MEM_SIZE_RETENTION_INFO 1024

typedef void (*nrc_ps_cb_t)(enum ps_event event,
							enum sys_operation operation);

struct nrc_ps_ops {
	nrc_ps_cb_t cb;
};

/* ============================================================================== */
/* =================== Internal INTERFACE =========================================== */
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

#if !defined(UCODE)
void nrc_ps_init();
void nrc_ps_init_retention_info(bool cold_boot);
void nrc_ps_recovery();
int nrc_ps_config_wakeup_pin(bool check_debounce, int pin_number);
int nrc_ps_config_power_indication_pin(bool enable, int pin_number);
int nrc_ps_config_wakeup_source(uint8_t wakeup_source);
int nrc_ps_get_wakeup_pin(bool *check_debounce, int *pin_number);
int nrc_ps_get_power_indication_pin(bool *enable, int *pin_number);
int nrc_ps_get_wakeup_source(uint8_t *wakeup_source);
int nrc_ps_get_wakeup_reason(uint8_t *wakeup_reason);
int nrc_ps_get_wakeup_count(uint32_t *wakeup_count);
#endif

#endif /*__NRC_PS_API_H__*/
