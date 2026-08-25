#ifndef GLOCO89_PROFILES_H
#define GLOCO89_PROFILES_H

#include "gloco89.h"

#ifdef __cplusplus
extern "C" {
#endif

void gloco_load_default_profile_bank(GLOCO_Context *ctx);
void gloco_make_tactical_profile(GLOCO_Profile *p);
void gloco_make_arcade_profile(GLOCO_Profile *p);
void gloco_make_heavy_profile(GLOCO_Profile *p);

#ifdef __cplusplus
}
#endif

#endif
