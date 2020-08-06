#ifndef __PROTOCOL_S1G_H__
#define __PROTOCOL_S1G_H__
/************************************************************************************************************/
/*						 				Structures for 802.11ah HaLow										*/
/************************************************************************************************************/

#define NDP_MAC_FRAME_PROBE_REQ_TYPE			7

struct S1GGenericFrameHeader {
	/* Frame Control fields (16bits) */
	uint16_t version        : 2;    /* default 0 */
	uint16_t type           : 2;    /* 0: Management , 1: Control,  2: Data,  3: Extension */
	uint16_t subtype        : 4;

	uint16_t to_ds          : 1;    /* to AP */
	uint16_t from_ds        : 1;    /* from AP */
	uint16_t more_frag      : 1;
	uint16_t retry          : 1;
	uint16_t pwr_mgt        : 1;
	uint16_t more_data      : 1;
	uint16_t protect        : 1;
	uint16_t order          : 1;

	uint16_t duration_id;
};

struct S1GControlFrameHeader {
	/* Frame Control fields (16bits) */
	uint16_t version        : 2;    /* default 0 */
	uint16_t type           : 2;    /* 0: Management , 1: Control,  2: Data,  3: Extension */
	uint16_t subtype        : 4;

	uint16_t bw_ind         : 3;
	uint16_t dynamic_ind    : 1;
	uint16_t pwr_mgt        : 1;
	uint16_t more_data      : 1;
	uint16_t protect        : 1;
	uint16_t order          : 1;

	uint16_t duration_id;
};

struct S1GControlFrameHeaderST3 {
	/* Frame Control fields (16bits) */
	uint16_t version        : 2;    /* 0: PV0 */
	uint16_t type           : 2;    /* 1: Control */
	uint16_t subtype        : 4;	/* 3: TACK */

	uint16_t bw_ind         : 3;
	uint16_t dynamic_ind    : 1;
	uint16_t next_twt_info  : 1;
	uint16_t more_data      : 1;
	uint16_t flow_control	: 1;
	uint16_t reserved		: 1;

	uint16_t duration_id;

};

struct S1GControlFrameHeaderST10 {
	/* Frame Control fields (16bits) */
	uint16_t version        : 2;    /* default 0: PV0 */
	uint16_t type           : 2;    /* 1: Control */
	uint16_t subtype        : 4;	/* 10: PS-Poll */
	uint16_t bw_ind         : 3;
	uint16_t dynamic_ind    : 1;
	uint16_t pwr_mgt        : 1;
	uint16_t more_data      : 1;
	uint16_t poll_type		: 2;
	uint16_t duration_id;
};

typedef struct _S1GExtensionFrameHeader {
	/* Frame Control fields (16bits) */
	uint16_t version        	: 2;    /* default 0 */
	uint16_t type           	: 2;    /* 0: Management , 1: Control,  2: Data,  3: Extension */
	uint16_t subtype       		: 4;

	uint16_t next_tbtt_present	: 1;
	uint16_t cssid_present		: 1;
	uint16_t ano_present		: 1;
	uint16_t bss_bw				: 3;
	uint16_t security			: 1;
	uint16_t ap_pm				: 1;

	uint16_t duration_id;
	uint8_t  sa[6];
	uint32_t timestamp;
	uint8_t  change_sequence;
} __attribute__((packed)) S1GExtensionFrameHeader;


typedef struct _S1GBeaconExtendedHeader {
	uint8_t		next_tbtt_lo;
	uint16_t 	next_tbtt_hi;
	uint32_t	comp_ssid;
}__attribute__((packed)) S1GBeaconExtendedHeader;

/* PV1 */
typedef struct _PV1SID {
	uint16_t	aid:13;
	uint16_t	a3_present:1;
	uint16_t	a4_present:1;
	uint16_t	amsdu:1;
} PV1SID;

typedef struct _PV1AddressDS0 {
	uint8_t		a1[6];
	PV1SID		a2;
} PV1AddressDS0;

