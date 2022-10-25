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

#ifndef __NRC_HIF_H__
#define __NRC_HIF_H__
/**********************************************************************************************/

#include "nrc_sdk.h"

#if CONFIG_ATCMD_TASK_PRIORITY == 0
#undef CONFIG_ATCMD_TASK_PRIORITY
#define CONFIG_ATCMD_TASK_PRIORITY		NRC_TASK_PRIORITY
#endif

/**********************************************************************************************/

#define CONFIG_HIF_RX_TASK_PRIORITY			CONFIG_ATCMD_TASK_PRIORITY
#define CONFIG_HIF_RX_TASK_STACK_SIZE		((4 * 1024) / sizeof(StackType_t))

/**********************************************************************************************/

#if defined(NRC7292)
#define CONFIG_HIF_FIFO_MEM_SIZE		(32 * 1024)
#else
#define CONFIG_HIF_FIFO_MEM_SIZE		(4 * 1024)
#endif

/*
 *	RX: target from host
 *	TX: target to host
 */

/* HSPI */
#define CONFIG_HIF_HSPI_SLOT_SIZE		512 /* Don't change */
#define CONFIG_HIF_HSPI_SLOT_NUM		((CONFIG_HIF_FIFO_MEM_SIZE / CONFIG_HIF_HSPI_SLOT_SIZE) / 2)

/* HSUART */
#define CONFIG_HIF_UART_SLOT_SIZE		128	/* Don't change */

#define CONFIG_HIF_UART_RX_SLOT_NUM		(CONFIG_HIF_FIFO_MEM_SIZE / CONFIG_HIF_UART_SLOT_SIZE)
#define CONFIG_HIF_UART_TX_SLOT_NUM		0

#if defined(CONFIG_ATCMD_HSPI) || defined(CONFIG_ATCMD_HSPI_HOST)
#define CONFIG_HIF_RX_FIFO_SIZE			(CONFIG_HIF_HSPI_SLOT_SIZE * CONFIG_HIF_HSPI_SLOT_NUM)
#define CONFIG_HIF_TX_FIFO_SIZE			(CONFIG_HIF_HSPI_SLOT_SIZE * CONFIG_HIF_HSPI_SLOT_NUM)
#elif defined(CONFIG_ATCMD_UART) || defined(CONFIG_ATCMD_UART_HFC) || \
      defined(CONFIG_ATCMD_UART_HOST) || defined(CONFIG_ATCMD_UART_HFC_HOST)
#define CONFIG_HIF_RX_FIFO_SIZE			(CONFIG_HIF_UART_SLOT_SIZE * CONFIG_HIF_UART_RX_SLOT_NUM)
#define CONFIG_HIF_TX_FIFO_SIZE			(CONFIG_HIF_UART_SLOT_SIZE * CONFIG_HIF_UART_TX_SLOT_NUM)
#endif

/**********************************************************************************************/

#define _hif_malloc						pvPortMalloc
#define _hif_free						vPortFree
#define _hif_printf						hal_uart_printf

#define _hif_log(fmt, ...)				_hif_printf("[ATHIF] " fmt, ##__VA_ARGS__)
#define _hif_info(fmt, ...)				_hif_log(fmt, ##__VA_ARGS__)
#define _hif_error(fmt, ...)			_hif_log("%s: " fmt, __func__, ##__VA_ARGS__)
#define _hif_debug(fmt, ...)			_hif_log(fmt, ##__VA_ARGS__)
#define _hif_trace()					_hif_log("%s::%d\r\n", __func__, __LINE__)

/**********************************************************************************************/

enum _HIF_TYPE
{
	_HIF_TYPE_NONE = -1,
	_HIF_TYPE_HSPI = 0,
	_HIF_TYPE_UART,
	_HIF_TYPE_UART_HFC,

	_HIF_TYPE_NUM
};

