#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>

#define SYSTEM_BASE 			0x40000000
#define SYSTEM_APB1_BASE 		0x40040000

//#define RETENTION __attribute__((section(".retent")))
#define RETENTION
#ifndef NULL
#define NULL ((void*)0)
#endif

#if defined (INCLUDE_RUNRAM)
#define ATTR_RUNRAM		__attribute__((section(".run_ram")))
#else
#define ATTR_RUNRAM
#endif

/*****************************************************************************************************************
* Global Definition
******************************************************************************************************************/

#define XTAL_MAIN	(32*MHz)
#define XTAL_PLL 	(96*MHz)
#define XTAL_PMIC   (16*MHz)
#define XTAL_RTC    (32.768*KHz)

#define BIT0    (0x00000001L << 0)
#define BIT1    (0x00000001L << 1)
#define BIT2    (0x00000001L << 2)
#define BIT3    (0x00000001L << 3)
#define BIT4    (0x00000001L << 4)
#define BIT5    (0x00000001L << 5)
#define BIT6    (0x00000001L << 6)
#define BIT7    (0x00000001L << 7)
#define BIT8    (0x00000001L << 8)
#define BIT9    (0x00000001L << 9)
#define BIT10   (0x00000001L << 10)
#define BIT11   (0x00000001L << 11)
#define BIT12   (0x00000001L << 12)
#define BIT13   (0x00000001L << 13)
#define BIT14   (0x00000001L << 14)
#define BIT15   (0x00000001L << 15)
#define BIT16   (0x00000001L << 16)
#define BIT17   (0x00000001L << 17)
#define BIT18   (0x00000001L << 18)
#define BIT19   (0x00000001L << 19)
#define BIT20   (0x00000001L << 20)
#define BIT21   (0x00000001L << 21)
#define BIT22   (0x00000001L << 22)
#define BIT23   (0x00000001L << 23)
#define BIT24   (0x00000001L << 24)
#define BIT25   (0x00000001L << 25)
#define BIT26   (0x00000001L << 26)
#define BIT27   (0x00000001L << 27)
#define BIT28   (0x00000001L << 28)
#define BIT29   (0x00000001L << 29)
#define BIT30   (0x00000001L << 30)
#define BIT31   (0x00000001L << 31)
#define BITZero    (0x00000000L)

//===================================================================================================================
// Subsystem delimiter
//===================================================================================================================
#define CLOCK_BASE_ADDR  (SYSTEM_BASE + 0x1000)

#define RegCLK_CTRL        	(*((volatile uint32_t *)(CLOCK_BASE_ADDR + 0x00)))
#define RegCLK_DIV     		(*((volatile uint32_t *)(CLOCK_BASE_ADDR + 0x04)))
#define RegCLK_WIFI        	(*((volatile uint32_t *)(CLOCK_BASE_ADDR + 0x08)))
#define RegCLK_BR      		(*((volatile uint32_t *)(CLOCK_BASE_ADDR + 0x0C)))
#define RegSW_RESET        	(*((volatile uint32_t *)(CLOCK_BASE_ADDR + 0x10)))
#define RegHCLK_MASK   		(*((volatile uint32_t *)(CLOCK_BASE_ADDR + 0x18)))

#define RegCLK_CFG00_TIMER0        (*((volatile uint32_t *)(CLOCK_BASE_ADDR + 0x20)))
#define RegCLK_CFG01_TIMER1        (*((volatile uint32_t *)(CLOCK_BASE_ADDR + 0x24)))
#define RegCLK_CFG02_TIMER2        (*((volatile uint32_t *)(CLOCK_BASE_ADDR + 0x28)))
#define RegCLK_CFG03_TIMER3        (*((volatile uint32_t *)(CLOCK_BASE_ADDR + 0x2C)))
#define RegCLK_CFG04_TIMER4        (*((volatile uint32_t *)(CLOCK_BASE_ADDR + 0x30)))
#define RegCLK_CFG05_TIMER5        (*((volatile uint32_t *)(CLOCK_BASE_ADDR + 0x34)))

#define RegCLK_CFG08_PWM0      (*((volatile uint32_t *)(CLOCK_BASE_ADDR + 0x40)))
#define RegCLK_CFG09_PWM1      (*((volatile uint32_t *)(CLOCK_BASE_ADDR + 0x44)))

#define RegCLK_CFG12_SPI0      (*((volatile uint32_t *)(CLOCK_BASE_ADDR + 0x50)))
#define RegCLK_CFG13_SPI1      (*((volatile uint32_t *)(CLOCK_BASE_ADDR + 0x54)))
#define RegCLK_CFG14_SPI2      (*((volatile uint32_t *)(CLOCK_BASE_ADDR + 0x58)))
#define RegCLK_CFG15_SPI3      (*((volatile uint32_t *)(CLOCK_BASE_ADDR + 0x5C)))

#define RegCLK_CFG16_UART0     (*((volatile uint32_t *)(CLOCK_BASE_ADDR + 0x60)))
#define RegCLK_CFG17_UART1     (*((volatile uint32_t *)(CLOCK_BASE_ADDR + 0x64)))
#define RegCLK_CFG18_UART2     (*((volatile uint32_t *)(CLOCK_BASE_ADDR + 0x68)))
#define RegCLK_CFG19_UART3     (*((volatile uint32_t *)(CLOCK_BASE_ADDR + 0x6C)))
#define RegCLK_CFG20_I2C       (*((volatile uint32_t *)(CLOCK_BASE_ADDR + 0x70)))

//Wi-Fi
#define CLK_WIFI_EN            BIT31
#define CLK_WIFI_ADDA_96M_EN       BIT20
#define CLK_WIFI_ADDA_32M_EN       BIT19
#define CLK_WIFI_ADDA_16M_EN       BIT18
#define CLK_WIFI_ADDA_8M_EN        BIT17
#define CLK_WIFI_ADDA_4M_EN        BIT16
#define CLK_WIFI_PHY_16M_EN        BIT12
#define CLK_WIFI_PHY_8M_EN     BIT11
#define CLK_WIFI_PHY_4M_EN     BIT10
#define CLK_WIFI_PHY_2M_EN     BIT9
#define CLK_WIFI_PHY_1M_EN     BIT8
#define CLK_WIFI_MAC_TIMER_EN      BIT2
#define CLK_WIFI_MAC_SEC_EN        BIT1
#define CLK_WIFI_MAC_EN            BIT0

//Boot Reason
#define BR_SYSRST_CM0  BIT5
#define BR_SYSRST_CM3  BIT4
#define BR_HOSTINF     BIT3
#define BR_PMC         BIT2
#define BR_WDOG        BIT1
#define BR_POR         BIT0

//HCLK SEL
#define CLK_SEL_LPO16M     0
#define CLK_SEL_LPO16M_DIV 1
#define CLK_SEL_PLL        2
#define CLK_SEL_PLL_DIV    3
#define CLK_SEL_XIN        4
#define CLK_SEL_XIN_DIV    5
#define CLK_SEL_XTIN       6
#define CLK_SEL_XTIN_DIV   7