typedef struct _PV1AddressDS1 {
	PV1SID		a1;
	uint8_t		a2[6];
} PV1AddressDS1;

typedef struct _PV1QoSData3Header {
	/* Frame Control fields (16bits) */
	uint16_t version        : 2;    /* 1: PV1 */
	uint16_t type           : 3;    /* 0: QoS Data OR 1: Management */
	uint16_t subtype        : 3;

	uint16_t from_ds		: 1;
	uint16_t more_frag		: 1;
	uint16_t pwr_mgt		: 1;
	uint16_t more_data		: 1;
	uint16_t protect		: 1;
	uint16_t eosp			: 1;
	uint16_t relayed_frame	: 1;
	uint16_t ack_policy		: 1;

	uint8_t a1[6]; /* RA */
	uint8_t a2[6]; /* TA */

	/* Sequence Control (16bits) */
	uint16_t fragment_number    : 4;
	uint16_t sequence_number    : 12;

	uint8_t a3[0];
} PV1QoSData3Header;

typedef struct _PV1GenericFrameHeader {
	/* Frame Control fields (16bits) */
	uint16_t version        : 2;    /* 1: PV1 */
	uint16_t type           : 3;    /* 0: QoS Data OR 1: Management */
	uint16_t subtype        : 3;

	uint16_t from_ds		: 1;
	uint16_t more_frag		: 1;
	uint16_t pwr_mgt		: 1;
	uint16_t more_data		: 1;
	uint16_t protect		: 1;
	uint16_t eosp			: 1;
	uint16_t relayed_frame	: 1;
	uint16_t ack_policy		: 1;

	union {
		PV1AddressDS0	address_ds0;
		PV1AddressDS1	address_ds1;
	};

	/* Sequence Control (16bits) */
	uint16_t fragment_number    : 4;
	uint16_t sequence_number    : 12;

	uint8_t a3[0];
} PV1GenericFrameHeader;

/* for PV1 probe response */
typedef struct _PV1ManagementFrameHeader {
	/* Frame Control fields (16bits) */
	uint16_t version        	: 2;
	uint16_t type           	: 3;
	uint16_t subtype       		: 3;

	uint16_t next_tbtt_present	: 1;
	uint16_t cssid_present		: 1;
	uint16_t ano_present		: 1;
	uint16_t bss_bw				: 3;
	uint16_t security			: 1;
	uint16_t ap_pm				: 1;

	uint8_t a1[6];
	uint8_t a2[6];

	uint32_t timestamp;
	uint8_t  change_sequence;
} __attribute__((packed)) PV1ManagementFrameHeader;

/*=============================================================================
	NDP CMAC Frame format
=============================================================================*/

/* 1M NDP formats */

/* NDP CTS 1M (25 bits) */
typedef struct _NdpCTS1M {
	uint32_t type                   : 3;
	uint32_t cts_cfend_ind          : 1;
	uint32_t addr_ind               : 1;
	uint32_t ra_partial_bssid       : 9;
	uint32_t duration               : 10;
	uint32_t early_sector_ind       : 1;
	uint32_t ndp_ind                : 1;        /* 1 for NDP */
	uint32_t reserved               : 6;
} NdpCTS1M;


/* NDP CF-END 1M (25 bits) */
typedef struct _NdpCFEnd1M {
	uint32_t type                   : 3;
	uint32_t cts_cfend_ind          : 1;
	uint32_t partial_bssid          : 9;
	uint32_t duration               : 10;
	uint32_t reserved1              : 2;
	uint32_t ndp_ind                : 1;        /* 1 for NDP */
	uint32_t reserved2              : 6;
} NdpCFEnd1M;

/* NDP PS-Poll 1M (25 bits) */
typedef struct _NdpPSPoll1M {
	uint32_t type                   : 3;
	uint32_t ra                     : 9;
	uint32_t ta                     : 9;
	uint32_t preferred_mcs          : 3;
	uint32_t udi                    : 1;
	uint32_t ndp_ind                : 1;        /* 1 for NDP */
	uint32_t reserved               : 6;

} NdpPSPoll1M;


