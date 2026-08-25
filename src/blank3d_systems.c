#include "blank3d_systems.h"
#include "blank3d_weapon_ini.h"
#include "blank3d_weapon_loadout.h"
#include "blank3d_weapon_host_io.h"

#include <string.h>
#include <stdio.h>

#include "../vendor/gamlib3d/math_helpers/gamlib3d_math.h"

#define B3D_AMMO_ITEM_BASE 100
#define B3D_NUM_OWNER B3D_PLAYER_ACTOR_ID
#define B3D_PLAYER_HEALTH_MAX 100
#define B3D_PBB_RULE_HEALTH_BELOW_MAX 1
#define B3D_PBB_EFFECT_HEAL_FROM_ITEM_AMOUNT 1

static int b3d_clamp_int(int value, int minimum, int maximum)
{
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

static gkinv_u16 b3d_ammo_item_id(int ammo_id)
{
    return (gkinv_u16)(B3D_AMMO_ITEM_BASE + ammo_id);
}

static gkinv_u16 b3d_weapon_item_id(int weapon_id)
{
    return (gkinv_u16)weapon_id;
}

static void b3d_set_status(Blank3DSystems *systems, const char *text)
{
    if (!systems) return;
    if (!text) text = "";
    strncpy(systems->status, text, sizeof(systems->status) - 1U);
    systems->status[sizeof(systems->status) - 1U] = '\0';
}

static int b3d_item_grant_weapon_provider(void *user,
                                          int host_actor_id,
                                          int weapon_id,
                                          int amount,
                                          int auto_equip)
{
    return blank3d_systems_grant_weapon((Blank3DSystems *)user,
                                        host_actor_id, weapon_id, amount,
                                        auto_equip);
}

static int b3d_item_grant_ammo_provider(void *user,
                                        int host_actor_id,
                                        int ammo_id,
                                        int amount)
{
    return blank3d_systems_grant_ammo((Blank3DSystems *)user,
                                      host_actor_id, ammo_id, amount);
}

static int b3d_item_custom_rule(PBB_ItemWorld *world,
                                int actor_id,
                                int item_id,
                                const PBB_ItemRule *rule)
{
    Blank3DSystems *systems;
    int maximum;
    (void)item_id;
    if (!world || !rule) return 0;
    systems = (Blank3DSystems *)pbb_item_world_get_user_data(world);
    if (!systems || actor_id != systems->player_item_actor_id) return 0;
    if (rule->a == B3D_PBB_RULE_HEALTH_BELOW_MAX) {
        maximum = rule->b > 0 ? rule->b : B3D_PLAYER_HEALTH_MAX;
        return blank3d_systems_player_health(systems) < maximum;
    }
    return 0;
}

static int b3d_item_custom_effect(PBB_ItemWorld *world,
                                  int actor_id,
                                  int item_id,
                                  const PBB_ItemEffect *effect)
{
    Blank3DSystems *systems;
    const PBB_Item *item;
    int current;
    int maximum;
    int amount;
    if (!world || !effect) return 0;
    systems = (Blank3DSystems *)pbb_item_world_get_user_data(world);
    if (!systems || actor_id != systems->player_item_actor_id) return 0;
    if (effect->a != B3D_PBB_EFFECT_HEAL_FROM_ITEM_AMOUNT) return 0;
    item = pbb_item_get_const(world, item_id);
    if (!item) return 0;
    maximum = effect->b > 0 ? effect->b : B3D_PLAYER_HEALTH_MAX;
    current = blank3d_systems_player_health(systems);
    if (current >= maximum) return 0;
    amount = item->amount;
    if (amount <= 0) return 0;
    if (amount > maximum - current) amount = maximum - current;
    blank3d_systems_heal_player(systems, amount);
    return 1;
}

static void b3d_init_item_contact(Blank3DSystems *systems)
{
    PBBCTW89_WeaponProvider provider;
    int actor_id;
    if (!systems) return;

    ct89_init(&systems->contact_triggers);
    pbb_item_world_init(&systems->item_world);
    pbb_item_world_set_callbacks(&systems->item_world,
                                 b3d_item_custom_rule,
                                 b3d_item_custom_effect,
                                 systems);
    pbbctw89_init(&systems->item_contact_bridge);
    (void)pbbctw89_attach(&systems->item_contact_bridge,
                          &systems->contact_triggers,
                          &systems->item_world);

    pbbctw89_weapon_provider_init(&provider);
    provider.user = systems;
    provider.grant_weapon = b3d_item_grant_weapon_provider;
    provider.grant_ammo = b3d_item_grant_ammo_provider;
    pbbctw89_set_weapon_provider(&systems->item_contact_bridge, &provider);

    actor_id = pbb_item_actor_create(&systems->item_world,
                                     PBB_ACTOR_CLASS_PLAYER,
                                     1UL,
                                     0L, 0L,
                                     PBB_FIXED_HALF,
                                     PBB_FIXED_HALF);
    systems->player_item_actor_id = actor_id;
    if (actor_id != PBB_ITEM_INVALID_ID) {
        (void)pbb_item_actor_set_masks(&systems->item_world, actor_id,
                                       PBB_ITEM_CATEGORY_ANY,
                                       PBB_ITEM_CATEGORY_ANY);
        (void)pbbctw89_bind_actor(&systems->item_contact_bridge,
                                  (CT89_Subject)B3D_PLAYER_ACTOR_ID,
                                  actor_id, B3D_PLAYER_ACTOR_ID);
    }
}

void blank3d_systems_reset_item_contact(Blank3DSystems *systems)
{
    b3d_init_item_contact(systems);
}

static const char *b3d_flag_key(int key)
{
    switch (key) {
    case GWP89_FLAG_WEAPON_ENABLED: return "weapon.enabled";
    case GWP89_FLAG_CAN_FIRE: return "weapon.can_fire";
    case GWP89_FLAG_CAN_RELOAD: return "weapon.can_reload";
    case GWP89_FLAG_ALLOW_DRY_FIRE: return "weapon.allow_dry_fire";
    case GWP89_FLAG_USE_INTERNAL_CLIP: return "weapon.use_internal_clip";
    case GWP89_FLAG_ACTIVE_RELOAD_ENABLED: return "weapon.active_reload";
    case GWP89_FLAG_EMIT_PROJECTILE: return "weapon.emit_projectile";
    case GWP89_FLAG_EMIT_MUZZLE: return "weapon.emit_muzzle";
    case GWP89_FLAG_EMIT_CASING: return "weapon.emit_casing";
    case GWP89_FLAG_EMIT_TRAIL: return "weapon.emit_trail";
    case GWP89_FLAG_EMIT_VISUALS: return "weapon.emit_visuals";
    case GWP89_FLAG_APPLY_RECOIL: return "weapon.apply_recoil";
    default: return 0;
    }
}

static ns_id b3d_numeric_value_id(const Blank3DSystems *systems, int key)
{
    if (!systems) return NS_INVALID_ID;
    switch (key) {
    case GWP89_NUM_DAMAGE_FX: return systems->damage_multiplier_value;
    case GWP89_NUM_SPEED_FX: return systems->speed_multiplier_value;
    case GWP89_NUM_RECOIL_FX: return systems->recoil_multiplier_value;
    default: return NS_INVALID_ID;
    }
}

static gwp89_fx b3d_apply_q10_multiplier(gwp89_fx base, ns_fx multiplier)
{
    long whole;
    long remainder;
    long result;
    if (multiplier < 0L) multiplier = 0L;
    whole = multiplier / NS_FX_ONE;
    remainder = multiplier % NS_FX_ONE;
    result = base * whole;
    result += (base * remainder) / NS_FX_ONE;
    return (gwp89_fx)result;
}

static GWP89_Vec3 b3d_gwp_from_gamlib(Vec3 value)
{
    GWP89_Vec3 result;
    result.x = (gwp89_fx)value.x;
    result.y = (gwp89_fx)value.y;
    result.z = (gwp89_fx)value.z;
    return result;
}

static Vec3 b3d_gamlib_from_gwp(GWP89_Vec3 value)
{
    return gamlib_vec3((g3d_fix)value.x,
                       (g3d_fix)value.y,
                       (g3d_fix)value.z);
}

static int b3d_math_provider(void *context, GWP89_ProviderPacket *packet)
{
    Vec3 a;
    Vec3 b;
    Vec3 result;
    (void)context;
    if (!packet || packet->phase != GWP89_PHASE_PRE)
        return GWP89_PROVIDER_PASS;
    a = b3d_gamlib_from_gwp(packet->vec_a);
    if (packet->operation == GWP89_OP_MATH_ADD) {
        b = b3d_gamlib_from_gwp(packet->vec_b);
        gamlib_vec3_add(&result, &a, &b);
        packet->vec_c = b3d_gwp_from_gamlib(result);
        return GWP89_PROVIDER_HANDLED;
    }
    if (packet->operation == GWP89_OP_MATH_SCALE) {
        gamlib_vec3_scale(&result, &a, (g3d_fix)packet->fx_value);
        packet->vec_c = b3d_gwp_from_gamlib(result);
        return GWP89_PROVIDER_HANDLED;
    }
    if (packet->operation == GWP89_OP_MATH_NORMALIZE) {
        if (gamlib_vec3_length(&a) <= G3D_FIX_EPSILON)
            result = gamlib_vec3(0, 0, 0);
        else
            gamlib_vec3_normalize(&result, &a);
        packet->vec_c = b3d_gwp_from_gamlib(result);
        return GWP89_PROVIDER_HANDLED;
    }
    return GWP89_PROVIDER_PASS;
}

static int b3d_numeric_provider(void *context, GWP89_ProviderPacket *packet)
{
    Blank3DSystems *systems;
    ns_id value_id;
    ns_fx multiplier;
    systems = (Blank3DSystems *)context;
    if (!systems || !packet) return GWP89_PROVIDER_PASS;
    if (packet->phase != GWP89_PHASE_PRE) return GWP89_PROVIDER_PASS;
    if (packet->operation != GWP89_OP_QUERY_FX) return GWP89_PROVIDER_PASS;
    value_id = b3d_numeric_value_id(systems, packet->key);
    if (value_id == NS_INVALID_ID) return GWP89_PROVIDER_PASS;
    if (ns_get_by_id(&systems->numbers, value_id, &multiplier) != NS_OK)
        return GWP89_PROVIDER_PASS;
    if (packet->actor_id == B3D_PLAYER_ACTOR_ID &&
        packet->key == GWP89_NUM_SPEED_FX &&
        systems->launch_speed_override_q16 > 0L) {
        /* Manager fixed point is Q20.12; charge providers publish Q16.16. */
        packet->fx_value = (gwp89_fx)(systems->launch_speed_override_q16 / 16L);
    }
    packet->fx_value = b3d_apply_q10_multiplier(packet->fx_value, multiplier);
    return GWP89_PROVIDER_MODIFIED;
}

static int b3d_player_alive(const Blank3DSystems *systems)
{
    ns_fx health;
    if (!systems) return 0;
    if (ns_get_by_id(&systems->numbers, systems->player_health_value,
                     &health) != NS_OK)
        return 0;
    return health > NS_FX_ZERO;
}

static const char *b3d_actor_flag_key(int actor_id, int key)
{
    if (actor_id == B3D_PLAYER_ACTOR_ID) return b3d_flag_key(key);
    switch (key) {
    case GWP89_FLAG_CAN_FIRE: return "npc.weapon.can_fire";
    case GWP89_FLAG_CAN_RELOAD: return "npc.weapon.can_reload";
    case GWP89_FLAG_ACTIVE_RELOAD_ENABLED:
        return "npc.weapon.active_reload";
    default: return b3d_flag_key(key);
    }
}

static int b3d_flags_provider(void *context, GWP89_ProviderPacket *packet)
{
    Blank3DSystems *systems;
    const char *key;
    FlagsValue value;
    systems = (Blank3DSystems *)context;
    if (!systems || !packet) return GWP89_PROVIDER_PASS;
    if (packet->phase != GWP89_PHASE_PRE) return GWP89_PROVIDER_PASS;

    if (packet->operation == GWP89_OP_QUERY_FLAG) {
        key = b3d_actor_flag_key(packet->actor_id, packet->key);
        if (!key) return GWP89_PROVIDER_PASS;
        if (!flagstore_get(&systems->flags, key, &value))
            return GWP89_PROVIDER_PASS;
        if (value.type == FLAGS_VAL_BOOL || value.type == FLAGS_VAL_INT) {
            packet->i_value = value.as.i ? 1 : 0;
            /* Health is an actor-local fire/reload gate. It must never be
               written into the shared FlagStore, because NPC managers reuse
               this provider context and would inherit the player's death. */
            if (packet->actor_id == B3D_PLAYER_ACTOR_ID &&
                (packet->key == GWP89_FLAG_CAN_FIRE ||
                 packet->key == GWP89_FLAG_CAN_RELOAD) &&
                !b3d_player_alive(systems))
                packet->i_value = 0;
            if (packet->i_value && packet->profile) {
                if (packet->key == GWP89_FLAG_EMIT_CASING)
                    packet->i_value =
                        blank3d_weapon_modules_has_casing(packet->weapon_id);
                else if (packet->key == GWP89_FLAG_EMIT_MUZZLE)
                    packet->i_value =
                        blank3d_weapon_modules_has_muzzle(packet->weapon_id);
                else if (packet->key == GWP89_FLAG_EMIT_TRAIL)
                    packet->i_value =
                        blank3d_weapon_modules_has_trail(packet->weapon_id);
            }
            return GWP89_PROVIDER_HANDLED;
        }
    }
    if (packet->operation == GWP89_OP_FIRE_VALIDATE) {
        key = packet->actor_id == B3D_PLAYER_ACTOR_ID
            ? "weapon.can_fire" : "npc.weapon.can_fire";
        if (!blank3d_systems_get_flag(systems, key, 1))
            return GWP89_PROVIDER_CANCEL;
        if (packet->actor_id == B3D_PLAYER_ACTOR_ID &&
            !b3d_player_alive(systems))
            return GWP89_PROVIDER_CANCEL;
    }
    return GWP89_PROVIDER_PASS;
}

static int b3d_inventory_set_count(Blank3DSystems *systems, gkinv_u16 item_id, int amount)
{
    int current;
    int result;
    if (!systems) return 0;
    amount = b3d_clamp_int(amount, 0, 65535);
    current = (int)gkinv_count_item(&systems->inventory, item_id);
    if (current > 0) {
        result = gkinv_remove_item(&systems->item_db, &systems->inventory,
                                   item_id, (gkinv_u16)current);
        if (result != GKINV_OK) return 0;
    }
    if (amount > 0) {
        result = gkinv_add_item(&systems->item_db, &systems->inventory,
                                item_id, (gkinv_u16)amount, 0);
        if (result != GKINV_OK) return 0;
    }
    return 1;
}

static int b3d_inventory_provider(void *context, GWP89_ProviderPacket *packet)
{
    Blank3DSystems *systems;
    gkinv_u16 item_id;
    int weapon_id;
    int current;
    int result;
    systems = (Blank3DSystems *)context;
    if (!systems || !packet) return GWP89_PROVIDER_PASS;
    if (packet->phase != GWP89_PHASE_PRE) return GWP89_PROVIDER_PASS;
    /* Player inventory is external GKInventory. NPC actors deliberately use
       the weapon manager's internal clip/ammo bank. */
    if (packet->actor_id != B3D_PLAYER_ACTOR_ID)
        return GWP89_PROVIDER_PASS;

    if (packet->operation == GWP89_OP_EQUIP) {
        item_id = b3d_weapon_item_id(packet->weapon_id);
        if (!gkinv_has_item(&systems->inventory, item_id, 1U)) {
            packet->result_code = GWP89_NOT_FOUND;
            return GWP89_PROVIDER_CANCEL;
        }
        return GWP89_PROVIDER_PASS;
    }

    if (packet->operation == GWP89_OP_CLIP_QUERY ||
        packet->operation == GWP89_OP_CLIP_SET) {
        weapon_id = packet->weapon_id;
        if (weapon_id < 0 || weapon_id >= B3D_WEAPON_CLIP_CAPACITY) {
            packet->result_code = GWP89_BAD_ARG;
            return GWP89_PROVIDER_HANDLED;
        }
        if (packet->operation == GWP89_OP_CLIP_QUERY) {
            packet->i_value = systems->weapon_clips[weapon_id];
        } else {
            systems->weapon_clips[weapon_id] = b3d_clamp_int(packet->i_value, 0, 65535);
            packet->i_value = systems->weapon_clips[weapon_id];
        }
        packet->result_code = GWP89_OK;
        return GWP89_PROVIDER_HANDLED;
    }

    item_id = b3d_ammo_item_id(packet->ammo_id);
    if (packet->operation == GWP89_OP_AMMO_QUERY) {
        packet->i_value = (int)gkinv_count_item(&systems->inventory, item_id);
        return GWP89_PROVIDER_HANDLED;
    }
    if (packet->operation == GWP89_OP_AMMO_CONSUME) {
        current = (int)gkinv_count_item(&systems->inventory, item_id);
        if (current < packet->amount) {
            packet->result_code = GWP89_NO_AMMO;
            return GWP89_PROVIDER_HANDLED;
        }
        result = gkinv_remove_item(&systems->item_db, &systems->inventory,
                                   item_id, (gkinv_u16)packet->amount);
        packet->result_code = result == GKINV_OK ? GWP89_OK : GWP89_PROVIDER_ERROR;
        return GWP89_PROVIDER_HANDLED;
    }
    if (packet->operation == GWP89_OP_AMMO_SET) {
        packet->result_code = b3d_inventory_set_count(systems, item_id, packet->i_value)
                            ? GWP89_OK : GWP89_PROVIDER_ERROR;
        return GWP89_PROVIDER_HANDLED;
    }
    if (packet->operation == GWP89_OP_AMMO_ADD) {
        result = gkinv_add_item(&systems->item_db, &systems->inventory,
                                item_id, (gkinv_u16)b3d_clamp_int(packet->amount, 0, 65535), 0);
        packet->result_code = result == GKINV_OK ? GWP89_OK : GWP89_PROVIDER_ERROR;
        return GWP89_PROVIDER_HANDLED;
    }
    return GWP89_PROVIDER_PASS;
}

static void b3d_define_item(gkinv_ItemDef *def,
                            int id,
                            const char *name,
                            int max_stack,
                            gkinv_u32 flags,
                            gkinv_u32 category)
{
    if (!def) return;
    memset(def, 0, sizeof(*def));
    def->id = (gkinv_u16)id;
    def->name = name;
    def->max_stack = (gkinv_u16)max_stack;
    def->grid_w = 1U;
    def->grid_h = 1U;
    def->flags = flags;
    def->category_mask = category;
    def->weight = 0;
}

static void b3d_init_inventory(Blank3DSystems *systems)
{
    int i;
    int item_count;
    int weapon_amount;
    int ammo_amount;
    int ammo_seen[B3D_WLOAD_MAX_WEAPONS + 1];
    char status[160];
    const Blank3DWeaponCatalogEntry *entry;
    if (!systems) return;

    memset(ammo_seen, 0, sizeof(ammo_seen));
    (void)gweaponloadout89_catalog_load(&systems->weapons,
        &systems->weapon_catalog, "config/weapons/weapons.ini",
        status, sizeof(status));
    (void)gweaponloadout89_load(&systems->weapons,
        &systems->player_weapon_loadout, "config/weapons/player_weapons.ini",
        status, sizeof(status));
    systems->starting_weapon_id = systems->player_weapon_loadout.equipped_weapon_id;

    item_count = 0;
    for (i = 0; i < B3D_WLOAD_MAX_WEAPONS && item_count < B3D_ITEM_COUNT; ++i) {
        entry = &systems->weapon_catalog.entries[i];
        if (!entry->used) continue;
        b3d_define_item(&systems->item_defs[item_count++], entry->weapon_id,
                        entry->name, entry->max_owned > 0 ? entry->max_owned : 1,
                        GKINV_ITEM_EQUIPPABLE | GKINV_ITEM_UNIQUE,
                        GKINV_CAT_WEAPON);
    }
    for (i = 0; i < B3D_WLOAD_MAX_WEAPONS && item_count < B3D_ITEM_COUNT; ++i) {
        entry = &systems->weapon_catalog.entries[i];
        if (!entry->used || entry->ammo_id <= 0 ||
            entry->ammo_id > B3D_WLOAD_MAX_WEAPONS || ammo_seen[entry->ammo_id])
            continue;
        ammo_seen[entry->ammo_id] = 1;
        b3d_define_item(&systems->item_defs[item_count++],
                        B3D_AMMO_ITEM_BASE + entry->ammo_id,
                        entry->ammo_name,
                        entry->ammo_capacity > 0 ? entry->ammo_capacity : 999,
                        GKINV_ITEM_STACKABLE | GKINV_ITEM_CONSUMABLE,
                        GKINV_CAT_AMMO);
    }
    systems->item_db.items = systems->item_defs;
    systems->item_db.count = (gkinv_u16)item_count;
    gkinv_container_init(&systems->inventory, systems->inventory_slots,
                         B3D_INV_SLOT_CAPACITY,
                         GKINV_CONT_STACKS | GKINV_CONT_ALLOW_PARTIAL);
    gkinv_container_set_limits(&systems->inventory,
                               GKINV_CAT_WEAPON | GKINV_CAT_AMMO, 0);

    for (i = 1; i <= B3D_WLOAD_MAX_WEAPONS; ++i) {
        entry = blank3d_weapon_catalog_find_id(&systems->weapon_catalog, i);
        if (!entry) continue;
        weapon_amount = systems->player_weapon_loadout.weapon_amount[i];
        if (weapon_amount > entry->max_owned) weapon_amount = entry->max_owned;
        if (weapon_amount > 0)
            (void)gkinv_add_item(&systems->item_db, &systems->inventory,
                                 (gkinv_u16)i, (gkinv_u16)weapon_amount, 0);
    }
    for (i = 1; i <= B3D_WLOAD_MAX_WEAPONS; ++i) {
        ammo_amount = systems->player_weapon_loadout.ammo_amount[i];
        if (ammo_amount <= 0) continue;
        (void)gkinv_add_item(&systems->item_db, &systems->inventory,
                             (gkinv_u16)(B3D_AMMO_ITEM_BASE + i),
                             (gkinv_u16)ammo_amount, 0);
    }
}

static void b3d_init_numbers(Blank3DSystems *systems)
{
    ns_id type_id;
    if (!systems) return;
    ns_init(&systems->numbers);
    type_id = ns_define_type(&systems->numbers, "player.health",
                             NS_SCOPE_INSTANCE, NS_KIND_INT,
                             NS_FX_FROM_INT(100), NS_FX_ZERO, NS_FX_FROM_INT(100),
                             NS_OVERFLOW_CLAMP, NS_FLAG_SAVE | NS_FLAG_HUD);
    systems->player_health_value = ns_attach_type(&systems->numbers,
                                                   B3D_NUM_OWNER, type_id);
    type_id = ns_define_type(&systems->numbers, "weapon.damage_multiplier",
                             NS_SCOPE_INSTANCE, NS_KIND_FIXED,
                             NS_FX_ONE, NS_FX_ZERO, NS_FX_FROM_INT(8),
                             NS_OVERFLOW_CLAMP, NS_FLAG_SAVE);
    systems->damage_multiplier_value = ns_attach_type(&systems->numbers,
                                                       B3D_NUM_OWNER, type_id);
    type_id = ns_define_type(&systems->numbers, "weapon.speed_multiplier",
                             NS_SCOPE_INSTANCE, NS_KIND_FIXED,
                             NS_FX_ONE, NS_FX_ZERO, NS_FX_FROM_INT(8),
                             NS_OVERFLOW_CLAMP, NS_FLAG_SAVE);
    systems->speed_multiplier_value = ns_attach_type(&systems->numbers,
                                                      B3D_NUM_OWNER, type_id);
    type_id = ns_define_type(&systems->numbers, "weapon.recoil_multiplier",
                             NS_SCOPE_INSTANCE, NS_KIND_FIXED,
                             NS_FX_ONE, NS_FX_ZERO, NS_FX_FROM_INT(8),
                             NS_OVERFLOW_CLAMP, NS_FLAG_SAVE);
    systems->recoil_multiplier_value = ns_attach_type(&systems->numbers,
                                                       B3D_NUM_OWNER, type_id);
}

static void b3d_init_flags(Blank3DSystems *systems)
{
    if (!systems) return;
    flagstore_init(&systems->flags, systems->flag_entries, B3D_FLAG_CAPACITY,
                   systems->flag_pool, B3D_FLAG_POOL_CAPACITY);
    flagstore_set_bool(&systems->flags, "weapon.enabled", 1);
    flagstore_set_bool(&systems->flags, "weapon.can_fire", 1);
    flagstore_set_bool(&systems->flags, "weapon.can_reload", 1);
    flagstore_set_bool(&systems->flags, "weapon.allow_dry_fire", 1);
    flagstore_set_bool(&systems->flags, "weapon.use_internal_clip", 1);
    flagstore_set_bool(&systems->flags, "weapon.active_reload", 1);
    flagstore_set_bool(&systems->flags, "weapon.emit_projectile", 1);
    flagstore_set_bool(&systems->flags, "weapon.emit_muzzle", 1);
    flagstore_set_bool(&systems->flags, "weapon.emit_casing", 1);
    flagstore_set_bool(&systems->flags, "weapon.emit_trail", 1);
    flagstore_set_bool(&systems->flags, "weapon.emit_visuals", 1);
    flagstore_set_bool(&systems->flags, "weapon.apply_recoil", 1);
    /* Actor-local NPC gates. These deliberately do not alias the player's
       can_fire/can_reload flags. Normal NPC reload does not use active reload. */
    flagstore_set_bool(&systems->flags, "npc.weapon.can_fire", 1);
    flagstore_set_bool(&systems->flags, "npc.weapon.can_reload", 1);
    flagstore_set_bool(&systems->flags, "npc.weapon.active_reload", 0);
}


static void b3d_profile_identity(GWP89_WeaponProfile *profile,
                                 const char *name,
                                 int weapon_id,
                                 int ammo_id,
                                 int projectile_id,
                                 int mesh_id)
{
    if (!profile) return;
    gwp89_profile_defaults(profile);
    gwp89_copy_id(profile->name, GWP89_NAME_MAX, name);
    gwp89_copy_id(profile->gun_name, GWP89_NAME_MAX, name);
    gwp89_copy_id(profile->ammo_name, GWP89_NAME_MAX, name);
    gwp89_copy_id(profile->projectile_name, GWP89_NAME_MAX, name);
    gwp89_copy_id(profile->shell_name, GWP89_NAME_MAX, name);
    gwp89_copy_id(profile->muzzle_name, GWP89_NAME_MAX, name);
    gwp89_copy_id(profile->casing_name, GWP89_NAME_MAX, name);
    gwp89_copy_id(profile->trail_name, GWP89_NAME_MAX, name);
    gwp89_copy_id(profile->projectile_mesh_name, GWP89_NAME_MAX, name);
    gwp89_copy_id(profile->shell_mesh_name, GWP89_NAME_MAX, name);
    profile->weapon_id = weapon_id;
    profile->gun_id = weapon_id;
    profile->ammo_id = ammo_id;
    profile->projectile_id = projectile_id;
    profile->shell_id = weapon_id;
    profile->muzzle_id = weapon_id;
    profile->casing_id = weapon_id;
    profile->trail_id = weapon_id;
    profile->projectile_mesh_id = mesh_id;
    profile->shell_mesh_id = weapon_id;
}

static void b3d_add_heavy_profiles(Blank3DSystems *systems)
{
    GWP89_WeaponProfile profile;
    if (!systems) return;

    b3d_profile_identity(&profile, "grenade_launcher", 6, 5, 6, 6);
    gwp89_copy_id(profile.projectile_mesh_name, GWP89_NAME_MAX,
                  "grenade40_lv");
    gwp89_copy_id(profile.projectile_name, GWP89_NAME_MAX,
                  "grenade40_lv");
    gwp89_copy_id(profile.shell_mesh_name, GWP89_NAME_MAX,
                  "shell_40mm_spent_case");
    profile.fire_mode = GWP89_FIRE_HOLD_ONCE;
    profile.clip_size = 6;
    profile.ammo_per_shot = 1;
    profile.cooldown_ms = 720U;
    profile.reload_ms = 1450U;
    profile.projectile_life_ms = 5200U;
    profile.damage_fx = gwp89_fx_from_text("82.0");
    profile.speed_fx = gwp89_fx_from_text("22.0");
    profile.range_fx = gwp89_fx_from_text("105.0");
    profile.pellet_count = 1;
    profile.spread_fx = gwp89_fx_from_text("0.4");
    profile.projectile_radius_fx = gwp89_fx_from_text("0.24");
    profile.projectile_mesh_scale_fx = gwp89_fx_from_text("0.72");
    profile.shell_mesh_scale_fx = gwp89_fx_from_text("0.45");
    profile.recoil_fx = gwp89_fx_from_text("3.4");
    (void)gwp89_add_weapon(&systems->weapons, &profile);

    b3d_profile_identity(&profile, "rocket_launcher", 7, 6, 7, 7);
    gwp89_copy_id(profile.projectile_mesh_name, GWP89_NAME_MAX,
                  "survival_rpg7_cone");
    gwp89_copy_id(profile.projectile_name, GWP89_NAME_MAX,
                  "survival_rpg7_cone");
    gwp89_copy_id(profile.shell_mesh_name, GWP89_NAME_MAX,
                  "none");
    profile.fire_mode = GWP89_FIRE_HOLD_ONCE;
    profile.clip_size = 1;
    profile.ammo_per_shot = 1;
    profile.cooldown_ms = 1350U;
    profile.reload_ms = 1900U;
    profile.projectile_life_ms = 7500U;
    profile.damage_fx = gwp89_fx_from_text("180.0");
    profile.speed_fx = gwp89_fx_from_text("18.0");
    profile.range_fx = gwp89_fx_from_text("145.0");
    profile.pellet_count = 1;
    profile.spread_fx = 0;
    profile.projectile_radius_fx = gwp89_fx_from_text("0.38");
    profile.projectile_mesh_scale_fx = gwp89_fx_from_text("1.05");
    profile.shell_mesh_scale_fx = gwp89_fx_from_text("0.60");
    profile.recoil_fx = gwp89_fx_from_text("5.2");
    (void)gwp89_add_weapon(&systems->weapons, &profile);

    b3d_profile_identity(&profile, "gatling_gun", 8, 7, 2, 2);
    gwp89_copy_id(profile.projectile_mesh_name, GWP89_NAME_MAX,
                  "machinegun_projectile");
    gwp89_copy_id(profile.projectile_name, GWP89_NAME_MAX,
                  "machinegun_projectile");
    gwp89_copy_id(profile.shell_mesh_name, GWP89_NAME_MAX,
                  "machinegun_shell");
    profile.shell_id = 2;
    profile.shell_mesh_id = 2;
    gwp89_copy_id(profile.trail_name, GWP89_NAME_MAX,
                  "gatling_tracer");
    profile.fire_mode = GWP89_FIRE_AUTO;
    profile.clip_size = 300;
    profile.ammo_per_shot = 1;
    profile.cooldown_ms = 35U;
    profile.reload_ms = 3200U;
    profile.projectile_life_ms = 2600U;
    profile.damage_fx = gwp89_fx_from_text("6.5");
    profile.speed_fx = gwp89_fx_from_text("62.0");
    profile.range_fx = gwp89_fx_from_text("125.0");
    profile.pellet_count = 1;
    profile.spread_fx = gwp89_fx_from_text("1.15");
    profile.projectile_radius_fx = gwp89_fx_from_text("0.10");
    profile.projectile_mesh_scale_fx = gwp89_fx_from_text("0.44");
    profile.shell_mesh_scale_fx = gwp89_fx_from_text("0.26");
    profile.recoil_fx = gwp89_fx_from_text("0.48");
    (void)gwp89_add_weapon(&systems->weapons, &profile);

    /* Non-firearm proof: charge-release launcher composed from the same core.
       Bolt3D owns flight, gravity, drag and bounce; no casing or muzzle event. */
    b3d_profile_identity(&profile, "slingshot", 9, 8, 9, 9);
    gwp89_copy_id(profile.projectile_mesh_name, GWP89_NAME_MAX,
                  "slingshot_stone_primitive");
    gwp89_copy_id(profile.projectile_name, GWP89_NAME_MAX,
                  "slingshot_stone");
    gwp89_copy_id(profile.shell_mesh_name, GWP89_NAME_MAX, "none");
    gwp89_copy_id(profile.muzzle_name, GWP89_NAME_MAX, "none");
    gwp89_copy_id(profile.casing_name, GWP89_NAME_MAX, "none");
    gwp89_copy_id(profile.trail_name, GWP89_NAME_MAX, "aoi_arc_trail");
    profile.shell_id = 0;
    profile.shell_mesh_id = 0;
    profile.muzzle_id = 0;
    profile.casing_id = 0;
    profile.trail_id = 9;
    profile.fire_mode = GWP89_FIRE_SEMI;
    profile.clip_size = 1;
    profile.ammo_per_shot = 1;
    profile.cooldown_ms = 180U;
    profile.reload_ms = 360U;
    profile.projectile_life_ms = 8000U;
    profile.damage_fx = gwp89_fx_from_text("16.0");
    profile.speed_fx = gwp89_fx_from_text("46.0");
    profile.range_fx = gwp89_fx_from_text("160.0");
    profile.pellet_count = 1;
    profile.spread_fx = gwp89_fx_from_text("0.15");
    profile.projectile_radius_fx = gwp89_fx_from_text("0.22");
    profile.projectile_mesh_scale_fx = gwp89_fx_from_text("0.62");
    profile.shell_mesh_scale_fx = 0;
    profile.recoil_fx = gwp89_fx_from_text("0.08");
    (void)gwp89_add_weapon(&systems->weapons, &profile);
}

void blank3d_systems_init_from_ini(Blank3DSystems *systems,
                                      const char *weapon_manifest_path)
{
    int i;
    int loaded;
    const GWP89_WeaponProfile *profile;
    char weapon_status[160];
    if (!systems) return;
    memset(systems, 0, sizeof(*systems));
    b3d_init_numbers(systems);
    b3d_init_flags(systems);

    gwp89_init(&systems->weapons);
    (void)blank3d_weapon_host_io_bind(&systems->weapons);
    /* Loadout is Weapon-System policy; filesystem access remains a host hook. */
    b3d_init_inventory(systems);
    b3d_init_item_contact(systems);

    gwp89_add_provider(&systems->weapons, GWP89_SERVICE_MATH3D, 120,
                       "blank3d.gamlib3d-math", systems, b3d_math_provider);
    gwp89_add_provider(&systems->weapons, GWP89_SERVICE_NUMERIC, 100,
                       "blank3d.numsys", systems, b3d_numeric_provider);
    gwp89_add_provider(&systems->weapons, GWP89_SERVICE_FLAGS, 100,
                       "blank3d.flags", systems, b3d_flags_provider);
    gwp89_add_provider(&systems->weapons, GWP89_SERVICE_INVENTORY, 100,
                       "blank3d.gkinventory", systems, b3d_inventory_provider);

    weapon_status[0] = '\0';
    loaded = blank3d_weapon_ini_load_manifest(
        &systems->weapons,
        weapon_manifest_path ? weapon_manifest_path
                             : "config/weapons/weapons.ini",
        weapon_status, sizeof(weapon_status));
    if (loaded <= 0) {
        blank3d_weapon_modules_load_defaults();
        (void)gwp89_load_default_profiles(&systems->weapons);
        b3d_add_heavy_profiles(systems);
    }

    for (i = 0; i < systems->weapons.weapon_count; ++i) {
        profile = gwp89_get_weapon(&systems->weapons, i);
        if (profile && profile->weapon_id >= 0 &&
            profile->weapon_id < B3D_WEAPON_CLIP_CAPACITY) {
            systems->weapon_clips[profile->weapon_id] = profile->clip_size;
        }
    }
    gwp89_bind_actor(&systems->weapons, B3D_PLAYER_ACTOR_ID,
                     B3D_PLAYER_KIND, B3D_PLAYER_TEAM);
    if (blank3d_systems_equip_id(systems, systems->starting_weapon_id) != GWP89_OK)
        (void)gwp89_equip_name(&systems->weapons, B3D_PLAYER_ACTOR_ID, "pistol", 0);
    systems->current_weapon_id = blank3d_systems_weapon_id(systems);
    systems->last_trigger_down = 0;
    if (loaded > 0)
        b3d_set_status(systems, weapon_status);
    else
        b3d_set_status(systems,
            "weapon INI fallback: compiled profiles + provider stack");
}

void blank3d_systems_init(Blank3DSystems *systems)
{
    blank3d_systems_init_from_ini(systems, "config/weapons/weapons.ini");
}

void blank3d_systems_reset_combat(Blank3DSystems *systems)
{
    if (!systems) return;
    ns_set_by_id(&systems->numbers, systems->player_health_value,
                 NS_FX_FROM_INT(100));
    systems->last_trigger_down = 0;
    gwp89_clear_events(&systems->weapons);
}

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
                            const GWP89_Vec3 *camera_forward)
{
    GWP89_FireInput input;
    int result;
    if (!systems) return;

    /* Contact Trigger is part of the systems frame now.  With no spatial
       provider installed CT89 is a cheap no-op; once the host binds one,
       touch/proximity pickups begin working without a second update loop. */
    (void)ct89_step(&systems->contact_triggers, (unsigned long)dt_ms);

    memset(&input, 0, sizeof(input));
    input.actor_id = B3D_PLAYER_ACTOR_ID;
    input.actor_kind = B3D_PLAYER_KIND;
    input.team_id = B3D_PLAYER_TEAM;
    input.view_style = view_style;
    input.dt_ms = dt_ms;
    input.zoom_fx = zoom_fx > 0 ? zoom_fx : GWP89_FIX_ONE;
    if (trigger_down) input.trigger_flags |= GWP89_TRIGGER_DOWN;
    if (trigger_pressed) input.trigger_flags |= GWP89_TRIGGER_PRESSED;
    if (trigger_released) input.trigger_flags |= GWP89_TRIGGER_RELEASED;
    if (socket_origin) input.socket_origin = *socket_origin;
    if (socket_forward) input.socket_forward = *socket_forward;
    if (socket_right) input.socket_right = *socket_right;
    if (socket_up) input.socket_up = *socket_up;
    if (camera_origin) input.camera_origin = *camera_origin;
    if (camera_forward) input.camera_forward = *camera_forward;

    if (trigger_down || trigger_pressed) result = gwp89_try_fire(&systems->weapons, &input);
    else result = gwp89_update_actor(&systems->weapons, &input);
    if (result == GWP89_OK || result == GWP89_COOLDOWN ||
        result == GWP89_TRIGGER_LOCKED || result == GWP89_NO_AMMO) {
        b3d_set_status(systems, gwp89_status(&systems->weapons));
    }
    systems->last_trigger_down = trigger_down ? 1 : 0;
    systems->current_weapon_id = blank3d_systems_weapon_id(systems);
    /* Charge speed is a one-shot provider override. */
    systems->launch_speed_override_q16 = 0L;
}

