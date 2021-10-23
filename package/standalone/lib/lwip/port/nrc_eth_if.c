#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/dhcp.h"
#include "netif/etharp.h"
#include "nrc_wifi.h"
#include "driver_nrc.h"
#ifdef ETH_DRIVER_ENC28J60
#include "enc28j60.h"
#endif
#ifdef ETH_DRIVER_W5500
#include "w5500.h"
#endif
#include "eth.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"

struct netif eth_netif;

static esp_eth_mac_t *nrc_eth_mac;
extern struct netif* nrc_netif[MAX_IF];

static void print_buffer(uint8_t *buffer, uint32_t size)
{
	int i = 0;
	system_printf("\n");
	system_printf("   0        1        2        3        4        5        6        7        8        9       10       11       12       13       14       15    \n");
	system_printf("-----------------------------------------------------------------------------------------------------------------------------------------------\n");
	for (i = 0; i < size; i++) {
		system_printf("0x%02x (%c) ", buffer[i],
					  ((buffer[i] >= 32) && (buffer[i] <= 126))? buffer[i] : 0x20);
		if ((i % 16) == 15) {
			system_printf("\n");
		}
	}
	system_printf("\n\n");
}

static err_t nrc_eth_output( struct netif *netif, struct pbuf *p )
{
	struct pbuf *q;
	err_t xReturn = ERR_OK;
	uint8_t buffer[ETH_MAX_PACKET_SIZE];
	uint32_t length = 0;

	for( q = p; q != NULL; q = q->next )
	{
		memcpy(buffer + length, q->payload, q->len);
		length += q->len;
	}

	V(TT_NET, "[%s] transmitting length = %d...\n", __func__, length);
//	print_buffer(buffer, length);

	nrc_eth_mac->transmit(nrc_eth_mac, buffer, length);

	LINK_STATS_INC(link.xmit);

	return ERR_OK;
}

static err_t eth_init( struct netif *netif )
{
	/*
	* Initialize the snmp variables and counters inside the struct netif.
	* The last argument should be replaced with your link speed, in units
	* of bits per second.
	*/
	NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, 100);

	netif->name[0] = 'e';
	netif->name[1] = 't';

	/* We directly use etharp_output() here to save a function call.
	* You can instead declare your own function an call etharp_output()
	* from it if you have to do some checks before sending (e.g. if link
	* is available...)
	*/
	#if LWIP_IPV4
	netif->output = etharp_output;
	#endif /* LWIP_IPV4 */
	#if LWIP_IPV6
	netif->output_ip6 = ethip6_output;
	#endif /* LWIP_IPV6 */
	netif->linkoutput = nrc_eth_output;

	/* initialize the hardware */
	/* set MAC hardware address length */
	netif->hwaddr_len = NETIF_MAX_HWADDR_LEN;

	/* maximum transfer unit */
	netif->mtu = 1500;

	/* broadcast capability */
	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

	return ERR_OK;
}

static void status_callback(struct netif *eth_if)
{
	if (netif_is_up(eth_if)) {
		I(TT_NET, "[%s] netif_is_up, local interface IP is %s\n",
					  __func__,
					  ip4addr_ntoa(netif_ip4_addr(eth_if)));
		if (!ip4_addr_isany_val(*netif_ip4_addr(eth_if))) {
			I(TT_NET, "[%s] IP is ready\n", __func__);
		}
	} else {
		I(TT_NET, "[%s] netif_is_down\n", __func__);
	}
}

static void link_callback(struct netif *eth_if)
{
	if (netif_is_link_up(eth_if)) {
		I(TT_NET, "[%s] UP\n", __func__);
	} else {
	    I(TT_NET, "[%s] DOWN\n", __func__);
	}
}

static void nrc_bind_eth_if(esp_eth_mac_t *mac)
{
    ip4_addr_t ipaddr, netmask, gw;
    ip4_addr_set_zero(&gw);
    ip4_addr_set_zero(&ipaddr);
    ip4_addr_set_zero(&netmask);

    eth_netif.num = 2;
    /* save mac as nrc_eth_mac to be used in LWIP nrc_eth_output callback */
    nrc_eth_mac = mac;

    /* set MAC hardware address to be used by lwIP */
    mac->get_addr(mac, eth_netif.hwaddr);
//    netif_add(&eth_netif, &ipaddr, &netmask, &gw, NULL, eth_init, tcpip_input);
    netif_add(&eth_netif, &ipaddr, &netmask, &gw, NULL, eth_init, ethernet_input);
    netif_set_status_callback(&eth_netif, status_callback);
    netif_set_link_callback(&eth_netif, link_callback);

    netif_set_default(&eth_netif);

    /* bring it up */
    netif_set_up(&eth_netif);
}

