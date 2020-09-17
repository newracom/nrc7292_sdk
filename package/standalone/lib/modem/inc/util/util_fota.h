#ifndef UTIL_FOTA_H
#define UTIL_FOTA_H

typedef struct {
	uint32_t fw_length;
	uint32_t crc;
} fota_info_t;

#define BACKUP_BUF_SIZE 4096
#define BOOTLOAD_VEC_OFFSET 8

void util_fota_write(uint32_t dst, uint8_t *src, uint32_t len);
void util_fota_erase(uint32_t dst, uint32_t len);
void util_fota_set_info(uint32_t len, uint32_t crc);
void util_fota_update_done(fota_info_t* fw_info);
uint32_t util_fota_cal_crc(uint8_t* data, uint32_t len);
#endif /* UTIL_FOTA_H */
