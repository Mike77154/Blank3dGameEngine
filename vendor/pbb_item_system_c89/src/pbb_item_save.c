#include "pbb_item_internal.h"
#include <string.h>

void pbb_item_save_state_init(PBB_ItemSaveState *state)
{
    if (state == 0) {
        return;
    }

    memset(state, 0, sizeof(*state));
    state->magic = PBB_ITEM_SAVE_MAGIC;
    state->version_major = PBB_ITEM_VERSION_MAJOR;
    state->version_minor = PBB_ITEM_VERSION_MINOR;
    state->version_patch = PBB_ITEM_VERSION_PATCH;
}

int pbb_item_save_runtime(const PBB_ItemWorld *world, PBB_ItemSaveState *state)
{
    if (world == 0 || state == 0) {
        return 0;
    }

    pbb_item_save_state_init(state);
    memcpy(state->actors, world->actors, sizeof(world->actors));
    memcpy(state->items, world->items, sizeof(world->items));
    memcpy(state->world_vars, world->world_vars, sizeof(world->world_vars));
    memcpy(state->world_flags, world->world_flags, sizeof(world->world_flags));
    state->rng_state = world->rng_state;
    state->strict_droppable = world->strict_droppable;
    state->recycle_consumed_items = world->recycle_consumed_items;
    return 1;
}

int pbb_item_load_runtime(PBB_ItemWorld *world, const PBB_ItemSaveState *state)
{
    if (world == 0 || state == 0) {
        return 0;
    }

    if (state->magic != PBB_ITEM_SAVE_MAGIC) {
        return 0;
    }

    if (state->version_major != PBB_ITEM_VERSION_MAJOR) {
        return 0;
    }

    memcpy(world->actors, state->actors, sizeof(world->actors));
    memcpy(world->items, state->items, sizeof(world->items));
    memcpy(world->world_vars, state->world_vars, sizeof(world->world_vars));
    memcpy(world->world_flags, state->world_flags, sizeof(world->world_flags));
    world->rng_state = state->rng_state;
    if (world->rng_state == 0UL) {
        world->rng_state = 1UL;
    }
    world->strict_droppable = state->strict_droppable;
    world->recycle_consumed_items = state->recycle_consumed_items;
    pbb_item_event_clear(world);
    return 1;
}
