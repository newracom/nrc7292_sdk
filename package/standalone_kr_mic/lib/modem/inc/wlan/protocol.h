#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_ADDR_LEN        (6)

#define FC_PV0 0
#define FC_PV1 1

#define FC_PV0_TYPE_MGMT        0
#define FC_PV0_TYPE_CTRL        1
#define FC_PV0_TYPE_DATA        2
#define FC_PV0_TYPE_EXT         3

#define FC_PV0_TYPE_CTRL_PS_POLL            10
#define FC_PV0_TYPE_CTRL_CF_END             14

#define FC_PV0_TYPE_MGMT_ASSOC_REQ          0
#define FC_PV0_TYPE_MGMT_ASSOC_RSP          1
#define FC_PV0_TYPE_MGMT_REASSOC_REQ        2
#define FC_PV0_TYPE_MGMT_REASSOC_RSP        3
#define FC_PV0_TYPE_MGMT_PROBE_REQ          4
#define FC_PV0_TYPE_MGMT_PROBE_RSP          5
#define FC_PV0_TYPE_MGMT_TIMING_ADVERTISE   6
#define FC_PV0_TYPE_MGMT_RESERVED           7
#define FC_PV0_TYPE_MGMT_BEACON             8
#define FC_PV0_TYPE_MGMT_ATIM               9
#define FC_PV0_TYPE_MGMT_DISASSOC           10
#define FC_PV0_TYPE_MGMT_AUTH               11
#define FC_PV0_TYPE_MGMT_DEAUTH             12
#define FC_PV0_TYPE_MGMT_ACTION             13
#define FC_PV0_TYPE_MGMT_ACTION_NOACK       14
#define FC_PV0_TYPE_MGMT_MAX                15

#define FC_PV0_TYPE_MGMT_ACTION_BLOCK_ACK					3
#define FC_PV0_TYPE_MGMT_ACTION_BLOCK_ACK_ADDBA_REQ			0
#define FC_PV0_TYPE_MGMT_ACTION_BLOCK_ACK_ADDBA_RESP		1
#define FC_PV0_TYPE_MGMT_ACTION_BLOCK_ACK_DELBA_REQ			2
#define FC_PV0_TYPE_MGMT_ACTION_BLOCK_ACK_NDP_ADDBA_REQ		128
#define FC_PV0_TYPE_MGMT_ACTION_BLOCK_ACK_NDP_ADDBA_RESP	129
#define FC_PV0_TYPE_MGMT_ACTION_BLOCK_ACK_NDP_DELBA_REQ 	130

#define FC_PV0_TYPE_MGMT_ACTION_NOACK_TWT_SETUP		6
#define FC_PV0_TYPE_MGMT_ACTION_NOACK_TWT_TEARDOWN	7
#define FC_PV0_TYPE_MGMT_ACTION_NOACK_TWT_INFO		11

#define FC_PV0_TYPE_DATA_DATA                   0
#define FC_PV0_TYPE_DATA_DATA_CF_ACK            1
#define FC_PV0_TYPE_DATA_DATA_CF_POLL           2
#define FC_PV0_TYPE_DATA_DATA_CF_ACK_POLL       3
#define FC_PV0_TYPE_DATA_DATA_NULL              4
#define FC_PV0_TYPE_DATA_CF_ACK                 5
#define FC_PV0_TYPE_DATA_CF_POLL                6
#define FC_PV0_TYPE_DATA_CF_ACK_POLL            7
#define FC_PV0_TYPE_DATA_QOS_DATA               8
#define FC_PV0_TYPE_DATA_QOS_DATA_CF_ACK        9
#define FC_PV0_TYPE_DATA_QOS_DATA_CF_POLL       10
#define FC_PV0_TYPE_DATA_QOS_DATA_CF_ACK_POLL   11
#define FC_PV0_TYPE_DATA_QOS_NULL               12
#define FC_PV0_TYPE_DATA_RESERVED               13
#define FC_PV0_TYPE_DATA_QOS_CF_POLL            14
#define FC_PV0_TYPE_DATA_QOS_CF_ACK_POLL        15
#define FC_PV0_TYPE_DATA_MAX                    16

#define FC_PV0_TYPE_EXT_S1G_BEACON				1

#define FC_PV0_PROTECTED    0x4000

