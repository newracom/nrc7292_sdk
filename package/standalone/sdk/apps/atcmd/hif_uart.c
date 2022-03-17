/*
 * MIT License
 *
 * Copyright (c) 2020 Newracom, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */


#include "hif.h"
#include "hal_uart.h"

/*********************************************************************************************/

/*
 *	FIFO depth -> TX: 16x8, RX: 16x12
 * 	FIFO level -> 0:1/8, 1:1/4, 2:1/2, 3:3/4, 4:7/8
 */
#define _HIF_UART_RX_HW_FIFO_LEVEL		2
#define _HIF_UART_TX_HW_FIFO_LEVEL		0

/*********************************************************************************************/

#define _HIF_UART_REG_BASE				g_hif_uart_reg_base[g_hif_uart.channel]

#define _hif_uart_rx_dma_enable()		RegHSUART_DMACR(_HIF_UART_REG_BASE) |= BIT0
#define _hif_uart_rx_dma_disable()		RegHSUART_DMACR(_HIF_UART_REG_BASE) &= ~BIT0

#define _hif_uart_tx_dma_enable()		RegHSUART_DMACR(_HIF_UART_REG_BASE) |= BIT1
#define _hif_uart_tx_dma_disable()		RegHSUART_DMACR(_HIF_UART_REG_BASE) &= ~BIT1

#define _hif_uart_rx_int_enable()		RegHSUART_IMSC(_HIF_UART_REG_BASE) |= (IMSC_RX|IMSC_RT)
#define _hif_uart_rx_int_disable()		RegHSUART_IMSC(_HIF_UART_REG_BASE) &= ~(IMSC_RX|IMSC_RT)

#define _hif_uart_tx_int_enable()		RegHSUART_IMSC(_HIF_UART_REG_BASE) |= IMSC_TX
#define _hif_uart_tx_int_disable()		RegHSUART_IMSC(_HIF_UART_REG_BASE) &= ~IMSC_TX

#define _hif_uart_int_get_status()		RegHSUART_MIS(_HIF_UART_REG_BASE)
#define _hif_uart_int_set_status(mask)	RegHSUART_MIS(_HIF_UART_REG_BASE) |= mask

#define _hif_uart_int_clear(mask)		RegHSUART_ICR(_HIF_UART_REG_BASE) |= mask

#define _hif_uart_rx_data()				(RegHSUART_DR(_HIF_UART_REG_BASE) & 0xff)
#define _hif_uart_rx_full()				(RegHSUART_FR(_HIF_UART_REG_BASE) & FR_RXFF)
#define _hif_uart_rx_empty()			(RegHSUART_FR(_HIF_UART_REG_BASE) & FR_RXFE)

#define _hif_uart_tx_data(data)			RegHSUART_DR(_HIF_UART_REG_BASE) = data
#define _hif_uart_tx_full()				(RegHSUART_FR(_HIF_UART_REG_BASE) & FR_TXFF)
#define _hif_uart_tx_empty()			(RegHSUART_FR(_HIF_UART_REG_BASE) & FR_TXFE)

/*********************************************************************************************/

const uint32_t g_hif_uart_reg_base[4] =
{
	HSUART0_BASE_ADDR, HSUART1_BASE_ADDR, HSUART2_BASE_ADDR, HSUART3_BASE_ADDR
};

/*********************************************************************************************/

static _hif_uart_t g_hif_uart =
{
	.channel = -1,
	.baudrate = -1,
	.hfc = false
};

static void _hif_uart_init_info (_hif_uart_t *info)
{
	info->channel = -1;
	info->baudrate = -1;
	info->data_bits = UART_DB8;
	info->stop_bits = UART_SB1;
	info->parity = UART_PB_NONE;
	info->hfc = UART_HFC_DISABLE;
}

static void _hif_uart_set_info (_hif_uart_t *info)
{
	if (info)
		memcpy(&g_hif_uart, info, sizeof(_hif_uart_t));
}

void _hif_uart_get_info (_hif_uart_t *info)
{
	if (info)
		memcpy(info, &g_hif_uart, sizeof(_hif_uart_t));
}

/*********************************************************************************************/

static _hif_fifo_t *g_hif_uart_rx_fifo = NULL;
static _hif_fifo_t *g_hif_uart_tx_fifo = NULL;

