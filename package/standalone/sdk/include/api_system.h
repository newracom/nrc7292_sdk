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

#ifndef __NRC_API_SYSTEM_H__
#define __NRC_API_SYSTEM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "util_trace.h"

/**********************************************
 * @fn nrc_err_t nrc_get_rtc(uint64_t *rtc_time)
 *
 * @brief Read RTC time since boot
 *
 * @param rtc_time: A pointer for getting RTC (uint64)
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_get_rtc(uint64_t *rtc_time);

/**********************************************
 * @fn void nrc_reset_rtc(void)
 *
 * @brief Set RTC hardware to 0
 *
 * @return None
 ***********************************************/
void nrc_reset_rtc(void);

/**********************************************
 * @fn void nrc_sw_reset(void)
 *
 * @brief Reset SW
 ***********************************************/
void nrc_sw_reset(void);

/**********************************************
 * @fn nrc_err_t nrc_get_user_factory(char* data, uint16_t buf_len)
 *
 * @brief Get user factory data
 *
 * @param data: A pointer to store user factory data
 *
 * @param buf_len: buffer length (should be 512 Bytes)
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_get_user_factory(char* data, uint16_t buf_len);

/**********************************************
 * @fn nrc_err_t nrc_get_user_factory_info(uint32_t *addr, uint32_t *size)
 *
 * @brief Get base address and total size of user factory area
 *
 * @param addr: A pointer to store base address of user factory area
 *
 * @param size: A pointer to store total size of user factory area
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_get_user_factory_info(uint32_t *addr, uint32_t *size);

/**********************************************
 * @fn void nrc_led_trx_init(int tx_gpio, int rx_gpio, int timer_period, bool invert)
 *
 * @brief Initializes the Tx/Rx LED blinking feature
 *
 * @param tx_gpio: The GPIO pin for the Tx LED
 *
 * @param rx_gpio: The GPIO pin for the Rx LED
 *
 * @param timer_period: The period for checking the status of the LED blinking
 *
 * @param invert: A boolean value to indicate whether to invert the LED blinking signal
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_led_trx_init(int tx_gpio, int rx_gpio, int timer_period, bool invert);


/**********************************************
 * @fn  int nrc_led_trx_deinit(void)
 *
 * @brief Deinitializes the Tx/Rx LED blinking feature
 *
 * @param void
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_led_trx_deinit(void);


/**********************************************
 * @fn	nrc_err_t nrc_wdt_enable(void)
 *
 * @brief Enable watchdog monitoring. The default is enabled
 *
 * @param void
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_wdt_enable(void);


/**********************************************
 * @fn	nrc_err_t nrc_wdt_disable(void)
 *
 * @brief Disable watchdog monitoring. The default is enabled
 *
 * @param void
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_wdt_disable(void);


/**********************************************
 * @fn nrc_err_t nrc_set_app_version(VERSION_T* version)
 *
 * @brief Set application version, which is a mandatory to use broadcast FOTA
 *
 * @param A pointer application version
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_set_app_version(VERSION_T* version);


/**********************************************
 * @fn  VERSION_T* nrc_get_app_version(void)
 *
 * @brief Get application version, which is a mandatory to use broadcast FOTA
 *
 * @param void
 *
 * @return A pointer to version
 ***********************************************/
VERSION_T* nrc_get_app_version(void);


/**********************************************
 * @fn nrc_err_t nrc_set_app_name(char* appname)
 *
 * @brief Set application name, which is a mandatory to use broadcast FOTA
 *
 * @param A pointer application name
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_set_app_name(char* appname);


/**********************************************
 * @fn  char* nrc_get_app_name(void)
 *
 * @brief Get application name, which is a mandatory to use broadcast FOTA
 *
 * @param void
 *
 * @return A pointer to app name
 ***********************************************/
char* nrc_get_app_name(void);


/**********************************************
 * @fn	VERSION_T* nrc_get_sdk_version(void)
 *
 * @brief Get SDK version
 *
 * @param void
 *
 * @return A pointer to version
 ***********************************************/
VERSION_T* nrc_get_sdk_version(void);


