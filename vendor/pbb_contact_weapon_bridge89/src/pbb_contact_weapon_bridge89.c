#include "pbb_contact_weapon_bridge89.h"
#include <string.h>

static PBBCTW89_ActorBinding *pbbctw89_find_actor_subject(
    PBBCTW89_Bridge *bridge, CT89_Subject subject)
{
    int i;
    if (!bridge || subject == CT89_SUBJECT_INVALID) return 0;
    for (i = 0; i < PBBCTW89_MAX_ACTOR_BINDINGS; ++i) {
        if (bridge->actors[i].used && bridge->actors[i].subject == subject)
            return &bridge->actors[i];
    }
    return 0;
}

static PBBCTW89_ActorBinding *pbbctw89_find_actor_pbb(
    PBBCTW89_Bridge *bridge, int pbb_actor_id)
{
    int i;
    if (!bridge || pbb_actor_id < 0) return 0;
    for (i = 0; i < PBBCTW89_MAX_ACTOR_BINDINGS; ++i) {
        if (bridge->actors[i].used &&
            bridge->actors[i].pbb_actor_id == pbb_actor_id)
            return &bridge->actors[i];
    }
    return 0;
}

static PBBCTW89_ItemBinding *pbbctw89_find_item_trigger(
    PBBCTW89_Bridge *bridge, CT89_Trigger trigger)
{
    int i;
    if (!bridge || trigger == CT89_TRIGGER_INVALID) return 0;
    for (i = 0; i < PBBCTW89_MAX_ITEM_BINDINGS; ++i) {
        if (bridge->item_bindings[i].used &&
            bridge->item_bindings[i].trigger == trigger)
            return &bridge->item_bindings[i];
    }
    return 0;
}

static PBBCTW89_PickupBinding *pbbctw89_find_pickup(
    PBBCTW89_Bridge *bridge, int pbb_item_id)
{
    int i;
    if (!bridge || pbb_item_id < 0) return 0;
    for (i = 0; i < PBBCTW89_MAX_ITEM_BINDINGS; ++i) {
        if (bridge->pickup_bindings[i].used &&
            bridge->pickup_bindings[i].pbb_item_id == pbb_item_id)
            return &bridge->pickup_bindings[i];
    }
    return 0;
}

void pbbctw89_weapon_provider_init(PBBCTW89_WeaponProvider *provider)
{
    if (provider) memset(provider, 0, sizeof(*provider));
}

void pbbctw89_init(PBBCTW89_Bridge *bridge)
{
    if (!bridge) return;
    memset(bridge, 0, sizeof(*bridge));
    ct89_action_provider_init(&bridge->previous_action_provider);
    bridge->last_result = PBBCTW89_OK;
}

int pbbctw89_attach(PBBCTW89_Bridge *bridge,
                    CT89_Context *contact,
                    PBB_ItemWorld *items)
{
    CT89_ActionProvider provider;
    if (!bridge || !contact || !items) return PBBCTW89_ERR_ARGUMENT;
    if (bridge->attached) pbbctw89_detach(bridge);

    bridge->contact = contact;
    bridge->items = items;
    bridge->previous_action_provider = contact->action_provider;
    bridge->previous_action_gate = pbb_item_world_get_action_gate(items);
    bridge->previous_action_gate_user = pbb_item_world_get_action_gate_user(items);

    ct89_action_provider_init(&provider);
    provider.user = bridge;
    provider.execute = pbbctw89_contact_action;
    ct89_set_action_provider(contact, &provider);
    pbb_item_world_set_action_gate(items, pbbctw89_item_action_gate, bridge);
    bridge->attached = 1;
    bridge->last_result = PBBCTW89_OK;
    return PBBCTW89_OK;
}

void pbbctw89_detach(PBBCTW89_Bridge *bridge)
{
    if (!bridge || !bridge->attached) return;
    if (bridge->contact)
        ct89_set_action_provider(bridge->contact,
                                 &bridge->previous_action_provider);
    if (bridge->items)
        pbb_item_world_set_action_gate(bridge->items,
                                       bridge->previous_action_gate,
                                       bridge->previous_action_gate_user);
    bridge->contact = 0;
    bridge->items = 0;
    bridge->attached = 0;
}

