#include <stdio.h>

#include "blank3d_casing_physics.h"

static Blank3DVPhysics g_physics;
static Blank3DCollision g_collision;
static Blank3DCasingPhysics g_casings;

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

static g3d_fix vec_abs_sum(Vec3 value)
{
    return g3d_fix_add_sat(g3d_fix_abs(value.x),
           g3d_fix_add_sat(g3d_fix_abs(value.y),
                           g3d_fix_abs(value.z)));
}

int main(void)
{
    int i;
    int saw_downward;
    int saw_rebound;
    int slept;
    Vec3 origin;
    Vec3 right;
    Vec3 up;
    Vec3 forward;
    Vec3 linear;
    Vec3 angular;
    g3d_fix initial_angular;
    g3d_fix late_angular;
    signed int matrix[4][4];

    blank3d_collision_init(&g_collision);
    if (!blank3d_vphysics_init(&g_physics, &g_collision))
        return fail(blank3d_vphysics_status(&g_physics));
    blank3d_casing_physics_init(&g_casings, &g_physics);

    origin = gamlib_vec3(0, G3D_FIX_FROM_INT(2),
                         G3D_FIX_FROM_INT(18));
    right = gamlib_vec3(G3D_FIX_ONE, 0, 0);
    up = gamlib_vec3(0, G3D_FIX_ONE, 0);
    forward = gamlib_vec3(0, 0, G3D_FIX_ONE);
    if (!blank3d_casing_physics_spawn(&g_casings, 0, 1, 1,
            &origin, &right, &up, &forward))
        return fail("casing spawn");
    if (!blank3d_casing_physics_get_velocity(&g_casings, 0,
            &linear, &angular))
        return fail("initial velocity");
    initial_angular = vec_abs_sum(angular);
    if (linear.y <= 0 || initial_angular <= 0)
        return fail("ejection impulse and tumble");

    saw_downward = 0;
    saw_rebound = 0;
    late_angular = initial_angular;
    for (i = 0; i < 600; ++i) {
        blank3d_collision_step(&g_collision);
        blank3d_vphysics_step_q12(&g_physics, GWP89_FIX_ONE / 60L);
        if (!blank3d_casing_physics_get_velocity(&g_casings, 0,
                &linear, &angular))
            return fail("velocity during step");
        if (linear.y < -G3D_FIX_ONE / 10L) saw_downward = 1;
        if (saw_downward && linear.y > G3D_FIX_ONE / 5L)
            saw_rebound = 1;
        if (i == 240) late_angular = vec_abs_sum(angular);
        if (blank3d_casing_physics_is_sleeping(&g_casings, 0)) break;
    }
    slept = blank3d_casing_physics_is_sleeping(&g_casings, 0);
    if (!saw_downward) return fail("casing never fell");
    if (!saw_rebound) return fail("casing did not rebound from floor");
    if (late_angular >= initial_angular)
        return fail("angular damping did not reduce tumble");
    if (!slept) return fail("casing did not settle/sleep");
    if (!blank3d_casing_physics_get_draw_matrix_q16(
            &g_casings, 0, G3D_FIX_ONE, matrix))
        return fail("visual pose matrix");
    if (matrix[3][3] != (signed int)VP_FX_ONE)
        return fail("visual matrix homogeneous row");

    blank3d_casing_physics_release(&g_casings, 0);
    if (blank3d_casing_physics_spawned(&g_casings) != 1UL ||
        blank3d_casing_physics_rejected(&g_casings) != 0UL)
        return fail("casing stats");

    printf("PASS: casing VPhysics rebound + angular damping + sleep in %d steps\n",
           i + 1);
    return 0;
}
