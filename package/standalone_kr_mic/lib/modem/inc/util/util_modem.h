#ifndef UTIL_MODEM_H
#define UTIL_MODEM_H

#include "system.h"

uint32_t ff_log2(uint32_t v);
uint32_t ff_log10(uint32_t v);
uint32_t util_modem_compute_snr(uint32_t signal, uint32_t noise);
uint32_t util_modem_compute_snr_i(uint32_t signal, uint32_t noise);
uint32_t log10_i(uint32_t v);
int ff_10log10(uint32_t v);

void util_modem_init_pn9(void);
uint8_t util_modem_get_pn9_byte(void);

#endif
