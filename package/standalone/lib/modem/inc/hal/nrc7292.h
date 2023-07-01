 /**************************************************************************//**
 * @file     nrc7292.h
 * @brief    CMSIS Core Peripheral Access Layer Header File for
 *           NRC7292 Device
 * @version  V1.0.0
 * @date     03. April 2022
 ******************************************************************************/
/*
 * Copyright (c) 2022 Newracom Inc. All rights reserved.
 */

#ifndef __NRC7292_H__
#define __NRC7292_H__

#ifdef __cplusplus
extern "C" {
#endif



/* -------------------------  Interrupt Number Definition  ------------------------ */

typedef enum IRQn
{
/* -------------------  Processor Exceptions Numbers  ----------------------------- */
  NonMaskableInt_IRQn           = -14,     /*  2 Non Maskable Interrupt */
  HardFault_IRQn                = -13,     /*  3 HardFault Interrupt */
  MemoryManagement_IRQn         = -12,     /*  4 Memory Management Interrupt */
  BusFault_IRQn                 = -11,     /*  5 Bus Fault Interrupt */
  UsageFault_IRQn               = -10,     /*  6 Usage Fault Interrupt */
  SVCall_IRQn                   =  -5,     /* 11 SV Call Interrupt */
  DebugMonitor_IRQn             =  -4,     /* 12 Debug Monitor Interrupt */
  PendSV_IRQn                   =  -2,     /* 14 Pend SV Interrupt */
  SysTick_IRQn                  =  -1,     /* 15 System Tick Interrupt */

/* -------------------  Processor Interrupt Numbers  ------------------------------ */
  PMU_IRQn						=   0,      /*!< PMU Interrupt	                    */
  TIMER4_IRQn					=   1,      /*!< TIMER4 Interrupt	                    */
  RTC_IRQn		       			=   2,      /*!< RTC Interrupt                      */
  WDT_IRQn		       			=   3,      /*!< WDT Interrupt                      */
  TIMER0_IRQn					=   4,      /*!< TIMER0 Interrupt                   */
  WIFI0_IRQn					=   5,      /*!< WIFI0  Interrupt                   */
  GDMAINTTC_IRQn				=   6,      /*!< DMAINTTC  Interrupt                */
  TIMER5_IRQn					=   7,      /*!< TIMER5 Interrupt	        	    */
  TIMER1_IRQn					=   8,      /*!< TIMER1 Interrupt	        	    */
  HSUART0_IRQn					=   9,      /*!< HSUART0 Interrupt                  */
  SSP0_IRQn						=  10,      /*!< SSP0 Interrupt                     */
  PWR_IRQn                      =  11,      /*!< PWR Interrupt                      */
  LPO_IRQn                      =  12,      /*!< LPO     Interrupt                  */
  GDMAINTERR_IRQn               =  13,      /*!< DMAINTERR Interrupt                */
  HSUART3_IRQn					=  15,		/*!< SECIP Interrupt               		*/
  TIMER2_IRQn                   =  16,      /*!< TIMER2 Interrupt                   */
  HSUART1_IRQn                  =  17,      /*!< HSUART1 Interrupt                  */
  SSP1_IRQn                     =  18,      /*!< SSP1 Interrupt                     */
  I2C_IRQn                      =  19,      /*!< I2C ADC/DAC Interrupt              */
  WIFI1_IRQn                    =  20,      /*!< WIFI1  Interrupt                   */
  WIFI2_IRQn                    =  21,      /*!< WIFI2  Interrupt                   */
  WIFI3_IRQn               		=  22,      /*!< WIFI3  Interrupt                   */
  RTCEST_IRQn			  		=  23,      /*!< RTCEST_IRQn Interrupt              */
  TIMER3_IRQn                   =  24,      /*!< TIMER3 Interrupt                   */
  HSUART2_IRQn                  =  25,      /*!< HSUART1 Interrupt                  */
  MBXTX_IRQn					=  26,		/*!< SMC Interrupt   	                */
  MBXRX_IRQn					=  27,		/*!< SMC Interrupt   	                */
  EXTINT0_IRQn                  =  28,      /*!< External0 Interrupt                */
  EXTINT1_IRQn                  =  29,      /*!< External1 Interrupt                */
  TXQUE_IRQn                    =  30,      /*!< HOST_INF_TX Interrupt              */
  RXQUE_IRQn                    =  31,      /*!< HOST_INF_RX Interrupt              */
} IRQn_Type;


/* ================================================================================ */
/* ================      Processor and Core Peripheral Section     ================ */
/* ================================================================================ */

/* -------  Start of section using anonymous unions and disabling warnings  ------- */
#if   defined (__CC_ARM)
  #pragma push
  #pragma anon_unions
#elif defined (__ICCARM__)
  #pragma language=extended
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wc11-extensions"
  #pragma clang diagnostic ignored "-Wreserved-id-macro"
#elif defined (__GNUC__)
  /* anonymous unions are enabled by default */
#elif defined (__TMS470__)
  /* anonymous unions are enabled by default */
#elif defined (__TASKING__)
  #pragma warning 586
#elif defined (__CSMC__)
  /* anonymous unions are enabled by default */
#else
  #warning Not supported compiler type
#endif


/* --------  Configuration of Core Peripherals  ----------------------------------- */
#define __CM3_REV                 0x0201U   /* Core revision r2p1 */
#define __MPU_PRESENT             1U        /* MPU present */
#define __VTOR_PRESENT            1U        /* VTOR present */
#define __NVIC_PRIO_BITS          3U        /* Number of Bits used for Priority Levels */
#define __Vendor_SysTickConfig    0U        /* Set to 1 if different SysTick Config is used */

//#include "core_cm3.h"                       /* Processor and core peripherals */
#include "system_nrc7292.h"                  /* System Header */


/* =========================================================================================================================== */
/* ================                            Device Specific Peripheral Section                             ================ */
/* =========================================================================================================================== */


/** @addtogroup Device_Peripheral_peripherals
  * @{
  */

#include "reg_sfc.h"
#include "reg_hsuart.h"
#include "reg_gdma.h"
#include "reg_auxadc.h"
#include "reg_efuse.h"
#include "reg_ssp.h"


#include "reg_gpio.h"
#include "reg_ckc.h"
#include "reg_scfg.h"


/*
#include "reg_pmsspi.h"

#include "reg_wdt.h"
#include "reg_timer_32bit.h"
#include "reg_timer_64bit.h"
#include "reg_rtc.h"
#include "reg_rtccal.h"
#include "reg_lpocal.h"
#include "reg_pmu.h"
#include "reg_hspi.h"
#include "reg_pwm.h"
#include "reg_rdma.h"
#include "reg_rf.h"
#include "reg_i2c.h"
#include "reg_phy.h"
#include "reg_mac.h"
*/

/*@}*/ /* end of group NRC7292_Peripherals */



/* --------  End of section using anonymous unions and disabling warnings  -------- */
#if   defined (__CC_ARM)
  #pragma pop
#elif defined (__ICCARM__)
  /* leave anonymous unions enabled */
#elif (defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050))
  #pragma clang diagnostic pop
#elif defined (__GNUC__)
  /* anonymous unions are enabled by default */
#elif defined (__TMS470__)
  /* anonymous unions are enabled by default */
#elif defined (__TASKING__)
  #pragma warning restore
#elif defined (__CSMC__)
  /* anonymous unions are enabled by default */
#else
  #warning Not supported compiler type
#endif


/* =========================================================================================================================== */
/* ================                          Device Specific Peripheral Address Map                           ================ */
/* =========================================================================================================================== */

/* Peripheral and SRAM base address */
#define FLASH_BASE					((     uint32_t)0x00000000)
#define BOOTROM_CM0_BASE			((     uint32_t)0x10100000)
#define BOOTROM_CM3_BASE			((     uint32_t)0x10200000)
#define SRAM0_BASE					((     uint32_t)0x20000000)
#define SRAM1_BASE					((     uint32_t)0x20040000)
#define SRAM2_BASE					((     uint32_t)0x20080000)
#define SRAM3_BASE					((     uint32_t)0x204C0000)

#define SRAM3_RETENTION_BASE		((     uint32_t)0x204C0000)

#define APB0_BASE					((     uint32_t)0x40000000)
#define APB1_BASE					((     uint32_t)0x40040000)

/* Peripheral memory map */
#define WIFI_BASE					(APB0_BASE		+ 0x80000)

#define HSPI_BASE					(APB0_BASE		+ 0x90000)

#define GDMA_BASE					(APB0_BASE		+ 0x91000)

#define SFC_BASE					(APB0_BASE		+ 0x92000)

#define MAILBOX_BASE				(APB0_BASE		+ 0x93000)

#define RTC_BASE           			(APB0_BASE		+ 0x00000)

#define SCU_BASE      				(APB0_BASE		+ 0x01000)

#define PMU_BASE       				(APB0_BASE		+ 0x02000)

#define WDT_BASE          			(APB0_BASE		+ 0x03000)

#define TIMER0_BASE        			(APB0_BASE		+ 0x04000)
#define TIMER1_BASE        			(APB0_BASE		+ 0x04100)
#define TIMER2_BASE        			(APB0_BASE		+ 0x04200)
#define TIMER3_BASE        			(APB0_BASE		+ 0x04300)
#define TIMER4_BASE        			(APB1_BASE		+ 0x00000)
#define TIMER5_BASE        			(APB1_BASE		+ 0x00100)

#define GPIO_BASE          			(APB0_BASE		+ 0x05000)
#define GPIOBUS_BASE         		(APB0_BASE		+ 0x05800)

#define EINT_BASE          			(APB0_BASE		+ 0x05838)

#define I2C0_BASE          			(APB0_BASE		+ 0x06000)
#define I2C1_BASE          			(APB0_BASE		+ 0x06040)
#define I2C2_BASE          			(APB0_BASE		+ 0x06100)
#define I2C3_BASE          			(APB0_BASE		+ 0x06140)

#define SSP0_BASE          			(APB0_BASE		+ 0x07000)
#define SSP1_BASE          			(APB0_BASE		+ 0x08000)
#define SSP2_BASE          			(APB0_BASE		+ 0x0B000)
#define SSP3_BASE          			(APB0_BASE		+ 0x0C000)

#define HSUART0_BASE       			(APB0_BASE		+ 0x09000)
#define HSUART1_BASE       			(APB0_BASE		+ 0x0A000)
#define HSUART2_BASE       			(APB1_BASE		+ 0x01000)
#define HSUART3_BASE       			(APB1_BASE		+ 0x02000)

#define PWM0_BASE          			(APB0_BASE		+ 0x0D000)
#define PWM1_BASE          			(APB0_BASE		+ 0x0D100)

#define SCFG_BASE          			(APB0_BASE		+ 0x0F000)

#define RTCCAL_BASE        			(APB1_BASE		+ 0x0A000)

#define LPOCAL_BASE        			(APB1_BASE		+ 0x0B000)

#define AUXADC_BASE        			(APB1_BASE		+ 0x0C000)

#define EFUSE_BASE         			(APB1_BASE		+ 0x0D000)

#define PMS_BASE           			(APB1_BASE		+ 0x0E000)

#define RF_BASE            			(APB1_BASE		+ 0x0F000)


/** @} */ /* End of group Device_Peripheral_peripheralAddr */


/******************************************************************************/
/*                         Peripheral Definitions                             */
/******************************************************************************/
#define GDMA		((volatile GDMA_Type *)(GDMA_BASE))

#define SFC			((volatile SFC_Type *)(SFC_BASE))

#define SCU			((volatile SCU_Type	*)(SCU_BASE))

#define GPIO		((volatile GPIO_Type *)(GPIO_BASE))

#define SSP0		((volatile SSP_Type *)(SSP0_BASE))
#define SSP1		((volatile SSP_Type *)(SSP1_BASE))
#define SSP2		((volatile SSP_Type *)(SSP2_BASE))
#define SSP3		((volatile SSP_Type *)(SSP3_BASE))

#define HSUART0		((volatile HSUART_Type *)(HSUART0_BASE))
#define HSUART1		((volatile HSUART_Type *)(HSUART1_BASE))
#define HSUART2		((volatile HSUART_Type *)(HSUART2_BASE))
#define HSUART3		((volatile HSUART_Type *)(HSUART3_BASE))

#define AUXADC		((volatile AUXADC_Type *)(AUXADC_BASE))

#define SCFG		((volatile SCFG_Type *)(SCFG_BASE))

#define EFUSE		((volatile EFUSE_Type *)(EFUSE_BASE))

/*
#define HSPI		((volatile HSPI_Type *)(HSPI_BASE))

#define RTC			((volatile RTC_Type *)(RTC_BASE))



#define PMU			((volatile PMU_Type *)(PMU_BASE))

#define WDT			((volatile WDT_Type *)(WDT_BASE))

#define TIMER0		((volatile TIMER_32BIT_Type *)(TIMER0_BASE))
#define TIMER1		((volatile TIMER_32BIT_Type *)(TIMER1_BASE))
#define TIMER2		((volatile TIMER_32BIT_Type *)(TIMER2_BASE))
#define TIMER3		((volatile TIMER_64BIT_Type *)(TIMER3_BASE))
#define TIMER4		((volatile TIMER_32BIT_Type *)(TIMER4_BASE))
#define TIMER5		((volatile TIMER_64BIT_Type *)(TIMER5_BASE))



#define I2C0		((volatile I2C_Type *)(I2C0_BASE))
#define I2C1		((volatile I2C_Type *)(I2C1_BASE))
#define I2C2		((volatile I2C_Type *)(I2C2_BASE))
#define I2C3		((volatile I2C_Type *)(I2C3_BASE))

#define PWM			((volatile PWM_Type *)(PWM_BASE))



#define LPOCAL		((volatile LPOCAL_Type *)(LPOCAL_BASE))

#define RTCCAL		((volatile RTCCAL_Type *)(RTCCAL_BASE))



#define PMSSPI		((volatile PMSSPI_Type *)PMSSPI_BASE)

#define RF          ((volatile RF_Type *)(RF_BASE))

*/

#ifdef __cplusplus
}
#endif

#endif  /* __NRC7292_H__ */