static void _hif_uart_fifo_delete (void)
{
	if (g_hif_uart_rx_fifo)
	{
		_hif_fifo_delete(g_hif_uart_rx_fifo);
		g_hif_uart_rx_fifo = NULL;
	}

	if (g_hif_uart_tx_fifo)
	{
		_hif_fifo_delete(g_hif_uart_tx_fifo);
		g_hif_uart_tx_fifo = NULL;
	}
}

static int _hif_uart_fifo_create (_hif_info_t *info)
{
	_hif_buf_t *rx_fifo = &info->rx_fifo;
	_hif_buf_t *tx_fifo = &info->tx_fifo;

	if (rx_fifo->size > 0)
	{
		g_hif_uart_rx_fifo = _hif_fifo_create(rx_fifo->addr, rx_fifo->size, true);
		if (!g_hif_uart_rx_fifo)
		{
			_hif_error("_hif_fifo_create() failed, rx_addr=%p rx_size=%d\n",
			   			rx_fifo->addr, rx_fifo->size);

			return -1;
		}
	}

	if (tx_fifo->size > 0)
	{
		g_hif_uart_tx_fifo = _hif_fifo_create(tx_fifo->addr, tx_fifo->size, false);
		if (!g_hif_uart_tx_fifo)
		{
			_hif_error("_hif_fifo_create() failed, tx_addr=%p tx_size=%d\n",
			   			tx_fifo->addr, tx_fifo->size);
	
			_hif_uart_fifo_delete();
			return -1;
		}
	}

/*	_hif_debug("UART FIFO: rx=%p, tx=%p\n", g_hif_uart_rx_fifo, g_hif_uart_tx_fifo); */

	_hif_info("UART FIFO: rx=%u, tx=%u\n",
				g_hif_uart_rx_fifo ? g_hif_uart_rx_fifo->size : 0,
				g_hif_uart_tx_fifo ? g_hif_uart_tx_fifo->size : 0);

	return 0;
}

/*********************************************************************************************/

#define _uart_dma_read_info(fmt, ...)		//_hif_info("uart_dma_read: " fmt, ##__VA_ARGS__)
#define _uart_dma_write_info(fmt, ...)		//_hif_info("uart_dma_write: " fmt, ##__VA_ARGS__)

typedef struct
{
	int channel;

	uint32_t mem_addr;
	uint32_t mem_size;

	uint32_t slot_addr;
	uint32_t slot_size;

	uint32_t n_desc;
	dma_desc_t desc[0];
} _hif_uart_dma_t;

static _hif_uart_dma_t *g_hif_uart_rx_dma = NULL;

static void _hif_uart_rx_dma_inttc_isr (int channel)
{
	_hif_rx_resume_isr();
}

static void _hif_uart_rx_dma_interr_isr (int channel)
{
	_hif_info("_hif_uart_rx_dma_interr\n");
}

static int _hif_uart_rx_dma_update_fifo (void)
{
	_hif_uart_dma_t *uart_dma = g_hif_uart_rx_dma;
	uint32_t slot_next_addr = _hif_dma_dest_addr(uart_dma->channel);
	uint32_t slot_done_size;
	uint32_t fifo_size;

	if (slot_next_addr == uart_dma->slot_addr)
	{
		_uart_dma_read_info("no transfer\n");
		return 0;
	}

	if (slot_next_addr >= uart_dma->slot_addr)
		slot_done_size = slot_next_addr - uart_dma->slot_addr;
	else
	{
		slot_done_size = (uart_dma->mem_addr + uart_dma->mem_size) - uart_dma->slot_addr;
		slot_done_size += slot_next_addr - uart_dma->mem_addr;
	}

	slot_next_addr = uart_dma->slot_addr + slot_done_size;
	if ((slot_next_addr - uart_dma->mem_addr) >= uart_dma->mem_size)
		slot_next_addr -= uart_dma->mem_size;

	if (slot_done_size == 0)
		return 0;

	fifo_size = _hif_fifo_free_size(g_hif_uart_rx_fifo);

	_uart_dma_read_info("0x%08X -> 0x%08X, %u/%u\n",
			uart_dma->slot_addr, slot_next_addr, slot_done_size, fifo_size);

	if (fifo_size < slot_done_size)
	{
		_hif_error("fifo overflow\n");

		slot_done_size = fifo_size;
	}

	_hif_fifo_write(g_hif_uart_rx_fifo, NULL, slot_done_size);

	uart_dma->slot_addr = slot_next_addr;

	return slot_done_size;
}

