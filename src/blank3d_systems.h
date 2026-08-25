#ifndef BLANK3D_SYSTEMS_H
#define BLANK3D_SYSTEMS_H

#include "numsys.h"
#include "flagstore.h"
#include "gkinv.h"
#include "gweapon89.h"
#include "blank3d_weapon_modules.h"
#include "blank3d_weapon_loadout.h"
#include "blank3d_list_cycle.h"
#include "3d_contact_trigger89.h"
#include "pbb_item_system.h"
#include "pbb_contact_weapon_bridge89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_SYSTEMS_PICKUP_KIND_HEALTH 3
#define B3D_PLAYER_ACTOR_ID 1
#define B3D_PLAYER_KIND 1
#define B3D_PLAYER_TEAM 1
#define B3D_INV_SLOT_CAPACITY 32
#define B3D_FLAG_CAPACITY 64
#define B3D_FLAG_POOL_CAPACITY 4096
#define B3D_ITEM_COUNT 32
#define B3D_WEAPON_CLIP_CAPACITY GWP89_MAX_WEAPONS

typedef struct Blank3DSystemsTag {
    NS_World numbers;
    ns_id player_health_value;
    ns_id damage_multiplier_value;
    ns_id speed_multiplier_value;
    ns_id recoil_multiplier_value;

    FlagStore flags;
    FlagStoreEntry flag_entries[B3D_FLAG_CAPACITY];
    char flag_pool[B3D_FLAG_POOL_CAPACITY];

    gkinv_ItemDef item_defs[B3D_ITEM_COUNT];
    gkinv_ItemDB item_db;
    gkinv_Slot inventory_slots[B3D_INV_SLOT_CAPACITY];
    gkinv_Container inventory;

    Blank3DWeaponCatalog weapon_catalog;
    Blank3DPlayerWeaponLoadout player_weapon_loadout;

    /* Vendored world-item/contact spine. Spatial collision remains a provider
       of CT89; PBB owns item rules/effects; the bridge gates weapon grants. */
    CT89_Context contact_triggers;
    PBB_ItemWorld item_world;
    PBBCTW89_Bridge item_contact_bridge;
    int player_item_actor_id;

    /* One persistent magazine per weapon id. The weapon manager asks this
       inventory provider for CLIP_QUERY/CLIP_SET, so switching weapons never
       gifts free ammunition and never transfers one weapon's clip to another. */
    int weapon_clips[B3D_WEAPON_CLIP_CAPACITY];

    GWP89_Manager weapons;
    int last_trigger_down;
    int current_weapon_id;
    int starting_weapon_id;
    /* Absolute Q16.16 launch speed for the next charge-release shot.
       Zero means use the profile's normal speed. */
    long launch_speed_override_q16;
    char status[192];
} Blank3DSystems;

void blank3d_systems_init(Blank3DSystems *systems);
void blank3d_systems_init_from_ini(Blank3DSystems *systems,
                                   const char *weapon_manifest_path);
void blank3d_systems_reset_combat(Blank3DSystems *systems);
void blank3d_systems_update(Blank3DSystems *systems,
                            unsigned short dt_ms,
                            int trigger_down,
                            int trigger_pressed,
                            int trigger_released,
                            int view_style,
                            gwp89_fx zoom_fx,
                            const GWP89_Vec3 *socket_origin,
                            const GWP89_Vec3 *socket_forward,
                            const GWP89_Vec3 *socket_right,
                            const GWP89_Vec3 *socket_up,
                            const GWP89_Vec3 *camera_origin,
                            const GWP89_Vec3 *camera_forward);

int blank3d_systems_reload(Blank3DSystems *systems);
int blank3d_systems_active_reload(Blank3DSystems *systems);
int blank3d_systems_cycle_next(Blank3DSystems *systems);
int blank3d_systems_cycle_prev(Blank3DSystems *systems);
int blank3d_systems_register_cycle_lists(Blank3DSystems *systems,
                                         Blank3DListCycleRegistry *registry);
