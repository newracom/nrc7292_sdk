#ifndef UTIL_CHAIN_H
#define UTIL_CHAIN_H

#include "util_list.h"

struct chain_item {
    struct dl_list entry;
    struct chain_item *head;
    uint32_t flags;
    int chain_id;
    void (*fn)(struct chain_item *item);
};

#define from_chain(param, type, field) \
            container_of(param, type, field)

int chain_create(int *chain_id);
int chain_delete(int chain_id);
int chain_run(int id);
int chain_add_item(struct chain_item *item);
int chain_add_item_front(struct chain_item *item);
int chain_delete_item(struct chain_item *item);
int chain_delete_item_by_fn(void (*func)(struct chain_item *));

#endif /* UTIL_CHAIN_H */