typedef struct
{
#define CONFIG_HIF_FIFO_MUTEX

	uint32_t size;
	uint32_t cnt;

	uint32_t push_idx;
	uint32_t pop_idx;

	bool static_buffer;
	char *buffer;
	char *buffer_end;

#ifdef CONFIG_HIF_FIFO_MUTEX
	struct
	{
		SemaphoreHandle_t handle;
		StaticSemaphore_t buffer;
	} mutex;
#endif
} _hif_fifo_t;

typedef struct
{
	int channel;
	int baudrate;

	enum uart_data_bit data_bits;
	enum uart_stop_bit stop_bits;
	enum uart_parity_bit parity;
	enum uart_hardware_flow_control hfc;
} _hif_uart_t;

typedef struct
{
	uint32_t sw_id;
	uint32_t bd_id;
} _hif_hspi_t;

typedef struct
{
	char *addr;
	int size;
} _hif_buf_t;

typedef void (*_hif_rxcb_t) (char *buf, int len);

typedef struct
{
	_hif_buf_t buf;
	_hif_rxcb_t cb;
} _hif_rx_params_t; // used by _hif_rx_task().

typedef struct
{
	enum _HIF_TYPE type;

	union
	{
		_hif_uart_t uart;
		_hif_hspi_t hspi;
	};

	_hif_buf_t rx_fifo;
	_hif_buf_t tx_fifo;

	_hif_rx_params_t rx_params;
} _hif_info_t;

/**********************************************************************************************/

#include "hif_dma.h"

#ifdef CONFIG_HIF_FIFO_MUTEX
extern bool _hif_fifo_mutex_take (_hif_fifo_t *fifo);
extern bool _hif_fifo_mutex_take_isr (_hif_fifo_t *fifo);
extern bool _hif_fifo_mutex_give (_hif_fifo_t *fifo);
extern bool _hif_fifo_mutex_give_isr (_hif_fifo_t *fifo);
#endif
extern _hif_fifo_t *_hif_fifo_create (char *buffer, int size, bool mutex);
extern void _hif_fifo_delete (_hif_fifo_t *fifo);
extern void _hif_fifo_reset (_hif_fifo_t *fifo);
extern char *_hif_fifo_push_addr (_hif_fifo_t *fifo, int offset);
extern char *_hif_fifo_pop_addr (_hif_fifo_t *fifo, int offset);
extern int _hif_fifo_free_size (_hif_fifo_t *fifo);
extern int _hif_fifo_fill_size (_hif_fifo_t *fifo);
extern bool _hif_fifo_empty (_hif_fifo_t *fifo);
extern bool _hif_fifo_full (_hif_fifo_t *fifo);
extern char _hif_fifo_getc (_hif_fifo_t *fifo);
extern void _hif_fifo_putc (_hif_fifo_t *fifo, char c);
extern int _hif_fifo_read (_hif_fifo_t *fifo, char *buf, int len);
extern int _hif_fifo_write (_hif_fifo_t *fifo, char *buf, int len);

extern int _hif_uart_open (_hif_info_t *info);
extern void _hif_uart_close (void);
extern int _hif_uart_change (_hif_uart_t *uart);
extern int _hif_uart_read (char *buf, int len);
extern int _hif_uart_write (char *buf, int len);
extern void _hif_uart_get_info (_hif_uart_t *info);

extern int _hif_hspi_open (_hif_info_t *info);
extern void _hif_hspi_close (void);
extern int _hif_hspi_read (char *buf, int len);
extern int _hif_hspi_write (char *buf, int len);

extern enum _HIF_TYPE _hif_get_type (void);
extern int _hif_open (_hif_info_t *info);
extern void _hif_close (void);
extern int _hif_read (char *buf, int len);
extern int _hif_write (char *buf, int len);
extern int _hif_read_isr (char *buf, int len);
extern int _hif_write_isr (char *buf, int len);
extern int _hif_rx_suspend (int time);
extern void _hif_rx_resume (void);
extern void _hif_rx_resume_isr (void);

/**********************************************************************************************/
#endif /* #ifndef __NRC_HIF_H__ */

