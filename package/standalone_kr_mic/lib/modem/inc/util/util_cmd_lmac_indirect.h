#ifndef _INDIRECT_REG_H_
#define _INDIRECT_REG_H_

#include "util_byte_stream.h"

#if defined (INCLUDE_LMAC_DEBUG)
#if defined(STANDARD_11N)
#define N_CATEGORY     5
enum {
    MAX_TX = 112,
    MAX_RX = 45,
    MAX_DMA = 26,
    MAX_SEC = 12,
    MAX_IRQ = 16,
    CNT_MAX = 115
};
#endif /* #if defined(STANDARD_11N) */
#if defined(STANDARD_11AH)
#define N_CATEGORY     5
enum {
    MAX_TX = 97,
    MAX_RX = 53,
    MAX_SEC = 20,
    MAX_DMA = 26,
    MAX_IRQ = 27,
    CNT_MAX = 100
};
#endif /* #if defined(STANDARD_11AH) */

typedef struct {
	uint16_t    offset;
	uint16_t    operand;
	uint8_t     start_bit;
	uint8_t     end_bit;
	char        notation;
	const char  *description;
	const char  *unit;
} RegDetail_t;


typedef struct {
	const char  *category;
	uint32_t    addr;
	uint32_t    data_addr;
	uint16_t    list_count;
	RegDetail_t reg_list[CNT_MAX];
} IndirectReg_t;

void dump_indirect_reg(struct byte_stream *bs);
void show_indirect_reg(const char *cat);
#else
static inline void dump_indirect_reg(struct byte_stream *bs){};
static inline void show_indirect_reg(const char *cat){};
#endif //#if defined (INCLUDE_LMAC_DEBUG)
#endif /* #ifndef _INDIRECT_REG_H_ */
