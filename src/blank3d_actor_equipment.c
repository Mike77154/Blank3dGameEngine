#include "blank3d_actor_equipment.h"

#include <stdio.h>
#include <string.h>

static void b3d_equipment_copy(char *dst, unsigned int cap,
                               const char *src)
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

static int b3d_equipment_find_slot(
    const Blank3DActorEquipmentSystem *equipment,
    int actor_id)
{
    int i;
    if (equipment == 0) return -1;
    for (i = 0; i < B3D_EQUIPMENT_MAX_ACTORS; ++i) {
        if (equipment->instances[i].used &&
            equipment->instances[i].actor_id == actor_id)
            return i;
    }
    return -1;
}

static int b3d_equipment_ensure_slot(
    Blank3DActorEquipmentSystem *equipment,
    int actor_id,
    int actor_kind)
{
    int i;
    i = b3d_equipment_find_slot(equipment, actor_id);
    if (i >= 0) return i;
    for (i = 0; i < B3D_EQUIPMENT_MAX_ACTORS; ++i) {
        if (!equipment->instances[i].used) {
            memset(&equipment->instances[i], 0,
                   sizeof(equipment->instances[i]));
            equipment->instances[i].used = 1;
            equipment->instances[i].actor_id = actor_id;
            equipment->instances[i].actor_kind = actor_kind;
            equipment->instances[i].object_id =
                B3D_EQUIPMENT_OBJECT_BASE + i;
            equipment->instances[i].visible = 1;
            equipment->count += 1;
            return i;
        }
    }
    return -1;
}

void blank3d_actor_equipment_init(
    Blank3DActorEquipmentSystem *equipment,
    Blank3DAttachmentWorld *attachments,
    const Blank3DWeaponPresentationRegistry *presentations)
{
    if (equipment == 0) return;
    memset(equipment, 0, sizeof(*equipment));
    equipment->attachments = attachments;
    equipment->presentations = presentations;
    equipment->initialized = attachments != 0 && presentations != 0;
    equipment->last_result = equipment->initialized ? 1 : 0;
    b3d_equipment_copy(equipment->status, sizeof(equipment->status),
        equipment->initialized
        ? "universal actor equipment initialized"
        : "universal actor equipment missing provider");
}

int blank3d_actor_equipment_define_socket(
    Blank3DActorEquipmentSystem *equipment,
    int actor_id,
    const char *socket_name,
    int socket_index,
    const GAtt89_Xform *local_offset)
{
    if (equipment == 0 || !equipment->initialized || socket_name == 0)
        return 0;
    return blank3d_attachment_define_socket(equipment->attachments,
        actor_id, socket_name, socket_index, local_offset) >= 0;
}

Blank3DActorEquipmentInstance *blank3d_actor_equipment_find(
    Blank3DActorEquipmentSystem *equipment,
    int actor_id)
{
    int slot;
    slot = b3d_equipment_find_slot(equipment, actor_id);
    return slot >= 0 ? &equipment->instances[slot] : 0;
}

const Blank3DActorEquipmentInstance *blank3d_actor_equipment_find_const(
    const Blank3DActorEquipmentSystem *equipment,
    int actor_id)
{
    int slot;
    slot = b3d_equipment_find_slot(equipment, actor_id);
    return slot >= 0 ? &equipment->instances[slot] : 0;
}