int blank3d_systems_equip_id(Blank3DSystems *systems, int weapon_id);
int blank3d_systems_poll_event(Blank3DSystems *systems, GWP89_Event *event_out);

int blank3d_systems_player_health(const Blank3DSystems *systems);
void blank3d_systems_damage_player(Blank3DSystems *systems, int amount);
void blank3d_systems_heal_player(Blank3DSystems *systems, int amount);
int blank3d_systems_clip(Blank3DSystems *systems);
int blank3d_systems_reserve(Blank3DSystems *systems);
int blank3d_systems_weapon_id(const Blank3DSystems *systems);
int blank3d_systems_ammo_id(const Blank3DSystems *systems);
const char *blank3d_systems_weapon_name(const Blank3DSystems *systems);
const char *blank3d_systems_status(const Blank3DSystems *systems);

int blank3d_systems_set_flag(Blank3DSystems *systems, const char *key, int value);
int blank3d_systems_get_flag(const Blank3DSystems *systems, const char *key, int fallback);
int blank3d_systems_inventory_count(const Blank3DSystems *systems, int item_id);
int blank3d_systems_grant_weapon(Blank3DSystems *systems, int actor_id,
                                 int weapon_id, int amount, int auto_equip);
int blank3d_systems_grant_ammo(Blank3DSystems *systems, int actor_id,
                               int ammo_id, int amount);
CT89_Context *blank3d_systems_contact_triggers(Blank3DSystems *systems);
PBB_ItemWorld *blank3d_systems_item_world(Blank3DSystems *systems);
PBBCTW89_Bridge *blank3d_systems_item_contact_bridge(Blank3DSystems *systems);
void blank3d_systems_reset_item_contact(Blank3DSystems *systems);
int blank3d_systems_player_item_actor(const Blank3DSystems *systems);
int blank3d_systems_bind_item_actor(Blank3DSystems *systems,
                                    CT89_Subject subject,
                                    int host_actor_id,
                                    unsigned long class_mask,
                                    unsigned long team_mask,
                                    unsigned long touch_mask,
                                    unsigned long interact_mask);
int blank3d_systems_activate_item_trigger(Blank3DSystems *systems,
                                           CT89_Trigger trigger,
                                           CT89_Subject activator_subject);
int blank3d_systems_define_weapon_pickup(Blank3DSystems *systems,
                                         const char *name,
                                         int weapon_id,
                                         int amount,
                                         int auto_equip,
                                         int interact_required,
                                         CT89_Subject owner_subject,
                                         int sensor_mode,
                                         CT89_FX radius_fx,
                                         unsigned long contact_category_mask,
                                         int consume_policy,
                                         int *out_item_id,
                                         CT89_Trigger *out_trigger);
int blank3d_systems_define_ammo_pickup(Blank3DSystems *systems,
                                       const char *name,
                                       int ammo_id,
                                       int amount,
                                       int interact_required,
                                       CT89_Subject owner_subject,
                                       int sensor_mode,
                                       CT89_FX radius_fx,
                                       unsigned long contact_category_mask,
                                       int consume_policy,
                                       int *out_item_id,
                                       CT89_Trigger *out_trigger);
int blank3d_systems_define_health_pickup(Blank3DSystems *systems,
                                         const char *name,
                                         int amount,
                                         int interact_required,
                                         CT89_Subject owner_subject,
                                         int sensor_mode,
                                         CT89_FX radius_fx,
                                         unsigned long contact_category_mask,
                                         int consume_policy,
                                         int *out_item_id,
                                         CT89_Trigger *out_trigger);
int blank3d_systems_set_multiplier_q16(Blank3DSystems *systems, int numeric_key,
                                       long value_q16);
long blank3d_systems_get_multiplier_q16(const Blank3DSystems *systems,
                                        int numeric_key, long fallback_q16);
void blank3d_systems_set_launch_speed_q16(Blank3DSystems *systems,
                                          long speed_q16);

#ifdef __cplusplus
}
#endif

#endif
