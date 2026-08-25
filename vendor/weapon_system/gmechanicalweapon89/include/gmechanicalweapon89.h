#ifndef GMECHANICALWEAPON89_H
#define GMECHANICALWEAPON89_H

#include "gattach89.h"
#include "nationalmecanicanimal89.h"
#include "gweaponpresentation89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GMW89_MAX_PACKETS 8
#define GMW89_TICK_MS 16U

#define GMW89_MESH_BODY 0
#define GMW89_MESH_SLIDE 1
#define GMW89_MESH_MAGAZINE 2
#define GMW89_MESH_BARREL 3
#define GMW89_MESH_COUNT 4

#define GMW89_PART_ROOT 1
#define GMW89_PART_RECOIL 2
#define GMW89_PART_BODY 3
#define GMW89_PART_SLIDE 4
#define GMW89_PART_MAGAZINE 5
#define GMW89_PART_BARREL 6
#define GMW89_PART_MUZZLE_SOCKET 7

#define GMW89_ACTION_FIRE 1
#define GMW89_EVENT_SLIDE_REAR 1

typedef struct GMechanicalWeapon89Tag {
    nm89_rig rig;
    nm89_provider provider;
    nm89_i16 provider_id;
    nm89_i16 root_part;
    nm89_i16 recoil_part;
    nm89_i16 body_part;
    nm89_i16 slide_part;
    nm89_i16 magazine_part;
    nm89_i16 barrel_part;
    nm89_i16 muzzle_socket_part;
    nm89_i16 fire_clip;

    GWeaponPresentation89 presentation;
    nm89_matrix attachment_world;
    int attachment_valid;
    int current_weapon_id;
    int reload_active;
    unsigned short reload_elapsed_ms;
    unsigned short reload_total_ms;
    unsigned int tick_remainder_ms;

    nm89_geometry_packet packets[GMW89_MAX_PACKETS];
    int packet_count;
    int emitted_event_count;
    int initialized;
    int last_result;
} GMechanicalWeapon89;

int gmechanicalweapon89_init(GMechanicalWeapon89 *weapon);
int gmechanicalweapon89_init_profile(
    GMechanicalWeapon89 *weapon,
    const GWeaponPresentation89 *presentation);
void gmechanicalweapon89_set_attachment(
    GMechanicalWeapon89 *weapon,
    const GAtt89_Xform *attachment_world);
void gmechanicalweapon89_set_weapon(
    GMechanicalWeapon89 *weapon,
    int weapon_id);
void gmechanicalweapon89_set_reload(
    GMechanicalWeapon89 *weapon,
    int active,
    unsigned short elapsed_ms,
    unsigned short total_ms);
void gmechanicalweapon89_trigger_fire(
    GMechanicalWeapon89 *weapon);
int gmechanicalweapon89_update(
    GMechanicalWeapon89 *weapon,
    unsigned short frame_ms);

int gmechanicalweapon89_packet_count(
    const GMechanicalWeapon89 *weapon);
const nm89_geometry_packet *gmechanicalweapon89_packet(
    const GMechanicalWeapon89 *weapon,
    int index);
const nm89_pose *gmechanicalweapon89_part_pose(
    const GMechanicalWeapon89 *weapon,
    int part_tag);
const nm89_pose *gmechanicalweapon89_muzzle_socket(
    const GMechanicalWeapon89 *weapon);
const nm89_matrix *gmechanicalweapon89_muzzle_world(
    const GMechanicalWeapon89 *weapon);
const char *gmechanicalweapon89_status(
    const GMechanicalWeapon89 *weapon);

#ifdef __cplusplus
}
#endif

#endif
