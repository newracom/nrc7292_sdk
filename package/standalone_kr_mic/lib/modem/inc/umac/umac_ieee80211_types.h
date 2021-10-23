#ifndef UMAC_IEEE80211_TYPES_H
#define UMAC_IEEE80211_TYPES_H

/****************************************************************************
  IEEE 802.11 definitions
*****************************************************************************/
#define WLAN_FC_PV0					0
#define WLAN_FC_PV1					1

#define WLAN_FC_TYPE_MGMT           0
#define WLAN_FC_TYPE_CTRL           1
#define WLAN_FC_TYPE_DATA           2
#define WLAN_FC_TYPE_EXTN           3
#define WLAN_FC_TYPE_1QD0			4
#define WLAN_FC_TYPE_1MGT			5
#define WLAN_FC_TYPE_1CTL			6
#define WLAN_FC_TYPE_1QD3			7
#define WLAN_FC_TYPE_NDP			8

/* management */
#define WLAN_FC_STYPE_ASSOC_REQ     0
#define WLAN_FC_STYPE_ASSOC_RESP    1
#define WLAN_FC_STYPE_REASSOC_REQ   2
#define WLAN_FC_STYPE_REASSOC_RESP  3
#define WLAN_FC_STYPE_PROBE_REQ     4
#define WLAN_FC_STYPE_PROBE_RESP    5
#define WLAN_FC_STYPE_TIME_AD	    6
#define WLAN_FC_STYPE_RESERVED	    7
#define WLAN_FC_STYPE_BEACON        8
#define WLAN_FC_STYPE_ATIM          9
#define WLAN_FC_STYPE_DISASSOC      10
#define WLAN_FC_STYPE_AUTH          11
#define WLAN_FC_STYPE_DEAUTH        12
#define WLAN_FC_STYPE_ACTION        13

/* NDP subtype */
#define WLAN_FC_STYPE_NDP_PSPOLL		1
#define WLAN_FC_STYPE_NDP_ACK			2
#define WLAN_FC_STYPE_NDP_PSPOLL_ACK	3
#define WLAN_FC_STYPE_NDP_PROBE_REQ		7

/* Extension */
#define WLAN_FC_STYPE_DMG_BEACON    0
#define WLAN_FC_STYPE_S1G_BEACON    1

/* PV1 */
#define WLAN_FC_PV1_TYPE_QOSDATA0    0
#define WLAN_FC_PV1_TYPE_MGMT        1
#define WLAN_FC_PV1_TYPE_CTRL        2
#define WLAN_FC_PV1_TYPE_QOSDATA3    3

/* PV1 management */
#define WLAN_FC_PV1_STYPE_ACTION				0
#define WLAN_FC_PV1_STYPE_ACTION_NOACK			1
#define WLAN_FC_PV1_STYPE_SHORT_PROBE_RSP		2
#define WLAN_FC_PV1_STYPE_RESOURCE_ALLOC		3

