#include "gveh.h"
#include "gveh_engine_adapter.h"
#include "gveh_profile_bank.h"
#include "vehicleprovider89.h"
#include <stdio.h>

static gveh_i32 movement_calls;
static gveh_i32 physics_calls;
static gveh_i32 decline_integrate;

static gveh_i32 test_movement(void *user,
    const vehicleprovider89_movement_request *request)
{
    gveh_vec3 force;
    (void)user;
    if (!request || !request->vehicle) return VEHICLEPROVIDER89_DECLINED;
    movement_calls++;
    if (request->module != VEHICLEPROVIDER89_MODULE_CAR)
        return VEHICLEPROVIDER89_DECLINED;
    request->vehicle->grounded_count = 4;
    request->vehicle->grounded_ratio = GVEH_FX_ONE;
    force = gveh_v3_scale(request->basis.fwd, gveh_fx_from_int(25));
    gveh_body_add_force(&request->vehicle->body, force);
    return VEHICLEPROVIDER89_HANDLED;
}

static gveh_i32 test_physics(void *user,
    const vehicleprovider89_physics_request *request)
{
    (void)user;
    if (!request || !request->vehicle) return VEHICLEPROVIDER89_DECLINED;
    physics_calls++;
    if (request->phase != VEHICLEPROVIDER89_PHYS_INTEGRATE)
        return VEHICLEPROVIDER89_DECLINED;
    if (decline_integrate) return VEHICLEPROVIDER89_DECLINED;
    request->vehicle->body.pos.x += gveh_fx_from_int(7);
    return VEHICLEPROVIDER89_HANDLED;
}

int main(void)
{
    gveh_profile profile;
    gveh_vehicle vehicle;
    gveh_runtime runtime;
    gveh_input input;
    vehicleprovider89_movement movement;
    vehicleprovider89_physics physics;
    gveh_vehicle_world world;
    gveh_i16 id;
    gveh_vehicle *spawned;

    if (!gveh_profile_bank_make_by_name("warthog89", &profile)) return 1;
    gveh_runtime_init(&runtime);
    gveh_input_clear(&input);
    input.throttle = GVEH_FX_ONE;
    gveh_vehicle_init(&vehicle, &profile, gveh_v3(0, gveh_fx_from_int(3), 0));

    vehicleprovider89_movement_clear(&movement);
    movement.user = 0;
    movement.module_mask = VEHICLEPROVIDER89_MODULE_CAR;
    movement.step = test_movement;
    vehicleprovider89_physics_clear(&physics);
    physics.user = 0;
    physics.phase_mask = VEHICLEPROVIDER89_PHYS_INTEGRATE;
    physics.step = test_physics;
    gveh_vehicle_set_movement_provider(&vehicle, &movement);
    gveh_vehicle_set_physics_provider(&vehicle, &physics);

    movement_calls = 0;
    physics_calls = 0;
    decline_integrate = 0;
    gveh_vehicle_step(&runtime, &vehicle, &input, GVEH_FX_ONE / 60);
    if (movement_calls != 1) return 2;
    if (physics_calls != 1) return 3;
    if (vehicle.body.pos.x != gveh_fx_from_int(7)) return 4;
    /* Gravity was accumulated by fallback, but provider-owned integration
       prevented the internal body step from turning it into velocity. */
    if (vehicle.body.vel.y != 0) return 5;

    /* A provider may decline a claimed capability at runtime; fallback must
       immediately resume for that phase. */
    gveh_vehicle_init(&vehicle, &profile, gveh_v3(0, gveh_fx_from_int(3), 0));
    gveh_vehicle_set_physics_provider(&vehicle, &physics);
    movement_calls = 0;
    physics_calls = 0;
    decline_integrate = 1;
    gveh_vehicle_step(&runtime, &vehicle, &input, GVEH_FX_ONE / 60);
    if (physics_calls != 1) return 6;
    if (vehicle.body.vel.y >= 0) return 7;

    /* World defaults must reach both existing and future spawned vehicles. */
    gveh_vehicle_world_init(&world);
    gveh_vehicle_world_set_movement_provider(&world, &movement);
    gveh_vehicle_world_set_physics_provider(&world, &physics);
    id = gveh_vehicle_world_spawn(&world, &profile,
        gveh_v3(0, gveh_fx_from_int(3), 0), 1);
    if (id < 0) return 8;
    spawned = gveh_vehicle_world_get(&world, id);
    if (!spawned) return 9;
    if (spawned->movement_provider.step != test_movement) return 10;
    if (spawned->physics_provider.step != test_physics) return 11;
    gveh_vehicle_world_clear_providers(&world);
    if (spawned->movement_provider.step != 0) return 12;
    if (spawned->physics_provider.step != 0) return 13;

    printf("gvehicle89 provider dispatch: OK\n");
    return 0;
}
