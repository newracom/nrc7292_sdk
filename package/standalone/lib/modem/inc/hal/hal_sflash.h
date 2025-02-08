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
#define XT25Q16D_JEDEC_ID (0x0B6015)
#define XT25W16F_JEDEC_ID (0x0B6515)
#define IS25LP080D_JEDEC_ID (0x9D6014)
#define IS25LP016D_JEDEC_ID (0x9D6015)
#define IS25LP128D_JEDEC_ID (0x9D6018)
#define IS25WP016D_JEDEC_ID (0x9D7015)
#define EN25S16B_JEDEC_ID (0x1C3815)
#define EN25QH16B_JEDEC_ID (0x1C7015)
#define W25Q16FW_JEDEC_ID (0xEF6015)
#define W25Q80EW_JEDEC_ID (0xEF6014)
#define GD25LQ16C_JEDEC_ID (0xC86015)
#define GD25LQ40C_JEDEC_ID (0xC86013)
#define GD25WQ16E_JEDEC_ID (0xC86515)
#define GD25WQ32E_JEDEC_ID (0xC86516)
#define GD25Q16CE2GR_JEDEC_ID (0xC84015)
#define MX25V8035F_JEDEC_ID (0xC22314)
#define MX25V1635F_JEDEC_ID (0xC22315)
#define MX25U1633F_JEDEC_ID (0xC22535)
#define MX25R3235F_JEDEC_ID (0xC22816)
#define MX25R1635F_JEDEC_ID (0xC22815)
#define MX25R8035F_JEDEC_ID (0xC22814)
#define EN25QW32A_JEDEC_ID (0x1C6116)
#define EN25S32A_JEDEC_ID (0x1C3816)
#define EN25SE32A_JEDEC_ID (0x1C4816)
#define XT25Q32F_JEDEC_ID (0x0B6016)
#define P25Q32U_JEDEC_ID (0x856016)
#define P25Q16U_JEDEC_ID (0x856015)
#define GT25Q16A_JEDEC_ID (0xC46015)
#define GT25Q32A_JEDEC_ID (0xC46016)
#define FM25W16A_JEDEC_ID (0xA12815)
#define FM25W32A_JEDEC_ID (0xA12816)
#define ZB25WQ16A_JEDEC_ID (0x5E3415)
#define BY25Q32ES_JEDEC_ID (0x684016)

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
	SF_ENTER_OTP = 0x3A,
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
static bool mem_map_valid = 0;
typedef struct {
	uint32_t PROFILE:16;
	uint32_t MAP_VERSION:16;
	uint32_t FW;
	uint32_t FW_INFO;
	uint32_t CORE_DUMP;
	uint32_t USER_CONFIG_1;
	uint32_t USER_CONFIG_2;
	uint32_t USER_CONFIG_3;
	uint32_t USER_CONFIG_4;
	uint32_t SYS_CONFIG;
	uint32_t RF_CAL;
	uint32_t FOTA;
	uint32_t DEVICE_INFO;
	uint32_t USER_DATA;
	uint32_t FOTA_INFO;
} sf_mem_map_t;

enum sf_store_area_e {
	SF_BOOTLOADER_4MB = 0x0,
	SF_FW_4MB = 0x10000,
	SF_USER_DATA_4MB = 0x3DA000,/*100KB*/
	SF_FW_INFO_4MB = 0x3FE000,
	SF_CORE_DUMP_4MB = 0x3F3000,
	SF_USER_CONFIG_1_4MB = 0x3F7000,/*4KB*/
	SF_USER_CONFIG_2_4MB = 0x3F8000,/*4KB*/
	SF_USER_CONFIG_3_4MB = 0x3F9000,/*4KB*/
	SF_USER_CONFIG_4_4MB = 0x3FA000,/*4KB*/
	SF_DEVICE_INFO_4MB = 0x3FB000,/*4KB*/
	SF_SYSTEM_CONFIG_4MB = 0x3FC000,
	SF_RF_CAL_4MB = 0x3FD000,
	SF_FOTA_4MB = 0x1F5000,
	SF_FOTA_INFO_4MB = 0x3FF000,
	SF_BOOTLOADER_2MB = 0x0,
	SF_FW_2MB = 0x10000,
	SF_FW_INFO_2MB = 0xF5000,
	SF_CORE_DUMP_2MB = 0xF6000,
	SF_USER_CONFIG_1_2MB = 0xFA000,
	SF_USER_CONFIG_2_2MB = 0xFB000,
	SF_USER_CONFIG_3_2MB = 0xFC000,
	SF_USER_CONFIG_4_2MB = 0xFD000,
	SF_SYSTEM_CONFIG_2MB = 0xFE000,
	SF_MAC_ADDR_2MB = 0xFE000,
	SF_MAC_ADDR_MC_2MB = 0xFE008,
	SF_RF_CAL_2MB = 0xFF000,
	SF_FOTA_2MB = 0x100000,
	SF_DEVICE_INFO_2MB = 0x1E5000,
	SF_USER_DATA_2MB = 0x1E6000,
	SF_FOTA_INFO_2MB = 0x1FF000
};

