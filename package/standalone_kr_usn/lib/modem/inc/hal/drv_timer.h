#ifndef __HAL_TIMER_NRC7292_H__
#define __HAL_TIMER_NRC7292_H__

#include "system_common.h"
#include "hal_timer.h"

#define NRC7292_TIMER_CH_NUM 6
#define NRC7292_TIMER_32_BITS 32
#define NRC7292_TIMER_64_BITS 64
#define NRC7292_TIMER_IS_VALID(ch, bit, ret)								\
do {																		\
	if ((!nrc_timer_is_valid(ch)) || (nrc_timer_get_resolution(ch) != bit)) {	\
		return ret; 														\
	}																		\
} while(0);

typedef union {
	struct {
		uint32_t prescale   : 24;   //[23:0]-RW: Pre-scale counter load value
		uint32_t enable     : 1;    //[24]-RW: Counter Enable
		uint32_t loadzero   : 1;    //[25]-RW: By default, counter starts from LOADVAL
		uint32_t stopmode   : 1;    //[26]-RW: 0=Free Running Mode, 1=Stop Mode
		uint32_t reserved1  : 1;    //[27]-RO: Reserved
		uint32_t ldm0       : 1;    //[28]-RW: Re-load counter
		uint32_t ldm1       : 1;    //[29]-RW: Re-load counter
		uint32_t reserved2  : 2;    //[31:30] - Reserved
	} bit;
	uint32_t word;
} timer_tcctrl_t;


typedef union {
	struct {
		uint32_t loadval   : 32;   //[31:0]-RW: Counter Load Value
	} bit;
	uint32_t word;
} timer_tcldv_t;


typedef union {
	struct {
		uint32_t cmp0   : 32;   //[31:0]-RW: Counter Match Value0
	} bit;
	uint32_t word;
} timer_tcmv0_t;


typedef union {
	struct {
		uint32_t cmp1   : 32;   //[31:0]-RW: Counter Match Value1
	} bit;
	uint32_t word;
} timer_tcmv1_t;


typedef union {
	struct {
		uint32_t pcnt     : 24;   //[23:0]-RO: Pre-scale counter current value
		uint32_t reserved : 8;    //[31:24]-RO: Reserved
	} bit;
	uint32_t word;
} timer_tcpcnt_t;


typedef union {
	struct {
		uint32_t mcnt     : 32;   //[31:0]-RO: Main counter current value
	} bit;
	uint32_t word;
} timer_tcmcnt_t;


typedef union {
	struct {
		uint32_t mcnt     : 32;   //[63:32]-RO: Main counter current value
	} bit;
	uint32_t word;
} timer_tcmcnt1_t;


typedef union {
	struct {
		uint32_t irqmstat     : 5;   //[4:0]-RW: Masked Interrupt Status = IRQRSTAT & IRQEN
		uint32_t reserved1    : 3;   //[7:5]-RO: Reserved
		uint32_t irqrstat     : 5;   //[12:8]-RO: Interrupt Raw Status. Refer to the description for IRQEN above.
		uint32_t reserved2    : 3;   //[15:13]-RO: Reserved
		uint32_t irqen0       : 1;   //[16]-RW: Enable Interrupt when the counter value matched with CMP0
		uint32_t irqen1       : 1;   //[17]-RW: Enable Interrupt when the counter value matched with CMP1
		uint32_t irqen2       : 1;   //[18]-RW: Enable Interrupt at the end of count
		uint32_t irqen3       : 1;   //[19]-RW: Enable Interrupt at the end of pre-scale count
		uint32_t irqen4       : 1;   //[20]-RW: Enable Interrupt at the rising edge of a counter but selected by BITSEL.
		uint32_t reserved3    : 3;   //[23:21]-RO: Reserved
		uint32_t bitsel       : 6;   //[29:24]-RW: Counter bit selection value for interrupt generation.
		uint32_t rsync        : 1;   //[30]-RW: Synchronization control for Counter Current Value Registers(TCPCNT & TCMCNT)
		uint32_t irqclr       : 1;   //[31]-RW: Interrupt Clear Control. 0 = IRQRSTAT are cleared by reading this register.
	} bit;
	uint32_t word;
} timer_tcirq_t;


