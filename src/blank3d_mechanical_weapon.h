#ifndef BLANK3D_MECHANICAL_WEAPON_H
#define BLANK3D_MECHANICAL_WEAPON_H

#include "../vendor/gattach89/include/gattach89.h"
#include "../vendor/nationalmecanicanimal89/include/nationalmecanicanimal89.h"
#include "blank3d_weapon_presentation.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_MECH89_MAX_PACKETS 8
#define B3D_MECH89_TICK_MS 16U

#define B3D_MECH89_MESH_BODY 0
#define B3D_MECH89_MESH_SLIDE 1
#define B3D_MECH89_MESH_MAGAZINE 2
#define B3D_MECH89_MESH_BARREL 3
#define B3D_MECH89_MESH_COUNT 4

#define B3D_MECH89_PART_ROOT 1
#define B3D_MECH89_PART_RECOIL 2
#define B3D_MECH89_PART_BODY 3
#define B3D_MECH89_PART_SLIDE 4
#define B3D_MECH89_PART_MAGAZINE 5
#define B3D_MECH89_PART_BARREL 6
#define B3D_MECH89_PART_MUZZLE_SOCKET 7

#define B3D_MECH89_ACTION_FIRE 1
#define B3D_MECH89_EVENT_SLIDE_REAR 1

typedef struct Blank3DMechanicalWeaponTag {
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

    Blank3DWeaponPresentation presentation;
    nm89_matrix attachment_world;
    int attachment_valid;
    int current_weapon_id;
    int reload_active;
    unsigned short reload_elapsed_ms;
    unsigned short reload_total_ms;
    unsigned int tick_remainder_ms;

    nm89_geometry_packet packets[B3D_MECH89_MAX_PACKETS];
    int packet_count;
    int emitted_event_count;
    int initialized;
    int last_result;
} Blank3DMechanicalWeapon;

int blank3d_mechanical_weapon_init(Blank3DMechanicalWeapon *weapon);
int blank3d_mechanical_weapon_init_profile(
    Blank3DMechanicalWeapon *weapon,
    const Blank3DWeaponPresentation *presentation);
void blank3d_mechanical_weapon_set_attachment(
    Blank3DMechanicalWeapon *weapon,
    const GAtt89_Xform *attachment_world);
void blank3d_mechanical_weapon_set_weapon(
    Blank3DMechanicalWeapon *weapon,
    int weapon_id);
void blank3d_mechanical_weapon_set_reload(
    Blank3DMechanicalWeapon *weapon,
    int active,
    unsigned short elapsed_ms,
    unsigned short total_ms);
void blank3d_mechanical_weapon_trigger_fire(
    Blank3DMechanicalWeapon *weapon);
int blank3d_mechanical_weapon_update(
    Blank3DMechanicalWeapon *weapon,
    unsigned short frame_ms);

int blank3d_mechanical_weapon_packet_count(
    const Blank3DMechanicalWeapon *weapon);
const nm89_geometry_packet *blank3d_mechanical_weapon_packet(
    const Blank3DMechanicalWeapon *weapon,
    int index);
const nm89_pose *blank3d_mechanical_weapon_part_pose(
    const Blank3DMechanicalWeapon *weapon,
    int part_tag);
const nm89_pose *blank3d_mechanical_weapon_muzzle_socket(
    const Blank3DMechanicalWeapon *weapon);
const nm89_matrix *blank3d_mechanical_weapon_muzzle_world(
    const Blank3DMechanicalWeapon *weapon);
const char *blank3d_mechanical_weapon_status(
    const Blank3DMechanicalWeapon *weapon);

#ifdef __cplusplus
}
#endif

#endif