sf_mem_map_t* nrc_sf_get_mem_map(void);
static sf_mem_map_t sf_mem_map;
#define SF_BOOTLOADER 0x0
#define SF_MEM_MAP 0xF000
#define SF_FW 0x10000
#define SF_FW_INFO nrc_sf_get_mem_map()->FW_INFO
#define SF_CORE_DUMP nrc_sf_get_mem_map()->CORE_DUMP
#define SF_USER_CONFIG_1 nrc_sf_get_mem_map()->USER_CONFIG_1
#define SF_USER_CONFIG_2 nrc_sf_get_mem_map()->USER_CONFIG_2
#define SF_USER_CONFIG_3 nrc_sf_get_mem_map()->USER_CONFIG_3
#define SF_USER_CONFIG_4 nrc_sf_get_mem_map()->USER_CONFIG_4
#define SF_SYSTEM_CONFIG nrc_sf_get_mem_map()->SYS_CONFIG
#define SF_MAC_ADDR half_addr_sf - 0x2000
#define SF_MAC_ADDR_MC half_addr_sf - 0x1FF8
#define SF_RF_CAL nrc_sf_get_mem_map()->RF_CAL
#define SF_FOTA nrc_sf_get_mem_map()->FOTA
#define SF_DEVICE_INFO nrc_sf_get_mem_map()->DEVICE_INFO
#define SF_FOTA_INFO nrc_sf_get_mem_map()->FOTA_INFO
#define SF_USER_DATA nrc_sf_get_mem_map()->USER_DATA

void nrc_sf_init_memory_map(void);
enum sf_reg_override_ctrl {
	SF_REG_OVER_CTRL_ENABLE = 1,
	SF_REG_OVER_CTRL_DISABLE = 0
};

#define SF_SLOT_DATA_OFFSET 12
#define SF_SLOT_CRC_OFFSET 8
#define SF_SLOT_MAX_LEN 4084 /* 4KB - size of Header */

#define SF_SECTOR_SIZE            4096

#define SYSCONFIG_SECTOR_SIZE               4096
#define SYSCONFIG_PRE_USER_FACTORY_SIZE      256
#define SYSCONFIG_USER_FACTORY_SIZE          512
#define SF_DEVICE_INFO_SIZE 4096

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
	uint8_t mac_addr0[6]; /*mac address for interface 0*/
	uint8_t mac_addr1[6]; /*mac address for interface 1*/
	uint32_t cal_use :8; /*enable/disable the usage of calibration data*/
	uint32_t reserved0 :8;
	uint32_t hw_version:16; /* HW Version */
	struct sf_reg_override rf_pllldo12_tr;
	uint8_t reserved1[228];
	char user_factory[SYSCONFIG_USER_FACTORY_SIZE];
} sf_sys_config_t;

typedef struct {
	uint32_t valid;
	uint32_t version;
	uint32_t cal_use; /*enable/disable the usage of calibration data*/
	uint32_t hw_version; /* HW Version */
	struct sf_reg_override rf_pllldo12_tr;
} sf_sys_config_mem_t;

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
uint32_t nrc_sf_erase_and_write(uint32_t address, uint8_t *buffer, size_t size);
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
int32_t nrc_sf_get_size_memmap(uint32_t mem_map_entry, bool is_boot_xip);
bool nrc_sf_userconfig_erase(uint32_t address);
bool nrc_sf_userconfig_read(uint32_t address, uint8_t *data, size_t size);
bool nrc_sf_userconfig_write(uint32_t address, uint8_t *data, size_t size);
uint16_t nrc_sf_get_hw_version(void);
enum sf_migration_res_e {
	SF_MIGRATION_RES_OK = 0,
	SF_MIGRATION_RES_ALREADY = 1,
	SF_MIGRATION_RES_ORG_FAIL = 2,
	SF_MIGRATION_RES_INVALID_SIZE = 3,
	SF_MIGRATION_RES_ERROR = 4,
	SF_MIGRATION_RES_NEED_FWINFO = 5
};
uint8_t nrc_sf_migration_1m_to_2m(void);
uint8_t nrc_sf_migration_2m_to_1m(void);
bool nrc_sf_update_hw_version(uint16_t version);
bool nrc_sf_update_memory_map(sf_mem_map_t *mem_map);
bool nrc_sf_set_etag_id(uint16_t etag_id);
uint16_t nrc_sf_get_etag_id(void);
uint8_t nrc_sf_get_cal_use(void);
uint16_t nrc_sf_get_esl_ch(void);
bool nrc_sf_update_user_factory(char* data, uint16_t len);
bool nrc_sf_get_user_factory(char* data, uint16_t buf_len);

bool nrc_sf_write_common(uint32_t mem_map_entry, uint32_t user_data_offset, uint8_t* data, uint32_t size);
bool nrc_sf_read_common(uint32_t mem_map_entry, uint8_t* data, uint32_t user_data_offset, uint32_t size);

uint32_t nrc_sf_get_user_data_area_address(void);
uint32_t nrc_sf_get_user_data_area_size(void);
bool nrc_sf_erase_user_data_area(void);
bool nrc_sf_write_user_data(uint32_t user_data_offset, uint8_t* data, uint32_t size);
bool nrc_sf_read_user_data(uint8_t* data, uint32_t user_data_offset, uint32_t size);
bool nrc_sf_write_device_info(uint8_t* data, uint16_t size);
bool nrc_sf_read_device_info(uint8_t* data, uint16_t size);
bool nrc_sf_fota_support(void);
uint32_t nrc_sf_fota_max_fw_size(void);
uint32_t nrc_sf_fota_addr(void);
uint32_t nrc_sf_fota_info_addr(void);
uint32_t nrc_sf_max_flash_size(uint32_t offset);


#define NRC_SF_VERIFY_SLOT_OKAY           0
#define NRC_SF_VERIFY_SLOT_MIN_SIZE_FAIL  1
#define NRC_SF_VERIFY_SLOT_SIGNATURE_FAIL 2
#define NRC_SF_VERIFY_SLOT_OVERSIZE_FAIL  3
#define NRC_SF_VERIFY_SLOT_CHECKSUM_FAIL  4

uint32_t nrc_sf_verify_slot_and_read_data(uint8_t *status, uint32_t address, uint32_t max_length, uint8_t *data_buf);

#ifdef __cplusplus
} // extern "C"
#endif

#endif //__HAL_SFLASH_NRC7292_H__
