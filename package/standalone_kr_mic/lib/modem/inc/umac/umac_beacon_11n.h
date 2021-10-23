#ifndef UMAC_BEACON_11N_H
#define UMAC_BEACON_11N_H

typedef struct {
	/*  Word 0 : MAC Header Word 0 */
	/* 16 bit Frame Control */
	uint16_t    version        : 2;	/*  default 0 */
	uint16_t    type           : 2;	/* 0: Management , 1: Control,  2: Data,  3: reserved */
	uint16_t    subtype        : 4;
	uint16_t    to_ds          : 1;	/*   to AP */
	uint16_t    from_ds        : 1;	/*   from AP */
	uint16_t    more_frag      : 1;
	uint16_t    retry          : 1;
	uint16_t    pwr_mgt        : 1;
	uint16_t    more_data      : 1;
	uint16_t    protect        : 1;
	uint16_t    order          : 1;

	/* 16 bit Duration */
	uint16_t	duration_id;

	/* Word 1 ~ Word 5: MAC Header Word 1 ~ MAC Header Word 5(2byte) */
	uint8_t     address1[6];
	uint8_t     address2[6];
	uint8_t     address3[6];

	/* Word 5 : MAC Header Word 5(2byte) */
	uint16_t    fragment_number    : 4;
	uint16_t    sequence_number    : 12;

	/* Word 6 ~ Word 7 */
	uint64_t    timestamp;

	/* Word 8 */
	uint16_t    beacon_interval;
   	uint16_t    capa_info;

	/* SSID IE Word 9 */
	uint8_t 	element[0];

} legacy_beacon_format;

void umac_beacon_init(int8_t vif_id);
void umac_beacon_start(int8_t vif_id);
void umac_beacon_stop(uint8_t vif_id);

void umac_beacon_update(int8_t vif_id, uint8_t* b, uint16_t len);
void umac_beacon_update_tim(uint8_t vif_id, uint16_t aid, bool flag);
void umac_beacon_update_dtim_period(int8_t vif_id, uint8_t period );
void umac_beacon_set_beacon_interval(int8_t vif_id, uint16_t interval);
void umac_beacon_set_ssid(int8_t vif_id, uint8_t *ssid , uint8_t ssid_len);
void umac_beacon_tbtt_cb(int8_t vif_id);

#endif /* UMAC_BEACON_11N_H */
