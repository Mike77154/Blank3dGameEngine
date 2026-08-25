#include "gveh_entity_bridge.h"

void gveh_entity_state_from_vehicle(gveh_entity_state *out_state, gveh_i16 owner_id, const gveh_vehicle *v)
{
    out_state->owner_id = owner_id;
    out_state->pos = v->body.pos;
    out_state->vel = v->body.vel;
    out_state->yaw = v->body.yaw;
    out_state->pitch = v->body.pitch;
    out_state->roll = v->body.roll;
}

void gveh_entity_state_to_vehicle(gveh_vehicle *v, const gveh_entity_state *state)
{
    v->body.pos = state->pos;
    v->body.vel = state->vel;
    v->body.yaw = state->yaw;
    v->body.pitch = state->pitch;
    v->body.roll = state->roll;
}

gveh_i32 gveh_entity_bridge_pull(const gveh_entity_bridge_i *bridge, gveh_i16 owner_id, gveh_vehicle *v)
{
    gveh_entity_state st;
    if (bridge == 0) return 0;
    if (bridge->read_state == 0) return 0;
    if (!bridge->read_state(bridge->user, owner_id, &st)) return 0;
    gveh_entity_state_to_vehicle(v, &st);
    return 1;
}

void gveh_entity_bridge_push(const gveh_entity_bridge_i *bridge, gveh_i16 owner_id, const gveh_vehicle *v)
{
    gveh_entity_state st;
    if (bridge == 0) return;
    if (bridge->write_state == 0) return;
    gveh_entity_state_from_vehicle(&st, owner_id, v);
    bridge->write_state(bridge->user, owner_id, &st);
}
