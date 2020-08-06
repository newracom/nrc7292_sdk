#ifndef UMAC_IE_CONFIG_H
#define UMAC_IE_CONFIG_H

#include "system.h"
#include "../util/util_byte_stream.h"
#include "umac_ieee80211_types.h"

#define WLAN_EXT_TAG_OWE_DH		32
#define OWE_DH_PUBLIC_KEY_LEN	32
#define MAX_EXTENSION_IE		8
#define NON_IE					0xFFFF
#define AUTH_GROUP_ID_LEN		sizeof(uint8_t) * 2
#define AUTH_SCALAR_LEN			sizeof(uint8_t) * 32
#define AUTH_FF_ELEMENT_LEN		sizeof(uint8_t) * 64
#define RFC1042_HDR_LEN			sizeof(uint8_t) * 6

typedef struct {
	uint16_t group_id;
	uint8_t public_key[OWE_DH_PUBLIC_KEY_LEN];
} ie_owe_dh;

typedef struct {
	uint8_t eid;
	uint8_t length;
	uint8_t ext_eid;
	uint8_t info[INFO_ELEMENT_MAX_LENGTH];
} ie_extension;

typedef struct {
	uint16_t auth_alg;
	uint16_t auth_seq;
	uint16_t status_code;
	uint16_t group_id;
	uint8_t scalar[AUTH_SCALAR_LEN];
	uint8_t ff_element[AUTH_FF_ELEMENT_LEN];
} sae_auth;

#define IEEE8021X_REPLAY_COUNTER_LEN	8
#define IEEE8021X_KEY_SIGN_LEN			16
#define IEEE8021X_KEY_IV_LEN			16

struct ieee802_1x_auth {
	uint8_t version;
	uint8_t type;
	uint8_t length[2];
	uint8_t key_desc;
	uint8_t key_info[2];
	uint8_t key_length[2];
	uint8_t replay_counter[IEEE8021X_REPLAY_COUNTER_LEN];
	uint8_t wpa_key_nonce[32];
	uint8_t key_iv[IEEE8021X_KEY_IV_LEN];
	uint8_t wpa_key_rsc[8];
	uint8_t wpa_key_id[8];
	uint8_t wpa_key_mic[16];
	uint8_t data_length[2];
	uint8_t key_data[0];
};

void umac_ie_config_handler(uint16_t eid_, uint8_t length, uint8_t *data);
bool umac_ie_config_replace(struct byte_stream *bs, bool is_tx, int8_t vif_id, ie_general *ie);
void umac_ie_config_replace_auth(GenericMacHeader *gmh);
void umac_ie_config_sta_auth_frame(GenericMacHeader *gmh);
void umac_ie_config_legacy_beacon(uint8_t *buf, uint8_t length);

#endif