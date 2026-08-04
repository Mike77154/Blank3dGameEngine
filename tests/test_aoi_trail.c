#include "trail3d89.h"
#include "trail3d89_profiles.h"

#include <stdio.h>
#include <string.h>

#define TEST_VERTEX_CAP 512
#define TEST_INDEX_CAP 1536

int main(void)
{
    t3d89_ctx ctx;
    t3d89_desc desc;
    t3d89_camera camera;
    t3d89_mesh mesh;
    t3d89_vertex vertices[TEST_VERTEX_CAP];
    unsigned short indices[TEST_INDEX_CAP];
    int trail_id;
    int i;

    memset(&camera, 0, sizeof(camera));
    camera.y = t3d89_fp_from_int(2);
    camera.up_y = T3D89_FP_ONE;

    t3d89_init(&ctx);
    t3d89_profile_bullet(&desc);
    desc.max_points = 32;
    desc.min_ticks = 1;
    trail_id = t3d89_create(&ctx, &desc);
    if (trail_id < 0) return 1;

    for (i = 0; i < 12; ++i) {
        if (t3d89_emit_point(&ctx, trail_id,
                             t3d89_fp_from_int(i),
                             t3d89_fp_from_int(1),
                             t3d89_fp_from_int(i * 2)) != T3D89_OK)
            return 2;
        (void)t3d89_tick(&ctx, 1);
    }
    if (t3d89_get_point_count(&ctx, trail_id) < 3) return 3;

    t3d89_mesh_clear(&mesh);
    mesh.vertices = vertices;
    mesh.indices = indices;
    mesh.max_vertices = TEST_VERTEX_CAP;
    mesh.max_indices = TEST_INDEX_CAP;
    if (t3d89_build_mesh(&ctx, trail_id, &camera, &mesh) != T3D89_OK)
        return 4;
    if (mesh.vertex_count < 4 || mesh.index_count < 6) return 5;
    if (mesh.overflowed) return 6;

    printf("Blank3D Aoi Trail3D native mesh test: OK (%d vertices, %d indices)\n",
           mesh.vertex_count, mesh.index_count);
    return 0;
}
