#ifndef __NRC_ETH_IF_H__
#define __NRC_ETH_IF_H__

#ifdef __cplusplus
extern "C" {
#endif

void nrc_eth_raw_transmit(uint8_t *buffer, uint32_t length);
nrc_err_t ethernet_init(void);

#endif
