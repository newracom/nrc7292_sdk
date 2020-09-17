#include "system_common.h"
#include "system_memory_manager.h"
#include "system_modem_api.h"
#include "umac_s1g_agent.h"
#include "lmac_common.h"
#include "utils/includes.h"
#include "utils/common.h"
#include "../common/wpa_common.h"
#include "../common/ieee802_11_common.h"
#include "../common/eapol_common.h"
#include "utils/eloop.h"
#include "utils/wpa_debug.h"
#include "driver_nrc.h"
#include "driver_nrc_scan.h"
#include "driver_nrc_debug.h"
#include "nrc-wim-types.h"
//#include "lwip/pbuf.h"

#ifdef CONFIG_NO_STDOUT_DEBUG
#ifdef wpa_printf
#undef wpa_printf
static void wpa_printf(int level, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	system_vprintf(fmt, ap);
	system_printf("\n");
}
#endif
#endif


static int nrc_add_ccmp_hdr(struct nrc_wpa_key *key, uint8_t* pos)
{
	if (!key)
		return 0;

	uint64_t pn = key->tsc;

	*pos++ = pn;		// PN 0
	*pos++ = pn >> 8;	// PN 1
	*pos++ = 0;		// RSV
	*pos++ = 0x20 | key->ix << 6;
	*pos++ = pn >> 16;	// PN 2
	*pos++ = pn >> 24;	// PN 3
	*pos++ = pn >> 32;	// PN 4
	*pos++ = pn >> 40;	// PN 5

	key->tsc++;

	return 8;
}

static int nrc_add_tkip_hdr(struct nrc_wpa_key *key, uint8_t* pos)
{
	if (!key)
		return 0;

	// Upper 32 bits of tsc for IV32 and lower 16 bits for IV16
	uint16_t iv16 = key->tsc & 0xFFFF;
	uint32_t iv32 = key->tsc >> 32;

	if (++iv16 == 0)
		iv32++;

	*pos++ = iv16 >> 8;	// TSC 1
	*pos++ = ((iv16 >> 8) | 0x20) & 0x7f;	// WEP Seed
	*pos++ = iv16 & 0xFF;	// TSC 0

	*pos++ = (key->ix << 6) | (1 << 5);	// Rsv|EXT IV|Key ID

	*pos++ = iv32;	//TSC 2
	*pos++ = iv32 >> 8;	//TSC 3
	*pos++ = iv32 >> 16;	//TSC 4
	*pos++ = iv32 >> 24;	//TSC 5

	key->tsc = iv32;
	key->tsc <<= 32;
	key->tsc |= iv16;

	return 8;
}

static bool is_weak_wep(struct nrc_wpa_key *key, uint32_t iv)
{
	uint8_t t = 0;
	if ((iv & 0xff00) == 0xff00) {
		uint8_t t = (iv >> 16) & 0xff;
		if (t >= 3 && t < 3 + key->key_len)
			return true;
	}
	return false;
}

static int nrc_add_wep_hdr(struct nrc_wpa_key *key, uint8_t* pos)
{
	if (!key)
		return 0;

	uint32_t iv = key->tsc;

	iv++;
	if (is_weak_wep(key, iv))
		iv += 0x100;

	*pos++ = (iv >> 16) & 0xff;
	*pos++ = (iv >> 8) & 0xff;
	*pos++ = iv & 0xff;
	*pos++ = key->ix << 6;

	key->tsc = iv;

	return 4;
}

static int nrc_add_sec_hdr(struct nrc_wpa_key *key, uint8_t *pos)
{
	int sec = 0, cipher = 0;

	if (!key || key->cipher == WIM_CIPHER_TYPE_NONE)
		return 0;

	cipher = key->cipher;
	switch (cipher) {
		case WIM_CIPHER_TYPE_CCMP:
			sec = nrc_add_ccmp_hdr(key, pos);
			break;
		case WIM_CIPHER_TYPE_TKIP:
			sec = nrc_add_tkip_hdr(key, pos);
			break;
		case WIM_CIPHER_TYPE_WEP40:
		case WIM_CIPHER_TYPE_WEP104:
			sec = nrc_add_wep_hdr(key, pos);
			break;
		default:
			wpa_printf(MSG_ERROR, "Unsupp cipher (type: %d)", cipher);
	}
	return sec;
}

static SYS_BUF * alloc_sys_buf(int hif_len) {
#ifndef NRC7291_SDK_DUAL_CM3
	const int POOL_TX = 1;
	return sys_buf_alloc(POOL_TX, hif_len);
#else
	return  mbx_alloc_sys_buf_hif_len(hif_len);
#endif
}

