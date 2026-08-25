/*
 * gscopeprovider89.h - provider hub for the gscope89 bundle.
 * C89, fixed-point/integer only, no dynamic allocation.
 *
 * Providers are optional. Every routed domain can be:
 *   internal          - always use the bundled implementation
 *   auto:<name>       - try provider, then fall back to bundled implementation
 *   external:<name>   - provider is required; no bundled fallback
 */
#ifndef GSCOPEPROVIDER89_H
#define GSCOPEPROVIDER89_H

#include "gscopeini89.h"
#include "gscopepaint89.h"
#include "gscopevector89.h"
#include "gscoperaster89.h"
#include "gscopebars89.h"
#include "gscopepresets89.h"
#include "gscopeanim89.h"
#include "gsniperhud89.h"
#include "gtelescopiczoom89.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GSCOPEPROVIDER89_API
#define GSCOPEPROVIDER89_API
#endif

#define GPR89_ABI_VERSION 2u
#define GPR89_MAX_PROVIDERS 8
#define GPR89_MAX_NAME 32

#define GPR89_MODE_INTERNAL 0
#define GPR89_MODE_AUTO     1
#define GPR89_MODE_EXTERNAL 2

#define GPR89_DOMAIN_RECIPE     0
#define GPR89_DOMAIN_PRESET     1
#define GPR89_DOMAIN_VECTOR     2
#define GPR89_DOMAIN_PRIMITIVE  3
#define GPR89_DOMAIN_RASTER     4
#define GPR89_DOMAIN_BARS       5
#define GPR89_DOMAIN_PAINT      6
#define GPR89_DOMAIN_HUD        7
#define GPR89_DOMAIN_TELEMETRY  8
#define GPR89_DOMAIN_ASSET      9
#define GPR89_DOMAIN_ZOOM       10
#define GPR89_DOMAIN_ANIMATION  11
#define GPR89_DOMAIN_COUNT      12

#define GPR89_CAP_RECIPE      (1UL << GPR89_DOMAIN_RECIPE)
#define GPR89_CAP_PRESET      (1UL << GPR89_DOMAIN_PRESET)
#define GPR89_CAP_VECTOR      (1UL << GPR89_DOMAIN_VECTOR)
#define GPR89_CAP_PRIMITIVE   (1UL << GPR89_DOMAIN_PRIMITIVE)
#define GPR89_CAP_RASTER      (1UL << GPR89_DOMAIN_RASTER)
#define GPR89_CAP_BARS        (1UL << GPR89_DOMAIN_BARS)
#define GPR89_CAP_PAINT       (1UL << GPR89_DOMAIN_PAINT)
#define GPR89_CAP_HUD         (1UL << GPR89_DOMAIN_HUD)
#define GPR89_CAP_TELEMETRY   (1UL << GPR89_DOMAIN_TELEMETRY)
#define GPR89_CAP_ASSET       (1UL << GPR89_DOMAIN_ASSET)
#define GPR89_CAP_ZOOM        (1UL << GPR89_DOMAIN_ZOOM)
#define GPR89_CAP_ANIMATION   (1UL << GPR89_DOMAIN_ANIMATION)
#define GPR89_CAP_ALL         ((1UL << GPR89_DOMAIN_COUNT) - 1UL)

/* Callback result convention. */
#define GPR89_FALLBACK 0
#define GPR89_HANDLED  1
#define GPR89_ERROR    (-1)

typedef int (*gpr89_load_recipe_fn)(void *user,
                                    const char *path,
                                    gri89_doc *out_doc);
typedef const gsvp89_preset *(*gpr89_find_preset_fn)(void *user,
                                                      const char *name);
typedef int (*gpr89_emit_vector_fn)(void *user,
                                    gsp89_painter *painter,
                                    const gsv89_shape *shapes,
                                    short shape_count,
                                    const gsv89_palette *palette,
                                    short global_alpha);
typedef int (*gpr89_emit_primitive_fn)(void *user,
                                       gsp89_painter *painter,
                                       const gsv89_shape *shape,
                                       const gsv89_palette *palette,
                                       short global_alpha);
