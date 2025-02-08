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

enum ATCMD_BASIC_EVENT
{
	ATCMD_BASIC_EVENT_FWBINDL_IDLE = 0,
	ATCMD_BASIC_EVENT_FWBINDL_DROP,
	ATCMD_BASIC_EVENT_FWBINDL_FAIL,
	ATCMD_BASIC_EVENT_FWBINDL_DONE,

#if defined(CONFIG_ATCMD_SFUSER)
	ATCMD_BASIC_EVENT_SFUSER_IDLE,
	ATCMD_BASIC_EVENT_SFUSER_DROP,
	ATCMD_BASIC_EVENT_SFUSER_FAIL,
	ATCMD_BASIC_EVENT_SFUSER_DONE
#endif		
};

#define ATCMD_MSG_BEVENT(fmt, ...)		ATCMD_MSG_EVENT("BEVENT", fmt, ##__VA_ARGS__)

extern int atcmd_basic_enable (void);
extern void atcmd_basic_disable (void);

extern void atcmd_boot_reason (void);

extern bool atcmd_gpio_pin_valid (int pin);

#if defined(CONFIG_ATCMD_FWUPDATE)
extern int atcmd_firmware_download (char *buf, int len);
extern void atcmd_firmware_download_event_idle (uint32_t len, uint32_t cnt);
extern void atcmd_firmware_download_event_drop (uint32_t len);
extern void atcmd_firmware_download_event_fail (uint32_t len);
extern void atcmd_firmware_download_event_done (uint32_t len);
#endif

#if defined(CONFIG_ATCMD_SFUSER)
#define ATCMD_MSG_RXD_SFUSER(buf, len, fmt, ...)	\
		atcmd_msg_snprint(ATCMD_MSG_TYPE_EVENT, buf, len, "RXD_SFUSER:" fmt, ##__VA_ARGS__)

extern int atcmd_sf_user_write (uint32_t offset, uint32_t length, char *data);
extern void atcmd_sf_user_write_event_idle (uint32_t offset, uint32_t length, uint32_t count);
extern void atcmd_sf_user_write_event_drop (uint32_t offset, uint32_t length);
extern void atcmd_sf_user_write_event_fail (uint32_t offset, uint32_t length);
extern void atcmd_sf_user_write_event_done (uint32_t offset, uint32_t length);
#endif

#if defined(CONFIG_ATCMD_SFSYSUSER)
#define ATCMD_MSG_RXD_SFSYSUSER(buf, len, fmt, ...)	\
		atcmd_msg_snprint(ATCMD_MSG_TYPE_EVENT, buf, len, "RXD_SFSYSUSER:" fmt, ##__VA_ARGS__)
#endif

/**********************************************************************************************/
#endif /* #ifndef __NRC_ATCMD_BASIC_H__ */

