#include <stdio.h>
#include <string.h>

#include "blank3d_player_projectile_aim.h"
#include "blank3d_universal_aim.h"
#include "blank3d_weapon_modules.h"

static int fail(const char *message)
{
    printf("FAIL: %s\n", message);
    return 1;
}

static GWP89_Vec3 to_gwp(Vec3 value)
{
    GWP89_Vec3 out;
    out.x = value.x;
    out.y = value.y;
    out.z = value.z;
    return out;
}

static Vec3 from_gwp(GWP89_Vec3 value)
{
    return gamlib_vec3((g3d_fix)value.x,
                       (g3d_fix)value.y,
                       (g3d_fix)value.z);
}

static long abs_long(long value)
{
    return value < 0L ? -value : value;
}

typedef struct RayFixtureTag {
    int call_count;
    int camera_hit;
    int muzzle_hit;
    GWP89_Vec3 camera_point;
    GWP89_Vec3 muzzle_point;
} RayFixture;

static int fixture_raycast(
    void *user,
    const GWP89_Vec3 *origin,
    const GWP89_Vec3 *direction,
    gwp89_fx range_fx,
    unsigned int layer_mask,
    Blank3DCollisionHit *out_hit)
{
    RayFixture *fixture;
    int hit;
    GWP89_Vec3 point;
    (void)origin;
    (void)direction;
    (void)range_fx;
    (void)layer_mask;
    fixture = (RayFixture *)user;
    if (fixture == 0 || out_hit == 0) return 0;
    fixture->call_count++;
    if (fixture->call_count == 1) {
        hit = fixture->camera_hit;
        point = fixture->camera_point;
    } else {
        hit = fixture->muzzle_hit;
        point = fixture->muzzle_point;
    }
    memset(out_hit, 0, sizeof(*out_hit));
    out_hit->enemy_index = -1;
    if (!hit) return 0;
    out_hit->hit = 1;
    out_hit->material_id = B3D_COLLISION_MATERIAL_WORLD;
    out_hit->point = point;
    return 1;
}

static int check_no_hit_mode(int view_mode)
{
    GWP89_Event source;
    GWP89_Event prepared;
    RayFixture fixture;
    Vec3 eye;
    Vec3 forward;
    Vec3 right;
    Vec3 up;
    Vec3 origin;
    Vec3 target;
    Vec3 camera_delta;
    Vec3 expected_direction;
    Vec3 actual_direction;
    g3d_fix target_range;
    g3d_fix dot;

    memset(&source, 0, sizeof(source));
    memset(&fixture, 0, sizeof(fixture));
    source.actor_id = 0;
    source.weapon_id = B3D_WEAPON_ID_ROCKET_LAUNCHER;
    source.range_fx = gwp89_fx_from_int(120);
    source.speed_fx = gwp89_fx_from_int(18);

    eye = gamlib_vec3(G3D_FIX_FROM_INT(2),
                      G3D_FIX_FROM_INT(5),
                      G3D_FIX_FROM_INT(-8));
    forward = gamlib_vec3(0, -G3D_FIX_ONE / 10L, G3D_FIX_ONE);
    gamlib_vec3_normalize(&forward, &forward);
    right = gamlib_vec3(G3D_FIX_ONE, 0, 0);
    up = gamlib_vec3(0, G3D_FIX_ONE, 0);
    origin = gamlib_vec3(0, G3D_FIX_FROM_INT(1), 0);
    source.camera_origin = to_gwp(eye);
    source.camera_forward = to_gwp(forward);
    source.camera_right = to_gwp(right);
    source.camera_up = to_gwp(up);
    source.origin = to_gwp(origin);
    source.muzzle_origin = source.origin;
    source.direction = source.camera_forward;

    if (!blank3d_player_projectile_aim_prepare(
            &source, view_mode, fixture_raycast, &fixture,
            B3D_COLLISION_LAYER_WORLD | B3D_COLLISION_LAYER_ENEMY,
            &prepared))
        return fail("player physical projectile aim rejected empty-space ray");

    target = from_gwp(prepared.hit_point);
    gamlib_vec3_sub(&camera_delta, &target, &eye);
    target_range = gamlib_vec3_length(&camera_delta);
    if (target_range < G3D_FIX_FROM_INT(118) ||
        target_range > G3D_FIX_FROM_INT(121))
        return fail("physical projectile target collapsed to old 32-unit zero plane");

    gamlib_vec3_sub(&expected_direction, &target, &origin);
    gamlib_vec3_normalize(&expected_direction, &expected_direction);
    actual_direction = from_gwp(prepared.direction);
    if (abs_long(actual_direction.x - expected_direction.x) > 4L ||
        abs_long(actual_direction.y - expected_direction.y) > 4L ||
        abs_long(actual_direction.z - expected_direction.z) > 4L)
        return fail("physical projectile did not launch from origin to camera target");

    dot = gamlib_vec3_dot(&actual_direction, &forward);
    if (dot < (G3D_FIX_ONE * 97L) / 100L)
        return fail("physical projectile visibly diverged from reticle ray");
    return 0;
}

