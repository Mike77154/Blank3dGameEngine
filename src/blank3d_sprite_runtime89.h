#ifndef BLANK3D_SPRITE_RUNTIME89_H
#define BLANK3D_SPRITE_RUNTIME89_H

#include "blank3d_image_assets.h"
#include "spriteasset89.h"
#include "spriteverbs89.h"
#include "spriteverbs89_assetroute.h"
#include "assetroute89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_SPRITE89_IMPORT_CAP 65536U
#define B3D_SPRITE89_STATUS_CAP 192U

typedef int (*Blank3DSprite89DrawFn)(void *user,
                                      int image_id,
                                      int sx, int sy, int sw, int sh,
                                      int dx, int dy,
                                      unsigned int flags);

typedef struct Blank3DSprite89RenderHostTag {
    void *user;
    Blank3DSprite89DrawFn draw;
} Blank3DSprite89RenderHost;

typedef struct Blank3DSpriteRuntime89Tag {
    AssetRoute89 routes;
    AR89_FileProvider file_provider;
    SpriteAsset89 assets;
    SpriteVerbs89 verbs;
    SV89_AssetRouteAdapter lazy_route;
    Blank3DImageAssets *images;
    Blank3DSprite89RenderHost render_host;
    int current_image_id;
    int initialized;
    char status[B3D_SPRITE89_STATUS_CAP];
} Blank3DSpriteRuntime89;

void blank3d_sprite_runtime89_init(Blank3DSpriteRuntime89 *runtime,
                                    Blank3DImageAssets *images);
void blank3d_sprite_runtime89_set_render_host(
    Blank3DSpriteRuntime89 *runtime,
    const Blank3DSprite89RenderHost *host);
int blank3d_sprite_runtime89_add_root(Blank3DSpriteRuntime89 *runtime,
                                      int kind, const char *path,
                                      int recursive);
int blank3d_sprite_runtime89_scan(Blank3DSpriteRuntime89 *runtime);
int blank3d_sprite_runtime89_alias(Blank3DSpriteRuntime89 *runtime,
                                   int kind, const char *name,
                                   const char *path);
int blank3d_sprite_runtime89_resolve(Blank3DSpriteRuntime89 *runtime,
                                     int kind, const char *request,
                                     char *out_path, unsigned int out_cap);
int blank3d_sprite_runtime89_define_static(Blank3DSpriteRuntime89 *runtime,
                                           const char *asset_name,
                                           const char *request,
                                           unsigned int duration_ms);
int blank3d_sprite_runtime89_define_grid(Blank3DSpriteRuntime89 *runtime,
                                         const char *asset_name,
                                         const char *clip_name,
                                         const char *request,
                                         int frame_w, int frame_h,
                                         unsigned int frame_count,
                                         unsigned int columns,
                                         unsigned int duration_ms,
                                         int loop_mode);
int blank3d_sprite_runtime89_import_renlist_file(
    Blank3DSpriteRuntime89 *runtime, const char *path,
    sa89_id *out_asset_id);
int blank3d_sprite_runtime89_import_aseprite_file(
    Blank3DSpriteRuntime89 *runtime, const char *asset_name,
    const char *json_path, const char *sheet_override,
    sa89_id *out_asset_id);
int blank3d_sprite_runtime89_execute(Blank3DSpriteRuntime89 *runtime,
                                     const char *target,
                                     const char *verb,
                                     const char *argument);
int blank3d_sprite_runtime89_execute_ddsl(Blank3DSpriteRuntime89 *runtime,
                                          const char *verb,
                                          const char *value_text);
void blank3d_sprite_runtime89_step(Blank3DSpriteRuntime89 *runtime,
                                   unsigned int delta_ms);
int blank3d_sprite_runtime89_render(Blank3DSpriteRuntime89 *runtime);
int blank3d_sprite_runtime89_loop_mode(const char *text);
int blank3d_sprite_runtime89_kind(const char *text);
const char *blank3d_sprite_runtime89_status(
    const Blank3DSpriteRuntime89 *runtime);

#ifdef __cplusplus
}
#endif

#endif
