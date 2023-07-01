#include "lwip/sys.h"
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/mem.h"
#include "netif/wlif.h"
#if LWIP_IPV6
#include "lwip/nd6.h"
#endif
#if LWIP_IPV4 && LWIP_DHCPS
#include "dhcpserver/dhcpserver.h"
#endif /* LWIP_IPV4 && LWIP_DHCPS */
#include "nrc_lwip.h"
#if LWIP_DNS && LWIP_DHCPS
#include "captdns.h"
#endif

#include "standalone.h"

#ifdef SUPPORT_ETHERNET_ACCESSPOINT
#include "nrc_eth_if.h"

extern struct netif br_netif;
extern struct netif eth_netif;
#endif /* SUPPORT_ETHERNET_ACCESSPOINT */

struct netif* nrc_netif[MAX_IF];
char* hostname;
bool default_hostname;
static bool dhcpc_start_flag[MAX_IF] = {0, };
struct ip_info ipinfo[MAX_IF];

bool g_flag_dhcp_ok = false;
u8_t wifi_mode;

#define NETIF_ADDRS &ipaddr, &netmask, &gw

#define DEFAULT_NET_IF WLAN0_INTERFACE

typedef struct wifi_ip_mode
{
	uint8_t dhcp_enabled : 1;		/* 0: Static IP, 1:Dynamic IP */
	uint8_t dhcp_running : 1;		/* DHCP running status */
	uint8_t role :2;			/* 0: STA, 1:AP */
	struct ip_info ip_info;
}wifi_ip_mode_t;

wifi_ip_mode_t s_wifi_ip_mode[MAX_IF];

static const char *module_name()
{
	return "net: ";
}
#if LWIP_IPV4 && LWIP_DHCP
int dhcp_run(int vif)
{
	if (vif >= MAX_IF) {
		E(TT_NET, "%s%d incorrect vif\n",
			module_name(), vif);
		return -1;
	}
	I(TT_NET, "%s%d dhcp_run\n",
		module_name(), vif);

	if (set_standalone_hook_dhcp(vif) == 0)
		return 0;

	wifi_station_dhcpc_start(vif);

	return 0;
}

int static_run(int vif)
{
	if (vif >= MAX_IF) {
		E(TT_NET, "%s%d incorrect vif\n",
			module_name(), vif);
		return -1;
	}
	I(TT_NET, "%s%d static_run\n",
		module_name(), vif);

	if (set_standalone_hook_static(vif) == 0)
		return 0;

	return 0;
}

int wifi_station_dhcpc_start(int vif)
{
#if LWIP_IPV4 && LWIP_DHCP
	int8_t ret;
	struct netif *target_if;

#ifdef SUPPORT_ETHERNET_ACCESSPOINT
	if (nrc_eth_get_network_mode() == NRC_NETWORK_MODE_BRIDGE) {
		target_if = &br_netif;
	} else {
		target_if = nrc_netif[vif];
	}
#else
	target_if = nrc_netif[vif];
#endif /* SUPPORT_ETHERNET_ACCESSPOINT */

	if(wifi_station_dhcpc_status(vif)){
		dhcp_stop(target_if);
		dhcp_cleanup(target_if);
		dhcpc_start_flag[vif] = false;

		ip_addr_set_zero(&target_if->ip_addr);
		ip_addr_set_zero(&target_if->netmask);
		ip_addr_set_zero(&target_if->gw);
	}

	ret = dhcp_start(target_if);
	if(!ret){
		dhcpc_start_flag[vif] = true;
	}else{
		dhcpc_start_flag[vif] = false;
	}
	I(TT_NET, "%s%d dhcp client start, ret:%d\n",
		module_name(), vif, ret);
    return true;
#else
	return false;
#endif /* LWIP_IPV4 && LWIP_DHCP */
}

int wifi_station_dhcpc_stop(int vif)
{
	if (wifi_station_dhcpc_status(vif) == true) {
		dhcp_stop(nrc_netif[vif]);
	}
	dhcpc_start_flag[vif] = false;
	I(TT_NET, "%s%d stop dhcp client\n",
		module_name(), vif);
	return true;
}

