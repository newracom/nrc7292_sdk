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

#ifndef __NRC_API_TIMER_H__
#define __NRC_API_TIMER_H__


/** @brief invalid timer id */
#define INVALID_TIMER_ID        0xFF

/** @brief typedef timer_id */
typedef char timer_id;

/**********************************************
 * @brief Function prototype for timer interrupt handler callback function. It is called by SDK
 * when the timer has been expired
 *
 * @param id: timer_id returned from nrc_timer_create()
 ***********************************************/
typedef void (*timer_callback)(const timer_id id);

/**********************************************
 * @fn void nrc_timers_init(void)
 *
 * @brief Initialize timers
 *
 * @return N/A
 ***********************************************/
void nrc_timers_init(void);

/**********************************************
 * @fn timer_id nrc_timer_create(uint64_t time, bool repeat, timer_callback handler)
 *
 * @brief Create timer with the time duration in microsecond and starts
 *
 * @param time: time duration value in microsecond
 *
 * @param repeat: true(repeat) or false(one shot)
 *
 * @param timer_callback: callback handler function when the timer expired
 *
 * @return timer_id: this id can be used to stop the timer.
 ***********************************************/
timer_id nrc_timer_create(uint64_t time, bool repeat, timer_callback handler);

/**********************************************
 * @fn void nrc_timer_stop(timer_id id)
 *
 * @brief Stop timer
 *
 * @param id: timer id
 *
 * @return N/A
 ***********************************************/
void nrc_timer_stop(timer_id id);

#endif /* __NRC_API_TIMER_H__ */