/*
 * MIT License
 *
 * Copyright (c) 2022 Newracom, Inc.
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

/**********************************************************************************************/

#if !defined(CONFIG_HIF_HSPI_SLOT_NUM)
#error "Not defined CONFIG_HIF_HSPI_SLOT_NUM"
#endif

#if !defined(CONFIG_HIF_HSPI_SLOT_SIZE)
#error "Not defined CONFIG_HIF_HSPI_SLOT_SIZE"
#endif

#define _hspi_read_info(fmt, ...)		/* _hif_info("hspi_read: " fmt, ##__VA_ARGS__) */
#define _hspi_write_info(fmt, ...)		/* _hif_info("hspi_write: " fmt, ##__VA_ARGS__) */

/**********************************************************************************************/

enum _hspi_queue_index
{
	_HSPI_TXQ = 0, 	/* target to host 	*/
	_HSPI_RXQ,		/* target from host	*/

	_HSPI_QUE_NUM
};

typedef struct
{
	uint32_t device_ready; /* start to download firmware, write 0x1ACCE551 */
	uint32_t device_sleep; /* write 0x1ACCE551 */

	uint32_t reserved1[2];

	uint32_t device_message[4];

	uint32_t reserved2[4];

	uint32_t chip_id; 	/* hw fix, read only */
	uint32_t modem_id; 	/* hw fix, read only */
	uint32_t sw_id;
	uint32_t board_id;
} _hspi_sys_reg_t;

typedef struct
{
	uint32_t reset:1; /* auto clear */
	uint32_t reserved1:31;

	union
	{
		uint32_t val;

		struct
		{
			uint32_t bit_msb:1;
			uint32_t byte_msb:1;
			uint32_t reserved1:2;
			uint32_t header_on:1;
			uint32_t reserved2:26;
			uint32_t enable:1;
		};
	} que_mode;

	union
	{
		uint32_t val;

		struct
		{
			uint32_t active:1; 	/* 0:low, 1:high 	*/
			uint32_t trigger:1; /* 0:level, 1:edge	*/
			uint32_t reserved:29;
			uint32_t enable:1;
		};
	} irq_mode;

	uint32_t reserved2;

	union
	{
		uint32_t val[2];

		struct
		{
			uint32_t target_address; 	/* word align */

			uint32_t target_size:16; 	/* size (word) */
			uint32_t reserved1:11;
			uint32_t target_intmask:1; 	/* 0:enable, 1:disable */
			uint32_t reserved2:2;
			uint32_t eirq_event:1; 		/* to host, auto clear */
			uint32_t que_update:1; 		/* auto clear */
		};
	} set;

#define _HSPI_SLOT_CNT_MAX	0x3F /* 63 */

	union
	{
		uint32_t val;

		struct
		{
			uint32_t total:16; 		/* queue slot size (word), read only */
			uint32_t count:6; 		/* queue number, read only */
			uint32_t reserved1:1;
			uint32_t error:1; 		/* read only */
			uint32_t reserved2:7;
			uint32_t irq:1; 		/* clear when writing. */
		};
	} status;
} _hspi_que_reg_t;

typedef _hspi_que_reg_t		_hspi_txq_reg_t;
typedef _hspi_que_reg_t		_hspi_rxq_reg_t;

typedef struct
{
	uint16_t size;
	uint16_t num;
} _hspi_slot_info_t;

typedef struct
{
#define _HSPI_SLOT_SIZE_MAX		512
#define _HSPI_SLOT_HDR_SIZE		4
#define _HSPI_SLOT_START_SIZE	2
#define _HSPI_SLOT_START_CHAR	"HS"
#define _HSPI_SLOT_SEQ_MAX		0x3F

	uint8_t start[2];
	uint16_t len:10;
	uint16_t seq:6;
} _hspi_hdr_t;

/**********************************************************************************************/

#define _HSPI_RX_SLOT_NUM				CONFIG_HIF_HSPI_SLOT_NUM
#define _HSPI_RX_SLOT_SIZE				CONFIG_HIF_HSPI_SLOT_SIZE
#define _HSPI_RX_SLOT_DATA_LEN_MAX		(_HSPI_RX_SLOT_SIZE - _HSPI_SLOT_HDR_SIZE)

#define _HSPI_TX_SLOT_NUM				CONFIG_HIF_HSPI_SLOT_NUM
#define _HSPI_TX_SLOT_SIZE				CONFIG_HIF_HSPI_SLOT_SIZE
#define _HSPI_TX_SLOT_DATA_LEN_MAX		(_HSPI_TX_SLOT_SIZE - _HSPI_SLOT_HDR_SIZE)

