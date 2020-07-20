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

#ifndef __NRC_API_PS_H__
#define __NRC_API_PS_H__

typedef enum _power_save_sleep_mode {
	POWER_SAVE_MODEM_SLEEP_MODE,
	POWER_SAVE_DEEP_SLEEP_MODE
} power_save_sleep_mode;

#define WAKEUP_SOURCE_RTC (0x00000001L << 0)
#define WAKEUP_SOURCE_GPIO (0x00000001L << 1)

/**********************************************
 * @fn void nrc_ps_set_sleep(uint8_t mode, uint16_t interval)
 *
 * @brief The function is to choose the power save mode & power save protocol
 *
 * @param mode: 0(MODEM SLEEP : TBD), 1(DEEP SLEEP)
 *
 * @param interval: 0 (Tim), non-zero (Non-Tim : TBD) (ms)
 *
 * @return true or false
 ***********************************************/
bool nrc_ps_set_sleep(uint8_t sleep_mode, uint16_t interval);

/**********************************************
 * @fn bool nrc_ps_set_gpio_wakeup_pin(int pin_number)
 *
 * @brief   Configure a wakeup-gpio-pin when system state is uCode or deepsleep.
 *			Call this function before deepsleep if user want to config
 * 			the wakeup-gpio-pin.
 *
 * @param pin_number: Select wakeup GPIO Pin number (0~31)
 *
 * @return true or false
 ***********************************************/
bool nrc_ps_set_gpio_wakeup_pin(int pin_number);

/**********************************************
 * @fn bool nrc_ps_set_wakeup_source(uint8_t wakeup_source)
 *
 * @brief   Configure wakeup sources when system state is deepsleep.
 *			Call this function before deepsleep if user want to config
 * 			the wakeup sources.
 *
 * @param wakeup_source: WAKEUP_SOURCE_RTC or WAKEUP_SOURCE_GPIO
 *
 * @return true or false
 ***********************************************/
bool nrc_ps_set_wakeup_source(uint8_t wakeup_source);

#endif // __NRC_API_PS_H__
