#ifndef	SYSTEM_COMMON_H
#define SYSTEM_COMMON_H

#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>

#include "system_type.h"
#include "system_macro.h"
#include "system_constant.h"
#include "system_freertos.h"
#include "util_trace.h"


void system_task_init();
void background_task_init();

void show_assert(const char *x, const char *file, const char *func, unsigned int line);
void show_assert_v(const char *x, const char *file, const char *func, unsigned int line, int v);

uint16_t    mem_read_16(const uint8_t *ptr);
uint32_t    mem_read_32(const uint8_t *ptr);
void        mem_write_16(uint16_t u16, uint8_t *ptr);
void        mem_write_32(uint32_t u32, uint8_t *ptr);

uint32_t    swap_uint32(uint32_t val);
int32_t     swap_int32(int32_t val);
uint16_t    swap_uint16(uint16_t val);
int16_t     swap_int16(int16_t val);

#if defined (NRC7291) || (NRC7292) || (NRC7391) || (NRC7392) || (NRC4791) || (NRC5291)
// -- General Include --
// -- Hal Include --
#if defined(NRC7291)
#include "hal_uart_nrc7291.h"
#include "lmac_nrc7291.h"
#include "hal_clock_nrc7291.h"
#include "hal_gpio_nrc7291.h"
#include "hal_i2c_nrc7291.h"
#include "hal_timer_nrc7291.h"
#include "hal_pwm_nrc7291.h"
#include "hal_nadc_nrc7291.h"
#include "hal_ssp_nrc7291.h"
#include "hal_cspi_nrc7291.h"
#include "hal_adc_sfc_nrc7291.h"
#include "hal_sflash_nrc7291.h"
#include "hal_phy_nrc7291.h"
#include "hal_rf_nrc7291.h"
#endif /* defined(NRC7291) */

#if defined(NRC7292)
#include "hal_uart.h"
#include "hal_clock.h"
#include "hal_gpio.h"
#include "hal_i2c.h"
#include "drv_timer.h"
#include "hal_pwm.h"
#include "hal_nadc.h"
#include "hal_ssp.h"
#include "hal_cspi.h"
#include "hal_adc_sfc.h"
#include "hal_sflash.h"
#if defined(NRC7292_LMACTEST_FPGA_AVIA)
    #include "hal_phy_avia.h"
    #include "hal_rf_avia.h"
#else
    #include "hal_phy.h"
    #include "hal_rf.h"
#endif /* defined(NRC7292_LMACTEST_FPGA_AVIA) */
/* TODO: This has to be accessed through HAL and driver. */ 
#include "reg_gdma.h"
#endif /* defined(NRC7292) */

#if defined(NRC7391)
#include    "hal_uart.h"
#include    "hal_gpio.h"
#include   "hal_clock.h"
#include   "drv_timer.h"
#include     "hal_i2c.h"
#include      "hal_rf.h"

//#include    "lmac.h"
#include    "hal_phy_register.h"

#include     "hal_pwm.h"
#include     "hal_ssp.h"
#include     "hal_gdma.h"
#include     "hal_wdt.h"
#include    "hal_nadc.h"
#include    "hal_cspi.h"
#include "hal_adc_sfc.h"
#include  "hal_sflash.h"
#include     "hal_phy.h"
#endif /* defined(NRC7391) */

#if defined(NRC7392) || defined(NRC4791) || defined(NRC5291)
#include    "hal_uart.h"
#include    "hal_gpio.h"
#include   "hal_clock.h"
#include   "drv_timer.h"
#include     "hal_i2c.h"

#if defined(INCLUDE_RF_AVIA)
#include      "hal_rf_avia.h"
#include      "hal_phy_avia.h"
#elif defined(INCLUDE_RF_NRC7292RFE)
#include      "hal_rf_nrc7292rfe.h"
#include      "hal_phy.h"
#else
#include      "hal_rf.h"
#include      "hal_phy.h"
#endif /* defined(INCLUDE_RF_AVIA) */