int blank3d_systems_reload(Blank3DSystems *systems)
{
    if (!systems) return GWP89_BAD_ARG;
    if (!blank3d_systems_get_flag(systems, "weapon.can_reload", 1))
        return GWP89_CANCELLED;
    return gwp89_begin_reload(&systems->weapons, B3D_PLAYER_ACTOR_ID);
}

int blank3d_systems_active_reload(Blank3DSystems *systems)
{
    if (!systems) return GWP89_BAD_ARG;
    return gwp89_active_reload_press(&systems->weapons, B3D_PLAYER_ACTOR_ID);
}

static int b3d_player_owns_weapon(const Blank3DSystems *systems, int weapon_id)
{
    if (!systems || weapon_id <= 0) return 0;
    return gkinv_count_item(&systems->inventory,
                            b3d_weapon_item_id(weapon_id)) > 0U;
}

static int b3d_player_weapon_cycle_count(void *context)
{
    Blank3DSystems *systems;
    systems = (Blank3DSystems *)context;
    if (!systems) return 0;
    return systems->weapons.weapon_count;
}

static int b3d_player_weapon_cycle_read(void *context, int index,
                                        cycler89_item *item_out)
{
    Blank3DSystems *systems;
    systems = (Blank3DSystems *)context;
    if (!systems || !item_out || index < 0 ||
        index >= systems->weapons.weapon_count)
        return 0;
    *item_out = (cycler89_item)systems->weapons.weapons[index].weapon_id;
    return 1;
}

