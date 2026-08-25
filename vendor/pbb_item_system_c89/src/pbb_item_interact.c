#include "pbb_item_internal.h"
#include <string.h>

static int pbb_item_actor_can_touch_item(const PBB_ItemWorld *world, int actor_id, int item_id)
{
    const PBB_ItemActor *actor;
    const PBB_Item *item;

    actor = pbb_item_actor_get_const(world, actor_id);
    item = pbb_item_get_const(world, item_id);
    if (actor == 0 || item == 0) {
        return 0;
    }

    return ((actor->touch_mask & item->category_mask) != 0UL);
}

static int pbb_item_actor_can_interact_item(const PBB_ItemWorld *world, int actor_id, int item_id)
{
    const PBB_ItemActor *actor;
    const PBB_Item *item;

    actor = pbb_item_actor_get_const(world, actor_id);
    item = pbb_item_get_const(world, item_id);
    if (actor == 0 || item == 0) {
        return 0;
    }

    return ((actor->interact_mask & item->category_mask) != 0UL);
}

int pbb_item_touch_actor_item(PBB_ItemWorld *world, int actor_id, int item_id)
{
    PBB_Item *item;
    PBB_ItemDef *def;
    PBB_Fixed x;
    PBB_Fixed y;
    int def_id;
    int pass;

    if (pbb_item_actor_get(world, actor_id) == 0) {
        return 0;
    }

    item = pbb_item_get(world, item_id);
    if (item == 0) {
        return 0;
    }

    if (!pbb_item_is_world_active(world, item_id)) {
        return 0;
    }

    if ((item->flags & PBB_ITEMF_TOUCHABLE) == 0UL) {
        return 0;
    }

    if (!pbb_item_actor_can_touch_item(world, actor_id, item_id)) {
        pbb_item_event_push(world,
                            PBB_EVENT_ITEM_RULE_BLOCKED,
                            actor_id,
                            item_id,
                            item->def_id,
                            PBB_ITEM_HOOK_TOUCH,
                            PBB_ITEM_BLOCK_REASON_MASK,
                            0,
                            item->x,
                            item->y);
        return 0;
    }

    def_id = item->def_id;
    def = pbb_item_def_get(world, def_id);
    if (def == 0) {
        return 0;
    }

    pass = pbb_item_eval_rule_block(world,
                                    actor_id,
                                    item_id,
                                    def->rule_first[PBB_ITEM_HOOK_TOUCH],
                                    def->rule_count[PBB_ITEM_HOOK_TOUCH]);
    if (!pass) {
        pbb_item_event_push(world,
                            PBB_EVENT_ITEM_RULE_BLOCKED,
                            actor_id,
                            item_id,
                            def_id,
                            PBB_ITEM_HOOK_TOUCH,
                            0,
                            0,
                            item->x,
                            item->y);
        return 0;
    }

    if (world->action_gate != 0 &&
        !world->action_gate(world, actor_id, item_id,
                            PBB_ITEM_HOOK_TOUCH,
                            world->action_gate_user)) {
        pbb_item_event_push(world,
                            PBB_EVENT_ITEM_RULE_BLOCKED,
                            actor_id,
                            item_id,
                            def_id,
                            PBB_ITEM_HOOK_TOUCH,
                            PBB_ITEM_BLOCK_REASON_PROVIDER,
                            0,
                            item->x,
                            item->y);
        return 0;
    }

    x = item->x;
    y = item->y;
    item->touch_count += 1;
    pbb_item_apply_effect_block(world,
                                actor_id,
                                item_id,
                                def->effect_first[PBB_ITEM_HOOK_TOUCH],
                                def->effect_count[PBB_ITEM_HOOK_TOUCH]);

    pbb_item_event_push(world,
                        PBB_EVENT_ITEM_TOUCHED,
                        actor_id,
                        item_id,
                        def_id,
                        0,
                        0,
                        0,
                        x,
                        y);

    return 1;
}

int pbb_item_touch_actor_all(PBB_ItemWorld *world, int actor_id)
{
    int i;
    int touched;

    if (world == 0) {
        return 0;
    }

    if (pbb_item_actor_get(world, actor_id) == 0) {
        return 0;
    }

    touched = 0;
    for (i = 0; i < PBB_ITEM_MAX_ITEMS; ++i) {
        if (world->items[i].used) {
            if (pbb_item_is_world_active(world, i)) {
                if ((world->items[i].flags & PBB_ITEMF_TOUCHABLE) != 0UL) {
                    if (pbb_item_aabb_overlap_actor_item(world, actor_id, i)) {
                        if (pbb_item_touch_actor_item(world, actor_id, i)) {
                            touched += 1;
                        }
                    }
                }
            }
        }
    }

    return touched;
}

