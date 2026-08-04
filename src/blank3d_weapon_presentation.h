#ifndef BLANK3D_WEAPON_PRESENTATION_H
#define BLANK3D_WEAPON_PRESENTATION_H

#include <stddef.h>

#include "gweapon89.h"
#include "../vendor/gattach89/include/gattach89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_WPRES_MAX_PROFILES 16
#define B3D_WPRES_NAME_CAP 64
#define B3D_WPRES_PATH_CAP 256

#define B3D_MECHANISM_FIXED 0
#define B3D_MECHANISM_SLIDE_MAGAZINE 1
#define B3D_MECHANISM_BOLT_MAGAZINE 2
#define B3D_MECHANISM_PUMP_TUBE 3
#define B3D_MECHANISM_REVOLVER 4
#define B3D_MECHANISM_GATLING_BELT 5
#define B3D_MECHANISM_ELASTIC 6

#define B3D_WEAPON_MODEL_GENERIC 1

typedef struct Blank3DWeaponPresentationTag {
    int used;
    int weapon_id;
    int model_id;
    int mechanism_kind;
    int detachable_magazine;
    unsigned short fire_ticks;
    char weapon_name[B3D_WPRES_NAME_CAP];
    char model_name[B3D_WPRES_NAME_CAP];
    char socket_name[GATTACH89_NAME_LEN];
    char attachment_name[GATTACH89_NAME_LEN];
    GAtt89_Xform grip_offset;

    gatt_fix recoil_z;
    gatt_fix recoil_pitch;
    gatt_fix action_home_z;
    gatt_fix action_fire_z;
    gatt_fix feed_home_y;
    gatt_fix feed_home_z;
    gatt_fix feed_out_y;
    gatt_fix feed_out_z;
    gatt_fix barrel_home_z;
    gatt_fix muzzle_y;
    gatt_fix muzzle_z;
} Blank3DWeaponPresentation;

typedef struct Blank3DWeaponPresentationRegistryTag {
    Blank3DWeaponPresentation profiles[B3D_WPRES_MAX_PROFILES];
    int count;
    char status[160];
} Blank3DWeaponPresentationRegistry;

void blank3d_weapon_presentation_registry_init(
    Blank3DWeaponPresentationRegistry *registry);
void blank3d_weapon_presentation_defaults(
    Blank3DWeaponPresentation *profile,
    int weapon_id,
    const char *weapon_name);
int blank3d_weapon_presentation_load_manifest(
    Blank3DWeaponPresentationRegistry *registry,
    const char *manifest_path,
    char *status,
    size_t status_capacity);
const Blank3DWeaponPresentation *blank3d_weapon_presentation_find(
    const Blank3DWeaponPresentationRegistry *registry,
    int weapon_id);
int blank3d_weapon_presentation_mechanism_from_text(const char *text);
const char *blank3d_weapon_presentation_mechanism_name(int kind);

#ifdef __cplusplus
}
#endif

#endif