static int b3d_player_weapon_cycle_active(void *context, int index,
                                          cycler89_item item)
{
    Blank3DSystems *systems;
    (void)index;
    systems = (Blank3DSystems *)context;
    return b3d_player_owns_weapon(systems, (int)item);
}

static void b3d_player_weapon_cycle_list(Blank3DSystems *systems,
                                         Cycler89List *list)
{
    if (!list) return;
    list->context = systems;
    list->count = b3d_player_weapon_cycle_count;
    list->read = b3d_player_weapon_cycle_read;
    list->is_active = b3d_player_weapon_cycle_active;
}

static int b3d_cycle_weapon(Blank3DSystems *systems, int direction)
{
    Cycler89List list;
    cycler89_item selected;
    int current;
    int result;
    if (!systems) return GWP89_BAD_ARG;
    b3d_player_weapon_cycle_list(systems, &list);
    current = blank3d_systems_weapon_id(systems);
    result = cycler89_step(&list, (cycler89_item)current, current > 0,
                           direction, &selected, 0);
    if (result != CYCLER89_OK) return GWP89_NOT_FOUND;
    return blank3d_systems_equip_id(systems, (int)selected);
}

static int b3d_player_cycle_current(void *context,
                                    cycler89_item *item_out)
{
    Blank3DSystems *systems;
    int current;
    systems = (Blank3DSystems *)context;
    if (!systems || !item_out) return 0;
    current = blank3d_systems_weapon_id(systems);
    if (current <= 0) return 0;
    *item_out = (cycler89_item)current;
    return 1;
}

