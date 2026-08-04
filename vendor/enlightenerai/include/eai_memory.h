#ifndef EAI_MEMORY_H
#define EAI_MEMORY_H

#include "eai_context.h"

void eai_memory_clear_entity(EAI_Context* ctx, EAI_EntityId entity_id);
void eai_memory_update(EAI_Context* ctx, EAI_Fixed dt);

#endif
