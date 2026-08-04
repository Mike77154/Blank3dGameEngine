#include "eai_debug.h"

void eai_debug_get_summary(const EAI_Context* ctx, EAI_DebugSummary* out_summary)
{
    EAI_U16 i;
    EAI_U16 active_stimuli;

    if (out_summary == 0)
    {
        return;
    }

    active_stimuli = 0;
    for (i = 0; i < EAI_MAX_STIMULI; ++i)
    {
        if (ctx->stimuli[i].active != 0)
        {
            ++active_stimuli;
        }
    }

    out_summary->entities_used = ctx->entity_count;
    out_summary->nodes_used = ctx->node_count;
    out_summary->edges_used = ctx->edge_count;
    out_summary->zones_used = ctx->zone_count;
    out_summary->queued_events = (EAI_U16)eai_events_count(ctx);
    out_summary->active_stimuli = active_stimuli;
}

int eai_debug_get_entity_info(const EAI_Context* ctx, EAI_EntityId entity_id, EAI_DebugEntityInfo* out_info)
{
    const EAI_Entity* e;

    if (out_info == 0)
    {
        return EAI_FALSE;
    }
    if (entity_id >= EAI_MAX_ENTITIES || ctx->entities[entity_id].used == 0)
    {
        return EAI_FALSE;
    }

    e = &ctx->entities[entity_id];
    out_info->used = e->used;
    out_info->team = e->team;
    out_info->flags = e->flags;
    out_info->current_node = e->current_node;
    out_info->zone_lock_id = e->zone_lock_id;
    out_info->target_id = e->memory.target_id;
    out_info->alertness = e->memory.alertness;
    out_info->suspicion = e->memory.suspicion;
    out_info->action_type = ctx->intents[entity_id].type;
    return EAI_TRUE;
}
