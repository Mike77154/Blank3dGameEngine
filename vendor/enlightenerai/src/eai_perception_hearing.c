#include "eai_perception.h"
#include "eai_math.h"

static void eai_stimulus_clear(EAI_Stimulus* s)
{
    EAI_U16 i;
    s->active = 0;
    s->kind = 0;
    s->source_id = EAI_INVALID_ID;
    eai_vec3_set(&s->pos, EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0));
    s->radius = EAI_FX_FROM_RAW(0);
    s->strength = EAI_FX_FROM_RAW(0);
    s->age = EAI_FX_FROM_RAW(0);
    for (i = 0; i < EAI_SOUND_MASK_WORDS; ++i)
    {
        s->heard_mask[i] = 0u;
    }
}

static int eai_stimulus_has_heard(const EAI_Stimulus* s, EAI_EntityId entity_id)
{
    EAI_U16 word_index;
    EAI_U16 bit_index;
    word_index = (EAI_U16)(entity_id / 32u);
    bit_index = (EAI_U16)(entity_id % 32u);
    return (s->heard_mask[word_index] & (1uL << bit_index)) != 0u;
}

static void eai_stimulus_mark_heard(EAI_Stimulus* s, EAI_EntityId entity_id)
{
    EAI_U16 word_index;
    EAI_U16 bit_index;
    word_index = (EAI_U16)(entity_id / 32u);
    bit_index = (EAI_U16)(entity_id % 32u);
    s->heard_mask[word_index] |= (1uL << bit_index);
}

int eai_perception_emit_sound(EAI_Context* ctx,
                              const EAI_Vec3* pos,
                              EAI_Fixed radius,
                              EAI_Fixed strength,
                              EAI_EntityId source_id)
{
    EAI_U16 i;

    for (i = 0; i < EAI_MAX_STIMULI; ++i)
    {
        if (ctx->stimuli[i].active == 0)
        {
            EAI_Stimulus* s;
            s = &ctx->stimuli[i];
            eai_stimulus_clear(s);
            s->active = 1;
            s->kind = EAI_STIMULUS_SOUND;
            s->source_id = source_id;
            s->pos = *pos;
            s->radius = radius;
            s->strength = strength;
            s->age = EAI_FX_FROM_RAW(0);
            return EAI_TRUE;
        }
    }

    return EAI_FALSE;
}

void eai_perception_update_hearing(EAI_Context* ctx, EAI_Fixed dt)
{
    EAI_U16 i;

    for (i = 0; i < EAI_MAX_STIMULI; ++i)
    {
        EAI_Stimulus* s;

        s = &ctx->stimuli[i];
        if (s->active == 0)
        {
            continue;
        }

        s->age = eai_fx_add_sat(s->age, dt);
        if (s->age > EAI_SOUND_LIFETIME)
        {
            eai_stimulus_clear(s);
            continue;
        }

        {
            EAI_U16 entity_index;
            for (entity_index = 0; entity_index < EAI_MAX_ENTITIES; ++entity_index)
            {
                EAI_Entity* e;
                EAI_Fixed max_dist;

                e = &ctx->entities[entity_index];
                if (e->used == 0 || (e->flags & EAI_ENTITY_FLAG_CAN_HEAR) == 0)
                {
                    continue;
                }
                if (eai_stimulus_has_heard(s, entity_index))
                {
                    continue;
                }

                max_dist = eai_fx_add_sat(e->hearing_range, s->radius);
                if (eai_vec3_dist_sq(&e->pos, &s->pos) <= eai_fx_mul(max_dist, max_dist))
                {
                    int blocked;

                    blocked = EAI_FALSE;
                    if (ctx->world_ops.raycast_world != 0)
                    {
                        blocked = ctx->world_ops.raycast_world(ctx->world_user, &e->pos, &s->pos, 0u);
                    }

                    if (!blocked)
                    {
                        EAI_Event ev;

                        eai_stimulus_mark_heard(s, entity_index);
                        e->memory.last_heard_source = s->source_id;
                        e->memory.last_heard_pos = s->pos;
                        e->memory.alertness = eai_clampf(eai_fx_madd_sat(e->memory.alertness, EAI_FX_FROM_RAW(9830), s->strength), EAI_FX_ZERO, EAI_FX_ONE);
                        e->memory.suspicion = eai_clampf(eai_fx_madd_sat(e->memory.suspicion, EAI_FX_FROM_RAW(22938), s->strength), EAI_FX_ZERO, EAI_FX_ONE);

                        ev.type = EAI_EVENT_HEAR_SOUND;
                        ev.self_id = entity_index;
                        ev.other_id = s->source_id;
                        ev.position = s->pos;
                        ev.value = s->strength;
                        eai_events_push(ctx, &ev);
                    }
                }
            }
        }
    }
}
