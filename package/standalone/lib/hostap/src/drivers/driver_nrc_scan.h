/*
 * Driver interface definition
 * Copyright (c) 2003-2015, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 *
 * This file defines a driver interface used by both %wpa_supplicant and
 * hostapd. The first part of the file defines data structures used in various
 * driver operations. This is followed by the struct wpa_driver_ops that each
 * driver wrapper will beed to define with callback functions for requesting
 * driver operations. After this, there are definitions for driver event
 * reporting with wpa_supplicant_event() and some convenience helper functions
 * that can be used to report events.
 */

#ifndef DRIVER_FREERTOS_SCAN_H
#define DRIVER_FREERTOS_SCAN_H

#include "common/defs.h"
#include "utils/list.h"

#include <stdbool.h>

#define DRIVER_SCAN_DEBUG_DUMP_ENTRY
#define DRIVER_SCAN_DEBUG_TEST_SUITE
#define MAX_SCAN_SSID_LIST			(16)
#define MAX_SCAN_HISTORY			(8)

struct nrc_wpa_scan_res {
	struct dl_list list;
	struct wpa_scan_res *res;
};

struct last_scan {
	uint8_t bssid[6];
	uint32_t time;
};

struct nrc_scan_info {
	bool		is_run;
	struct		dl_list scan_list;
	struct		wpa_driver_scan_params params;
	int			history_cnt;
	struct		last_scan history[MAX_SCAN_HISTORY];
	uint16_t	curr_freq;
	uint16_t	last_freq;
};

struct nrc_scan_info * scan_init();
void scan_start(struct nrc_scan_info *scan);
void scan_stop(struct nrc_scan_info *scan);
void scan_cancel(struct nrc_scan_info *scan);
uint16_t scan_resume_from(struct nrc_scan_info *scan);
int scan_add(struct nrc_scan_info *scan, uint16_t freq, uint16_t rssi,
		uint8_t* frame, uint16_t len);
void scan_config(struct nrc_scan_info *scan, struct wpa_driver_scan_params *p,
		uint16_t last_freq);
struct wpa_scan_results* get_scan_results(struct nrc_scan_info *scan);
void scan_flush(struct nrc_scan_info *scan);
void scan_deinit(struct nrc_scan_info *scan);

void nrc_scan_test();

#endif // DRIVER_FREERTOS_SCAN_H
