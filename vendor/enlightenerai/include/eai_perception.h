#ifndef EAI_PERCEPTION_H
#define EAI_PERCEPTION_H

#include "eai_context.h"

#define EAI_STIMULUS_SOUND 1

void eai_perception_update(EAI_Context* ctx, EAI_Fixed dt);
int  eai_perception_can_see(EAI_Context* ctx, EAI_EntityId self_id, EAI_EntityId other_id);
int  eai_perception_emit_sound(EAI_Context* ctx,
                               const EAI_Vec3* pos,
                               EAI_Fixed radius,
                               EAI_Fixed strength,
                               EAI_EntityId source_id);
int  eai_perception_is_visible(const EAI_Context* ctx, EAI_EntityId self_id, EAI_EntityId other_id);

#endif