static volatile _hspi_sys_reg_t *g_hspi_sys_reg = (volatile _hspi_sys_reg_t *)HIF_BASE_ADDR;
static volatile _hspi_que_reg_t *g_hspi_que_reg[_HSPI_QUE_NUM] =
{
	(volatile _hspi_txq_reg_t *)HIF_TXQUE_BASE_ADDR,
	(volatile _hspi_rxq_reg_t *)HIF_RXQUE_BASE_ADDR
};

static _hif_fifo_t *g_hif_hspi_rx_fifo = NULL;
static _hif_fifo_t *g_hif_hspi_tx_fifo = NULL;

/**********************************************************************************************/

static void _hif_hspi_print_regs (void)
{
	int i;

	_hif_info("");

	_hif_info("[ HSPI SYS Registers ]");
	_hif_info(" - device_ready : 0x%08X", g_hspi_sys_reg->device_ready);
	_hif_info(" - device_sleep : 0x%08X", g_hspi_sys_reg->device_sleep);
	_hif_info(" - device_message[0] : 0x%08X", g_hspi_sys_reg->device_message[0]);
	_hif_info(" - device_message[1] : 0x%08X", g_hspi_sys_reg->device_message[1]);
	_hif_info(" - device_message[2] : 0x%08X", g_hspi_sys_reg->device_message[2]);
	_hif_info(" - device_message[3] : 0x%08X", g_hspi_sys_reg->device_message[3]);
	_hif_info(" - chip_id : 0x%08X", g_hspi_sys_reg->chip_id);
	_hif_info(" - modem_id : 0x%08X", g_hspi_sys_reg->modem_id);
	_hif_info(" - sw_id : 0x%08X", g_hspi_sys_reg->sw_id);
	_hif_info(" - board_id : 0x%08X", g_hspi_sys_reg->board_id);

	for (i = 0 ; i < 2 ; i++)
	{
		_hif_info("[ HSPI %cXQ Registers ]", i == 0 ? 'T' : 'R');
		_hif_info(" - reset	: %u", g_hspi_que_reg[i]->reset);
		_hif_info(" - que_mode : 0x%08X", g_hspi_que_reg[i]->que_mode.val);
		_hif_info(" - irq_mode : 0x%08X", g_hspi_que_reg[i]->irq_mode.val);
		_hif_info(" - set[0]   : 0x%08X", g_hspi_que_reg[i]->set.val[0]);
		_hif_info(" - set[1]   : 0x%08X", g_hspi_que_reg[i]->set.val[1]);
		_hif_info(" - status   : 0x%08X", g_hspi_que_reg[i]->status);
	}

	_hif_info("");
}

/**********************************************************************************************/

static void _hif_hspi_ready (void)
{
	g_hspi_sys_reg->device_ready = 0x1ACCE551;
}

static void _hif_hspi_sleep (void)
{
	g_hspi_sys_reg->device_sleep = 0x1ACCE551;
}

static void _hif_hspi_set_sw_id (uint32_t id)
{
	g_hspi_sys_reg->sw_id = id;
}

static uint32_t _hif_hspi_get_sw_id (uint32_t id)
{
	return g_hspi_sys_reg->sw_id;
}

static void _hif_hspi_set_board_id (uint32_t id)
{
	g_hspi_sys_reg->board_id = id;
}

static uint32_t _hif_hspi_get_board_id (uint32_t id)
{
	return g_hspi_sys_reg->board_id;
}

static void _hif_hspi_set_message (int idx, uint32_t msg)
{
	g_hspi_sys_reg->device_message[idx] = msg;
}

static uint32_t _hif_hspi_get_message (int idx)
{
	return g_hspi_sys_reg->device_message[idx];
}

static void _hif_hspi_queue_reset (int idx)
{
	volatile _hspi_que_reg_t *reg = g_hspi_que_reg[idx];

	reg->reset = 1;
}

static void _hif_hspi_queue_enable (int idx, int header_on, int byte_msb, int bit_msb)
{
	volatile _hspi_que_reg_t *reg = g_hspi_que_reg[idx];

	reg->que_mode.header_on = header_on;
	reg->que_mode.byte_msb = byte_msb;
	reg->que_mode.bit_msb = bit_msb;
	reg->que_mode.enable = 1;
}

