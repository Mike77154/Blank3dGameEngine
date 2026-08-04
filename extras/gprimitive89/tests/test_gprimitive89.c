#include "gprimitive89.h"

#include <stdio.h>

#define TEST_VERTICES 20000U
#define TEST_TRIANGLES 40000U

static gp89_vertex vertices[TEST_VERTICES];
static gp89_triangle triangles[TEST_TRIANGLES];

static int failures = 0;

static void check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        failures += 1;
    }
}

static void validate_mesh(const gp89_mesh *mesh, const char *name)
{
    unsigned int i;
    const gp89_triangle *triangle;

    check(mesh->status == GP89_OK, name);
    check(mesh->vertex_count > 0U, "mesh has vertices");
    check(mesh->triangle_count > 0U, "mesh has triangles");
    check(mesh->vertex_count <= mesh->vertex_capacity, "vertex capacity respected");
    check(mesh->triangle_count <= mesh->triangle_capacity, "triangle capacity respected");
    for (i = 0U; i < mesh->triangle_count; ++i) {
        triangle = &mesh->triangles[i];
        check((unsigned int)triangle->a < mesh->vertex_count, "index a valid");
        check((unsigned int)triangle->b < mesh->vertex_count, "index b valid");
        check((unsigned int)triangle->c < mesh->vertex_count, "index c valid");
        check(triangle->a != triangle->b, "triangle a b distinct");
        check(triangle->b != triangle->c, "triangle b c distinct");
        check(triangle->a != triangle->c, "triangle a c distinct");
    }
}

