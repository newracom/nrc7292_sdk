#ifndef LMAC_TYPE_H
#define LMAC_TYPE_H
#include "system_type.h"
#if	defined(STANDARD_11N)
#include "lmac_11n.h"
#endif /* defined(STANDARD_11N) */
#if defined(STANDARD_11AH)
#include "protocol_11ah.h"
#include "lmac_11ah.h"
#endif /* defined(STANDARD_11AH) */


#define VIF_MAX     (2)
#define MAX_HW_GTK  (1)

#define VIF_BROADCAST               VIF_MAX
#define MAGIC_CODE 0x17291729	/* MAGIC_CODE */

enum key_type {
	KEY_PTK = 0,
	KEY_GTK = 1,
	KEY_IGTK = 2,
	KEY_MAX = 3,
};

enum key_cmd {
	KEY_ADD = 0,
	KEY_DEL = 1,
	KEY_DELALL = 2,
	KEY_UNKNOWN = 8,
};

enum {
	DIR_TX 	= 0,
	DIR_RX
};

enum {
	LOG_NONE 	= 0,
	LOG_DATA 	= 1 << 0, // 1
	LOG_MGMT 	= 1 << 1, // 2
	LOG_BEACON	= 1 << 2, // 4
	LOG_ALL		= LOG_DATA | LOG_MGMT | LOG_BEACON, //7
};

typedef	enum mac_stats_status {
	STATS_OK = 0,
	STATS_NOK,
	MAX_STATS_STATUS
} MAC_STATS_STATUS;

typedef	enum mac_stats_mcs {
	STATS_MCS0 =0,
	STATS_MCS1,
	STATS_MCS2,
	STATS_MCS3,
	STATS_MCS4,
	STATS_MCS5,
	STATS_MCS6,
	STATS_MCS7,
	STATS_MCS10,
	STATS_MCS_MAX
} MAC_STATS_MCS;

typedef	enum mac_stats_type {
	STATS_TYPES_MGMT =0,
	STATS_TYPES_CTRL,
	STATS_TYPES_DATA,
	STATS_TYPES_BEACON,
	STATS_TYPES_MAX
} MAC_STATS_TYPE;

typedef struct mac_stats_info {
	uint8_t print_flag;
	uint8_t last_mcs;
	uint32_t n_mpdu[STATS_TYPES_MAX][MAX_STATS_STATUS];
	uint32_t b_mpdu[STATS_TYPES_MAX][MAX_STATS_STATUS];
	uint32_t n_ac[MAX_AC][MAX_STATS_STATUS];
	uint32_t b_ac[MAX_AC][MAX_STATS_STATUS];
	uint32_t n_mcs[STATS_MCS_MAX][MAX_STATS_STATUS];
	uint32_t b_mcs[STATS_MCS_MAX][MAX_STATS_STATUS];
} MAC_STATS_INFO;

typedef enum  _LMacEvent {
	LME_DOWNLINK        = 0,
	LME_ULDONE          = 1,
	LME_TBTT            = 2,
	LME_CONC            = 3,
	LME_IDLE            = 4,
	LME_ACTIVE          = 5,
	LME_PSPOOL          = 6,
	LME_NULL            = 7,
	LME_MIC             = 8,
	LME_NAN_DW_START    = 9,
	LME_NAN_DW_END      = 10,
	LME_NAN_TBTT        = 11,
	LME_POWERSAVE       = 12,
	LME_DATA            = 13,
	LME_PS_WAKEUP       = 14,
	LME_CSA             = 15,
	LME_MAX,
} LMacEvent;

typedef struct {
	union {
		void* address;
		struct {
			uint8_t vif_id;
			uint8_t sub_event;
		};
	};
	LMacEvent event;
} LMAC_SIGNAL;

enum {
	FRAG_SINGLE = 0,
	FRAG_LAST,
	FRAG_FIRST,
	FRAG_MID,
};

enum {
	RTS_OFF = 0,
	RTS_ON,
	RTS_DEFAULT,
	RTS_NDP,
	RTS_LEGACY,
};

enum {
	MODE_STA = 0,
	MODE_AP  = 1,
	MODE_MESH = 2,
	MODE_MAX,
};

enum {
	/* Ownership bit */
	DESC_SW = 0,
	DESC_HW = 1,
	/* Length limit (based on the size of control.length) */
	DESC_MAX_LENGTH = (1 << 11)
};

enum {
	MAX_KEY_ID = 4,
	MAX_KEY_LEN = 4,
	WEP_IV_LEN = 4,
	CIPHER_HEADER_LEN = 8,
	CIPHER_HEADER_LEN_WEP = 4,
	CIPHER_HEADER_LEN_WAPI = 18,
	MIC_LEN = 8,
	MIC_LEN_WAPI = 16,
	ICV_LEN	= 4,
	FCS_SIZE = 4,
	DELIMITER_SIZE = 4,
	MGMT_HEADER_SIZE = 24,
	CFO_WINDOW_SIZE = 16,
};

enum CipherType {
	CIP_WEP40 = 0,
	CIP_WEP104 = 1,
	CIP_TKIP = 2,
	CIP_CCMP = 3,
	CIP_WAPI = 4,
	CIP_MAX = 5,
	CIP_NONE = CIP_MAX,
};

#define TID_TO_AC(_tid) (      \
	((_tid) == 0 || (_tid) == 3) ? ACI_BE : \
	((_tid) < 3) ? ACI_BK : \
	((_tid) < 6) ? ACI_VI : \
	ACI_VO)

#endif /* HAL_LMAC_TYPE_H */
