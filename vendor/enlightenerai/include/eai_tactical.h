#ifndef EAI_TACTICAL_H
#define EAI_TACTICAL_H

#include "eai_context.h"

int eai_tactical_position_exposed(EAI_Context* ctx,
                                  const EAI_Vec3* pos,
                                  EAI_EntityId against_id);

int eai_tactical_find_cover(EAI_Context* ctx,
                            EAI_EntityId self_id,
                            EAI_EntityId threat_id,
                            EAI_NodeId* out_node);

int eai_tactical_find_flank(EAI_Context* ctx,
                            EAI_EntityId self_id,
                            EAI_EntityId target_id,
                            EAI_NodeId* out_node);

#endif
