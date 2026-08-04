#include <stdio.h>

#include "blank3d_automotion.h"

static int failures = 0;

static void expect_true(int condition, const char *name)
{
    if (!condition) {
        printf("FAIL: %s\n", name);
        failures += 1;
    }
}

static void test_archetype_and_flat_pattern(void)
{
    Blank3DAutomotion motion;
    Vec3 current;
    Vec3 target;
    Vec3 output;
    int reached;

    blank3d_automotion_init_archetype(&motion, "hopper_enemy");
    expect_true(blank3d_automotion_style_name(&motion)[0] == 'z',
                "hopper defaults to zigzag");

    current = gamlib_vec3(0, G3D_FIX_FROM_INT(3), 0);
    target = gamlib_vec3(G3D_FIX_FROM_INT(12),
                         G3D_FIX_FROM_INT(20), 0);
    reached = 0;
    expect_true(blank3d_automotion_step_pattern(&motion,
        &current, &target, G3D_FIX_FROM_INT(8),
        G3D_FIX_ONE / 4, 1, &output, &reached),
        "flat zigzag step valid");
    expect_true(output.y == current.y, "flat pattern preserves Y");
    expect_true(output.x != current.x || output.z != current.z,
                "flat pattern moves XZ");
}

static void test_style_switch_and_direct(void)
{
    Blank3DAutomotion motion;
    Vec3 current;
    Vec3 target;
    Vec3 output;
    int reached;

    blank3d_automotion_init(&motion);
    expect_true(blank3d_automotion_set_style(&motion, "helix"),
                "helix style accepted");
    expect_true(blank3d_automotion_style_name(&motion)[0] == 'h',
                "helix style selected");

    current = gamlib_vec3(0, 0, 0);
    target = gamlib_vec3(G3D_FIX_FROM_INT(2), 0, 0);
    reached = 0;
    expect_true(blank3d_automotion_step_direct(&motion,
        &current, &target, G3D_FIX_FROM_INT(10), G3D_FIX_ONE,
        0, &output, &reached), "direct step valid");
    expect_true(reached, "direct step reaches nearby target");
}

static void test_move_away(void)
{
    Blank3DAutomotion motion;
    Vec3 current;
    Vec3 threat;
    Vec3 fallback;
    Vec3 output;
    int reached;

    blank3d_automotion_init(&motion);
    current = gamlib_vec3(G3D_FIX_FROM_INT(1), 0, 0);
    threat = gamlib_vec3(0, 0, 0);
    fallback = gamlib_vec3(G3D_FIX_ONE, 0, 0);
    reached = 0;
    expect_true(blank3d_automotion_step_away(&motion,
        &current, &threat, &fallback, G3D_FIX_FROM_INT(4),
        G3D_FIX_FROM_INT(6), G3D_FIX_ONE / 2, 1,
        &output, &reached), "away step valid");
    expect_true(output.x > current.x, "away step increases separation");
    expect_true(output.y == current.y, "away flat preserves Y");
}

int main(void)
{
    test_archetype_and_flat_pattern();
    test_style_switch_and_direct();
    test_move_away();

    if (failures != 0) {
        printf("%d automotion test(s) failed\n", failures);
        return 1;
    }
    puts("automotion bridge tests passed");
    return 0;
}
