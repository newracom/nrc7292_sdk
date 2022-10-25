#ifndef	__SYSTEM_TYPE_H__
#define __SYSTEM_TYPE_H__

//#if !defined(CUNIT)
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

struct _SYS_BUF;
// 2 Word HIF_HDR
typedef struct _HIF_HDR {
	uint8_t type;
	uint8_t subtype;
	uint8_t flags;
	int8_t vifindex;
	uint16_t len;
	uint16_t tlv_len;
} HIF_HDR;

// 1 Word FRAME_HDR
typedef struct _FRAME_HDR {
	union {
		struct _rx_flags {
			uint8_t error_mic:1;
			uint8_t iv_stripped:1;
			uint8_t reserved:6;
			uint8_t rssi;
		} rx;
		struct _tx_flags {
			uint8_t ac;
			uint8_t reserved;
		} tx;
	} flags;
	union {
		struct _rx_info {
			uint16_t frequency;
		} rx;

		struct _tx_info {
			uint8_t cipher;
			uint8_t tlv_len;
		} tx;
	} info;
} FRAME_HDR;

#include "protocol.h"
#if defined(STANDARD_11AH)
#include "protocol_11ah.h"
#include "lmac_11ah.h"
#endif /* defined(STANDARD_11AH) */

#if	defined(STANDARD_11N)
#include "lmac_11n.h"
#endif /* defined(STANDARD_11N) */

typedef union _UINT64 {
	struct {
		uint32_t low;
		uint32_t high;
	} u;
	uint64_t q;
} UINT64;

typedef union _INT64 {
	struct {
		uint32_t low;
		int32_t high;
	} u;
	int64_t q;
} INT64;

typedef void (*isr_callback_t)(int vector);

typedef struct _sys_info {
	//README swki - 2018-0730
	//current_channel and current_channel
	//These must always be set together.
	uint16_t    current_channel;  			//actually, center frequency
	uint16_t    current_channel_number; 	//channel number (under 255)
} sys_info;

typedef int32_t (*sys_task_func )(void *);
typedef void    (*sys_task_func_cb)(int32_t , void*);

typedef struct _SYS_TASK {
	sys_task_func       func;
	void*               param;
	sys_task_func_cb    cb;
} SYS_TASK;

#if defined (INCLUDE_NEW_TASK_ARCH)
typedef enum _TQUEUE_EVENT_ID {
	TASK_QUEUE_EVENT_TBTT		= 0,
	TASK_QUEUE_EVENT_WAKE_MODEM,
	TASK_QUEUE_EVENT_RECOVERY_MODEM,
	TASK_QUEUE_EVENT_WDT,
	TASK_QUEUE_EVENT_MAC_DATA_RX,
	TASK_QUEUE_EVENT_MAC_MGMT_RX,
	TASK_QUEUE_EVENT_MAX,
} TQUEUE_EVENT_ID;

typedef struct _TQUEUE_MESSAGE {
	uint8_t vif_id;
	uint8_t q_event_id;
	void* param;
} TQUEUE_MESSAGE;
#define TQUEUE_MESSAGE_SIZE sizeof (TQUEUE_MESSAGE)
#endif //#if defined (INCLUDE_NEW_TASK_ARCH)

struct _SYS_BUF;

// Any Wordsize of SYS_HDR
typedef struct _SYS_HDR {
	int         (*m_cb)(struct _SYS_BUF*);
	uint8_t     m_reserved		:2;
	uint8_t     m_credit_ac		:2;	/* this field used when different AC between actual AC and credited AC. */
	uint8_t     m_tracker_tag	:4;
	uint8_t     m_ref_count;
	uint8_t     m_n_credit		:4;
	uint8_t     m_hooked		:1;
	uint8_t     m_prealloc_id	:2;
	uint8_t     vif_id			:1;
	uint8_t     pool_id			;	// TODO Pool* m_pool

	uint16_t    m_payload_length;
	uint16_t    m_payload_offset;
	struct      _SYS_BUF*	m_next;
	struct      _SYS_BUF*	m_link;
} SYS_HDR;

// Common System Buffer Structure
typedef struct _SYS_BUF {
	SYS_HDR	sys_hdr;
	union {
		struct {
			LMAC_TXHDR	lmac_txhdr;
			union {
				struct {
					HIF_HDR	hif_hdr;
					FRAME_HDR	frame_hdr;
				};
				PHY_TXVECTOR    phy_txvector;
				uint8_t more_payload[0];
			};
			union {
				GenericMacHeader    tx_mac_header;
				uint8_t payload[0];
			};
		};
		struct {
			LMAC_RXHDR		 lmac_rxhdr;
			GenericMacHeader rx_mac_header;
		};
#if defined(NRC7291_SDK_DUAL_CM0) || defined(NRC7291_SDK_DUAL_CM3)
		struct {
			MBX_HDR		 mbx_hdr;
		};
#endif
	};
} SYS_BUF;



#if defined(NRC7291_SDK_DUAL_CM0) || defined(NRC7291_SDK_DUAL_CM3)
// Mailbox Header
typedef struct _MBX_HDR{
	uint8_t type;
	uint8_t length;
	uint16_t channel;
	uint8_t data[0];
} MBX_HDR;
#endif

//#endif /* !defined(CUNIT) */
#endif /* __SYSTEM_TYPE_H__ */
