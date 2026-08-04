#include "blank3d_bighud.h"
#include "gbar89_bighud.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* BigVaderHudder is intentionally static: its parser arena is large and the
 * game owns one HUD document. This keeps the engine heap-free and avoids a
 * megabyte-sized parser object inside gameplay structs or on the stack. */
static BVH_Context b3d_bighud_context;

typedef struct B3DBigHudBuildTag {
    Blank3DBigHud *hud;
    Blank3DEcgVitals *ecg;
    Blank3DBigHudNode *current;
    int failed;
    char error[B3D_BIGHUD_ERROR_CAP];
} B3DBigHudBuild;

static int b3d_text_eq(const char *a, const char *b)
{
    int ca;
    int cb;
    if (!a || !b) return 0;
    while (*a && *b) {
        ca = (unsigned char)*a++;
        cb = (unsigned char)*b++;
        if (ca >= 'A' && ca <= 'Z') ca += 'a' - 'A';
        if (cb >= 'A' && cb <= 'Z') cb += 'a' - 'A';
        if (ca != cb) return 0;
    }
    return *a == '\0' && *b == '\0';
}

static void b3d_copy(char *dst, unsigned int cap, const char *src)
{
    unsigned int i;
    if (!dst || cap == 0u) return;
    if (!src) src = "";
    i = 0u;
    while (src[i] && i + 1u < cap) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static const char *b3d_trim_raw(const char *raw, char *out, unsigned int cap)
{
    const char *begin;
    const char *end;
    unsigned int len;
    if (!out || cap == 0u) return "";
    if (!raw) raw = "";
    begin = raw;
    while (*begin == ' ' || *begin == '\t' || *begin == '\r' || *begin == '\n') begin++;
    end = begin + strlen(begin);
    while (end > begin && (end[-1] == ' ' || end[-1] == '\t' ||
           end[-1] == '\r' || end[-1] == '\n')) end--;
    if (end > begin + 1 && ((*begin == '"' && end[-1] == '"') ||
                            (*begin == '\'' && end[-1] == '\''))) {
        begin++;
        end--;
    }
    len = (unsigned int)(end - begin);
    if (len + 1u > cap) len = cap - 1u;
    if (len > 0u) memcpy(out, begin, len);
    out[len] = '\0';
    return out;
}

static int b3d_parse_long(const char *text, long *out)
{
    char *end;
    long v;
    if (!text || !out) return 0;
    v = strtol(text, &end, 0);
    while (*end == ' ' || *end == '\t') end++;
    if (*end != '\0') return 0;
    *out = v;
    return 1;
}

static int b3d_parse_q8(const char *text, int *out)
{
    const char *p;
    long whole;
    long frac;
    long denom;
    int sign;
    if (!text || !out) return 0;
    p = text;
    sign = 1;
    if (*p == '-') { sign = -1; ++p; }
    else if (*p == '+') ++p;
    if (!isdigit((unsigned char)*p) && *p != '.') return 0;
    whole = 0L;
    while (isdigit((unsigned char)*p)) { whole = whole * 10L + (*p - '0'); ++p; }
    frac = 0L;
    denom = 1L;
    if (*p == '.') {
        ++p;
        while (isdigit((unsigned char)*p) && denom < 1000000L) {
            frac = frac * 10L + (*p - '0');
            denom *= 10L;
            ++p;
        }
        while (isdigit((unsigned char)*p)) ++p;
    }
    while (*p == ' ' || *p == '\t') ++p;
    if (*p != '\0') return 0;
    *out = (int)(whole * 256L + (frac * 256L) / denom);
    if (sign < 0) *out = -*out;
    return 1;
}

static int b3d_parse_bool(const char *text, int *out)
{
    long v;
    if (b3d_text_eq(text, "true") || b3d_text_eq(text, "yes") ||
        b3d_text_eq(text, "on")) { *out = 1; return 1; }
    if (b3d_text_eq(text, "false") || b3d_text_eq(text, "no") ||
        b3d_text_eq(text, "off")) { *out = 0; return 1; }
    if (!b3d_parse_long(text, &v)) return 0;
    *out = v != 0L;
    return 1;
}

static int b3d_hex_nibble(int ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return 10 + ch - 'a';
    if (ch >= 'A' && ch <= 'F') return 10 + ch - 'A';
    return -1;
}

static int b3d_parse_color(const char *text, unsigned long *out)
{
    unsigned long value;
    unsigned int digits;
    int n;
    long signed_value;
    if (!text || !out) return 0;
    if (b3d_text_eq(text, "white")) { *out = 0xFFFFFFFFUL; return 1; }
    if (b3d_text_eq(text, "black")) { *out = 0x000000FFUL; return 1; }
    if (b3d_text_eq(text, "red")) { *out = 0xFF0000FFUL; return 1; }
    if (b3d_text_eq(text, "green")) { *out = 0x00FF00FFUL; return 1; }
    if (b3d_text_eq(text, "blue")) { *out = 0x0000FFFFUL; return 1; }
    if (b3d_text_eq(text, "yellow")) { *out = 0xFFFF00FFUL; return 1; }
    if (b3d_text_eq(text, "orange")) { *out = 0xFF8000FFUL; return 1; }
    if (b3d_text_eq(text, "cyan")) { *out = 0x00FFFFFFUL; return 1; }
    if (b3d_text_eq(text, "magenta")) { *out = 0xFF00FFFFUL; return 1; }
    if (*text != '#') {
        if (!b3d_parse_long(text, &signed_value)) return 0;
        *out = (unsigned long)signed_value;
        return 1;
    }
    text++;
    value = 0UL;
    digits = 0u;
    while (*text && digits < 8u) {
        n = b3d_hex_nibble((unsigned char)*text++);
        if (n < 0) return 0;
        value = (value << 4) | (unsigned long)n;
        digits++;
    }
    if (*text != '\0' || (digits != 6u && digits != 8u)) return 0;
    if (digits == 6u) value = (value << 8) | 0xFFUL;
    *out = value;
    return 1;
}

static int b3d_parse_unit_points(const char *text,
                                 GBar89_VectorPoint *points,
                                 int capacity)
{
    const char *p;
    char *end;
    long x;
    long y;
    int count;
    if (!text || !points || capacity < 1) return 0;
    p = text;
    count = 0;
    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '|' || *p == ';') ++p;
        if (!*p) break;
        if (count >= capacity) return -1;
        x = strtol(p, &end, 10);
        if (end == p) return -1;
        p = end;
        while (*p == ' ' || *p == '\t') ++p;
        if (*p != ',' && *p != ':') return -1;
        ++p;
        while (*p == ' ' || *p == '\t') ++p;
        y = strtol(p, &end, 10);
        if (end == p) return -1;
        p = end;
        if (x < 0L || x > 1000L || y < 0L || y > 1000L) return -1;
        points[count].x = (int)x;
        points[count].y = (int)y;
        ++count;
        while (*p == ' ' || *p == '\t') ++p;
        if (*p && *p != '|' && *p != ';') return -1;
    }
    return count;
}

