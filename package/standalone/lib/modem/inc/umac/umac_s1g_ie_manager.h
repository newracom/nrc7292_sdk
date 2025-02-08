#ifndef UMAC_S1G_IE_MANAGER_H
#define UMAC_S1G_IE_MANAGER_H

#if defined(INCLUDE_S1G_HOOK)
#include "system.h"
#include "system_type.h"
#include "umac_ieee80211_types.h"
#include "../util/util_byte_stream.h"
#include "umac_info.h"

#if defined(INCLUDE_AUTH_CONTROL) && defined(INCLUDE_STANDALONE)
typedef struct _auth_ctrl_info {
	uint8_t bssid[6];
	uint8_t ssid[MAX_SSID_LEN];
	uint8_t ssid_len:6;
	uint8_t cac:1;
	uint8_t dac:1;
	uint16_t bi;
	uint8_t slot;
	uint8_t ti_min;
	uint8_t ti_max;
} auth_ctrl_info;
	
typedef struct Node {
	auth_ctrl_info *data;
	struct Node* next;
} Node;
	
typedef struct LinkedList {
	Node* head;
} LinkedList;
	
Node* searchByBSSID(uint8_t* bssid);
void freeNodeByBSSID(uint8_t* bssid);
void freeList(void);
#endif

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

#if defined (INCLUDE_STA_CSA_SUPPORT)
 struct external_csa_info {
	uint32_t		ext_csa_cnt_tsf;	//TSF Limit of ECSA_CNT
	uint8_t			ext_csa_ch_move;	//CH to move by ECSA
	ie_ext_csa		ie_ext_csa_format;	//ECSA IE
};
#endif

typedef struct {
	uint8_t peer_addr[MAC_ADDR_LEN]; //searching key (6B)
	union {
		s1g_operation_information op_info; //S1G Operation IE(4B)
		struct {
			uint32_t bw:4;
			uint32_t tsf:28;
		};
	};
	bool used;
} op_ch_info;
#define OP_CH_INFO_SIZE sizeof(op_ch_info) //11B
#define S1G_OP_INFO_SIZE sizeof (s1g_operation_information)

void umac_init_op_ch_info(uint8_t vif_id);
void umac_deinit_op_ch_info(uint8_t vif_id);
void umac_clear_all_op_ch_info(uint8_t vif_id);
bool umac_add_op_ch_info(uint8_t vif_id, uint8_t *bssid, s1g_operation_information *op_info);
op_ch_info *umac_get_op_ch_info(uint8_t *bssid, int prim_loc);
#if defined (MATCH_OP_BW_PREQ_PRSP)
bool umac_add_op_bw_info(uint8_t vif_id, uint8_t *peer_addr, uint8_t bw);
int umac_get_op_bw_info(uint8_t vif_id, uint8_t *peer_addr);
#endif

