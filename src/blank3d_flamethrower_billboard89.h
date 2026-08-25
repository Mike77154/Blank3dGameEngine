#ifndef BLANK3D_FLAMETHROWER_BILLBOARD89_H
#define BLANK3D_FLAMETHROWER_BILLBOARD89_H

#include "../vendor/gamlib3d/gamlib3d_transform.h"
#include "blank3d_weapon_modules.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Blank3DFlameBillboardSampleTag {
    unsigned short frame_index;
    unsigned short atlas_column;
    unsigned short atlas_row;
    g3d_fix width_q12;
    g3d_fix height_q12;
    g3d_fix hot_scale_q12;
    g3d_fix glow_scale_q12;
    unsigned char core_r;
    unsigned char core_g;
    unsigned char core_b;
    unsigned char core_a;
    unsigned char hot_a;
    unsigned char glow_a;
    unsigned char flip_x;
} Blank3DFlameBillboardSample;

/* Generic recipe-driven projectile billboard sampler.  The historical file
   name is retained for source compatibility, but no asset path, atlas size,
   cadence, scale or effect intensity is compiled into this module. */
void blank3d_projectile_billboard_sample(
    const Blank3DWeaponModules *modules,
    unsigned int age_ms,
    unsigned int life_ms,
    int projectile_slot,
    g3d_fix mesh_scale_q12,
    Blank3DFlameBillboardSample *out_sample);

#ifdef __cplusplus
}
#endif

#endif
