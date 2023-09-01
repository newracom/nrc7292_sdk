#ifndef UTIL_BCAST_FOTA_H
#define UTIL_BCAST_FOTA_H

#include "util_version.h"

void bcast_fota_enqueue_ready(int cmd, uint8_t* data);
void bcast_fota_enqueue_write(int cmd, uint8_t* data, uint16_t len);
void bcast_fota_init(void);
uint8_t bcast_fota_get_mode(void);
void bcast_fota_set_mode(uint8_t mode);
bool bcast_fota_get_delayed_reboot(void);
void bcast_fota_set_delayed_reboot(bool reboot);
bool bcast_fota_get_enable(void);
void bcast_fota_set_enable(bool enable);
bool bcast_fota_get_fast(void);
uint16_t bcast_fota_get_bcn_intv(void);
#if defined(INCLUDE_ESL_PARAM)
const char* bcast_fota_get_mqtt_server_topic();
void bcast_fota_set_mqtt_server_topic(const char* topic, int length);
#endif

enum bcast_fota_mode {
	BC_FOTA_MODE_ANY = 1,
	BC_FOTA_MODE_CONNECTED = 2
};
typedef struct {
	uint8_t mode; /* 1: FOTA operates any status.
				2: FOTA operates in connected status only. */
	bool delayed_reboot;
	bool enable;
} bcast_fota_config_t;

typedef struct {
	uint32_t enable;
	uint32_t chip_id;
	char app_name[34];
	version_t fw_version;
	version_t app_version;
	uint32_t crc;
	uint32_t total_size;
	uint32_t total_chunk;
	uint32_t retry_cnt;
	uint32_t force;
} bcast_fota_info_t;

typedef struct {
	int cmd;
	uint8_t* data;
	uint16_t len;
} bcast_fota_msg_t;

#endif /* UTIL_BCAST_FOTA_H */
