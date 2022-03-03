#include "system_common.h"

#include "utils/includes.h"
#include "utils/common.h"
#include "utils/eloop.h"
#include "utils/wpa_debug.h"
#include "driver.h"
#include "l2_packet.h"
#include "driver_nrc_tx.h"

#ifdef TAG
#undef TAG
#endif
#define TAG "wpa: "

static struct l2_packet_data *l2_head = NULL;

struct l2_packet_data {
	char ifname[8];
	void *rx_cb_ctx;
	void (*rx_cb)(void *ctx, const u8 *src, const u8 *buf, size_t len);
	int l2_hdr;
	uint8_t addr[ETH_ALEN];
	unsigned short protocol;
	struct l2_packet_data * next;
};

int wpas_l2_packet_filter(uint8_t *buffer, int len)
{
	struct l2_packet_data *l2 = l2_head;
	struct l2_ethhdr *hdr = (struct l2_ethhdr *) buffer;
	const int HLEN = sizeof(struct l2_ethhdr);

	if (len < HLEN)
		return 0;

	while (l2) {
		if (l2->protocol == hdr->h_proto) {
			if (memcmp(hdr->h_dest, l2->addr, ETH_ALEN) == 0) {
				l2->rx_cb(l2->rx_cb_ctx, hdr->h_source,
						buffer + HLEN, len - HLEN);
				return 1;
			}
		}
		l2 = l2->next;
	}

	return 0;
}

struct l2_packet_data * l2_packet_init(const char *ifname, const u8 *own_addr,
			unsigned short protocol,
			void (*rx_cb)(void *ctx, const u8 *src, const u8 *buf, size_t len),
			void *rx_cb_ctx,
			int l2_hdr)
{
	struct l2_packet_data *l2 = NULL, *l2_pos = NULL;

	if (protocol != ETH_P_EAPOL) {
		E(TT_WPAS, TAG "Unsupported l2_pkt requested (0x%X)\n", protocol);
		return NULL;
	}

	l2 = (struct l2_packet_data *) os_zalloc(sizeof(*l2));

	if (!l2) {
		E(TT_WPAS, TAG "Failed to allocate l2_pkt\n");
		return NULL;
	}

	os_strlcpy(l2->ifname, ifname, sizeof(l2->ifname));

	l2->rx_cb = rx_cb;
	l2->rx_cb_ctx = rx_cb_ctx;
	l2->l2_hdr = l2_hdr;
	os_memcpy(l2->addr, own_addr, ETH_ALEN);
	l2->protocol = htons(protocol);
	l2->next = NULL;

	if (!l2_head)
		l2_head = l2;
	else {
		l2_pos = l2_head;
		while (l2_pos->next)
			l2_pos = l2_pos->next;
		l2_pos->next = l2;
	}
	I(TT_WPAS, TAG "l2_pkt initialized (proto: 0x%X)\n", protocol);

	return l2;
}

struct l2_packet_data * l2_packet_init_bridge(const char *br_ifname,
			const char *ifname, const u8 *own_addr,
			unsigned short protocol,
			void (*rx_cb)(void *ctx, const u8 *src_addr,
				const u8 *buf, size_t len),
			void *rx_cb_ctx, int l2_hdr)
{
	return NULL;
}

int l2_packet_send(struct l2_packet_data *l2, const u8 *dst_addr, u16 proto,
			const u8 *buf, size_t len)
{
	int ret = 0;
	uint8_t vif_id = 0;

	if (os_strcmp(l2->ifname, "wlan1") == 0)
		vif_id = 1;

	if (!l2 || !buf || len < sizeof(struct l2_ethhdr))
		return -1;

	if (l2->l2_hdr){
		ret = nrc_transmit_from_8023(vif_id, (uint8_t *) buf, len);
		if (ret < 0) {
			I(TT_WPAS, TAG "l2_packet_send (l2->l2_hdr) failed\n");
			return -1;
		}
	} else {
		struct l2_ethhdr *eth = (struct l2_ethhdr *) os_malloc(sizeof(*eth)
										+ len);
		if (eth == NULL)
			return -1;

		os_memcpy(eth->h_dest, dst_addr, ETH_ALEN);
		os_memcpy(eth->h_source, l2->addr, ETH_ALEN);
		eth->h_proto = htons(proto);
		os_memcpy(eth + 1, buf, len);
		ret = nrc_transmit_from_8023(vif_id,(uint8_t *) eth, len + sizeof(*eth));
		os_free(eth);

		if (ret < 0) {
			I(TT_WPAS, TAG "l2_packet_send failed\n");
			return -1;
		}
	}

	return 0;
}

int l2_packet_get_ip_addr(struct l2_packet_data *l2, char *buf, size_t len)
{
	/* TODO: get interface IP address */
	return -1;
}

void l2_packet_notify_auth_start(struct l2_packet_data *l2)
{
	/* Do nothing */
}

int l2_packet_set_packet_filter(struct l2_packet_data *l2,
				enum l2_packet_filter_type type)
{
	return -1;
}

int l2_packet_get_own_addr(struct l2_packet_data *l2, u8 *addr)
{
	os_memcpy(addr, l2->addr, ETH_ALEN);
	return 0;
}

void l2_packet_deinit(struct l2_packet_data *l2)
{
	struct l2_packet_data *l2_pos = l2_head;

	if (!l2)
		return;

	if (l2 == l2_head) {
		os_free(l2_head);
		l2_head = NULL;
	}

	while (l2_pos) {
		if (l2_pos->next == l2) {
			l2_pos->next = l2->next;
			os_free(l2);
		}
	}
}

#undef TAG
