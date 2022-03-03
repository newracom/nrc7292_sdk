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

typedef enum {
	POWER_SAVE_MODEM_SLEEP_MODE,
	POWER_SAVE_DEEP_SLEEP_MODE
} POWER_SAVE_SLEEP_MODE;

typedef enum {
	POWER_SAVE_NON_TIM,
	POWER_SAVE_TIM
} POWER_SAVE_TIM_MODE;

#define WAKEUP_SOURCE_NO_SLEEP    (0x0)
#define WAKEUP_SOURCE_RTC (0x00000001L << 0)
#define WAKEUP_SOURCE_GPIO (0x00000001L << 1)

#define SET_PS_SLEEP_MODE(variable, value)	\
	do {									\
		variable &= ~(0x03); 				\
		variable |= value & 0x03; 			\
	} while(0);

#define SET_PS_TIM_MODE(variable, value)	\
	do {									\
		variable &= ~((0x01) << 2);			\
		variable |= (value & 0x01) << 2;	\
	} while(0);

/**********************************************
 * @fn nrc_err_t nrc_ps_set_sleep(POWER_SAVE_SLEEP_MODE mode, uint64_t interval, uint32_t timeout)
 *
 * @brief The function is to set the power save mode & power save protocol
 *
 * @param mode: 0(MODEM SLEEP : TBD) or 1(DEEP SLEEP)
 *
 * @param interval(ms): 0(Tim) or non-zero(Non-Tim : interval >= 1000ms)
 *
 * @param timeout(ms): only works in TIM mode with modem sleep
 *			timeout > 0: sleep alone (non-tim deepsleep) mode.
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_ps_set_sleep(POWER_SAVE_SLEEP_MODE sleep_mode, uint64_t interval, uint32_t timeout);

/**********************************************
 * @fn nrc_err_t nrc_ps_deep_sleep(uint64_t interval)
 *
 * @brief Command the device to go to deep sleep.
 *
 * @param interval: The duration (interval >= 1000ms) for sleep. The unit is ms.
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_ps_deep_sleep(uint64_t interval);

/**********************************************
 * @fn nrc_err_t nrc_ps_modem_sleep(uint64_t interval, uint32_t timeout)
 *
 * @brief Command the device WiFi to go to sleep.
 *
 * @param interval: The duration (interval >= 1000ms) for WiFi sleep. The unit is ms.
 *
 * @param timeout: wait time enter the modem sleep. The unit is ms.
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_ps_modem_sleep(uint64_t interval, uint32_t timeout);

/**********************************************
 * @fn nrc_err_t nrc_ps_tim_sleep(uint64_t timeout)
 *
 * @brief The function commands device to WiFi sleep.
 *        The WiFi wakes up if Traffic Indication Map signal received.
 *
 * @param timeout: wait time enter the sleep. The unit is ms.
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_ps_tim_sleep(uint64_t timeout);

/**********************************************
 * @fn nrc_err_t nrc_ps_set_modemsleep_stop(void)
 *
 * @brief The function is to stop the modem sleep
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_ps_set_modemsleep_stop(void);

/**********************************************
 * @fn nrc_err_t nrc_ps_set_gpio_wakeup_pin(bool check_debounce, int pin_number)
 *
 * @brief   Configure a wakeup-gpio-pin when system state is uCode or deepsleep.
 *			Call this function before deepsleep if user want to config
 * 			the wakeup-gpio-pin.
 *
 * @param check_debounce: Eliminates mechanical vibration of a switch.
 *
 * @param pin_number: Select wakeup GPIO Pin number(0~31)
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_ps_set_gpio_wakeup_pin(bool check_debounce, int pin_number);

/**********************************************
 * @fn nrc_err_t nrc_ps_set_wakeup_source(uint8_t wakeup_source)
 *
 * @brief   Configure wakeup sources when system state is deepsleep.
 *			Call this function before deepsleep if user want to config
 * 			the wakeup sources.
 *
 * @param wakeup_source: WAKEUP_SOURCE_RTC or WAKEUP_SOURCE_GPIO
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_ps_set_wakeup_source(uint8_t wakeup_source);

/**********************************************
 * @fn nrc_err_t nrc_ps_wakeup_reason(uint8_t *reason);
 *
 * @brief   Get the wakeup reason
 *
 * @param reason: WAKEUP_SOURCE_RTC / WAKEUP_SOURCE_GPIO / WAKEUP_SOURCE_NO_SLEEP
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_ps_wakeup_reason(uint8_t *reason);

#endif // __NRC_API_PS_H__
