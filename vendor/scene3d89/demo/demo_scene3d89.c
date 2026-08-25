#include <stdio.h>
#include "sm3d_scene.h"
#include "sm3d_scene_bridge.h"

static int event_printer(SM3D_Context *ctx, int event_type, SM3D_Handle node, int scene_id, void *user)
{
    (void)ctx;
    (void)user;
    printf("event=%d scene=%d node=%lu\n", event_type, scene_id, node.value);
    return 0;
}

static int render_feed(const SM3D_RenderFeed *feed, void *user)
{
    int *count;
    count = (int *)user;
    (*count)++;
    printf("render handle=%lu res=%lu pos=(%d,%d,%d)\n",
           feed->handle.value,
           feed->resource_id,
           sm3d_fx_to_int(feed->world.pos.x),
           sm3d_fx_to_int(feed->world.pos.y),
           sm3d_fx_to_int(feed->world.pos.z));
    return 0;
}

static int visitor(SM3D_Context *ctx, SM3D_Handle h, const SM3D_Node *n, void *user)
{
    int *count;
    (void)ctx;
    count = (int *)user;
    (*count)++;
    printf("visit handle=%lu type=%d scene=%d flags=%lu\n", h.value, n->type, n->scene_id, n->flags);
    return SM3D_VISIT_CONTINUE;
}

int main(void)
{
    SM3D_Context ctx;
    SM3D_Config cfg;
    SM3D_Bridge bridge;
    SM3D_Handle root;
    SM3D_Handle actor;
    SM3D_Handle mesh;
    SM3D_Handle spawn;
    SM3D_TraverseParams params;
    const SM3D_Scene *scene;
    int scene_id;
    int visit_count;
    int render_count;
    int r;
    SM3D_Aabb mesh_bounds;

    sm3d_config_defaults(&cfg);
    cfg.grid.enabled = 1;
    cfg.grid.origin_x = sm3d_fx_from_int(-512);
    cfg.grid.origin_z = sm3d_fx_from_int(-512);
    cfg.grid.cell_size = sm3d_fx_from_int(64);
    cfg.auto_update_grid = 1;

    sm3d_init(&ctx, &cfg);
    bridge.event_fn = event_printer;
    bridge.user = 0;
    sm3d_set_bridge(&ctx, &bridge);

    r = sm3d_scene_create(&ctx, sm3d_hash_cstr("mansion_hall"), SM3D_SCENE_FLAG_PERSISTENT, &scene_id);
    if (r != SM3D_OK) return 1;

    scene = sm3d_scene_get(&ctx, scene_id);
    root = sm3d_handle_make(scene->root_node, ctx.nodes[scene->root_node].generation);

    sm3d_node_create(&ctx, scene_id, root, SM3D_NODE_TYPE_ACTOR, sm3d_hash_cstr("player"), &actor);
    sm3d_node_create(&ctx, scene_id, actor, SM3D_NODE_TYPE_MESH_REF, sm3d_hash_cstr("player_mesh"), &mesh);
    sm3d_node_create(&ctx, scene_id, root, SM3D_NODE_TYPE_SPAWN_POINT, sm3d_hash_cstr("spawn_main"), &spawn);

    sm3d_node_set_local_pos(&ctx, actor, sm3d_vec3(sm3d_fx_from_int(10), sm3d_fx_from_int(0), sm3d_fx_from_int(20)));
    sm3d_node_set_local_pos(&ctx, mesh, sm3d_vec3(sm3d_fx_from_int(1), sm3d_fx_from_int(2), sm3d_fx_from_int(3)));
    mesh_bounds = sm3d_aabb(sm3d_vec3(0, 0, 0), sm3d_vec3(sm3d_fx_from_int(1), sm3d_fx_from_int(2), sm3d_fx_from_int(1)));
    sm3d_node_set_bounds(&ctx, mesh, &mesh_bounds);
    sm3d_node_set_flags(&ctx, mesh, SM3D_NODE_FLAG_RENDERABLE, SM3D_TRUE);
    sm3d_node_set_user_ids(&ctx, mesh, 101UL, 0UL, 0UL);

    sm3d_update_transforms(&ctx);

    visit_count = 0;
    sm3d_traverse_params_defaults(&params);
    params.scene_id = scene_id;
    sm3d_traverse(&ctx, &params, visitor, &visit_count);
    printf("visited=%d\n", visit_count);

    render_count = 0;
    sm3d_feed_renderables(&ctx, scene_id, SM3D_LAYER_ALL, render_feed, &render_count);
    printf("renderables=%d\n", render_count);

    return 0;
}
