#ifndef UTIL_BYTE_STREAM_H
#define UTIL_BYTE_STREAM_H

#include "system.h"
#include "system_type.h"
#include "util_prealloc_buffer.h"

enum {
	BS_NO_FRAME = 0,
	BS_UMAC_FRAME = 1,
	BS_LMAC_FRAME = 2
};

struct byte_stream {
	SYS_BUF *head;
	SYS_BUF *ptr;
	uint8_t *curr;
	int remain;
	int used_payload;
	int total;
	int offset;
	uint8_t pool_id;
	uint8_t bs_frame_type;
	struct prealloc_buffer *pb;
};

bool util_byte_stream_init(struct byte_stream *bs, uint8_t pool_id, uint8_t bs_frame_type,
									SYS_BUF *buf, uint16_t length, struct prealloc_buffer *pb);
bool util_byte_stream_is_empty(struct byte_stream *bs);
void util_byte_stream_load(struct byte_stream *bs, uint8_t pool_id, uint8_t bs_frame_type,
									SYS_BUF *buf, uint16_t offset);
void util_byte_stream_put_uint8(struct byte_stream *bs, uint8_t v);
void util_byte_stream_put_uint16(struct byte_stream *bs, uint16_t v);
void util_byte_stream_put_uint32(struct byte_stream *bs, uint32_t v);
void util_byte_stream_put_string(struct byte_stream *bs, char *string);

bool util_byte_stream_get_uint8(struct byte_stream *bs, uint8_t *v);
bool util_byte_stream_get_uint16(struct byte_stream *bs, uint16_t *v);
bool util_byte_stream_get_uint32(struct byte_stream *bs, uint32_t *v);
char *util_byte_stream_get_string(struct byte_stream *bs);
#endif /* UTIL_BYTE_STREAM_H */
