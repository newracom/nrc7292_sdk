#ifndef __DRV_EEPROM_H__
#define __DRV_EEPROM_H__

#define EEPROM_I2C_CHANNEL			(0)
#define EEPROM_I2C_CLOCK			(400000)
#define EEPROM_I2C_WIDTH			(I2C_8BIT)
#define EEPROM_I2C_CLOCK_SOURCE		(0) /* 0:clock controller, 1:PCLK */

#ifndef EEPROM_DEFAULT_SCL
#ifdef NRC7394
#define EEPROM_DEFAULT_SCL			(6)
#else
#define EEPROM_DEFAULT_SCL			(30)
#endif
#endif

#ifndef EEPROM_DEFAULT_SDA
#ifdef NRC7394
#define EEPROM_DEFAULT_SDA			(7)
#else
#define EEPROM_DEFAULT_SDA			(31)
#endif
#endif

#define CAT24C64_SLAVE_ADDRESS		(0xA0)
#define EEPROM24FC256_SLAVE_ADDRESS		(0xAE)

#define EEPROM_SLAVE_ADDRESS EEPROM24FC256_SLAVE_ADDRESS

enum i2c_eeprom_type {
	EEPROM_TYPE_CAT24C64,
	EEPROM_TYPE_MAX
};

typedef struct _eeprom {
	const char *name;
	int index;
	uint8_t  slave_address;
} eeprom_t;

//#define EEPROM_VALIDATION_CHECK_ADDRESS 0x1FFFF
#define EEPROM_SIZE 4096
#define EEPROM_VALIDATION_CHECK_ADDRESS (4096-4)

static eeprom_t eeprom_list[EEPROM_TYPE_MAX] = {
	/**
	 * The CAT24C64 is a 64Kb CMOS Serial EEPROM device,
	 * internally orgranized as 8192 words of 8 bits each.
	 * [Address] : 0x0000(0) ~ 0x1FFF(8191)
	 */
	{ "CAT24C64", EEPROM_TYPE_CAT24C64, CAT24C64_SLAVE_ADDRESS}
};

bool nrc_eeprom_read_data(uint32_t id, uint16_t start_address, uint8_t *data, uint16_t length);
bool nrc_eeprom_write_data(uint32_t id, uint16_t start_address, uint8_t *data, uint16_t length);
bool nrc_eeprom_clear_data(uint32_t id, uint16_t start_address, uint16_t length);
bool nrc_eeprom_update_cal_use(uint8_t cal_use);
bool nrc_eeprom_update_macaddr(uint8_t *macaddr, uint8_t interface);
bool nrc_eeprom_get_macaddr(uint8_t *macaddr, uint8_t interface);
uint16_t nrc_eeprom_get_hw_version(void);
bool nrc_eeprom_update_hw_version(uint16_t version);
void nrc_eeprom_init(void);

#endif //__DRV_EEPROM_H__
