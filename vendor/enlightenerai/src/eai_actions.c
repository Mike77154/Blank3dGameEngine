#include "eai_actions.h"
#include "eai_context.h"

static int eai_actions_valid_entity(const EAI_Context* ctx, EAI_EntityId entity_id)
{
    return (entity_id < EAI_MAX_ENTITIES && ctx->entities[entity_id].used != 0);
}

void eai_actions_clear_entity(EAI_Context* ctx, EAI_EntityId entity_id)
{
    EAI_ActionIntent* intent;

    if (!eai_actions_valid_entity(ctx, entity_id))
    {
        return;
    }

    intent = &ctx->intents[entity_id];
    intent->active = 0;
    intent->type = EAI_ACTION_NONE;
    intent->flags = 0;
    intent->target_id = EAI_INVALID_ID;
    intent->path.count = 0;
    intent->scalar = EAI_FX_FROM_RAW(0);
    intent->position.x = EAI_FX_FROM_RAW(0);
    intent->position.y = EAI_FX_FROM_RAW(0);
    intent->position.z = EAI_FX_FROM_RAW(0);
}

void eai_actions_clear_all(EAI_Context* ctx)
{
    EAI_U16 i;

    for (i = 0; i < EAI_MAX_ENTITIES; ++i)
    {
        ctx->intents[i].active = 0;
        ctx->intents[i].type = EAI_ACTION_NONE;
        ctx->intents[i].flags = 0;
        ctx->intents[i].target_id = EAI_INVALID_ID;
        ctx->intents[i].path.count = 0;
        ctx->intents[i].scalar = EAI_FX_FROM_RAW(0);
        ctx->intents[i].position.x = EAI_FX_FROM_RAW(0);
        ctx->intents[i].position.y = EAI_FX_FROM_RAW(0);
        ctx->intents[i].position.z = EAI_FX_FROM_RAW(0);
    }
}

void eai_actions_move_to(EAI_Context* ctx, EAI_EntityId entity_id, const EAI_Vec3* position)
{
    EAI_ActionIntent* intent;

    if (!eai_actions_valid_entity(ctx, entity_id))
    {
        return;
    }

    intent = &ctx->intents[entity_id];
    intent->active = 1;
    intent->type = EAI_ACTION_MOVE_TO;
    intent->target_id = EAI_INVALID_ID;
    intent->position = *position;
    intent->path.count = 0;
}

void eai_actions_follow_path(EAI_Context* ctx, EAI_EntityId entity_id, const EAI_Path* path)
{
    EAI_ActionIntent* intent;

    if (!eai_actions_valid_entity(ctx, entity_id))
    {
        return;
    }

    intent = &ctx->intents[entity_id];
    intent->active = 1;
    intent->type = EAI_ACTION_FOLLOW_PATH;
    intent->target_id = EAI_INVALID_ID;
    intent->path = *path;
}

void eai_actions_look_at(EAI_Context* ctx, EAI_EntityId entity_id, const EAI_Vec3* position)
{
    EAI_ActionIntent* intent;

    if (!eai_actions_valid_entity(ctx, entity_id))
    {
        return;
    }

    intent = &ctx->intents[entity_id];
    intent->active = 1;
    intent->type = EAI_ACTION_LOOK_AT;
    intent->target_id = EAI_INVALID_ID;
    intent->position = *position;
}

void eai_actions_fire_at(EAI_Context* ctx, EAI_EntityId entity_id, EAI_EntityId target_id)
{
    EAI_ActionIntent* intent;

    if (!eai_actions_valid_entity(ctx, entity_id))
    {
        return;
    }

    intent = &ctx->intents[entity_id];
    intent->active = 1;
    intent->type = EAI_ACTION_FIRE_AT;
    intent->target_id = target_id;
    intent->path.count = 0;
}

void eai_actions_stop(EAI_Context* ctx, EAI_EntityId entity_id)
{
    EAI_ActionIntent* intent;

    if (!eai_actions_valid_entity(ctx, entity_id))
    {
        return;
    }

    intent = &ctx->intents[entity_id];
    intent->active = 1;
    intent->type = EAI_ACTION_STOP;
    intent->target_id = EAI_INVALID_ID;
    intent->path.count = 0;
}

void eai_actions_investigate(EAI_Context* ctx, EAI_EntityId entity_id, const EAI_Vec3* position)
{
    EAI_ActionIntent* intent;

    if (!eai_actions_valid_entity(ctx, entity_id))
    {
        return;
    }

    intent = &ctx->intents[entity_id];
    intent->active = 1;
    intent->type = EAI_ACTION_INVESTIGATE;
    intent->target_id = EAI_INVALID_ID;
    intent->position = *position;
    intent->path.count = 0;
}

const EAI_ActionIntent* eai_actions_get(const EAI_Context* ctx, EAI_EntityId entity_id)
{
    if (!eai_actions_valid_entity(ctx, entity_id))
    {
        return 0;
    }
    return &ctx->intents[entity_id];
}
