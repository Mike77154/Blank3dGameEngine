/*
 * gsniperhud89.h - renderer-agnostic sniper optic and telemetry HUD.
 */
#ifndef GSNIPERHUD89_H
#define GSNIPERHUD89_H

#include "../../include/gscope89_common.h"
#include "../../gscopeini89/include/gscopeini89.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GSH89_API
#define GSH89_API
#endif

#define GSH89_HUD_BLACKOUT       1
#define GSH89_HUD_CROSSHAIR      2
#define GSH89_HUD_CENTER_DOT     4
#define GSH89_HUD_RANGE_TICKS    8
#define GSH89_HUD_VIGNETTE       16
#define GSH89_HUD_OPTIC_DATA     32
#define GSH89_HUD_TARGET_DATA    64
#define GSH89_HUD_BREATH_DATA    128
#define GSH89_HUD_IMPACT_DATA    256

enum {
    GSH89_CMD_SCOPE_MASK = 1,
    GSH89_CMD_RETICLE_LINE = 2,
    GSH89_CMD_RETICLE_DOT = 3,
    GSH89_CMD_VIGNETTE = 4,
    GSH89_CMD_RANGE_TICK = 5,
    GSH89_CMD_OPTIC_TELEMETRY = 6,
    GSH89_CMD_TARGET_TELEMETRY = 7,
    GSH89_CMD_BREATH_TELEMETRY = 8,
    GSH89_CMD_IMPACT_WORLD = 9
};

typedef struct gsh89_cmd {
    short kind;
    short id;
    short x0;
    short y0;
    short x1;
    short y1;
    short thickness;
    short alpha;
    long value0;
    long value1;
    long value2;
    long value3;
} gsh89_cmd;

typedef void (*gsh89_emit_cb)(void *user, const gsh89_cmd *cmd);

typedef struct gsh89_profile {
    short overlay_id;
    short hud_id;
    short flags;
    short reticle_gap_px;
    short reticle_arm_px;
    short reticle_thickness;
    short tick_count;
    short tick_spacing_px;
    short mask_alpha;
    short vignette_alpha;
} gsh89_profile;

typedef struct gsh89_telemetry {
    short current_fov_deg_x100;
    short zoom_x100;
    short sensitivity_pct;
    short sway_yaw_deg_x1000;
    short sway_pitch_deg_x1000;
    short breath_wave_x1000;
    short hold_remaining_pct;
    short breath_exhausted;
    short target_valid;
    short target_id;
    short target_kind;
    short target_part;
    g89_fx target_distance;
    g89_vec3 impact_position;
} gsh89_telemetry;

typedef struct gsh89_ctx {
    gsh89_profile profile;
    short screen_w;
    short screen_h;
    short visible;
    short blend_x1000;
    gsh89_emit_cb emit_cb;
    void *user;
} gsh89_ctx;

GSH89_API void gsh89_init(gsh89_ctx *ctx,
                          const gsh89_profile *profile,
                          short screen_w,
                          short screen_h,
                          gsh89_emit_cb emit_cb,
                          void *user);
GSH89_API void gsh89_set_profile(gsh89_ctx *ctx,
                                 const gsh89_profile *profile);
GSH89_API void gsh89_set_screen(gsh89_ctx *ctx,
                                short screen_w,
                                short screen_h);
GSH89_API void gsh89_set_visibility(gsh89_ctx *ctx,
                                    short visible,
                                    short blend_x1000);
GSH89_API void gsh89_emit_overlay(gsh89_ctx *ctx);
GSH89_API void gsh89_emit_telemetry(gsh89_ctx *ctx,
                                    const gsh89_telemetry *telemetry);
GSH89_API gsh89_profile gsh89_profile_sniper_default(void);
GSH89_API int gsh89_profile_from_recipe(const gri89_doc *doc,
                                        gsh89_profile *out_profile);
GSH89_API int gsh89_profile_load_ini(const char *path,
                                     gsh89_profile *out_profile);

#ifdef __cplusplus
}
#endif

#endif