static int b3d_copy_unit_shape(Blank3DBigHudNode *node, const char *name)
{
    static const GBar89_VectorPoint diamond[] = {
        {500, 0}, {1000, 500}, {500, 1000}, {0, 500}
    };
    static const GBar89_VectorPoint bullet[] = {
        {200, 80}, {650, 80}, {900, 300}, {900, 700},
        {650, 920}, {200, 920}, {80, 700}, {80, 300}
    };
    static const GBar89_VectorPoint heart[] = {
        {500, 1000}, {70, 570}, {70, 260}, {230, 80},
        {500, 240}, {770, 80}, {930, 260}, {930, 570}
    };
    static const GBar89_VectorPoint shield[] = {
        {500, 0}, {900, 150}, {850, 650}, {500, 1000},
        {150, 650}, {100, 150}
    };
    static const GBar89_VectorPoint hexagon[] = {
        {250, 0}, {750, 0}, {1000, 500},
        {750, 1000}, {250, 1000}, {0, 500}
    };
    static const GBar89_VectorPoint chevron[] = {
        {0, 150}, {500, 500}, {0, 850},
        {300, 850}, {800, 500}, {300, 150}
    };
    static const GBar89_VectorPoint battery[] = {
        {0, 180}, {820, 180}, {820, 350}, {1000, 350},
        {1000, 650}, {820, 650}, {820, 820}, {0, 820}
    };
    const GBar89_VectorPoint *src;
    int count;
    int i;
    if (!node || !name) return 0;
    src = (const GBar89_VectorPoint *)0;
    count = 0;
    if (b3d_text_eq(name, "diamond")) { src = diamond; count = 4; }
    else if (b3d_text_eq(name, "bullet") || b3d_text_eq(name, "cartridge")) { src = bullet; count = 8; }
    else if (b3d_text_eq(name, "heart")) { src = heart; count = 8; }
    else if (b3d_text_eq(name, "shield")) { src = shield; count = 6; }
    else if (b3d_text_eq(name, "hex") || b3d_text_eq(name, "hexagon")) { src = hexagon; count = 6; }
    else if (b3d_text_eq(name, "chevron")) { src = chevron; count = 6; }
    else if (b3d_text_eq(name, "battery")) { src = battery; count = 8; }
    else if (b3d_text_eq(name, "none")) {
        node->unit_point_count = 0;
        node->meter.unit_points = (const GBar89_VectorPoint *)0;
        node->meter.unit_point_count = 0;
        return 1;
    } else return 0;
    for (i = 0; i < count; ++i) node->unit_points[i] = src[i];
    node->unit_point_count = count;
    node->meter.unit_points = node->unit_points;
    node->meter.unit_point_count = count;
    return 1;
}

static void b3d_rebind_unit_points(Blank3DBigHudNode *node)
{
    if (!node) return;
    if (node->unit_point_count > 0) node->meter.unit_points = node->unit_points;
    else node->meter.unit_points = (const GBar89_VectorPoint *)0;
}

