#ifndef BLANK3D_MUZZLE_LIGHT_H
#define BLANK3D_MUZZLE_LIGHT_H

#include "gweaponmodules89.h"
#include "gweapon89.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Blank3DMuzzleLightTag {
    int active;
    int fresh;
    unsigned short life_ms;
    unsigned short total_ms;
    long x_q12;
    long y_q12;
    long z_q12;
    long peak_intensity_q16;
    long radius_q16;
    unsigned short r;
    unsigned short g;
    unsigned short b;
} Blank3DMuzzleLight;

typedef struct Blank3DMuzzleLightSampleTag {
    int active;
    long x_q12;
    long y_q12;
    long z_q12;
    long intensity_q16;
    long radius_q16;
    unsigned short r;
    unsigned short g;
    unsigned short b;
} Blank3DMuzzleLightSample;

void blank3d_muzzle_light_init(Blank3DMuzzleLight *light);
int blank3d_muzzle_light_emit(Blank3DMuzzleLight *light,
                              const GWeaponModules89 *modules,
                              const GWP89_Event *event);
void blank3d_muzzle_light_update(Blank3DMuzzleLight *light,
                                 unsigned int dt_ms);
int blank3d_muzzle_light_sample(const Blank3DMuzzleLight *light,
                                Blank3DMuzzleLightSample *sample);

#ifdef __cplusplus
}
#endif

#endif
