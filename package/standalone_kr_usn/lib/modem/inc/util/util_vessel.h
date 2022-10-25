#ifndef __UTIL_VESSEL_H__
#define __UTIL_VESSEL_H__

#include "system_common.h"
#include "util_list.h"

struct vessel {
	struct dl_list list;
	void *info;
};

void util_vessel_init();
void util_vessel_deinit();
struct vessel* util_vessel_alloc();
void util_vessel_free();

#endif /*__UTIL_VESSEL_H__*/