typedef union {
	struct {
		uint32_t irqmstat     : 5;   //[4:0]-RW: Masked Interrupt Status = IRQRSTAT & IRQEN
		uint32_t reserved1    : 3;   //[7:5]-RO: Reserved
		uint32_t irqrstat     : 5;   //[12:8]-RO: Interrupt Raw Status. Refer to the description for IRQEN above.
		uint32_t reserved2    : 3;   //[15:13]-RO: Reserved
		uint32_t irqen0       : 1;   //[16]-RW: Enable Interrupt when the counter value matched with CMP0
		uint32_t irqen1       : 1;   //[17]-RW: Enable Interrupt when the counter value matched with CMP1
		uint32_t irqen2       : 1;   //[18]-RW: Enable Interrupt at the end of count
		uint32_t irqen3       : 1;   //[19]-RW: Enable Interrupt at the end of pre-scale count
		uint32_t irqen4       : 1;   //[20]-RW: Enable Interrupt at the rising edge of a counter but selected by BITSEL.
		uint32_t reserved3    : 2;   //[22:21]-RO: Reserved
		uint32_t bitsel       : 7;   //[29:23]-RW: Counter bit selection value for interrupt generation.
		uint32_t rsync        : 1;   //[30]-RW: Synchronization control for Counter Current Value Registers(TCPCNT & TCMCNT)
		uint32_t irqclr       : 1;   //[31]-RW: Interrupt Clear Control. 0 = IRQRSTAT are cleared by reading this register.
	} bit;
	uint32_t word;
} timer_tcirq64_t;


typedef struct {
	volatile timer_tcctrl_t tcctrl;
	volatile timer_tcldv_t  tcldv;
	volatile timer_tcmv0_t  tcmv0;
	volatile timer_tcmv1_t  tcmv1;
	volatile timer_tcpcnt_t tcpcnt;
	volatile timer_tcmcnt_t tcmcnt;
	volatile timer_tcirq_t  tcirq;
} timer_controller_t;


typedef struct {
	volatile timer_tcctrl_t tcctrl;
	volatile timer_tcldv_t  tcldv;
	volatile timer_tcmv0_t  tcmv0;
	volatile timer_tcmv1_t  tcmv1;
	volatile timer_tcpcnt_t tcpcnt;
	volatile timer_tcmcnt_t tcmcnt0;
	volatile timer_tcirq64_t tcirq64;
	volatile timer_tcmcnt1_t tcmcnt1;
} timer_controller64_t;

struct tmr_ops *nrc_timer_7292_ops(void);
uint32_t nrc_timer_get_resolution(int channel_index);
bool nrc_timer_is_valid(int channel_index);
bool nrc_timer_init(int channel_index, int prescale);
void nrc_timer_enable(int channel_index, bool enable);
bool nrc_timer_get_enable(int channel_index);
void nrc_timer_set_target(int channel_index, uint32_t target);
uint32_t nrc_timer_get_target(int channel_index);
void nrc_timer_stopmode(int channel_index, bool enable);
void nrc_timer_get_tick(int channel_index, uint32_t *tick);
uint32_t nrc_timer_get_irq(int channel_index);
void nrc_timer_clr_irq(int channel_index);

bool nrc_timer64_init(int channel_index, int prescale);
void nrc_timer64_enable(int channel_index, bool enable);
bool nrc_timer64_get_enable(int channel_index);
void nrc_timer64_set_target(int channel_index, uint64_t target);
uint64_t nrc_timer64_get_target(int channel_index);
void nrc_timer64_stopmode(int channel_index, bool enable);
void nrc_timer64_get_tick(int channel_index, uint64_t *tick);
uint32_t nrc_timer64_get_irq(int channel_index);
void nrc_timer64_clr_irq(int channel_index);

#endif //__HAL_TIMER_NRC7292_H__
