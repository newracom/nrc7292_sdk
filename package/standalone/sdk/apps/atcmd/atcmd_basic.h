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

#ifndef __NRC_ATCMD_BASIC_H__
#define __NRC_ATCMD_BASIC_H__
/**********************************************************************************************/

extern int atcmd_basic_enable (void);
extern void atcmd_basic_disable (void);

extern bool atcmd_gpio_pin_valid (int pin);

extern void atcmd_firmware_write (char *buf, int len);
extern void atcmd_firmware_download_event_idle (uint32_t len, uint32_t cnt);
extern void atcmd_firmware_download_event_drop (uint32_t len);
extern void atcmd_firmware_download_event_done (uint32_t len);

/**********************************************************************************************/
#endif /* #ifndef __NRC_ATCMD_BASIC_H__ */

