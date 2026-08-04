#ifndef GFACTION89_BRIDGE_H
#define GFACTION89_BRIDGE_H

#include "gfaction89.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*GFA_BridgeGetInt)(void *user, int entity_id, int field);
typedef void (*GFA_BridgeSetInt)(void *user, int entity_id, int field, int value);

typedef struct GFA_Bridge {
    GFA_BridgeGetInt get_int;
    GFA_BridgeSetInt set_int;
    void *user;
} GFA_Bridge;

#define GFA_BRIDGE_FIELD_FACTION  1
#define GFA_BRIDGE_FIELD_TEAM     2
#define GFA_BRIDGE_FIELD_ROLE     3
#define GFA_BRIDGE_FIELD_THREAT   4
#define GFA_BRIDGE_FIELD_MORALE   5
#define GFA_BRIDGE_FIELD_ALIVE    6

int gfa_bridge_pull_entity(GFA_World *world, const GFA_Bridge *bridge, int entity_id);
int gfa_bridge_push_entity(const GFA_World *world, const GFA_Bridge *bridge, int entity_id);

#ifdef __cplusplus
}
#endif

#endif
