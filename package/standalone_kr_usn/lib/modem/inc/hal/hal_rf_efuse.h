#ifndef __NRC_RF_EFUSE_H__
#define __NRC_RF_EFUSE_H__

bool hal_rf_efuse_process();
bool hal_rf_efuse_get_pllldo12_tr(uint32_t *value);
int hal_rf_efuse_get_version();

#endif// __NRC_RF_EFUSE_H__
