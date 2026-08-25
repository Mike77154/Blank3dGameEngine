#ifndef BLANK3D_IMAGE_GL_H
#define BLANK3D_IMAGE_GL_H

#include "blank3d_image_assets.h"
#include "blank3d_hud.h"
#include "spriteplane89.h"
#include "gamlib3d_transform.h"

#ifdef __cplusplus
extern "C" {
#endif

void blank3d_image_gl_make_backend(Blank3DImageBackend *backend);
void blank3d_image_gl_make_hud_provider(Blank3DHudSpriteProvider *provider,
                                        Blank3DImageAssets *assets);
void blank3d_image_gl_draw_spriteplanes(Blank3DImageAssets *assets,
                                        const sprpl89_emit *emit);
void blank3d_image_gl_begin_overlay(int width, int height);
void blank3d_image_gl_end_overlay(void);
int blank3d_image_gl_draw_subrect(Blank3DImageAssets *assets, int image_id,
                                  int sx, int sy, int sw, int sh,
                                  int dx, int dy, int flip_x, int flip_y);
int blank3d_image_gl_world_effect_begin(Blank3DImageAssets *assets,
                                        int image_id, int additive);
void blank3d_image_gl_world_quad_subrect(
    const Blank3DImageAsset *asset,
    int sx, int sy, int sw, int sh,
    const Vec3 corners[4], int flip_x, int flip_y,
    unsigned long tint_rgba);
void blank3d_image_gl_world_billboard_subrect(
    const Blank3DImageAsset *asset,
    int sx, int sy, int sw, int sh,
    const Vec3 *center, const Vec3 *camera_right, const Vec3 *camera_up,
    g3d_fix width_q12, g3d_fix height_q12,
    int flip_x, unsigned long tint_rgba);
void blank3d_image_gl_world_effect_end(void);

#ifdef __cplusplus
}
#endif

#endif
