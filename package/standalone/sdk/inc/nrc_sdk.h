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

#if !defined (NRC_SDK_RELEASE)
#include "system_common.h"
#include "umac_s1g_channel.h"
#include "system_modem_api.h"
#include "util_cli_freertos.h"
#include "util_fota.h"
#include "lmac_ps_common.h"
#else //#if !defined (NRC_SDK_RELEASE)

#ifndef bool
#define bool uint32_t
#endif
#define true 1
#define false 0

// Type Definition
typedef signed char                 int8_t;
typedef unsigned char               uint8_t;
typedef signed short                int16_t;
typedef unsigned short              uint16_t;
typedef signed long                 int32_t;
typedef unsigned long               uint32_t;
typedef signed long long            int64_t;
typedef unsigned long long          uint64_t;
#endif

typedef uint64_t u64;
typedef unsigned int u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "stdio.h"
#include "string.h"

#include "nrc_user_app.h"

#include "util_fota.h"
#include "lmac_ps_common.h"

#include "api_wifi.h"
#include "api_uart.h"
#include "api_timer.h"
#include "api_gpio.h"
#include "api_i2c.h"
#include "api_nadc.h"
#include "api_pwm.h"
#include "api_spi.h"
#include "api_httpc.h"
#include "api_fota.h"
#include "api_ps.h"
#include "api_sflash.h"

#ifndef __must_check
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#define __must_check __attribute__((__warn_unused_result__))
#else
#define __must_check
#endif /* __GNUC__ */
#endif /* __must_check */

#define nrc_mem_malloc	pvPortMalloc
#define nrc_mem_free	vPortFree

#define nrc_usr_print	nrc_uart_printf
#define nrc_usr_vprint	nrc_uart_vprintf

/* delay in millisecond */
#define _delay_ms(x)   vTaskDelay(pdMS_TO_TICKS(x))
/* delay in microsecond */
#define _delay_us(x)   vTaskDelay(pdMS_TO_TICKS(x) / 1000)

#endif /* __NRC_SDK_H__ */
