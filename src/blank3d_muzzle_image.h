#ifndef BLANK3D_MUZZLE_IMAGE_H
#define BLANK3D_MUZZLE_IMAGE_H

#include "blank3d_spriteplanes.h"
#include "gweaponmodules89.h"
#include "gweapon89.h"

#ifdef __cplusplus
extern "C" {
#endif

int blank3d_muzzle_image_emit(Blank3DSpritePlaneWorld *planes,
                              const GWeaponModules89 *modules,
                              const GWP89_Event *event);

#ifdef __cplusplus
}
#endif

#endif
