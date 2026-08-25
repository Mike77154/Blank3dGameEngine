#include "pbb_item_internal.h"
#include <string.h>

void pbb_item_world_init(PBB_ItemWorld *world)
{
    if (world == 0) {
        return;
    }

    memset(world, 0, sizeof(*world));
    world->rng_state = 1UL;
    world->strict_droppable = PBB_ITEM_DEFAULT_STRICT_DROPPABLE;
    world->recycle_consumed_items = PBB_ITEM_DEFAULT_RECYCLE_CONSUMED;
}

void pbb_item_world_clear_runtime(PBB_ItemWorld *world)
{
    if (world == 0) {
        return;
    }

    memset(world->actors, 0, sizeof(world->actors));
    memset(world->items, 0, sizeof(world->items));
    memset(world->events, 0, sizeof(world->events));
    memset(world->world_vars, 0, sizeof(world->world_vars));
    memset(world->world_flags, 0, sizeof(world->world_flags));
    world->event_read = 0;
    world->event_write = 0;
    world->event_count = 0;
    world->events_lost = 0;
}

void pbb_item_world_set_callbacks(PBB_ItemWorld *world,
                                  PBB_ItemCustomRuleFn custom_rule,
                                  PBB_ItemCustomEffectFn custom_effect,
                                  void *user_data)
{
    if (world == 0) {
        return;
    }

    world->custom_rule = custom_rule;
    world->custom_effect = custom_effect;
    world->user_data = user_data;
}

void *pbb_item_world_get_user_data(PBB_ItemWorld *world)
{
    if (world == 0) {
        return 0;
    }

    return world->user_data;
}

void pbb_item_world_set_action_gate(PBB_ItemWorld *world,
                                    PBB_ItemActionGateFn gate,
                                    void *user)
{
    if (world == 0) {
        return;
    }
    world->action_gate = gate;
    world->action_gate_user = user;
}

PBB_ItemActionGateFn pbb_item_world_get_action_gate(const PBB_ItemWorld *world)
{
    if (world == 0) {
        return 0;
    }
    return world->action_gate;
}

void *pbb_item_world_get_action_gate_user(const PBB_ItemWorld *world)
{
    if (world == 0) {
        return 0;
    }
    return world->action_gate_user;
}

void pbb_item_world_set_rng(PBB_ItemWorld *world, unsigned long seed)
{
    if (world == 0) {
        return;
    }

    if (seed == 0UL) {
        seed = 1UL;
    }

    world->rng_state = seed;
}

unsigned long pbb_item_world_rand(PBB_ItemWorld *world)
{
    if (world == 0) {
        return 0UL;
    }

    world->rng_state = world->rng_state * 1103515245UL + 12345UL;
    return (world->rng_state >> 16) & 0x7FFFUL;
}

int pbb_item_world_rand_range(PBB_ItemWorld *world, int min_value, int max_value)
{
    unsigned long r;
    int tmp;
    int span;

    if (world == 0) {
        return min_value;
    }

    if (max_value < min_value) {
        tmp = min_value;
        min_value = max_value;
        max_value = tmp;
    }

    span = max_value - min_value + 1;
    if (span <= 0) {
        return min_value;
    }

    r = pbb_item_world_rand(world);
    return min_value + (int)(r % (unsigned long)span);
}

void pbb_item_world_set_strict_droppable(PBB_ItemWorld *world, int enabled)
{
    if (world == 0) {
        return;
    }

    world->strict_droppable = enabled ? 1 : 0;
}

int pbb_item_world_get_strict_droppable(const PBB_ItemWorld *world)
{
    if (world == 0) {
        return 0;
    }

    return world->strict_droppable;
}

void pbb_item_world_set_recycle_consumed(PBB_ItemWorld *world, int enabled)
{
    if (world == 0) {
        return;
    }

    world->recycle_consumed_items = enabled ? 1 : 0;
}

int pbb_item_world_get_recycle_consumed(const PBB_ItemWorld *world)
{
    if (world == 0) {
        return 0;
    }

    return world->recycle_consumed_items;
}

int pbb_item_world_set_var(PBB_ItemWorld *world, int var_id, int value)
{
    if (world == 0) {
        return 0;
    }

    if (!pbb_item_i_var_valid(var_id, PBB_ITEM_MAX_WORLD_VARS)) {
        return 0;
    }

    world->world_vars[var_id] = value;
    return 1;
}

int pbb_item_world_get_var(const PBB_ItemWorld *world, int var_id)
{
    if (world == 0) {
        return 0;
    }

    if (!pbb_item_i_var_valid(var_id, PBB_ITEM_MAX_WORLD_VARS)) {
        return 0;
    }

    return world->world_vars[var_id];
}

int pbb_item_world_add_var(PBB_ItemWorld *world, int var_id, int delta)
{
    if (world == 0) {
        return 0;
    }

    if (!pbb_item_i_var_valid(var_id, PBB_ITEM_MAX_WORLD_VARS)) {
        return 0;
    }

    world->world_vars[var_id] += delta;
    return 1;
}

int pbb_item_world_set_flag(PBB_ItemWorld *world, int flag_id, int value)
{
    if (world == 0) {
        return 0;
    }

    return pbb_item_i_flag_set_words(world->world_flags, flag_id, value);
}

int pbb_item_world_get_flag(const PBB_ItemWorld *world, int flag_id)
{
    if (world == 0) {
        return 0;
    }

    return pbb_item_i_flag_get_words(world->world_flags, flag_id);
}

