#ifndef BLANK3D_WEAPON_HOST_IO_H
#define BLANK3D_WEAPON_HOST_IO_H

#include "gweapon89.h"

#ifdef __cplusplus
extern "C" {
#endif

int blank3d_weapon_host_io_provider(void *context,
                                    GWP89_ProviderPacket *packet);
int blank3d_weapon_host_io_bind(GWP89_Manager *manager);

#ifdef __cplusplus
}
#endif

#endif