static void b3d_node_default(Blank3DBigHudNode *node, const char *name)
{
    if (!node) return;
    memset(node, 0, sizeof(*node));
    node->enabled = 1;
    node->anchor = BVH_ANCHOR_TOP_LEFT;
    node->width = 240;
    node->height = 16;
    node->z = 0;
    node->ecg_scale_x_q8 = 256;
    node->ecg_scale_y_q8 = 256;
    node->ecg_content_alpha = 255u;
    node->ecg_background_alpha = 96u;
    node->ecg_clear_alpha = 0u;
    node->counter_mode = B3D_BIGHUD_COUNTER_VALUE;
    node->counter_scale = 2;
    node->counter_pad = 0;
    node->counter_pad2 = 0;
    node->counter_gap = 1;
    node->counter_shadow = 1;
    node->counter_background = 0;
    node->counter_border = 0;
    node->counter_color = 0xFFFFFFFFUL;
    node->counter_shadow_color = 0x000000C0UL;
    node->counter_background_color = 0x000000A0UL;
    node->counter_border_color = 0xFFFFFFFFUL;
    b3d_copy(node->counter_separator, B3D_BIGHUD_TEXT_CAP, "/");
    b3d_copy(node->name, B3D_BIGHUD_NAME_CAP, name);
    gbar89_init(&node->meter);
}

void blank3d_bighud_init(Blank3DBigHud *hud)
{
    if (!hud) return;
    memset(hud, 0, sizeof(*hud));
    hud->enabled = 1;
    hud->canvas_width = 1;
    hud->canvas_height = 1;
}

static int b3d_node_type(const char *text)
{
    if (b3d_text_eq(text, "bar") || b3d_text_eq(text, "meter") ||
        b3d_text_eq(text, "gbar89")) return B3D_BIGHUD_NODE_BAR;
    if (b3d_text_eq(text, "ecg") || b3d_text_eq(text, "waveform") ||
        b3d_text_eq(text, "vitals")) return B3D_BIGHUD_NODE_ECG;
    if (b3d_text_eq(text, "counter") || b3d_text_eq(text, "text") ||
        b3d_text_eq(text, "number")) return B3D_BIGHUD_NODE_COUNTER;
    return B3D_BIGHUD_NODE_NONE;
}

static void b3d_build_error(B3DBigHudBuild *build, const char *text)
{
    if (!build || build->failed) return;
    build->failed = 1;
    b3d_copy(build->error, B3D_BIGHUD_ERROR_CAP, text);
}

static void b3d_begin(void *user, int entity_kind, const char *name)
{
    B3DBigHudBuild *build;
    Blank3DBigHudNode *node;
    build = (B3DBigHudBuild *)user;
    if (!build || !build->hud) return;
    if (entity_kind == BVH_ENTITY_NODE) {
        if (build->hud->node_count >= B3D_BIGHUD_MAX_NODES) {
            b3d_build_error(build, "too many .bighud nodes");
            build->current = (Blank3DBigHudNode *)0;
            return;
        }
        node = &build->hud->nodes[build->hud->node_count++];
        b3d_node_default(node, name);
        build->current = node;
    }
}

static void b3d_end(void *user, int entity_kind)
{
    B3DBigHudBuild *build;
    build = (B3DBigHudBuild *)user;
    if (!build) return;
    if (entity_kind == BVH_ENTITY_NODE) build->current = (Blank3DBigHudNode *)0;
}

static int b3d_apply_counter(Blank3DBigHudNode *node,
                             const char *key,
                             const char *text)
{
    long number;
    int boolean_value;
    unsigned long color;
    if (!node || !key || !text) return 0;
    if (b3d_text_eq(key, "mode")) {
        if (b3d_text_eq(text, "pair")) node->counter_mode = B3D_BIGHUD_COUNTER_PAIR;
        else if (b3d_text_eq(text, "value")) node->counter_mode = B3D_BIGHUD_COUNTER_VALUE;
        else return -1;
        return 1;
    }
#define B3D_COUNTER_INT(prop, field) \
    if (b3d_text_eq(key, prop)) { if (!b3d_parse_long(text, &number)) return -1; node->field = (int)number; return 1; }
#define B3D_COUNTER_BOOL(prop, field) \
    if (b3d_text_eq(key, prop)) { if (!b3d_parse_bool(text, &boolean_value)) return -1; node->field = boolean_value; return 1; }
#define B3D_COUNTER_COLOR(prop, field) \
    if (b3d_text_eq(key, prop)) { if (!b3d_parse_color(text, &color)) return -1; node->field = color; return 1; }
    B3D_COUNTER_INT("scale", counter_scale)
    B3D_COUNTER_INT("pad", counter_pad)
    B3D_COUNTER_INT("pad2", counter_pad2)
    B3D_COUNTER_INT("glyph_gap", counter_gap)
    B3D_COUNTER_BOOL("shadow", counter_shadow)
    B3D_COUNTER_BOOL("background", counter_background)
    B3D_COUNTER_BOOL("border", counter_border)
    B3D_COUNTER_COLOR("color", counter_color)
    B3D_COUNTER_COLOR("shadow_color", counter_shadow_color)
    B3D_COUNTER_COLOR("background_color", counter_background_color)
    B3D_COUNTER_COLOR("border_color", counter_border_color)
#undef B3D_COUNTER_INT
#undef B3D_COUNTER_BOOL
#undef B3D_COUNTER_COLOR
    if (b3d_text_eq(key, "label")) { b3d_copy(node->counter_label, B3D_BIGHUD_TEXT_CAP, text); return 1; }
    if (b3d_text_eq(key, "prefix")) { b3d_copy(node->counter_prefix, B3D_BIGHUD_TEXT_CAP, text); return 1; }
    if (b3d_text_eq(key, "separator")) { b3d_copy(node->counter_separator, B3D_BIGHUD_TEXT_CAP, text); return 1; }
    if (b3d_text_eq(key, "suffix")) { b3d_copy(node->counter_suffix, B3D_BIGHUD_TEXT_CAP, text); return 1; }
    return 0;
}

