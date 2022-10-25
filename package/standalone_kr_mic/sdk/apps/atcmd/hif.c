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


#include "atcmd.h"


/********************************************************************************************/

static enum _HIF_TYPE g_hif_type = _HIF_TYPE_NONE;

static bool _hif_valid_type (enum _HIF_TYPE type)
{
	switch (type)
	{
		case _HIF_TYPE_HSPI:
		case _HIF_TYPE_UART:
		case _HIF_TYPE_UART_HFC:
			return true;

		default:
			return false;
	}
}

static void _hif_set_type (enum _HIF_TYPE type)
{
	g_hif_type = type;
}

enum _HIF_TYPE _hif_get_type (void)
{
	return g_hif_type;
}

/********************************************************************************************/

enum _HIF_MUTEX_ID
{
	_HIF_MUTEX_RX = 0,
	_HIF_MUTEX_TX,

	_HIF_MUTEX_NUM
};

static SemaphoreHandle_t g_hif_mutex[_HIF_MUTEX_NUM] = { NULL, NULL };

static void _hif_mutex_delete (void)
{
	int i;

	for (i = 0 ; i < _HIF_MUTEX_NUM ; i++)
	{
		if (g_hif_mutex[i])
			vSemaphoreDelete(g_hif_mutex[i]);
	}
}

static int _hif_mutex_create (void)
{
	static StaticSemaphore_t buffer[2];
	int i;

	for (i = 0 ; i < _HIF_MUTEX_NUM ; i++)
	{
		g_hif_mutex[i] = xSemaphoreCreateMutexStatic(&buffer[i]);
		if (!g_hif_mutex[i])
			break;
	}

	if (i < 2)
	{
		_hif_mutex_delete();
		return -1;
	}

	return 0;
}

static bool _hif_mutex_take (enum _HIF_MUTEX_ID id, bool isr)
{
	SemaphoreHandle_t mutex = g_hif_mutex[id];
	bool take = false;

	if (mutex)
	{
		if (!isr)
		{
			int time_ms = 10000;

			take = xSemaphoreTake(mutex, pdMS_TO_TICKS(time_ms));
		}
		else
		{
			BaseType_t xHigherPriorityTaskWoken = pdFALSE;

			take = xSemaphoreTakeFromISR(mutex, &xHigherPriorityTaskWoken);

			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
		}

		if (!take)
			_hif_error("fail (id=%d, isr=%d)\n", id, isr);
	}

	return take;
}

static bool _hif_mutex_give (enum _HIF_MUTEX_ID id, bool isr)
{
	SemaphoreHandle_t mutex = g_hif_mutex[id];
	bool give = false;

	if (mutex)
	{
		if (!isr)
			give = xSemaphoreGive(mutex);
		else
		{
			BaseType_t xHigherPriorityTaskWoken = pdFALSE;

			give = xSemaphoreGiveFromISR(mutex, &xHigherPriorityTaskWoken);

			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
		}

		if (!give)
			_hif_error("fail (id=%d, isr=%d)\n", id, isr);
	}

	return give;
}

/********************************************************************************************/

#define _hif_read_lock(isr)			_hif_mutex_take(_HIF_MUTEX_RX, isr)
#define _hif_read_unlock(isr)		_hif_mutex_give(_HIF_MUTEX_RX, isr)

#define _hif_write_lock(isr)		_hif_mutex_take(_HIF_MUTEX_TX, isr)
#define _hif_write_unlock(isr)		_hif_mutex_give(_HIF_MUTEX_TX, isr)

static int __hif_read (char *buf, int len, bool isr)
{
	int rd_size = 0;

	if (_hif_read_lock(isr))
	{
		switch (_hif_get_type())
		{
			case _HIF_TYPE_HSPI:
#if defined(CONFIG_ATCMD_HSPI) || defined(CONFIG_ATCMD_HSPI_HOST)
				rd_size = _hif_hspi_read(buf, len);
#endif
				break;

			case _HIF_TYPE_UART:
			case _HIF_TYPE_UART_HFC:
#if defined(CONFIG_ATCMD_UART) || defined(CONFIG_ATCMD_UART_HFC) || \
	defined(CONFIG_ATCMD_UART_HOST) || defined(CONFIG_ATCMD_UART_HFC_HOST)
				rd_size = _hif_uart_read(buf, len);
#endif
				break;

			default:
				break;
		}

		_hif_read_unlock(isr);
	}

	return rd_size;
}

static int __hif_write (char *buf, int len, bool isr)
{
	int wr_size = 0;

	if (_hif_write_lock(isr))
	{
		switch (_hif_get_type())
		{
			case _HIF_TYPE_HSPI:
#if defined(CONFIG_ATCMD_HSPI) || defined(CONFIG_ATCMD_HSPI_HOST)
				wr_size = _hif_hspi_write(buf, len);
#endif
				break;

			case _HIF_TYPE_UART:
			case _HIF_TYPE_UART_HFC:
#if defined(CONFIG_ATCMD_UART) || defined(CONFIG_ATCMD_UART_HFC) || \
	defined(CONFIG_ATCMD_UART_HOST) || defined(CONFIG_ATCMD_UART_HFC_HOST)
				wr_size = _hif_uart_write(buf, len);
#endif
				break;

			default:
				break;
		}

		_hif_write_unlock(isr);
	}

	return wr_size;
}

int _hif_read (char *buf, int len)
{
	return __hif_read(buf, len, false);
}

int _hif_write (char *buf, int len)
{
	return __hif_write(buf, len, false);
}

int _hif_read_isr (char *buf, int len)
{
	return __hif_read(buf, len, true);
}

int _hif_write_isr (char *buf, int len)
{
	return __hif_write(buf, len, true);
}

