#ifndef __LMAC_TWT_COMMON_H__
#define __LMAC_TWT_COMMON_H__

#include "util_list.h"
#include "protocol_11ah.h"

#define TWT_DEBUG             0    // Debug log
#define MAX_TWT_AGREEMENT     1    // TWT Max Agreement per STA (Spec: 8)
#define TWT_AP_TARGET_SUPPORT 0    // AP Target support TWT
#define TWT_OUI_TYPE          0xDF // TWT Vendor IE OUI type
#if defined(INCLUDE_TWT_NDP_PAGING)
#define TWT_NDP_PAGING        1    // NDP Paging support
#else
#define TWT_NDP_PAGING        0    // NDP Paging support
#endif /* defined(INCLUDE_TWT_NDP_PAGING) */

typedef enum {
	TWT_MODE_NONE = 0,
	TWT_MODE_NULL,
	TWT_MODE_PSPOLL,
	TWT_MODE_MAX,
} TWT_MODE;

typedef enum {
	TWT_VENDOR_IE_SP = 0,
	TWT_VENDOR_IE_TSF,
	TWT_VENDOR_IE_MAX
} TWT_VENDOR_IE_CATEGORY;

typedef enum {
	TWT_FLOW_ANNOUNCED = 0,
	TWT_FLOW_UNANNOUNCED
} TWT_FLOW_TYPE;

// EID 216 : TWT
typedef struct {
	uint32_t		pid: 9;
	uint32_t		max_ndp_paging_period: 8;
	uint32_t		partial_tsf_offset: 4;
	uint32_t		action: 3;
	uint32_t		min_sleep_duration: 6;
	uint32_t		reserved: 2;
} __attribute__((packed)) twt_ndp_paging;

typedef struct {
	uint8_t		twt_group_id: 7;
	uint8_t		zero_offset_present: 1;
	uint8_t		zero_offset[6];
	uint16_t		twt_unit: 4;
	uint16_t		twt_offset: 12;
} __attribute__((packed)) twt_group_assignment;

typedef struct {
	uint16_t	twt_request: 1;
	uint16_t	twt_setup_command: 3;
	uint16_t	reserved: 1;
	uint16_t	implicit: 1;
	uint16_t	flow_type: 1;
	uint16_t	twt_flow_id: 3;
	uint16_t	twt_wake_interval_exp: 5;
	uint16_t	twt_protection: 1;
} __attribute__((packed)) twt_request_type;

typedef struct {
	uint8_t		ndp_paging_indicator: 1;
	uint8_t		responder_pm_mode: 1;
	uint8_t		reserved: 6;
} __attribute__((packed)) twt_control;

typedef struct {
	uint8_t     eid;
	uint8_t     length;
	uint8_t     oui[3];
	uint8_t     oui_type;
	union {
		uint64_t    service_period_us;
		uint64_t    tsf;
	} u;
} __attribute__((packed)) ie_vendor_twt;

typedef struct {
	uint8_t 	eid;
	uint8_t 	length;
	twt_control		control;
	twt_request_type		request_type;
	uint64_t	target_wake_time;
	uint8_t		nom_min_twt_wake_duration;
	uint16_t 	twt_wake_int_mantissa;
	uint8_t		twt_channel;
	twt_ndp_paging	ndp_paging;
} __attribute__((packed)) ie_twt;

typedef struct {
	uint8_t 	eid;
	uint8_t 	length;
	twt_control		control;
	twt_request_type		request_type;
	twt_group_assignment	twt_group;
	uint8_t		nom_min_twt_wake_duration;
	uint16_t 	twt_wake_int_mantissa;
	uint8_t		twt_channel;
	twt_ndp_paging	ndp_paging;
} __attribute__((packed)) ie_twt_group;

typedef struct _TWT_Setup {
	uint8_t category				: 8;
	uint8_t action					: 8;
	uint8_t dial_token				: 8;
	ie_twt twt_ie;
	ie_vendor_twt twt_vendor_ie;
} __attribute__((packed)) TWT_Setup ;

typedef struct _TWT_Setup_Group {
	uint8_t category				: 8;
	uint8_t action					: 8;
	uint8_t dial_token				: 8;
	ie_twt_group twt_ie;
} __attribute__((packed)) TWT_Setup_Group;

typedef struct _TWT_Teardown {
	uint8_t category				: 8;
	uint8_t action					: 8;
	uint8_t flow					: 3;
	uint8_t reserved				: 5;
} __attribute__((packed)) TWT_Teardown;

