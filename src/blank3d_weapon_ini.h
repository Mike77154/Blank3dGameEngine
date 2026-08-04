#ifndef BLANK3D_WEAPON_INI_H
#define BLANK3D_WEAPON_INI_H

#include <stddef.h>
#include "gweapon89.h"

#ifdef __cplusplus
extern "C" {
#endif

int blank3d_weapon_ini_load_manifest(GWP89_Manager *manager,
                                     const char *manifest_path,
                                     char *status,
                                     size_t status_capacity);

#ifdef __cplusplus
}
#endif

#endif
