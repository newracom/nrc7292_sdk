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

#ifndef __NRC_FOTA_API_H__
#define __NRC_FOTA_API_H__

/**********************************************
 * @fn void nrc_fota_write(uint32_t dst, uint8_t *src, uint32_t len)
 *
 * @brief write len size from src to dst in fota memory area
 *
 * @param dst: offset from fota_memory start address
 *
 * @param src: source address
 *
 * @param len: source data length
 *
 * @return N/A
 ***********************************************/
void nrc_fota_write(uint32_t dst, uint8_t *src, uint32_t len);

/**********************************************
 * @fn void nrc_fota_erase(uint32_t dst, uint32_t len)
 *
 * @brief erase len size from (fota start address + dst)
 *
 * @param dst: offset from fota_memory start address
 *
 * @param len: length for erase
 *
 * @return N/A
 ***********************************************/
void nrc_fota_erase(uint32_t dst, uint32_t len);

/**********************************************
 * @fn void nrc_fota_set_info(uint32_t len, uint32_t crc)
 *
 * @brief set fota binary information (binary length and crc)
 *
 * @param len: binary size
 *
 * @param crc: crc value for binary
 *
 * @return N/A
 ***********************************************/
void nrc_fota_set_info(uint32_t len, uint32_t crc);

/**********************************************
 * @fn void nrc_fota_update_done(fota_info_t* fw_info)
 *
 * @brief finish fota and reboot
 *
 * @param fw_info: fota binary information (binary length and crc)
 *
 * @return N/A
 ***********************************************/
void nrc_fota_update_done(fota_info_t* fw_info);

/**********************************************
 * @fn uint32_t nrc_fota_cal_crc(uint8_t* data, uint32_t len)
 *
 * @brief calculate crc32 value
 *
 * @param data: data address
 *
 * @param len: length for crc
 *
 * @return crc value
 ***********************************************/
uint32_t nrc_fota_cal_crc(uint8_t* data, uint32_t len);

#endif // __NRC_FOTA_API_H__
