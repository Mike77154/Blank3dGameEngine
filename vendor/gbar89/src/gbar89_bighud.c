#include "gbar89_bighud.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define GBH_TEXT_MAX 192

static int gbh_upper(int c)
{
    if (c >= 'a' && c <= 'z') return c - ('a' - 'A');
    return c;
}

static int gbh_eq(const char *a, const char *b)
{
    if (!a || !b) return 0;
    while (*a && *b) {
        if (gbh_upper((unsigned char)*a) != gbh_upper((unsigned char)*b))
            return 0;
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

static void gbh_trim_copy(char out[GBH_TEXT_MAX], const char *src)
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
    if (n >= GBH_TEXT_MAX) n = GBH_TEXT_MAX - 1u;
    for (i = 0u; i < n; ++i) out[i] = begin[i];
    out[n] = '\0';
}

static int gbh_parse_long(const char *text, long *out)
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

static int gbh_parse_int(const char *text, int *out)
{
    long value;
    if (!out || !gbh_parse_long(text, &value)) return 0;
    *out = (int)value;
    return (long)*out == value;
}

static int gbh_parse_bool(const char *text, int *out)
{
    if (!text || !out) return 0;
    if (gbh_eq(text, "true") || gbh_eq(text, "yes") ||
        gbh_eq(text, "on") || gbh_eq(text, "1")) {
        *out = 1;
        return 1;
    }
    if (gbh_eq(text, "false") || gbh_eq(text, "no") ||
        gbh_eq(text, "off") || gbh_eq(text, "0")) {
        *out = 0;
        return 1;
    }
    return 0;
}

static int gbh_hex_digit(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    c = gbh_upper(c);
    if (c >= 'A' && c <= 'F') return 10 + c - 'A';
    return -1;
}

static int gbh_parse_color(const char *text, unsigned long *out)
{
    unsigned long value;
    unsigned int i;
    int digit;
    unsigned int len;
    if (!text || !out) return 0;
    if (gbh_eq(text, "white")) { *out = 0xFFFFFFFFUL; return 1; }
    if (gbh_eq(text, "black")) { *out = 0x000000FFUL; return 1; }
    if (gbh_eq(text, "red")) { *out = 0xFF0000FFUL; return 1; }
    if (gbh_eq(text, "green")) { *out = 0x00FF00FFUL; return 1; }
    if (gbh_eq(text, "blue")) { *out = 0x0000FFFFUL; return 1; }
    if (gbh_eq(text, "yellow")) { *out = 0xFFFF00FFUL; return 1; }
    if (gbh_eq(text, "orange")) { *out = 0xFF8000FFUL; return 1; }
    if (gbh_eq(text, "magenta")) { *out = 0xFF00FFFFUL; return 1; }
    if (gbh_eq(text, "cyan")) { *out = 0x00FFFFFFUL; return 1; }
    if (*text != '#') {
        long signed_value;
        if (!gbh_parse_long(text, &signed_value)) return 0;
        *out = (unsigned long)signed_value;
        return 1;
    }
    ++text;
    len = (unsigned int)strlen(text);
    if (len != 6u && len != 8u) return 0;
    value = 0UL;
    for (i = 0u; i < len; ++i) {
        digit = gbh_hex_digit((unsigned char)text[i]);
        if (digit < 0) return 0;
        value = (value << 4) | (unsigned long)digit;
    }
    if (len == 6u) value = (value << 8) | 0xFFUL;
    *out = value;
    return 1;
}

static int gbh_parse_fix(const char *text, GBar89_Fix *out)
{
    int sign;
    unsigned long whole;
    unsigned long frac;
    unsigned long denom;
    const char *p;
    GBar89_Fix value;
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
    value = (GBar89_Fix)(whole << GBAR89_FIX_SHIFT);
    if (denom > 1UL)
        value += (GBar89_Fix)((frac << GBAR89_FIX_SHIFT) / denom);
    *out = sign < 0 ? -value : value;
    return 1;
}

static int gbh_parse_values(const char *text, long *values, int capacity)
{
    char copy[GBH_TEXT_MAX];
    char *p;
    char *end;
    int count;
    if (!text || !values || capacity <= 0) return 0;
    gbh_trim_copy(copy, text);
    p = copy;
    count = 0;
    while (*p && count < capacity) {
        while (*p == ' ' || *p == '\t' || *p == ',') ++p;
        if (!*p) break;
        values[count] = strtol(p, &end, 0);
        if (end == p) return -1;
        ++count;
        p = end;
        while (*p == ' ' || *p == '\t') ++p;
        if (*p && *p != ',') return -1;
    }
    return count;
}

static int gbh_kind(const char *text, int *out)
{
    if (gbh_eq(text, "linear") || gbh_eq(text, "horizontal") || gbh_eq(text, "vertical")) *out = GBAR89_KIND_LINEAR;
    else if (gbh_eq(text, "segmented") || gbh_eq(text, "segments") || gbh_eq(text, "ammo")) *out = GBAR89_KIND_SEGMENTED;
    else if (gbh_eq(text, "sprite_clip")) *out = GBAR89_KIND_SPRITE_CLIP;
    else if (gbh_eq(text, "nineslice") || gbh_eq(text, "nine_slice")) *out = GBAR89_KIND_NINESLICE;
    else if (gbh_eq(text, "radial_pie") || gbh_eq(text, "pie") || gbh_eq(text, "circular")) *out = GBAR89_KIND_RADIAL_PIE;
    else if (gbh_eq(text, "radial_ring") || gbh_eq(text, "ring") || gbh_eq(text, "circle")) *out = GBAR89_KIND_RADIAL_RING;
    else return gbh_parse_int(text, out);
    return 1;
}

static int gbh_direction(const char *text, int *out)
{
    if (gbh_eq(text, "left_to_right") || gbh_eq(text, "ltr") || gbh_eq(text, "right")) *out = GBAR89_DIR_LEFT_TO_RIGHT;
    else if (gbh_eq(text, "right_to_left") || gbh_eq(text, "rtl") || gbh_eq(text, "left")) *out = GBAR89_DIR_RIGHT_TO_LEFT;
    else if (gbh_eq(text, "top_to_bottom") || gbh_eq(text, "ttb") || gbh_eq(text, "down")) *out = GBAR89_DIR_TOP_TO_BOTTOM;
    else if (gbh_eq(text, "bottom_to_top") || gbh_eq(text, "btt") || gbh_eq(text, "up")) *out = GBAR89_DIR_BOTTOM_TO_TOP;
    else if (gbh_eq(text, "center_horizontal") || gbh_eq(text, "center_h")) *out = GBAR89_DIR_CENTER_HORIZONTAL;
    else if (gbh_eq(text, "center_vertical") || gbh_eq(text, "center_v")) *out = GBAR89_DIR_CENTER_VERTICAL;
    else return gbh_parse_int(text, out);
    return 1;
}

static int gbh_state(const char *text, int *out)
{
    static const char *names[GBAR89_STATE_COUNT] = {
        "normal", "low", "critical", "poisoned", "regenerating",
        "shielded", "overheat", "locked", "broken", "custom"
    };
    int i;
    for (i = 0; i < GBAR89_STATE_COUNT; ++i) {
        if (gbh_eq(text, names[i])) { *out = i; return 1; }
    }
    return gbh_parse_int(text, out);
}

static int gbh_pattern(const char *text, int *out)
{
    if (gbh_eq(text, "none")) *out = GBAR89_PATTERN_NONE;
    else if (gbh_eq(text, "vertical_stripes")) *out = GBAR89_PATTERN_VERTICAL_STRIPES;
    else if (gbh_eq(text, "horizontal_stripes")) *out = GBAR89_PATTERN_HORIZONTAL_STRIPES;
    else if (gbh_eq(text, "crosshatch")) *out = GBAR89_PATTERN_CROSSHATCH;
    else if (gbh_eq(text, "dots")) *out = GBAR89_PATTERN_DOTS;
    else if (gbh_eq(text, "ticks")) *out = GBAR89_PATTERN_TICKS;
    else return gbh_parse_int(text, out);
    return 1;
}

static int gbh_mask(const char *text, int *out)
{
    if (gbh_eq(text, "none")) *out = GBAR89_MASK_NONE;
    else if (gbh_eq(text, "slant_right")) *out = GBAR89_MASK_SLANT_RIGHT;
    else if (gbh_eq(text, "slant_left")) *out = GBAR89_MASK_SLANT_LEFT;
    else if (gbh_eq(text, "hexagon") || gbh_eq(text, "hex")) *out = GBAR89_MASK_HEXAGON;
    else if (gbh_eq(text, "diamond")) *out = GBAR89_MASK_DIAMOND;
    else if (gbh_eq(text, "custom_slices")) *out = GBAR89_MASK_CUSTOM_SLICES;
    else return gbh_parse_int(text, out);
    return 1;
}

static int gbh_frame(const char *text, int *out)
{
    if (gbh_eq(text, "simple")) *out = GBAR89_FRAME_SIMPLE;
    else if (gbh_eq(text, "double")) *out = GBAR89_FRAME_DOUBLE;
    else if (gbh_eq(text, "bevel_out") || gbh_eq(text, "bevel")) *out = GBAR89_FRAME_BEVEL_OUT;
    else if (gbh_eq(text, "bevel_in") || gbh_eq(text, "inset")) *out = GBAR89_FRAME_BEVEL_IN;
    else if (gbh_eq(text, "brackets")) *out = GBAR89_FRAME_BRACKETS;
    else if (gbh_eq(text, "rail")) *out = GBAR89_FRAME_RAIL;
    else if (gbh_eq(text, "pixel")) *out = GBAR89_FRAME_PIXEL;
    else if (gbh_eq(text, "none")) *out = GBAR89_FRAME_NONE;
    else return gbh_parse_int(text, out);
    return 1;
}

static int gbh_background(const char *text, int *out)
{
    if (gbh_eq(text, "solid")) *out = GBAR89_BG_SOLID;
    else if (gbh_eq(text, "grid")) *out = GBAR89_BG_GRID;
    else if (gbh_eq(text, "checker")) *out = GBAR89_BG_CHECKER;
    else if (gbh_eq(text, "scanlines")) *out = GBAR89_BG_SCANLINES;
    else if (gbh_eq(text, "diagonal")) *out = GBAR89_BG_DIAGONAL;
    else if (gbh_eq(text, "dither")) *out = GBAR89_BG_DITHER;
    else return gbh_parse_int(text, out);
    return 1;
}

static int gbh_radial_cap(const char *text, int *out)
{
    if (gbh_eq(text, "butt") || gbh_eq(text, "flat")) *out = GBAR89_RADIAL_CAP_BUTT;
    else if (gbh_eq(text, "round")) *out = GBAR89_RADIAL_CAP_ROUND;
    else if (gbh_eq(text, "square")) *out = GBAR89_RADIAL_CAP_SQUARE;
    else return gbh_parse_int(text, out);
    return 1;
}

static int gbh_ease(const char *text, int *out)
{
    if (gbh_eq(text, "linear")) *out = GBAR89_EASE_LINEAR;
    else if (gbh_eq(text, "in_quad")) *out = GBAR89_EASE_IN_QUAD;
    else if (gbh_eq(text, "out_quad")) *out = GBAR89_EASE_OUT_QUAD;
    else if (gbh_eq(text, "in_out_quad")) *out = GBAR89_EASE_IN_OUT_QUAD;
    else if (gbh_eq(text, "smoothstep")) *out = GBAR89_EASE_SMOOTHSTEP;
    else if (gbh_eq(text, "out_cubic")) *out = GBAR89_EASE_OUT_CUBIC;
    else return gbh_parse_int(text, out);
    return 1;
}

static long gbh_flag_for_name(const char *name)
{
    if (gbh_eq(name, "clamp_value")) return GBAR89_FLAG_CLAMP_VALUE;
    if (gbh_eq(name, "draw_bg")) return GBAR89_FLAG_DRAW_BG;
    if (gbh_eq(name, "draw_border")) return GBAR89_FLAG_DRAW_BORDER;
    if (gbh_eq(name, "damage_lag")) return GBAR89_FLAG_DAMAGE_LAG;
    if (gbh_eq(name, "smooth_value")) return GBAR89_FLAG_SMOOTH_VALUE;
    if (gbh_eq(name, "draw_markers")) return GBAR89_FLAG_DRAW_MARKERS;
    if (gbh_eq(name, "draw_overlay")) return GBAR89_FLAG_DRAW_OVERLAY;
    if (gbh_eq(name, "use_visual")) return GBAR89_FLAG_USE_VISUAL;
    if (gbh_eq(name, "invert_ratio")) return GBAR89_FLAG_INVERT_RATIO;
    if (gbh_eq(name, "layered")) return GBAR89_FLAG_LAYERED;
    if (gbh_eq(name, "draw_value_overlay")) return GBAR89_FLAG_DRAW_VALUE_OVERLAY;
    if (gbh_eq(name, "draw_pattern")) return GBAR89_FLAG_DRAW_PATTERN;
    if (gbh_eq(name, "use_mask")) return GBAR89_FLAG_USE_MASK;
    if (gbh_eq(name, "auto_state")) return GBAR89_FLAG_AUTO_STATE;
    if (gbh_eq(name, "state_blink")) return GBAR89_FLAG_STATE_BLINK;
    if (gbh_eq(name, "ease_value")) return GBAR89_FLAG_EASE_VALUE;
    if (gbh_eq(name, "draw_layer_pips")) return GBAR89_FLAG_DRAW_LAYER_PIPS;
    if (gbh_eq(name, "draw_mid_value")) return GBAR89_FLAG_DRAW_MID_VALUE;
    if (gbh_eq(name, "draw_vector_units")) return GBAR89_FLAG_DRAW_VECTOR_UNITS;
    if (gbh_eq(name, "pixel_quantize")) return GBAR89_FLAG_PIXEL_QUANTIZE;
    return 0L;
}

static unsigned long gbh_fx_for_name(const char *name)
{
    if (!name) return 0UL;
    if (gbh_eq(name, "outer_outline") || gbh_eq(name, "fx_outer_outline")) return GBAR89_FX_OUTER_OUTLINE;
    if (gbh_eq(name, "drop_shadow") || gbh_eq(name, "fx_drop_shadow")) return GBAR89_FX_DROP_SHADOW;
    if (gbh_eq(name, "extrude") || gbh_eq(name, "fx_extrude")) return GBAR89_FX_EXTRUDE;
    if (gbh_eq(name, "inner_shadow") || gbh_eq(name, "fx_inner_shadow")) return GBAR89_FX_INNER_SHADOW;
    if (gbh_eq(name, "gloss") || gbh_eq(name, "fx_gloss")) return GBAR89_FX_GLOSS;
    if (gbh_eq(name, "fill_grid") || gbh_eq(name, "fx_fill_grid")) return GBAR89_FX_FILL_GRID;
    if (gbh_eq(name, "fill_scanlines") || gbh_eq(name, "fx_fill_scanlines")) return GBAR89_FX_FILL_SCANLINES;
    if (gbh_eq(name, "pixel_cells") || gbh_eq(name, "fx_pixel_cells")) return GBAR89_FX_PIXEL_CELLS;
    if (gbh_eq(name, "radial_spokes") || gbh_eq(name, "fx_radial_spokes")) return GBAR89_FX_RADIAL_SPOKES;
    if (gbh_eq(name, "radial_rings") || gbh_eq(name, "fx_radial_rings")) return GBAR89_FX_RADIAL_RINGS;
    if (gbh_eq(name, "radial_ticks") || gbh_eq(name, "fx_radial_ticks")) return GBAR89_FX_RADIAL_TICKS;
    if (gbh_eq(name, "radial_sweep_highlight") || gbh_eq(name, "fx_radial_sweep_highlight")) return GBAR89_FX_RADIAL_SWEEP_HIGHLIGHT;
    return 0UL;
}

static int gbh_parse_fx_flags(const char *text, unsigned long *out)
{
    char token[48];
    const char *p;
    unsigned int ti;
    unsigned long flags;
    unsigned long bit;
    long numeric;
    if (!text || !out) return 0;
    if (gbh_parse_long(text, &numeric)) { *out = (unsigned long)numeric; return 1; }
    flags = 0UL;
    p = text;
    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '|' || *p == ',' || *p == '+') ++p;
        if (!*p) break;
        ti = 0u;
        while (*p && *p != ' ' && *p != '\t' && *p != '|' && *p != ',' && *p != '+') {
            if (ti + 1u < sizeof(token)) token[ti++] = *p;
            ++p;
        }
        token[ti] = '\0';
        if (gbh_eq(token, "none")) continue;
        bit = gbh_fx_for_name(token);
        if (bit == 0UL) return 0;
        flags |= bit;
    }
    *out = flags;
    return 1;
}

