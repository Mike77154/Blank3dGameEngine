#include "eai_tactical.h"
#include "eai_nav.h"
#include "eai_math.h"

static int eai_tactical_cover_pass(EAI_Context* ctx,
                                   EAI_EntityId self_id,
                                   EAI_EntityId threat_id,
                                   int require_cover_flag,
                                   EAI_NodeId* out_best_node,
                                   EAI_Fixed* out_best_score)
{
    EAI_Entity* self;
    EAI_U16 i;

    self = &ctx->entities[self_id];

    for (i = 0; i < EAI_MAX_NAV_NODES; ++i)
    {
        EAI_Path path;
        EAI_Fixed score;
        EAI_Fixed dist_to_self;
        EAI_Node* node;

        node = &ctx->nodes[i];
        if (node->used == 0)
        {
            continue;
        }
        if (!eai_nav_can_entity_use_node(ctx, self_id, i))
        {
            continue;
        }
        if (require_cover_flag && (node->flags & EAI_NODE_FLAG_COVER) == 0)
        {
            continue;
        }

        dist_to_self = eai_vec3_dist(&self->pos, &node->pos);
        if (dist_to_self > EAI_COVER_SEARCH_RADIUS)
        {
            continue;
        }

        if (eai_tactical_position_exposed(ctx, &node->pos, threat_id))
        {
            continue;
        }

        if (self->current_node == EAI_INVALID_ID)
        {
            continue;
        }

        if (!eai_nav_find_path(ctx, self_id, self->current_node, i, &path))
        {
            continue;
        }

        score = dist_to_self;
        if ((node->flags & EAI_NODE_FLAG_COVER) != 0)
        {
            score = eai_fx_sub_sat(score, EAI_FX_FROM_RAW(131072));
        }
        if (i == self->current_node)
        {
            score = eai_fx_add_sat(score, EAI_FX_FROM_RAW(65536));
        }
        score = eai_fx_add_sat(score, eai_fx_mul(eai_fx_from_int((EAI_Fixed)path.count), EAI_FX_FROM_RAW(9830)));

        if (*out_best_node == EAI_INVALID_ID || score < *out_best_score)
        {
            *out_best_score = score;
            *out_best_node = i;
        }
    }

    return (*out_best_node != EAI_INVALID_ID);
}

int eai_tactical_position_exposed(EAI_Context* ctx,
                                  const EAI_Vec3* pos,
                                  EAI_EntityId against_id)
{
    EAI_Entity* against;

    if (against_id >= EAI_MAX_ENTITIES || ctx->entities[against_id].used == 0)
    {
        return EAI_FALSE;
    }

    against = &ctx->entities[against_id];

    if (ctx->world_ops.raycast_world != 0)
    {
        if (ctx->world_ops.raycast_world(ctx->world_user, pos, &against->pos, 0u) != 0)
        {
            return EAI_FALSE;
        }
    }

    return EAI_TRUE;
}

int eai_tactical_find_cover(EAI_Context* ctx,
                            EAI_EntityId self_id,
                            EAI_EntityId threat_id,
                            EAI_NodeId* out_node)
{
    EAI_Fixed best_score;
    EAI_NodeId best_node;

    if (out_node == 0)
    {
        return EAI_FALSE;
    }
    *out_node = EAI_INVALID_ID;

    if (self_id >= EAI_MAX_ENTITIES || threat_id >= EAI_MAX_ENTITIES)
    {
        return EAI_FALSE;
    }
    if (ctx->entities[self_id].used == 0 || ctx->entities[threat_id].used == 0)
    {
        return EAI_FALSE;
    }

    best_score = EAI_FX_MAX;
    best_node = EAI_INVALID_ID;

    if (!eai_tactical_cover_pass(ctx, self_id, threat_id, EAI_TRUE, &best_node, &best_score))
    {
        if (!eai_tactical_cover_pass(ctx, self_id, threat_id, EAI_FALSE, &best_node, &best_score))
        {
            return EAI_FALSE;
        }
    }

    ctx->entities[self_id].memory.cover_node = best_node;
    *out_node = best_node;
    return EAI_TRUE;
}
