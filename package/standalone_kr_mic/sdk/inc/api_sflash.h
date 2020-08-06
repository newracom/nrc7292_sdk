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

#ifndef __NRC_SFLASH_API_H__
#define __NRC_SFLASH_API_H__

#define USER_CONFIG_MAX_LEN 4084 /* 4KB - size of Header */

enum user_config_area {
	USER_CONFIG_AREA_1,
	USER_CONFIG_AREA_2,
	USER_CONFIG_AREA_3,
	USER_CONFIG_AREA_MAX,
};

/**********************************************
 * @fn bool nrc_sf_erase_user_config(uint8_t user_area)
 *
 * @brief  erase 4KB user config area from Userconfig start address
 *
 * @param user_area: user config area
 *
 * @return true or false
 ***********************************************/

bool nrc_sf_erase_user_config(uint8_t user_area);

/**********************************************
 * @fn bool nrc_sf_read_user_config(uint8_tuser_area, uint8_t *data, size_t size)
 *
 * @brief read user config area from user_config area
 *
 * @param user_area: user config area
 *
 * @param data: user data
 *
 * @param size: user data size
 *
 * @return true or false
 ***********************************************/

bool nrc_sf_read_user_config(uint8_t user_area, uint8_t *data, size_t size);


/**********************************************
 * @fn bool nrc_sf_write_user_config(uint8_t user_area, uint8_t *data, size_t size)
 *
 * @brief write user config area from user_config address
 *
 * @param user_area: user config area
 *
 * @param data: user data
 *
 * @param size: user data size
 *
 * @return true or false
 ***********************************************/

bool nrc_sf_write_user_config(uint8_t user_area, uint8_t *data, size_t size);

#endif // __NRC_SFLASH_API_H__
