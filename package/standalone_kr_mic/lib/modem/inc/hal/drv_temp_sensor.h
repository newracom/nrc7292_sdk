#ifndef __DRV_TEMP_SENSOR_H__
#define __DRV_TEMP_SENSOR_H__

#define TEMP_SENSOR_I2C_CHANNEL I2C_3
#define TEMP_SENSOR_I2C_CLOCK	400000
#define TEMP_SENSOR_I2C_WIDTH	I2C_8BIT
#define TEMP_SENSOR_I2C_CLOCK_SOURCE	0 /* 0:clock controller, 1:PCLK */

#ifndef TEMP_SENSOR_DEFAULT_SCL
#define TEMP_SENSOR_DEFAULT_SCL		31
#endif

#ifndef TEMP_SENSOR_DEFAULT_SDA
#define TEMP_SENSOR_DEFAULT_SDA		30
#endif

#define TMP103B_SLAVE_ADDRESS	0x71
#define MAX31875R0_SLAVE_ADDRESS	0x48
#define MAX31875R1_SLAVE_ADDRESS	0x49
#define MAX31875R2_SLAVE_ADDRESS	0x4A
#define MAX31875R3_SLAVE_ADDRESS	0x4B
#define MAX31875R4_SLAVE_ADDRESS	0x4C
#define MAX31875R5_SLAVE_ADDRESS	0x4D
#define MAX31875R6_SLAVE_ADDRESS	0x4E
#define MAX31875R7_SLAVE_ADDRESS	0x4F

typedef struct {
	int8_t temperature;
	int16_t delta;
} TEMP_DELTA_TBL;

#define TEMP_TABLE_MAX 14
static const TEMP_DELTA_TBL tdelta_tbl[TEMP_TABLE_MAX] = {
	//temperature, delta*100
	{-45, 250},
	{-35, 210},
	{-25, 190},
	{-15, 150},
	{-5, 120},
	{5, 80},
	{15, 40},
	{25, 0},
	{35, -40},
	{45, -100},
	{55, -160},
	{65, -210},
	{75, -270},
	{85, -310},
};

enum i2c_temp_sensor_type {
	TEMP_SENSOR_TYPE_TMP103B,
	TEMP_SENSOR_TYPE_MAX31875R0,
	TEMP_SENSOR_TYPE_MAX31875R1,
	TEMP_SENSOR_TYPE_MAX31875R2,
	TEMP_SENSOR_TYPE_MAX31875R3,
	TEMP_SENSOR_TYPE_MAX31875R4,
	TEMP_SENSOR_TYPE_MAX31875R5,
	TEMP_SENSOR_TYPE_MAX31875R6,
	TEMP_SENSOR_TYPE_MAX31875R7,
	TEMP_SENSOR_TYPE_MAX
};

typedef struct _temp_sensor {
	const char *name;
	int index;
	uint8_t  slave_address;
	uint8_t value_on;
	uint8_t value_off;
} temp_sensor_t;

static temp_sensor_t temp_sensor[TEMP_SENSOR_TYPE_MAX] = {
	{ "TMP310B",	TEMP_SENSOR_TYPE_TMP103B,	TMP103B_SLAVE_ADDRESS, 0x2, 0x0},
	{ "MAX31875R0",	TEMP_SENSOR_TYPE_MAX31875R0,	MAX31875R0_SLAVE_ADDRESS, 0x0, 0x1},
	{ "MAX31875R1",	TEMP_SENSOR_TYPE_MAX31875R1,	MAX31875R1_SLAVE_ADDRESS, 0x0, 0x1},
	{ "MAX31875R2",	TEMP_SENSOR_TYPE_MAX31875R2,	MAX31875R2_SLAVE_ADDRESS, 0x0, 0x1},
	{ "MAX31875R3",	TEMP_SENSOR_TYPE_MAX31875R3,	MAX31875R3_SLAVE_ADDRESS, 0x0, 0x1},
	{ "MAX31875R4",	TEMP_SENSOR_TYPE_MAX31875R4,	MAX31875R4_SLAVE_ADDRESS, 0x0, 0x1},
	{ "MAX31875R5",	TEMP_SENSOR_TYPE_MAX31875R5,	MAX31875R5_SLAVE_ADDRESS, 0x0, 0x1},
	{ "MAX31875R6",	TEMP_SENSOR_TYPE_MAX31875R6,	MAX31875R6_SLAVE_ADDRESS, 0x0, 0x1},
	{ "MAX31875R7",	TEMP_SENSOR_TYPE_MAX31875R7,	MAX31875R7_SLAVE_ADDRESS, 0x0, 0x1}
};

bool nrc_check_temp_sensor_exist(void);
bool nrc_get_temp_sensor_register(uint32_t id, uint8_t dev, uint8_t reg, int8_t *value);
bool nrc_set_temp_sensor_register(uint32_t id, uint8_t dev, uint8_t reg, int8_t value);
uint8_t nrc_get_temp_sensor_address(void);
uint8_t nrc_get_temp_sensor_value_on(void);
uint8_t nrc_get_temp_sensor_value_off(void);
int16_t nrc_calculate_temp_sensor_delta(void);
void nrc_temp_sensor_init(uint8_t scl, uint8_t sda);

#endif //__DRV_TEMP_SENSOR_H__
