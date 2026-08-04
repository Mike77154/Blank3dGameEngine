#ifndef EAI_ACTIONS_H
#define EAI_ACTIONS_H

#include "eai_types.h"

#define EAI_ACTION_NONE         0
#define EAI_ACTION_MOVE_TO      1
#define EAI_ACTION_FOLLOW_PATH  2
#define EAI_ACTION_LOOK_AT      3
#define EAI_ACTION_FIRE_AT      4
#define EAI_ACTION_STOP         5
#define EAI_ACTION_INVESTIGATE  6

typedef struct EAI_ActionIntent
{
    EAI_U8 active;
    EAI_U8 type;
    EAI_U16 flags;
    EAI_EntityId target_id;
    EAI_Vec3 position;
    EAI_Path path;
    EAI_Fixed scalar;
} EAI_ActionIntent;

#include "eai_fwd.h"

void eai_actions_clear_all(EAI_Context* ctx);
void eai_actions_clear_entity(EAI_Context* ctx, EAI_EntityId entity_id);
void eai_actions_move_to(EAI_Context* ctx, EAI_EntityId entity_id, const EAI_Vec3* position);
void eai_actions_follow_path(EAI_Context* ctx, EAI_EntityId entity_id, const EAI_Path* path);
void eai_actions_look_at(EAI_Context* ctx, EAI_EntityId entity_id, const EAI_Vec3* position);
void eai_actions_fire_at(EAI_Context* ctx, EAI_EntityId entity_id, EAI_EntityId target_id);
void eai_actions_stop(EAI_Context* ctx, EAI_EntityId entity_id);
void eai_actions_investigate(EAI_Context* ctx, EAI_EntityId entity_id, const EAI_Vec3* position);
const EAI_ActionIntent* eai_actions_get(const EAI_Context* ctx, EAI_EntityId entity_id);

#endif
