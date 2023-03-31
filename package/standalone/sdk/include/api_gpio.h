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

#ifndef __NRC_API_GPIO_H__
#define __NRC_API_GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/** @brief GPIO PIN */
/* Some pins are shared with other functionalities on EVK as shown on below comments. */
/* GPIO pins are multi-purpose, use them as how NRC7292 is connected on custom boards */
#if defined (NRC7292)
typedef enum {
	GPIO_00 = 0,	/**< GPIO_00 */ /* UART2 TX on EVK - */
	GPIO_01 = 1,	/**< GPIO_01 */ /* UART2 RX on EVK */
	GPIO_02 = 2,	/**< GPIO_02 */ /* UART2 RTS on EVK */
	GPIO_03 = 3,	/**< GPIO_03 */ /* UART2 CTS on EVK */
	GPIO_04 = 4,	/**< GPIO_04 */ /* UART0 TX on EVK */
	GPIO_05 = 5,	/**< GPIO_05 */ /* UART0 RX on EVK */
	GPIO_06 = 6,	/**< GPIO_06 */ /* UART3 TX on EVK */
	GPIO_07 = 7,	/**< GPIO_07 */ /* UART3 RX on EVK */
	GPIO_08 = 8,	/**< GPIO_08 */ /* Green LED on EVK */
	GPIO_09 = 9,	/**< GPIO_09 */ /* Red LED on EVK */
	GPIO_10 = 10,	/**< GPIO_10 */
	GPIO_11 = 11,	/**< GPIO_11 */
	GPIO_12 = 12,	/**< GPIO_12 */
	GPIO_13 = 13,	/**< GPIO_13 */
	GPIO_14 = 14,	/**< GPIO_14 */
	GPIO_15 = 15,	/**< GPIO_15 */
	GPIO_16 = 16,	/**< GPIO_16 */
	GPIO_17 = 17,	/**< GPIO_17 */
} NRC_GPIO_PIN;
#elif defined(NRC7393)||defined(NRC7394)
typedef enum {
	GPIO_00 = 0,	/**< GPIO_00 */ /* Not available on EVK*/
	GPIO_01 = 1,	/**< GPIO_01 */ /* Not available on EVK*/
	GPIO_02 = 2,	/**< GPIO_02 */  /* Not available on EVK*/
	GPIO_03 = 3,	/**< GPIO_03 */ /* Not available on EVK*/
	GPIO_04 = 4,	/**< GPIO_04 */ /* Not available on EVK*/
	GPIO_05 = 5,	/**< GPIO_05 */ /* Not available on EVK*/
	GPIO_06 = 6,	/**< GPIO_06 */ /* HSPI MOSI on EVK - */
	GPIO_07 = 7,	/**< GPIO_07 */ /* HSPI_CLK on EVK */
	GPIO_08 = 8,	/**< GPIO_08 */ /* UART0 TX on EVK */
	GPIO_09 = 9,	/**< GPIO_09 */ /* UART0 RX on EVK */
	GPIO_10 = 10,	/**< GPIO_10 */ /* TMS/SWD_IO on EVK */
	GPIO_11 = 11,	/**< GPIO_11 */ /* TCK/SWD_CLK on EVK */
	GPIO_12 = 12,	/**< GPIO_12 */ /* TDO/UART1_TXD on EVK */
	GPIO_13 = 13,	/**< GPIO_13 */ /* TDI/UART1_RXD on EVK */
	GPIO_14 = 14,	/**< GPIO_14 */ /* UART1_CTS on EVK */
	GPIO_15 = 15,	/**< GPIO_15 */ /* Not available on EVK*/
	GPIO_16 = 16,	/**< GPIO_16 */ /* Not available on EVK*/
	GPIO_17 = 17,	/**< GPIO_17 */ /* ADC0 on EVK */
	GPIO_18 = 18,	/**< GPIO_18 */ /* ADC1 on EVK */
	GPIO_19 = 19,	/**< GPIO_19 */ /* MODE on EVK */
	GPIO_20 = 20,	/**< GPIO_20 */ /* UART1_RTS on EVK */
	GPIO_21 = 21,	/**< GPIO_21 */ /* Not available on EVK*/
	GPIO_22 = 22,	/**< GPIO_22 */ /* Not available on EVK*/
	GPIO_23 = 23,	/**< GPIO_23 */ /* Not available on EVK*/
	GPIO_24 = 24,	/**< GPIO_24 */ /* PA_EN on EVK */
	GPIO_25 = 25,	/**< GPIO_25 */ /* on EVK */
	GPIO_26 = 26,	/**< GPIO_26 */ /* Not available on EVK*/
	GPIO_27 = 27,	/**< GPIO_27 */ /* Not available on EVK*/
	GPIO_28 = 28,	/**< GPIO_28 */ /* HSPI_CS on EVK */
	GPIO_29 = 29,	/**< GPIO_29 */ /* HSPI_MISO on EVK */
	GPIO_30 = 30,	/**< GPIO_30 */ /* HSPI_EIRQ on EVK */
} NRC_GPIO_PIN;
#endif

