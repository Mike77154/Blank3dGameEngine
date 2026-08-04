#ifndef EAI_CORE_H
#define EAI_CORE_H

#include "eai_context.h"

void eai_init(EAI_Context* ctx, const EAI_WorldOps* ops, void* world_user);
void eai_reset(EAI_Context* ctx);
void eai_update(EAI_Context* ctx, EAI_Fixed dt);

EAI_EntityId eai_entity_create(EAI_Context* ctx);
void         eai_entity_destroy(EAI_Context* ctx, EAI_EntityId id);

void eai_entity_set_position(EAI_Context* ctx, EAI_EntityId id, const EAI_Vec3* pos);
void eai_entity_set_forward(EAI_Context* ctx, EAI_EntityId id, const EAI_Vec3* dir);
void eai_entity_set_team(EAI_Context* ctx, EAI_EntityId id, EAI_U8 team);
void eai_entity_set_view(EAI_Context* ctx, EAI_EntityId id, EAI_Fixed range, EAI_Fixed cos_half_fov);
void eai_entity_set_hearing(EAI_Context* ctx, EAI_EntityId id, EAI_Fixed range);

#endif