static int _hif_uart_rx_dma_register (int uart_channel, uint32_t mem_addr, uint32_t mem_size)
{
	const uint32_t slot_size = CONFIG_HIF_UART_SLOT_SIZE;
	const int hsuart_peri_id[] =
	{
		HIF_DMA_PERI_HSUART0_RX,
		HIF_DMA_PERI_HSUART1_RX,
		HIF_DMA_PERI_HSUART2_RX,
		HIF_DMA_PERI_HSUART3_RX,
	};
	_hif_uart_dma_t *uart_dma;
	dma_peri_t peri;
	dma_desc_t *desc;
	int n_desc;
	int channel;
	int err;
	int i;

	_hif_info("UART_RX_DMA: mem_addr=%p mem_size=%u slot_size=%u\n",
							mem_addr, mem_size, slot_size);

	n_desc = mem_size / slot_size;

	uart_dma = _hif_malloc(sizeof(_hif_uart_dma_t) + (sizeof(dma_desc_t) * n_desc));
   	if (!uart_dma)
	{
		_hif_error("_hif_malloc() fail\n");
		goto uart_rx_dma_register_fail;
	}

	memset(uart_dma, 0, sizeof(_hif_uart_dma_t));
	memset(uart_dma->desc, 0, sizeof(dma_desc_t) * n_desc);

	if (_hif_dma_peri_init(&peri, hsuart_peri_id[uart_channel],
				g_hif_uart_reg_base[uart_channel], false, false) != 0)
	{
		_hif_error("_hif_dma_peri_init() fail\n");
		goto uart_rx_dma_register_fail;
	}

	for (desc = uart_dma->desc, i = 0 ; i < n_desc ; i++)
	{
		_hif_dma_desc_init(desc + i, peri.Addr, mem_addr + (i * slot_size), slot_size);
		_hif_dma_desc_set_addr_inc(desc + i, false, true);
		_hif_dma_desc_set_width(desc + i, HIF_DMA_WIDTH_8, HIF_DMA_WIDTH_8);
		_hif_dma_desc_set_bsize(desc + i, HIF_DMA_BSIZE_1, HIF_DMA_BSIZE_128);
		_hif_dma_desc_set_inttc(desc + i, true);
		_hif_dma_desc_set_ahb_master(desc + i, HIF_DMA_AHB_M1, HIF_DMA_AHB_M2);
		_hif_dma_desc_set_protection(desc + i, false, true, true);

		if (i >= 1)
			_hif_dma_desc_link(desc + i - 1, desc + i);
	}
	_hif_dma_desc_link(desc + i -1, uart_dma->desc);

	channel = _hif_dma_get_channel(true);
	if (channel < 0)
	{
		_hif_error("_hif_dma_get_channel() fail, err=%d\n", channel);
		goto uart_rx_dma_register_fail;
	}

	err = _hif_dma_config_p2m(channel, &peri,
			_hif_uart_rx_dma_inttc_isr, _hif_uart_rx_dma_interr_isr);
	if (err)
	{
		_hif_error("_hif_dma_config_p2m() fail, err=%d\n", err);
		goto uart_rx_dma_register_fail;
	}

	err = _hif_dma_start(channel, uart_dma->desc);
	if (err)
	{
		_hif_error("_hif_dma_start() fail, err=%d\n", err);
		goto uart_rx_dma_register_fail;
	}

	uart_dma->channel = channel;
	uart_dma->mem_addr = mem_addr;
	uart_dma->mem_size = mem_size;
	uart_dma->slot_addr = mem_addr;
	uart_dma->slot_size = slot_size;
	uart_dma->n_desc = n_desc;

	g_hif_uart_rx_dma = uart_dma;

	return 0;

uart_rx_dma_register_fail:

	if (uart_dma)
		_hif_free(uart_dma);

	return -1;
}

static void _hif_uart_rx_dma_unregister (void)
{
	if (g_hif_uart_rx_dma)
	{
		_hif_dma_stop(g_hif_uart_rx_dma->channel);

		_hif_free(g_hif_uart_rx_dma);
	}
}

