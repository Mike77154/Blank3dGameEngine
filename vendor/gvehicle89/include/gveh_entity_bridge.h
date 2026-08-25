#ifndef GVEH_ENTITY_BRIDGE_H
#define GVEH_ENTITY_BRIDGE_H

#include "gveh.h"

typedef struct gveh_entity_state_s {
    gveh_i16 owner_id;
    gveh_vec3 pos;
    gveh_vec3 vel;
    gveh_i32 yaw;
    gveh_i32 pitch;
    gveh_i32 roll;
} gveh_entity_state;

typedef struct gveh_entity_bridge_i_s {
    void *user;
    gveh_i32 (*read_state)(void *user, gveh_i16 owner_id, gveh_entity_state *out_state);
    void (*write_state)(void *user, gveh_i16 owner_id, const gveh_entity_state *state);
} gveh_entity_bridge_i;

void gveh_entity_state_from_vehicle(gveh_entity_state *out_state, gveh_i16 owner_id, const gveh_vehicle *v);
void gveh_entity_state_to_vehicle(gveh_vehicle *v, const gveh_entity_state *state);
gveh_i32 gveh_entity_bridge_pull(const gveh_entity_bridge_i *bridge, gveh_i16 owner_id, gveh_vehicle *v);
void gveh_entity_bridge_push(const gveh_entity_bridge_i *bridge, gveh_i16 owner_id, const gveh_vehicle *v);

#endif