static void _hif_hspi_queue_disable (int idx)
{
	volatile _hspi_que_reg_t *reg = g_hspi_que_reg[idx];

	reg->que_mode.header_on = 0;
	reg->que_mode.byte_msb = 0;
	reg->que_mode.bit_msb = 0;
	reg->que_mode.enable = 0;
}

static void _hif_hspi_queue_enable_irq (int idx, int trigger, int active)
{
	volatile _hspi_que_reg_t *reg = g_hspi_que_reg[idx];

	reg->irq_mode.trigger = trigger;
	reg->irq_mode.active = active;
	reg->irq_mode.enable = 1;
}

static void _hif_hspi_queue_disable_irq (int idx)
{
	volatile _hspi_que_reg_t *reg = g_hspi_que_reg[idx];

	reg->irq_mode.trigger = 0;
	reg->irq_mode.active = 0;
	reg->irq_mode.enable = 0;
}

static void _hif_hspi_queue_clear_irq (int idx)
{
	volatile _hspi_que_reg_t *reg = g_hspi_que_reg[idx];

	reg->status.irq = 1;
}

static void _hif_hspi_queue_setup (int idx, int size, int irq)
{
	volatile _hspi_que_reg_t *reg = g_hspi_que_reg[idx];

	reg->set.target_size = (uint16_t)(size >> 2);
	reg->set.target_intmask = irq ? 0 : 1;
}

static int _hif_hspi_queue_update (int idx, char *addr)
{
	volatile _hspi_que_reg_t *reg = g_hspi_que_reg[idx];
	volatile char *slot_base = NULL;
	uint32_t slot_offset;
	uint32_t slot_size;

	if (!addr || (uint32_t)addr & 0x3)
	{
		_hif_error("addr=%p", addr);
		return -1;
	}

	switch (idx)
	{
		case _HSPI_TXQ:
/*			_hspi_write_info("hspi_txq_update: %p\n", addr); */

			slot_base = g_hif_hspi_tx_fifo->buffer;
			slot_size = _HSPI_TX_SLOT_SIZE;
			break;

		case _HSPI_RXQ:
/*			_hspi_read_info("hspi_rxq_update: %p\n", addr); */

			slot_base = g_hif_hspi_rx_fifo->buffer;
			slot_size = _HSPI_RX_SLOT_SIZE;
			break;

		default:
			return -1;
	}

	slot_offset = (uint32_t)(addr - slot_base);

	if (slot_offset % slot_size)
	{
		_hif_error("addr=%p (offset=%u size=%u)",
						addr, slot_offset, slot_size);
		return -1;
	}

	reg->set.target_address = (uint32_t)addr;
	reg->set.que_update = 1;

	return 0;
}

static void _hif_hspi_queue_event_to_host (int idx)
{
#if 0
	volatile _hspi_que_reg_t *reg = g_hspi_que_reg[idx];

	reg->set.eirq_event = 1; /* auto clear */
#else
	g_hspi_sys_reg->device_sleep = 0x1ACCE551;
#endif
}

static int _hif_hspi_queue_is_error (int idx)
{
	volatile _hspi_que_reg_t *reg = g_hspi_que_reg[idx];

	return reg->status.error;
}

static int _hif_hspi_queue_count (int idx)
{
	volatile _hspi_que_reg_t *reg = g_hspi_que_reg[idx];

	return reg->status.count;
}

static int _hif_hspi_queue_total (int idx)
{
	volatile _hspi_que_reg_t *reg = g_hspi_que_reg[idx];

	return reg->status.total;
}

static void _hif_hspi_queue_status_print (int idx)
{
	const char *que_name[_HSPI_QUE_NUM] = { "txq", "rxq" };
	volatile _hspi_que_reg_t *reg = g_hspi_que_reg[idx];

	_hif_info("%s_status: val(0x%08X), count(%u/%u), error(%u)", que_name[idx],
			reg->status.val, reg->status.count, reg->status.total, reg->status.error);
}

/**********************************************************************************************/

static void _hif_hspi_rxq_isr (int vector) /* from host */
{
	_hif_hspi_queue_clear_irq(_HSPI_RXQ);

	_hif_rx_resume_isr();

/*	_hif_hspi_queue_status_print(_HSPI_RXQ); */
}

static void _hif_hspi_txq_isr (int vector) /* to host */
{
	_hif_hspi_queue_clear_irq(_HSPI_TXQ);

/*	_hif_hspi_queue_status_print(_HSPI_TXQ); */
}

/**********************************************************************************************/