int pbb_item_interact_actor_item(PBB_ItemWorld *world, int actor_id, int item_id)
{
    PBB_Item *item;
    PBB_ItemDef *def;
    PBB_Fixed x;
    PBB_Fixed y;
    int def_id;
    int pass;

    if (pbb_item_actor_get(world, actor_id) == 0) {
        return 0;
    }

    item = pbb_item_get(world, item_id);
    if (item == 0) {
        return 0;
    }

    if (!pbb_item_is_world_active(world, item_id)) {
        return 0;
    }

    if ((item->flags & PBB_ITEMF_INTERACTABLE) == 0UL) {
        return 0;
    }

    if (!pbb_item_actor_can_interact_item(world, actor_id, item_id)) {
        pbb_item_event_push(world,
                            PBB_EVENT_ITEM_RULE_BLOCKED,
                            actor_id,
                            item_id,
                            item->def_id,
                            PBB_ITEM_HOOK_INTERACT,
                            PBB_ITEM_BLOCK_REASON_MASK,
                            0,
                            item->x,
                            item->y);
        return 0;
    }

    def_id = item->def_id;
    def = pbb_item_def_get(world, def_id);
    if (def == 0) {
        return 0;
    }

    pass = pbb_item_eval_rule_block(world,
                                    actor_id,
                                    item_id,
                                    def->rule_first[PBB_ITEM_HOOK_INTERACT],
                                    def->rule_count[PBB_ITEM_HOOK_INTERACT]);
    if (!pass) {
        pbb_item_event_push(world,
                            PBB_EVENT_ITEM_RULE_BLOCKED,
                            actor_id,
                            item_id,
                            def_id,
                            PBB_ITEM_HOOK_INTERACT,
                            0,
                            0,
                            item->x,
                            item->y);
        return 0;
    }

    if (world->action_gate != 0 &&
        !world->action_gate(world, actor_id, item_id,
                            PBB_ITEM_HOOK_INTERACT,
                            world->action_gate_user)) {
        pbb_item_event_push(world,
                            PBB_EVENT_ITEM_RULE_BLOCKED,
                            actor_id,
                            item_id,
                            def_id,
                            PBB_ITEM_HOOK_INTERACT,
                            PBB_ITEM_BLOCK_REASON_PROVIDER,
                            0,
                            item->x,
                            item->y);
        return 0;
    }

    x = item->x;
    y = item->y;
    item->interact_count += 1;
    pbb_item_apply_effect_block(world,
                                actor_id,
                                item_id,
                                def->effect_first[PBB_ITEM_HOOK_INTERACT],
                                def->effect_count[PBB_ITEM_HOOK_INTERACT]);

    pbb_item_event_push(world,
                        PBB_EVENT_ITEM_INTERACTED,
                        actor_id,
                        item_id,
                        def_id,
                        0,
                        0,
                        0,
                        x,
                        y);

    return 1;
}

int pbb_item_interact_actor_nearest(PBB_ItemWorld *world,
                                    int actor_id,
                                    PBB_Fixed max_center_distance_x,
                                    PBB_Fixed max_center_distance_y)
{
    PBB_ItemActor *actor;
    PBB_Item *item;
    int i;
    int best_id;
    PBB_Fixed dx;
    PBB_Fixed dy;
    PBB_Fixed score;
    PBB_Fixed best_score;

    if (world == 0) {
        return 0;
    }

    actor = pbb_item_actor_get(world, actor_id);
    if (actor == 0) {
        return 0;
    }

    best_id = PBB_ITEM_INVALID_ID;
    best_score = 0;

    for (i = 0; i < PBB_ITEM_MAX_ITEMS; ++i) {
        item = &world->items[i];
        if (item->used) {
            if (pbb_item_is_world_active(world, i)) {
                if ((item->flags & PBB_ITEMF_INTERACTABLE) != 0UL &&
                    pbb_item_actor_can_interact_item(world, actor_id, i)) {
                    dx = pbb_item_i_fixed_abs(item->x - actor->x);
                    dy = pbb_item_i_fixed_abs(item->y - actor->y);
                    if (dx <= max_center_distance_x && dy <= max_center_distance_y) {
                        score = dx + dy;
                        if (best_id == PBB_ITEM_INVALID_ID || score < best_score) {
                            best_id = i;
                            best_score = score;
                        }
                    }
                }
            }
        }
    }

    if (best_id == PBB_ITEM_INVALID_ID) {
        return 0;
    }

    return pbb_item_interact_actor_item(world, actor_id, best_id);
}