//RTC CLK SEL
#define RCLK_SEL_EXT       0
#define RCLK_SEL_LPO       1

//CLK MASK
#define CLK_MASK_CM0_F     BIT31
#define CLK_MASK_CM0_H     BIT30
#define CLK_MASK_CM3_F     BIT29
#define CLK_MASK_CM3_H     BIT28
#define CLK_MASK_AHB       BIT27
#define CLK_MASK_IMC3      BIT24
#define CLK_MASK_ROM_CM0   BIT23
#define CLK_MASK_ROM_CM3   BIT22
#define CLK_MASK_SCFG      BIT21
#define CLK_MASK_CKC       BIT20
#define CLK_MASK_PMC       BIT19
#define CLK_MASK_GPIO      BIT18
#define CLK_MASK_WDOG      BIT17
#define CLK_MASK_RTC       BIT16
#define CLK_MASK_HOSTIF        BIT15
#define CLK_MASK_WIFI      BIT14
#define CLK_MASK_GDMA      BIT13
#define CLK_MASK_XIP       BIT12
#define CLK_MASK_UART3     BIT11
#define CLK_MASK_UART2     BIT10
#define CLK_MASK_UART1     BIT9
#define CLK_MASK_UART0     BIT8
#define CLK_MASK_SPI3      BIT7
#define CLK_MASK_SPI2      BIT6
#define CLK_MASK_SPI1      BIT5
#define CLK_MASK_SPI0      BIT4
#define CLK_MASK_I2C       BIT3
#define CLK_MASK_PWM       BIT2
#define CLK_MASK_TIMER1        BIT1
#define CLK_MASK_TIMER0        BIT0

//SW RESET
#define SW_RESET_CM0_F     (CLK_MASK_CM0_F)
#define SW_RESET_CM0_H     (CLK_MASK_CM0_H)
#define SW_RESET_CM3_F     (CLK_MASK_CM3_F)
#define SW_RESET_CM3_H     (CLK_MASK_CM3_H)
#define SW_RESET_AHB       (CLK_MASK_AHB)
#define SW_RESET_IMC3      (CLK_MASK_IMC3)
#define SW_RESET_ROM_CM0   (CLK_MASK_ROM_CM0)
#define SW_RESET_ROM_CM3   (CLK_MASK_ROM_CM3)
#define SW_RESET_SCFG      (CLK_MASK_SCFG)
#define SW_RESET_CKC       (CLK_MASK_CKC)
#define SW_RESET_PMC       (CLK_MASK_PMC)
#define SW_RESET_GPIO      (CLK_MASK_GPIO)
#define SW_RESET_WDOG      (CLK_MASK_WDOG)
#define SW_RESET_RTC       (CLK_MASK_RTC)
#define SW_RESET_HOSTIF        (CLK_MASK_HOSTIF)
#define SW_RESET_WIFI      (CLK_MASK_WIFI)
#define SW_RESET_GDMA      (CLK_MASK_GDMA)
#define SW_RESET_XIP       (CLK_MASK_XIP)
#define SW_RESET_UART3     (CLK_MASK_UART3)
#define SW_RESET_UART2     (CLK_MASK_UART2)
#define SW_RESET_UART1     (CLK_MASK_UART1)
#define SW_RESET_UART0     (CLK_MASK_UART0)
#define SW_RESET_SPI3      (CLK_MASK_SPI3)
#define SW_RESET_SPI2      (CLK_MASK_SPI2)
#define SW_RESET_SPI1      (CLK_MASK_SPI1)
#define SW_RESET_SPI0      (CLK_MASK_SPI0)
#define SW_RESET_I2C       (CLK_MASK_I2C)
#define SW_RESET_PWM       (CLK_MASK_PWM)
#define SW_RESET_TIMER1        (CLK_MASK_TIMER1)
#define SW_RESET_TIMER0        (CLK_MASK_TIMER0)

//===================================================================================================================
// AUX ADC
//===================================================================================================================
//#define ADC_BASE_ADDR  (SYSTEM_BASE + 0x0080)
#define ADC_BASE_ADDR  (SYSTEM_BASE + 0x0004C000) /* 0x4004C000 */

#define RegADC_CTRL   						(*((volatile uint32_t *)(ADC_BASE_ADDR + 0x00)))
#define RegADC_DATA01   					(*((volatile uint32_t *)(ADC_BASE_ADDR + 0x04)))
#define RegADC_DATA23  						(*((volatile uint32_t *)(ADC_BASE_ADDR + 0x08)))

//===================================================================================================================
// PWM (Pulse Width Modulation) Defines
//===================================================================================================================
#define PWM_BASE_ADDR (SYSTEM_BASE + 0xD000)

#define PWM_BASE(CH)		(PWM_BASE_ADDR + CH*0x100)


#define RegPWM_ST(CH)		(*((volatile uint32_t *)(PWM_BASE_ADDR + CH*0x100 + 0x00)))
#define RegPWM_EN(CH)		(*((volatile uint32_t *)(PWM_BASE_ADDR + CH*0x100 + 0x04)))
#define RegPWM_MODE(CH)		(*((volatile uint32_t *)(PWM_BASE_ADDR + CH*0x100 + 0x08)))
#define RegPWM_LOOP(CH)		(*((volatile uint32_t *)(PWM_BASE_ADDR + CH*0x100 + 0x0C)))

#define RegPWM_AP12(CH)		(*((volatile uint32_t *)(PWM_BASE_ADDR + CH*0x100 + 0x10)))
#define RegPWM_AP34(CH)		(*((volatile uint32_t *)(PWM_BASE_ADDR + CH*0x100 + 0x14)))
#define RegPWM_BP12(CH)		(*((volatile uint32_t *)(PWM_BASE_ADDR + CH*0x100 + 0x18)))
#define RegPWM_BP34(CH)		(*((volatile uint32_t *)(PWM_BASE_ADDR + CH*0x100 + 0x1C)))
#define RegPWM_CP12(CH)		(*((volatile uint32_t *)(PWM_BASE_ADDR + CH*0x100 + 0x20)))
#define RegPWM_CP34(CH)		(*((volatile uint32_t *)(PWM_BASE_ADDR + CH*0x100 + 0x24)))
#define RegPWM_DP12(CH)		(*((volatile uint32_t *)(PWM_BASE_ADDR + CH*0x100 + 0x28)))
#define RegPWM_DP34(CH)		(*((volatile uint32_t *)(PWM_BASE_ADDR + CH*0x100 + 0x2C)))