static int _hif_uart_dma_register (int uart_channel,
							_hif_fifo_t *rx_fifo, _hif_fifo_t *tx_fifo)
{
	_hif_dma_enable();

	if (rx_fifo && _hif_uart_rx_dma_register(uart_channel,
					(uint32_t)rx_fifo->buffer, rx_fifo->size) != 0)
		return -1;

/*	if (tx_fifo && _hif_uart_tx_dma_register(uart_channeli,
					(uint32_t)tx_fifo->buffer, tx_fifo->size) != 0)
		return -1; */

	_hif_uart_rx_dma_enable();
/*	_hif_uart_tx_dma_enable(); */

	return 0;
}

static void _hif_uart_dma_unregister (void)
{
	_hif_uart_rx_dma_disable();
/*	_hif_uart_tx_dma_disable(); */

	_hif_uart_rx_dma_unregister();
/*	_hif_uart_tx_dma_unregister(); */

	_hif_dma_disable();
}

/*********************************************************************************************/

static bool _hif_uart_channel_valid (int channel, bool hfc)
{
	switch (channel)
	{
#ifdef NRC7292		
		case 2:
#else			
		case 1:
#endif			
			break;

		default:
			_hif_error("invalid channel %d\n", channel);
			return false;
	}

	if (channel != 2 && hfc)
	{
		_hif_error("channel %d dose not support rts/cts pins\n", channel);
		return false;
	}

	return true;	
}

static bool _hif_uart_baudrate_valid (int baudrate, bool hfc)
{
	switch (baudrate)
	{
		case 19200:
		case 38400:
		case 57600:
		case 115200:
			break;

		case 230400:
		case 460800:
		case 500000:
		case 576000:
		case 921600:
		case 1000000:
		case 1152000:
		case 1500000:
		case 2000000:
			if (hfc)
				break;
			
			_hif_error("Hardware flow control must be enabled to use baudrate %d.\n", baudrate);

		default:
			return false;
	}

	return true;
}

static int _hif_uart_channel_to_vector (int channel)
{
	const int vector[4] = {EV_HSUART0, EV_HSUART1, EV_HSUART2, EV_HSUART3};

	return vector[channel];
}

/*********************************************************************************************/

int _hif_uart_getc (char *data)
{
	if (g_hif_uart.channel < 0)
		return -1;

	if (g_hif_uart_rx_fifo)
	{
		_hif_fifo_mutex_take(g_hif_uart_rx_fifo);

		if (_hif_fifo_read(g_hif_uart_rx_fifo, data, 1) != 1)
		{
			_hif_fifo_mutex_give(g_hif_uart_rx_fifo);
			return -1;
		}

		_hif_fifo_mutex_give(g_hif_uart_rx_fifo);
	}
	else
	{
		if (_hif_uart_rx_empty())
			return -1;

		*data = _hif_uart_rx_data();
	}

	return 0;
}

int _hif_uart_putc (char data)
{
	if (g_hif_uart.channel < 0)
		return -1;

	if (g_hif_uart_tx_fifo)
	{
/*		_hif_fifo_mutex_take(g_hif_uart_tx_fifo); */

		if (_hif_fifo_write(g_hif_uart_tx_fifo, &data, 1) != 1)
		{
/*			_hif_fifo_mutex_give(g_hif_uart_tx_fifo); */
			return -1;
		}

/*		_hif_fifo_mutex_give(g_hif_uart_tx_fifo); */
	}
	else
	{
		if (_hif_uart_tx_full())
			return -1;

		_hif_uart_tx_data(data);
	}

	return 0;
}

int _hif_uart_read (char *buf, int len)
{
	int rx_cnt = 0;

	if (g_hif_uart.channel < 0)
		return 0;

	if (g_hif_uart_rx_fifo)
	{
		if (_hif_fifo_mutex_take(g_hif_uart_rx_fifo))
		{
			if (g_hif_uart.hfc == UART_HFC_ENABLE)
			{
				_hif_uart_rx_int_disable();
				rx_cnt = _hif_fifo_read(g_hif_uart_rx_fifo, buf, len);
				_hif_uart_rx_int_enable();
			}
			else
			{
				_hif_uart_rx_dma_update_fifo();
				rx_cnt = _hif_fifo_read(g_hif_uart_rx_fifo, buf, len);
			}

			_hif_fifo_mutex_give(g_hif_uart_rx_fifo);
		}
	}
	else
	{
		for (rx_cnt = 0 ; rx_cnt < len ; rx_cnt++)
		{
			if (_hif_uart_rx_empty())
				break;

			buf[rx_cnt] = _hif_uart_rx_data();
		}
	}

	return rx_cnt;
}

