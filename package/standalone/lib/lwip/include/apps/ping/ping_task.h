/*
 * MIT License
 *
 * Copyright (c) 2021 Newracom, Inc.
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

#ifndef __PING_TASK_H__
#define __PING_TASK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ping.h"

void ping_mutex_init(void);
void ping_mutex_lock(void);
void ping_mutex_unlock(void);

	sys_thread_t ping_init(void *arg);
void ping_deinit(void *arg);

void ping_list_add(ping_parm_t* item);
void ping_list_remove(ping_parm_t* item);
ping_parm_t *ping_get_session(ip_addr_t* addr);
void ping_list_display(void);

#ifdef __cplusplus
}
#endif

#endif /* __PING_TASK_H__ */
