LWIP_BASE_INC	= $(LWIP_BASE)/include
LWIP_BASE_APPS	= $(LWIP_BASE)/apps
LWIP_LIB		= $(LWIP_BASE)/lwip

LWIP_LIB_SRC	= $(LWIP_LIB)/src
LWIP_INC		= $(LWIP_LIB_SRC)/include
LWIP_CORE		= $(LWIP_LIB_SRC)/core
LWIP_API		= $(LWIP_LIB_SRC)/api
LWIP_NETIF		= $(LWIP_LIB_SRC)/netif
LWIP_APPS		= $(LWIP_LIB_SRC)/apps
LWIP_IPV4		= $(LWIP_CORE)/ipv4
LWIP_IPV6		= $(LWIP_CORE)/ipv6
SNMP_APP		= $(LWIP_APPS)/snmp

LWIP_COMPAT		= $(LWIP_INC)/compat
LWIP_POSIX		= $(LWIP_COMPAT)/posix

LWIP_PORT			= $(LWIP_BASE)/port
LWIP_PORT_INC		= $(LWIP_PORT)/include
LWIP_PORT_ARCH		= $(LWIP_PORT)/arch
LWIP_PORT_ARCH_INC	= $(LWIP_PORT_INC)/arch
LWIP_PORT_FREERTOS	= $(LWIP_PORT)/freertos
LWIP_PORT_NETIF		= $(LWIP_PORT)/netif
LWIP_PORT_NETIF_INC	= $(LWIP_PORT_INC)/netif
LWIP_PORT_NAT		= $(LWIP_PORT)/lwip-nat
LWIP_PORT_NAT_INC	= $(LWIP_PORT_INC)/lwip-nat

LWIP_APPS_INC	= $(LWIP_BASE_INC)/apps

LWIP_PING		= $(LWIP_BASE_APPS)/ping
LWIP_IPERF		= $(LWIP_BASE_APPS)/iperf
LWIP_DHCPS		= $(LWIP_BASE_APPS)/dhcpserver

LWIP_PING_INC		= $(LWIP_APPS_INC)/ping
LWIP_IPERF_INC		= $(LWIP_APPS_INC)/iperf
LWIP_DHCPS_INC		= $(LWIP_APPS_INC)/dhcpserver

INCLUDE += -I$(LWIP_BASE_INC)
INCLUDE += -I$(LWIP_APPS_INC)
INCLUDE += -I$(LWIP_PING_INC)
INCLUDE += -I$(LWIP_IPERF_INC)
INCLUDE += -I$(LWIP_DHCPS_INC)
INCLUDE += -I$(LWIP_INC)
INCLUDE += -I$(LWIP_PORT_INC)
INCLUDE += -I$(LWIP_PORT_ARCH_INC)
INCLUDE += -I$(LWIP_PORT_NETIF_INC)
INCLUDE += -I$(LWIP_PORT_NAT_INC)
INCLUDE += -I$(LWIP_POSIX)
INCLUDE += -I$(SNMP_APP)

VPATH	+= $(LWIP_CORE)
VPATH	+= $(LWIP_NETIF)
VPATH	+= $(LWIP_API)
VPATH	+= $(LWIP_IPV4)
VPATH	+= $(LWIP_IPV6)
VPATH	+= $(LWIP_PORT)
VPATH	+= $(LWIP_PORT_ARCH)
VPATH	+= $(LWIP_PORT_FREERTOS)
VPATH	+= $(LWIP_PORT_NETIF)
VPATH	+= $(LWIP_PING)
VPATH	+= $(LWIP_IPERF)
VPATH	+= $(LWIP_DHCPS)
VPATH	+= $(LWIP_PORT_NAT)
VPATH	+= $(SNMP_APP)

DEFINE	+= -DNRC_LWIP

# COREFILES, CORE4FILES: The minimum set of files needed for lwIP.
COREFILES	= \
	init.c \
	def.c \
	dns.c \
	inet_chksum.c \
	ip.c \
	mem.c \
	memp.c \
	netif.c \
	pbuf.c \
	raw.c \
	stats.c \
	sys.c \
	altcp.c \
	altcp_alloc.c \
	altcp_tcp.c \
	tcp.c \
	tcp_in.c \
	tcp_out.c \
	timeouts.c \
	udp.c

CORE4FILES	= \
	autoip.c \
	dhcp.c \
	etharp.c \
	icmp.c \
	igmp.c \
	ip4_frag.c \
	ip4.c \
	ip4_addr.c

# APIFILES: The files which implement the sequential and socket APIs.
APIFILES= \
	api_lib.c \
	api_msg.c \
	err.c \
	if_api.c \
	netbuf.c \
	netdb.c \
	netifapi.c \
	sockets.c \
	tcpip.c

# NETIFFILES: Files implementing various generic network interface functions
NETIFFILES= \
	ethernet.c \
	bridgeif.c \
	bridgeif_fdb.c \
	slipif.c

# PORTING_FILES
LWIP_PORTING = \
	sys_arch.c \
	wlif.c \
	nrc_ping.c \
	nrc_iperf.c \
	nrc_lwip.c

# LWIP NAT FILES
LWIP_NAT_FILES = \
	ip4_input_nat.c \
	ip4_prerouting_hook.c \
	nat.c \
	nat_proto_icmp4.c \
	nat_proto_ip4.c \
	nat_proto_tcp.c \
	nat_proto_udp.c

ifeq ($(CONFIG_ETHERNET), y)
LWIP_PORTING_ETH = \
	nrc_eth_if.c \
	$(LWIP_NAT_FILES)
endif

LWIP_APP= \
	ping_task.c \
	nrc_iperf_tcp.c\
	nrc_iperf_udp.c\

# IPv4
LWIP_APP+= \
	ping.c \
	captdns.c \
	dhcpserver.c \

# IPv6
ifeq ($(CONFIG_IPV6), y)
LWIP_APP+= \
	ping6.c

CORE6FILES	= \
	dhcp6.c \
	ethip6.c \
	icmp6.c \
	inet6.c \
	ip6.c \
	ip6_addr.c \
	ip6_frag.c \
	mld6.c \
	nd6.c

CSRCS += \
	$(CORE6FILES)
endif

# SNMPv3 agent
LWIP_SNMP_CSRCS = \
	snmp_asn1.c \
	snmp_core.c \
	snmp_mib2.c \
	snmp_mib2_icmp.c \
	snmp_mib2_interfaces.c \
	snmp_mib2_ip.c \
	snmp_mib2_snmp.c \
	snmp_mib2_system.c \
	snmp_mib2_tcp.c \
	snmp_mib2_udp.c \
	snmp_snmpv2_framework.c \
	snmp_snmpv2_usm.c \
	snmp_msg.c \
	snmpv3.c \
	snmp_netconn.c \
	snmp_pbuf_stream.c \
	snmp_raw.c \
	snmp_scalar.c \
	snmp_table.c \
	snmp_threadsync.c \
	snmp_traps.c \

LWIP_APP+= \
	$(LWIP_SNMP_CSRCS)

# add files to CSRCS list
CSRCS += \
	$(COREFILES) \
	$(CORE4FILES) \
	$(APIFILES) \
	$(NETIFFILES) \
	$(LWIP_PORTING)\
	$(LWIP_APP) \
	$(LWIP_PORTING_ETH)
