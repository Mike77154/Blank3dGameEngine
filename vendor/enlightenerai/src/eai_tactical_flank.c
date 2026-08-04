#include "eai_tactical.h"
#include "eai_nav.h"
#include "eai_math.h"

int eai_tactical_find_flank(EAI_Context* ctx,
                            EAI_EntityId self_id,
                            EAI_EntityId target_id,
                            EAI_NodeId* out_node)
{
    EAI_Entity* self;
    EAI_Entity* target;
    EAI_Vec3 target_to_self;
    EAI_Fixed best_score;
    EAI_NodeId best_node;
    EAI_U16 i;

    if (out_node == 0)
    {
        return EAI_FALSE;
    }
    *out_node = EAI_INVALID_ID;

    if (self_id >= EAI_MAX_ENTITIES || target_id >= EAI_MAX_ENTITIES)
    {
        return EAI_FALSE;
    }
    if (ctx->entities[self_id].used == 0 || ctx->entities[target_id].used == 0)
    {
        return EAI_FALSE;
    }

    self = &ctx->entities[self_id];
    target = &ctx->entities[target_id];
    eai_vec3_sub(&target_to_self, &self->pos, &target->pos);
    eai_vec3_normalize(&target_to_self, &target_to_self);

    best_score = -EAI_FX_MAX;
    best_node = EAI_INVALID_ID;

    for (i = 0; i < EAI_MAX_NAV_NODES; ++i)
    {
        EAI_Node* node;
        EAI_Vec3 target_to_node;
        EAI_Fixed target_dist;
        EAI_Fixed angle_score;
        EAI_Fixed move_cost;
        EAI_Fixed score;
        EAI_Path path;

        node = &ctx->nodes[i];
        if (node->used == 0)
        {
            continue;
        }
        if (!eai_nav_can_entity_use_node(ctx, self_id, i))
        {
            continue;
        }

        target_dist = eai_vec3_dist(&target->pos, &node->pos);
        if (target_dist > EAI_FLANK_SEARCH_RADIUS)
        {
            continue;
        }

        eai_vec3_sub(&target_to_node, &node->pos, &target->pos);
        eai_vec3_normalize(&target_to_node, &target_to_node);
        angle_score = eai_fx_sub_sat(EAI_FX_ONE, eai_vec3_dot(&target_to_self, &target_to_node));

        if (self->current_node == EAI_INVALID_ID)
        {
            continue;
        }

        if (!eai_nav_find_path(ctx, self_id, self->current_node, i, &path))
        {
            continue;
        }

        move_cost = eai_vec3_dist(&self->pos, &node->pos);
        score = eai_fx_sub_sat(eai_fx_mul(angle_score, EAI_FX_FROM_RAW(6553600)), move_cost);

        if ((node->flags & EAI_NODE_FLAG_COVER) != 0)
        {
            score = eai_fx_add_sat(score, EAI_FX_FROM_RAW(327680));
        }

        if (score > best_score)
        {
            best_score = score;
            best_node = i;
        }
    }

    if (best_node == EAI_INVALID_ID)
    {
        return EAI_FALSE;
    }

    *out_node = best_node;
    return EAI_TRUE;
}