static void b3d_prop(void *user, const char *key, const char *raw,
                     const BVH_Value *value)
{
    B3DBigHudBuild *build;
    Blank3DBigHudNode *node;
    char text[BVH_VALUE_TEXT_MAX + 1];
    long number;
    int boolean_value;
    int result;
    build = (B3DBigHudBuild *)user;
    if (!build || !build->hud || !key) return;
    b3d_trim_raw(raw, text, sizeof(text));

    if (!build->current) {
        if (b3d_text_eq(key, "enabled")) {
            if (!b3d_parse_bool(text, &boolean_value))
                b3d_build_error(build, "bad HUD enabled value");
            else build->hud->enabled = boolean_value;
        } else if (b3d_text_eq(key, "canvas")) {
            if (value && value->kind == BVH_VALUE_VEC2I &&
                value->a > 0 && value->b > 0) {
                build->hud->canvas_width = (int)value->a;
                build->hud->canvas_height = (int)value->b;
            } else b3d_build_error(build, "canvas must be width,height");
        } else if (b3d_text_eq(key, "canvas_width")) {
            if (!b3d_parse_long(text, &number) || number < 1L)
                b3d_build_error(build, "bad HUD canvas_width");
            else build->hud->canvas_width = (int)number;
        } else if (b3d_text_eq(key, "canvas_height")) {
            if (!b3d_parse_long(text, &number) || number < 1L)
                b3d_build_error(build, "bad HUD canvas_height");
            else build->hud->canvas_height = (int)number;
        }
        return;
    }
    node = build->current;
    if (b3d_text_eq(key, "type")) {
        node->type = b3d_node_type(text);
        if (node->type == B3D_BIGHUD_NODE_NONE) b3d_build_error(build, "unknown HUD node type");
        return;
    }
    if (b3d_text_eq(key, "visible") || b3d_text_eq(key, "enabled")) {
        if (!b3d_parse_bool(text, &node->enabled)) b3d_build_error(build, "bad node visible value");
        return;
    }
    if (b3d_text_eq(key, "bind")) { b3d_copy(node->bind, B3D_BIGHUD_BIND_CAP, text); return; }
    if (b3d_text_eq(key, "bind2") || b3d_text_eq(key, "secondary_bind")) { b3d_copy(node->bind2, B3D_BIGHUD_BIND_CAP, text); return; }
    if (b3d_text_eq(key, "min_bind")) { b3d_copy(node->min_bind, B3D_BIGHUD_BIND_CAP, text); return; }
    if (b3d_text_eq(key, "max_bind")) { b3d_copy(node->max_bind, B3D_BIGHUD_BIND_CAP, text); return; }
    if (b3d_text_eq(key, "overlay_bind")) { b3d_copy(node->overlay_bind, B3D_BIGHUD_BIND_CAP, text); return; }
    if (b3d_text_eq(key, "overlay_min_bind")) { b3d_copy(node->overlay_min_bind, B3D_BIGHUD_BIND_CAP, text); return; }
    if (b3d_text_eq(key, "overlay_max_bind")) { b3d_copy(node->overlay_max_bind, B3D_BIGHUD_BIND_CAP, text); return; }
    if (b3d_text_eq(key, "mid_bind")) { b3d_copy(node->mid_bind, B3D_BIGHUD_BIND_CAP, text); return; }
    if (b3d_text_eq(key, "segments_bind")) { b3d_copy(node->segments_bind, B3D_BIGHUD_BIND_CAP, text); return; }
    if (b3d_text_eq(key, "state_bind")) { b3d_copy(node->state_bind, B3D_BIGHUD_BIND_CAP, text); return; }
    if (b3d_text_eq(key, "radial_phase_bind")) { b3d_copy(node->radial_phase_bind, B3D_BIGHUD_BIND_CAP, text); return; }
    if (b3d_text_eq(key, "unit_count_bind")) { b3d_copy(node->unit_count_bind, B3D_BIGHUD_BIND_CAP, text); return; }
    if (b3d_text_eq(key, "layer_count_bind")) { b3d_copy(node->layer_count_bind, B3D_BIGHUD_BIND_CAP, text); return; }
    if (b3d_text_eq(key, "layer_size_bind")) { b3d_copy(node->layer_size_bind, B3D_BIGHUD_BIND_CAP, text); return; }
    if (b3d_text_eq(key, "z")) {
        if (!b3d_parse_long(text, &number)) b3d_build_error(build, "bad node z value");
        else node->z = (int)number;
        return;
    }
    if (b3d_text_eq(key, "pos")) {
        if (value && value->kind == BVH_VALUE_ANCHOR_POS) {
            node->anchor = (int)value->a;
            node->offset_x = (int)value->b;
            node->offset_y = (int)value->c;
            node->has_pos = 1;
        } else b3d_build_error(build, "pos must use an anchor and optional offset");
        return;
    }
    if (b3d_text_eq(key, "size")) {
        if (value && value->kind == BVH_VALUE_VEC2I) {
            node->width = (int)value->a;
            node->height = (int)value->b;
            node->has_size = 1;
        } else b3d_build_error(build, "size must be width,height");
        return;
    }
    if (b3d_text_eq(key, "screen_scale")) {
        if (!b3d_parse_q8(text, &node->ecg_scale_x_q8) || node->ecg_scale_x_q8 <= 0)
            b3d_build_error(build, "bad ECG screen_scale");
        else node->ecg_scale_y_q8 = node->ecg_scale_x_q8;
        return;
    }
    if (b3d_text_eq(key, "screen_scale_x_q8")) {
        if (!b3d_parse_long(text, &number)) b3d_build_error(build, "bad ECG screen_scale_x_q8");
        else node->ecg_scale_x_q8 = (int)number;
        return;
    }
    if (b3d_text_eq(key, "screen_scale_y_q8")) {
        if (!b3d_parse_long(text, &number)) b3d_build_error(build, "bad ECG screen_scale_y_q8");
        else node->ecg_scale_y_q8 = (int)number;
        return;
    }
    if (b3d_text_eq(key, "content_alpha") ||
        b3d_text_eq(key, "ecg_content_alpha")) {
        if (!b3d_parse_long(text, &number) || number < 0L || number > 255L)
            b3d_build_error(build, "bad ECG content_alpha");
        else node->ecg_content_alpha = (unsigned int)number;
        return;
    }
    if (b3d_text_eq(key, "background_alpha") ||
        b3d_text_eq(key, "ecg_background_alpha")) {
        if (!b3d_parse_long(text, &number) || number < 0L || number > 255L)
            b3d_build_error(build, "bad ECG background_alpha");
        else node->ecg_background_alpha = (unsigned int)number;
        return;
    }
    if (b3d_text_eq(key, "clear_alpha") ||
        b3d_text_eq(key, "ecg_clear_alpha")) {
        if (!b3d_parse_long(text, &number) || number < 0L || number > 255L)
            b3d_build_error(build, "bad ECG clear_alpha");
        else node->ecg_clear_alpha = (unsigned int)number;
        return;
    }

    if (node->type == B3D_BIGHUD_NODE_BAR) {
        if (b3d_text_eq(key, "unit_shape") || b3d_text_eq(key, "vector_shape")) {
            if (!b3d_copy_unit_shape(node, text)) b3d_build_error(build, "bad GBar89 unit_shape");
            return;
        }
        if (b3d_text_eq(key, "unit_points") || b3d_text_eq(key, "vector_points")) {
            result = b3d_parse_unit_points(text, node->unit_points, GBAR89_MAX_VECTOR_POINTS);
            if (result < 2) b3d_build_error(build, "bad GBar89 unit_points");
            else {
                node->unit_point_count = result;
                node->meter.unit_points = node->unit_points;
                node->meter.unit_point_count = result;
            }
            return;
        }
        if (b3d_text_eq(key, "unit_point_count")) {
            if (!b3d_parse_long(text, &number) || number < 0L ||
                number > (long)node->unit_point_count)
                b3d_build_error(build, "bad GBar89 unit_point_count");
            else node->meter.unit_point_count = (int)number;
            return;
        }
        if (b3d_text_eq(key, "radial_phase_speed") ||
            b3d_text_eq(key, "radial_phase_speed_deg_per_sec")) {
            if (!b3d_parse_long(text, &number) || number < -36000L || number > 36000L)
                b3d_build_error(build, "bad GBar89 radial_phase_speed");
            else node->radial_phase_speed_deg_per_sec = (int)number;
            return;
        }
        result = gbar89_bighud_apply_property(&node->meter, key, text);
        if (result == GBAR89_BIGHUD_BAD_VALUE) {
            char message[B3D_BIGHUD_ERROR_CAP];
            sprintf(message, "bad GBar89 property %.96s=%.96s", key, text);
            b3d_build_error(build, message);
        } else if (result == GBAR89_BIGHUD_UNKNOWN) {
            char message[B3D_BIGHUD_ERROR_CAP];
            sprintf(message, "unknown GBar89 property: %.180s", key);
            b3d_build_error(build, message);
        }
        return;
    }
    if (node->type == B3D_BIGHUD_NODE_ECG) {
        result = blank3d_ecg_vitals_apply_bighud_property(build->ecg, key, text);
        if (result < 0) b3d_build_error(build, "bad ECG property value");
        return;
    }
    if (node->type == B3D_BIGHUD_NODE_COUNTER) {
        result = b3d_apply_counter(node, key, text);
        if (result < 0) b3d_build_error(build, "bad counter property value");
        return;
    }
}