typedef int (*gpr89_emit_raster_fn)(void *user,
                                    gsp89_painter *painter,
                                    const gsr89_layer *layer,
                                    short global_alpha);
typedef int (*gpr89_emit_bar_fn)(void *user,
                                 gsp89_painter *painter,
                                 const gsb89_bar *bar,
                                 const gsb89_channel *channel,
                                 short global_alpha);
typedef int (*gpr89_emit_draw_cmd_fn)(void *user,
                                      const gsp89_draw_cmd *cmd);
typedef int (*gpr89_emit_hud_cmd_fn)(void *user,
                                     const gsh89_cmd *cmd);
typedef int (*gpr89_fill_telemetry_fn)(void *user,
                                       gsh89_telemetry *inout_telemetry);
typedef int (*gpr89_resolve_asset_fn)(void *user,
                                      const char *asset_name,
                                      short *out_asset_id);
typedef int (*gpr89_animate_shapes_fn)(void *user,
                                       const gsa89_pose *pose,
                                       const gsv89_shape *src,
                                       short shape_count,
                                       gsv89_shape *dst,
                                       short dst_capacity);

typedef struct gpr89_provider {
    unsigned short abi_version;
    char name[GPR89_MAX_NAME];
    unsigned long capabilities;
    void *user;

    gpr89_load_recipe_fn load_recipe;
    gpr89_find_preset_fn find_preset;
    gpr89_emit_vector_fn emit_vector;
    gpr89_emit_primitive_fn emit_primitive;
    gpr89_emit_raster_fn emit_raster;
    gpr89_emit_bar_fn emit_bar;
    gpr89_emit_draw_cmd_fn emit_draw_cmd;
    gpr89_emit_hud_cmd_fn emit_hud_cmd;
    gpr89_fill_telemetry_fn fill_telemetry;
    gpr89_resolve_asset_fn resolve_asset;
    gpr89_animate_shapes_fn animate_shapes;

    /* Zoom provider callbacks map directly to gtelescopiczoom89 ABI 2. */
    g89_camera *zoom_camera;
    gtz89_provider_read_base_fov_fn zoom_read_base_fov;
    gtz89_provider_write_fov_fn zoom_write_fov;
    gtz89_provider_write_sensitivity_fn zoom_write_sensitivity;
    gtz89_provider_solve_fov_fn zoom_solve_fov;
    gtz89_provider_lerp_short_fn zoom_lerp_short;
} gpr89_provider;

typedef struct gpr89_route {
    short mode;
    char provider_name[GPR89_MAX_NAME];
} gpr89_route;

typedef struct gpr89_hub {
    gpr89_provider providers[GPR89_MAX_PROVIDERS];
    short provider_count;
    gpr89_route routes[GPR89_DOMAIN_COUNT];
    short last_error;
} gpr89_hub;

#define GPR89_OK 0
#define GPR89_ERR_FULL 1
#define GPR89_ERR_ABI 2
#define GPR89_ERR_NAME 3
#define GPR89_ERR_ROUTE 4

GSCOPEPROVIDER89_API void gpr89_provider_init(gpr89_provider *provider,
                                               const char *name);
GSCOPEPROVIDER89_API void gpr89_hub_init(gpr89_hub *hub);
GSCOPEPROVIDER89_API int gpr89_hub_register(gpr89_hub *hub,
                                             const gpr89_provider *provider);
GSCOPEPROVIDER89_API const gpr89_provider *gpr89_hub_find(const gpr89_hub *hub,
                                                           const char *name);
GSCOPEPROVIDER89_API void gpr89_hub_set_route(gpr89_hub *hub,
                                               short domain,
                                               short mode,
                                               const char *provider_name);
GSCOPEPROVIDER89_API int gpr89_hub_apply_recipe(gpr89_hub *hub,
                                                 const gri89_doc *doc);
GSCOPEPROVIDER89_API const gpr89_provider *gpr89_hub_provider_for(
    const gpr89_hub *hub,
    short domain,
    short *out_mode);
GSCOPEPROVIDER89_API const char *gpr89_domain_name(short domain);

#ifdef __cplusplus
}
#endif

#endif