typedef struct _TWT_Info {
	uint8_t category				: 8;
	uint8_t action					: 8;
	uint8_t	twt_flow_id				: 3;
	uint8_t res_requested			: 1;
	uint8_t next_twt_req			: 1;
	uint8_t next_twt_subfield_size	: 2;
	uint8_t reserved				: 1;
	uint64_t next_twt;
} __attribute__((packed)) TWT_Info;

/* TWT Agreement in Retention */
typedef struct _ret_twt {
#if defined(INCLUDE_TWT_NDP_PAGING)
	uint8_t ptsf_offset             : 4;
	uint8_t reserved                : 3;
	uint8_t ndp_paging              : 1;
#else
	uint8_t flow_type               : 1; //TODO: place it outside with multi-agreement support
	uint8_t suspend_tx              : 1; //TODO: place it outside with multi-agreement support
	uint8_t reserved                : 6;
#endif /* defined(INCLUDE_TWT_NDP_PAGING) */
	uint8_t wake_interval_exp;
	uint16_t wake_interval_mantissa;
	uint32_t service_period_ms;
	uint64_t start_time_us;
} __attribute__ ((packed)) ret_twt;

void lmac_process_twt(void);
bool lmac_twt_action_process(uint8_t vif_id, uint8_t *addr, void *action_ptr, uint16_t len, bool is_tx);

bool lmac_twt_insert_setup_ie(ie_twt* ie, uint8_t vif_id, uint8_t *destaddr, uint8_t flow_id, bool group);
bool lmac_twt_insert_vendor_ie(void* ie, uint8_t vif_id, uint8_t category);
bool lmac_twt_parse_setup_ie(ie_twt* ie, uint8_t vif_id, uint8_t *srcaddr);
bool lmac_twt_parse_vendor_ie(void* ie, uint8_t vif_id, uint8_t category);
bool lmac_twt_parse_ndp_paging(uint8_t vif_id, uint16_t aid, NdpPaging1M *paging);

void lmac_twt_init(uint8_t vif_id);
void lmac_twt_start(uint8_t vif_id);
void lmac_twt_stop(uint8_t vif_id);
void lmac_twt_disable(uint8_t vif_id);
void lmac_twt_teardown(uint8_t vif_id, uint8_t *addr, uint8_t flow_id, bool is_tx);
void lmac_twt_recovery(uint8_t vif_id, bool fast_connect);
void lmac_twt_send_setup(uint8_t vif_id, uint8_t *destaddr, int flow_id);
void lmac_twt_send_teardown(uint8_t vif_id, uint8_t *destaddr, int flow_id);
void lmac_twt_send_info(uint8_t vif_id, uint8_t *destaddr, int flow_id, uint64_t next_twt);

bool lmac_twt_get_enable();
bool lmac_twt_get_active(void);
bool lmac_twt_get_service(void);
bool lmac_twt_get_agm_exist(uint8_t *addr, uint8_t flow_id);
bool lmac_twt_set_dynamic_ps_expired(bool expired);
void lmac_twt_set_tsf_sync(bool sync);
void lmac_twt_set_initial_margin(uint64_t initial_margin_us, bool force);
void lmac_twt_set_wake_interval(uint64_t wake_interval_us);
uint64_t lmac_twt_get_wake_interval(uint8_t flow_id);
uint64_t lmac_twt_get_remaining_time(uint8_t vif_id);
void lmac_twt_set_suspend_tx(bool enable);
bool lmac_twt_get_suspend_tx(void);
bool lmac_twt_is_requestor(void);
bool lmac_twt_is_responder(void);
bool lmac_twt_get_responder_support(void);
void lmac_twt_set_responder_support(bool support);
void lmac_twt_update_start_margin(bool cold_boot);
void lmac_twt_set_user_time(uint32_t user_time_ms);
void lmac_twt_set_flow_type(TWT_FLOW_TYPE type);
void lmac_twt_set_authorized(bool authorized);

#if (TWT_DEBUG != 0)
uint64_t lmac_twt_get_current_start_time(uint8_t vif_id);
uint64_t lmac_twt_get_current_end_time(uint8_t vif_id);
uint64_t lmac_twt_get_pm0_sent_time(uint8_t vif_id);
uint64_t lmac_twt_get_pm0_ack_time(uint8_t vif_id);
#endif

#endif // __LMAC_TWT_COMMON_H__