static int __hif_hspi_read (char *buf, int len)
{
	static uint8_t seq = 0;
	_hspi_hdr_t hdr;
	int ret;
	int i;

	for (i = 0 ; (len - i) >= _HSPI_RX_SLOT_DATA_LEN_MAX ; i += hdr.len)
	{
		ret = _hif_fifo_read(g_hif_hspi_rx_fifo, (char *)&hdr, _HSPI_SLOT_HDR_SIZE);
		if (ret == 0)
		{
			if (i == 0)
				_hspi_read_info("empty\n");
			break;
		}
		else if (ret != _HSPI_SLOT_HDR_SIZE)
		{
			_hif_error("slot_hdr: %d/%d", ret, _HSPI_SLOT_HDR_SIZE);
			break;
		}

		if (memcmp(hdr.start, _HSPI_SLOT_START_CHAR, _HSPI_SLOT_START_SIZE) != 0 || hdr.len > _HSPI_RX_SLOT_DATA_LEN_MAX)
		{
			hdr.len = 0;

			_hif_error("slot: addr=%p start=%02X,%02X len=%u",
							_hif_fifo_pop_addr(g_hif_hspi_rx_fifo, 0) - _HSPI_SLOT_HDR_SIZE,
							hdr.start[0], hdr.start[1], 
							hdr.len);
		}
		else
		{
			_hspi_read_info("seq=%u len=%u\n", hdr.seq, hdr.len);

			if (hdr.seq != seq)
			{
				_hif_error("slot_seq: %u -> %u", seq, hdr.seq);

				seq = hdr.seq;
			}

			if (++seq > _HSPI_SLOT_SEQ_MAX)
			   seq = 0;
		}

		ret = _hif_fifo_read(g_hif_hspi_rx_fifo, hdr.len > 0 ? buf + i : NULL, _HSPI_RX_SLOT_DATA_LEN_MAX);
		if (ret != _HSPI_RX_SLOT_DATA_LEN_MAX)
		{
			_hif_error("slot_data: %d/%d", ret, _HSPI_RX_SLOT_DATA_LEN_MAX);
			break;
		}
	}

	return i;
}

int _hif_hspi_read (char *buf, int len)
{
	static int rxq_total_cnt = 0;
	int rxq_done_cnt;
	int rxq_free_cnt;
	int rxq_update_cnt;
	char *rxq_addr;
	int ret;
	int i;

	if (!buf || !len)
		return 0;

	rxq_done_cnt = rxq_total_cnt - _hif_hspi_queue_count(_HSPI_RXQ);
	if (rxq_done_cnt > 0)
	{
		rxq_total_cnt -= rxq_done_cnt;
		_hif_fifo_write(g_hif_hspi_rx_fifo, NULL, rxq_done_cnt * _HSPI_RX_SLOT_SIZE);
	}

	ret = __hif_hspi_read(buf, len);

	rxq_free_cnt = _hif_fifo_free_size(g_hif_hspi_rx_fifo) / _HSPI_RX_SLOT_SIZE;

	rxq_update_cnt = rxq_free_cnt - rxq_total_cnt;

	if (_HSPI_RX_SLOT_NUM > _HSPI_SLOT_CNT_MAX)
		rxq_update_cnt -= (_HSPI_RX_SLOT_NUM - _HSPI_SLOT_CNT_MAX);

	for (i = 0 ; i < rxq_update_cnt ; i++)
	{
		rxq_addr = _hif_fifo_push_addr(g_hif_hspi_rx_fifo, (rxq_total_cnt + i) * _HSPI_RX_SLOT_SIZE);

		_hif_hspi_queue_update(_HSPI_RXQ, rxq_addr);
	}

	rxq_total_cnt += rxq_update_cnt;

/*	if (rxq_done_cnt > 0 || rxq_update_cnt > 0 || ret > 0) */
	{
		_hspi_read_info("done=%d update=%d total=%d len=%d/%d\n",
				rxq_done_cnt, rxq_update_cnt, rxq_total_cnt, ret, len);
	}

	return ret;
}

