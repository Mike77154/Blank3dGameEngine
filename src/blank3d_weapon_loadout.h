#ifndef BLANK3D_WEAPON_LOADOUT_H
#define BLANK3D_WEAPON_LOADOUT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_WLOAD_MAX_WEAPONS 16
#define B3D_WLOAD_NAME_CAP 64
#define B3D_WLOAD_PATH_CAP 256

typedef struct Blank3DWeaponCatalogEntryTag {
    int used;
    int weapon_id;
    int ammo_id;
    int max_owned;
    int ammo_capacity;
    char name[B3D_WLOAD_NAME_CAP];
    char ammo_name[B3D_WLOAD_NAME_CAP];
    char profile[B3D_WLOAD_PATH_CAP];
} Blank3DWeaponCatalogEntry;

typedef struct Blank3DWeaponCatalogTag {
    Blank3DWeaponCatalogEntry entries[B3D_WLOAD_MAX_WEAPONS];
    int count;
} Blank3DWeaponCatalog;

typedef struct Blank3DPlayerWeaponLoadoutTag {
    int weapon_amount[B3D_WLOAD_MAX_WEAPONS + 1];
    int ammo_amount[B3D_WLOAD_MAX_WEAPONS + 1];
    int equipped_weapon_id;
} Blank3DPlayerWeaponLoadout;

void blank3d_weapon_catalog_defaults(Blank3DWeaponCatalog *catalog);
void blank3d_player_weapon_loadout_defaults(Blank3DPlayerWeaponLoadout *loadout);
int blank3d_weapon_catalog_load(Blank3DWeaponCatalog *catalog,
                                const char *path,
                                char *status,
                                size_t status_capacity);
int blank3d_player_weapon_loadout_load(Blank3DPlayerWeaponLoadout *loadout,
                                       const char *path,
                                       char *status,
                                       size_t status_capacity);
const Blank3DWeaponCatalogEntry *blank3d_weapon_catalog_find_id(
    const Blank3DWeaponCatalog *catalog, int weapon_id);

#ifdef __cplusplus
}
#endif

#endif
