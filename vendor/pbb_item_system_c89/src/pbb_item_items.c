#include "pbb_item_internal.h"
#include <string.h>

int pbb_item_i_spawn_internal(PBB_ItemWorld *world,
                                   int def_id,
                                   PBB_Fixed x,
                                   PBB_Fixed y,
                                   int amount,
                                   int state,
                                   int run_spawn_hook,
                                   int emit_spawn_event)
{
    PBB_ItemDef *def;
    PBB_Item *item;
    int i;
    int item_id;
    int pass;

    if (world == 0) {
        return PBB_ITEM_INVALID_ID;
    }

    def = pbb_item_def_get(world, def_id);
    if (def == 0) {
        return PBB_ITEM_INVALID_ID;
    }

    item_id = PBB_ITEM_INVALID_ID;
    for (i = 0; i < PBB_ITEM_MAX_ITEMS; ++i) {
        if (!world->items[i].used) {
            item_id = i;
            break;
        }
    }

    if (item_id == PBB_ITEM_INVALID_ID) {
        return PBB_ITEM_INVALID_ID;
    }

    item = &world->items[item_id];
    memset(item, 0, sizeof(*item));
    item->used = 1;
    item->item_id = item_id;
    item->def_id = def_id;
    item->state = state;
    item->amount = amount;
    if (item->amount <= 0) {
        item->amount = def->default_amount;
    }
    if (item->amount <= 0) {
        item->amount = 1;
    }
    item->owner_actor_id = PBB_ITEM_INVALID_ID;
    item->flags = def->default_flags;
    item->category_mask = def->default_category_mask;
    item->x = x;
    item->y = y;
    item->half_w = def->default_half_w;
    item->half_h = def->default_half_h;

    if (run_spawn_hook) {
        pass = pbb_item_eval_rule_block(world,
                                        PBB_ITEM_INVALID_ID,
                                        item_id,
                                        def->rule_first[PBB_ITEM_HOOK_SPAWN],
                                        def->rule_count[PBB_ITEM_HOOK_SPAWN]);
        if (!pass) {
            pbb_item_event_push(world,
                                PBB_EVENT_ITEM_RULE_BLOCKED,
                                PBB_ITEM_INVALID_ID,
                                item_id,
                                def_id,
                                PBB_ITEM_HOOK_SPAWN,
                                0,
                                0,
                                x,
                                y);
            item->used = 0;
            return PBB_ITEM_INVALID_ID;
        }

        pbb_item_apply_effect_block(world,
                                    PBB_ITEM_INVALID_ID,
                                    item_id,
                                    def->effect_first[PBB_ITEM_HOOK_SPAWN],
                                    def->effect_count[PBB_ITEM_HOOK_SPAWN]);
    }

    if (emit_spawn_event) {
        pbb_item_event_push(world,
                            PBB_EVENT_ITEM_SPAWNED,
                            PBB_ITEM_INVALID_ID,
                            item_id,
                            def_id,
                            item->amount,
                            state,
                            0,
                            x,
                            y);
    }

    return item_id;
}


int pbb_item_spawn(PBB_ItemWorld *world,
                   int def_id,
                   PBB_Fixed x,
                   PBB_Fixed y,
                   int amount)
{
    return pbb_item_i_spawn_internal(world,
                                   def_id,
                                   x,
                                   y,
                                   amount,
                                   PBB_ITEM_STATE_WORLD,
                                   1,
                                   1);
}

int pbb_item_destroy(PBB_ItemWorld *world, int item_id)
{
    PBB_Item *item;
    int def_id;
    PBB_Fixed x;
    PBB_Fixed y;

    item = pbb_item_get(world, item_id);
    if (item == 0) {
        return 0;
    }

    def_id = item->def_id;
    x = item->x;
    y = item->y;
    item->used = 0;
    item->state = PBB_ITEM_STATE_NONE;

    pbb_item_event_push(world,
                        PBB_EVENT_ITEM_DESTROYED,
                        PBB_ITEM_INVALID_ID,
                        item_id,
                        def_id,
                        0,
                        0,
                        0,
                        x,
                        y);
    return 1;
}

int pbb_item_hide(PBB_ItemWorld *world, int item_id)
{
    PBB_Item *item;

    item = pbb_item_get(world, item_id);
    if (item == 0) {
        return 0;
    }

    item->state = PBB_ITEM_STATE_HIDDEN;
    item->flags &= ~(PBB_ITEMF_ACTIVE | PBB_ITEMF_VISIBLE | PBB_ITEMF_TOUCHABLE | PBB_ITEMF_INTERACTABLE);
    return 1;
}

int pbb_item_show(PBB_ItemWorld *world, int item_id)
{
    PBB_Item *item;

    item = pbb_item_get(world, item_id);
    if (item == 0) {
        return 0;
    }

    item->state = PBB_ITEM_STATE_WORLD;
    item->flags |= (PBB_ITEMF_ACTIVE | PBB_ITEMF_VISIBLE);
    return 1;
}