/**********************************************
 * @fn     nrc_err_t nrc_set_flash_device_info(uint8_t* data, unit16_t len)
 *
 * @brief  Write data to the device info page on the flash memory.
 *
 * @param  data: the source buffer that holds the data to be written.
 *
 * @param  len:  the length of the data in bytes. (max 4096 bytes).
 *
 * @return If successful, NRC_SUCCESS is returned. Otherwise, NRC_FAIL is returned;
 ***********************************************/
nrc_err_t nrc_set_flash_device_info(uint8_t* data, uint16_t len);


/**********************************************
 * @fn     nrc_err_t nrc_get_flash_device_info(uint8_t* data, unit16_t len)
 *
 * @brief  Read data from the device info page on the flash memory.
 *
 * @param  data: the destination buffer that the data read from the flash memory will be copied to.
 *
 * @param  len:  the length of the data in bytes. (max 4096 bytes).
 *
 * @return If successful, NRC_SUCCESS is returned. Otherwise, NRC_FAIL is returned;
 ***********************************************/
nrc_err_t nrc_get_flash_device_info(uint8_t* data, uint16_t len);


/**********************************************
 * @fn     uint32_t nrc_get_user_data_area_address(void)
 *
 * @brief  Get the base address of the user data area in flash memory.
 *
 * @param  void
 *
 * @return The base address of the user data area.
 ***********************************************/
uint32_t nrc_get_user_data_area_address(void);


/**********************************************
 * @fn     uint32_t nrc_get_user_data_area_size(void)
 *
 * @brief  Get the size of the user data area in flash memory.
 *
 * @param  void
 *
 * @return The size of the user data area in bytes.
 ***********************************************/
uint32_t nrc_get_user_data_area_size(void);


/**********************************************
 * @fn    nrc_err_t nrc_erase_user_data_area(void)
 *
 * @brief  Erase the user data area in flash memory
 *
 * @return If successful, NRC_SUCCESS is returned. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_erase_user_data_area(void);



/**********************************************
 * @fn    nrc_err_t nrc_write_user_data(uint32_t user_data_offset, uint8_t* data, uint32_t size)
 *
 * @brief  Write user data to the user data area in flash memory
 *
 * @param  user_data_offset : The offset from the user data area's base address. It should be aligned to a 4-byte boundary.
 *
 * @param  *data: A pointer to the data to be written.
 *
 * @param  size: The size of data to write.
 *
 * @return If successful, NRC_SUCCESS is returned. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_write_user_data(uint32_t user_data_offset, uint8_t* data, uint32_t size);



/**********************************************
 * @fn     nrc_err_t nrc_read_user_data(uint8_t* buffer, uint32_t user_data_offset, uint32_t size)
 *
 * @brief  Read user data from the user data area in flash memory
 *
 * @param  *buffer : A pointer to the buffer where the data will be stored.
 *
 * @param  user_data_offset : The offset from the user data area's base address. It should be aligned to a 4-byte boundary.
 *
 * @param  size: The size of data to read.
 *
 * @return If successful, NRC_SUCCESS is returned. Otherwise, NRC_FAIL is returned.

 ***********************************************/
nrc_err_t nrc_read_user_data(uint8_t* buffer, uint32_t user_data_offset, uint32_t size);

#if !defined(NRC7292)
/**********************************************
 * @fn     uint8_t nrc_get_xtal_status(void)
 *
 * @brief  Get the status of the crystal (xtal)
 *           It is designed specifically for NRC7394
 *
 * @param  void
 *
 * @return Crystal(xtal) status
 *           0(Crystal status not checked)
 *           1(Crystal is working)
 *           2(Crystal is not working)
 ***********************************************/
uint8_t nrc_get_xtal_status(void);
#endif /* !defined(NRC7292) */



/**********************************************
 * @fn     void nrc_set_jtag(bool enable)
 *
 * @brief  Set the JTAG pin
 *
 * @param  bool enable
 * 			enable/disable JTAG debug pin
 *
 * @return None
 ***********************************************/
void nrc_set_jtag(bool enable);
#ifdef __cplusplus
}
#endif

#endif /* __NRC_API_SYSTEM_H__ */