SYS_BUF * alloc_sys_buf_try(int hif_len, int nTry) {
	SYS_BUF * buf = NULL;
	const int LMAC_ALLOC_SYS_BUF_DELAY = 5; // ms

	while (nTry--) {
		buf = alloc_sys_buf(hif_len);
		if (buf)
			break;
		vTaskDelay(pdMS_TO_TICKS(LMAC_ALLOC_SYS_BUF_DELAY));
	}
	return buf;
}

static int copy_to_sysbuf(SYS_BUF *to, uint8_t* from, uint16_t remain) {
	uint8_t *pos = from;
	int len = SYS_BUF_LENGTH(to) - sizeof(LMAC_TXHDR) - sizeof(PHY_TXVECTOR);
	memcpy(&to->payload, pos, len);
	pos += len; remain -= len;
	while ((to = SYS_BUF_LINK(to)) && remain > 0) {
		len = SYS_BUF_LENGTH(to) - sizeof(LMAC_TXHDR);
		memcpy(&to->more_payload, pos, min(len, remain));
		pos += len;
		remain -= len;
	}
	return pos - from;
}

static uint8_t get_tid(SYS_BUF *buffer) {
	GenericMacHeader *gmh = &TX_MAC_HDR(buffer);
	switch (gmh->type) {
		case FC_PV0_TYPE_DATA:
		if (gmh->subtype == FC_PV0_TYPE_DATA_QOS_DATA ||
			gmh->subtype == FC_PV0_TYPE_DATA_QOS_NULL)
				return TX_MAC_HDR(buffer).qos_tid;
		break;
	}
	return MAX_TID;
}

static uint8_t get_ac(SYS_BUF *buffer) {
	GenericMacHeader *gmh = &TX_MAC_HDR(buffer);
	uint8_t ac = ACI_BE;  //AC_BK(0), AC_BE(1), AC_VI(2), AC_VO(3)

	switch (gmh->type) {
		case FC_PV0_TYPE_MGMT:
		return ACI_VO;
		break;
	}
	return ACI_BE;
}

int nrc_raw_transmit(struct nrc_wpa_if* intf, uint8_t *frm, const uint16_t len,
				const int ac)
{
	const int nAllocTry = 10;
	int hif_len = len + sizeof(FRAME_HDR);
	SYS_BUF *buffer = NULL;
	uint8_t cipher = WIM_CIPHER_TYPE_NONE;
	struct ieee80211_hdr *hdr = (struct ieee80211_hdr *) frm;
	uint8_t *pos = (uint8_t *) (hdr + 1);

	buffer = alloc_sys_buf_try(hif_len, nAllocTry);

	if (!buffer)
		return -1;

	if (hdr->frame_control & WLAN_FC_ISWEP) {
		struct nrc_wpa_key *wpa_key = nrc_wpa_get_key(intf, hdr->addr1);

		if (hdr->frame_control & (WLAN_FC_STYPE_QOS_DATA << 4))
			pos += 2;

		cipher = wpa_key->cipher;
		nrc_add_sec_hdr(wpa_key, pos);
	}
	copy_to_sysbuf(buffer, frm, len);
	memset( &HIF_HDR(buffer) , 0 , sizeof(HIF_HDR) );

	HIF_HDR(buffer).type        = HIF_TYPE_FRAME;
	HIF_HDR(buffer).len         = hif_len;
	HIF_HDR(buffer).vifindex    = intf->vif_id;

	FRAME_HDR(buffer).info.tx.tlv_len    = 0;
	FRAME_HDR(buffer).info.tx.cipher     = cipher;
	FRAME_HDR(buffer).flags.tx.ac = ac;

	bool ret = umac_s1g_agent_convert_legacy_to_s1g(&buffer);
	if (!ret) {
		system_memory_pool_free(buffer);
		system_printf("[%s] UL req sysbuf\n", __func__);
		return 0;
	}

#ifndef NRC7291_SDK_DUAL_CM3

	LMAC_PS_SEM_TAKE(MODEM_SEMAPHORE_TASK);
	lmac_uplink_request_sysbuf(buffer);
	LMAC_PS_SEM_GIVE(MODEM_SEMAPHORE_TASK);
#else
	nrc_mbx_send_data_address(buffer);
#endif
	return 0;
}

int nrc_transmit(struct nrc_wpa_if* intf, uint8_t *frm, const uint16_t len)
{
	return nrc_raw_transmit(intf, frm, len, ACI_VO);
}