void pbbctw89_set_weapon_provider(PBBCTW89_Bridge *bridge,
                                  const PBBCTW89_WeaponProvider *provider)
{
    if (!bridge) return;
    if (provider) bridge->weapon_provider = *provider;
    else pbbctw89_weapon_provider_init(&bridge->weapon_provider);
}

int pbbctw89_bind_actor(PBBCTW89_Bridge *bridge,
                        CT89_Subject subject,
                        int pbb_actor_id,
                        int host_actor_id)
{
    int i;
    PBBCTW89_ActorBinding *binding;
    if (!bridge || subject == CT89_SUBJECT_INVALID || pbb_actor_id < 0)
        return PBBCTW89_ERR_ARGUMENT;
    binding = pbbctw89_find_actor_subject(bridge, subject);
    if (!binding) {
        for (i = 0; i < PBBCTW89_MAX_ACTOR_BINDINGS; ++i) {
            if (!bridge->actors[i].used) {
                binding = &bridge->actors[i];
                break;
            }
        }
    }
    if (!binding) return PBBCTW89_ERR_FULL;
    binding->used = 1U;
    binding->subject = subject;
    binding->pbb_actor_id = pbb_actor_id;
    binding->host_actor_id = host_actor_id;
    return PBBCTW89_OK;
}

int pbbctw89_unbind_actor(PBBCTW89_Bridge *bridge, CT89_Subject subject)
{
    PBBCTW89_ActorBinding *binding;
    binding = pbbctw89_find_actor_subject(bridge, subject);
    if (!binding) return PBBCTW89_ERR_NOT_FOUND;
    memset(binding, 0, sizeof(*binding));
    return PBBCTW89_OK;
}

int pbbctw89_bind_item_trigger(PBBCTW89_Bridge *bridge,
                               CT89_Trigger trigger,
                               CT89_Subject owner_subject,
                               int pbb_item_id,
                               int action_id)
{
    int i;
    PBBCTW89_ItemBinding *binding;
    if (!bridge || trigger == CT89_TRIGGER_INVALID || pbb_item_id < 0)
        return PBBCTW89_ERR_ARGUMENT;
    if (action_id != PBBCTW89_ACTION_TOUCH &&
        action_id != PBBCTW89_ACTION_INTERACT)
        return PBBCTW89_ERR_ARGUMENT;
    binding = pbbctw89_find_item_trigger(bridge, trigger);
    if (!binding) {
        for (i = 0; i < PBBCTW89_MAX_ITEM_BINDINGS; ++i) {
            if (!bridge->item_bindings[i].used) {
                binding = &bridge->item_bindings[i];
                break;
            }
        }
    }
    if (!binding) return PBBCTW89_ERR_FULL;
    binding->used = 1U;
    binding->trigger = trigger;
    binding->owner_subject = owner_subject;
    binding->pbb_item_id = pbb_item_id;
    binding->action_id = action_id;
    return PBBCTW89_OK;
}

int pbbctw89_unbind_item_trigger(PBBCTW89_Bridge *bridge,
                                 CT89_Trigger trigger)
{
    PBBCTW89_ItemBinding *binding;
    binding = pbbctw89_find_item_trigger(bridge, trigger);
    if (!binding) return PBBCTW89_ERR_NOT_FOUND;
    memset(binding, 0, sizeof(*binding));
    return PBBCTW89_OK;
}

static int pbbctw89_bind_pickup(PBBCTW89_Bridge *bridge,
                                int pbb_item_id,
                                int pickup_kind,
                                int resource_id,
                                int amount,
                                int auto_equip,
                                unsigned long hook_mask)
{
    int i;
    PBBCTW89_PickupBinding *binding;
    if (!bridge || pbb_item_id < 0 || resource_id <= 0 || amount <= 0)
        return PBBCTW89_ERR_ARGUMENT;
    if (pickup_kind != PBBCTW89_PICKUP_WEAPON &&
        pickup_kind != PBBCTW89_PICKUP_AMMO)
        return PBBCTW89_ERR_ARGUMENT;
    binding = pbbctw89_find_pickup(bridge, pbb_item_id);
    if (!binding) {
        for (i = 0; i < PBBCTW89_MAX_ITEM_BINDINGS; ++i) {
            if (!bridge->pickup_bindings[i].used) {
                binding = &bridge->pickup_bindings[i];
                break;
            }
        }
    }
    if (!binding) return PBBCTW89_ERR_FULL;
    memset(binding, 0, sizeof(*binding));
    binding->used = 1U;
    binding->pbb_item_id = pbb_item_id;
    binding->pickup_kind = pickup_kind;
    binding->resource_id = resource_id;
    binding->amount = amount;
    binding->auto_equip = auto_equip ? 1 : 0;
    binding->hook_mask = hook_mask;
    return PBBCTW89_OK;
}

