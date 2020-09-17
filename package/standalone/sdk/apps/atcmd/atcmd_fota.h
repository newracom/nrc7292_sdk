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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifndef __NRC_ATCMD_FOTA_H__
#define __NRC_ATCMD_FOTA_H__
/**********************************************************************************************/

enum FW_VER
{
	FW_VER_SDK = 0,
	FW_VER_ATCMD,

	FW_VER_NUM
};

enum FW_BIN
{
	FW_BIN_HSPI = 0,
	FW_BIN_UART,
	FW_BIN_UART_HFC,

	FW_BIN_NUM
};

typedef struct
{
	int major;
	int minor;
	int revision;
} fw_ver_t;

typedef struct
{
#define ATCMD_FW_NAME_LEN_MAX		50

	char name[ATCMD_FW_NAME_LEN_MAX + 1];
	uint32_t crc;
	uint32_t size;
} fw_bin_t;

/**********************************************************************************************/

typedef struct
{
	fw_ver_t fw_ver[FW_VER_NUM];
	fw_bin_t fw_bin[FW_BIN_NUM];
} atcmd_fota_info_t;

typedef struct
{
#define ATCMD_FOTA_SERVER_URL_LEN_MAX	50

	enum FW_BIN fw_bin_type;

	int check_time; // sec
	void (*check_done_cb) (const char *sdk_ver, const char *atcmd_ver);

	char server_url[ATCMD_FOTA_SERVER_URL_LEN_MAX + 1];
} atcmd_fota_params_t;

typedef struct
{
	bool enable;
	bool update;

	atcmd_fota_info_t info;
	atcmd_fota_params_t params;

	TaskHandle_t task;
} atcmd_fota_t;

/**********************************************************************************************/

extern bool atcmd_fota_valid_params (atcmd_fota_params_t *params);
extern int atcmd_fota_get_params (atcmd_fota_params_t *params);
extern int atcmd_fota_set_params (atcmd_fota_params_t *params);
extern int atcmd_fota_update_firmware (void);

/**********************************************************************************************/
#endif /* #ifndef __NRC_ATCMD_FOTA_H__*/

