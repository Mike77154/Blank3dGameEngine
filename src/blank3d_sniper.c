#include "blank3d_sniper.h"

#include <string.h>

void blank3d_sniper_init(Blank3DSniper *sniper,
                         short screen_w,
                         short screen_h,
                         short base_fov_deg_x100,
                         gaq89_raycast_cb raycast_cb,
                         void *raycast_user,
                         const char *preset_name)
{
    gtz89_profile zoom_profile;
    gsw89_profile sway_profile;
    gsh89_profile hud_profile;
    if (!sniper) return;
    memset(sniper, 0, sizeof(*sniper));
    zoom_profile = gtz89_profile_sniper8x();
    sway_profile = gsw89_profile_sniper_default();
    hud_profile = gsh89_profile_sniper_default();
    gtz89_init(&sniper->zoom, &zoom_profile, base_fov_deg_x100);
    gsw89_init(&sniper->sway, &sway_profile);
    gaq89_init(&sniper->aim, raycast_cb, raycast_user);
    gsh89_init(&sniper->hud, &hud_profile, screen_w, screen_h, 0, 0);
    gsp89_painter_init(&sniper->painter, screen_w, screen_h, 0, 0);
    sniper->preset = preset_name ? gsvp89_find(preset_name) : 0;
    if (!sniper->preset) sniper->preset = gsvp89_get(GSVP89_RE5_PSG1_GAME_SCOPE);
    if (sniper->preset)
        gsvp89_default_palette(sniper->preset, &sniper->palette);
    else
        gsv89_palette_init(&sniper->palette);
    sniper->current_fov_deg_x100 = base_fov_deg_x100;
    sniper->sensitivity_pct = 100;
}

void blank3d_sniper_set_screen(Blank3DSniper *sniper,
                               short screen_w,
                               short screen_h)
{
    if (!sniper) return;
    gsh89_set_screen(&sniper->hud, screen_w, screen_h);
    gsp89_painter_init(&sniper->painter, screen_w, screen_h,
                       sniper->painter.emit_cb, sniper->painter.user);
}

void blank3d_sniper_update(Blank3DSniper *sniper,
                           short active,
                           short zoom_held,
                           short hold_breath,
                           short movement_pct,
                           short stress_pct,
                           short dt_frames,
                           const g89_camera *camera)
{
    const gaq89_hit *hit;
    if (!sniper) return;
    if (dt_frames < 1) dt_frames = 1;
    sniper->active = active ? 1 : 0;
    sniper->scoped = (short)(sniper->active && zoom_held);
    if (camera) sniper->camera = *camera;

    gtz89_set_base_fov(&sniper->zoom, sniper->camera.base_fov_deg_x100);
    if (sniper->scoped) gtz89_begin(&sniper->zoom);
    else gtz89_end(&sniper->zoom);
    sniper->current_fov_deg_x100 = gtz89_update(&sniper->zoom, dt_frames);
    sniper->sensitivity_pct = sniper->zoom.sensitivity_pct;

    gsw89_set_hold(&sniper->sway,
                    (short)(sniper->scoped && hold_breath));
    gsw89_set_movement_pct(&sniper->sway, movement_pct);
    gsw89_set_stress_pct(&sniper->sway, stress_pct);
    gsw89_update(&sniper->sway, dt_frames);

    memset(&sniper->telemetry, 0, sizeof(sniper->telemetry));
    sniper->telemetry.current_fov_deg_x100 =
        sniper->current_fov_deg_x100;
    sniper->telemetry.zoom_x100 = sniper->zoom.profile.zoom_x100;
    sniper->telemetry.sensitivity_pct = sniper->sensitivity_pct;
    sniper->telemetry.sway_yaw_deg_x1000 = sniper->sway.yaw_out_deg_x1000;
    sniper->telemetry.sway_pitch_deg_x1000 = sniper->sway.pitch_out_deg_x1000;
    sniper->telemetry.breath_wave_x1000 = sniper->sway.breath_wave_x1000;
    sniper->telemetry.hold_remaining_pct = sniper->sway.hold_remaining_pct;
    sniper->telemetry.breath_exhausted = sniper->sway.exhausted;

    if (sniper->scoped &&
        gaq89_query_center(&sniper->aim, &sniper->camera,
                           G89_FX_FROM_INT(300))) {
        hit = gaq89_get_last_hit(&sniper->aim);
        if (hit) {
            sniper->telemetry.target_valid = hit->valid;
            sniper->telemetry.target_id = hit->target_id;
            sniper->telemetry.target_kind = hit->target_kind;
            sniper->telemetry.target_part = hit->target_part;
            sniper->telemetry.target_distance = hit->distance;
            sniper->telemetry.impact_position = hit->position;
        }
    }
    gsh89_set_visibility(&sniper->hud,
                         sniper->scoped,
                         sniper->zoom.blend_x1000);
}

short blank3d_sniper_fov_deg_x100(const Blank3DSniper *sniper)
{
    return sniper ? sniper->current_fov_deg_x100 : 7000;
}

short blank3d_sniper_sensitivity_pct(const Blank3DSniper *sniper)
{
    return sniper ? sniper->sensitivity_pct : 100;
}

short blank3d_sniper_sway_yaw_x1000(const Blank3DSniper *sniper)
{
    return sniper ? sniper->sway.yaw_out_deg_x1000 : 0;
}

short blank3d_sniper_sway_pitch_x1000(const Blank3DSniper *sniper)
{
    return sniper ? sniper->sway.pitch_out_deg_x1000 : 0;
}

short blank3d_sniper_is_scoped(const Blank3DSniper *sniper)
{
    return sniper ? sniper->scoped : 0;
}

void blank3d_sniper_emit(Blank3DSniper *sniper,
                         gsh89_emit_cb hud_emit,
                         void *hud_user,
                         gsp89_emit_cb preset_emit,
                         void *preset_user)
{
    short radius;
    short alpha_255;
    if (!sniper || !sniper->scoped) return;
    sniper->hud.emit_cb = hud_emit;
    sniper->hud.user = hud_user;
    sniper->painter.emit_cb = preset_emit;
    sniper->painter.user = preset_user;
    radius = sniper->hud.screen_w < sniper->hud.screen_h
           ? (short)(sniper->hud.screen_w / 2)
           : (short)(sniper->hud.screen_h / 2);
    if (radius > 24) radius = (short)(radius - 12);
    alpha_255 = (short)(((long)sniper->zoom.blend_x1000 * 255L) / 1000L);
    if (alpha_255 < 0) alpha_255 = 0;
    if (alpha_255 > 255) alpha_255 = 255;
    gsp89_painter_set_view(&sniper->painter,
                           (short)(sniper->hud.screen_w / 2),
                           (short)(sniper->hud.screen_h / 2),
                           radius, GSP89_FX_ONE, 0, 0,
                           alpha_255);
    gsh89_emit_overlay(&sniper->hud);
    if (sniper->preset)
        gsvp89_emit(&sniper->painter, sniper->preset,
                    &sniper->palette, alpha_255);
    gsh89_emit_telemetry(&sniper->hud, &sniper->telemetry);
}
