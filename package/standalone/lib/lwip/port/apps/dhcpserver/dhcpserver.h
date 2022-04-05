#ifndef __DHCPS_H__
#define __DHCPS_H__

#define USE_DNS

#include "nrc_wifi.h"
#include "lwip/ip_addr.h"
#if 0
struct ip_info {
	ip_addr_t ip;      /**< IP address */
	ip_addr_t netmask; /**< netmask */
	ip_addr_t gw;      /**< gateway */
};
#endif
typedef struct dhcps_state {
	s16_t state;
} dhcps_state;


typedef struct dhcps_msg {
	u8_t op, htype, hlen, hops;
	u8_t xid[4];
	u16_t secs, flags;
	u8_t ciaddr[4];
	u8_t yiaddr[4];
	u8_t siaddr[4];
	u8_t giaddr[4];
	u8_t chaddr[16];
	u8_t sname[64];
	u8_t file[128];
	u8_t options[312];
} dhcps_msg;

#ifndef LWIP_OPEN_SRC
struct dhcps_lease {
	bool enable;
	ip_addr_t start_ip;
	ip_addr_t end_ip;
};

enum dhcps_offer_option {
	OFFER_START = 0x00,
	OFFER_ROUTER = 0x01,
	OFFER_END
};
#endif

struct dhcps_pool {
	ip_addr_t ip;
	u8_t mac[6];
	u32_t lease_timer;
};

typedef struct _list_node {
	void* pnode;
	struct _list_node* pnext;
} list_node;

extern u32_t dhcps_lease_time;
#define DHCPS_COARSE_TIMER_SECS  1
/** period (in milliseconds) of the application calling dhcps_coarse_tmr() */
#define DHCPS_COARSE_TIMER_MSECS (DHCPS_COARSE_TIMER_SECS * 1000UL)

#define DHCPS_LEASE_TIMER  dhcps_lease_time  //0x05A0
#define DHCPS_MAX_LEASE 0x64
#define BOOTP_BROADCAST 0x8000

#define DHCP_REPLY          2
#define DHCP_HTYPE_ETHERNET 1
#define DHCP_HLEN_ETHERNET  6
#define DHCP_MSG_LEN      236

#define DHCPS_SERVER_PORT  67
#define DHCPS_CLIENT_PORT  68

#define DHCPDISCOVER  1
#define DHCPOFFER     2
#define DHCPREQUEST   3
#define DHCPDECLINE   4
#define DHCPACK       5
#define DHCPNAK       6
#define DHCPRELEASE   7

#define DHCP_OPTION_SUBNET_MASK   1
#define DHCP_OPTION_ROUTER        3
#define DHCP_OPTION_DNS_SERVER    6
#define DHCP_OPTION_REQ_IPADDR   50
#define DHCP_OPTION_LEASE_TIME   51
#define DHCP_OPTION_MSG_TYPE     53
#define DHCP_OPTION_SERVER_ID    54
#define DHCP_OPTION_INTERFACE_MTU 26
#define DHCP_OPTION_PERFORM_ROUTER_DISCOVERY 31
#define DHCP_OPTION_BROADCAST_ADDRESS 28
#define DHCP_OPTION_REQ_LIST     55
#define DHCP_OPTION_END         255

#define MAX_STATION_NUM      20

#define DHCPS_STATE_OFFER 1
#define DHCPS_STATE_DECLINE 2
#define DHCPS_STATE_ACK 3
#define DHCPS_STATE_NAK 4
#define DHCPS_STATE_IDLE 5
#define DHCPS_STATE_RELEASE 6

#define   dhcps_router_enabled(offer)	((offer & OFFER_ROUTER) != 0)

void dhcps_start(struct ip_info* info, struct netif *net_if);
void dhcps_stop(void);

void dhcps_coarse_tmr(void);

int dhcps_status(void);
struct netif *dhcps_get_interface(void);
bool wifi_softap_set_dhcps_lease_time(u32_t minute);
bool wifi_softap_reset_dhcps_lease_time(void);
u32_t wifi_softap_get_dhcps_lease_time(void); // minute
#endif

