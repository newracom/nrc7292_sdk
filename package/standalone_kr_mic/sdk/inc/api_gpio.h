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

#ifndef __NRC_API_GPIO_H__
#define __NRC_API_GPIO_H__


/** @brief GPIO PIN */
#if defined (NRC7292)
typedef enum {
	GPIO_00 = 0,	/**< GPIO_00 */
	GPIO_01 = 1,	/**< GPIO_01 */
	GPIO_02 = 2,	/**< GPIO_02 */
	GPIO_03 = 3,	/**< GPIO_03 */
	GPIO_08 = 8,	/**< GPIO_08 */
	GPIO_09 = 9,	/**< GPIO_09 */
	GPIO_10 = 10,	/**< GPIO_10 */
	GPIO_11 = 11,	/**< GPIO_11 */
	GPIO_12 = 12,	/**< GPIO_12 */
	GPIO_13 = 13,	/**< GPIO_13 */
	GPIO_14 = 14,	/**< GPIO_14 */
	GPIO_15 = 15,	/**< GPIO_15 */
	GPIO_16 = 16,	/**< GPIO_16 */
	GPIO_17 = 17,	/**< GPIO_17 */
} NRC_GPIO_PIN;
#else
typedef enum {
	GPIO_00 = 27,	/**< PD_27_GPIO_00 */
	GPIO_01 = 26,	/**< PD_26_GPIO_01 */
	GPIO_02 = 21,	/**< PD_21_GPIO_02 */
	GPIO_03 = 20,	/**< PD_20_GPIO_03 */
	GPIO_04 = 30,	/**< PD_30_GPIO_04 */
	GPIO_05 = 25,	/**< PD_25_GPIO_05 */
	GPIO_06 = 19,	/**< PD_19_GPIO_06 */
	GPIO_07 = 18,	/**< PD_18_GPIO_07 */
	GPIO_08 = 10,	/**< PD_10_GPIO_08 */
	GPIO_09 = 9,	/**< PD_09_GPIO_09 */
	GPIO_10 = 31,	/**< PD_31_GPIO_10 */
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
	GPIO_NOMAL_OP = 1,	/**< GPIO Normal operation	*/
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

/**********************************************
 * @fn nrc_err_t nrc_gpio_register_interrupt_handler(int pin, intr_handler_fn cb)
 *
 * @brief register handler for GPIO input with pin
 *
 * @param pin: pin number
 *
 * @param cb: handler
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t  nrc_gpio_register_interrupt_handler(int pin, intr_handler_fn cb);

#endif /* __NRC_API_GPIO_H__ */
