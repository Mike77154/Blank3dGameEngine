#ifndef GWEAPONLOADOUT89_H
#define GWEAPONLOADOUT89_H

#include <stddef.h>
#include "gweapon89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GWLOAD89_MAX_WEAPONS 16
#define GWLOAD89_NAME_CAP 64
#define GWLOAD89_PATH_CAP 256
#define GWLOAD89_TEXT_CAP 16384

typedef struct GWeaponCatalogEntry89Tag {
    int used;
    int weapon_id;
    int ammo_id;
    int max_owned;
    int ammo_capacity;
    char name[GWLOAD89_NAME_CAP];
    char ammo_name[GWLOAD89_NAME_CAP];
    char profile[GWLOAD89_PATH_CAP];
} GWeaponCatalogEntry89;

typedef struct GWeaponCatalog89Tag {
    GWeaponCatalogEntry89 entries[GWLOAD89_MAX_WEAPONS];
    int count;
} GWeaponCatalog89;

typedef struct GWeaponLoadout89Tag {
    int weapon_amount[GWLOAD89_MAX_WEAPONS + 1];
    int ammo_amount[GWLOAD89_MAX_WEAPONS + 1];
    int equipped_weapon_id;
} GWeaponLoadout89;

void gweaponloadout89_catalog_defaults(GWeaponCatalog89 *catalog);
void gweaponloadout89_defaults(GWeaponLoadout89 *loadout);

int gweaponloadout89_catalog_parse_text(GWeaponCatalog89 *catalog,
                                         const char *text,
                                         char *status,
                                         size_t status_capacity);
int gweaponloadout89_parse_text(GWeaponLoadout89 *loadout,
                                 const char *text,
                                 char *status,
                                 size_t status_capacity);

/* File access is delegated to GWP89_SERVICE_IO. */
int gweaponloadout89_catalog_load(GWP89_Manager *manager,
                                  GWeaponCatalog89 *catalog,
                                  const char *path,
                                  char *status,
                                  size_t status_capacity);
int gweaponloadout89_load(GWP89_Manager *manager,
                          GWeaponLoadout89 *loadout,
                          const char *path,
                          char *status,
                          size_t status_capacity);

const GWeaponCatalogEntry89 *gweaponloadout89_catalog_find_id(
    const GWeaponCatalog89 *catalog, int weapon_id);

#ifdef __cplusplus
}
#endif

#endif
