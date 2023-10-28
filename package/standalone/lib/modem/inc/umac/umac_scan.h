#ifndef UMAC_SCAN_H
#define UMAC_SCAN_H

#include "nrc-wim-types.h"

#define UMAC_SCAN_CONFIG_MAX_PROBEREQ_LEN    512
#define UMAC_SCAN_CONFIG_INTERVAL           (100) // 100 ms

typedef struct _scan_info {
	uint8_t			vif_id;
	struct _SYS_BUF*	probereq;
	uint16_t		probereq_len;
	void			(*scan_done_cb)(int);
	uint8_t			n_channels;
	uint8_t			n_bssids;
	uint16_t		scan_channel[WIM_MAX_SCAN_CHANNEL];
	uint8_t			bssid[6];
	int16_t			curr_channel_index;
	uint16_t		return_channel;
	uint8_t			*pos_dsparam_ie;
	uint32_t		flags;
	bool			passive_scan_flag;
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
int32_t	umac_scan_done(void*);
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
#endif /* UMAC_SCAN_H */
