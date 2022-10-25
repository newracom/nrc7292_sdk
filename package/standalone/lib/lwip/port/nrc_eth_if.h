#ifndef __NRC_ETH_IF_H__
#define __NRC_ETH_IF_H__

#include "nrc_types.h"
#include "lwip/prot/ethernet.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	NRC_ETH_MODE_AP,
	NRC_ETH_MODE_STA,
} nrc_eth_mode_t;

typedef enum {
	NRC_NETWORK_MODE_BRIDGE,
	NRC_NETWORK_MODE_NAT,
} nrc_network_mode_t;

void nrc_eth_raw_transmit(uint8_t *buffer, uint32_t length);
nrc_err_t set_ethernet_mode(nrc_eth_mode_t mode);
nrc_eth_mode_t get_ethernet_mode();
nrc_err_t set_network_mode(nrc_network_mode_t mode);
nrc_network_mode_t get_network_mode();
nrc_err_t ethernet_init(uint8_t *mac_addr);
#if defined(SUPPORT_ETHERNET_ACCESSPOINT)
void set_peer_mac(const uint8_t *eth);
struct eth_addr *get_peer_mac(void);
#endif
#endif
