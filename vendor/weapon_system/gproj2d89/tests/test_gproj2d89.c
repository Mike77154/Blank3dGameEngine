#include <stdio.h>
#include "gproj2d89.h"

#define TEST_PATHS 512
#define TEST_POINTS 8192

static gp2d_path test_paths[TEST_PATHS];
static gp2d_vec2 test_points[TEST_POINTS];
static int failures = 0;

static void check(int condition, const char *message)
{
    if (condition) {
        printf("PASS: %s\n", message);
    } else {
        printf("FAIL: %s\n", message);
        ++failures;
    }
}

static void test_catalog(void)
{
    gp2d_scene scene;
    gp2d_transform transform;
    gp2d_draw_options options;
    const gp2d_profile *profile;
    unsigned short i;
    int status;

    gp2d_scene_init(&scene, test_paths, TEST_PATHS,
                    test_points, TEST_POINTS);
    transform = gp2d_transform_identity();
    transform.scale.x = gp2d_fx_from_int(64L);
    transform.scale.y = gp2d_fx_from_int(64L);
    options = gp2d_draw_options_default();

    check(gp2d_profile_count() == GP2D_AMMO_COUNT,
          "catalog exposes every profile");

    for (i = 0U; i < gp2d_profile_count(); ++i) {
        profile = gp2d_profile_at(i);
        check(profile != 0, "profile lookup by index");
        if (profile == 0) {
            continue;
        }
        gp2d_scene_reset(&scene);
        status = gp2d_build_part(&scene, profile->ammo_id,
                                 GP2D_PART_PROJECTILE,
                                 &transform, &options);
        check(status == GP2D_OK, "projectile build succeeds");
        check(scene.path_count > 0U && scene.point_count > 0U,
              "projectile emits vector data");
        check(gp2d_scene_validate(&scene) == GP2D_OK,
              "projectile scene validates");

        if (gp2d_profile_has_part(profile->ammo_id, GP2D_PART_SHELL)) {
            gp2d_scene_reset(&scene);
            status = gp2d_build_part(&scene, profile->ammo_id,
                                     GP2D_PART_SHELL,
                                     &transform, &options);
            check(status == GP2D_OK, "shell build succeeds");
            check(scene.path_count > 0U && scene.point_count > 0U,
                  "shell emits vector data");
            check(gp2d_scene_validate(&scene) == GP2D_OK,
                  "shell scene validates");
        }

        gp2d_scene_reset(&scene);
        status = gp2d_build_part(&scene, profile->ammo_id,
                                 GP2D_PART_COMPLETE,
                                 &transform, &options);
        check(status == GP2D_OK, "complete icon build succeeds");
        check(gp2d_scene_validate(&scene) == GP2D_OK,
              "complete icon validates");
    }
}

static void test_style_controls(void)
{
    gp2d_scene scene;
    gp2d_transform transform;
    gp2d_draw_options options;
    gp2d_color pink;
    gp2d_color black;
    unsigned short i;
    int status;
    int found_fill;

    gp2d_scene_init(&scene, test_paths, TEST_PATHS,
                    test_points, TEST_POINTS);
    transform = gp2d_transform_identity();
    options = gp2d_draw_options_default();
    options.outline_enabled = GP2D_FALSE;
    options.override_mask = GP2D_OVERRIDE_FILL;
    options.fill_override = gp2d_color_rgba(240U, 72U, 156U, 255U);
    status = gp2d_build_part(&scene, GP2D_AMMO_SNIPER,
                             GP2D_PART_COMPLETE, &transform, &options);
    check(status == GP2D_OK, "custom color build succeeds");
    found_fill = 0;
    for (i = 0U; i < scene.path_count; ++i) {
        if (scene.paths[i].style.fill_enabled) {
            found_fill = 1;
            check(scene.paths[i].style.fill.r == 240U,
                  "fill override reaches filled paths");
        }
        check(scene.paths[i].style.outline_enabled == GP2D_FALSE,
              "outline can be disabled");
    }
    check(found_fill, "custom color test has filled paths");

    pink = gp2d_color_rgba(255U, 80U, 180U, 255U);
    gp2d_scene_recolor_role(&scene, GP2D_ROLE_BAND, pink);
    black = gp2d_color_rgba(0U, 0U, 0U, 255U);
    gp2d_scene_set_outline(&scene, GP2D_TRUE, black,
                           gp2d_fx_from_ratio(1L, 16L));
    for (i = 0U; i < scene.path_count; ++i) {
        check(scene.paths[i].style.outline_enabled == GP2D_TRUE,
              "post-build outline enable works");
    }
}

