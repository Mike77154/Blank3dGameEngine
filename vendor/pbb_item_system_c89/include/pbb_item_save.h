#ifndef PBB_ITEM_SAVE_H
#define PBB_ITEM_SAVE_H
#include "pbb_item_types.h"
#ifdef __cplusplus
extern "C" {
#endif
#define PBB_ITEM_SAVE_MAGIC 0x50424932UL
typedef struct PBB_ItemSaveState {
    unsigned long magic;
    int version_major;
    int version_minor;
    int version_patch;
    PBB_ItemActor actors[PBB_ITEM_MAX_ACTORS];
    PBB_Item items[PBB_ITEM_MAX_ITEMS];
    int world_vars[PBB_ITEM_MAX_WORLD_VARS];
    unsigned long world_flags[PBB_ITEM_FLAG_WORDS];
    unsigned long rng_state;
    int strict_droppable;
    int recycle_consumed_items;
} PBB_ItemSaveState;
void pbb_item_save_state_init(PBB_ItemSaveState *state);
int pbb_item_save_runtime(const PBB_ItemWorld *world, PBB_ItemSaveState *state);
int pbb_item_load_runtime(PBB_ItemWorld *world, const PBB_ItemSaveState *state);
#ifdef __cplusplus
}
#endif
#endif
