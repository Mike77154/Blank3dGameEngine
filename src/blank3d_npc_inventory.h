#ifndef BLANK3D_NPC_INVENTORY_H
#define BLANK3D_NPC_INVENTORY_H

#include <stddef.h>
#include "gweapon89.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef B3D_NPC_INV_MAX_ACTORS
#define B3D_NPC_INV_MAX_ACTORS 32
#endif

#ifndef B3D_NPC_INV_MAX_WEAPONS
#define B3D_NPC_INV_MAX_WEAPONS GWP89_MAX_WEAPONS
#endif

#define B3D_NPC_INV_NAME_CAP 64
#define B3D_NPC_INV_LINE_CAP 512

typedef struct Blank3DNpcInventoryTag {
    int active;
    int actor_id;
    int equipped_weapon_id;
    int weapon_count;
    int weapon_ids[B3D_NPC_INV_MAX_WEAPONS];
    int weapon_clips[B3D_NPC_INV_MAX_WEAPONS];
    int ammo_amount[GWP89_MAX_AMMO_TYPES];
} Blank3DNpcInventory;

typedef struct Blank3DNpcInventoryBankTag {
    Blank3DNpcInventory actors[B3D_NPC_INV_MAX_ACTORS];
} Blank3DNpcInventoryBank;

void blank3d_npc_inventory_bank_init(Blank3DNpcInventoryBank *bank);
Blank3DNpcInventory *blank3d_npc_inventory_bind(
    Blank3DNpcInventoryBank *bank, int actor_id);
Blank3DNpcInventory *blank3d_npc_inventory_find(
    Blank3DNpcInventoryBank *bank, int actor_id);
const Blank3DNpcInventory *blank3d_npc_inventory_find_const(
    const Blank3DNpcInventoryBank *bank, int actor_id);

int blank3d_npc_inventory_give_id(Blank3DNpcInventoryBank *bank,
                                  GWP89_Manager *manager,
                                  int actor_id,
                                  int weapon_id,
                                  int fill_clip);
int blank3d_npc_inventory_give_name(Blank3DNpcInventoryBank *bank,
                                    GWP89_Manager *manager,
                                    int actor_id,
                                    const char *weapon_name,
                                    int fill_clip);
int blank3d_npc_inventory_take_id(Blank3DNpcInventoryBank *bank,
                                  int actor_id,
                                  int weapon_id);
int blank3d_npc_inventory_has_id(const Blank3DNpcInventoryBank *bank,
                                 int actor_id,
                                 int weapon_id);
int blank3d_npc_inventory_has_name(const Blank3DNpcInventoryBank *bank,
                                   const GWP89_Manager *manager,
                                   int actor_id,
                                   const char *weapon_name);

int blank3d_npc_inventory_equip_id(Blank3DNpcInventoryBank *bank,
                                   GWP89_Manager *manager,
                                   int actor_id,
                                   int weapon_id);
int blank3d_npc_inventory_equip_name(Blank3DNpcInventoryBank *bank,
                                     GWP89_Manager *manager,
                                     int actor_id,
                                     const char *weapon_name);
int blank3d_npc_inventory_cycle_next(Blank3DNpcInventoryBank *bank,
                                     GWP89_Manager *manager,
                                     int actor_id);
int blank3d_npc_inventory_cycle_prev(Blank3DNpcInventoryBank *bank,
                                     GWP89_Manager *manager,
                                     int actor_id);

int blank3d_npc_inventory_set_ammo(Blank3DNpcInventoryBank *bank,
                                   int actor_id,
                                   int ammo_id,
                                   int amount);
int blank3d_npc_inventory_add_ammo(Blank3DNpcInventoryBank *bank,
                                   int actor_id,
                                   int ammo_id,
                                   int amount);
int blank3d_npc_inventory_ammo(const Blank3DNpcInventoryBank *bank,
                               int actor_id,
                               int ammo_id);
int blank3d_npc_inventory_clip(const Blank3DNpcInventoryBank *bank,
                               int actor_id,
                               int weapon_id);
int blank3d_npc_inventory_weapon_id(const Blank3DNpcInventoryBank *bank,
                                    int actor_id);

int blank3d_npc_inventory_load_ini(Blank3DNpcInventoryBank *bank,
                                   GWP89_Manager *manager,
                                   int actor_id,
                                   const char *path,
                                   char *status,
                                   size_t status_capacity);

/* GWeapon89 inventory provider. Register this on the NPC weapon manager with
   a priority greater than generic fallback providers. */
int blank3d_npc_inventory_provider(void *context,
                                   GWP89_ProviderPacket *packet);

#ifdef __cplusplus
}
#endif

#endif
