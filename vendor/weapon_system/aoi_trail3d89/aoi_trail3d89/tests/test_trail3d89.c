#include "trail3d89.h"
#include <stdio.h>

#define TEST_VERTS 4096
#define TEST_INDICES 8192

static t3d89_vertex g_vertices[TEST_VERTS];
static unsigned short g_indices[TEST_INDICES];

static int fail_count = 0;


typedef struct provider_state_s {
    int base_x;
    int skip_tick;
    int fail_tick;
} provider_state;

static int test_transform_provider(
    void *user,
    int trail_id,
    int tick,
    t3d89_transform *out_transform
)
{
    provider_state *state;

    state = (provider_state *)user;
    if (state == 0 || out_transform == 0) {
        return -1;
    }
    if (tick == state->skip_tick) {
        return T3D89_PROVIDER_SKIP;
    }
    if (tick == state->fail_tick) {
        return -77;
    }

    t3d89_transform_identity(out_transform);
    out_transform->tx = state->base_x + t3d89_fp_from_int(tick);
    out_transform->ty = t3d89_fp_from_int(trail_id);
    return T3D89_PROVIDER_READY;
}

static void check_int(const char *name, int got, int expected)
{
    if (got != expected) {
        printf("FAIL %s got=%d expected=%d\n", name, got, expected);
        fail_count++;
    } else {
        printf("OK %s=%d\n", name, got);
    }
}

static void check_true(const char *name, int expr)
{
    if (!expr) {
        printf("FAIL %s\n", name);
        fail_count++;
    } else {
        printf("OK %s\n", name);
    }
}

static void setup_mesh(t3d89_mesh *mesh)
{
    mesh->vertices = g_vertices;
    mesh->indices = g_indices;
    mesh->max_vertices = TEST_VERTS;
    mesh->max_indices = TEST_INDICES;
    mesh->vertex_count = 0;
    mesh->index_count = 0;
    mesh->flags = 0;
    mesh->overflowed = 0;
}

static t3d89_camera make_camera(void)
{
    t3d89_camera cam;
    cam.x = t3d89_fp_from_int(0);
    cam.y = t3d89_fp_from_int(2);
    cam.z = t3d89_fp_from_int(8);
    cam.up_x = 0;
    cam.up_y = T3D89_FP_ONE;
    cam.up_z = 0;
    return cam;
}

static void emit_line(t3d89_ctx *ctx, int id, int n)
{
    int i;
    i = 0;
    while (i < n) {
        t3d89_emit_point(ctx, id, t3d89_fp_from_int(i), 0, 0);
        t3d89_tick(ctx, 1);
        i++;
    }
}

static void test_ribbon(void)
{
    t3d89_ctx ctx;
    t3d89_desc desc;
    t3d89_mesh mesh;
    t3d89_camera cam;
    int id;
    int rc;

    t3d89_init(&ctx);
    t3d89_default_desc(&desc);
    desc.mode = T3D89_MODE_VIEW_RIBBON;
    desc.max_points = 16;
    desc.life_ticks = 100;
    desc.min_dist = T3D89_FP_ONE / 2;
    desc.min_ticks = 1;
    id = t3d89_create(&ctx, &desc);
    emit_line(&ctx, id, 4);
    check_int("ribbon_points", t3d89_get_point_count(&ctx, id), 4);
    setup_mesh(&mesh);
    cam = make_camera();
    rc = t3d89_build_mesh(&ctx, id, &cam, &mesh);
    check_int("ribbon_rc", rc, T3D89_OK);
    check_int("ribbon_vertices", mesh.vertex_count, 8);
    check_int("ribbon_indices", mesh.index_count, 18);
}