/********************************************************************************************/

static TaskHandle_t g_hif_rx_task = NULL;

static void _hif_rx_task (void *pvParameters)
{
	_hif_rx_params_t *params = (_hif_rx_params_t *)pvParameters;
	char *buf = params->buf.addr;
	int len = params->buf.size;
	int ret;

	while (1)
	{
		atcmd_trace_task_loop(ATCMD_TRACE_TASK_HIF_RX);

		ret = _hif_read(buf, len);

		if (ret > 0 && params->cb)
			params->cb(buf, ret);
		else
			_hif_rx_suspend(100);
	}
}

static int _hif_rx_task_create (_hif_rx_params_t *params)
{
	if (!params || !params->buf.addr || !params->buf.size || !params->cb)
		return -1;

	if (!g_hif_rx_task)
	{
		static _hif_rx_params_t _params;
		BaseType_t ret;

		memcpy(&_params, params, sizeof(_hif_rx_params_t));

		ret = xTaskCreate(_hif_rx_task, "_hif_rx",
						CONFIG_HIF_RX_TASK_STACK_SIZE,
						&_params,
						CONFIG_HIF_RX_TASK_PRIORITY,
						&g_hif_rx_task);

		if (ret == pdPASS && g_hif_rx_task)
			return 0;
	}

	return -1;
}

static void _hif_rx_task_delete (void)
{
	if (g_hif_rx_task)
	{
		vTaskDelete(g_hif_rx_task);
		g_hif_rx_task = NULL;
	}
}

int _hif_rx_suspend (int time)
{
	if (g_hif_rx_task)
	{
		if (time <= 0)
		{
			_hif_error("invalid time (%d)\n", time);
			return -1;
		}

		atcmd_trace_task_suspend(ATCMD_TRACE_TASK_HIF_RX);

		return ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(time));
	}

	return -1;
}

void _hif_rx_resume (void)
{
	if (g_hif_rx_task)
	{
		atcmd_trace_task_resume(ATCMD_TRACE_TASK_HIF_RX);

		xTaskNotifyGive(g_hif_rx_task);
	}
}

void _hif_rx_resume_isr (void)
{
	if (g_hif_rx_task)
	{
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;

		atcmd_trace_task_resume(ATCMD_TRACE_TASK_HIF_RX);

		vTaskNotifyGiveFromISR(g_hif_rx_task, &xHigherPriorityTaskWoken);

		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

/********************************************************************************************/

/* #define CONFIG_HIF_FIFO_STATIC */

#ifdef CONFIG_HIF_FIFO_STATIC
static char g_nrc_hif_trx_fifo[CONFIG_HIF_RXFIFO_SIZE + CONFIG_HIF_TXFIFO_SIZE];
#endif

int _hif_open (_hif_info_t *info)
{
	int (*open[_HIF_TYPE_NUM])(_hif_info_t *info) =
	{
#if defined(CONFIG_ATCMD_HSPI) || defined(CONFIG_ATCMD_HSPI_HOST)
		[_HIF_TYPE_HSPI] = _hif_hspi_open,
#endif
#if defined(CONFIG_ATCMD_UART) || defined(CONFIG_ATCMD_UART_HOST)
		[_HIF_TYPE_UART] = _hif_uart_open,
#endif
#if defined(CONFIG_ATCMD_UART_HFC) || defined(CONFIG_ATCMD_UART_HFC_HOST)
		[_HIF_TYPE_UART_HFC] = _hif_uart_open
#endif
	};

	if (!info)
		return -1;

/*	_hif_debug("Open: type=%d\n", info->type); */

	if (!_hif_valid_type(info->type))
		return -1;

	if (_hif_get_type() != _HIF_TYPE_NONE)
		return -1;

	if (_hif_mutex_create() != 0)
		return -1;

#ifdef CONFIG_HIF_FIFO_STATIC
	info->rx_fifo.addr = g_nrc_hif_trx_fifo;
#else
	info->rx_fifo.addr = NULL;
#endif
	info->rx_fifo.size = CONFIG_HIF_RX_FIFO_SIZE;

#ifdef CONFIG_HIF_FIFO_STATIC
	info->tx_fifo.addr = &g_nrc_hif_trx_fifo[CONFIG_HIF_RX_FIFO_SIZE];
#else
	info->tx_fifo.addr = NULL;
#endif
	info->tx_fifo.size = CONFIG_HIF_TX_FIFO_SIZE;

	if (open[info->type](info) != 0)
	{
		_hif_close();
		return -1;
	}

	if (_hif_rx_task_create(&info->rx_params) != 0)
		return -1;

	_hif_set_type(info->type);

/*	_hif_debug("Open: success\n"); */

	return 0;
}

void _hif_close (void)
{
	void (*close[_HIF_TYPE_NUM])(void) =
	{
#if defined(CONFIG_ATCMD_HSPI) || defined(CONFIG_ATCMD_HSPI_HOST)
		[_HIF_TYPE_HSPI] = _hif_hspi_close,
#endif
#if defined(CONFIG_ATCMD_UART) || defined(CONFIG_ATCMD_UART_HOST)
		[_HIF_TYPE_UART] = _hif_uart_close,
#endif
#if defined(CONFIG_ATCMD_UART_HFC) || defined(CONFIG_ATCMD_UART_HFC_HOST)
		[_HIF_TYPE_UART_HFC] = _hif_uart_close
#endif
	};

	if (_hif_get_type() == _HIF_TYPE_NONE)
		return;

/*	_hif_debug("Close\n"); */

	_hif_rx_task_delete();

	close[_hif_get_type()]();

	_hif_mutex_delete();

	_hif_set_type(_HIF_TYPE_NONE);
}

