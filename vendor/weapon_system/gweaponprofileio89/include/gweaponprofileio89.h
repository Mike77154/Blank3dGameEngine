#ifndef GWEAPONPROFILEIO89_H
#define GWEAPONPROFILEIO89_H

#include <stddef.h>
#include "gweapon89.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Parse and register weapon profiles through the host-supplied IO provider.
   This module understands Weapon-System data, but owns no filesystem backend. */
int gweaponprofileio89_load_manifest(GWP89_Manager *manager,
                                     const char *manifest_path,
                                     char *status,
                                     size_t status_capacity);

#ifdef __cplusplus
}
#endif

#endif