static int gbh_parse_flags(const char *text, long *out)
{
    char copy[GBH_TEXT_MAX];
    char token[48];
    unsigned int ti;
    char *p;
    long flags;
    long numeric;
    long bit;
    if (gbh_parse_long(text, &numeric)) { *out = numeric; return 1; }
    gbh_trim_copy(copy, text);
    p = copy;
    flags = 0L;
    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '|' || *p == ',' || *p == '+') ++p;
        if (!*p) break;
        ti = 0u;
        while (*p && *p != ' ' && *p != '\t' && *p != '|' && *p != ',' && *p != '+') {
            if (ti + 1u < sizeof(token)) token[ti++] = *p;
            ++p;
        }
        token[ti] = '\0';
        if (gbh_eq(token, "none")) continue;
        bit = gbh_flag_for_name(token);
        if (bit == 0L) return 0;
        flags |= bit;
    }
    *out = flags;
    return 1;
}

static int gbh_suffix_index(const char *key, const char *prefix, int limit)
{
    const char *p;
    int index;
    if (strncmp(key, prefix, strlen(prefix)) != 0) return -1;
    p = key + strlen(prefix);
    if (*p == '.' || *p == '_') ++p;
    if (!gbh_parse_int(p, &index) || index < 0 || index >= limit) return -1;
    return index;
}

