#ifndef UMAC_S1G_CONFIG_H
#define	UMAC_S1G_CONFIG_H

#include "system.h"
#include "umac_ieee80211_types.h"

//FIXME: This will be cleaned up once header dependency is resolved.
#if defined(CUNIT)
#include "umac_info.h"
#endif /* defined(CUINT) */

//////////////////////////////////////////////////////
// S1G Default Configuration
//////////////////////////////////////////////////////
// Common
#define DEF_CFG_COMM_S1G_SUPPORT_CH_WIDTH				1
#define DEF_CFG_COMM_S1G_NDP_PS_POLL_SUPPORT			1	// ( 0: Not, 1: Support )
#define DEF_CFG_COMM_S1G_SUPPORTED_NSS					1 	// (1/2/3/4)
#define DEF_CFG_COMM_S1G_MAX_MCS_1_NSS					1	// (0: QPSK, 1: 64QAM, 2: 256QAM, 3: Not Support)
#define DEF_CFG_COMM_S1G_MAX_MCS_2_NSS					3	// (0: QPSK, 1: 64QAM, 2: 256QAM, 3: Not Support)
#define DEF_CFG_COMM_S1G_MAX_MCS_3_NSS					3	// (0: QPSK, 1: 64QAM, 2: 256QAM, 3: Not Support)
#define DEF_CFG_COMM_S1G_MAX_MCS_4_NSS					3	// (0: QPSK, 1: 64QAM, 2: 256QAM, 3: Not Support)
#define DEF_CFG_COMM_S1G_MIN_MCS_1_NSS					0	// (0: No restrict, 1: MCS0 not recommend, 2: MCS0/1 not recommend)
#define DEF_CFG_COMM_S1G_MIN_MCS_2_NSS					0	// (0: No restrict, 1: MCS0 not recommend, 2: MCS0/1 not recommend)
#define DEF_CFG_COMM_S1G_MIN_MCS_3_NSS					0	// (0: No restrict, 1: MCS0 not recommend, 2: MCS0/1 not recommend)
#define DEF_CFG_COMM_S1G_MIN_MCS_4_NSS					0	// (0: No restrict, 1: MCS0 not recommend, 2: MCS0/1 not recommend)
#define DEF_CFG_COMM_S1G_PRIM_CH_WIDTH					1 	// (0: 2M, 1: 1M)
#define DEF_CFG_COMM_S1G_PVB_ENCODING					0	// (0: Block Bitmap, 1: Single AID, 2: OLB, 3: ADE)
#define DEF_CFG_COMM_S1G_PVB_INVERSE_BITMAP				1	// (0: Not inverse, 1: Inverse)
#define DEF_CFG_COMM_S1G_DUP_1M_SUPPORT					0	// (0: Not, 1: Support)
#define DEF_CFG_COMM_S1G_1M_CTRL_RESP_SUPPORT			0	// (0: Not, 1: Support)
#define DEF_CFG_COMM_ESS_TYPE_NETWORK					1	// (0: Non-ESS, 1: ESS)
#define DEF_CFG_COMM_S1G_NONTIM_SUPPORT					1
#define DEF_CFG_COMM_S1G_PV1_SUPPORT					1
#define DEF_CFG_COMM_S1G_AMPDU							1

#define DEF_CFG_COMM_SHORTGI_OPTION_1M_ACTIVATED		1
#define DEF_CFG_COMM_SHORTGI_OPTION_2M_ACTIVATED		1
#define DEF_CFG_COMM_SHORTGI_OPTION_4M_ACTIVATED		1


// AP
#define DEF_CFG_AP_S1G_STA_TYPE_SUPPORT					0   // ( 0: Both, 1: Sensor, 2: Non-Sensor)
#define DEF_CFG_AP_S1G_NDP_PROBE_REQ_SUPPORT			1	// ( 0: Not, 1: Support )

// STA
#define DEF_CFG_STA_S1G_STA_TYPE_SUPPORT				1	// ( 1: Sensor, 2: Non-Sensor)
#define DEF_CFG_STA_S1G_NDP_PROBE_REQ_SUPPORT			1	// ( 0: Not, 1: Support )
#define DEF_CFG_STA_S1G_NDP_PROBE_REQ_RESP_TYPE			0	// ( 0: PV0, 1: PV1 )


//////////////////////////////////////////////////////
//					Variables						//
//////////////////////////////////////////////////////

typedef struct {
	s1g_capabilities_info s1g_capa;       				/// S1G Capa Info
	supported_s1g_msc_nss_set s1g_supported_mcs;	/// S1G Supported MCS
} s1g_capa_conf;

