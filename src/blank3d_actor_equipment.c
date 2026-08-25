#include "blank3d_actor_equipment.h"

#include <string.h>

static void b3d_equipment_copy(char *dst, unsigned int cap, const char *src)
{
    unsigned int i;
    if (dst == 0 || cap == 0U) return;
    if (src == 0) src = "";
    i = 0U;
    while (i + 1U < cap && src[i] != '\0') {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static int b3d_eq_resolve_item(void *user, int item_id, EQ89_ItemDesc *out)
{
    Blank3DActorEquipmentSystem *equipment;
    const Blank3DWeaponPresentation *presentation;
    equipment = (Blank3DActorEquipmentSystem *)user;
    if (equipment == 0 || out == 0 || equipment->presentations == 0) return 0;
    presentation = blank3d_weapon_presentation_find(equipment->presentations,
                                                     item_id);
    if (presentation == 0) return 0;
    memset(out, 0, sizeof(*out));
    out->item_id = item_id;
    out->model_id = presentation->model_id;
    b3d_equipment_copy(out->socket_name, sizeof(out->socket_name),
                       presentation->socket_name);
    b3d_equipment_copy(out->attachment_name, sizeof(out->attachment_name),
                       presentation->attachment_name);
    b3d_equipment_copy(out->model_name, sizeof(out->model_name),
                       presentation->model_name);
    out->host_profile = presentation;
    out->local_offset = &presentation->grip_offset;
    return 1;
}

static int b3d_eq_define_socket(void *user, int actor_id,
                                const char *socket_name, int socket_index,
                                const void *local_offset)
{
    Blank3DActorEquipmentSystem *equipment;
    equipment = (Blank3DActorEquipmentSystem *)user;
    if (equipment == 0 || equipment->attachments == 0) return 0;
    return blank3d_attachment_define_socket(equipment->attachments,
        actor_id, socket_name, socket_index,
        (const GAtt89_Xform *)local_offset) >= 0;
}

static int b3d_eq_attach(void *user, int actor_id, int object_id,
                         const char *attachment_name,
                         const char *socket_name, const void *local_offset)
{
    Blank3DActorEquipmentSystem *equipment;
    equipment = (Blank3DActorEquipmentSystem *)user;
    if (equipment == 0 || equipment->attachments == 0) return 0;
    return blank3d_attachment_attach_object(equipment->attachments,
        actor_id, object_id, attachment_name, socket_name,
        (const GAtt89_Xform *)local_offset) >= 0;
}

static int b3d_eq_detach(void *user, int actor_id, int object_id,
                         const char *attachment_name)
{
    Blank3DActorEquipmentSystem *equipment;
    equipment = (Blank3DActorEquipmentSystem *)user;
    if (equipment == 0 || equipment->attachments == 0) return 0;
    return blank3d_attachment_detach_object(equipment->attachments,
        actor_id, object_id, attachment_name) >= 0;
}

static int b3d_eq_visible(void *user, int actor_id, int object_id,
                          const char *attachment_name, int visible)
{
    Blank3DActorEquipmentSystem *equipment;
    equipment = (Blank3DActorEquipmentSystem *)user;
    if (equipment == 0 || equipment->attachments == 0) return 0;
    return blank3d_attachment_set_object_visible(equipment->attachments,
        actor_id, object_id, attachment_name, visible) >= 0;
}

static int b3d_eq_get_xform(void *user, int object_id,
                            void *out_xform, unsigned int out_size)
{
    Blank3DActorEquipmentSystem *equipment;
    GAtt89_Xform xform;
    equipment = (Blank3DActorEquipmentSystem *)user;
    if (equipment == 0 || equipment->attachments == 0 || out_xform == 0 ||
        out_size < sizeof(xform)) return 0;
    if (!blank3d_attachment_get_object_xform(equipment->attachments,
            object_id, &xform)) return 0;
    memcpy(out_xform, &xform, sizeof(xform));
    return 1;
}

static int b3d_eq_runtime_init(void *user, int slot,
                               const EQ89_ItemDesc *desc)
{
    Blank3DActorEquipmentSystem *equipment;
    const Blank3DWeaponPresentation *presentation;
    equipment = (Blank3DActorEquipmentSystem *)user;
    if (equipment == 0 || desc == 0 || slot < 0 ||
        slot >= B3D_EQUIPMENT_MAX_ACTORS) return 0;
    presentation = (const Blank3DWeaponPresentation *)desc->host_profile;
    if (presentation == 0) return 0;
    return blank3d_mechanical_weapon_init_profile(
        &equipment->animators[slot], presentation);
}

static int b3d_eq_runtime_sync(void *user, int slot, int actor_id, int item_id,
                               const void *object_xform,
                               unsigned int xform_size,
                               unsigned short frame_ms)
{
    Blank3DActorEquipmentSystem *equipment;
    GAtt89_Xform aligned_xform;
    int user_slot;
    GWP89_UserState *state;
    const GWP89_WeaponProfile *profile;
    equipment = (Blank3DActorEquipmentSystem *)user;
    if (equipment == 0 || equipment->weapon_manager == 0 || slot < 0 ||
        slot >= B3D_EQUIPMENT_MAX_ACTORS) return 0;
    if (object_xform == 0 || xform_size < sizeof(aligned_xform)) return 0;
    memcpy(&aligned_xform, object_xform, sizeof(aligned_xform));
    user_slot = gwp89_find_user_slot(equipment->weapon_manager, actor_id);
    if (user_slot < 0) return 0;
    state = &equipment->weapon_manager->users[user_slot];
    profile = gwp89_get_weapon(equipment->weapon_manager, state->weapon_slot);
    if (profile == 0 || profile->weapon_id != item_id) return 0;
    blank3d_mechanical_weapon_set_attachment(&equipment->animators[slot],
                                              &aligned_xform);
    blank3d_mechanical_weapon_set_reload(&equipment->animators[slot],
        state->reload_active, state->reload_elapsed_ms,
        profile->reload_ms > 0U ? profile->reload_ms : 1U);
    return blank3d_mechanical_weapon_update(&equipment->animators[slot],
                                             frame_ms);
}

static void b3d_eq_runtime_trigger(void *user, int slot, int event_id)
{
    Blank3DActorEquipmentSystem *equipment;
    equipment = (Blank3DActorEquipmentSystem *)user;
    if (equipment == 0 || slot < 0 || slot >= B3D_EQUIPMENT_MAX_ACTORS) return;
    if (event_id == B3D_EQUIPMENT_EVENT_FIRE)
        blank3d_mechanical_weapon_trigger_fire(&equipment->animators[slot]);
}

static int b3d_eq_packet_count(void *user, int slot)
{
    Blank3DActorEquipmentSystem *equipment;
    equipment = (Blank3DActorEquipmentSystem *)user;
    if (equipment == 0 || slot < 0 || slot >= B3D_EQUIPMENT_MAX_ACTORS) return 0;
    return blank3d_mechanical_weapon_packet_count(&equipment->animators[slot]);
}

static const void *b3d_eq_packet(void *user, int slot, int packet_index)
{
    Blank3DActorEquipmentSystem *equipment;
    equipment = (Blank3DActorEquipmentSystem *)user;
    if (equipment == 0 || slot < 0 || slot >= B3D_EQUIPMENT_MAX_ACTORS) return 0;
    return blank3d_mechanical_weapon_packet(&equipment->animators[slot],
                                             packet_index);
}

static const void *b3d_eq_muzzle(void *user, int slot)
{
    Blank3DActorEquipmentSystem *equipment;
    equipment = (Blank3DActorEquipmentSystem *)user;
    if (equipment == 0 || slot < 0 || slot >= B3D_EQUIPMENT_MAX_ACTORS) return 0;
    return blank3d_mechanical_weapon_muzzle_world(&equipment->animators[slot]);
}

void blank3d_actor_equipment_init(
    Blank3DActorEquipmentSystem *equipment,
    Blank3DAttachmentWorld *attachments,
    const Blank3DWeaponPresentationRegistry *presentations)
{
    EQ89_Provider provider;
    if (equipment == 0) return;
    memset(equipment, 0, sizeof(*equipment));
    equipment->attachments = attachments;
    equipment->presentations = presentations;
    memset(&provider, 0, sizeof(provider));
    provider.user = equipment;
    provider.resolve_item = b3d_eq_resolve_item;
    provider.define_socket = b3d_eq_define_socket;
    provider.attach = b3d_eq_attach;
    provider.detach = b3d_eq_detach;
    provider.set_visible = b3d_eq_visible;
    provider.get_object_xform = b3d_eq_get_xform;
    provider.runtime_init = b3d_eq_runtime_init;
    provider.runtime_sync = b3d_eq_runtime_sync;
    provider.runtime_trigger = b3d_eq_runtime_trigger;
    provider.runtime_packet_count = b3d_eq_packet_count;
    provider.runtime_packet = b3d_eq_packet;
    provider.runtime_muzzle = b3d_eq_muzzle;
    eq89_init(&equipment->core, B3D_EQUIPMENT_OBJECT_BASE, &provider);
    equipment->initialized = equipment->core.initialized;
    equipment->last_result = equipment->core.last_result;
    b3d_equipment_copy(equipment->status, sizeof(equipment->status),
                       eq89_status(&equipment->core));
}

int blank3d_actor_equipment_define_socket(
    Blank3DActorEquipmentSystem *equipment, int actor_id,
    const char *socket_name, int socket_index,
    const GAtt89_Xform *local_offset)
{
    return equipment != 0 ? eq89_define_socket(&equipment->core, actor_id,
        socket_name, socket_index, local_offset) : 0;
}

int blank3d_actor_equipment_equip(
    Blank3DActorEquipmentSystem *equipment, int actor_id,
    int actor_kind, int weapon_id)
{
    int result;
    if (equipment == 0) return 0;
    result = eq89_equip(&equipment->core, actor_id, actor_kind, weapon_id);
    equipment->last_result = result;
    b3d_equipment_copy(equipment->status, sizeof(equipment->status),
                       eq89_status(&equipment->core));
    return result;
}

int blank3d_actor_equipment_unequip(
    Blank3DActorEquipmentSystem *equipment, int actor_id)
{
    return equipment != 0 ? eq89_unequip(&equipment->core, actor_id) : 0;
}

int blank3d_actor_equipment_set_visible(
    Blank3DActorEquipmentSystem *equipment, int actor_id, int visible)
{
    return equipment != 0 ? eq89_set_visible(&equipment->core, actor_id,
                                               visible) : 0;
}

static int b3d_equipped_weapon_id(GWP89_Manager *manager, int actor_id)
{
    int user_slot;
    const GWP89_UserState *state;
    const GWP89_WeaponProfile *profile;
    if (manager == 0) return 0;
    user_slot = gwp89_find_user_slot(manager, actor_id);
    if (user_slot < 0) return 0;
    state = &manager->users[user_slot];
    profile = gwp89_get_weapon(manager, state->weapon_slot);
    return profile != 0 ? profile->weapon_id : 0;
}

int blank3d_actor_equipment_sync_actor(
    Blank3DActorEquipmentSystem *equipment, GWP89_Manager *weapon_manager,
    int actor_id, int actor_kind, unsigned short frame_ms)
{
    int weapon_id;
    int result;
    if (equipment == 0 || weapon_manager == 0) return 0;
    equipment->weapon_manager = weapon_manager;
    weapon_id = b3d_equipped_weapon_id(weapon_manager, actor_id);
    if (weapon_id <= 0) return 0;
    result = eq89_sync(&equipment->core, actor_id, actor_kind,
                       weapon_id, frame_ms);
    equipment->last_result = result;
    return result;
}

void blank3d_actor_equipment_trigger_fire(
    Blank3DActorEquipmentSystem *equipment, int actor_id, int weapon_id)
{
    if (equipment == 0) return;
    eq89_trigger(&equipment->core, actor_id, weapon_id,
                 B3D_EQUIPMENT_EVENT_FIRE);
}

Blank3DActorEquipmentInstance *blank3d_actor_equipment_find(
    Blank3DActorEquipmentSystem *equipment, int actor_id)
{
    return equipment != 0 ? eq89_find(&equipment->core, actor_id) : 0;
}

const Blank3DActorEquipmentInstance *blank3d_actor_equipment_find_const(
    const Blank3DActorEquipmentSystem *equipment, int actor_id)
{
    return equipment != 0 ? eq89_find_const(&equipment->core, actor_id) : 0;
}

Blank3DMechanicalWeapon *blank3d_actor_equipment_animator(
    Blank3DActorEquipmentSystem *equipment, int actor_id)
{
    int slot;
    if (equipment == 0) return 0;
    slot = eq89_find_slot(&equipment->core, actor_id);
    return slot >= 0 ? &equipment->animators[slot] : 0;
}

int blank3d_actor_equipment_packet_count(
    const Blank3DActorEquipmentSystem *equipment, int actor_id)
{
    return equipment != 0 ? eq89_packet_count(&equipment->core, actor_id) : 0;
}

const nm89_geometry_packet *blank3d_actor_equipment_packet(
    const Blank3DActorEquipmentSystem *equipment, int actor_id,
    int packet_index)
{
    return equipment != 0 ? (const nm89_geometry_packet *)eq89_packet(
        &equipment->core, actor_id, packet_index) : 0;
}

const nm89_matrix *blank3d_actor_equipment_muzzle_world(
    const Blank3DActorEquipmentSystem *equipment, int actor_id)
{
    return equipment != 0 ? (const nm89_matrix *)eq89_muzzle(
        &equipment->core, actor_id) : 0;
}

int blank3d_actor_equipment_model_id(
    const Blank3DActorEquipmentSystem *equipment, int actor_id)
{
    return equipment != 0 ? eq89_model_id(&equipment->core, actor_id) : 0;
}

const char *blank3d_actor_equipment_model_name(
    const Blank3DActorEquipmentSystem *equipment, int actor_id)
{
    return equipment != 0 ? eq89_model_name(&equipment->core, actor_id) : "";
}

int blank3d_actor_equipment_object_id(
    const Blank3DActorEquipmentSystem *equipment, int actor_id)
{
    return equipment != 0 ? eq89_object_id(&equipment->core, actor_id)
                          : GATTACH89_INVALID_ID;
}

const char *blank3d_actor_equipment_status(
    const Blank3DActorEquipmentSystem *equipment)
{
    return equipment != 0 ? eq89_status(&equipment->core)
                          : "actor equipment: null";
}
