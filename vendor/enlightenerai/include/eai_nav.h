#ifndef EAI_NAV_H
#define EAI_NAV_H

#include "eai_context.h"

void       eai_nav_reset(EAI_Context* ctx);
EAI_NodeId eai_nav_add_node(EAI_Context* ctx, const EAI_Vec3* pos, EAI_ZoneId zone_id, EAI_U8 flags);
int        eai_nav_add_edge(EAI_Context* ctx, EAI_NodeId from, EAI_NodeId to, EAI_Fixed cost, EAI_U8 flags);
int        eai_nav_add_bidirectional_edge(EAI_Context* ctx, EAI_NodeId a, EAI_NodeId b, EAI_Fixed cost, EAI_U8 flags);
EAI_NodeId eai_nav_find_nearest_node(EAI_Context* ctx, const EAI_Vec3* pos, EAI_ZoneId zone_filter);
int        eai_nav_find_path(EAI_Context* ctx, EAI_EntityId entity_id, EAI_NodeId from, EAI_NodeId to, EAI_Path* out_path);
int        eai_nav_path_smooth(EAI_Context* ctx, EAI_EntityId entity_id, EAI_Path* path);
int        eai_nav_path_get_point(const EAI_Context* ctx, const EAI_Path* path, EAI_U16 index, EAI_Vec3* out_pos);
int        eai_nav_can_entity_use_node(const EAI_Context* ctx, EAI_EntityId entity_id, EAI_NodeId node_id);
EAI_Fixed      eai_nav_estimate_cost(const EAI_Context* ctx, EAI_NodeId a, EAI_NodeId b);

EAI_ZoneId eai_zone_create(EAI_Context* ctx, EAI_U8 flags);
void       eai_zone_set_flags(EAI_Context* ctx, EAI_ZoneId zone_id, EAI_U8 flags);
int        eai_zone_is_enabled(const EAI_Context* ctx, EAI_ZoneId zone_id);
void       eai_entity_set_zone_lock(EAI_Context* ctx, EAI_EntityId entity_id, EAI_ZoneId zone_id, int enabled);

#endif
