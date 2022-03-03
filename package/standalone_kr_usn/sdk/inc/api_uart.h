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

#ifndef __NRC_API_UART_H__
#define __NRC_API_UART_H__

#include "nrc_types.h"

typedef enum  {
	NRC_UART_INT_NONE,
	NRC_UART_INT_ERROR,
	NRC_UART_INT_TIMEOUT,
	NRC_UART_INT_RX_DONE,
	NRC_UART_INT_TX_EMPTY,
	NRC_UART_INT_MAX
} NRC_UART_INT_TYPE;

/** @brief UART channel	*/
typedef enum  {
	NRC_UART_CH0 = 0,	/**< Channel 0	*/
	NRC_UART_CH1 = 1,	/**< Channel 1	*/
	NRC_UART_CH2 = 2,	/**< Channel 2	*/
	NRC_UART_CH3 = 3,	/**< Channel 3	*/
} NRC_UART_CHANNEL;

/** @brief data bit	*/
typedef enum  {
	NRC_UART_DB5 = 0,	/**< Data bit 5	*/
	NRC_UART_DB6 = 1,	/**< Data bit 6	*/
	NRC_UART_DB7 = 2,	/**< Data bit 7	*/
	NRC_UART_DB8 = 3,	/**< Data bit 8	*/
} NRC_UART_DATA_BIT;

/** @brief stop bit	*/
typedef enum  {
	NRC_UART_SB1 = 0,	/**< Stop bit 1	*/
	NRC_UART_SB2 = 1,	/**< Stop bit 2	*/
} NRC_UART_STOP_BIT;

/** @brief parity bit	*/
typedef enum {
	NRC_UART_PB_NONE = 0,	/**< Parity bit none	*/
	NRC_UART_PB_ODD  = 1,	/**< Odd Parity bit	*/
	NRC_UART_PB_EVEN = 2,	/**< Even Parity bit	*/
} NRC_UART_PARITY_BIT;

/** @brief Flow control	*/
typedef enum  {
	NRC_UART_HFC_DISABLE,	/**< Disable of flow control	*/
	NRC_UART_HFC_ENABLE,	/**< Enable of flow control	*/
} NRC_UART_HW_FLOW_CTRL;

/** @brief FIFO	*/
typedef enum  {
	NRC_UART_FIFO_DISABLE,	/**< Disable FIFO	*/
	NRC_UART_FIFO_ENABLE,	/**< Enable FIFO	*/
} NRC_UART_FIFO;

/**********************************************
 * @struct NRC_UART_CONFIG
 * @brief UART configuration parameters
 ***********************************************/
typedef struct {
	int ch;								/**< channel number */
	NRC_UART_DATA_BIT db;					/**< Data bit */
	int br;								/**< Baud rate */
	NRC_UART_STOP_BIT stop_bit;			/**< Stop bit */
	NRC_UART_PARITY_BIT parity_bit;			/**< Parity bit */
	NRC_UART_HW_FLOW_CTRL hw_flow_ctrl;	/**< HW flow control */
	NRC_UART_FIFO fifo;					/**< FIFO */
} NRC_UART_CONFIG;


/**********************************************
 * @fn nrc_err_t nrc_uart_set_config(NRC_UART_CONFIG *conf)
 *
 * @brief set UART configuration
 *
 * @param conf: configuration
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_uart_set_config(NRC_UART_CONFIG *conf);


/**********************************************
 * @fn nrc_err_t nrc_uart_set_channel(int ch)
 *
 * @brief Set UART channel
 *
 * @param ch channel number
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_uart_set_channel(int ch);


/**********************************************
 * @fn nrc_err_t nrc_uart_get_interrupt_type(int ch, NRC_UART_INT_TYPE *type)
 *
 * @brief get interrupt status
 *
 * @param ch: channel number
 *
 * @param type: interrupt type
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_uart_get_interrupt_type(int ch, NRC_UART_INT_TYPE *type);


/**********************************************
 * @fn nrc_err_t nrc_uart_set_interrupt(int ch, bool tx_en, bool rx_en)
 *
 * @brief set UART interrupt
 *
 * @param ch: channel number
 *
 * @param tx_en: TX enable flag
 *
 * @param rx_en: RX enable flag
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_uart_set_interrupt(int ch, bool tx_en, bool rx_en);


/**********************************************
 * @fn nrc_err_t nrc_uart_clear_interrupt(int ch, bool tx_int, bool rx_int , bool timeout_int )
 *
 * @brief clear UART interrupt
 *
 * @param ch: channel number
 *
 * @param tx_int: TX interrupt
 *
 * @param rx_int: RX interrupt
 *
 * @param timeout_int TIMEOUT interrupt
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_uart_clear_interrupt(int ch, bool tx_int, bool rx_int , bool timeout_int );


/**********************************************
 * @fn nrc_err_t nrc_uart_put(int ch, char data)
 *
 * @brief put a character data
 *
 * @param ch: channel number
 *
 * @param data: data put
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_uart_put(int ch, char data);


/**********************************************
 * @fn nrc_err_t nrc_uart_get(int ch, char *data)
 *
 * @brief get a character data
 *
 * @param ch: channel number
 *
 * @param data: A pointer to get data
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_uart_get(int ch, char *data);


/**********************************************
 * @fn nrc_err_t nrc_uart_register_intr_handler(int ch, intr_handler_fn cb)
 *
 * @brief register user callback for UART input
 *
 * @param ch: channel number
 *
 * @param cb: user handler for UART input
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_uart_register_interrupt_handler(int ch, intr_handler_fn cb);


/**********************************************
 * @fn nrc_err_t nrc_uart_console_enable(bool enabled)
 *
 * @brief enable uart console
 *
 * @param enabled: enable/disable console print and command handler
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_uart_console_enable(bool enabled);

#endif /* __NRC_API_UART_H__ */
