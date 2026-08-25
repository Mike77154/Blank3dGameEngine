#ifndef PBB_ITEM_WORLD_H
#define PBB_ITEM_WORLD_H
#include "pbb_item_types.h"
#ifdef __cplusplus
extern "C" {
#endif
void pbb_item_world_init(PBB_ItemWorld *world);
void pbb_item_world_clear_runtime(PBB_ItemWorld *world);
void pbb_item_world_set_callbacks(PBB_ItemWorld *world, PBB_ItemCustomRuleFn custom_rule, PBB_ItemCustomEffectFn custom_effect, void *user_data);
void *pbb_item_world_get_user_data(PBB_ItemWorld *world);
void pbb_item_world_set_action_gate(PBB_ItemWorld *world, PBB_ItemActionGateFn gate, void *user);
PBB_ItemActionGateFn pbb_item_world_get_action_gate(const PBB_ItemWorld *world);
void *pbb_item_world_get_action_gate_user(const PBB_ItemWorld *world);
void pbb_item_world_set_rng(PBB_ItemWorld *world, unsigned long seed);
unsigned long pbb_item_world_rand(PBB_ItemWorld *world);
int pbb_item_world_rand_range(PBB_ItemWorld *world, int min_value, int max_value);
void pbb_item_world_set_strict_droppable(PBB_ItemWorld *world, int enabled);
int pbb_item_world_get_strict_droppable(const PBB_ItemWorld *world);
void pbb_item_world_set_recycle_consumed(PBB_ItemWorld *world, int enabled);
int pbb_item_world_get_recycle_consumed(const PBB_ItemWorld *world);
int pbb_item_world_set_var(PBB_ItemWorld *world, int var_id, int value);
int pbb_item_world_get_var(const PBB_ItemWorld *world, int var_id);
int pbb_item_world_add_var(PBB_ItemWorld *world, int var_id, int delta);
int pbb_item_world_set_flag(PBB_ItemWorld *world, int flag_id, int value);
int pbb_item_world_get_flag(const PBB_ItemWorld *world, int flag_id);
int pbb_item_actor_create(PBB_ItemWorld *world, unsigned long class_mask, unsigned long team_mask, PBB_Fixed x, PBB_Fixed y, PBB_Fixed half_w, PBB_Fixed half_h);
int pbb_item_actor_destroy(PBB_ItemWorld *world, int actor_id);
PBB_ItemActor *pbb_item_actor_get(PBB_ItemWorld *world, int actor_id);
const PBB_ItemActor *pbb_item_actor_get_const(const PBB_ItemWorld *world, int actor_id);
int pbb_item_actor_set_box(PBB_ItemWorld *world, int actor_id, PBB_Fixed x, PBB_Fixed y, PBB_Fixed half_w, PBB_Fixed half_h);
int pbb_item_actor_set_masks(PBB_ItemWorld *world, int actor_id, unsigned long touch_mask, unsigned long interact_mask);
int pbb_item_actor_set_var(PBB_ItemWorld *world, int actor_id, int var_id, int value);
int pbb_item_actor_get_var(const PBB_ItemWorld *world, int actor_id, int var_id);
int pbb_item_actor_add_var(PBB_ItemWorld *world, int actor_id, int var_id, int delta);
int pbb_item_actor_set_flag(PBB_ItemWorld *world, int actor_id, int flag_id, int value);
int pbb_item_actor_get_flag(const PBB_ItemWorld *world, int actor_id, int flag_id);
int pbb_item_def_create(PBB_ItemWorld *world, const char *name);
PBB_ItemDef *pbb_item_def_get(PBB_ItemWorld *world, int def_id);
const PBB_ItemDef *pbb_item_def_get_const(const PBB_ItemWorld *world, int def_id);
int pbb_item_def_set_defaults(PBB_ItemWorld *world, int def_id, unsigned long default_flags, int default_amount, PBB_Fixed default_half_w, PBB_Fixed default_half_h);
int pbb_item_def_set_category(PBB_ItemWorld *world, int def_id, unsigned long category_mask);
int pbb_item_def_set_rule_block(PBB_ItemWorld *world, int def_id, int hook, int first_rule, int rule_count);
int pbb_item_def_set_effect_block(PBB_ItemWorld *world, int def_id, int hook, int first_effect, int effect_count);
int pbb_item_rule_add(PBB_ItemWorld *world, int type, int a, int b, int c, int d);
PBB_ItemRule *pbb_item_rule_get(PBB_ItemWorld *world, int rule_id);
int pbb_item_effect_add(PBB_ItemWorld *world, int type, int a, int b, int c, int d, PBB_Fixed fx, PBB_Fixed fy);
PBB_ItemEffect *pbb_item_effect_get(PBB_ItemWorld *world, int effect_id);
#ifdef __cplusplus
}
#endif
#endif
