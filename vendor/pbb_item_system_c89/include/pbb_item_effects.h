#ifndef PBB_ITEM_EFFECTS_H
#define PBB_ITEM_EFFECTS_H
#include "pbb_item_types.h"
#ifdef __cplusplus
extern "C" {
#endif
int pbb_item_apply_effect_block(PBB_ItemWorld *world, int actor_id, int item_id, int first_effect, int effect_count);
int pbb_item_apply_effect(PBB_ItemWorld *world, int actor_id, int item_id, const PBB_ItemEffect *effect);
#ifdef __cplusplus
}
#endif
#endif
