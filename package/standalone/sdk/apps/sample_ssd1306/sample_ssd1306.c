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

#include <stdio.h>
#include <stdbool.h>
#include "oled96.h"
#include "nrc_types.h"
#include "api_i2c.h"
#include "api_uart.h"

void user_init(void)
{
	const uint8_t display_sda = 17;
	const uint8_t display_scl = 16;
	const uint8_t display_address= 0x3c;

	nrc_uart_console_enable(true);
	oledInit(display_sda, display_scl, I2C_MASTER_1,
		display_address, OLED_128x64, false, false);

	oledFill(0); // fill with black
	oledPrintf(0,0, FONT_NORMAL, "OLED 96 Library!");
	oledPrintf(0,0, FONT_BIG, "BIG!");
	oledPrintf(0,0, FONT_SMALL, "Small");
}