void blank3d_bighud_sort(Blank3DBigHud *hud)
{
    int i;
    int j;
    Blank3DBigHudNode temp;
    if (!hud) return;
    for (i = 1; i < hud->node_count; ++i) {
        temp = hud->nodes[i];
        j = i - 1;
        while (j >= 0 && hud->nodes[j].z > temp.z) {
            hud->nodes[j + 1] = hud->nodes[j];
            j--;
        }
        hud->nodes[j + 1] = temp;
    }
    for (i = 0; i < hud->node_count; ++i) b3d_rebind_unit_points(&hud->nodes[i]);
}

static Blank3DBigHudNode *b3d_add_fallback(Blank3DBigHud *hud,
                                           const char *name,
                                           int type)
{
    Blank3DBigHudNode *node;
    if (!hud || hud->node_count >= B3D_BIGHUD_MAX_NODES) return (Blank3DBigHudNode *)0;
    node = &hud->nodes[hud->node_count++];
    b3d_node_default(node, name);
    node->type = type;
    return node;
}

void blank3d_bighud_install_fallback(Blank3DBigHud *hud,
                                     Blank3DEcgVitals *ecg)
{
    Blank3DBigHudNode *node;
    if (!hud || !ecg) return;
    blank3d_ecg_vitals_init(ecg);
    hud->node_count = 0;

    node = b3d_add_fallback(hud, "vitals_ecg", B3D_BIGHUD_NODE_ECG);
    if (node) {
        node->anchor = BVH_ANCHOR_TOP_LEFT;
        node->offset_x = 20;
        node->offset_y = 18;
        node->has_pos = 1;
        node->width = B3D_ECG_WIDTH;
        node->height = B3D_ECG_HEIGHT;
        node->has_size = 1;
    }

    node = b3d_add_fallback(hud, "ammo_bar", B3D_BIGHUD_NODE_BAR);
    if (node) {
        node->anchor = BVH_ANCHOR_TOP_LEFT;
        node->offset_x = 24;
        node->offset_y = 144;
        node->has_pos = 1;
        node->width = 240;
        node->height = 14;
        node->has_size = 1;
        b3d_copy(node->bind, B3D_BIGHUD_BIND_CAP, "weapon.loaded");
        b3d_copy(node->min_bind, B3D_BIGHUD_BIND_CAP, "zero");
        b3d_copy(node->max_bind, B3D_BIGHUD_BIND_CAP, "weapon.capacity");
        b3d_copy(node->segments_bind, B3D_BIGHUD_BIND_CAP, "weapon.capacity");
        gbar89_set_kind(&node->meter, GBAR89_KIND_SEGMENTED);
        gbar89_set_segments(&node->meter, 15, 2);
        node->meter.style.color_fill = 0xE0C040FFUL;
        node->meter.style.color_lag = 0xA08030FFUL;
        node->meter.style.color_empty = 0x302A18D0UL;
    }

    node = b3d_add_fallback(hud, "ammo_counter", B3D_BIGHUD_NODE_COUNTER);
    if (node) {
        node->anchor = BVH_ANCHOR_TOP_LEFT;
        node->offset_x = 24;
        node->offset_y = 166;
        node->has_pos = 1;
        b3d_copy(node->bind, B3D_BIGHUD_BIND_CAP, "weapon.loaded");
        b3d_copy(node->bind2, B3D_BIGHUD_BIND_CAP, "weapon.reserve");
        b3d_copy(node->counter_label, B3D_BIGHUD_TEXT_CAP, "AMMO ");
        node->counter_mode = B3D_BIGHUD_COUNTER_PAIR;
        node->counter_scale = 2;
        node->counter_color = 0xE8D070FFUL;
    }
    blank3d_bighud_sort(hud);
}