static int b3d_player_cycle_activate(void *context, cycler89_item item)
{
    Blank3DSystems *systems;
    systems = (Blank3DSystems *)context;
    if (!systems) return 0;
    return blank3d_systems_equip_id(systems, (int)item) == GWP89_OK;
}

int blank3d_systems_cycle_next(Blank3DSystems *systems)
{
    return b3d_cycle_weapon(systems, 1);
}

int blank3d_systems_cycle_prev(Blank3DSystems *systems)
{
    return b3d_cycle_weapon(systems, -1);
}

int blank3d_systems_register_cycle_lists(Blank3DSystems *systems,
                                         Blank3DListCycleRegistry *registry)
{
    Cycler89List list;
    if (!systems || !registry) return 0;
    b3d_player_weapon_cycle_list(systems, &list);
    return blank3d_list_cycle_register(registry, "active_weapon",
                                       &list, systems,
                                       b3d_player_cycle_current,
                                       b3d_player_cycle_activate);
}

int blank3d_systems_equip_id(Blank3DSystems *systems, int weapon_id)
{
    int slot;
    if (!systems) return GWP89_BAD_ARG;
    slot = gwp89_find_weapon_slot_by_id(&systems->weapons, weapon_id);
    if (slot < 0) return slot;
    return gwp89_equip_slot(&systems->weapons, B3D_PLAYER_ACTOR_ID, slot, 0);
}