static void test_shotgun_shell_blocks(void)
{
    gp2d_scene scene;
    gp2d_transform transform;
    gp2d_draw_options options;
    const gp2d_path *hull;
    const gp2d_path *base;
    unsigned short i;
    int status;

    gp2d_scene_init(&scene, test_paths, TEST_PATHS,
                    test_points, TEST_POINTS);
    transform = gp2d_transform_identity();
    options = gp2d_draw_options_default();
    status = gp2d_build_part(&scene, GP2D_AMMO_SHOTGUN_BUCKSHOT,
                             GP2D_PART_SHELL, &transform, &options);
    check(status == GP2D_OK, "shotgun block shell build succeeds");

    hull = 0;
    base = 0;
    for (i = 0U; i < scene.path_count; ++i) {
        if (scene.paths[i].role == GP2D_ROLE_HULL) {
            hull = &scene.paths[i];
        } else if (scene.paths[i].role == GP2D_ROLE_SHELL_RIM) {
            base = &scene.paths[i];
        }
    }
    check(hull != 0, "shotgun shell has red hull path");
    check(base != 0, "shotgun shell has yellow base path");
    if (hull != 0) {
        check(hull->point_count == 4U,
              "shotgun hull is emitted as a four-point rectangle");
        check(hull->style.fill.r == 214U &&
              hull->style.fill.g == 42U &&
              hull->style.fill.b == 42U,
              "shotgun hull default fill is red");
    }
    if (base != 0) {
        check(base->point_count == 4U,
              "shotgun base is emitted as a four-point rectangle");
        check(base->style.fill.r == 255U &&
              base->style.fill.g == 211U &&
              base->style.fill.b == 48U,
              "shotgun base default fill is yellow");
    }
}

static void test_capacity_and_math(void)
{
    gp2d_path tiny_paths[1];
    gp2d_vec2 tiny_points[3];
    gp2d_scene scene;
    gp2d_transform transform;
    gp2d_draw_options options;
    gp2d_fx a;
    gp2d_fx b;
    gp2d_fx product;
    int status;

    a = gp2d_fx_from_ratio(3L, 2L);
    b = gp2d_fx_from_int(2L);
    product = gp2d_fx_mul(a, b);
    check(gp2d_fx_to_int(product) == 3L,
          "fixed-point multiply produces expected integer");

    gp2d_scene_init(&scene, tiny_paths, 1U, tiny_points, 3U);
    transform = gp2d_transform_identity();
    options = gp2d_draw_options_default();
    status = gp2d_build_part(&scene, GP2D_AMMO_HAND_GRENADE,
                             GP2D_PART_PROJECTILE,
                             &transform, &options);
    check(status == GP2D_ERR_CAPACITY,
          "capacity failure is reported before overflow");
    check(scene.path_count <= scene.path_capacity,
          "path count remains bounded after capacity failure");
    check(scene.point_count <= scene.point_capacity,
          "point count remains bounded after capacity failure");
}

int main(void)
{
    printf("GProj2D89 tests\n");
    test_catalog();
    test_style_controls();
    test_shotgun_shell_blocks();
    test_capacity_and_math();
    if (failures != 0) {
        printf("FAILURES: %d\n", failures);
        return 1;
    }
    printf("ALL TESTS PASSED\n");
    return 0;
}