/* Information Element IDs */
#define WLAN_EID_SSID 							0	// 2 TO 34
#define WLAN_EID_SUPPORTED_RATES 				1	// 3 TO 10
#define WLAN_EID_FH_PARAMETER_SET 				2	//  7
#define WLAN_EID_DS_PARAMETER_SET 				3	//  3
#define WLAN_EID_CF_PARAMETER_SET 				4	//  8
#define WLAN_EID_TIM 							5	//  6 TO 256
#define WLAN_EID_IBSS_PARAMETER_SET  			6	//  4
#define WLAN_EID_COUNTRY 						7	//  8 TO 256
#define WLAN_EID_HOPPING_PATTERN_PARAMETERS		8	//  4
#define WLAN_EID_HOPPING_PATTERN_TABLE 			9	//  6 TO 256
#define WLAN_EID_REQUEST 						10	//  2 TO 256
#define WLAN_EID_BSS_LOAD 						11	//  7
#define WLAN_EID_EDCA_PARAMETER_SET 			12	//  20
#define WLAN_EID_TSPEC 							13	//  57
#define WLAN_EID_TCLAS 							14	//  2 TO 257
#define WLAN_EID_SCHEDULE 						15	//  16
#define WLAN_EID_CHALLENGE_TEXT 				16	//  3 TO 255
// RESERVED 17-31
#define WLAN_EID_POWER_CONSTRAINT 				32	//  3
#define WLAN_EID_POWER_CAPABILITY 				33	//  4
#define WLAN_EID_TPC_REQUEST 					34	//  2
#define WLAN_EID_TPC_REPORT 					35	//  4
#define WLAN_EID_SUPPORTED_CHANNELS 			36	//  4 TO 256
#define WLAN_EID_CHANNEL_SWITCH_ANNOUNCEMENT 	37	//  5
#define WLAN_EID_MEASUREMENT_REQUEST 			38	//  5 TO 16
#define WLAN_EID_MEASUREMENT_REPORT 			39	//  5 TO 24
#define WLAN_EID_QUIET 							40	//  8
#define WLAN_EID_IBSS_DFS 						41	//  10 TO 255
#define WLAN_EID_ERP_INFORMATION				42	//  3
#define WLAN_EID_TS_DELAY				 		43	//  6
#define WLAN_EID_TCLAS_PROCESSING				44	//  3
#define WLAN_EID_HT_CAPABILITY                  45
#define WLAN_EID_QOS_CAPABILITY					46 	//  3
// RESERVED 47
#define WLAN_EID_RSN				 			48	//  36 TO 256
// RESERVED 49
#define WLAN_EID_EXTENDED_SUPPORTED_RATES		50	//  3 TO 257
#define WLAN_EID_AP_CHANNEL_REPORT              51  //  3 TO 257
// Reserved 51-55
#define WLAN_EID_MOBILITY_DOMAIN                54  //  5
#define WLAN_EID_TIMEOUT_INTERVAL 				56	//  7
#define WLAN_EID_SUPPORTED_OPERATING_CLASSES    59  //  2 TO 253
#define WLAN_EID_EXTENDED_CHANNEL_SWITCH_ANNOUNCEMENT   60
#define WLAN_EID_HT_OPERATION                   61
#define WLAN_EID_BSS_AVERAGE_ACCESS_DELAY       63  //  3
#define WLAN_EID_ANTENNA                        64  //  3
#define WLAN_EID_MEASUREMENT_PILOT_TRANSMISSION 66  //  3 TO 255
#define WLAN_EID_BSS_AVAILABLE_ADMISSION_CAPACITY   67  // 4 TO 28
#define WLAN_EID_BSS_AC_ACCESS_DELAY            68  //  6
#define WLAN_EID_TIME_ADVERTISEMENT             69  //  3 TO 18
// Reserved 57-75
#define WLAN_EID_RM_ENABLED_CAPABILITIES        70  //  7
#define WLAN_EID_MULTIPLE_BSSID                 71  //  3 TO 257
#define WLAN_EID_MANAGEMENT_MIC					76	//  18
#define WLAN_EID_EVENT_REQUEST					78	//	5 TO 257
#define WLAN_EID_EVENT_REPORT					79	//	5 TO 257
#define WLAN_EID_DIAGNOSTIC_REQUEST				80	//	6 TO 257
#define WLAN_EID_DIAGNOSTIC_REPORT				81	//	5 TO 257
#define WLAN_EID_LOCATION_PARAMETERS			82	//	2 TO 257
#define WLAN_EID_NONTRANSMITTED_BSSID_CAPABILITIES	83	// 4
#define WLAN_EID_SSID_LIST						84	//	2 TO 257
#define WLAN_MULTIPLE_BSSID_INDEX				85	//	3 TO 5
#define WLAN_EID_FMS_DESCRIPTOR					86	//	3 TO 257
#define WLAN_EID_QOS_TRAFFIC_CAPABILITY         89  //  3 TO 5
// Reserved 77-126
#define WLAN_EID_BSS_MAX_IDLE_PERIOD            90  //  3
#define WLAN_EID_CHANNEL_USAGE                  97  //  3 TO 257
#define WLAN_EID_TIME_ZONE                      98  //  3 TO 257
#define WLAN_EID_INTERWORKING                   107 //  3,5,9,11
#define WLAN_EID_ADVERTISEMENT_PROTOCOL         108 //  variable
#define WLAN_EID_ROAMING_CONSORTIUM             111 //  variable
#define WLAN_EID_EMERGENCY_ALERT_IDENTIFIER     112 //  10
#define WLAN_EID_MESH_CONFIG					113
#define WLAN_EID_MESH_ID						114
#define WLAN_EID_MESH_LINK_METRIC_REPORT		115
#define WLAN_EID_MESH_PEERING_MANAGEMENT		117
#define WLAN_EID_MESH_CH_SWITCH_PARAM			118
#define WLAN_EID_MESH_AWAKE_WINDOW				119
#define WLAN_EID_EXTENDED_CAPABILITIES 			127	//  2 TO 257
// RESERVED 128-206
#define WLAN_EID_VHT_CAPABILITY					191
#define WLAN_EID_VHT_OPERATION                  192
#define WLAN_EID_S1G_OPENLOOP_LINKMARGIN_INDEX	207	//	3
#define WLAN_EID_RPS 							208 //	5 TO
#define WLAN_EID_PAGE_SLICE						209	//	6 TO 10
#define WLAN_EID_AID_REQUEST					210	//	3 TO 18
#define WLAN_EID_AID_RESPONSE					211	//	7
#define WLAN_EID_S1G_SECTOR_OPERATION			212	//	4 TO 16
#define WLAN_EID_S1G_BEACON_COMPATIBILITY       213	//	10
#define WLAN_EID_SHORT_BEACON_INTERVAL          214 //	4
#define WLAN_EID_CHANGE_SEQUENCE                215 //	3
#define WLAN_EID_TWT                            216 //	9 TO 30
#define WLAN_EID_S1G_CAPABILITIES 				217	//  2 TO 257
#define WLAN_EID_SUBCHANNEL_SELECTIVE_TRANSMISSION    220 // 16
#define WLAN_EID_WPS							220
#define WLAN_EID_AUTHENTICATION_CONTROL         222 //  4 TO 5
#define WLAN_EID_TSF_TIMER_ACCURACY             223 //  3
#define WLAN_EID_S1G_RELAY                      224 //
#define WLAN_EID_RECHABLE_ADDRESS               225 //
#define WLAN_EID_S1G_RELAY_DISCOVERY            226
#define WLAN_EID_AID_ANNOUNCEMENT               228
#define WLAN_EID_SHORT_PROBE_RESPONSE_OPTION    229
#define WLAN_EID_EL_OPERATION         230
#define WLAN_EID_SECTORIZED_GROUP_ID_LIST       231 //
#define WLAN_EID_S1G_OPERATION                  232 // 8
#define WLAN_EID_HEADER_COMPRESSION             233
#define WLAN_EID_SST_OPERATION                  234
#define WLAN_EID_MAD                            235
#define WLAN_EID_S1G_RELAY_ACTIVATION           236
#define WLAN_EID_RSNX							244
#define WLAN_EID_VENDOR_SPECIFIC 				221 //  3 TO 257
// RESERVED 222-255
#define WLAN_EID_INFORMATION_ELEMENT_EXTENSION	255

