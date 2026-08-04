#include "blank3d_vertical_axis.h"

#include <stdio.h>
#include <string.h>

typedef struct TestPhysicsTag {
    int moves;
} TestPhysics;

static int test_physics_move(void *user, void *actor,
                             const vm89_vec3 *desired,
                             int clamp_to_floor,
                             vm89_scalar floor_y,
                             vm89_vec3 *out_resolved)
{
    TestPhysics *physics;
    Transform *transform;
    vm89_vec3 resolved;
    physics = (TestPhysics *)user;
    transform = (Transform *)actor;
    if (!physics || !transform || !desired) return 0;
    resolved = *desired;
    if (clamp_to_floor && resolved.y < floor_y) resolved.y = floor_y;
    transform->position.x = (g3d_fix)resolved.x;
    transform->position.y = (g3d_fix)resolved.y;
    transform->position.z = (g3d_fix)resolved.z;
    physics->moves++;
    if (out_resolved) *out_resolved = resolved;
    return 1;
}

int main(void)
{
    Blank3DVerticalAxis axis;
    Blank3DVerticalBody body;
    Transform actor;
    Vec3 target;
    Vec3 saved;
    vm89_physics_provider provider;
    TestPhysics physics;
    g3d_fix quarter;
    g3d_fix twentieth;
    int i;
    int reached;

    transform_init(&actor);
    actor.position.y = G3D_FIX_FROM_INT(5);
    blank3d_vertical_axis_init_gamlib3d(&axis);
    blank3d_vertical_body_init(&body, &actor, actor.position.y, 0);
    quarter = G3D_FIX_ONE / 4L;
    twentieth = G3D_FIX_ONE / 20L;

    if (!blank3d_vertical_axis_fly(&axis, &body,
                                    G3D_FIX_FROM_INT(4), quarter,
                                    FLY89_UP) ||
        actor.position.y != G3D_FIX_FROM_INT(6)) {
        puts("fly89 upward displacement failed");
        return 1;
    }
    if (!blank3d_vertical_axis_fly(&axis, &body,
                                    G3D_FIX_FROM_INT(40), quarter,
                                    FLY89_DOWN) || actor.position.y != 0) {
        puts("fly89 floor clamp failed");
        return 2;
    }

    actor.position.y = G3D_FIX_FROM_INT(5);
    blank3d_vertical_axis_configure_gravity(&axis, 1,
        G3D_FIX_FROM_INT(8), G3D_FIX_FROM_INT(22));
    blank3d_vertical_axis_tick(&axis, &body, quarter);
    if (actor.position.y != G3D_FIX_FROM_INT(3)) {
        puts("engine dumb gravity failed");
        return 3;
    }
    blank3d_vertical_axis_configure_gravity(&axis, 0,
        G3D_FIX_FROM_INT(8), G3D_FIX_FROM_INT(22));
    blank3d_vertical_axis_tick(&axis, &body, quarter);
    if (actor.position.y != G3D_FIX_FROM_INT(3)) {
        puts("gravity INI flag behavior failed");
        return 4;
    }
    blank3d_vertical_axis_configure_gravity(&axis, 1,
        G3D_FIX_FROM_INT(8), G3D_FIX_FROM_INT(22));
    blank3d_vertical_body_set_flying(&body, 1);
    blank3d_vertical_axis_tick(&axis, &body, quarter);
    if (actor.position.y != G3D_FIX_FROM_INT(3)) {
        puts("flying entity gravity immunity failed");
        return 5;
    }

    /* Keep flying_entity enabled: the same player body must support
       explicit fly89 movement and a grounded jump89 arc. */
    actor.position.y = 0;
    jump89_state_reset(&body.jump_state);
    if (!blank3d_vertical_axis_jump(&axis, &body,
                                     G3D_FIX_FROM_INT(10))) {
        puts("jump89 start failed");
        return 6;
    }
    for (i = 0; i < 200 &&
         blank3d_vertical_axis_jumping(&body); ++i)
        blank3d_vertical_axis_tick(&axis, &body, twentieth);
    if (blank3d_vertical_axis_jumping(&body) ||
        actor.position.y != 0 ||
        !blank3d_vertical_axis_grounded(&axis, &body)) {
        puts("dual-mode flying-entity jump89 landing failed");
        return 7;
    }

    actor.position = gamlib_vec3(0, G3D_FIX_FROM_INT(6), 0);
    blank3d_vertical_body_set_flying(&body, 1);
    if (!blank3d_vertical_axis_save_position(&axis, &body)) {
        puts("airdiver89 save position failed");
        return 8;
    }
    reached = 0;
    if (!blank3d_vertical_axis_descend_to_y(&axis, &body,
            G3D_FIX_FROM_INT(2), G3D_FIX_FROM_INT(8), quarter,
            &reached) || actor.position.y != G3D_FIX_FROM_INT(4)) {
        puts("airdiver89 controlled descent failed");
        return 9;
    }
    target = gamlib_vec3(G3D_FIX_FROM_INT(3), 0, 0);
    for (i = 0; i < 20 && !reached; ++i)
        (void)blank3d_vertical_axis_ram_point(&axis, &body, &target,
            G3D_FIX_FROM_INT(12), quarter, &reached);
    if (!reached || actor.position.x != target.x ||
        actor.position.y != target.y) {
        puts("airdiver89 ram target failed");
        return 10;
    }
    reached = 0;
    for (i = 0; i < 20 && !reached; ++i)
        (void)blank3d_vertical_axis_return_saved(&axis, &body,
            G3D_FIX_FROM_INT(12), quarter, &reached);
    if (!reached ||
        !blank3d_vertical_axis_saved_position(&body, &saved) ||
        actor.position.x != saved.x || actor.position.y != saved.y ||
        actor.position.z != saved.z) {
        puts("airdiver89 return position failed");
        return 11;
    }

    memset(&physics, 0, sizeof(physics));
    memset(&provider, 0, sizeof(provider));
    provider.user = &physics;
    provider.move_position = test_physics_move;
    blank3d_vertical_axis_set_physics_provider(&axis, &provider);
    (void)blank3d_vertical_axis_fly(&axis, &body,
        G3D_FIX_FROM_INT(4), quarter, FLY89_DOWN);
    if (physics.moves != 1) {
        puts("external physics provider was not used");
        return 12;
    }

    puts("Blank3D formal vertical axis + fly89/jump89/airdiver89: OK");
    return 0;
}