#define RegPWM_OUTA1(CH)	(*((volatile uint32_t *)(PWM_BASE_ADDR + CH*0x100 + 0x30)))
#define RegPWM_OUTA2(CH)	(*((volatile uint32_t *)(PWM_BASE_ADDR + CH*0x100 + 0x34)))
#define RegPWM_OUTA3(CH)	(*((volatile uint32_t *)(PWM_BASE_ADDR + CH*0x100 + 0x38)))
#define RegPWM_OUTA4(CH)	(*((volatile uint32_t *)(PWM_BASE_ADDR + CH*0x100 + 0x3C)))
#define RegPWM_OUTB1(CH)	(*((volatile uint32_t *)(PWM_BASE_ADDR + CH*0x100 + 0x40)))
#define RegPWM_OUTB2(CH)	(*((volatile uint32_t *)(PWM_BASE_ADDR + CH*0x100 + 0x44)))
#define RegPWM_OUTB3(CH)	(*((volatile uint32_t *)(PWM_BASE_ADDR + CH*0x100 + 0x48)))
#define RegPWM_OUTB4(CH)	(*((volatile uint32_t *)(PWM_BASE_ADDR + CH*0x100 + 0x4C)))
#define RegPWM_OUTC1(CH)	(*((volatile uint32_t *)(PWM_BASE_ADDR + CH*0x100 + 0x50)))
#define RegPWM_OUTC2(CH)	(*((volatile uint32_t *)(PWM_BASE_ADDR + CH*0x100 + 0x54)))
#define RegPWM_OUTC3(CH)	(*((volatile uint32_t *)(PWM_BASE_ADDR + CH*0x100 + 0x58)))
#define RegPWM_OUTC4(CH)	(*((volatile uint32_t *)(PWM_BASE_ADDR + CH*0x100 + 0x5C)))
#define RegPWM_OUTD1(CH)	(*((volatile uint32_t *)(PWM_BASE_ADDR + CH*0x100 + 0x60)))
#define RegPWM_OUTD2(CH)	(*((volatile uint32_t *)(PWM_BASE_ADDR + CH*0x100 + 0x64)))
#define RegPWM_OUTD3(CH)	(*((volatile uint32_t *)(PWM_BASE_ADDR + CH*0x100 + 0x68)))
#define RegPWM_OUTD4(CH)	(*((volatile uint32_t *)(PWM_BASE_ADDR + CH*0x100 + 0x6C)))

//===================================================================================================================
// TIMER
//===================================================================================================================
#define TIMER_CH_MAX (6)

#define SYS_TIMER_0 (0)  //APB0 32bit Timer
#define SYS_TIMER_1 (1)  //APB0 32bit Timer
#define SYS_TIMER_2 (2)  //APB0 32bit Timer
#define SYS_TIMER_3 (3)  //APB0 64bit Timer
#define SYS_TIMER_4 (4)  //APB1 32bit Timer
#define SYS_TIMER_5 (5)  //APB1 64bit Timer

#define TIMER_BASE_ADDR(APB) (SYSTEM_BASE + 0x4000 + APB*0x40000 - APB*0x4000)

#define RegTimer_BASE_ADDR(APB,CH)   ((volatile uint32_t *)(TIMER_BASE_ADDR(APB) + CH*0x100 + 0x00))
#define RegTimer_TCCTRL(APB,CH)    (*((volatile uint32_t *)(TIMER_BASE_ADDR(APB) + CH*0x100 + 0x00)))
#define RegTimer_TCLDV(APB,CH)     (*((volatile uint32_t *)(TIMER_BASE_ADDR(APB) + CH*0x100 + 0x04)))
#define RegTimer_TCMV0(APB,CH)     (*((volatile uint32_t *)(TIMER_BASE_ADDR(APB) + CH*0x100 + 0x08)))
#define RegTimer_TCMV1(APB,CH)     (*((volatile uint32_t *)(TIMER_BASE_ADDR(APB) + CH*0x100 + 0x0C)))
#define RegTimer_TCPCNT(APB,CH)    (*((volatile uint32_t *)(TIMER_BASE_ADDR(APB) + CH*0x100 + 0x10)))
#define RegTimer_TCMCNT(APB,CH)    (*((volatile uint32_t *)(TIMER_BASE_ADDR(APB) + CH*0x100 + 0x14)))
#define RegTimer_TCIRQ(APB,CH)     (*((volatile uint32_t *)(TIMER_BASE_ADDR(APB) + CH*0x100 + 0x18)))
#define RegTimer_TCIRQ64(APB,CH)   (*((volatile uint32_t *)(TIMER_BASE_ADDR(APB) + CH*0x100 + 0x18)))
#define RegTimer_TCMCNT1(APB,CH)   (*((volatile uint32_t *)(TIMER_BASE_ADDR(APB) + CH*0x100 + 0x1c)))

////////////////////////////////////////////////////////////////////////////////
// GPIO / IOMUX
////////////////////////////////////////////////////////////////////////////////
#define GPIO_BASE_ADDR   (SYSTEM_BASE + 0x5000)
#define GPIO_IO_MAX (32)

#define GPIO_OUT (1)
#define GPIO_IN (0)

#define IOMUX_BASE_ADDR   (SYSTEM_BASE + 0x5800)

//===================================================================================================================
// I2C
//===================================================================================================================
#define I2C_BASE_ADDR	(SYSTEM_BASE + 0x6000)
#define I2C_BASE(CH)	(I2C_BASE_ADDR + CH*0x40 + (CH>>1)*0x80)

#define RegI2C_PRES(CH)		(*((volatile uint32_t *)(I2C_BASE(CH) + 0x00)))
#define RegI2C_CTRL(CH)		(*((volatile uint32_t *)(I2C_BASE(CH) + 0x04)))
#define RegI2C_TXR(CH)		(*((volatile uint32_t *)(I2C_BASE(CH) + 0x08)))
#define RegI2C_CMD(CH)		(*((volatile uint32_t *)(I2C_BASE(CH) + 0x0C)))
#define RegI2C_RXR(CH)		(*((volatile uint32_t *)(I2C_BASE(CH) + 0x10)))
#define RegI2C_SR(CH)		(*((volatile uint32_t *)(I2C_BASE(CH) + 0x14)))
#define RegI2C_TR(CH)		(*((volatile uint32_t *)(I2C_BASE(CH) + 0x18)))
#define RegI2C_SWRST		(*((volatile uint32_t *)(I2C_BASE_ADDR + 0x210)))
#define RegI2C_STATUS		(*((volatile uint32_t *)(I2C_BASE_ADDR + 0x214)))

//===================================================================================================================
// HSUART (PrimeCell UART PL011) Defines
//===================================================================================================================
#define HSUART0_BASE_ADDR 		(SYSTEM_BASE + 0x9000)
#define HSUART1_BASE_ADDR 		(SYSTEM_BASE + 0xA000)
#define HSUART2_BASE_ADDR 		(SYSTEM_APB1_BASE + 0x1000)
#define HSUART3_BASE_ADDR 		(SYSTEM_APB1_BASE + 0x2000)
#define RegHSUART_DR(base)		(*((volatile uint32_t *)(base + 0x00)))
#define RegHSUART_RSRECR(base) (*((volatile uint32_t *)(base + 0x04)))
#define RegHSUART_FR(base)  	(*((volatile uint32_t *)(base + 0x18)))
#define RegHSUART_IBRD(base)	(*((volatile uint32_t *)(base + 0x24)))
#define RegHSUART_FBRD(base)	(*((volatile uint32_t *)(base + 0x28)))
#define RegHSUART_LCRH(base)	(*((volatile uint32_t *)(base + 0x2c)))
#define RegHSUART_CR(base)  	(*((volatile uint32_t *)(base + 0x30)))
#define RegHSUART_FLS(base) 	(*((volatile uint32_t *)(base + 0x34)))
#define RegHSUART_IMSC(base)	(*((volatile uint32_t *)(base + 0x38)))
#define RegHSUART_MIS(base) 	(*((volatile uint32_t *)(base + 0x40)))
#define RegHSUART_ICR(base) 	(*((volatile uint32_t *)(base + 0x44)))
#define RegHSUART_DMACR(base)	(*((volatile uint32_t *)(base + 0x48)))