int __hif_hspi_write (char *buf, int len)
{
	static uint8_t seq = 0;
	_hspi_hdr_t hdr;
	int i;

	memcpy(hdr.start, _HSPI_SLOT_START_CHAR, _HSPI_SLOT_START_SIZE);
	hdr.len = _HSPI_TX_SLOT_DATA_LEN_MAX;

	for (i = 0 ; i < len ; i += hdr.len)
	{
		if ((len - i) < _HSPI_TX_SLOT_DATA_LEN_MAX)
			hdr.len = len - i;

		hdr.seq = seq;
		if (++seq > _HSPI_SLOT_SEQ_MAX)
			seq = 0;

		_hif_fifo_write(g_hif_hspi_tx_fifo, (char *)&hdr, _HSPI_SLOT_HDR_SIZE);
		_hif_fifo_write(g_hif_hspi_tx_fifo, buf + i, hdr.len);
	}

	if (hdr.len < _HSPI_TX_SLOT_DATA_LEN_MAX)
		_hif_fifo_write(g_hif_hspi_tx_fifo, NULL, _HSPI_TX_SLOT_DATA_LEN_MAX - hdr.len);

	if (i != len)
		_hif_error("slot_data: %d/%d", i, len);

	return i;
}

int _hif_hspi_write (char *buf, int len)
{
	static int txq_total_cnt = 0;
	int txq_done_cnt;
	int txq_update_cnt;
	char *txq_addr;
	int ret = 0;
	int i;

	if (!buf || !len)
		return 0;

	txq_update_cnt = (len / _HSPI_TX_SLOT_DATA_LEN_MAX) + ((len % _HSPI_TX_SLOT_DATA_LEN_MAX) ? 1 : 0);

	txq_done_cnt = txq_total_cnt - _hif_hspi_queue_count(_HSPI_TXQ);
	if (txq_done_cnt > 0)
	{
		txq_total_cnt -= txq_done_cnt;
		_hif_fifo_read(g_hif_hspi_tx_fifo, NULL, txq_done_cnt * _HSPI_TX_SLOT_SIZE);
	}

	if ((txq_update_cnt + txq_total_cnt) > _HSPI_TX_SLOT_NUM)
		txq_update_cnt = _HSPI_TX_SLOT_NUM - txq_total_cnt;

	if ((txq_update_cnt + txq_total_cnt) > _HSPI_SLOT_CNT_MAX)
		txq_update_cnt = _HSPI_SLOT_CNT_MAX - txq_total_cnt;

	if ((txq_update_cnt * _HSPI_TX_SLOT_DATA_LEN_MAX) < len)
		len = txq_update_cnt * _HSPI_TX_SLOT_DATA_LEN_MAX;

	if (txq_update_cnt > 0)
	{
		ret = __hif_hspi_write(buf, len);

		for (i = 0 ; i < txq_update_cnt ; i++)
		{
			txq_addr = _hif_fifo_push_addr(g_hif_hspi_tx_fifo, (i - txq_update_cnt) * _HSPI_TX_SLOT_SIZE);

			_hif_hspi_queue_update(_HSPI_TXQ, txq_addr);
		}

		txq_total_cnt += txq_update_cnt;

		_hif_hspi_queue_event_to_host(_HSPI_TXQ);
	}

/*	if (txq_done_cnt > 0 || txq_update_cnt > 0 || ret > 0) */
	{
		_hspi_write_info("done=%d update=%d total=%d len=%d/%d\n",
						txq_done_cnt, txq_update_cnt, txq_total_cnt, ret, len);
	}

	return ret;
}

/**********************************************************************************************/

#if defined(NRC7394)

#if 1 /* Need to define in hal/nrc739x/hal_gpio.h */
#define UIO_SEL_HSPI	UIO_SEL_SPI2
#define UIO_SEL_EIRQ	UIO_SEL_SPI3
#endif

#define _hif_hspi_pin_info(fmt, ...)	/*  _hif_info(fmt, ##__VA_ARGS__) */

typedef struct
{
	int8_t cs;
	int8_t clk;
	int8_t mosi;
	int8_t miso;
	int8_t eirq;
} _hif_hspi_pin_t;

static const _hif_hspi_pin_t g_hif_hspi_pin = { 28, 7, 6, 29, 30 };

