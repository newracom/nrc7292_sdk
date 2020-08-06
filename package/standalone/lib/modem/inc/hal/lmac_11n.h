#ifndef HAL_LMAC_11N_H
#define HAL_LMAC_11N_H

#include "hal_lmac_register.h"
#include "nrc-wim-types.h"

#define MAX_PPDU_NUMBER             64
#define MAX_AGG_SCHED_NUMBER        64
#define STAT_AGG_MAX                64

#define MAX_TID		8

#define NOW     (*(volatile uint32_t*)(MAC_REG_TSF_0_LOWER_READONLY))
#define NOW1     (*(volatile uint32_t*)(MAC_REG_TSF_1_LOWER_READONLY))
#define TSF     (*(volatile uint32_t*)(MAC_REG_TSF_0_LOWER_READONLY))
#define TSF1    (*(volatile uint32_t*)(MAC_REG_TSF_1_LOWER_READONLY))

// Descriptor macro for DL and RX/TX descriptors
#define SW_OWNED(desc)          (!((desc) & MAC_REG_RX_WRITE_EN_DL_DESC0_OWNERSHIP_MASK))
#define HW_OWNED(desc)          ((desc) & MAC_REG_RX_WRITE_EN_DL_DESC0_OWNERSHIP_MASK)
#define FIRST_FRAGMENT(desc)    (!((desc) & (1 << MAC_REG_RX_WRITE_EN_DL_DESC0_FRAGMENT_SHIFT)))
#define LAST_FRAGMENT(desc)     (!((desc) & (1 << (MAC_REG_RX_WRITE_EN_DL_DESC0_FRAGMENT_SHIFT + 1))))
// macro for address calculation
#define BUFFER_ADDRESS(base, offset)    ((base) + ((offset) << 2))
#define OFFSET_ADDRESS(base, addr)      ((((addr) - (base)) >> 2) & MAC_REG_RX_REG_DL_DESC0_DATA_ADDRESS_OFFSET_MASK)

enum {
	MAX_SEQUENCE_NUMBER = 1 << 12,
	MAX_HT_PSDU_LEN     = 65535,
};
enum ch_width {
	BW_20M = 0,
	BW_40M,
	BW_80M,
	BW_MAX,
};

enum {
	LONG_PREAMBLE = 0,
	SHORT_PREAMBLE,
};

enum {
	MAX_MCS             = 8, // 7
	MAX_DSSS_INDEX      = 4,
};

enum {
	FORMAT_11AG         = 0,
	FORMAT_11B          = 1,
	FORMAT_NONHT_DUP    = 2,
	FORMAT_HT_MIXED     = 3,
	FORMAT_VHT          = 4,
	FORMAT_MAX,
};


enum {
	CODING_BCC = 0,
	CODING_LDPC = 1,
};

enum {
	PER_SYMBOL 				= 4,
	SIFS_DURATION 			= 16,
	SLOT_DURATION 			= 9,
	L_STF 					= 8,
	L_LTF 					= 8,
	L_SIG 					= 4,
	HT_STF 					= 4,
	HT_LTF 					= 4,
	HT_SIG 					= 8,
	SERVICE_SCRAMBLER_TAIL 	= 4,

	DSSS_PLCP_PREAMBLE 		= 144,
	DSSS_PLCP_HEADER 		= 48,
	DSSS_PLCP_LENGTH 		= DSSS_PLCP_PREAMBLE + DSSS_PLCP_HEADER, // 192usec

	NON_HT_PLCP_LENGTH = L_STF + L_LTF + L_SIG, // [TBD] include service field, Scrambler, tail, pad
	NON_HT_HEADER_LENGTH = NON_HT_PLCP_LENGTH + SERVICE_SCRAMBLER_TAIL,

	HT_HEADER_LENGTH = L_STF + L_LTF + L_SIG + HT_SIG + HT_STF + HT_LTF,
};

enum {
	QUEUE_REAR,
	QUEUE_FRONT
};

