#include <stdio.h>
#include <string.h>

#include "blank3d_vphysics.h"

static Blank3DVPhysics g_physics;
static Blank3DCollision g_collision;

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

int main(void)
{
    int i;
    vpTransform transform;
    const Blank3DVPhysicsStats *stats;
    signed int matrix[4][4];

    blank3d_collision_init(&g_collision);
    if (!blank3d_vphysics_init(&g_physics, &g_collision))
        return fail(blank3d_vphysics_status(&g_physics));
    if (!blank3d_vphysics_get_transform_provider(&g_physics) ||
        !blank3d_vphysics_get_collision_provider(&g_physics) ||
        !blank3d_vphysics_get_total_solver(&g_physics) ||
        !blank3d_vphysics_get_world(&g_physics))
        return fail("provider getter contract");
    if (!blank3d_vphysics_get_transform_provider(&g_physics)->readWorld ||
        !blank3d_vphysics_get_transform_provider(&g_physics)->writeWorld ||
        !blank3d_vphysics_get_collision_provider(&g_physics)->generateContacts ||
        !blank3d_vphysics_get_collision_provider(&g_physics)->bodySweepTOI)
        return fail("provider getter callbacks");

    if (!blank3d_vphysics_create_dynamic_sphere_q12(&g_physics, 101UL,
            0, 5L * GWP89_FIX_ONE, 18L * GWP89_FIX_ONE,
            GWP89_FIX_ONE / 2L, GWP89_FIX_ONE))
        return fail("dynamic object creation");
    if (!blank3d_vphysics_create_dynamic_sphere_q12(&g_physics, 102UL,
            GWP89_FIX_ONE / 2L, 5L * GWP89_FIX_ONE,
            18L * GWP89_FIX_ONE,
            GWP89_FIX_ONE / 2L, GWP89_FIX_ONE))
        return fail("second dynamic object creation");
    if (!blank3d_vphysics_create_kinematic_box_q12(&g_physics, 103UL,
            0, 3L * GWP89_FIX_ONE, 20L * GWP89_FIX_ONE,
            GWP89_FIX_ONE / 2L, GWP89_FIX_ONE / 2L,
            GWP89_FIX_ONE / 2L, 0))
        return fail("kinematic object creation");

    if (!blank3d_vphysics_move_q12(&g_physics, 103UL,
            GWP89_FIX_ONE, 0, 0))
        return fail("provider move");
    if (!blank3d_vphysics_rotate_y_q12(&g_physics, 103UL,
            45L * GWP89_FIX_ONE))
        return fail("provider rotate");
    if (!blank3d_vphysics_scale_q12(&g_physics, 103UL,
            2L * GWP89_FIX_ONE,
            GWP89_FIX_ONE,
            GWP89_FIX_ONE / 2L))
        return fail("provider scale");

    for (i = 0; i < 240; ++i) {
        blank3d_collision_step(&g_collision);
        blank3d_vphysics_step_q12(&g_physics, GWP89_FIX_ONE / 60L);
    }

    if (!blank3d_vphysics_get_transform(&g_physics, 101UL, &transform))
        return fail("transform writeback");
    if (transform.position.y < (vp_fx)(VP_FX_ONE / 3L) ||
        transform.position.y > (vp_fx)(VP_FX_ONE * 2L))
        return fail("collision provider did not settle body near ground");

    if (!blank3d_vphysics_get_transform(&g_physics, 103UL, &transform))
        return fail("kinematic transform read");
    if (transform.position.x < (vp_fx)(VP_FX_ONE - VP_FX_ONE / 32L))
        return fail("move did not persist through transform provider");
    if (transform.scale.x != (vp_fx)(2L * VP_FX_ONE) ||
        transform.scale.z != (vp_fx)(VP_FX_ONE / 2L))
        return fail("scale did not persist through transform provider");

    if (!blank3d_vphysics_get_draw_matrix_q16(&g_physics, 103UL, matrix))
        return fail("draw matrix");
    if (matrix[0][3] < (signed int)(VP_FX_ONE - VP_FX_ONE / 32L))
        return fail("draw matrix translation");

    stats = blank3d_vphysics_stats(&g_physics);
    if (!stats || stats->steps == 0UL)
        return fail("solver step stats");
    if (stats->transform_reads == 0UL || stats->transform_writes == 0UL)
        return fail("transform provider callbacks were not used");
    if (stats->contacts_total == 0UL)
        return fail("collision provider generated no contacts");
    if (stats->sweep_queries == 0UL)
        return fail("CCD sweep provider was not queried");

    printf("PASS: VPhysics providers steps=%lu reads=%lu writes=%lu contacts=%lu sweeps=%lu hits=%lu\n",
        (unsigned long)stats->steps,
        (unsigned long)stats->transform_reads,
        (unsigned long)stats->transform_writes,
        (unsigned long)stats->contacts_total,
        (unsigned long)stats->sweep_queries,
        (unsigned long)stats->sweep_hits);
    return 0;
}
