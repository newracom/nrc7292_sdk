# lwIP DNAT Support

This add-on provides basic DNAT support for lwIP. Currently supported
protocols are:
	- IPv4
	- TCP
	- UDP
	- ICMP4

The code is organized to make it easy to add support for additional protocols.

Ideally, DNAT operates at a prerouting step, analyzing packets and making
modifications before a routing decision is made. lwIP does not currently
offer a prerouting hook, so the ip input hook is used instead. Much of the
basic ip input functionality is duplicated within the hook contained in
ip4_input_nat.c. This includes verifying the packet's IP header integrity,
performing reassembly, and determining where the packet would be routed.
The information is then passed on to the prerouting hook and then the packet
is returned to ip4_input.

Reassembly is necessary for DNAT because only the first fragment contains the
protocol headers. Normally lwIP does not reassemble packets unless there are
to be delivered locally.

DNAT can be configured in three different ways:

* Source Interface - All packets forwarded outbound from the source interface
network have their source address rewritten to match the outbound interface.
* Outbound Interface - All packets forwarded outbound via the outbound
interface have their source address rewritten to match the outbound interface.
* Source/Outbound Pair - Only packets forwarded from the given source
interface out via the outbound interface have their source address rewritten
to match the outbound interface

Any packets that would be forwarded in the opposite direction of such a rule
are dropped. A call to nat_rule_add with an appropriate struct nat_rule is
used to add a new DNAT rule.

# Protocol Support

Each protocol should support 4 basic functions:

	* Find/generate DNAT rule for packet
	* Modify packet based on DNAT rule
	* Find/generate DNAT rule for ICMP embedded header
	* Modify ICMP embedded header based on DNAT rule

New DNAT rules should only be generated for packets being forwarded from the
source interface to the outbound interface. Protocols that embed others
(such as UDP within IPv4) should chain together starting with the outermost
protocol (IPv4).

# Connection Tracking

Connection tracking is peformed using a struct nat_pcb. Each protocol should
use the same header, but the trailing bytes will vary based on protocol. At the
top level, the following information is tracked:

	* Source Interface (int_netif_idx)
	* Outbound Interface (ext_netif_idx)
	* Expiration time (timeout)
	* Remote IP (remote_ip)
	* DNAT Source IP (nat_local_ip)
	* Outbound IP (local_ip)

When packets are sent, the nat_local_ip is rewritten withe local_ip and
vice-versa on return. Each enclosed protocol will have additional data that
needs to be tracked, such as port numbers.

Note that for TCP and UDP, a portion of the data structure matches the struct
tcp_pcb and struct tcp_udp respectively. This is so that the connection
tracking information can also be shared with lwIP so that lwIP does not reuse
the port number.

# lwIP Integration

The code is currently standalone but could be integrated with lwIP for a
cleaner implementation and smaller codesize. The ip4_input function would
need to be modified to perform reassembly and then calling the prerouting
hook before making a routing decision if NAT is active. The TCP and UDP code
could also be modified to make the tracking of NAT connections clearer.

# License

DNAT support for lwIP is released under the same 3-clause BSD terms as lwIP.
Copyright and authorship is Russ Dill <russ.dill@gmail.com>.

ip4_input_nat.c copied from src/core/ipv4/ip4.c and maintains it's original
authorship and copyright.

