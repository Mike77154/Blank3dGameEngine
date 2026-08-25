#ifndef PBB_ITEM_RUNTIME_H
#define PBB_ITEM_RUNTIME_H
#include "pbb_item_types.h"
#ifdef __cplusplus
extern "C" {
#endif
int pbb_item_spawn(PBB_ItemWorld *world, int def_id, PBB_Fixed x, PBB_Fixed y, int amount);
int pbb_item_destroy(PBB_ItemWorld *world, int item_id);
int pbb_item_hide(PBB_ItemWorld *world, int item_id);
int pbb_item_show(PBB_ItemWorld *world, int item_id);
int pbb_item_set_state(PBB_ItemWorld *world, int item_id, int state);
int pbb_item_set_category(PBB_ItemWorld *world, int item_id, unsigned long category_mask);
int pbb_item_set_box(PBB_ItemWorld *world, int item_id, PBB_Fixed x, PBB_Fixed y, PBB_Fixed half_w, PBB_Fixed half_h);
PBB_Item *pbb_item_get(PBB_ItemWorld *world, int item_id);
const PBB_Item *pbb_item_get_const(const PBB_ItemWorld *world, int item_id);
int pbb_item_is_world_active(const PBB_ItemWorld *world, int item_id);
int pbb_item_aabb_overlap_actor_item(const PBB_ItemWorld *world, int actor_id, int item_id);
int pbb_item_count_used(const PBB_ItemWorld *world);
int pbb_item_count_active(const PBB_ItemWorld *world);
int pbb_item_count_visible(const PBB_ItemWorld *world);
int pbb_item_gc_consumed(PBB_ItemWorld *world);
int pbb_item_touch_actor_item(PBB_ItemWorld *world, int actor_id, int item_id);
int pbb_item_touch_actor_all(PBB_ItemWorld *world, int actor_id);
int pbb_item_interact_actor_item(PBB_ItemWorld *world, int actor_id, int item_id);
int pbb_item_interact_actor_nearest(PBB_ItemWorld *world, int actor_id, PBB_Fixed max_center_distance_x, PBB_Fixed max_center_distance_y);
int pbb_item_drop_from_actor(PBB_ItemWorld *world, int actor_id, int def_id, int amount, PBB_Fixed offset_x, PBB_Fixed offset_y);
int pbb_item_drop_at(PBB_ItemWorld *world, int actor_id, int def_id, int amount, PBB_Fixed x, PBB_Fixed y);
#ifdef __cplusplus
}
#endif
#endif
