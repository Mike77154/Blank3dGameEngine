#include "eai_targeting.h"
#include "eai_math.h"

void eai_targeting_set_hostile(EAI_Context* ctx, EAI_U8 team_a, EAI_U8 team_b, int hostile)
{
    if (team_a >= EAI_MAX_TEAMS || team_b >= EAI_MAX_TEAMS)
    {
        return;
    }
    ctx->hostility[team_a][team_b] = hostile ? 1u : 0u;
}

int eai_targeting_is_hostile(const EAI_Context* ctx, EAI_U8 team_a, EAI_U8 team_b)
{
    if (team_a >= EAI_MAX_TEAMS || team_b >= EAI_MAX_TEAMS)
    {
        return EAI_FALSE;
    }
    return ctx->hostility[team_a][team_b] != 0;
}

EAI_Fixed eai_targeting_score(EAI_Context* ctx, EAI_EntityId self_id, EAI_EntityId other_id)
{
    EAI_Entity* self;
    EAI_Entity* other;
    EAI_Fixed dist;
    EAI_Fixed score;

    if (self_id >= EAI_MAX_ENTITIES || other_id >= EAI_MAX_ENTITIES)
    {
        return -EAI_FX_MAX;
    }

    self = &ctx->entities[self_id];
    other = &ctx->entities[other_id];
    if (self->used == 0 || other->used == 0 || self_id == other_id)
    {
        return -EAI_FX_MAX;
    }

    if (!eai_targeting_is_hostile(ctx, self->team, other->team))
    {
        return -EAI_FX_MAX;
    }

    dist = eai_vec3_dist(&self->pos, &other->pos);
    score = EAI_FX_FROM_RAW(0);

    if (ctx->visibility[self_id][other_id] != 0)
    {
        score = eai_fx_add_sat(score, EAI_FX_FROM_RAW(65536000));
    }
    else if (self->memory.last_seen_entity == other_id &&
             self->memory.target_lost_time < EAI_INVESTIGATE_MEMORY_TIME)
    {
        score = eai_fx_add_sat(score, eai_fx_sub_sat(EAI_FX_FROM_RAW(7864320), eai_fx_mul(self->memory.target_lost_time, EAI_FX_FROM_RAW(1310720))));
    }

    score = eai_fx_add_sat(score, eai_fx_div(EAI_FX_FROM_RAW(16384000), eai_fx_add_sat(EAI_FX_ONE, dist)));

    if (self->memory.target_id == other_id)
    {
        score = eai_fx_add_sat(score, EAI_FX_FROM_RAW(5242880));
    }

    score = eai_fx_madd_sat(score, self->memory.alertness, EAI_FX_FROM_RAW(3276800));
    return score;
}

EAI_EntityId eai_targeting_select_best(EAI_Context* ctx, EAI_EntityId self_id)
{
    EAI_Fixed best_score;
    EAI_EntityId best_id;
    EAI_U16 i;

    if (self_id >= EAI_MAX_ENTITIES || ctx->entities[self_id].used == 0)
    {
        return EAI_INVALID_ID;
    }

    best_score = -EAI_FX_MAX;
    best_id = EAI_INVALID_ID;

    for (i = 0; i < EAI_MAX_ENTITIES; ++i)
    {
        EAI_Fixed score;
        score = eai_targeting_score(ctx, self_id, i);
        if (score > best_score)
        {
            best_score = score;
            best_id = i;
        }
    }

    if (best_score < EAI_FX_ZERO)
    {
        return EAI_INVALID_ID;
    }

    return best_id;
}

void eai_targeting_update(EAI_Context* ctx, EAI_Fixed dt)
{
    EAI_U16 i;
    (void)dt;

    for (i = 0; i < EAI_MAX_ENTITIES; ++i)
    {
        EAI_Entity* e;
        EAI_EntityId best;
        EAI_EntityId old;

        e = &ctx->entities[i];
        if (e->used == 0)
        {
            continue;
        }

        old = e->memory.target_id;
        best = eai_targeting_select_best(ctx, i);

        if (best != EAI_INVALID_ID)
        {
            e->memory.target_id = best;
            if (old != best)
            {
                EAI_Event ev;
                ev.type = EAI_EVENT_TARGET_ACQUIRED;
                ev.self_id = i;
                ev.other_id = best;
                ev.position = ctx->entities[best].pos;
                ev.value = EAI_FX_ONE;
                eai_events_push(ctx, &ev);
            }
        }
        else
        {
            if (old != EAI_INVALID_ID && e->memory.target_lost_time > EAI_TARGET_FORGET_TIME)
            {
                EAI_Event ev2;
                ev2.type = EAI_EVENT_TARGET_LOST;
                ev2.self_id = i;
                ev2.other_id = old;
                ev2.position = e->memory.last_seen_pos;
                ev2.value = EAI_FX_ZERO;
                eai_events_push(ctx, &ev2);
                e->memory.target_id = EAI_INVALID_ID;
            }
        }
    }
}
