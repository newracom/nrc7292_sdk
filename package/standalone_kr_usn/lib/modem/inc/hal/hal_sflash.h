#ifndef __HAL_SFLASH_NRC7292_H__
#define __HAL_SFLASH_NRC7292_H__

#include "system.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------
 * W25XX JEDEC ID
 * ---------------------------------- */
#define W25XX_JEDEC_ID (0x621614)
#define IS25WP080D_JEDEC_ID (0x9D7014)
#define XT25Q08B_JEDEC_ID (0x0B6014)
#define IS25LP080D_JEDEC_ID (0x9D6014)
#define IS25LP128D_JEDEC_ID (0x9D6018)
#define EN25S16B_JEDEC_ID (0x1C3815)
#define EN25QH16B_JEDEC_ID (0x1C7015)
#define W25Q16FW_JEDEC_ID (0xEF6015)
#define W25Q80EW_JEDEC_ID (0xEF6014)
#define GD25LQ16C_JEDEC_ID (0xC86015)
#define GD25LQ40C_JEDEC_ID (0xC86013)
#define MX25V8035F_JEDEC_ID (0xC22314)
#define MX25U1633F_JEDEC_ID (0xC22535)
#define MX25R3235F_JEDEC_ID (0xC22816)

/* ----------------------------------
 * W26XX JEDEC ID
 * ---------------------------------- */
#define W26WF080B_JEDEC_ID (0xBF2658)
#define W26WF040B_JEDEC_ID (0xBF2654)

enum sf_cmd_e {
	SF_WRITE_ENABLE = 0x06,
	SF_WRITE_DISABLE = 0x04,
	SF_READ_STATUS = 0x05,
	SF_WRITE_STATUS = 0x01,
	SF_READ = 0x03,
	SF_FAST_READ = 0x0B,
	SF_DUAL_READ = 0x3B,
	SF_PAGE_PROG = 0x02,
	SF_ERASE_BLOCK = 0xD8,
	SF_ERASE_SECTOR = 0x20,
	//SF_ERASE_ENTIRE = 0x60,
	SF_ERASE_ENTIRE = 0xC7,
	SF_READ_ID  = 0xAB,
	SF_READ_JEDECID = 0x9F,
	SF_POWER_DOWN = 0xB9,
	SF_POWER_UP = 0xAB,
	SF_GLOBAL_PROT_UNLOCK = 0x98,
	SF_NOP = 0x00
};

enum sf_burst_len_e {
	SF_BURST_8 = 0x00,
	SF_BURST_16 = 0x01,
	SF_BURST_32 = 0x02,
	SF_BURST_64 = 0x03
};

uint32_t nrc_sf_get_size(void);
static uint32_t half_addr_sf = 0;

enum sf_store_area_e {
#if defined(SUPPORT_4MB_FLASH)
	SF_BOOTLOADER = 0x0,
	SF_FW = 0x10000,
	SF_FW_INFO = 0x190000,
	SF_CORE_DUMP = 0x191000,
	SF_USER_CONFIG_1 = 0x195000,/*200KB*/
	SF_USER_CONFIG_2 = 0x1C7000,/*200KB*/
	SF_USER_CONFIG_3 = 0x1F9000,/*200KB*/
	SF_USER_CONFIG_4 = 0x22B000,/*200KB*/
	SF_RESERVED = 0x25D000,/*120KB*/
	SF_BDF = 0x27B000, /*4KB*/
	SF_SYSTEM_CONFIG = 0x27C000,
	SF_MAC_ADDR = 0x27C000, /*not used*/
	SF_MAC_ADDR_MC = 0x27C008,/*not used*/
	SF_RF_CAL = 0x27D000,
	SF_FOTA = 0x27E000,
	SF_FOTA_INFO = 0x3FE000
#else
	SF_BOOTLOADER = 0x0,
	SF_FW = 0x10000,
	SF_FW_INFO = 0xF5000,
	SF_CORE_DUMP = 0xF6000,
	SF_USER_CONFIG_1 = 0xFA000,
	SF_USER_CONFIG_2 = 0xFB000,
	SF_USER_CONFIG_3 = 0xFC000,
	SF_USER_CONFIG_4 = 0xFD000,
	SF_SYSTEM_CONFIG = 0xFE000,
	SF_MAC_ADDR = 0xFE000,
	SF_MAC_ADDR_MC = 0xFE008,
	SF_RF_CAL = 0xFF000,
	SF_FOTA = 0x100000,
	SF_USER_DATA = 0x1E5000,
	SF_BDF = 0x1FE000,
	SF_FOTA_INFO = 0x1FF000
#endif
};