#define FR_RI       BIT8
#define FR_TXFE     BIT7
#define FR_RXFF     BIT6
#define FR_TXFF     BIT5
#define FR_RXFE     BIT4
#define FR_BUSY     BIT3
#define FR_DCD      BIT2
#define FR_DSR      BIT1
#define FR_CTS      BIT0

#define MIS_OE      BIT10
#define MIS_BE      BIT9
#define MIS_PE      BIT8
#define MIS_FE      BIT7
#define MIS_RT      BIT6
#define MIS_TX      BIT5
#define MIS_RX      BIT4
#define MIS_DSRR    BIT3
#define MIS_DCDR    BIT2
#define MIS_CTSR    BIT1
#define MIS_RIR     BIT0

#define ICR_OE      BIT10
#define ICR_BE      BIT9
#define ICR_PE      BIT8
#define ICR_FE      BIT7
#define ICR_RT      BIT6
#define ICR_TX      BIT5
#define ICR_RX      BIT4
#define ICR_DSRM    BIT3
#define ICR_CTSM    BIT2
#define ICR_DCDM    BIT1
#define ICR_RIM     BIT0

#define LCRH_SPS    BIT7
#define LCRH_WLEN8  (BIT6|BIT5)
#define LCRH_WLEN7  (BIT6)
#define LCRH_WLEN6  (BIT5)
#define LCRH_WLEN5  (BITZero)
#define LCRH_FEN    BIT4
#define LCRH_STP2   BIT3
#define LCRH_EPS    BIT2
#define LCRH_PEN    BIT1
#define LCRH_BRK    BIT0

#define CR_CTSE     BIT15
#define CR_RTSE     BIT14
#define CR_OUT2     BIT13
#define CR_OUT1     BIT12
#define CR_RTS      BIT11
#define CR_DTR      BIT10
#define CR_RXE      BIT9
#define CR_TXE      BIT8
#define CR_LBE      BIT7
#define CR_SIRLP    BIT2
#define CR_SIREN    BIT1
#define CR_UARTEN   BIT0

#define IMSC_OE     BIT10
#define IMSC_BE     BIT9
#define IMSC_PE     BIT8
#define IMSC_FE     BIT7
#define IMSC_RT     BIT6
#define IMSC_TX     BIT5
#define IMSC_RX     BIT4
#define IMSC_DSRM   BIT3
#define IMSC_DCDM   BIT2
#define IMSC_CTSM   BIT1
#define IMSC_RIM    BIT0

#define DR_DATA_MASK        (0xFF)
#define RSRECR_ERR_MASK     (0x0F)

//===================================================================================================================
// SSP (PrimeCell SSP PL022) Defines
//===================================================================================================================
#define SSP0_BASE_ADDR 		(SYSTEM_BASE + 0x7000)
#define SSP1_BASE_ADDR 		(SYSTEM_BASE + 0x8000)
#define SSP2_BASE_ADDR 		(SYSTEM_BASE + 0xB000)
#define SSP3_BASE_ADDR 		(SYSTEM_BASE + 0xC000)

#define RegSSP_CR0(base)	(*((volatile uint32_t *)(base + 0x00)))
#define RegSSP_CR1(base)	(*((volatile uint32_t *)(base + 0x04)))
#define RegSSP_DR(base)		(*((volatile uint32_t *)(base + 0x08)))
#define RegSSP_SR(base)		(*((volatile uint32_t *)(base + 0x0C)))
#define RegSSP_CPSR(base)	(*((volatile uint32_t *)(base + 0x10)))
#define RegSSP_IMSC(base)	(*((volatile uint32_t *)(base + 0x14)))
#define RegSSP_RIS(base)	(*((volatile uint32_t *)(base + 0x18)))
#define RegSSP_MIS(base)	(*((volatile uint32_t *)(base + 0x1C)))
#define RegSSP_ICR(base)	(*((volatile uint32_t *)(base + 0x20)))
#define RegSSP_DMACR(base)	(*((volatile uint32_t *)(base + 0x24)))

#define SSP_FIFO_MAX   (8)

#define SSP_INT_RXOV 	BIT0
#define SSP_INT_RXTO 	BIT1
#define SSP_INT_RX 		BIT2
#define SSP_INT_TX 		BIT3

#define SSP_DMA_RX		BIT0
#define SSP_DMA_TX		BIT1


//===================================================================================================================
// EFUSE Defines
//===================================================================================================================
#define EFUSE_BASE_ADDR 		(SYSTEM_APB1_BASE + 0xD000)
#define RegEFUSE_CTRL        	(*((volatile uint32_t *)(EFUSE_BASE_ADDR + 0x00)))
#define RegEFUSE_WD        		(*((volatile uint32_t *)(EFUSE_BASE_ADDR + 0x04)))
#define RegEFUSE_RD        		(*((volatile uint32_t *)(EFUSE_BASE_ADDR + 0x08)))
#define RegEFUSE_INT       		(*((volatile uint32_t *)(EFUSE_BASE_ADDR + 0x0C)))

#define EFUSE_CTRL_ADDR_MASK         (BIT28|BIT27|BIT26|BIT25|BIT24)
#define EFUSE_CTRL_ADDR_SHIFT        (24)
#define EFUSE_CTRL_LD_MASK           (BIT20|BIT19|BIT18|BIT17|BIT16|BIT15|BIT14|BIT13|BIT12)
#define EFUSE_CTRL_LD_SHIFT          (12)
#define EFUSE_CTRL_HOLD_LD_MASK      (BIT11|BIT10)
#define EFUSE_CTRL_HOLD_LD_SHIFT     (10)
#define EFUSE_CTRL_SETUP_LD_MASK     (BIT9|BIT8)
#define EFUSE_CTRL_SETUP_LD_SHIFT    (8)
#define EFUSE_CTRL_MODE    			 (BIT1)
#define EFUSE_CTRL_START  			 (BIT0)

#define EFUSE_INT 					 (BIT0)

typedef union {
	struct {
		uint32_t start 			: 1;
		uint32_t mode 			: 1;
		uint32_t reserved0		: 5;
		uint32_t int_en			: 1;
		uint32_t setup_ld 		: 2;
		uint32_t hold_ld 		: 2;
		uint32_t ld				: 9;
		uint32_t reserved1		: 3;
		uint32_t addr			: 5;
		uint32_t reserved2		: 3;
	} bit;
	uint32_t word;
} efuse_ctrl_t;

typedef union {
	struct {
		uint32_t clear_done	: 1;
	} bit;
	uint32_t word;
} efuse_int_t;

