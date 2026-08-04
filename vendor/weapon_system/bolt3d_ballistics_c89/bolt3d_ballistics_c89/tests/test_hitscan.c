#include <stdio.h>
#include "bolt3d/bolt3d.h"

#define TEST_PROJECTILES 1
#define TEST_EVENTS 16
#define TEST_COLLIDERS 4

static B3D_Projectile g_projectiles[TEST_PROJECTILES];
static B3D_Event g_events[TEST_EVENTS];
static B3D_Collider g_colliders[TEST_COLLIDERS];

int main(void)
{
    B3D_World world;
    B3D_Hit hit;
    int result;

    b3d_world_init(&world, g_projectiles, TEST_PROJECTILES, g_events, TEST_EVENTS, g_colliders, TEST_COLLIDERS);
    b3d_world_add_collider(&world, 333, B3D_LAYER_ENEMY, b3d_vec3(b3d_fixed_from_int(12), 0, 0), b3d_fixed_from_int(1), 0, 0);

    result = b3d_fire_hitscan(
        &world,
        1,
        b3d_vec3(0, 0, 0),
        b3d_vec3(B3D_FIXED_ONE, 0, 0),
        b3d_fixed_from_int(20),
        b3d_fixed_from_int(9),
        B3D_DAMAGE_BULLET,
        B3D_LAYER_ENEMY,
        &hit
    );

    if (!result || hit.target_id != 333) {
        printf("FAIL hitscan target=%d\n", hit.target_id);
        return 1;
    }

    printf("test_hitscan OK\n");
    return 0;
}