static int nrc_get_80211_hdr(struct ieee80211_hdr *hdr,
									struct nrc_wpa_if *intf,
									struct ieee8023_hdr *eth_hdr,
									struct nrc_wpa_key *wpa_key,
									bool qos)
{
	int hdrlen = sizeof(*hdr);
	os_memset(hdr, 0, sizeof(struct ieee80211_hdr));

	hdr->frame_control = _FC(DATA, DATA);

	if (qos) {
		hdr->frame_control = _FC(DATA, QOS_DATA);
		hdrlen += 2;
	}

	if (!intf->is_ap) {
		os_memcpy(hdr->addr1, intf->bss.bssid, ETH_ALEN); // BSSID
		os_memcpy(hdr->addr2, eth_hdr->src, ETH_ALEN); // SA
		os_memcpy(hdr->addr3, eth_hdr->dest, ETH_ALEN); // DA
		hdr->frame_control |= WLAN_FC_TODS;
	} else {
		os_memcpy(hdr->addr1, eth_hdr->dest, ETH_ALEN); // DA
		os_memcpy(hdr->addr2, intf->bss.bssid, ETH_ALEN); // BSSID
		os_memcpy(hdr->addr3, eth_hdr->src, ETH_ALEN); // SA
		hdr->frame_control |= WLAN_FC_FROMDS;
	}

	if (wpa_key && wpa_key->cipher != WIM_CIPHER_TYPE_NONE)
		hdr->frame_control |= WLAN_FC_ISWEP;

	return hdrlen;
}


int nrc_get_llc_len(uint16_t ethertype)
{
	return (ethertype >= ETH_P_802_3_MIN) ? 8 : 0;
}

int nrc_add_llc(uint16_t ethertype, uint8_t *pos)
{
	uint8_t* llc = pos;
	const uint8_t rfc1042_header[]			= { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00 };
	const uint8_t bridge_tunnel_header[]	= { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0xf8 };

	if (ethertype < ETH_P_802_3_MIN)
		return 0;

	if (ethertype == ETH_P_AARP || ethertype == ETH_P_IPX) {
		os_memcpy(llc, bridge_tunnel_header, 6);
		llc += sizeof(bridge_tunnel_header);
	} else if (ethertype >= ETH_P_802_3_MIN) {
		os_memcpy(llc, rfc1042_header, 6);
		llc += sizeof(rfc1042_header);
	}

	*llc++ = (ethertype >> 8);
	*llc++ = (ethertype & 0xFF);

	return (llc - pos);
}

int nrc_add_qos(uint8_t dscp, uint8_t *pos)
{
	/**
	 * Fill this value with IP TOS field
	*/

	uint8_t tid = (dscp & 0xe0) >> 5;

	*pos++ = tid;
	*pos++ = 0x0;

	return 2;
}

#define NRC_WPA_BUFFER_ALLOC_TRY (10)

int nrc_transmit_from_8023(uint8_t vif_id, uint8_t *frame, const uint16_t len) {
	uint8_t *frames[] = {frame,};
	uint16_t lens[] = {len,};
	return nrc_transmit_from_8023_mb(vif_id, frames, lens, 1);
}

