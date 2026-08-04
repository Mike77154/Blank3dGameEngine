#include "eai_nav.h"
#include "eai_math.h"

static EAI_U16 eai_nav_count_free_edges(const EAI_Context* ctx)
{
    EAI_U16 i;
    EAI_U16 free_count;

    free_count = 0;
    for (i = 0; i < EAI_MAX_NAV_EDGES; ++i)
    {
        if (ctx->edges[i].used == 0)
        {
            ++free_count;
            if (free_count >= 2u)
            {
                return free_count;
            }
        }
    }

    return free_count;
}

void eai_nav_reset(EAI_Context* ctx)
{
    EAI_U16 i;

    ctx->node_count = 0;
    ctx->edge_count = 0;
    ctx->zone_count = 0;

    for (i = 0; i < EAI_MAX_NAV_NODES; ++i)
    {
        ctx->nodes[i].used = 0;
        ctx->nodes[i].flags = 0;
        ctx->nodes[i].zone_id = EAI_INVALID_ID;
        ctx->nodes[i].first_edge = EAI_INVALID_ID;
        eai_vec3_set(&ctx->nodes[i].pos, EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0), EAI_FX_FROM_RAW(0));
    }

    for (i = 0; i < EAI_MAX_NAV_EDGES; ++i)
    {
        ctx->edges[i].used = 0;
        ctx->edges[i].flags = 0;
        ctx->edges[i].from = EAI_INVALID_ID;
        ctx->edges[i].to = EAI_INVALID_ID;
        ctx->edges[i].next_from = EAI_INVALID_ID;
        ctx->edges[i].cost = EAI_FX_FROM_RAW(0);
    }

    for (i = 0; i < EAI_MAX_ZONES; ++i)
    {
        ctx->zones[i].used = 0;
        ctx->zones[i].flags = 0;
    }
}

EAI_NodeId eai_nav_add_node(EAI_Context* ctx, const EAI_Vec3* pos, EAI_ZoneId zone_id, EAI_U8 flags)
{
    EAI_U16 i;

    for (i = 0; i < EAI_MAX_NAV_NODES; ++i)
    {
        if (ctx->nodes[i].used == 0)
        {
            ctx->nodes[i].used = 1;
            ctx->nodes[i].flags = flags;
            ctx->nodes[i].zone_id = zone_id;
            ctx->nodes[i].first_edge = EAI_INVALID_ID;
            ctx->nodes[i].pos = *pos;
            ++ctx->node_count;
            return i;
        }
    }

    return EAI_INVALID_ID;
}

int eai_nav_add_edge(EAI_Context* ctx, EAI_NodeId from, EAI_NodeId to, EAI_Fixed cost, EAI_U8 flags)
{
    EAI_U16 i;

    if (from >= EAI_MAX_NAV_NODES || to >= EAI_MAX_NAV_NODES)
    {
        return EAI_FALSE;
    }

    if (ctx->nodes[from].used == 0 || ctx->nodes[to].used == 0)
    {
        return EAI_FALSE;
    }

    for (i = 0; i < EAI_MAX_NAV_EDGES; ++i)
    {
        if (ctx->edges[i].used == 0)
        {
            ctx->edges[i].used = 1;
            ctx->edges[i].flags = flags;
            ctx->edges[i].from = from;
            ctx->edges[i].to = to;
            ctx->edges[i].cost = cost;
            ctx->edges[i].next_from = ctx->nodes[from].first_edge;
            ctx->nodes[from].first_edge = i;
            ++ctx->edge_count;
            return EAI_TRUE;
        }
    }

    return EAI_FALSE;
}

int eai_nav_add_bidirectional_edge(EAI_Context* ctx, EAI_NodeId a, EAI_NodeId b, EAI_Fixed cost, EAI_U8 flags)
{
    if (a >= EAI_MAX_NAV_NODES || b >= EAI_MAX_NAV_NODES)
    {
        return EAI_FALSE;
    }

    if (ctx->nodes[a].used == 0 || ctx->nodes[b].used == 0)
    {
        return EAI_FALSE;
    }

    if (eai_nav_count_free_edges(ctx) < 2u)
    {
        return EAI_FALSE;
    }

    if (!eai_nav_add_edge(ctx, a, b, cost, flags))
    {
        return EAI_FALSE;
    }
    if (!eai_nav_add_edge(ctx, b, a, cost, flags))
    {
        return EAI_FALSE;
    }
    return EAI_TRUE;
}

EAI_NodeId eai_nav_find_nearest_node(EAI_Context* ctx, const EAI_Vec3* pos, EAI_ZoneId zone_filter)
{
    EAI_Fixed best_d;
    EAI_NodeId best_id;
    EAI_U16 i;

    best_d = EAI_FX_FROM_RAW(0);
    best_id = EAI_INVALID_ID;

    for (i = 0; i < EAI_MAX_NAV_NODES; ++i)
    {
        EAI_Fixed d;
        const EAI_Node* n;

        n = &ctx->nodes[i];
        if (n->used == 0)
        {
            continue;
        }
        if ((n->flags & EAI_NODE_FLAG_DISABLED) != 0)
        {
            continue;
        }
        if (zone_filter != EAI_INVALID_ID && n->zone_id != zone_filter)
        {
            continue;
        }
        d = eai_vec3_dist_sq(pos, &n->pos);
        if (best_id == EAI_INVALID_ID || d < best_d)
        {
            best_d = d;
            best_id = i;
        }
    }

    return best_id;
}

int eai_nav_can_entity_use_node(const EAI_Context* ctx, EAI_EntityId entity_id, EAI_NodeId node_id)
{
    const EAI_Node* node;

    if (node_id >= EAI_MAX_NAV_NODES)
    {
        return EAI_FALSE;
    }

    node = &ctx->nodes[node_id];
    if (node->used == 0)
    {
        return EAI_FALSE;
    }
    if ((node->flags & EAI_NODE_FLAG_DISABLED) != 0)
    {
        return EAI_FALSE;
    }

    if (node->zone_id != EAI_INVALID_ID)
    {
        if (node->zone_id >= EAI_MAX_ZONES)
        {
            return EAI_FALSE;
        }
        if (ctx->zones[node->zone_id].used != 0 &&
            (ctx->zones[node->zone_id].flags & (EAI_ZONE_FLAG_DISABLED | EAI_ZONE_FLAG_LOCKED)) != 0)
        {
            return EAI_FALSE;
        }
    }

    if (entity_id < EAI_MAX_ENTITIES && ctx->entities[entity_id].used != 0)
    {
        const EAI_Entity* e;
        e = &ctx->entities[entity_id];
        if ((e->flags & EAI_ENTITY_FLAG_LOCK_ZONE) != 0 &&
            e->zone_lock_id != EAI_INVALID_ID &&
            node->zone_id != EAI_INVALID_ID &&
            node->zone_id != e->zone_lock_id)
        {
            return EAI_FALSE;
        }
    }

    return EAI_TRUE;
}

EAI_Fixed eai_nav_estimate_cost(const EAI_Context* ctx, EAI_NodeId a, EAI_NodeId b)
{
    return eai_vec3_dist(&ctx->nodes[a].pos, &ctx->nodes[b].pos);
}