enum sf_reg_override_ctrl {
	SF_REG_OVER_CTRL_ENABLE = 1,
	SF_REG_OVER_CTRL_DISABLE = 0
};

#define SF_SLOT_DATA_OFFSET 12
#define SF_SLOT_CRC_OFFSET 8
#define SF_SLOT_MAX_LEN 4084 /* 4KB - size of Header */

typedef struct {
	uint32_t signature; /*'nrct'*/
	uint32_t crc32; /*computed by util_crc_compute_crc32() in util_crc.c*/
	uint32_t length; /*actual data length*/
} sf_slot_t;

#define SYS_CONFIG_STRUCTURE_VERSION 2
struct sf_reg_override {
	uint32_t control;
	uint32_t value;
};

typedef struct {
	uint32_t version; /* sys_config structure version*/
	uint8_t mac_addr[6]; /*mac address for interface 0*/
	uint8_t mac_addr_1[6]; /*mac address for interface 1*/
	uint32_t cal_use :8; /*enable/disable the usage of calibration data*/
	uint32_t bdf_use :8; /*enable/disable the usage of bdf data*/
	uint32_t hw_version:16; /* HW Version */
	struct sf_reg_override rf_pllldo12_tr;
	uint32_t brd_rev_value:7;
	uint32_t disable_gpio_brd_rev:1;
	uint32_t reserved:24;
} sf_sys_config_t;

typedef struct {
	uint32_t fw_size;
	uint32_t fw_crc;
} sf_fw_info_t;

typedef struct {
	uint32_t block_size;
	uint32_t sector_size;
	uint32_t page_size;
	uint32_t total_size;
} sf_info_t;

typedef struct sf_ops_ {
	void (*init)(uint32_t, bool, bool);
	void (*deinit)(void);
	bool (*erase)(sf_info_t *, uint32_t, size_t);
	bool (*erase_entire)(void);
	bool (*protect)(bool);
	uint32_t (*read)(uint32_t, uint8_t *, size_t);
	uint32_t (*write)(uint32_t, uint8_t *, size_t);
	bool (*power_down)(bool);
	uint32_t (*read_id)(void);
} sf_ops;

uint32_t nrc_sf_identify(void);
void nrc_sf_init(uint32_t id, bool read_only, bool quad);
void nrc_sf_deinit(void);
bool nrc_sf_erase(uint32_t address, size_t size);
bool nrc_sf_erase_entire(void);
bool nrc_sf_protect(bool enable);
uint32_t nrc_sf_read(uint32_t address, uint8_t *buffer, size_t size);
uint32_t nrc_sf_write(uint32_t address, uint8_t *buffer, size_t size);
bool nrc_sf_power_down(bool enable);
uint32_t nrc_sf_read_id(void);
uint32_t nrc_sf_read_jedec(void);
bool nrc_sf_check_slot_sig(uint32_t* signature);
bool nrc_sf_update_slot(uint32_t address, uint8_t *data, size_t size);
bool nrc_sf_read_slot_info(uint32_t address, sf_slot_t *data);
bool nrc_sf_read_slot_data(uint32_t address, size_t size, uint8_t *buffer);
bool nrc_sf_update_version(uint32_t version);
bool nrc_sf_update_macaddr(uint8_t *macaddr, uint8_t interface);
bool nrc_sf_get_macaddr(uint8_t *macaddr, uint8_t interface);
bool nrc_sf_update_cal_use(uint8_t cal_use);
bool nrc_sf_update_rf_pllldo12_tr(uint32_t control, uint32_t value);
void nrc_sf_get_pllldo(uint32_t *value);
bool nrc_sf_valid_pllldo(void);
uint32_t nrc_sf_get_size(void);
bool nrc_sf_userconfig_erase(uint32_t address);
bool nrc_sf_userconfig_read(uint32_t address, uint8_t *data, size_t size);
bool nrc_sf_userconfig_write(uint32_t address, uint8_t *data, size_t size);
bool nrc_sf_update_bdf(uint32_t length);
bool nrc_sf_is_bdf_use(void);
bool nrc_sf_update_bdf_use(uint8_t bdf_use);
uint16_t nrc_sf_get_hw_version(void);
bool nrc_sf_update_disable_gpio_brd_rev(uint8_t value);
bool nrc_sf_get_disable_gpio_brd_rev(void);
bool nrc_sf_update_brd_rev(uint8_t version);
uint8_t nrc_sf_get_brd_rev(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif //__HAL_SFLASH_NRC7292_H__