int nrc_transmit_from_8023_mb(uint8_t vif_id, uint8_t **frames, const uint16_t len[], int n_frames)
{
	struct nrc_wpa_if *intf = wpa_driver_get_interface(vif_id);
	struct ieee80211_hdr hdr;
	uint16_t hif_len = 0;
	struct ieee8023_hdr *eth_hdr = (struct ieee8023_hdr *) frames[0];
	bool qos = false;
	struct nrc_wpa_key *wpa_key = NULL;
	uint16_t ethertype;
	struct nrc_wpa_sta *dsta = NULL;
	int i = 0, j = 0, remain = 0;
	SYS_BUF *buffer = NULL, *cur_buf = NULL;
	uint8_t *pos, *limit;
	uint8_t tid, ac;

	if (!frames || !frames[0])
		return 0;

	dsta = nrc_wpa_find_sta(intf, eth_hdr->dest);

	if (!dsta) {
		wpa_printf(MSG_ERROR, "Failed to find sta (" MACSTR ")",
				MAC2STR(eth_hdr->dest));
		return 0;
	}
	ethertype = (frames[0][12] << 8) | frames[0][13];

	if (dsta->qos && ethertype != ETH_P_EAPOL)
		qos = true;

	if (intf->is_ap)
		wpa_key = nrc_wpa_get_key(intf, eth_hdr->dest);
	else
		wpa_key = nrc_wpa_get_key(intf, intf->bss.bssid);

	//wpa_driver_debug_key(wpa_key);
	hif_len = sizeof(FRAME_HDR);
	hif_len += nrc_get_80211_hdr(&hdr, intf, eth_hdr, wpa_key, qos);
	hif_len += nrc_get_sec_hdr_len(wpa_key);
	hif_len += nrc_get_llc_len(ethertype);

	for (i = 0; i < n_frames; i++)
		hif_len += len[i];
	hif_len -= HLEN_8023;

	buffer = alloc_sys_buf_try(hif_len, NRC_WPA_BUFFER_ALLOC_TRY);

	if (!buffer) {
		//TODO: add stats
		//wpa_printf(MSG_ERROR, "Failed to allocate tx buf.");
		return -1;
	}

	cur_buf = buffer;
	pos = cur_buf->payload;
	remain = SYS_BUF_DATA_LENGTH(cur_buf);

	//wpa_printf(MSG_ERROR, " n_frames = %d", n_frames);
	for (i = 0; i < n_frames; i++) {
		uint8_t *frame_pos = frames[i];
		uint16_t frame_len = len[i];
		int copy = 0;
		uint8_t *limit = pos + remain;

		if (i == 0) { // 80233 -> 80211
			//remain -= (sizeof(HIF_HDR) + sizeof(FRAME_HDR));
			remain -= sizeof(PHY_TXVECTOR);
			frame_pos += HLEN_8023;
			frame_len -= HLEN_8023;
			os_memcpy(pos, &hdr, sizeof(hdr));
			pos += sizeof(hdr);
			if (qos) {
				uint8_t dscp = frames[0][15];
				pos += nrc_add_qos(dscp, pos);
			}
			pos += nrc_add_sec_hdr(wpa_key, pos);
			pos += nrc_add_llc(ethertype, pos);
			remain -= (pos - cur_buf->payload);
		}
		copy = min(frame_len, remain);

		os_memcpy(pos, frame_pos, copy);
		remain -= copy;

		//frame_len -= remain; frame_pos += remain;
		pos += copy; frame_len -= copy; frame_pos += copy;

		while (frame_len > 0) {
			cur_buf = SYS_BUF_LINK(cur_buf);
			pos = cur_buf->more_payload;
			remain = SYS_BUF_DATA_LENGTH(cur_buf);
			copy = min(frame_len, remain);

			os_memcpy(pos, frame_pos, copy);
			pos += copy; frame_len -= copy; frame_pos += copy;
			remain -= copy;
		}
	}

	memset(&HIF_HDR(buffer) , 0, sizeof(HIF_HDR));

	HIF_HDR(buffer).type = HIF_TYPE_FRAME;
	HIF_HDR(buffer).len = hif_len;
	HIF_HDR(buffer).vifindex = vif_id;

	tid = get_tid(buffer);
	ac = (tid < MAX_TID) ? TID_TO_AC(tid) : get_ac(buffer);

	FRAME_HDR(buffer).info.tx.tlv_len = 0;
	FRAME_HDR(buffer).info.tx.cipher = wpa_key ? wpa_key->cipher : WIM_CIPHER_TYPE_NONE;
	FRAME_HDR(buffer).flags.tx.ac = ac;

#ifndef NRC7291_SDK_DUAL_CM3
	LMAC_PS_SEM_TAKE(MODEM_SEMAPHORE_TASK);
	if (tid < MAX_TID && dsta->block_ack[tid] == BLOCK_ACK_TX) {
		SYS_BUF_TO_LMAC_TXBUF(buffer)->txi.ampdu = 1;
		lmac_uplink_request_sysbuf(buffer);
	} else {
		lmac_uplink_request_sysbuf(buffer);
	}
	LMAC_PS_SEM_GIVE(MODEM_SEMAPHORE_TASK);
#else
	nrc_mbx_send_data_address(buffer);
#endif

	/* Update Last TX time for Keep Alive */
	if (intf->associated)
		os_get_time(&intf->bss.last_tx_time);

	return 0;
}

#if defined(INCLUDE_TWT_SUPPORT)
static int send_twt_setup(int vif, int tid, uint8_t* addr)
{
	SYS_BUF* packet;
	uint16_t hif_len, mpdu_len, ac = TID_TO_AC(tid);
	struct nrc_wpa_if* intf = wpa_driver_get_interface(vif);

	struct nrc_wpa_sta* sta = nrc_wpa_find_sta(intf, addr);
	if (!sta)
		return CMD_RET_FAILURE;
	if (!intf->is_ap){
		memcpy(addr, system_modem_api_get_bssid(vif), MAC_ADDR_LEN);
	}

	/* disregarding qos/payload field in GenericMacHeader */
	hif_len = sizeof(FRAME_HDR) + sizeof(GenericMacHeader) - 2
				+ sizeof(TWT_Setup);
    packet = alloc_sys_buf_try(hif_len, NRC_WPA_BUFFER_ALLOC_TRY);

	if (!packet)
		return CMD_RET_FAILURE;

	HIF_HDR(packet).vifindex = vif;
	HIF_HDR(packet).len = hif_len;
	HIF_HDR(packet).tlv_len = 0;

	memset(&TX_MAC_HDR(packet), 0, sizeof(GenericMacHeader));
	TX_MAC_HDR(packet).version = FC_PV0;
	TX_MAC_HDR(packet).type = FC_PV0_TYPE_MGMT;
	TX_MAC_HDR(packet).subtype = FC_PV0_TYPE_MGMT_ACTION_NOACK;

	FRAME_HDR(packet).flags.tx.ac = get_ac(packet);

	memcpy(TX_MAC_HDR(packet).address1, addr, MAC_ADDR_LEN);
	memcpy(TX_MAC_HDR(packet).address3, addr, MAC_ADDR_LEN);
	memcpy(TX_MAC_HDR(packet).address2,
		system_modem_api_get_mac_address(vif), MAC_ADDR_LEN);

	memset( &TX_MAC_HDR(packet).payload , 0 , sizeof(TWT_Setup) );

	TWT_Setup *twt_setup = (TWT_Setup*) TX_MAC_HDR(packet).payload;

	twt_setup->category = WLAN_ACTION_S1G;
	twt_setup->action = WLAN_ACTION_S1G_TWT_SETUP;

	lmac_uplink_request_sysbuf(packet);

	return CMD_RET_SUCCESS;
}

