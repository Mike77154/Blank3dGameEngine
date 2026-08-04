#ifndef EAI_WORLD_H
#define EAI_WORLD_H

#include "eai_types.h"

typedef int (*EAI_RaycastWorldFn)(void* user,
                                  const EAI_Vec3* from,
                                  const EAI_Vec3* to,
                                  EAI_U32 mask);

typedef int (*EAI_RaycastEntityFn)(void* user,
                                   EAI_EntityId self_id,
                                   EAI_EntityId other_id,
                                   const EAI_Vec3* from,
                                   const EAI_Vec3* to);

typedef EAI_Fixed (*EAI_EdgeCostFn)(void* user,
                                EAI_NodeId from,
                                EAI_NodeId to,
                                EAI_EntityId entity_id,
                                EAI_Fixed default_cost);

typedef struct EAI_WorldOps
{
    EAI_RaycastWorldFn  raycast_world;
    EAI_RaycastEntityFn raycast_entity;
    EAI_EdgeCostFn      edge_cost;
} EAI_WorldOps;

#endif
