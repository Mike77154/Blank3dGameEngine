#include <stdio.h>
#include <string.h>

#include "blank3d_shotgun.h"

static GWP89_Vec3 q12v(int x, int y, int z)
{
    return gwp89_v3(gwp89_fx_from_int(x),
                    gwp89_fx_from_int(y),
                    gwp89_fx_from_int(z));
}

static long dot_q12(GWP89_Vec3 a, GWP89_Vec3 b)
{
    long result;
    result = (a.x * b.x) >> GWP89_FIX_SHIFT;
    result += (a.y * b.y) >> GWP89_FIX_SHIFT;
    result += (a.z * b.z) >> GWP89_FIX_SHIFT;
    return result;
}

int main(void)
{
    GWP89_Event source;
    GWP89_Event pellets[B3D_SHOTGUN_PELLET_COUNT];
    GWP89_Vec3 eye;
    GWP89_Vec3 forward;
    GWP89_Vec3 right;
    GWP89_Vec3 up;
    GWP89_Vec3 target;
    int i;
    int j;
    int distinct_pairs;
    int have_left;
    int have_right;
    int have_up;
    int have_down;

    memset(&source, 0, sizeof(source));
    source.actor_id = 1;
    source.weapon_id = B3D_SHOTGUN_WEAPON_ID;
    source.projectile_id = 3;
    source.projectile_mesh_id = 1; /* finalizer must correct this */
    source.pellet_count = 1;       /* finalizer must restore seven */
    source.origin = q12v(1, 1, 0);
    source.speed_fx = gwp89_fx_from_int(24);
    source.range_fx = gwp89_fx_from_int(28);
    source.spread_fx = gwp89_fx_from_text("7.0");
    source.life_ms = 1500U;
    source.projectile_mesh_scale_fx = gwp89_fx_from_text("0.20");

    eye = q12v(0, 3, -6);
    forward = q12v(0, 0, 1);
    right = q12v(1, 0, 0);
    up = q12v(0, 1, 0);
    target = q12v(0, 3, 26);

    for (i = 0; i < B3D_SHOTGUN_PELLET_COUNT; ++i) {
        source.pellet_index = i;
        if (!blank3d_shotgun_prepare_pellet(
                &source, &eye, &forward, &right, &up, &target,
                &pellets[i]))
            return 1;
        if (pellets[i].pellet_count != B3D_SHOTGUN_PELLET_COUNT)
            return 2;
        if (pellets[i].projectile_id != 3 ||
            pellets[i].projectile_mesh_id != 3)
            return 3;
        if (pellets[i].life_ms < B3D_SHOTGUN_MIN_LIFE_MS)
            return 4;
        if (dot_q12(pellets[i].direction, forward) <= 0)
            return 5;
    }

    distinct_pairs = 0;
    for (i = 0; i < B3D_SHOTGUN_PELLET_COUNT; ++i) {
        for (j = i + 1; j < B3D_SHOTGUN_PELLET_COUNT; ++j) {
            if (pellets[i].direction.x != pellets[j].direction.x ||
                pellets[i].direction.y != pellets[j].direction.y ||
                pellets[i].direction.z != pellets[j].direction.z)
                distinct_pairs++;
        }
    }
    if (distinct_pairs < 18) return 6;

    have_left = 0;
    have_right = 0;
    have_up = 0;
    have_down = 0;
    for (i = 1; i < B3D_SHOTGUN_PELLET_COUNT; ++i) {
        if (pellets[i].direction.x < pellets[0].direction.x) have_left = 1;
        if (pellets[i].direction.x > pellets[0].direction.x) have_right = 1;
        if (pellets[i].direction.y > pellets[0].direction.y) have_up = 1;
        if (pellets[i].direction.y < pellets[0].direction.y) have_down = 1;
    }
    if (!have_left || !have_right || !have_up || !have_down) return 7;

    puts("Blank3D shotgun runtime pellet finalizer test: OK");
    return 0;
}
