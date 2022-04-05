#include "lwip/sys.h"
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/mem.h"
#if LWIP_DHCPS
#include "apps/dhcpserver/dhcpserver.h"
#endif
#include "arch/wlif.h"
#include "nrc_wifi.h"
#if LWIP_DNS && LWIP_DHCPS
#include "captdns.h"
#endif

#include "netif/conf_wl.h"
#include "standalone.h"

#ifdef SUPPORT_ETHERNET_ACCESSPOINT
#include "nrc_eth_if.h"

extern struct netif br_netif;
#endif /* SUPPORT_ETHERNET_ACCESSPOINT */

struct netif* nrc_netif[MAX_IF];
char* hostname;
bool default_hostname;
static bool dhcpc_start_flag[MAX_IF] = {DHCP_STOPPED, DHCP_STOPPED};
static bool dhcps_start_flag[MAX_IF] = {DHCP_STOPPED, DHCP_STOPPED};
struct ip_info ipinfo[MAX_IF];

bool g_flag_dhcp_ok = false;
u8_t wifi_mode;

#define MAC_STR				"%02X:%02X:%02X:%02X:%02X:%02X"
#define MAC_VALUE(a)			(a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define IP4_ADDR_STR			"%" U16_F ".%" U16_F ".%" U16_F ".%" U16_F
#define IP4_ADDR_VALUE(addr)	ip4_addr1_16(addr), ip4_addr2_16(addr),ip4_addr3_16(addr), ip4_addr4_16(addr)

#define NETIF_ADDRS &ipaddr, &netmask, &gw

#define DEFAULT_NET_IF SOFTAP_IF

static const char *module_name()
{
	return "net: ";
}

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