//#include    "lmac.h"
#include    "hal_phy_register.h"

#include     "hal_pwm.h"
#include     "hal_ssp.h"
#include     "hal_gdma.h"
#include     "hal_wdt.h"
#include    "hal_nadc.h"
#include    "hal_cspi.h"
#if defined(NRC_ROMLIB)
#include "hal_adc_sfc.h"
#include  "hal_sflash_lib.h"
#else
#include "hal_adc_sfc.h"
#include  "hal_sflash.h"
#endif /* defined(NRC_ROMLIB) */
#endif /* defined(NRC7392) || defined(NRC4791) || defined(NRC5291) */

// -- UTIL Include --
#include "util_trace.h"
#include "util_cmd.h"

#if defined(NRC7292)
#include "util_core_dump.h"
#endif /* defined(NRC7292) */

typedef enum _COUNTRY_CODE_INDEX {
	COUNTRY_CODE_JP = 0,
	COUNTRY_CODE_KR,
	COUNTRY_CODE_TW,
	COUNTRY_CODE_US,
	COUNTRY_CODE_EU,
	COUNTRY_CODE_CN, //5
	COUNTRY_CODE_NZ, //6
	COUNTRY_CODE_AU, //7
	COUNTRY_CODE_MAX,
} CONTRY_CODE_INDEX;

// -- IRQ --
void system_init();
void system_isr_init();
void system_irq_prio(int evid , int prio);
void system_irq_mask(int evid);
void system_irq_unmask(int evid);
void system_register_isr(int evid , isr_callback_t isr);

void system_constructor_init();
#endif // (NRC7291) || (NRC7292) || (NRC7391) |||(NRC4791)

#if defined(TS8266) || defined(TR6260)
typedef void    (*isr_callback_t)(int vector);
void            system_isr_init();
void            system_register_isr(int evid , isr_callback_t isr);
#define system_irq_save             portSET_INTERRUPT_MASK_FROM_ISR
#define system_irq_restore          portCLEAR_INTERRUPT_MASK_FROM_ISR
#define system_irq_enable			portENABLE_INTERRUPTS
#define system_irq_disable			portDISABLE_INTERRUPTS
void            system_irq_mask(int evid);
void            system_irq_unmask(int evid);
void            system_irq_prio(int evid , int prio);
void            system_constructor_init();

// -- Print System ---
// -- Print System ---
#define PRINT_BUFFER_SIZE	256
void system_register_printf_port(unsigned int portID, void (* printf)(unsigned int, const unsigned char *, unsigned int));
void system_printf(const char *f, ...);
void system_vprintf(const char *f, va_list args);
void system_oprintf(const char *f, unsigned int len);

#ifdef UART_WPA_SEPARATION
void system_register_wpa_port(unsigned int portID);
void system_wpa_write(const char *f, ...);
#endif


#ifdef RELEASE
    #define system_printf(...)
    #define system_vprintf(...)
#else
	//FIXME: change for tr6260 test -- swki -- 2019-0627
void tr6260_system_printf(const char *f, ...);
void tr6260_system_vprintf(const char *f, va_list args);
void usdelay(unsigned int delay);
void msdelay(unsigned int delay);

#if defined(TR6260)
#define system_printf 	tr6260_system_printf
#define system_vprintf	tr6260_system_vprintf
#endif

#if defined(TS8266)
#define system_printf 	hal_uart_printf
#define system_vprintf	hal_uart_vprintf
#endif

#endif

// -- System Buffer --

// -- General Include --
#include "FreeRTOS.h"
#include "semphr.h"
#include "bsp_hal.h"
#include "hal.h"
#include "nds32.h"

// -- UTIL Include --
#include "util_trace.h"
#include "util_cli_freertos.h"
#include "util_cmd.h"


#if defined(TS8266)
#include "nrc_system_8266.h"
// -- Hal Include --
#include "hal_uart_ts8266.h"
#include "lmac_ts8266.h"
#include "hal_sdio_ts8266.h"
#include "hal_phy_ts8266.h"
#include "hal_rf_ts8266_nrc6111.h"
#endif