#define WLAN_EID_INFORMATION_ELEMENT_EXTENSION	255

#define MAX_LEGACY_LISTEN_INTERVAL				65535
#define MAX_S1G_UNSCALED_INTERVAL				16383

#define USF2SF(usf)	((usf == 0) ? 1 : (usf == 1) ? 10 : (usf == 2) ? 1000: 10000);

typedef enum {
    MAC_STA_TYPE_STA = 0,
    MAC_STA_TYPE_AP,
	MAC_STA_TYPE_MESH_POINT
} MAC_STA_TYPE;

typedef enum  {
    BOTH = 0,
    SENSOR = 1,
    NON_SENSOR = 2,
} STA_TYPE_SUPPORT_STA;

#define HT_CTL_SIZE		(4)
/************************************************************************************************
      ieee 802.11 mangement frame structure
**************************************************************************************************/

#pragma pack(push, 1)
struct listen_interval_usf {
	unsigned short unscaled_interval		:14;
	unsigned short unified_scaling_factor	:2;
};
///
///struct probe_req {
///	unsigned char variable;
///};
///
///struct probe_resp {
///	unsigned char timestamp [8];
///	unsigned short beacon_interval;
///	unsigned short capab_info;
///	/* possibly followed by SSID, supported rates, and etc. */
///	unsigned char variable;
///};
///
struct s1g_auth {
	unsigned short auth_alg;
	unsigned short auth_transaction;
	unsigned short status_code;
	/* possibly followed by Challenge text */
};
///
///struct deauth {
///	unsigned short reason_code;
///};
///
typedef struct {
	unsigned short capab_info;
	union {
		struct listen_interval_usf	 interval_usf;
		unsigned short listen_interval;
	} u;
	/* followed by SSID and Supported rates */
} assoc_req;
///
struct s1g_assoc_resp {
	unsigned short capab_info;
	unsigned short status_code;
	unsigned short aid;
	/* followed by Supported rates */
};
///
///struct reassoc_req {
///	unsigned short capab_info;
///	union {
///		struct listen_interval_usf	interval_usf;
///		unsigned short listen_interval;
///	} u;
///	unsigned char current_ap[6];
///	/* followed by SSID and Supported rates */
///};
///
///struct disassoc {
///	unsigned short reason_code;
///};
///
struct beacon {
	unsigned long long timestamp;
	unsigned short beacon_int;
	unsigned short capab_info;
	/* followed by some of SSID, Supported rates,
	 * FH Params, DS Params, CF Params, IBSS Params, TIM */
};
///
///struct action {
///	unsigned char category;
///	union {
///		struct {
///			unsigned char action_code;
///			unsigned char dialog_token;
///			unsigned char variable;
///		} type1;
///		struct {
///			unsigned char action_code;
///			unsigned char field;
///		} type2;
///		struct {
///			unsigned char action_code;
///			unsigned short field;
///		} type3;
///		struct {
///			unsigned char action_code;
///		} type4;
///	} u;
///};
///
struct s1g_ieee80211_frame_control {
	unsigned short version        : 2;    // default 0
	unsigned short type           : 2;    // 0: Management , 1: Control,  2: Data,  3: reserved
	unsigned short subtype        : 4;
	unsigned short to_ds          : 1;    // to AP
	unsigned short from_ds        : 1;    // from AP
	unsigned short more_frag      : 1;
	unsigned short retry          : 1;
	unsigned short pwr_mgt        : 1;
	unsigned short more_data      : 1;
	unsigned short protect        : 1;
	unsigned short order          : 1;
};
///
///struct ieee80211_mgmt_header {
///	struct ieee80211_frame_control frame_control;
///	unsigned short duration;
///	unsigned char da[6];
///	unsigned char sa[6];
///	unsigned char bssid[6];
///	unsigned short seq_ctrl;
///};
///
struct s1g_ieee80211_mgmt {
	struct s1g_ieee80211_frame_control frame_control;
	unsigned short duration;
	unsigned char da[6];
	unsigned char sa[6];
	unsigned char bssid[6];
	unsigned short seq_ctrl;
	union {
///		struct probe_req    probe_req;
///		struct probe_resp   probe_resp;
		struct s1g_auth         auth;
///		struct deauth       deauth;
///		struct assoc_req    assoc_req;
///
		struct s1g_assoc_resp   assoc_resp;
///		struct assoc_resp   reassoc_resp;
///
///		struct reassoc_req  reassoc_req;
///		struct disassoc     disassoc;
///		struct beacon       beacon;
///		struct action         action;
	} u;
};
///
///struct ieee80211_pspoll {
///	struct ieee80211_frame_control frame_control;
///	unsigned short aid;
///	unsigned char bssid[6];
///	unsigned char ta[6];
///};
///
///struct ieee80211_hdr {
///	struct ieee80211_frame_control  frame_control;
///	unsigned short  duration_id;
///	unsigned char   addr1[6];
///	unsigned char   addr2[6];
///	unsigned char   addr3[6];
///	unsigned short  seq_ctrl;
///};


