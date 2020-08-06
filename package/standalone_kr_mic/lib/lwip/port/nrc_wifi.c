#include "lwip/sys.h"
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/mem.h"
#if LWIP_PING
#include "apps/ping/ping.h"
#endif
#if LWIP_IPERF
#include "apps/lwiperf/lwiperf.h"
#endif
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

struct netif* nrc_netif[MAX_IF];
char* hostname;
bool default_hostname;
static bool dhcpc_start_flag[MAX_IF] = {false, false};
static bool dhcps_flag[MAX_IF] = {true, true};
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

void dhcp_run(int vif)
{
	if (vif >= MAX_IF) {
		E(TT_NET, "%s%d incorrect vif\n",
			module_name(), vif);
		return;
	}
	I(TT_NET, "%s%d dhcp_run\n",
		module_name(), vif);
	wifi_station_dhcpc_start(vif);
}

int wifi_station_dhcpc_start(int vif)
{
	int8_t ret;

	if(wifi_station_dhcpc_status(vif)){
		//dhcp_release(nrc_netif[vif]);
		dhcp_stop(nrc_netif[vif]);
		dhcp_cleanup(nrc_netif[vif]);
		dhcpc_start_flag[vif] = false;

		ip_addr_set_zero(&nrc_netif[vif]->ip_addr);
		ip_addr_set_zero(&nrc_netif[vif]->netmask);
		ip_addr_set_zero(&nrc_netif[vif]->gw);
	}

	ret = dhcp_start(nrc_netif[vif]);

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
	struct netif *netif_temp = nrc_netif[if_index];
	
	A("wlan%d     ", if_index);
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

static void wifi_ifconfig_display_all()
{
	int i;
	for(i = 0; i < MAX_IF; i++) {
		wifi_ifconfig_display(i);
		A("\n");
	}
}

bool wifi_ifconfig(int num_param, char *params[])
{
	char *if_name = params[0];
	int if_idx = 0;
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
	} else if (strcmp(if_name, "wlan1") == 0) {
		if_idx = 1;
	} else {
		A("%s is unsupported. run iperf -h\n",
			params[0] == NULL ?	"NULL" : params[0]);
		return false;
	}

	nif = nrc_netif[if_idx];

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

	wifi_ifconfig_display(if_idx);

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
    return dhcps_flag[0];
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

#if LWIP_PING
const char ping_usage_str[] = "Usage: ping [-c count] [-i interval] [-s packetsize] destination\n"
                           "      `ping destination stop' for stopping\n"
                           "      `ping -st' for checking current ping applicatino status\n";

void ping_usage(void)
{
	A("%s\r\n", ping_usage_str);
}

void  ping_run(char *cmd)
{
	char *str = NULL;
	char *pcmdStr_log = NULL;
	char *pcmdStr = (char *)cmd;
	double interval = PING_DELAY_DEFAULT;
	u32_t  packet_size= PING_DATA_SIZE;
	u32_t count = PING_COUNT_DEFAULT;
	ip4_addr_t ping_addr;
	u8_t vif_id = 0xFF;
	ping_parm_t* ping_conn = NULL;
	ping_parm_t* ping_stop_conn = NULL;
	int ip4addr_aton_is_valid = false;

	ip4_addr_set_any(&ping_addr);

	if(pcmdStr != NULL)
		memcpy((char **)&pcmdStr_log, (char **)&pcmdStr, sizeof((char *)&pcmdStr));

	ping_conn = (ping_parm_t *)mem_malloc(sizeof(ping_parm_t));
	if (ping_conn == NULL) {
		E(TT_NET,"%s memory allocation fail!\n", module_name());
		return;
	}

	str = strtok_r(NULL, " ", &pcmdStr_log);
	ip4addr_aton_is_valid = ip4addr_aton(str, &ping_addr);  // Check address existence

	if(ip4addr_aton_is_valid){
		vif_id = wifi_get_vif_id(&ping_addr);
		for(;;)
		{
			str = strtok_r(NULL, " ", &pcmdStr_log);
			if(str == NULL || str[0] == '\0'){
				break;
			}else if(strcmp(str , "stop") == 0) {
				ping_stop_conn = ping_get_session(&ping_addr);
				if(ping_stop_conn!= NULL){
					ping_stop_conn->force_stop = 1;
				}else{
					I(TT_NET, "%s%d Nothing to stop\n", module_name(), vif_id);
				}
				mem_free(ping_conn);
				return;
			}else if(strcmp(str, "-i") == 0){
				str = strtok_r(NULL, " ", &pcmdStr_log);
				interval = PING_DELAY_UNIT * atof(str);
			}else if(strcmp(str, "-c") == 0){
				str = strtok_r(NULL, " ", &pcmdStr_log);
				count = atoi(str);
			}else if(strcmp(str, "-s") == 0){
				str = strtok_r(NULL, " ", &pcmdStr_log);
				packet_size = atoi(str);
			}else{
				E(TT_NET, "%s%d Error!! '%s' is unknown option. Run help ping\n",
					module_name(), vif_id, str);
				mem_free(ping_conn);
				return;
			}
		}
	}else{
		if(strcmp(str, "-st") == 0){
			ping_list_display();
		}else if(strcmp(str, "-h") == 0){
			ping_usage();
		}else{
			E(TT_NET, "%s Error!! There is no target address. run help ping\n",
				module_name());
		}
		mem_free(ping_conn);
		return;
	}

	if(ip4_addr_isany_val(ping_addr)){
		E(TT_NET, "%s Error!! There is no target address. run help ping\n",
			module_name());
		mem_free(ping_conn);
		return;
	}

	ip4_addr_copy((ping_conn->addr), ping_addr);
	ping_conn->interval = (u32_t)interval;
	ping_conn->target_count = (u32_t)count;
	ping_conn->packet_size = (u32_t)packet_size;
	ping_conn->vif_id  = vif_id;

	if(ping_get_session(&ping_conn->addr) != NULL){
		I(TT_NET, "%s Ping application is already running\n",
			module_name());
		mem_free(ping_conn);
		return;
	}

	ping_mutex_init();
	ping_conn->ping_thread = (sys_thread_t)ping_init((void*)ping_conn);
	if(ping_conn->ping_thread.thread_handle == NULL){
		mem_free(ping_conn);
		return;
	}
}
#endif /* LWIP_PING */

#if LWIP_IPERF
const char usage_shortstr[] = " Usage: iperf [-s|-c host] [options] [stop]\n"
                           "Try `iperf -h' for more information.\n";

const char usage_longstr1[] = "\nUsage: iperf [-s|-c host] [options] [stop]\n\n"
                           "  -s                run in server mode\n"
                           "  -c <host>         run in client mode, connecting to <host> \n";
const char usage_longstr2[] =  "\n  -u                use UDP protocols\n"
			"  -p      #         server port to listen on/connect to #\n"
			"  -B <host>         bind to the specific interface associated with address host\n";
const char usage_longstr3[] = "  -S      #         set the IP type of service(TOS), 0-255.\n"
			"  -t      #         time in seconds to transmit (default ";
const char usage_longstr4[] = "  -b      #         UDP bandwidth to send at in bits/sec, (default ";
const char usage_longstr5[] = "  -st               show current iperf operation status\n"
                           "\n  [stop]            for stopping iperf based on [-s|-c host] [options]\n"
			"                    (ex) iperf -s <host> -u stop : for stopping udp server in wlan0\n\n";

const u32_t lwiperf_default_udp_bandwidth = 10 * MEGA;
static TaskHandle_t iperf_task_handle = NULL;

enum iperf_role {IPERF_CLIENT, IPERF_SERVER, IPERF_INVALID_ROLE};
enum iperf_mode {IPERF_UDP, IPERF_TCP};
enum iperf_command_status {IPERF_COMMAND_NONE,IPERF_COMMAND_TRIGGERED};

//#define IPERF_PARAMETER_DEBUG TRUE
/*
 * Definitions for IP type of service (ip_tos)
 */
#ifndef IPTOS_LOWDELAY
# define IPTOS_LOWDELAY          0x10
# define IPTOS_THROUGHPUT        0x08
# define IPTOS_RELIABILITY       0x04
# define IPTOS_LOWCOST           0x02
# define IPTOS_MINCOST           IPTOS_LOWCOST
#endif /* IPTOS_LOWDELAY */

/*
 * Definitions for DiffServ Codepoints as per RFC2474
 */
#ifndef IPTOS_DSCP_AF11
# define	IPTOS_DSCP_AF11		0x28
# define	IPTOS_DSCP_AF12		0x30
# define	IPTOS_DSCP_AF13		0x38
# define	IPTOS_DSCP_AF21		0x48
# define	IPTOS_DSCP_AF22		0x50
# define	IPTOS_DSCP_AF23		0x58
# define	IPTOS_DSCP_AF31		0x68
# define	IPTOS_DSCP_AF32		0x70
# define	IPTOS_DSCP_AF33		0x78
# define	IPTOS_DSCP_AF41		0x88
# define	IPTOS_DSCP_AF42		0x90
# define	IPTOS_DSCP_AF43		0x98
# define	IPTOS_DSCP_EF		0xb8
#endif /* IPTOS_DSCP_AF11 */

#ifndef IPTOS_DSCP_CS0
# define	IPTOS_DSCP_CS0		0x00
# define	IPTOS_DSCP_CS1		0x20
# define	IPTOS_DSCP_CS2		0x40
# define	IPTOS_DSCP_CS3		0x60
# define	IPTOS_DSCP_CS4		0x80
# define	IPTOS_DSCP_CS5		0xa0
# define	IPTOS_DSCP_CS6		0xc0
# define	IPTOS_DSCP_CS7		0xe0
#endif /* IPTOS_DSCP_CS0 */
#ifndef IPTOS_DSCP_EF
# define	IPTOS_DSCP_EF		0xb8
#endif /* IPTOS_DSCP_EF */

static const struct {
	const char *name;
	int value;
} ipqos[] = {
	{ "af11", IPTOS_DSCP_AF11 },
	{ "af12", IPTOS_DSCP_AF12 },
	{ "af13", IPTOS_DSCP_AF13 },
	{ "af21", IPTOS_DSCP_AF21 },
	{ "af22", IPTOS_DSCP_AF22 },
	{ "af23", IPTOS_DSCP_AF23 },
	{ "af31", IPTOS_DSCP_AF31 },
	{ "af32", IPTOS_DSCP_AF32 },
	{ "af33", IPTOS_DSCP_AF33 },
	{ "af41", IPTOS_DSCP_AF41 },
	{ "af42", IPTOS_DSCP_AF42 },
	{ "af43", IPTOS_DSCP_AF43 },
	{ "cs0", IPTOS_DSCP_CS0 },
	{ "cs1", IPTOS_DSCP_CS1 },
	{ "cs2", IPTOS_DSCP_CS2 },
	{ "cs3", IPTOS_DSCP_CS3 },
	{ "cs4", IPTOS_DSCP_CS4 },
	{ "cs5", IPTOS_DSCP_CS5 },
	{ "cs6", IPTOS_DSCP_CS6 },
	{ "cs7", IPTOS_DSCP_CS7 },
	{ "ef", IPTOS_DSCP_EF },
	{ "lowdelay", IPTOS_LOWDELAY },
	{ "throughput", IPTOS_THROUGHPUT },
	{ "reliability", IPTOS_RELIABILITY },
	{ NULL, -1 }
};

int parse_qos(const char *cp)
{
	unsigned int i;
	char *ep = NULL;
	long val;

	if (cp == NULL)
		return -1;
	for (i = 0; ipqos[i].name != NULL; i++) {
		if (strcasecmp(cp, ipqos[i].name) == 0)
			return ipqos[i].value;
	}
	/* Try parsing as an integer */
	val = strtol(cp, &ep, 0);
	if (*cp == '\0' || *ep != '\0' || val < 0 || val > 255)
		return -1;
	return val;
}


static void lwiperf_report(void *arg, enum lwiperf_report_type report_type,
	const ip_addr_t* local_addr, u16_t local_port, const ip_addr_t* remote_addr, u16_t remote_port,
	u32_t bytes_transferred, u32_t ms_duration, u32_t bandwidth_kbitpsec)
{
	char *str_report_type = NULL;

	LWIP_UNUSED_ARG(arg);
	LWIP_UNUSED_ARG(local_addr);
	LWIP_UNUSED_ARG(local_port);

	lwiperf_mutex_lock();

	switch (report_type)
	{
		case LWIPERF_TCP_DONE_SERVER: 
			str_report_type = "tcp_server";
			break;

		case LWIPERF_TCP_DONE_CLIENT:
		   	str_report_type = "tcp_client";
		   	break;

		case LWIPERF_UDP_DONE_SERVER:
		   	str_report_type = "udp_server";
			break;

		case LWIPERF_UDP_DONE_CLIENT:
			str_report_type = "udp_client";
		    break;
		
		default:
			str_report_type = "unknown";
	}

	A("\n[IPERF Report] %s, Remote: %s:%d, %lu [Bytes], Duration:%d [msec]",\
			str_report_type, 
			ipaddr_ntoa(remote_addr), (int)remote_port, 
			bytes_transferred, ms_duration);
	A(", %d [kbits/s]\n", bandwidth_kbitpsec);

	if (arg)
	{
		lwiperf_report_cb_t report_cb = (lwiperf_report_cb_t)arg;

		report_cb(str_report_type, remote_addr, remote_port, 
				bytes_transferred, ms_duration, bandwidth_kbitpsec);
	}

	lwiperf_mutex_unlock();
}

void iperf_usage(void)
{
	A("%s\r\n", usage_shortstr);
}

void iperf_detailed_usage(void)
{
	lwiperf_mutex_lock();
	A("%s", usage_longstr1);
	A("%s", usage_longstr2);
	A("%s", usage_longstr3);
	A("%d sec)\n", LWIPERF_DEFAULT_DURATION);
	A("%s", usage_longstr4);
	A("%d bit/sec)\n", lwiperf_default_udp_bandwidth);
	A("%s", usage_longstr5);
	lwiperf_mutex_unlock();
}

iperf_parm_t * iperf_init_parameters(void)
{
	iperf_parm_t* parm = NULL;

	parm = (iperf_parm_t *)mem_malloc(sizeof(iperf_parm_t));
	if (parm == NULL) {
		E(TT_NET, "%s memory allocation fail!\n", module_name());
		return NULL;
	}

	parm->mode=IPERF_TCP;
	parm->role=IPERF_INVALID_ROLE;
	parm->tos = IPTOS_DSCP_CS0 ;
	parm->client_type = LWIPERF_CLIENT ;
	parm->bandwidth =lwiperf_default_udp_bandwidth;
	parm->port = 5001;
	parm->duration = LWIPERF_DEFAULT_DURATION;
	ip4_addr_set_zero(&parm->addr);

	return parm;
}

void iperf_deinit_parameters(iperf_parm_t* parm)
{
	if (parm != NULL) {
		mem_free(parm);
	}
}

void iperf_display_parameters(iperf_parm_t* parm)
{
#if defined(IPERF_PARAMETER_DEBUG) && (IPERF_PARAMETER_DEBUG == TRUE)
	lwiperf_mutex_lock();

	A("\n------------------------ iperf paramters ----------------------------\n");
	A(" %s_%s\t\t",
		parm->mode == IPERF_TCP ? "tcp":"udp",parm->role == IPERF_SERVER ? "server":"client");
	A(" %"U16_F".%"U16_F".%"U16_F".%"U16_F":%d\n",
		ip4_addr1_16(&parm->addr), ip4_addr2_16(&parm->addr),
		ip4_addr3_16(&parm->addr), ip4_addr4_16(&parm->addr), parm->port);
	A(" bandwidth: %d bits/sec\n" , parm->bandwidth);
	A(" duration: %d s\t\ttos: %d\n",parm->duration, parm->tos);
	A("----------------------------------------------------------------------\n");
	lwiperf_mutex_unlock();
#endif
}

int iperf_start_session(iperf_parm_t* parm, void *report_cb)
{
	lwiperf_state_tcp_t* s = NULL;
	int ret = false;

	if(parm->role == IPERF_SERVER){
		if(parm->mode == IPERF_TCP){
			s =lwiperf_start_tcp_server((const ip_addr_t* )&parm->addr, parm->port,lwiperf_report, report_cb);
		}else{
			s =lwiperf_start_udp_server((const ip_addr_t* )&parm->addr, parm->port,lwiperf_report, report_cb);
		}
	}else{
		if(parm->mode==IPERF_TCP){
			s =lwiperf_start_tcp_client((const ip_addr_t* )&parm->addr, parm->port, parm->client_type,
									 lwiperf_report,report_cb, parm->duration, parm->tos);
		}else{
			s =lwiperf_start_udp_client((const ip_addr_t* )&parm->addr, parm->port,parm->duration,
									 lwiperf_report,report_cb, parm->tos, parm->bandwidth, parm->client_type);
		}
	}

	iperf_display_parameters(parm);

	if(s!=NULL)
		ret = true;

	return ret;
}

int iperf_stop_session(iperf_parm_t* parm)
{
	lwiperf_state_tcp_t* conn = lwiperf_get_session(parm);
	if(conn == NULL){
		E(TT_NET, "%s There is no session for stopping\n",
			module_name());
		return false;
	}
	if(parm->role == IPERF_SERVER){
		I(TT_NET, "%s iperf %s server mode stop\n",
			module_name(), (parm->mode == IPERF_TCP) ? "tcp" : "udp");
		if(parm->mode == IPERF_TCP){
			lwiperf_tcp_server_close(conn);
		}else{
			lwiperf_udp_server_close(conn);
		}
	}else if(parm->role == IPERF_CLIENT){
		I(TT_NET, "%s iperf % client mode stop\n",
			module_name(), (parm->mode == IPERF_TCP) ? "tcp" : "udp");
		conn->force_stop = true;
	}else{
		E(TT_NET, "%s Invalid role for iperf stop session\n",
			module_name());
	}
	return true;
}

int  iperf_run(char *cmd, void *report_cb)
{
	char *str = NULL;
	char *pcmdStr_log = NULL;
	char *pcmdStr = (char *)cmd;
	char suffix = '\0';
	int len = 0;
	int i=0;
	int ip4addr_aton_is_valid = false;
	int ret = true;
	ip4_addr_t addr_temp;

	iperf_parm_t* iperf_parm = NULL;

	if(pcmdStr != NULL)
		memcpy((char **)&pcmdStr_log, (char **)&pcmdStr, sizeof((char *)&pcmdStr));

	iperf_parm = iperf_init_parameters();
	if(iperf_parm == NULL){
		E(TT_NET, "iperf parameter init fail!!\n");
		return false;
	}

	lwiperf_mutex_init();

	for(;;)
	{
		str = strtok_r(NULL, " ", &pcmdStr_log);
		if(str == NULL || str[0] == '\0'){
			break;
		}else if(strcmp(str, "-u") == 0){
			iperf_parm->mode = IPERF_UDP;
		}else if(strcmp(str, "-s") == 0){
			iperf_parm->role = IPERF_SERVER;
		}else if(strcmp(str, "-c") == 0){
			iperf_parm->role  = IPERF_CLIENT;
			str = strtok_r(NULL, " ", &pcmdStr_log);
			ip4addr_aton_is_valid = ip4addr_aton(str, &iperf_parm->addr);
			if(!ip4addr_aton_is_valid){
				E(TT_NET, "%s error!! unknown address value\n",
					module_name());
				ret = false;
				goto free_and_return;
			}
		}else if(strcmp(str, "-t") == 0){
			str = strtok_r(NULL, " ", &pcmdStr_log);
			iperf_parm->duration = atoi(str);
		}else if(strcmp(str, "-b") == 0){
			str = strtok_r(NULL, " ", &pcmdStr_log);
			len = strlen(str);
			suffix = str[len-1];
			switch(suffix)
			{
				case 'm': case 'M':
					str[len-1]='\0';
					iperf_parm->bandwidth = atof(str)*MEGA;
					break;
				case 'k': case 'K':
					str[len-1]='\0';
					iperf_parm->bandwidth = atof(str)*KILO;
					break;
				default:
					iperf_parm->bandwidth = atof(str);
					break;
			}
		}else if(strcmp(str, "-p") == 0){
			str = strtok_r(NULL, " ", &pcmdStr_log);
			iperf_parm->port = atoi(str);
		}else if(strcmp(str, "-S") == 0){
			str = strtok_r(NULL, " ", &pcmdStr_log);
			iperf_parm->tos = parse_qos(str);
		}else if(strcmp(str , "stop") == 0) {
			goto iperf_stop;
		}else if(strcmp(str, "-st") == 0){
			lwiperf_list_display();
			goto free_and_return;
		}else if(strcmp(str, "-h") == 0 || strcmp(str, "-help") == 0){
			iperf_detailed_usage();
			goto free_and_return;
		}else{
			E(TT_NET, "%s unknown options : %s\n",
				module_name(), str);
		}
	}

	if(iperf_parm->role == IPERF_CLIENT){
		if(ip4_addr_isany_val(iperf_parm->addr)){
			E(TT_NET, "%s There is no ip address. Add ip address\n",
				module_name());
			return false;
		}

		if(lwiperf_get_session(iperf_parm) == NULL){
			ret = iperf_start_session(iperf_parm, report_cb);
		}else{
			I(TT_NET, "%s iperf application is already running\n",
				module_name());
			ret = false;
		}
	}else if(iperf_parm->role == IPERF_SERVER){
		for(i=0; i<MAX_IF; i++){
			if(!ip4_addr_isany_val(nrc_netif[i]->ip_addr)){
				ip4_addr_copy(iperf_parm->addr, nrc_netif[i]->ip_addr);

				if(lwiperf_get_session(iperf_parm) == NULL){
					ret = iperf_start_session(iperf_parm, report_cb);
				}else{
					I(TT_NET, "%s iperf application is already running\n",
						module_name());
					ret = false;
				}
			}
		}
	}else{
		iperf_usage();
		ret = false;
	}
	goto free_and_return;

iperf_stop:
	if(iperf_parm->role == IPERF_SERVER){
		for(i=0; i<MAX_IF; i++){
			if(!ip4_addr_isany_val(nrc_netif[i]->ip_addr)){
				ip4_addr_copy(iperf_parm->addr, nrc_netif[i]->ip_addr);
				iperf_stop_session(iperf_parm);
			}
		}
	}else{
		iperf_stop_session(iperf_parm);
	}

free_and_return:
	iperf_deinit_parameters(iperf_parm);
	return ret;
}

#endif /* LWIP_IPERF */


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

void setup_wifi_ap_mode(int vif)
{
	A("%s\n", __func__);

	wifi_set_opmode((wifi_get_opmode()|WIFI_STATIONAP_MODE)&USE_WIFI_MODE);
	wifi_get_ip_info(vif, &ipinfo[vif]);

#if !defined(NRC_USER_APP)
	/* Set default IP address */
	IP4_ADDR(&ipinfo[vif].ip, 192, 168, 50, 1);
	IP4_ADDR(&ipinfo[vif].gw, 192, 168, 50, 1);
	IP4_ADDR(&ipinfo[vif].netmask, 255, 255, 255, 0);
#endif /* ! NRC_USER_APP */
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
	dhcps_start(&ipinfo[vif], vif);
#endif

}

#if 0
bool wifi_softap_dhcps_start(void)
{
	dhcps_start(&ipinfo[SOFTAP_IF]);
	return true;
}

bool wifi_softap_dhcps_stop(void)
{
	dhcps_stop();
	return true;
}
#endif

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
		A("%d netif_is_up, local interface IP is %s\n\n",
			module_name(), ip4addr_ntoa(netif_ip4_addr(state_netif)));

		if (!ip4_addr_isany_val(*netif_ip4_addr(state_netif))) {
			A("%s IP is ready\n", module_name());
			set_dhcp_status(true);
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
