/*
 * WPA Supplicant / Configuration backend: empty starting point
 * Copyright (c) 2003-2005, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 *
 * This file implements dummy example of a configuration backend. None of the
 * functions are actually implemented so this can be used as a simple
 * compilation test or a starting point for a new configuration backend.
 */

#include "includes.h"
#include "system.h"

#include "common.h"
#include "config.h"
#include "base64.h"

#define USE_WLAN0_AUTO_CONFIG 0 //0: use (WPA)CLI, 1:use config

#if 0 //Sample Config for  reference
//Open
const char* __attribute__((weak)) wlan0_conf =  "\n\
{\n\
ssid=\"halow_demo\"\n \
key_mgmt=NONE\n \
}";

//WPA2-PSK
const char* __attribute__((weak)) wlan0_conf =  "\n\
{\n\
ssid=\"halow_demo\"\n \
proto=RSN\n \
ieee80211w=0\n \
key_mgmt=WPA-PSK\n \
psk=\"12345678\"\n \
}";

//WPA3-SAE
const char* __attribute__((weak)) wlan0_conf =  "\n\
{\n\
ssid=\"halow_demo\"\n \
proto=RSN\n \
ieee80211w=2\n \
key_mgmt=SAE\n \
sae_password=\"12345678\"\n \
}";

//WPA3-OWE
const char* __attribute__((weak)) wlan0_conf =  "\n\
{\n\
ssid=\"halow_demo\"\n \
proto=RSN\n \
ieee80211w=2\n \
key_mgmt=OWE\n \
}";
#endif

#if USE_WLAN0_AUTO_CONFIG
//WPA2
const char* __attribute__((weak)) wlan0_conf =  "\n\
{\n\
ssid=\"halow_ap_sae\"\n \
proto=RSN\n \
ieee80211w=2\n \
key_mgmt=SAE\n \
sae_password=\"12345678\"\n \
}";
#else
const char* __attribute__((weak)) wlan0_conf = NULL;
#endif
const char* __attribute__((weak)) wlan1_conf = NULL;

#if USE_WLAN0_AUTO_CONFIG
const char* __attribute__((weak)) global_conf = "\n\
COUNTRY=US\n \
NDP_PREQ=1";
#else
const char* __attribute__((weak)) global_conf = NULL;
#endif

extern const char* wlan0_conf;
extern const char* wlan1_conf;
extern const char* global_conf;

static char *os_strnchr(const char *s, const char ch, int *count, const int bound)
{
	*count = 0;
	for ( ; *s != ch; s++) {
		if ((*count)++ >= bound)
			return NULL;

		if (*s == '\0')
			return NULL;
	}
	return (char *)s;
}

int find_text(char *start, char *end, char *copy_to)
{
	int len = end - start;

	if (!start || !end || len < 0 || len > 1024) {
		wpa_printf(MSG_DEBUG, "Failed to find text. (st:%p, end:%p, len:%d)",
			start, end, len);
		return -1;
	}

	memcpy(copy_to, start, len);
	copy_to[len] = 0;
	return 0;
}

static struct wpa_ssid * wpa_config_read_network(char* str_conf, int id,
											const int start, const int end)
{
	char *pos = str_conf + start, *delimiter;
	int err = 0, count = 0;
	struct wpa_ssid *ssid = os_malloc(sizeof(*ssid));

	if (!ssid) {
		wpa_printf(MSG_DEBUG, "%s: Failed to allocate ssid.", __func__);
		return NULL;
	}
	memset( ssid , 0 , sizeof(*ssid) );

	dl_list_init(&ssid->psk_list);
	ssid->id = id;
	wpa_config_set_network_defaults(ssid);

	while (pos++ < str_conf + end - 1) {
		char name[255], value[1024];

		if(*pos == '\n' || *pos == '\t' || *pos == ' ' || *pos == '{')
			continue;

		delimiter = os_strnchr(pos, '=', &count, end - start);
		if (find_text(pos, delimiter, name) != 0) {
			wpa_printf(MSG_DEBUG, "Failed find text");
			err++;
			break;
		}

		pos = delimiter + 1;
		delimiter = os_strnchr(pos, '\n', &count, end - start);

		if (find_text(pos, delimiter, value) != 0) {
			wpa_printf(MSG_DEBUG, "Failed find text");
			err++;
			break;
		}
		pos = delimiter + 1;
		if (wpa_config_set(ssid, name, value, 0) != 0) {
			wpa_printf(MSG_DEBUG, "Failed to set config. (ssid:%p, name: %s,value: %s)",
				ssid, name, value);
			err++;
		}
	}

	if (ssid->passphrase) {
		if (ssid->psk_set) {
			wpa_printf(MSG_ERROR, "Both PSK and passphrase set at the same time");
			err++;
		}
		wpa_config_update_psk(ssid);
	}

	if ((ssid->group_cipher & WPA_CIPHER_CCMP) &&
		!(ssid->pairwise_cipher & WPA_CIPHER_CCMP) &&
		!(ssid->pairwise_cipher & WPA_CIPHER_NONE)) {
		wpa_printf(MSG_DEBUG, "Wrong cipher set.");
		ssid->group_cipher &= ~WPA_CIPHER_CCMP;
	}

	if (err > 0) {
		wpa_config_free_ssid(ssid);
		ssid = NULL;
	}

	return ssid;
}