static int send_twt_teardown(int vif, int tid, uint8_t* addr)
{
	SYS_BUF* packet;
	uint16_t hif_len, mpdu_len, ac = TID_TO_AC(tid);
	struct nrc_wpa_if* intf = wpa_driver_get_interface(vif);

	struct nrc_wpa_sta* sta = nrc_wpa_find_sta(intf, addr);
	if (!sta)
		return CMD_RET_FAILURE;
	if (!intf->is_ap){
		memcpy(addr, system_modem_api_get_bssid(vif), MAC_ADDR_LEN);
	}

	/* disregarding qos/payload field in GenericMacHeader */
	hif_len = sizeof(FRAME_HDR) + sizeof(GenericMacHeader) - 2
				+ sizeof(TWT_Teardown);
    packet = alloc_sys_buf_try(hif_len, NRC_WPA_BUFFER_ALLOC_TRY);

	if (!packet)
		return CMD_RET_FAILURE;

	HIF_HDR(packet).vifindex = vif;
	HIF_HDR(packet).len = hif_len;
	HIF_HDR(packet).tlv_len = 0;

	memset(&TX_MAC_HDR(packet), 0, sizeof(GenericMacHeader));
	TX_MAC_HDR(packet).version = FC_PV0;
	TX_MAC_HDR(packet).type = FC_PV0_TYPE_MGMT;
	TX_MAC_HDR(packet).subtype = FC_PV0_TYPE_MGMT_ACTION_NOACK;

	FRAME_HDR(packet).flags.tx.ac = get_ac(packet);

	memcpy(TX_MAC_HDR(packet).address1, addr, MAC_ADDR_LEN);
	memcpy(TX_MAC_HDR(packet).address3, addr, MAC_ADDR_LEN);
	memcpy(TX_MAC_HDR(packet).address2,
		system_modem_api_get_mac_address(vif), MAC_ADDR_LEN);

	memset( &TX_MAC_HDR(packet).payload , 0 , sizeof(TWT_Teardown) );

	TWT_Teardown *twt_teardown = (TWT_Teardown*) TX_MAC_HDR(packet).payload;

	twt_teardown->category = WLAN_ACTION_S1G;
	twt_teardown->action = WLAN_ACTION_S1G_TWT_TEARDOWN;

	lmac_uplink_request_sysbuf(packet);

	return CMD_RET_SUCCESS;
}

static int send_twt_info(int vif, int tid, uint8_t* addr)
{
	SYS_BUF* packet;
	uint16_t hif_len, mpdu_len, ac = TID_TO_AC(tid);
	struct nrc_wpa_if* intf = wpa_driver_get_interface(vif);

	struct nrc_wpa_sta* sta = nrc_wpa_find_sta(intf, addr);
	if (!sta)
		return CMD_RET_FAILURE;
	/*
	if (!intf->is_ap){
		memcpy(addr, system_modem_api_get_bssid(vif), MAC_ADDR_LEN);
	} */

	/* disregarding qos/payload field in GenericMacHeader */
	hif_len = sizeof(FRAME_HDR) + sizeof(GenericMacHeader) - 2
				+ sizeof(TWT_Info);
    packet = alloc_sys_buf_try(hif_len, NRC_WPA_BUFFER_ALLOC_TRY);

	if (!packet)
		return CMD_RET_FAILURE;

	HIF_HDR(packet).vifindex = vif;
	HIF_HDR(packet).len = hif_len;
	HIF_HDR(packet).tlv_len = 0;

	memset(&TX_MAC_HDR(packet), 0, sizeof(GenericMacHeader));
	TX_MAC_HDR(packet).version = FC_PV0;
	TX_MAC_HDR(packet).type = FC_PV0_TYPE_MGMT;
	TX_MAC_HDR(packet).subtype = FC_PV0_TYPE_MGMT_ACTION_NOACK;

	FRAME_HDR(packet).flags.tx.ac = get_ac(packet);

	memcpy(TX_MAC_HDR(packet).address1, addr, MAC_ADDR_LEN);
	memcpy(TX_MAC_HDR(packet).address3, addr, MAC_ADDR_LEN);
	memcpy(TX_MAC_HDR(packet).address2,
		system_modem_api_get_mac_address(vif), MAC_ADDR_LEN);

	memset( &TX_MAC_HDR(packet).payload , 0 , sizeof(TWT_Info) );

	TWT_Info *twt_info = (TWT_Info*) TX_MAC_HDR(packet).payload;

	twt_info->category = WLAN_ACTION_S1G;
	twt_info->action = WLAN_ACTION_S1G_TWT_INFO;

	lmac_uplink_request_sysbuf(packet);

	return CMD_RET_SUCCESS;
}

