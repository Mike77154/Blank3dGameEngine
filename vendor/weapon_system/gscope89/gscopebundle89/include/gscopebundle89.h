/*
 * gscopebundle89.h - provider-aware facade for the complete gscope89 bundle.
 * C89, no heap. Internal modules remain usable directly; this facade adds
 * provider routing with automatic fallback to the bundled implementations.
 */
#ifndef GSCOPEBUNDLE89_H
#define GSCOPEBUNDLE89_H

#include "gscopeprovider89.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GSCOPEBUNDLE89_API
#define GSCOPEBUNDLE89_API
#endif

#define GSCB89_ABI_VERSION 2u

#define GSCB89_OK 0
#define GSCB89_ERR_PROVIDER_REQUIRED 1
#define GSCB89_ERR_PROVIDER 2
#define GSCB89_ERR_RECIPE 3

typedef struct gscb89_ctx {
    gpr89_hub hub;

    gsp89_emit_cb paint_sink;
    void *paint_sink_user;

    gsh89_emit_cb hud_sink;
    void *hud_sink_user;

    short last_error;
} gscb89_ctx;

GSCOPEBUNDLE89_API void gscb89_init(gscb89_ctx *ctx);
GSCOPEBUNDLE89_API int gscb89_register_provider(gscb89_ctx *ctx,
                                                const gpr89_provider *provider);
GSCOPEBUNDLE89_API void gscb89_set_provider_route(gscb89_ctx *ctx,
                                                   short domain,
                                                   short mode,
                                                   const char *provider_name);
GSCOPEBUNDLE89_API int gscb89_apply_provider_recipe(gscb89_ctx *ctx,
                                                    const gri89_doc *doc);
GSCOPEBUNDLE89_API int gscb89_load_recipe(gscb89_ctx *ctx,
                                          const char *path,
                                          gri89_doc *out_doc);

/* Painter bridge: provider paint backend first, application sink second. */
GSCOPEBUNDLE89_API void gscb89_painter_init(gscb89_ctx *ctx,
                                             gsp89_painter *painter,
                                             short screen_w,
                                             short screen_h,
                                             gsp89_emit_cb final_sink,
                                             void *final_sink_user);

/* Vector batch -> vector provider -> primitive provider -> internal vector. */
GSCOPEBUNDLE89_API int gscb89_emit_vector(gscb89_ctx *ctx,
                                          gsp89_painter *painter,
                                          const gsv89_shape *shapes,
                                          short shape_count,
                                          const gsv89_palette *palette,
                                          short global_alpha);
GSCOPEBUNDLE89_API int gscb89_emit_primitive(gscb89_ctx *ctx,
                                             gsp89_painter *painter,
                                             const gsv89_shape *shape,
                                             const gsv89_palette *palette,
                                             short global_alpha);

GSCOPEBUNDLE89_API int gscb89_emit_raster_layer(gscb89_ctx *ctx,
                                                 gsp89_painter *painter,
                                                 const gsr89_layer *layer,
                                                 short global_alpha);
GSCOPEBUNDLE89_API int gscb89_emit_raster_stack(gscb89_ctx *ctx,
                                                 gsp89_painter *painter,
                                                 const gsr89_stack *stack,
                                                 short global_alpha);

GSCOPEBUNDLE89_API int gscb89_emit_bar(gscb89_ctx *ctx,
                                       gsp89_painter *painter,
                                       const gsb89_bar *bar,
                                       const gsb89_channel *channel,
                                       short global_alpha);
GSCOPEBUNDLE89_API int gscb89_emit_bar_layout(gscb89_ctx *ctx,
                                              gsp89_painter *painter,
                                              const gsb89_layout *layout,
                                              const gsb89_channel *channels,
                                              short channel_count,
                                              short global_alpha);

/* Preset provider may replace the INI catalog. Auto mode falls back to it. */
GSCOPEBUNDLE89_API const gsvp89_preset *gscb89_find_preset(gscb89_ctx *ctx,
                                                           const char *name);
GSCOPEBUNDLE89_API const gsvp89_preset *gscb89_preset_from_recipe(
    gscb89_ctx *ctx,
    const gri89_doc *doc,
    const char *section_name);
GSCOPEBUNDLE89_API int gscb89_emit_preset(gscb89_ctx *ctx,
                                          gsp89_painter *painter,
                                          const gsvp89_preset *preset,
                                          const gsv89_palette *palette_override,
                                          short global_alpha);
GSCOPEBUNDLE89_API int gscb89_emit_animated_preset(
    gscb89_ctx *ctx,
    gsp89_painter *painter,
    const gsvp89_preset *preset,
    const gsv89_palette *palette_override,
    const gsa89_pose *pose,
    gsv89_shape *scratch_shapes,
    short scratch_capacity,
    short global_alpha);

/* HUD bridge and optional telemetry source. */
GSCOPEBUNDLE89_API void gscb89_hud_init(gscb89_ctx *ctx,
                                         gsh89_ctx *hud,
                                         const gsh89_profile *profile,
                                         short screen_w,
                                         short screen_h,
                                         gsh89_emit_cb final_sink,
                                         void *final_sink_user);
GSCOPEBUNDLE89_API int gscb89_fill_telemetry(gscb89_ctx *ctx,
                                             gsh89_telemetry *inout_telemetry);

/* Optional named asset resolver for raster/font/UI systems. */
GSCOPEBUNDLE89_API int gscb89_resolve_asset(gscb89_ctx *ctx,
                                            const char *asset_name,
                                            short *out_asset_id);

/* Adapts the selected bundle zoom provider to gtelescopiczoom89 ABI 2. */
GSCOPEBUNDLE89_API int gscb89_bind_zoom_provider(gscb89_ctx *ctx,
                                                 gtz89_ctx *zoom);

#ifdef __cplusplus
}
#endif

#endif