#if defined(STANDARD_11AH)
struct ndp_probereq_1m {
	uint32_t		frame_type						: 3;
	uint32_t		cssid_ano_present				: 1;
	uint32_t		cssid_ano						: 16;
	uint32_t		requested_resp_type				: 1;
	uint32_t		reserved						: 4;
	uint32_t		ndp_indication					: 1;
	uint32_t		reserved2						: 6;

	uint32_t		reserved3;
};

struct ndp_probereq_2m {
	uint32_t		frame_type						: 3;
	uint32_t		cssid_ano_present				: 1;
	uint32_t		cssid_ano_lower					: 20;
	uint32_t		reserved						: 8;

	uint32_t		cssid_ano_upper					: 12;
	uint32_t		requested_resp_type				: 1;
	uint32_t		ndp_indication					: 1;
	uint32_t		reserved2						: 18;

};

typedef union {
	struct ndp_probereq_1m			ndp_probereq_1m_format;
	struct ndp_probereq_2m			ndp_probereq_2m_format;
} ndp_probereq;

// EID 217 : S1G Capabilities
typedef struct {
	uint8_t s1g_long_support        				: 1;
	uint8_t shortgi_for_1mhz        				: 1;
	uint8_t shortgi_for_2mhz        				: 1;
	uint8_t shortgi_for_4mhz        				: 1;
	uint8_t shortgi_for_8mhz        				: 1;
	uint8_t shortgi_for_16mhz       				: 1;
	uint8_t supported_channel_width 				: 2;

	uint8_t rxldpc                  				: 1;
	uint8_t txstbc                  				: 1;
	uint8_t rxstbc                  				: 1;
	uint8_t su_beamformer_capable   				: 1;
	uint8_t su_beamformee_capable   				: 1;
	uint8_t beamformee_sts_capability   			: 3;

	uint8_t numberofsounding_dimensions 			: 3;
	uint8_t mu_beamformer_capable   				: 1;
	uint8_t mu_beamformee_capable   				: 1;
	uint8_t htc_vht_capable         				: 1;
	uint8_t traveling_pilot_support 				: 2;

	uint8_t rd_responder            				: 1;
	uint8_t ht_delayed_block_ack    				: 1;
	uint8_t maximum_mpdu_length     				: 1;
	uint8_t maximum_ampdu_length_exponent  			: 2;
	uint8_t minimum_mpdu_start_spacing  			: 3;

	uint8_t uplink_sync_capable     				: 1;
	uint8_t dynamic_aid_support     				: 1;
	uint8_t batsupport              				: 1;
	uint8_t tim_ade_support         				: 1;
	uint8_t non_tim_support         				: 1;
	uint8_t group_aid_support  						: 1;
	uint8_t sta_type_support        				: 2;

	uint8_t centralized_auth_control				: 1;
	uint8_t distributed_auth_control				: 1;
	uint8_t amsdu_supported         				: 1;
	uint8_t ampdu_supported         				: 1;
	uint8_t asymmetric_block_ack_supported  		: 1;
	uint8_t flow_control_supported  				: 1;
	uint8_t sectorizedbeam_capable  				: 2;

	uint8_t obss_mitigation_support 				: 1;
	uint8_t fragment_ba_support     				: 1;
	uint8_t ndp_ps_poll_supported   				: 1;
	uint8_t rawoperation_support    				: 1;
	uint8_t pageslicing_support     				: 1;
	uint8_t txopsharing_implicit_ack_support   		: 1;
	uint8_t vhtlink_adaptation_capable         		: 2;

	uint8_t tack_support_as_ps_poll_response    	: 1;
	uint8_t duplicated_1mhz_support        			: 1;
	uint8_t mcsnegotiation_support         			: 1;
	uint8_t _1mhz_controlresponsepreamble_support 	: 1;
	uint8_t ndp_beamforming_report_poll_supported   : 1;
	uint8_t unsolicited_dynamic_aid_support 		: 1;
	uint8_t sector_training_operation_support   	: 1;
	uint8_t temporary_psmode_switch        			: 1;

	uint8_t twtgrouping_support     				: 1;
	uint8_t bdt_capable             				: 1;
	uint8_t color                   				: 3;
	uint8_t twt_requester_support   				: 1;
	uint8_t twt_responder_support   				: 1;
	uint8_t pv1_frame_support       				: 1;

	uint8_t la_per_normal_control_response_capable  : 1;
	uint8_t reserved                				: 7;
} s1g_capabilities_info;