static void _hif_hspi_pin_enable (void)
{
	const _hif_hspi_pin_t *hspi_pin = &g_hif_hspi_pin;
	gpio_io_t gpio;
	uio_sel_t uio;

	/*
	 * NOTE: Only 56 pin package is supported.
	 */
	_hif_hspi_pin_info("PKG_OPT: 0x%08X\n", RegSCFG_PKG_OPT);

	if (!(RegSCFG_PKG_OPT & 0x1))
	{
		/*
		 * Access key is required as 0xCA78 on [31:16] to change [0] value.
		 */
		RegSCFG_PKG_OPT = (0xCA78 << 16) | 0x1;

		_hif_hspi_pin_info("PKG_OPT: 0x%08X\n", RegSCFG_PKG_OPT);
	}

	/* GPIO_DIR */
	nrc_gpio_get_dir(&gpio);
	_hif_hspi_pin_info("GPIO_DIR: 0x%08X\n", gpio.word);

	gpio.word &= ~(1 << hspi_pin->cs);   /* input  */
	gpio.word &= ~(1 << hspi_pin->clk);  /* input  */
	gpio.word &= ~(1 << hspi_pin->mosi); /* input  */
	gpio.word |= (1 << hspi_pin->miso); /* output */
	gpio.word |= (1 << hspi_pin->eirq); /* output */

	nrc_gpio_config_dir(&gpio);

/*	nrc_gpio_outb(hspi_pin->cs, 0);
	nrc_gpio_outb(hspi_pin->clk, 0);
	nrc_gpio_outb(hspi_pin->mosi, 0); */
	nrc_gpio_outb(hspi_pin->miso, 1);
	nrc_gpio_outb(hspi_pin->eirq, 1);

	nrc_gpio_get_dir(&gpio);
	_hif_hspi_pin_info("GPIO_DIR: 0x%08X\n", gpio.word);

	/* GPIO_ATL2 */
	nrc_gpio_get_alt2(&gpio);
	_hif_hspi_pin_info("GPIO_ALT2: 0x%08X\n", gpio.word);

	gpio.word |= (1 << hspi_pin->cs);
	gpio.word |= (1 << hspi_pin->clk);
	gpio.word |= (1 << hspi_pin->mosi);
	gpio.word |= (1 << hspi_pin->miso);
	gpio.word |= (1 << hspi_pin->eirq);

	nrc_gpio_set_alt2(&gpio);
	nrc_gpio_get_alt2(&gpio);
	_hif_hspi_pin_info("GPIO_ALT2: 0x%08X\n", gpio.word);

	/* UIO_SEL */
	nrc_gpio_get_uio_sel(UIO_SEL_HSPI, &uio);
	_hif_hspi_pin_info("UIO_SEL_HSPI: 0x%08X\n", uio.word);

	uio.bit.sel7_0 = hspi_pin->clk;
	uio.bit.sel15_8 = hspi_pin->cs;
	uio.bit.sel23_16 = hspi_pin->miso;
	uio.bit.sel31_24 = hspi_pin->mosi;

	nrc_gpio_set_uio_sel(UIO_SEL_HSPI, &uio);
	nrc_gpio_get_uio_sel(UIO_SEL_HSPI, &uio);
	_hif_hspi_pin_info("UIO_SEL_HSPI: 0x%08X\n", uio.word);

	nrc_gpio_get_uio_sel(UIO_SEL_EIRQ, &uio);
	_hif_hspi_pin_info("UIO_SEL_EIRQ: 0x%08X\n", uio.word);

	uio.bit.sel7_0 = hspi_pin->eirq;
	uio.bit.sel15_8 = 0xff;
	uio.bit.sel23_16 = 0xff;
	uio.bit.sel31_24 = 0xff;

	nrc_gpio_set_uio_sel(UIO_SEL_EIRQ, &uio);
	nrc_gpio_get_uio_sel(UIO_SEL_EIRQ, &uio);
	_hif_hspi_pin_info("UIO_SEL_EIRQ: 0x%08X\n", uio.word);
}

