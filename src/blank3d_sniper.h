#ifndef BLANK3D_SNIPER_H
#define BLANK3D_SNIPER_H

#include "gtelescopiczoom89.h"
#include "gsway89.h"
#include "gsniperhud89.h"
#include "gaimquery89.h"
#include "gscopepaint89.h"
#include "gscopepresets89.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Blank3DSniperTag {
    gtz89_ctx zoom;
    gsw89_ctx sway;
    gaq89_ctx aim;
    gsh89_ctx hud;
    gsp89_painter painter;
    gsv89_palette palette;
    const gsvp89_preset *preset;
    g89_camera camera;
    gsh89_telemetry telemetry;
    short active;
    short scoped;
    short current_fov_deg_x100;
    short sensitivity_pct;
} Blank3DSniper;

void blank3d_sniper_init(Blank3DSniper *sniper,
                         short screen_w,
                         short screen_h,
                         short base_fov_deg_x100,
                         gaq89_raycast_cb raycast_cb,
                         void *raycast_user,
                         const char *preset_name);
void blank3d_sniper_set_screen(Blank3DSniper *sniper,
                               short screen_w,
                               short screen_h);
void blank3d_sniper_update(Blank3DSniper *sniper,
                           short active,
                           short zoom_held,
                           short hold_breath,
                           short movement_pct,
                           short stress_pct,
                           short dt_frames,
                           const g89_camera *camera);
short blank3d_sniper_fov_deg_x100(const Blank3DSniper *sniper);
short blank3d_sniper_sensitivity_pct(const Blank3DSniper *sniper);
short blank3d_sniper_sway_yaw_x1000(const Blank3DSniper *sniper);
short blank3d_sniper_sway_pitch_x1000(const Blank3DSniper *sniper);
short blank3d_sniper_is_scoped(const Blank3DSniper *sniper);
void blank3d_sniper_emit(Blank3DSniper *sniper,
                         gsh89_emit_cb hud_emit,
                         void *hud_user,
                         gsp89_emit_cb preset_emit,
                         void *preset_user);

#ifdef __cplusplus
}
#endif

#endif
