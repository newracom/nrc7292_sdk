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

#ifndef __NRC_ATCMD_PARAM_H__
#define __NRC_ATCMD_PARAM_H__
/**********************************************************************************************/

extern int atcmd_param_to_int8 (const char *param, int8_t *val);
extern int atcmd_param_to_int16 (const char *param, int16_t *val);
extern int atcmd_param_to_int32 (const char *param, int32_t *val);

extern int atcmd_param_to_uint8 (const char *param, uint8_t *val);
extern int atcmd_param_to_uint16 (const char *param, uint16_t *val);
extern int atcmd_param_to_uint32 (const char *param, uint32_t *val);

extern int atcmd_param_to_float (const char *param, float *val);
extern int atcmd_param_to_hex (const char *param, uint32_t *val);

extern char *atcmd_param_to_str (const char *param, char *str, int len);
extern char *atcmd_str_to_param (const char *str, char *param, int len);

/**********************************************************************************************/
#endif /* #ifndef __NRC_ATCMD_PARAM_H__ */
