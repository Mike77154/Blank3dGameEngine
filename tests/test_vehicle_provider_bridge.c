#include "blank3d_mount_vehicle.h"
#include <stdio.h>

static int move_calls;
static int phys_calls;

static gveh_i32 bridge_move(void *user,
    const vehicleprovider89_movement_request *request)
{
    (void)user;
    if (!request || !request->vehicle) return VEHICLEPROVIDER89_DECLINED;
    move_calls++;
    return VEHICLEPROVIDER89_DECLINED;
}

static gveh_i32 bridge_phys(void *user,
    const vehicleprovider89_physics_request *request)
{
    (void)user;
    if (!request || !request->vehicle) return VEHICLEPROVIDER89_DECLINED;
    phys_calls++;
    return VEHICLEPROVIDER89_DECLINED;
}

int main(void)
{
    Blank3DMountVehicle mv;
    Transform player;
    vehicleprovider89_movement mp;
    vehicleprovider89_physics pp;
    gveh_vehicle *sim;

    transform_init(&player);
    player.position.x = 0;
    player.position.y = 0;
    player.position.z = 0;
    if (!blank3d_mount_vehicle_init(&mv, &player, 1)) return 1;

    vehicleprovider89_movement_clear(&mp);
    mp.module_mask = VEHICLEPROVIDER89_MODULE_CAR;
    mp.step = bridge_move;
    vehicleprovider89_physics_clear(&pp);
    pp.phase_mask = VEHICLEPROVIDER89_PHYS_ALL;
    pp.step = bridge_phys;

    blank3d_mount_vehicle_set_movement_provider(&mv, &mp);
    blank3d_mount_vehicle_set_physics_provider(&mv, &pp);
    blank3d_mount_vehicle_reset(&mv,
        G3D_FIX_FROM_INT(5),
        (g3d_fix)(24L * G3D_FIX_ONE / 10L),
        G3D_FIX_FROM_INT(6));

    sim = gveh_vehicle_world_get(&mv.vehicle_world, mv.vehicle_id);
    if (!sim) return 2;
    if (sim->movement_provider.step != bridge_move) return 3;
    if (sim->physics_provider.step != bridge_phys) return 4;

    move_calls = 0;
    phys_calls = 0;
    if (!blank3d_mount_vehicle_update(&mv, G3D_FIX_ONE / 60)) return 5;
    if (move_calls != 1) return 6;
    if (phys_calls != 4) return 7;

    blank3d_mount_vehicle_clear_providers(&mv);
    if (sim->movement_provider.step != 0) return 8;
    if (sim->physics_provider.step != 0) return 9;

    printf("Blank3D vehicle provider bridge: OK\n");
    return 0;
}
