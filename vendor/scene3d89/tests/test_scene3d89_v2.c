#include "sm3d_scene.h"
#include <stdio.h>
#include <string.h>

static int fail(const char *msg)
{
    printf("FAIL: %s\n", msg);
    return 1;
}

static int count_visit(SM3D_Context *ctx, SM3D_Handle h, const SM3D_Node *n, void *user)
{
    int *count;
    (void)ctx;
    (void)h;
    (void)n;
    count = (int *)user;
    (*count)++;
    return SM3D_VISIT_CONTINUE;
}

int main(void)
{
    SM3D_Context ctx;
    SM3D_Config cfg;
    int base_scene;
    int level_scene;
    int room_scene;
    int req;
    const SM3D_SceneRequest *rq;
    const SM3D_Scene *sc;
    SM3D_Handle root;
    SM3D_Handle a;
    SM3D_Handle b;
    SM3D_Handle sector_a;
    SM3D_Handle sector_b;
    SM3D_Handle portal;
    SM3D_Handle out[64];
    SM3D_QueryResult qr;
    SM3D_PrefabNodeDesc pd;
    int prefab;
    SM3D_Handle prefab_root;
    SM3D_Transform t;
    int dirty_count;

    sm3d_config_defaults(&cfg);
    sm3d_init(&ctx, &cfg);

    if (sm3d_scene_create_in_slot(&ctx, sm3d_hash_cstr("persistent"), SM3D_SCENE_FLAG_PERSISTENT, SM3D_SCENE_SLOT_PERSISTENT, 0, &base_scene) != SM3D_OK) return fail("create persistent slot");
    if (sm3d_scene_create_in_slot(&ctx, sm3d_hash_cstr("level01"), SM3D_SCENE_FLAG_ADDITIVE, SM3D_SCENE_SLOT_LEVEL, 0, &level_scene) != SM3D_OK) return fail("create level slot");
    if (sm3d_scene_dependency_add(&ctx, level_scene, base_scene, SM3D_DEP_REQUIRED, 0UL, 0UL) != SM3D_OK) return fail("add scene dep");
    if (!sm3d_scene_dependencies_loaded(&ctx, level_scene)) return fail("deps loaded");
    if (sm3d_scene_unload_request(&ctx, base_scene, &req) != SM3D_OK) return fail("unload req persistent");
    if (sm3d_scene_process_requests(&ctx, 1) != SM3D_OK) return fail("process unload req");
    rq = sm3d_scene_request_get(&ctx, req);
    if (rq == 0 || rq->state != SM3D_REQ_FAILED) return fail("required dependent blocked unload");

    if (sm3d_scene_load_request(&ctx, sm3d_hash_cstr("room_a"), SM3D_SCENE_SLOT_ROOM, 1, SM3D_SCENE_FLAG_STREAMABLE, &req) != SM3D_OK) return fail("load request");
    if (sm3d_scene_process_requests(&ctx, 1) != SM3D_OK) return fail("process load request");
    rq = sm3d_scene_request_get(&ctx, req);
    if (rq == 0 || rq->state != SM3D_REQ_DONE || rq->scene_id <= 0) return fail("load request done");
    room_scene = rq->scene_id;
    sc = sm3d_scene_get(&ctx, room_scene);
    if (sc == 0 || sc->slot_type != SM3D_SCENE_SLOT_ROOM || sc->slot_index != 1) return fail("room slot fields");

    root = sm3d_handle_make(sc->root_node, ctx.nodes[sc->root_node].generation);
    if (sm3d_node_create(&ctx, room_scene, root, SM3D_NODE_TYPE_ACTOR, sm3d_hash_cstr("actor_a"), &a) != SM3D_OK) return fail("actor a");
    if (sm3d_node_create(&ctx, room_scene, root, SM3D_NODE_TYPE_ACTOR, sm3d_hash_cstr("actor_b"), &b) != SM3D_OK) return fail("actor b");
    if (sm3d_dag_link_add(&ctx, a, b, SM3D_DAG_KIND_LOGIC, SM3D_DAG_FLAG_ACYCLIC, 0UL) != SM3D_OK) return fail("dag link");
    if (sm3d_dag_link_add(&ctx, b, a, SM3D_DAG_KIND_LOGIC, SM3D_DAG_FLAG_ACYCLIC, 0UL) != SM3D_ERR_DAG_CYCLE) return fail("dag cycle guard");
    qr.out = out;
    qr.capacity = 64;
    if (sm3d_dag_collect_outputs(&ctx, a, SM3D_DAG_KIND_LOGIC, &qr) != SM3D_OK || qr.count != 1) return fail("dag outputs");

    memset(&pd, 0, sizeof(pd));
    if (sm3d_prefab_create(&ctx, sm3d_hash_cstr("enemy_pair"), SM3D_PREFAB_FLAG_ACTIVE, &prefab) != SM3D_OK) return fail("prefab create");
    pd.parent_index = -1;
    pd.type = SM3D_NODE_TYPE_ACTOR;
    pd.name_hash = sm3d_hash_cstr("enemy_root");
    pd.flags = SM3D_NODE_FLAG_RENDERABLE;
    pd.layer_mask = SM3D_LAYER_DEFAULT;
    pd.local = sm3d_transform_identity();
    pd.local_bounds = sm3d_aabb(sm3d_vec3(0,0,0), sm3d_vec3(sm3d_fx_from_int(1), sm3d_fx_from_int(1), sm3d_fx_from_int(1)));
    if (sm3d_prefab_add_node(&ctx, prefab, &pd, 0) != SM3D_OK) return fail("prefab add root");
    pd.parent_index = 0;
    pd.type = SM3D_NODE_TYPE_MESH_REF;
    pd.name_hash = sm3d_hash_cstr("enemy_mesh");
    if (sm3d_prefab_add_node(&ctx, prefab, &pd, 0) != SM3D_OK) return fail("prefab add child");
    t = sm3d_transform_identity();
    t.pos.x = sm3d_fx_from_int(7);
    if (sm3d_prefab_instantiate(&ctx, prefab, room_scene, root, &t, &prefab_root) != SM3D_OK) return fail("prefab instantiate");
    if (sm3d_node_get_const(&ctx, prefab_root) == 0) return fail("prefab root resolve");

    if (sm3d_node_create(&ctx, room_scene, root, SM3D_NODE_TYPE_SECTOR, sm3d_hash_cstr("sector_a"), &sector_a) != SM3D_OK) return fail("sector a");
    if (sm3d_node_create(&ctx, room_scene, root, SM3D_NODE_TYPE_SECTOR, sm3d_hash_cstr("sector_b"), &sector_b) != SM3D_OK) return fail("sector b");
    if (sm3d_node_create(&ctx, room_scene, root, SM3D_NODE_TYPE_PORTAL, sm3d_hash_cstr("portal_ab"), &portal) != SM3D_OK) return fail("portal");
    if (sm3d_portal_hint_add(&ctx, room_scene, portal, sector_a, sector_b, SM3D_PORTAL_FLAG_OPEN | SM3D_PORTAL_FLAG_VISIBLE, SM3D_LAYER_ALL, 0UL) != SM3D_OK) return fail("portal add");
    qr.out = out;
    qr.capacity = 64;
    if (sm3d_portal_collect_visible(&ctx, room_scene, sector_a, 4, &qr) != SM3D_OK || qr.count != 2) return fail("portal visibility");

    if (sm3d_node_set_local_pos(&ctx, a, sm3d_vec3(sm3d_fx_from_int(3), 0, 0)) != SM3D_OK) return fail("dirty move");
    dirty_count = 0;
    if (sm3d_traverse_dirty(&ctx, room_scene, SM3D_LAYER_ALL, count_visit, &dirty_count) != SM3D_OK) return fail("dirty traverse");
    if (dirty_count <= 0) return fail("dirty count");
    if (sm3d_update_dirty_transforms(&ctx) != SM3D_OK) return fail("dirty update");
    if (ctx.dirty_count != 0) return fail("dirty clear after update");

    if (sm3d_manifest_read_text(&ctx, "scene overlay_debug overlay 2 0x0008\n", 0) != SM3D_OK) return fail("manifest scene");
    if (sm3d_scene_find_by_hash(&ctx, sm3d_hash_cstr("overlay_debug")) == SM3D_INVALID_INDEX) return fail("manifest scene find");

    printf("OK scene3d89 v2 tests passed\n");
    return 0;
}
