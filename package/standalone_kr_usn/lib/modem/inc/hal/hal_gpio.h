#ifndef __HAL_GPIO_NRC7292_H__
#define __HAL_GPIO_NRC7292_H__

#include "system.h"

#define GPIO_27 27
#define GPIO_28 28
#define GPIO_29 29

// GPIO[27:29]
#define NRC7292_A_VALUE 7 /* 0b111 */
#define NRC7292_B_VALUE 0 /* 0b000 */
#define NRC7292_C_VALUE 4 /* 0b100 */

typedef enum {
    NRC7292_BOARD_REV_INVALID = -1,
    NRC7292_A = 0,
    NRC7292_B,
    NRC7292_C,
    NRC7292_BOARD_REV_MAX
} board_revision;

typedef union {
	struct {
		uint32_t io0 : 1;
		uint32_t io1 : 1;
		uint32_t io2 : 1;
		uint32_t io3 : 1;
		uint32_t io4 : 1;
		uint32_t io5 : 1;
		uint32_t io6 : 1;
		uint32_t io7 : 1;
		uint32_t io8 : 1;
		uint32_t io9 : 1;
		uint32_t io10 : 1;
		uint32_t io11 : 1;
		uint32_t io12 : 1;
		uint32_t io13 : 1;
		uint32_t io14 : 1;
		uint32_t io15 : 1;
		uint32_t io16 : 1;
		uint32_t io17 : 1;
		uint32_t io18 : 1;
		uint32_t io19 : 1;
		uint32_t io20 : 1;
		uint32_t io21 : 1;
		uint32_t io22 : 1;
		uint32_t io23 : 1;
		uint32_t io24 : 1;
		uint32_t io25 : 1;
		uint32_t io26 : 1;
		uint32_t io27 : 1;
		uint32_t io28 : 1;
		uint32_t io29 : 1;
		uint32_t io30 : 1;
		uint32_t io31 : 1;
	} bit;
	uint32_t word;
} gpio_io_t;

typedef struct {
	volatile gpio_io_t data;			//00
	volatile gpio_io_t dir;				//04
	volatile gpio_io_t alt;				//08
	volatile gpio_io_t alt1;			//0c
	volatile uint32_t reserved1;		//10
	volatile uint32_t reserved2;		//14
	volatile uint32_t reserved3;		//18
	volatile uint32_t reserved4;		//1c
	volatile uint32_t reserved5;		//20
	volatile uint32_t reserved6;		//24
	volatile uint32_t reserved7;		//28
	volatile gpio_io_t input_enable;	//2c /* This register is not used. */
	volatile gpio_io_t pullup;			//30
	volatile gpio_io_t pulldown;		//34
	volatile gpio_io_t drv_cur_ctrl0;	//38
	volatile gpio_io_t drv_cur_ctrl1;	//3c
	volatile gpio_io_t prf_data;		//40
	volatile gpio_io_t prf_dir;			//44
	volatile gpio_io_t prf_alt0;		//48
	volatile gpio_io_t prf_alt1;		//5c
	volatile gpio_io_t prf_alt2;		//50
	volatile uint32_t reserved8;		//54
	volatile uint32_t reserved9;		//58
	volatile uint32_t reserved10;		//5c
	volatile uint32_t reserved11;		//60
	volatile uint32_t reserved12;		//64
	volatile uint32_t reserved13;		//68
	volatile gpio_io_t prf_inen;		//6c
	volatile gpio_io_t prf_pullup;		//70
	volatile gpio_io_t prf_pulldown;	//74
	volatile gpio_io_t prf_drv0;		//78
	volatile gpio_io_t prf_drv1;		//7c
} gpio_controller;

#define IOMUX_BASE_ADDR   (SYSTEM_BASE + 0x5800)

typedef enum {
    UIO_SEL_I2C_M1_M0, //I2C Master 1, 0
    UIO_SEL_I2C_M3_M2,
    UIO_SEL_I2C_S1_S0,
    UIO_SEL_RESERVERD,
    UIO_SEL_SPI0,
    UIO_SEL_SPI1,
    UIO_SEL_SPI2,
    UIO_SEL_SPI3,
    UIO_SEL_UART0,
    UIO_SEL_UART1,
    UIO_SEL_UART2,
    UIO_SEL_UART3,
    UIO_SEL_PWM3_PWM0,
    UIO_SEL_PWM7_PWM4,
    UIO_SEL_EINT3_EINT0,
    UIO_SEL_MAX
} UIO_SEL_e;

typedef union {
	struct {
		uint32_t sel7_0: 8;
		uint32_t sel15_8: 8;
		uint32_t sel23_16: 8;
		uint32_t sel31_24: 8;
	} bit;
	uint32_t word;
} uio_sel_t;

typedef struct {
	volatile uio_sel_t uio_sel[UIO_SEL_MAX];
} iomux_controller;


void nrc_gpio_init(void);
void nrc_gpio_deinit(void);
void nrc_gpio_config_dir(gpio_io_t *gpio);
void nrc_gpio_get_dir(gpio_io_t *gpio);
void nrc_gpio_get_int_enable(gpio_io_t *gpio); /* This function will be deprecated since the register is not used. */
void nrc_gpio_set_int_enable(gpio_io_t *gpio); /* This function will be deprecated since the register is not used. */
void nrc_gpio_out(gpio_io_t *gpio);
void nrc_gpio_outb(int pin, int level);
void nrc_gpio_in(gpio_io_t *gpio);
int nrc_gpio_inb(int pin);
void nrc_gpio_config_pullup(gpio_io_t *gpio);
void nrc_gpio_get_pullup(gpio_io_t *gpio);
void nrc_gpio_config_pulldown(gpio_io_t *gpio);
void nrc_gpio_get_pulldown(gpio_io_t *gpio);
void nrc_gpio_set_drv_current(gpio_io_t *ctl0, gpio_io_t *ctl1);
void nrc_gpio_set_alt(gpio_io_t *gpio);
void nrc_gpio_get_alt(gpio_io_t *gpio);
void nrc_gpio_set_alt1(gpio_io_t *gpio);
void nrc_gpio_get_alt1(gpio_io_t *gpio);
//void nrc_gpio_get_uio_sel(UIO_SEL_e uio, uio_sel_t *sel);
void nrc_gpio_get_uio_sel(int uio, uio_sel_t *sel);
//void nrc_gpio_set_uio_sel(UIO_SEL_e uio, uio_sel_t *sel);
void nrc_gpio_set_uio_sel(int uio, uio_sel_t *sel);
void nrc_gpio_test();
void nrc_gpio_set_deepsleep();
void nrc_gpio_set_modemsleep();
int nrc_gpio_get_brd_rev(void);
int nrc_get_brd_rev(void);

void nrc_gprf_config_dir(gpio_io_t *gprf);
void nrc_gprf_get_dir(gpio_io_t *gprf);
void nrc_gprf_out(gpio_io_t *gprf);
void nrc_gprf_in(gpio_io_t *gprf);
void nrc_gprf_config_pullup(gpio_io_t *gprf);
void nrc_gprf_get_pullup(gpio_io_t *gprf);
void nrc_gprf_config_pulldown(gpio_io_t *gprf);
void nrc_gprf_config_alt0(gpio_io_t *gprf);
void nrc_gprf_config_alt1(gpio_io_t *gprf);
void nrc_gprf_config_alt2(gpio_io_t *gprf);
void nrc_gprf_config_drv0(gpio_io_t *gprf);
void nrc_gprf_config_drv1(gpio_io_t *gprf);

#endif //__HAL_GPIO_NRC7292_H__
