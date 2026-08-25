#include "w3d89.h"
#include <stdio.h>
#include <string.h>

#define TEST_MEM_SIZE 262144UL
static unsigned char g_mem[TEST_MEM_SIZE];

static int g_fail;
static int g_cell_events;
static int g_render_events;
static int g_collision_events;

static void check_int(const char *name, int got, int want)
{
    if (got != want) {
        printf("FAIL %s got=%d want=%d\n", name, got, want);
        g_fail = 1;
    }
}

static void check_true(const char *name, int cond)
{
    if (!cond) {
        printf("FAIL %s\n", name);
        g_fail = 1;
    }
}

static int on_cell(struct w3d_world_s *world, const struct w3d_cell_s *cell, int action, void *user)
{
    (void)world;
    (void)cell;
    (void)action;
    (void)user;
    g_cell_events++;
    return W3D_BRIDGE_ACCEPTED;
}

static int on_render(struct w3d_world_s *world, const struct w3d_cell_s *cell, int action, void *user)
{
    (void)world;
    (void)cell;
    (void)action;
    (void)user;
    g_render_events++;
    return W3D_BRIDGE_ACCEPTED;
}

static int on_collision(struct w3d_world_s *world, const struct w3d_cell_s *cell, int action, void *user)
{
    (void)world;
    (void)cell;
    (void)action;
    (void)user;
    g_collision_events++;
    return W3D_BRIDGE_ACCEPTED;
}

static void setup_world(w3d_world *w, w3d_world_config *cfg, w3d_world_callbacks *cb)
{
    w3d_u32 need;
    w3d_world_default_config(cfg);
    cfg->max_cells = 64U;
    cfg->max_cell_hash_entries = 139U;
    cfg->max_entities = 128U;
    cfg->max_stream_sources = 2U;
    cfg->max_events = 256U;
    cfg->max_portals = 16U;
    cfg->cell_size = w3d_fp_from_int(10);
    cfg->auto_commit_streaming = 1U;
    cfg->budget.max_load_requests_per_tick = 2U;
    cfg->budget.max_unload_requests_per_tick = 2U;
    cfg->budget.max_activate_requests_per_tick = 2U;
    cfg->budget.max_deactivate_requests_per_tick = 2U;
    need = w3d_world_memory_required(cfg);
    check_true("memory_required", need > 0UL && need <= TEST_MEM_SIZE);
    cb->cell_event = on_cell;
    cb->render_event = on_render;
    cb->collision_event = on_collision;
    cb->entity_event = 0;
    cb->portal_event = 0;
    cb->asset_event = 0;
    cb->physics_event = 0;
    cb->nav_event = 0;
    cb->audio_event = 0;
    cb->script_event = 0;
    check_int("world_init", w3d_world_init(w, cfg, g_mem, TEST_MEM_SIZE, cb, 0), W3D_OK);
}

static void test_hash_and_handle(void)
{
    w3d_world w;
    w3d_world_config cfg;
    w3d_world_callbacks cb;
    w3d_aabb box;
    w3d_handle h1;
    w3d_handle h2;
    int c0;
    setup_world(&w, &cfg, &cb);
    c0 = w3d_world_define_cell(&w, 0, 0, 0, 0UL, W3D_LAYER_ALL, 100UL);
    check_true("define c0", c0 >= 0);
    check_int("hash find", w3d_world_find_cell(&w, 0, 0, 0), c0);
    box = w3d_aabb_make(w3d_v3_make(-W3D_FP_HALF, -W3D_FP_HALF, -W3D_FP_HALF), w3d_v3_make(W3D_FP_HALF, W3D_FP_HALF, W3D_FP_HALF));
    h1 = w3d_entity_spawn(&w, w3d_v3_make(w3d_fp_from_int(1), 0, 0), box, W3D_ENTITY_QUERYABLE, W3D_LAYER_ALL, 0UL, 1UL, 9UL);
    check_true("spawn h1", h1 != W3D_INVALID_HANDLE);
    check_true("resolve h1", w3d_entity_resolve(&w, h1) != 0);
    check_int("remove h1", w3d_entity_remove(&w, h1), W3D_OK);
    check_true("old invalid after remove", w3d_entity_resolve(&w, h1) == 0);
    h2 = w3d_entity_spawn(&w, w3d_v3_make(w3d_fp_from_int(1), 0, 0), box, W3D_ENTITY_QUERYABLE, W3D_LAYER_ALL, 0UL, 2UL, 10UL);
    check_true("spawn h2", h2 != W3D_INVALID_HANDLE);
    check_true("old invalid after respawn", w3d_entity_resolve(&w, h1) == 0);
    check_true("new valid", w3d_entity_resolve(&w, h2) != 0);
}