static nrc_err_t eth_copy_buffer_to_pbuf(uint8_t *buffer, uint32_t length, struct pbuf **p)
{
	struct pbuf *q;
	int remain = length;
	int offset = 0;

	*p = pbuf_alloc(PBUF_RAW, length, PBUF_POOL);
	if (*p == NULL) {
		return NRC_FAIL;
	} else {
		for (q = *p; q != NULL; (q = q->next) && remain) {
			memcpy(q->payload, (uint8_t *) buffer + offset, (remain > q->len) ? q->len : remain);
			remain -= q->len;
			offset += q->len;
		}
	}
	return NRC_SUCCESS;
}

static nrc_err_t eth_stack_input_handler(esp_eth_handle_t eth_handle, uint8_t *buffer, uint32_t length, void *priv)
{
	struct pbuf *p = NULL;
	struct eth_hdr *ethhdr = (struct eth_hdr *) buffer;
	struct etharp_hdr *hdr;
	struct ip_hdr *iphdr;
	ip4_addr_t dipaddr;
	ip4_addr_t sipaddr;
	uint8_t eth_broadcast[ETH_HWADDR_LEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	struct nrc_wpa_if *intf = wpa_driver_get_interface(0);
	struct nrc_wpa_sta *sta = NULL;

	if (intf) {
		V(TT_NET, "[%s] nrc_wpa_if found...\n", __func__);
		if (intf->is_ap) {
			sta = nrc_wpa_find_sta(intf, ethhdr->dest.addr);
			if (sta) {
				V(TT_NET, "[%s] station found ...\n", __func__);
			} else {
				V(TT_NET, "[%s] station not found...\n", __func__);
			}
		} else {
			V(TT_NET, "[%s] nrc_wpa_if not AP...\n", __func__);
		}
	}
	V(TT_NET, "[%s] buffer of size %d received...\n", __func__, length);
//	print_buffer(buffer, length);

	if ((buffer == NULL) || !netif_is_up(&eth_netif)) {
		if (buffer) {
			free(buffer);
		}
		return NRC_FAIL;
	}

	switch (htons(ethhdr->type)) {
		/* IP or ARP packet? */
		case ETHTYPE_ARP:
			hdr = (struct etharp_hdr *) (buffer + sizeof(struct eth_hdr));
			V(TT_NET, "[%s] arp: ip in header 0x%x 0x%x\n",  __func__, hdr->dipaddr.addrw[0], hdr->dipaddr.addrw[1]);
			IPADDR_WORDALIGNED_COPY_TO_IP4_ADDR_T(&dipaddr, &hdr->dipaddr);
			IPADDR_WORDALIGNED_COPY_TO_IP4_ADDR_T(&sipaddr, &hdr->sipaddr);
			V(TT_NET, "[%s] arp: dest ip received %s\n",  __func__, ip4addr_ntoa(&dipaddr));
			V(TT_NET, "[%s] arp: src ip received %s\n",  __func__, ip4addr_ntoa(&sipaddr));
			V(TT_NET, "[%s] arp: my ip addr %s\n", __func__, ip4addr_ntoa(netif_ip4_addr(&eth_netif)));

			if ((memcmp(ethhdr->dest.addr, eth_broadcast, ETH_HWADDR_LEN) == 0)
				 && (memcmp(ethhdr->src.addr, eth_netif.hwaddr, ETH_HWADDR_LEN) == 0)) {
				V(TT_NET, "[%s] arp packet sent by us, dropping...\n", __func__);
			} else if (ip4_addr_cmp(&dipaddr, netif_ip4_addr(&eth_netif))) {
				V(TT_NET, "[%s] arp packet destined to us...\n", __func__);
				if (eth_copy_buffer_to_pbuf(buffer, length, &p) == NRC_FAIL) {
					free(buffer);
					return NRC_FAIL;
				} else {
					eth_netif.input(p, &eth_netif);
				}
			} else {
				if (sta) {
					if (netif_is_up(nrc_netif[SOFTAP_IF])) {
						V(TT_NET, "[%s] arp packet destined to someone else sending to softAP IF...\n", __func__);
						nrc_transmit_from_8023(nrc_netif[SOFTAP_IF]->num, buffer, length);
					}
				}
			}
			break;
#if PPPOE_SUPPORT
		/* PPPoE packet? */
		case ETHTYPE_PPPOEDISC:
		case ETHTYPE_PPPOE:
#endif /* PPPOE_SUPPORT */
		case ETHTYPE_IP:
			iphdr = (struct ip_hdr *) (buffer + sizeof(struct eth_hdr));
			IPADDR_WORDALIGNED_COPY_TO_IP4_ADDR_T(&dipaddr, &iphdr->dest);
			IPADDR_WORDALIGNED_COPY_TO_IP4_ADDR_T(&sipaddr, &iphdr->src);
			V(TT_NET, "[%s] destination address %s\n", __func__, ip4addr_ntoa(&dipaddr));
			V(TT_NET, "[%s] source address %s\n", __func__, ip4addr_ntoa(&sipaddr));
			/* full packet send to tcpip_thread to process */
            if (((memcmp(ethhdr->dest.addr, eth_broadcast, ETH_HWADDR_LEN) == 0)
                 && (memcmp(ethhdr->src.addr, eth_netif.hwaddr, ETH_HWADDR_LEN) == 0)) ||
				(memcmp(ethhdr->dest.addr, eth_netif.hwaddr, ETH_HWADDR_LEN) == 0)) {
				V(TT_NET, "[%s] IP packet received for us...\n", __func__);
				if (eth_copy_buffer_to_pbuf(buffer, length, &p) == NRC_FAIL) {
					free(buffer);
					return NRC_FAIL;
				} else {
					eth_netif.input(p, &eth_netif);
				}
			} else if(ip4_addr_cmp(&sipaddr, netif_ip4_addr(&eth_netif))
					  && (sta == NULL)) {
				V(TT_NET, "[%s] IP packet sent by us received, not sending to softAP IF...\n", __func__);
			} else {
				if (sta) {
					if (netif_is_up(nrc_netif[SOFTAP_IF])) {
						V(TT_NET, "[%s] IP packet received for someone else sending to softAP IF...\n", __func__);
						nrc_transmit_from_8023(nrc_netif[SOFTAP_IF]->num, buffer, length);
					}
				}
			}
			break;

		default:
			V(TT_NET, "[%s] unknown packet...\n", __func__);
			break;
	}
	V(TT_NET, "[%s] free buffer...\n", __func__);
	free(buffer);
	return NRC_SUCCESS;
}

static nrc_err_t eth_linkup_handler(esp_eth_handle_t eth_handle)
{
	I(TT_NET, "[%s] ethernet link detected...\n", __func__);
	netif_set_link_up(&eth_netif);
	dhcp_start(&eth_netif);
	return NRC_SUCCESS;
}

static nrc_err_t eth_linkdown_handler(esp_eth_handle_t eth_handle)
{
	I(TT_NET, "[%s] ethernet disconnected...\n", __func__);
	dhcp_stop(&eth_netif);
	dhcp_cleanup(&eth_netif);

	ip4_addr_set_zero(&eth_netif.ip_addr);
	ip4_addr_set_zero(&eth_netif.netmask);
	ip4_addr_set_zero(&eth_netif.gw);

	netif_set_link_down(&eth_netif);
	return NRC_SUCCESS;
}

void nrc_eth_raw_transmit(uint8_t *buffer, uint32_t length)
{
	V(TT_NET, "[%s] transmitting length [%d]...\n", __func__, length);
	nrc_eth_mac->transmit(nrc_eth_mac, buffer, length);
}

nrc_err_t ethernet_init(void)
{
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    mac_config.smi_mdc_gpio_num = -1;
    mac_config.smi_mdio_gpio_num = -1;
#ifdef ETH_DRIVER_ENC28J60
    esp_eth_mac_t *mac = esp_eth_mac_new_enc28j60(&mac_config);
#endif
#ifdef ETH_DRIVER_W5500
	esp_eth_mac_t *mac = esp_eth_mac_new_w5500(&mac_config);
#endif
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.autonego_timeout_ms = 0; // ENC28J60 doesn't support auto-negotiation
    phy_config.reset_gpio_num = -1; // ENC28J60 doesn't have a pin to reset internal PHY
#ifdef ETH_DRIVER_ENC28J60
    esp_eth_phy_t *phy = esp_eth_phy_new_enc28j60(&phy_config);
#endif
#ifdef ETH_DRIVER_W5500
	esp_eth_phy_t *phy = esp_eth_phy_new_w5500(&phy_config);
#endif
    esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
    eth_config.stack_input = eth_stack_input_handler;
    eth_config.on_linkup = eth_linkup_handler;
    eth_config.on_linkdown = eth_linkdown_handler;
    esp_eth_handle_t eth_handle = NULL;

    if (esp_eth_driver_install(&eth_config, &eth_handle) != NRC_SUCCESS) {
		nrc_usr_print("[%s] Error installing ethernet driver...\n", __func__);
		return NRC_FAIL;
	}

    /* ENC28J60 doesn't burn any factory MAC address, we need to set it manually.
       02:00:00 is a Locally Administered OUI range so should not be used except when testing on a LAN under your control.
    */
    mac->set_addr(mac, (uint8_t[]) {
#ifdef ETH_DRIVER_ENC28J60
        0x02, 0x00, 0x00, 0x12, 0x28, 0x60
#else
        0x02, 0x00, 0x00, 0x12, 0x55, 0x00
#endif
    });
    /* set ethernet interface in promiscuous mode to act as bridge */
	mac->set_promiscuous(mac, true);

    /* attach Ethernet driver to TCP/IP stack */
    nrc_bind_eth_if(mac);

    /* start Ethernet driver state machine */
	if (esp_eth_start(eth_handle) != NRC_SUCCESS) {
		nrc_usr_print("[%s] Error starting ethernet...\n", __func__);
		return NRC_FAIL;
	}
	return NRC_SUCCESS;
}
