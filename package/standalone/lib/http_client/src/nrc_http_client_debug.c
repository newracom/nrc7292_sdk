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


#include "nrc_sdk.h"
#include "nrc_http_client_debug.h"

#define MAX_LOG_LINE_SIZE 1024

static httpc_log_level_t current_log_level = HTTPC_LOG_LEVEL_ERROR;

void httpc_set_log_level(httpc_log_level_t level)
{
    current_log_level = level;
}

httpc_log_level_t httpc_get_log_level(void)
{
    return current_log_level;
}

void httpc_log(httpc_log_level_t level, const char *format, ...)
{
	if (level <= current_log_level) {
		const char *level_strings[] = {"E", "W", "I", "D"};
		char log_message[MAX_LOG_LINE_SIZE];

		// Print the log level prefix
		int offset = snprintf(log_message, sizeof(log_message), "[%s] ", level_strings[level]);

		// Handle variable arguments
		va_list args;
		va_start(args, format);
		vsnprintf(log_message + offset, sizeof(log_message) - offset, format, args);
		va_end(args);

		// Print the formatted log message
		fprintf(stderr, "%s\n", log_message);
	}
}