/* NDP ACK 1M (25 bits) */
typedef struct _NdpAck1M {
	uint32_t type                   : 3;
	uint32_t ack_id                 : 9;
	uint32_t more_data              : 1;
	uint32_t idle_ind               : 1;
	uint32_t duration               : 10;
	uint32_t relayed_frame          : 1;
	uint32_t ndp_ind                : 1;        /* 1 for NDP */
	uint32_t reserved               : 6;

} NdpAck1M;

/* NDP PS-Poll ACK 1M (25 bits) */
typedef struct _NdpPSPollAck1M {
	uint32_t type                   : 3;
	uint32_t ack_id                 : 9;
	uint32_t more_data              : 1;
	uint32_t idle_ind               : 1;
	uint32_t duration               : 10;
	uint32_t reserved1              : 1;
	uint32_t ndp_ind                : 1;        /* 1 for NDP */
	uint32_t reserved2              : 6;

} NdpPSPollAck1M;

/* NDP Block ACK 1M (25 bits) */
typedef struct _NdpBlockAck1M {
	uint32_t type                   : 3;
	uint32_t ack_id                 : 2;
	uint32_t ssn                    : 12;
	uint32_t bitmap                 : 8;
	uint32_t ndp_ind                : 1;        /* 1 for NDP */
	uint32_t reserved               : 6;

} NdpBlockAck1M;

/* NDP Paging 1M (25 bits) */
typedef struct _NdpPaging1M {
	uint32_t type					: 3;
	uint32_t pid					: 9;
	uint32_t apdi_paid				: 9;
	uint32_t direction				: 1;
	uint32_t reserved1              : 3;
	uint32_t ndp_ind				: 1;
	uint32_t reserved2				: 6;
} NdpPaging1M;

/* NDP Probe Req 1M (25 bits) */
typedef struct _NdpProbeReq1M {
	uint32_t type					: 3;
	uint32_t cssid_ano_present		: 1;
	uint32_t comp_ssid_ano			: 16;
	uint32_t requested_type			: 1;
	uint32_t reserved1				: 4;
	uint32_t ndp_ind				: 1;		/* 1 for NDP */
	uint32_t reserved2				: 6;
} NdpProbeReq1M;

/* 2M NDP formats */
/* NDP CF-END 2M (37 bits) */
typedef struct _NdpCFEnd2M {
	uint32_t type                   : 3;
	uint32_t cts_cfend_ind          : 1;
	uint32_t partial_bssid          : 9;
	uint32_t duration_low           : 11;
	uint32_t reserved1              : 8;

	uint32_t duration_high          : 4;
	uint32_t reserved2              : 9;
	uint32_t ndp_ind                : 1;        /* 1 for NDP */
	uint32_t reserved3              : 18;
} NdpCFEnd2M;


/* NDP CTS 2M (37 bits) */
typedef struct _NdpCTS2M {
	uint32_t type                   : 3;
	uint32_t cts_cfend_ind          : 1;
	uint32_t addr_ind               : 1;
	uint32_t ra_partial_bssid       : 9;
	uint32_t duration_low 	        : 10;
	uint32_t reserved1              : 8;

	uint32_t duration_high          : 5;
	uint32_t early_sector_ind       : 1;
	uint32_t bw_ind                 : 3;
	uint32_t reserved2              : 4;
	uint32_t ndp_ind                : 1;        /* 1 for NDP */
	uint32_t reserved3              : 18;
} NdpCTS2M;


/* NDP PS-Poll 2M (37 bits) */
typedef struct _NdpPSPoll2M {
	uint32_t type                   : 3;
	uint32_t ra                     : 9;
	uint32_t ta                     : 9;
	uint32_t preferred_mcs_low      : 3;
	uint32_t reserved1              : 8;

	uint32_t preferred_mcs_high     : 1;
	uint32_t udi                    : 12;
	uint32_t ndp_ind                : 1;        /* 1 for NDP */
	uint32_t reserved2              : 18;
} NdpPSPoll2M;