enum{
	CH_1 = 1,
	CH_2,
	CH_3,
	CH_4,
	CH_5,
	CH_6,
	CH_7,
	CH_8,
	CH_9,
	CH_10,
	CH_11,
	CH_12,
	CH_13,
	CH_14,
	CH_15,
	CH_16,
	CH_17,
	CH_18,

};

///=============================================================================
///
/// Duration Calculation
///
///=============================================================================
/*
Format = HT(3) or VHT(4)

HT Table for 20MHz width
--------------------------------------------------------------------------------
MCS    Modulation    Code Rate    N_ss    N_dbps    PHY Rate(Long/Short)(Mbps)
--------------------------------------------------------------------------------
 0       BPSK           1/2         1       26             6.5 / 7.2
 1       QPSK           1/2         1       52            13.0 / 14.4
 2       QPSK           3/4         1       78            19.5 / 21.7
 3      16-QAM          1/2         1      104            26.0 / 28.9
 4      16-QAM          3/4         1      156            39.0 / 43.3
 5      64-QAM          2/3         1      208            52.0 / 57.8
 6      64-QAM          3/4         1      234            58.5 / 65.0
 7      64-QAM          5/6         1      260            65.0 / 72.2
--------------------------------------------------------------------------------
HT Table for 40MHz width
--------------------------------------------------------------------------------
MCS    Modulation    Code Rate    N_ss    N_dbps    PHY Rate(Long/Short)(Mbps)
--------------------------------------------------------------------------------
 0       BPSK           1/2         1       54            13.5 / 15.0
 1       QPSK           1/2         1      108            27.0 / 30.0
 2       QPSK           3/4         1      162            40.5 / 45.0
 3      16-QAM          1/2         1      216            54.0 / 60.0
 4      16-QAM          3/4         1      324            81.0 / 90.0
 5      64-QAM          2/3         1      432           108.0 / 120.0
 6      64-QAM          3/4         1      486           121.5 / 135.0
 7      64-QAM          5/6         1      540           135.0 / 150.0
--------------------------------------------------------------------------------
*/

static const uint16_t ht_bits_per_symbol[MAX_MCS] =
{26, 52, 78, 104, 156, 208, 234, 260};

static const uint16_t ht_phy_rate[MAX_MCS] =
{6500, 13000, 19500, 26000, 39000, 52000, 58500, 65000}; // kbps

static const uint8_t non_ht_bits_per_symbol[MAX_MCS] =
{24, 36, 48,  72,  96, 144, 192, 216};

static const uint16_t non_ht_phy_rate[MAX_MCS] =
{6000, 9000, 12000, 18000, 24000, 36000, 48000, 54000}; // kbps

static const uint8_t dsss_phy_rate[MAX_DSSS_INDEX] =
{1.0, 2.0, 5.5, 11.0};

/* for MCS fields

Format = 11b(1)
Legacy Rate(11b)
    0x0 : 1 Mbps
    0x1 : 2 Mbps
    0x2 : 5.5 Mbps
    0x3 : 11 Mbps
---------------------------------------------------
Modulation    PHY Rate(Mbps)
---------------------------------------------------
 DBPSK              1.0
 DQPSK              2.0
 DQPSK              5.5
 DQPSK             11.0
---------------------------------------------------

Format = Legacy OFDM(0) or NON_HT Duplicate(2)
Legacy Rate(11a/g)
    0x0 : 6 Mbps
    0x1 : 9 Mbps
    0x2 : 12 Mbps
    0x3 : 18 Mbps
    0x4 : 24 Mbps
    0x5 : 36 Mbps
    0x6 : 48 Mbps
    0x7 : 54 Mbps
--------------------------------------------------------------------------------
Modulation      Code Rate      N_dbps      PHY Rate(Mbps)
--------------------------------------------------------------------------------
   BPSK            1/2           24              6
   BPSK            3/4           36              9
   QPSK            1/2           48             12
   QPSK            3/4           72             18
  16-QAM           1/2           96             24
  16-QAM           3/4          144             36
  64-QAM           2/3          192             48
  64-QAM           3/4          216             54
--------------------------------------------------------------------------------
*/

enum {
	LGI_BW20M = 0,
	SGI_BW20M,
	//	LGI_BW40M,
	//	SGI_BW40M,
	XGI_MAX
};