static void test_cross_and_tube(void)
{
    t3d89_ctx ctx;
    t3d89_desc desc;
    t3d89_mesh mesh;
    t3d89_camera cam;
    int id;
    int rc;

    cam = make_camera();
    t3d89_init(&ctx);
    t3d89_default_desc(&desc);
    desc.mode = T3D89_MODE_CROSS_RIBBON;
    desc.max_points = 16;
    desc.life_ticks = 100;
    desc.min_dist = T3D89_FP_ONE / 2;
    id = t3d89_create(&ctx, &desc);
    emit_line(&ctx, id, 4);
    setup_mesh(&mesh);
    rc = t3d89_build_mesh(&ctx, id, &cam, &mesh);
    check_int("cross_rc", rc, T3D89_OK);
    check_int("cross_vertices", mesh.vertex_count, 16);
    check_int("cross_indices", mesh.index_count, 36);

    t3d89_destroy(&ctx, id);
    t3d89_default_desc(&desc);
    desc.mode = T3D89_MODE_TUBE_LITE;
    desc.max_points = 16;
    desc.life_ticks = 100;
    desc.min_dist = T3D89_FP_ONE / 2;
    id = t3d89_create(&ctx, &desc);
    emit_line(&ctx, id, 4);
    setup_mesh(&mesh);
    rc = t3d89_build_mesh(&ctx, id, &cam, &mesh);
    check_int("tube_rc", rc, T3D89_OK);
    check_int("tube_vertices", mesh.vertex_count, 16);
    check_int("tube_indices", mesh.index_count, 72);
}

static void test_beam_and_socket(void)
{
    t3d89_ctx ctx;
    t3d89_desc desc;
    t3d89_mesh mesh;
    t3d89_camera cam;
    int id;
    int rc;
    int i;

    cam = make_camera();
    t3d89_init(&ctx);
    t3d89_default_desc(&desc);
    desc.mode = T3D89_MODE_BEAM_AB;
    desc.max_points = 8;
    desc.life_ticks = 100;
    desc.sampler_mask = T3D89_SAMPLE_FORCE;
    id = t3d89_create(&ctx, &desc);
    i = 0;
    while (i < 2) {
        t3d89_emit_segment(&ctx, id, 0, t3d89_fp_from_int(i), 0, t3d89_fp_from_int(3), t3d89_fp_from_int(i), 0);
        t3d89_tick(&ctx, 1);
        i++;
    }
    setup_mesh(&mesh);
    rc = t3d89_build_mesh(&ctx, id, &cam, &mesh);
    check_int("beam_rc", rc, T3D89_OK);
    check_int("beam_vertices", mesh.vertex_count, 8);
    check_int("beam_indices", mesh.index_count, 12);

    t3d89_destroy(&ctx, id);
    t3d89_default_desc(&desc);
    desc.mode = T3D89_MODE_SOCKET_SWEEP;
    desc.max_points = 8;
    desc.life_ticks = 100;
    desc.sampler_mask = T3D89_SAMPLE_FORCE;
    id = t3d89_create(&ctx, &desc);
    i = 0;
    while (i < 4) {
        t3d89_emit_segment(&ctx, id, t3d89_fp_from_int(i), 0, 0, t3d89_fp_from_int(i), t3d89_fp_from_int(2), 0);
        t3d89_tick(&ctx, 1);
        i++;
    }
    setup_mesh(&mesh);
    rc = t3d89_build_mesh(&ctx, id, &cam, &mesh);
    check_int("socket_rc", rc, T3D89_OK);
    check_int("socket_vertices", mesh.vertex_count, 8);
    check_int("socket_indices", mesh.index_count, 18);
}