/* NDP ACK 2M (37 bits) */
typedef struct _NdpAck2M {
	uint32_t type                   : 3;
	uint32_t ack_id                 : 16;
	uint32_t more_data              : 1;
	uint32_t idle_ind               : 1;
	uint32_t duration_low	        : 3;
	uint32_t reserved1              : 8;

	uint32_t duration_high           : 11;
	uint32_t relayed_frame          : 1;
	uint32_t reserved2              : 1;
	uint32_t ndp_ind                : 1;        /* 1 for NDP */
	uint32_t reserved3              : 18;
} NdpAck2M;

/* NDP PS-Poll ACK 2M (37 bits) */
typedef struct _NdpPSPollAck2M {
	uint32_t type                   : 3;
	uint32_t ack_id                 : 16;
	uint32_t more_data              : 1;
	uint32_t idle_ind               : 1;
	uint32_t duration_low           : 3;
	uint32_t reserved1              : 8;

	uint32_t duration_high          : 11;
	uint32_t reserved2              : 2;
	uint32_t ndp_ind                : 1;        /* 1 for NDP */
	uint32_t reserved3              : 18;
} NdpPSPollAck2M;

/* NDP Block ACK 2M (37 bits) */
typedef struct _NdpBlockAck2M {
	uint32_t type                   : 3;
	uint32_t ack_id                 : 6;
	uint32_t ssn                    : 12;
	uint32_t bitmap_low            : 3;
	uint32_t reserved1              : 8;

	uint32_t bitmap_high             : 13;
	uint32_t ndp_ind                : 1;        /* 1 for NDP */
	uint32_t reserved2              : 18;
} NdpBlockAck2M;

/* NDP Paging 2M (37 bits) */
typedef struct _NdpPaging2M {
	uint32_t type					: 3;
	uint32_t pid					: 9;
	uint32_t apdi_paid				: 9;
	uint32_t direction				: 1;
	uint32_t reserved1              : 10;

	uint32_t reserved2				: 13;
	uint32_t ndp_ind				: 1;
	uint32_t reserved3				: 18;
} NdpPaging2M;

/* NDP Probe Req 2M (37 bits) */
typedef struct _NdpProbeReq2M {
	uint32_t type					: 3;
	uint32_t cssid_ano_present		: 1;
	uint32_t comp_ssid_ano_low		: 20;
	uint32_t reserved1				: 8;

	uint32_t comp_ssid_ano_high		: 12;
	uint32_t requested_type			: 1;
	uint32_t ndp_ind				: 1;		/* 1 for NDP */
	uint32_t reserved2				: 18;

} NdpProbeReq2M;


/* NDP Beamforming Report Poll 2M (37 bits) */
typedef struct _NdpBFReportPoll2M {
	uint32_t type                   : 3;
	uint32_t ap_addr                : 9;
	uint32_t non_ap_addr_low   	    : 12;
	uint32_t reserved1              : 8;

	uint32_t non_ap_addr_high       : 1;
	uint32_t fb_segment_retx_bitmap : 8;
	uint32_t reserved               : 4;
	uint32_t ndp_ind                : 1;        /* 1 for NDP */
	uint32_t reserved3              : 18;
} NdpBFReportPoll2M;

/*=============================================================================
	S1G Control frame format
	PV1 Control frame format
=============================================================================*/

struct	TACKData {
	uint32_t	ra_low					: 32;
	uint32_t	ra_high					: 16;
	uint32_t	ta_low					: 16;
	uint32_t	ta_high					: 32;

	uint32_t	bcn_sequence			: 8;
	uint32_t	partial_timestamp_low	: 8;
	uint32_t	partial_timestamp_mid	: 16;
	uint32_t	partial_timestamp_high	: 16;
	uint32_t	next_twt_info_low		: 16;
	uint32_t	next_twt_info_high;

};

