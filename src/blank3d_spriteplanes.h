#ifndef BLANK3D_SPRITEPLANES_H
#define BLANK3D_SPRITEPLANES_H

#include "blank3d_image_assets.h"
#include "spriteplane89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_SPRITEPLANE_PERSISTENT 0UL

typedef struct Blank3DSpritePlaneWorldTag {
    sprpl89_ctx ctx;
    unsigned long life_ms[SP89_MAX_SPRITES];
    Blank3DImageAssets *images;
} Blank3DSpritePlaneWorld;

typedef struct Blank3DSpritePlaneSpecTag {
    int image_id;
    long x_q16, y_q16, z_q16;
    long width_q16, height_q16;
    int billboard_mode;
    int blend_mode;
    int depth_test;
    unsigned long life_ms;
    unsigned long tint_rgba;
    int plane_axis; /* 0=XY, 1=XZ, 2=YZ for fixed/surface modes */
} Blank3DSpritePlaneSpec;

void blank3d_spriteplanes_init(Blank3DSpritePlaneWorld *world,
                               Blank3DImageAssets *images);
void blank3d_spriteplanes_clear(Blank3DSpritePlaneWorld *world);
int blank3d_spriteplanes_spawn(Blank3DSpritePlaneWorld *world,
                               const Blank3DSpritePlaneSpec *spec);
int blank3d_spriteplanes_spawn_path(Blank3DSpritePlaneWorld *world,
                                    const char *path,
                                    const Blank3DSpritePlaneSpec *spec);
void blank3d_spriteplanes_update(Blank3DSpritePlaneWorld *world,
                                 unsigned int dt_ms);
int blank3d_spriteplanes_emit(Blank3DSpritePlaneWorld *world,
                              const sprpl89_camera *camera,
                              sprpl89_emit *out);
int blank3d_spriteplanes_mode_from_text(const char *text);
int blank3d_spriteplanes_axis_from_text(const char *text);

#ifdef __cplusplus
}
#endif

#endif
