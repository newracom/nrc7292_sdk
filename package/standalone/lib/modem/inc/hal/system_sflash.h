#ifndef __SYSTEM_SFLASH_H___
#define __SYSTEM_SFLASH_H___

#define SFC_MEM_BASE_ADDR (0x11000000)

#define SF_SYSTEM_CONFIG 		system_sfc_get_sysconfig_address()
#define SF_SYSTEM_CONFIG_4MB 	0x3FC000
#define SF_SYSTEM_CONFIG_2MB 	0xFE000
#define SF_RF_CAL				system_sfc_get_rf_cal_address()
#define SF_RF_CAL_4MB 			0x3FD000
#define SF_RF_CAL_2MB 			0xFF000
#define SF_FOTA					system_sfc_get_fota_address()
#define SF_FOTA_2MB				0x100000
#define SF_FOTA_4MB				0x1F5000
#define SF_FOTA_INFO			system_sfc_get_fota_info_address()
#define SF_FOTA_INFO_2MB		0x1FF000
#define SF_FOTA_INFO_4MB		0x3FF000
#define SF_CORE_DUMP			system_sfc_get_core_dump_address()
#define SF_CORE_DUMP_2MB		0xF6000
#define SF_CORE_DUMP_4MB		0x3F3000
#define SF_FW_INFO			system_sfc_get_fw_info_address()
#define SF_FW_INFO_2MB		0xF5000
#define SF_FW_INFO_4MB		0x3FE000
#define SF_USER_CONFIG_1		system_sfc_get_user_config1_address()
#define SF_USER_CONFIG_1_2MB		0xFA000
#define SF_USER_CONFIG_1_4MB		0x3F7000
#define SF_FW			0x10000

extern uint32_t sf_size;

#define SF_SLOT_DATA_OFFSET 12
#define SF_SLOT_CRC_OFFSET 8
#define SF_SLOT_MAX_LEN 4084 /* 4KB - size of Header */

#define SIZE_OF_TX_PWR_TBL      64
#define SIZE_OF_FREQ_DELTA_TBL  64
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

void 		system_sfc_init(void);
void 		system_sfc_init_sam(void);
/* SFC DRIVER */
void 		system_sfc_set_size(uint32_t id);
uint32_t 	system_sfc_get_sysconfig_address(void);
uint32_t 	system_sfc_get_rf_cal_address(void);
uint32_t 	system_sfc_get_fota_address(void);
uint32_t 	system_sfc_get_fota_info_address(void);
uint32_t	system_sfc_get_fw_info_address(void);
uint32_t	system_sfc_get_core_dump_address(void);
#ifdef __cplusplus
extern "C" {
uint32_t	system_sfc_get_user_config1_address(void);
} // extern "C"
#else
uint32_t	system_sfc_get_user_config1_address(void);
#endif
void 		system_sfc_cmd(uint8_t cmd);
void 		system_sfc_erase_sector(uint32_t addr);
void 		system_sfc_erase_block(uint32_t addr);
void 		system_sfc_erase_all();
void		system_sfc_set_status(uint8_t status);
uint32_t 	system_sfc_write_mem(uint32_t offset, uint8_t *wbuffer, size_t size);
uint32_t 	system_sfc_read_mem(uint32_t offset, uint8_t *rbuffer, size_t size);

/* HAL SFLASH */
void 		hal_sf_erase(uint32_t address, size_t size);
void 		hal_sf_erase_entire(void);
void 		hal_sf_read(uint32_t address, uint8_t *buffer, size_t size);
void		hal_sf_erase_and_write(uint32_t address, uint8_t *buffer, size_t size);
void 		hal_sf_write(uint32_t address, uint8_t *buffer, size_t size);
void 		hal_sf_set_slot_sig(uint32_t* signature);
bool 		hal_sf_check_slot_sig(uint32_t* signature);
bool 		hal_sf_update_slot(uint32_t address, uint8_t *data, size_t size);
bool 		hal_sf_read_slot_info(uint32_t address, sf_slot_t *data);
bool 		hal_sf_read_slot_data(uint32_t address, size_t size, uint8_t *buffer);
bool 		hal_sf_get_flash_sector_data(uint32_t address, uint8_t *buffer, size_t size);

/* ----------------------------------
 * W25XX JEDEC ID
 * ---------------------------------- */
#define W25XX_JEDEC_ID (0x621614)
#define IS25WP080D_JEDEC_ID (0x9D7014)
#define XT25Q08B_JEDEC_ID (0x0B6014)
#define XT25Q16D_JEDEC_ID (0x0B6015)
#define XT25W16F_JEDEC_ID (0x0B6515)
#define IS25LP080D_JEDEC_ID (0x9D6014)
#define IS25LP128D_JEDEC_ID (0x9D6018)
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
#define EN25QW32A_JEDEC_ID (0x1C6116)
#define EN25S32A_JEDEC_ID (0x1C3816)
#define EN25SE32A_JEDEC_ID (0x1C4816)
#define XT25Q32F_JEDEC_ID (0x0B6016)
#define P25Q32U_JEDEC_ID (0x856016)
#define P25Q16U_JEDEC_ID (0x856015)
#define UC25WQ08_JEDEC_ID (0xB36014)
#define UC25WQ16_JEDEC_ID (0xB36015)
#define UC25WQ32_JEDEC_ID (0xB36016)
#define GT25Q16A_JEDEC_ID (0xC46015)
#define GT25Q32A_JEDEC_ID (0xC46016)
#define FM25W16A_JEDEC_ID (0xA12815)

/* ----------------------------------
 * W26XX JEDEC ID
 * ---------------------------------- */
#define W26WF080B_JEDEC_ID (0xBF2658)
#define W26WF040B_JEDEC_ID (0xBF2654)
#endif
