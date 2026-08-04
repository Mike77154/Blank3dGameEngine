#include "eai_core.h"
#include "eai_math.h"
#include "eai_nav.h"
#include "eai_perception.h"
#include "eai_memory.h"
#include "eai_targeting.h"

static void eai_zero_hostility(EAI_Context* ctx)
{
    EAI_U16 i;
    EAI_U16 j;

    for (i = 0; i < EAI_MAX_TEAMS; ++i)
    {
        for (j = 0; j < EAI_MAX_TEAMS; ++j)
        {
            ctx->hostility[i][j] = 0;
        }
    }
}

static void eai_zero_visibility(EAI_Context* ctx)
{
    EAI_U16 i;
    EAI_U16 j;

    for (i = 0; i < EAI_MAX_ENTITIES; ++i)
    {
        for (j = 0; j < EAI_MAX_ENTITIES; ++j)
        {
            ctx->visibility[i][j] = 0;
        }
    }
}

void eai_reset(EAI_Context* ctx)
{
    EAI_U16 i;

    ctx->entity_count = 0;
    eai_zero_hostility(ctx);
    eai_zero_visibility(ctx);
    eai_events_clear(ctx);
    eai_nav_reset(ctx);
    eai_actions_clear_all(ctx);

    for (i = 0; i < EAI_MAX_ENTITIES; ++i)
    {
        ctx->entities[i].used = 0;
        ctx->entities[i].team = 0;
        ctx->entities[i].flags = 0;
        ctx->entities[i].reserved = 0;
        eai_vec3_set(&ctx->entities[i].pos, EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0));
        eai_vec3_set(&ctx->entities[i].forward, EAI_FX_FROM_RAW(65536), EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0));
        ctx->entities[i].current_node = EAI_INVALID_ID;
        ctx->entities[i].zone_lock_id = EAI_INVALID_ID;
        ctx->entities[i].view_range = EAI_DEFAULT_VIEW_RANGE;
        ctx->entities[i].view_cos_half_fov = EAI_DEFAULT_VIEW_COS_HALF_FOV;
        ctx->entities[i].hearing_range = EAI_DEFAULT_HEARING_RANGE;
        eai_memory_clear_entity(ctx, i);
    }

    for (i = 0; i < EAI_MAX_STIMULI; ++i)
    {
        ctx->stimuli[i].active = 0;
    }
}

void eai_init(EAI_Context* ctx, const EAI_WorldOps* ops, void* world_user)
{
    if (ops != 0)
    {
        ctx->world_ops = *ops;
    }
    else
    {
        ctx->world_ops.raycast_world = 0;
        ctx->world_ops.raycast_entity = 0;
        ctx->world_ops.edge_cost = 0;
    }
    ctx->world_user = world_user;
    eai_reset(ctx);
}

EAI_EntityId eai_entity_create(EAI_Context* ctx)
{
    EAI_U16 i;

    for (i = 0; i < EAI_MAX_ENTITIES; ++i)
    {
        if (ctx->entities[i].used == 0)
        {
            ctx->entities[i].used = 1;
            ctx->entities[i].team = 0;
            ctx->entities[i].flags = (EAI_U8)(EAI_ENTITY_FLAG_CAN_SEE | EAI_ENTITY_FLAG_CAN_HEAR);
            ctx->entities[i].current_node = EAI_INVALID_ID;
            ctx->entities[i].zone_lock_id = EAI_INVALID_ID;
            ctx->entities[i].view_range = EAI_DEFAULT_VIEW_RANGE;
            ctx->entities[i].view_cos_half_fov = EAI_DEFAULT_VIEW_COS_HALF_FOV;
            ctx->entities[i].hearing_range = EAI_DEFAULT_HEARING_RANGE;
            eai_vec3_set(&ctx->entities[i].pos, EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0));
            eai_vec3_set(&ctx->entities[i].forward, EAI_FX_FROM_RAW(65536), EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0));
            eai_memory_clear_entity(ctx, i);
            eai_actions_clear_entity(ctx, i);
            ++ctx->entity_count;
            return i;
        }
    }

    return EAI_INVALID_ID;
}

void eai_entity_destroy(EAI_Context* ctx, EAI_EntityId id)
{
    EAI_U16 i;

    if (id >= EAI_MAX_ENTITIES || ctx->entities[id].used == 0)
    {
        return;
    }

    ctx->entities[id].used = 0;
    if (ctx->entity_count > 0)
    {
        --ctx->entity_count;
    }

    for (i = 0; i < EAI_MAX_ENTITIES; ++i)
    {
        ctx->visibility[id][i] = 0;
        ctx->visibility[i][id] = 0;
    }

    eai_actions_clear_entity(ctx, id);
}

void eai_entity_set_position(EAI_Context* ctx, EAI_EntityId id, const EAI_Vec3* pos)
{
    if (id >= EAI_MAX_ENTITIES || ctx->entities[id].used == 0)
    {
        return;
    }

    ctx->entities[id].pos = *pos;
    ctx->entities[id].current_node = eai_nav_find_nearest_node(ctx, pos, EAI_INVALID_ID);
}

void eai_entity_set_forward(EAI_Context* ctx, EAI_EntityId id, const EAI_Vec3* dir)
{
    if (id >= EAI_MAX_ENTITIES || ctx->entities[id].used == 0)
    {
        return;
    }

    eai_vec3_normalize(&ctx->entities[id].forward, dir);
}

void eai_entity_set_team(EAI_Context* ctx, EAI_EntityId id, EAI_U8 team)
{
    if (id >= EAI_MAX_ENTITIES || ctx->entities[id].used == 0)
    {
        return;
    }

    ctx->entities[id].team = team;
}

void eai_entity_set_view(EAI_Context* ctx, EAI_EntityId id, EAI_Fixed range, EAI_Fixed cos_half_fov)
{
    if (id >= EAI_MAX_ENTITIES || ctx->entities[id].used == 0)
    {
        return;
    }

    ctx->entities[id].view_range = range;
    ctx->entities[id].view_cos_half_fov = cos_half_fov;
}

void eai_entity_set_hearing(EAI_Context* ctx, EAI_EntityId id, EAI_Fixed range)
{
    if (id >= EAI_MAX_ENTITIES || ctx->entities[id].used == 0)
    {
        return;
    }

    ctx->entities[id].hearing_range = range;
}

void eai_update(EAI_Context* ctx, EAI_Fixed dt)
{
    eai_perception_update(ctx, dt);
    eai_memory_update(ctx, dt);
    eai_targeting_update(ctx, dt);
}
