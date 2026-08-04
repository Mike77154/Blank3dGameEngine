#include "blank3d_ecg_vitals.h"
#include "ecg_bighud.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const ECG_I8 b3d_ecg_flatline_signal[ECG_SIGNAL_COLS] = {
    15,15,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15
};

static int b3d_ecg_clamp_percent(int value)
{
    if (value < 0) return 0;
    if (value > 100) return 100;
    return value;
}

static const char *b3d_ecg_state_for_health(const Blank3DEcgVitals *vitals,
                                                   int health)
{
    if (health <= 0) return "FLATLINE";
    if (health < vitals->dynamics.danger_below) return "DANGER";
    if (health < vitals->dynamics.orange_below) return "ORANGE";
    if (health < vitals->dynamics.caution_below) return "CAUTION";
    return "FINE";
}

static unsigned int b3d_ecg_compute_bpm(const Blank3DEcgVitals *vitals,
                                                int health, int threat_level)
{
    unsigned int bpm;
    int injury;

    if (health <= 0) return 0u;
    injury = 100 - health;
    if (injury < 0) injury = 0;
    if (injury > 100) injury = 100;
    bpm = vitals->dynamics.bpm_base;
    bpm += (unsigned int)((threat_level *
            (int)vitals->dynamics.bpm_threat_span) / 100);
    bpm += (unsigned int)((injury *
            (int)vitals->dynamics.bpm_injury_span) / 100);
    if (bpm < vitals->dynamics.bpm_min) bpm = vitals->dynamics.bpm_min;
    if (bpm > vitals->dynamics.bpm_max) bpm = vitals->dynamics.bpm_max;
    return bpm;
}

static unsigned int b3d_ecg_step_interval_ms(const Blank3DEcgVitals *vitals,
                                                    unsigned int bpm)
{
    unsigned int interval;
    if (bpm == 0u) return vitals->dynamics.interval_max_ms;
    interval = vitals->dynamics.interval_numerator / bpm;
    if (interval < vitals->dynamics.interval_min_ms)
        interval = vitals->dynamics.interval_min_ms;
    if (interval > vitals->dynamics.interval_max_ms)
        interval = vitals->dynamics.interval_max_ms;
    return interval;
}

static void b3d_ecg_rotate_profiles(Blank3DEcgVitals *vitals)
{
    ECG_I8 signal[ECG_SIGNAL_COLS];
    unsigned int profile_index;
    unsigned int i;
    unsigned int source_index;
    ECG_Profile *base;

    if (vitals == (Blank3DEcgVitals *)0) return;
    for (profile_index = 0u;
         profile_index < B3D_ECG_PROFILE_COUNT;
         profile_index++) {
        base = &vitals->profiles[profile_index];
        for (i = 0u; i < ECG_SIGNAL_COLS; i++) {
            source_index = (i + vitals->phase) % ECG_SIGNAL_COLS;
            signal[i] = base->samples[source_index];
        }
        (void)ecg_profile_from_signal(
            &vitals->render_profiles[profile_index],
            base->name, base->color, base->gradient, signal);
    }
}