static int gbh_apply_bool_flag(GBar89_Meter *m, const char *key, const char *text)
{
    long mask;
    int enabled;
    mask = gbh_flag_for_name(key);
    if (mask == 0L) return GBAR89_BIGHUD_UNKNOWN;
    if (!gbh_parse_bool(text, &enabled)) return GBAR89_BIGHUD_BAD_VALUE;
    if (enabled) m->flags |= mask;
    else m->flags &= ~mask;
    return GBAR89_BIGHUD_APPLIED;
}

static int gbh_apply_bool_fx(GBar89_Meter *m, const char *key, const char *text)
{
    unsigned long mask;
    int enabled;
    mask = gbh_fx_for_name(key);
    if (mask == 0UL) return GBAR89_BIGHUD_UNKNOWN;
    if (!gbh_parse_bool(text, &enabled)) return GBAR89_BIGHUD_BAD_VALUE;
    if (enabled) m->style.fx_flags |= mask;
    else m->style.fx_flags &= ~mask;
    return GBAR89_BIGHUD_APPLIED;
}

#define GBH_LONG_FIELD(name, field) \
    if (gbh_eq(key, name)) { if (!gbh_parse_long(text, &lv)) return GBAR89_BIGHUD_BAD_VALUE; meter->field = lv; return GBAR89_BIGHUD_APPLIED; }