int pbbctw89_bind_weapon_pickup(PBBCTW89_Bridge *bridge,
                                 int pbb_item_id,
                                 int weapon_id,
                                 int amount,
                                 int auto_equip,
                                 unsigned long hook_mask)
{
    return pbbctw89_bind_pickup(bridge, pbb_item_id,
                                PBBCTW89_PICKUP_WEAPON,
                                weapon_id, amount, auto_equip, hook_mask);
}

int pbbctw89_bind_ammo_pickup(PBBCTW89_Bridge *bridge,
                               int pbb_item_id,
                               int ammo_id,
                               int amount,
                               unsigned long hook_mask)
{
    return pbbctw89_bind_pickup(bridge, pbb_item_id,
                                PBBCTW89_PICKUP_AMMO,
                                ammo_id, amount, 0, hook_mask);
}

int pbbctw89_clear_pickup_binding(PBBCTW89_Bridge *bridge, int pbb_item_id)
{
    PBBCTW89_PickupBinding *binding;
    binding = pbbctw89_find_pickup(bridge, pbb_item_id);
    if (!binding) return PBBCTW89_ERR_NOT_FOUND;
    memset(binding, 0, sizeof(*binding));
    return PBBCTW89_OK;
}

int pbbctw89_reset_pickup(PBBCTW89_Bridge *bridge, int pbb_item_id)
{
    PBBCTW89_PickupBinding *binding;
    binding = pbbctw89_find_pickup(bridge, pbb_item_id);
    if (!binding) return PBBCTW89_ERR_NOT_FOUND;
    binding->granted = 0U;
    return PBBCTW89_OK;
}

CT89_Trigger pbbctw89_create_item_trigger(PBBCTW89_Bridge *bridge,
                                           CT89_Subject owner_subject,
                                           int pbb_item_id,
                                           int sensor_mode,
                                           CT89_FX radius_fx,
                                           unsigned long category_mask,
                                           int action_id,
                                           int consume_policy)
{
    CT89_TriggerDesc desc;
    CT89_Trigger trigger;
    if (!bridge || !bridge->contact || pbb_item_id < 0 ||
        owner_subject == CT89_SUBJECT_INVALID)
        return CT89_TRIGGER_INVALID;
    if (action_id != PBBCTW89_ACTION_TOUCH &&
        action_id != PBBCTW89_ACTION_INTERACT)
        return CT89_TRIGGER_INVALID;

    ct89_trigger_desc_defaults(&desc);
    desc.owner = owner_subject;
    desc.sensor_mode = sensor_mode;
    desc.radius_fx = radius_fx;
    desc.category_mask = category_mask;
    desc.action_id = action_id;
    desc.consume_policy = consume_policy;
    if (sensor_mode == CT89_SENSOR_MANUAL) {
        /* A manual trigger has no ENTER/STAY/EXIT relation.  Keep the PBB
           semantic (touch vs interact), but drive it from ACTIVATE. */
        desc.action_event_mask = CT89_EVENT_MASK_ACTIVATE;
        desc.notify_event_mask = CT89_EVENT_MASK_ACTIVATE;
    } else if (action_id == PBBCTW89_ACTION_TOUCH) {
        desc.action_event_mask = CT89_EVENT_MASK_ENTER;
        desc.notify_event_mask = CT89_EVENT_MASK_ENTER | CT89_EVENT_MASK_EXIT;
    } else {
        desc.action_event_mask = CT89_EVENT_MASK_ACTIVATE;
        desc.notify_event_mask = CT89_EVENT_MASK_ENTER |
                                 CT89_EVENT_MASK_STAY |
                                 CT89_EVENT_MASK_EXIT |
                                 CT89_EVENT_MASK_ACTIVATE;
    }
    trigger = ct89_trigger_create(bridge->contact, &desc);
    if (trigger == CT89_TRIGGER_INVALID) return trigger;
    if (pbbctw89_bind_item_trigger(bridge, trigger, owner_subject,
                                   pbb_item_id, action_id) != PBBCTW89_OK) {
        (void)ct89_trigger_destroy(bridge->contact, trigger);
        return CT89_TRIGGER_INVALID;
    }
    return trigger;
}

