#include <stdio.h>
#include "gvpos.h"

typedef struct DemoWorld {
    int visible_actor_1;
    int visible_actor_2;
    int attach_count;
    int drive_ticks;
} DemoWorld;

static void demo_event(GVPos_Context *ctx, const GVPos_Event *event, void *user)
{
    (void)ctx;
    (void)user;
    printf("event=%s actor=%d vehicle=%d seat=%d result=%s\n",
           gvpos_event_name(event->type),
           event->actor_id,
           event->vehicle_id,
           event->seat_slot,
           gvpos_result_name(event->result));
}

static GVPos_Bool demo_can_enter(GVPos_Context *ctx, GVPos_Id actor_id, GVPos_Id vehicle_id, int seat_slot, void *user)
{
    (void)ctx;
    (void)actor_id;
    (void)vehicle_id;
    (void)seat_slot;
    (void)user;
    return GVPOS_TRUE;
}

static GVPos_Bool demo_exit_clear(GVPos_Context *ctx, GVPos_Id actor_id, GVPos_Id vehicle_id, int seat_slot, GVPos_Vec3 exit_pos, GVPos_FP radius, void *user)
{
    (void)ctx;
    (void)actor_id;
    (void)vehicle_id;
    (void)seat_slot;
    (void)exit_pos;
    (void)radius;
    (void)user;
    return GVPOS_TRUE;
}

static void demo_hidden(GVPos_Context *ctx, GVPos_Id actor_id, int hidden, void *user)
{
    DemoWorld *w;
    (void)ctx;
    w = (DemoWorld *)user;
    if (actor_id == 1) {
        w->visible_actor_1 = hidden ? 0 : 1;
    }
    if (actor_id == 2) {
        w->visible_actor_2 = hidden ? 0 : 1;
    }
    printf("actor %d visible=%d\n", actor_id, hidden ? 0 : 1);
}

static void demo_attach(GVPos_Context *ctx, GVPos_Id actor_id, GVPos_Id vehicle_id, int seat_slot, int attached, void *user)
{
    DemoWorld *w;
    (void)ctx;
    w = (DemoWorld *)user;
    if (attached != 0) {
        w->attach_count = w->attach_count + 1;
    }
    printf("actor %d %s vehicle %d seat %d\n", actor_id, attached ? "attached to" : "detached from", vehicle_id, seat_slot);
}

static void demo_vehicle_input(GVPos_Context *ctx, GVPos_Id vehicle_id, GVPos_Id driver_actor_id, const GVPos_Input *input, void *user)
{
    DemoWorld *w;
    (void)ctx;
    w = (DemoWorld *)user;
    w->drive_ticks = w->drive_ticks + 1;
    printf("drive vehicle=%d driver=%d throttle=%d steer=%d\n",
           vehicle_id,
           driver_actor_id,
           GVPOS_TO_INT(input->throttle),
           GVPOS_TO_INT(input->steer));
}

static GVPos_Vec3 v3(int x, int y, int z)
{
    GVPos_Vec3 v;
    v.x = GVPOS_FROM_INT(x);
    v.y = GVPOS_FROM_INT(y);
    v.z = GVPOS_FROM_INT(z);
    return v;
}