int pbb_item_set_state(PBB_ItemWorld *world, int item_id, int state)
{
    PBB_Item *item;

    item = pbb_item_get(world, item_id);
    if (item == 0) {
        return 0;
    }

    item->state = state;
    return 1;
}

int pbb_item_set_category(PBB_ItemWorld *world, int item_id, unsigned long category_mask)
{
    PBB_Item *item;

    item = pbb_item_get(world, item_id);
    if (item == 0) {
        return 0;
    }

    item->category_mask = category_mask;
    return 1;
}

int pbb_item_set_box(PBB_ItemWorld *world,
                     int item_id,
                     PBB_Fixed x,
                     PBB_Fixed y,
                     PBB_Fixed half_w,
                     PBB_Fixed half_h)
{
    PBB_Item *item;

    item = pbb_item_get(world, item_id);
    if (item == 0) {
        return 0;
    }

    item->x = x;
    item->y = y;
    item->half_w = half_w;
    item->half_h = half_h;
    return 1;
}

PBB_Item *pbb_item_get(PBB_ItemWorld *world, int item_id)
{
    if (world == 0) {
        return 0;
    }

    if (item_id < 0 || item_id >= PBB_ITEM_MAX_ITEMS) {
        return 0;
    }

    if (!world->items[item_id].used) {
        return 0;
    }

    return &world->items[item_id];
}

const PBB_Item *pbb_item_get_const(const PBB_ItemWorld *world, int item_id)
{
    if (world == 0) {
        return 0;
    }

    if (item_id < 0 || item_id >= PBB_ITEM_MAX_ITEMS) {
        return 0;
    }

    if (!world->items[item_id].used) {
        return 0;
    }

    return &world->items[item_id];
}

int pbb_item_is_world_active(const PBB_ItemWorld *world, int item_id)
{
    const PBB_Item *item;

    item = pbb_item_get_const(world, item_id);
    if (item == 0) {
        return 0;
    }

    if ((item->flags & PBB_ITEMF_ACTIVE) == 0UL) {
        return 0;
    }

    if ((item->flags & PBB_ITEMF_VISIBLE) == 0UL) {
        return 0;
    }

    if (item->state != PBB_ITEM_STATE_WORLD && item->state != PBB_ITEM_STATE_DROPPED) {
        return 0;
    }

    return 1;
}

int pbb_item_aabb_overlap_actor_item(const PBB_ItemWorld *world, int actor_id, int item_id)
{
    const PBB_ItemActor *actor;
    const PBB_Item *item;
    PBB_Fixed a_left;
    PBB_Fixed a_right;
    PBB_Fixed a_top;
    PBB_Fixed a_bottom;
    PBB_Fixed b_left;
    PBB_Fixed b_right;
    PBB_Fixed b_top;
    PBB_Fixed b_bottom;

    actor = pbb_item_actor_get_const(world, actor_id);
    item = pbb_item_get_const(world, item_id);

    if (actor == 0 || item == 0) {
        return 0;
    }

    a_left = actor->x - actor->half_w;
    a_right = actor->x + actor->half_w;
    a_top = actor->y - actor->half_h;
    a_bottom = actor->y + actor->half_h;

    b_left = item->x - item->half_w;
    b_right = item->x + item->half_w;
    b_top = item->y - item->half_h;
    b_bottom = item->y + item->half_h;

    if (a_right < b_left) {
        return 0;
    }
    if (a_left > b_right) {
        return 0;
    }
    if (a_bottom < b_top) {
        return 0;
    }
    if (a_top > b_bottom) {
        return 0;
    }

    return 1;
}

int pbb_item_count_used(const PBB_ItemWorld *world)
{
    int i;
    int count;

    if (world == 0) {
        return 0;
    }

    count = 0;
    for (i = 0; i < PBB_ITEM_MAX_ITEMS; ++i) {
        if (world->items[i].used) {
            count += 1;
        }
    }

    return count;
}

int pbb_item_count_active(const PBB_ItemWorld *world)
{
    int i;
    int count;

    if (world == 0) {
        return 0;
    }

    count = 0;
    for (i = 0; i < PBB_ITEM_MAX_ITEMS; ++i) {
        if (pbb_item_is_world_active(world, i)) {
            count += 1;
        }
    }

    return count;
}

int pbb_item_count_visible(const PBB_ItemWorld *world)
{
    int i;
    int count;

    if (world == 0) {
        return 0;
    }

    count = 0;
    for (i = 0; i < PBB_ITEM_MAX_ITEMS; ++i) {
        if (world->items[i].used) {
            if ((world->items[i].flags & PBB_ITEMF_VISIBLE) != 0UL) {
                count += 1;
            }
        }
    }

    return count;
}

int pbb_item_gc_consumed(PBB_ItemWorld *world)
{
    int i;
    int count;

    if (world == 0) {
        return 0;
    }

    count = 0;
    for (i = 0; i < PBB_ITEM_MAX_ITEMS; ++i) {
        if (world->items[i].used &&
            world->items[i].state == PBB_ITEM_STATE_CONSUMED) {
            memset(&world->items[i], 0, sizeof(world->items[i]));
            count += 1;
        }
    }

    return count;
}

