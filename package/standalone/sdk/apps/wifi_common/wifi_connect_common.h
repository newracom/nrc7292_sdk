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

#ifndef __WIFI_CONNECT_COMMON_H__
#define __WIFI_CONNECT_COMMON_H__

#include "wifi_config_setup.h"

int wifi_init(WIFI_CONFIG *param);
int wifi_connect(WIFI_CONFIG *param);
int wifi_start_softap(WIFI_CONFIG *param);

/**********************************************
 * @fn void  wifi_set_ip_to_static(uint8_t mode)
 *
 * @brief Override IP setting mode to static if mode set to 1.
 *
 * @param mode : 1 to override IP setting to static
 * regardless of initial setting
 *
 * @return None
 ***********************************************/
void wifi_set_ip_to_static(uint8_t mode);

/**********************************************
 * @fn bool  nrc_get_ip_override()
 *
 * @brief Returns IP mode overriden value.
 *
 * @return 1 indicating static IP setting used.
 *         0 system initial setting will be used.
 ***********************************************/
uint8_t wifi_get_ip_override();
#endif /* __WIFI_CONNECT_COMMON_H__ */
