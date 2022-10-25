#ifndef __DRV_WDT_NRC7292_H__
#define __DRV_WDT_NRC7292_H__

#include "system.h"

//======================================================================================================================
// Watchdog Timer
//======================================================================================================================
#define WDT_ACCESS_ENABLE (0x1ACCE551)
#define WDT_ACCESS_DISABLE (0x1ACCE550)
#define WDT_INT_CLEAR_CODE (0xC1EA2B00)

#define WDT_BASE_ADDR  (SYSTEM_BASE + 0x3000)
#define RegWDT_LOAD		(*((volatile uint32_t *)(WDT_BASE_ADDR + 0x00)))
#define RegWDT_VALUE	(*((volatile uint32_t *)(WDT_BASE_ADDR + 0x04)))
#define RegWDT_CTRL		(*((volatile uint32_t *)(WDT_BASE_ADDR + 0x08)))
#define RegWDT_INTCLR	(*((volatile uint32_t *)(WDT_BASE_ADDR + 0x0C)))
#define RegWDT_RIS		(*((volatile uint32_t *)(WDT_BASE_ADDR + 0x10)))
#define RegWDT_MIS		(*((volatile uint32_t *)(WDT_BASE_ADDR + 0x14)))
#define RegWDT_LOCK		(*((volatile uint32_t *)(WDT_BASE_ADDR + 0xC00)))

typedef union {
	struct {
		uint32_t interrupt	: 1;   //[0]-RW: enable interrupt event
		uint32_t reset		: 1;   //[1]-RW: enable reset output
		uint32_t reserved	: 30;
	} bit;
	uint32_t word;
} wdt_ctrl_t;

typedef union {
	struct {
		uint32_t interrupt	: 1;   //[0]-RO: raw interrupt status
		uint32_t reserved	: 31;
	} bit;
	uint32_t word;
} wdt_int_status_t;

typedef struct {
	volatile uint32_t	load;		//RW: containing the value from which the counter is to decrement.
	volatile uint32_t	value;		//RO: current value of the decrementing counter
	volatile wdt_ctrl_t	ctrl;		//RW: select event source 
	volatile uint32_t	int_clr;	//WO: write any value to clear interrupt
	volatile wdt_int_status_t	ris;	//RO: raw interrupt status
	volatile wdt_int_status_t	mis;	//RO: interrupt status
} wdt_controller_t;

bool nrc_wdt_init();
void nrc_wdt_deinit();

void nrc_wdt_access(bool enable);
void nrc_wdt_set_load(uint32_t value);
uint32_t nrc_wdt_get_load();
uint32_t nrc_wdt_get_value();
void nrc_wdt_int_enable(bool enable);
void nrc_wdt_int_clear();
void nrc_wdt_rst_enable(bool enable);
bool nrc_wdt_ris();
bool nrc_wdt_mis();
void nrc_wdt_test();

#endif //__DRV_WDT_NRC7292_H__
