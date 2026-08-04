#ifndef BLANK3D_ACTOR_EQUIPMENT_H
#define BLANK3D_ACTOR_EQUIPMENT_H

#include "blank3d_attachment.h"
#include "blank3d_mechanical_weapon.h"
#include "blank3d_weapon_presentation.h"
#include "gweapon89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_EQUIPMENT_MAX_ACTORS 40
#define B3D_EQUIPMENT_OBJECT_BASE 20000

#define B3D_EQUIPMENT_ACTOR_GENERIC 0
#define B3D_EQUIPMENT_ACTOR_PLAYER  1
#define B3D_EQUIPMENT_ACTOR_ENEMY   2
#define B3D_EQUIPMENT_ACTOR_ALLY    3

typedef struct Blank3DActorEquipmentInstanceTag {
    int used;
    int actor_id;
    int actor_kind;
    int object_id;
    int weapon_id;
    int model_id;
    int attached;
    int visible;
    int last_result;
    char socket_name[GATTACH89_NAME_LEN];
    char attachment_name[GATTACH89_NAME_LEN];
    char model_name[B3D_WPRES_NAME_CAP];
    Blank3DMechanicalWeapon animator;
} Blank3DActorEquipmentInstance;

typedef struct Blank3DActorEquipmentSystemTag {
    Blank3DAttachmentWorld *attachments;
    const Blank3DWeaponPresentationRegistry *presentations;
    Blank3DActorEquipmentInstance instances[B3D_EQUIPMENT_MAX_ACTORS];
    int count;
    int initialized;
    int last_result;
    char status[192];
} Blank3DActorEquipmentSystem;

void blank3d_actor_equipment_init(
    Blank3DActorEquipmentSystem *equipment,
    Blank3DAttachmentWorld *attachments,
    const Blank3DWeaponPresentationRegistry *presentations);
int blank3d_actor_equipment_define_socket(
    Blank3DActorEquipmentSystem *equipment,
    int actor_id,
    const char *socket_name,
    int socket_index,
    const GAtt89_Xform *local_offset);
int blank3d_actor_equipment_equip(
    Blank3DActorEquipmentSystem *equipment,
    int actor_id,
    int actor_kind,
    int weapon_id);
int blank3d_actor_equipment_unequip(
    Blank3DActorEquipmentSystem *equipment,
    int actor_id);
int blank3d_actor_equipment_set_visible(
    Blank3DActorEquipmentSystem *equipment,
    int actor_id,
    int visible);
int blank3d_actor_equipment_sync_actor(
    Blank3DActorEquipmentSystem *equipment,
    GWP89_Manager *weapon_manager,
    int actor_id,
    int actor_kind,
    unsigned short frame_ms);
void blank3d_actor_equipment_trigger_fire(
    Blank3DActorEquipmentSystem *equipment,
    int actor_id,
    int weapon_id);
Blank3DActorEquipmentInstance *blank3d_actor_equipment_find(
    Blank3DActorEquipmentSystem *equipment,
    int actor_id);
const Blank3DActorEquipmentInstance *blank3d_actor_equipment_find_const(
    const Blank3DActorEquipmentSystem *equipment,
    int actor_id);
int blank3d_actor_equipment_packet_count(
    const Blank3DActorEquipmentSystem *equipment,
    int actor_id);
const nm89_geometry_packet *blank3d_actor_equipment_packet(
    const Blank3DActorEquipmentSystem *equipment,
    int actor_id,
    int packet_index);
const nm89_matrix *blank3d_actor_equipment_muzzle_world(
    const Blank3DActorEquipmentSystem *equipment,
    int actor_id);
int blank3d_actor_equipment_model_id(
    const Blank3DActorEquipmentSystem *equipment,
    int actor_id);
const char *blank3d_actor_equipment_model_name(
    const Blank3DActorEquipmentSystem *equipment,
    int actor_id);
int blank3d_actor_equipment_object_id(
    const Blank3DActorEquipmentSystem *equipment,
    int actor_id);
const char *blank3d_actor_equipment_status(
    const Blank3DActorEquipmentSystem *equipment);

#ifdef __cplusplus
}
#endif

#endif
