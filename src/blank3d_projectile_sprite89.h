#ifndef BLANK3D_PROJECTILE_SPRITE89_H
#define BLANK3D_PROJECTILE_SPRITE89_H

#include "blank3d_weapon_modules.h"
#include "blank3d_image_assets.h"
#include "assetroute89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_PSPR89_CACHE_CAP B3D_WEAPON_MODULE_CAPACITY
#define B3D_PSPR89_FRAME_CAP 128
#define B3D_PSPR89_REQUEST_CAP 260
#define B3D_PSPR89_STATUS_CAP 160
#define B3D_PSPR89_FLIP_X 0x00000001U
#define B3D_PSPR89_FLIP_Y 0x00000002U

typedef struct Blank3DProjectileSpriteFrame89Tag {
    char image_request[B3D_PSPR89_REQUEST_CAP];
    int source_enabled;
    int source_x;
    int source_y;
    int source_w;
    int source_h;
    long scale_x_q16;
    long scale_y_q16;
    unsigned int duration_ms;
    unsigned int flags;
} Blank3DProjectileSpriteFrame89;

typedef struct Blank3DProjectileSpriteClip89Tag {
    int used;
    int valid;
    int weapon_id;
    int source_mode;
    int loop_mode;
    unsigned short frame_count;
    unsigned int total_ms;
    char status[B3D_PSPR89_STATUS_CAP];
    Blank3DProjectileSpriteFrame89 frames[B3D_PSPR89_FRAME_CAP];
} Blank3DProjectileSpriteClip89;

typedef struct Blank3DProjectileSpriteRuntime89Tag {
    Blank3DProjectileSpriteClip89 clips[B3D_PSPR89_CACHE_CAP];
    Blank3DImageAssets *images;
    AssetRoute89 *routes;
} Blank3DProjectileSpriteRuntime89;

typedef struct Blank3DProjectileSpriteSample89Tag {
    int valid;
    int image_id;
    int source_enabled;
    int source_x;
    int source_y;
    int source_w;
    int source_h;
    long scale_x_q16;
    long scale_y_q16;
    unsigned int frame_index;
    unsigned int flags;
} Blank3DProjectileSpriteSample89;

void blank3d_projectile_sprite89_init(Blank3DProjectileSpriteRuntime89 *runtime,
                                      Blank3DImageAssets *images,
                                      AssetRoute89 *routes);
void blank3d_projectile_sprite89_reset(Blank3DProjectileSpriteRuntime89 *runtime);
int blank3d_projectile_sprite89_prepare(Blank3DProjectileSpriteRuntime89 *runtime,
                                        const Blank3DWeaponModules *modules);
int blank3d_projectile_sprite89_sample(Blank3DProjectileSpriteRuntime89 *runtime,
                                       const Blank3DWeaponModules *modules,
                                       unsigned int age_ms,
                                       int projectile_slot,
                                       Blank3DProjectileSpriteSample89 *out_sample);
const char *blank3d_projectile_sprite89_status(
    const Blank3DProjectileSpriteRuntime89 *runtime, int weapon_id);

#ifdef __cplusplus
}
#endif

#endif
