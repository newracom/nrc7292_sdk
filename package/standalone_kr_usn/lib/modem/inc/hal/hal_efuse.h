#ifndef __HAL_EFUSE_NRC7292_H__
#define __HAL_EFUSE_NRC7292_H__

#include "system.h"

void nrc_efuse_init(void);
void nrc_efuse_deinit(void);
uint8_t nrc_efuse_readb(uint32_t addr);
uint32_t nrc_efuse_readw(uint32_t addr);
#endif //__HAL_EFUSE_NRC7292_H__
