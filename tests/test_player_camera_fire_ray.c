#include <stdio.h>
#include <string.h>

#include "blank3d_player_fire_ray.h"

#define TEST_EPS 8

typedef struct MockRayWorldTag {
    int call_count;
    int camera_hit_enabled;
    int muzzle_hit_enabled;
    GWP89_Vec3 camera_hit_point;
    GWP89_Vec3 muzzle_hit_point;
    int camera_enemy_index;
    int muzzle_enemy_index;
    GWP89_Vec3 first_origin;
    GWP89_Vec3 first_direction;
    GWP89_Vec3 second_origin;
    GWP89_Vec3 second_direction;
} MockRayWorld;

static int close_fx(g3d_fix a, g3d_fix b)
{
    g3d_fix delta;
    delta = a >= b ? a - b : b - a;
    return delta <= TEST_EPS;
}

static int close_vec(Vec3 a, Vec3 b)
{
    return close_fx(a.x, b.x) &&
           close_fx(a.y, b.y) &&
           close_fx(a.z, b.z);
}

static GWP89_Vec3 qvec(int x, int y, int z)
{
    GWP89_Vec3 out;
    out.x = G3D_FIX_FROM_INT(x);
    out.y = G3D_FIX_FROM_INT(y);
    out.z = G3D_FIX_FROM_INT(z);
    return out;
}

static int mock_raycast(void *user,
                        const GWP89_Vec3 *origin,
                        const GWP89_Vec3 *direction,
                        gwp89_fx range_fx,
                        unsigned int layer_mask,
                        Blank3DCollisionHit *out_hit)
{
    MockRayWorld *world;
    int is_camera_call;
    (void)range_fx;
    (void)layer_mask;
    world = (MockRayWorld *)user;
    if (!world || !origin || !direction || !out_hit) return 0;
    is_camera_call = world->call_count == 0;
    if (is_camera_call) {
        world->first_origin = *origin;
        world->first_direction = *direction;
    } else {
        world->second_origin = *origin;
        world->second_direction = *direction;
    }
    world->call_count += 1;
    memset(out_hit, 0, sizeof(*out_hit));
    out_hit->enemy_index = -1;
    if (is_camera_call && world->camera_hit_enabled) {
        out_hit->hit = 1;
        out_hit->enemy_index = world->camera_enemy_index;
        out_hit->material_id = world->camera_enemy_index >= 0
                             ? B3D_COLLISION_MATERIAL_FLESH
                             : B3D_COLLISION_MATERIAL_WORLD;
        out_hit->point = world->camera_hit_point;
        out_hit->distance_fx = G3D_FIX_FROM_INT(20);
        return 1;
    }
    if (!is_camera_call && world->muzzle_hit_enabled) {
        out_hit->hit = 1;
        out_hit->enemy_index = world->muzzle_enemy_index;
        out_hit->material_id = world->muzzle_enemy_index >= 0
                             ? B3D_COLLISION_MATERIAL_FLESH
                             : B3D_COLLISION_MATERIAL_WORLD;
        out_hit->point = world->muzzle_hit_point;
        out_hit->distance_fx = G3D_FIX_FROM_INT(3);
        return 1;
    }
    return 0;
}

static Blank3DPlayerFireRayInput default_input(void)
{
    Blank3DPlayerFireRayInput input;
    memset(&input, 0, sizeof(input));
    input.view_mode = B3D_PLAYER_FIRE_VIEW_TPS;
    input.camera_origin = gamlib_vec3(0, G3D_FIX_FROM_INT(2), 0);
    input.camera_forward = gamlib_vec3(0, 0, G3D_FIX_ONE);
    input.camera_right = gamlib_vec3(G3D_FIX_ONE, 0, 0);
    input.camera_up = gamlib_vec3(0, G3D_FIX_ONE, 0);
    input.muzzle_origin = gamlib_vec3(G3D_FIX_FROM_INT(1),
                                      G3D_FIX_FROM_INT(1),
                                      G3D_FIX_FROM_INT(2));
    input.range = G3D_FIX_FROM_INT(100);
    input.spread_degrees = 0;
    input.pellet_index = 0;
    input.pellet_count = 1;
    input.layer_mask = B3D_COLLISION_LAYER_WORLD |
                       B3D_COLLISION_LAYER_ENEMY;
    return input;
}

