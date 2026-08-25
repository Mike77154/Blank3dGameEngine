#ifndef BLANK3D_GPROJ_AMMO_GBAR_H
#define BLANK3D_GPROJ_AMMO_GBAR_H

#include "gbar89.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Blank3DGProjAmmoGBarTag {
    int ammo_id;
    int enabled;
} Blank3DGProjAmmoGBar;

void blank3d_gproj_ammo_gbar_init(Blank3DGProjAmmoGBar *bridge);
void blank3d_gproj_ammo_gbar_set_ammo(Blank3DGProjAmmoGBar *bridge,
                                      int weapon_ammo_id);
void blank3d_gproj_ammo_gbar_attach(Blank3DGProjAmmoGBar *bridge,
                                    GBar89_Meter *meter);
void blank3d_gproj_ammo_gbar_render(void *user,
                                    const GBar89_RenderOps *ops,
                                    const GBar89_Rect *slot,
                                    unsigned long fill_rgba,
                                    unsigned long outline_rgba,
                                    int scale_percent);
int blank3d_gproj_ammo_gbar_profile(int weapon_ammo_id);

#ifdef __cplusplus
}
#endif

#endif
