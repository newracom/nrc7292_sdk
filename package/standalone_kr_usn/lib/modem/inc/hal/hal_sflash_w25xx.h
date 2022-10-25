#ifndef __NRC_SERIAL_FLASH_W25XX_H__
#define __NRC_SERIAL_FLASH_W25XX_H__

#include "system.h"

#define W25XX_BUSY  (BIT0)
#define W25XX_WEL   (BIT1)
#define W25XX_BP0   (BIT2)
#define W25XX_BP1   (BIT3)
#define W25XX_BP2   (BIT4)
#define W25XX_BP3   (BIT5)
#define W25XX_QE    (BIT6)
#define W25XX_SRWD  (BIT7)

#define W25XX_BP_MASK (BIT2|BIT3|BIT4|BIT5)

enum tbp_level_e {
    BP_LEVEL0,          /* Full Memory Array unprotected */
    BP_TOP_LEVEL1,      /* 1/16 Top Memory Block protected */
    BP_TOP_LEVEL2,      /* 1/8 Top Memory Block protected */
    BP_TOP_LEVEL3,      /* 1/4 Top Memory Block protected */
    BP_TOP_LEVEL4,      /* 1/2 Top Memory Block protected */
    BP_BOTTOM_LEVEL1,   /* 1/16 Bottom Memory Block protected */
    BP_BOTTOM_LEVEL2,   /* 1/8 Bottom Memory Block protected */
    BP_BOTTOM_LEVEL3,   /* 1/4 Bottom Memory Block protected */
    BP_BOTTOM_LEVEL4,   /* 1/2 Bottom Memory Block protected */
    BP_LEVEL5,          /* Full Memory Array protected */
};

void nrc_sf_w25xx_init(uint32_t flash_id, bool read_only, bool quad);
void nrc_sf_w25xx_deinit(void);
bool nrc_sf_w25xx_erase(sf_info_t *sfi, uint32_t address, size_t size);
bool nrc_sf_w25xx_erase_entire(void);
bool nrc_sf_w25xx_protect(bool enable);
uint32_t nrc_sf_w25xx_read(uint32_t address, uint8_t *buffer, size_t size);
uint32_t nrc_sf_w25xx_write(uint32_t address, uint8_t *buffer, size_t size);
bool nrc_sf_w25xx_power_down(bool enable);
uint32_t nrc_sf_w25xx_read_id(void);
uint32_t nrc_sf_w25xx_read_jedec(void);


#endif //__NRC_SERIAL_FLASH_W25XX_H__
