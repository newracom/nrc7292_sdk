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

#ifndef _HTTPC_DEBUG_H_
#define _HTTPC_DEBUG_H_

#include <stdio.h>
#include <stdarg.h>

#define HTTPC_DEBUG 1

typedef enum {
    HTTPC_LOG_LEVEL_ERROR = 0,
    HTTPC_LOG_LEVEL_WARN,
    HTTPC_LOG_LEVEL_INFO,
    HTTPC_LOG_LEVEL_DEBUG ,
} httpc_log_level_t;

#ifdef HTTPC_DEBUG
#define HTTPC_LOG(level, format, ...) httpc_log(level,format, ##__VA_ARGS__)
#define HTTPC_LOGD(format, ...) httpc_log(HTTPC_LOG_LEVEL_DEBUG, format, ##__VA_ARGS__)
#define HTTPC_LOGI(format, ...) httpc_log(HTTPC_LOG_LEVEL_INFO, format, ##__VA_ARGS__)
#define HTTPC_LOGW(format, ...) httpc_log(HTTPC_LOG_LEVEL_WARN,format, ##__VA_ARGS__)
#define HTTPC_LOGE(format, ...) httpc_log(HTTPC_LOG_LEVEL_ERROR,format, ##__VA_ARGS__)
#define HTTPC_LOGA(format, ...) httpc_log(HTTPC_LOG_LEVEL_ALWAYS, format, ##__VA_ARGS__)
#else
#define HTTPC_LOG(level, format, ...)
#define HTTPC_LOGD(format, ...)
#define HTTPC_LOGI(format, ...)
#define HTTPC_LOGW(format, ...)
#define HTTPC_LOGE(format, ...)
#define HTTPC_LOGA(format, ...)
#endif

// Function declarations
void httpc_set_log_level(httpc_log_level_t level);
httpc_log_level_t httpc_get_log_level(void);
void httpc_log(httpc_log_level_t level, const char *format, ...);

#endif // _HTTPC_DEBUG_H_

