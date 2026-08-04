#include "blank3d_motion_attack.h"

#include <stdio.h>
#include <string.h>

static g3d_fix ratio(int n, int d)
{
    return g3d_fix_div(G3D_FIX_FROM_INT(n), G3D_FIX_FROM_INT(d));
}

static void reset_transform_test(Transform *transform,
                                 g3d_fix x, g3d_fix y, g3d_fix z)
{
    transform_init(transform);
    transform->position = gamlib_vec3(x, y, z);
}

static int test_air_lunge(void)
{
    Blank3DMotionAttack attack;
    Transform actor;
    Vec3 target;
    Vec3 facing;
    g3d_fix dt;
    int i;
    int saw_active;
    int saw_impact;

    blank3d_motion_attack_init(&attack);
    if (!blank3d_motion_attack_set_air_intent(&attack, "claw")) return 1;
    if (!blank3d_motion_attack_set_air_target_policy(&attack, "track")) return 2;
    blank3d_motion_attack_set_air_steering(&attack, ratio(3, 4));
    blank3d_motion_attack_set_contact_radius(&attack, ratio(4, 5));
    reset_transform_test(&actor, 0, G3D_FIX_FROM_INT(4), 0);
    target = gamlib_vec3(G3D_FIX_FROM_INT(8), G3D_FIX_FROM_INT(1), 0);
    dt = ratio(1, 60);
    if (!blank3d_motion_attack_request_air(&attack,
                                            G3D_FIX_FROM_INT(14))) return 3;

    saw_active = 0;
    saw_impact = 0;
    for (i = 0; i < 180; ++i) {
        (void)blank3d_motion_attack_tick(&attack, &actor, &target,
                                         0, 0, dt, &facing);
        if (blank3d_motion_attack_is_active(&attack)) saw_active = 1;
        if (blank3d_motion_attack_has_impact(&attack)) {
            saw_impact = 1;
            blank3d_motion_attack_clear_impact(&attack);
        }
        if (blank3d_motion_attack_finished(&attack)) break;
    }
    if (!saw_active) return 4;
    if (!saw_impact) return 5;
    if (!blank3d_motion_attack_finished(&attack)) return 6;
    if (actor.position.x <= G3D_FIX_FROM_INT(5)) return 7;
    if (blank3d_motion_attack_mode(&attack) !=
        B3D_MOTION_ATTACK_AIR_LUNGE) return 8;
    blank3d_motion_attack_reset(&attack);
    if (blank3d_motion_attack_mode(&attack) !=
        B3D_MOTION_ATTACK_NONE) return 9;
    return 0;
}

static int test_ground_lance(void)
{
    Blank3DMotionAttack attack;
    Transform actor;
    Vec3 target;
    Vec3 facing;
    Vec3 separation;
    g3d_fix dt;
    g3d_fix distance;
    g3d_fix tolerance;
    int i;
    int saw_impact;

    blank3d_motion_attack_init_archetype(&attack,
                                         "ground_lancer_enemy");
    if (attack.ground.cfg.style != GGL_STYLE_LANCE) return 1;
    if (attack.ground.cfg.hop_height != 0) return 2;
    if (attack.ground.cfg.contact_policy != GGL_CONTACT_STOP) return 3;
    reset_transform_test(&actor, 0, 0, 0);
    target = gamlib_vec3(G3D_FIX_FROM_INT(10), 0, 0);
    dt = ratio(1, 60);
    if (!blank3d_motion_attack_request_ground(&attack,
                                               G3D_FIX_FROM_INT(12), 1))
        return 4;

    saw_impact = 0;
    for (i = 0; i < 180; ++i) {
        (void)blank3d_motion_attack_tick(&attack, &actor, &target,
                                         0, 1, dt, &facing);
        if (actor.position.y != 0) return 5;
        if (blank3d_motion_attack_has_impact(&attack)) {
            saw_impact = 1;
            blank3d_motion_attack_clear_impact(&attack);
        }
        if (blank3d_motion_attack_finished(&attack)) break;
    }
    if (actor.position.x <= G3D_FIX_FROM_INT(7)) return 6;
    if (!saw_impact) return 7;
    if (!blank3d_motion_attack_finished(&attack)) return 8;
    if (blank3d_motion_attack_mode(&attack) !=
        B3D_MOTION_ATTACK_GROUND_LANCE) return 9;

    gamlib_vec3_sub(&separation, &target, &actor.position);
    separation.y = 0;
    distance = gamlib_vec3_length(&separation);
    tolerance = ratio(1, 32);
    if (distance < g3d_fix_sub_sat(attack.contact_radius, tolerance) ||
        distance > g3d_fix_add_sat(attack.contact_radius, tolerance))
        return 10;
    return 0;
}

static int test_ground_pegasus_opt_in(void)
{
    Blank3DMotionAttack attack;
    Transform actor;
    Vec3 target;
    Vec3 facing;
    g3d_fix dt;
    g3d_fix max_y;
    int i;

    blank3d_motion_attack_init(&attack);
    if (!blank3d_motion_attack_set_ground_style(&attack,
                                                 "pegasus")) return 1;
    if (!blank3d_motion_attack_set_ground_contact(&attack,
                                                   "stop")) return 2;
    blank3d_motion_attack_set_ground_hop_height(&attack, ratio(2, 5));
    blank3d_motion_attack_set_contact_radius(&attack, ratio(4, 5));
    reset_transform_test(&actor, 0, 0, 0);
    target = gamlib_vec3(G3D_FIX_FROM_INT(6), 0, 0);
    dt = ratio(1, 60);
    if (!blank3d_motion_attack_request_ground(&attack,
                                               G3D_FIX_FROM_INT(12), 1))
        return 3;

    max_y = 0;
    for (i = 0; i < 180; ++i) {
        (void)blank3d_motion_attack_tick(&attack, &actor, &target,
                                         0, 1, dt, &facing);
        if (actor.position.y > max_y) max_y = actor.position.y;
        if (blank3d_motion_attack_finished(&attack)) break;
    }
    if (max_y <= 0) return 4;
    if (!blank3d_motion_attack_finished(&attack)) return 5;
    return 0;
}

int main(void)
{
    int result;
    result = test_air_lunge();
    if (result != 0) {
        printf("air lunge bridge failed: %d\n", result);
        return result;
    }
    result = test_ground_lance();
    if (result != 0) {
        printf("ground lance bridge failed: %d\n", result);
        return 20 + result;
    }
    result = test_ground_pegasus_opt_in();
    if (result != 0) {
        printf("ground pegasus opt-in failed: %d\n", result);
        return 40 + result;
    }
    puts("Blank3D gairlunge89 + ggroundlance89 bridge: OK");
    return 0;
}
