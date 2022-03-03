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

#ifndef __NRC_API_SYSTEM_H__
#define __NRC_API_SYSTEM_H__

#include "util_trace.h"

/**********************************************
 * @fn nrc_err_t nrc_wifi_set_bdf_use(bool enable)
 *
 * @brief Set bdf_use(Board data) enbale or disable.
 *
 * @param enabled: 1(enabled) or 0(disabled)
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_wifi_set_bdf_use(bool enable);


/**********************************************
 * @fn nrc_err_t nrc_wifi_get_bdf_use(bool *enable)
 *
 * @brief Get bdf_use(Board data) enbale or disable.
 *
 * @param *enabled: A pointer for 1(enabled) or 0(disabled)
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_wifi_get_bdf_use(bool *enabled);


/**********************************************
 * @fn nrc_err_t nrc_wifi_set_cal_use(bool enable)
 *
 * @brief Set cal_use(Calibration value) enbale or disable.
 *
 * @param enabled: 1(enabled) or 0(disabled)
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_wifi_set_cal_use(bool enable);


/**********************************************
 * @fn nrc_err_t nrc_wifi_get_cal_use(bool *enable)
 *
 * @brief Get cal_use(Calibration value) enbale or disable.
 *
 * @param *enabled: A pointer for 1(enabled) or 0(disabled)
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_wifi_get_cal_use(bool *enabled);


/**********************************************
 * @fn nrc_err_t nrc_wifi_set_log_level(TRACE_TYPES type_id, TRACE_LEVEL level)
 *
 * @brief Set the log_level of input type_id
 *
 * @param type_id: defined TRACE_TYPES in util_trace.h
 *
 * @param level: 0(TL_VB) or 1(TL_INFO) or 2(TL_ERR)
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_wifi_set_log_level(TRACE_TYPES type_id, TRACE_LEVEL level);


/**********************************************
 * @fn nrc_err_t nrc_wifi_get_log_level(TRACE_TYPES type_id, TRACE_LEVEL *level)
 *
 * @brief Get the log_level of input type_id
 *
 * @param type_id: defined TRACE_TYPES in util_trace.h
 *
 * @param level: A pointer for 0(TL_VB) or 1(TL_INFO) or 2(TL_ERR)
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_wifi_get_log_level(TRACE_TYPES type_id, TRACE_LEVEL *level);

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

#endif /* __NRC_API_SYSTEM_H__ */
