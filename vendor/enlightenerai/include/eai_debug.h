#ifndef EAI_DEBUG_H
#define EAI_DEBUG_H

#include "eai_context.h"

typedef struct EAI_DebugSummary
{
    EAI_U16 entities_used;
    EAI_U16 nodes_used;
    EAI_U16 edges_used;
    EAI_U16 zones_used;
    EAI_U16 queued_events;
    EAI_U16 active_stimuli;
} EAI_DebugSummary;

typedef struct EAI_DebugEntityInfo
{
    EAI_U8 used;
    EAI_U8 team;
    EAI_U8 flags;
    EAI_NodeId current_node;
    EAI_ZoneId zone_lock_id;
    EAI_EntityId target_id;
    EAI_Fixed alertness;
    EAI_Fixed suspicion;
    EAI_U8 action_type;
} EAI_DebugEntityInfo;

void eai_debug_get_summary(const EAI_Context* ctx, EAI_DebugSummary* out_summary);
int  eai_debug_get_entity_info(const EAI_Context* ctx, EAI_EntityId entity_id, EAI_DebugEntityInfo* out_info);

#endif