static char * find_next_network_block(const char* s, int offset, int len,
			int *start, int *end)
{
	const char ch_begin             = '{';
	const char ch_end               = '}';
	char*   		res = NULL;
	int             count = 0;


	if (offset >= len)
		return NULL;

	res = os_strnchr(s + offset, ch_begin, &count, len - offset);

	if (res) {
		*start = offset + count;
		res = os_strnchr(s + *start, ch_end, &count, len - *start);
		*end = *start + count;
	}

	return res;
}

static void wpa_config_read_global(struct wpa_config *config, const char *s)
{
	const char *t;
	char* p = (char *) s;
	int count = 0;

	if (!s)
		return;

	t = s + os_strlen(s);

	while(*p != 0) {
		*p = (char) toupper((int) *p);
		p++;
	}
	p = (char *) s;

	while (os_strnchr(p, '\n', &count, t - p) != NULL || p < t) {

		while ((*p == ' ' || *p == '\t') && count) {
			p++; count--;
		};

		if (os_strncmp("COUNTRY=", p, 8) == 0) {
			char* v = p + 8;
			config->country[0] = *v;
			config->country[1] = *(v+1);
			config->changed_parameters |= CFG_CHANGED_COUNTRY;

			wpa_printf(MSG_DEBUG, "country = '%c%c'",
				config->country[0], config->country[1]);
		}

#ifdef CONFIG_NDP_PREQ
		if (os_strncmp("NDP_PREQ=", p, 9) == 0) {
			int v = atoi ((p + 9));
			if (v) config->fst_llt = v;
			wpa_printf(MSG_DEBUG, "NDP_PREQ = '%d'", config->fst_llt);
		}
#endif

		p += (count + 1);
	}
}

static int wpa_config_read_networks(struct wpa_config *config, const char* str_conf)
{
	struct wpa_ssid *ssid = NULL, *tail = NULL, *head = NULL;
	int offset = 0, start = 0, end = 0;
	int failed_try = 5;
	int id = 0;

	if (!config || !str_conf)
		return 0;
	while (find_next_network_block(str_conf, offset, os_strlen(str_conf), &start, &end)
				&& failed_try) {

		offset = end + 1;
		ssid = wpa_config_read_network((char *) str_conf, id++, start, end);

		if (!ssid) {
			wpa_printf(MSG_ERROR, "Failed to read config.");
			// TODO: Neeed to clean up stored configuration?
			failed_try--;
			continue;
		}

		if (head == NULL) {
			head = tail = ssid;
		} else {
			tail->next = ssid;
			tail = ssid;
		}

		if (wpa_config_add_prio_network(config, ssid) != 0) {
			wpa_printf(MSG_ERROR, "Failed to add new network into to priority lists.");
			failed_try--;
			continue;
		}
	}
	config->ssid = head;
	tail->next = NULL;


	return (failed_try == 0) ? -1 : 0;
}

struct wpa_config * wpa_config_read(const char *name, struct wpa_config *config)
{
	int i, res = -1;
	wpa_printf(MSG_DEBUG, "Reading config file '%s'", name ? name : "NULL");
	if (!name)
		return NULL;

	if (!config)
		config = wpa_config_alloc_empty(NULL, NULL);

	#if defined(STANDARD_11AH)
	/* IEEE 802.11 draft; 11-18-1177-02-000m */
	for (i = 0; i < 4; i++)
		config->wmm_ac_params[i].txop_limit = 469;
	#endif

	wpa_config_read_global(config, global_conf);

	if (os_strcmp("wlan0", name) == 0)
		res = wpa_config_read_networks(config, wlan0_conf);
	else if (os_strcmp("wlan1", name) == 0)
		res = wpa_config_read_networks(config, wlan1_conf);
	else {
		wpa_printf(MSG_DEBUG, "Failed to read config file '%s'", name);
		wpa_config_free(config);
		return NULL;
	}
	wpa_config_debug_dump_networks(config);
	return config;
}

int wpa_config_write(const char *name, struct wpa_config *config)
{
	struct wpa_ssid *ssid;
	struct wpa_config_blob *blob;

	wpa_printf(MSG_DEBUG, "Writing configuration file '%s'", name);

	/* TODO: write global config parameters */

	for (ssid = config->ssid; ssid; ssid = ssid->next) {
		/* TODO: write networks */
	}

	for (blob = config->blobs; blob; blob = blob->next) {
		/* TODO: write blobs */
	}

	return 0;
}
