#include "ecg_bighud.h"
#include "ecg_profiles.h"
#include "ecg_surface.h"
#include "ecg_fixed.h"

#include <stdlib.h>
#include <string.h>

#define EBH_TEXT_MAX 192

static int ebh_upper(int c)
{
    if (c >= 'a' && c <= 'z') return c - ('a' - 'A');
    return c;
}

static int ebh_eq(const char *a, const char *b)
{
    if (!a || !b) return 0;
    while (*a && *b) {
        if (ebh_upper((unsigned char)*a) != ebh_upper((unsigned char)*b))
            return 0;
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

static void ebh_trim_copy(char out[EBH_TEXT_MAX], const char *src)
{
    const char *begin;
    const char *end;
    unsigned int n;
    unsigned int i;
    if (!out) return;
    out[0] = '\0';
    if (!src) return;
    begin = src;
    while (*begin == ' ' || *begin == '\t' || *begin == '\r') ++begin;
    end = begin + strlen(begin);
    while (end > begin && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r')) --end;
    if (end > begin + 1 && begin[0] == '"' && end[-1] == '"') {
        ++begin;
        --end;
    }
    n = (unsigned int)(end - begin);
    if (n >= EBH_TEXT_MAX) n = EBH_TEXT_MAX - 1u;
    for (i = 0u; i < n; ++i) out[i] = begin[i];
    out[n] = '\0';
}

static int ebh_parse_long(const char *text, long *out)
{
    char *end;
    long value;
    if (!text || !out || *text == '\0') return 0;
    value = strtol(text, &end, 0);
    while (*end == ' ' || *end == '\t') ++end;
    if (*end != '\0') return 0;
    *out = value;
    return 1;
}

static int ebh_parse_int(const char *text, int *out)
{
    long value;
    if (!out || !ebh_parse_long(text, &value)) return 0;
    *out = (int)value;
    return (long)*out == value;
}

static int ebh_parse_uint(const char *text, unsigned int *out)
{
    long value;
    if (!out || !ebh_parse_long(text, &value) || value < 0L) return 0;
    *out = (unsigned int)value;
    return (long)*out == value;
}

static int ebh_parse_bool(const char *text, unsigned int *out)
{
    if (ebh_eq(text, "true") || ebh_eq(text, "yes") || ebh_eq(text, "on") || ebh_eq(text, "1")) {
        *out = 1u;
        return 1;
    }
    if (ebh_eq(text, "false") || ebh_eq(text, "no") || ebh_eq(text, "off") || ebh_eq(text, "0")) {
        *out = 0u;
        return 1;
    }
    return 0;
}

static int ebh_hex_digit(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    c = ebh_upper(c);
    if (c >= 'A' && c <= 'F') return 10 + c - 'A';
    return -1;
}

static int ebh_parse_color(const char *text, ECG_Color *out)
{
    unsigned long value;
    unsigned int len;
    unsigned int i;
    int digit;
    long values[3];
    int count;
    const char *p;
    char *end;
    if (!text || !out) return 0;
    if (ebh_eq(text, "white")) { *out = ecg_color_make(255u,255u,255u); return 1; }
    if (ebh_eq(text, "black")) { *out = ecg_color_make(0u,0u,0u); return 1; }
    if (ebh_eq(text, "red")) { *out = ecg_color_make(255u,0u,0u); return 1; }
    if (ebh_eq(text, "green")) { *out = ecg_color_make(0u,255u,0u); return 1; }
    if (ebh_eq(text, "blue")) { *out = ecg_color_make(0u,0u,255u); return 1; }
    if (ebh_eq(text, "yellow")) { *out = ecg_color_make(255u,255u,0u); return 1; }
    if (ebh_eq(text, "orange")) { *out = ecg_color_make(255u,128u,0u); return 1; }
    if (ebh_eq(text, "magenta")) { *out = ecg_color_make(255u,0u,255u); return 1; }
    if (ebh_eq(text, "cyan")) { *out = ecg_color_make(0u,255u,255u); return 1; }
    if (*text == '#') {
        ++text;
        len = (unsigned int)strlen(text);
        if (len != 6u && len != 8u) return 0;
        value = 0UL;
        for (i = 0u; i < 6u; ++i) {
            digit = ebh_hex_digit((unsigned char)text[i]);
            if (digit < 0) return 0;
            value = (value << 4) | (unsigned long)digit;
        }
        out->r = (ECG_U8)((value >> 16) & 255UL);
        out->g = (ECG_U8)((value >> 8) & 255UL);
        out->b = (ECG_U8)(value & 255UL);
        return 1;
    }
    p = text;
    count = 0;
    while (*p && count < 3) {
        while (*p == ' ' || *p == '\t' || *p == ',') ++p;
        if (!*p) break;
        values[count] = strtol(p, &end, 10);
        if (end == p || values[count] < 0L || values[count] > 255L) return 0;
        ++count;
        p = end;
        while (*p == ' ' || *p == '\t') ++p;
        if (*p && *p != ',') return 0;
    }
    if (count != 3) return 0;
    out->r = (ECG_U8)values[0]; out->g = (ECG_U8)values[1]; out->b = (ECG_U8)values[2];
    return 1;
}

static int ebh_parse_fixed(const char *text, ECG_FixedQ8 *out)
{
    int sign;
    unsigned long whole;
    unsigned long frac;
    unsigned long denom;
    const char *p;
    ECG_FixedQ8 value;
    int digits;
    if (!text || !out || *text == '\0') return 0;
    sign = 1;
    p = text;
    if (*p == '-') { sign = -1; ++p; }
    else if (*p == '+') ++p;
    whole = 0UL;
    while (*p >= '0' && *p <= '9') {
        whole = whole * 10UL + (unsigned long)(*p - '0');
        ++p;
    }
    frac = 0UL;
    denom = 1UL;
    digits = 0;
    if (*p == '.') {
        ++p;
        while (*p >= '0' && *p <= '9' && digits < 6) {
            frac = frac * 10UL + (unsigned long)(*p - '0');
            denom *= 10UL;
            ++digits;
            ++p;
        }
    }
    if (*p != '\0') return 0;
    value = (ECG_FixedQ8)(whole << 8);
    if (denom > 1UL) value += (ECG_FixedQ8)((frac << 8) / denom);
    *out = sign < 0 ? -value : value;
    return 1;
}

static int ebh_style(const char *text, unsigned int *out)
{
    if (ebh_eq(text, "continuous") || ebh_eq(text, "solid")) *out = ECG_SIGNAL_STYLE_CONTINUOUS;
    else if (ebh_eq(text, "dotted") || ebh_eq(text, "dots")) *out = ECG_SIGNAL_STYLE_DOTTED;
    else if (ebh_eq(text, "bars") || ebh_eq(text, "columns")) *out = ECG_SIGNAL_STYLE_BARS;
    else return ebh_parse_uint(text, out);
    return 1;
}

static void ebh_set_flag(unsigned long *flags, unsigned long mask, unsigned int enabled)
{
    if (enabled) *flags |= mask;
    else *flags &= ~mask;
}

static int ebh_show(ECG_HudConfig *c, const char *name, const char *text)
{
    unsigned int enabled;
    unsigned long mask;
    if (!ebh_parse_bool(text, &enabled)) return ECG_BIGHUD_BAD_VALUE;
    mask = 0UL;
    if (ebh_eq(name, "active")) mask = ECG_HUD_DRAW_ACTIVE;
    else if (ebh_eq(name, "overview")) mask = ECG_HUD_DRAW_OVERVIEW;
    else if (ebh_eq(name, "state_text")) mask = ECG_HUD_DRAW_STATE_TEXT;
    else if (ebh_eq(name, "custom_text")) mask = ECG_HUD_DRAW_CUSTOM_TEXT;
    else if (ebh_eq(name, "background_box")) mask = ECG_HUD_DRAW_BACKGROUND_BOX;
    else if (ebh_eq(name, "overlay_box")) mask = ECG_HUD_DRAW_OVERLAY_BOX;
    else return ECG_BIGHUD_UNKNOWN;
    ebh_set_flag(&c->draw_flags, mask, enabled);
    return ECG_BIGHUD_APPLIED;
}

static int ebh_render_show(ECG_RenderConfig *r, const char *name, const char *text)
{
    unsigned int enabled;
    unsigned long mask;
    if (!ebh_parse_bool(text, &enabled)) return ECG_BIGHUD_BAD_VALUE;
    mask = 0UL;
    if (ebh_eq(name, "grid")) mask = ECG_RENDER_DRAW_GRID;
    else if (ebh_eq(name, "axis")) mask = ECG_RENDER_DRAW_AXIS;
    else if (ebh_eq(name, "frame")) mask = ECG_RENDER_DRAW_FRAME;
    else if (ebh_eq(name, "signal")) mask = ECG_RENDER_DRAW_SIGNAL;
    else if (ebh_eq(name, "window")) mask = ECG_RENDER_DRAW_WINDOW;
    else return ECG_BIGHUD_UNKNOWN;
    ebh_set_flag(&r->draw_flags, mask, enabled);
    return ECG_BIGHUD_APPLIED;
}

static int ebh_apply_render(ECG_RenderConfig *r, const char *key, const char *text)
{
    unsigned int uv;
    unsigned long lv;
    ECG_FixedQ8 fix;
    ECG_Color color;
    if (!r || !key || !text) return ECG_BIGHUD_BAD_VALUE;
    if (strncmp(key, "show_", 5u) == 0) return ebh_render_show(r, key + 5, text);
    if (strncmp(key, "show.", 5u) == 0) return ebh_render_show(r, key + 5, text);
    if (ebh_eq(key, "draw_flags")) {
        {
            long signed_value;
            if (!ebh_parse_long(text, &signed_value)) return ECG_BIGHUD_BAD_VALUE;
            lv = (unsigned long)signed_value;
        }
        r->draw_flags = lv;
        return ECG_BIGHUD_APPLIED;
    }
    if (ebh_eq(key, "x_step") || ebh_eq(key, "scale_x") || ebh_eq(key, "x_step_q8")) {
        if (!ebh_parse_fixed(text, &fix) || fix <= 0L) return ECG_BIGHUD_BAD_VALUE;
        r->x_step_q8 = fix; return ECG_BIGHUD_APPLIED;
    }
    if (ebh_eq(key, "y_step") || ebh_eq(key, "scale_y") || ebh_eq(key, "y_step_q8")) {
        if (!ebh_parse_fixed(text, &fix) || fix <= 0L) return ECG_BIGHUD_BAD_VALUE;
        r->y_step_q8 = fix; return ECG_BIGHUD_APPLIED;
    }
#define EBH_RENDER_UINT(name, field) if (ebh_eq(key, name)) { if (!ebh_parse_uint(text, &uv)) return ECG_BIGHUD_BAD_VALUE; r->field = uv; return ECG_BIGHUD_APPLIED; }
    EBH_RENDER_UINT("bar_width", bar_width_px)
    EBH_RENDER_UINT("bar_width_px", bar_width_px)
    EBH_RENDER_UINT("grid_x_units", grid_x_units)
    EBH_RENDER_UINT("grid_y_units", grid_y_units)
    EBH_RENDER_UINT("axis_y_units", axis_y_units)
    EBH_RENDER_UINT("waveform_y_units", waveform_y_units)
    EBH_RENDER_UINT("dot_on", dot_on_px)
    EBH_RENDER_UINT("dot_on_px", dot_on_px)
    EBH_RENDER_UINT("dot_off", dot_off_px)
    EBH_RENDER_UINT("dot_off_px", dot_off_px)
    EBH_RENDER_UINT("glow_radius", glow_radius_px)
    EBH_RENDER_UINT("glow_radius_px", glow_radius_px)
    EBH_RENDER_UINT("glow_intensity", glow_intensity)
    EBH_RENDER_UINT("use_profile_glow_color", use_profile_glow_color)
#undef EBH_RENDER_UINT
    if (ebh_eq(key, "signal_style") || ebh_eq(key, "style")) {
        if (!ebh_style(text, &uv)) return ECG_BIGHUD_BAD_VALUE;
        r->signal_style = uv; return ECG_BIGHUD_APPLIED;
    }
    if (ebh_eq(key, "glow") || ebh_eq(key, "glow_enabled")) {
        if (!ebh_parse_bool(text, &uv)) return ECG_BIGHUD_BAD_VALUE;
        r->glow_enabled = uv; return ECG_BIGHUD_APPLIED;
    }
#define EBH_RENDER_COLOR(name, field) if (ebh_eq(key, name)) { if (!ebh_parse_color(text, &color)) return ECG_BIGHUD_BAD_VALUE; r->field = color; return ECG_BIGHUD_APPLIED; }
    EBH_RENDER_COLOR("grid_color", grid_color)
    EBH_RENDER_COLOR("axis_color", axis_color)
    EBH_RENDER_COLOR("frame_color", frame_color)
    EBH_RENDER_COLOR("window_color", window_color)
    EBH_RENDER_COLOR("glow_color", glow_color)
#undef EBH_RENDER_COLOR
    return ECG_BIGHUD_UNKNOWN;
}

static ECG_HudBoxConfig *ebh_box(ECG_HudConfig *c, const char *prefix, const char **field)
{
    unsigned int n;
    n = (unsigned int)strlen(prefix);
    if (strncmp(*field, prefix, n) != 0) return (ECG_HudBoxConfig *)0;
    *field += n;
    if (**field == '.' || **field == '_') ++*field;
    return ebh_eq(prefix, "background") ? &c->background_box : &c->overlay_box;
}

static int ebh_apply_box(ECG_HudBoxConfig *box, const char *field, const char *text)
{
    int iv;
    unsigned int uv;
    ECG_Color color;
    if (ebh_eq(field, "x")) { if (!ebh_parse_int(text, &iv)) return -1; box->x = iv; }
    else if (ebh_eq(field, "y")) { if (!ebh_parse_int(text, &iv)) return -1; box->y = iv; }
    else if (ebh_eq(field, "width") || ebh_eq(field, "w")) { if (!ebh_parse_uint(text, &uv)) return -1; box->width = uv; }
    else if (ebh_eq(field, "height") || ebh_eq(field, "h")) { if (!ebh_parse_uint(text, &uv)) return -1; box->height = uv; }
    else if (ebh_eq(field, "filled")) { if (!ebh_parse_bool(text, &uv)) return -1; box->filled = uv; }
    else if (ebh_eq(field, "color")) { if (!ebh_parse_color(text, &color)) return -1; box->color = color; }
    else return 0;
    return 1;
}

static int ebh_state_field(ECG_HudConfig *c, const char *key, const char *text)
{
    const char *name;
    ECG_HudState state;
    ECG_Color color;
    unsigned int profile;
    ECG_Status status;
    int field;
    field = -1;
    name = (const char *)0;
    if (strncmp(key, "state_color.", 12u) == 0) { field = 0; name = key + 12; }
    else if (strncmp(key, "state_gradient.", 15u) == 0) { field = 1; name = key + 15; }
    else if (strncmp(key, "state_glow.", 11u) == 0) { field = 2; name = key + 11; }
    else if (strncmp(key, "state_profile.", 14u) == 0) { field = 3; name = key + 14; }
    if (field < 0) return ECG_BIGHUD_UNKNOWN;
    status = ecg_hud_config_find_state(c, name, &state);
    if (status == ECG_STATUS_NOT_FOUND) {
        status = ecg_hud_config_add_state(c, name,
                    ecg_color_make(255u,255u,255u), ecg_color_make(4u,4u,4u),
                    ecg_color_make(255u,255u,255u), ECG_PROFILE_FINE, &state);
    }
    if (status != ECG_STATUS_OK) return ECG_BIGHUD_BAD_VALUE;
    if (field == 3) {
        if (!ebh_parse_uint(text, &profile)) return ECG_BIGHUD_BAD_VALUE;
        c->states[state].profile_index = profile;
    } else {
        if (!ebh_parse_color(text, &color)) return ECG_BIGHUD_BAD_VALUE;
        if (field == 0) c->states[state].color = color;
        else if (field == 1) c->states[state].gradient = color;
        else c->states[state].glow_color = color;
    }
    return ECG_BIGHUD_APPLIED;
}

int ecg_bighud_apply_property(ECG_HudConfig *c,
                              const char *key,
                              const char *raw_value)
{
    char text[EBH_TEXT_MAX];
    int iv;
    unsigned int uv;
    unsigned long lv;
    ECG_FixedQ8 fix;
    ECG_Color color;
    ECG_HudBoxConfig *box;
    const char *field;
    int result;

    if (!c || !key || !raw_value) return ECG_BIGHUD_BAD_VALUE;
    ebh_trim_copy(text, raw_value);

    if (strncmp(key, "active.", 7u) == 0) return ebh_apply_render(&c->active_render, key + 7, text);
    if (strncmp(key, "overview.", 9u) == 0) return ebh_apply_render(&c->overview_render, key + 9, text);
    if (strncmp(key, "render.", 7u) == 0) {
        result = ebh_apply_render(&c->active_render, key + 7, text);
        if (result <= 0) return result;
        return ebh_apply_render(&c->overview_render, key + 7, text);
    }

    result = ebh_state_field(c, key, text);
    if (result != ECG_BIGHUD_UNKNOWN) return result;

    field = key;
    box = ebh_box(c, "background", &field);
    if (!box) { field = key; box = ebh_box(c, "overlay", &field); }
    if (box) {
        result = ebh_apply_box(box, field, text);
        return result > 0 ? ECG_BIGHUD_APPLIED : (result < 0 ? ECG_BIGHUD_BAD_VALUE : ECG_BIGHUD_UNKNOWN);
    }

    if (strncmp(key, "show_", 5u) == 0) return ebh_show(c, key + 5, text);
    if (strncmp(key, "show.", 5u) == 0) return ebh_show(c, key + 5, text);
    if (strncmp(key, "signal_", 7u) == 0) {
        result = ebh_apply_render(&c->active_render, key + 7, text);
        if (result <= 0) return result;
        return ebh_apply_render(&c->overview_render, key + 7, text);
    }

    if (ebh_eq(key, "x")) { if (!ebh_parse_int(text, &iv)) return -1; c->x = iv; return 1; }
    if (ebh_eq(key, "y")) { if (!ebh_parse_int(text, &iv)) return -1; c->y = iv; return 1; }
    if (ebh_eq(key, "scale") || ebh_eq(key, "scale_x") || ebh_eq(key, "scale_y")) {
        if (!ebh_parse_fixed(text, &fix) || fix <= 0L) return -1;
        if (ebh_eq(key, "scale") || ebh_eq(key, "scale_x")) c->scale_x_q8 = fix;
        if (ebh_eq(key, "scale") || ebh_eq(key, "scale_y")) c->scale_y_q8 = fix;
        return 1;
    }
    if (ebh_eq(key, "state")) return ecg_hud_config_set_state_name(c, text) == ECG_STATUS_OK ? 1 : -1;
    if (ebh_eq(key, "offset")) { if (!ebh_parse_uint(text, &uv)) return -1; c->offset = uv; return 1; }
    if (ebh_eq(key, "visible_cols")) { if (!ebh_parse_uint(text, &uv) || uv == 0u) return -1; c->visible_cols = uv; return 1; }
    if (ebh_eq(key, "draw_flags")) { long signed_value; if (!ebh_parse_long(text, &signed_value)) return -1; lv = (unsigned long)signed_value; c->draw_flags = lv; return 1; }

#define EBH_CFG_INT(name, fieldname) if (ebh_eq(key, name)) { if (!ebh_parse_int(text, &iv)) return -1; c->fieldname = iv; return 1; }
#define EBH_CFG_UINT(name, fieldname) if (ebh_eq(key, name)) { if (!ebh_parse_uint(text, &uv)) return -1; c->fieldname = uv; return 1; }
    EBH_CFG_INT("active_x", active_x)
    EBH_CFG_INT("active_y", active_y)
    EBH_CFG_INT("overview_x", overview_x)
    EBH_CFG_INT("overview_y", overview_y)
    EBH_CFG_INT("state_text_x", state_text_x)
    EBH_CFG_INT("state_text_y", state_text_y)
    EBH_CFG_UINT("state_text_scale", state_text_scale)
    EBH_CFG_INT("custom_text_x", custom_text_x)
    EBH_CFG_INT("custom_text_y", custom_text_y)
    EBH_CFG_UINT("custom_text_scale", custom_text_scale)
#undef EBH_CFG_INT
#undef EBH_CFG_UINT
    if (ebh_eq(key, "custom_text")) return ecg_hud_config_set_custom_text(c, text) == ECG_STATUS_OK ? 1 : -1;
    if (ebh_eq(key, "custom_text_color")) { if (!ebh_parse_color(text, &color)) return -1; c->custom_text_color = color; return 1; }

    if (ebh_eq(key, "grid_color") || ebh_eq(key, "axis_color") ||
        ebh_eq(key, "frame_color") || ebh_eq(key, "window_color") ||
        ebh_eq(key, "glow_color")) {
        result = ebh_apply_render(&c->active_render, key, text);
        if (result <= 0) return result;
        return ebh_apply_render(&c->overview_render, key, text);
    }
    if (ebh_eq(key, "signal_style") || ebh_eq(key, "bar_width") ||
        ebh_eq(key, "grid_x_units") || ebh_eq(key, "grid_y_units") ||
        ebh_eq(key, "axis_y_units") || ebh_eq(key, "waveform_y_units") ||
        ebh_eq(key, "dot_on") || ebh_eq(key, "dot_off") ||
        ebh_eq(key, "glow") || ebh_eq(key, "glow_enabled") ||
        ebh_eq(key, "glow_radius") || ebh_eq(key, "glow_intensity") ||
        ebh_eq(key, "use_profile_glow_color")) {
        result = ebh_apply_render(&c->active_render, key, text);
        if (result <= 0) return result;
        return ebh_apply_render(&c->overview_render, key, text);
    }

    return ECG_BIGHUD_UNKNOWN;
}
