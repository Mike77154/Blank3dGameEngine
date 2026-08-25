#include "blank3d_gloco_profile_ini.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

static char *b3d_trim(char *s)
{
    char *e;
    while (*s && isspace((unsigned char)*s)) ++s;
    e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1])) --e;
    *e = '\0';
    return s;
}

static int b3d_parse_q8(const char *s, GLOCO_FX *out)
{
    long whole;
    long frac;
    long scale;
    int neg;
    int digits;
    const char *p;
    if (!s || !out) return 0;
    p = s;
    neg = 0;
    if (*p == '-') { neg = 1; ++p; }
    else if (*p == '+') ++p;
    if (!isdigit((unsigned char)*p) && *p != '.') return 0;
    whole = 0;
    while (isdigit((unsigned char)*p)) {
        whole = whole * 10L + (long)(*p - '0');
        ++p;
    }
    frac = 0;
    scale = 1;
    digits = 0;
    if (*p == '.') {
        ++p;
        while (isdigit((unsigned char)*p) && digits < 5) {
            frac = frac * 10L + (long)(*p - '0');
            scale *= 10L;
            ++p;
            ++digits;
        }
        while (isdigit((unsigned char)*p)) ++p;
    }
    while (*p && isspace((unsigned char)*p)) ++p;
    if (*p != '\0') return 0;
    whole = whole * GLOCO_FX_ONE + (frac * GLOCO_FX_ONE) / scale;
    if (neg) whole = -whole;
    *out = (GLOCO_FX)whole;
    return 1;
}

static int b3d_parse_u16(const char *s, GLOCO_U16 *out)
{
    long v;
    const char *p;
    if (!s || !out) return 0;
    p = s;
    if (!isdigit((unsigned char)*p)) return 0;
    v = 0;
    while (isdigit((unsigned char)*p)) {
        v = v * 10L + (long)(*p - '0');
        if (v > 65535L) v = 65535L;
        ++p;
    }
    while (*p && isspace((unsigned char)*p)) ++p;
    if (*p != '\0') return 0;
    *out = (GLOCO_U16)v;
    return 1;
}

static int b3d_preset(const char *s)
{
    if (!s) return -1;
    if (strcmp(s, "default") == 0) return GLOCO_PROFILE_DEFAULT;
    if (strcmp(s, "tactical") == 0) return GLOCO_PROFILE_TACTICAL;
    if (strcmp(s, "arcade") == 0) return GLOCO_PROFILE_ARCADE;
    if (strcmp(s, "heavy") == 0) return GLOCO_PROFILE_HEAVY;
    return -1;
}

#define B3D_Q8_FIELD(KEY, FIELD) \
    if (strcmp(key, KEY) == 0) return b3d_parse_q8(value, &p->FIELD)
#define B3D_U16_FIELD(KEY, FIELD) \
    if (strcmp(key, KEY) == 0) return b3d_parse_u16(value, &p->FIELD)

static int b3d_apply(GLOCO_Profile *p, const char *key, const char *value)
{
    B3D_Q8_FIELD("walk_speed", walk_speed);
    B3D_Q8_FIELD("run_speed", run_speed);
    B3D_Q8_FIELD("sprint_speed", sprint_speed);
    B3D_Q8_FIELD("aim_speed", aim_speed);
    B3D_Q8_FIELD("crouch_speed", crouch_speed);
    B3D_Q8_FIELD("acceleration", acceleration);
    B3D_Q8_FIELD("sprint_acceleration", sprint_acceleration);
    B3D_Q8_FIELD("braking", braking);
    B3D_Q8_FIELD("hard_braking", hard_braking);
    B3D_Q8_FIELD("ground_friction", ground_friction);
    B3D_Q8_FIELD("air_control", air_control);
    B3D_Q8_FIELD("side_scale", side_scale);
    B3D_Q8_FIELD("back_scale", back_scale);
    B3D_Q8_FIELD("aim_side_scale", aim_side_scale);
    B3D_Q8_FIELD("turn_rate", turn_rate);
    B3D_Q8_FIELD("yaw_lag", yaw_lag);
    B3D_Q8_FIELD("gravity", gravity);
    B3D_Q8_FIELD("terminal_fall", terminal_fall);
    B3D_Q8_FIELD("ground_snap", ground_snap);
    B3D_Q8_FIELD("max_slope_dot", max_slope_dot);
    B3D_Q8_FIELD("capsule_radius", capsule_radius);
    B3D_Q8_FIELD("capsule_height", capsule_height);
    B3D_Q8_FIELD("evade_speed", evade_speed);
    B3D_U16_FIELD("evade_active_ms", evade_active_ms);
    B3D_U16_FIELD("evade_recover_ms", evade_recover_ms);
    B3D_Q8_FIELD("evade_friction", evade_friction);
    B3D_Q8_FIELD("evade_control", evade_control);
    B3D_Q8_FIELD("evade_stamina_cost", evade_stamina_cost);
    B3D_Q8_FIELD("slide_min_speed", slide_min_speed);
    B3D_Q8_FIELD("slide_friction", slide_friction);
    B3D_U16_FIELD("slide_ms", slide_ms);
    B3D_Q8_FIELD("stamina_max", stamina_max);
    B3D_Q8_FIELD("stamina_sprint_drain", stamina_sprint_drain);
    B3D_Q8_FIELD("stamina_recover", stamina_recover);
    B3D_Q8_FIELD("stamina_min_sprint", stamina_min_sprint);
    B3D_Q8_FIELD("stride_walk", stride_walk);
    B3D_Q8_FIELD("stride_run", stride_run);
    B3D_Q8_FIELD("stride_sprint", stride_sprint);
    return 1;
}

int blank3d_gloco_profile_load_ini(GLOCO_Profile *profile,
                                    const char *path,
                                    int fallback_preset,
                                    char *status,
                                    unsigned int status_cap)
{
    FILE *f;
    char line[256];
    int active;
    int preset;
    if (!profile) return 0;
    gloco_profile_defaults(profile, fallback_preset);
    if (!path || !path[0]) return 1;
    f = fopen(path, "rb");
    if (!f) {
        if (status && status_cap) { unsigned int n_; const char *m_; m_ = "GLOCO profile fallback"; n_ = (unsigned int)strlen(m_); if (n_ >= status_cap) n_ = status_cap - 1U; memcpy(status, m_, n_); status[n_] = '\0'; }
        return 1;
    }
    active = 0;
    while (fgets(line, sizeof(line), f)) {
        char *s;
        char *eq;
        char *key;
        char *value;
        s = b3d_trim(line);
        if (!*s || *s == '#' || *s == ';') continue;
        if (*s == '[') {
            active = strcmp(s, "[locomotion]") == 0 || strcmp(s, "[profile]") == 0;
            continue;
        }
        if (!active) continue;
        eq = strchr(s, '=');
        if (!eq) continue;
        *eq = '\0';
        key = b3d_trim(s);
        value = b3d_trim(eq + 1);
        if (strcmp(key, "preset") == 0) {
            preset = b3d_preset(value);
            if (preset >= 0) gloco_profile_defaults(profile, preset);
        } else {
            (void)b3d_apply(profile, key, value);
        }
    }
    fclose(f);
    if (status && status_cap) { unsigned int n_; const char *m_; m_ = "GLOCO profile loaded"; n_ = (unsigned int)strlen(m_); if (n_ >= status_cap) n_ = status_cap - 1U; memcpy(status, m_, n_); status[n_] = '\0'; }
    return 1;
}