uint8_t umac_probe_req_update(int vif_id, uint8_t* ven_ie, uint16_t len);
uint8_t umac_probe_rsp_update(int vif_id, uint8_t* ven_ie, uint16_t len);
uint8_t umac_assoc_req_update(int vif_id, uint8_t* ven_ie, uint16_t len);

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
bool insert_ie_s1g_twt(struct byte_stream *bs, bool is_tx, int8_t vif_id, bool ap_sta, STAINFO *sta_info);
#endif /* defined(INCLUDE_TWT_SUPPORT) */
#if defined(INCLUDE_AUTH_CONTROL)
bool insert_ie_s1g_auth_control(struct byte_stream *bs, bool is_tx, int8_t vif_id);
#endif
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
bool insert_ie_vendor_specific_twt_ie(struct byte_stream *bs, bool is_tx, int8_t vif_id);
bool insert_ie_extension_all(struct byte_stream *bs, bool is_tx, int8_t vif_id);
bool insert_ie_vendor_specific_probe_req(struct byte_stream *bs, bool is_tx, int8_t vif_id);
bool insert_ie_vendor_specific_probe_rsp(struct byte_stream *bs, bool is_tx, int8_t vif_id);
bool insert_ie_vendor_specific_assoc_req(struct byte_stream *bs, bool is_tx, int8_t vif_id);
bool insert_ie_saved_s1g_twt_req (struct byte_stream *bs, bool is_tx, int8_t vif_id);
bool insert_ie_saved_s1g_twt_resp (struct byte_stream *bs, bool is_tx, int8_t vif_id);
#if defined(INCLUDE_H2E_SUPPORT)
bool insert_ie_rsn_extension(struct byte_stream *bs, bool is_tx, int8_t vif_id);
#endif /* defined(INCLUDE_H2E_SUPPORT) */
#if defined(INCLUDE_TSF_SYNC_VENDOR_IE)
bool insert_ie_vendor_specific_tsf_sync(struct byte_stream *bs, bool is_tx, int8_t vif_id);
#endif /* defined(INCLUDE_TSF_SYNC_VENDOR_IE) */
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
bool parse_no_ie_vendor_specific(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
#if defined(INCLUDE_VENDOR_REMOTECMD)
void parse_ie_vendor_remotecmd(int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
static void remotecmd_callback_enqueue(int v);
static int32_t remotecmd_callback(void *param);
#endif // INCLUDE_VENDOR_REMOTECMD
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
bool parse_no_ie_s1g_twt(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
#endif /* defined(INCLUDE_TWT_SUPPORT) */
bool save_ie_s1g_twt_req (struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
bool save_ie_s1g_twt_resp (struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
#if defined(INCLUDE_H2E_SUPPORT)
bool parse_ie_rsn_extension(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
#endif /* defined(INCLUDE_H2E_SUPPORT) */
#if defined(INCLUDE_AUTH_CONTROL) && defined(INCLUDE_STANDALONE)
bool parse_ie_s1g_auth_control(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
#endif
// Functions for parsed information
bool parse_ie_vendor_specific_probe_req(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
bool parse_ie_vendor_specific_probe_rsp(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
bool parse_ie_vendor_specific_assoc_req(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
#if defined (INCLUDE_IBSS)
compatibility_info* get_g_cap_info(void);
ie_ibss_param_set* get_g_ie_ibss_param_set(void);
ie_multiple_bssid* get_g_ie_mbssid(void);
bool parse_ie_ibss_param_set(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
bool insert_ie_ibss_param_set(struct byte_stream *bs, bool is_tx, int8_t vif_id);
bool parse_ie_informal_mbssid(struct _SYS_BUF *buf, int8_t vif_id, ie_general *ie, bool is_tx, bool ap_sta);
bool insert_multiple_bssid_ie(struct byte_stream *bs, bool is_tx, int8_t vif_id);
bool build_informal_mbssid_ie(uint8_t * bssid);
#endif

bool umac_s1g_is_valid_ie_associated_beaconframe(SYS_BUF *buf, int8_t vif_id);
bool umac_s1g_is_valid_ie_associated_proberesp(SYS_BUF *buf, int8_t vif_id);

bool		umac_s1g_has_rsn_ie_existence();

uint8_t		umac_s1g_get_ie_current_channel();
void		umac_s1g_init_ie_params();

uint8_t 	umac_s1g_get_ie_ssid_len();
uint8_t* 	umac_s1g_get_ie_ssid();

void		umac_s1g_set_ie_ssid_len(int8_t vif_id, uint8_t ssid_len);
void 		umac_s1g_set_ie_ssid(int8_t vif_id, uint8_t *ssid, uint8_t ssid_len);

bool	 	umac_s1g_get_ie_vendor_specific_wmm_existence();
uint16_t	umac_s1g_get_frame_beacon_interval();
uint16_t	umac_s1g_get_frame_short_beacon_interval();

void		umac_s1g_update_short_bi(SYS_BUF *buf, int8_t vif_id, bool is_tx, bool short_beacon);

uint32_t	get_assoc_resp_status_code(uint8_t*);

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
