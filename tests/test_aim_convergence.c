#include <stdio.h>

#include "blank3d_systems.h"
#include "blank3d_ballistics.h"
#include "../vendor/gamlib3d/math_helpers/gamlib3d_math.h"

static int fail(const char *message)
{
    printf("FAIL: %s\n", message);
    return 1;
}

static GWP89_Vec3 q12v(int x, int y, int z)
{
    return gwp89_v3(gwp89_fx_from_int(x),
                    gwp89_fx_from_int(y),
                    gwp89_fx_from_int(z));
}

static long abs_long(long value)
{
    return value < 0L ? -value : value;
}

int main(void)
{
    Blank3DSystems systems;
    GWP89_Vec3 muzzle;
    GWP89_Vec3 camera;
    GWP89_Vec3 forward;
    GWP89_Vec3 right;
    GWP89_Vec3 up;
    GWP89_Event event;
    GWP89_Event grenade;
    GWP89_Event backward;
    GWP89_Event aligned;
    GWP89_Event tps_empty;
    GWP89_Event tps_hit;
    GWP89_Event tps_close;
    GWP89_Event tps_spread_left;
    GWP89_Event tps_spread_right;
    GWP89_Event aligned_left;
    GWP89_Event aligned_right;
    GWP89_Vec3 launch;
    gwp89_fx gravity;
    Vec3 expected;
    Vec3 actual;
    Vec3 delta;
    Vec3 velocity;
    Vec3 tps_eye;
    Vec3 tps_forward;
    Vec3 tps_ray;
    Vec3 tps_target_dir;
    Vec3 tps_launch_dir;
    g3d_fix horizontal_distance;
    g3d_fix horizontal_speed;
    g3d_fix flight_time;
    g3d_fix final_y;
    g3d_fix half_gravity_t2;
    int found;

    blank3d_systems_init(&systems);
    muzzle = q12v(1, 1, 0);
    camera = q12v(0, 1, 0);
    forward = q12v(0, 0, 1);
    right = q12v(1, 0, 0);
    up = q12v(0, 1, 0);

    blank3d_systems_update(&systems, 16U, 1, 1, 0,
                           GWP89_VIEW_OVER_SHOULDER, GWP89_FIX_ONE,
                           &muzzle, &forward, &right, &up,
                           &camera, &forward);
    found = 0;
    while (blank3d_systems_poll_event(&systems, &event)) {
        if (event.type != GWP89_EVENT_PROJECTILE_REQUEST) continue;
        found = 1;
        if (event.camera_origin.x != camera.x ||
            event.camera_origin.y != camera.y ||
            event.camera_origin.z != camera.z)
            return fail("projectile lost fire-time camera origin snapshot");
        if (event.camera_forward.x != forward.x ||
            event.camera_forward.y != forward.y ||
            event.camera_forward.z != forward.z)
            return fail("projectile lost fire-time camera forward snapshot");
        if (event.camera_right.x != right.x ||
            event.camera_right.y != right.y ||
            event.camera_right.z != right.z ||
            event.camera_up.x != up.x ||
            event.camera_up.y != up.y ||
            event.camera_up.z != up.z)
            return fail("projectile lost fire-time camera basis snapshot");
        delta = gamlib_vec3((g3d_fix)(event.hit_point.x - event.origin.x),
                            (g3d_fix)(event.hit_point.y - event.origin.y),
                            (g3d_fix)(event.hit_point.z - event.origin.z));
        gamlib_vec3_normalize(&expected, &delta);
        actual = gamlib_vec3((g3d_fix)event.direction.x,
                             (g3d_fix)event.direction.y,
                             (g3d_fix)event.direction.z);
        if (event.direction.x >= 0L)
            return fail("muzzle ray did not converge left toward HUD center");
        if (event.hit_point.x != camera.x || event.hit_point.y != camera.y)
            return fail("no-hit target was not built from camera/HUD ray");
        if (abs_long(actual.x - expected.x) > 3L ||
            abs_long(actual.y - expected.y) > 3L ||
            abs_long(actual.z - expected.z) > 3L) {
            printf("actual=%ld,%ld,%ld expected=%ld,%ld,%ld origin=%ld,%ld,%ld hit=%ld,%ld,%ld\n",
                   actual.x, actual.y, actual.z, expected.x, expected.y, expected.z,
                   event.origin.x, event.origin.y, event.origin.z,
                   event.hit_point.x, event.hit_point.y, event.hit_point.z);
            return fail("projectile direction is not muzzle-to-crosshair");
        }
    }
    if (!found) return fail("projectile event missing");

    /* Realistic TPS regression: camera above/behind the muzzle, no world hit.
       Empty space must converge on a practical center-ray plane rather than
       climbing toward eye height at a tiny distance or waiting until max range. */
    tps_eye = gamlib_vec3(G3D_FIX_FROM_INT(2),
                          G3D_FIX_FROM_INT(5),
                          G3D_FIX_FROM_INT(-8));
    tps_forward = gamlib_vec3(0,
                              -G3D_FIX_ONE / 10L,
                              G3D_FIX_ONE);
    gamlib_vec3_normalize(&tps_forward, &tps_forward);
    tps_empty = event;
    tps_empty.view_style = GWP89_VIEW_OVER_SHOULDER;
    tps_empty.flags &= ~GWP89_EVENT_FLAG_HIT_VALID;
    tps_empty.origin = q12v(0, 1, 1);
    tps_empty.range_fx = gwp89_fx_from_int(125);
    tps_empty.hit_point.x = (gwp89_fx)(tps_eye.x +
        g3d_fix_mul(tps_forward.x, G3D_FIX_FROM_INT(125)));
    tps_empty.hit_point.y = (gwp89_fx)(tps_eye.y +
        g3d_fix_mul(tps_forward.y, G3D_FIX_FROM_INT(125)));
    tps_empty.hit_point.z = (gwp89_fx)(tps_eye.z +
        g3d_fix_mul(tps_forward.z, G3D_FIX_FROM_INT(125)));
    camera = gwp89_v3((gwp89_fx)tps_eye.x,
                      (gwp89_fx)tps_eye.y,
                      (gwp89_fx)tps_eye.z);
    forward = gwp89_v3((gwp89_fx)tps_forward.x,
                       (gwp89_fx)tps_forward.y,
                       (gwp89_fx)tps_forward.z);
    if (!blank3d_ballistics_align_event_to_view(
            &tps_empty, &camera, &forward, &aligned))
        return fail("TPS empty-space zeroing rejected event");
    tps_ray = gamlib_vec3((g3d_fix)(aligned.hit_point.x - camera.x),
                          (g3d_fix)(aligned.hit_point.y - camera.y),
                          (g3d_fix)(aligned.hit_point.z - camera.z));
    if (gamlib_vec3_length(&tps_ray) > G3D_FIX_FROM_INT(33) ||
        gamlib_vec3_length(&tps_ray) < G3D_FIX_FROM_INT(31))
        return fail("TPS empty-space target did not use stable zero plane");
    gamlib_vec3_normalize(&tps_target_dir, &tps_ray);
    if (abs_long(tps_target_dir.x - tps_forward.x) > 4L ||
        abs_long(tps_target_dir.y - tps_forward.y) > 4L ||
        abs_long(tps_target_dir.z - tps_forward.z) > 4L)
        return fail("TPS zero target left the center HUD ray");
    tps_launch_dir = gamlib_vec3((g3d_fix)aligned.direction.x,
                                 (g3d_fix)aligned.direction.y,
                                 (g3d_fix)aligned.direction.z);
    if (g3d_fix_abs(tps_launch_dir.y) > G3D_FIX_HALF)
        return fail("TPS empty-space launch became excessively vertical");


    /* Shotgun regression: v3.6.5 collapsed every no-hit pellet back onto
       camera_forward while shortening the TPS target to 32 units.  The stable
       zero plane must preserve each pellet's already-spread camera ray. */
    tps_spread_left = tps_empty;
    tps_spread_left.flags &= ~GWP89_EVENT_FLAG_HIT_VALID;
    tps_spread_left.pellet_count = 7;
    tps_spread_left.pellet_index = 1;
    tps_spread_left.hit_point.x = (gwp89_fx)(tps_eye.x - G3D_FIX_FROM_INT(10));
    tps_spread_left.hit_point.y = (gwp89_fx)(tps_eye.y +
        g3d_fix_mul(tps_forward.y, G3D_FIX_FROM_INT(125)));
    tps_spread_left.hit_point.z = (gwp89_fx)(tps_eye.z +
        g3d_fix_mul(tps_forward.z, G3D_FIX_FROM_INT(125)));

    tps_spread_right = tps_empty;
    tps_spread_right.flags &= ~GWP89_EVENT_FLAG_HIT_VALID;
    tps_spread_right.pellet_count = 7;
    tps_spread_right.pellet_index = 2;
    tps_spread_right.hit_point.x = (gwp89_fx)(tps_eye.x + G3D_FIX_FROM_INT(10));
    tps_spread_right.hit_point.y = tps_spread_left.hit_point.y;
    tps_spread_right.hit_point.z = tps_spread_left.hit_point.z;

    if (!blank3d_ballistics_align_event_to_view(
            &tps_spread_left, &camera, &forward, &aligned_left) ||
        !blank3d_ballistics_align_event_to_view(
            &tps_spread_right, &camera, &forward, &aligned_right))
        return fail("TPS shotgun spread alignment rejected pellet");
    if (aligned_left.hit_point.x >= camera.x ||
        aligned_right.hit_point.x <= camera.x)
        return fail("TPS shotgun pellets collapsed onto center HUD ray");
    if (aligned_left.direction.x >= 0L || aligned_right.direction.x <= 0L)
        return fail("TPS shotgun launch directions lost left/right spread");
    if (aligned_left.direction.x == aligned_right.direction.x &&
        aligned_left.direction.y == aligned_right.direction.y &&
        aligned_left.direction.z == aligned_right.direction.z)
        return fail("TPS shotgun pellets became identical directions");

    /* A valid center-ray hit in front of the muzzle must not be replaced by
       the generic zero plane. */
    tps_hit = tps_empty;
    tps_hit.flags |= GWP89_EVENT_FLAG_HIT_VALID;
    tps_hit.hit_point.x = (gwp89_fx)(tps_eye.x +
        g3d_fix_mul(tps_forward.x, G3D_FIX_FROM_INT(18)));
    tps_hit.hit_point.y = (gwp89_fx)(tps_eye.y +
        g3d_fix_mul(tps_forward.y, G3D_FIX_FROM_INT(18)));
    tps_hit.hit_point.z = (gwp89_fx)(tps_eye.z +
        g3d_fix_mul(tps_forward.z, G3D_FIX_FROM_INT(18)));
    if (!blank3d_ballistics_align_event_to_view(
            &tps_hit, &camera, &forward, &aligned))
        return fail("TPS valid-hit zeroing rejected event");
    if (aligned.hit_point.x != tps_hit.hit_point.x ||
        aligned.hit_point.y != tps_hit.hit_point.y ||
        aligned.hit_point.z != tps_hit.hit_point.z)
        return fail("TPS valid crosshair hit was incorrectly replaced");

    /* A camera hit behind the physical muzzle plane cannot be launched at.
       It must fall back to the stable frontal zero plane, not to a steep
       eye-height point just in front of the camera. */
    tps_close = tps_empty;
    tps_close.flags |= GWP89_EVENT_FLAG_HIT_VALID;
    tps_close.hit_point.x = (gwp89_fx)(tps_eye.x +
        g3d_fix_mul(tps_forward.x, G3D_FIX_FROM_INT(4)));
    tps_close.hit_point.y = (gwp89_fx)(tps_eye.y +
        g3d_fix_mul(tps_forward.y, G3D_FIX_FROM_INT(4)));
    tps_close.hit_point.z = (gwp89_fx)(tps_eye.z +
        g3d_fix_mul(tps_forward.z, G3D_FIX_FROM_INT(4)));
    if (!blank3d_ballistics_align_event_to_view(
            &tps_close, &camera, &forward, &aligned))
        return fail("TPS close-hit repair rejected event");
    if ((aligned.flags & GWP89_EVENT_FLAG_HIT_VALID) != 0)
        return fail("unreachable TPS camera hit remained marked valid");
    tps_launch_dir = gamlib_vec3((g3d_fix)aligned.direction.x,
                                 (g3d_fix)aligned.direction.y,
                                 (g3d_fix)aligned.direction.z);
    if (g3d_fix_abs(tps_launch_dir.y) > G3D_FIX_HALF)
        return fail("TPS close-hit repair still launches vertically");

    /* Regression: a target and direction accidentally exported behind the
       camera must be rebuilt into the visible +Z hemisphere. */
    backward = event;
    backward.origin = q12v(1, 1, 0);
    backward.hit_point = q12v(0, 1, -30);
    backward.direction = q12v(0, 0, -1);
    backward.range_fx = gwp89_fx_from_int(30);
    if (!blank3d_ballistics_align_event_to_view(
            &backward, &camera, &forward, &aligned))
        return fail("reverse-hemisphere correction rejected event");
    if (aligned.hit_point.z <= camera.z)
        return fail("reverse target was not rebuilt in front of camera");
    if (aligned.direction.z <= 0L)
        return fail("reverse projectile direction was not corrected");

    grenade = event;
    grenade.weapon_id = B3D_WEAPON_ID_GRENADE_LAUNCHER;
    grenade.origin = q12v(1, 1, 0);
    grenade.hit_point = q12v(0, 1, 30);
    grenade.speed_fx = gwp89_fx_from_int(20);
    delta = gamlib_vec3((g3d_fix)(grenade.hit_point.x - grenade.origin.x),
                        (g3d_fix)(grenade.hit_point.y - grenade.origin.y),
                        (g3d_fix)(grenade.hit_point.z - grenade.origin.z));
    gamlib_vec3_normalize(&actual, &delta);
    grenade.direction.x = actual.x;
    grenade.direction.y = actual.y;
    grenade.direction.z = actual.z;

    if (!blank3d_ballistics_resolve_launch(
            &grenade,
            blank3d_weapon_modules_get(B3D_WEAPON_ID_GRENADE_LAUNCHER),
            &launch, &gravity))
        return fail("gravity zeroing rejected grenade");
    if (launch.y <= grenade.direction.y)
        return fail("gravity projectile received no upward compensation");
    if (gravity >= 0L) return fail("gravity sign invalid");

    velocity = gamlib_vec3(
        g3d_fix_mul((g3d_fix)launch.x, (g3d_fix)grenade.speed_fx),
        g3d_fix_mul((g3d_fix)launch.y, (g3d_fix)grenade.speed_fx),
        g3d_fix_mul((g3d_fix)launch.z, (g3d_fix)grenade.speed_fx));
    horizontal_distance = g3d_fix_sqrt(
        g3d_fix_add_sat(
            g3d_fix_mul(delta.x, delta.x),
            g3d_fix_mul(delta.z, delta.z)));
    horizontal_speed = g3d_fix_sqrt(
        g3d_fix_add_sat(
            g3d_fix_mul(velocity.x, velocity.x),
            g3d_fix_mul(velocity.z, velocity.z)));
    if (horizontal_speed <= G3D_FIX_EPSILON)
        return fail("gravity zeroing produced no horizontal velocity");
    flight_time = g3d_fix_div(horizontal_distance, horizontal_speed);
    half_gravity_t2 = g3d_fix_mul((g3d_fix)gravity,
                                  g3d_fix_mul(flight_time, flight_time));
    half_gravity_t2 /= 2L;
    final_y = g3d_fix_add_sat((g3d_fix)grenade.origin.y,
              g3d_fix_add_sat(g3d_fix_mul(velocity.y, flight_time),
                              half_gravity_t2));
    if (g3d_fix_abs(final_y - (g3d_fix)grenade.hit_point.y) >
        G3D_FIX_FROM_INT(1))
        return fail("gravity trajectory does not converge near HUD target");

    puts("Blank3D crosshair convergence + ballistic zeroing test: OK");
    return 0;
}
