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

#ifndef __NRC_API_SYSTEM_H__
#define __NRC_API_SYSTEM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "util_trace.h"

/**********************************************
 * @fn nrc_err_t nrc_get_rtc(uint64_t *rtc_time);
 *
 * @brief Read RTC time since boot
 *
 * @param rtc_time: A pointer for getting RTC (uint64)
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_get_rtc(uint64_t *rtc_time);

/**********************************************
 * @fn void nrc_reset_rtc(void);
 *
 * @brief Set RTC hardware to 0
 *
 * @return None
 ***********************************************/
void nrc_reset_rtc(void);

/**********************************************
 * @fn void nrc_sw_reset(void)
 *
 * @brief Reset SW
 ***********************************************/
void nrc_sw_reset(void);

/**********************************************
 * @fn void nrc_get_user_factory(char* data, uint16_t buf_len)
 *
 * @brief Get user factory data
 *
 * @param data: A pointer to store user factory data
 *
 * @param buf_len: buffer length (should be 512 Bytes)
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_get_user_factory(char* data, uint16_t buf_len);


/**********************************************
 * @fn void nrc_led_trx_init(int tx_gpio, int rx_gpio, int timer_period, bool invert)
 *
 * @brief Initializes the Tx/Rx LED blinking feature
 *
 * @param tx_gpio: The GPIO pin for the Tx LED
 *
 * @param rx_gpio: The GPIO pin for the Rx LED
 *
 * @param timer_period: The period for checking the status of the LED blinking
 *
 * @param invert: A boolean value to indicate whether to invert the LED blinking signal
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_led_trx_init(int tx_gpio, int rx_gpio, int timer_period, bool invert);


/**********************************************
 * @fn  int nrc_led_trx_deinit(void)
 *
 * @brief Deinitializes the Tx/Rx LED blinking feature
 *
 * @param void
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_led_trx_deinit(void);


#ifdef __cplusplus
}
#endif

#endif /* __NRC_API_SYSTEM_H__ */
