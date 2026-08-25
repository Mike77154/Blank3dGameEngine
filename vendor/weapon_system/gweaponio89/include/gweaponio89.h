#ifndef GWEAPONIO89_H
#define GWEAPONIO89_H

#include "gweapon89.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * IO is a host service, never a Weapon-System backend.
 * The helper below forwards a whole-text read request through the existing
 * GWeapon89 provider bus (GWP89_SERVICE_IO / GWP89_OP_READ_TEXT_FILE).
 */
int gweaponio89_read_text(GWP89_Manager *manager,
                          const char *path,
                          char *buffer,
                          int capacity,
                          int *length_out);

#ifdef __cplusplus
}
#endif

#endif