#define FC_PV1_TYPE_QOSDATA0    0
#define FC_PV1_TYPE_MGMT        1
#define FC_PV1_TYPE_CTRL        2
#define FC_PV1_TYPE_QOSDATA3    3

#define FC_PV1_TYPE_MGMT_ACTION             0
#define FC_PV1_TYPE_MGMT_ACTION_NOACK       1
#define FC_PV1_TYPE_MGMT_SHORT_PROBE_RSP    2
#define FC_PV1_TYPE_MGMT_RESOURCE_ALLOC     3

#define ETH_P_PAE 0x888E /* Port Access Entity (IEEE 802.1X) */
#define ETH_P_ARP 0x0608 /* ARP */

#define IEEE80211_QOSCTL_ACK_POLICY_NORMAL 		0
#define IEEE80211_QOSCTL_ACK_POLICY_NOACK 		1
#define IEEE80211_QOSCTL_ACK_POLICY_NOEXPL		2
#define IEEE80211_QOSCTL_ACK_POLICY_BA 			3


typedef struct _QoSField {
	uint16_t    qos_tid                 : 4;
    uint16_t    qos_eosp                : 1;
	uint16_t    qos_ack_policy          : 2;   /* 00: normal ack, 01: no ack, 10: no explicit ack or scheduled ack under PSMP, 11: block ack */
	uint16_t    qos_amsdu_present       : 1;   /* AMSDU Presence */
	uint16_t    qos_bit8_15             : 8;
} QoSField;

typedef struct _GenericMacHeader {
    /* Word 0 : MAC Header Word 0 */
    /* 16 bit Frame Control */
	uint16_t    version        : 2;    /* default 0 */
	uint16_t    type           : 2;    /* 0: Management , 1: Control,  2: Data,  3: reserved */
	uint16_t    subtype        : 4;
	uint16_t    to_ds          : 1;    /* to AP */
	uint16_t    from_ds        : 1;    /* from AP */
	uint16_t    more_frag      : 1;
	uint16_t    retry          : 1;
	uint16_t    pwr_mgt        : 1;
	uint16_t    more_data      : 1;
	uint16_t    protect        : 1;
	uint16_t    order          : 1;
    /* 16 bit Duration */
	uint16_t duration_id;

    /* Word 1 ~ Word 5: MAC Header Word 1 ~ MAC Header Word 5(2byte) */
	uint8_t     address1[6];
	uint8_t     address2[6];
	uint8_t     address3[6];

	/* Word 5 : MAC Header Word 5(2byte) */
	uint16_t    fragment_number    : 4;
	uint16_t    sequence_number    : 12;

    /* Qos Control Field(16bit) exist only subtype is QoS Data or Qos Null */
    /* Word 6 : MAC Header Word 6(2Byte) */
    union {
        struct {
        	uint16_t    qos_tid                 : 4;
    	    uint16_t    qos_eosp                : 1;
        	uint16_t    qos_ack_policy          : 2;   /* 00: normal ack, 01: no ack, 10: no explicit ack or scheduled ack under PSMP, 11: block ack */
        	uint16_t    qos_amsdu_present       : 1;   /* AMSDU Presence */
        	uint16_t    qos_bit8_15             : 8;
            uint8_t     qos_payload[0];
        };
        uint8_t payload[0];
    };
} GenericMacHeader;

struct ieee80211_beacon {
	uint16_t	version        : 2;
	uint16_t	type           : 2;
	uint16_t 	subtype        : 4;
	uint16_t 	to_ds          : 1;
	uint16_t 	from_ds        : 1;	
	uint16_t 	more_frag      : 1;
	uint16_t 	retry          : 1;
	uint16_t 	pwr_mgt        : 1;
	uint16_t 	more_data      : 1;
	uint16_t 	protect        : 1;
	uint16_t	order          : 1;
	/* 16 bit Duration */
	uint16_t	duration_id;
	/* Word 1 ~ Word 5: MAC Header Word 1 ~ MAC Header Word 5(2byte) */
	uint8_t     address1[6];
	uint8_t     address2[6];
	uint8_t     address3[6];
	/* Word 5 : MAC Header Word 5(2byte) */
	uint16_t    fragment_number    : 4;
	uint16_t    sequence_number    : 12;
	/* Word 6 ~ Word 7 */
	uint64_t    timestamp;
	/* Word 8 */
	uint16_t    beacon_interval;
   	uint16_t    capa_info;
	/* SSID IE Word 9 */
	uint8_t 	element[0];
};

