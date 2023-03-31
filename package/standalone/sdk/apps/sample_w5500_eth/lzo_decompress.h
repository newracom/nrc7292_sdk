#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define LZO_VERSION		0x2020
#define LZO_VERSION_STRING	"2.02"
#define LZO_VERSION_DATE	"Oct 17 2005"

#define M1_MAX_OFFSET	0x0400
#define M2_MAX_OFFSET	0x0800
#define M3_MAX_OFFSET	0x4000
#define M4_MAX_OFFSET	0xbfff

#define M1_MIN_LEN	2
#define M1_MAX_LEN	2
#define M2_MIN_LEN	3
#define M2_MAX_LEN	8
#define M3_MIN_LEN	3
#define M3_MAX_LEN	33
#define M4_MIN_LEN	3
#define M4_MAX_LEN	9

#define M1_MARKER	0
#define M2_MARKER	64
#define M3_MARKER	32
#define M4_MARKER	16

#define D_BITS		14
#define D_MASK		((1u << D_BITS) - 1)
#define D_HIGH		((D_MASK >> 1) + 1)

#define DX2(p, s1, s2)	(((((size_t)((p)[2]) << (s2)) ^ (p)[1]) \
							<< (s1)) ^ (p)[0])
#define DX3(p, s1, s2, s3)	((DX2((p)+1, s2, s3) << (s1)) ^ (p)[0])

#define LZO_E_OK   0
#define LZO_E_ERROR   (-1)
#define LZO_E_OUT_OF_MEMORY   (-2)
#define LZO_E_NOT_COMPRESSIBLE   (-3)
#define LZO_E_INPUT_OVERRUN   (-4)
#define LZO_E_OUTPUT_OVERRUN   (-5)
#define LZO_E_LOOKBEHIND_OVERRUN   (-6)
#define LZO_E_EOF_NOT_FOUND   (-7)
#define LZO_E_INPUT_NOT_CONSUMED   (-8)
#define LZO_E_NOT_YET_IMPLEMENTED   (-9)

int lzo1x_decompress_safe(const unsigned char *in, size_t in_len,
			unsigned char *out, size_t *out_len);
int lzop_decompress(const unsigned char *src, size_t src_len,
		    unsigned char *dst, size_t *dst_len);
