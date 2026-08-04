#include <stdio.h>
#include "bolt3d/bolt3d.h"

#define DEMO_PROJECTILES 16
#define DEMO_EVENTS 64
#define DEMO_COLLIDERS 16

static B3D_Projectile g_projectiles[DEMO_PROJECTILES];
static B3D_Event g_events[DEMO_EVENTS];
static B3D_Collider g_colliders[DEMO_COLLIDERS];

static const char *demo_event_name(int type_value)
{
    switch (type_value) {
    case B3D_EVENT_SPAWN:
        return "SPAWN";
    case B3D_EVENT_MOVE:
        return "MOVE";
    case B3D_EVENT_HIT_ENTITY:
        return "HIT_ENTITY";
    case B3D_EVENT_HIT_WORLD:
        return "HIT_WORLD";
    case B3D_EVENT_DAMAGE:
        return "DAMAGE";
    case B3D_EVENT_EXPLODE:
        return "EXPLODE";
    case B3D_EVENT_EXPIRE:
        return "EXPIRE";
    case B3D_EVENT_DESTROY:
        return "DESTROY";
    case B3D_EVENT_HITSCAN:
        return "HITSCAN";
    default:
        return "OTHER";
    }
}

int main(void)
{
    B3D_World world;
    B3D_ProjectileDef bullet;
    B3D_Event event_value;
    B3D_Vec3 origin;
    B3D_Vec3 direction;
    B3D_Fixed dt;
    int projectile_id;
    int frame;

    b3d_world_init(&world, g_projectiles, DEMO_PROJECTILES, g_events, DEMO_EVENTS, g_colliders, DEMO_COLLIDERS);

    b3d_world_add_collider(
        &world,
        1001,
        B3D_LAYER_ENEMY,
        b3d_vec3(b3d_fixed_from_int(8), 0, 0),
        b3d_fixed_from_int(1),
        0,
        0
    );

    bullet = b3d_projectile_def_bullet();
    bullet.speed = b3d_fixed_from_int(12);
    bullet.damage = b3d_fixed_from_int(7);
    bullet.flags |= B3D_PROJ_EMIT_MOVE;

    origin = b3d_vec3(0, 0, 0);
    direction = b3d_vec3(B3D_FIXED_ONE, 0, 0);
    projectile_id = b3d_spawn_projectile(&world, &bullet, 42, origin, direction);
    dt = b3d_fixed_div(b3d_fixed_from_int(1), b3d_fixed_from_int(4));

    printf("spawned projectile id=%d\n", projectile_id);

    for (frame = 0; frame < 8; ++frame) {
        b3d_world_update(&world, dt);
        while (b3d_world_poll_event(&world, &event_value)) {
            printf("frame=%d event=%s target=%d damage=%ld x=%ld\n",
                frame,
                demo_event_name(event_value.type),
                event_value.target_id,
                b3d_fixed_to_int(event_value.damage),
                b3d_fixed_to_int(event_value.position.x));
        }
    }

    return 0;
}