typedef struct {
	s1g_operation_information s1g_oper;  		/// S1G Oper Info
	basic_s1g_mcs_and_nss_set s1g_basic_mcs; 		/// S1G Basic MCS
} s1g_oper_conf;



#if defined(INCLUDE_S1G_CONFIG)
//////////////////////
// Public Functions //
/////////////////////
// Initialize
//-----------
void		umac_s1g_config_init();
void 		umac_s1g_config_load(int8_t vif_id, MAC_STA_TYPE type);

// S1G Capability
//---------------
//// Supported Channel Width
void 		umac_s1g_config_set_supportedchwidth(uint8_t w);
uint8_t 	umac_s1g_config_get_supportedchwidth();

// S1G Capability - shortGI for 1m,2m,4m,8m,16m
void 		umac_s1g_config_set_shortgi(uint8_t sgi1m, uint8_t sgi2m, uint8_t sgi4m, uint8_t sgi8m, uint8_t sgi16m);
void 		umac_s1g_config_get_shortgi(uint8_t *sgi1m, uint8_t *sgi2m, uint8_t *sgi4m, uint8_t *sgi8m, uint8_t *sgi16m);

// NDP PS Poll Support
void		umac_s1g_config_set_ndppspoll_support(bool ndp_probe_req);
bool		umac_s1g_config_get_ndppspoll_support();

//// Supported MCS and NSS
void 		umac_s1g_config_set_sup_mcs_nss(uint8_t nss, uint8_t max_mcs);
uint8_t		umac_s1g_config_get_sup_mcs_nss(uint8_t nss);

// STA Type Support
void 		umac_s1g_config_set_statype_support(uint8_t sta_type);
uint8_t  	umac_s1g_config_get_statype_support();

// NDP Probe Request Support
void		umac_s1g_config_set_ndpprobereq_support(bool ndp_probe_req);
bool		umac_s1g_config_get_ndpprobereq_support();
void		umac_s1g_config_set_ndpprobereq_resp_type(uint8_t pv0);
uint8_t		umac_s1g_config_get_ndpprobereq_resp_type();

// Duplicated 1MHz Support
void		umac_s1g_config_set_dup1mhz_support(bool dup_1m);
bool		umac_s1g_config_get_dup1mhz_support();

// 1Mhz Control Response Preamble Support
void		umac_s1g_config_set_1mctrlresppreamble_support(bool ctrl_1m);
bool		umac_s1g_config_get_1mctrlresppreamble_support();

// PV1 Frame Support
void 		umac_s1g_config_set_pv1frame_support(bool pv1);
bool 		umac_s1g_config_get_pv1frame_support();

// Non-TIM Support
void 		umac_s1g_config_set_nontimmode_support(bool nontim);
bool 		umac_s1g_config_get_nontimmode_support();

// TWT
void		umac_s1g_config_set_twtresponder_support(bool twt);
bool		umac_s1g_config_get_twtresponder_support();
void		umac_s1g_config_set_twtrequester_support(bool twt);
bool		umac_s1g_config_get_twtrequester_support();

// AMPDU Support
void		umac_s1g_config_set_ampdu_support(bool ampdu);
bool		umac_s1g_config_get_ampdu_support();

// color
void 		umac_s1g_config_set_color(uint8_t color);
uint8_t 	umac_s1g_config_get_color();

// S1G Long
void		umac_s1g_config_set_s1glong_support(uint8_t s1glong);
bool		umac_s1g_config_get_s1glong_support();

// traveling pilot
void		umac_s1g_config_set_tp_support(uint8_t tp);
bool		umac_s1g_config_get_tp_support();

// maximum mpdu length
void		umac_s1g_config_set_maximum_mpdu_length(bool max_mpdu_len);
bool		umac_s1g_config_get_maximum_mpdu_length();

// maximum ampdu length exp
void		umac_s1g_config_set_maximum_ampdu_len_exp(uint8_t max_ampdu_len_exp);
uint8_t		umac_s1g_config_get_maximum_ampdu_len_exp();

// minimum mpdu start spacing
void		umac_s1g_config_set_minimum_mpdu_ssp(uint8_t min_mpdu_ssp);
uint8_t		umac_s1g_config_get_minimum_mpdu_ssp();

// rx s1gmcs map
void		umac_s1g_config_set_rx_s1gmcs_map(uint8_t rx_s1gmcs_map);
uint8_t		umac_s1g_config_get_rx_s1gmcs_map();