typedef struct _ActionFrame {
	uint8_t category				: 8;
	uint8_t action_field 			: 8;
	uint8_t action_detail[0];
} ActionFrame;

typedef struct _ADDBA_Req {
	uint8_t category 				: 8;
	uint8_t ba_action 				: 8;
	uint8_t dial_token 				: 8;

	uint8_t amsdu 					: 1;
	uint8_t policy 					: 1;
	uint8_t tid 					: 4;
	uint16_t max_buf 				: 10;

	uint16_t timeout				: 16;
	uint16_t frag 					: 4;
	uint16_t ssn 					: 12;
} __attribute__((packed)) ADDBA_Req ;

typedef struct _ADDBA_Resp {
	uint8_t category 				: 8;
	uint8_t ba_action 				: 8;
	uint8_t dial_token 				: 8;
	uint16_t status 				: 16;
	uint8_t amsdu 					: 1;
	uint8_t policy 					: 1;
	uint8_t tid 					: 4;
	uint16_t max_buf 				: 10;
	uint16_t timeout 				: 16;
} __attribute__((packed)) ADDBA_Resp;

typedef struct _DELBA_Req {
	uint8_t category 				: 8;
	uint8_t ba_action 				: 8;
	uint16_t rsv 					: 11;
	uint8_t init 					: 1;
	uint8_t tid 					: 4;
	uint16_t reason 				: 16;
} __attribute__((packed)) DELBA_Req;

typedef struct _CCMPHeader {
	uint8_t	pn0;
	uint8_t	pn1;

	uint8_t	reserved1;

	uint8_t	reserved2		: 5;
	uint8_t	ext_iv			: 1;
	uint8_t	key_id			: 2;

	uint8_t	pn2;
	uint8_t pn3;
	uint8_t	pn4;
	uint8_t	pn5;
} CCMPHeader;
#define CCMPH(x) reinterpret_cast<CCMPHeader*>(x)

#if defined(INCLUDE_TWT_SUPPORT)
typedef struct _TWT_Setup {
	uint8_t category				: 8;
	uint8_t action					: 8;
	uint8_t dial_token				: 8;

	uint8_t ndp_paging_indicator: 1;
	uint8_t responder_pm_mode: 1;
	uint8_t reserved1: 6;
	uint16_t twt_request: 1;
	uint16_t twt_setup_command: 3;
	uint16_t reserved2: 1;
	uint16_t implicit: 1;
	uint16_t flow_type: 1;
	uint16_t twt_flow_id: 3;
	uint16_t twt_wake_interval_exp: 5;
	uint16_t twt_protection: 1;
	uint32_t target_wake_time_lo;
	uint32_t target_wake_time_hi;
	/* Remove because of Non-Support
	uint8_t twt_group_id: 7;
	uint8_t zero_offset_present: 1;
	uint8_t zero_offset[6];
	uint16_t twt_unit: 4;
	uint16_t twt_offset: 12;
	*/
	uint8_t nom_min_twt_wake_duration;
	uint16_t twt_wake_int_mantissa;
	uint8_t twt_channel;
	/* Remove because of Non-Support
	uint32_t pid: 9;
	uint32_t max_ndp_paging_period: 8;
	uint32_t partial_tsf_offset: 4;
	uint32_t ndp_paging_action: 3;
	uint32_t min_sleep_duration: 6;
	uint32_t reserved3: 2;
	*/
} __attribute__((packed)) TWT_Setup ;

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
#endif /* defined(INCLUDE_TWT_SUPPORT) */

typedef struct _WEPHeader {
	uint8_t	iv0;
	uint8_t	iv1;
	uint8_t	iv2;


	uint8_t	reserved2		: 6;
	uint8_t	key_id			: 2;

} WEPHeader;
#define WEPH(x) reinterpret_cast<WEPHeader*>(x)