typedef struct {
	volatile efuse_ctrl_t 			ctrl;
	volatile uint32_t				wdata;
	volatile uint32_t				rdata;
	volatile efuse_int_t 			interrupt;
} efuse_controller_t;

//===================================================================================================================
// Cortex Define
//===================================================================================================================

#define EV_DEFAULT      (0)
#define EV_RESET        (1)
#define EV_NMI          (2)
#define EV_HARDFAULT    (3)
#define EV_MEMFAULT     (4)
#define EV_BUSFAULT     (5)
#define EV_USAGEFAULT   (6)

#define EV_RESERVED0    (7)
#define EV_RESERVED1    (8)
#define EV_RESERVED2    (9)
#define EV_RESERVED3    (10)

#define EV_SVC          (11)
#define EV_DEBUG        (12)
#define EV_RESERVED4    (13)
#define EV_PENDSV       (14)
#define EV_SYSTICK      (15)
#define EV_PMU          (16)	//IRQ0
#define EV_TIMER4	    (17)
#define EV_RTC          (18)
#define EV_WDT          (19)
#define EV_TIMER0       (20)
#define EV_HMAC0	    (21)
#define EV_DMACINTTC    (22)
#define EV_TIMER5       (23)
#define EV_TIMER1       (24)
#define EV_HSUART0      (25)
#define EV_SSP0         (26)
#define EV_PWR_FAIL     (27)
#define EV_LPO		    (28)
#define EV_DMACINTERR   (29)
#define EV_HIF		    (30)
#define EV_HSUART3      (31)
#define EV_TIMER2       (32)
#define EV_HSUART1      (33)
#define EV_SSP1         (34)
#define EV_I2C          (35)
#define EV_HMAC1        (36)
#define EV_HMAC2        (37)
#define EV_HIF_TXDONE   (38)
//#if defined(NRC7291_S1)
//#define EV_RTC_EST   	(39)
//#else
#define EV_HIF_RXDONE   (39)
//#endif
#define EV_TIMER3       (40)
#define EV_HSUART2      (41)
#define EV_MBXTX   		(42)
#define EV_MBXRX   		(43)
#define EV_EXT0       	(44)
#define EV_EXT1       	(45)
#define EV_TXQUE        (46)
#define EV_RXQUE        (47)

#define EV_MAX          (48)

#define SETENA  (*((volatile uint32_t *)(0xE000E100)))
#define CLRENA  (*((volatile uint32_t *)(0xE000E180)))

#define SETPEND  (*((volatile uint32_t *)(0xE000E200)))
#define CLRPEND  (*((volatile uint32_t *)(0xE000E280)))

#if defined(CPU_CM3)
#define VTOR (*((volatile uint32_t *)(0xE000ED08)))
#endif
#define SCR  (*((volatile uint32_t *)(0xE000ED10)))


#define IRQ_PRIO_ADDR       (0xE000E400)
#define RegIRQ_PRIO(NUM)  (*((volatile uint32_t *)(IRQ_PRIO_ADDR + NUM*0x04)))

#define IRQ_PRIO0       (0)
#define IRQ_PRIO1       (1)
#define IRQ_PRIO2       (2)
#define IRQ_PRIO3       (3)
#define IRQ_PRIO_MAX    (IRQ_PRIO3)

#define RegSYSTICK_CTRL     (*((volatile uint32_t *)(0xE000E010)))
#define RegSYSTICK_LOAD     (*((volatile uint32_t *)(0xE000E014)))
#define RegSYSTICK_VAL      (*((volatile uint32_t *)(0xE000E018)))
#define RegSYSTICK_CALIB    (*((volatile uint32_t *)(0xE000E01C)))

#define SYSTICK_COUNTFLAG   (BIT16)
#define SYSTICK_CLKSOURCE   (BIT2)
#define SYSTICK_INT         (BIT1)
#define SYSTICK_ENABLE      (BIT0)

#define SYSTICK_NOREF       (BIT31)
#define SYSTICK_SKEW        (BIT30)
#define SYSTICK_RELOAD      (0x00FFFFFF)
#define SYSTICK_CURRENT     (0x00FFFFFF)
#define SYSTICK_TENMS       (0x00FFFFFF)

#define RegCPUID			(*((volatile uint32_t *)(0xE000ED00)))
#define RegICSR				(*((volatile uint32_t *)(0xE000ED04)))
#define ICSR_VECTACTIVE_MASK (0x3F)
#define RegCCR 				(*((volatile uint32_t *)(0xE000ED14)))
#define CCR_STKALIGN		(BIT9)
#define CCR_BFHFNMIGN		(BIT8)
#define CCR_DIV_0_TRP		(BIT4)
#define CCR_UNALIGN_TRP		(BIT3)
#define CCR_USERSETMPEND	(BIT1)
#define CCR_NONBASETHRDENA	(BIT0)

#define RegSHP_MMF 			(*((volatile uint32_t *)(0xE000ED18)))
#define RegSHP_BF 			(*((volatile uint32_t *)(0xE000ED1C)))
#define RegSHP_UF 			(*((volatile uint32_t *)(0xE000ED20)))

// System Handler Control and State Register
#define RegSHCSR 				(*((volatile uint32_t *)(0xE000ED24)))
#define SHCSR_USGFAULTENA		(BIT18)
#define SHCSR_BUSFAULTENA		(BIT17)
#define SHCSR_MEMFAULTENA		(BIT16)
#define SHCSR_SVCALLPENDED		(BIT15)
#define SHCSR_BUSFAULTPENDED	(BIT14)
#define SHCSR_MEMFAULTPENDED	(BIT13)
#define SHCSR_USGFAULTPENDED	(BIT12)
#define SHCSR_SYSTICKACT		(BIT11)
#define SHCSR_PENDSVACT			(BIT10)
#define SHCSR_MONITORACT		(BIT8)
#define SHCSR_SVCALLACT			(BIT7)
#define SHCSR_USGFAULTACT		(BIT3)
#define SHCSR_BUSFAULTACT		(BIT1)
#define SHCSR_MEMFAULTACT		(BIT0)

//Usage Fault Status Register 			[31:16]
//Bus Fault Status Register 			[15:8]
//Memory manage Fault Status Register 	[7:0]
#define RegCFSR 			(*((volatile uint32_t *)(0xE000ED28)))
#define CFSR_DIVBYZERO		(BIT25)
#define CFSR_UNALIGNED		(BIT24)
#define CFSR_NOCP			(BIT19)
#define CFSR_INVPC			(BIT18)
#define CFSR_INVSTATE		(BIT17)
#define CFSR_UNDEFINSTR		(BIT16)
#define CFSR_BFARVALID		(BIT13)
#define CFSR_STKERR			(BIT12)
#define CFSR_UNSTKERR		(BIT11)
#define CFSR_IMPRECISERR	(BIT10)
#define CFSR_PRECISERR		(BIT9)
#define CFSR_IBUSERR		(BIT8)
#define CFSR_MMFARVALID		(BIT7)
#define CFSR_MLSPERR		(BIT5)
#define CFSR_MSTKERR		(BIT4)
#define CFSR_MUNSTKERR		(BIT3)
#define CFSR_DACCVIOL		(BIT1)
#define CFSR_IACCVIOL		(BIT0)