static int cmd_test_twt_setup(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = -1, vif = -1;
	uint16_t tid;
	uint8_t addr[MAC_ADDR_LEN] = {0,};

	if (argc < 3 || argc > 4)
		return CMD_RET_USAGE;

	tid = (uint16_t)strtoul(argv[2], NULL, 0);
	vif = atoi(argv[1]);

	if (vif == 1){
		A("Error: Cannot Start TWT setup on VIF 1\n");
		return CMD_RET_FAILURE;
	}

	if (tid != 0){ /* Temporary condition */
		A("Warning: Can Start TWT setup only on TID 0\n");
	}

	if (argc == 4){
		ret = util_cmd_parse_hwaddr(argv[3], addr);
		if (ret == -1)
			return CMD_RET_FAILURE;
	}

	return send_twt_setup(vif, tid, addr);
}

SUBCMD(test,
		twt_setup,
		cmd_test_twt_setup,
		"send TWT setup",
		"test twtsetup <vif_id> <tid> {sta-mac-addr}");

static int cmd_test_twt_teardown(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = -1, vif = -1;
	uint16_t tid;
	uint8_t addr[MAC_ADDR_LEN] = {0,};


	if (argc < 3 || argc > 4)
		return CMD_RET_USAGE;

	tid = (uint16_t)strtoul(argv[2], NULL, 0);
	vif = atoi(argv[1]);

	if (vif == 1){
		A("Error: Cannot Start TWT teardown on VIF 1\n");
		return CMD_RET_FAILURE;
	}

	if (tid != 0){ /* Temporary condition */
		A("Warning: Can Start TWT teardown only on TID 0\n");
	}

	if (argc == 4){
		ret = util_cmd_parse_hwaddr(argv[3], addr);
		if (ret == -1)
			return CMD_RET_FAILURE;
	}

	return send_twt_teardown(vif, tid, addr);
}

SUBCMD(test,
		twt_teardown,
		cmd_test_twt_teardown,
		"send TWT teardown",
		"test twtteardown <vif_id> <tid> {sta-mac-addr}");

static int cmd_test_twt_info(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = -1, vif = -1;
	uint16_t tid;
	uint8_t addr[MAC_ADDR_LEN] = {0,};


	if (argc < 3 || argc > 4)
		return CMD_RET_USAGE;

	tid = (uint16_t)strtoul(argv[2], NULL, 0);
	vif = atoi(argv[1]);

	if (vif == 1){
		A("Error: Cannot Start TWT info on VIF 1\n");
		return CMD_RET_FAILURE;
	}

	if (tid != 0){ /* Temporary condition */
		A("Warning: Can Start TWT info only on TID 0\n");
	}

	if (argc == 4){
		ret = util_cmd_parse_hwaddr(argv[3], addr);
		if (ret == -1)
			return CMD_RET_FAILURE;
	}

	return send_twt_info(vif, tid, addr);
}

SUBCMD(test,
		twt_info,
		cmd_test_twt_info,
		"send TWT info",
		"test twtinfo <vif_id> <tid> {sta-mac-addr}");
#endif /* defined(INCLUDE_TWT_SUPPORT) */

