#include <stdio.h>
#include "bolt3d/bolt3d.h"

#define TEST_PROJECTILES 8
#define TEST_EVENTS 32
#define TEST_COLLIDERS 8

static B3D_Projectile g_projectiles[TEST_PROJECTILES];
static B3D_Event g_events[TEST_EVENTS];
static B3D_Collider g_colliders[TEST_COLLIDERS];

int main(void)
{
    B3D_World world;
    B3D_ProjectileDef bullet;
    B3D_Event event_value;
    int saw_damage;

    b3d_world_init(&world, g_projectiles, TEST_PROJECTILES, g_events, TEST_EVENTS, g_colliders, TEST_COLLIDERS);
    b3d_world_add_collider(&world, 101, B3D_LAYER_ENEMY, b3d_vec3(b3d_fixed_from_int(5), 0, 0), b3d_fixed_from_int(1), 0, 0);

    bullet = b3d_projectile_def_bullet();
    bullet.speed = b3d_fixed_from_int(10);
    bullet.damage = b3d_fixed_from_int(11);
    bullet.flags |= B3D_PROJ_EMIT_MOVE;

    b3d_spawn_projectile(&world, &bullet, 1, b3d_vec3(0, 0, 0), b3d_vec3(B3D_FIXED_ONE, 0, 0));
    b3d_world_update(&world, B3D_FIXED_ONE);

    saw_damage = 0;
    while (b3d_world_poll_event(&world, &event_value)) {
        if (event_value.type == B3D_EVENT_DAMAGE && event_value.target_id == 101) {
            saw_damage = 1;
        }
    }

    if (!saw_damage) {
        printf("FAIL projectile damage not emitted\n");
        return 1;
    }

    printf("test_projectiles OK\n");
    return 0;
}
