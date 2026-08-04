#include "blank3d_npc_inventory.h"
#include "blank3d_weapon_ini.h"

#include <stdio.h>
#include <string.h>

static int equipped_id(const GWP89_Manager *manager, int actor_id)
{
    int user_slot;
    user_slot = gwp89_find_user_slot(manager, actor_id);
    if (user_slot < 0) return 0;
    return manager->users[user_slot].weapon_id;
}

int main(void)
{
    GWP89_Manager manager;
    Blank3DNpcInventoryBank bank;
    char status[160];
    int loaded;
    int result;

    gwp89_init(&manager);
    loaded = blank3d_weapon_ini_load_manifest(&manager,
        "tests/data/npc_custom/weapons.ini", status, sizeof(status));
    if (loaded != 2 || manager.weapon_count != 2) {
        printf("custom weapon manifest failed: %s loaded=%d count=%d\n",
               status, loaded, manager.weapon_count);
        return 1;
    }

    blank3d_npc_inventory_bank_init(&bank);
    if (gwp89_add_provider(&manager, GWP89_SERVICE_INVENTORY, 180,
            "test.npc-inventory", &bank,
            blank3d_npc_inventory_provider) < 0) {
        puts("NPC inventory provider registration failed");
        return 2;
    }
    if (gwp89_bind_actor(&manager, 1000, 2, 2) < 0) return 3;

    loaded = blank3d_npc_inventory_load_ini(&bank, &manager, 1000,
        "tests/data/npc_custom/loadout.ini", status, sizeof(status));
    if (loaded <= 0 || equipped_id(&manager, 1000) != 42) {
        printf("custom NPC loadout failed: %s equipped=%d\n",
               status, equipped_id(&manager, 1000));
        return 4;
    }
    if (!blank3d_npc_inventory_has_name(&bank, &manager, 1000,
                                         "arc_thrower") ||
        gwp89_query_clip(&manager, 1000, 42) != 17 ||
        gwp89_query_ammo(&manager, 1000, 11, 42) != 90) {
        puts("custom ownership, clip or ammo resolution failed");
        return 5;
    }

    result = blank3d_npc_inventory_equip_name(&bank, &manager, 1000,
                                               "arc_thrower");
    if (result != GWP89_OK || equipped_id(&manager, 1000) != 77 ||
        gwp89_query_clip(&manager, 1000, 77) != 2) {
        puts("runtime equip by custom INI name failed");
        return 6;
    }
    (void)gwp89_set_clip(&manager, 1000, 77, 1);
    if (blank3d_npc_inventory_equip_name(&bank, &manager, 1000,
                                         "plasma_carbine") != GWP89_OK ||
        gwp89_query_clip(&manager, 1000, 42) != 17) {
        puts("per-weapon clip persistence failed on first switch");
        return 7;
    }
    if (blank3d_npc_inventory_equip_name(&bank, &manager, 1000,
                                         "arc_thrower") != GWP89_OK ||
        gwp89_query_clip(&manager, 1000, 77) != 1) {
        puts("per-weapon clip persistence failed on return switch");
        return 8;
    }

    if (gwp89_bind_actor(&manager, 1001, 2, 2) < 0) return 9;
    loaded = blank3d_npc_inventory_load_ini(&bank, &manager, 1001,
        "tests/data/npc_custom/loadout.ini", status, sizeof(status));
    if (loaded <= 0) return 10;
    (void)gwp89_set_clip(&manager, 1001, 42, 5);
    if (gwp89_query_clip(&manager, 1000, 42) != 17 ||
        gwp89_query_clip(&manager, 1001, 42) != 5) {
        puts("per-NPC clip isolation failed");
        return 11;
    }
    (void)gwp89_set_ammo(&manager, 1001, 11, 7);
    if (gwp89_query_ammo(&manager, 1000, 11, 42) != 90 ||
        gwp89_query_ammo(&manager, 1001, 11, 42) != 7) {
        puts("per-NPC reserve isolation failed");
        return 12;
    }

    if (blank3d_npc_inventory_take_id(&bank, 1000, 77) != GWP89_OK ||
        blank3d_npc_inventory_equip_id(&bank, &manager, 1000, 77)
            != GWP89_NOT_FOUND) {
        puts("ownership gate failed");
        return 13;
    }

    puts("Blank3D NPC arbitrary INI weapon inventory test: OK");
    return 0;
}
