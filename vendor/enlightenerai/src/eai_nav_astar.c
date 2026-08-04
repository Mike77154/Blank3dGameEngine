#include "eai_nav.h"
#include "eai_math.h"

static void eai_astar_reset_records(EAI_Context* ctx)
{
    EAI_U16 i;

    for (i = 0; i < EAI_MAX_NAV_NODES; ++i)
    {
        ctx->astar[i].opened = 0;
        ctx->astar[i].closed = 0;
        ctx->astar[i].parent = EAI_INVALID_ID;
        ctx->astar[i].g = EAI_FX_FROM_RAW(0);
        ctx->astar[i].f = EAI_FX_FROM_RAW(0);
    }
    ctx->open_count = 0;
}

static int eai_astar_push_open(EAI_Context* ctx, EAI_NodeId node_id)
{
    if (ctx->open_count >= EAI_MAX_SEARCH_OPEN)
    {
        return EAI_FALSE;
    }
    ctx->open_nodes[ctx->open_count] = node_id;
    ++ctx->open_count;
    return EAI_TRUE;
}

static EAI_NodeId eai_astar_pop_best(EAI_Context* ctx)
{
    EAI_U16 best_index;
    EAI_U16 i;
    EAI_NodeId best_node;

    best_index = 0;
    best_node = ctx->open_nodes[0];

    for (i = 1; i < ctx->open_count; ++i)
    {
        EAI_NodeId candidate;
        candidate = ctx->open_nodes[i];
        if (ctx->astar[candidate].f < ctx->astar[best_node].f)
        {
            best_index = i;
            best_node = candidate;
        }
    }

    ctx->open_nodes[best_index] = ctx->open_nodes[ctx->open_count - 1u];
    --ctx->open_count;

    return best_node;
}

static EAI_Fixed eai_astar_edge_cost(EAI_Context* ctx,
                                 EAI_NodeId from,
                                 EAI_NodeId to,
                                 EAI_EntityId entity_id,
                                 EAI_Fixed default_cost)
{
    if (ctx->world_ops.edge_cost != 0)
    {
        return ctx->world_ops.edge_cost(ctx->world_user, from, to, entity_id, default_cost);
    }
    return default_cost;
}

static int eai_astar_reconstruct_path(EAI_Context* ctx, EAI_NodeId end_id, EAI_Path* out_path)
{
    EAI_NodeId temp[EAI_MAX_PATH_LEN];
    EAI_U16 count;
    EAI_NodeId cursor;

    count = 0;
    cursor = end_id;

    while (cursor != EAI_INVALID_ID)
    {
        if (count >= EAI_MAX_PATH_LEN)
        {
            return EAI_FALSE;
        }

        temp[count] = cursor;
        ++count;
        cursor = ctx->astar[cursor].parent;
    }

    out_path->count = count;
    while (count > 0)
    {
        --count;
        out_path->nodes[out_path->count - 1u - count] = temp[count];
    }

    return EAI_TRUE;
}

int eai_nav_find_path(EAI_Context* ctx, EAI_EntityId entity_id, EAI_NodeId from, EAI_NodeId to, EAI_Path* out_path)
{
    EAI_NodeId current;

    if (out_path == 0)
    {
        return EAI_FALSE;
    }

    out_path->count = 0;

    if (from == EAI_INVALID_ID || to == EAI_INVALID_ID)
    {
        return EAI_FALSE;
    }

    if (!eai_nav_can_entity_use_node(ctx, entity_id, from) ||
        !eai_nav_can_entity_use_node(ctx, entity_id, to))
    {
        return EAI_FALSE;
    }

    if (from == to)
    {
        out_path->count = 1;
        out_path->nodes[0] = from;
        return EAI_TRUE;
    }

    eai_astar_reset_records(ctx);

    ctx->astar[from].opened = 1;
    ctx->astar[from].parent = EAI_INVALID_ID;
    ctx->astar[from].g = EAI_FX_FROM_RAW(0);
    ctx->astar[from].f = eai_nav_estimate_cost(ctx, from, to);
    if (!eai_astar_push_open(ctx, from))
    {
        return EAI_FALSE;
    }

    while (ctx->open_count > 0)
    {
        current = eai_astar_pop_best(ctx);
        ctx->astar[current].closed = 1;

        if (current == to)
        {
            if (!eai_astar_reconstruct_path(ctx, to, out_path))
            {
                return EAI_FALSE;
            }
            return eai_nav_path_smooth(ctx, entity_id, out_path);
        }

        {
            EAI_EdgeId edge_id;
            edge_id = ctx->nodes[current].first_edge;

            while (edge_id != EAI_INVALID_ID)
            {
                EAI_Edge* edge;
                EAI_NodeId next;
                EAI_Fixed candidate_g;

                edge = &ctx->edges[edge_id];
                next = edge->to;

                if (edge->used == 0 || (edge->flags & EAI_EDGE_FLAG_DISABLED) != 0)
                {
                    edge_id = edge->next_from;
                    continue;
                }

                if (!eai_nav_can_entity_use_node(ctx, entity_id, next))
                {
                    edge_id = edge->next_from;
                    continue;
                }

                if (ctx->astar[next].closed != 0)
                {
                    edge_id = edge->next_from;
                    continue;
                }

                candidate_g = eai_fx_add_sat(ctx->astar[current].g,
                                             eai_astar_edge_cost(ctx, current, next, entity_id, edge->cost));

                if (ctx->astar[next].opened == 0 || candidate_g < ctx->astar[next].g)
                {
                    ctx->astar[next].opened = 1;
                    ctx->astar[next].parent = current;
                    ctx->astar[next].g = candidate_g;
                    ctx->astar[next].f = eai_fx_add_sat(candidate_g, eai_nav_estimate_cost(ctx, next, to));

                    if (ctx->astar[next].opened != 0)
                    {
                        EAI_U16 i;
                        int already_open;

                        already_open = EAI_FALSE;
                        for (i = 0; i < ctx->open_count; ++i)
                        {
                            if (ctx->open_nodes[i] == next)
                            {
                                already_open = EAI_TRUE;
                                break;
                            }
                        }
                        if (!already_open)
                        {
                            if (!eai_astar_push_open(ctx, next))
                            {
                                return EAI_FALSE;
                            }
                        }
                    }
                }

                edge_id = edge->next_from;
            }
        }
    }

    return EAI_FALSE;
}
