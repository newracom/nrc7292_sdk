#ifndef __UTIL_TRACE_H__
#define __UTIL_TRACE_H__

#include "system.h"
#include "hal_uart.h"
#include "system_common.h"

/*=============================================================================
	Trace level and type enumerations
=============================================================================*/
typedef enum {
	TL_VB		= 0,
	TL_INFO		= 1,
	TL_ERR		= 2,
	TL_MAX,
}TRACE_LEVEL;

typedef enum {
	TT_QM = 0,
	TT_HIF,
	TT_WIM,
	TT_API,
	TT_MSG,
	TT_RX,
	TT_TX,
	TT_DL,
	TT_UL,
	TT_PHY,
	TT_RF = 10,
	TT_UMAC,
	TT_PS,
	TT_TWT,
	TT_HALOW,
	TT_WPAS,
	TT_RC,
	TT_NET,
	TT_CMD,
	TT_MM,
	TT_BA = 20,
	TT_FRAG,
	TT_RCV,
	TT_BMT,
	TT_TEMP_SENSOR,
	TT_LED = 25,
	TT_WPA,
	TT_SYS,
	TT_STAINFO,
	TT_SBR = 29,
	/* insert type for MAC below (Max:29)*/
	TT_SDK_GPIO = 30,
	TT_RESERVED1,
	TT_SDK_HTTPD,
	TT_SDK_FOTA,
	TT_SDK_PS,
	TT_SDK_I2C,
	TT_SDK_UART,
	TT_SDK_ADC,
	TT_SDK_PWM,
	TT_SDK_SPI,
	TT_SDK_TIMER = 40,
	TT_SDK_WIFI,
	TT_SDK_WLAN_MANAGER = 42,
	/* insert type for SDK here */
	TT_MAX,             /* must be last */
}TRACE_TYPES;

/*=============================================================================
	Trace level and Map structure
=============================================================================*/
struct TraceLevel {
	uint8_t    value;
	const char *string;
	const char *alias1;
	const char *alias2;
} __packed;


struct TraceMap {
	const char *string;
	uint8_t     level;
} __packed;

extern struct TraceMap g_trace_map[TT_MAX];
void show_log_level();
uint8_t util_trace_set_log_level(int type_id, uint32_t level);
uint8_t util_trace_get_log_level(int type_id);
#if defined(INCLUDE_MEASURE_AIRTIME)
void util_trace_set_log_level_all(uint32_t level);
#endif /* defined(INCLUDE_MEASURE_AIRTIME) */

extern int oledInit(uint8_t sda, uint8_t scl, int iChannel, int iAddr, int iType, int bFlip, int bInvert);
extern void oledPrintf(int x, int y,  int Fontsize, char *format, ...);
extern int oledFill(unsigned char ucData);
extern void oledWriteFlashBlock(uint8_t *s, int iLen);

#define xPRINT(x, format, ...)		system_printf(format, ##__VA_ARGS__)

//#if !defined(RELEASE) && !defined(UCODE)
#if !defined(UCODE)

//CLI PRINT
#if !defined(BOOT_LOADER)
#define CPA(format, ...)	\
do {						\
	system_printf(format, ##__VA_ARGS__);\
} while (0)
#else
#define CPA(format, ...) do{}while (0)
#endif

#if defined(INCLUDE_TRACE_ALWAYS)
#define A(format, ...)	\
do {						\
	system_printf(format, ##__VA_ARGS__);\
} while (0)
#else
#define A(format, ...) do{}while (0)
#endif /* #if defined(INCLUDE_TRACE_ALWAYS) */

#if defined(INCLUDE_TRACE_VERBOSE)
#define V(x, format, ...)	\
do {						\
	if(TL_VB >= g_trace_map[x].level)		{\
		xPRINT(x, format, ##__VA_ARGS__);	}\
} while (0)

#define VCOND(x)			\
do {						\
	if (TL_VB < g_trace_map[x].level)		{\
		return;								}\
} while (0)

#else
#define V(x, format, ...)	do{}while(0)
#define VCOND(x)			do{}while(0)
#endif /* #if defined(INCLUDE_TRACE_VERBOSE) */

#if defined(INCLUDE_TRACE_INFO)
#define I(x, format, ...) 		\
do {						\
	if (TL_INFO >= g_trace_map[x].level)	{\
		xPRINT(x, format, ##__VA_ARGS__);	}\
} while (0)

#define ICOND(x)			\
do {						\
	if (TL_INFO < g_trace_map[x].level)		{\
		return;								}\
} while (0)
#else
#define I(x, format, ...) 	do{}while(0)
#define ICOND(x)			do{}while(0)
#endif /* #if defined(INCLUDE_TRACE_INFO) */

#if defined(INCLUDE_TRACE_ERROR)
#define E(x, format, ...)		\
do {						\
	if (TL_ERR >= g_trace_map[x].level)		{\
		xPRINT(x, format, ##__VA_ARGS__);	}\
} while (0)

#define ECOND(x)			\
do {						\
	if (TL_ERR < g_trace_map[x].level)		{\
		return;								}\
} while (0)
#else
#define E(x, format, ...)	do{}while(0)
#define ECOND(x)			do{}while(0)
#endif /* #if defined(INCLUDE_TRACE_ERROR) */

#else
#define A(format, ...)	do{}while(0)
#define V(x, format, ...)	do{}while(0)
#define I(x, format, ...) 	do{}while(0)
#define E(x, format, ...)	do{}while(0)
#define VCOND(x)			do{}while(0)
#define ICOND(x)			do{}while(0)
#define ECOND(x)			do{}while(0)
//#endif /* #if !defined(RELEASE) && !defined(UCODE) */
#endif //#if !defined(UCODE)

#define oled_init(sda, scl, ch, addr, type, flip, invert) do { \
	oledInit(sda, scl, ch, addr, type, flip, invert); \
} while(0)
#define oled_log(x, y, ...) do { \
	oledPrintf(x, y, 2,  __VA_ARGS__); \
} while(0)
#define oled_fill(x) do { \
	oledFill(x); \
} while(0)
#define oled_write_flash_block(x, y) do { \
	oledWriteFlashBlock(x, y); \
} while(0)

#endif    /* __UTIL_TRACE_H__ */