int wifi_station_dhcpc_start(int vif)
{
	int8_t ret;
	struct netif *target_if;

#ifdef SUPPORT_ETHERNET_ACCESSPOINT
	if (get_network_mode() == NRC_NETWORK_MODE_BRIDGE) {
		A("[%s] starting dhcp client on bridge...\n", __func__);
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

void reset_ip_address(int vif)
{
	I(TT_NET, "%s%d reset_ip_address after disassociation\n",
		module_name(), vif);

	dhcp_stop(nrc_netif[vif]);
	dhcp_cleanup(nrc_netif[vif]);
	dhcpc_start_flag[vif] = false;

	ip4_addr_set_zero(&nrc_netif[vif]->ip_addr);
	ip4_addr_set_zero(&nrc_netif[vif]->netmask);
	ip4_addr_set_zero(&nrc_netif[vif]->gw);
}

void wifi_ifconfig_help_display(void)
{
	A("Usage:\n");
	A("   ifconfig <interface> [<address>]\n");
	A("   ifconfig <interface> [mtu <NN>]\n");
}

void wifi_ifconfig_display(WIFI_INTERFACE if_index)
{
	struct netif *netif = nrc_netif[if_index];
	int i = 0;

	/* LWIP netif default index starts from 1, so adjust it by subtracting 1 */
	A("wlan%d      ", netif_get_index(netif) - 1);
	A("HWaddr ");
	A(MAC_STR,MAC_VALUE(netif->hwaddr) );
	A("   MTU:%d\n", netif->mtu);
	A("           inet:");
	ip_addr_debug_print_val(LWIP_DBG_ON, (netif->ip_addr));
	A("\tnetmask:");
	ip_addr_debug_print_val(LWIP_DBG_ON, (netif->netmask));
	A("\tgateway:");
	ip_addr_debug_print_val(LWIP_DBG_ON, (netif->gw));
	A("\n");
}


#ifdef SUPPORT_ETHERNET_ACCESSPOINT
bool support_ethernet_accesspoint(void)
{
	return true;
}

extern struct netif eth_netif;

void eth_ifconfig_display(void)
{
	struct netif *netif_temp = &eth_netif;

	A("eth        ");
	A("HWaddr ");
	A(MAC_STR,MAC_VALUE(netif_temp->hwaddr) );
	A("   MTU:%d\n", netif_temp->mtu);
	A("          inet addr:");
	A(IP4_ADDR_STR,IP4_ADDR_VALUE(&(netif_temp->ip_addr)) );
	A("   Mask:");
	A(IP4_ADDR_STR,IP4_ADDR_VALUE(&(netif_temp->netmask)) );
	A("   Gw:");
	A(IP4_ADDR_STR,IP4_ADDR_VALUE(&(netif_temp->gw)) );
	A("\n");
}

void br_ifconfig_display(void)
{
	struct netif *netif_temp = &br_netif;
	A("br         ");
	A("HWaddr ");
	A(MAC_STR,MAC_VALUE(netif_temp->hwaddr) );
	A("   MTU:%d\n", netif_temp->mtu);
	A("          inet addr:");
	A(IP4_ADDR_STR,IP4_ADDR_VALUE(&(netif_temp->ip_addr)) );
	A("   Mask:");
	A(IP4_ADDR_STR,IP4_ADDR_VALUE(&(netif_temp->netmask)) );
	A("   Gw:");
	A(IP4_ADDR_STR,IP4_ADDR_VALUE(&(netif_temp->gw)) );
	A("\n");
}
#endif /* SUPPORT_ETHERNET_ACCESSPOINT */

static void wifi_ifconfig_display_all()
{
	int i;
#ifdef SUPPORT_ETHERNET_ACCESSPOINT
	if (get_network_mode() == NRC_NETWORK_MODE_BRIDGE) {
		br_ifconfig_display( );
		A("\n");
	}
	eth_ifconfig_display( );
	A("\n");
#endif /* SUPPORT_ETHERNET_ACCESSPOINT */
	for(i = 0; i < MAX_IF; i++) {
		wifi_ifconfig_display(i);
		A("\n");
	}
}

bool wifi_ifconfig(int num_param, char *params[])
{
	char *if_name = params[0];
	int if_idx = -1;
	struct netif *nif = NULL;
	ip_addr_t dnsserver;

	if (num_param <= 0) {
		wifi_ifconfig_display_all();
		return true;
	}

	if (strcmp(params[0], "-h") == 0) {
		wifi_ifconfig_help_display();
		return true;
	}

	if (strcmp(if_name, "wlan0") == 0) {
		if_idx = 0;
		nif = nrc_netif[if_idx];
	} else if (strcmp(if_name, "wlan1") == 0) {
		if_idx = 1;
		nif = nrc_netif[if_idx];
#ifdef SUPPORT_ETHERNET_ACCESSPOINT
	} else if (strcmp(if_name, "eth") == 0) {
		if_idx = 2;
		nif = &eth_netif;
	} else if (strcmp(if_name, "br") == 0) {
		if_idx = 3;
		nif = &br_netif;
#endif /* SUPPORT_ETHERNET_ACCESSPOINT */
	} else {
		A("%s is unsupported. run iperf -h\n",
			params[0] == NULL ?	"NULL" : params[0]);
		return false;
	}

	if (num_param == 1) {
		// pass
	} else if (num_param == 3 && strcmp(params[1], "mtu") == 0) {
		nif->mtu = atoi(params[2]);
	} else if (num_param == 2) {
		struct ip_info info;
		ip4addr_aton(params[1], &info.ip);
		IP4_ADDR(&info.gw, ip4_addr1_16(&info.ip), ip4_addr2_16(&info.ip), ip4_addr3_16(&info.ip), 1);
		IP4_ADDR(&info.netmask, 255, 255, 255, 0);
		netif_set_addr(nif, &info.ip, &info.netmask, &info.gw);
	} else {
		return false;
	}

	/* Set DNS Server */
	inet_pton(AF_INET,WIFI_PRIMARY_DNS_SERVER, &dnsserver);
	dns_setserver(0, &dnsserver);
	inet_pton(AF_INET, WIFI_SECONDARY_DNS_SERVER, &dnsserver);
	dns_setserver(1, &dnsserver);

	if ((if_idx == 0) || (if_idx == 1)) {
		wifi_ifconfig_display(if_idx);
#ifdef SUPPORT_ETHERNET_ACCESSPOINT
	} else if (if_idx == 2) {
		eth_ifconfig_display();
	} else if (if_idx == 3) {
		br_ifconfig_display();
#endif /* SUPPORT_ETHERNET_ACCESSPOINT */
	}

	return true;
}

bool wifi_set_ip_info(WIFI_INTERFACE if_index, struct ip_info *info)
{
	netif_set_addr(nrc_netif[if_index], &info->ip,  &info->netmask, &info->gw);
	return true;
}

bool wifi_get_ip_info(WIFI_INTERFACE if_index, struct ip_info *info)
{
	info->ip = (nrc_netif[if_index])->ip_addr;
	info->netmask = (nrc_netif[if_index])->netmask;
	info->gw = (nrc_netif[if_index])->gw;
	return true;
}

bool wifi_set_dns_server(void)
{
	/* Set DNS Server */
	ip_addr_t dnsserver;
	inet_pton(AF_INET,WIFI_PRIMARY_DNS_SERVER, &dnsserver);
	dns_setserver(0, &dnsserver);
	inet_pton(AF_INET, WIFI_SECONDARY_DNS_SERVER, &dnsserver);
	dns_setserver(1, &dnsserver);
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

enum dhcp_status wifi_softap_dhcps_status(void)
{
	struct netif *net_if = dhcps_get_interface();

	if (net_if->dhcps_pcb)
		return DHCP_STARTED;
	else
		return DHCP_STOPPED;
}

u8_t wifi_get_vif_id(ip4_addr_t* addr)
{
	u8_t vif_id = DEFAULT_NET_IF;
	int i;

	for(i=0; i<MAX_IF; i++){
		if(nrc_netif[i] == ip_route(IP46_ADDR_ANY(IP_GET_TYPE(addr)), addr)){
			vif_id = i;
			break;
		}
	}
	return vif_id;
}

#if defined(SUPPORT_MBEDTLS) && defined(SUPPORT_AES_SAMPLE_TEST)
#include "mbedtls/aes.h"
int aes_sample_test(void)
{
	mbedtls_aes_context context_in, context_out;
	int index;

	unsigned char key[32] = { 'p', 'a', 's', 's', 'w', 'o', 'r', 'd', 0, 0, 0, 0, 0, 0, 0, 0,
	                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	unsigned char iv_init[16] = { 14, 31, 6, 126, 18, 12, 36, 70, 100, 9, 42, 51, 111, 84, 3, 25 };
	unsigned char iv[16];;

	unsigned char input[128] = { 'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd', '!', 0, 0, 0, 0,
	                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	unsigned char encrypt[128];
	unsigned char decrypt[128];

	/* display input string */
	A("Input   : ");
	for(index = 0; index < 16; ++index){
		A("%c", (char)input[index]);
	}
	A("\n");

	/* set IV for encoder parts */
	for(index = 0; index < 16; ++index){
		iv[index] = 	iv_init[index];
	}

	mbedtls_aes_setkey_enc( &context_in, key, 256 );
	mbedtls_aes_crypt_cbc( &context_in, MBEDTLS_AES_ENCRYPT, 16, iv, input, encrypt );

	/* encoded string */
	A("Encoded : ");
	for(index = 0; index < 16; ++index){
		A("%c", (char)encrypt[index]);
	}
	A("\n");

	/* set IV for decoder parts */
	for(index = 0; index < 16; ++index){
		iv[index] = 	iv_init[index];
	}

	mbedtls_aes_setkey_dec( &context_out, key, 256 );
	mbedtls_aes_crypt_cbc( &context_out, MBEDTLS_AES_DECRYPT, 16, iv, encrypt, decrypt );

	/* decoded string */
	A("Decoded : ");
	for(index = 0; index < 16; ++index){
		A("%c", (char)decrypt[index]);
	}
	A("\n");
	return 0;
}
#endif /* (SUPPORT_MBEDTLS) && (SUPPORT_AES_SAMPLE_TEST) */

int setup_wifi_ap_mode(int vif, int updated_lease_time)
{
	A("%s vif:%d :%d min\n", __func__, vif, updated_lease_time);

	wifi_set_opmode((wifi_get_opmode()|WIFI_STATIONAP_MODE)&USE_WIFI_MODE);
	wifi_get_ip_info(vif, &ipinfo[vif]);

	if(ip_addr_isany_val(ipinfo[vif].ip)){
		/* Set default IP address */
		IP4_ADDR(&ipinfo[vif].ip, 192, 168, 50, 1);
		IP4_ADDR(&ipinfo[vif].gw, 192, 168, 50, 1);
		IP4_ADDR(&ipinfo[vif].netmask, 255, 255, 255, 0);
	}

	A("ip:%s\n", ip4addr_ntoa((const ip4_addr_t*)ip_2_ip4(&ipinfo[vif].ip)));
	A("gw:%s\n",ip4addr_ntoa((const ip4_addr_t*)ip_2_ip4(&ipinfo[vif].gw)));
	A("netmask:%s\n", ip4addr_ntoa((const ip4_addr_t*)ip_2_ip4(&ipinfo[vif].netmask)));
	wifi_set_ip_info(vif, &ipinfo[vif]);

#if LWIP_DNS && LWIP_DHCPS
	captdnsInit();
#endif

	// Set default network interface to ap (legacy : 0)
	netif_set_default( nrc_netif[vif] );

#if LWIP_DHCPS
	dhcps_start(&ipinfo[vif], nrc_netif[vif]);
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

	vif = net_if->num;
	A("%s netif name : %c%c%d\n", __func__, net_if->name[0], net_if->name[1], net_if->num);
	A("%s vif:%d :%d min\n", __func__, vif, updated_lease_time);

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

	netif_set_default( net_if );

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

static void
status_callback(struct netif *state_netif)
{
	int i=0;
	struct ip_info if_ip;

	if (netif_is_up(state_netif)) {
		A("%s netif_is_up, local interface IP is %s\n\n",
			module_name(), ip4addr_ntoa(netif_ip4_addr(state_netif)));

		if (!ip4_addr_isany_val(*netif_ip4_addr(state_netif))) {
			A("%s IP is ready\n", module_name());
			set_dhcp_status(true);
			set_standalone_ipaddr(0, state_netif->ip_addr.addr,
				state_netif->netmask.addr, state_netif->gw.addr);
  		}
	} else {
		I(TT_NET, "%s netif_is_down\n", module_name());
	}
}

static void
link_callback(struct netif *state_netif)
{

	if (netif_is_link_up(state_netif)) {
		A("link_callback==UP\n");
	} else {
		A("link_callback==DOWN\n");
	}
}

static void lwip_handle_interfaces(void * param)
{
	int i =0 ;
	ip4_addr_t ipaddr, netmask, gw;
	ip4_addr_set_zero(&gw);
	ip4_addr_set_zero(&ipaddr);
	ip4_addr_set_zero(&netmask);

	for(i=0; i<MAX_IF;i++){
		nrc_netif[i] = (struct netif*)mem_malloc(sizeof(*nrc_netif[i]));
		nrc_netif[i]->num = i;
		/* set MAC hardware address to be used by lwIP */
		get_standalone_macaddr(i, nrc_netif[i]->hwaddr);
		netif_add(nrc_netif[i], NETIF_ADDRS, NULL, wlif_init, tcpip_input);
		netif_set_flags(nrc_netif[i], NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET);
		netif_set_status_callback(nrc_netif[i], status_callback);
		netif_set_link_callback(nrc_netif[i], link_callback);

	/* make it the default interface */
		if(i == DEFAULT_NET_IF)
			netif_set_default( nrc_netif[i] );

	/* bring it up */
		netif_set_up( nrc_netif[i] );
	}
}

/* Initialisation required by lwIP. */
void wifi_lwip_init( void )
{
	/* Init lwIP and start lwIP tasks. */
	tcpip_init( lwip_handle_interfaces, NULL );
}