typedef struct {
	uint16_t rxs1g_mcs_map                        			: 8;
	uint16_t rxhighestsupported_long_gi_datarate  			: 9;
	uint16_t txs1g_mcs_map                        			: 8;
	uint16_t txhighestsupported_long_gi_datarate  			: 9;
	uint16_t rxsingle_spatialstream_and_s1gmcsmap_for_1mhz	: 2;
	uint16_t txsingle_spatialstream_and_s1gmcsmap_for_1mhz	: 2;
	uint16_t reserved                               		: 2;
} supported_s1g_msc_nss_set;


/* Table 10-20-S1G BSS operating channel width */
typedef struct {
	uint8_t bss_primary_ch_width		: 1;
	uint8_t	bss_operation_ch_width		: 4;
	uint8_t location_of_primary_ch		: 1;
	uint8_t	reserved 					: 1;
	uint8_t	mcs10_permitted				: 1;
} s1g_operation_info_channel_width;

typedef struct {
	s1g_operation_info_channel_width		channelwidth;
	uint8_t									operatingclass;
	uint8_t									primarychnumber;
	uint8_t									chcenterfrequency;
} s1g_operation_information;

typedef struct {
	uint8_t			mins1g_mcs_for_1ss		: 2;
	uint8_t			maxs1g_mcs_for_1ss		: 2;
	uint8_t			mins1g_mcs_for_2ss		: 2;
	uint8_t			maxs1g_mcs_for_2ss		: 2;
	uint8_t			mins1g_mcs_for_3ss		: 2;
	uint8_t			maxs1g_mcs_for_3ss		: 2;
	uint8_t			mins1g_mcs_for_4ss		: 2;
	uint8_t			maxs1g_mcs_for_4ss		: 2;
} basic_s1g_mcs_and_nss_set;

#endif /* defined(STANDARD_11AH) **/

