#ifndef __NRC_ETH_IF_H__
#define __NRC_ETH_IF_H__

#include "nrc_types.h"
#include "api_spi.h"
#include "lwip/prot/ethernet.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	NRC_ETH_MODE_STA,
	NRC_ETH_MODE_AP,
} nrc_eth_mode_t;

typedef enum {
	NRC_NETWORK_MODE_BRIDGE,
	NRC_NETWORK_MODE_NAT,
} nrc_network_mode_t;

typedef enum {
	NRC_ETH_IP_MODE_DHCP,
	NRC_ETH_IP_MODE_STATIC,
} nrc_eth_ip_mode_t;

void nrc_eth_raw_transmit(uint8_t *buffer, uint32_t length);
void nrc_eth_set_ethernet_mode(nrc_eth_mode_t mode);
nrc_eth_mode_t get_ethernet_mode();
void nrc_eth_set_network_mode(nrc_network_mode_t mode);
nrc_network_mode_t nrc_eth_get_network_mode();
void nrc_eth_set_ip_mode(nrc_eth_ip_mode_t mode);
nrc_eth_ip_mode_t nrc_get_ip_mode();

nrc_err_t ethernet_init(spi_device_t *eth_spi, uint8_t *mac_addr, int gpio_int_pin);
#if defined(SUPPORT_ETHERNET_ACCESSPOINT)
void set_peer_mac(const uint8_t *eth);
struct eth_addr *get_peer_mac(void);
#endif
#endif
