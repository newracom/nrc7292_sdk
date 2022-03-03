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

#ifndef __NRC_SDK_H__
#define __NRC_SDK_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "nrc_types.h"
#include "nrc_user_app.h"

#include "system_modem_api.h"

#include "util_fota.h"
#include "util_trace.h"
#include "util_cli_freertos.h"

#include "umac_scan.h"
#include "umac_s1g_channel.h"

#include "lmac_config.h"
#include "lmac_ps_common.h"
#include "lmac_rate_control.h"

#include "api_wifi.h"
#include "api_system.h"
#include "api_uart.h"
#include "api_timer.h"
#include "api_gpio.h"
#include "api_i2c.h"
#include "api_adc.h"
#include "api_pwm.h"
#include "api_spi.h"
#include "api_httpc.h"
#include "api_fota.h"
#include "api_ps.h"

#ifndef __must_check
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#define __must_check __attribute__((__warn_unused_result__))
#else
#define __must_check
#endif /* __GNUC__ */
#endif /* __must_check */

#define nrc_mem_malloc	pvPortMalloc
#define nrc_mem_free	vPortFree

#define nrc_usr_print	hal_uart_printf

/* delay in millisecond */
#define _delay_ms(x)   vTaskDelay(pdMS_TO_TICKS(x))
/* delay in microsecond */
#define _delay_us(x)   vTaskDelay(pdMS_TO_TICKS(x) / 1000)

#endif /* __NRC_SDK_H__ */
