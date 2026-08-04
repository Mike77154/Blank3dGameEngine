#include "eai_perception.h"
#include "eai_nav.h"
#include "eai_math.h"

static int eai_perception_valid_entity(const EAI_Context* ctx, EAI_EntityId id)
{
    return (id < EAI_MAX_ENTITIES && ctx->entities[id].used != 0 &&
            (ctx->entities[id].flags & EAI_ENTITY_FLAG_DISABLED) == 0);
}

static void eai_perception_push_seen_event(EAI_Context* ctx,
                                           EAI_U8 type,
                                           EAI_EntityId self_id,
                                           EAI_EntityId other_id,
                                           const EAI_Vec3* pos,
                                           EAI_Fixed value)
{
    EAI_Event ev;

    ev.type = type;
    ev.self_id = self_id;
    ev.other_id = other_id;
    ev.position = *pos;
    ev.value = value;
    eai_events_push(ctx, &ev);
}

int eai_perception_can_see(EAI_Context* ctx, EAI_EntityId self_id, EAI_EntityId other_id)
{
    EAI_Entity* self;
    EAI_Entity* other;
    EAI_Vec3 dir;
    EAI_Vec3 self_forward;
    EAI_Fixed dist_sq;
    EAI_Fixed view_range_sq;
    EAI_Fixed facing;

    if (!eai_perception_valid_entity(ctx, self_id) ||
        !eai_perception_valid_entity(ctx, other_id) ||
        self_id == other_id)
    {
        return EAI_FALSE;
    }

    self = &ctx->entities[self_id];
    other = &ctx->entities[other_id];

    if ((self->flags & EAI_ENTITY_FLAG_CAN_SEE) == 0)
    {
        return EAI_FALSE;
    }

    if ((self->flags & EAI_ENTITY_FLAG_LOCK_ZONE) != 0 &&
        self->zone_lock_id != EAI_INVALID_ID &&
        other->current_node != EAI_INVALID_ID &&
        ctx->nodes[other->current_node].zone_id != EAI_INVALID_ID &&
        ctx->nodes[other->current_node].zone_id != self->zone_lock_id)
    {
        return EAI_FALSE;
    }

    dist_sq = eai_vec3_dist_sq(&self->pos, &other->pos);
    view_range_sq = eai_fx_mul(self->view_range, self->view_range);
    if (dist_sq > view_range_sq)
    {
        return EAI_FALSE;
    }

    eai_vec3_sub(&dir, &other->pos, &self->pos);
    eai_vec3_normalize(&dir, &dir);
    eai_vec3_normalize(&self_forward, &self->forward);
    facing = eai_vec3_dot(&self_forward, &dir);
    if (facing < self->view_cos_half_fov)
    {
        return EAI_FALSE;
    }

    if (ctx->world_ops.raycast_world != 0)
    {
        if (ctx->world_ops.raycast_world(ctx->world_user, &self->pos, &other->pos, 0u) != 0)
        {
            return EAI_FALSE;
        }
    }

    if (ctx->world_ops.raycast_entity != 0)
    {
        if (ctx->world_ops.raycast_entity(ctx->world_user, self_id, other_id, &self->pos, &other->pos) != 0)
        {
            return EAI_FALSE;
        }
    }

    return EAI_TRUE;
}

int eai_perception_is_visible(const EAI_Context* ctx, EAI_EntityId self_id, EAI_EntityId other_id)
{
    if (self_id >= EAI_MAX_ENTITIES || other_id >= EAI_MAX_ENTITIES)
    {
        return EAI_FALSE;
    }
    return ctx->visibility[self_id][other_id] != 0;
}

static void eai_perception_update_vision(EAI_Context* ctx)
{
    EAI_U16 i;
    EAI_U16 j;

    for (i = 0; i < EAI_MAX_ENTITIES; ++i)
    {
        if (!eai_perception_valid_entity(ctx, i))
        {
            continue;
        }

        for (j = 0; j < EAI_MAX_ENTITIES; ++j)
        {
            int now_visible;
            int prev_visible;

            if (i == j || !eai_perception_valid_entity(ctx, j))
            {
                ctx->visibility[i][j] = 0;
                continue;
            }

            prev_visible = ctx->visibility[i][j];
            now_visible = eai_perception_can_see(ctx, i, j);
            ctx->visibility[i][j] = (EAI_U8)now_visible;

            if (now_visible)
            {
                ctx->entities[i].memory.last_seen_entity = j;
                ctx->entities[i].memory.last_seen_pos = ctx->entities[j].pos;
                ctx->entities[i].memory.alertness = eai_clampf(eai_fx_add_sat(ctx->entities[i].memory.alertness, EAI_FX_FROM_RAW(26214)), EAI_FX_ZERO, EAI_FX_ONE);
                ctx->entities[i].memory.suspicion = eai_clampf(eai_fx_add_sat(ctx->entities[i].memory.suspicion, EAI_FX_FROM_RAW(13107)), EAI_FX_ZERO, EAI_FX_ONE);

                if (!prev_visible)
                {
                    eai_perception_push_seen_event(ctx,
                                                   EAI_EVENT_SEE_TARGET,
                                                   i,
                                                   j,
                                                   &ctx->entities[j].pos,
                                                   EAI_FX_ONE);
                }
            }
            else
            {
                if (prev_visible)
                {
                    eai_perception_push_seen_event(ctx,
                                                   EAI_EVENT_LOSE_TARGET,
                                                   i,
                                                   j,
                                                   &ctx->entities[j].pos,
                                                   EAI_FX_ZERO);
                }
            }
        }
    }
}

void eai_perception_update(EAI_Context* ctx, EAI_Fixed dt)
{
    EAI_U16 i;

    for (i = 0; i < EAI_MAX_ENTITIES; ++i)
    {
        if (ctx->entities[i].used != 0)
        {
            ctx->entities[i].current_node =
                eai_nav_find_nearest_node(ctx, &ctx->entities[i].pos, EAI_INVALID_ID);
        }
    }

    eai_perception_update_vision(ctx);
#if EAI_ENABLE_HEARING
    {
        extern void eai_perception_update_hearing(EAI_Context* ctx, EAI_Fixed dt);
        eai_perception_update_hearing(ctx, dt);
    }
#else
    (void)dt;
#endif
}
