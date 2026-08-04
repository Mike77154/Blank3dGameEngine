#include "gprimitive89.h"
#include "gprimitive89_obj.h"

#include <stdio.h>
#include <string.h>

#define PREVIEW_VERTEX_CAPACITY 20000U
#define PREVIEW_TRIANGLE_CAPACITY 40000U

static gp89_vertex preview_vertices[PREVIEW_VERTEX_CAPACITY];
static gp89_triangle preview_triangles[PREVIEW_TRIANGLE_CAPACITY];

static int save_mesh(const char *directory,
                     const char *name,
                     const gp89_mesh *mesh)
{
    char path[512];
    FILE *file;
    int result;

    sprintf(path, "%s/%s.obj", directory, name);
    file = fopen(path, "w");
    if (file == (FILE *)0) {
        fprintf(stderr, "cannot open %s\n", path);
        return 1;
    }
    result = gp89_write_obj(file, mesh, name);
    fclose(file);
    if (result != GP89_OK) {
        fprintf(stderr, "cannot write %s\n", path);
        return 1;
    }
    printf("%-24s %5u vertices %5u triangles\n",
           name, mesh->vertex_count, mesh->triangle_count);
    return 0;
}

#define BUILD_AND_SAVE(call_expression, mesh_name) \
    do { \
        if ((call_expression) != GP89_OK) { \
            fprintf(stderr, "build failed: %s\n", mesh_name); \
            return 1; \
        } \
        if (save_mesh(output_directory, mesh_name, &mesh) != 0) { \
            return 1; \
        } \
    } while (0)

int main(int argc, char **argv)
{
    gp89_mesh mesh;
    const char *output_directory;

    output_directory = argc > 1 ? argv[1] : ".";
    gp89_mesh_init(&mesh,
                   preview_vertices,
                   PREVIEW_VERTEX_CAPACITY,
                   preview_triangles,
                   PREVIEW_TRIANGLE_CAPACITY);

    BUILD_AND_SAVE(gp89_make_triangle(&mesh, gp89_fx_from_int(2), gp89_fx_from_int(2)), "triangle");
    BUILD_AND_SAVE(gp89_make_quad(&mesh, gp89_fx_from_int(2), gp89_fx_from_int(2)), "quad");
    BUILD_AND_SAVE(gp89_make_plane(&mesh, gp89_fx_from_int(2), gp89_fx_from_int(2), 5U, 5U), "plane_grid");
    BUILD_AND_SAVE(gp89_make_annulus(&mesh, GP89_FX_HALF, gp89_fx_from_int(1), 32U), "annulus");
    BUILD_AND_SAVE(gp89_make_cube(&mesh, gp89_fx_from_int(2)), "cube");
    BUILD_AND_SAVE(gp89_make_uv_sphere(&mesh, gp89_fx_from_int(1), 28U, 14U), "uv_sphere");
    BUILD_AND_SAVE(gp89_make_capsule(&mesh, GP89_FX_HALF, gp89_fx_from_int(2), 28U, 7U), "capsule");
    BUILD_AND_SAVE(gp89_make_cylinder(&mesh, gp89_fx_from_int(1), gp89_fx_from_int(2), 28U, GP89_CAP_BOTH), "cylinder");
    BUILD_AND_SAVE(gp89_make_cone(&mesh, gp89_fx_from_int(1), gp89_fx_from_int(2), 28U, 1), "cone");
    BUILD_AND_SAVE(gp89_make_frustum(&mesh, gp89_fx_from_int(1), GP89_FX_HALF, gp89_fx_from_int(2), 28U, GP89_CAP_BOTH), "frustum");
    BUILD_AND_SAVE(gp89_make_torus(&mesh, gp89_fx_from_int(1), GP89_FX_ONE / 3, 28U, 14U), "torus");
    BUILD_AND_SAVE(gp89_make_pyramid(&mesh, 3U, gp89_fx_from_int(1), gp89_fx_from_int(2), 1), "pyramid_3");
    BUILD_AND_SAVE(gp89_make_pyramid(&mesh, 4U, gp89_fx_from_int(1), gp89_fx_from_int(2), 1), "pyramid_4");
    BUILD_AND_SAVE(gp89_make_prism(&mesh, 6U, gp89_fx_from_int(1), gp89_fx_from_int(2), GP89_CAP_BOTH), "prism_6");
    BUILD_AND_SAVE(gp89_make_antiprism(&mesh, 5U, gp89_fx_from_int(1), gp89_fx_from_int(2), GP89_CAP_BOTH), "antiprism_5");
    BUILD_AND_SAVE(gp89_make_bipyramid(&mesh, 5U, gp89_fx_from_int(1), gp89_fx_from_int(1), gp89_fx_from_int(1)), "bipyramid_5");
    BUILD_AND_SAVE(gp89_make_wedge(&mesh, gp89_fx_from_int(2), gp89_fx_from_int(2), gp89_fx_from_int(2)), "wedge");
    BUILD_AND_SAVE(gp89_make_regular_polyhedron(&mesh, GP89_POLY_TETRAHEDRON, gp89_fx_from_int(1)), "tetrahedron");
    BUILD_AND_SAVE(gp89_make_regular_polyhedron(&mesh, GP89_POLY_OCTAHEDRON, gp89_fx_from_int(1)), "octahedron");
    BUILD_AND_SAVE(gp89_make_regular_polyhedron(&mesh, GP89_POLY_DODECAHEDRON, gp89_fx_from_int(1)), "dodecahedron");
    BUILD_AND_SAVE(gp89_make_regular_polyhedron(&mesh, GP89_POLY_ICOSAHEDRON, gp89_fx_from_int(1)), "icosahedron");
    BUILD_AND_SAVE(gp89_make_regular_polyhedron(&mesh, GP89_POLY_CUBOCTAHEDRON, gp89_fx_from_int(1)), "cuboctahedron");
    BUILD_AND_SAVE(gp89_make_regular_polyhedron(&mesh, GP89_POLY_RHOMBIC_DODECAHEDRON, gp89_fx_from_int(1)), "rhombic_dodecahedron");
    BUILD_AND_SAVE(gp89_make_regular_polyhedron(&mesh, GP89_POLY_TRUNCATED_OCTAHEDRON, gp89_fx_from_int(1)), "truncated_octahedron");

    return 0;
}