int _hif_uart_write (char *buf, int len)
{
	int tx_cnt = 0;

	if (g_hif_uart.channel < 0)
		return 0;

	if (g_hif_uart_tx_fifo)
	{
/*		_hif_fifo_mutex_take(g_hif_uart_tx_fifo); */

		tx_cnt = _hif_fifo_write(g_hif_uart_tx_fifo, buf, len);

/*		_hif_fifo_mutex_give(g_hif_uart_tx_fifo); */

		if (tx_cnt > 0)
			_hif_uart_tx_int_enable();
	}
	else
	{
		if (_hif_uart_tx_empty())
		{
			for (tx_cnt = 0 ; tx_cnt < 16 && tx_cnt < len ; tx_cnt++)
			{
				if (_hif_uart_tx_full())
				{
/*					_hif_debug("uart_tx_full, cnt=%d\n", tx_cnt); */
					break;
				}

				_hif_uart_tx_data(buf[tx_cnt]);
			}
		}
	}

	return tx_cnt;
}

/*********************************************************************************************/

static void _hif_uart_rx_isr (void)
{
	if (_hif_fifo_mutex_take_isr(g_hif_uart_rx_fifo))
	{
		int fifo_size;

		fifo_size = _hif_fifo_free_size(g_hif_uart_rx_fifo);
		if (fifo_size == 0)
		{
/*			_hif_info("fifo_full\n"); */

			_hif_uart_rx_int_disable();
		}
		else
		{
			int i;

			for (i = 0 ; i < fifo_size ; i++)
			{
				if (_hif_uart_rx_empty())
					break;

				_hif_fifo_putc(g_hif_uart_rx_fifo, _hif_uart_rx_data());
			}
		}

		_hif_rx_resume_isr();
		_hif_fifo_mutex_give_isr(g_hif_uart_rx_fifo);
	}
}

#if 0
static void _hif_uart_tx_isr (void)
{
	static char buf[16 + 1];
	static unsigned int size = 0;
	static unsigned int cnt = 0;
	int i;

	if (g_hif_uart_tx_fifo)
	{
/*		if (!_hif_fifo_mutex_take_isr(g_hif_uart_tx_fifo))
			return; */

		if (cnt >= size)
		{
			cnt = 0;
			size = _hif_fifo_read(g_hif_uart_tx_fifo, buf, sizeof(buf));
			if (size == 0)
			{
				_hif_uart_tx_int_disable();
				return;
			}
		}

/*		_hif_fifo_mutex_give_isr(g_hif_uart_tx_fifo); */

		for (i = 0 ; i < (size - cnt) ; i++)
		{
			if (_hif_uart_tx_full())
				break;

			_hif_uart_tx_data(buf[cnt + i]);
		}

		cnt += i;

		_hif_uart_tx_int_enable();
	}
}
#endif

static void _hif_uart_isr (int vector)
{
	if (g_hif_uart.channel > 0)
	{
		static volatile uint32_t status;

		status = _hif_uart_int_get_status();

/*		_hif_debug("uart_int: 0x%08X\n", status); */

		_hif_uart_int_clear(status & (ICR_RT|ICR_RX|ICR_TX));

		if (status & (MIS_RX|MIS_RT))
			_hif_uart_rx_isr();

/*		if (status & MIS_TX)
			_hif_uart_tx_isr(); */
	}
}

/*********************************************************************************************/

#define _hif_uart_pin_info(fmt, ...)		//_hif_info(fmt, ##__VA_ARGS__)