static int check_muzzle_obstruction(void)
{
    GWP89_Event source;
    GWP89_Event prepared;
    RayFixture fixture;
    Vec3 eye;
    Vec3 forward;
    Vec3 origin;
    Vec3 camera_point;
    Vec3 muzzle_point;

    memset(&source, 0, sizeof(source));
    memset(&fixture, 0, sizeof(fixture));
    source.actor_id = 0;
    source.weapon_id = B3D_WEAPON_ID_ROCKET_LAUNCHER;
    source.range_fx = gwp89_fx_from_int(120);
    eye = gamlib_vec3(0, G3D_FIX_FROM_INT(4), G3D_FIX_FROM_INT(-8));
    forward = gamlib_vec3(0, 0, G3D_FIX_ONE);
    origin = gamlib_vec3(G3D_FIX_FROM_INT(1),
                         G3D_FIX_FROM_INT(1), 0);
    camera_point = gamlib_vec3(0, G3D_FIX_FROM_INT(4),
                               G3D_FIX_FROM_INT(60));
    muzzle_point = gamlib_vec3(G3D_FIX_FROM_INT(1),
                               G3D_FIX_FROM_INT(2),
                               G3D_FIX_FROM_INT(3));
    source.camera_origin = to_gwp(eye);
    source.camera_forward = to_gwp(forward);
    source.camera_right = to_gwp(gamlib_vec3(G3D_FIX_ONE, 0, 0));
    source.camera_up = to_gwp(gamlib_vec3(0, G3D_FIX_ONE, 0));
    source.origin = to_gwp(origin);
    source.muzzle_origin = source.origin;
    source.direction = source.camera_forward;
    fixture.camera_hit = 1;
    fixture.muzzle_hit = 1;
    fixture.camera_point = to_gwp(camera_point);
    fixture.muzzle_point = to_gwp(muzzle_point);

    if (!blank3d_player_projectile_aim_prepare(
            &source, B3D_PLAYER_FIRE_VIEW_OTS,
            fixture_raycast, &fixture,
            B3D_COLLISION_LAYER_WORLD,
            &prepared))
        return fail("OTS projectile obstruction resolve failed");
    if (prepared.hit_point.x != fixture.muzzle_point.x ||
        prepared.hit_point.y != fixture.muzzle_point.y ||
        prepared.hit_point.z != fixture.muzzle_point.z)
        return fail("OTS physical projectile ignored muzzle obstruction");
    return 0;
}

static int check_backend(int weapon_id)
{
    GWP89_Event source;
    GWP89_Event prepared;
    RayFixture fixture;
    GWP89_Vec3 launch;
    gwp89_fx gravity;
    Vec3 eye;
    Vec3 forward;
    Vec3 origin;
    Vec3 actual;
    g3d_fix dot;
    const Blank3DWeaponModules *modules;

    memset(&source, 0, sizeof(source));
    memset(&fixture, 0, sizeof(fixture));
    source.actor_id = 0;
    source.weapon_id = weapon_id;
    source.range_fx = gwp89_fx_from_int(120);
    source.speed_fx = weapon_id == B3D_WEAPON_ID_ROCKET_LAUNCHER
                    ? gwp89_fx_from_int(18)
                    : (weapon_id == B3D_WEAPON_ID_GRENADE_LAUNCHER
                       ? gwp89_fx_from_int(22)
                       : gwp89_fx_from_int(46));
    eye = gamlib_vec3(G3D_FIX_FROM_INT(2),
                      G3D_FIX_FROM_INT(5),
                      G3D_FIX_FROM_INT(-8));
    forward = gamlib_vec3(0, -G3D_FIX_ONE / 12L, G3D_FIX_ONE);
    gamlib_vec3_normalize(&forward, &forward);
    origin = gamlib_vec3(0, G3D_FIX_FROM_INT(1), 0);
    source.camera_origin = to_gwp(eye);
    source.camera_forward = to_gwp(forward);
    source.camera_right = to_gwp(gamlib_vec3(G3D_FIX_ONE, 0, 0));
    source.camera_up = to_gwp(gamlib_vec3(0, G3D_FIX_ONE, 0));
    source.origin = to_gwp(origin);
    source.muzzle_origin = source.origin;
    source.direction = source.camera_forward;

    if (!blank3d_player_projectile_aim_prepare(
            &source, B3D_PLAYER_FIRE_VIEW_OTS,
            fixture_raycast, &fixture,
            B3D_COLLISION_LAYER_WORLD | B3D_COLLISION_LAYER_ENEMY,
            &prepared))
        return fail("backend camera target preparation failed");

    modules = blank3d_weapon_modules_get(weapon_id);
    if (modules == 0)
        return fail("weapon module missing");
    if (!blank3d_universal_aim_finalize_event(
            &prepared, modules, &launch, &gravity))
        return fail("physical backend launch finalization failed");
    actual = from_gwp(launch);
    dot = gamlib_vec3_dot(&actual, &forward);
    if (dot <= 0)
        return fail("physical backend launched behind the active camera");
    if (weapon_id == B3D_WEAPON_ID_ROCKET_LAUNCHER &&
        dot < (G3D_FIX_ONE * 97L) / 100L)
        return fail("rocket launch diverged from the center camera ray");
    if ((weapon_id == B3D_WEAPON_ID_GRENADE_LAUNCHER ||
         weapon_id == B3D_WEAPON_ID_SLINGSHOT) && gravity >= 0)
        return fail("arc projectile lost negative gravity");
    return 0;
}

int main(void)
{
    blank3d_weapon_modules_load_defaults();
    if (check_no_hit_mode(B3D_PLAYER_FIRE_VIEW_FPS)) return 1;
    if (check_no_hit_mode(B3D_PLAYER_FIRE_VIEW_TPS)) return 1;
    if (check_no_hit_mode(B3D_PLAYER_FIRE_VIEW_OTS)) return 1;
    if (check_muzzle_obstruction()) return 1;
    if (check_backend(B3D_WEAPON_ID_GRENADE_LAUNCHER)) return 1;
    if (check_backend(B3D_WEAPON_ID_ROCKET_LAUNCHER)) return 1;
    if (check_backend(B3D_WEAPON_ID_SLINGSHOT)) return 1;
    puts("Blank3D player physical projectile camera aim test: OK");
    return 0;
}
