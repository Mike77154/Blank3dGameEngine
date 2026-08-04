#include "trail3d89.h"
#include <stdio.h>

#define DEMO_VERTS 2048
#define DEMO_INDICES 4096

static t3d89_vertex demo_vertices[DEMO_VERTS];
static unsigned short demo_indices[DEMO_INDICES];

static void setup_mesh(t3d89_mesh *mesh)
{
    mesh->vertices = demo_vertices;
    mesh->indices = demo_indices;
    mesh->max_vertices = DEMO_VERTS;
    mesh->max_indices = DEMO_INDICES;
    mesh->vertex_count = 0;
    mesh->index_count = 0;
    mesh->flags = 0;
    mesh->overflowed = 0;
}

static t3d89_camera setup_camera(void)
{
    t3d89_camera cam;
    cam.x = t3d89_fp_from_int(0);
    cam.y = t3d89_fp_from_int(3);
    cam.z = t3d89_fp_from_int(9);
    cam.up_x = 0;
    cam.up_y = T3D89_FP_ONE;
    cam.up_z = 0;
    return cam;
}

int main(void)
{
    t3d89_ctx ctx;
    t3d89_desc bullet;
    t3d89_desc dash;
    t3d89_desc sword;
    t3d89_mesh mesh;
    t3d89_camera cam;
    int bullet_id;
    int dash_id;
    int sword_id;
    int i;
    int rc;

    t3d89_init(&ctx);
    cam = setup_camera();

    t3d89_default_desc(&bullet);
    bullet.mode = T3D89_MODE_VIEW_RIBBON;
    bullet.life_ticks = 10;
    bullet.min_dist = T3D89_FP_ONE / 4;
    bullet.width_head = T3D89_FP_ONE / 8;
    bullet.width_tail = 0;
    bullet.color_head = t3d89_color_make(255, 240, 64, 230);
    bullet.color_tail = t3d89_color_make(255, 80, 0, 0);
    bullet_id = t3d89_create(&ctx, &bullet);

    t3d89_default_desc(&dash);
    dash.mode = T3D89_MODE_AXIS_RIBBON;
    dash.life_ticks = 16;
    dash.min_dist = T3D89_FP_ONE / 3;
    dash.width_head = T3D89_FP_ONE;
    dash.width_tail = T3D89_FP_ONE / 8;
    dash.axis_x = 0;
    dash.axis_y = T3D89_FP_ONE;
    dash.axis_z = 0;
    dash.color_head = t3d89_color_make(64, 180, 255, 180);
    dash.color_tail = t3d89_color_make(64, 80, 255, 0);
    dash_id = t3d89_create(&ctx, &dash);

    t3d89_default_desc(&sword);
    sword.mode = T3D89_MODE_SOCKET_SWEEP;
    sword.life_ticks = 8;
    sword.sampler_mask = T3D89_SAMPLE_FORCE;
    sword.width_head = T3D89_FP_ONE / 4;
    sword.color_head = t3d89_color_make(255, 255, 255, 200);
    sword.color_tail = t3d89_color_make(100, 200, 255, 0);
    sword_id = t3d89_create(&ctx, &sword);

    i = 0;
    while (i < 12) {
        t3d89_emit_point(&ctx, bullet_id, t3d89_fp_from_int(i), t3d89_fp_from_int(1), 0);
        t3d89_emit_point(&ctx, dash_id, t3d89_fp_from_int(i / 2), 0, t3d89_fp_from_int(i / 3));
        t3d89_emit_segment(&ctx, sword_id,
                           t3d89_fp_from_int(0), t3d89_fp_from_int(i / 2), 0,
                           t3d89_fp_from_int(1), t3d89_fp_from_int(i / 2), t3d89_fp_from_int(1));
        t3d89_tick(&ctx, 1);
        i++;
    }

    setup_mesh(&mesh);
    rc = t3d89_build_all(&ctx, &cam, &mesh);
    printf("rc=%d vertices=%d indices=%d overflow=%d\n", rc, mesh.vertex_count, mesh.index_count, mesh.overflowed);
    printf("bullet_points=%d dash_points=%d sword_points=%d\n",
           t3d89_get_point_count(&ctx, bullet_id),
           t3d89_get_point_count(&ctx, dash_id),
           t3d89_get_point_count(&ctx, sword_id));
    return 0;
}
