#ifndef GWEAPONSNIPER89_H
#define GWEAPONSNIPER89_H

#include "gtelescopiczoom89.h"
#include "gsway89.h"
#include "gsniperhud89.h"
#include "gaimquery89.h"
#include "gscopebundle89.h"
#include "gscopeanim89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GWSN89_SCOPE_MAX_ANIM_SHAPES 512

typedef struct GWeaponSniper89Tag {
    gscb89_ctx scope_bundle;
    gri89_doc scope_recipe;
    gtz89_ctx zoom;
    gsw89_ctx sway;
    gaq89_ctx aim;
    gsh89_ctx hud;
    gsp89_painter painter;
    gsa89_ctx animation;
    gsa89_pose animation_pose;
    gsv89_shape animation_scratch[GWSN89_SCOPE_MAX_ANIM_SHAPES];
    gsv89_palette palette;
    const gsvp89_preset *preset;
    g89_camera camera;
    gsh89_telemetry telemetry;
    short recipe_loaded;
    short animation_loaded;
    short active;
    short scoped;
    short current_fov_deg_x100;
    short sensitivity_pct;
} GWeaponSniper89;

void gweaponsniper89_init(GWeaponSniper89 *sniper,
                         short screen_w,
                         short screen_h,
                         short base_fov_deg_x100,
                         gaq89_raycast_cb raycast_cb,
                         void *raycast_user,
                         const char *scope_recipe_or_legacy_preset);
void gweaponsniper89_set_screen(GWeaponSniper89 *sniper,
                               short screen_w,
                               short screen_h);
void gweaponsniper89_update(GWeaponSniper89 *sniper,
                           short active,
                           short zoom_held,
                           short hold_breath,
                           short movement_pct,
                           short stress_pct,
                           short dt_frames,
                           unsigned short dt_ms,
                           const g89_camera *camera);
short gweaponsniper89_fov_deg_x100(const GWeaponSniper89 *sniper);
short gweaponsniper89_sensitivity_pct(const GWeaponSniper89 *sniper);
short gweaponsniper89_sway_yaw_x1000(const GWeaponSniper89 *sniper);
short gweaponsniper89_sway_pitch_x1000(const GWeaponSniper89 *sniper);
short gweaponsniper89_is_scoped(const GWeaponSniper89 *sniper);
short gweaponsniper89_scope_recipe_loaded(const GWeaponSniper89 *sniper);
short gweaponsniper89_scope_animation_loaded(const GWeaponSniper89 *sniper);
short gweaponsniper89_trigger_event(GWeaponSniper89 *sniper,
                                   const char *trigger_name);
short gweaponsniper89_scope_last_error(const GWeaponSniper89 *sniper);
int gweaponsniper89_register_scope_provider(GWeaponSniper89 *sniper,
                                            const gpr89_provider *provider);
void gweaponsniper89_emit(GWeaponSniper89 *sniper,
                         gsh89_emit_cb hud_emit,
                         void *hud_user,
                         gsp89_emit_cb preset_emit,
                         void *preset_user);

#ifdef __cplusplus
}
#endif

#endif