struct	BlockAckReqData {
	uint32_t	ra_low;
	uint32_t	ra_high			: 16;
	uint32_t	ta_low			: 16;
	uint32_t	ta_high;

	uint32_t	bar_ack_policy	: 1;
	uint32_t	multi_tid		: 1;
	uint32_t	comp_bitmap		: 1;
	uint32_t	gcr				: 1;
	uint32_t	reserved		: 8;
	uint32_t	tid_info		: 4;

	uint32_t	ba_info_high	: 16;
	uint32_t	ba_info_low;

};

struct	BlockAckData {
	uint32_t	ra_low;
	uint32_t	ra_high			: 16;
	uint32_t	ta_low			: 16;
	uint32_t	ta_high;

	uint32_t	ba_ack_policy	: 1;
	uint32_t	multi_tid		: 1;
	uint32_t	comp_bitmap		: 1;
	uint32_t	gcr				: 1;
	uint32_t	reserved		: 8;
	uint32_t	tid_info		: 4;

	uint32_t	ba_seq_control	: 16;
	uint32_t 	ba_bitmap_low;
	uint32_t	ba_bitmap_high;

};

struct PSPollData {
	uint32_t	ra_low;
	uint32_t	ra_high			: 16;
	uint32_t	ta_low			: 16;
	uint32_t	ta_high;

};

struct  RTSData {
	uint32_t	ra_low;
	uint32_t	ra_high			: 16;
	uint32_t	ta_low			: 16;
	uint32_t	ta_high;
};

struct  CTSData {
	uint32_t	ra_low;
	uint32_t	ra_high			: 16;
	uint32_t	reserved 		: 16;
};

struct  ACKData {
	uint32_t	ra_low;
	uint32_t	ra_high			: 16;
	uint32_t	reserved		: 16;
};

struct  CFEndData {
	uint32_t	ra_low;
	uint32_t	ra_high			: 16;
	uint32_t	ta_high			: 16;
	uint32_t	ta_low;
};

struct  CFEndAckData {
	uint32_t	ra_low;
	uint32_t	ra_high			: 16;
	uint32_t	ta_high			: 16;
	uint32_t	ta_low;
};

struct STACKData {
	uint32_t	A1				: 16;
	uint32_t	A2				: 16;
	uint32_t	next_twt_info;
};

struct BATData {
	uint32_t	A1					: 16;
	uint32_t	A2_low				: 16;
	uint32_t	A2_high;
	uint32_t	beacon_seq			: 8;
	uint32_t	pent_timestamp_low	: 8;
	uint32_t	pent_timestamp_mid	: 16;
	uint32_t	pent_timestamp_high : 16;
	uint32_t	next_twt_info_low	: 16;
	uint32_t	next_twt_info_high;
	uint32_t 	starting_seq_control: 16;
	uint32_t	BAT_bitmap_low		: 16;
	uint32_t	BAT_bitmap_high		: 16;
	uint32_t	reserved			: 16;
};


/*=============================================================================
	SIG structure
=============================================================================*/

/* SIG for S1G 1M */
typedef struct _SigS1G1M {
	/* sig bit pattern LSB -> MSB */
	uint32_t nsts               : 2;        /* 0=1 space time stream, 1=2 space time stream, etc. */
	uint32_t short_gi           : 1;        /* 0 for no use of short guard interval, otherwise 1 */
	uint32_t coding             : 2;        /* x0=BCC, x1=LDPC */
	uint32_t stbc               : 1;        /* 0=no spatial streams has STBC, 1=all spatial streams has STBC */
	uint32_t reserved2          : 1;
	uint32_t mcs                : 4;
	uint32_t aggregation        : 1;        /* 1 for aggregation */
	uint32_t length             : 9;        /* PPDU length in symbols in case of aggregation, otherwise, byte length */
	uint32_t response_ind       : 2;        /* 0=no rsp, 1=NDP rsp, 2=normal rsp, 3=long rsp */
	uint32_t smoothing          : 1;        /* 1 for recommending channel smoothing */
	uint32_t doppler            : 1;        /* 0=regular pilot tone, 1=traveling pilots */
	uint32_t ndp_ind            : 1;        /* 1 for NDP */
	uint32_t crc                : 4;        /* for TX use (CRC calulation for S1G SIGA fields) */
	uint32_t reserved1          : 2;

	uint32_t magic_code_3       : 8;
	uint32_t magic_code_2       : 8;
	uint32_t magic_code_1       : 8;
	uint32_t magic_code_0       : 8;
} SigS1G1M;