static int test_fps_camera_is_authority(void)
{
    Blank3DPlayerFireRayInput input;
    Blank3DPlayerFireRayResult result;
    MockRayWorld world;
    input = default_input();
    input.view_mode = B3D_PLAYER_FIRE_VIEW_FPS;
    memset(&world, 0, sizeof(world));
    world.camera_hit_enabled = 1;
    world.camera_hit_point = qvec(0, 2, 25);
    world.camera_enemy_index = 4;
    if (!blank3d_player_fire_ray_resolve(&input, mock_raycast,
                                         &world, &result)) return 0;
    if (world.call_count != 1) return 0;
    if (!close_vec(result.tracer_origin, input.camera_origin)) return 0;
    if (!close_vec(result.impact_point,
                   gamlib_vec3(0, G3D_FIX_FROM_INT(2),
                               G3D_FIX_FROM_INT(25)))) return 0;
    if (result.hit.enemy_index != 4) return 0;
    return 1;
}

static int test_tps_uses_camera_then_muzzle_cover(void)
{
    Blank3DPlayerFireRayInput input;
    Blank3DPlayerFireRayResult result;
    MockRayWorld world;
    input = default_input();
    memset(&world, 0, sizeof(world));
    world.camera_hit_enabled = 1;
    world.camera_hit_point = qvec(0, 2, 30);
    world.camera_enemy_index = 2;
    world.muzzle_hit_enabled = 1;
    world.muzzle_hit_point = qvec(1, 1, 5);
    world.muzzle_enemy_index = -1;
    if (!blank3d_player_fire_ray_resolve(&input, mock_raycast,
                                         &world, &result)) return 0;
    if (world.call_count != 2) return 0;
    if (!result.blocked_from_muzzle) return 0;
    if (!close_vec(result.tracer_origin, input.muzzle_origin)) return 0;
    if (!close_vec(result.impact_point,
                   gamlib_vec3(G3D_FIX_FROM_INT(1),
                               G3D_FIX_FROM_INT(1),
                               G3D_FIX_FROM_INT(5)))) return 0;
    if (result.hit.material_id != B3D_COLLISION_MATERIAL_WORLD) return 0;
    return 1;
}

static int test_ots_reaches_camera_target_when_clear(void)
{
    Blank3DPlayerFireRayInput input;
    Blank3DPlayerFireRayResult result;
    MockRayWorld world;
    input = default_input();
    input.view_mode = B3D_PLAYER_FIRE_VIEW_OTS;
    memset(&world, 0, sizeof(world));
    world.camera_hit_enabled = 1;
    world.camera_hit_point = qvec(0, 2, 40);
    world.camera_enemy_index = 1;
    if (!blank3d_player_fire_ray_resolve(&input, mock_raycast,
                                         &world, &result)) return 0;
    if (world.call_count != 2) return 0;
    if (result.blocked_from_muzzle) return 0;
    if (result.hit.enemy_index != 1) return 0;
    if (!close_vec(result.impact_point,
                   gamlib_vec3(0, G3D_FIX_FROM_INT(2),
                               G3D_FIX_FROM_INT(40)))) return 0;
    return 1;
}

static int test_shotgun_spread_stays_forward(void)
{
    Blank3DPlayerFireRayInput input;
    Blank3DPlayerFireRayResult result;
    MockRayWorld world;
    int i;
    input = default_input();
    input.pellet_count = 7;
    input.spread_degrees = G3D_FIX_FROM_INT(7);
    for (i = 0; i < 7; ++i) {
        memset(&world, 0, sizeof(world));
        input.pellet_index = i;
        if (!blank3d_player_fire_ray_resolve(&input, mock_raycast,
                                             &world, &result)) return 0;
        if (gamlib_vec3_dot(&result.camera_ray_direction,
                            &input.camera_forward) <= 0) return 0;
        if (gamlib_vec3_dot(&result.tracer_direction,
                            &input.camera_forward) <= 0) return 0;
    }
    return 1;
}

int main(void)
{
    if (!test_fps_camera_is_authority()) {
        puts("FAIL: FPS camera authority");
        return 1;
    }
    if (!test_tps_uses_camera_then_muzzle_cover()) {
        puts("FAIL: TPS two-ray cover validation");
        return 1;
    }
    if (!test_ots_reaches_camera_target_when_clear()) {
        puts("FAIL: OTS camera target convergence");
        return 1;
    }
    if (!test_shotgun_spread_stays_forward()) {
        puts("FAIL: shotgun camera spread");
        return 1;
    }
    puts("PASS: player-only camera ray authority (FPS/TPS/OTS)");
    return 0;
}
