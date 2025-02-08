/*
 * MIT License
 *
 * Copyright (c) 2024 Newracom, Inc.
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

#ifndef __API_UART_DMA_H__
#define __API_UART_DMA_H__
/*****************************************************************************/

#include "nrc_sdk.h"
#include "api_dma.h"

typedef struct
{
	int channel;
	int baudrate;

	enum uart_data_bit data_bits;
	enum uart_stop_bit stop_bits;
	enum uart_parity_bit parity;
	enum uart_hardware_flow_control hfc;
} uart_dma_t;

typedef struct
{
	char *addr;
	int size;
} uart_dma_buf_t;

typedef void (*uart_dma_rxcb_t) (char *buf, int len);

typedef struct
{
	uart_dma_buf_t buf;
	uart_dma_rxcb_t cb;
} uart_dma_rx_params_t; // used by uart_dma_rx_task().

typedef struct
{
	uart_dma_t uart;

	uart_dma_buf_t rx_fifo;
	uart_dma_buf_t tx_fifo; // Not used.

	uart_dma_rx_params_t rx_params;
} uart_dma_info_t;

/*****************************************************************************/

/**********************************************
 * @fn nrc_uart_dma_open (uart_dma_info_t *info)
 *
 * @brief Initialize UART and DMA setting.
 *        uart_dma_info_t should be initialize and passed to this function.
 *        i.e.
 *        // uart channel to be used.
 *        info.uart.channel = NRC_UART_CH2;
 *        // uart baud rate.
 *        info.uart.baudrate = 9600;
 *        // data bit setting for uart communication.
 *        info.uart.data_bits = UART_DB8;
 *        // stop bit setting for uart communication.
 *        info.uart.stop_bits = UART_SB1;
 *        // parity bit for uart communication.
 *        info.uart.parity = UART_PB_NONE;
 *        // whether to enable/disable hardware flow control.
 *        info.uart.hfc = UART_HFC_DISABLE;
 *        // Buffer used for data received.
 *        info.rx_params.buf.addr = uart_rx_buffer;
 *        // size of buffer to be used for RX data.
 *        info.rx_params.buf.size = RXBUF_SIZE;
 *        // callback function when there is RX data available.
 *        info.rx_params.cb = uart_data_receive;
 *
 * @param info : uart setting to be used.
 *
 * @return 0 if successful, -1 otherwise.
 ***********************************************/
int nrc_uart_dma_open (uart_dma_info_t *info);

/**********************************************
 * @fn nrc_uart_dma_close (void)
 *
 * @brief Close uart and clean up data used for uart/dma operations.
 *
 * @param NONE
 *
 * @return NONE
 ***********************************************/
void nrc_uart_dma_close (void);

/**********************************************
 * @fn nrc_uart_dma_change (uart_dma_t *uart);
 *
 * @brief Change UART setting if necessary.
 *
 * @param uart : new UART setting to be used.
 *
 * @return 0 if successful, -1 otherwise.
 ***********************************************/
int nrc_uart_dma_change (uart_dma_t *uart);

/**********************************************
 * @fn nrc_uart_dma_read (char *buf, int len)
 *
 * @brief Read UART data received using DMA subsystem.
 *
 * @param buf : buffer to collect received data.
 * @param len : size of buffer.
 *
 * @return size of UART data received.
 ***********************************************/
int nrc_uart_dma_read (char *buf, int len);

/**********************************************
 * @fn nrc_uart_write (char *buf, int len)
 *
 * @brief Write data to UART. DMA isn't used for TX. (only RX)
 *
 * @param buf : data to be trasnmitted through UART.
 * @param len : size of data to be transmissted.
 *
 * @return size of UART data transmitted.
 ***********************************************/
int nrc_uart_write (char *buf, int len);

/****************************************************************************/
#endif /* #ifndef __API_UART_DMA_H__ */

