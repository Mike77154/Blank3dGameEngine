#ifndef GWEAPONPRESENTATION89_H
#define GWEAPONPRESENTATION89_H

#include <stddef.h>

#include "gweapon89.h"
#include "gattach89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GWPRES89_MAX_PROFILES 16
#define GWPRES89_NAME_CAP 64
#define GWPRES89_PATH_CAP 256
#define GWPRES89_TEXT_CAP 16384

#define GWPRES89_MECHANISM_FIXED 0
#define GWPRES89_MECHANISM_SLIDE_MAGAZINE 1
#define GWPRES89_MECHANISM_BOLT_MAGAZINE 2
#define GWPRES89_MECHANISM_PUMP_TUBE 3
#define GWPRES89_MECHANISM_REVOLVER 4
#define GWPRES89_MECHANISM_GATLING_BELT 5
#define GWPRES89_MECHANISM_ELASTIC 6

#define GWPRES89_MODEL_GENERIC 1

typedef struct GWeaponPresentation89Tag {
    int used;
    int weapon_id;
    int model_id;
    int mechanism_kind;
    int detachable_magazine;
    unsigned short fire_ticks;
    char weapon_name[GWPRES89_NAME_CAP];
    char model_name[GWPRES89_NAME_CAP];
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
} GWeaponPresentation89;

typedef struct GWeaponPresentationRegistry89Tag {
    GWeaponPresentation89 profiles[GWPRES89_MAX_PROFILES];
    int count;
    char status[160];
} GWeaponPresentationRegistry89;

void gweaponpresentation89_registry_init(
    GWeaponPresentationRegistry89 *registry);
void gweaponpresentation89_defaults(
    GWeaponPresentation89 *profile,
    int weapon_id,
    const char *weapon_name);
int gweaponpresentation89_load_manifest(
    GWP89_Manager *manager,
    GWeaponPresentationRegistry89 *registry,
    const char *manifest_path,
    char *status,
    size_t status_capacity);
const GWeaponPresentation89 *gweaponpresentation89_find(
    const GWeaponPresentationRegistry89 *registry,
    int weapon_id);
int gweaponpresentation89_mechanism_from_text(const char *text);
const char *gweaponpresentation89_mechanism_name(int kind);

#ifdef __cplusplus
}
#endif

#endif
