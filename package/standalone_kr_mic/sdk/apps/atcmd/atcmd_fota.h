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

#define ATCMD_FOTA_SERVER_URL_LEN_MAX	128
#define ATCMD_FOTA_BIN_NAME_LEN_MAX		128

#define ATCMD_FOTA_INFO_FILE			"fota.json"
#define ATCMD_FOTA_RECV_BUF_SIZE		(1024 * 8)

#define ATCMD_FOTA_TASK_PRIORITY		ATCMD_TASK_PRIORITY
#define ATCMD_FOTA_TASK_STACK_SIZE		((4 * 1024) / sizeof(StackType_t))

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
	char name[ATCMD_FOTA_BIN_NAME_LEN_MAX + 1];
	uint32_t crc32;
	uint32_t size;
} fw_bin_t;

/**********************************************************************************************/

enum FOTA_EVENT
{
	FOTA_EVT_VERSION = 0,
	FOTA_EVT_BINARY,
	FOTA_EVT_DOWNLOAD,
	FOTA_EVT_UPDATE,
	FOTA_EVT_FAIL,
};

typedef struct
{
	enum FOTA_EVENT type;

	union
	{
		struct
		{
			const char *sdk;
			const char *atcmd;
		} version;

		struct
		{
			const char *name;
		} binary;

		struct
		{
			uint32_t total;
			uint32_t len;
		} download;

		struct
		{
			const char *bin_name;
			uint32_t bin_size;
			uint32_t bin_crc32;
		} update;
	};
} atcmd_fota_event_t;

typedef struct
{
	bool new_fw;

	fw_ver_t fw_ver[FW_VER_NUM];
	fw_bin_t fw_bin[FW_BIN_NUM];
} atcmd_fota_info_t;

typedef struct
{
	enum FW_BIN fw_bin_type;
	void (*event_cb) (atcmd_fota_event_t *event);

	int32_t check_time; // sec
	char server_url[ATCMD_FOTA_SERVER_URL_LEN_MAX + 1];
	char bin_name[ATCMD_FOTA_BIN_NAME_LEN_MAX + 1];
	uint32_t bin_crc32;
} atcmd_fota_params_t;

typedef struct
{
	bool enable;
	bool update;

	atcmd_fota_info_t info;
	atcmd_fota_params_t params;

	TaskHandle_t task;

	struct
	{
		char *addr;
		int len;
	} recv_buf;
} atcmd_fota_t;

/**********************************************************************************************/

extern int atcmd_fota_get_params (atcmd_fota_params_t *params);
extern int atcmd_fota_set_params (atcmd_fota_params_t *params);
extern int atcmd_fota_update_firmware (void);

/**********************************************************************************************/
#endif /* #ifndef __NRC_ATCMD_FOTA_H__*/

