#ifndef UMAC_S1G_IE_MANAGER_H
#define UMAC_S1G_IE_MANAGER_H

#if defined(INCLUDE_S1G_HOOK)
#include "system.h"
#include "system_type.h"
#include "umac_ieee80211_types.h"
#include "../util/util_byte_stream.h"

#if defined(UMAC_CONFIG_WTS)
#include "umac_ie_config.h"
#endif /* defiend(UMAC_CONFIG_WTS) */

#define	S1G_VERIFY_BUFFER(_cur, _add) do {			\
	int _left = LMAC_CONFIG_BUFFER_SIZE - _cur;		\
	if ((_left) <= 0 || (_left) < (_add)) { 		\
		ASSERT(0);									\
		return false;								\
	}												\
} while (0)

typedef enum {
	TWT_RESPONDER = 0,
	TWT_REQUESTER,
	TWT_REQUEST_MAX
} S1G_TWT_REQUEST_TYPE;

typedef enum {
	TWT_REQUEST = 0,
	TWT_SUGGEST,
	TWT_DEMAND,
	TWT_GROUPING,
	TWT_ACCEPT,
	TWT_ALTERNATE,
	TWT_DICTATE,
	TWT_REJECT,
	TWT_MAX
} S1G_TWT_SETUP_COMMAND;

#if defined (INCLUDE_STA_CSA_SUPPORT)
 struct external_csa_info {
	uint32_t		ext_csa_cnt_tsf;	//TSF Limit of ECSA_CNT
	uint8_t			ext_csa_ch_move;	//CH to move by ECSA
	ie_ext_csa		ie_ext_csa_format;	//ECSA IE
};
#endif

//////////////////////
// Public Functions //
//////////////////////

struct byte_stream;

// IE Managment Functions
bool insert_ie_s1g_beacon_compatibility(struct byte_stream *bs, bool is_tx, int8_t vif_id, uint16_t capa);
bool insert_ie_s1g_tim(struct byte_stream *bs, bool is_tx, int8_t vif_id);
bool insert_ie_csa(struct byte_stream *bs, bool is_tx, int8_t vif_id);
bool insert_ie_s1g_aid_request(struct byte_stream *bs, bool is_tx, int8_t vif_id);
bool insert_ie_s1g_shortbeaconinterval(struct byte_stream *bs, bool is_tx, int8_t vif_id);
bool insert_ie_s1g_capabilities(struct byte_stream *bs, bool is_tx, int8_t vif_id);
bool insert_ie_s1g_operation(struct byte_stream *bs, bool is_tx, int8_t vif_id);
bool insert_ie_s1g_aid_response(struct byte_stream *bs, bool is_tx, int8_t vif_id, uint16_t aid);
bool insert_ie_timeout_interval(struct byte_stream *bs, bool is_tx, int8_t vif_id);
bool insert_ie_bss_max_idle_period(struct byte_stream *bs, bool is_tx, int8_t vif_id, bool ap_sta);
bool insert_ie_s1g_header_compression(struct byte_stream *bs, bool is_tx, int8_t vif_id, uint8_t* addr);
bool insert_ie_edca_parameter_set(struct byte_stream *bs);
#if defined(INCLUDE_TWT_SUPPORT)
bool insert_ie_s1g_twt(struct byte_stream *bs, bool is_tx, int8_t vif_id, bool ap_sta);
#endif /* defined(INCLUDE_TWT_SUPPORT) */

bool insert_ie_supported_rates(struct byte_stream *bs);
bool insert_ie_ssid(struct byte_stream *bs, bool is_tx, int8_t vif_id, bool short_beacon);
bool insert_ie_legacy_supported_rates(struct byte_stream *bs, bool is_tx, int8_t vif_id);
bool insert_ie_legacy_ds_param_set(struct byte_stream *bs, bool is_tx, int8_t vif_id, bool short_beacon);
bool insert_ie_legacy_tim(struct byte_stream *bs, bool is_tx, int8_t vif_id);
bool insert_ie_legacy_rsn(struct byte_stream *bs, bool is_tx, int8_t vif_id);
bool insert_ie_legacy_ht_capability(struct byte_stream *bs, bool is_tx, int8_t vif_id);
bool insert_ie_legacy_ht_operation(struct byte_stream *bs, bool is_tx, int8_t vif_id);
bool insert_ie_legacy_extended_capabilities(struct byte_stream *bs, bool is_tx, int8_t vif_id);
bool insert_ie_mesh_config(struct byte_stream *bs, bool is_tx, int8_t vif_id);
bool insert_ie_mesh_id(struct byte_stream *bs, bool is_tx, int8_t vif_id);
bool insert_ie_mesh_link_metric_report(struct byte_stream *bs, bool is_tx, int8_t vif_id);
bool insert_ie_mesh_peering_management(struct byte_stream *bs, bool is_tx, int8_t vif_id);
bool insert_ie_mesh_ch_switch_param(struct byte_stream *bs, bool is_tx, int8_t vif_id);
bool insert_ie_mesh_awake_window(struct byte_stream *bs, bool is_tx, int8_t vif_id);
bool insert_ie_vendor_specific_wmm(struct byte_stream *bs, bool is_tx, int8_t vif_id, OUI_SUBTYPE oui_stype);
bool insert_ie_vendor_specific_all_others(struct byte_stream *bs, bool is_tx, int8_t vif_id);
bool insert_ie_extension_all(struct byte_stream *bs, bool is_tx, int8_t vif_id);
#if defined(INCLUDE_H2E_SUPPORT)
bool insert_ie_rsn_extension(struct byte_stream *bs, bool is_tx, int8_t vif_id);
#endif /* defined(INCLUDE_H2E_SUPPORT) */
bool umac_s1g_beacon_is_short(SYS_BUF* buf, bool is_tx);
uint8_t* umac_s1g_beacon_find_ie(SYS_BUF *buf, int eid, bool is_tx);
uint8_t* umac_s1g_short_beacon_find_ie(SYS_BUF *buf, int eid, bool is_tx);

