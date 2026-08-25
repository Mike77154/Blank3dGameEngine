#include "gweaponsniper89.h"

#include <string.h>

#define GWSN89_SCOPE_RETICLE_CATALOG "config/scope/reticles/catalog.ini"
#define GWSN89_SCOPE_DEFAULT_PRESET "re5_psg1_game_scope"
#define GWSN89_SCOPE_DEFAULT_ANIMATION "config/scope/animations/reticle_default.ini"

static short b3d_scope_is_recipe_path(const char *text)
{
    if (!text || !text[0]) return 0;
    if (strstr(text, ".ini") != 0) return 1;
    if (strchr(text, '/') != 0 || strchr(text, '\\') != 0) return 1;
    return 0;
}

void gweaponsniper89_init(GWeaponSniper89 *sniper,
                         short screen_w,
                         short screen_h,
                         short base_fov_deg_x100,
                         gaq89_raycast_cb raycast_cb,
                         void *raycast_user,
                         const char *scope_recipe_or_legacy_preset)
{
    gtz89_profile zoom_profile;
    gsw89_profile sway_profile;
    gsh89_profile hud_profile;
    const char *legacy_preset;
    short recipe_ok;
    if (!sniper) return;
    memset(sniper, 0, sizeof(*sniper));

    gscb89_init(&sniper->scope_bundle);
    gsvp89_set_catalog_path(GWSN89_SCOPE_RETICLE_CATALOG);

    recipe_ok = 0;
    legacy_preset = 0;
    if (scope_recipe_or_legacy_preset && scope_recipe_or_legacy_preset[0]) {
        if (b3d_scope_is_recipe_path(scope_recipe_or_legacy_preset)) {
            recipe_ok = (short)gscb89_load_recipe(&sniper->scope_bundle,
                                                   scope_recipe_or_legacy_preset,
                                                   &sniper->scope_recipe);
        } else {
            legacy_preset = scope_recipe_or_legacy_preset;
        }
    }
    sniper->recipe_loaded = recipe_ok;

    gsa89_init(&sniper->animation);
    if (recipe_ok) {
        sniper->animation_loaded = (short)(
            gsa89_load_doc(&sniper->animation, &sniper->scope_recipe) &&
            sniper->animation.clip_count > 0);
    }
    if (!sniper->animation_loaded) {
        sniper->animation_loaded = (short)(
            gsa89_load(&sniper->animation, GWSN89_SCOPE_DEFAULT_ANIMATION) &&
            sniper->animation.clip_count > 0);
    }
    gsa89_sample(&sniper->animation, &sniper->animation_pose);

    if (recipe_ok) {
        if (!gtz89_profile_from_recipe(&sniper->scope_recipe, &zoom_profile))
            zoom_profile = gtz89_profile_sniper8x();
        if (!gsw89_profile_from_recipe(&sniper->scope_recipe, &sway_profile))
            sway_profile = gsw89_profile_sniper_default();
        if (!gsh89_profile_from_recipe(&sniper->scope_recipe, &hud_profile))
            hud_profile = gsh89_profile_sniper_default();
        sniper->preset = gscb89_preset_from_recipe(&sniper->scope_bundle,
                                                    &sniper->scope_recipe,
                                                    "vector_reticle");
    } else {
        zoom_profile = gtz89_profile_sniper8x();
        sway_profile = gsw89_profile_sniper_default();
        hud_profile = gsh89_profile_sniper_default();
        if (legacy_preset)
            sniper->preset = gscb89_find_preset(&sniper->scope_bundle,
                                                 legacy_preset);
    }

    if (!sniper->preset)
        sniper->preset = gscb89_find_preset(&sniper->scope_bundle,
                                             GWSN89_SCOPE_DEFAULT_PRESET);

    /* A formal vector reticle supersedes the lightweight semantic crosshair.
       The semantic commands stay available automatically if no vector preset
       could be resolved. */
    if (sniper->preset) {
        hud_profile.flags = (short)(hud_profile.flags &
            (short)~(GSH89_HUD_CROSSHAIR |
                     GSH89_HUD_CENTER_DOT |
                     GSH89_HUD_RANGE_TICKS));
        gsvp89_default_palette(sniper->preset, &sniper->palette);
    } else {
        gsv89_palette_init(&sniper->palette);
    }

    gtz89_init(&sniper->zoom, &zoom_profile, base_fov_deg_x100);
    (void)gscb89_bind_zoom_provider(&sniper->scope_bundle, &sniper->zoom);
    gsw89_init(&sniper->sway, &sway_profile);
    gaq89_init(&sniper->aim, raycast_cb, raycast_user);
    gscb89_hud_init(&sniper->scope_bundle, &sniper->hud, &hud_profile,
                     screen_w, screen_h, 0, 0);
    gscb89_painter_init(&sniper->scope_bundle, &sniper->painter,
                        screen_w, screen_h, 0, 0);
    sniper->current_fov_deg_x100 = base_fov_deg_x100;
    sniper->sensitivity_pct = 100;
}