int blank3d_systems_poll_event(Blank3DSystems *systems, GWP89_Event *event_out)
{
    if (!systems || !event_out) return 0;
    return gwp89_poll_event(&systems->weapons, event_out);
}

int blank3d_systems_player_health(const Blank3DSystems *systems)
{
    ns_fx value;
    if (!systems) return 0;
    if (ns_get_by_id(&systems->numbers, systems->player_health_value, &value) != NS_OK)
        return 0;
    return NS_FX_TO_INT(value);
}

void blank3d_systems_damage_player(Blank3DSystems *systems, int amount)
{
    if (!systems || amount <= 0) return;
    ns_sub_by_id(&systems->numbers, systems->player_health_value,
                 NS_FX_FROM_INT(amount));
}

void blank3d_systems_heal_player(Blank3DSystems *systems, int amount)
{
    if (!systems || amount <= 0) return;
    ns_add_by_id(&systems->numbers, systems->player_health_value,
                 NS_FX_FROM_INT(amount));
}

static const GWP89_UserState *b3d_player_user(const Blank3DSystems *systems)
{
    int slot;
    if (!systems) return 0;
    slot = gwp89_find_user_slot(&systems->weapons, B3D_PLAYER_ACTOR_ID);
    if (slot < 0) return 0;
    return &systems->weapons.users[slot];
}

