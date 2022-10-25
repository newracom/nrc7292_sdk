#ifndef SYSTEM_CHAIN_HANDLER_H
#define SYSTEM_CHAIN_HANDLER_H

#include "util_byte_stream.h"

/* Chain Handler Structure */
struct chain_handler_item {
	int (*fn) (struct byte_stream *bs, void *arg);
	void *arg;
};

int system_chain_handler_do(const struct chain_handler_item *list, struct byte_stream *bs);

#endif //SYSTEM_CHAIN_HANDLER_H
