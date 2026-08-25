#ifndef PBB_ITEM_DROPS_H
#define PBB_ITEM_DROPS_H
#include "pbb_item_types.h"
#ifdef __cplusplus
extern "C" {
#endif
int pbb_item_drop_table_create(PBB_ItemWorld *world, const char *name);
int pbb_item_drop_table_add_entry(PBB_ItemWorld *world, int table_id, int def_id, int min_amount, int max_amount, int chance_per_10000, PBB_Fixed offset_x, PBB_Fixed offset_y);
int pbb_item_drop_table_roll_at(PBB_ItemWorld *world, int table_id, int actor_id, PBB_Fixed x, PBB_Fixed y);
int pbb_item_drop_table_roll_from_actor(PBB_ItemWorld *world, int table_id, int actor_id);
#ifdef __cplusplus
}
#endif
#endif