int pbbctw89_contact_action(void *user, const CT89_Event *event)
{
    PBBCTW89_Bridge *bridge;
    PBBCTW89_ItemBinding *item_binding;
    PBBCTW89_ActorBinding *actor_binding;
    int result;
    bridge = (PBBCTW89_Bridge *)user;
    if (!bridge || !event || !bridge->items)
        return CT89_ACTION_UNHANDLED;

    item_binding = pbbctw89_find_item_trigger(bridge, event->trigger);
    if (!item_binding || item_binding->action_id != event->action_id) {
        if (bridge->previous_action_provider.execute)
            return bridge->previous_action_provider.execute(
                bridge->previous_action_provider.user, event);
        return CT89_ACTION_UNHANDLED;
    }
    actor_binding = pbbctw89_find_actor_subject(bridge, event->other);
    if (!actor_binding) return CT89_ACTION_REJECTED;

    if (item_binding->action_id == PBBCTW89_ACTION_TOUCH)
        result = pbb_item_touch_actor_item(bridge->items,
                                           actor_binding->pbb_actor_id,
                                           item_binding->pbb_item_id);
    else
        result = pbb_item_interact_actor_item(bridge->items,
                                              actor_binding->pbb_actor_id,
                                              item_binding->pbb_item_id);
    return result ? CT89_ACTION_ACCEPTED : CT89_ACTION_REJECTED;
}

int pbbctw89_item_action_gate(PBB_ItemWorld *world,
                              int pbb_actor_id,
                              int pbb_item_id,
                              int hook,
                              void *user)
{
    PBBCTW89_Bridge *bridge;
    PBBCTW89_PickupBinding *pickup;
    PBBCTW89_ActorBinding *actor;
    int accepted;
    bridge = (PBBCTW89_Bridge *)user;
    if (!bridge || !world) return 0;

    if (bridge->previous_action_gate) {
        accepted = bridge->previous_action_gate(
            world, pbb_actor_id, pbb_item_id, hook,
            bridge->previous_action_gate_user);
        if (!accepted) return 0;
    }

    pickup = pbbctw89_find_pickup(bridge, pbb_item_id);
    if (!pickup) return 1;
    if ((pickup->hook_mask & PBBCTW89_HOOK_BIT(hook)) == 0UL) return 1;
    if (pickup->granted) return 1;

    actor = pbbctw89_find_actor_pbb(bridge, pbb_actor_id);
    if (!actor) return 0;

    accepted = 0;
    if (pickup->pickup_kind == PBBCTW89_PICKUP_WEAPON) {
        if (!bridge->weapon_provider.grant_weapon) return 0;
        accepted = bridge->weapon_provider.grant_weapon(
            bridge->weapon_provider.user,
            actor->host_actor_id,
            pickup->resource_id,
            pickup->amount,
            pickup->auto_equip);
    } else if (pickup->pickup_kind == PBBCTW89_PICKUP_AMMO) {
        if (!bridge->weapon_provider.grant_ammo) return 0;
        accepted = bridge->weapon_provider.grant_ammo(
            bridge->weapon_provider.user,
            actor->host_actor_id,
            pickup->resource_id,
            pickup->amount);
    }
    if (!accepted) return 0;
    pickup->granted = 1U;
    return 1;
}

const char *pbbctw89_error_string(int result)
{
    switch (result) {
        case PBBCTW89_OK: return "ok";
        case PBBCTW89_ERR_ARGUMENT: return "bad argument";
        case PBBCTW89_ERR_FULL: return "binding table full";
        case PBBCTW89_ERR_NOT_FOUND: return "binding not found";
        case PBBCTW89_ERR_PROVIDER: return "provider rejected";
        case PBBCTW89_ERR_TRIGGER: return "trigger error";
        default: break;
    }
    return "unknown";
}
