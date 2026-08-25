#include "blank3d_pickups.h"
#include <stdio.h>
#include <string.h>

#define TEST_UZI_WEAPON_ID 11
#define TEST_UZI_AMMO_ITEM_ID 110

static void tick(Blank3DSystems *systems)
{
    GWP89_Vec3 origin;
    GWP89_Vec3 forward;
    GWP89_Vec3 right;
    GWP89_Vec3 up;
    memset(&origin, 0, sizeof(origin));
    memset(&forward, 0, sizeof(forward));
    memset(&right, 0, sizeof(right));
    memset(&up, 0, sizeof(up));
    forward.z = -GWP89_FIX_ONE;
    right.x = GWP89_FIX_ONE;
    up.y = GWP89_FIX_ONE;
    blank3d_systems_update(systems, 16U, 0, 0, 0,
                           GWP89_VIEW_FPS, GWP89_FIX_ONE,
                           &origin, &forward, &right, &up,
                           &origin, &forward);
}

int main(void)
{
    Blank3DSystems systems;
    Blank3DPickupWorld pickups;
    Transform player;
    unsigned int health_slot;
    const Blank3DPickupInstance *health_pickup;
    const char *health_rpy_args[5];

    blank3d_systems_init_from_ini(&systems, "config/weapons/weapons.ini");
    transform_init(&player);
    blank3d_pickups_init(&pickups, &systems, &player);

    health_rpy_args[0] = "config/pickups/health_box.ini";
    health_rpy_args[1] = "pos";
    health_rpy_args[2] = "4";
    health_rpy_args[3] = "0";
    health_rpy_args[4] = "4";
    if (!blank3d_pickups_spawn_rpyl_args(&pickups, health_rpy_args, 5,
                                         0, &health_slot)) return 1;
    health_pickup = blank3d_pickups_get_const(&pickups, health_slot);
    if (!health_pickup) return 2;
    if (health_pickup->kind != B3D_PICKUP_KIND_HEALTH) return 3;
    if (health_pickup->resource_id != 0) return 4;
    if (health_pickup->amount != 25) return 5;
    if (health_pickup->part_count != 1) return 6;
    if (!blank3d_pickups_is_world_active(&pickups, health_slot)) return 7;

    /* A full-health player must NOT consume the health box. */
    player.position.x = G3D_FIX_FROM_INT(4);
    player.position.y = 0;
    player.position.z = G3D_FIX_FROM_INT(4);
    tick(&systems);
    if (blank3d_systems_player_health(&systems) != 100) return 8;
    if (!blank3d_pickups_is_world_active(&pickups, health_slot)) return 9;

    /* Leave the trigger so the next entry is a fresh CT89 ENTER. */
    player.position.x = G3D_FIX_FROM_INT(0);
    player.position.z = G3D_FIX_FROM_INT(0);
    tick(&systems);

    blank3d_systems_damage_player(&systems, 40);
    if (blank3d_systems_player_health(&systems) != 60) return 10;

    player.position.x = G3D_FIX_FROM_INT(4);
    player.position.z = G3D_FIX_FROM_INT(4);
    tick(&systems);
    if (blank3d_systems_player_health(&systems) != 85) return 11;
    if (blank3d_pickups_is_world_active(&pickups, health_slot)) return 12;

    /* Prove this item did not route through the weapon/ammo provider. */
    if (blank3d_systems_inventory_count(&systems, TEST_UZI_WEAPON_ID) != 0)
        return 13;
    if (blank3d_systems_inventory_count(&systems, TEST_UZI_AMMO_ITEM_ID) != 0)
        return 14;

    printf("generic health pickup: PASS (PBB rule/effect, no weapon provider)\n");
    return 0;
}