int blank3d_actor_equipment_equip(
    Blank3DActorEquipmentSystem *equipment,
    int actor_id,
    int actor_kind,
    int weapon_id)
{
    int slot;
    int attachment_result;
    Blank3DActorEquipmentInstance *instance;
    const Blank3DWeaponPresentation *presentation;
    if (equipment == 0 || !equipment->initialized || weapon_id <= 0)
        return 0;
    presentation = blank3d_weapon_presentation_find(
        equipment->presentations, weapon_id);
    if (presentation == 0) {
        equipment->last_result = 0;
        b3d_equipment_copy(equipment->status, sizeof(equipment->status),
                           "weapon has no presentation profile");
        return 0;
    }
    slot = b3d_equipment_ensure_slot(equipment, actor_id, actor_kind);
    if (slot < 0) {
        equipment->last_result = 0;
        b3d_equipment_copy(equipment->status, sizeof(equipment->status),
                           "actor equipment capacity exhausted");
        return 0;
    }
    instance = &equipment->instances[slot];
    if (instance->attached && instance->weapon_id == weapon_id)
        return 1;
    if (instance->attached) {
        (void)blank3d_attachment_detach_object(equipment->attachments,
            instance->actor_id, instance->object_id,
            instance->attachment_name);
        instance->attached = 0;
    }
    b3d_equipment_copy(instance->socket_name,
                        sizeof(instance->socket_name),
                        presentation->socket_name);
    b3d_equipment_copy(instance->attachment_name,
                        sizeof(instance->attachment_name),
                        presentation->attachment_name);
    b3d_equipment_copy(instance->model_name,
                        sizeof(instance->model_name),
                        presentation->model_name);
    instance->weapon_id = weapon_id;
    instance->model_id = presentation->model_id;
    instance->actor_kind = actor_kind;
    if (!blank3d_mechanical_weapon_init_profile(
            &instance->animator, presentation)) {
        instance->last_result = 0;
        equipment->last_result = 0;
        b3d_equipment_copy(equipment->status, sizeof(equipment->status),
                           "mechanical animator profile failed");
        return 0;
    }
    attachment_result = blank3d_attachment_attach_object(
        equipment->attachments,
        actor_id,
        instance->object_id,
        instance->attachment_name,
        instance->socket_name,
        &presentation->grip_offset);
    if (attachment_result < 0) {
        instance->last_result = 0;
        equipment->last_result = 0;
        b3d_equipment_copy(equipment->status, sizeof(equipment->status),
                           "GAttach could not mount weapon object");
        return 0;
    }
    instance->attached = 1;
    instance->visible = 1;
    instance->last_result = 1;
    equipment->last_result = 1;
    sprintf(equipment->status,
            "actor %d equipped model %s with %s mechanism",
            actor_id, instance->model_name,
            blank3d_weapon_presentation_mechanism_name(
                presentation->mechanism_kind));
    return 1;
}

int blank3d_actor_equipment_unequip(
    Blank3DActorEquipmentSystem *equipment,
    int actor_id)
{
    Blank3DActorEquipmentInstance *instance;
    if (equipment == 0 || !equipment->initialized) return 0;
    instance = blank3d_actor_equipment_find(equipment, actor_id);
    if (instance == 0) return 1;
    if (instance->attached) {
        if (!blank3d_attachment_detach_object(equipment->attachments,
                instance->actor_id, instance->object_id,
                instance->attachment_name))
            return 0;
    }
    memset(instance, 0, sizeof(*instance));
    if (equipment->count > 0) equipment->count -= 1;
    return 1;
}

int blank3d_actor_equipment_set_visible(
    Blank3DActorEquipmentSystem *equipment,
    int actor_id,
    int visible)
{
    Blank3DActorEquipmentInstance *instance;
    if (equipment == 0 || !equipment->initialized) return 0;
    instance = blank3d_actor_equipment_find(equipment, actor_id);
    if (instance == 0 || !instance->attached) return 0;
    if (!blank3d_attachment_set_object_visible(equipment->attachments,
            instance->actor_id, instance->object_id,
            instance->attachment_name, visible))
        return 0;
    instance->visible = visible ? 1 : 0;
    return 1;
}