static void run_shape_tests(void)
{
    gp89_mesh mesh;
    int kind;
    gp89_rgba low_color;
    gp89_rgba high_color;

    gp89_mesh_init(&mesh, vertices, TEST_VERTICES, triangles, TEST_TRIANGLES);

    check(gp89_make_triangle(&mesh, gp89_fx_from_int(2), gp89_fx_from_int(2)) == GP89_OK, "triangle build");
    validate_mesh(&mesh, "triangle");
    check(mesh.vertex_count == 3U && mesh.triangle_count == 1U, "triangle counts");

    check(gp89_make_quad(&mesh, gp89_fx_from_int(2), gp89_fx_from_int(2)) == GP89_OK, "quad build");
    validate_mesh(&mesh, "quad");
    check(mesh.vertex_count == 4U && mesh.triangle_count == 2U, "quad counts");

    check(gp89_make_plane(&mesh, gp89_fx_from_int(4), gp89_fx_from_int(3), 8U, 6U) == GP89_OK, "plane build");
    validate_mesh(&mesh, "plane");

    check(gp89_make_disc(&mesh, gp89_fx_from_int(1), 24U) == GP89_OK, "disc build");
    validate_mesh(&mesh, "disc");

    check(gp89_make_annulus(&mesh, GP89_FX_HALF, gp89_fx_from_int(1), 24U) == GP89_OK, "annulus build");
    validate_mesh(&mesh, "annulus");

    check(gp89_make_cube(&mesh, gp89_fx_from_int(2)) == GP89_OK, "cube build");
    validate_mesh(&mesh, "cube");

    check(gp89_make_box(&mesh, gp89_fx_from_int(2), gp89_fx_from_int(3), gp89_fx_from_int(4)) == GP89_OK, "box build");
    validate_mesh(&mesh, "box");

    check(gp89_make_uv_sphere(&mesh, gp89_fx_from_int(1), 32U, 16U) == GP89_OK, "sphere build");
    validate_mesh(&mesh, "sphere");

    check(gp89_make_capsule(&mesh, GP89_FX_HALF, gp89_fx_from_int(2), 32U, 8U) == GP89_OK, "capsule build");
    validate_mesh(&mesh, "capsule");

    check(gp89_make_spherocylinder(&mesh, GP89_FX_HALF, gp89_fx_from_int(2), 32U, 8U) == GP89_OK, "spherocylinder alias build");
    validate_mesh(&mesh, "spherocylinder alias");

    check(gp89_make_rounded_cylinder(&mesh, GP89_FX_HALF, gp89_fx_from_int(2), 32U, 8U) == GP89_OK, "rounded cylinder alias build");
    validate_mesh(&mesh, "rounded cylinder alias");

    check(gp89_make_cylinder(&mesh, gp89_fx_from_int(1), gp89_fx_from_int(2), 24U, GP89_CAP_BOTH) == GP89_OK, "cylinder build");
    validate_mesh(&mesh, "cylinder");

    check(gp89_make_cone(&mesh, gp89_fx_from_int(1), gp89_fx_from_int(2), 24U, 1) == GP89_OK, "cone build");
    validate_mesh(&mesh, "cone");

    check(gp89_make_frustum(&mesh, gp89_fx_from_int(1), GP89_FX_HALF, gp89_fx_from_int(2), 24U, GP89_CAP_BOTH) == GP89_OK, "frustum build");
    validate_mesh(&mesh, "frustum");

    check(gp89_make_torus(&mesh, gp89_fx_from_int(1), GP89_FX_HALF, 32U, 16U) == GP89_OK, "torus build");
    validate_mesh(&mesh, "torus");

    check(gp89_make_prism(&mesh, 7U, gp89_fx_from_int(1), gp89_fx_from_int(2), GP89_CAP_BOTH) == GP89_OK, "prism build");
    validate_mesh(&mesh, "prism");

    check(gp89_make_pyramid(&mesh, 3U, gp89_fx_from_int(1), gp89_fx_from_int(2), 1) == GP89_OK, "triangular pyramid build");
    validate_mesh(&mesh, "triangular pyramid");

    check(gp89_make_pyramid(&mesh, 4U, gp89_fx_from_int(1), gp89_fx_from_int(2), 1) == GP89_OK, "quadrangular pyramid build");
    validate_mesh(&mesh, "quadrangular pyramid");

    check(gp89_make_antiprism(&mesh, 6U, gp89_fx_from_int(1), gp89_fx_from_int(2), GP89_CAP_BOTH) == GP89_OK, "antiprism build");
    validate_mesh(&mesh, "antiprism");

    check(gp89_make_bipyramid(&mesh, 5U, gp89_fx_from_int(1), gp89_fx_from_int(1), gp89_fx_from_int(1)) == GP89_OK, "bipyramid build");
    validate_mesh(&mesh, "bipyramid");

    check(gp89_make_wedge(&mesh, gp89_fx_from_int(2), gp89_fx_from_int(2), gp89_fx_from_int(2)) == GP89_OK, "wedge build");
    validate_mesh(&mesh, "wedge");

    for (kind = GP89_POLY_TETRAHEDRON; kind <= GP89_POLY_TRUNCATED_OCTAHEDRON; ++kind) {
        check(gp89_make_regular_polyhedron(&mesh, kind, gp89_fx_from_int(1)) == GP89_OK, "polyhedron build");
        validate_mesh(&mesh, "polyhedron");
    }

    low_color.r = 10U;
    low_color.g = 20U;
    low_color.b = 30U;
    low_color.a = 255U;
    high_color.r = 240U;
    high_color.g = 220U;
    high_color.b = 200U;
    high_color.a = 255U;
    gp89_color_axis_gradient(&mesh, GP89_AXIS_Y, low_color, high_color);
    gp89_material_cycle(&mesh, 5U);
    gp89_uv_transform(&mesh, gp89_fx_from_int(2), gp89_fx_from_int(2), 0, 0);
    gp89_scale(&mesh, gp89_fx_from_int(2), GP89_FX_ONE, GP89_FX_HALF);
    gp89_rotate_xyz(&mesh, 1000U, 2000U, 3000U);
    gp89_translate(&mesh, GP89_FX_HALF, 0, -GP89_FX_HALF);
    gp89_deform_taper_y(&mesh, GP89_FX_HALF, gp89_fx_from_int(2));
    gp89_deform_twist_y(&mesh, GP89_QUARTER_TURN);
    gp89_deform_shear(&mesh, GP89_FX_ONE / 8, -GP89_FX_ONE / 8);
    gp89_deform_spherize(&mesh, gp89_fx_from_int(2), GP89_FX_HALF);
    gp89_deform_wave_y(&mesh, GP89_FX_ONE / 8, GP89_FX_ONE / 4, GP89_FX_ONE / 8);
    validate_mesh(&mesh, "processed mesh");
}

static void run_math_tests(void)
{
    gp89_fx s;
    gp89_fx c;

    check(gp89_fx_mul(gp89_fx_from_int(2), gp89_fx_from_int(3)) == gp89_fx_from_int(6), "fixed multiply");
    check(gp89_fx_div(gp89_fx_from_int(6), gp89_fx_from_int(3)) == gp89_fx_from_int(2), "fixed divide");
    check(gp89_fx_sqrt(gp89_fx_from_int(4)) >= gp89_fx_from_int(2) - 2 &&
          gp89_fx_sqrt(gp89_fx_from_int(4)) <= gp89_fx_from_int(2) + 2, "fixed sqrt");
    gp89_sincos(0U, &s, &c);
    check(s > -16 && s < 16, "sin zero");
    check(c > GP89_FX_ONE - 32 && c < GP89_FX_ONE + 32, "cos zero");
    gp89_sincos(GP89_QUARTER_TURN, &s, &c);
    check(s > GP89_FX_ONE - 32 && s < GP89_FX_ONE + 32, "sin quarter");
    check(c > -32 && c < 32, "cos quarter");
}

int main(void)
{
    run_math_tests();
    run_shape_tests();
    if (failures != 0) {
        fprintf(stderr, "%d test failures\n", failures);
        return 1;
    }
    puts("gprimitive89: all tests passed");
    return 0;
}