typedef struct _TKIPHeader {
	uint8_t	tsc1;
	uint8_t	wep_seed;
	uint8_t	tsc0;

	uint8_t	reserved2		: 5;
	uint8_t	ext_iv			: 1;
	uint8_t	key_id			: 2;

	uint8_t	tsc2;
	uint8_t	tsc3;
	uint8_t	tsc4;
	uint8_t	tsc5;

} TKIPHeader;
#define TKIPH(x) reinterpret_cast<TKIPHeader*>(x)

typedef struct _LLCField {
	uint8_t    dsap;
    uint8_t    ssap;
	uint8_t    frame_type : 2;
	uint8_t    command    : 6;
	uint8_t    organization_code[3];
	uint16_t   type;
} LLCField;

#define MULTICAST_IP 0xE0
typedef struct _IPField {
	uint8_t    hdr_len		: 4;
	uint8_t    version		: 4;
	uint8_t    diff_svc;
	uint16_t   total_len;
	uint16_t   id;
	uint16_t   flags;
	uint8_t    ttl;
	uint8_t    protocol;
	uint16_t   cs;
	uint8_t    src[4];
	uint8_t    dest[4];
} IPField;

typedef struct _UDPField {
	uint16_t   src;
	uint16_t   dest;
	uint16_t   length;
	uint16_t   cs;
} UDPField;

int ieee80211_ver(void* frame);
bool ieee80211_is_pv0(void* frame);
bool ieee80211_is_pv1(void* frame);
bool ieee80211_group_addr(uint8_t *addr);
bool ieee80211_broadcast_addr(uint8_t *addr);
bool ieee80211_is_pv1_mgmt(GenericMacHeader *gmh);
bool ieee80211_is_mgmt(GenericMacHeader *gmh);
bool ieee80211_is_ctrl(GenericMacHeader *gmh);
bool ieee80211_is_data(GenericMacHeader *gmh);
bool ieee80211_is_extn(GenericMacHeader *gmh);
bool ieee80211_is_non_qos_data(GenericMacHeader *gmh);
bool ieee80211_is_null_data(GenericMacHeader *gmh);
bool ieee80211_is_qos_data(GenericMacHeader *gmh);
bool ieee80211_is_qos_null(GenericMacHeader *gmh);
bool ieee80211_is_preq(GenericMacHeader *gmh);
bool ieee80211_is_pv1_presp(GenericMacHeader *gmh);
bool ieee80211_is_presp(GenericMacHeader *gmh);
bool ieee80211_is_pspoll(GenericMacHeader *gmh);
bool ieee80211_is_retry(GenericMacHeader *gmh);
bool ieee80211_is_more_data(GenericMacHeader *gmh);
bool ieee80211_is_beacon(GenericMacHeader* gmh);
bool ieee80211_is_auth(GenericMacHeader* gmh);
bool ieee80211_is_deauth(GenericMacHeader* gmh);
bool ieee80211_is_disasoc(GenericMacHeader* gmh);
bool ieee80211_is_assoc_resp(GenericMacHeader* gmh);
bool lmac_check_action_frame(GenericMacHeader* gmh);
bool ieee80211_is_protected(GenericMacHeader* gmh);
bool ieee80211_is_s1g_beacon(GenericMacHeader *gmh);
bool ieee80211_is_frag(GenericMacHeader* gmh);
bool ieee80211_is_amsdu(GenericMacHeader* gmh);
bool ieee80211_is_eapol(GenericMacHeader *gmh, int len);
bool ieee80211_has_htc(GenericMacHeader* gmh);
uint8_t* ieee80211_get_bssid(GenericMacHeader *gmh);
bool ieee80211_is_arp(GenericMacHeader* gmh);
bool ieee80211_is_dhcp(GenericMacHeader* gmh);
bool ieee80211_is_robust(GenericMacHeader* gmh);
bool ieee80211_is_ip(GenericMacHeader* gmh);

bool ieee80211_has_fromds(GenericMacHeader* gmh);
bool ieee80211_has_tods(GenericMacHeader* gmh);
uint8_t* ieee80211_da(GenericMacHeader* gmh);
bool ieee80211_is_any_mgmt(void *mh);

const uint8_t* broadcast_addr();
bool is_broadcast_addr(const uint8_t* addr);

uint8_t get_mesh_control_length(GenericMacHeader* gmh);
uint8_t ieee80211_mhd_length(GenericMacHeader* gmh);
#endif /* __PROTOCOL_H__ */