int blank3d_actor_equipment_sync_actor(
    Blank3DActorEquipmentSystem *equipment,
    GWP89_Manager *weapon_manager,
    int actor_id,
    int actor_kind,
    unsigned short frame_ms)
{
    int user_slot;
    GWP89_UserState *user;
    const GWP89_WeaponProfile *weapon_profile;
    Blank3DActorEquipmentInstance *instance;
    GAtt89_Xform object_world;
    if (equipment == 0 || !equipment->initialized || weapon_manager == 0)
        return 0;
    user_slot = gwp89_find_user_slot(weapon_manager, actor_id);
    if (user_slot < 0) return 0;
    user = &weapon_manager->users[user_slot];
    weapon_profile = gwp89_get_weapon(weapon_manager, user->weapon_slot);
    if (weapon_profile == 0) return 0;
    instance = blank3d_actor_equipment_find(equipment, actor_id);
    if (instance == 0 || !instance->attached ||
        instance->weapon_id != weapon_profile->weapon_id) {
        if (!blank3d_actor_equipment_equip(equipment, actor_id,
                actor_kind, weapon_profile->weapon_id)) return 0;
        instance = blank3d_actor_equipment_find(equipment, actor_id);
        if (instance == 0) return 0;
    }
    if (!blank3d_attachment_get_object_xform(equipment->attachments,
            instance->object_id, &object_world)) return 0;
    blank3d_mechanical_weapon_set_attachment(&instance->animator,
                                              &object_world);
    blank3d_mechanical_weapon_set_reload(&instance->animator,
        user->reload_active,
        user->reload_elapsed_ms,
        weapon_profile->reload_ms > 0U ? weapon_profile->reload_ms : 1U);
    instance->last_result = blank3d_mechanical_weapon_update(
        &instance->animator, frame_ms);
    equipment->last_result = instance->last_result;
    return instance->last_result;
}

void blank3d_actor_equipment_trigger_fire(
    Blank3DActorEquipmentSystem *equipment,
    int actor_id,
    int weapon_id)
{
    Blank3DActorEquipmentInstance *instance;
    if (equipment == 0 || !equipment->initialized) return;
    instance = blank3d_actor_equipment_find(equipment, actor_id);
    if (instance == 0 || !instance->attached) return;
    if (weapon_id > 0 && instance->weapon_id != weapon_id) return;
    blank3d_mechanical_weapon_trigger_fire(&instance->animator);
}

int blank3d_actor_equipment_packet_count(
    const Blank3DActorEquipmentSystem *equipment,
    int actor_id)
{
    const Blank3DActorEquipmentInstance *instance;
    instance = blank3d_actor_equipment_find_const(equipment, actor_id);
    if (instance == 0 || !instance->visible) return 0;
    return blank3d_mechanical_weapon_packet_count(&instance->animator);
}

const nm89_geometry_packet *blank3d_actor_equipment_packet(
    const Blank3DActorEquipmentSystem *equipment,
    int actor_id,
    int packet_index)
{
    const Blank3DActorEquipmentInstance *instance;
    instance = blank3d_actor_equipment_find_const(equipment, actor_id);
    if (instance == 0 || !instance->visible) return 0;
    return blank3d_mechanical_weapon_packet(&instance->animator,
                                             packet_index);
}

const nm89_matrix *blank3d_actor_equipment_muzzle_world(
    const Blank3DActorEquipmentSystem *equipment,
    int actor_id)
{
    const Blank3DActorEquipmentInstance *instance;
    instance = blank3d_actor_equipment_find_const(equipment, actor_id);
    if (instance == 0 || !instance->visible) return 0;
    return blank3d_mechanical_weapon_muzzle_world(&instance->animator);
}

int blank3d_actor_equipment_model_id(
    const Blank3DActorEquipmentSystem *equipment,
    int actor_id)
{
    const Blank3DActorEquipmentInstance *instance;
    instance = blank3d_actor_equipment_find_const(equipment, actor_id);
    return instance != 0 ? instance->model_id : 0;
}

const char *blank3d_actor_equipment_model_name(
    const Blank3DActorEquipmentSystem *equipment,
    int actor_id)
{
    const Blank3DActorEquipmentInstance *instance;
    instance = blank3d_actor_equipment_find_const(equipment, actor_id);
    return instance != 0 ? instance->model_name : "";
}

int blank3d_actor_equipment_object_id(
    const Blank3DActorEquipmentSystem *equipment,
    int actor_id)
{
    const Blank3DActorEquipmentInstance *instance;
    instance = blank3d_actor_equipment_find_const(equipment, actor_id);
    return instance != 0 ? instance->object_id : GATTACH89_INVALID_ID;
}

const char *blank3d_actor_equipment_status(
    const Blank3DActorEquipmentSystem *equipment)
{
    if (equipment == 0) return "actor equipment: null";
    return equipment->status;
}