static void _hif_uart_pin_enable (int channel)
{
	gpio_io_t gpio;
	uio_sel_t uio;

	/*
     * UART0: TX=GP4 RX=GP5
	 * UART2: TX=GP0 RX=GP1 RTS=GP2 CTS=GP3
	 * UART3: TX=GP6 RX=GP7
	 */

	/* GPIO_ATL0 */
	nrc_gpio_get_alt(&gpio);
	_hif_uart_pin_info("GPIO_ALT0: 0x%X\n", gpio.word);

	switch (channel)
	{
		case 0:
			gpio.bit.io4 = 1;
			gpio.bit.io5 = 1;
			break;

		case 2:
			gpio.bit.io0 = 1;
			gpio.bit.io1 = 1;
			gpio.bit.io2 = 1;
			gpio.bit.io3 = 1;
			break;

		case 3:
			gpio.bit.io6 = 1;
			gpio.bit.io7 = 1;
	}

	nrc_gpio_set_alt(&gpio);

	nrc_gpio_get_alt(&gpio);
	_hif_uart_pin_info("GPIO_ALT0: 0x%X\n", gpio.word);

	/* UIO_SEL */
	switch (channel)
	{
		case 0:
			nrc_gpio_get_uio_sel(UIO_SEL_UART0, &uio);
			_hif_uart_pin_info("UIO_SEL_UART0: 0x%X\n", uio.word);

			uio.bit.sel7_0 = 4;
			uio.bit.sel15_8 = 5;
			uio.bit.sel23_16 = 0xff;
			uio.bit.sel31_24 = 0xff;

			nrc_gpio_set_uio_sel(UIO_SEL_UART0, &uio);

			nrc_gpio_get_uio_sel(UIO_SEL_UART0, &uio);
			_hif_uart_pin_info("UIO_SEL_UART0: 0x%X\n", uio.word);
			break;

		case 2:
			nrc_gpio_get_uio_sel(UIO_SEL_UART2, &uio);
			_hif_uart_pin_info("UIO_SEL_UART2: 0x%X\n", uio.word);

			uio.bit.sel7_0 = 0;
			uio.bit.sel15_8 = 1;
			uio.bit.sel23_16 = 2;
			uio.bit.sel31_24 = 3;

			nrc_gpio_set_uio_sel(UIO_SEL_UART2, &uio);

			nrc_gpio_get_uio_sel(UIO_SEL_UART2, &uio);
			_hif_uart_pin_info("UIO_SEL_UART2: 0x%X\n", uio.word);
			break;

		case 3:
			nrc_gpio_get_uio_sel(UIO_SEL_UART3, &uio);
			_hif_uart_pin_info("UIO_SEL_UART3: 0x%X\n", uio.word);

			uio.bit.sel7_0 = 6;
			uio.bit.sel15_8 = 7;
			uio.bit.sel23_16 = 0xff;
			uio.bit.sel31_24 = 0xff;

			nrc_gpio_set_uio_sel(UIO_SEL_UART3, &uio);

			nrc_gpio_get_uio_sel(UIO_SEL_UART3, &uio);
			_hif_uart_pin_info("UIO_SEL_UART3: 0x%X\n", uio.word);
	}

	/* GPIO_DIR */
	nrc_gpio_get_dir(&gpio);
	_hif_uart_pin_info("GPIO DIR: 0x%08X\n", gpio.word);

	gpio.word &= ~(1 << 9);
	nrc_gpio_config_dir(&gpio);

	nrc_gpio_get_dir(&gpio);
	_hif_uart_pin_info("GPIO DIR: 0x%08X\n", gpio.word);

	/* GPIO_PU */
	nrc_gpio_get_pullup(&gpio);
	_hif_uart_pin_info("GPIO PU: 0x%08X\n", gpio.word);

	/* GPIO_PD */
	nrc_gpio_get_pulldown(&gpio);
	_hif_uart_pin_info("GPIO PD: 0x%08X\n", gpio.word);
}

