#ifndef GCROSSHAIR_PROVIDER89_H
#define GCROSSHAIR_PROVIDER89_H

#include "gcrosshair_core89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GC89P_ABI_VERSION 2

#define GC89P_UNHANDLED 0
#define GC89P_HANDLED   1

/*
 * Provider bridge for gcrosshair89.
 *
 * The recipe/style layer never needs to know which renderer is active.
 * A host can provide a whole vector renderer, low-level primitives, HUD
 * coordinates/lifecycle, and/or image assets. Missing services fall back to
 * the lower layer. The original gcrosshair_core89 vector renderer remains the
 * built-in vector fallback.
 */

typedef struct GC89P_DrawMeta {
    int semantic_id;
    const char *semantic_name;
    const char *semantic_category;
} GC89P_DrawMeta;

typedef struct GC89P_VectorCommand {
    int screen_width;
    int screen_height;
    int center_x;
    int center_y;
    GC89_Fixed scale_fx;
    const GC89_DrawSpec *spec;
    const GC89P_DrawMeta *meta;
} GC89P_VectorCommand;

typedef int (*GC89P_DrawVectorFn)(void *user,
                                  const GC89P_VectorCommand *command,
                                  int *out_emitted_primitives);

typedef struct GC89P_VectorProvider {
    GC89P_DrawVectorFn draw_vector;
} GC89P_VectorProvider;

typedef int (*GC89P_DrawAssetFn)(void *user,
                                 int image_id,
                                 int x, int y,
                                 int width, int height,
                                 unsigned long tint_rgba);

typedef struct GC89P_AssetProvider {
    GC89P_DrawAssetFn draw_asset;
} GC89P_AssetProvider;

typedef int (*GC89P_HudViewportFn)(void *user,
                                   int *out_width,
                                   int *out_height);
typedef int (*GC89P_HudAnchorFn)(void *user,
                                 int screen_width,
                                 int screen_height,
                                 int *out_center_x,
                                 int *out_center_y);
typedef void (*GC89P_HudBeginFn)(void *user,
                                 int center_x,
                                 int center_y,
                                 const GC89_DrawSpec *spec);
typedef void (*GC89P_HudEndFn)(void *user,
                               int emitted_primitives);

typedef struct GC89P_HudProvider {
    GC89P_HudViewportFn get_viewport;
    GC89P_HudAnchorFn get_anchor;
    GC89P_HudBeginFn begin;
    GC89P_HudEndFn end;
} GC89P_HudProvider;

typedef struct GC89P_Runtime {
    GC89P_VectorProvider vector_provider;
    void *vector_user;

    GC89_DrawCallbacks primitive_provider;
    void *primitive_user;

    GC89_DrawCallbacks primitive_fallback;
    void *primitive_fallback_user;

    GC89P_AssetProvider asset_provider;
    void *asset_user;

    GC89P_HudProvider hud_provider;
    void *hud_user;

    int allow_internal_vector_fallback;
    int allow_primitive_fallback;
} GC89P_Runtime;

typedef struct GC89P_DrawReport {
    int center_x;
    int center_y;
    int emitted_primitives;
    int vector_provider_used;
    int internal_vector_used;
    int asset_provider_used;
    int primitive_image_used;
    int hud_provider_used;
} GC89P_DrawReport;

typedef struct GC89P_SoftwareSurface {
    unsigned char *rgba;
    int width;
    int height;
    int stride_bytes;
} GC89P_SoftwareSurface;

void gc89p_runtime_init(GC89P_Runtime *runtime);
void gc89p_runtime_set_vector_provider(GC89P_Runtime *runtime,
                                       const GC89P_VectorProvider *provider,
                                       void *user);
void gc89p_runtime_set_primitive_provider(GC89P_Runtime *runtime,
                                          const GC89_DrawCallbacks *provider,
                                          void *user);
void gc89p_runtime_set_primitive_fallback(GC89P_Runtime *runtime,
                                          const GC89_DrawCallbacks *provider,
                                          void *user);
void gc89p_runtime_set_asset_provider(GC89P_Runtime *runtime,
                                      const GC89P_AssetProvider *provider,
                                      void *user);
void gc89p_runtime_set_hud_provider(GC89P_Runtime *runtime,
                                    const GC89P_HudProvider *provider,
                                    void *user);
void gc89p_runtime_enable_internal_vector_fallback(GC89P_Runtime *runtime,
                                                    int enabled);
void gc89p_runtime_enable_primitive_fallback(GC89P_Runtime *runtime,
                                             int enabled);

/*
 * Route a resolved DrawSpec through the provider chain.
 *
 * Order:
 *   HUD viewport/anchor
 *   vector provider -> built-in gcrosshair_core89 vector fallback
 *   asset provider  -> primitive image callback
 *   HUD end
 */
int gc89p_draw_ex(const GC89P_Runtime *runtime,
                  const GC89_Core *core,
                  int fallback_screen_width,
                  int fallback_screen_height,
                  const GC89_DrawSpec *spec,
                  const GC89P_DrawMeta *meta,
                  GC89P_DrawReport *report);

int gc89p_draw(const GC89P_Runtime *runtime,
               const GC89_Core *core,
               int fallback_screen_width,
               int fallback_screen_height,
               const GC89_DrawSpec *spec,
               GC89P_DrawReport *report);

/* Caller-owned RGBA8888 target. No allocation is performed. */
void gc89p_surface_init(GC89P_SoftwareSurface *surface,
                        unsigned char *rgba,
                        int width,
                        int height,
                        int stride_bytes);
void gc89p_surface_clear(GC89P_SoftwareSurface *surface,
                         unsigned long rgba);
void gc89p_make_software_primitives(GC89_DrawCallbacks *callbacks);

/*
 * Plug-and-play vector path: use the built-in primitive rasterizer directly
 * on a caller-owned RGBA surface. Image-only content still needs an asset or
 * image provider; the current vector-only preset pack needs neither.
 */
int gc89p_draw_rgba(const GC89_Core *core,
                    const GC89_DrawSpec *spec,
                    GC89P_SoftwareSurface *surface,
                    GC89P_DrawReport *report);

#ifdef __cplusplus
}
#endif

#endif
