#ifndef __HAL_CLOCK_NRC7292_H__
#define __HAL_CLOCK_NRC7292_H__

#include "system.h"

typedef enum {
    PCLK_TIMER0,
    PCLK_TIMER1,
    PCLK_TIMER2,
    PCLK_TIMER3,
    PCLK_TIMER4,
    PCLK_TIMER5,
    PCLK_RESERVED,
    PCLK_RESERVED1,
    PCLK_PWM0,
    PCLK_PWM1,
    PCLK_RESERVED2,
    PCLK_RESERVED3,
    PCLK_SPI0,
    PCLK_SPI1,
    PCLK_SPI2,
    PCLK_SPI3,
    PCLK_UART0,
    PCLK_UART1,
    PCLK_UART2,
    PCLK_UART3,
    PCLK_I2C,
    PCLK_WDOG,
    PCLK_RTC,
    PCLK_CAL,
    PCLK_MAX
} pclk_e;

typedef union {
	struct {
		uint32_t hclk_sel: 		4;
		uint32_t sel_lpo32k: 	1;
		uint32_t reserved: 		24;
		uint32_t xtal32k_off: 	1;
		uint32_t lpo16m_off: 	1;
		uint32_t auxpll_en: 	1;
	} bit;
	uint32_t word;
} clk_ctl_t;

typedef union {
	struct {
		uint32_t xtin_div: 6;
		uint32_t reserved1: 1;
		uint32_t xtin_diven: 1;
		uint32_t xin_div: 6;
		uint32_t reserved2: 1;
		uint32_t xin_diven: 1;
		uint32_t pll_div: 6;
		uint32_t reserved3: 1;
		uint32_t pll_diven: 1;
		uint32_t lpo16m_div: 6;
		uint32_t reserved4: 1;
		uint32_t lpo16m_diven: 1;
	} bit;
	uint32_t word;
} clk_div_t;

typedef union {
	struct {
		uint32_t sw_mac_en0: 1;
		uint32_t sw_mac_en1: 1;
		uint32_t sw_mac_en2: 1;
		uint32_t reserved1: 5;
		uint32_t sw_phy_en0: 1;
		uint32_t sw_phy_en1: 1;
		uint32_t sw_phy_en2: 1;
		uint32_t sw_phy_en3: 1;
		uint32_t sw_phy_en4: 1;
		uint32_t reserved2: 3;
		uint32_t sw_adda_en0: 1;
		uint32_t sw_adda_en1: 1;
		uint32_t sw_adda_en2: 1;
		uint32_t sw_adda_en3: 1;
		uint32_t sw_adda_en4: 1;
		uint32_t reserved3: 10;
		uint32_t wifi_en: 1;
	} bit;
	uint32_t word;
} clk_wifi_t;

typedef union {
	struct {
		uint32_t por: 1;
		uint32_t wdog: 1;
		uint32_t pmc: 1;
		uint32_t hostinf: 1;
		uint32_t sys_rst_req_cm0: 1;
		uint32_t sys_rst_req_cm3: 1;
		uint32_t reserved: 26;
	} bit;
	uint32_t word;
} clk_boot_reason_t;

typedef union {
	struct {
		uint32_t div: 12;
		uint32_t reserved: 12;
		uint32_t sel: 4;
		uint32_t reserved1: 3;
		uint32_t en: 1;
	} bit;
	uint32_t word;
} pclk_config_t;

typedef struct {
	volatile clk_ctl_t clkctrl;
	volatile clk_div_t clkdivc;
	volatile clk_wifi_t clkwifi;
	volatile clk_boot_reason_t br;
	volatile uint32_t sw_reset;
	volatile uint32_t reserved2;
	volatile uint32_t hclk_mask;
	volatile uint32_t reserved3;
	volatile pclk_config_t pclk_cfg[PCLK_MAX];
} clock_controller_t;


void hal_nrc_clock_init(void);
void nrc_clock_wait_stable(void);

uint32_t nrc_clock_get_mclk(uint8_t);
uint32_t nrc_clock_get_rclk(void);
uint32_t nrc_clock_get_apb(void);
//uint32_t nrc_clock_get_pclk(pclk_e dev);
uint32_t nrc_clock_get_pclk(int);

void nrc_clock_reset(uint32_t);

uint32_t nrc_clock_get_mask_all();
void nrc_clock_set_mask_all(uint32_t);
bool nrc_clock_get_mask(int id);
void nrc_clock_set_mask(int id, bool enable);

//void nrc_clock_pclk_enable(pclk_e dev, bool);
uint8_t nrc_clock_get_hclk_mux();
void nrc_clock_hclk_mux(uint8_t); //HCLK_SEL_XXX
void nrc_clock_hclk_lpo16m(bool enable);
void nrc_clock_hclk_auxpll(bool enable);
void nrc_clock_get_rclk_mux(uint8_t); //RCLK_SEL_XXX
uint8_t nrc_clock_rclk_mux();
//void nrc_clock_pclk_mux(pclk_e dev, uint8_t); //HCLK_SEL_XXX
void nrc_clock_hclk_div(uint8_t);
void nrc_clock_wifi_en(uint32_t, bool);
void nrc_clock_pclk_div(pclk_e dev, uint8_t);
void nrc_clock_pclk_sel(pclk_e dev, uint8_t sel);
uint32_t nrc_clock_boot_reason();
void nrc_clock_boot_reason_clear();
uint8_t nrc_clock_get_pclk_div(pclk_e dev);
uint8_t nrc_clock_get_pclk_sel(pclk_e dev);

#endif //__HAL_CLOCK_NRC7292_H__