static void test_sampling_and_lod(void)
{
    t3d89_ctx ctx;
    t3d89_desc desc;
    t3d89_mesh mesh;
    t3d89_camera cam;
    int id;
    int rc;
    int i;

    cam = make_camera();
    t3d89_init(&ctx);
    t3d89_default_desc(&desc);
    desc.mode = T3D89_MODE_VIEW_RIBBON;
    desc.max_points = 32;
    desc.life_ticks = 100;
    desc.sampler_mask = T3D89_SAMPLE_DISTANCE;
    desc.min_dist = T3D89_FP_ONE;
    id = t3d89_create(&ctx, &desc);
    t3d89_emit_point(&ctx, id, 0, 0, 0);
    t3d89_tick(&ctx, 1);
    t3d89_emit_point(&ctx, id, T3D89_FP_ONE / 4, 0, 0);
    t3d89_tick(&ctx, 1);
    t3d89_emit_point(&ctx, id, T3D89_FP_ONE, 0, 0);
    check_int("distance_points", t3d89_get_point_count(&ctx, id), 2);
    check_int("distance_rejected", t3d89_get_rejected_count(&ctx, id), 1);

    t3d89_destroy(&ctx, id);
    t3d89_default_desc(&desc);
    desc.mode = T3D89_MODE_VIEW_RIBBON;
    desc.max_points = 32;
    desc.life_ticks = 100;
    desc.sampler_mask = T3D89_SAMPLE_FORCE;
    desc.lod_vertex_budget = 8;
    id = t3d89_create(&ctx, &desc);
    i = 0;
    while (i < 16) {
        t3d89_emit_point(&ctx, id, t3d89_fp_from_int(i), 0, 0);
        t3d89_tick(&ctx, 1);
        i++;
    }
    setup_mesh(&mesh);
    rc = t3d89_build_mesh(&ctx, id, &cam, &mesh);
    check_int("lod_rc", rc, T3D89_OK);
    check_true("lod_budget_vertices", mesh.vertex_count <= 18);
}

static void test_age_and_ring(void)
{
    t3d89_ctx ctx;
    t3d89_desc desc;
    int id;
    int i;

    t3d89_init(&ctx);
    t3d89_default_desc(&desc);
    desc.mode = T3D89_MODE_VIEW_RIBBON;
    desc.max_points = 4;
    desc.life_ticks = 3;
    desc.sampler_mask = T3D89_SAMPLE_FORCE;
    id = t3d89_create(&ctx, &desc);
    i = 0;
    while (i < 6) {
        t3d89_emit_point(&ctx, id, t3d89_fp_from_int(i), 0, 0);
        t3d89_tick(&ctx, 1);
        i++;
    }
    check_true("ring_count_limited", t3d89_get_point_count(&ctx, id) <= 4);
    t3d89_tick(&ctx, 8);
    check_int("age_empty", t3d89_get_point_count(&ctx, id), 0);
}

static void test_transform_math(void)
{
    t3d89_transform transform;
    t3d89_sample local_sample;
    t3d89_sample world_sample;
    int rc;

    t3d89_transform_identity(&transform);
    transform.tx = t3d89_fp_from_int(10);
    transform.ty = t3d89_fp_from_int(20);
    transform.tz = t3d89_fp_from_int(30);
    transform.m00 = 0;
    transform.m01 = -T3D89_FP_ONE;
    transform.m10 = T3D89_FP_ONE;
    transform.m11 = 0;
    transform.sx = t3d89_fp_from_int(2);
    transform.sy = t3d89_fp_from_int(3);

    t3d89_sample_clear(&local_sample);
    local_sample.x = t3d89_fp_from_int(1);
    local_sample.y = t3d89_fp_from_int(2);
    local_sample.right_x = T3D89_FP_ONE;
    local_sample.up_y = T3D89_FP_ONE;
    local_sample.a_x = 0;
    local_sample.a_y = 0;
    local_sample.b_x = t3d89_fp_from_int(1);
    local_sample.b_y = 0;
    local_sample.width = T3D89_FP_ONE;
    local_sample.flags = T3D89_EMIT_HAS_ORIENT |
                         T3D89_EMIT_HAS_AB |
                         T3D89_EMIT_HAS_WIDTH;

    rc = t3d89_transform_sample(
        &transform,
        &local_sample,
        &world_sample,
        T3D89_PROVIDER_SCALE_WIDTH
    );
    check_int("transform_rc", rc, T3D89_OK);
    check_int("transform_x", world_sample.x, t3d89_fp_from_int(4));
    check_int("transform_y", world_sample.y, t3d89_fp_from_int(22));
    check_int("transform_z", world_sample.z, t3d89_fp_from_int(30));
    check_int("transform_right_x", world_sample.right_x, 0);
    check_int("transform_right_y", world_sample.right_y, t3d89_fp_from_int(2));
    check_int("transform_up_x", world_sample.up_x, -t3d89_fp_from_int(3));
    check_int("transform_up_y", world_sample.up_y, 0);
    check_int("transform_a_x", world_sample.a_x, t3d89_fp_from_int(10));
    check_int("transform_a_y", world_sample.a_y, t3d89_fp_from_int(20));
    check_int("transform_b_x", world_sample.b_x, t3d89_fp_from_int(10));
    check_int("transform_b_y", world_sample.b_y, t3d89_fp_from_int(22));
    check_int("transform_width", world_sample.width, t3d89_fp_from_int(3));
}

