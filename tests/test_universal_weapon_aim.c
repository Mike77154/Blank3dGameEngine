#include <stdio.h>
#include <string.h>

#include "blank3d_universal_aim.h"

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

static GWP89_Vec3 to_gwp(Vec3 value)
{
    GWP89_Vec3 out;
    out.x = (gwp89_fx)value.x;
    out.y = (gwp89_fx)value.y;
    out.z = (gwp89_fx)value.z;
    return out;
}

static Vec3 from_gwp(GWP89_Vec3 value)
{
    return gamlib_vec3((g3d_fix)value.x,
                       (g3d_fix)value.y,
                       (g3d_fix)value.z);
}

static Vec3 pose_local_minus_z(const soq3d_pose *pose)
{
    Vec3 out;
    out.x = (g3d_fix)(-(pose->basis.m02 / 16L));
    out.y = (g3d_fix)(-(pose->basis.m12 / 16L));
    out.z = (g3d_fix)(-(pose->basis.m22 / 16L));
    gamlib_vec3_normalize(&out, &out);
    return out;
}

static soq3d_fx q12_to_q16(g3d_fix value)
{
    return (soq3d_fx)(value * 16L);
}

static int test_carrier_frame(void)
{
    Vec3 eye;
    Vec3 forward;
    Vec3 right;
    Vec3 up;
    Vec3 carrier;
    Vec3 visual_forward;
    Blank3DUniversalAimFrame frame;
    soq3d_pose raw;
    soq3d_pose aimed;
    int i;
    static const int x_positions[5] = { -7, -2, 0, 3, 9 };
    static const int z_positions[5] = { -1,  2, 6, 11, 17 };

    eye = gamlib_vec3(G3D_FIX_FROM_INT(2),
                      G3D_FIX_FROM_INT(5),
                      G3D_FIX_FROM_INT(-8));
    forward = gamlib_vec3(G3D_FIX_ONE / 5L,
                          -G3D_FIX_ONE / 10L,
                          G3D_FIX_ONE);
    gamlib_vec3_normalize(&forward, &forward);
    right = gamlib_vec3(-G3D_FIX_ONE, 0, G3D_FIX_ONE / 5L);
    gamlib_vec3_normalize(&right, &right);
    up = gamlib_vec3(0, G3D_FIX_ONE, 0);

    for (i = 0; i < 5; ++i) {
        carrier = gamlib_vec3(G3D_FIX_FROM_INT(x_positions[i]),
                              G3D_FIX_FROM_INT(1),
                              G3D_FIX_FROM_INT(z_positions[i]));
        raw = soq3d_pose_identity();
        raw.position.x = q12_to_q16(carrier.x);
        raw.position.y = q12_to_q16(carrier.y);
        raw.position.z = q12_to_q16(carrier.z);

        if (!blank3d_universal_aim_build(
                &eye, &forward, &right, &up, &carrier,
                0, 0, B3D_UNIVERSAL_AIM_ZERO_DISTANCE, &frame))
            return fail("universal carrier frame rejected movement pose");
        if (!blank3d_universal_aim_apply_pose(&raw, &frame, &aimed))
            return fail("universal carrier pose application failed");
        visual_forward = pose_local_minus_z(&aimed);
        if (gamlib_vec3_dot(&visual_forward,
                            &frame.launch_direction) <
            (G3D_FIX_ONE - G3D_FIX_ONE / 100L))
            return fail("weapon local -Z does not follow shared HUD aim");
        if (gamlib_vec3_dot(&visual_forward, &forward) <= 0)
            return fail("weapon carrier entered rear camera hemisphere");
        if (aimed.position.x != raw.position.x ||
            aimed.position.y != raw.position.y ||
            aimed.position.z != raw.position.z)
            return fail("aim carrier changed actor-owned socket position");
    }
    return 0;
}

static void prepare_event(GWP89_Event *event,
                          int weapon_id,
                          const Vec3 *eye,
                          const Vec3 *forward,
                          const Vec3 *right,
                          const Vec3 *up,
                          const Vec3 *muzzle)
{
    Vec3 target;
    Vec3 offset;
    Vec3 launch;
    memset(event, 0, sizeof(*event));
    gamlib_vec3_scale(&offset, forward, G3D_FIX_FROM_INT(32));
    gamlib_vec3_add(&target, eye, &offset);
    gamlib_vec3_sub(&launch, &target, muzzle);
    gamlib_vec3_normalize(&launch, &launch);

    event->type = GWP89_EVENT_PROJECTILE_REQUEST;
    event->actor_id = 1;
    event->view_style = GWP89_VIEW_OVER_SHOULDER;
    event->weapon_id = weapon_id;
    event->flags = GWP89_EVENT_FLAG_HIT_VALID;
    event->origin = to_gwp(*muzzle);
    event->muzzle_origin = event->origin;
    event->hit_point = to_gwp(target);
    event->direction = to_gwp(launch);
    event->camera_origin = to_gwp(*eye);
    event->camera_forward = to_gwp(*forward);
    event->camera_right = to_gwp(*right);
    event->camera_up = to_gwp(*up);
    event->range_fx = gwp89_fx_from_int(64);
    event->speed_fx = gwp89_fx_from_int(weapon_id == 6 ? 20 : 60);
}

