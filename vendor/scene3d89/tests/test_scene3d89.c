#include <stdio.h>
#include "sm3d_scene.h"

#define TASSERT(x) do { if (!(x)) { printf("FAIL line %d\n", __LINE__); return 1; } } while (0)

int main(void)
{
    SM3D_Context ctx;
    SM3D_Config cfg;
    const SM3D_Scene *scene;
    SM3D_Handle root;
    SM3D_Handle a;
    SM3D_Handle b;
    SM3D_Handle out[16];
    SM3D_QueryResult q;
    SM3D_Aabb box;
    SM3D_Aabb bounds;
    int scene_id;
    int r;

    sm3d_config_defaults(&cfg);
    cfg.grid.enabled = 1;
    cfg.grid.origin_x = sm3d_fx_from_int(-128);
    cfg.grid.origin_z = sm3d_fx_from_int(-128);
    cfg.grid.cell_size = sm3d_fx_from_int(16);
    sm3d_init(&ctx, &cfg);

    r = sm3d_scene_create(&ctx, sm3d_hash_cstr("test"), 0UL, &scene_id);
    TASSERT(r == SM3D_OK);
    scene = sm3d_scene_get(&ctx, scene_id);
    TASSERT(scene != 0);
    root = sm3d_handle_make(scene->root_node, ctx.nodes[scene->root_node].generation);

    r = sm3d_node_create(&ctx, scene_id, root, SM3D_NODE_TYPE_ACTOR, sm3d_hash_cstr("a"), &a);
    TASSERT(r == SM3D_OK);
    r = sm3d_node_create(&ctx, scene_id, a, SM3D_NODE_TYPE_MESH_REF, sm3d_hash_cstr("b"), &b);
    TASSERT(r == SM3D_OK);

    sm3d_node_set_local_pos(&ctx, a, sm3d_vec3(sm3d_fx_from_int(10), 0, sm3d_fx_from_int(0)));
    sm3d_node_set_local_pos(&ctx, b, sm3d_vec3(sm3d_fx_from_int(5), 0, sm3d_fx_from_int(0)));
    bounds = sm3d_aabb(sm3d_vec3(0, 0, 0), sm3d_vec3(sm3d_fx_from_int(1), sm3d_fx_from_int(1), sm3d_fx_from_int(1)));
    sm3d_node_set_bounds(&ctx, b, &bounds);
    sm3d_node_set_flags(&ctx, b, SM3D_NODE_FLAG_RENDERABLE, SM3D_TRUE);

    r = sm3d_update_transforms(&ctx);
    TASSERT(r == SM3D_OK);
    TASSERT(sm3d_fx_to_int(sm3d_node_get(&ctx, b)->world.pos.x) == 15);

    q.out = out;
    q.capacity = 16;
    q.count = 0;
    q.overflow = 0;
    box = sm3d_aabb(sm3d_vec3(sm3d_fx_from_int(15), 0, 0), sm3d_vec3(sm3d_fx_from_int(2), sm3d_fx_from_int(2), sm3d_fx_from_int(2)));
    r = sm3d_query_aabb(&ctx, scene_id, &box, SM3D_LAYER_ALL, 0UL, &q);
    TASSERT(r == SM3D_OK);
    TASSERT(q.count >= 1);

    r = sm3d_grid_rebuild(&ctx, scene_id);
    TASSERT(r == SM3D_OK);
    q.count = 0;
    r = sm3d_grid_query_aabb(&ctx, scene_id, &box, SM3D_LAYER_ALL, 0UL, &q);
    TASSERT(r == SM3D_OK);
    TASSERT(q.count >= 1);

    r = sm3d_node_set_parent(&ctx, a, b);
    TASSERT(r == SM3D_ERR_CYCLE);

    printf("OK scene3d89 tests passed\n");
    return 0;
}