static void _hif_uart_pin_disable (int channel)
{
	gpio_io_t gpio;
	uio_sel_t uio;

	/*
     * UART0: TX=GP4 RX=GP5
	 * UART2: TX=GP0 RX=GP1 RTS=GP2 CTS=GP3
	 * UART3: TX=GP6 RX=GP7
	 */

	/* GPIO_ATL0 */
	nrc_gpio_get_alt(&gpio);
	_hif_uart_pin_info("GPIO_ALT0: 0x%X\n", gpio.word);

	switch (channel)
	{
		case 0:
			gpio.bit.io4 = 0;
			gpio.bit.io5 = 0;
			break;

		case 2:
			gpio.bit.io0 = 0;
			gpio.bit.io1 = 0;
			gpio.bit.io2 = 0;
			gpio.bit.io3 = 0;
			break;

		case 3:
			gpio.bit.io6 = 0;
			gpio.bit.io7 = 0;
	}

	nrc_gpio_set_alt(&gpio);
	nrc_gpio_get_alt(&gpio);
	_hif_uart_pin_info("GPIO_ALT0: 0x%X\n", gpio.word);

	/* UIO_SEL */
	switch (channel)
	{
		case 0:
			nrc_gpio_get_uio_sel(UIO_SEL_UART0, &uio);
			_hif_uart_pin_info("UIO_SEL_UART0: 0x%X\n", uio.word);

			uio.bit.sel7_0 = 0xff;
			uio.bit.sel15_8 = 0xff;
			uio.bit.sel23_16 = 0xff;
			uio.bit.sel31_24 = 0xff;

			nrc_gpio_set_uio_sel(UIO_SEL_UART0, &uio);
			nrc_gpio_get_uio_sel(UIO_SEL_UART0, &uio);
			_hif_uart_pin_info("UIO_SEL_UART0: 0x%X\n", uio.word);
			break;

		case 2:
			nrc_gpio_get_uio_sel(UIO_SEL_UART2, &uio);
			_hif_uart_pin_info("UIO_SEL_UART2: 0x%X\n", uio.word);

			uio.bit.sel7_0 = 0xff;
			uio.bit.sel15_8 = 0xff;
			uio.bit.sel23_16 = 0xff;
			uio.bit.sel31_24 = 0xff;

			nrc_gpio_set_uio_sel(UIO_SEL_UART2, &uio);
			nrc_gpio_get_uio_sel(UIO_SEL_UART2, &uio);
			_hif_uart_pin_info("UIO_SEL_UART2: 0x%X\n", uio.word);
			break;

		case 3:
			nrc_gpio_get_uio_sel(UIO_SEL_UART3, &uio);
			_hif_uart_pin_info("UIO_SEL_UART3: 0x%X\n", uio.word);

			uio.bit.sel7_0 = 0xff;
			uio.bit.sel15_8 = 0xff;
			uio.bit.sel23_16 = 0xff;
			uio.bit.sel31_24 = 0xff;

			nrc_gpio_set_uio_sel(UIO_SEL_UART3, &uio);
			nrc_gpio_get_uio_sel(UIO_SEL_UART3, &uio);
			_hif_uart_pin_info("UIO_SEL_UART3: 0x%X\n", uio.word);
	}
}

static int _hif_uart_enable (_hif_uart_t *uart)
{
	bool tx_irq = false;
	bool rx_irq = true;

	if (!uart)
		return -1;

	_hif_info("UART Enable: channel=%d badurate=%d data=%d stop=%d parity=%s fifo=%d,%d hfc=%s\r\n",
			uart->channel, uart->baudrate, uart->data_bits + 5, uart->stop_bits + 1,
			uart->parity == UART_PB_ODD ? "odd" : (uart->parity == UART_PB_EVEN ? "even" : "none"),
			_HIF_UART_RX_HW_FIFO_LEVEL, _HIF_UART_TX_HW_FIFO_LEVEL,
			uart->hfc == UART_HFC_ENABLE ? "on" : "off");

	nrc_hsuart_init(uart->channel, uart->data_bits, uart->baudrate, uart->stop_bits, uart->parity,
						uart->hfc, UART_FIFO_ENABLE);

	nrc_hsuart_fifo_level(uart->channel, _HIF_UART_TX_HW_FIFO_LEVEL, _HIF_UART_RX_HW_FIFO_LEVEL);
	
	if (uart->hfc == UART_HFC_DISABLE)
	{
		tx_irq = false;
		rx_irq = false;
	}

	_hif_info("UART Enable: rx_irq=%s tx_irq=%s\r\n", rx_irq ? "on" : "off", tx_irq ? "on" : "off");

	nrc_hsuart_int_clr(uart->channel, true, true, true); /* tx_empty, rx_done, rx_timeout */
	nrc_hsuart_interrupt(uart->channel, tx_irq, rx_irq); /* tx_empty, rx_done & rx_timeout */

	if (tx_irq || rx_irq)
	{
		system_register_isr(_hif_uart_channel_to_vector(uart->channel), _hif_uart_isr);
		system_irq_unmask(_hif_uart_channel_to_vector(uart->channel));
	}

	_hif_uart_pin_enable(uart->channel);

	_hif_uart_set_info(uart);

	return 0;
}

