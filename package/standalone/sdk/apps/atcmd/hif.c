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


#include "nrc_sdk.h"
#include "hif.h"

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

void _hif_set_type (enum _HIF_TYPE type)
{
	g_hif_type = type;
}

enum _HIF_TYPE _hif_get_type (void)
{
	return g_hif_type;
}

/********************************************************************************************/

int _hif_read (char *buf, int len)
{
	int rd_size = 0;

	switch (_hif_get_type())
	{
		case _HIF_TYPE_HSPI:
#if defined(CONFIG_ATCMD_HSPI)
			rd_size = _hif_hspi_read(buf, len);
#endif
			break;

		case _HIF_TYPE_UART:
		case _HIF_TYPE_UART_HFC:
#if defined(CONFIG_ATCMD_UART) || defined(CONFIG_ATCMD_UART_HFC)
			rd_size = _hif_uart_read(buf, len);
#endif
			break;

		default:
			break;
	}

	return rd_size;
}

int _hif_write (char *buf, int len)
{
	int wr_size = 0;

	switch (_hif_get_type())
	{
		case _HIF_TYPE_HSPI:
#if defined(CONFIG_ATCMD_HSPI)
			wr_size = _hif_hspi_write(buf, len);
#endif
			break;

		case _HIF_TYPE_UART:
		case _HIF_TYPE_UART_HFC:
#if defined(CONFIG_ATCMD_UART) || defined(CONFIG_ATCMD_UART_HFC)
			wr_size = _hif_uart_write(buf, len);
#endif
			break;

		default:
			break;
	}

	return wr_size;
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

		ret = xTaskCreate(_hif_rx_task, "atcmd_hif_rx",
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

		return ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(time));
	}

	return -1;
}

void _hif_rx_resume (void)
{
	if (g_hif_rx_task)
	{
		xTaskNotifyGive(g_hif_rx_task);
	}
}

void _hif_rx_resume_isr (void)
{
	if (g_hif_rx_task)
	{
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;

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
#if defined(CONFIG_ATCMD_HSPI)
		[_HIF_TYPE_HSPI] = _hif_hspi_open,
#endif
#if defined(CONFIG_ATCMD_UART)
		[_HIF_TYPE_UART] = _hif_uart_open,
#endif
#if defined(CONFIG_ATCMD_UART_HFC)
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
#if defined(CONFIG_ATCMD_HSPI)
		[_HIF_TYPE_HSPI] = _hif_hspi_close,
#endif
#if defined(CONFIG_ATCMD_UART)
		[_HIF_TYPE_UART] = _hif_uart_close,
#endif
#if defined(CONFIG_ATCMD_UART_HFC)
		[_HIF_TYPE_UART_HFC] = _hif_uart_close
#endif
	};

	if (_hif_get_type() == _HIF_TYPE_NONE)
		return;

/*	_hif_debug("Close\n"); */

	_hif_rx_task_delete();

	close[_hif_get_type()]();

	_hif_set_type(_HIF_TYPE_NONE);
}

