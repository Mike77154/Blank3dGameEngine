#include "pbb_item_internal.h"
#include <string.h>

int pbb_item_actor_create(PBB_ItemWorld *world,
                          unsigned long class_mask,
                          unsigned long team_mask,
                          PBB_Fixed x,
                          PBB_Fixed y,
                          PBB_Fixed half_w,
                          PBB_Fixed half_h)
{
    PBB_ItemActor *actor;
    int i;

    if (world == 0) {
        return PBB_ITEM_INVALID_ID;
    }

    for (i = 0; i < PBB_ITEM_MAX_ACTORS; ++i) {
        if (!world->actors[i].used) {
            actor = &world->actors[i];
            memset(actor, 0, sizeof(*actor));
            actor->used = 1;
            actor->actor_id = i;
            actor->class_mask = class_mask;
            actor->team_mask = team_mask;
            actor->touch_mask = PBB_ITEM_CATEGORY_ANY;
            actor->interact_mask = PBB_ITEM_CATEGORY_ANY;
            actor->x = x;
            actor->y = y;
            actor->half_w = half_w;
            actor->half_h = half_h;
            return i;
        }
    }

    return PBB_ITEM_INVALID_ID;
}

int pbb_item_actor_destroy(PBB_ItemWorld *world, int actor_id)
{
    PBB_ItemActor *actor;

    actor = pbb_item_actor_get(world, actor_id);
    if (actor == 0) {
        return 0;
    }

    memset(actor, 0, sizeof(*actor));
    return 1;
}

PBB_ItemActor *pbb_item_actor_get(PBB_ItemWorld *world, int actor_id)
{
    if (world == 0) {
        return 0;
    }

    if (actor_id < 0 || actor_id >= PBB_ITEM_MAX_ACTORS) {
        return 0;
    }

    if (!world->actors[actor_id].used) {
        return 0;
    }

    return &world->actors[actor_id];
}

const PBB_ItemActor *pbb_item_actor_get_const(const PBB_ItemWorld *world, int actor_id)
{
    if (world == 0) {
        return 0;
    }

    if (actor_id < 0 || actor_id >= PBB_ITEM_MAX_ACTORS) {
        return 0;
    }

    if (!world->actors[actor_id].used) {
        return 0;
    }

    return &world->actors[actor_id];
}

int pbb_item_actor_set_box(PBB_ItemWorld *world,
                           int actor_id,
                           PBB_Fixed x,
                           PBB_Fixed y,
                           PBB_Fixed half_w,
                           PBB_Fixed half_h)
{
    PBB_ItemActor *actor;

    actor = pbb_item_actor_get(world, actor_id);
    if (actor == 0) {
        return 0;
    }

    actor->x = x;
    actor->y = y;
    actor->half_w = half_w;
    actor->half_h = half_h;
    return 1;
}

int pbb_item_actor_set_masks(PBB_ItemWorld *world,
                             int actor_id,
                             unsigned long touch_mask,
                             unsigned long interact_mask)
{
    PBB_ItemActor *actor;

    actor = pbb_item_actor_get(world, actor_id);
    if (actor == 0) {
        return 0;
    }

    actor->touch_mask = touch_mask;
    actor->interact_mask = interact_mask;
    return 1;
}

int pbb_item_actor_set_var(PBB_ItemWorld *world, int actor_id, int var_id, int value)
{
    PBB_ItemActor *actor;

    actor = pbb_item_actor_get(world, actor_id);
    if (actor == 0) {
        return 0;
    }

    if (!pbb_item_i_var_valid(var_id, PBB_ITEM_MAX_ACTOR_VARS)) {
        return 0;
    }

    actor->vars[var_id] = value;
    return 1;
}

int pbb_item_actor_get_var(const PBB_ItemWorld *world, int actor_id, int var_id)
{
    const PBB_ItemActor *actor;

    actor = pbb_item_actor_get_const(world, actor_id);
    if (actor == 0) {
        return 0;
    }

    if (!pbb_item_i_var_valid(var_id, PBB_ITEM_MAX_ACTOR_VARS)) {
        return 0;
    }

    return actor->vars[var_id];
}

int pbb_item_actor_add_var(PBB_ItemWorld *world, int actor_id, int var_id, int delta)
{
    PBB_ItemActor *actor;

    actor = pbb_item_actor_get(world, actor_id);
    if (actor == 0) {
        return 0;
    }

    if (!pbb_item_i_var_valid(var_id, PBB_ITEM_MAX_ACTOR_VARS)) {
        return 0;
    }

    actor->vars[var_id] += delta;
    return 1;
}

int pbb_item_actor_set_flag(PBB_ItemWorld *world, int actor_id, int flag_id, int value)
{
    PBB_ItemActor *actor;

    actor = pbb_item_actor_get(world, actor_id);
    if (actor == 0) {
        return 0;
    }

    return pbb_item_i_flag_set_words(actor->flags, flag_id, value);
}

int pbb_item_actor_get_flag(const PBB_ItemWorld *world, int actor_id, int flag_id)
{
    const PBB_ItemActor *actor;

    actor = pbb_item_actor_get_const(world, actor_id);
    if (actor == 0) {
        return 0;
    }

    return pbb_item_i_flag_get_words(actor->flags, flag_id);
}