/////////////////////////
// Information Elements

#define INFO_ELEMENT_MAX_LENGTH	255
typedef struct {
	uint8_t	eid;
	uint8_t	length;
	uint8_t	info[INFO_ELEMENT_MAX_LENGTH];
} ie_general;

// EID 0 : SSID
#define MAX_SSID_LEN					32
typedef struct {
	uint8_t	eid;
	uint8_t	length;
	uint8_t	ssid[MAX_SSID_LEN];
} ie_ssid;

// EID 1 : Supported rates
#define IE_LENGTH_SUPPORTED_RATES   	8
typedef struct {
	uint8_t		eid;
	uint8_t 	length;

	uint8_t 	rates[8];
} ie_supported_rates;

// EID 2 : FH Parameter Set
// EID 3 : DS Parameter Set
#define IE_LENGTH_DS_PARAMETER_SET		1
typedef struct {
	uint8_t		eid;
	uint8_t 	length;
	uint8_t 	currentchannel;
} ie_ds_parameter_set;

// EID 4 : CF Parameter Set
#define IE_LENGTH_CF_PARAMETER_SET		6
typedef struct {
	uint8_t		eid;
	uint8_t 	length;
	uint8_t 	cfp_count;
	uint8_t 	cfp_period;
	uint16_t 	cfp_maxduration; // tu
	uint16_t 	cfp_durremaining; // tu
} ie_cf_parameter_set;

// EID 5 : TIM
#define DOT11_VBM_SIZE  251		/* Virtual Bit Map size = 2008/8 =251 */

#define S1G_TIM_BITMAP_TRAFFIC_INDICATOR 	BIT(0)
typedef struct {
	uint8_t		eid;
	uint8_t 	length;
	uint8_t 	dtim_count;
	uint8_t 	dtim_period;
	uint8_t 	bitmapcontrol;
	uint8_t 	partialvirtualbitmap[DOT11_VBM_SIZE];
} ie_tim;

// EID 12 : EDCA Parameter Set
#define IE_LENGTH_EDCA_PARAMETER_SET	18

typedef  struct {
	uint8_t	_override   : 1;
	uint8_t	ps_poll_aci : 2;
	uint8_t	raw_aci     : 2;
	uint8_t	sta_type    : 2;
	uint8_t	reserved    : 1;
} update_edca_info;

#define ACI_AC_BE       0x0
#define ACI_AC_BK       0x1
#define ACI_AC_VI       0x2
#define ACI_AC_VO       0x3

typedef struct {
	uint8_t      aifsn       : 4;
	uint8_t      acm         : 1;
	uint8_t      aci         : 2;
	uint8_t      reserved    : 1;
	uint8_t      ecwmin      : 4;
	uint8_t      ecwmax      : 4;
	uint16_t     txoplimit;
} edca_param_record;

typedef struct {
	uint8_t 			eid;
	uint8_t 			length;

	uint8_t 	        qos_info;
	update_edca_info    updateedca_info;
	edca_param_record 	ac_be;
	edca_param_record 	ac_bk;
	edca_param_record   ac_vi;
	edca_param_record   ac_vo;
} ie_edca_parameter_set;

// EID 37 : WLAN_EID_CHANNEL_SWITCH_ANNOUNCEMENT
typedef struct
{
	uint8_t		eid;
	uint8_t		length;
	uint8_t		mode;
	uint8_t		channel;
	uint8_t		ch_switch_cnt;
}ie_csa;


// EID 45 : HT Capabilities
#define IE_LENGTH_HT_CAPABILITIES	26
typedef  struct {
	uint16_t	ldpc_coding_capability 	: 1;
	uint16_t	supported_channel_width	: 1;
	uint16_t	sm_power_save			: 2;
	uint16_t	ht_greenfield			: 1;
	uint16_t	shortgi_for_20mhz		: 1;
	uint16_t	shortgi_for_40mhz		: 1;
	uint16_t	txstbc					: 1;
	uint16_t	rxstbc					: 2;
	uint16_t	ht_delayed_blockack		: 1;
	uint16_t	maximum_amsdu_length	: 1;
	uint16_t	dsss_cck_mode_40mhz	: 1;
	uint16_t	reserved					: 1;
	uint16_t	fortymhzintolerant			: 1;
	uint16_t	l_sig_txop_protection	: 1;
} ht_capabilities_info;
typedef  struct {
	uint8_t		eid;
	uint8_t 	length;

	ht_capabilities_info ht_capabilities_info;
	uint8_t 	ampdu_parameters;
	uint8_t		supported_mcs_set[16];
	uint16_t	ht_extended_capabilities;
	uint32_t	transmit_beamforming_capabilities;
	uint8_t		asel_capabilities;
} ie_ht_capabilities;