//// Capabilities
s1g_capabilities_info* 		umac_s1g_config_get_s1g_capa_info(int8_t vif_id);
supported_s1g_msc_nss_set*	umac_s1g_config_get_s1g_capa_supp_mcs_nss_set();

// S1G Operation
//---------------

// Primary Channel Num
void		umac_s1g_config_set_primchnum(uint8_t ch);
uint8_t		umac_s1g_config_get_primchnum();

// Operation Class
void 		umac_s1g_config_set_operclass(uint8_t oper_class);
uint8_t 	umac_s1g_config_get_operclass();

// Center Frequency
void umac_s1g_config_set_centerfreq(uint8_t v);
uint8_t umac_s1g_config_get_centerfreq();

// Primary Channel Width
void 		umac_s1g_config_set_primchwidth(uint8_t w);
uint8_t		umac_s1g_config_get_primchwidth();

// Primary Channel Location
void 		umac_s1g_config_set_primchlocation(uint8_t w);
uint8_t 	umac_s1g_config_get_primchlocation();

// Operation Channel Width
void 		umac_s1g_config_set_operchwidth(uint8_t w);
uint8_t		umac_s1g_config_get_operchwidth();

// S1G Operation - MCS10 Permitted
void umac_s1g_config_set_mcs10permitted(uint8_t w);
uint8_t umac_s1g_config_get_mcs10permitted();

// Basic MCS and NSS Set
void 		umac_s1g_config_set_basicmcs_nss_set(uint8_t nss, uint8_t max_mcs, uint8_t min_mcs);
uint8_t		umac_s1g_config_get_basicmcs_nss_set_max(uint8_t nss);
uint8_t		umac_s1g_config_get_basicmcs_nss_set_min(uint8_t nss);

//// Operation
s1g_operation_information*	umac_s1g_config_get_s1g_oper_info();
basic_s1g_mcs_and_nss_set*	umac_s1g_config_get_s1g_oper_basic_mcs_nss_set();

// ETC
// ------------------

void		umac_s1g_config_set_s1gtim_block_control(uint8_t bc);

void		umac_s1g_config_set_block_control(uint8_t vif_id, uint8_t encoding, uint8_t inverse);
uint8_t		umac_s1g_config_get_block_control();

void		umac_s1g_config_set_ess_type_network(bool enable);
bool		umac_s1g_config_get_ess_type_network();

