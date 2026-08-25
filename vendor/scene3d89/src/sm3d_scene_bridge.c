#include "sm3d_scene_bridge.h"

typedef struct SM3D_RenderPack {
    SM3D_RenderFeedFn fn;
    void *user;
} SM3D_RenderPack;

typedef struct SM3D_PhysicsPack {
    SM3D_PhysicsFeedFn fn;
    void *user;
} SM3D_PhysicsPack;

static int sm3d_render_visit(SM3D_Context *ctx, SM3D_Handle h, const SM3D_Node *n, void *user)
{
    SM3D_RenderFeed feed;
    SM3D_RenderPack *pack;
    int r;
    (void)ctx;
    pack = (SM3D_RenderPack *)user;
    feed.handle = h;
    feed.resource_id = n->resource_id;
    feed.layer_mask = n->layer_mask;
    feed.world = n->world;
    feed.world_bounds = n->world_bounds;
    r = pack->fn(&feed, pack->user);
    if (r != 0) return SM3D_VISIT_STOP;
    return SM3D_VISIT_CONTINUE;
}

static int sm3d_physics_visit(SM3D_Context *ctx, SM3D_Handle h, const SM3D_Node *n, void *user)
{
    SM3D_PhysicsFeed feed;
    SM3D_PhysicsPack *pack;
    int r;
    (void)ctx;
    pack = (SM3D_PhysicsPack *)user;
    feed.handle = h;
    feed.collider_id = n->resource_id;
    feed.layer_mask = n->layer_mask;
    feed.world = n->world;
    feed.world_bounds = n->world_bounds;
    r = pack->fn(&feed, pack->user);
    if (r != 0) return SM3D_VISIT_STOP;
    return SM3D_VISIT_CONTINUE;
}

int sm3d_feed_renderables(SM3D_Context *ctx, int scene_id, sm3d_u32 layer_mask, SM3D_RenderFeedFn fn, void *user)
{
    SM3D_TraverseParams p;
    SM3D_RenderPack pack;
    if (ctx == 0 || fn == 0) return SM3D_ERR_BAD_ARGUMENT;
    sm3d_traverse_params_defaults(&p);
    p.scene_id = scene_id;
    p.required_flags = SM3D_NODE_FLAG_RENDERABLE | SM3D_NODE_FLAG_VISIBLE | SM3D_NODE_FLAG_ACTIVE;
    p.layer_mask = layer_mask;
    pack.fn = fn;
    pack.user = user;
    return sm3d_traverse(ctx, &p, sm3d_render_visit, &pack);
}

int sm3d_feed_collidables(SM3D_Context *ctx, int scene_id, sm3d_u32 layer_mask, SM3D_PhysicsFeedFn fn, void *user)
{
    SM3D_TraverseParams p;
    SM3D_PhysicsPack pack;
    if (ctx == 0 || fn == 0) return SM3D_ERR_BAD_ARGUMENT;
    sm3d_traverse_params_defaults(&p);
    p.scene_id = scene_id;
    p.required_flags = SM3D_NODE_FLAG_COLLIDABLE | SM3D_NODE_FLAG_ACTIVE;
    p.layer_mask = layer_mask;
    pack.fn = fn;
    pack.user = user;
    return sm3d_traverse(ctx, &p, sm3d_physics_visit, &pack);
}
