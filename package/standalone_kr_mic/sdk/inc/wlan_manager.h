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

#ifndef __WLAN_MANAGER_H__
#define __WLAN_MANAGER_H__

#include "nrc_types.h"

#define MAX_PMK_LENGTH 65

extern QueueHandle_t    x_wlan_mgr_queue_handle;
extern WLAN_SUPPLICANT_RET x_sup_ret;

#define SUP_RET				x_sup_ret.ret
#define SUP_RET_LEN			x_sup_ret.ret_len
#define CMD_BUFFER_LEN 512

void wlan_mgr_handle_cmd(WLAN_MESSAGE *message);
void wlan_mgr_handle_event(WLAN_MESSAGE *message);
void wlan_mgr_main(void *pvParameters);

#endif /* __WLAN_MANAGER_H__ */