// EID 48: RSN
typedef struct {
	uint8_t		eid;
	uint8_t		length;
	uint8_t		data[255];
} ie_rsn;

// EID 56: TIMEOUT INTERVAL
typedef struct {
	uint8_t		eid;
	uint8_t		length;
	uint8_t		timeout_interval_type;
	uint32_t	timeout_interval;
} ie_timeout_interval;

// EID 60 : WLAN_EID_EXT_CHANNEL_SWITCH_ANNOUNCEMENT
typedef struct
{
	uint8_t		eid;
	uint8_t		length;
	uint8_t		mode;
	uint8_t		class;
	uint8_t		channel;
	uint8_t		ch_switch_cnt;
}ie_ext_csa;

// EID 61: HT OPERATION
#define IE_LENGTH_HT_OPERATION		22
typedef  struct {
	uint8_t		eid;
	uint8_t 	length;
	uint8_t		primary_channel;

	uint8_t		secondary_channel_offset: 2;
	uint8_t		sta_channel_width: 1;
	uint8_t		rifs_mode: 1;
	uint8_t		reserved1: 4;

	uint16_t	ht_protection: 2;
	uint16_t	nongreenfield_ht_stas_present: 1;
	uint16_t	reserved2: 1;
	uint16_t	obss_nonht_stas_present: 1;
	uint16_t	reserved3: 11;

	uint16_t	reserved4: 6;
	uint16_t	dual_beacon: 1;
	uint16_t	dual_cts_protection: 1;
	uint16_t	stbc_beacon: 1;
	uint16_t	l_sig_txop_protection_full_support: 1;
	uint16_t	pco_active: 1;
	uint16_t	pco_phase: 1;
	uint16_t	reserved: 4;

	uint8_t 	basic_mcs_set[16];
} ie_ht_operation;

// EID 90: BSS Max Idle Period
#define IE_LENGTH_MAX_IDLE_PERIOD 3
typedef struct {
	uint16_t	unscaledinterval: 14;
	uint16_t	usfvalue: 2;
} max_idle_period;

typedef struct {
	union {
		max_idle_period		maxidleperiod;
		uint16_t			maxidleperiod_ms;
	};
	uint8_t				idleoption;
} bss_max_idle_info;

typedef struct {
	uint8_t	     		eid;
	uint8_t      		length;
	bss_max_idle_info 	info;
} ie_bss_max_idle_period;


// EID 127 :  Extended Capabilities
#define IE_LENGTH_EXTENDED_CAPABILITIES		8
typedef  struct {
	uint8_t	eid;
	uint8_t 	length;

	uint8_t	ext_capa[8];
} ie_extended_capabilities;

// EID 210 : AID Response
#define IE_LENGTH_AID_REQUEST 1
typedef  struct {
	uint8_t	eid;
	uint8_t length;

	uint8_t aid_request_mode;
} ie_aid_request;

// EID 211 : AID Response
#define IE_LENGTH_AID_RESPONSE		5
typedef  struct {
	uint8_t	eid;
	uint8_t length;

	uint16_t aid;
	uint8_t aid_switch_count;
	uint16_t aid_response_interval;

} ie_aid_response;


#if defined(STANDARD_11AH)

// EID 213 : S1G Beacon Compatibility
#define IE_LENGTH_S1G_BEACON_COMPATIBILITY  		8
typedef struct {
	uint8_t ess                     : 1;
	uint8_t ibss                    : 1;
	uint8_t cf_pollable             : 1;
	uint8_t cf_poll_request         : 1;
	uint8_t privacy                 : 1;
	uint8_t short_preamble          : 1;
	uint8_t tsf_rollover            : 1;
	uint8_t channel_agility         : 1;
	uint8_t spectrum_mgmt           : 1;
	uint8_t qos                     : 1;
	uint8_t short_slottime          : 1;
	uint8_t apsd                    : 1;
	uint8_t radio_measurement       : 1;
	uint8_t dsss_ofdm               : 1;
	uint8_t delayed_block_ack       : 1;
} compatibility_info;

typedef struct {
	uint8_t     eid;
	uint8_t     length;
	union {
		compatibility_info comp_info;
		uint16_t info;
	};
	uint16_t    beacon_interval;
	uint32_t    tsf_completion;
} ie_s1g_beacon_compatibility;


