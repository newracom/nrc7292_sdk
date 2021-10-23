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

#ifndef __NRC_API_PWM_H__
#define __NRC_API_PWM_H__


/** @brief channel number of PWM */
typedef enum {
	PWM_CH0,        /**< PWM channel 0 */
	PWM_CH1,        /**< PWM channel 1 */
	PWM_CH2,        /**< PWM channel 2 */
	PWM_CH3,        /**< PWM channel 3 */
#if !defined( NRC7292 )
	PWM_CH4,        /**< PWM channel 4 */
	PWM_CH5,        /**< PWM channel 5 */
	PWM_CH6,        /**< PWM channel 6 */
	PWM_CH7,        /**< PWM channel 7 */
#endif
}PWM_CH;

/**********************************************
 * @fn nrc_err_t nrc_pwm_hw_init(uint8_t ch, uint8_t gpio_num, uint8_t use_high_clk)
 *
 * @brief Initialize PWM controller
 *
 * @param ch: PWM channel (0~3)
 *
 * @param gpio_num: GPIO number assigned for PWM (8~11)
 *
 * @param use_high_clk: If 0, then the pulse duration for 1-bit in each pattern is about 20.8us.
 *                        Otherwise, about 10.4us.
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_pwm_hw_init(uint8_t ch, uint8_t gpio_num, uint8_t use_high_clk);


/**********************************************
 * @fn nrc_err_t nrc_pwm_set_config(uint8_t ch, uint32_t pattern1, uint32_t pattern2, uint32_t pattern3, uint32_t pattern4)
 *
 * @brief Set configuration parameters of PWM. one duty cycle consists of 4 pulse patterns(total 128-bit).
 *         it starts with the MSB of pattern1 and ends with the LSB of pattern4.
 *
 * @param ch: PWM channel
 *
 * @param pattern1: 1st pulse pattern
 *
 * @param pattern2: 2nd pulse pattern
 *
 * @param pattern3: 3rd pulse pattern
 *
 * @param pattern4: 4th pulse pattern
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_pwm_set_config(uint8_t ch, uint32_t pattern1, uint32_t pattern2, uint32_t pattern3, uint32_t pattern4);


/**********************************************
 * @fn nrc_err_t nrc_pwm_set_enable(uint32_t ch, bool enable)
 *
 * @brief Enable / disable the PWM.
 *
 * @param ch: PWM channel
 *
 * @param enable: true(enable) or false(diable)
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_pwm_set_enable(uint32_t ch, bool enable);

#endif //__NRC_API_PWM_H__