static void _hif_uart_disable (void)
{
	if (_hif_uart_channel_valid(g_hif_uart.channel, g_hif_uart.hfc))
	{
		int channel = g_hif_uart.channel;

		_hif_info("UART Disable: channel=%d\r\n", channel);

		nrc_hsuart_interrupt(channel, false, false); 	/* tx_en, rx_rt_en */
		nrc_hsuart_int_clr(channel, true, true, true); 	/* tx_int, rx_int, rt_int */

		/* system_irq_mask(_hif_uart_channel_to_vector(channel)); */
		system_register_isr(_hif_uart_channel_to_vector(channel), NULL);

		_hif_uart_pin_disable(channel);

		_hif_uart_init_info(&g_hif_uart);
	}
}

/*********************************************************************************************/

int _hif_uart_open (_hif_info_t *info)
{
	if (info)
	{
		switch (info->type)
		{
	 		case _HIF_TYPE_UART:
	 		case _HIF_TYPE_UART_HFC:
			{
				_hif_uart_t *uart = &info->uart;

				_hif_info("UART Open: channel=%d baudrate=%d hfc=%s\n",
							uart->channel, uart->baudrate,
							uart->hfc == UART_HFC_ENABLE ? "on" : "off");

				if (!_hif_uart_channel_valid(uart->channel, uart->hfc))
					goto uart_open_fail;

				if (!_hif_uart_baudrate_valid(uart->baudrate, uart->hfc))
					goto uart_open_fail;

				if (_hif_uart_fifo_create(info) != 0 || _hif_uart_enable(uart) != 0)
					goto uart_open_fail;

				if (uart->hfc == UART_HFC_DISABLE)
				{
					if (_hif_uart_dma_register(uart->channel, g_hif_uart_rx_fifo, g_hif_uart_tx_fifo) != 0)
						goto uart_open_fail;
				}

				_hif_info("UART Open: success\n");

				return 0;
			}

			default:
				break;
		}
	}

uart_open_fail:

	_hif_info("UART Open: fail\n");

	return -1;
}

void _hif_uart_close (void)
{
	if (_hif_uart_channel_valid(g_hif_uart.channel, g_hif_uart.hfc))
	{
		_hif_info("UART Close: channel=%d", g_hif_uart.channel);

	   	_hif_uart_dma_unregister();

		_hif_uart_disable();

		_hif_uart_fifo_delete();

		_hif_uart_init_info(&g_hif_uart);
	}
}

int _hif_uart_change (_hif_uart_t *new)
{
	_hif_uart_t old;

	if (new)
	{	
		if (!_hif_uart_channel_valid(new->channel, new->hfc))
			_hif_info("UART Change: invalid channel=%d\n", new->channel);
		else if (!_hif_uart_baudrate_valid(new->baudrate, new->hfc))
			_hif_info("UART Change: invalid baudrate=%d\n", new->baudrate);
		else
		{
			_hif_uart_get_info(&old);

			_hif_info("UART Change: \n");
			_hif_info(" - channel: %d->%d\n", old.channel, new->channel);
			_hif_info(" - baudrate: %d->%d\n", old.baudrate, new->baudrate);
			_hif_info(" - data bits: %d->%d\n", old.data_bits, new->data_bits);
			_hif_info(" - stop  bits: %d->%d\n", old.stop_bits, new->stop_bits);
			_hif_info(" - parity: %d->%d\n", old.parity, new->parity);
			_hif_info(" - hfc: %d->%d\n", old.hfc, new->hfc);

			if (old.hfc == UART_HFC_DISABLE)
				_hif_uart_dma_unregister();

			_hif_uart_disable();

			if (_hif_uart_enable(new) == 0)
			{
				int ret = 0;

				if (new->hfc == UART_HFC_DISABLE)
				{
					_hif_fifo_reset(g_hif_uart_rx_fifo);
					_hif_fifo_reset(g_hif_uart_tx_fifo);

					ret = _hif_uart_dma_register(new->channel, g_hif_uart_rx_fifo, g_hif_uart_tx_fifo);
				}

				if (ret == 0)
				{
					_hif_info("UART Change: success\n");

					return 0;
				}

				_hif_uart_disable();
			}

			_hif_uart_enable(&old);

			if (old.hfc == UART_HFC_DISABLE)
				_hif_uart_dma_register(old.channel, g_hif_uart_rx_fifo, g_hif_uart_tx_fifo);

			_hif_info("UART Change: fail\n");

			return -1;
		}
	}

	return -2; /* invalid */
}

