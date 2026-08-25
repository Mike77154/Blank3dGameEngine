#ifndef PBB_CONTACT_WEAPON_BRIDGE89_H
#define PBB_CONTACT_WEAPON_BRIDGE89_H

#include "3d_contact_trigger89.h"
#include "pbb_item_system.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PBBCTW89_MAX_ACTOR_BINDINGS
#define PBBCTW89_MAX_ACTOR_BINDINGS PBB_ITEM_MAX_ACTORS
#endif
#ifndef PBBCTW89_MAX_ITEM_BINDINGS
#define PBBCTW89_MAX_ITEM_BINDINGS PBB_ITEM_MAX_ITEMS
#endif

#define PBBCTW89_HOOK_BIT(hook_id) (1UL << (unsigned long)(hook_id))
#define PBBCTW89_HOOK_TOUCH PBBCTW89_HOOK_BIT(PBB_ITEM_HOOK_TOUCH)
#define PBBCTW89_HOOK_INTERACT PBBCTW89_HOOK_BIT(PBB_ITEM_HOOK_INTERACT)
#define PBBCTW89_HOOK_DROP PBBCTW89_HOOK_BIT(PBB_ITEM_HOOK_DROP)
#define PBBCTW89_HOOK_SPAWN PBBCTW89_HOOK_BIT(PBB_ITEM_HOOK_SPAWN)

#define PBBCTW89_ACTION_TOUCH 0x504201
#define PBBCTW89_ACTION_INTERACT 0x504202

enum {
    PBBCTW89_OK = 0,
    PBBCTW89_ERR_ARGUMENT = -1,
    PBBCTW89_ERR_FULL = -2,
    PBBCTW89_ERR_NOT_FOUND = -3,
    PBBCTW89_ERR_PROVIDER = -4,
    PBBCTW89_ERR_TRIGGER = -5
};

enum {
    PBBCTW89_PICKUP_NONE = 0,
    PBBCTW89_PICKUP_WEAPON = 1,
    PBBCTW89_PICKUP_AMMO = 2
};

typedef int (*PBBCTW89_GrantWeaponFn)(void *user,
                                       int host_actor_id,
                                       int weapon_id,
                                       int amount,
                                       int auto_equip);
typedef int (*PBBCTW89_GrantAmmoFn)(void *user,
                                     int host_actor_id,
                                     int ammo_id,
                                     int amount);

typedef struct PBBCTW89_WeaponProviderTag {
    void *user;
    PBBCTW89_GrantWeaponFn grant_weapon;
    PBBCTW89_GrantAmmoFn grant_ammo;
} PBBCTW89_WeaponProvider;

typedef struct PBBCTW89_ActorBindingTag {
    unsigned char used;
    CT89_Subject subject;
    int pbb_actor_id;
    int host_actor_id;
} PBBCTW89_ActorBinding;

typedef struct PBBCTW89_ItemBindingTag {
    unsigned char used;
    CT89_Trigger trigger;
    CT89_Subject owner_subject;
    int pbb_item_id;
    int action_id;
} PBBCTW89_ItemBinding;

typedef struct PBBCTW89_PickupBindingTag {
    unsigned char used;
    unsigned char granted;
    int pbb_item_id;
    int pickup_kind;
    int resource_id;
    int amount;
    int auto_equip;
    unsigned long hook_mask;
} PBBCTW89_PickupBinding;

typedef struct PBBCTW89_BridgeTag {
    CT89_Context *contact;
    PBB_ItemWorld *items;
    PBBCTW89_WeaponProvider weapon_provider;
    CT89_ActionProvider previous_action_provider;
    PBB_ItemActionGateFn previous_action_gate;
    void *previous_action_gate_user;
    PBBCTW89_ActorBinding actors[PBBCTW89_MAX_ACTOR_BINDINGS];
    PBBCTW89_ItemBinding item_bindings[PBBCTW89_MAX_ITEM_BINDINGS];
    PBBCTW89_PickupBinding pickup_bindings[PBBCTW89_MAX_ITEM_BINDINGS];
    int attached;
    int last_result;
} PBBCTW89_Bridge;

void pbbctw89_weapon_provider_init(PBBCTW89_WeaponProvider *provider);
void pbbctw89_init(PBBCTW89_Bridge *bridge);
int pbbctw89_attach(PBBCTW89_Bridge *bridge,
                    CT89_Context *contact,
                    PBB_ItemWorld *items);
void pbbctw89_detach(PBBCTW89_Bridge *bridge);
void pbbctw89_set_weapon_provider(PBBCTW89_Bridge *bridge,
                                  const PBBCTW89_WeaponProvider *provider);

int pbbctw89_bind_actor(PBBCTW89_Bridge *bridge,
                        CT89_Subject subject,
                        int pbb_actor_id,
                        int host_actor_id);
int pbbctw89_unbind_actor(PBBCTW89_Bridge *bridge, CT89_Subject subject);
int pbbctw89_bind_item_trigger(PBBCTW89_Bridge *bridge,
                               CT89_Trigger trigger,
                               CT89_Subject owner_subject,
                               int pbb_item_id,
                               int action_id);
int pbbctw89_unbind_item_trigger(PBBCTW89_Bridge *bridge,
                                 CT89_Trigger trigger);

int pbbctw89_bind_weapon_pickup(PBBCTW89_Bridge *bridge,
                                 int pbb_item_id,
                                 int weapon_id,
                                 int amount,
                                 int auto_equip,
                                 unsigned long hook_mask);
int pbbctw89_bind_ammo_pickup(PBBCTW89_Bridge *bridge,
                               int pbb_item_id,
                               int ammo_id,
                               int amount,
                               unsigned long hook_mask);
int pbbctw89_clear_pickup_binding(PBBCTW89_Bridge *bridge, int pbb_item_id);
int pbbctw89_reset_pickup(PBBCTW89_Bridge *bridge, int pbb_item_id);

CT89_Trigger pbbctw89_create_item_trigger(PBBCTW89_Bridge *bridge,
                                           CT89_Subject owner_subject,
                                           int pbb_item_id,
                                           int sensor_mode,
                                           CT89_FX radius_fx,
                                           unsigned long category_mask,
                                           int action_id,
                                           int consume_policy);

int pbbctw89_contact_action(void *user, const CT89_Event *event);
int pbbctw89_item_action_gate(PBB_ItemWorld *world,
                              int pbb_actor_id,
                              int pbb_item_id,
                              int hook,
                              void *user);

const char *pbbctw89_error_string(int result);

#ifdef __cplusplus
}
#endif

#endif
