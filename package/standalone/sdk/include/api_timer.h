/*
 * MIT License
 *
 * Copyright (c) 2024 Newracom, Inc.
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

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NRC7394
#define TIMER_CH_3	3 //APB0 64bit Timer
#define TIMER_MAX_SUPPORT	1	/* Support max 1 h/w timer */

#define TIMER_EV_3	40
#else
#define TIMER_CH_2	2  //APB0 32bit Timer
#define TIMER_CH_3	3  //APB0 64bit Timer
#define TIMER_CH_4	4 //APB1 32bit Timer
#define TIMER_MAX_SUPPORT	3	/* Support max 3 h/w timer */

#define TIMER_EV_2	32
#define TIMER_EV_3	40
#define TIMER_EV_4	17
#endif

typedef void (*timer_callback)(int ch);

typedef struct {
	bool initialized;
	int ch;
	timer_callback cb;
} TIMER_INFO;

typedef struct {
	TIMER_INFO       Timer[TIMER_MAX_SUPPORT];
} TIMER_STRUCT;


/**********************************************
 * @fn nrc_err_t nrc_hw_timer_init(int ch, timer_callback isr_cb)
 *
 * @brief hardware timer initialize
 *
 * @param ch: channel id
 *        isr_cb: callback function
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_hw_timer_init(int ch, timer_callback isr_cb);


/**********************************************
 * @fn nrc_err_t nrc_hw_timer_deinit(int ch)
 *
 * @brief hardware timer deinitialize
 *
 * @param ch: channel id
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_hw_timer_deinit(int ch);


/**********************************************
 * @fn nrc_err_t nrc_hw_timer_start(int ch, uint64_t time)
 *
 * @brief Start timer
 *
 * @param ch: channel id
 *        time: time duration
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_hw_timer_start(int ch, uint64_t time);


/**********************************************
 * @fn nrc_err_t nrc_hw_timer_stop(int ch)
 *
 * @brief Stop timer
 *
 * @param ch: channel id
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_hw_timer_stop(int ch);

/**********************************************
 * @fn nrc_err_t nrc_hw_timer_clear_irq(int ch)
 *
 * @brief hardware timer interrupt clear
 *
 * @param ch: channel id
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_hw_timer_clear_irq(int ch);


/**********************************************
 * @fn int nrc_hw_timer_get_channel(int vector)
 *
 * @brief Ger hardware timer number from event vector id
 *
 * @param vector: event vector id
 *
 * @return hardware timer channel number
 ***********************************************/
int nrc_hw_timer_get_channel(int vector);

#ifdef __cplusplus
}
#endif

#endif /* __NRC_API_TIMER_H__ */
