#include "eai_memory.h"
#include "eai_math.h"

void eai_memory_clear_entity(EAI_Context* ctx, EAI_EntityId entity_id)
{
    EAI_EntityMemory* m;

    if (entity_id >= EAI_MAX_ENTITIES || ctx->entities[entity_id].used == 0)
    {
        return;
    }

    m = &ctx->entities[entity_id].memory;
    m->target_id = EAI_INVALID_ID;
    m->last_seen_entity = EAI_INVALID_ID;
    m->last_heard_source = EAI_INVALID_ID;
    eai_vec3_set(&m->last_seen_pos, EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0));
    eai_vec3_set(&m->last_heard_pos, EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0));
    m->alertness = EAI_FX_FROM_RAW(0);
    m->suspicion = EAI_FX_FROM_RAW(0);
    m->target_visible_time = EAI_FX_FROM_RAW(0);
    m->target_lost_time = EAI_FX_FROM_RAW(0);
    m->cover_node = EAI_INVALID_ID;
    m->home_zone = EAI_INVALID_ID;
}

void eai_memory_update(EAI_Context* ctx, EAI_Fixed dt)
{
    EAI_U16 i;

    for (i = 0; i < EAI_MAX_ENTITIES; ++i)
    {
        EAI_Entity* e;
        EAI_EntityMemory* m;
        EAI_EntityId t;

        e = &ctx->entities[i];
        if (e->used == 0)
        {
            continue;
        }

        m = &e->memory;
        m->alertness = eai_clampf(eai_fx_sub_sat(m->alertness, eai_fx_mul(EAI_ALERT_DECAY_PER_SECOND, dt)), EAI_FX_ZERO, EAI_FX_ONE);
        m->suspicion = eai_clampf(eai_fx_sub_sat(m->suspicion, eai_fx_mul(EAI_SUSPICION_DECAY_PER_SECOND, dt)), EAI_FX_ZERO, EAI_FX_ONE);

        t = m->target_id;
        if (t != EAI_INVALID_ID && t < EAI_MAX_ENTITIES && ctx->entities[t].used != 0)
        {
            if (ctx->visibility[i][t] != 0)
            {
                m->target_visible_time = eai_fx_add_sat(m->target_visible_time, dt);
                m->target_lost_time = EAI_FX_FROM_RAW(0);
                m->last_seen_entity = t;
                m->last_seen_pos = ctx->entities[t].pos;
            }
            else
            {
                m->target_visible_time = EAI_FX_FROM_RAW(0);
                m->target_lost_time = eai_fx_add_sat(m->target_lost_time, dt);
            }
        }
        else
        {
            m->target_visible_time = EAI_FX_FROM_RAW(0);
            m->target_lost_time = eai_fx_add_sat(m->target_lost_time, dt);
        }
    }
}
