#ifndef __DEBUG_H
#define __DEBUG_H

#include "util_trace.h"

#if DEBUG
	#define Debug(__info,...) A("Debug: " __info,##__VA_ARGS__)
#else
	#define Debug(__info,...)
#endif

#endif