#if defined(TR6260)
#include "nrc_system_8266.h"
// -- Hal Include --
#include "hal_sdio_ts8266.h"
#include "hal_phy.h"
#include "hal_rf.h"
#endif
#endif /* defined(TS8266) || defined(TR6260) */

// ------------------------------------------------------------------------
// System Common Begin
// ------------------------------------------------------------------------
//

sys_info *get_sys_info();

bool system_schedule_work_queue_from_isr( sys_task_func func , void* param , sys_task_func_cb cb );
bool system_schedule_work_queue         ( sys_task_func func , void* param , sys_task_func_cb cb );

#if defined (INCLUDE_NEW_TASK_ARCH)
#if defined (INCLUDE_WDT)
/* System WDT  */
//define
#define WDT_DEBUG			0
#define WDT_TIMEOUT_RST			1	//System Reset by WDT timeout (1:Reset, 0:Print Log)
#define WDT_TIMEOUT_MS		 	10000	// 10000ms (10 sec)
#define WDT_RST_TRIGGER_CNT		3	//WDT RESET Threshold (waiting count)

//Common Task
#define WDT_ALIVE_LMAC_TASK	 BIT(0)		//LMAC Task
#define WDT_ALIVE_SYSTEM_TASK	 BIT(1)		//SYS Task
#define WDT_ALIVE_BG_TASK	 BIT (2)	//BG Task

#if defined(NRC7292_CSPI) || defined(NRC7392_CSPI)
#define WDT_ALIVE_MASK		(BIT(1)|BIT(0))
#endif

#if defined (INCLUDE_STANDALONE)
// TBD : Considering Standalone Tasks
//#define WDT_ALIVE_TCPIP_TASK		 BIT (3)	//TCP/IP Task
//#define WDT_ALIVE_WPASUP_TASK		 BIT (4)	//WPA Supplicant Task
//#define WDT_ALIVE_MASK		(BIT(3)|BIT(2)|BIT(1)|BIT(0))
#define WDT_ALIVE_MASK			(BIT(1)|BIT(0))
#endif

//external functions
void system_wdt_set_alive(uint16_t alive_flag);
uint16_t system_wdt_get_alive();
void system_wdt_isr();
#endif //#if defined (INCLUDE_WDT)

/* Fast Work Task */
//struct
struct fast_task_stats {
	uint32_t tbtt_cnt;
	uint32_t wake_cnt;
	uint32_t recovery_cnt;
	uint32_t wdt_mon_cnt;
};

//external functions
void fast_task_init();
bool fast_task_work_queue_from_isr(TQUEUE_MESSAGE *msg);
bool fast_task_work_queue(TQUEUE_MESSAGE *msg);
struct fast_task_stats* fast_task_get_stats_handle();

#define INIT_TQM(v, p, e) 								\
		TQUEUE_MESSAGE tq_message; 						\
		tq_message.vif_id = v; 							\
		tq_message.param = (void *)p; 					\
		tq_message.q_event_id = e;

#define TQM	&tq_message


#endif //#if defined (INCLUDE_NEW_TASK_ARCH)

void print_boot_reason(uint8_t br);
void print_hif_hdr(HIF_HDR * hif_hdr);
void print_hex(void *address, int count);
void print_sysbuf_hex(SYS_BUF *head);
void print_sys_hdr(SYS_BUF *head);
int linearize_sysbuf(SYS_BUF *src, uint8_t *dst, int src_offset,
			int src_offset2, int length);
void *system_get_idle_hook();
void *system_set_idle_hook(void *idlehook);

int get_number_of_buffer(struct _SYS_BUF *packet);
uint64_t system_get_systick_ms(void);
uint64_t system_get_time(void);
void system_time_reset(void);
#define TRACE A("%s %d\n", __func__, __LINE__)
#endif /* SYSTEM_COMMON_H */