// EID 214 : Short Beacon interval
#define IE_LENGTH_SHORT_BEACON_INTERVAL             2
typedef struct {
	uint8_t	    eid;
	uint8_t     length;
	uint16_t    short_beacon_interval;
} ie_short_beacon_interval;

#if defined(INCLUDE_TWT_SUPPORT)
#define IE_LENGTH_TWT 15
// EID 216 : TWT
typedef struct {
	uint32_t		pid: 9;
	uint32_t		max_ndp_paging_period: 8;
	uint32_t		partial_tsf_offset: 4;
	uint32_t		action: 3;
	uint32_t		min_sleep_duration: 6;
	uint32_t		reserved: 2;
} twt_ndp_paging;

typedef struct {
	uint8_t		twt_group_id: 7;
	uint8_t		zero_offset_present: 1;
	uint8_t		zero_offset[6];
	uint16_t		twt_unit: 4;
	uint16_t		twt_offset: 12;
} twt_group_assignment;

typedef struct {
	uint16_t	twt_request: 1;
	uint16_t	twt_setup_command: 3;
	uint16_t	reserved: 1;
	uint16_t	implicit: 1;
	uint16_t	flow_type: 1;
	uint16_t	twt_flow_id: 3;
	uint16_t	twt_wake_interval_exp: 5;
	uint16_t	twt_protection: 1;
} twt_request_type;

typedef struct {
	uint8_t		ndp_paging_indicator: 1;
	uint8_t		responder_pm_mode: 1;
	uint8_t		reserved: 6;
} twt_control;

typedef struct {
	uint8_t 	eid;
	uint8_t 	length;
	twt_control		control;
	twt_request_type		request_type;
	uint32_t	target_wake_time_lo;
	uint32_t	target_wake_time_hi;
	/* Remove field because of Non-support
	twt_group_assignment	twt_group;
	*/
	uint8_t		nom_min_twt_wake_duration;
	uint16_t 	twt_wake_int_mantissa;
	uint8_t		twt_channel;
	/* Remove field because of Non-support
	twt_ndp_paging	ndp_paging;
	*/
} ie_twt;
#endif /* defined(INCLUDE_TWT_SUPPORT) */

// EID 217 : S1G Capabilities
#define IE_LENGTH_S1G_CAPABILITIES      15
typedef struct {
	uint8_t 	eid;
	uint8_t 	length;
	s1g_capabilities_info s1g_capa_info;
	supported_s1g_msc_nss_set s1g_supp_mcs_nss_set;
} ie_s1g_capabilities;


// EID 232 : S1G Operation
#define IE_LENGTH_S1G_OPERATION						6
typedef struct {
	uint8_t	    eid;
	uint8_t     length;
	s1g_operation_information	operationinfo;
	basic_s1g_mcs_and_nss_set	mcsnssset;
} ie_s1g_operation;



//EID 233 : Header Compression
typedef struct __attribute__((packed)){
	uint8_t request_response: 1;
	uint8_t store_a3: 1;
	uint8_t store_a4: 1;
	uint8_t ccmp_update_present: 1;
	uint8_t pv1_data_type3_support: 1;
	uint8_t reserved: 3;
} header_comp_ctrl;

///struct ccmp_update {
///	uint32_t bpn:32; // pn2 ~ pn5
///	uint8_t keyid: 2;
///	uint8_t tid_aci:4;
///	uint8_t reserved:2;
///} __attribute__((packed));

typedef struct __attribute__((packed)) {
	uint8_t				eid;
	uint8_t				length;
	header_comp_ctrl	control;
	uint8_t				a3[6];
	uint8_t				a4[6];
	///ccmp_update			ccmpupdate;
} ie_header_compression;
#endif

// EID 221 : Vendor Specific
typedef enum {
    WMM_INFORMATION = 0,
    WMM_PARAMETER = 1
} OUI_SUBTYPE;

#define IE_LENGTH_VENDOR_SPECIFIC_INFO_WMM					7
#define IE_LENGTH_VENDOR_SPECIFIC_PARA_WMM					24
typedef struct {
	uint8_t 	eid;
	uint8_t 	length;
	uint8_t 	oui[3];
	uint8_t		oui_type;
	uint8_t		oui_subtype;
	uint8_t		version;
	uint8_t		qos_info;
	uint8_t		reserve;
	uint8_t		param[16];
} ie_vendor_specific;

// EID 244: RSN Extension
typedef struct {
	uint8_t		eid;
	uint8_t		length;
	uint8_t		data[255];
} ie_rsn_extension;

#pragma pack(pop)
#endif