int blank3d_bighud_load(Blank3DBigHud *hud,
                        Blank3DEcgVitals *ecg,
                        const char *path)
{
    BVH_RuntimeCallbacks callbacks;
    BVH_Error error;
    B3DBigHudBuild build;
    int ok;
    if (!hud || !ecg || !path) return 0;
    blank3d_bighud_init(hud);
    b3d_copy(hud->source_path, B3D_BIGHUD_PATH_CAP, path);
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.on_begin = b3d_begin;
    callbacks.on_end = b3d_end;
    callbacks.on_prop = b3d_prop;
    memset(&build, 0, sizeof(build));
    build.hud = hud;
    build.ecg = ecg;
    memset(&error, 0, sizeof(error));
    bvh_context_init(&b3d_bighud_context);
    ok = bvh_run_file(&b3d_bighud_context, path, &callbacks, &build, &error);
    if (!ok || build.failed || hud->node_count == 0) {
        if (build.failed) b3d_copy(hud->last_error, B3D_BIGHUD_ERROR_CAP, build.error);
        else if (!ok) {
            sprintf(hud->last_error, "%.170s:%d:%d: %.60s", path,
                    error.line, error.col, error.message);
        } else b3d_copy(hud->last_error, B3D_BIGHUD_ERROR_CAP, "HUD document has no nodes");
        blank3d_bighud_install_fallback(hud, ecg);
        hud->loaded_from_file = 0;
        return 0;
    }
    blank3d_bighud_sort(hud);
    hud->loaded_from_file = 1;
    return 1;
}

