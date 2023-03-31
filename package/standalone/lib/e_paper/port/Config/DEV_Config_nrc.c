
#include "system.h"
#include "system_common.h"
#include "hal_gpio.h"
#include "hal_ssp.h"
#include "DEV_Config.h"


/***************************************************************************************/

static void DEV_UART_Pins_Disable (void)
{
	gpio_io_t gpio;
	uio_sel_t sel;

	nrc_gpio_get_alt(&gpio);
	gpio.word &= ~((0x3 << 8) | 0x3f);
	nrc_gpio_set_alt(&gpio);

	memset(&sel, 0xff, sizeof(uio_sel_t));
	nrc_gpio_set_uio_sel(UIO_SEL_UART0, &sel);
#if defined(NRC7292)
	nrc_gpio_set_uio_sel(UIO_SEL_UART1, &sel);
	nrc_gpio_set_uio_sel(UIO_SEL_UART2, &sel);
#endif
}

static void DEV_GPIO_Config (int pin, bool output, int level)
{
	gpio_io_t gpio;

	nrc_gpio_get_alt(&gpio);
	gpio.word &= ~(1 << pin);
	nrc_gpio_set_alt(&gpio);

	nrc_gpio_get_dir(&gpio);
	gpio.word = (gpio.word & ~(1 << pin)) | (output ? (1 << pin) : 0);
	nrc_gpio_config_dir(&gpio);

	nrc_gpio_get_pullup(&gpio);
	gpio.word |= (1 << pin);
	nrc_gpio_config_pullup(&gpio);

	nrc_gpio_get_pulldown(&gpio);
	gpio.word &= ~(1 << pin);
	nrc_gpio_config_pulldown(&gpio);

	if (output && (level == 0 || level == 1))
		nrc_gpio_outb(pin, level);
}

static void DEV_SPI_Enable (int clk_pin, int mosi_pin, uint32_t clk_freq)
{
	gpio_io_t gpio;
	uio_sel_t sel;

	nrc_gpio_get_alt(&gpio);
	gpio.word |= (1 << clk_pin) | (1 << mosi_pin);
	nrc_gpio_set_alt(&gpio);

	nrc_gpio_get_uio_sel(UIO_SEL_SPI0, &sel);
	sel.bit.sel7_0 = clk_pin;
	sel.bit.sel15_8 = 0xff;
	sel.bit.sel23_16 = mosi_pin;
	sel.bit.sel31_24 = 0xff;
	nrc_gpio_set_uio_sel(UIO_SEL_SPI0, &sel);

	nrc_ssp_init(0, CPOL_LO, CPHA_LO, SPI_MSB, (8 - 1), clk_freq);
	nrc_ssp_lb(0, false);
	nrc_ssp(0, true);
	nrc_ssp_flush(0);
}

static void DEV_SPI_Disable (void)
{
	nrc_ssp(0, true);
	nrc_ssp_flush(0);
}

/***************************************************************************************/

static int EPD_POWER_PIN = -1;

int EPD_BUSY_PIN = -1;
int EPD_RST_PIN = -1;
int EPD_DC_PIN = -1;
int EPD_CS_PIN = -1;

int DEV_Module_Init (struct EPD_IO_PINS *Pins)
{
	if (!Pins)
		return -1;

	EPD_POWER_PIN = Pins->PWR;
	EPD_BUSY_PIN = Pins->BUSY;
	EPD_RST_PIN = Pins->RST;
	EPD_DC_PIN = Pins->DC;
	EPD_CS_PIN = Pins->CS;

	DEV_UART_Pins_Disable();

	DEV_GPIO_Config(Pins->BUSY, false, -1);
	DEV_GPIO_Config(Pins->RST, true, -1);
	DEV_GPIO_Config(Pins->DC, true, -1);
	DEV_GPIO_Config(Pins->CS, true, 1);
	DEV_GPIO_Config(Pins->PWR, true, 1);

	DEV_SPI_Enable(Pins->SCL, Pins->SDA, 16 * 1000 * 1000);

	DEV_Delay_ms(500);

	return 0;
}

void DEV_Module_Exit (void)
{
	DEV_SPI_Disable();

	DEV_Digital_Write(EPD_POWER_PIN, 0);
	DEV_Digital_Write(EPD_CS_PIN, 1);
	DEV_Digital_Write(EPD_DC_PIN, 0);
	DEV_Digital_Write(EPD_RST_PIN, 1);

#if 0
	EPD_POWER_PIN = -1;
	EPD_BUSY_PIN = -1;
	EPD_RST_PIN = -1;
	EPD_DC_PIN = -1;
	EPD_CS_PIN = -1;
#endif
}

void DEV_Digital_Write (UWORD Pin, UBYTE Value)
{
	nrc_gpio_outb(Pin, Value);
}

UBYTE DEV_Digital_Read (UWORD Pin)
{
	return nrc_gpio_inb(Pin);
}

void DEV_SPI_WriteByte (UBYTE Value)
{
	DEV_SPI_Write_nByte(&Value, 1);
}

void DEV_SPI_Write_nByte (UBYTE *pData, UDOUBLE Len)
{
	if (pData && Len > 0)
	{
		unsigned long flags = system_irq_save();
		uint32_t ret;

		ret = nrc_ssp_xfer(0, pData, NULL, Len);
	   	if (ret	!= Len)
		{
			A("%s: failed, ret=%d/%u\n", __func__, ret, Len);
		}

		system_irq_restore(flags);
	}
}

void DEV_Delay_ms (UDOUBLE xms)
{
	vTaskDelay(pdMS_TO_TICKS(xms));
}

