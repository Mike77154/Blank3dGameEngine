/*
 * gsway89.h - procedural aim sway with breathing and hold-breath state.
 */
#ifndef GSWAY89_H
#define GSWAY89_H

#include "../../include/gscope89_common.h"
#include "../../gscopeini89/include/gscopeini89.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GSW89_API
#define GSW89_API
#endif

typedef struct gsw89_profile {
    short yaw_amp_x1000;
    short pitch_amp_x1000;
    short drift_cycle_frames;
    short breath_amp_x1000;
    short breath_cycle_frames;
    short pulse_amp_x1000;
    short pulse_cycle_frames;
    short hold_reduction_pct;
    short hold_enter_frames;
    short hold_max_frames;
    short recovery_frames;
    short movement_gain_pct;
    short stress_gain_pct;
} gsw89_profile;

typedef struct gsw89_ctx {
    gsw89_profile profile;

    short hold_requested;
    short movement_pct;
    short stress_pct;

    short hold_blend_x1000;
    short hold_used_frames;
    short recovery_left_frames;
    short exhausted;
    short hold_remaining_pct;

    long yaw_phase_x1000;
    long pitch_phase_x1000;
    long breath_phase_x1000;
    long pulse_phase_x1000;

    short breath_wave_x1000;
    short pulse_wave_x1000;
    short yaw_out_deg_x1000;
    short pitch_out_deg_x1000;
} gsw89_ctx;

GSW89_API void gsw89_init(gsw89_ctx *ctx, const gsw89_profile *profile);
GSW89_API void gsw89_set_profile(gsw89_ctx *ctx,
                                 const gsw89_profile *profile);
GSW89_API void gsw89_set_hold(gsw89_ctx *ctx, short hold_requested);
GSW89_API void gsw89_set_movement_pct(gsw89_ctx *ctx, short movement_pct);
GSW89_API void gsw89_set_stress_pct(gsw89_ctx *ctx, short stress_pct);
GSW89_API void gsw89_reset_breath(gsw89_ctx *ctx);
GSW89_API void gsw89_update(gsw89_ctx *ctx, short dt_frames);
GSW89_API gsw89_profile gsw89_profile_sniper_default(void);
GSW89_API int gsw89_profile_from_recipe(const gri89_doc *doc, gsw89_profile *out_profile);
GSW89_API int gsw89_profile_load_ini(const char *path, gsw89_profile *out_profile);

#ifdef __cplusplus
}
#endif

#endif