#define RegHFSR 			(*((volatile uint32_t *)(0xE000ED2C)))
#define HFSR_DEBUGEVT		(BIT31)
#define HFSR_FORCED			(BIT30)
#define HFSR_VECTTBL		(BIT1)

#define RegMMFAR 		(*((volatile uint32_t *)(0xE000ED34)))
#define RegBFAR 		(*((volatile uint32_t *)(0xE000ED38)))

#define SVC(x) asm volatile ("svc %0"::"I"(x))

#define SVC_SAVE		(0x00)
#define SVC_RESTORE		(0x01)

#define SYSCALL_SAVE 		SVC(SVC_SAVE)
#define SYSCALL_RESTORE 	SVC(SVC_RESTORE)

#define SYS_NONVOLATILE_SIGN (0xFEEDBAAC)
#define SYS_CHECK_SIGN(x) ((x.sign == SYS_NONVOLATILE_SIGN) && ((x.n_restore+1) == x.n_save))
#define SYS_SIGN(x) do {x.sign=SYS_NONVOLATILE_SIGN;x.n_save++;} while(0)
#define SYS_CONFIRM_SIGN(x) do {x.sign = 0;x.n_restore++;} while(0)


struct stack_dump {
	volatile uint32_t r13;
	volatile uint32_t r4;
	volatile uint32_t r5;
	volatile uint32_t r6;
	volatile uint32_t r7;
	volatile uint32_t r8;
	volatile uint32_t r9;
	volatile uint32_t r10;
	volatile uint32_t r11;

	volatile uint32_t er0;
	volatile uint32_t er1;
	volatile uint32_t er2;
	volatile uint32_t er3;
	volatile uint32_t er12;
	volatile uint32_t er14;
	volatile uint32_t er15;
	volatile uint32_t epsr;
};


struct sys_nonvolatile {
	uint32_t sp;
	uint32_t sign;
	uint32_t n_save;
	uint32_t n_restore;
};

typedef struct {
	volatile unsigned long stack[8];
	volatile unsigned long CFSR;		// Configurable Fault Status
	volatile unsigned long HFSR;		// Hard Fault Status
	volatile unsigned long DFSR;		// Debug Fault Status
	volatile unsigned long AFSR;		// Auxiliary Fault Status
	volatile unsigned long BFAR;		// Bus Fault Address
	volatile unsigned long MMAR;		// MemManage Fault Address
	volatile unsigned long ICSR;		// Interrupt Control and State
} core_reg_t;

//===================================================================================================================
// Host Interface
//===================================================================================================================
#define HIF_BASE_ADDR (0x40090000)

#define HIF_TX (0)
#define HIF_RX (1)

#define HIF_CSPI_TX 0
#define HIF_CSPI_RX 1

//0x1ACCE551
#define RegHIF_DEVICE_READY    		(*((volatile uint32_t *)(HIF_BASE_ADDR + 0x00)))
#define RegHIF_DEVICE_SLEEP    		(*((volatile uint32_t *)(HIF_BASE_ADDR + 0x04)))

#define RegHIF_DEVICE_MESSAGE0 		(*((volatile uint32_t *)(HIF_BASE_ADDR + 0x10)))
#define RegHIF_DEVICE_MESSAGE1 		(*((volatile uint32_t *)(HIF_BASE_ADDR + 0x14)))
#define RegHIF_DEVICE_MESSAGE2 		(*((volatile uint32_t *)(HIF_BASE_ADDR + 0x18)))
#define RegHIF_DEVICE_MESSAGE3 		(*((volatile uint32_t *)(HIF_BASE_ADDR + 0x1C)))

#define RegHIF_SW_VERION       		(*((volatile uint32_t *)(HIF_BASE_ADDR + 0x38)))
#define RegHIF_BD_VERION       		(*((volatile uint32_t *)(HIF_BASE_ADDR + 0x3C)))

#define HIF_TXQUE_BASE_ADDR (HIF_BASE_ADDR + 0x0100)
#define RegHIF_TXQUE_RST	    		(*((volatile uint32_t *)(HIF_TXQUE_BASE_ADDR + 0x00)))
#define RegHIF_TXQUE_MODE	    		(*((volatile uint32_t *)(HIF_TXQUE_BASE_ADDR + 0x04)))
#define RegHIF_TXQUE_IRQ				(*((volatile uint32_t *)(HIF_TXQUE_BASE_ADDR + 0x08)))
#define RegHIF_TXQUE_ADDRESS	   		(*((volatile uint32_t *)(HIF_TXQUE_BASE_ADDR + 0x10)))
#define RegHIF_TXQUE_SET		   		(*((volatile uint32_t *)(HIF_TXQUE_BASE_ADDR + 0x14)))
#define RegHIF_TXQUE_STATUS		   		(*((volatile uint32_t *)(HIF_TXQUE_BASE_ADDR + 0x18)))


#define HIF_RXQUE_BASE_ADDR (HIF_BASE_ADDR + 0x0200)
#define RegHIF_RXQUE_RST	    		(*((volatile uint32_t *)(HIF_RXQUE_BASE_ADDR + 0x00)))
#define RegHIF_RXQUE_MODE	    		(*((volatile uint32_t *)(HIF_RXQUE_BASE_ADDR + 0x04)))
#define RegHIF_RXQUE_IRQ		   		(*((volatile uint32_t *)(HIF_RXQUE_BASE_ADDR + 0x08)))
#define RegHIF_RXQUE_ADDRESS	   		(*((volatile uint32_t *)(HIF_RXQUE_BASE_ADDR + 0x10)))
#define RegHIF_RXQUE_SET		   		(*((volatile uint32_t *)(HIF_RXQUE_BASE_ADDR + 0x14)))
#define RegHIF_RXQUE_STATUS		   		(*((volatile uint32_t *)(HIF_RXQUE_BASE_ADDR + 0x18)))


typedef union {
	struct {
		uint32_t rst 		: 1;
		uint32_t reserved	: 31;
	} bit;
	uint32_t word;
} hif_que_rst_t;

typedef union {
	struct {
		uint32_t bit_msb 		: 1;
		uint32_t byte_msb 		: 1;
		uint32_t reserved0 		: 2;
		uint32_t header_on		: 1;
		uint32_t reserved1		: 26;
		uint32_t enable			: 1;
	} bit;
	uint32_t word;
} hif_que_mode_t;

typedef union {
	struct {
		uint32_t active_hi		: 1;
		uint32_t edge 			: 1;
		uint32_t reserved		: 29;
		uint32_t enable			: 1;
	} bit;
	uint32_t word;
} hif_que_irq_t;

typedef union {
	struct {
		uint32_t address	: 32;
	} bit;
	uint32_t word;
} hif_que_address_t;

