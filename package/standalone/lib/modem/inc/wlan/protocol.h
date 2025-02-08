#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__
#include <sys/types.h>

#ifndef BIT
#define BIT(n) ((uint32_t)(0x00000001L << n))
#endif

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

#define FC_PV0_TYPE_MGMT_ACTION_UNPROTECTED_S1G		22
#define FC_PV0_TYPE_MGMT_ACTION_UNPROTECTED_TWT_SETUP		6
#define FC_PV0_TYPE_MGMT_ACTION_UNPROTECTED_TWT_TEARDOWN	7
#define FC_PV0_TYPE_MGMT_ACTION_UNPROTECTED_TWT_INFO		11
#define FC_PV0_TYPE_MGMT_ACTION_S1G				23
#define FC_PV0_TYPE_MGMT_ACTION_PROTECTED_TWT_SETUP		4
#define FC_PV0_TYPE_MGMT_ACTION_PROTECTED_TWT_TEARDOWN		5
#define FC_PV0_TYPE_MGMT_ACTION_PROTECTED_TWT_INFO		6

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

#define OUI_IEEE_REGISTRATION_AUTHORITY 0xFCFFAA

typedef struct _QoSField {
	uint16_t    qos_tid                 : 4;
	uint16_t    qos_eosp                : 1;
	uint16_t    qos_ack_policy          : 2;   /* 00: normal ack, 01: no ack, 10: no explicit ack or scheduled ack under PSMP, 11: block ack */
	uint16_t    qos_amsdu_present       : 1;   /* AMSDU Presence */
	uint16_t    qos_mesh_ctl_present    : 1;
	uint16_t    qos_mesh_ps_level       : 1;
	uint16_t    qos_mesh_rspi           : 1;
	uint16_t    qos_bit11_15            : 5;
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
			uint16_t    qos_mesh_ctl_present    : 1;
			uint16_t    qos_mesh_ps_level       : 1;
			uint16_t    qos_mesh_rspi           : 1;
			uint16_t    qos_bit11_15            : 5;
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

#if defined(TEST_SBR)
#define SBR_TYPE_BEACON 0
#define SBR_TYPE_WAKEUP 1
#define SBR_TYPE_SHORT_WAKEUP 2
#define SBR_TYPE_DATA_WAKEUP 3

#define SBR_SUBTYPE_CONTROL_DUTY 0
#define SBR_SUBTYPE_CONTROL_TBTT 1
#define SBR_SUBTYPE_CONTROL_THRESHOLD 2
#define SBR_SUBTYPE_GENERAL 7

typedef struct _SBR_Beacon{
	uint32_t	id 			: 18;
	uint32_t	type		: 2;
	uint32_t	cont		: 1;
	uint32_t	tsf			: 11;
} SBR_Beacon;

typedef struct _SBR_Wakeup {
	uint32_t	id 			: 18;
	uint32_t	type		: 2;
	uint32_t	cont		: 1;
	uint32_t	body_present: 1;
	uint32_t	length      : 7;
	uint32_t	reserved    : 3;
} SBR_Wakeup;

typedef struct _SBR_Short_Wakeup {
	uint32_t	id 			: 18;
	uint32_t	type		: 2;
	uint32_t	cont		: 1;
	uint32_t	reserved    : 3;
} SBR_Short_Wakeup;

typedef struct _SBR_Data_Wakeup {
	uint32_t	id 			: 18;
	uint32_t	type		: 2;
	uint32_t	cont		: 1;
	uint32_t	body_present: 1;
	uint32_t	length      : 7;
	uint32_t	subtype     : 3;
    uint8_t     body [127];
} SBR_Data_Wakeup;
#endif


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

#if defined(_BYTE_ORDER) && defined(_BIG_ENDIAN) && \
    defined(_LITTLE_ENDIAN)
#else
//#error "please include stdio.h or sys/types.h for endian.h"
#endif

typedef struct _TCPField {
	uint16_t   src;
	uint16_t   dest;
	uint32_t   seqnum;
	uint32_t   acknum;
#if _BYTE_ORDER == _BIG_ENDIAN
	uint16_t   offset	: 4;
	uint16_t   reserved : 3;
	uint16_t   ns		: 1;
	uint16_t   cwr		: 1;
	uint16_t   ece		: 1;
	uint16_t   urg		: 1;
	uint16_t   ack		: 1;
	uint16_t   psh		: 1;
	uint16_t   rst		: 1;
	uint16_t   syn		: 1;
	uint16_t   fin		: 1;
#elif _BYTE_ORDER == _LITTLE_ENDIAN
	uint16_t   ns		: 1;
	uint16_t   reserved : 3;
	uint16_t   offset	: 4;

	uint16_t   fin		: 1;
	uint16_t   syn		: 1;
	uint16_t   rst		: 1;
	uint16_t   psh		: 1;
	uint16_t   ack		: 1;
	uint16_t   urg		: 1;
	uint16_t   ece		: 1;
	uint16_t   cwr		: 1;
#else
	uint16_t   ns		: 1;
	uint16_t   reserved : 3;
	uint16_t   offset	: 4;

	uint16_t   fin		: 1;
	uint16_t   syn		: 1;
	uint16_t   rst		: 1;
	uint16_t   psh		: 1;
	uint16_t   ack		: 1;
	uint16_t   urg		: 1;
	uint16_t   ece		: 1;
	uint16_t   cwr		: 1;
#endif
	uint16_t   winsize;
	uint16_t   chksum;
	uint16_t   urgent;
} TCPField;

#define MESH_FLAGS_AE_A4    0x1
#define MESH_FLAGS_AE_A5_A6 0x2
#define MESH_FLAGS_AE       0x3
typedef struct ieee80211s_hdr {
	uint8_t    flags     : 2;
	uint8_t    propagate : 1;
	uint8_t    reserved  : 5;
	uint8_t    ttl;
	uint32_t   seqnum;
	uint8_t    eaddr1[6];
	uint8_t    eaddr2[6];
} __attribute__((packed)) MeshControlField;

/* EAPOL MSG */
#define EAPOL_TYPE_EAPOL_KEY			(3)
#define EAPOL_KEY_INFO_KEY_TYPE			BIT(3)
#define EAPOL_KEY_INFO_KEY_INDEX		(BIT(4) | BIT(5))
#define EAPOL_KEY_INFO_INSTALL			BIT(6)
#define EAPOL_KEY_INFO_TXRX				BIT(6)
#define EAPOL_KEY_INFO_ACK				BIT(7)
#define EAPOL_KEY_INFO_MIC				BIT(8)
#define EAPOL_KEY_INFO_SECURE			BIT(9)
#define EAPOL_KEY_INFO_ERROR			BIT(10)
#define EAPOL_KEY_INFO_REQUEST			BIT(11)
#define EAPOL_KEY_INFO_ENCR_KEY_DATA	BIT(12)
#define EAPOL_KEY_INFO_SMK_MESSAGE		BIT(13)

struct eapol_hdr {
	uint8_t version;
	uint8_t type;
	uint16_t length;
};

struct eapol_key {
	uint8_t type;
	uint8_t key_info[2];
	uint8_t key_length[2];
	uint8_t replay_counter[8];
	uint8_t key_nonce[32];
	uint8_t key_iv[16];
	uint8_t key_rsc[8];
	uint8_t key_id[8];
	uint8_t key_mic[16];
	uint16_t key_data_length;
} __attribute__((packed)) ;

enum eapol_msg {
	EAPOL_MSG_NONE = 0,
	EAPOL_MSG_M1,
	EAPOL_MSG_M2,
	EAPOL_MSG_M3,
	EAPOL_MSG_M4,
	EAPOL_MSG_MAX
};

int ieee80211_ver(void* frame);
bool ieee80211_is_pv0(void* frame);
bool ieee80211_is_pv1(void* frame);
bool ieee80211_group_addr(uint8_t *addr);
bool ieee80211_broadcast_addr(uint8_t *addr);
bool ieee80211_null_addr(uint8_t *addr);
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
bool ieee80211_is_assoc_req(GenericMacHeader* gmh);
bool ieee80211_is_reassoc_req(GenericMacHeader* gmh);
bool ieee80211_is_assoc_resp(GenericMacHeader* gmh);
bool ieee80211_is_reassoc_resp(GenericMacHeader* gmh);
bool ieee80211_is_action(GenericMacHeader* gmh);
bool ieee80211_is_action_noack(GenericMacHeader* gmh);
bool ieee80211_is_protected(GenericMacHeader* gmh);
bool ieee80211_is_s1g_beacon(GenericMacHeader *gmh);
bool ieee80211_is_frag(GenericMacHeader* gmh);
bool ieee80211_is_last_frag(GenericMacHeader* gmh);
bool ieee80211_has_morefrags(GenericMacHeader* gmh);
bool ieee80211_is_amsdu(GenericMacHeader* gmh);
uint8_t ieee80211_is_eapol(GenericMacHeader *gmh, int len);
bool ieee80211_has_htc(GenericMacHeader* gmh);
uint8_t* ieee80211_get_bssid(GenericMacHeader *gmh);
bool ieee80211_is_arp(GenericMacHeader* gmh);
bool ieee80211_is_dhcp(GenericMacHeader* gmh);
#if defined (INCLUDE_AVOID_FRAG_ATTACK_TEST)
bool ieee80211_is_icmp(GenericMacHeader *gmh);
#endif
bool ieee80211_is_robust(GenericMacHeader* gmh,uint32_t mpdu_len);
bool ieee80211_is_ip(GenericMacHeader* gmh);

bool ieee80211_has_fromds(GenericMacHeader* gmh);
bool ieee80211_has_tods(GenericMacHeader* gmh);
uint8_t* ieee80211_da(GenericMacHeader* gmh);
bool ieee80211_is_any_mgmt(void *mh);

const uint8_t* broadcast_addr();
bool is_broadcast_addr(const uint8_t* addr);
bool is_equal_mac_addr(uint8_t *addr1, uint8_t *addr2);

uint8_t get_mesh_control_length(GenericMacHeader* gmh);
uint8_t ieee80211_mhd_length(GenericMacHeader* gmh);
uint8_t ieee80211_get_tid(GenericMacHeader* gmh);
void ieee80211_set_qos_ack_policy (GenericMacHeader* gmh, int ack);
uint8_t ieee80211_get_qos_ack_policy (GenericMacHeader* gmh);
bool ieee80211_has_a4(GenericMacHeader * gmh);
#endif /* __PROTOCOL_H__ */