bool		umac_s1g_config_get_debug_mode();
#else /* defined(INCLUDE_S1G_CONFIG) */
static inline void umac_s1g_config_init() {}
static inline void umac_s1g_config_load(int8_t vif_id, MAC_STA_TYPE type){}
static inline void umac_s1g_config_set_supportedchwidth(uint8_t w){}
static inline uint8_t umac_s1g_config_get_supportedchwidth(){return 0;}
static inline void umac_s1g_config_set_shortgi(uint8_t sgi1m, uint8_t sgi2m, uint8_t sgi4m, uint8_t sgi8m, uint8_t sgi16m){}
static inline void umac_s1g_config_get_shortgi(uint8_t *sgi1m, uint8_t *sgi2m, uint8_t *sgi4m, uint8_t *sgi8m, uint8_t *sgi16m){}
static inline void umac_s1g_config_set_ndppspoll_support(bool ndp_probe_req){}
static inline bool umac_s1g_config_get_ndppspoll_support(){return false;}
static inline void umac_s1g_config_set_sup_mcs_nss(uint8_t nss, uint8_t max_mcs){}
static inline uint8_t umac_s1g_config_get_sup_mcs_nss(uint8_t nss){return 0;}
static inline void umac_s1g_config_set_statype_support(uint8_t sta_type){}
static inline uint8_t umac_s1g_config_get_statype_support(){return 0;}
static inline void umac_s1g_config_set_ndpprobereq_support(bool ndp_probe_req){}
static inline bool umac_s1g_config_get_ndpprobereq_support(){return false;}
static inline void	umac_s1g_config_set_ndpprobereq_resp_type(uint8_t pv0){}
static inline uint8_t umac_s1g_config_get_ndpprobereq_resp_type(){return 0;}
static inline void umac_s1g_config_set_dup1mhz_support(bool dup_1m){}
static inline bool	umac_s1g_config_get_dup1mhz_support(){return false;}
static inline void umac_s1g_config_set_1mctrlresppreamble_support(bool ctrl_1m){}
static inline bool umac_s1g_config_get_1mctrlresppreamble_support(){return false;}
static inline void umac_s1g_config_set_pv1frame_support(bool pv1){}
static inline bool umac_s1g_config_get_pv1frame_support() {return false;}
static inline void umac_s1g_config_set_nontimmode_support(bool nontim){}
static inline bool umac_s1g_config_get_nontimmode_support(){return false;}
static inline void umac_s1g_config_set_twtresponder_support(bool twt){}
static inline bool umac_s1g_config_get_twtresponder_support(){return false;}
static inline void umac_s1g_config_set_twtrequester_support(bool twt){}
static inline bool umac_s1g_config_get_twtrequester_support(){return false;}
static inline void umac_s1g_config_set_ampdu_support(bool ampdu){}
static inline bool umac_s1g_config_get_ampdu_support(){return false;}
static inline void umac_s1g_config_set_color(uint8_t color){}
static inline uint8_t umac_s1g_config_get_color(){return 0;}
static inline void umac_s1g_config_set_s1glong_support(uint8_t s1glong){}
static inline bool umac_s1g_config_get_s1glong_support(){return false;}
static inline void umac_s1g_config_set_tp_support(uint8_t tp){}
static inline bool umac_s1g_config_get_tp_support(){return false;}
static inline void umac_s1g_config_set_maximum_mpdu_length(bool max_mpdu_len){}
static inline bool umac_s1g_config_get_maximum_mpdu_length(){return false;}
static inline void umac_s1g_config_set_maximum_ampdu_len_exp(uint8_t max_ampdu_len_exp){}
static inline uint8_t umac_s1g_config_get_maximum_ampdu_len_exp(){return 0;}
static inline void umac_s1g_config_set_minimum_mpdu_ssp(uint8_t min_mpdu_ssp){}
static inline uint8_t umac_s1g_config_get_minimum_mpdu_ssp(){return 0;}
static inline void umac_s1g_config_set_rx_s1gmcs_map(uint8_t rx_s1gmcs_map){}
static inline uint8_t umac_s1g_config_get_rx_s1gmcs_map(){return 0;}
static inline s1g_capabilities_info* umac_s1g_config_get_s1g_capa_info(int8_t vif_id) {return NULL;};
static inline supported_s1g_msc_nss_set* umac_s1g_config_get_s1g_capa_supp_mcs_nss_set(){return NULL;};
static inline void umac_s1g_config_set_primchnum(uint8_t ch){}
static inline uint8_t umac_s1g_config_get_primchnum(){return 0;}
static inline void umac_s1g_config_set_operclass(uint8_t oper_class){}
static inline uint8_t umac_s1g_config_get_operclass(){return 0;}
static inline void umac_s1g_config_set_centerfreq(uint8_t v){}
static inline uint8_t umac_s1g_config_get_centerfreq(){return 0;}
static inline void umac_s1g_config_set_primchwidth(uint8_t w){}
static inline uint8_t umac_s1g_config_get_primchwidth(){return 0;}
static inline void umac_s1g_config_set_primchlocation(uint8_t w){}
static inline uint8_t umac_s1g_config_get_primchlocation(){return 0;}
static inline void umac_s1g_config_set_operchwidth(uint8_t w){};
static inline uint8_t umac_s1g_config_get_operchwidth(){return 0;};
static inline void umac_s1g_config_set_mcs10permitted(uint8_t w){}
static inline uint8_t umac_s1g_config_get_mcs10permitted(){return 0;}
static inline void umac_s1g_config_set_basicmcs_nss_set(uint8_t nss, uint8_t max_mcs, uint8_t min_mcs){}
static inline uint8_t umac_s1g_config_get_basicmcs_nss_set_max(uint8_t nss){return 0;}
static inline uint8_t umac_s1g_config_get_basicmcs_nss_set_min(uint8_t nss){return 0;}
static inline s1g_operation_information*	umac_s1g_config_get_s1g_oper_info() {return NULL;}
static inline basic_s1g_mcs_and_nss_set*	umac_s1g_config_get_s1g_oper_basic_mcs_nss_set() {return NULL;}
static inline void umac_s1g_config_set_s1gtim_block_control(uint8_t bc){}
static inline void umac_s1g_config_set_block_control(uint8_t encoding, uint8_t inverse) {}
static inline uint8_t umac_s1g_config_get_block_control() {return 0;}
static inline void umac_s1g_config_set_ess_type_network(bool enable){}
static inline bool umac_s1g_config_get_ess_type_network(){return false;}
static inline bool umac_s1g_config_get_debug_mode() {return false;}
#endif /* defined(INCLUDE_S1G_CONFIG) */
#endif /* UMAC_S1G_CONFIG_H */