#define GBH_INT_FIELD(name, field) \
    if (gbh_eq(key, name)) { if (!gbh_parse_int(text, &iv)) return GBAR89_BIGHUD_BAD_VALUE; meter->field = iv; return GBAR89_BIGHUD_APPLIED; }
#define GBH_STYLE_INT(name, field) \
    if (gbh_eq(key, name)) { if (!gbh_parse_int(text, &iv)) return GBAR89_BIGHUD_BAD_VALUE; meter->style.field = iv; return GBAR89_BIGHUD_APPLIED; }
#define GBH_STYLE_COLOR(name, field) \
    if (gbh_eq(key, name)) { if (!gbh_parse_color(text, &color)) return GBAR89_BIGHUD_BAD_VALUE; meter->style.field = color; return GBAR89_BIGHUD_APPLIED; }

int gbar89_bighud_apply_property(GBar89_Meter *meter,
                                 const char *key,
                                 const char *raw_value)
{
    char text[GBH_TEXT_MAX];
    long lv;
    long vals[GBAR89_MAX_MARKERS * 4];
    int iv;
    int count;
    int index;
    unsigned long color;
    unsigned long fx_flags;
    GBar89_Fix fix;
    int flag_result;
    GBar89_Rect *rect;

    if (!meter || !key || !raw_value) return GBAR89_BIGHUD_BAD_VALUE;
    gbh_trim_copy(text, raw_value);

    flag_result = gbh_apply_bool_flag(meter, key, text);
    if (flag_result != GBAR89_BIGHUD_UNKNOWN) return flag_result;
    flag_result = gbh_apply_bool_fx(meter, key, text);
    if (flag_result != GBAR89_BIGHUD_UNKNOWN) return flag_result;

    if (gbh_eq(key, "unit_closed")) {
        if (!gbh_parse_bool(text, &iv)) return GBAR89_BIGHUD_BAD_VALUE;
        meter->unit_closed = iv;
        return GBAR89_BIGHUD_APPLIED;
    }
    if (gbh_eq(key, "unit_filled")) {
        if (!gbh_parse_bool(text, &iv)) return GBAR89_BIGHUD_BAD_VALUE;
        meter->unit_filled = iv;
        return GBAR89_BIGHUD_APPLIED;
    }

    if (gbh_eq(key, "kind")) {
        if (!gbh_kind(text, &iv)) return GBAR89_BIGHUD_BAD_VALUE;
        meter->kind = iv;
        return GBAR89_BIGHUD_APPLIED;
    }
    if (gbh_eq(key, "direction")) {
        if (!gbh_direction(text, &iv)) return GBAR89_BIGHUD_BAD_VALUE;
        meter->direction = iv;
        return GBAR89_BIGHUD_APPLIED;
    }
    if (gbh_eq(key, "flags")) {
        if (!gbh_parse_flags(text, &lv)) return GBAR89_BIGHUD_BAD_VALUE;
        meter->flags = lv;
        return GBAR89_BIGHUD_APPLIED;
    }

    if (gbh_eq(key, "fx_flags") || gbh_eq(key, "effects")) {
        if (!gbh_parse_fx_flags(text, &fx_flags)) return GBAR89_BIGHUD_BAD_VALUE;
        meter->style.fx_flags = fx_flags;
        return GBAR89_BIGHUD_APPLIED;
    }

    GBH_LONG_FIELD("min", min_value)
    GBH_LONG_FIELD("min_value", min_value)
    GBH_LONG_FIELD("max", max_value)
    GBH_LONG_FIELD("max_value", max_value)
    GBH_LONG_FIELD("value", value)
    GBH_LONG_FIELD("visual_value", visual_value)
    GBH_LONG_FIELD("lag_value", lag_value)
    GBH_LONG_FIELD("smooth_speed", smooth_speed)
    GBH_LONG_FIELD("lag_speed", lag_speed)
    GBH_LONG_FIELD("overlay_min", overlay_min_value)
    GBH_LONG_FIELD("overlay_min_value", overlay_min_value)
    GBH_LONG_FIELD("overlay_max", overlay_max_value)
    GBH_LONG_FIELD("overlay_max_value", overlay_max_value)
    GBH_LONG_FIELD("overlay_value", overlay_value)
    GBH_LONG_FIELD("overlay_visual_value", overlay_visual_value)
    GBH_LONG_FIELD("overlay_speed", overlay_speed)
    GBH_LONG_FIELD("mid_value", mid_value)
    GBH_LONG_FIELD("mid_visual_value", mid_visual_value)
    GBH_LONG_FIELD("mid_speed", mid_speed)
    GBH_LONG_FIELD("layer_size", layer_size)
    GBH_LONG_FIELD("ease_start_value", ease_start_value)
    GBH_LONG_FIELD("ease_target_value", ease_target_value)

    GBH_INT_FIELD("x", rect.x)
    GBH_INT_FIELD("y", rect.y)
    GBH_INT_FIELD("width", rect.w)
    GBH_INT_FIELD("w", rect.w)
    GBH_INT_FIELD("height", rect.h)
    GBH_INT_FIELD("h", rect.h)
    GBH_INT_FIELD("segments", segments)
    GBH_INT_FIELD("radial_start", radial_start_deg)
    GBH_INT_FIELD("radial_start_deg", radial_start_deg)
    GBH_INT_FIELD("radial_sweep", radial_sweep_deg)
    GBH_INT_FIELD("radial_sweep_deg", radial_sweep_deg)
    GBH_INT_FIELD("radial_inner", radial_inner_percent)
    GBH_INT_FIELD("radial_inner_percent", radial_inner_percent)
    GBH_INT_FIELD("radial_steps", radial_steps)
    GBH_INT_FIELD("layer_count", layer_count)
    GBH_INT_FIELD("marker_count", marker_count)
    GBH_INT_FIELD("unit_point_count", unit_point_count)
    GBH_INT_FIELD("unit_closed", unit_closed)
    GBH_INT_FIELD("unit_filled", unit_filled)
    GBH_INT_FIELD("unit_count", unit_count)
    GBH_INT_FIELD("unit_padding", unit_padding)
    GBH_INT_FIELD("unit_scale_percent", unit_scale_percent)
    GBH_INT_FIELD("mask_amount", mask_amount)
    GBH_INT_FIELD("mask_steps", mask_steps)
    GBH_INT_FIELD("mask_slice_count", mask_slice_count)
    GBH_INT_FIELD("ease_active", ease_active)

    if (gbh_eq(key, "rect")) {
        count = gbh_parse_values(text, vals, 4);
        if (count != 4) return GBAR89_BIGHUD_BAD_VALUE;
        gbar89_set_rect(meter, (int)vals[0], (int)vals[1], (int)vals[2], (int)vals[3]);
        return GBAR89_BIGHUD_APPLIED;
    }
    if (gbh_eq(key, "range")) {
        count = gbh_parse_values(text, vals, 2);
        if (count != 2) return GBAR89_BIGHUD_BAD_VALUE;
        gbar89_set_range(meter, vals[0], vals[1]);
        return GBAR89_BIGHUD_APPLIED;
    }
    if (gbh_eq(key, "overlay_range")) {
        count = gbh_parse_values(text, vals, 2);
        if (count != 2) return GBAR89_BIGHUD_BAD_VALUE;
        gbar89_set_overlay_range(meter, vals[0], vals[1]);
        return GBAR89_BIGHUD_APPLIED;
    }
    if (gbh_eq(key, "radial_style")) {
        count = gbh_parse_values(text, vals, 5);
        if (count != 5) return GBAR89_BIGHUD_BAD_VALUE;
        gbar89_set_radial_style(meter, (int)vals[0], (int)vals[1],
                               (int)vals[2], (int)vals[3], (int)vals[4]);
        return GBAR89_BIGHUD_APPLIED;
    }
    if (gbh_eq(key, "frame_style")) {
        count = gbh_parse_values(text, vals, 4);
        if (count != 4) return GBAR89_BIGHUD_BAD_VALUE;
        meter->style.frame_kind = (int)vals[0];
        meter->style.frame_depth = (int)vals[1];
        meter->style.frame_gap = (int)vals[2];
        meter->style.frame_corner = (int)vals[3];
        return GBAR89_BIGHUD_APPLIED;
    }
    if (gbh_eq(key, "background_style")) {
        count = gbh_parse_values(text, vals, 3);
        if (count != 3) return GBAR89_BIGHUD_BAD_VALUE;
        meter->style.bg_kind = (int)vals[0];
        meter->style.bg_step = (int)vals[1];
        meter->style.bg_size = (int)vals[2];
        return GBAR89_BIGHUD_APPLIED;
    }
    if (gbh_eq(key, "shadow_offset")) {
        count = gbh_parse_values(text, vals, 2);
        if (count != 2) return GBAR89_BIGHUD_BAD_VALUE;
        meter->style.shadow_offset_x = (int)vals[0];
        meter->style.shadow_offset_y = (int)vals[1];
        return GBAR89_BIGHUD_APPLIED;
    }
    if (gbh_eq(key, "margins") || gbh_eq(key, "margin")) {
        count = gbh_parse_values(text, vals, 4);
        if (count != 4) return GBAR89_BIGHUD_BAD_VALUE;
        meter->style.margin_left = (int)vals[0];
        meter->style.margin_top = (int)vals[1];
        meter->style.margin_right = (int)vals[2];
        meter->style.margin_bottom = (int)vals[3];
        return GBAR89_BIGHUD_APPLIED;
    }
    if (gbh_eq(key, "padding")) {
        count = gbh_parse_values(text, vals, 4);
        if (count != 4) return GBAR89_BIGHUD_BAD_VALUE;
        meter->style.padding_left = (int)vals[0];
        meter->style.padding_top = (int)vals[1];
        meter->style.padding_right = (int)vals[2];
        meter->style.padding_bottom = (int)vals[3];
        return GBAR89_BIGHUD_APPLIED;
    }
    if (gbh_eq(key, "overlay_direction")) {
        if (!gbh_direction(text, &iv)) return GBAR89_BIGHUD_BAD_VALUE;
        meter->overlay_direction = iv;
        return GBAR89_BIGHUD_APPLIED;
    }
    if (gbh_eq(key, "mid_direction")) {
        if (!gbh_direction(text, &iv)) return GBAR89_BIGHUD_BAD_VALUE;
        meter->mid_direction = iv;
        return GBAR89_BIGHUD_APPLIED;
    }
    if (gbh_eq(key, "frame") || gbh_eq(key, "frame_kind")) {
        if (!gbh_frame(text, &iv)) return GBAR89_BIGHUD_BAD_VALUE;
        meter->style.frame_kind = iv;
        return GBAR89_BIGHUD_APPLIED;
    }
    if (gbh_eq(key, "background") || gbh_eq(key, "background_kind") || gbh_eq(key, "bg_kind")) {
        if (!gbh_background(text, &iv)) return GBAR89_BIGHUD_BAD_VALUE;
        meter->style.bg_kind = iv;
        return GBAR89_BIGHUD_APPLIED;
    }
    if (gbh_eq(key, "radial_cap") || gbh_eq(key, "radial_cap_kind")) {
        if (!gbh_radial_cap(text, &iv)) return GBAR89_BIGHUD_BAD_VALUE;
        meter->style.radial_cap_kind = iv;
        return GBAR89_BIGHUD_APPLIED;
    }
    if (gbh_eq(key, "state")) {
        if (!gbh_state(text, &iv)) return GBAR89_BIGHUD_BAD_VALUE;
        meter->state = iv;
        return GBAR89_BIGHUD_APPLIED;
    }
    if (gbh_eq(key, "pattern_kind")) {
        if (!gbh_pattern(text, &iv)) return GBAR89_BIGHUD_BAD_VALUE;
        meter->style.pattern_kind = iv;
        return GBAR89_BIGHUD_APPLIED;
    }
    if (gbh_eq(key, "mask_kind")) {
        if (!gbh_mask(text, &iv)) return GBAR89_BIGHUD_BAD_VALUE;
        meter->mask_kind = iv;
        return GBAR89_BIGHUD_APPLIED;
    }
    if (gbh_eq(key, "ease_kind")) {
        if (!gbh_ease(text, &iv)) return GBAR89_BIGHUD_BAD_VALUE;
        meter->ease_kind = iv;
        return GBAR89_BIGHUD_APPLIED;
    }
    if (gbh_eq(key, "low_ratio")) {
        if (!gbh_parse_fix(text, &fix)) return GBAR89_BIGHUD_BAD_VALUE;
        meter->low_ratio = fix;
        return GBAR89_BIGHUD_APPLIED;
    }
    if (gbh_eq(key, "critical_ratio")) {
        if (!gbh_parse_fix(text, &fix)) return GBAR89_BIGHUD_BAD_VALUE;
        meter->critical_ratio = fix;
        return GBAR89_BIGHUD_APPLIED;
    }
    if (gbh_eq(key, "state_timer")) {
        if (!gbh_parse_fix(text, &fix)) return GBAR89_BIGHUD_BAD_VALUE;
        meter->state_timer = fix;
        return GBAR89_BIGHUD_APPLIED;
    }
    if (gbh_eq(key, "blink_period")) {
        if (!gbh_parse_fix(text, &fix)) return GBAR89_BIGHUD_BAD_VALUE;
        meter->blink_period = fix;
        return GBAR89_BIGHUD_APPLIED;
    }
    if (gbh_eq(key, "ease_duration")) {
        if (!gbh_parse_fix(text, &fix)) return GBAR89_BIGHUD_BAD_VALUE;
        meter->ease_duration = fix;
        return GBAR89_BIGHUD_APPLIED;
    }
    if (gbh_eq(key, "ease_elapsed")) {
        if (!gbh_parse_fix(text, &fix)) return GBAR89_BIGHUD_BAD_VALUE;
        meter->ease_elapsed = fix;
        return GBAR89_BIGHUD_APPLIED;
    }
    if (gbh_eq(key, "markers")) {
        count = gbh_parse_values(text, vals, GBAR89_MAX_MARKERS);
        if (count < 0) return GBAR89_BIGHUD_BAD_VALUE;
        meter->marker_count = 0;
        for (iv = 0; iv < count; ++iv) meter->markers[meter->marker_count++] = vals[iv];
        return GBAR89_BIGHUD_APPLIED;
    }
    index = gbh_suffix_index(key, "marker", GBAR89_MAX_MARKERS);
    if (index >= 0) {
        if (!gbh_parse_long(text, &lv)) return GBAR89_BIGHUD_BAD_VALUE;
        meter->markers[index] = lv;
        if (meter->marker_count <= index) meter->marker_count = index + 1;
        return GBAR89_BIGHUD_APPLIED;
    }
    index = gbh_suffix_index(key, "mask_slice", GBAR89_MAX_MASK_SLICES);
    if (index >= 0) {
        count = gbh_parse_values(text, vals, 4);
        if (count != 4) return GBAR89_BIGHUD_BAD_VALUE;
        meter->mask_slices[index].y_percent = (int)vals[0];
        meter->mask_slices[index].h_percent = (int)vals[1];
        meter->mask_slices[index].inset_left = (int)vals[2];
        meter->mask_slices[index].inset_right = (int)vals[3];
        if (meter->mask_slice_count <= index) meter->mask_slice_count = index + 1;
        return GBAR89_BIGHUD_APPLIED;
    }

    GBH_STYLE_COLOR("color", color_fill)
    GBH_STYLE_COLOR("color_bg", color_bg)
    GBH_STYLE_COLOR("color_fill", color_fill)
    GBH_STYLE_COLOR("color_lag", color_lag)
    GBH_STYLE_COLOR("color_border", color_border)
    GBH_STYLE_COLOR("color_overlay", color_overlay)
    GBH_STYLE_COLOR("color_empty", color_empty)
    GBH_STYLE_COLOR("color_marker", color_marker)
    GBH_STYLE_COLOR("color_pattern", color_pattern)
    GBH_STYLE_COLOR("color_mid", color_mid)
    GBH_STYLE_COLOR("color_outline", color_outline)
    GBH_STYLE_COLOR("color_highlight", color_highlight)
    GBH_STYLE_COLOR("color_shadow", color_shadow)
    GBH_STYLE_COLOR("color_extrude", color_extrude)
    GBH_STYLE_COLOR("color_gloss", color_gloss)
    GBH_STYLE_COLOR("color_bg_detail", color_bg_detail)
    GBH_STYLE_COLOR("color_unit_fill", color_unit_fill)
    GBH_STYLE_COLOR("color_unit_empty", color_unit_empty)
    GBH_STYLE_COLOR("color_unit_outline", color_unit_outline)

    index = gbh_suffix_index(key, "color_state_fill", GBAR89_STATE_COUNT);
    if (index >= 0) {
        if (!gbh_parse_color(text, &color)) return GBAR89_BIGHUD_BAD_VALUE;
        meter->style.color_state_fill[index] = color;
        return GBAR89_BIGHUD_APPLIED;
    }
    index = gbh_suffix_index(key, "color_state_border", GBAR89_STATE_COUNT);
    if (index >= 0) {
        if (!gbh_parse_color(text, &color)) return GBAR89_BIGHUD_BAD_VALUE;
        meter->style.color_state_border[index] = color;
        return GBAR89_BIGHUD_APPLIED;
    }
    index = gbh_suffix_index(key, "color_layer_fill", GBAR89_MAX_LAYERS);
    if (index >= 0) {
        if (!gbh_parse_color(text, &color)) return GBAR89_BIGHUD_BAD_VALUE;
        meter->style.color_layer_fill[index] = color;
        return GBAR89_BIGHUD_APPLIED;
    }
    index = gbh_suffix_index(key, "color_layer_lag", GBAR89_MAX_LAYERS);
    if (index >= 0) {
        if (!gbh_parse_color(text, &color)) return GBAR89_BIGHUD_BAD_VALUE;
        meter->style.color_layer_lag[index] = color;
        return GBAR89_BIGHUD_APPLIED;
    }

    GBH_STYLE_INT("sprite_bg", sprite_bg)
    GBH_STYLE_INT("sprite_fill", sprite_fill)
    GBH_STYLE_INT("sprite_lag", sprite_lag)
    GBH_STYLE_INT("sprite_overlay", sprite_overlay)
    GBH_STYLE_INT("sprite_empty", sprite_empty)
    GBH_STYLE_INT("sprite_full", sprite_full)
    GBH_STYLE_INT("sprite_partial", sprite_partial)
    GBH_STYLE_INT("margin_left", margin_left)
    GBH_STYLE_INT("margin_top", margin_top)
    GBH_STYLE_INT("margin_right", margin_right)
    GBH_STYLE_INT("margin_bottom", margin_bottom)
    GBH_STYLE_INT("padding_left", padding_left)
    GBH_STYLE_INT("padding_top", padding_top)
    GBH_STYLE_INT("padding_right", padding_right)
    GBH_STYLE_INT("padding_bottom", padding_bottom)
    GBH_STYLE_INT("border_size", border_size)
    GBH_STYLE_INT("segment_gap", segment_gap)
    GBH_STYLE_INT("marker_size", marker_size)
    GBH_STYLE_INT("pattern_step", pattern_step)
    GBH_STYLE_INT("pattern_size", pattern_size)
    GBH_STYLE_INT("frame_depth", frame_depth)
    GBH_STYLE_INT("frame_gap", frame_gap)
    GBH_STYLE_INT("frame_corner", frame_corner)
    GBH_STYLE_INT("outline_size", outline_size)
    GBH_STYLE_INT("shadow_offset_x", shadow_offset_x)
    GBH_STYLE_INT("shadow_offset_y", shadow_offset_y)
    GBH_STYLE_INT("extrude_depth", extrude_depth)
    GBH_STYLE_INT("bg_step", bg_step)
    GBH_STYLE_INT("background_step", bg_step)
    GBH_STYLE_INT("bg_size", bg_size)
    GBH_STYLE_INT("background_size", bg_size)
    GBH_STYLE_INT("pixel_size", pixel_size)
    GBH_STYLE_INT("gloss_percent", gloss_percent)
    GBH_STYLE_INT("radial_segments", radial_segments)
    GBH_STYLE_INT("radial_gap", radial_gap_deg)
    GBH_STYLE_INT("radial_gap_deg", radial_gap_deg)
    GBH_STYLE_INT("radial_detail_rings", radial_detail_rings)
    GBH_STYLE_INT("radial_unit_radius", radial_unit_radius_percent)
    GBH_STYLE_INT("radial_unit_radius_percent", radial_unit_radius_percent)
    GBH_STYLE_INT("radial_marker_length", radial_marker_length_percent)
    GBH_STYLE_INT("radial_marker_length_percent", radial_marker_length_percent)
    GBH_STYLE_INT("radial_phase", radial_phase_deg)
    GBH_STYLE_INT("radial_phase_deg", radial_phase_deg)

    rect = (GBar89_Rect *)0;
    if (gbh_eq(key, "src_bg")) rect = &meter->style.src_bg;
    else if (gbh_eq(key, "src_fill")) rect = &meter->style.src_fill;
    else if (gbh_eq(key, "src_lag")) rect = &meter->style.src_lag;
    else if (gbh_eq(key, "src_overlay")) rect = &meter->style.src_overlay;
    else if (gbh_eq(key, "src_empty")) rect = &meter->style.src_empty;
    else if (gbh_eq(key, "src_full")) rect = &meter->style.src_full;
    else if (gbh_eq(key, "src_partial")) rect = &meter->style.src_partial;
    if (rect) {
        count = gbh_parse_values(text, vals, 4);
        if (count != 4) return GBAR89_BIGHUD_BAD_VALUE;
        rect->x = (int)vals[0]; rect->y = (int)vals[1];
        rect->w = (int)vals[2]; rect->h = (int)vals[3];
        return GBAR89_BIGHUD_APPLIED;
    }

    return GBAR89_BIGHUD_UNKNOWN;
}