int wifi_station_dhcpc_status(int vif)
{
    return dhcpc_start_flag[vif];
}
#endif
void reset_ip_address(int vif)
{
	int i;
	I(TT_NET, "%s%d reset_ip_address after disassociation\n",
		module_name(), vif);
#if LWIP_IPV4
	dhcp_stop(nrc_netif[vif]);
	dhcp_cleanup(nrc_netif[vif]);
	dhcpc_start_flag[vif] = false;
	ip_addr_set_zero(&nrc_netif[vif]->ip_addr);
	ip_addr_set_zero(&nrc_netif[vif]->netmask);
	ip_addr_set_zero(&nrc_netif[vif]->gw);
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
	for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
		ip_addr_set_zero_ip6(&nrc_netif[vif]->ip6_addr[i]);
		nrc_netif[vif]->ip6_addr_state[i] = IP6_ADDR_INVALID;
#if LWIP_IPV6_ADDRESS_LIFETIMES
		nrc_netif[vif]->ip6_addr_valid_life[i] = IP6_ADDR_LIFE_STATIC;
		nrc_netif[vif]->ip6_addr_pref_life[i] = IP6_ADDR_LIFE_STATIC;
#endif /* LWIP_IPV6_ADDRESS_LIFETIMES */
	}
#endif
}

void ifconfig_help_display(void)
{
	A("Usage:\n");
	A("   ifconfig <interface> <address> [-n <netmask>] [-g <gateway>] [-m <mtu>] [-d <dns1 dns2>]\n");
}

bool ifconfig_display(WIFI_INTERFACE if_index)
{
	struct netif *netif = NULL;

	if(if_index == WLAN0_INTERFACE || if_index == WLAN1_INTERFACE){
		netif = nrc_netif[if_index];
		A("wlan%d\t", if_index);
#ifdef SUPPORT_ETHERNET_ACCESSPOINT
	} else if(if_index == BRIDGE_INTERFACE){
		netif = &br_netif;
		A("br\t");
	} else if(if_index == ETHERNET_INTERFACE){
		netif = &eth_netif;
		A("eth\t");
#endif
	} else {
		A("[%s] invlid if_index:%d\n", __func__, if_index);
		return false;
	}

	A("HWaddr ");
	A(MAC_STR,MAC_VALUE(netif->hwaddr) );
	A("   MTU:%d\n", netif->mtu);
#if LWIP_IPV4
	A("\tinet:");
	ip_addr_debug_print_val(LWIP_DBG_ON, (netif->ip_addr));
	A("\tnetmask:");
	ip_addr_debug_print_val(LWIP_DBG_ON, (netif->netmask));
	A("\tgateway:");
	ip_addr_debug_print_val(LWIP_DBG_ON, (netif->gw));
	A("\n");
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
	for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
		A(" 		  inet6: %s/0x%"X8_F"\n", ip6addr_ntoa(netif_ip6_addr(netif, i)),
			netif_ip6_addr_state(netif, i));
	}
#endif
	A("\n");
	return true;
}


#ifdef SUPPORT_ETHERNET_ACCESSPOINT
bool support_ethernet_accesspoint(void)
{
	return true;
}
#endif /* SUPPORT_ETHERNET_ACCESSPOINT */

static void ifconfig_display_all()
{
	int i;
#ifdef SUPPORT_ETHERNET_ACCESSPOINT
	if (nrc_eth_get_network_mode() == NRC_NETWORK_MODE_BRIDGE) {
		ifconfig_display(BRIDGE_INTERFACE);
	}
	ifconfig_display(ETHERNET_INTERFACE);
#endif /* SUPPORT_ETHERNET_ACCESSPOINT */
	for(i = 0; i < MAX_IF; i++) {
		ifconfig_display(i);
	}
}