void blank3d_ecg_vitals_init(Blank3DEcgVitals *vitals)
{
    ECG_HudState flatline_state;

    if (vitals == (Blank3DEcgVitals *)0) return;
    memset(vitals, 0, sizeof(*vitals));

    vitals->dynamics.dynamic_state = 1;
    vitals->dynamics.dynamic_bpm = 1;
    vitals->dynamics.dynamic_offset = 1;
    vitals->dynamics.dynamic_text = 1;
    vitals->dynamics.dynamic_damage_overlay = 1;
    vitals->dynamics.phase_direction = 1;
    vitals->dynamics.danger_below = 25;
    vitals->dynamics.orange_below = 50;
    vitals->dynamics.caution_below = 75;
    vitals->dynamics.bpm_base = 62u;
    vitals->dynamics.bpm_threat_span = 58u;
    vitals->dynamics.bpm_injury_span = 46u;
    vitals->dynamics.bpm_min = 62u;
    vitals->dynamics.bpm_max = 184u;
    vitals->dynamics.interval_numerator = 7200u;
    vitals->dynamics.interval_min_ms = 34u;
    vitals->dynamics.interval_max_ms = 116u;
    vitals->dynamics.damage_interval_ms = 28u;
    vitals->dynamics.damage_flash_period_ms = 70u;
    vitals->dynamics.fixed_offset = ECG_SIGNAL_COLS - 1u;
    vitals->dynamics.clear_color = ecg_color_make(0u, 0u, 0u);

    vitals->surface.pixels = vitals->pixels;
    vitals->surface.width = B3D_ECG_WIDTH;
    vitals->surface.height = B3D_ECG_HEIGHT;
    vitals->surface.stride = B3D_ECG_WIDTH;

    (void)ecg_profiles_init(vitals->profiles);
    (void)ecg_profile_from_signal(
        &vitals->profiles[B3D_ECG_PROFILE_FLATLINE],
        "FLATLINE",
        ecg_color_make(255u, 26u, 26u),
        ecg_color_make(7u, 1u, 1u),
        b3d_ecg_flatline_signal);

    ecg_hud_config_default(&vitals->config);
    (void)ecg_hud_config_add_state(
        &vitals->config,
        "FLATLINE",
        ecg_color_make(255u, 26u, 26u),
        ecg_color_make(7u, 1u, 1u),
        ecg_color_make(255u, 0u, 0u),
        B3D_ECG_PROFILE_FLATLINE,
        &flatline_state);

    vitals->config.x = 8;
    vitals->config.y = 8;
    vitals->config.visible_cols = ECG_SIGNAL_COLS;
    vitals->config.active_x = 2;
    vitals->config.active_y = 48;
    vitals->config.state_text_x = 4;
    vitals->config.state_text_y = 5;
    vitals->config.state_text_scale = 2u;
    vitals->config.custom_text_x = 4;
    vitals->config.custom_text_y = 29;
    vitals->config.custom_text_scale = 1u;
    vitals->config.custom_text_color = ecg_color_make(190u, 220u, 205u);
    vitals->config.draw_flags = ECG_HUD_DRAW_BACKGROUND_BOX |
                                 ECG_HUD_DRAW_STATE_TEXT |
                                 ECG_HUD_DRAW_CUSTOM_TEXT |
                                 ECG_HUD_DRAW_ACTIVE;

    vitals->config.background_box.x = -4;
    vitals->config.background_box.y = -4;
    vitals->config.background_box.width = 344u;
    vitals->config.background_box.height = 100u;
    vitals->config.background_box.filled = 1u;
    vitals->config.background_box.color = ecg_color_make(4u, 10u, 8u);

    vitals->config.overlay_box.x = -4;
    vitals->config.overlay_box.y = -4;
    vitals->config.overlay_box.width = 344u;
    vitals->config.overlay_box.height = 100u;
    vitals->config.overlay_box.filled = 0u;
    vitals->config.overlay_box.color = ecg_color_make(255u, 28u, 20u);

    vitals->config.active_render.draw_flags = ECG_RENDER_DRAW_GRID |
                                               ECG_RENDER_DRAW_AXIS |
                                               ECG_RENDER_DRAW_FRAME |
                                               ECG_RENDER_DRAW_SIGNAL;
    vitals->config.active_render.x_step_q8 = ecg_fixed_from_int(4);
    vitals->config.active_render.y_step_q8 = ecg_fixed_from_ratio(3, 2);
    vitals->config.active_render.grid_x_units = 4u;
    vitals->config.active_render.grid_y_units = 5u;
    vitals->config.active_render.axis_y_units = 15u;
    vitals->config.active_render.waveform_y_units = 30u;
    vitals->config.active_render.bar_width_px = 2u;
    vitals->config.active_render.signal_style = ECG_SIGNAL_STYLE_CONTINUOUS;
    vitals->config.active_render.glow_enabled = 1u;
    vitals->config.active_render.glow_radius_px = 3u;
    vitals->config.active_render.glow_intensity = 86u;
    vitals->config.active_render.grid_color = ecg_color_make(9u, 31u, 22u);
    vitals->config.active_render.axis_color = ecg_color_make(18u, 58u, 42u);
    vitals->config.active_render.frame_color = ecg_color_make(30u, 92u, 65u);

    vitals->phase = 0u;
    b3d_ecg_rotate_profiles(vitals);
    vitals->phase_accum_ms = 0u;
    vitals->bpm = 62u;
    vitals->health = 100;
    vitals->threat_level = 0;
    vitals->initialized = 1;
}

