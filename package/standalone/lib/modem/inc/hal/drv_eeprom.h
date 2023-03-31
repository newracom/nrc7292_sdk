#ifndef __DRV_EEPROM_H__
#define __DRV_EEPROM_H__

#define EEPROM_I2C_CHANNEL			(I2C_2)
#define EEPROM_I2C_CLOCK			(400000)
#define EEPROM_I2C_WIDTH			(I2C_8BIT)
#define EEPROM_I2C_CLOCK_SOURCE		(0) /* 0:clock controller, 1:PCLK */

#if 1
#ifndef EEPROM_DEFAULT_SCL
#define EEPROM_DEFAULT_SCL			(30)
#endif

#ifndef EEPROM_DEFAULT_SDA
#define EEPROM_DEFAULT_SDA			(31)
#endif
#else
#ifndef EEPROM_DEFAULT_SCL
#define EEPROM_DEFAULT_SCL			(17)
#endif

#ifndef EEPROM_DEFAULT_SDA
#define EEPROM_DEFAULT_SDA			(16)
#endif
#endif

#define CAT24C64_SLAVE_ADDRESS		(0xA0)

enum i2c_eeprom_type {
	EEPROM_TYPE_CAT24C64,
	EEPROM_TYPE_MAX
};

typedef struct _eeprom {
	const char *name;
	int index;
	uint8_t  slave_address;
} eeprom_t;

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