bool wifi_ifconfig(int argc, char *argv[])
{
	char* if_name = NULL;
	char* ip = NULL;
	char* netmask = NULL;
	char* gateway = NULL;
	char* dns1 = NULL;
	char* dns2 = NULL;
	char* mtu = NULL;
	int if_idx = -1;
	struct netif *nif = NULL;
	ip_addr_t dnsserver;
	ip_addr_t addr;
	ip_addr_t gw_addr;
	ip_addr_t nm_addr;

	if ( argc == 0) {
		ifconfig_display_all();
		return true;
	}

	if (strcmp(argv[0], "-h") == 0) {
		ifconfig_help_display();
		return true;
	}

	if_name = argv[0];
	if (strcmp(if_name, "wlan0") == 0) {
		if_idx = WLAN0_INTERFACE;
		nif = nrc_netif[if_idx];
	} else if (strcmp(if_name, "wlan1") == 0) {
		if_idx = WLAN1_INTERFACE;
		nif = nrc_netif[if_idx];
#ifdef SUPPORT_ETHERNET_ACCESSPOINT
	} else if (strcmp(if_name, "eth") == 0) {
		if_idx = ETHERNET_INTERFACE;
		nif = &eth_netif;
	} else if (strcmp(if_name, "br") == 0) {
		if_idx = BRIDGE_INTERFACE;
		nif = &br_netif;
#endif /* SUPPORT_ETHERNET_ACCESSPOINT */
	} else {
		ifconfig_help_display();
		return false;
	}

	if ( argc == 1) {
		ifconfig_display(if_idx);
		return true;
	}

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
			netmask = argv[++i];
			if (!inet_pton(AF_INET, netmask, &nm_addr))
				return false;
			A("netmask : %s\n", netmask);
		} else if (strcmp(argv[i], "-g") == 0 && i + 1 < argc) {
			gateway = argv[++i];
			if (!inet_pton(AF_INET, gateway, &gw_addr))
				return false;
			A("gateway : %s\n", gateway);
		} else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
			mtu = argv[++i];
			A("mtu : %s\n", mtu);
		} else if (strcmp(argv[i], "-d") == 0 && i + 2 < argc) {
			dns1 = argv[++i];
			dns2 = argv[++i];
			A("dns1 : %s\n", dns1);
			A("dns2 : %s\n", dns2);
		} else {
			if (i==1){
				if(inet_pton(AF_INET, argv[i], &addr)){
					ip = argv[1];
					A("ip : %s\n", ip);
				}
			}
		}
	}
#if 0
	for (int j = 0; j < argc; j++)
		A("%s\n", argv[j]);
#endif

	if (ip == NULL) {
		goto exit;
	}

#if LWIP_IPV4
	if (IP_IS_V4(&addr)) {
		ip4_addr_t *ip4 = NULL, *nm = NULL, *gw = NULL;
		u8_t index = netif_name_to_index(if_name);

		ip4 = (ip4_addr_t *) &addr;
		gw = (ip4_addr_t *) &gw_addr;
		nm = (ip4_addr_t *) &nm_addr;

		if (gateway == NULL)
			IP4_ADDR(gw, ip4_addr1_16(ip4), ip4_addr2_16(ip4), ip4_addr3_16(ip4), 1);

		if (netmask == NULL)
			IP4_ADDR(nm, 255, 255, 255, 0);

		netif_set_addr(nif, ip4, nm, gw);
	}
	else
#endif
#if LWIP_IPV6
	if (IP_IS_V6(&addr)) {
		int8_t index = 0;
		/* Set static ipv6 address to available slot in netif */
		if (netif_add_ip6_address(nif, &addr.u_addr.ip6, &index) != ERR_OK) {
			E(TT_SDK_WIFI, "[%s] adding %s failed\n", 	  __func__, argv[1]);
			return false;
		}
		netif_ip6_addr_set_state(nrc_netif[0], index, IP6_ADDR_PREFERRED);
	}
	else
#endif
	{
		E(TT_NET, "%s error!! unknown address value\n", module_name());
		return false;
	}

	/* For Wifi interfaces, save static ip configured into ipinfo, */
	/* so that ip can be retrieved if disconnect happens. */
	if ((if_idx == 0) || (if_idx == 1)) {
		ipinfo[if_idx].ip = nif->ip_addr;
		ipinfo[if_idx].netmask = nif->netmask;
		ipinfo[if_idx].gw = nif->gw;
	}

exit:
	if (mtu != NULL)
		nif->mtu =atoi(mtu);

	if (dns1 != NULL) {
		inet_pton(AF_INET, dns1, &dnsserver);
		dns_setserver(0, &dnsserver);
	}

	if (dns2 != NULL) {
		inet_pton(AF_INET, dns2, &dnsserver);
		dns_setserver(1, &dnsserver);
	}
	ifconfig_display(if_idx);
	return true;
}

bool wifi_set_ip_info(int vif_id, struct ip_info *info)
{
	struct netif *netif = nrc_netif[vif_id];
	ip_addr_t addr = info->ip;
	ip_addr_t gw = info->gw;
	ip_addr_t netmask = info->netmask;
#if LWIP_IPV4
	if (IP_IS_V4_VAL(addr)) {
		netif_set_addr(netif , ip_2_ip4(&addr),ip_2_ip4(  &netmask), ip_2_ip4(&gw));
	}
#endif
	return true;
}

