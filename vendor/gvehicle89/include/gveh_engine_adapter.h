#ifndef GVEH_ENGINE_ADAPTER_H
#define GVEH_ENGINE_ADAPTER_H

#include "gveh.h"
#include "gveh_entity_bridge.h"
#include "gveh_collision_bridge.h"

typedef struct gveh_vehicle_slot_s {
    gveh_i16 active;
    gveh_i16 id;
    gveh_i16 owner_id;
    gveh_vehicle vehicle;
    gveh_input input;
} gveh_vehicle_slot;

typedef struct gveh_vehicle_world_s {
    gveh_runtime runtime;
    gveh_vehicle_slot slots[GVEH_MAX_VEHICLES];
    gveh_entity_bridge_i entity_bridge;
    gveh_i16 next_id;
    gveh_i16 active_count;
    gveh_i16 collision_hits;
    vehicleprovider89_movement movement_provider;
    vehicleprovider89_physics physics_provider;
} gveh_vehicle_world;

void gveh_vehicle_world_init(gveh_vehicle_world *world);
void gveh_vehicle_world_set_world(gveh_vehicle_world *world, gveh_world_i world_i);
void gveh_vehicle_world_set_entity_bridge(gveh_vehicle_world *world, gveh_entity_bridge_i bridge);
void gveh_vehicle_world_set_movement_provider(
    gveh_vehicle_world *world, const vehicleprovider89_movement *provider);
void gveh_vehicle_world_set_physics_provider(
    gveh_vehicle_world *world, const vehicleprovider89_physics *provider);
void gveh_vehicle_world_clear_providers(gveh_vehicle_world *world);
gveh_i16 gveh_vehicle_world_spawn(gveh_vehicle_world *world, const gveh_profile *profile, gveh_vec3 pos, gveh_i16 owner_id);
gveh_i32 gveh_vehicle_world_despawn(gveh_vehicle_world *world, gveh_i16 id);
gveh_vehicle *gveh_vehicle_world_get(gveh_vehicle_world *world, gveh_i16 id);
gveh_input *gveh_vehicle_world_input(gveh_vehicle_world *world, gveh_i16 id);
void gveh_vehicle_world_clear_inputs(gveh_vehicle_world *world);
void gveh_vehicle_world_step(gveh_vehicle_world *world, gveh_fx dt);

#endif
