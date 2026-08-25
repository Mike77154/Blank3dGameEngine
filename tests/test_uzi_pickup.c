#include "blank3d_pickups.h"
#include <stdio.h>
#include <string.h>

#define TEST_UZI_WEAPON_ID 11
#define TEST_UZI_AMMO_ID 10
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
    unsigned int weapon_slot;
    unsigned int ammo_slot;
    const Blank3DPickupInstance *weapon_pickup;
    const Blank3DPickupInstance *ammo_pickup;
    const char *weapon_rpy_args[7];
    const char *ammo_rpy_args[5];

    blank3d_systems_init_from_ini(&systems, "config/weapons/weapons.ini");
    transform_init(&player);
    blank3d_pickups_init(&pickups, &systems, &player);

    if (!blank3d_weapon_catalog_find_id(&systems.weapon_catalog,
                                         TEST_UZI_WEAPON_ID)) return 1;
    if (blank3d_systems_inventory_count(&systems, TEST_UZI_WEAPON_ID) != 0)
        return 2;
    if (blank3d_systems_inventory_count(&systems, TEST_UZI_AMMO_ITEM_ID) != 0)
        return 3;

    /* Vendor RPYL tokenizes an unquoted slash path into separate command
       arguments.  The host adapter must reconstruct it instead of silently
       skipping the spawn. */
    weapon_rpy_args[0] = "config";
    weapon_rpy_args[1] = "pickups";
    weapon_rpy_args[2] = "uzi_weapon.ini";
    weapon_rpy_args[3] = "pos";
    weapon_rpy_args[4] = "4";
    weapon_rpy_args[5] = "0";
    weapon_rpy_args[6] = "4";
    if (!blank3d_pickups_spawn_rpyl_args(&pickups, weapon_rpy_args, 7,
            B3D_PICKUP_KIND_WEAPON, &weapon_slot)) return 4;

    ammo_rpy_args[0] = "config/pickups/uzi_ammo.ini";
    ammo_rpy_args[1] = "pos";
    ammo_rpy_args[2] = "7";
    ammo_rpy_args[3] = "0";
    ammo_rpy_args[4] = "4";
    if (!blank3d_pickups_spawn_rpyl_args(&pickups, ammo_rpy_args, 5,
            B3D_PICKUP_KIND_AMMO, &ammo_slot)) return 5;

    weapon_pickup = blank3d_pickups_get_const(&pickups, weapon_slot);
    ammo_pickup = blank3d_pickups_get_const(&pickups, ammo_slot);
    if (!weapon_pickup || weapon_pickup->part_count != 2) return 6;
    if (!ammo_pickup || ammo_pickup->part_count != 2) return 7;

    tick(&systems);
    if (!blank3d_pickups_is_world_active(&pickups, weapon_slot)) return 8;
    if (!blank3d_pickups_is_world_active(&pickups, ammo_slot)) return 9;

    /* A zombie-like subject and a world/ground-like subject are not bound
       pickup actors. Even a forced activation must not consume the item. */
    if (blank3d_systems_activate_item_trigger(&systems,
            weapon_pickup->trigger, (CT89_Subject)1001UL) ==
            CT89_ACTION_ACCEPTED) return 17;
    if (!blank3d_pickups_is_world_active(&pickups, weapon_slot)) return 18;
    if (blank3d_systems_activate_item_trigger(&systems,
            weapon_pickup->trigger, (CT89_Subject)900000UL) ==
            CT89_ACTION_ACCEPTED) return 19;
    if (!blank3d_pickups_is_world_active(&pickups, weapon_slot)) return 20;

    player.position.x = G3D_FIX_FROM_INT(4);
    player.position.y = 0;
    player.position.z = G3D_FIX_FROM_INT(4);
    tick(&systems);
    if (blank3d_systems_inventory_count(&systems, TEST_UZI_WEAPON_ID) != 1)
        return 10;
    if (blank3d_systems_weapon_id(&systems) != TEST_UZI_WEAPON_ID) return 11;
    if (blank3d_pickups_is_world_active(&pickups, weapon_slot)) return 12;
    if (!blank3d_pickups_is_world_active(&pickups, ammo_slot)) return 13;

    player.position.x = G3D_FIX_FROM_INT(7);
    player.position.z = G3D_FIX_FROM_INT(4);
    tick(&systems);
    if (blank3d_systems_inventory_count(&systems,
                                         TEST_UZI_AMMO_ITEM_ID) != 64)
        return 14;
    if (blank3d_pickups_is_world_active(&pickups, ammo_slot)) return 15;

    if (TEST_UZI_AMMO_ID != 10) return 16;
    printf("uzi RPY/INI pickup content spine: PASS\n");
    return 0;
}