static int send_addba_req(int vif, int tid, uint8_t* addr)
{
	SYS_BUF* packet;
	uint16_t hif_len, mpdu_len, ac = TID_TO_AC(tid);
	struct nrc_wpa_if* intf = wpa_driver_get_interface(vif);

	struct nrc_wpa_sta* sta = nrc_wpa_find_sta(intf, addr);
	if (!sta || sta->block_ack[tid] == BLOCK_ACK_TX)
		return CMD_RET_FAILURE;
	if (!intf->is_ap){
		memcpy(addr, system_modem_api_get_bssid(vif), MAC_ADDR_LEN);
	}

	/* disregarding qos/payload field in GenericMacHeader */
	hif_len = sizeof(FRAME_HDR) + sizeof(GenericMacHeader) - 2
				+ sizeof(ADDBA_Req);
    packet = alloc_sys_buf_try(hif_len, NRC_WPA_BUFFER_ALLOC_TRY);

	if (!packet)
		return CMD_RET_FAILURE;

	HIF_HDR(packet).vifindex				= vif;
	HIF_HDR(packet).len						= hif_len;
	HIF_HDR(packet).tlv_len					= 0;

	memset(&TX_MAC_HDR(packet), 0, sizeof(GenericMacHeader));
	TX_MAC_HDR(packet).version 				= FC_PV0;
	TX_MAC_HDR(packet).type					= FC_PV0_TYPE_MGMT;
	TX_MAC_HDR(packet).subtype 				= FC_PV0_TYPE_MGMT_ACTION;

	FRAME_HDR(packet).flags.tx.ac			= get_ac(packet);

	memcpy(TX_MAC_HDR(packet).address1, addr, MAC_ADDR_LEN);
	memcpy(TX_MAC_HDR(packet).address3, addr, MAC_ADDR_LEN);
	memcpy(TX_MAC_HDR(packet).address2, system_modem_api_get_mac_address(vif),
				MAC_ADDR_LEN);

	memset( &TX_MAC_HDR(packet).payload , 0 , sizeof(ADDBA_Req) );

	ADDBA_Req *addba = (ADDBA_Req*) TX_MAC_HDR(packet).payload;

	addba->category 	= WLAN_ACTION_BLOCK_ACK;
	addba->ba_action 	= WLAN_BA_ADDBA_REQUEST;
	addba->max_buf		= WLAN_BA_MAX_BUF;
	addba->policy 		= WLAN_BA_POLICY_IMM_BA;
	addba->tid 			= tid;

	lmac_uplink_request_sysbuf(packet);

	return CMD_RET_SUCCESS;
}

static int send_delba_req(int vif, int tid, uint8_t* addr)
{
	SYS_BUF* packet;
	uint16_t hif_len, mpdu_len, ac = TID_TO_AC(tid);
	struct nrc_wpa_if* intf = wpa_driver_get_interface(vif);

	struct nrc_wpa_sta* sta = nrc_wpa_find_sta(intf, addr);
	if (!sta || sta->block_ack[tid] != BLOCK_ACK_TX)
		return CMD_RET_FAILURE;
	if (!intf->is_ap){
		memcpy(addr, system_modem_api_get_bssid(vif), MAC_ADDR_LEN);
	}

	/* disregarding qos/payload field in GenericMacHeader */
	hif_len = sizeof(FRAME_HDR) + sizeof(GenericMacHeader) - 2 +
				sizeof(DELBA_Req);
    packet = alloc_sys_buf_try(hif_len, NRC_WPA_BUFFER_ALLOC_TRY);

	if(!packet)
		return CMD_RET_FAILURE;

	HIF_HDR(packet).vifindex				= vif;
	HIF_HDR(packet).len						= hif_len;
	HIF_HDR(packet).tlv_len					= 0;

	FRAME_HDR(packet).flags.tx.ac			= ACI_VO;

	memset(&TX_MAC_HDR(packet), 0, sizeof(GenericMacHeader));

	TX_MAC_HDR(packet).version 				= FC_PV0;
	TX_MAC_HDR(packet).type					= FC_PV0_TYPE_MGMT;
	TX_MAC_HDR(packet).subtype 				= FC_PV0_TYPE_MGMT_ACTION;

	memcpy(TX_MAC_HDR(packet).address1, addr, MAC_ADDR_LEN);
	memcpy(TX_MAC_HDR(packet).address3, addr, MAC_ADDR_LEN);
	memcpy(TX_MAC_HDR(packet).address2, system_modem_api_get_mac_address(vif),
				 MAC_ADDR_LEN);

	memset(&TX_MAC_HDR(packet).payload, 0, sizeof(DELBA_Req));

	DELBA_Req *delba = (DELBA_Req*) TX_MAC_HDR(packet).payload;
	delba->category = WLAN_ACTION_BLOCK_ACK;
	delba->ba_action = WLAN_BA_DELBA;
	delba->init = true;
	delba->tid = tid;
	delba->reason = WLAN_REASON_END_BA;

	lmac_uplink_request_sysbuf(packet);

	sta->block_ack[tid] = BLOCK_ACK_INVALID;
	wpa_printf(MSG_INFO,"BA_TX_STOP:"MACSTR",TID:%d,AC:%d",MAC2STR(addr),tid,ac);

	return CMD_RET_SUCCESS;
}