static int test_all_weapon_profiles(void)
{
    Vec3 eye;
    Vec3 forward;
    Vec3 right;
    Vec3 up;
    Vec3 muzzle;
    Vec3 final_direction;
    Vec3 aligned_direction;
    GWP89_Event event;
    GWP89_Event aligned;
    GWP89_Vec3 output;
    gwp89_fx gravity;
    const Blank3DWeaponModules *modules;
    int weapon_id;

    blank3d_weapon_modules_load_defaults();
    eye = gamlib_vec3(G3D_FIX_FROM_INT(2),
                      G3D_FIX_FROM_INT(5),
                      G3D_FIX_FROM_INT(-8));
    forward = gamlib_vec3(G3D_FIX_ONE / 5L,
                          -G3D_FIX_ONE / 10L,
                          G3D_FIX_ONE);
    gamlib_vec3_normalize(&forward, &forward);
    right = gamlib_vec3(-G3D_FIX_ONE, 0, G3D_FIX_ONE / 5L);
    gamlib_vec3_normalize(&right, &right);
    up = gamlib_vec3(0, G3D_FIX_ONE, 0);
    muzzle = gamlib_vec3(G3D_FIX_FROM_INT(-1),
                         G3D_FIX_FROM_INT(1),
                         G3D_FIX_FROM_INT(4));

    for (weapon_id = 1; weapon_id <= 9; ++weapon_id) {
        prepare_event(&event, weapon_id, &eye, &forward,
                      &right, &up, &muzzle);
        if (!blank3d_ballistics_align_event_to_view(
                &event, &event.camera_origin, &event.camera_forward,
                &aligned))
            return fail("camera alignment rejected a catalog weapon");
        modules = blank3d_weapon_modules_get(weapon_id);
        if (modules == 0)
            return fail("catalog weapon has no module profile");
        gravity = 0;
        if (!blank3d_universal_aim_finalize_event(
                &aligned, modules, &output, &gravity))
            return fail("universal finalizer rejected a catalog weapon");
        final_direction = from_gwp(output);
        gamlib_vec3_normalize(&final_direction, &final_direction);
        if (gamlib_vec3_dot(&final_direction, &forward) <= 0)
            return fail("catalog weapon committed a rear-facing velocity");

        aligned_direction = from_gwp(aligned.direction);
        gamlib_vec3_normalize(&aligned_direction, &aligned_direction);
        if (modules->physics_backend == B3D_PHYSICS_LINEAR &&
            gamlib_vec3_dot(&final_direction, &aligned_direction) <
            (G3D_FIX_ONE - G3D_FIX_ONE / 100L))
            return fail("linear weapon lost its HUD-converged direction");
        if ((modules->physics_backend == B3D_PHYSICS_GRAVITY ||
             modules->physics_backend == B3D_PHYSICS_BOLT3D) &&
            gravity >= 0)
            return fail("ballistic weapon lost negative gravity");
    }
    return 0;
}

static int test_broken_reverse_event(void)
{
    GWP89_Event event;
    GWP89_Vec3 output;
    Vec3 direction;
    Vec3 forward;
    gwp89_fx gravity;

    memset(&event, 0, sizeof(event));
    event.weapon_id = 8;
    event.origin = gwp89_v3(0, gwp89_fx_from_int(1), 0);
    event.hit_point = gwp89_v3(0, gwp89_fx_from_int(1),
                               gwp89_fx_from_int(-20));
    event.direction = gwp89_v3(0, 0, -gwp89_fx_from_int(1));
    event.camera_forward = gwp89_v3(0, 0, gwp89_fx_from_int(1));
    event.speed_fx = gwp89_fx_from_int(60);
    event.range_fx = gwp89_fx_from_int(64);

    gravity = 0;
    if (!blank3d_universal_aim_finalize_event(
            &event, blank3d_weapon_modules_get(8), &output, &gravity))
        return fail("reverse Gatling event was rejected");
    direction = from_gwp(output);
    forward = from_gwp(event.camera_forward);
    if (gamlib_vec3_dot(&direction, &forward) <= 0)
        return fail("reverse Gatling event was not repaired forward");
    return 0;
}

static int test_shotgun_spread_survives(void)
{
    GWP89_Event left;
    GWP89_Event right;
    GWP89_Vec3 left_out;
    GWP89_Vec3 right_out;
    gwp89_fx gravity;

    memset(&left, 0, sizeof(left));
    left.weapon_id = 3;
    left.origin = gwp89_v3(0, gwp89_fx_from_int(1), 0);
    left.hit_point = gwp89_v3(-gwp89_fx_from_int(4),
                              gwp89_fx_from_int(1),
                              gwp89_fx_from_int(32));
    left.direction = left.hit_point;
    left.camera_forward = gwp89_v3(0, 0, gwp89_fx_from_int(1));
    left.speed_fx = gwp89_fx_from_int(60);
    left.range_fx = gwp89_fx_from_int(64);
    right = left;
    right.hit_point.x = gwp89_fx_from_int(4);
    right.direction.x = gwp89_fx_from_int(4);

    gravity = 0;
    if (!blank3d_universal_aim_finalize_event(
            &left, blank3d_weapon_modules_get(3), &left_out, &gravity) ||
        !blank3d_universal_aim_finalize_event(
            &right, blank3d_weapon_modules_get(3), &right_out, &gravity))
        return fail("shotgun spread finalization failed");
    if (left_out.x >= 0 || right_out.x <= 0)
        return fail("universal gate collapsed shotgun left/right spread");
    if (left_out.z <= 0 || right_out.z <= 0)
        return fail("shotgun spread left forward hemisphere");
    return 0;
}

int main(void)
{
    if (test_carrier_frame()) return 1;
    if (test_all_weapon_profiles()) return 1;
    if (test_broken_reverse_event()) return 1;
    if (test_shotgun_spread_survives()) return 1;
    puts("PASS: universal aim frame keeps carrier and all weapons on HUD ray");
    return 0;
}
