/*
 * Copyright (c) 2016-2021 Newracom, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DRIVER_FREERTOS_SCAN_H
#define DRIVER_FREERTOS_SCAN_H

#include "common/defs.h"
#include "utils/list.h"
#include "drivers/driver.h"

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
int scan_add(struct nrc_scan_info *scan, uint16_t freq, int8_t rssi,
		uint8_t* frame, uint16_t len);
void scan_config(struct nrc_scan_info *scan, struct wpa_driver_scan_params *p,
		uint16_t last_freq);
struct wpa_scan_results* get_scan_results(struct nrc_scan_info *scan);
void scan_flush(struct nrc_scan_info *scan);
void scan_deinit(struct nrc_scan_info *scan);

void nrc_scan_test();

#endif // DRIVER_FREERTOS_SCAN_H