static void _hif_hspi_pin_disable (void)
{
	const _hif_hspi_pin_t *hspi_pin = &g_hif_hspi_pin;
	gpio_io_t gpio;
	uio_sel_t uio;

	/* GPIO_ATL0 */
	nrc_gpio_get_alt(&gpio);
	_hif_hspi_pin_info("GPIO_ALT0: 0x%08X\n", gpio.word);

	gpio.word &= ~(1 << hspi_pin->cs);
	gpio.word &= ~(1 << hspi_pin->clk);
	gpio.word &= ~(1 << hspi_pin->mosi);
	gpio.word &= ~(1 << hspi_pin->miso);
	gpio.word &= ~(1 << hspi_pin->eirq);

	nrc_gpio_set_alt(&gpio);
	nrc_gpio_get_alt(&gpio);
	_hif_hspi_pin_info("GPIO_ALT0: 0x%08X\n", gpio.word);

	/* UIO_SEL */
	nrc_gpio_get_uio_sel(UIO_SEL_HSPI, &uio);
	_hif_hspi_pin_info("UIO_SEL_HSPI: 0x%08X\n", uio.word);

	uio.bit.sel7_0 = 0xff;
	uio.bit.sel15_8 = 0xff;
	uio.bit.sel23_16 = 0xff;
	uio.bit.sel31_24 = 0xff;

	nrc_gpio_set_uio_sel(UIO_SEL_HSPI, &uio);
	nrc_gpio_get_uio_sel(UIO_SEL_HSPI, &uio);
	_hif_hspi_pin_info("UIO_SEL_HSPI: 0x%08X\n", uio.word);

	nrc_gpio_get_uio_sel(UIO_SEL_EIRQ, &uio);
	_hif_hspi_pin_info("UIO_SEL_EIRQ: 0x%08X\n", uio.word);

	uio.bit.sel7_0 = 0xff;
	uio.bit.sel15_8 = 0xff;
	uio.bit.sel23_16 = 0xff;
	uio.bit.sel31_24 = 0xff;

	nrc_gpio_set_uio_sel(UIO_SEL_EIRQ, &uio);
	nrc_gpio_get_uio_sel(UIO_SEL_EIRQ, &uio);
	_hif_hspi_pin_info("UIO_SEL_EIRQ: 0x%08X\n", uio.word);

	/* GPIO_DIR */
	nrc_gpio_get_dir(&gpio);
	_hif_hspi_pin_info("GPIO_DIR: 0x%08X\n", gpio.word);

	gpio.word |= (1 << hspi_pin->cs);   /* output */
	gpio.word |= (1 << hspi_pin->clk);  /* output */
	gpio.word |= (1 << hspi_pin->mosi); /* output */
	gpio.word |= (1 << hspi_pin->miso); /* output */
	gpio.word |= (1 << hspi_pin->eirq); /* output */

	nrc_gpio_config_dir(&gpio);

	nrc_gpio_outb(hspi_pin->cs, 1);
	nrc_gpio_outb(hspi_pin->clk, 1);
	nrc_gpio_outb(hspi_pin->mosi, 1);
	nrc_gpio_outb(hspi_pin->miso, 1);
	nrc_gpio_outb(hspi_pin->eirq, 1);

	nrc_gpio_get_dir(&gpio);
	_hif_hspi_pin_info("GPIO_DIR: 0x%08X\n", gpio.word);
}

#endif /* #if defined(NRC7394) */

static void _hif_hspi_fifo_delete (void)
{
	if (g_hif_hspi_rx_fifo)
	{
		_hif_fifo_delete(g_hif_hspi_rx_fifo);
		g_hif_hspi_rx_fifo = NULL;
	}

	if (g_hif_hspi_tx_fifo)
	{
		_hif_fifo_delete(g_hif_hspi_tx_fifo);
		g_hif_hspi_tx_fifo = NULL;
	}
}

static int _hif_hspi_fifo_create (_hif_info_t *info)
{
	_hif_buf_t *rx_fifo = &info->rx_fifo;
	_hif_buf_t *tx_fifo = &info->tx_fifo;

	if (rx_fifo->size > 0)
		g_hif_hspi_rx_fifo = _hif_fifo_create(rx_fifo->addr, rx_fifo->size);

	if (tx_fifo->size > 0)
		g_hif_hspi_tx_fifo = _hif_fifo_create(tx_fifo->addr, tx_fifo->size);

	if (!g_hif_hspi_rx_fifo || !g_hif_hspi_tx_fifo)
	{
		_hif_error("rx=%p(%d) tx=%p(%d)",	g_hif_hspi_rx_fifo, rx_fifo->size,
						g_hif_hspi_tx_fifo, tx_fifo->size);

		_hif_hspi_fifo_delete();

		return -1;
	}

	_hif_info("HSPI_FIFO: rx=(%p, %u), tx=(%p, %u)",
				g_hif_hspi_rx_fifo ? g_hif_hspi_rx_fifo->buffer : 0,
				g_hif_hspi_rx_fifo ? g_hif_hspi_rx_fifo->size : 0,
				g_hif_hspi_tx_fifo ? g_hif_hspi_tx_fifo->buffer : 0,
				g_hif_hspi_tx_fifo ? g_hif_hspi_tx_fifo->size : 0);

	return 0;
}