bool wifi_get_ip_info(int vif_id, struct ip_info *info)
{
	struct netif *netif = nrc_netif[vif_id];
#if LWIP_IPV4
	info->ip = netif->ip_addr;
	info->netmask = netif->netmask;
	info->gw = netif->gw;
	return true;
#else
	return false;
#endif
}

bool wifi_set_dns_server(ip_addr_t *pri_dns, ip_addr_t *sec_dns)
{
	/* Set DNS Server */
	ip_addr_t dnsserver;
	if (pri_dns == NULL || ip_addr_isany(pri_dns)) {
		inet_pton(AF_INET,WIFI_PRIMARY_DNS_SERVER, &dnsserver);
		dns_setserver(0, &dnsserver);
		inet_pton(AF_INET, WIFI_SECONDARY_DNS_SERVER, &dnsserver);
		dns_setserver(1, &dnsserver);
	} else {
		dns_setserver(0, pri_dns);
		dns_setserver(1, sec_dns);
	}

	return true;
}

bool wifi_set_opmode(WIFI_MODE opmode)
{
	wifi_mode = opmode;
	return true;
}

 WIFI_MODE wifi_get_opmode()
{
	return wifi_mode;
}
#if LWIP_IPV4 && LWIP_DHCPS
enum dhcp_status wifi_softap_dhcps_status(void)
{
	struct netif *net_if = dhcps_get_interface();

	if (net_if->dhcps_pcb)
		return DHCP_STARTED;
	else
		return DHCP_STOPPED;
}
#endif /* LWIP_IPV4 && LWIP_DHCPS */
uint8_t wifi_get_vif_id(ip_addr_t* dest)
{
	u8_t vif_id = 0;
	int i;
#if LWIP_IPV4
	for(i=0; i<MAX_IF; i++){
		if(nrc_netif[i] == ip_route(IP46_ADDR_ANY(IP_GET_TYPE(dest)), dest)){
			vif_id = i;
			break;
		}
	}
	return vif_id;
#else
	return 0;
#endif
}

int setup_wifi_ap_mode(struct netif *net_if, int updated_lease_time)
{
	int vif = 0;

	vif = net_if->num;

	A("%s vif:%d :%d min\n", __func__, vif, updated_lease_time);

#if LWIP_DHCPS && LWIP_IPV4
	ip4_addr_t ip4;

	wifi_set_opmode((wifi_get_opmode()|WIFI_STATIONAP_MODE)&USE_WIFI_MODE);
	wifi_get_ip_info(vif, &ipinfo[vif]);

	if (ip_addr_isany(&ipinfo[vif].ip))
	{
		if (ip4addr_aton("192.168.50.1", &ip4))
			ip_addr_copy_from_ip4(ipinfo[vif].ip, ip4);

		if (ip4addr_aton("192.168.50.1", &ip4))
			ip_addr_copy_from_ip4(ipinfo[vif].gw, ip4);

		if (ip4addr_aton("255.255.255.0", &ip4))
			ip_addr_copy_from_ip4(ipinfo[vif].netmask, ip4);
	}

	A("ip:%s\n", ip4addr_ntoa((const ip4_addr_t*)ip_2_ip4(&ipinfo[vif].ip)));
	A("gw:%s\n",ip4addr_ntoa((const ip4_addr_t*)ip_2_ip4(&ipinfo[vif].gw)));
	A("netmask:%s\n", ip4addr_ntoa((const ip4_addr_t*)ip_2_ip4(&ipinfo[vif].netmask)));
	wifi_set_ip_info(vif, &ipinfo[vif]);
#endif /* LWIP_DHCPS && LWIP_IPV4 */

#if LWIP_IPV4 && LWIP_DNS
	captdnsInit();
#endif

#if LWIP_DHCPS
	dhcps_start(&ipinfo[vif], net_if);
	if(updated_lease_time > 0)
		wifi_softap_set_dhcps_lease_time(updated_lease_time);
#endif

	return 0;
}