typedef union {
	struct {
		uint32_t size		: 16;
		uint32_t reserved0	: 11;
		uint32_t int_mask	: 1;
		uint32_t reserved1	: 2;
		uint32_t eirq_event	: 1;		//tx only - trigger ext int
		uint32_t update		: 1;
	} bit;
	uint32_t word;
} hif_que_set_t;


typedef union {
	struct {
		uint32_t total		: 16;
		uint32_t count		: 6;
		uint32_t reserved0	: 1;
		uint32_t error		: 1;
		uint32_t reserved1	: 7;
		uint32_t irq		: 1;
	} bit;
	uint32_t word;
} hif_que_status_t;


typedef struct {
	volatile hif_que_rst_t 			rst;
	volatile hif_que_mode_t			mode;
	volatile hif_que_irq_t			irq;
	volatile uint32_t				reserved;
	volatile hif_que_address_t		address;
	volatile hif_que_set_t			set;
	volatile hif_que_status_t		status;
} hif_controller_t;

//===================================================================================================================
// PMC (Power Management Controller)
//===================================================================================================================
#define PMC_BASE_ADDR (SYSTEM_BASE + 0x2000)

#define RegPMC_RESET       		(*((volatile uint32_t *)(PMC_BASE_ADDR + 0x00)))
#define RegPMC_CTRL0       		(*((volatile uint32_t *)(PMC_BASE_ADDR + 0x04)))
#define RegPMC_CTRL1       		(*((volatile uint32_t *)(PMC_BASE_ADDR + 0x08)))
#define RegPMC_CTRL2       		(*((volatile uint32_t *)(PMC_BASE_ADDR + 0x0C)))
#define RegPMC_STATUS0       	(*((volatile uint32_t *)(PMC_BASE_ADDR + 0x10)))
#define RegPMC_STATUS1       	(*((volatile uint32_t *)(PMC_BASE_ADDR + 0x14)))
#define RegPMC_STATUS2       	(*((volatile uint32_t *)(PMC_BASE_ADDR + 0x18)))
#define RegPMC_PWR_SW_MASK     	(*((volatile uint32_t *)(PMC_BASE_ADDR + 0x20)))
#define RegPMC_PWR_MANUAL_EN    (*((volatile uint32_t *)(PMC_BASE_ADDR + 0x24)))
#define RegPMC_PWR_MANUAL     	(*((volatile uint32_t *)(PMC_BASE_ADDR + 0x28)))
#define RegPMC_ISO_MANUAL     	(*((volatile uint32_t *)(PMC_BASE_ADDR + 0x2C)))
#define RegPMC_ETC_CTRL			(*((volatile uint32_t *)(PMC_BASE_ADDR + 0x30)))
#define RegPMC_PWR_ALARM		(*((volatile uint32_t *)(PMC_BASE_ADDR + 0x34)))
#define RegPMC_PWR_ALARM_STATS	(*((volatile uint32_t *)(PMC_BASE_ADDR + 0x38)))

typedef union {
	struct {
		uint32_t rst 		: 1;
		uint32_t reserved	: 31;
	} bit;
	uint32_t word;
} pmc_reset_t;


typedef union {
	struct {
		uint32_t op_code			: 3;
		uint32_t reserved1 			: 5;
		uint32_t wakeup_src_mask	: 4;
		uint32_t extint0			: 1;
		uint32_t extint1			: 1;
		uint32_t reserved2 			: 16;
		uint32_t irqen 				: 1;
		uint32_t pmcen 				: 1;
	} bit;
	uint32_t word;
} pmc_ctrl0_t;


typedef union {
	struct {
		uint32_t x32k_wait 	: 16;
		uint32_t x32m_wait 	: 5;
		uint32_t reserved	: 11;
	} bit;
	uint32_t word;
} pmc_ctrl1_t;


typedef union {
	struct {
		uint32_t pwr_wait 		: 16;
		uint32_t reserved		: 15;
		uint32_t wait_method 	: 1;
	} bit;
	uint32_t word;
} pmc_ctrl2_t;


typedef union {
	struct {
		uint32_t interrupt 		: 8;
		uint32_t reserved		: 24;
	} bit;
	uint32_t word;
} pmc_status0_t;


typedef union {
	struct {
		uint32_t current		: 4;
		uint32_t busy			: 1;
		uint32_t reserved		: 27;
	} bit;
	uint32_t word;
} pmc_status1_t;


typedef union {
	struct {
		uint32_t history		: 24;
		uint32_t reserved		: 8;
	} bit;
	uint32_t word;
} pmc_status2_t;


typedef union {
	struct {
		uint32_t pmu_ctrl		: 1;
		uint32_t ldo_ctrl		: 1;
		uint32_t reserved1		: 6;		//FIXME NEED to check bit field
		uint32_t core			: 1;
		uint32_t modem			: 1;
		uint32_t mem			: 1;
		uint32_t rf				: 1;
		uint32_t reserved2		: 4;
		uint32_t mem_pd			: 1;
		uint32_t reserved3		: 15;
	} bit;
	uint32_t word;
} pmc_pwr_sw_t;

typedef union {
	struct {
		uint32_t reserved1		: 8;		//FIXME NEED to check bit field
		uint32_t core			: 1;
		uint32_t modem			: 1;
		uint32_t reserved2		: 1;
		uint32_t rf				: 1;
		uint32_t mem0			: 1;
		uint32_t mem1			: 1;
		uint32_t mem2			: 1;
		uint32_t mem3			: 1;
		uint32_t reserved3		: 16;
	} bit;
	uint32_t word;
} pmc_iso_t;

typedef union {
	struct {
		uint32_t xtal32_off		: 1;
		uint32_t lpo16m_off		: 1;
		uint32_t reserved		: 30;
	} bit;
	uint32_t word;
} pmc_etc_ctrl_t;


typedef union {
	struct {
		uint32_t alarm_cnt		: 16;
		uint32_t reserved		: 13;
		uint32_t alarm_cnt_init	: 1;
		uint32_t irq_en			: 1;
		uint32_t alarm_en		: 1;
	} bit;
	uint32_t word;
} pmc_pwr_alarm_t;


typedef union {
	struct {
		uint32_t alarm_cnt		: 16;
		uint32_t reserved		: 16;
	} bit;
	uint32_t word;
} pmc_pwr_alarm_status_t;


typedef struct {
	volatile pmc_reset_t 				rst;
	volatile pmc_ctrl0_t 				ctrl0;
	volatile pmc_ctrl1_t 				ctrl1;
	volatile pmc_ctrl2_t 				ctrl2;
	volatile pmc_status0_t 				status0;
	volatile pmc_status1_t 				status1;
	volatile pmc_status2_t 				status2;
	volatile uint32_t 					reserved1;
	volatile pmc_pwr_sw_t				pwr_sw_mask;
	volatile pmc_pwr_sw_t				pwr_sw_manual_en;
	volatile pmc_pwr_sw_t				pwr_sw_manual;
	volatile pmc_iso_t					iso_manual;
	volatile pmc_etc_ctrl_t				etc_ctrl;
	volatile pmc_pwr_alarm_t			pwr_alarm;
	volatile pmc_pwr_alarm_status_t		pwr_alarm_status;
} pmc_controller_t;

