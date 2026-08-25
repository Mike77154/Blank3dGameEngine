#ifndef BLANK3D_GLOCO_PROFILE_INI_H
#define BLANK3D_GLOCO_PROFILE_INI_H

#include "gloco89.h"

#ifdef __cplusplus
extern "C" {
#endif

int blank3d_gloco_profile_load_ini(GLOCO_Profile *profile,
                                    const char *path,
                                    int fallback_preset,
                                    char *status,
                                    unsigned int status_cap);

#ifdef __cplusplus
}
#endif

#endif
