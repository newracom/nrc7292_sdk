#ifndef __UTIL_EVENT_TRACKER_H__
#define __UTIL_EVENT_TRACKER_H__

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "system.h"
#include "system_type.h"
#include "nrc-wim-types.h"

#define UDH_INVALID (0)
#define UDH_UL_HIF (1)
#define UDH_UL_ENQUEUE (2)
#define UDH_UL_WINAC (3)
#define UDH_UL_TXDONE (4)
#define UDH_UL_REQUE (5)
#define UDH_UL_FAIL (6)
#define UDH_UL_RESTORE (7)
#define UDH_UL_SCH_BO (8)
#define UDH_UL_SCH_TXOP (9)
#define UDH_UL_FREE (10)

#define UDH_UL_WIM (11)
#define UDH_UL_WIM_RUN (12)
#define UDH_UL_BCN_TBTT (13)
#define UDH_UL_BCN_FEED (14)
#define UDH_UL_BCN_START (15)
#define UDH_UL_BCN_STOP (16)

#define UDH_UL_SCAN_START (17)
#define UDH_UL_SCAN_CB (18)
#define UDH_UL_SCAN_FEED (19)

#define UDH_QM (30)

#define UDH_HIF (40)

#define UDH_DL_RXISR (102)
#define UDH_DL_TASK (103)
#define UDH_DL_HIF (104)
#define UDH_DL_POST (105)
#define UDH_DL_DISCARD (106)
#define UDH_DL_FEED (107)

#define UDH_MC_ISR (150)

#define UDH_DL_HOOK_BEFORE (201)
#define UDH_DL_HOOK_AFTER (202)
#define UDH_UL_HOOK_BEFORE (203)
#define UDH_UL_HOOK_AFTER (204)
#define UDH_DL_HOOK (205)
#define UDH_UL_HOOK (206)

#define UDH_UL_RECOVERY (210)
#define UDH_DL_RECOVERY (211)

#define UDH_POWER_SAVE (220)

#define UDH_BCN_MON_FEED (230)

#define UDH_KEY_ADD (250)
#define UDH_KEY_DEL (251)
#define UDH_KEY_DEL_ALL (252)
#define UDH_MODEM_PRE	(253)
#define UDH_MODEM_POST (254)

#if defined(INCLUDE_EVENT_TRACKER)
#define UTIL_EVENT_TRACKER_INIT() util_event_tracker_init()
#define ET_SYSBUF(a,b,c) do {ASSERT(b);util_event_tracker_sysbuf(a,b,c);}while(0)
#define ET_IRQ(a,b) util_event_tracker_irq(a, b)
#define ET_WIM(a,b) util_event_tracker_wim(a, b)
#define ET_MODULE(a) util_event_tracker_module((a), module_name(), __LINE__, 0, (uint32_t)0, (uint32_t)0, (uint32_t)0)
#define ET_MODULE_A(a,a0) util_event_tracker_module(a, module_name(), __LINE__, 1, (uint32_t)a0, (uint32_t)0, (uint32_t)0)
#define ET_MODULE_AA(a,a0,a1) util_event_tracker_module(a, module_name(), __LINE__, 2, (uint32_t)a0, (uint32_t)a1, (uint32_t)0)
#define ET_MODULE_AAA(a,a0,a1,a2) util_event_tracker_module(a, module_name(), __LINE__, 3, (uint32_t)a0, (uint32_t)a1, (uint32_t)a2)

void util_event_tracker_init();
void util_event_tracker_deinit();
void util_event_tracker_sysbuf(uint8_t tag, SYS_BUF *head, int16_t size);
void util_event_tracker_irq(uint8_t tag, uint32_t status);
void util_event_tracker_wim(uint8_t tag, struct wim_hdr *wimh);
void util_event_tracker_module(uint8_t tag, const char *name, int32_t line, 
							uint32_t n_arg, uint32_t arg1, uint32_t arg2, uint32_t arg3);
void util_event_tracker_show();
#else 
#define UTIL_EVENT_TRACKER_INIT() 
#define ET_SYSBUF(a,b,c) 
#define ET_IRQ(a,b) 
#define ET_WIM(a,b) 
#define ET_MODULE(a) 
#define ET_MODULE_A(a,a1)
#define ET_MODULE_AA(a,a1,a2)
#define ET_MODULE_AAA(a,a1,a2,a3)

static inline void util_event_tracker_init(){};
static inline void util_event_tracker_deinit(){};
static inline void util_event_tracker_sysbuf(uint8_t tag, SYS_BUF *head, int16_t size){};
static inline void util_event_tracker_irq(uint8_t tag, uint32_t status){};
static inline void util_event_tracker_wim(uint8_t tag, struct wim_hdr *wimh){};
static inline void util_event_tracker_module(uint8_t tag, const char *name, int32_t line, 
				uint32_t n_arg, uint32_t arg1, uint32_t arg2, uint32_t arg3){};
static inline void util_event_tracker_show(){};
#endif //#if defined(INCLUDE_EVENT_TRACKER)


#endif /* __UTIL_EVENT_TRACKER_H__ */