void gweaponsniper89_set_screen(GWeaponSniper89 *sniper,
                               short screen_w,
                               short screen_h)
{
    gsp89_emit_cb paint_sink;
    void *paint_user;
    if (!sniper) return;
    gsh89_set_screen(&sniper->hud, screen_w, screen_h);
    paint_sink = sniper->scope_bundle.paint_sink;
    paint_user = sniper->scope_bundle.paint_sink_user;
    gscb89_painter_init(&sniper->scope_bundle, &sniper->painter,
                        screen_w, screen_h, paint_sink, paint_user);
}

void gweaponsniper89_update(GWeaponSniper89 *sniper,
                           short active,
                           short zoom_held,
                           short hold_breath,
                           short movement_pct,
                           short stress_pct,
                           short dt_frames,
                           unsigned short dt_ms,
                           const g89_camera *camera)
{
    const gaq89_hit *hit;
    short was_scoped;
    if (!sniper) return;
    if (dt_frames < 1) dt_frames = 1;
    if (dt_ms < 1) dt_ms = 1;
    was_scoped = sniper->scoped;
    if (sniper->animation_loaded)
        gsa89_tick(&sniper->animation, dt_ms);
    sniper->active = active ? 1 : 0;
    sniper->scoped = (short)(sniper->active && zoom_held);
    if (sniper->animation_loaded) {
        if (!was_scoped && sniper->scoped)
            (void)gsa89_trigger(&sniper->animation, "aim_enter");
        else if (was_scoped && !sniper->scoped)
            (void)gsa89_trigger(&sniper->animation, "aim_exit");
        gsa89_sample(&sniper->animation, &sniper->animation_pose);
    }
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
    (void)gscb89_fill_telemetry(&sniper->scope_bundle,
                                 &sniper->telemetry);
    gsh89_set_visibility(&sniper->hud,
                         sniper->scoped,
                         sniper->zoom.blend_x1000);
}

short gweaponsniper89_fov_deg_x100(const GWeaponSniper89 *sniper)
{
    return sniper ? sniper->current_fov_deg_x100 : 7000;
}

short gweaponsniper89_sensitivity_pct(const GWeaponSniper89 *sniper)
{
    return sniper ? sniper->sensitivity_pct : 100;
}

short gweaponsniper89_sway_yaw_x1000(const GWeaponSniper89 *sniper)
{
    return sniper ? sniper->sway.yaw_out_deg_x1000 : 0;
}

short gweaponsniper89_sway_pitch_x1000(const GWeaponSniper89 *sniper)
{
    return sniper ? sniper->sway.pitch_out_deg_x1000 : 0;
}

short gweaponsniper89_is_scoped(const GWeaponSniper89 *sniper)
{
    return sniper ? sniper->scoped : 0;
}

short gweaponsniper89_scope_recipe_loaded(const GWeaponSniper89 *sniper)
{
    return sniper ? sniper->recipe_loaded : 0;
}

short gweaponsniper89_scope_animation_loaded(const GWeaponSniper89 *sniper)
{
    return sniper ? sniper->animation_loaded : 0;
}

short gweaponsniper89_trigger_event(GWeaponSniper89 *sniper,
                                   const char *trigger_name)
{
    short count;
    if (!sniper || !sniper->animation_loaded || !trigger_name) return 0;
    count = gsa89_trigger(&sniper->animation, trigger_name);
    gsa89_sample(&sniper->animation, &sniper->animation_pose);
    return count;
}

short gweaponsniper89_scope_last_error(const GWeaponSniper89 *sniper)
{
    return sniper ? sniper->scope_bundle.last_error : GSCB89_ERR_RECIPE;
}

int gweaponsniper89_register_scope_provider(GWeaponSniper89 *sniper,
                                            const gpr89_provider *provider)
{
    if (!sniper || !provider) return 0;
    if (!gscb89_register_provider(&sniper->scope_bundle, provider)) return 0;
    return gscb89_bind_zoom_provider(&sniper->scope_bundle, &sniper->zoom);
}

void gweaponsniper89_emit(GWeaponSniper89 *sniper,
                         gsh89_emit_cb hud_emit,
                         void *hud_user,
                         gsp89_emit_cb preset_emit,
                         void *preset_user)
{
    short radius;
    short alpha_255;
    if (!sniper || !sniper->scoped) return;
    sniper->scope_bundle.hud_sink = hud_emit;
    sniper->scope_bundle.hud_sink_user = hud_user;
    sniper->scope_bundle.paint_sink = preset_emit;
    sniper->scope_bundle.paint_sink_user = preset_user;
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
    if (sniper->preset) {
        if (sniper->animation_loaded)
            (void)gscb89_emit_animated_preset(
                &sniper->scope_bundle,
                &sniper->painter,
                sniper->preset,
                &sniper->palette,
                &sniper->animation_pose,
                sniper->animation_scratch,
                GWSN89_SCOPE_MAX_ANIM_SHAPES,
                alpha_255);
        else
            (void)gscb89_emit_preset(&sniper->scope_bundle,
                                     &sniper->painter,
                                     sniper->preset,
                                     &sniper->palette,
                                     alpha_255);
    }
    gsh89_emit_telemetry(&sniper->hud, &sniper->telemetry);
}
