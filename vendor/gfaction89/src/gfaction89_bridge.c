#include "gfaction89_bridge.h"

int gfa_bridge_pull_entity(GFA_World *world, const GFA_Bridge *bridge, int entity_id)
{
    GFA_Entity *e;
    if (!world || !bridge || !bridge->get_int) return GFA_FALSE;
    if (!gfa_ensure_entity(world, entity_id)) return GFA_FALSE;
    e = gfa_get_entity(world, entity_id);
    if (!e) return GFA_FALSE;
    e->faction_id = bridge->get_int(bridge->user, entity_id, GFA_BRIDGE_FIELD_FACTION);
    e->team_id = bridge->get_int(bridge->user, entity_id, GFA_BRIDGE_FIELD_TEAM);
    e->role_id = bridge->get_int(bridge->user, entity_id, GFA_BRIDGE_FIELD_ROLE);
    e->threat = bridge->get_int(bridge->user, entity_id, GFA_BRIDGE_FIELD_THREAT);
    e->morale = bridge->get_int(bridge->user, entity_id, GFA_BRIDGE_FIELD_MORALE);
    e->alive = bridge->get_int(bridge->user, entity_id, GFA_BRIDGE_FIELD_ALIVE);
    return GFA_TRUE;
}

int gfa_bridge_push_entity(const GFA_World *world, const GFA_Bridge *bridge, int entity_id)
{
    const GFA_Entity *e;
    if (!world || !bridge || !bridge->set_int) return GFA_FALSE;
    e = gfa_get_entity_const(world, entity_id);
    if (!e) return GFA_FALSE;
    bridge->set_int(bridge->user, entity_id, GFA_BRIDGE_FIELD_FACTION, e->faction_id);
    bridge->set_int(bridge->user, entity_id, GFA_BRIDGE_FIELD_TEAM, e->team_id);
    bridge->set_int(bridge->user, entity_id, GFA_BRIDGE_FIELD_ROLE, e->role_id);
    bridge->set_int(bridge->user, entity_id, GFA_BRIDGE_FIELD_THREAT, e->threat);
    bridge->set_int(bridge->user, entity_id, GFA_BRIDGE_FIELD_MORALE, e->morale);
    bridge->set_int(bridge->user, entity_id, GFA_BRIDGE_FIELD_ALIVE, e->alive);
    return GFA_TRUE;
}