/* SIG for S1G SHORT */
typedef struct _SigS1GShort {
	/* sig bit pattern LSB -> MSB */
	uint32_t reserved2          : 1;
	uint32_t stbc               : 1;
	uint32_t uplink_ind         : 1;    /* set to the value of TXVECTOR parameter UPLINK_INDICATION */
	uint32_t bandwidth          : 2;    /* 0:2M, 1:4M, 2:8M, 3:16M */
	uint32_t nsts               : 2;
	uint32_t id                 : 9;
	uint32_t short_gi           : 1;
	uint32_t coding             : 2;
	uint32_t mcs                : 4;
	uint32_t smoothing          : 1;
	uint32_t reserved1          : 8;

	uint32_t aggregation        : 1;
	uint32_t length             : 9;
	uint32_t response_ind       : 2;
	uint32_t doppler            : 1;
	uint32_t ndp_ind            : 1;
	uint32_t crc                : 4;    /* for TX use */
	uint32_t reserved3          : 14;

} SigS1GShort;

/* SIG for S1G LONG */
typedef struct _SigS1GLong {

	/* sig bit pattern LSB -> MSB */
	uint32_t mu_su              : 1;
	uint32_t stbc               : 1;
	uint32_t uplink_ind         : 1;
	uint32_t bandwidth          : 2;    /* 0:2M, 1:4M, 2:8M, 3:16M */
	uint32_t nsts               : 2;
	uint32_t id                 : 9;
	uint32_t short_gi           : 1;
	uint32_t coding             : 2;
	uint32_t mcs                : 4;
	uint32_t smoothing          : 1;
	uint32_t reserved1          : 8;

	uint32_t aggregation        : 1;
	uint32_t length             : 9;
	uint32_t response_ind       : 2;
	uint32_t reserved3          : 1;
	uint32_t doppler            : 1;
	uint32_t crc                : 4;    /* for TX use */
	uint32_t reserved2          : 14;
} SigS1GLong;

/* SIG for NDP 1M */
typedef struct _SigNdp1M {
	union {
		NdpCTS1M        cts;
		NdpCFEnd1M      cfend;
		NdpPSPoll1M     pspoll;
		NdpAck1M        ack;
		NdpPSPollAck1M  pspoll_ack;
		NdpBlockAck1M   block_ack;
		NdpProbeReq1M	probe_req;
	} body;

	uint32_t reserved;
} SigNdp1M;


typedef struct _SigNdp2M {
	union {
		NdpCFEnd2M          cfend;
		NdpCTS2M            cts;
		NdpPSPoll2M         pspoll;
		NdpAck2M            ack;
		NdpPSPollAck2M      pspoll_ack;
		NdpBlockAck2M       block_ack;
		NdpProbeReq2M		probe_req;
		NdpBFReportPoll2M   bfr_poll;
	} body;
} SigNdp2M;

bool lmac_check_pv1_qos_data_frame(PV1GenericFrameHeader *pv1_hdr);
bool lmac_check_pv1_qos_data3_frame(PV1GenericFrameHeader *pv1_hdr);
uint8_t		lmac_s1g_beacon_timestamp_pos(S1GExtensionFrameHeader *header);
uint8_t		lmac_s1g_beacon_bcn_compatible_pos(S1GExtensionFrameHeader *header);
uint8_t		lmac_s1g_beacon_header_offset(S1GExtensionFrameHeader *header);

bool ieee80211ah_is_ndp_preq(void *sig);
#endif /* __PROTOCOL_S1G_H__ */