static int cmd_test_addba(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = -1, vif = -1;
	uint16_t tid;
	uint8_t addr[MAC_ADDR_LEN] = {0,};

	if (argc < 3 || argc > 4)
		return CMD_RET_USAGE;

	tid = (uint16_t)strtoul(argv[2], NULL, 0);
	vif = atoi(argv[1]);

	if (vif == 1){
		A("Error: Cannot Start Block Ack on VIF 1\n");
		return CMD_RET_FAILURE;
	}
	if (tid != 0){ /* Temporary condition */
		A("Warning: Can Start Block Ack only on TID 0\n");
	}

	if (argc == 4){
		ret = util_cmd_parse_hwaddr(argv[3], addr);
		if (ret == -1)
			return CMD_RET_FAILURE;
	}

	return send_addba_req(vif, tid, addr);
}

SUBCMD(test,
		addba,
		cmd_test_addba,
		"send ADDBA request",
		"test addba <vif_id> <tid> {sta-mac-addr}");

static int cmd_test_delba(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = -1, vif = -1;
	uint16_t tid;
	uint8_t addr[MAC_ADDR_LEN] = {0,};


	if (argc < 3 || argc > 4)
		return CMD_RET_USAGE;

	tid = (uint16_t)strtoul(argv[2], NULL, 0);
	vif = atoi(argv[1]);

	if (vif == 1){
		A("Error: Cannot Start Block Ack on VIF 1\n");
		return CMD_RET_FAILURE;
	}
	if (tid != 0){ /* Temporary condition */
		A("Warning: Can Start Block Ack only on TID 0\n");
	}

	if (argc == 4){
		ret = util_cmd_parse_hwaddr(argv[3], addr);
		if (ret == -1)
			return CMD_RET_FAILURE;
	}

	return send_delba_req(vif, tid, addr);
}

SUBCMD(test,
		delba,
		cmd_test_delba,
		"send DELBA request",
		"test delba <vif_id> <tid> {sta-mac-addr}");

static int cmd_show_agg(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = -1, vif = -1;
	int i, tid;
	bool flg = false;

	if (argc != 2)
		return CMD_RET_USAGE;

	vif = atoi(argv[1]);

	A("[Block Ack Session Status (vif %d)]\n", vif);
	struct nrc_wpa_if* intf = wpa_driver_get_interface(vif);

	if (intf->is_ap){
		for (i = 0; i < NRC_WPA_SOFTAP_MAX_STA; i++) {
			if (!intf->ap_sta[i])
				continue;

			struct nrc_wpa_sta* sta = intf->ap_sta[i];
			A("-STA "MACSTR" (aid %d)\n", MAC2STR(sta->addr), sta->aid);
			for (tid = 0; tid < MAX_TID; tid++) {
				if (sta->block_ack[tid] == BLOCK_ACK_INVALID)
					continue;
				flg = true;
				A(" -TID %d %s\n", tid, sta->block_ack[tid] == BLOCK_ACK_TX ?
											"BLOCK_ACK_TX" : "BLOCK_ACK_RX");
			}
		}
	} else {
		A("-AP "MACSTR" (ssid %s)\n", MAC2STR(intf->bss.bssid), intf->bss.ssid);
		for (tid = 0; tid < MAX_TID; tid++) {
			if (intf->sta.block_ack[tid] == BLOCK_ACK_INVALID)
				continue;
			flg = true;
			A(" -TID %d %s\n", tid, intf->sta.block_ack[tid] == BLOCK_ACK_TX ?
											"BLOCK_ACK_TX" : "BLOCK_ACK_RX");
		}
	}

	if (!flg){
		A("No Valid Block Ack Session\n");
	}

	return CMD_RET_SUCCESS;
}

SUBCMD(show,
		agg,
		cmd_show_agg,
		"show aggregation policy",
		"show agg <vif_id>");

#if defined(INCLUDE_TWT_SUPPORT)
int atcmd_send_twt_setup(int vif, int tid, uint8_t* addr)
{
	return send_twt_setup(vif, tid, addr);
}

int atcmd_send_twt_teardown(int vif, int tid, uint8_t* addr)
{
	return send_twt_teardown(vif, tid, addr);
}

int atcmd_send_twt_info(int vif, int tid, uint8_t* addr)
{
	return send_twt_info(vif, tid, addr);
}
#endif /* defined(INCLUDE_TWT_SUPPORT) */
int atcmd_send_addba_req(int vif, int tid, uint8_t* addr)
{
	return send_addba_req(vif, tid, addr);
}

int atcmd_send_delba_req(int vif, int tid, uint8_t* addr)
{
	return send_delba_req(vif, tid, addr);
}

