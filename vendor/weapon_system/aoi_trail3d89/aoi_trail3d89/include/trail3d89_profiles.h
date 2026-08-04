#ifndef TRAIL3D89_PROFILES_H
#define TRAIL3D89_PROFILES_H

#include "trail3d89.h"

#ifdef __cplusplus
extern "C" {
#endif

void t3d89_profile_bullet(t3d89_desc *desc);
void t3d89_profile_tracer(t3d89_desc *desc);
void t3d89_profile_dash_ground(t3d89_desc *desc);
void t3d89_profile_dash_air(t3d89_desc *desc);
void t3d89_profile_casing(t3d89_desc *desc);
void t3d89_profile_beam(t3d89_desc *desc);
void t3d89_profile_socket_sword(t3d89_desc *desc);
void t3d89_profile_claw(t3d89_desc *desc);
void t3d89_profile_energy_cross(t3d89_desc *desc);
void t3d89_profile_debug_path(t3d89_desc *desc);

#ifdef __cplusplus
}
#endif

#endif
