#include "eai_nav.h"

static int eai_nav_line_blocked(EAI_Context* ctx,
                                EAI_EntityId entity_id,
                                const EAI_Vec3* from,
                                const EAI_Vec3* to)
{
    (void)entity_id;

    if (ctx->world_ops.raycast_world != 0)
    {
        if (ctx->world_ops.raycast_world(ctx->world_user, from, to, 0u) != 0)
        {
            return EAI_TRUE;
        }
    }
    return EAI_FALSE;
}

int eai_nav_path_smooth(EAI_Context* ctx, EAI_EntityId entity_id, EAI_Path* path)
{
    EAI_Path smoothed;
    EAI_U16 source_index;

    if (path == 0 || path->count <= 2)
    {
        return EAI_TRUE;
    }

    smoothed.count = 0;
    source_index = 0;
    smoothed.nodes[smoothed.count] = path->nodes[source_index];
    ++smoothed.count;

    while (source_index < path->count - 1u)
    {
        EAI_U16 probe;
        EAI_U16 last_good;

        last_good = source_index + 1u;
        probe = last_good + 1u;

        while (probe < path->count)
        {
            const EAI_Vec3* a;
            const EAI_Vec3* b;

            a = &ctx->nodes[path->nodes[source_index]].pos;
            b = &ctx->nodes[path->nodes[probe]].pos;

            if (eai_nav_line_blocked(ctx, entity_id, a, b))
            {
                break;
            }

            last_good = probe;
            ++probe;
        }

        if (smoothed.count >= EAI_MAX_PATH_LEN)
        {
            return EAI_FALSE;
        }

        smoothed.nodes[smoothed.count] = path->nodes[last_good];
        ++smoothed.count;
        source_index = last_good;
    }

    *path = smoothed;
    return EAI_TRUE;
}

int eai_nav_path_get_point(const EAI_Context* ctx, const EAI_Path* path, EAI_U16 index, EAI_Vec3* out_pos)
{
    EAI_NodeId node_id;

    if (path == 0 || out_pos == 0)
    {
        return EAI_FALSE;
    }
    if (index >= path->count)
    {
        return EAI_FALSE;
    }

    node_id = path->nodes[index];
    if (node_id >= EAI_MAX_NAV_NODES || ctx->nodes[node_id].used == 0)
    {
        return EAI_FALSE;
    }

    *out_pos = ctx->nodes[node_id].pos;
    return EAI_TRUE;
}
