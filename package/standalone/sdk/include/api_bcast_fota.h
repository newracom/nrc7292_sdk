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

#ifndef __NRC_BCAST_FOTA_API_H__
#define __NRC_BCAST_FOTA_API_H__

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************
 * @fn void nrc_bcast_fota_init(void)
 *
 * @brief initialize broadcast FOTA
 *
 * @return NONE
 ***********************************************/
void nrc_bcast_fota_init(void);

/**********************************************
 * @fn void nrc_bcast_fota_set_mode(uint8_t mode)
 *
 * @brief setiing broadcast FOTA mode
 *
 * @param mode: BC_FOTA_MODE_ANY / BC_FOTA_MODE_CONNECTED
 *   BC_FOTA_MODE_ANY - Run broadcast FOTA without AP connection
 *   BC_FOTA_MODE_CONNECTED - Run broadcast FOTA when AP connected
 *
 * @return NONE
 ***********************************************/
void nrc_bcast_fota_set_mode(uint8_t mode);

/**********************************************
 * @fn void nrc_bcast_fota_enable(bool enable)
 *
 * @brief enable/disable broadcast FOTA
 *
 * @param enable: true / false
 *   true: enable broadcast FOTA
 *   false: disable broadcast FOTA
 *
 * @return NONE
 ***********************************************/
void nrc_bcast_fota_enable(bool enable);

#ifdef __cplusplus
}
#endif

#endif // __NRC_BCAST_FOTA_API_H__