int blank3d_systems_clip(Blank3DSystems *systems)
{
    const GWP89_UserState *user;
    if (!systems) return 0;
    user = b3d_player_user(systems);
    if (!user) return 0;
    return gwp89_query_clip(&systems->weapons, B3D_PLAYER_ACTOR_ID,
                            user->weapon_id);
}

int blank3d_systems_reserve(Blank3DSystems *systems)
{
    const GWP89_UserState *user;
    const GWP89_WeaponProfile *profile;
    if (!systems) return 0;
    user = b3d_player_user(systems);
    if (!user) return 0;
    profile = gwp89_get_weapon(&systems->weapons, user->weapon_slot);
    if (!profile) return 0;
    return gwp89_query_ammo(&systems->weapons, B3D_PLAYER_ACTOR_ID,
                            profile->ammo_id, profile->weapon_id);
}

int blank3d_systems_weapon_id(const Blank3DSystems *systems)
{
    const GWP89_UserState *user;
    user = b3d_player_user(systems);
    return user ? user->weapon_id : 0;
}

int blank3d_systems_ammo_id(const Blank3DSystems *systems)
{
    const GWP89_UserState *user;
    const GWP89_WeaponProfile *profile;
    user = b3d_player_user(systems);
    if (!user) return 0;
    profile = gwp89_get_weapon(&systems->weapons, user->weapon_slot);
    return profile ? profile->ammo_id : 0;
}