int main(void)
{
    GVPos_Context ctx;
    GVPos_Config cfg;
    GVPos_Callbacks cb;
    DemoWorld world;
    unsigned char arena_memory[8192];
    GVPos_Arena arena;
    GVPos_Input input;
    int r;
    int i;

    world.visible_actor_1 = 1;
    world.visible_actor_2 = 1;
    world.attach_count = 0;
    world.drive_ticks = 0;

    gvpos_default_config(&cfg);
    cfg.default_enter_ticks = 2;
    cfg.default_exit_ticks = 2;
    cfg.default_entry_radius = GVPOS_FROM_INT(3);

    gvpos_init(&ctx, &cfg);
    gvpos_arena_init(&arena, arena_memory, sizeof(arena_memory));
    r = gvpos_bind_arena(&ctx, &arena, 8, 4, 16, 32);
    if (r != GVPOS_OK) {
        printf("bind failed: %s\n", gvpos_result_name(r));
        return 1;
    }

    cb.can_enter = demo_can_enter;
    cb.can_exit = 0;
    cb.get_actor_pos = 0;
    cb.get_vehicle_pos = 0;
    cb.test_exit_clear = demo_exit_clear;
    cb.on_event = demo_event;
    cb.set_actor_hidden = demo_hidden;
    cb.set_actor_attached = demo_attach;
    cb.apply_vehicle_input = demo_vehicle_input;
    gvpos_set_callbacks(&ctx, &cb, &world);

    r = gvpos_add_actor(&ctx, 1, GVPOS_ACTOR_FLAG_PLAYER | GVPOS_ACTOR_FLAG_ALLOW_DRIVE | GVPOS_ACTOR_FLAG_ALLOW_RIDE, v3(0, 0, 0));
    if (r != GVPOS_OK) {
        printf("add actor failed\n");
        return 1;
    }
    r = gvpos_add_actor(&ctx, 2, GVPOS_ACTOR_FLAG_NPC | GVPOS_ACTOR_FLAG_ALLOW_DRIVE | GVPOS_ACTOR_FLAG_ALLOW_RIDE, v3(1, 0, 0));
    if (r != GVPOS_OK) {
        printf("add npc failed\n");
        return 1;
    }
    r = gvpos_add_vehicle(&ctx, 100, GVPOS_VEHICLE_FLAG_USABLE | GVPOS_VEHICLE_FLAG_AI_ALLOWED, 0, v3(0, 0, 1));
    if (r != GVPOS_OK) {
        printf("add vehicle failed\n");
        return 1;
    }
    gvpos_add_seat(&ctx, 100, 0, GVPOS_SEAT_FLAG_DRIVER | GVPOS_SEAT_FLAG_ALLOW_PLAYER | GVPOS_SEAT_FLAG_ALLOW_NPC, GVPOS_FROM_INT(3), v3(0, 0, 0), v3(-2, 0, 0));
    gvpos_add_seat(&ctx, 100, 1, GVPOS_SEAT_FLAG_PASSENGER | GVPOS_SEAT_FLAG_ALLOW_PLAYER | GVPOS_SEAT_FLAG_ALLOW_NPC, GVPOS_FROM_INT(3), v3(1, 0, 0), v3(2, 0, 0));

    printf("player enters driver seat\n");
    r = gvpos_request_mount(&ctx, 1, 100, 0, GVPOS_REQ_CHECK_DISTANCE | GVPOS_REQ_CHECK_CALLBACK);
    printf("mount request=%s\n", gvpos_result_name(r));
    for (i = 0; i < 3; ++i) {
        gvpos_update(&ctx);
    }

    gvpos_zero_input(&input);
    input.throttle = GVPOS_FROM_INT(1);
    input.steer = GVPOS_FROM_INT(-1);
    gvpos_set_actor_input(&ctx, 1, &input);
    gvpos_update(&ctx);

    printf("npc enters passenger seat\n");
    r = gvpos_request_mount(&ctx, 2, 100, 1, GVPOS_REQ_CHECK_DISTANCE | GVPOS_REQ_CHECK_CALLBACK);
    printf("npc mount request=%s\n", gvpos_result_name(r));
    for (i = 0; i < 3; ++i) {
        gvpos_update(&ctx);
    }

    printf("player exits\n");
    r = gvpos_request_exit(&ctx, 1, GVPOS_REQ_CHECK_DISTANCE | GVPOS_REQ_CHECK_CALLBACK);
    printf("exit request=%s\n", gvpos_result_name(r));
    for (i = 0; i < 3; ++i) {
        gvpos_update(&ctx);
    }

    printf("final driver=%d drive_ticks=%d visible1=%d visible2=%d\n",
           gvpos_vehicle_driver(&ctx, 100),
           world.drive_ticks,
           world.visible_actor_1,
           world.visible_actor_2);

    return 0;
}
