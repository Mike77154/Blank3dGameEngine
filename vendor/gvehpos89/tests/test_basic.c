#include <stdio.h>
#include "gvpos.h"

static GVPos_Vec3 v3(int x, int y, int z)
{
    GVPos_Vec3 v;
    v.x = GVPOS_FROM_INT(x);
    v.y = GVPOS_FROM_INT(y);
    v.z = GVPOS_FROM_INT(z);
    return v;
}

static int expect(int cond, const char *msg)
{
    if (!cond) {
        printf("FAIL: %s\n", msg);
        return 1;
    }
    return 0;
}

int main(void)
{
    GVPos_Context ctx;
    GVPos_Config cfg;
    GVPos_Actor actors[4];
    GVPos_Vehicle vehicles[2];
    GVPos_Seat seats[4];
    GVPos_Event events[16];
    int fails;
    int r;
    int i;

    fails = 0;
    gvpos_default_config(&cfg);
    cfg.default_enter_ticks = 0;
    cfg.default_exit_ticks = 0;
    gvpos_init(&ctx, &cfg);
    r = gvpos_bind_storage(&ctx, actors, 4, vehicles, 2, seats, 4, events, 16);
    fails += expect(r == GVPOS_OK, "bind storage");
    r = gvpos_add_actor(&ctx, 7, GVPOS_ACTOR_FLAG_PLAYER | GVPOS_ACTOR_FLAG_ALLOW_DRIVE | GVPOS_ACTOR_FLAG_ALLOW_RIDE, v3(0,0,0));
    fails += expect(r == GVPOS_OK, "add actor");
    r = gvpos_add_vehicle(&ctx, 9, GVPOS_VEHICLE_FLAG_USABLE, 0, v3(0,0,0));
    fails += expect(r == GVPOS_OK, "add vehicle");
    r = gvpos_add_seat(&ctx, 9, 0, GVPOS_SEAT_FLAG_DRIVER | GVPOS_SEAT_FLAG_ALLOW_PLAYER, GVPOS_FROM_INT(2), v3(0,0,0), v3(1,0,0));
    fails += expect(r == GVPOS_OK, "add driver seat");
    r = gvpos_request_mount(&ctx, 7, 9, 0, GVPOS_REQ_CHECK_DISTANCE);
    fails += expect(r == GVPOS_OK, "mount ok");
    fails += expect(gvpos_actor_is_mounted(&ctx, 7) == GVPOS_TRUE, "actor mounted immediately");
    fails += expect(gvpos_vehicle_driver(&ctx, 9) == 7, "driver set");
    r = gvpos_request_exit(&ctx, 7, 0);
    fails += expect(r == GVPOS_OK, "exit ok");
    fails += expect(gvpos_actor_is_mounted(&ctx, 7) == GVPOS_FALSE, "actor unmounted");
    for (i = 0; i < 2; ++i) {
        gvpos_update(&ctx);
    }
    if (fails == 0) {
        printf("PASS\n");
    }
    return fails == 0 ? 0 : 1;
}
