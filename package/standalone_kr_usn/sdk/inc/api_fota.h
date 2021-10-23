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


typedef struct {
	uint32_t fw_length;
	uint32_t crc;
	uint32_t ready;
} FOTA_INFO;


/**********************************************
 * @fn bool nrc_fota_is_support(void)
 *
 * @brief check flash is able to support fota
 *
 * @return True or False
 ***********************************************/
bool nrc_fota_is_support(void);

/**********************************************
 * @fn nrc_err_t nrc_fota_write(uint32_t dst, uint8_t *src, uint32_t len)
 *
 * @brief write len size from src to dst in fota memory area
 *
 * @param dst: offset from fota_memory start address
 *
 * @param src: source address
 *
 * @param len: source data length
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_fota_write(uint32_t dst, uint8_t *src, uint32_t len);

/**********************************************
 * @fn nrc_err_t nrc_fota_erase(uint32_t dst, uint32_t len)
 *
 * @brief erase len size from (fota start address + dst)
 *
 * @param dst: offset from fota_memory start address
 *
 * @param len: length for erase
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_fota_erase(uint32_t dst, uint32_t len);

/**********************************************
 * @fn nrc_err_t nrc_fota_set_info(uint32_t len, uint32_t crc)
 *
 * @brief set fota binary information (binary length and crc)
 *
 * @param len: binary size
 *
 * @param crc: crc value for binary
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_fota_set_info(uint32_t len, uint32_t crc);

/**********************************************
 * @fn nrc_err_t nrc_fota_update_done(FOTA_INFO* fw_info)
 *
 * @brief Updated firmware and reboot
 *
 * @param fw_info: fota binary information (binary length and crc)
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_fota_update_done(FOTA_INFO* fw_info);

/**********************************************
 * @fn nrc_err_t nrc_fota_update_done_bootloader(FOTA_INFO* fw_info)
 *
 * @brief Updated bootloader and reboot
 *
 * @param fw_info: fota binary information (binary length and crc)
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_fota_update_done_bootloader(FOTA_INFO* fw_info);

/**********************************************
 * @fn nrc_err_t nrc_fota_cal_crc(uint8_t* data, uint32_t len, uint32_t *crc)
 *
 * @brief calculate crc32 value
 *
 * @param data: data address
 *
 * @param len: length for crc
 *
 * @param crc: calculated crc value
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_fota_cal_crc(uint8_t* data, uint32_t len, uint32_t *crc);

#endif // __NRC_FOTA_API_H__
