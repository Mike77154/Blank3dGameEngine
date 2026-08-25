#include "gveh_engine_adapter.h"
#include <string.h>

static gveh_i16 gveh_vehicle_world_find_slot(gveh_vehicle_world *world, gveh_i16 id)
{
    gveh_i16 i;
    i = 0;
    while (i < GVEH_MAX_VEHICLES) {
        if (world->slots[i].active != 0 && world->slots[i].id == id) return i;
        i++;
    }
    return -1;
}

void gveh_vehicle_world_init(gveh_vehicle_world *world)
{
    gveh_i16 i;
    memset(world, 0, sizeof(*world));
    gveh_runtime_init(&world->runtime);
    vehicleprovider89_movement_clear(&world->movement_provider);
    vehicleprovider89_physics_clear(&world->physics_provider);
    world->next_id = 1;
    i = 0;
    while (i < GVEH_MAX_VEHICLES) {
        world->slots[i].active = 0;
        world->slots[i].id = 0;
        world->slots[i].owner_id = -1;
        gveh_input_clear(&world->slots[i].input);
        i++;
    }
}

void gveh_vehicle_world_set_world(gveh_vehicle_world *world, gveh_world_i world_i)
{
    gveh_runtime_set_world(&world->runtime, world_i);
}

void gveh_vehicle_world_set_entity_bridge(gveh_vehicle_world *world, gveh_entity_bridge_i bridge)
{
    world->entity_bridge = bridge;
}

void gveh_vehicle_world_set_movement_provider(
    gveh_vehicle_world *world, const vehicleprovider89_movement *provider)
{
    gveh_i16 i;
    if (!world) return;
    if (provider) world->movement_provider = *provider;
    else vehicleprovider89_movement_clear(&world->movement_provider);
    i = 0;
    while (i < GVEH_MAX_VEHICLES) {
        if (world->slots[i].active != 0)
            gveh_vehicle_set_movement_provider(&world->slots[i].vehicle, provider);
        i++;
    }
}

void gveh_vehicle_world_set_physics_provider(
    gveh_vehicle_world *world, const vehicleprovider89_physics *provider)
{
    gveh_i16 i;
    if (!world) return;
    if (provider) world->physics_provider = *provider;
    else vehicleprovider89_physics_clear(&world->physics_provider);
    i = 0;
    while (i < GVEH_MAX_VEHICLES) {
        if (world->slots[i].active != 0)
            gveh_vehicle_set_physics_provider(&world->slots[i].vehicle, provider);
        i++;
    }
}

void gveh_vehicle_world_clear_providers(gveh_vehicle_world *world)
{
    if (!world) return;
    gveh_vehicle_world_set_movement_provider(world, 0);
    gveh_vehicle_world_set_physics_provider(world, 0);
}

gveh_i16 gveh_vehicle_world_spawn(gveh_vehicle_world *world, const gveh_profile *profile, gveh_vec3 pos, gveh_i16 owner_id)
{
    gveh_i16 i;
    gveh_i16 id;
    i = 0;
    while (i < GVEH_MAX_VEHICLES) {
        if (world->slots[i].active == 0) {
            id = world->next_id;
            world->next_id++;
            if (world->next_id <= 0) world->next_id = 1;
            world->slots[i].active = 1;
            world->slots[i].id = id;
            world->slots[i].owner_id = owner_id;
            gveh_vehicle_init(&world->slots[i].vehicle, profile, pos);
            gveh_vehicle_set_movement_provider(&world->slots[i].vehicle,
                world->movement_provider.step ? &world->movement_provider : 0);
            gveh_vehicle_set_physics_provider(&world->slots[i].vehicle,
                world->physics_provider.step ? &world->physics_provider : 0);
            gveh_input_clear(&world->slots[i].input);
            world->active_count++;
            gveh_entity_bridge_push(&world->entity_bridge, owner_id, &world->slots[i].vehicle);
            return id;
        }
        i++;
    }
    return -1;
}

gveh_i32 gveh_vehicle_world_despawn(gveh_vehicle_world *world, gveh_i16 id)
{
    gveh_i16 slot;
    slot = gveh_vehicle_world_find_slot(world, id);
    if (slot < 0) return 0;
    world->slots[slot].active = 0;
    world->slots[slot].id = 0;
    world->slots[slot].owner_id = -1;
    gveh_input_clear(&world->slots[slot].input);
    if (world->active_count > 0) world->active_count--;
    return 1;
}

gveh_vehicle *gveh_vehicle_world_get(gveh_vehicle_world *world, gveh_i16 id)
{
    gveh_i16 slot;
    slot = gveh_vehicle_world_find_slot(world, id);
    if (slot < 0) return 0;
    return &world->slots[slot].vehicle;
}

gveh_input *gveh_vehicle_world_input(gveh_vehicle_world *world, gveh_i16 id)
{
    gveh_i16 slot;
    slot = gveh_vehicle_world_find_slot(world, id);
    if (slot < 0) return 0;
    return &world->slots[slot].input;
}

void gveh_vehicle_world_clear_inputs(gveh_vehicle_world *world)
{
    gveh_i16 i;
    i = 0;
    while (i < GVEH_MAX_VEHICLES) {
        if (world->slots[i].active != 0) gveh_input_clear(&world->slots[i].input);
        i++;
    }
}

void gveh_vehicle_world_step(gveh_vehicle_world *world, gveh_fx dt)
{
    gveh_i16 i;
    gveh_i16 j;
    gveh_i16 iter;
    gveh_i32 frame_tick;
    gveh_collision_contact contact;

    frame_tick = world->runtime.tick;
    world->collision_hits = 0;
    i = 0;
    while (i < GVEH_MAX_VEHICLES) {
        if (world->slots[i].active != 0) {
            gveh_entity_bridge_pull(&world->entity_bridge, world->slots[i].owner_id, &world->slots[i].vehicle);
            world->runtime.tick = frame_tick;
            gveh_vehicle_step(&world->runtime, &world->slots[i].vehicle, &world->slots[i].input, dt);
        }
        i++;
    }

    iter = 0;
    while (iter < GVEH_COLLISION_ITERATIONS) {
        i = 0;
        while (i < GVEH_MAX_VEHICLES) {
            if (world->slots[i].active != 0) {
                j = (gveh_i16)(i + 1);
                while (j < GVEH_MAX_VEHICLES) {
                    if (world->slots[j].active != 0) {
                        if (gveh_collision_resolve_pair(&world->slots[i].vehicle, &world->slots[j].vehicle, world->slots[i].id, world->slots[j].id, &contact)) {
                            world->collision_hits++;
                        }
                    }
                    j++;
                }
            }
            i++;
        }
        iter++;
    }

    i = 0;
    while (i < GVEH_MAX_VEHICLES) {
        if (world->slots[i].active != 0) {
            gveh_entity_bridge_push(&world->entity_bridge, world->slots[i].owner_id, &world->slots[i].vehicle);
        }
        i++;
    }
    world->runtime.tick = frame_tick + 1;
}
