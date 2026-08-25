#include "skyboxrecipe89.h"

#include <string.h>

static void sbr89_copy(char *dst, unsigned int cap, const char *src)
{
    unsigned int i;
    if (!dst || cap == 0U) return;
    if (!src) src = "";
    i = 0U;
    while (src[i] && i + 1U < cap) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static int sbr89_streq(const char *a, const char *b)
{
    if (!a || !b) return 0;
    return strcmp(a, b) == 0;
}

static int sbr89_is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static char *sbr89_trim(char *text)
{
    char *end;
    if (!text) return text;
    while (*text && sbr89_is_space(*text)) ++text;
    end = text + strlen(text);
    while (end > text && sbr89_is_space(end[-1])) --end;
    *end = '\0';
    return text;
}

static void sbr89_unquote(char *text)
{
    unsigned int len;
    if (!text) return;
    len = (unsigned int)strlen(text);
    if (len >= 2U && ((text[0] == '"' && text[len - 1U] == '"') ||
                     (text[0] == '\'' && text[len - 1U] == '\''))) {
        memmove(text, text + 1, len - 2U);
        text[len - 2U] = '\0';
    }
}

static long sbr89_to_long(const char *text, long fallback)
{
    long sign;
    long value;
    int any;
    if (!text) return fallback;
    while (*text && sbr89_is_space(*text)) ++text;
    sign = 1L;
    if (*text == '-') {
        sign = -1L;
        ++text;
    } else if (*text == '+') {
        ++text;
    }
    value = 0L;
    any = 0;
    while (*text >= '0' && *text <= '9') {
        any = 1;
        value = value * 10L + (long)(*text - '0');
        ++text;
    }
    return any ? value * sign : fallback;
}

static long sbr89_to_q16(const char *text, long fallback)
{
    long sign;
    long whole;
    long frac;
    long scale;
    int any;
    if (!text) return fallback;
    while (*text && sbr89_is_space(*text)) ++text;
    sign = 1L;
    if (*text == '-') {
        sign = -1L;
        ++text;
    } else if (*text == '+') {
        ++text;
    }
    whole = 0L;
    any = 0;
    while (*text >= '0' && *text <= '9') {
        any = 1;
        whole = whole * 10L + (long)(*text - '0');
        ++text;
    }
    frac = 0L;
    scale = 1L;
    if (*text == '.') {
        ++text;
        while (*text >= '0' && *text <= '9' && scale < 1000000L) {
            frac = frac * 10L + (long)(*text - '0');
            scale *= 10L;
            ++text;
        }
    }
    if (!any && scale == 1L) return fallback;
    return sign * (whole * SBR89_FX_ONE + (frac * SBR89_FX_ONE) / scale);
}

static int sbr89_to_bool(const char *text, int fallback)
{
    if (!text) return fallback;
    if (sbr89_streq(text, "1") || sbr89_streq(text, "true") ||
        sbr89_streq(text, "yes") || sbr89_streq(text, "on")) return 1;
    if (sbr89_streq(text, "0") || sbr89_streq(text, "false") ||
        sbr89_streq(text, "no") || sbr89_streq(text, "off")) return 0;
    return fallback;
}

static int sbr89_parse_color(const char *text, SBR89Color *out)
{
    int values[4];
    int count;
    const char *p;
    long value;
    if (!text || !out) return 0;
    count = 0;
    p = text;
    while (*p && count < 4) {
        while (*p && (sbr89_is_space(*p) || *p == ',' || *p == '|')) ++p;
        if (!*p) break;
        value = 0L;
        if (*p < '0' || *p > '9') return 0;
        while (*p >= '0' && *p <= '9') {
            value = value * 10L + (long)(*p - '0');
            ++p;
        }
        if (value < 0L) value = 0L;
        if (value > 255L) value = 255L;
        values[count++] = (int)value;
    }
    if (count < 3) return 0;
    out->r = (unsigned char)values[0];
    out->g = (unsigned char)values[1];
    out->b = (unsigned char)values[2];
    out->a = (unsigned char)(count >= 4 ? values[3] : 255);
    return 1;
}

static int sbr89_parse_rect(const char *text, SBR89Rect *out)
{
    long values[4];
    int count;
    const char *p;
    char token[48];
    unsigned int n;
    if (!text || !out) return 0;
    count = 0;
    p = text;
    while (*p && count < 4) {
        while (*p && (sbr89_is_space(*p) || *p == ',' || *p == '|')) ++p;
        if (!*p) break;
        n = 0U;
        while (*p && !sbr89_is_space(*p) && *p != ',' && *p != '|' &&
               n + 1U < (unsigned int)sizeof(token)) {
            token[n++] = *p++;
        }
        token[n] = '\0';
        values[count++] = sbr89_to_q16(token, -1L);
    }
    if (count != 4) return 0;
    if (values[0] < 0L || values[1] < 0L ||
        values[2] < 0L || values[3] < 0L) return 0;
    out->u0_q16 = values[0];
    out->v0_q16 = values[1];
    out->u1_q16 = values[2];
    out->v1_q16 = values[3];
    return 1;
}

static void sbr89_rect_cell(SBR89Rect *out, int col, int row,
                            int columns, int rows)
{
    if (!out || columns <= 0 || rows <= 0) return;
    out->u0_q16 = ((long)col * SBR89_FX_ONE) / (long)columns;
    out->v0_q16 = ((long)row * SBR89_FX_ONE) / (long)rows;
    out->u1_q16 = ((long)(col + 1) * SBR89_FX_ONE) / (long)columns;
    out->v1_q16 = ((long)(row + 1) * SBR89_FX_ONE) / (long)rows;
}

static void sbr89_set_error(SBR89Workspace *ws, const char *text, int line)
{
    if (!ws) return;
    sbr89_copy(ws->last_error, (unsigned int)sizeof(ws->last_error), text);
    ws->last_error_line = line;
}

void sbr89_recipe_defaults(SBR89Recipe *recipe)
{
    int i;
    if (!recipe) return;
    memset(recipe, 0, sizeof(*recipe));
    recipe->enabled = 1;
    recipe->screen_enabled = 1;
    recipe->cube_enabled = 0;
    recipe->dome_enabled = 0;
    recipe->radius_q16 = 80L * SBR89_FX_ONE;
    recipe->source_type = SBR89_SOURCE_PROCEDURAL;
    recipe->layout = SBR89_LAYOUT_NONE;
    recipe->convention = SBR89_CONVENTION_AXIS;
    recipe->uv_inset_pixels = 1;
    recipe->screen_depth_func = 1;
    recipe->screen_top.r = 80U;
    recipe->screen_top.g = 128U;
    recipe->screen_top.b = 190U;
    recipe->screen_top.a = 255U;
    recipe->screen_bottom.r = 170U;
    recipe->screen_bottom.g = 200U;
    recipe->screen_bottom.b = 230U;
    recipe->screen_bottom.a = 255U;
    recipe->dome_segments = 12;
    recipe->dome_rings = 5;
    recipe->dome_blend_mode = SBR89_BLEND_ALPHA;
    recipe->dome_top.r = 34U;
    recipe->dome_top.g = 84U;
    recipe->dome_top.b = 160U;
    recipe->dome_top.a = 160U;
    recipe->dome_horizon.r = 220U;
    recipe->dome_horizon.g = 230U;
    recipe->dome_horizon.b = 240U;
    recipe->dome_horizon.a = 96U;
    for (i = 0; i < SBR89_FACE_COUNT; ++i) {
        recipe->face_uv[i].u0_q16 = 0L;
        recipe->face_uv[i].v0_q16 = 0L;
        recipe->face_uv[i].u1_q16 = SBR89_FX_ONE;
        recipe->face_uv[i].v1_q16 = SBR89_FX_ONE;
    }
}

void sbr89_workspace_init(SBR89Workspace *workspace)
{
    if (!workspace) return;
    memset(workspace, 0, sizeof(*workspace));
}

const char *sbr89_last_error(const SBR89Workspace *workspace)
{
    return workspace ? workspace->last_error : "skyboxrecipe89 unavailable";
}

int sbr89_last_error_line(const SBR89Workspace *workspace)
{
    return workspace ? workspace->last_error_line : 0;
}

int sbr89_source_type_from_text(const char *text)
{
    if (!text) return SBR89_SOURCE_PROCEDURAL;
    if (sbr89_streq(text, "faces6") || sbr89_streq(text, "cube6") ||
        sbr89_streq(text, "six_faces")) return SBR89_SOURCE_FACES6;
    if (sbr89_streq(text, "atlas") || sbr89_streq(text, "cross") ||
        sbr89_streq(text, "strip") || sbr89_streq(text, "grid"))
        return SBR89_SOURCE_ATLAS;
    if (sbr89_streq(text, "screen") || sbr89_streq(text, "single_screen"))
        return SBR89_SOURCE_SINGLE_SCREEN;
    if (sbr89_streq(text, "dome") || sbr89_streq(text, "panorama") ||
        sbr89_streq(text, "equirect") || sbr89_streq(text, "single_dome"))
        return SBR89_SOURCE_SINGLE_DOME;
    if (sbr89_streq(text, "family") || sbr89_streq(text, "skyset") ||
        sbr89_streq(text, "source_family") || sbr89_streq(text, "specialized"))
        return SBR89_SOURCE_FAMILY;
    return SBR89_SOURCE_PROCEDURAL;
}

int sbr89_layout_from_text(const char *text)
{
    if (!text) return SBR89_LAYOUT_NONE;
    if (sbr89_streq(text, "strip6x1") || sbr89_streq(text, "horizontal_strip") ||
        sbr89_streq(text, "strip_h")) return SBR89_LAYOUT_STRIP_6X1;
    if (sbr89_streq(text, "strip1x6") || sbr89_streq(text, "vertical_strip") ||
        sbr89_streq(text, "strip_v")) return SBR89_LAYOUT_STRIP_1X6;
    if (sbr89_streq(text, "cross4x3") || sbr89_streq(text, "horizontal_cross") ||
        sbr89_streq(text, "cross_h")) return SBR89_LAYOUT_CROSS_4X3;
    if (sbr89_streq(text, "cross3x4") || sbr89_streq(text, "vertical_cross") ||
        sbr89_streq(text, "cross_v")) return SBR89_LAYOUT_CROSS_3X4;
    if (sbr89_streq(text, "grid3x2")) return SBR89_LAYOUT_GRID_3X2;
    if (sbr89_streq(text, "grid2x3")) return SBR89_LAYOUT_GRID_2X3;
    if (sbr89_streq(text, "custom")) return SBR89_LAYOUT_CUSTOM;
    return SBR89_LAYOUT_NONE;
}

int sbr89_convention_from_text(const char *text)
{
    if (!text) return SBR89_CONVENTION_AXIS;
    if (sbr89_streq(text, "source") || sbr89_streq(text, "valve") ||
        sbr89_streq(text, "source6")) return SBR89_CONVENTION_SOURCE;
    if (sbr89_streq(text, "word") || sbr89_streq(text, "words"))
        return SBR89_CONVENTION_WORD;
    return SBR89_CONVENTION_AXIS;
}

int sbr89_apply_layout(SBR89Recipe *recipe)
{
    int i;
    static const signed char cross4x3[SBR89_FACE_COUNT][2] = {
        { 2, 1 }, { 0, 1 }, { 1, 0 }, { 1, 2 }, { 1, 1 }, { 3, 1 }
    };
    static const signed char cross3x4[SBR89_FACE_COUNT][2] = {
        { 2, 1 }, { 0, 1 }, { 1, 0 }, { 1, 2 }, { 1, 1 }, { 1, 3 }
    };
    if (!recipe) return 0;
    if (recipe->layout == SBR89_LAYOUT_CUSTOM) return 1;
    if (recipe->layout == SBR89_LAYOUT_STRIP_6X1) {
        for (i = 0; i < SBR89_FACE_COUNT; ++i)
            sbr89_rect_cell(&recipe->face_uv[i], i, 0, 6, 1);
        return 1;
    }
    if (recipe->layout == SBR89_LAYOUT_STRIP_1X6) {
        for (i = 0; i < SBR89_FACE_COUNT; ++i)
            sbr89_rect_cell(&recipe->face_uv[i], 0, i, 1, 6);
        return 1;
    }
    if (recipe->layout == SBR89_LAYOUT_CROSS_4X3) {
        for (i = 0; i < SBR89_FACE_COUNT; ++i)
            sbr89_rect_cell(&recipe->face_uv[i], (int)cross4x3[i][0],
                            (int)cross4x3[i][1], 4, 3);
        return 1;
    }
    if (recipe->layout == SBR89_LAYOUT_CROSS_3X4) {
        for (i = 0; i < SBR89_FACE_COUNT; ++i)
            sbr89_rect_cell(&recipe->face_uv[i], (int)cross3x4[i][0],
                            (int)cross3x4[i][1], 3, 4);
        return 1;
    }
    if (recipe->layout == SBR89_LAYOUT_GRID_3X2) {
        for (i = 0; i < SBR89_FACE_COUNT; ++i)
            sbr89_rect_cell(&recipe->face_uv[i], i % 3, i / 3, 3, 2);
        return 1;
    }
    if (recipe->layout == SBR89_LAYOUT_GRID_2X3) {
        for (i = 0; i < SBR89_FACE_COUNT; ++i)
            sbr89_rect_cell(&recipe->face_uv[i], i % 2, i / 2, 2, 3);
        return 1;
    }
    return 0;
}

static int sbr89_append(char *dst, unsigned int cap, const char *text)
{
    unsigned int used;
    unsigned int i;
    if (!dst || !text || cap == 0U) return 0;
    used = (unsigned int)strlen(dst);
    i = 0U;
    while (text[i] && used + 1U < cap) dst[used++] = text[i++];
    dst[used] = '\0';
    return text[i] == '\0';
}

int sbr89_build_family_faces(SBR89Recipe *recipe)
{
    static const char *axis_suffix[SBR89_FACE_COUNT] = {
        "px", "nx", "py", "ny", "pz", "nz"
    };
    static const char *source_suffix[SBR89_FACE_COUNT] = {
        "RT", "LF", "UP", "DN", "FT", "BK"
    };
    static const char *word_suffix[SBR89_FACE_COUNT] = {
        "right", "left", "up", "down", "front", "back"
    };
    const char **suffixes;
    int i;
    if (!recipe || !recipe->family_base[0]) return 0;
    suffixes = axis_suffix;
    if (recipe->convention == SBR89_CONVENTION_SOURCE) suffixes = source_suffix;
    else if (recipe->convention == SBR89_CONVENTION_WORD) suffixes = word_suffix;
    for (i = 0; i < SBR89_FACE_COUNT; ++i) {
        recipe->face_image[i][0] = '\0';
        if (!sbr89_append(recipe->face_image[i], SBR89_REQUEST_CAP,
                          recipe->family_base)) return 0;
        if (recipe->convention == SBR89_CONVENTION_WORD) {
            if (!sbr89_append(recipe->face_image[i], SBR89_REQUEST_CAP, "_"))
                return 0;
        }
        if (!sbr89_append(recipe->face_image[i], SBR89_REQUEST_CAP, suffixes[i]))
            return 0;
        if (recipe->family_extension[0]) {
            if (recipe->family_extension[0] != '.') {
                if (!sbr89_append(recipe->face_image[i], SBR89_REQUEST_CAP, "."))
                    return 0;
            }
            if (!sbr89_append(recipe->face_image[i], SBR89_REQUEST_CAP,
                              recipe->family_extension)) return 0;
        }
    }
    return 1;
}

static int sbr89_is_absolute(const char *path)
{
    if (!path || !path[0]) return 0;
    if (path[0] == '/' || path[0] == '\\') return 1;
    if (path[0] && path[1] == ':') return 1;
    return 0;
}

static int sbr89_normalize_path(const char *src, char *out, unsigned int cap)
{
    char temp[SBR89_PATH_CAP * 2];
    unsigned int seg_start[64];
    unsigned int seg_count;
    unsigned int i;
    unsigned int n;
    unsigned int start;
    int absolute;
    int drive;
    if (!src || !out || cap == 0U) return 0;
    n = (unsigned int)strlen(src);
    if (n + 1U > (unsigned int)sizeof(temp)) return 0;
    for (i = 0U; i <= n; ++i) {
        char c;
        c = src[i];
        temp[i] = c == '\\' ? '/' : c;
    }
    absolute = temp[0] == '/';
    drive = (n >= 2U && temp[1] == ':') ? 1 : 0;
    seg_count = 0U;
    start = drive ? 2U : 0U;
    while (temp[start] == '/') ++start;
    i = start;
    while (1) {
        unsigned int end;
        unsigned int len;
        while (temp[i] && temp[i] != '/') ++i;
        end = i;
        len = end - start;
        if (len > 0U) {
            if (len == 1U && temp[start] == '.') {
                /* skip */
            } else if (len == 2U && temp[start] == '.' && temp[start + 1U] == '.') {
                if (seg_count > 0U) --seg_count;
                else if (!absolute && !drive) {
                    if (seg_count >= 64U) return 0;
                    seg_start[seg_count++] = start;
                }
            } else {
                if (seg_count >= 64U) return 0;
                seg_start[seg_count++] = start;
            }
        }
        if (!temp[i]) break;
        ++i;
        while (temp[i] == '/') ++i;
        start = i;
    }
    n = 0U;
    if (drive) {
        if (cap < 3U) return 0;
        out[n++] = temp[0];
        out[n++] = ':';
        if (temp[2] == '/') out[n++] = '/';
    } else if (absolute) {
        if (cap < 2U) return 0;
        out[n++] = '/';
    }
    for (i = 0U; i < seg_count; ++i) {
        unsigned int pos;
        unsigned int end;
        pos = seg_start[i];
        end = pos;
        while (temp[end] && temp[end] != '/') ++end;
        if (n > 0U && out[n - 1U] != '/' && n + 1U < cap) out[n++] = '/';
        while (pos < end) {
            if (n + 1U >= cap) return 0;
            out[n++] = temp[pos++];
        }
    }
    if (n == 0U) {
        if (cap < 2U) return 0;
        out[n++] = '.';
    }
    out[n] = '\0';
    return 1;
}

static int sbr89_join_relative(const char *parent_path, const char *child,
                               char *out, unsigned int cap)
{
    const char *slash;
    const char *slash2;
    unsigned int prefix;
    unsigned int i;
    char combined[SBR89_PATH_CAP * 2];
    if (!child || !out || cap == 0U) return 0;
    if (sbr89_is_absolute(child)) return sbr89_normalize_path(child, out, cap);
    slash = parent_path ? strrchr(parent_path, '/') : 0;
    slash2 = parent_path ? strrchr(parent_path, '\\') : 0;
    if (!slash || (slash2 && slash2 > slash)) slash = slash2;
    prefix = slash ? (unsigned int)(slash - parent_path + 1) : 0U;
    if (prefix + strlen(child) + 1U > (unsigned int)sizeof(combined)) return 0;
    for (i = 0U; i < prefix; ++i) combined[i] = parent_path[i];
    combined[prefix] = '\0';
    if (!sbr89_append(combined, (unsigned int)sizeof(combined), child)) return 0;
    return sbr89_normalize_path(combined, out, cap);
}

static int sbr89_parse_line(char *line, char *section, unsigned int section_cap,
                            char **out_key, char **out_value)
{
    char *p;
    char *end;
    char *eq;
    p = sbr89_trim(line);
    if (!*p || *p == '#' || *p == ';') return 0;
    if (*p == '[') {
        end = strchr(p + 1, ']');
        if (!end) return -1;
        *end = '\0';
        sbr89_copy(section, section_cap, sbr89_trim(p + 1));
        return 0;
    }
    eq = strchr(p, '=');
    if (!eq) return 0;
    *eq = '\0';
    *out_key = sbr89_trim(p);
    *out_value = sbr89_trim(eq + 1);
    sbr89_unquote(*out_value);
    return 1;
}

static int sbr89_line_next(char *text, unsigned int size, unsigned int *cursor,
                           char **out_line)
{
    unsigned int start;
    if (!text || !cursor || !out_line || *cursor >= size) return 0;
    start = *cursor;
    while (*cursor < size && text[*cursor] != '\n' && text[*cursor] != '\r')
        ++(*cursor);
    if (*cursor < size) {
        text[*cursor] = '\0';
        ++(*cursor);
        if (*cursor < size && (text[*cursor] == '\n' || text[*cursor] == '\r'))
            ++(*cursor);
    }
    *out_line = &text[start];
    return 1;
}

static void sbr89_apply_key(SBR89Recipe *r, const char *section,
                            const char *key, const char *value)
{
    int face;
    if (!r || !section || !key || !value) return;
    if (sbr89_streq(section, "skybox")) {
        if (sbr89_streq(key, "enabled")) r->enabled = sbr89_to_bool(value, r->enabled);
        else if (sbr89_streq(key, "screen")) r->screen_enabled = sbr89_to_bool(value, r->screen_enabled);
        else if (sbr89_streq(key, "cube")) r->cube_enabled = sbr89_to_bool(value, r->cube_enabled);
        else if (sbr89_streq(key, "dome")) r->dome_enabled = sbr89_to_bool(value, r->dome_enabled);
        else if (sbr89_streq(key, "radius")) r->radius_q16 = sbr89_to_q16(value, r->radius_q16);
        return;
    }
    if (sbr89_streq(section, "source")) {
        if (sbr89_streq(key, "type") || sbr89_streq(key, "mode"))
            r->source_type = sbr89_source_type_from_text(value);
        else if (sbr89_streq(key, "image") || sbr89_streq(key, "asset"))
            sbr89_copy(r->shared_image, SBR89_REQUEST_CAP, value);
        else if (sbr89_streq(key, "layout")) r->layout = sbr89_layout_from_text(value);
        else if (sbr89_streq(key, "convention")) r->convention = sbr89_convention_from_text(value);
        else if (sbr89_streq(key, "base") || sbr89_streq(key, "prefix"))
            sbr89_copy(r->family_base, SBR89_REQUEST_CAP, value);
        else if (sbr89_streq(key, "extension") || sbr89_streq(key, "ext"))
            sbr89_copy(r->family_extension, (unsigned int)sizeof(r->family_extension), value);
        else if (sbr89_streq(key, "uv_inset_pixels"))
            r->uv_inset_pixels = (int)sbr89_to_long(value, r->uv_inset_pixels);
        return;
    }
    if (sbr89_streq(section, "faces")) {
        face = -1;
        if (sbr89_streq(key, "px") || sbr89_streq(key, "right")) face = SBR89_FACE_PX;
        else if (sbr89_streq(key, "nx") || sbr89_streq(key, "left")) face = SBR89_FACE_NX;
        else if (sbr89_streq(key, "py") || sbr89_streq(key, "up") || sbr89_streq(key, "top")) face = SBR89_FACE_PY;
        else if (sbr89_streq(key, "ny") || sbr89_streq(key, "down") || sbr89_streq(key, "bottom")) face = SBR89_FACE_NY;
        else if (sbr89_streq(key, "pz") || sbr89_streq(key, "front")) face = SBR89_FACE_PZ;
        else if (sbr89_streq(key, "nz") || sbr89_streq(key, "back")) face = SBR89_FACE_NZ;
        if (face >= 0) sbr89_copy(r->face_image[face], SBR89_REQUEST_CAP, value);
        return;
    }
    if (sbr89_streq(section, "atlas")) {
        if (sbr89_streq(key, "image") || sbr89_streq(key, "asset"))
            sbr89_copy(r->shared_image, SBR89_REQUEST_CAP, value);
        else if (sbr89_streq(key, "layout")) r->layout = sbr89_layout_from_text(value);
        else if (sbr89_streq(key, "px")) (void)sbr89_parse_rect(value, &r->face_uv[SBR89_FACE_PX]);
        else if (sbr89_streq(key, "nx")) (void)sbr89_parse_rect(value, &r->face_uv[SBR89_FACE_NX]);
        else if (sbr89_streq(key, "py")) (void)sbr89_parse_rect(value, &r->face_uv[SBR89_FACE_PY]);
        else if (sbr89_streq(key, "ny")) (void)sbr89_parse_rect(value, &r->face_uv[SBR89_FACE_NY]);
        else if (sbr89_streq(key, "pz")) (void)sbr89_parse_rect(value, &r->face_uv[SBR89_FACE_PZ]);
        else if (sbr89_streq(key, "nz")) (void)sbr89_parse_rect(value, &r->face_uv[SBR89_FACE_NZ]);
        return;
    }
    if (sbr89_streq(section, "cube")) {
        if (sbr89_streq(key, "flip_u_mask")) r->flip_u_mask = (int)sbr89_to_long(value, r->flip_u_mask);
        else if (sbr89_streq(key, "flip_v_mask")) r->flip_v_mask = (int)sbr89_to_long(value, r->flip_v_mask);
        else if (sbr89_streq(key, "swap_uv_mask")) r->swap_uv_mask = (int)sbr89_to_long(value, r->swap_uv_mask);
        return;
    }
    if (sbr89_streq(section, "screen")) {
        if (sbr89_streq(key, "image") || sbr89_streq(key, "asset"))
            sbr89_copy(r->screen_image, SBR89_REQUEST_CAP, value);
        else if (sbr89_streq(key, "top")) (void)sbr89_parse_color(value, &r->screen_top);
        else if (sbr89_streq(key, "bottom")) (void)sbr89_parse_color(value, &r->screen_bottom);
        else if (sbr89_streq(key, "depth_func")) {
            if (sbr89_streq(value, "always")) r->screen_depth_func = 0;
            else if (sbr89_streq(value, "equal")) r->screen_depth_func = 2;
            else if (sbr89_streq(value, "off")) r->screen_depth_func = 3;
            else r->screen_depth_func = 1;
        }
        return;
    }
    if (sbr89_streq(section, "dome")) {
        if (sbr89_streq(key, "image") || sbr89_streq(key, "asset"))
            sbr89_copy(r->dome_image, SBR89_REQUEST_CAP, value);
        else if (sbr89_streq(key, "segments")) r->dome_segments = (int)sbr89_to_long(value, r->dome_segments);
        else if (sbr89_streq(key, "rings")) r->dome_rings = (int)sbr89_to_long(value, r->dome_rings);
        else if (sbr89_streq(key, "top")) (void)sbr89_parse_color(value, &r->dome_top);
        else if (sbr89_streq(key, "horizon")) (void)sbr89_parse_color(value, &r->dome_horizon);
        else if (sbr89_streq(key, "blend")) {
            if (sbr89_streq(value, "add") || sbr89_streq(value, "additive")) r->dome_blend_mode = SBR89_BLEND_ADD;
            else if (sbr89_streq(value, "off") || sbr89_streq(value, "none")) r->dome_blend_mode = SBR89_BLEND_OFF;
            else r->dome_blend_mode = SBR89_BLEND_ALPHA;
        }
    }
}

static int sbr89_stack_contains(const SBR89Workspace *ws, const char *path,
                                int depth)
{
    int i;
    if (!ws || !path) return 0;
    for (i = 0; i < depth; ++i)
        if (strcmp(ws->path_stack[i], path) == 0) return 1;
    return 0;
}

static int sbr89_load_file_recursive(SBR89Recipe *recipe,
                                     SBR89Workspace *ws,
                                     const SBR89IOProvider *io,
                                     const char *path,
                                     int depth)
{
    unsigned int size;
    unsigned int cursor;
    char section[48];
    char *line;
    char *key;
    char *value;
    char include_path[SBR89_PATH_CAP];
    int parse_rc;
    int line_no;
    if (!recipe || !ws || !io || !io->load_text || !path) return 0;
    if (depth < 0 || depth >= SBR89_MAX_INCLUDE_DEPTH) {
        sbr89_set_error(ws, "include depth exceeded", 0);
        return 0;
    }
    if (sbr89_stack_contains(ws, path, depth)) {
        sbr89_set_error(ws, "recipe include cycle", 0);
        return 0;
    }
    sbr89_copy(ws->path_stack[depth], SBR89_PATH_CAP, path);
    size = 0U;
    if (!io->load_text(io->user, path, ws->text[depth], SBR89_TEXT_CAP, &size)) {
        sbr89_set_error(ws, "recipe file load failed", 0);
        return 0;
    }
    if (size >= SBR89_TEXT_CAP) {
        sbr89_set_error(ws, "recipe file too large", 0);
        return 0;
    }
    ws->text[depth][size] = '\0';

    /* First pass: all includes, so local fields always override subrecipes. */
    section[0] = '\0';
    cursor = 0U;
    line_no = 0;
    while (sbr89_line_next(ws->text[depth], size, &cursor, &line)) {
        ++line_no;
        key = 0;
        value = 0;
        parse_rc = sbr89_parse_line(line, section, (unsigned int)sizeof(section), &key, &value);
        if (parse_rc < 0) {
            sbr89_set_error(ws, "bad INI section", line_no);
            return 0;
        }
        if (parse_rc == 1 && sbr89_streq(section, "recipe") &&
            (sbr89_streq(key, "include") || sbr89_streq(key, "subrecipe"))) {
            if (!sbr89_join_relative(path, value, include_path,
                                     (unsigned int)sizeof(include_path))) {
                sbr89_set_error(ws, "include path too long", line_no);
                return 0;
            }
            if (!sbr89_load_file_recursive(recipe, ws, io, include_path, depth + 1))
                return 0;
        }
    }

    /* Reload because the first pass nul-terminates line boundaries in-place. */
    size = 0U;
    if (!io->load_text(io->user, path, ws->text[depth], SBR89_TEXT_CAP, &size)) {
        sbr89_set_error(ws, "recipe file reload failed", 0);
        return 0;
    }
    ws->text[depth][size] = '\0';
    section[0] = '\0';
    cursor = 0U;
    line_no = 0;
    while (sbr89_line_next(ws->text[depth], size, &cursor, &line)) {
        ++line_no;
        key = 0;
        value = 0;
        parse_rc = sbr89_parse_line(line, section, (unsigned int)sizeof(section), &key, &value);
        if (parse_rc < 0) {
            sbr89_set_error(ws, "bad INI section", line_no);
            return 0;
        }
        if (parse_rc == 1 && !sbr89_streq(section, "recipe"))
            sbr89_apply_key(recipe, section, key, value);
    }
    return 1;
}

static void sbr89_finalize(SBR89Recipe *recipe)
{
    int i;
    if (!recipe) return;
    if (recipe->radius_q16 < 2L * SBR89_FX_ONE) recipe->radius_q16 = 2L * SBR89_FX_ONE;
    if (recipe->radius_q16 > 120L * SBR89_FX_ONE) recipe->radius_q16 = 120L * SBR89_FX_ONE;
    if (recipe->dome_segments < 4) recipe->dome_segments = 4;
    if (recipe->dome_segments > 16) recipe->dome_segments = 16;
    if (recipe->dome_rings < 2) recipe->dome_rings = 2;
    if (recipe->dome_rings > 5) recipe->dome_rings = 5;
    if (recipe->uv_inset_pixels < 0) recipe->uv_inset_pixels = 0;
    if (recipe->uv_inset_pixels > 8) recipe->uv_inset_pixels = 8;

    if (recipe->source_type == SBR89_SOURCE_FAMILY) {
        (void)sbr89_build_family_faces(recipe);
        recipe->cube_enabled = 1;
    } else if (recipe->source_type == SBR89_SOURCE_ATLAS) {
        (void)sbr89_apply_layout(recipe);
        if (recipe->shared_image[0]) {
            for (i = 0; i < SBR89_FACE_COUNT; ++i)
                sbr89_copy(recipe->face_image[i], SBR89_REQUEST_CAP,
                           recipe->shared_image);
        }
        recipe->cube_enabled = 1;
    } else if (recipe->source_type == SBR89_SOURCE_FACES6) {
        recipe->cube_enabled = 1;
    } else if (recipe->source_type == SBR89_SOURCE_SINGLE_SCREEN) {
        if (!recipe->screen_image[0])
            sbr89_copy(recipe->screen_image, SBR89_REQUEST_CAP, recipe->shared_image);
        recipe->screen_enabled = 1;
    } else if (recipe->source_type == SBR89_SOURCE_SINGLE_DOME) {
        if (!recipe->dome_image[0])
            sbr89_copy(recipe->dome_image, SBR89_REQUEST_CAP, recipe->shared_image);
        recipe->dome_enabled = 1;
    }
}

int sbr89_load_recipe(SBR89Recipe *out_recipe,
                      SBR89Workspace *workspace,
                      const SBR89IOProvider *io,
                      const char *recipe_path)
{
    if (!out_recipe || !workspace || !io || !recipe_path || !recipe_path[0])
        return 0;
    sbr89_workspace_init(workspace);
    sbr89_recipe_defaults(out_recipe);
    if (!sbr89_load_file_recursive(out_recipe, workspace, io, recipe_path, 0))
        return 0;
    sbr89_copy(out_recipe->loaded_recipe, SBR89_PATH_CAP, recipe_path);
    sbr89_finalize(out_recipe);
    return 1;
}

int sbr89_catalog_resolve(SBR89Workspace *workspace,
                          const SBR89IOProvider *io,
                          const char *catalog_path,
                          const char *recipe_name,
                          char *out_recipe_path,
                          unsigned int out_capacity)
{
    unsigned int size;
    unsigned int cursor;
    char section[48];
    char *line;
    char *key;
    char *value;
    int parse_rc;
    int line_no;
    char relative[SBR89_PATH_CAP];
    if (!workspace || !io || !io->load_text || !catalog_path ||
        !recipe_name || !out_recipe_path || out_capacity == 0U) return 0;
    workspace->last_error[0] = '\0';
    workspace->last_error_line = 0;
    size = 0U;
    if (!io->load_text(io->user, catalog_path, workspace->text[0],
                       SBR89_TEXT_CAP, &size)) {
        sbr89_set_error(workspace, "skybox catalog load failed", 0);
        return 0;
    }
    workspace->text[0][size] = '\0';
    section[0] = '\0';
    cursor = 0U;
    line_no = 0;
    while (sbr89_line_next(workspace->text[0], size, &cursor, &line)) {
        ++line_no;
        key = 0;
        value = 0;
        parse_rc = sbr89_parse_line(line, section, (unsigned int)sizeof(section), &key, &value);
        if (parse_rc < 0) {
            sbr89_set_error(workspace, "bad catalog section", line_no);
            return 0;
        }
        if (parse_rc == 1 && sbr89_streq(section, "recipes") &&
            sbr89_streq(key, recipe_name)) {
            sbr89_copy(relative, (unsigned int)sizeof(relative), value);
            if (!sbr89_join_relative(catalog_path, relative, out_recipe_path,
                                     out_capacity)) {
                sbr89_set_error(workspace, "catalog recipe path too long", line_no);
                return 0;
            }
            return 1;
        }
    }
    sbr89_set_error(workspace, "named skybox recipe not found", 0);
    return 0;
}

int sbr89_load_named_recipe(SBR89Recipe *out_recipe,
                            SBR89Workspace *workspace,
                            const SBR89IOProvider *io,
                            const char *catalog_path,
                            const char *recipe_name)
{
    char path[SBR89_PATH_CAP];
    if (!out_recipe || !workspace || !io || !catalog_path || !recipe_name)
        return 0;
    if (!sbr89_catalog_resolve(workspace, io, catalog_path, recipe_name,
                               path, (unsigned int)sizeof(path))) return 0;
    if (!sbr89_load_recipe(out_recipe, workspace, io, path)) return 0;
    sbr89_copy(out_recipe->selected_name, SBR89_NAME_CAP, recipe_name);
    return 1;
}
