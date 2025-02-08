#ifndef __UMAC_WIM_MANAGER_H__
#define __UMAC_WIM_MANAGER_H__

/**
 * enum nl80211_chan_width - channel width definitions
 *
 * These values are used with the %NL80211_ATTR_CHANNEL_WIDTH
 * attribute.
 *
 * @NL80211_CHAN_WIDTH_20_NOHT: 20 MHz, non-HT channel
 * @NL80211_CHAN_WIDTH_20: 20 MHz HT channel
 * @NL80211_CHAN_WIDTH_40: 40 MHz channel, the %NL80211_ATTR_CENTER_FREQ1
 *	attribute must be provided as well
 * @NL80211_CHAN_WIDTH_80: 80 MHz channel, the %NL80211_ATTR_CENTER_FREQ1
 *	attribute must be provided as well
 * @NL80211_CHAN_WIDTH_80P80: 80+80 MHz channel, the %NL80211_ATTR_CENTER_FREQ1
 *	and %NL80211_ATTR_CENTER_FREQ2 attributes must be provided as well
 * @NL80211_CHAN_WIDTH_160: 160 MHz channel, the %NL80211_ATTR_CENTER_FREQ1
 *	attribute must be provided as well
 * @NL80211_CHAN_WIDTH_5: 5 MHz OFDM channel
 * @NL80211_CHAN_WIDTH_10: 10 MHz OFDM channel
 */
enum nl80211_chan_width {
	NL80211_CHAN_WIDTH_20_NOHT,
	NL80211_CHAN_WIDTH_20,
	NL80211_CHAN_WIDTH_40,
	NL80211_CHAN_WIDTH_80,
	NL80211_CHAN_WIDTH_80P80,
	NL80211_CHAN_WIDTH_160,
	NL80211_CHAN_WIDTH_5,
	NL80211_CHAN_WIDTH_10,
	NL80211_CHAN_WIDTH_1,
	NL80211_CHAN_WIDTH_2,
	NL80211_CHAN_WIDTH_4,
	NL80211_CHAN_WIDTH_8,
	NL80211_CHAN_WIDTH_16,
};

struct wim_manager_external_interface {
	void (*tx_enable)(bool enable);
	void (*send_to)(SYS_BUF *packet);
} __packed;

typedef SYS_BUF* (*wim_handler)(int vif_id, struct nrc_tlv**);
const wim_handler *umac_wim_manager_init();
void umac_wim_manager_run(int vif_id, uint16_t cmd, struct nrc_tlv **map);
void umac_wim_manager_run2(void *param);
bool umac_wim_manager_run_by_work_queue(void *param);
void umac_wim_manager_register_external_interface(struct wim_manager_external_interface *ext_if);
void umac_wim_manager_restart();
void umac_wim_manager_check_ps_abnormal();
void umac_wim_manager_inform_CSA();
void umac_wim_manager_inform_chan_switch();
void umac_wim_manager_inform_lbt_enable(bool enable);
#if defined(INCLUDE_MANAGE_BLACKLIST)
void umac_wim_manager_clean_txq_sta(uint8_t *mac_addr, uint8_t vif_id);
#endif
uint8_t *ssid_string(uint8_t *ssid, uint8_t ssidlen);

#endif //__UMAC_WIM_MANAGER_H__
