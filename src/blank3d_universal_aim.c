#include "blank3d_universal_aim.h"

#include <limits.h>
#include <string.h>

static Vec3 b3d_uaim_normalized(Vec3 value, Vec3 fallback)
{
    Vec3 out;
    if (gamlib_vec3_length(&value) <= G3D_FIX_EPSILON)
        value = fallback;
    if (gamlib_vec3_length(&value) <= G3D_FIX_EPSILON)
        value = gamlib_vec3(0, 0, G3D_FIX_ONE);
    gamlib_vec3_normalize(&out, &value);
    return out;
}

static soq3d_fx b3d_uaim_q12_to_q16(g3d_fix value)
{
    long result;
    if (value > 134217727L) return (soq3d_fx)INT_MAX;
    if (value < -134217728L) return (soq3d_fx)INT_MIN;
    result = value * 16L;
    if (result > (long)INT_MAX) return (soq3d_fx)INT_MAX;
    if (result < (long)INT_MIN) return (soq3d_fx)INT_MIN;
    return (soq3d_fx)result;
}

int blank3d_universal_aim_build(
    const Vec3 *camera_origin,
    const Vec3 *camera_forward,
    const Vec3 *camera_right,
    const Vec3 *camera_up,
    const Vec3 *carrier_origin,
    const Vec3 *hit_target,
    int hit_valid,
    g3d_fix zero_distance,
    Blank3DUniversalAimFrame *out_frame)
{
    Vec3 forward;
    Vec3 right;
    Vec3 up;
    Vec3 target;
    Vec3 camera_to_target;
    Vec3 camera_to_carrier;
    Vec3 carrier_to_target;
    Vec3 offset;
    g3d_fix target_depth;
    g3d_fix carrier_depth;
    g3d_fix minimum_depth;
    g3d_fix depth;
    int repaired;

    if (camera_origin == 0 || camera_forward == 0 ||
        carrier_origin == 0 || out_frame == 0)
        return 0;

    memset(out_frame, 0, sizeof(*out_frame));
    forward = b3d_uaim_normalized(*camera_forward,
                                  gamlib_vec3(0, 0, G3D_FIX_ONE));
    right = camera_right != 0
          ? b3d_uaim_normalized(*camera_right,
                                gamlib_vec3(G3D_FIX_ONE, 0, 0))
          : gamlib_vec3(G3D_FIX_ONE, 0, 0);
    up = camera_up != 0
       ? b3d_uaim_normalized(*camera_up,
                             gamlib_vec3(0, G3D_FIX_ONE, 0))
       : gamlib_vec3(0, G3D_FIX_ONE, 0);

    if (zero_distance <= B3D_UNIVERSAL_AIM_CLEARANCE)
        zero_distance = B3D_UNIVERSAL_AIM_ZERO_DISTANCE;

    repaired = 0;
    target = hit_target != 0 ? *hit_target : *camera_origin;
    gamlib_vec3_sub(&camera_to_target, &target, camera_origin);
    gamlib_vec3_sub(&camera_to_carrier, carrier_origin, camera_origin);
    target_depth = gamlib_vec3_dot(&camera_to_target, &forward);
    carrier_depth = gamlib_vec3_dot(&camera_to_carrier, &forward);
    minimum_depth = g3d_fix_add_sat(carrier_depth,
                                    B3D_UNIVERSAL_AIM_CLEARANCE);

    if (!hit_valid || target_depth <= minimum_depth) {
        depth = zero_distance;
        if (depth < minimum_depth) depth = minimum_depth;
        gamlib_vec3_scale(&offset, &forward, depth);
        gamlib_vec3_add(&target, camera_origin, &offset);
        hit_valid = 0;
        repaired = 1;
    }

    gamlib_vec3_sub(&carrier_to_target, &target, carrier_origin);
    out_frame->launch_direction = b3d_uaim_normalized(
        carrier_to_target, forward);
    if (gamlib_vec3_dot(&out_frame->launch_direction, &forward) <= 0) {
        depth = zero_distance;
        if (depth < minimum_depth) depth = minimum_depth;
        gamlib_vec3_scale(&offset, &forward, depth);
        gamlib_vec3_add(&target, camera_origin, &offset);
        gamlib_vec3_sub(&carrier_to_target, &target, carrier_origin);
        out_frame->launch_direction = b3d_uaim_normalized(
            carrier_to_target, forward);
        if (gamlib_vec3_dot(&out_frame->launch_direction, &forward) <= 0)
            out_frame->launch_direction = forward;
        hit_valid = 0;
        repaired = 1;
    }

    out_frame->camera_origin = *camera_origin;
    out_frame->camera_forward = forward;
    out_frame->camera_right = right;
    out_frame->camera_up = up;
    out_frame->carrier_origin = *carrier_origin;
    out_frame->target = target;
    out_frame->target_is_hit = hit_valid ? 1 : 0;
    out_frame->repaired = repaired;
    return 1;
}