enum {
	LGI = 0,
	SGI = 1,
};

static const uint16_t max_ul_bytes[XGI_MAX][MAX_MCS] = {
	/* LGI BW20M */
	{4423, 8850,  13276,  17703,  26556,  35409,  39835,  44262},
	/* SGI BW20M */
	{4914, 9831,  14749,  19666,  29500,  39335,  44252,  49169},
};

uint32_t* max_ul_table(int, int);
uint16_t compute_duration(uint16_t, uint8_t, uint8_t, bool);
uint16_t compute_symbol(uint16_t, uint8_t, uint8_t);

typedef enum {
	ACI_BK = 0,
	ACI_BE,
	ACI_VI,
	ACI_VO,
	ACI_BEACON,
	ACI_CONC,
	ACI_MAX
} ACI;
typedef enum {
	VIF0_BK = 0,
	VIF0_BE,
	VIF0_VI,
	VIF0_VO,
	VIF0_BEACON,
	VIF_CONC,
	VIF1_BK,
	VIF1_BE,
	VIF1_VI,
	VIF1_VO,
	VIF1_BEACON,
	MAX_AC
} VIFAC;

static const uint8_t AIFSN[MAX_AC] = { 7,  3,  2,  2,  1,  1,  7,  3,  2,  2,  1};
static const uint8_t CWMIN[MAX_AC] = { 5,  5,  4,  3,  0,  0,  5,  5,  4,  3,  0};
static const uint8_t CWMAX[MAX_AC] = {10, 10,  5,  4,  0,  0,  10, 10,  5,  4,  0};

// microsecond unit
//static const uint16_t TXOPLIMIT[MAX_AC] = {2080, 2080, 4096, 2080, 4096, 0, 2080, 2080, 4096, 2080, 4096};
static const uint16_t TXOPLIMIT[MAX_AC] = {2080, 2080, 4096, 2080, 0xffff, 0, 2080, 2080, 4096, 2080, 0xffff};
//static const uint16_t TXOPLIMIT[MAX_AC] = {0, 0, 4096, 2080, 0xffff, 0, 2080, 2080, 4096, 2080, 0xffff}; // TEST


///=============================================================================
/// TXVECTOR for transmission
///=============================================================================
typedef struct _TXVECTOR{
	// Word 9,10,11 exist only at first fragment (segment == 2'b01)
	// Word 9 : Vector
	uint32_t    format         : 3;        // 0=11a/g, 1=11b, 2=Non HT duplicate, 3=HT(11n, fixed format), 4=VHT
#ifndef FPGA
		uint32_t    reserved7      : 1;
		uint32_t    tx_power       : 12;
		uint32_t    reserved0      : 6;
#else
		uint32_t    reserved7      : 16;
		uint32_t    tx_power       : 3;
#endif
	uint32_t    bandwidth      : 2;        // 0=20, 1=40, 2=80

	uint32_t    reserved8      : 1;
	uint32_t    dynamic_bw     : 1;        // 0=static, 1=dynamic
	uint32_t    reserved9      : 2;
	uint32_t    gi_type        : 1;        // for 11b, 0=long preamble, 1=short preamble
	// for non 11b, 0=long GI, 1=short GI
	uint32_t    reserved10     : 3;

	// Word 10 : SIG0
	uint32_t    reserved11     : 2;
	uint32_t    smoothing      : 1;
	uint32_t    aggregation    : 1;
	uint32_t    fec_coding     : 1;        // 0=BCC, 1=LDPC
	uint32_t    beamformed     : 1;        // 0=not apply, 1=apply
	uint32_t    txop_ps_not    : 1;        // 0=allow, 1=not allow
	uint32_t    reserved12     : 1;

	uint32_t    ppdu_length    : 20;       // PPDU length after scheduling
	uint32_t    reserved13     : 4;

	// Word 11 : SIG 1
	uint32_t    mcs            : 7;
	uint32_t    reserved14     : 1;
	uint32_t    group_id       : 1;
	uint32_t    reserved15     : 7;
	uint32_t    partial_aid    : 9;
	uint32_t    reserved16     : 7;

} TXVECTOR , PHY_TXVECTOR;

