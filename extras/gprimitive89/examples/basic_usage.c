#include "gprimitive89.h"
#include "gprimitive89_obj.h"

#include <stdio.h>

#define EXAMPLE_VERTEX_CAPACITY 4096U
#define EXAMPLE_TRIANGLE_CAPACITY 8192U

static gp89_vertex example_vertices[EXAMPLE_VERTEX_CAPACITY];
static gp89_triangle example_triangles[EXAMPLE_TRIANGLE_CAPACITY];

int main(void)
{
    gp89_mesh mesh;
    gp89_rgba bottom;
    gp89_rgba top;
    FILE *file;

    gp89_mesh_init(&mesh,
                   example_vertices,
                   EXAMPLE_VERTEX_CAPACITY,
                   example_triangles,
                   EXAMPLE_TRIANGLE_CAPACITY);

    if (gp89_make_capsule(&mesh,
                          GP89_FX_ONE / 2,
                          gp89_fx_from_int(2),
                          32U,
                          8U) != GP89_OK) {
        fprintf(stderr, "capsule generation failed: %d\n", mesh.status);
        return 1;
    }

    bottom.r = 42U;
    bottom.g = 96U;
    bottom.b = 220U;
    bottom.a = 255U;
    top.r = 244U;
    top.g = 72U;
    top.b = 160U;
    top.a = 255U;
    gp89_color_axis_gradient(&mesh, GP89_AXIS_Y, bottom, top);

    gp89_deform_twist_y(&mesh, GP89_QUARTER_TURN / 4U);
    gp89_uv_transform(&mesh,
                      gp89_fx_from_int(2),
                      GP89_FX_ONE,
                      0,
                      0);

    file = fopen("capsule_example.obj", "w");
    if (file == (FILE *)0) {
        fputs("cannot create capsule_example.obj\n", stderr);
        return 1;
    }
    if (gp89_write_obj(file, &mesh, "capsule_example") != GP89_OK) {
        fclose(file);
        fputs("OBJ export failed\n", stderr);
        return 1;
    }
    fclose(file);

    printf("generated %u vertices and %u triangles\n",
           mesh.vertex_count,
           mesh.triangle_count);
    return 0;
}