/** @brief GPIO direction  */
typedef enum {
	GPIO_INPUT = 0,		/**< GPIO INPUT	*/
	GPIO_OUTPUT = 1,		/**< GPIO OUTPUT	*/
} NRC_GPIO_DIR;

/** @brief GPIO direction  */
typedef enum {
	GPIO_LEVEL_LOW = 0,		/**< GPIO LOW	*/
	GPIO_LEVEL_HIGH = 1,		/**< GPIO HIGH	*/
} NRC_GPIO_LEVEL;

/** @brief GPIO mode */
typedef enum {
	GPIO_PULL_UP = 0,	/**< GPIO Pull up	*/
	GPIO_PULL_DOWN,	/**< GPIO Pull down	*/
	GPIO_FLOATING,		/**< GPIO Floating	*/
} NRC_GPIO_MODE;

/** @brief GPIO alternative function */
typedef enum {
	GPIO_FUNC = 0,		/**< GPIO function	*/
	GPIO_NORMAL_OP = 1,	/**< GPIO Normal operation	*/
} NRC_GPIO_ALT;

/**
@struct NRC_GPIO_CONFIG
@brief GPIO configuration
 */
typedef struct {
	NRC_GPIO_PIN gpio_pin;			/**< Pin number */
	NRC_GPIO_DIR gpio_dir;			/**< Direction */
	NRC_GPIO_ALT gpio_alt;			/**< Alternative function */
	NRC_GPIO_MODE gpio_mode;		/**< Mode */
} NRC_GPIO_CONFIG;

#if defined(NRC7393)||defined(NRC7394)
typedef enum {
	TRIGGER_EDGE,
	TRIGGER_LEVEL
} nrc_gpio_trigger_t;

typedef enum {
	TRIGGER_HIGH,
	TRIGGER_LOW
} nrc_gpio_trigger_level_t;
#endif

/* supported gpio interrupt vector */
#define INT_VECTOR0 EV_EXT0
#define INT_VECTOR1 EV_EXT1

/**********************************************
 * @fn nrc_err_t nrc_gpio_config(NRC_GPIO_CONFIG *conf)
 *
 * @brief set GPIO Configuration
 *
 * @param conf configuration
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_gpio_config(NRC_GPIO_CONFIG *conf);

/**********************************************
 * @fn nrc_err_t nrc_gpio_output(uint32_t *word)
 *
 * @brief set gpio data (32bits)
 *
 * @param word: a pointer for data
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_gpio_output(uint32_t *word);

/**********************************************
 * @fn nrc_err_t nrc_gpio_outputb(int pin, int level)
 *
 * @brief set output bit for a specified pin
 *
 * @param pin: pin num
 *
 * @param level: output level
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_gpio_outputb(int pin, int level);

/**********************************************
 * @fn nrc_err_t nrc_gpio_input(uint32_t *word)
 *
 * @brief get gpio data (32bits)
 *
 * @param word: a pointer for data
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_gpio_input(uint32_t *word);

/**********************************************
 * @fn nrc_err_t nrc_gpio_inputb(int pin, NRC_GPIO_LEVEL *level)
 *
 * @brief get input bit for a specified pin
 *
 * @param pin: pin num
 *
 * @param level: 0 or 1
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_gpio_inputb(int pin, int *level);

#if defined(NRC7393)||defined(NRC7394)
/**********************************************
 * @fn nrc_err_t nrc_gpio_trigger_config(int vector, nrc_gpio_trigger_t trigger, nrc_gpio_trigger_level_t level)
 *
 * @brief Configure GPIO interrupt trigger (LEVEL/EDGE, HIGH/LOW signal)
 *
 * @param vector: interrupt vector (INT_VECTOR0 or INT_VECTOR1)
 *
 * @param trigger: TRIGGER_EDGE or TRIGGER_LEVEL
 *
 * @param level : TRIGGER_HIGH or TRIGGER_LOW
 *
 * @param debounce : true or false to enable/disable debounce logic
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_gpio_trigger_config(int vector, nrc_gpio_trigger_t trigger, nrc_gpio_trigger_level_t level, bool debounce);
#endif

/**********************************************
 * @fn nrc_err_t nrc_gpio_register_interrupt_handler(int vector, int pin, intr_handler_fn cb)
 *
 * @brief register handler for GPIO input with pin
 *
 * @param vector: interrupt vector (INT_VECTOR0 or INT_VECTOR1)
 * @param pin: pin number
 *
 * @param cb: handler
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t  nrc_gpio_register_interrupt_handler(int vector, int pin, intr_handler_fn cb);

#ifdef __cplusplus
}
#endif

#endif /* __NRC_API_GPIO_H__ */