typedef struct _LMAC_TXHDR {
	// Word 0
	uint8_t temp0[4];
	// Word 1
	uint32_t temp1;
	//Word 2
	struct _LMAC_TXBUF  *m_next;
	//Word 3
	struct _LMAC_TXBUF  *m_link;
	union {
		struct {
			//Word 4 : BD info 0
			uint32_t    n_ppdu              : 6;
			uint32_t    ack_policy          : 2;
			uint32_t    cipher_type         : 3;        // 0:wep40, 1:wep128, 2:tkip, 3:ccmp, 4:wapi
			uint32_t    rts_request         : 1;
			uint32_t    cts_self_request    : 1;
			uint32_t    cf_end_request      : 1;
			uint32_t    reserved1           : 4;        // security key location
			uint32_t    timestamp_update    : 1;        // flag for updating timestamp in the frame
			uint32_t    timestamp_position  : 6;        // in byte, timestamp position in frame
			uint32_t	bssid_idx			: 2;
			uint32_t	mac_idx				: 2;
			uint32_t    reserved2           : 3;

			// Word 5 : BD info 1
			uint32_t    rts_duration        : 16;
			uint32_t	key_location		: 10;
			uint32_t    reserved3           : 6;

			// Word 6 : BD info 2
			uint32_t    reserved4;

			// Word 7 : BD info 3
			uint32_t    mpdu_length         : 14;
			uint32_t    ampdu_segment       : 2;        // 00:single, 10:first, 11:middle, 01:last
			uint32_t    mac_header_length   : 11;
			uint32_t    ampdu_spacing       : 5;

			// Word 8 : BD info 4
			uint32_t    reserved5	        : 8;    // data start position from BD start (in bytes)
			uint32_t    data_length         : 11;   // data length
			uint32_t    segment             : 2;    // MPDU segmentation
			uint32_t    reserved6           : 3;
			uint32_t    ac                 : 4;	// Assigned Queue Manager ID
			uint32_t    tx_count            : 4;
		};
		uint32_t    bd[5];
	};
} LMAC_TXHDR;

typedef struct _LMAC_TXBD {
	union {
		struct {
			// BD info #0
			uint32_t    n_ppdu              : 6;
			uint32_t    ack_policy          : 2;
			uint32_t    cipher_type         : 3;        // 0:wep40, 1:wep128, 2:tkip, 3:ccmp, 4:wapi
			uint32_t    rts_request         : 1;
			uint32_t    cts_self_request    : 1;
			uint32_t    cf_end_request      : 1;
			uint32_t    reserved1           : 4;      // security key location
			uint32_t    timestamp_update    : 1;        // flag for updating timestamp in the frame
			uint32_t    timestamp_position  : 6;        // in byte, timestamp position in frame
			uint32_t	bssid_idx			: 2;
			uint32_t	mac_idx				: 2;
			uint32_t    reserved2           : 3;

			// BD info #1
			uint32_t    rts_duration        : 16;
			uint32_t	key_location		: 10;
			uint32_t    reserved3           : 6;

			// BD info #2
			uint32_t    reserved4;

			// BD info #3
			uint32_t    mpdu_length         : 14;
			uint32_t    ampdu_segment       : 2;        // 00:single, 10:first, 11:middle, 01:last
			uint32_t    mac_header_length   : 11;
			uint32_t    ampdu_spacing       : 5;

			// BD info #4
			uint32_t    rate_index	        : 8;    // LMAC FW use only, Used for Rate Control
			uint32_t    data_length         : 11;   // data length
			uint32_t    segment             : 2;    // MPDU segmentation
			uint32_t    reserved6           : 3;
			uint32_t    ac                 : 4;	// Assigned Queue Manager ID
			uint32_t    tx_count            : 4;
		};
		struct {
			uint32_t    bd[5];
		};
	};

	union {
		struct {
			// Three word exist only at first fragment (segment == 2'b01)
			// Vector
			uint32_t    format         : 3;        // 0=11a/g, 1=11b, 2=Non HT duplicate, 3=HT(11n, fixed format), 4=VHT
	#ifndef FPGA
			uint32_t    reserved7      : 1;
			uint32_t    tx_power       : 12;
			uint32_t    reserved0      : 6;
	#else
			uint32_t    reserved7      : 16;
			uint32_t    tx_power       : 3;
	#endif
			uint32_t    bandwidth      : 2;        // 0=20, 1=40, 2=80

			uint32_t    reserved8      : 1;
			uint32_t    dynamic_bw     : 1;        // 0=static, 1=dynamic
			uint32_t    reserved9      : 2;
			uint32_t    gi_type        : 1;        // for 11b, 0=long preamble, 1=short preamble
			// for non 11b, 0=long GI, 1=short GI
			uint32_t    reserved10     : 3;

			// SIG0
			uint32_t    reserved11     : 2;
			uint32_t    smoothing      : 1;
			uint32_t    aggregation    : 1;
			uint32_t    fec_coding     : 1;        // 0=BCC, 1=LDPC
			uint32_t    beamformed     : 1;        // 0=not apply, 1=apply
			uint32_t    txop_ps_not    : 1;        // 0=allow, 1=not allow
			uint32_t    reserved12     : 1;

			uint32_t    ppdu_length    : 20;       // PPDU length after scheduling
			uint32_t    reserved13     : 4;

			// SIG 1
			uint32_t    mcs            : 7;
			uint32_t    reserved14     : 1;
			uint32_t    group_id       : 1;
			uint32_t    reserved15     : 7;
			uint32_t    partial_aid    : 9;
			uint32_t    reserved16     : 7;
			GenericMacHeader  machdr[0];
		};
		uint32_t          vector[3];
		uint8_t           payload[0];
	};
} LMAC_TXBD;