#ifdef SUPPORT_ETHERNET_ACCESSPOINT
int start_dhcps_on_if(struct netif *net_if, int updated_lease_time)
{
	int vif = 0;
	struct ip_info ipinfo;

	A("%s netif name : %c%c\n", __func__, net_if->name[0], net_if->name[1]);
	A("%s lease time : %d min\n", __func__, updated_lease_time);

	if(ip_addr_isany_val(net_if->ip_addr)){
		/* Set default IP address */
		IP4_ADDR(&net_if->ip_addr, 192, 168, 50, 1);
		IP4_ADDR(&net_if->gw, 192, 168, 50, 1);
		IP4_ADDR(&net_if->netmask, 255, 255, 255, 0);
	}

	A("ip:%s\n", ip4addr_ntoa((const ip4_addr_t*)ip_2_ip4(&net_if->ip_addr)));
	A("gw:%s\n",ip4addr_ntoa((const ip4_addr_t*)ip_2_ip4(&net_if->gw)));
	A("netmask:%s\n", ip4addr_ntoa((const ip4_addr_t*)ip_2_ip4(&net_if->netmask)));

#if LWIP_DNS && LWIP_DHCPS
	captdnsInit();
#endif

#if LWIP_DHCPS
	ipinfo.ip = net_if->ip_addr;
	ipinfo.gw = net_if->gw;
	ipinfo.netmask = net_if->netmask;

	dhcps_start(&ipinfo, net_if);
	if(updated_lease_time > 0)
		wifi_softap_set_dhcps_lease_time(updated_lease_time);
#endif

	return 0;
}
#endif /* SUPPORT_ETHERNET_ACCESSPOINT */

void set_dhcp_status(bool status)
{
	g_flag_dhcp_ok = status;
}

bool get_dhcp_status(void)
{
	return g_flag_dhcp_ok;
}

int reset_wifi_ap_mode(int vif)
{
	A("%s vif:%d\n", __func__, vif);
#if LWIP_DHCPS
	dhcps_stop();
	s_wifi_ip_mode[vif].dhcp_running = DHCP_STOPPED;
#endif

	return 0;
}

static void
status_callback(struct netif *state_netif)
{
	int i=0;
	struct ip_info if_ip;

	if (netif_is_up(state_netif)) {
#if defined(INCLUDE_TRACE_WAKEUP)
		A("%s wlan%d is up.\n", module_name(), netif_get_index(state_netif) - 1);
#endif /* INCLUDE_TRACE_WAKEUP */
#if LWIP_IPV4
		if (!ip4_addr_isany_val(*netif_ip4_addr(state_netif))) {
#if defined(INCLUDE_TRACE_WAKEUP)
			A("%s IP is ready : ", module_name());
			ip_addr_debug_print_val(LWIP_DBG_ON, (state_netif->ip_addr));
			A("\n", module_name());
#endif /* INCLUDE_TRACE_WAKEUP */
			set_dhcp_status(true);
			set_standalone_ipaddr(0,
				ip4_addr_get_u32(ip_2_ip4(&state_netif->ip_addr)),
				ip4_addr_get_u32(ip_2_ip4(&state_netif->netmask)),
				ip4_addr_get_u32(ip_2_ip4(&state_netif->gw)));
  		}
#endif /* LWIP_IPV4 */
	} else {
		I(TT_NET, "%s netif_is_down\n", module_name());
	}
}

static void
link_callback(struct netif *state_netif)
{

	if (netif_is_link_up(state_netif)) {
		A("link_callback==UP\n");
#if LWIP_IPV6
		netif_create_ip6_linklocal_address(state_netif, 1);
		A("ip6 linklocal address: %s\n", ip6addr_ntoa(netif_ip6_addr(state_netif, 0)));

		nd6_tmr(); /* tick nd to join multicast groups */
		nd6_restart_netif(state_netif);
#endif
	} else {
		A("link_callback==DOWN\n");
	}
}

static void set_default_dns_server(void)
{
	ip_addr_t *dnsserver = NULL;

	dnsserver = (ip_addr_t *)dns_getserver(0);
	if (ip_addr_isany(dnsserver)) {
		inet_pton(AF_INET, WIFI_PRIMARY_DNS_SERVER, dnsserver);
		dns_setserver(0, dnsserver);
	}

	dnsserver = (ip_addr_t *)dns_getserver(1);
	if (ip_addr_isany(dnsserver)) {
		inet_pton(AF_INET, WIFI_SECONDARY_DNS_SERVER, dnsserver);
		dns_setserver(1, dnsserver);
	}
}