long blank3d_bighud_resolve_binding(const Blank3DBigHudTelemetry *t,
                                    const char *name,
                                    long fallback)
{
    long literal;
    if (!name || !*name) return fallback;
    if (b3d_parse_long(name, &literal)) return literal;
    if (b3d_text_eq(name, "zero")) return 0L;
    if (b3d_text_eq(name, "one")) return 1L;
    if (!t) return fallback;
    if (b3d_text_eq(name, "player.health")) return t->player_health;
    if (b3d_text_eq(name, "player.health_max")) return t->player_health_max;
    if (b3d_text_eq(name, "weapon.loaded")) return t->weapon_loaded;
    if (b3d_text_eq(name, "weapon.capacity")) return t->weapon_capacity;
    if (b3d_text_eq(name, "weapon.reserve")) return t->weapon_reserve;
    if (b3d_text_eq(name, "gameplay.threat")) return t->gameplay_threat;
    if (b3d_text_eq(name, "ecg.bpm")) return t->ecg_bpm;
    if (b3d_text_eq(name, "damage.flash_ms")) return t->damage_flash_ms;
    if (b3d_text_eq(name, "camera.first_person")) return t->first_person;
    if (b3d_text_eq(name, "weapon.muzzle_flash")) return t->muzzle_flash;
    if (b3d_text_eq(name, "numbar.value")) return t->numbar_value;
    if (b3d_text_eq(name, "numbar.min")) return t->numbar_min;
    if (b3d_text_eq(name, "numbar.max")) return t->numbar_max;
    if (b3d_text_eq(name, "numbar.overlay")) return t->numbar_overlay;
    if (b3d_text_eq(name, "numbar.overlay_min")) return t->numbar_overlay_min;
    if (b3d_text_eq(name, "numbar.overlay_max")) return t->numbar_overlay_max;
    if (b3d_text_eq(name, "numbar.mid")) return t->numbar_mid;
    if (b3d_text_eq(name, "numbar.segments")) return t->numbar_segments;
    if (b3d_text_eq(name, "numbar.state")) return t->numbar_state;
    if (b3d_text_eq(name, "numbar.phase")) return t->numbar_phase;
    if (b3d_text_eq(name, "numbar.units")) return t->numbar_units;
    if (b3d_text_eq(name, "numbar.layer_count")) return t->numbar_layer_count;
    if (b3d_text_eq(name, "numbar.layer_size")) return t->numbar_layer_size;
    if (b3d_text_eq(name, "numbar.value2")) return t->numbar_value2;
    if (b3d_text_eq(name, "player.dead")) return t->player_health <= 0L ? 1L : 0L;
    if (b3d_text_eq(name, "weapon.empty")) return t->weapon_loaded <= 0L ? 1L : 0L;
    return fallback;
}

void blank3d_bighud_resolve_rect(const Blank3DBigHudNode *node,
                                 int screen_w,
                                 int screen_h,
                                 int default_w,
                                 int default_h,
                                 int *out_x,
                                 int *out_y,
                                 int *out_w,
                                 int *out_h)
{
    int x;
    int y;
    int w;
    int h;
    int ox;
    int oy;
    int anchor;
    if (!node) return;
    w = node->has_size ? node->width : default_w;
    h = node->has_size ? node->height : default_h;
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    ox = node->has_pos ? node->offset_x : node->meter.rect.x;
    oy = node->has_pos ? node->offset_y : node->meter.rect.y;
    anchor = node->has_pos ? node->anchor : BVH_ANCHOR_TOP_LEFT;
    x = ox;
    y = oy;
    switch (anchor) {
    case BVH_ANCHOR_TOP_CENTER: x = (screen_w - w) / 2 + ox; y = oy; break;
    case BVH_ANCHOR_TOP_RIGHT: x = screen_w - w - ox; y = oy; break;
    case BVH_ANCHOR_CENTER: x = (screen_w - w) / 2 + ox; y = (screen_h - h) / 2 + oy; break;
    case BVH_ANCHOR_BOTTOM_LEFT: x = ox; y = screen_h - h - oy; break;
    case BVH_ANCHOR_BOTTOM_CENTER: x = (screen_w - w) / 2 + ox; y = screen_h - h - oy; break;
    case BVH_ANCHOR_BOTTOM_RIGHT: x = screen_w - w - ox; y = screen_h - h - oy; break;
    case BVH_ANCHOR_TOP_LEFT:
    default: x = ox; y = oy; break;
    }
    if (out_x) *out_x = x;
    if (out_y) *out_y = y;
    if (out_w) *out_w = w;
    if (out_h) *out_h = h;
}

