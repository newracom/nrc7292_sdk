#ifndef __NRC_SERIAL_FLASH_W26XX_H__
#define __NRC_SERIAL_FLASH_W26XX_H__

#include "system.h"

#define W26XX_BUSY  (BIT0)
#define W26XX_WEL   (BIT1)
#define W26XX_WSE   (BIT2)
#define W26XX_WSP   (BIT3)
#define W26XX_WPLD  (BIT4)
#define W26XX_SEC   (BIT5)
#define W26XX_RES   (BIT6)

void nrc_sf_w26xx_init(uint32_t flash_id, bool read_only, bool quad);
void nrc_sf_w26xx_deinit(void);
bool nrc_sf_w26xx_erase(sf_info_t *sfi, uint32_t address, size_t size);
bool nrc_sf_w26xx_erase_entire(void);
bool nrc_sf_w26xx_protect(bool enable);
uint32_t nrc_sf_w26xx_read(uint32_t address, uint8_t *buffer, size_t size);
uint32_t nrc_sf_w26xx_write(uint32_t address, uint8_t *buffer, size_t size);
bool nrc_sf_w26xx_power_down(bool enable);
uint32_t nrc_sf_w26xx_read_id(void);
uint32_t nrc_sf_w26xx_read_jedec(void);


#endif //__NRC_SERIAL_FLASH_W26XX_H__
