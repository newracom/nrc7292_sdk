#ifndef UTIL_CHAIN_H
#define UTIL_CHAIN_H

#include "util_list.h"
#if defined(NRC_ROMLIB)
#include "romlib.h"
#endif /* defined(NRC_ROMLIB) */
struct chain_item {
    struct dl_list entry;
    struct chain_item *head;
    uint32_t flags;
    int chain_id;
    void (*fn)(struct chain_item *item);
};

#define from_chain(param, type, field) \
            container_of(param, type, field)

#if defined(NRC_ROMLIB)
int chain_create(struct rlif_t *rlif, int *chain_id);
int chain_delete(struct rlif_t *rlif, int chain_id);
int chain_run(struct rlif_t *rlif, int id);
int chain_add_item(struct rlif_t *rlif, struct chain_item *item);
int chain_add_item_front(struct rlif_t *rlif, struct chain_item *item);
///int chain_delete_item(struct rlif_t *rlif, struct chain_item *item);
int chain_delete_item(struct chain_item *item);
int chain_delete_items_by_fn(struct rlif_t *rlif, void (*func)(struct chain_item *));
#else
int chain_create(int *chain_id);
int chain_delete(int chain_id);
int chain_run(int id);
int chain_add_item(struct chain_item *item);
int chain_add_item_front(struct chain_item *item);
int chain_delete_item(struct chain_item *item);
int chain_delete_items_by_fn(void (*func)(struct chain_item *));
#endif /* defined(NRC_ROMLIB) */

#endif /* UTIL_CHAIN_H */