const char *blank3d_systems_weapon_name(const Blank3DSystems *systems)
{
    const GWP89_UserState *user;
    const GWP89_WeaponProfile *profile;
    user = b3d_player_user(systems);
    if (!user) return "none";
    profile = gwp89_get_weapon(&systems->weapons, user->weapon_slot);
    return profile ? profile->name : "none";
}

const char *blank3d_systems_status(const Blank3DSystems *systems)
{
    return systems ? systems->status : "systems unavailable";
}

int blank3d_systems_set_flag(Blank3DSystems *systems, const char *key, int value)
{
    if (!systems || !key) return 0;
    return flagstore_set_bool(&systems->flags, key, value ? 1 : 0);
}

int blank3d_systems_get_flag(const Blank3DSystems *systems, const char *key, int fallback)
{
    FlagsValue value;
    if (!systems || !key) return fallback;
    if (!flagstore_get(&systems->flags, key, &value)) return fallback;
    if (value.type == FLAGS_VAL_BOOL || value.type == FLAGS_VAL_INT)
        return value.as.i ? 1 : 0;
    return fallback;
}

int blank3d_systems_inventory_count(const Blank3DSystems *systems, int item_id)
{
    if (!systems || item_id < 0 || item_id > 65535) return 0;
    return (int)gkinv_count_item(&systems->inventory, (gkinv_u16)item_id);
}

static int b3d_ammo_capacity(const Blank3DSystems *systems, int ammo_id)
{
    int i;
    int capacity;
    const Blank3DWeaponCatalogEntry *entry;
    if (!systems || ammo_id <= 0) return 0;
    capacity = 0;
    for (i = 0; i < B3D_WLOAD_MAX_WEAPONS; ++i) {
        entry = &systems->weapon_catalog.entries[i];
        if (!entry->used || entry->ammo_id != ammo_id) continue;
        if (entry->ammo_capacity > capacity) capacity = entry->ammo_capacity;
    }
    return capacity;
}

int blank3d_systems_grant_weapon(Blank3DSystems *systems, int actor_id,
                                 int weapon_id, int amount, int auto_equip)
{
    const Blank3DWeaponCatalogEntry *entry;
    int current;
    int maximum;
    int result;
    if (!systems || actor_id != B3D_PLAYER_ACTOR_ID ||
        weapon_id <= 0 || amount <= 0)
        return 0;
    entry = blank3d_weapon_catalog_find_id(&systems->weapon_catalog, weapon_id);
    if (!entry) return 0;
    current = (int)gkinv_count_item(&systems->inventory,
                                    b3d_weapon_item_id(weapon_id));
    maximum = entry->max_owned > 0 ? entry->max_owned : 1;
    if (current > maximum || amount > maximum - current) return 0;
    if (amount > 65535) return 0;
    result = gkinv_add_item(&systems->item_db, &systems->inventory,
                            b3d_weapon_item_id(weapon_id),
                            (gkinv_u16)amount, 0);
    if (result != GKINV_OK) return 0;
    if (auto_equip) {
        result = blank3d_systems_equip_id(systems, weapon_id);
        if (result != GWP89_OK) {
            (void)gkinv_remove_item(&systems->item_db, &systems->inventory,
                                    b3d_weapon_item_id(weapon_id),
                                    (gkinv_u16)amount);
            return 0;
        }
        systems->current_weapon_id = weapon_id;
    }
    return 1;
}

int blank3d_systems_grant_ammo(Blank3DSystems *systems, int actor_id,
                               int ammo_id, int amount)
{
    int current;
    int capacity;
    int result;
    if (!systems || actor_id != B3D_PLAYER_ACTOR_ID ||
        ammo_id <= 0 || amount <= 0 || amount > 65535)
        return 0;
    capacity = b3d_ammo_capacity(systems, ammo_id);
    if (capacity <= 0) return 0;
    current = (int)gkinv_count_item(&systems->inventory,
                                    b3d_ammo_item_id(ammo_id));
    if (current > capacity || amount > capacity - current) return 0;
    result = gkinv_add_item(&systems->item_db, &systems->inventory,
                            b3d_ammo_item_id(ammo_id),
                            (gkinv_u16)amount, 0);
    return result == GKINV_OK ? 1 : 0;
}

CT89_Context *blank3d_systems_contact_triggers(Blank3DSystems *systems)
{
    return systems ? &systems->contact_triggers : 0;
}

PBB_ItemWorld *blank3d_systems_item_world(Blank3DSystems *systems)
{
    return systems ? &systems->item_world : 0;
}

PBBCTW89_Bridge *blank3d_systems_item_contact_bridge(Blank3DSystems *systems)
{
    return systems ? &systems->item_contact_bridge : 0;
}

int blank3d_systems_player_item_actor(const Blank3DSystems *systems)
{
    return systems ? systems->player_item_actor_id : PBB_ITEM_INVALID_ID;
}

int blank3d_systems_bind_item_actor(Blank3DSystems *systems,
                                    CT89_Subject subject,
                                    int host_actor_id,
                                    unsigned long class_mask,
                                    unsigned long team_mask,
                                    unsigned long touch_mask,
                                    unsigned long interact_mask)
{
    int actor_id;
    if (!systems || subject == CT89_SUBJECT_INVALID) return PBB_ITEM_INVALID_ID;
    actor_id = pbb_item_actor_create(&systems->item_world,
                                     class_mask, team_mask,
                                     0L, 0L,
                                     PBB_FIXED_HALF, PBB_FIXED_HALF);
    if (actor_id == PBB_ITEM_INVALID_ID) return actor_id;
    (void)pbb_item_actor_set_masks(&systems->item_world, actor_id,
                                   touch_mask, interact_mask);
    if (pbbctw89_bind_actor(&systems->item_contact_bridge,
                            subject, actor_id, host_actor_id) != PBBCTW89_OK) {
        (void)pbb_item_actor_destroy(&systems->item_world, actor_id);
        return PBB_ITEM_INVALID_ID;
    }
    return actor_id;
}

int blank3d_systems_activate_item_trigger(Blank3DSystems *systems,
                                           CT89_Trigger trigger,
                                           CT89_Subject activator_subject)
{
    if (!systems || trigger == CT89_TRIGGER_INVALID ||
        activator_subject == CT89_SUBJECT_INVALID)
        return CT89_ACTION_UNHANDLED;
    return ct89_activate(&systems->contact_triggers, trigger,
                         activator_subject);
}