int blank3d_universal_aim_apply_pose(
    const soq3d_pose *carrier_pose,
    const Blank3DUniversalAimFrame *frame,
    soq3d_pose *out_pose)
{
    Vec3 forward;
    Vec3 right;
    Vec3 up;
    Vec3 world_up;
    Vec3 negative_forward;

    if (carrier_pose == 0 || frame == 0 || out_pose == 0) return 0;

    forward = b3d_uaim_normalized(frame->launch_direction,
                                  frame->camera_forward);
    world_up = gamlib_vec3(0, G3D_FIX_ONE, 0);
    gamlib_vec3_cross(&right, &forward, &world_up);
    if (gamlib_vec3_length(&right) <= G3D_FIX_EPSILON)
        right = frame->camera_right;
    right = b3d_uaim_normalized(right,
                                gamlib_vec3(G3D_FIX_ONE, 0, 0));
    gamlib_vec3_cross(&up, &right, &forward);
    up = b3d_uaim_normalized(up, frame->camera_up);
    negative_forward = gamlib_vec3(g3d_fix_neg_sat(forward.x),
                                   g3d_fix_neg_sat(forward.y),
                                   g3d_fix_neg_sat(forward.z));

    *out_pose = *carrier_pose;
    out_pose->basis.m00 = b3d_uaim_q12_to_q16(right.x);
    out_pose->basis.m10 = b3d_uaim_q12_to_q16(right.y);
    out_pose->basis.m20 = b3d_uaim_q12_to_q16(right.z);
    out_pose->basis.m01 = b3d_uaim_q12_to_q16(up.x);
    out_pose->basis.m11 = b3d_uaim_q12_to_q16(up.y);
    out_pose->basis.m21 = b3d_uaim_q12_to_q16(up.z);
    out_pose->basis.m02 = b3d_uaim_q12_to_q16(negative_forward.x);
    out_pose->basis.m12 = b3d_uaim_q12_to_q16(negative_forward.y);
    out_pose->basis.m22 = b3d_uaim_q12_to_q16(negative_forward.z);
    return 1;
}

int blank3d_universal_aim_guard_direction(
    const Vec3 *camera_forward,
    const Vec3 *origin,
    const Vec3 *target,
    Vec3 *in_out_direction)
{
    Vec3 forward;
    Vec3 direction;
    Vec3 delta;
    int repaired;

    if (camera_forward == 0 || in_out_direction == 0) return 0;
    forward = b3d_uaim_normalized(*camera_forward,
                                  gamlib_vec3(0, 0, G3D_FIX_ONE));
    direction = b3d_uaim_normalized(*in_out_direction, forward);
    repaired = 0;

    if (gamlib_vec3_dot(&direction, &forward) <= 0) {
        if (origin != 0 && target != 0) {
            gamlib_vec3_sub(&delta, target, origin);
            direction = b3d_uaim_normalized(delta, forward);
        }
        if (gamlib_vec3_dot(&direction, &forward) <= 0)
            direction = forward;
        repaired = 1;
    }
    *in_out_direction = direction;
    return repaired ? 2 : 1;
}
int blank3d_universal_aim_finalize_event(
    const GWP89_Event *event,
    const Blank3DWeaponModules *modules,
    GWP89_Vec3 *out_direction,
    gwp89_fx *out_gravity_fx)
{
    GWP89_Vec3 direction_q12;
    gwp89_fx gravity_q12;
    Vec3 direction;
    Vec3 origin;
    Vec3 target;
    Vec3 forward;

    if (event == 0 || out_direction == 0) return 0;
    direction_q12 = event->direction;
    gravity_q12 = 0;

    if (modules != 0 &&
        (modules->physics_backend == B3D_PHYSICS_GRAVITY ||
         modules->physics_backend == B3D_PHYSICS_BOLT3D)) {
        (void)blank3d_ballistics_resolve_launch(
            event, modules, &direction_q12, &gravity_q12);
    }

    direction = gamlib_vec3((g3d_fix)direction_q12.x,
                            (g3d_fix)direction_q12.y,
                            (g3d_fix)direction_q12.z);
    origin = gamlib_vec3((g3d_fix)event->origin.x,
                         (g3d_fix)event->origin.y,
                         (g3d_fix)event->origin.z);
    target = gamlib_vec3((g3d_fix)event->hit_point.x,
                         (g3d_fix)event->hit_point.y,
                         (g3d_fix)event->hit_point.z);
    forward = gamlib_vec3((g3d_fix)event->camera_forward.x,
                          (g3d_fix)event->camera_forward.y,
                          (g3d_fix)event->camera_forward.z);
    if (gamlib_vec3_length(&forward) <= G3D_FIX_EPSILON)
        forward = gamlib_vec3((g3d_fix)event->direction.x,
                              (g3d_fix)event->direction.y,
                              (g3d_fix)event->direction.z);

    (void)blank3d_universal_aim_guard_direction(
        &forward, &origin, &target, &direction);
    direction = b3d_uaim_normalized(direction, forward);
    out_direction->x = (gwp89_fx)direction.x;
    out_direction->y = (gwp89_fx)direction.y;
    out_direction->z = (gwp89_fx)direction.z;
    if (out_gravity_fx != 0) *out_gravity_fx = gravity_q12;
    return 1;
}
