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

#ifndef __NRC_API_PS_H__
#define __NRC_API_PS_H__

#ifdef __cplusplus
extern "C" {
#endif

#define WAKEUP_SOURCE_RTC (0x00000001L << 0)
#define WAKEUP_SOURCE_GPIO (0x00000001L << 1)

/* List of Wake up reasons, use nrc_ps_wakeup_reason() to retrieve wakeup reason */
enum nrc_ps_wakeup_reason {
	NRC_WAKEUP_REASON_COLDBOOT,
	NRC_WAKEUP_REASON_RTC,
	NRC_WAKEUP_REASON_GPIO,
	NRC_WAKEUP_REASON_TIM,
	NRC_WAKEUP_REASON_TIM_TIMER,
	NRC_WAKEUP_REASON_NOT_SUPPORTED
};

/**********************************************
 * @fn nrc_err_t nrc_ps_deep_sleep(uint64_t sleep_ms)
 *
 * @brief Command the device to go to NONTIM mode deep sleep.
 * If used after a previous WiFi pairing has been completed, the device will utilize
 * the saved WiFi connection information in retention memory for faster pairing recovery.
 * Note that the sleep_ms parameter may be overridden by the BSS MAX IDLE set to AP,
 * with the default value being 3 minutes. The value of bss_max_idle parameter may be set
 * to override this default value when the nrc host driver is loaded in AP.
 *
 * @param sleep_ms: duration for deep sleep. The unit is ms. (>= 1000ms)
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_ps_deep_sleep(uint64_t sleep_ms);

/**********************************************
 * @fn nrc_err_t nrc_ps_deepsleep(uint64_t sleep_ms)
 *
 * @brief Command the device to go to NONTIM deep sleep.
 * Unlike nrc_ps_deep_sleep, it will not save pairing information,
 * potentially leading to longer WiFi reconnection time.
 * Additionally, this API will not override the sleep duration specified by the sleep_ms parameter.
 *
 * @param sleep_ms: duration for deep sleep. The unit is ms. (>= 1000ms)
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_ps_sleep_alone(uint64_t sleep_ms);

/**********************************************
 * @fn nrc_ps_wifi_tim_deep_sleep(uint32_t idle_timout_ms, uint32_t sleep_ms)
 *
 * @brief The function commands device to WiFi TIM sleep.
 *        The WiFi wakes up if Traffic Indication Map signal received or sleep duration expired.
 *        If sleep_ms is set to 0, the device will wakeup only for TIM traffic.
 *
 * @param idle_timout_ms: wait time before entering the modem sleep. The unit is ms.  (0 <= time < 10000ms)
 *
 * @param sleep_ms: duration for deep sleep. The unit is ms. (0(not use) or time >= 1000ms)
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 *
 * @NOTE tim-mode deep sleep consumpts more power than nontim by checking beacons periodically during deep sleep
 ***********************************************/
nrc_err_t nrc_ps_wifi_tim_deep_sleep(uint32_t idle_timout_ms, uint32_t sleep_ms);

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

/**********************************************
 * @fn void nrc_ps_set_gpio_mask(uint32_t mask)
 *
 * @brief   Set the gpio direction mask in deep sleep
 *
 * @param bitmask: set bitmask of GPIO direction, as bits 0-31 (input:0, output:1)
 *
 * @return N/A
 ***********************************************/
void nrc_ps_set_gpio_direction(uint32_t bitmask);

/**********************************************
 * @fn void nrc_ps_set_gpio_out(uint32_t mask)
 *
 * @brief   Set the gpio out mask in deep sleep
 *
 * @param bitmask: set bitmask of GPIO out value, as bits 0-31 (low:0, high:1)
 *
 * @return N/A
 ***********************************************/
void nrc_ps_set_gpio_out(uint32_t bitmask);

/**********************************************
 * @fn void nrc_ps_set_gpio_pullup(uint32_t mask)
 *
 * @brief   Set the gpio pullup mask in deep sleep
 *
 * @param bitmask: set bitmask of GPIO pullup value, as bits 0-31 (pulldown:0, pullup:1)
 *
 * @return N/A
 ***********************************************/
void nrc_ps_set_gpio_pullup(uint32_t bitmask);

/***********************************************
 * @fn nrc_err_t nrc_ps_add_schedule(uint32_t timeout, bool net_init, scheduled_callback func);
 *
 * @brief    Add schedules to the deep sleep scheduler (NON TIM mode)
 *           timeout, whether to enable wifi, and callback funtion to execute when
 *           the scheduled time is reached.
 *           Current implementation can accept up to 4 individual schedules.
 *           Each individual schedule should have at least one minute apart in timeout.
 *           When adding schedule the callback should be able to finish in the time window.
 *
 * @param timeout: value in msec for this schedule.
 * @param net_init: whether callback will require wifi connection
 * @param func: scheduled_callback function pointer defined as
 *              void (*scheduled_callback)()
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_ps_add_schedule(uint32_t timeout, bool net_init, scheduled_callback func);

/**********************************************
 * @fn  nrc_err_t  nrc_ps_add_gpio_callback(bool net_init, schedule_callback func)
 *
 * @brief Add gpio exception callback to handle gpio interrupted wake up.
 *        This information will be added into retention memory and
 *        processed if gpio interrupt occurs.
 *        If net_init is set to true, then wifi and network will be initialized.
 *
 * @param net_init: whether callback will require wifi connection
 * @param func: scheduled_callback funtion pointer defined as
 *              void (*scheduled_callback)()
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t  nrc_ps_add_gpio_callback(bool net_init, scheduled_callback func);

/**********************************************
 * @fn nrc_err_t nrc_ps_start_schedule();
 *
 * @brief    Start the scheduled deep sleep configured using nrc_ps_add_schedule.
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_ps_start_schedule();

/**********************************************
 * @fn nrc_err_t nrc_ps_resume_deep_sleep()
 *
 * @brief Command the device to go to deep sleep for remaining scheduled time.
 *        This function is used to sleep after none-scheduled wakeup such as GPIO interrupt.
 *
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_ps_resume_deep_sleep();

#ifdef __cplusplus
}
#endif

#endif // __NRC_API_PS_H__