static int _hif_hspi_slot_setup (uint32_t rx_fifo_size, uint32_t tx_fifo_size)
{
	uint32_t fifo_size[_HSPI_QUE_NUM] = { tx_fifo_size, rx_fifo_size } ;
	uint16_t slot_num;
	uint16_t slot_size;
	union
	{
		uint32_t val[4];

		struct
		{
			char id[8];

			_hspi_slot_info_t slot[_HSPI_QUE_NUM];
		};
	} msg;
	int i;

	memcpy(msg.id, "NRC-HSPI", 8);

	msg.slot[_HSPI_TXQ].num = _HSPI_TX_SLOT_NUM;
	msg.slot[_HSPI_TXQ].size = _HSPI_TX_SLOT_SIZE;

	msg.slot[_HSPI_RXQ].num = _HSPI_RX_SLOT_NUM;
	msg.slot[_HSPI_RXQ].size = _HSPI_RX_SLOT_SIZE;

	for (i = 0 ; i < 4 ; i++)
		_hif_hspi_set_message(i, msg.val[i]);

	for (i = 0 ; i < 4 ; i++)
		msg.val[i] = _hif_hspi_get_message(i);

	_hif_info("HSPI_MSG:");
	_hif_info(" - msg[0]: 0x%08X, %c%c%c%c",
					msg.val[0], msg.id[0], msg.id[1], msg.id[2], msg.id[3]);
	_hif_info(" - msg[1]: 0x%08X, %c%c%c%c",
					msg.val[1], msg.id[4], msg.id[5], msg.id[6], msg.id[7]);
	_hif_info(" - msg[2]: 0x%08X, slot = %u x %uB (TXQ)",
					msg.val[2], msg.slot[_HSPI_TXQ].num, msg.slot[_HSPI_TXQ].size);
	_hif_info(" - msg[3]: 0x%08X, slot = %u x %uB (RXQ)",
					msg.val[3], msg.slot[_HSPI_RXQ].num, msg.slot[_HSPI_RXQ].size);
	_hif_info("");

	return 0;
}

int _hif_hspi_open (_hif_info_t *info)
{
	int slot_size[_HSPI_QUE_NUM] = { _HSPI_TX_SLOT_SIZE, _HSPI_RX_SLOT_SIZE };
	bool irq_en[_HSPI_QUE_NUM] = { false, true };
	int evid[_HSPI_QUE_NUM] = { EV_TXQUE, EV_RXQUE };
	isr_callback_t isr[_HSPI_QUE_NUM] = { _hif_hspi_txq_isr, _hif_hspi_rxq_isr };
	int i;

	if (!info || info->type != _HIF_TYPE_HSPI)
	   return -1;

	_hif_info("HSPI_OPEN: sw_id=0x%08X bd_id=0x%08X",
				info->hspi.sw_id, info->hspi.bd_id);

	if (_hif_hspi_fifo_create(info) != 0)
		return -1;

	_hif_hspi_set_sw_id(info->hspi.sw_id);
	_hif_hspi_set_board_id(info->hspi.bd_id);

	if (_hif_hspi_slot_setup(g_hif_hspi_rx_fifo->size, g_hif_hspi_tx_fifo->size) != 0)
		return -1;

	for (i = _HSPI_TXQ ; i <= _HSPI_RXQ ; i++)
	{
		_hif_hspi_queue_reset(i);
		_hif_hspi_queue_setup(i, slot_size[i], irq_en[i]);
		_hif_hspi_queue_enable(i, 0, 0, 0); /* header_on=0, byte_msb=0, bit_msb=0 */

		if (irq_en[i])
		{
			_hif_hspi_queue_enable_irq(i, 0, 1); /* trigger=0, active=1 */

			system_register_isr(evid[i], isr[i]);
			system_irq_unmask(evid[i]);
		}
	}

#if defined(NRC7394)
	_hif_hspi_pin_enable();
#endif

	_hif_hspi_ready();

/*	_hif_hspi_print_regs(); */

/*	_hif_info("HSPI_OPEN: success"); */

	return 0;
}

void _hif_hspi_close (void)
{
	int evid[_HSPI_QUE_NUM] = { EV_TXQUE, EV_RXQUE };
	int i;

	_hif_info("HSPI_CLOSE");

	for (i = _HSPI_TXQ ; i <= _HSPI_RXQ ; i++)
	{
/*		system_irq_mask(evid[i]); */

		_hif_hspi_queue_clear_irq(i);
		_hif_hspi_queue_disable_irq(i);
		_hif_hspi_queue_disable(i);
		_hif_hspi_queue_setup(i, 0, 0);
/*		_hif_hspi_queue_reset(i); */

		system_register_isr(evid[i], NULL);
	}

	_hif_hspi_fifo_delete();

#if defined(NRC7394)
	_hif_hspi_pin_disable();
#endif
}

