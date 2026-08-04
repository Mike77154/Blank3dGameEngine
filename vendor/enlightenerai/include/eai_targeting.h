#ifndef EAI_TARGETING_H
#define EAI_TARGETING_H

#include "eai_context.h"

void         eai_targeting_set_hostile(EAI_Context* ctx, EAI_U8 team_a, EAI_U8 team_b, int hostile);
int          eai_targeting_is_hostile(const EAI_Context* ctx, EAI_U8 team_a, EAI_U8 team_b);
EAI_Fixed        eai_targeting_score(EAI_Context* ctx, EAI_EntityId self_id, EAI_EntityId other_id);
EAI_EntityId eai_targeting_select_best(EAI_Context* ctx, EAI_EntityId self_id);
void         eai_targeting_update(EAI_Context* ctx, EAI_Fixed dt);

#endif