static int b3d_define_pickup(Blank3DSystems *systems,
                             const char *name,
                             int pickup_kind,
                             int resource_id,
                             int amount,
                             int auto_equip,
                             int interact_required,
                             CT89_Subject owner_subject,
                             int sensor_mode,
                             CT89_FX radius_fx,
                             unsigned long contact_category_mask,
                             int consume_policy,
                             int *out_item_id,
                             CT89_Trigger *out_trigger)
{
    int def_id;
    int item_id;
    int effect_id;
    int consume_effect_id;
    int rule_id;
    int hook;
    int action_id;
    unsigned long flags;
    unsigned long hook_mask;
    CT89_Trigger trigger;
    if (!systems || !name || amount <= 0 ||
        owner_subject == CT89_SUBJECT_INVALID)
        return 0;
    if ((pickup_kind == PBBCTW89_PICKUP_WEAPON ||
         pickup_kind == PBBCTW89_PICKUP_AMMO) && resource_id <= 0)
        return 0;
    if (pickup_kind != PBBCTW89_PICKUP_WEAPON &&
        pickup_kind != PBBCTW89_PICKUP_AMMO &&
        pickup_kind != B3D_SYSTEMS_PICKUP_KIND_HEALTH)
        return 0;

    def_id = pbb_item_def_create(&systems->item_world, name);
    if (def_id == PBB_ITEM_INVALID_ID) return 0;
    flags = PBB_ITEMF_ACTIVE | PBB_ITEMF_VISIBLE;
    if (interact_required) flags |= PBB_ITEMF_INTERACTABLE;
    else flags |= PBB_ITEMF_TOUCHABLE;
    (void)pbb_item_def_set_defaults(&systems->item_world, def_id,
                                    flags, amount,
                                    PBB_FIXED_HALF, PBB_FIXED_HALF);
    (void)pbb_item_def_set_category(&systems->item_world, def_id,
                                    PBB_ITEM_CATEGORY_PICKUP);
    hook = interact_required ? PBB_ITEM_HOOK_INTERACT : PBB_ITEM_HOOK_TOUCH;

    if (pickup_kind == B3D_SYSTEMS_PICKUP_KIND_HEALTH) {
        rule_id = pbb_item_rule_add(&systems->item_world,
                                    PBB_RULE_CALL_CUSTOM,
                                    B3D_PBB_RULE_HEALTH_BELOW_MAX,
                                    B3D_PLAYER_HEALTH_MAX, 0, 0);
        if (rule_id < 0) return 0;
        (void)pbb_item_def_set_rule_block(&systems->item_world,
                                          def_id, hook, rule_id, 1);
        effect_id = pbb_item_effect_add(&systems->item_world,
                                        PBB_EFFECT_CALL_CUSTOM,
                                        B3D_PBB_EFFECT_HEAL_FROM_ITEM_AMOUNT,
                                        B3D_PLAYER_HEALTH_MAX,
                                        0, 0, 0L, 0L);
        if (effect_id < 0) return 0;
        consume_effect_id = pbb_item_effect_add(&systems->item_world,
                                                PBB_EFFECT_CONSUME_ITEM,
                                                0, 0, 0, 0, 0L, 0L);
        if (consume_effect_id != effect_id + 1) return 0;
        (void)pbb_item_def_set_effect_block(&systems->item_world,
                                            def_id, hook, effect_id, 2);
    } else {
        effect_id = pbb_item_effect_add(&systems->item_world,
                                        PBB_EFFECT_CONSUME_ITEM,
                                        0, 0, 0, 0, 0L, 0L);
        if (effect_id < 0) return 0;
        (void)pbb_item_def_set_effect_block(&systems->item_world,
                                            def_id, hook, effect_id, 1);
    }

    item_id = pbb_item_spawn(&systems->item_world, def_id, 0L, 0L, amount);
    if (item_id == PBB_ITEM_INVALID_ID) return 0;

    hook_mask = interact_required ? PBBCTW89_HOOK_INTERACT : PBBCTW89_HOOK_TOUCH;
    if (pickup_kind == PBBCTW89_PICKUP_WEAPON) {
        if (pbbctw89_bind_weapon_pickup(&systems->item_contact_bridge,
                                        item_id, resource_id, amount,
                                        auto_equip, hook_mask) != PBBCTW89_OK) {
            (void)pbb_item_destroy(&systems->item_world, item_id);
            return 0;
        }
    } else if (pickup_kind == PBBCTW89_PICKUP_AMMO) {
        if (pbbctw89_bind_ammo_pickup(&systems->item_contact_bridge,
                                      item_id, resource_id, amount,
                                      hook_mask) != PBBCTW89_OK) {
            (void)pbb_item_destroy(&systems->item_world, item_id);
            return 0;
        }
    }

    action_id = interact_required ? PBBCTW89_ACTION_INTERACT
                                  : PBBCTW89_ACTION_TOUCH;
    trigger = pbbctw89_create_item_trigger(&systems->item_contact_bridge,
                                           owner_subject, item_id,
                                           sensor_mode, radius_fx,
                                           contact_category_mask,
                                           action_id, consume_policy);
    if (trigger == CT89_TRIGGER_INVALID) {
        if (pickup_kind == PBBCTW89_PICKUP_WEAPON ||
            pickup_kind == PBBCTW89_PICKUP_AMMO)
            (void)pbbctw89_clear_pickup_binding(&systems->item_contact_bridge,
                                                item_id);
        (void)pbb_item_destroy(&systems->item_world, item_id);
        return 0;
    }
    if (out_item_id) *out_item_id = item_id;
    if (out_trigger) *out_trigger = trigger;
    return 1;
}

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
                                         CT89_Trigger *out_trigger)
{
    return b3d_define_pickup(systems, name, PBBCTW89_PICKUP_WEAPON,
                             weapon_id, amount, auto_equip,
                             interact_required, owner_subject,
                             sensor_mode, radius_fx, contact_category_mask,
                             consume_policy, out_item_id, out_trigger);
}

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
                                       CT89_Trigger *out_trigger)
{
    return b3d_define_pickup(systems, name, PBBCTW89_PICKUP_AMMO,
                             ammo_id, amount, 0,
                             interact_required, owner_subject,
                             sensor_mode, radius_fx, contact_category_mask,
                             consume_policy, out_item_id, out_trigger);
}

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
                                         CT89_Trigger *out_trigger)
{
    return b3d_define_pickup(systems, name, B3D_SYSTEMS_PICKUP_KIND_HEALTH,
                             0, amount, 0,
                             interact_required, owner_subject,
                             sensor_mode, radius_fx, contact_category_mask,
                             consume_policy, out_item_id, out_trigger);
}

int blank3d_systems_set_multiplier_q16(Blank3DSystems *systems, int numeric_key,
                                       long value_q16)
{
    ns_id value_id;
    ns_fx value_q10;
    if (!systems) return NS_ERR_BAD_ARG;
    value_id = b3d_numeric_value_id(systems, numeric_key);
    if (value_id == NS_INVALID_ID) return NS_ERR_NOT_FOUND;
    value_q10 = (ns_fx)(value_q16 / 64L);
    return ns_set_by_id(&systems->numbers, value_id, value_q10);
}

long blank3d_systems_get_multiplier_q16(const Blank3DSystems *systems,
                                        int numeric_key, long fallback_q16)
{
    ns_id value_id;
    ns_fx value_q10;
    if (!systems) return fallback_q16;
    value_id = b3d_numeric_value_id(systems, numeric_key);
    if (value_id == NS_INVALID_ID) return fallback_q16;
    if (ns_get_by_id(&systems->numbers, value_id, &value_q10) != NS_OK)
        return fallback_q16;
    return (long)value_q10 * 64L;
}

void blank3d_systems_set_launch_speed_q16(Blank3DSystems *systems,
                                          long speed_q16)
{
    if (!systems) return;
    if (speed_q16 < 0L) speed_q16 = 0L;
    systems->launch_speed_override_q16 = speed_q16;
}
