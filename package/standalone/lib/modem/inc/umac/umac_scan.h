#ifndef UMAC_SCAN_H
#define UMAC_SCAN_H

#include "nrc-wim-types.h"

#define UMAC_SCAN_CONFIG_MAX_PROBEREQ_LEN    512
#define UMAC_SCAN_CONFIG_INTERVAL           (100) // 100 ms

typedef struct _scan_ssid {
	uint8_t ssid[IEEE80211_MAX_SSID_LEN];
	uint8_t ssid_len;
} scan_ssid_t;

typedef struct _scan_info {
	uint8_t			vif_id;
	struct _SYS_BUF*	probereq;
	uint16_t		probereq_len;
	void			(*scan_done_cb)(int);
	uint8_t			n_bssids;

	uint8_t			n_channels;
	int8_t			curr_channel_index;
	uint16_t		scan_channel[WIM_MAX_SCAN_CHANNEL];

	uint8_t			n_ssids;
	int8_t			curr_ssid_index;
	scan_ssid_t		scan_ssid[WIM_MAX_SCAN_SSID];

	uint8_t			bssid[6];
	uint16_t		return_channel;
	uint8_t			*pos_dsparam_ie;
	uint32_t		flags;
	uint8_t			*ies;
	uint8_t			ies_len;
	bool			bg_scan;
} scan_info;

void	umac_scan_init( void (*cb)(int) );
void	umac_scan_deinit(void);
bool	umac_scan_set_param(int vif_id, struct wim_scan_param* param);
void	umac_scan_set_probereq_ie(uint16_t len , uint8_t* ie);
uint8_t* umac_scan_get_probereq_ie_buffer(uint16_t len);
void	umac_scan_start();
void	umac_scan_stop();
int32_t umac_scan_done();
void	umac_scan_set_return_channel(uint16_t channel);
void	umac_scan_set_channel(uint16_t channel);
bool	umac_scan_check_bg_supported(void);
void	umac_scan_probe();
bool	umac_scan_check_ch();
#if defined(INCLUDE_SCAN_MODE)
void	umac_scan_change_period(uint32_t multiply);
#endif
#define UMAC_SCAN_MAC_HDR_LEN   24
#define UMAC_SCAN_EID_SSID      0

/* umac scan ssid */
void umac_scan_ssid_init (void);
void umac_scan_ssid_cur_info (uint8_t **ssid, uint8_t *ssid_len);
bool umac_scan_ssid_has_next (void);
void umac_scan_ssid_go_next (void);

#endif /* UMAC_SCAN_H */