static void test_query_budget_hysteresis(void)
{
    w3d_world w;
    w3d_world_config cfg;
    w3d_world_callbacks cb;
    w3d_aabb box;
    w3d_aabb query;
    w3d_handle out[8];
    w3d_world_status st;
    int i;
    setup_world(&w, &cfg, &cb);
    for (i = -3; i <= 3; ++i) {
        check_true("define stream cell", w3d_world_define_cell(&w, (w3d_i16)i, 0, 0, 0UL, W3D_LAYER_ALL, (w3d_u32)(100 + i + 3)) >= 0);
    }
    box = w3d_aabb_make(w3d_v3_make(-W3D_FP_HALF, -W3D_FP_HALF, -W3D_FP_HALF), w3d_v3_make(W3D_FP_HALF, W3D_FP_HALF, W3D_FP_HALF));
    w3d_entity_spawn(&w, w3d_v3_make(w3d_fp_from_int(1), 0, 0), box, W3D_ENTITY_QUERYABLE, W3D_LAYER_ALL, 1UL, 1UL, 1UL);
    w3d_entity_spawn(&w, w3d_v3_make(w3d_fp_from_int(25), 0, 0), box, W3D_ENTITY_QUERYABLE, W3D_LAYER_ALL, 1UL, 2UL, 2UL);
    query = w3d_aabb_make(w3d_v3_make(w3d_fp_from_int(0), w3d_fp_from_int(-2), w3d_fp_from_int(-2)), w3d_v3_make(w3d_fp_from_int(3), w3d_fp_from_int(2), w3d_fp_from_int(2)));
    check_int("query one", w3d_query_aabb(&w, query, W3D_LAYER_ALL, out, 8U), 1);
    w3d_world_get_status(&w, &st);
    check_true("query touched cells", st.last_query_touched_cells < st.cell_count);
    check_int("set source hyst", w3d_world_set_stream_source_hysteresis(&w, 0U, w3d_v3_make(0,0,0), w3d_fp_from_int(15), w3d_fp_from_int(20), w3d_fp_from_int(30), w3d_fp_from_int(40), W3D_LAYER_ALL, W3D_PHASE_ALL, W3D_DIFF_ALL, 1), W3D_OK);
    check_int("update streaming", w3d_world_update_streaming(&w), W3D_OK);
    w3d_world_get_status(&w, &st);
    check_true("load budget respected", st.load_requests_last_tick <= 2U);
    check_true("deferred present", st.deferred_requests_last_tick > 0U);
    check_int("move source inside exit", w3d_world_set_stream_source_hysteresis(&w, 0U, w3d_v3_make(w3d_fp_from_int(17),0,0), w3d_fp_from_int(15), w3d_fp_from_int(25), w3d_fp_from_int(30), w3d_fp_from_int(40), W3D_LAYER_ALL, W3D_PHASE_ALL, W3D_DIFF_ALL, 1), W3D_OK);
    check_int("update streaming 2", w3d_world_update_streaming(&w), W3D_OK);
    check_true("callbacks fired", g_cell_events > 0 && g_render_events > 0 && g_collision_events > 0);
}

static void test_portal_descriptor_debug(void)
{
    w3d_world w;
    w3d_world_config cfg;
    w3d_world_callbacks cb;
    w3d_aabb zero;
    w3d_u16 visible[8];
    w3d_debug_cell dc[8];
    const char *txt;
    setup_world(&w, &cfg, &cb);
    txt = "cell 0 0 0 100 32 4294967295 1 4294967295 127\ncell 1 0 0 101 32 4294967295 1 4294967295 127\nproxy 1 0 0 900 20\nportal 0 1 1 77\n";
    check_int("descriptor", w3d_descriptor_apply_text(&w, txt, (w3d_u32)strlen(txt)), W3D_OK);
    zero = w3d_aabb_make(w3d_v3_make(0,0,0), w3d_v3_make(0,0,0));
    check_true("manual portal", w3d_world_define_portal(&w, 0U, 1U, zero, W3D_PORTAL_OPEN, W3D_LAYER_ALL, W3D_PHASE_ALL, 33UL) >= 0);
    check_int("visible cells", w3d_world_query_visible_cells_from(&w, 0U, 2U, W3D_LAYER_ALL, visible, 8U), 2);
    check_true("debug cells", w3d_debug_emit_cells(&w, dc, 8U) >= 2);
}

int main(void)
{
    test_hash_and_handle();
    test_query_budget_hysteresis();
    test_portal_descriptor_debug();
    if (g_fail) return 1;
    printf("PASS world3d89 v2 tests\n");
    return 0;
}