typedef struct _LMAC_TXBUF {
	//Word 0-1 : TXINFO
	struct frame_tx_info_param txi;

	//Word 2
	struct _LMAC_TXBUF  *m_next;

	//Word 3
	struct _LMAC_TXBUF  *m_link;

	union {
		struct {
			//Word 4 : BD info 0
			uint32_t    n_ppdu              : 6;
			uint32_t    ack_policy          : 2;
			uint32_t    cipher_type         : 3;        // 0:wep40, 1:wep128, 2:tkip, 3:ccmp, 4:wapi
			uint32_t    rts_request         : 1;
			uint32_t    cts_self_request    : 1;
			uint32_t    cf_end_request      : 1;
			uint32_t    reserved1           : 4;      // security key location
			uint32_t    timestamp_update    : 1;        // flag for updating timestamp in the frame
			uint32_t    timestamp_position  : 6;        // in byte, timestamp position in frame
			uint32_t	bssid_idx			: 2;
			uint32_t	mac_idx				: 2;
			uint32_t    reserved2           : 3;

			// Word 5 : BD info 1
			uint32_t    rts_duration        : 16;
			uint32_t	key_location		: 10;
			uint32_t    reserved3           : 6;

			// Word 6 : BD info 2
			uint32_t    reserved4;

			// Word 7 : BD info 3
			uint32_t    mpdu_length         : 14;
			uint32_t    ampdu_segment       : 2;        // 00:single, 10:first, 11:middle, 01:last
			uint32_t    mac_header_length   : 11;
			uint32_t    ampdu_spacing       : 5;

			// Word 8 : BD info 4
			uint32_t    rate_index	        : 8;    // LMAC FW use only, Used for Rate Control
			uint32_t    data_length         : 11;   // data length
			uint32_t    segment             : 2;    // MPDU segmentation
			uint32_t    reserved6           : 3;
			uint32_t    ac                 : 4;	// Assigned Queue Manager ID
			uint32_t    tx_count            : 4;
		};
		struct {
			uint32_t    bd[5];
		};
	};

	union {
		struct {
			// Word 9,10,11 exist only at first fragment (segment == 2'b01)
			// Word 9 : Vector
			uint32_t    format         : 3;        // 0=11a/g, 1=11b, 2=Non HT duplicate, 3=HT(11n, fixed format), 4=VHT
#ifndef FPGA
			uint32_t    reserved7      : 1;
			uint32_t    tx_power       : 12;
			uint32_t    reserved0      : 6;
#else
			uint32_t    reserved7      : 16;
			uint32_t    tx_power       : 3;
#endif
			uint32_t    bandwidth      : 2;        // 0=20, 1=40, 2=80

			uint32_t    reserved8      : 1;
			uint32_t    dynamic_bw     : 1;        // 0=static, 1=dynamic
			uint32_t    reserved9      : 2;
			uint32_t    gi_type        : 1;        // for 11b, 0=long preamble, 1=short preamble
			// for non 11b, 0=long GI, 1=short GI
			uint32_t    reserved10     : 3;

			// Word 10 : SIG0
			uint32_t    reserved11     : 2;
			uint32_t    smoothing      : 1;
			uint32_t    aggregation    : 1;
			uint32_t    fec_coding     : 1;        // 0=BCC, 1=LDPC
			uint32_t    beamformed     : 1;        // 0=not apply, 1=apply
			uint32_t    txop_ps_not    : 1;        // 0=allow, 1=not allow
			uint32_t    reserved12     : 1;

			uint32_t    ppdu_length    : 20;       // PPDU length after scheduling
			uint32_t    reserved13     : 4;

			// Word 11 : SIG 1
			uint32_t    mcs            : 7;
			uint32_t    reserved14     : 1;
			uint32_t    group_id       : 1;
			uint32_t    reserved15     : 7;
			uint32_t    partial_aid    : 9;
			uint32_t    reserved16     : 7;
			GenericMacHeader  machdr[0];
		};
		uint32_t            vector[3];

	};
} LMAC_TXBUF;

