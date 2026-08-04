#include <stdio.h>
#include "bolt3d/bolt3d.h"

#define HOOK_PROJECTILES 8
#define HOOK_EVENTS 32
#define HOOK_COLLIDERS 8

static B3D_Projectile g_projectiles[HOOK_PROJECTILES];
static B3D_Event g_events[HOOK_EVENTS];
static B3D_Collider g_colliders[HOOK_COLLIDERS];

typedef struct DemoWorldGeo {
    B3D_Fixed wall_x;
} DemoWorldGeo;

static int trace_wall_x(void *user_data, const B3D_TraceRequest *request, B3D_Hit *out_hit)
{
    DemoWorldGeo *geo;
    B3D_Fixed dx;
    B3D_Fixed t;

    geo = (DemoWorldGeo *)user_data;
    if (geo == 0 || request == 0 || out_hit == 0) {
        return B3D_FALSE;
    }

    b3d_hit_clear(out_hit);
    dx = b3d_fixed_sub_sat(request->to.x, request->from.x);
    if (dx <= 0) {
        return B3D_FALSE;
    }
    if (request->from.x > geo->wall_x || request->to.x < geo->wall_x) {
        return B3D_FALSE;
    }

    t = b3d_fixed_div(b3d_fixed_sub_sat(geo->wall_x, request->from.x), dx);
    if (t < 0 || t > B3D_FIXED_ONE) {
        return B3D_FALSE;
    }

    out_hit->hit = B3D_TRUE;
    out_hit->hit_kind = B3D_HIT_WORLD;
    out_hit->target_id = 700;
    out_hit->target_slot = B3D_ID_NONE;
    out_hit->target_layer = B3D_LAYER_WORLD;
    out_hit->t = t;
    out_hit->point = b3d_vec3_lerp(request->from, request->to, t);
    out_hit->normal = b3d_vec3(b3d_fixed_from_int(-1), 0, 0);
    return B3D_TRUE;
}

static void on_event_print(void *user_data, const B3D_Event *event_value)
{
    (void)user_data;
    if (event_value->type == B3D_EVENT_HIT_WORLD) {
        printf("hook: projectile hit world at x=%ld\n", b3d_fixed_to_int(event_value->position.x));
    }
}

int main(void)
{
    B3D_World world;
    B3D_Callbacks callbacks;
    B3D_ProjectileDef rocket;
    DemoWorldGeo geo;
    B3D_Fixed dt;

    geo.wall_x = b3d_fixed_from_int(5);
    b3d_world_init(&world, g_projectiles, HOOK_PROJECTILES, g_events, HOOK_EVENTS, g_colliders, HOOK_COLLIDERS);
    b3d_callbacks_clear(&callbacks);
    callbacks.user_data = &geo;
    callbacks.trace_world = trace_wall_x;
    callbacks.on_event = on_event_print;
    b3d_world_set_callbacks(&world, &callbacks);

    rocket = b3d_projectile_def_rocket();
    rocket.speed = b3d_fixed_from_int(10);
    rocket.explosion_radius = b3d_fixed_from_int(3);

    b3d_spawn_projectile(&world, &rocket, 1, b3d_vec3(0, 0, 0), b3d_vec3(B3D_FIXED_ONE, 0, 0));
    dt = b3d_fixed_div(b3d_fixed_from_int(1), b3d_fixed_from_int(2));
    b3d_world_update(&world, dt);

    return 0;
}
