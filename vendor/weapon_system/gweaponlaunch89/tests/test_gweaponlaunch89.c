#include <stdio.h>
#include <string.h>

#include "gweaponlaunch89.h"

static GWP89_Vec3 v3(long x, long y, long z)
{
    GWP89_Vec3 v;
    v.x = gwp89_fx_from_int(x);
    v.y = gwp89_fx_from_int(y);
    v.z = gwp89_fx_from_int(z);
    return v;
}

static int fail(const char *message)
{
    printf("gweaponlaunch89: FAIL: %s\n", message);
    return 1;
}

int main(void)
{
    GWP89_Event event;
    GWP89_Event aligned;
    GWP89_Vec3 camera;
    GWP89_Vec3 forward;
    GWP89_Vec3 launch;
    GWL89_PhysicsConfig physics;
    unsigned long flags;
    gwp89_fx gravity;

    memset(&event, 0, sizeof(event));
    event.origin = v3(1, 1, 0);
    event.hit_point = v3(0, 1, -30);
    event.direction = v3(0, 0, -1);
    event.camera_origin = v3(0, 1, 0);
    event.camera_forward = v3(0, 0, 1);
    event.range_fx = gwp89_fx_from_int(30);
    event.speed_fx = gwp89_fx_from_int(20);
    event.view_style = GWP89_VIEW_FPS;
    camera = event.camera_origin;
    forward = event.camera_forward;

    if (!gwl89_align_event_to_view(&event, &camera, &forward, &aligned))
        return fail("reverse-hemisphere alignment rejected event");
    if (aligned.hit_point.z <= camera.z)
        return fail("target remained behind camera");
    if (aligned.direction.z <= 0)
        return fail("projectile direction remained behind camera");

    gwl89_physics_config_defaults(&physics);
    physics.physics_mode = G3DZ89_PHYSICS_LINEAR;
    flags = 0UL;
    gravity = 123;
    if (!gwl89_finalize_event(&aligned, &physics, &launch, &gravity, &flags))
        return fail("linear finalize failed");
    if (launch.z <= 0)
        return fail("final launch direction violated forward hemisphere");
    if (gravity != 0)
        return fail("linear launch unexpectedly retained gravity");

    printf("gweaponlaunch89: OK\n");
    return 0;
}