void blank3d_bighud_update_bar(Blank3DBigHudNode *node,
                               const Blank3DBigHudTelemetry *telemetry,
                               unsigned int frame_ms)
{
    long min_value;
    long max_value;
    long value;
    long overlay;
    long overlay_min;
    long overlay_max;
    long mid_value;
    long segments;
    long dynamic_value;
    long whole;
    long remainder;
    long delta_q16;
    GBar89_Fix dt;
    if (!node || node->type != B3D_BIGHUD_NODE_BAR) return;
    min_value = blank3d_bighud_resolve_binding(telemetry, node->min_bind, node->meter.min_value);
    max_value = blank3d_bighud_resolve_binding(telemetry, node->max_bind, node->meter.max_value);
    value = blank3d_bighud_resolve_binding(telemetry, node->bind, node->meter.value);
    if (max_value <= min_value) max_value = min_value + 1L;
    gbar89_set_range(&node->meter, min_value, max_value);
    gbar89_set_value(&node->meter, value);

    overlay_min = blank3d_bighud_resolve_binding(telemetry,
                                                  node->overlay_min_bind,
                                                  node->meter.overlay_min_value);
    overlay_max = blank3d_bighud_resolve_binding(telemetry,
                                                  node->overlay_max_bind,
                                                  node->meter.overlay_max_value);
    if (overlay_max <= overlay_min) overlay_max = overlay_min + 1L;
    gbar89_set_overlay_range(&node->meter, overlay_min, overlay_max);
    if (node->overlay_bind[0]) {
        overlay = blank3d_bighud_resolve_binding(telemetry, node->overlay_bind, node->meter.overlay_value);
        gbar89_set_overlay_value(&node->meter, overlay);
    }
    if (node->mid_bind[0]) {
        mid_value = blank3d_bighud_resolve_binding(telemetry, node->mid_bind, node->meter.mid_value);
        gbar89_set_mid_value(&node->meter, mid_value);
    }
    if (node->segments_bind[0]) {
        segments = blank3d_bighud_resolve_binding(telemetry, node->segments_bind, node->meter.segments);
        if (segments < 1L) segments = 1L;
        if (segments > 256L) segments = 256L;
        node->meter.segments = (int)segments;
    }
    if (node->state_bind[0]) {
        dynamic_value = blank3d_bighud_resolve_binding(telemetry, node->state_bind, node->meter.state);
        if (dynamic_value < 0L) dynamic_value = 0L;
        if (dynamic_value >= GBAR89_STATE_COUNT) dynamic_value = GBAR89_STATE_COUNT - 1;
        node->meter.state = (int)dynamic_value;
    }
    if (node->unit_count_bind[0]) {
        dynamic_value = blank3d_bighud_resolve_binding(telemetry, node->unit_count_bind, node->meter.unit_count);
        if (dynamic_value < 0L) dynamic_value = 0L;
        if (dynamic_value > 1024L) dynamic_value = 1024L;
        node->meter.unit_count = (int)dynamic_value;
    }
    if (node->layer_count_bind[0]) {
        dynamic_value = blank3d_bighud_resolve_binding(telemetry, node->layer_count_bind, node->meter.layer_count);
        if (dynamic_value < 0L) dynamic_value = 0L;
        if (dynamic_value > GBAR89_MAX_LAYERS) dynamic_value = GBAR89_MAX_LAYERS;
        node->meter.layer_count = (int)dynamic_value;
    }
    if (node->layer_size_bind[0]) {
        dynamic_value = blank3d_bighud_resolve_binding(telemetry, node->layer_size_bind, node->meter.layer_size);
        if (dynamic_value < 0L) dynamic_value = 0L;
        node->meter.layer_size = dynamic_value;
    }
    if (node->radial_phase_bind[0]) {
        dynamic_value = blank3d_bighud_resolve_binding(telemetry, node->radial_phase_bind,
                                                       node->meter.style.radial_phase_deg);
        dynamic_value %= 360L;
        node->meter.style.radial_phase_deg = (int)dynamic_value;
        node->radial_phase_accum_q16 = dynamic_value * GBAR89_FIX_ONE;
    } else if (node->radial_phase_speed_deg_per_sec != 0 && frame_ms > 0u) {
        if (frame_ms > 1000u) frame_ms = 1000u;
        dynamic_value = (long)node->radial_phase_speed_deg_per_sec * (long)frame_ms;
        whole = dynamic_value / 1000L;
        remainder = dynamic_value % 1000L;
        delta_q16 = whole * GBAR89_FIX_ONE +
                    (remainder * GBAR89_FIX_ONE) / 1000L;
        node->radial_phase_accum_q16 += delta_q16;
        while (node->radial_phase_accum_q16 >= 360L * GBAR89_FIX_ONE)
            node->radial_phase_accum_q16 -= 360L * GBAR89_FIX_ONE;
        while (node->radial_phase_accum_q16 <= -360L * GBAR89_FIX_ONE)
            node->radial_phase_accum_q16 += 360L * GBAR89_FIX_ONE;
        node->meter.style.radial_phase_deg =
            (int)(node->radial_phase_accum_q16 / GBAR89_FIX_ONE);
    }

    b3d_rebind_unit_points(node);
    dt = (GBar89_Fix)(((long)frame_ms * GBAR89_FIX_ONE) / 1000L);
    gbar89_tick(&node->meter, dt);
}

const char *blank3d_bighud_error(const Blank3DBigHud *hud)
{
    if (!hud) return "BigHUD unavailable";
    return hud->last_error;
}