static void lwip_handle_interfaces(void * param)
{
	int i =0 ;
	for(i=0; i<MAX_IF;i++){
		nrc_netif[i] = (struct netif*)mem_malloc(sizeof(struct netif));
		nrc_netif[i]->num = i;
		/* set MAC hardware address to be used by lwIP */
		get_standalone_macaddr(i, nrc_netif[i]->hwaddr);
#if LWIP_IPV4
		ip4_addr_t ipaddr, netmask, gw;
		memset(&ipaddr, 0, sizeof(ipaddr));
		memset(&netmask, 0, sizeof(netmask));
		memset(&gw, 0, sizeof(gw));
		netif_add(nrc_netif[i], &ipaddr, &netmask, &gw, NULL, wlif_init, tcpip_input);
#endif

#if LWIP_IPV6
#if !LWIP_IPV4
		netif_add(nrc_netif[i], NULL, wlif_init, tcpip_input);
#endif
		netif_create_ip6_linklocal_address(nrc_netif[i], 1);
#if LWIP_IPV6_AUTOCONFIG
		nrc_netif[i]->ip6_autoconfig_enabled = 1;
#endif
		A("ip6 linklocal address: %s\n", ip6addr_ntoa(netif_ip6_addr(nrc_netif[i], 0)));
#endif

		netif_set_flags(nrc_netif[i], NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET);
		netif_set_status_callback(nrc_netif[i], status_callback);
		netif_set_link_callback(nrc_netif[i], link_callback);

	/* make it the default interface */
		if(i == DEFAULT_NET_IF)
			netif_set_default( nrc_netif[i] );

	/* bring it up */
		netif_set_up( nrc_netif[i] );
	}
#if LWIP_IPV6
	nd6_tmr(); /* tick nd to join multicast groups */
	for(i=0; i<MAX_IF;i++){
		nd6_restart_netif( nrc_netif[i] );
	}
#endif
	set_default_dns_server();
}

#if LWIP_IPV6
void wifi_nd6_restart_netif( int vif )
{
	nd6_restart_netif( nrc_netif[vif] );
}
#endif

/* Initialisation required by lwIP. */
void wifi_lwip_init( void )
{
	/* Init lwIP and start lwIP tasks. */
	tcpip_init( lwip_handle_interfaces, NULL );
}

static void dhcps_help_display(void)
{
	A("Usage:\n");
	A("  dhcps [-i <intf name>] [-lt <lease time(unit:min)] [-st]\n");

}

bool wifi_dhcps(int argc, char *argv[])
{
	int if_idx = -1;
	struct netif *nif = NULL;
	int updated_lease_time = 0;

	if (argc  == 0) {
		/* Run DHCP server on wlan0 */
		nif = nrc_netif[0];
		setup_wifi_ap_mode(nif, 0);
		return true;
	}

	if (strcmp(argv[0], "-h") == 0) {
		dhcps_help_display();
		return true;
	}

	if (strcmp(argv[0], "-st") == 0) {
		dhcps_status();
		return true;
	}

	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
			i++;
			if (strcmp(argv[i], "wlan0") == 0) {
				if_idx = WLAN0_INTERFACE;
				nif = nrc_netif[if_idx];
			} else if (strcmp(argv[i], "wlan1") == 0) {
				if_idx = WLAN1_INTERFACE;
				nif = nrc_netif[if_idx];
#ifdef SUPPORT_ETHERNET_ACCESSPOINT
			} else if (strcmp(argv[i], "eth") == 0) {
				if_idx = ETHERNET_INTERFACE;
				nif = &eth_netif;
			} else if (strcmp(argv[i], "br") == 0) {
				if_idx = BRIDGE_INTERFACE;
				nif = &br_netif;
#endif /* SUPPORT_ETHERNET_ACCESSPOINT */
			} else {
				dhcps_help_display();
				return false;
			}
			A("interface : %s\n", argv[i]);
		} else if (strcmp(argv[i], "-lt") == 0 && i + 1 < argc) {
			updated_lease_time = atoi(argv[++i]);
			A("lease time : %d\n", updated_lease_time);
		}
	}
#if 0
	for (int j = 0; j < argc; j++)
		A("%s\n", argv[j]);
#endif

	if (if_idx == WLAN0_INTERFACE || if_idx == WLAN1_INTERFACE) {
		setup_wifi_ap_mode(nif, updated_lease_time);
	} else {
#ifdef SUPPORT_ETHERNET_ACCESSPOINT
		start_dhcps_on_if(nif, updated_lease_time);
#else
		return false;
#endif /* SUPPORT_ETHERNET_ACCESSPOINT */
	}
	return true;
}