void blank3d_ecg_vitals_update(Blank3DEcgVitals *vitals,
                               int health,
                               int threat_level,
                               unsigned int damage_flash_ms,
                               unsigned int frame_ms)
{
    ECG_Color background;
    unsigned int interval_ms;
    char text[ECG_HUD_CUSTOM_TEXT_CAPACITY];

    if (vitals == (Blank3DEcgVitals *)0) return;
    if (!vitals->initialized) blank3d_ecg_vitals_init(vitals);

    health = b3d_ecg_clamp_percent(health);
    threat_level = b3d_ecg_clamp_percent(threat_level);
    vitals->health = health;
    vitals->threat_level = threat_level;
    if (vitals->dynamics.dynamic_bpm)
        vitals->bpm = b3d_ecg_compute_bpm(vitals, health, threat_level);

    interval_ms = b3d_ecg_step_interval_ms(vitals, vitals->bpm);
    if (interval_ms == 0u) interval_ms = 1u;
    if (damage_flash_ms > 0u &&
        interval_ms > vitals->dynamics.damage_interval_ms)
        interval_ms = vitals->dynamics.damage_interval_ms;
    vitals->phase_accum_ms += frame_ms;
    while (vitals->phase_accum_ms >= interval_ms) {
        vitals->phase_accum_ms -= interval_ms;
        if (vitals->dynamics.phase_direction < 0) {
            if (vitals->phase == 0u) vitals->phase = ECG_SIGNAL_COLS - 1u;
            else vitals->phase--;
        } else {
            vitals->phase = (vitals->phase + 1u) % ECG_SIGNAL_COLS;
        }
    }
    if (vitals->dynamics.dynamic_offset)
        vitals->config.offset = vitals->dynamics.fixed_offset;
    b3d_ecg_rotate_profiles(vitals);

    if (vitals->dynamics.dynamic_state)
        (void)ecg_hud_config_set_state_name(
            &vitals->config, b3d_ecg_state_for_health(vitals, health));

    if (vitals->dynamics.dynamic_text && health <= 0) {
        sprintf(text, "HP 000  NO PULSE  THREAT %03d", threat_level);
    } else if (vitals->dynamics.dynamic_text) {
        sprintf(text, "HP %03d  BPM %03u  THREAT %03d",
                health, vitals->bpm, threat_level);
    }
    if (vitals->dynamics.dynamic_text)
        (void)ecg_hud_config_set_custom_text(&vitals->config, text);

    if (vitals->dynamics.dynamic_damage_overlay &&
        damage_flash_ms > 0u &&
        vitals->dynamics.damage_flash_period_ms > 0u &&
        ((damage_flash_ms / vitals->dynamics.damage_flash_period_ms) & 1u) != 0u) {
        vitals->config.draw_flags |= ECG_HUD_DRAW_OVERLAY_BOX;
    } else if (vitals->dynamics.dynamic_damage_overlay) {
        vitals->config.draw_flags &= ~ECG_HUD_DRAW_OVERLAY_BOX;
    }

    background = vitals->dynamics.clear_color;
    (void)ecg_surface_clear(&vitals->surface, background);
    (void)ecg_draw_hud_ex(&vitals->surface,
                          vitals->render_profiles,
                          B3D_ECG_PROFILE_COUNT,
                          &vitals->config);
}

static int b3d_ecg_text_eq(const char *a, const char *b)
{
    int ca;
    int cb;
    if (!a || !b) return 0;
    while (*a && *b) {
        ca = (unsigned char)*a++;
        cb = (unsigned char)*b++;
        if (ca >= 'a' && ca <= 'z') ca -= ('a' - 'A');
        if (cb >= 'a' && cb <= 'z') cb -= ('a' - 'A');
        if (ca != cb) return 0;
    }
    return *a == '\0' && *b == '\0';
}

static int b3d_ecg_parse_long(const char *text, long *out)
{
    char *end;
    long value;
    if (!text || !out) return 0;
    value = strtol(text, &end, 0);
    while (*end == ' ' || *end == '\t') ++end;
    if (*end != '\0') return 0;
    *out = value;
    return 1;
}

static int b3d_ecg_parse_bool(const char *text, int *out)
{
    long value;
    if (b3d_ecg_text_eq(text, "true") || b3d_ecg_text_eq(text, "yes") ||
        b3d_ecg_text_eq(text, "on")) { *out = 1; return 1; }
    if (b3d_ecg_text_eq(text, "false") || b3d_ecg_text_eq(text, "no") ||
        b3d_ecg_text_eq(text, "off")) { *out = 0; return 1; }
    if (!b3d_ecg_parse_long(text, &value)) return 0;
    *out = value != 0L;
    return 1;
}