//===================================================================================================================
// RTC (Real Time Clock)
//===================================================================================================================
#define RTC_BASE_ADDR  (SYSTEM_BASE + 0x0000)
#define RTC_CAL_BASE_ADDR (SYSTEM_BASE + 0x4A000)

#define RegRTC_CTRL   						(*((volatile uint32_t *)(RTC_BASE_ADDR + 0x00)))
#define RegRTC_SET_CNT_H   					(*((volatile uint32_t *)(RTC_BASE_ADDR + 0x04)))
#define RegRTC_SET_CNT_L   					(*((volatile uint32_t *)(RTC_BASE_ADDR + 0x08)))
#define RegRTC_CURR_CNT_H   				(*((volatile uint32_t *)(RTC_BASE_ADDR + 0x0C)))
#define RegRTC_CURR_CNT_L   				(*((volatile uint32_t *)(RTC_BASE_ADDR + 0x10)))
#define RegRTC_OFFSET_COMPENSATION   		(*((volatile uint32_t *)(RTC_BASE_ADDR + 0x14)))
#define RegRTC_LOAD_CNT_H   				(*((volatile uint32_t *)(RTC_BASE_ADDR + 0x18)))
#define RegRTC_LOAD_CNT_L   				(*((volatile uint32_t *)(RTC_BASE_ADDR + 0x1C)))
#define RegRTC_INT_STATUS 	 				(*((volatile uint32_t *)(RTC_BASE_ADDR + 0x30)))

#define RegRTC_FREQ_OFFSET_ESTIMATOR_CTRL	(*((volatile uint32_t *)(RTC_CAL_BASE_ADDR + 0x00)))
#define RegRTC_TRACKING_INCREMENT			(*((volatile uint32_t *)(RTC_CAL_BASE_ADDR + 0x04)))
#define RegRTC_FREQ_OFFSET_IN_PPM			(*((volatile uint32_t *)(RTC_CAL_BASE_ADDR + 0x08)))
#define RegRTC_FREQ_OFFSET_ONBD				(*((volatile uint32_t *)(RTC_CAL_BASE_ADDR + 0x0C)))
#define RegRTC_MEASURED_REFCLK_CNT			(*((volatile uint32_t *)(RTC_CAL_BASE_ADDR + 0x10)))

//===================================================================================================================
// LPO CAL
//===================================================================================================================
#define LPO_BASE_ADDR  (SYSTEM_BASE + 0x00A0)

#define RegLPO_CTRL   						(*((volatile uint32_t *)(LPO_BASE_ADDR + 0x00)))
#define RegLPO_CMP   						(*((volatile uint32_t *)(LPO_BASE_ADDR + 0x04)))
#define RegLPO_CTRL1   						(*((volatile uint32_t *)(LPO_BASE_ADDR + 0x08)))

//===================================================================================================================
// SFC (Serial Flash Controller) Defines
//===================================================================================================================
#define SFC_BASE_ADDR (0x40092000)
#define SFC_MEM_BASE_ADDR (0x11000000)

#define RegSFC_FLMODE       (*((volatile uint32_t *)(SFC_BASE_ADDR + 0x00)))
#define RegSFC_FLBRT        (*((volatile uint32_t *)(SFC_BASE_ADDR + 0x04)))
#define RegSFC_FLCSH        (*((volatile uint32_t *)(SFC_BASE_ADDR + 0x08)))
#define RegSFC_FLPEM        (*((volatile uint32_t *)(SFC_BASE_ADDR + 0x0C)))

#define RegSFC_FLCMD        (*((volatile uint32_t *)(SFC_BASE_ADDR + 0x10)))
#define RegSFC_FLSTS        (*((volatile uint32_t *)(SFC_BASE_ADDR + 0x14)))
#define RegSFC_FLSEA        (*((volatile uint32_t *)(SFC_BASE_ADDR + 0x18)))
#define RegSFC_FLBEA        (*((volatile uint32_t *)(SFC_BASE_ADDR + 0x1C)))

#define RegSFC_FLDAT        (*((volatile uint32_t *)(SFC_BASE_ADDR + 0x20)))
#define RegSFC_FLDAT_SHORT  (*((volatile uint16_t *)(SFC_BASE_ADDR + 0x20)))
#define RegSFC_FLWCP        (*((volatile uint32_t *)(SFC_BASE_ADDR + 0x24)))
#define RegSFC_FLCKDLY      (*((volatile uint32_t *)(SFC_BASE_ADDR + 0x28)))
#define RegSFC_FLSTS2       (*((volatile uint32_t *)(SFC_BASE_ADDR + 0x2C)))
#define RegSFC_CCTRL        (*((volatile uint32_t *)(SFC_BASE_ADDR + 0x30)))
#define RegSFC_WRCMD        (*((volatile uint32_t *)(SFC_BASE_ADDR + 0x38)))

#define READMODE_SINGLE		(0)
#define READMODE_DUAL		(1)
#define READMODE_QUAD		(2)

#define CACHE_INVALID		(0x4)
#define CACHE_EN		(0x1)
#define CACHE_IDLE		(0x0)

//===================================================================================================================
// System Configuration
//===================================================================================================================
#define SCFG_BASE_ADDR (0x4000F000)
#define RegSCFG_MODE				(*((volatile uint32_t *)(SCFG_BASE_ADDR + 0x00)))
#define RegSCFG_REMAP				(*((volatile uint32_t *)(SCFG_BASE_ADDR + 0x04)))
#define RegSCFG_MEM_RWAIT			(*((volatile uint32_t *)(SCFG_BASE_ADDR + 0x08)))
#define RegSCFG_BUS_INC_EN			(*((volatile uint32_t *)(SCFG_BASE_ADDR + 0x0C)))
#define RegSCFG_SEL_RFSPI			(*((volatile uint32_t *)(SCFG_BASE_ADDR + 0x10)))
#define RegSCFG_LPO32K_CAL			(*((volatile uint32_t *)(SCFG_BASE_ADDR + 0x14)))
#define RegSCFG_BR					(*((volatile uint32_t *)(SCFG_BASE_ADDR + 0x18)))
#define RegSCFG_XIP_DEEPSLEEP		(*((volatile uint32_t *)(SCFG_BASE_ADDR + 0x1C)))

#define REMAP_XIP		(0)
#if defined(CPU_CM3)
#define REMAP_ROM		(2)
#else
#define REMAP_ROM		(1)
#endif
#define REMAP_SRAM_0	(3)
#define REMAP_SRAM_2	(4)

//===================================================================================================================
// RF SPI  Defines
//===================================================================================================================
#define NRF_SPI_SEL_ADDR	(0x4000f010)
#define NRF_SPI_BASE_ADDR	(0x4004f000)
#define NRF_SPI_ADDR_MIN	((NRF_SPI_BASE_ADDR + 0x0000))
#define NRF_SPI_ADDR_MAX	((NRF_SPI_BASE_ADDR + 0x016C))

void system_default_setting(int vif_id);
void system_print_logo();
char *system_prompt_func();
void system_get_core_reg(core_reg_t *core_reg);

#endif