typedef struct _RXINFO {
	/// RX Info 0
	uint32_t mpdu_length    : 14;
	uint32_t center_freq    : 10;
	uint32_t mpdu_count     : 8;

	/// RX Info 1
	uint32_t ack_policy     : 2;
	uint32_t reserved7      : 6;
	uint32_t cur_active_vif : 1;
	uint32_t received_vif   : 1;
	uint32_t reserved8  	: 3;
	uint32_t cipher_type    : 3;
	uint32_t reserved9      : 1;
	uint32_t rxnpayload_av_ind      : 1;
	uint32_t reserved10     : 1;
	uint32_t eof_ind        : 1;
	uint32_t ndp_ind        : 1;
	uint32_t agg		    : 1;
	uint32_t protection     : 1;
	uint32_t error_delimiter: 1;
	uint32_t error_icv      : 1;
	uint32_t error_mic      : 1;
	uint32_t error_key      : 1;
	uint32_t error_length   : 1;
	uint32_t error_match    : 1;
	uint32_t error_crc      : 1;
	uint32_t error_sequence : 1;
	uint32_t okay           : 1;

	/// RX Info 2
	uint32_t timestamp;
} RXINFO;

typedef struct _LMAC_RXBUF {
	union {
		struct {
			// word 0 : RX Vector Head0
			uint32_t reserved0     : 1;
			uint32_t smt           : 1;
			uint32_t format        : 2;  // 0=11a/g, 1=11b, 2=Non HT duplicate, 3=HT(11n, fixed format)
			uint32_t stbc          : 1;
			uint32_t aggregation   : 1;
			uint32_t gi_type       : 1;
			uint32_t bandwidth     : 1;  // 0=20, 1=40
			uint32_t reserved1     : 1;
			uint32_t mcs           : 7;
			uint32_t cfo_estimate  : 13;
			uint32_t reserved2     : 3;

			/// word 1 : RX Vector Head1
			uint32_t reserved3     : 8;
			uint32_t length        : 20;
			uint32_t reserved4     : 4;

			/// word 2 : RX Vector Tail
			uint32_t rssi          : 8;
			uint32_t rcpi          : 8;
			uint32_t snr	       : 9;
			uint32_t reserved5     : 7;

			/// Rxinfo 3 word
			RXINFO	rxinfo;

//      	uint8_t  payload[0];
		};
		uint32_t            rx_vector[6];
    };
} LMAC_RXBUF , LMAC_RXHDR;



#endif //HAL_LMAC_11N_H
