#ifndef UTIL_FOTA_H
#define UTIL_FOTA_H

typedef struct {
	uint32_t fw_length;
	uint32_t crc;
	uint32_t ready;
} fota_info_t;

#define FOTA_READY 0x1234ABCD
#define BACKUP_BUF_SIZE 4096
#define BOOTLOAD_VEC_OFFSET 8

bool util_fota_is_support(void);
uint32_t util_fota_fw_addr (void);
uint32_t util_fota_fw_info_addr (void);
uint32_t util_fota_fw_max_size (void);
void util_fota_write(uint32_t dst, uint8_t *src, uint32_t len);
void util_fota_erase(uint32_t dst, uint32_t len);
void util_fota_erase_info(void);
void util_fota_set_info(uint32_t len, uint32_t crc);
void util_fota_set_ready(bool ready);
bool util_fota_is_ready(void);
int util_fota_update_firmware(fota_info_t* fw_info);
void util_fota_reboot_firmware(void);
int util_fota_update_done(void);
void util_fota_update_done_bootloader(fota_info_t* fw_info);
int util_fota_update_run(fota_info_t* fw_info);
uint32_t util_fota_cal_crc(uint8_t* data, uint32_t len);
uint32_t util_fota_get_max_fw_size(void);
void clear_fota_ready(void);
#endif /* UTIL_FOTA_H */