int blank3d_ecg_vitals_apply_bighud_property(Blank3DEcgVitals *vitals,
                                             const char *key,
                                             const char *value)
{
    long number;
    int boolean_value;
    int result;
    ECG_Color color;
    if (!vitals || !key || !value) return -1;
    result = ecg_bighud_apply_property(&vitals->config, key, value);
    if (result != ECG_BIGHUD_UNKNOWN) return result;
#define B3D_ECG_BOOL(name, field) if (b3d_ecg_text_eq(key, name)) { if (!b3d_ecg_parse_bool(value, &boolean_value)) return -1; vitals->dynamics.field = boolean_value; return 1; }
#define B3D_ECG_INT(name, field) if (b3d_ecg_text_eq(key, name)) { if (!b3d_ecg_parse_long(value, &number)) return -1; vitals->dynamics.field = (int)number; return 1; }
#define B3D_ECG_UINT(name, field) if (b3d_ecg_text_eq(key, name)) { if (!b3d_ecg_parse_long(value, &number) || number < 0L) return -1; vitals->dynamics.field = (unsigned int)number; return 1; }
    B3D_ECG_BOOL("dynamic_state", dynamic_state)
    B3D_ECG_BOOL("dynamic_bpm", dynamic_bpm)
    B3D_ECG_BOOL("dynamic_offset", dynamic_offset)
    B3D_ECG_BOOL("dynamic_text", dynamic_text)
    B3D_ECG_BOOL("dynamic_damage_overlay", dynamic_damage_overlay)
    B3D_ECG_INT("phase_direction", phase_direction)
    B3D_ECG_INT("danger_below", danger_below)
    B3D_ECG_INT("orange_below", orange_below)
    B3D_ECG_INT("caution_below", caution_below)
    B3D_ECG_UINT("bpm_base", bpm_base)
    B3D_ECG_UINT("bpm_threat_span", bpm_threat_span)
    B3D_ECG_UINT("bpm_injury_span", bpm_injury_span)
    B3D_ECG_UINT("bpm_min", bpm_min)
    B3D_ECG_UINT("bpm_max", bpm_max)
    if (b3d_ecg_text_eq(key, "interval_numerator")) { if (!b3d_ecg_parse_long(value, &number) || number < 1L) return -1; vitals->dynamics.interval_numerator = (unsigned int)number; return 1; }
    if (b3d_ecg_text_eq(key, "interval_min_ms")) { if (!b3d_ecg_parse_long(value, &number) || number < 1L) return -1; vitals->dynamics.interval_min_ms = (unsigned int)number; return 1; }
    if (b3d_ecg_text_eq(key, "interval_max_ms")) { if (!b3d_ecg_parse_long(value, &number) || number < 1L) return -1; vitals->dynamics.interval_max_ms = (unsigned int)number; return 1; }
    if (b3d_ecg_text_eq(key, "damage_interval_ms")) { if (!b3d_ecg_parse_long(value, &number) || number < 1L) return -1; vitals->dynamics.damage_interval_ms = (unsigned int)number; return 1; }
    if (b3d_ecg_text_eq(key, "damage_flash_period_ms")) { if (!b3d_ecg_parse_long(value, &number) || number < 1L) return -1; vitals->dynamics.damage_flash_period_ms = (unsigned int)number; return 1; }
    B3D_ECG_UINT("fixed_offset", fixed_offset)
    if (b3d_ecg_text_eq(key, "bpm") || b3d_ecg_text_eq(key, "fixed_bpm")) { if (!b3d_ecg_parse_long(value, &number) || number < 0L) return -1; vitals->bpm = (unsigned int)number; return 1; }
    if (b3d_ecg_text_eq(key, "phase")) { if (!b3d_ecg_parse_long(value, &number) || number < 0L) return -1; vitals->phase = (unsigned int)number % ECG_SIGNAL_COLS; return 1; }
    if (b3d_ecg_text_eq(key, "phase_accum_ms")) { if (!b3d_ecg_parse_long(value, &number) || number < 0L) return -1; vitals->phase_accum_ms = (unsigned int)number; return 1; }
#undef B3D_ECG_BOOL
#undef B3D_ECG_INT
#undef B3D_ECG_UINT
    if (b3d_ecg_text_eq(key, "clear_color")) {
        ECG_HudConfig scratch;
        ecg_hud_config_default(&scratch);
        result = ecg_bighud_apply_property(&scratch, "background_color", value);
        if (result == 1) {
            color = scratch.background_box.color;
            vitals->dynamics.clear_color = color;
            return 1;
        }
        return -1;
    }
    return 0;
}

const ECG_Surface *blank3d_ecg_vitals_surface(
                               const Blank3DEcgVitals *vitals)
{
    if (vitals == (const Blank3DEcgVitals *)0 || !vitals->initialized)
        return (const ECG_Surface *)0;
    return &vitals->surface;
}

unsigned int blank3d_ecg_vitals_bpm(const Blank3DEcgVitals *vitals)
{
    if (vitals == (const Blank3DEcgVitals *)0) return 0u;
    return vitals->bpm;
}