bool parse_ie_ssid(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
bool parse_ie_rsn(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
bool parse_ie_timeout_interval(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
bool parse_ie_bss_max_idle_period(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
bool parse_ie_s1g_capabilities(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
bool parse_ie_s1g_operation(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
bool parse_ie_s1g_aid_response(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
bool parse_ie_s1g_beacon_compatibility(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
bool parse_ie_s1g_header_compression(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
bool parse_ie_s1g_shortbeaconinterval(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
bool parse_ie_edca_parameter_set(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
bool parse_ie_vendor_specific(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
bool parse_ie_extension(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
bool parse_ie_mesh_config(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
bool parse_ie_mesh_id(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
bool parse_ie_mesh_link_metric_report(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
bool parse_ie_mesh_peering_management(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
bool parse_ie_mesh_ch_switch_param(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
bool parse_ie_mesh_awake_window(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
bool parse_ie_ext_csa(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
#if defined(INCLUDE_TWT_SUPPORT)
bool parse_ie_s1g_twt(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
#endif /* defined(INCLUDE_TWT_SUPPORT) */
#if defined(INCLUDE_H2E_SUPPORT)
bool parse_ie_rsn_extension(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
#endif /* defined(INCLUDE_H2E_SUPPORT) */
// Functions for parsed information
bool umac_s1g_is_valid_ie_associated_beaconframe(SYS_BUF *buf, int8_t vif_id);

bool		umac_s1g_has_rsn_ie_existence();

uint8_t		umac_s1g_get_ie_current_channel();
void		umac_s1g_init_ie_params();

uint8_t 	umac_s1g_get_ie_ssid_len();
uint8_t* 	umac_s1g_get_ie_ssid();

void		umac_s1g_set_ie_ssid_len(int8_t vif_id, uint8_t ssid_len);
void 		umac_s1g_set_ie_ssid(int8_t vif_id, uint8_t *ssid, uint8_t ssid_len);

bool	 	umac_s1g_get_ie_vendor_specific_wmm_existence();
uint16_t	umac_s1g_get_frame_beacon_interval();

void		umac_s1g_update_short_bi(SYS_BUF *buf, int8_t vif_id, bool is_tx, bool short_beacon);

uint32_t	get_assoc_resp_status_code(uint8_t*);

#if defined(SOFT_AP_BSS_MAX_IDLE)
bool umac_s1g_set_softap_ie_bss_max_idle(int period);
#endif /* defined(SOFT_AP_BSS_MAX_IDLE) */

// IE Structure Verification
bool validate_frame_ie_structure_mul(struct byte_stream *bs, struct _SYS_BUF *buf, bool is_tx, uint16_t offset, const char *msg);

void update_s1gbuffer_diff(struct _SYS_BUF *new_buf, struct _SYS_BUF *ori_buf);

#if defined (INCLUDE_STA_CSA_SUPPORT)
struct external_csa_info* get_ext_csa_info(void);
 void init_ext_csa_info(void);
 #endif

#else
static inline uint8_t* umac_s1g_beacon_find_ie(SYS_BUF *b, int e, bool i)
{
	return NULL;
}

static inline void umac_s1g_set_ie_ssid_len(int8_t v, uint8_t l) {}
static inline void umac_s1g_set_ie_ssid(int8_t v, uint8_t *s, uint8_t l) {}
static inline bool umac_s1g_beacon_is_short(SYS_BUF* buf, bool is_tx)
{
	return false;
}
#endif /* defined(INCLUDE_S1G_HOOK) */
uint16_t umac_convert_usf(int interval);
#endif /* UMAC_S1G_IE_MANAGER_H */
