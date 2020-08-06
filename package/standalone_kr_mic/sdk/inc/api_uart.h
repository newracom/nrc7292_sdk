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

typedef void (*intr_handler_fn)(int vector);

/**********************************************
 * @fn void nrc_uart_set_config(NRC_UART_CONFIG *conf)
 *
 * @brief set UART configuration
 *
 * @param conf: configuration
 *
 * @return N/A
 ***********************************************/
void nrc_uart_set_config(NRC_UART_CONFIG *conf);


/**********************************************
 * @fn void nrc_uart_set_channel(int ch)
 *
 * @brief Set UART channel
 *
 * @param ch channel number
 *
 * @return true(1) or false(0)
 ***********************************************/
bool nrc_uart_set_channel(int ch);


/**********************************************
 * @fn int nrc_uart_get_intr_type(int ch)
 *
 * @brief get interrupt status
 *
 * @param ch: channel number
 *
 * @return Interrupt type \n
 *		UART_INT_NONE, \n
 *		UART_INT_ERROR, \n
 *		UART_INT_TIMEOUT, \n
 *		UART_INT_RX_DONE, \n
 *		UART_INT_TX_EMPTY
 ***********************************************/
int nrc_uart_get_intr_type(int ch);


/**********************************************
 * @fn void nrc_uart_set_interrupt(int ch, bool tx_en, bool rx_en)
 *
 * @brief set UART interrupt
 *
 * @param ch: channel number
 * @param tx_en: TX enable flag
 * @param rx_en: RX enable flag
 *
 * @return N/A
 ***********************************************/
void nrc_uart_set_interrupt(int ch, bool tx_en, bool rx_en);


/**********************************************
 * @fn void nrc_uart_intr_clr(int ch, bool tx_int, bool rx_int , bool to_int )
 *
 * @brief clear UART interrupt
 *
 * @param ch: channel number
 * @param tx_int: TX interrupt
 * @param rx_int: RX interrupt
 *
 * @param to_int TIMEOUT interrupt
 *
 * @return N/A
 ***********************************************/
void nrc_uart_intr_clr(int ch, bool tx_int, bool rx_int , bool to_int );



/**********************************************
 * @fn bool nrc_uart_put(int ch, char c)
 *
 * @brief put data
 *
 * @param ch: channel number
 * @param c: data put
 *
 * @return true(1) or false(0)
 ***********************************************/
bool nrc_uart_put(int ch, char c);


/**********************************************
 * @fn bool nrc_uart_get(int ch, char *c)
 *
 * @brief get data
 *
 * @param ch: channel number
 * @param c: data get
 *
 * @return true(1) or false(0)
 ***********************************************/
bool nrc_uart_get(int ch, char *c);


/**********************************************
 * @fn void nrc_uart_register_intr_handler(int ch, intr_handler_fn cb)
 *
 * @brief register user callback for UART input
 *
 * @param ch: channel number
 * @param cb: user handler for UART input
 *
 * @return N/A
 ***********************************************/
void nrc_uart_register_intr_handler(int ch, intr_handler_fn cb);


/**********************************************
 * @fn void nrc_uart_console_enable(void)
 *
 * @brief enable uart console (if enabled, you can use console for debugging)
 *
 * @return N/A
 ***********************************************/
void nrc_uart_console_enable(void);


/**********************************************
 * @fn void nrc_uart_printf(const char *f, ...)
 *
 * @brief print log on the console
 *
 * @param f: parameter point
 *
 * @return N/A
 ***********************************************/
void nrc_uart_printf(const char *f, ...);


/**********************************************
 * @fn void nrc_uart_vprintf(const char *f, va_list ap)
 *
 * @brief print log on the console
 *
 * @param f: parameter point
 * @param ap: parameter list
 *
 * @return N/A
 ***********************************************/
void nrc_uart_vprintf(const char *f, va_list ap);

#endif /* __NRC_API_UART_H__ */