static void test_provider_auto_and_manual(void)
{
    t3d89_ctx ctx;
    t3d89_desc desc;
    t3d89_sample local_sample;
    provider_state state;
    t3d89_point *point;
    int id;
    int rc;

    t3d89_init(&ctx);
    t3d89_default_desc(&desc);
    desc.max_points = 16;
    desc.life_ticks = 100;
    desc.sampler_mask = T3D89_SAMPLE_FORCE;
    id = t3d89_create(&ctx, &desc);

    state.base_x = t3d89_fp_from_int(100);
    state.skip_tick = 2;
    state.fail_tick = 4;

    t3d89_sample_clear(&local_sample);
    local_sample.x = t3d89_fp_from_int(1);
    local_sample.y = t3d89_fp_from_int(2);
    local_sample.z = t3d89_fp_from_int(3);

    rc = t3d89_bind_transform_provider(
        &ctx,
        id,
        test_transform_provider,
        &state,
        &local_sample,
        T3D89_PROVIDER_AUTOSTEP
    );
    check_int("provider_bind", rc, T3D89_OK);

    rc = t3d89_tick(&ctx, 1);
    check_int("provider_tick_1", rc, T3D89_OK);
    check_int("provider_points_1", t3d89_get_point_count(&ctx, id), 1);
    point = &ctx.points[id][ctx.trails[id].start];
    check_int("provider_world_x", point->x, t3d89_fp_from_int(102));
    check_int("provider_world_y", point->y, t3d89_fp_from_int(2));
    check_int("provider_world_z", point->z, t3d89_fp_from_int(3));

    rc = t3d89_tick(&ctx, 1);
    check_int("provider_tick_skip", rc, T3D89_OK);
    check_int("provider_points_skip", t3d89_get_point_count(&ctx, id), 1);
    check_int("provider_skip_count", t3d89_get_provider_skip_count(&ctx, id), 1);

    rc = t3d89_tick(&ctx, 1);
    check_int("provider_tick_3", rc, T3D89_OK);
    check_int("provider_points_2", t3d89_get_point_count(&ctx, id), 2);
    check_int("provider_calls_3", t3d89_get_provider_call_count(&ctx, id), 3);

    rc = t3d89_tick(&ctx, 1);
    check_int("provider_tick_error", rc, T3D89_ERR_PROVIDER);
    check_int("provider_error_count", t3d89_get_provider_error_count(&ctx, id), 1);
    check_int("provider_last_raw_error", t3d89_get_provider_last_result(&ctx, id), -77);

    rc = t3d89_reset(&ctx, id);
    check_int("provider_reset", rc, T3D89_OK);
    check_int("provider_reset_points", t3d89_get_point_count(&ctx, id), 0);
    state.fail_tick = -1;
    state.skip_tick = -1;
    rc = t3d89_step_provider(&ctx, id);
    check_int("provider_manual_after_reset", rc, T3D89_OK);
    check_int("provider_manual_points", t3d89_get_point_count(&ctx, id), 1);

    rc = t3d89_unbind_transform_provider(&ctx, id);
    check_int("provider_unbind", rc, T3D89_OK);
    rc = t3d89_set_provider_enabled(&ctx, id, 1);
    check_int("provider_enable_unbound", rc, T3D89_ERR_PROVIDER);
}

int main(void)
{
    test_ribbon();
    test_cross_and_tube();
    test_beam_and_socket();
    test_sampling_and_lod();
    test_age_and_ring();
    test_transform_math();
    test_provider_auto_and_manual();
    if (fail_count != 0) {
        printf("TESTS FAILED count=%d\n", fail_count);
        return 1;
    }
    printf("ALL TESTS OK\n");
    return 0;
}
