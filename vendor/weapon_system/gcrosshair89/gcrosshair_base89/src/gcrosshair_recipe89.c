#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "gcrosshair_base89.h"
#include "gcrosshair_recipe89.h"

typedef struct GCB89_RecipePresetData {
    int present;
    int id;
    char name[GCB89_RECIPE_MAX_NAME];
    char category[GCB89_RECIPE_MAX_CATEGORY];
    GC89_Style style;
    GCB89_AnimationPreset animation;
    GCB89_AnimationRecipe animation_recipe;
} GCB89_RecipePresetData;

typedef struct GCB89_RecipeAssetData {
    int present;
    int image_id;
    char filename[GCB89_RECIPE_MAX_PATH];
} GCB89_RecipeAssetData;

static GCB89_RecipePresetData g_recipe_presets[GCB89_RECIPE_MAX_PRESETS];
static GCB89_RecipeAssetData g_recipe_assets[GCB89_RECIPE_MAX_ASSETS];
static int g_recipe_preset_count = 0;
static int g_recipe_asset_count = 0;
static int g_recipe_loaded = 0;
static char g_recipe_root[GCB89_RECIPE_MAX_PATH];
static char g_recipe_error[256];
static GCB89_RecipeIoProvider g_recipe_io_provider;
static void *g_recipe_io_user;
static int g_recipe_io_enabled;

static void *gcb89_io_open_read(const char *path)
{
    if (g_recipe_io_enabled && g_recipe_io_provider.open_read)
        return g_recipe_io_provider.open_read(g_recipe_io_user, path);
    return (void *)fopen(path, "rb");
}

static int gcb89_io_read_line(void *handle, char *buffer, int capacity)
{
    if (!handle || !buffer || capacity <= 1) return 0;
    if (g_recipe_io_enabled && g_recipe_io_provider.read_line)
        return g_recipe_io_provider.read_line(g_recipe_io_user, handle,
                                              buffer, capacity) ? 1 : 0;
    return fgets(buffer, capacity, (FILE *)handle) ? 1 : 0;
}

static void gcb89_io_close(void *handle)
{
    if (!handle) return;
    if (g_recipe_io_enabled && g_recipe_io_provider.close) {
        g_recipe_io_provider.close(g_recipe_io_user, handle);
        return;
    }
    fclose((FILE *)handle);
}

void gcb89_recipe_set_io_provider(const GCB89_RecipeIoProvider *provider,
                                  void *user)
{
    if (!provider || !provider->open_read ||
        !provider->read_line || !provider->close) {
        gcb89_recipe_clear_io_provider();
        return;
    }
    g_recipe_io_provider = *provider;
    g_recipe_io_user = user;
    g_recipe_io_enabled = 1;
}

void gcb89_recipe_clear_io_provider(void)
{
    memset(&g_recipe_io_provider, 0, sizeof(g_recipe_io_provider));
    g_recipe_io_user = 0;
    g_recipe_io_enabled = 0;
}

static void gcb89_set_error(const char *text)
{
    size_t n;
    if (!text) text = "unknown error";
    n = strlen(text);
    if (n >= sizeof(g_recipe_error)) n = sizeof(g_recipe_error) - 1U;
    memcpy(g_recipe_error, text, n);
    g_recipe_error[n] = '\0';
}

static void gcb89_set_error_path(const char *prefix, const char *path)
{
    size_t a;
    size_t b;
    size_t room;
    g_recipe_error[0] = '\0';
    if (!prefix) prefix = "";
    if (!path) path = "";
    a = strlen(prefix);
    if (a >= sizeof(g_recipe_error)) a = sizeof(g_recipe_error) - 1U;
    memcpy(g_recipe_error, prefix, a);
    g_recipe_error[a] = '\0';
    room = sizeof(g_recipe_error) - a - 1U;
    b = strlen(path);
    if (b > room) b = room;
    memcpy(g_recipe_error + a, path, b);
    g_recipe_error[a + b] = '\0';
}

static char *gcb89_ltrim(char *s)
{
    while (*s != '\0' && isspace((unsigned char)*s)) ++s;
    return s;
}

static void gcb89_rtrim(char *s)
{
    size_t n;
    n = strlen(s);
    while (n > 0U && isspace((unsigned char)s[n - 1U])) {
        s[n - 1U] = '\0';
        --n;
    }
}

static char *gcb89_trim(char *s)
{
    s = gcb89_ltrim(s);
    gcb89_rtrim(s);
    return s;
}

static void gcb89_copy_text(char *dst, size_t cap, const char *src)
{
    size_t n;
    if (!dst || cap == 0U) return;
    if (!src) src = "";
    n = strlen(src);
    if (n >= cap) n = cap - 1U;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

static int gcb89_is_absolute_path(const char *path)
{
    if (!path || path[0] == '\0') return 0;
    if (path[0] == '/' || path[0] == '\\') return 1;
    if (isalpha((unsigned char)path[0]) && path[1] == ':') return 1;
    return 0;
}

static void gcb89_dirname(const char *path, char *out, size_t cap)
{
    const char *slash;
    const char *bslash;
    const char *last;
    size_t n;
    if (!out || cap == 0U) return;
    out[0] = '\0';
    if (!path) return;
    slash = strrchr(path, '/');
    bslash = strrchr(path, '\\');
    last = slash;
    if (!last || (bslash && bslash > last)) last = bslash;
    if (!last) {
        gcb89_copy_text(out, cap, ".");
        return;
    }
    n = (size_t)(last - path);
    if (n == 0U) n = 1U;
    if (n >= cap) n = cap - 1U;
    memcpy(out, path, n);
    out[n] = '\0';
}

static int gcb89_join_path(const char *base_file,
                           const char *child,
                           char *out,
                           size_t cap)
{
    char dir[GCB89_RECIPE_MAX_PATH];
    size_t a;
    size_t b;
    if (!child || !out || cap == 0U) return 0;
    if (gcb89_is_absolute_path(child)) {
        gcb89_copy_text(out, cap, child);
        return 1;
    }
    gcb89_dirname(base_file, dir, sizeof(dir));
    a = strlen(dir);
    b = strlen(child);
    if (a + 1U + b + 1U > cap) return 0;
    memcpy(out, dir, a);
    if (a > 0U && dir[a - 1U] != '/' && dir[a - 1U] != '\\') {
        out[a++] = '/';
    }
    memcpy(out + a, child, b);
    out[a + b] = '\0';
    return 1;
}

static int gcb89_parse_long(const char *text, long *out)
{
    char *end;
    long v;
    if (!text || !out) return 0;
    v = strtol(text, &end, 0);
    if (end == text) return 0;
    while (*end != '\0' && isspace((unsigned char)*end)) ++end;
    if (*end != '\0') return 0;
    *out = v;
    return 1;
}

static int gcb89_parse_ulong(const char *text, unsigned long *out)
{
    char *end;
    unsigned long v;
    if (!text || !out) return 0;
    v = strtoul(text, &end, 0);
    if (end == text) return 0;
    while (*end != '\0' && isspace((unsigned char)*end)) ++end;
    if (*end != '\0') return 0;
    *out = v;
    return 1;
}

static int gcb89_parse_bool(const char *text, int *out)
{
    long v;
    if (!text || !out) return 0;
    if (strcmp(text, "true") == 0 || strcmp(text, "yes") == 0 || strcmp(text, "on") == 0) {
        *out = 1;
        return 1;
    }
    if (strcmp(text, "false") == 0 || strcmp(text, "no") == 0 || strcmp(text, "off") == 0) {
        *out = 0;
        return 1;
    }
    if (!gcb89_parse_long(text, &v)) return 0;
    *out = v ? 1 : 0;
    return 1;
}

static int gcb89_parse_draw_mode(const char *text, int *out)
{
    long v;
    if (strcmp(text, "vector") == 0) { *out = GC89_DRAW_VECTOR; return 1; }
    if (strcmp(text, "image") == 0) { *out = GC89_DRAW_IMAGE; return 1; }
    if (strcmp(text, "hybrid") == 0) { *out = GC89_DRAW_HYBRID; return 1; }
    if (!gcb89_parse_long(text, &v)) return 0;
    *out = (int)v;
    return 1;
}

static int gcb89_parse_shape(const char *text, int *out)
{
    long v;
    if (strcmp(text, "cross") == 0) { *out = GC89_SHAPE_CROSS; return 1; }
    if (strcmp(text, "circle") == 0) { *out = GC89_SHAPE_CIRCLE; return 1; }
    if (strcmp(text, "square") == 0) { *out = GC89_SHAPE_SQUARE; return 1; }
    if (strcmp(text, "diamond") == 0) { *out = GC89_SHAPE_DIAMOND; return 1; }
    if (strcmp(text, "chevrons") == 0) { *out = GC89_SHAPE_CHEVRONS; return 1; }
    if (strcmp(text, "hexagon") == 0) { *out = GC89_SHAPE_HEXAGON; return 1; }
    if (strcmp(text, "brackets") == 0) { *out = GC89_SHAPE_BRACKETS; return 1; }
    if (strcmp(text, "open_triangle") == 0) { *out = GC89_SHAPE_OPEN_TRIANGLE; return 1; }
    if (!gcb89_parse_long(text, &v)) return 0;
    *out = (int)v;
    return 1;
}

static int gcb89_parse_spread_mode(const char *text, int *out)
{
    long v;
    if (strcmp(text, "none") == 0) { *out = GC89_SPREAD_NONE; return 1; }
    if (strcmp(text, "gap") == 0) { *out = GC89_SPREAD_GAP; return 1; }
    if (strcmp(text, "shape") == 0 || strcmp(text, "shape_size") == 0) {
        *out = GC89_SPREAD_SHAPE_SIZE; return 1;
    }
    if (strcmp(text, "both") == 0) { *out = GC89_SPREAD_BOTH; return 1; }
    if (!gcb89_parse_long(text, &v)) return 0;
    *out = (int)v;
    return 1;
}

static int gcb89_parse_fx_value(const char *key,
                                const char *text,
                                GC89_Fixed *out)
{
    long v;
    size_t n;
    if (!gcb89_parse_long(text, &v)) return 0;
    n = strlen(key);
    if (n >= 3U && strcmp(key + n - 3U, "_px") == 0) {
        *out = GC89_FX_FROM_INT(v);
    } else if (n >= 8U && strcmp(key + n - 8U, "_percent") == 0) {
        *out = (GC89_Fixed)((v * GC89_FX_ONE) / 100L);
    } else {
        *out = (GC89_Fixed)v;
    }
    return 1;
}

static GC89_Variant *gcb89_section_variant(GCB89_RecipePresetData *preset,
                                            const char *section)
{
    if (strcmp(section, "normal") == 0) return &preset->style.normal;
    if (strcmp(section, "aim") == 0) return &preset->style.aim;
    if (strcmp(section, "fire") == 0) return &preset->style.fire;
    if (strcmp(section, "hit") == 0) return &preset->style.hit;
    return 0;
}

static int gcb89_apply_variant_key(GCB89_RecipePresetData *preset,
                                   const char *section,
                                   const char *key,
                                   const char *value)
{
    GC89_Variant *v;
    GC89_Variant *src;
    long lv;
    unsigned long uv;
    int iv;
    v = gcb89_section_variant(preset, section);
    if (!v) return 0;

    if (strcmp(key, "inherit") == 0) {
        src = gcb89_section_variant(preset, value);
        if (!src) return 0;
        *v = *src;
        return 1;
    }
    if (strcmp(key, "draw_mode") == 0) {
        if (!gcb89_parse_draw_mode(value, &iv)) return 0;
        v->draw_mode = iv; return 1;
    }
    if (strcmp(key, "arm_mask") == 0) {
        if (!gcb89_parse_long(value, &lv)) return 0;
        v->arm_mask = (int)lv; return 1;
    }
    if (strcmp(key, "dot_enabled") == 0) {
        if (!gcb89_parse_bool(value, &iv)) return 0;
        v->dot_enabled = iv; return 1;
    }
    if (strcmp(key, "gap_px") == 0 || strcmp(key, "gap_fx") == 0) {
        return gcb89_parse_fx_value(key, value, &v->gap_fx);
    }
    if (strcmp(key, "arm_length_px") == 0 || strcmp(key, "arm_length_fx") == 0) {
        return gcb89_parse_fx_value(key, value, &v->arm_length_fx);
    }
    if (strcmp(key, "thickness_px") == 0 || strcmp(key, "thickness_fx") == 0) {
        return gcb89_parse_fx_value(key, value, &v->thickness_fx);
    }
    if (strcmp(key, "dot_size_px") == 0 || strcmp(key, "dot_size_fx") == 0) {
        return gcb89_parse_fx_value(key, value, &v->dot_size_fx);
    }
    if (strcmp(key, "image_id") == 0) {
        if (!gcb89_parse_long(value, &lv)) return 0;
        v->image_id = (int)lv; return 1;
    }
    if (strcmp(key, "image_width_px") == 0 || strcmp(key, "image_width_fx") == 0) {
        return gcb89_parse_fx_value(key, value, &v->image_width_fx);
    }
    if (strcmp(key, "image_height_px") == 0 || strcmp(key, "image_height_fx") == 0) {
        return gcb89_parse_fx_value(key, value, &v->image_height_fx);
    }
    if (strcmp(key, "color") == 0 || strcmp(key, "color_rgba") == 0) {
        if (!gcb89_parse_ulong(value, &uv)) return 0;
        v->color_rgba = uv; return 1;
    }
    if (strcmp(key, "image_tint") == 0 || strcmp(key, "image_tint_rgba") == 0) {
        if (!gcb89_parse_ulong(value, &uv)) return 0;
        v->image_tint_rgba = uv; return 1;
    }
    if (strcmp(key, "shape") == 0 || strcmp(key, "shape_type") == 0) {
        if (!gcb89_parse_shape(value, &iv)) return 0;
        v->shape_type = iv; return 1;
    }
    if (strcmp(key, "segment_mask") == 0 || strcmp(key, "shape_segment_mask") == 0) {
        if (!gcb89_parse_long(value, &lv)) return 0;
        v->shape_segment_mask = (int)lv; return 1;
    }
    if (strcmp(key, "direction_mask") == 0 || strcmp(key, "shape_direction_mask") == 0) {
        if (!gcb89_parse_long(value, &lv)) return 0;
        v->shape_direction_mask = (int)lv; return 1;
    }
    if (strcmp(key, "spread_mode") == 0) {
        if (!gcb89_parse_spread_mode(value, &iv)) return 0;
        v->spread_mode = iv; return 1;
    }
    if (strcmp(key, "radius_x_px") == 0 || strcmp(key, "shape_radius_x_fx") == 0) {
        return gcb89_parse_fx_value(key, value, &v->shape_radius_x_fx);
    }
    if (strcmp(key, "radius_y_px") == 0 || strcmp(key, "shape_radius_y_fx") == 0) {
        return gcb89_parse_fx_value(key, value, &v->shape_radius_y_fx);
    }
    if (strcmp(key, "depth_px") == 0 || strcmp(key, "shape_depth_fx") == 0) {
        return gcb89_parse_fx_value(key, value, &v->shape_depth_fx);
    }
    if (strcmp(key, "rotation_deg") == 0 || strcmp(key, "shape_rotation_deg_fx") == 0) {
        if (strcmp(key, "rotation_deg") == 0) {
            if (!gcb89_parse_long(value, &lv)) return 0;
            v->shape_rotation_deg_fx = GC89_FX_FROM_INT(lv);
            return 1;
        }
        return gcb89_parse_fx_value(key, value, &v->shape_rotation_deg_fx);
    }
    if (strcmp(key, "break_px") == 0 || strcmp(key, "shape_break_fx") == 0) {
        return gcb89_parse_fx_value(key, value, &v->shape_break_fx);
    }
    if (strcmp(key, "outline_enabled") == 0) {
        if (!gcb89_parse_bool(value, &iv)) return 0;
        v->outline_enabled = iv; return 1;
    }
    if (strcmp(key, "outline_width_px") == 0 || strcmp(key, "outline_width_fx") == 0) {
        return gcb89_parse_fx_value(key, value, &v->outline_width_fx);
    }
    if (strcmp(key, "outline_color") == 0 || strcmp(key, "outline_color_rgba") == 0) {
        if (!gcb89_parse_ulong(value, &uv)) return 0;
        v->outline_color_rgba = uv; return 1;
    }
    return 0;
}


static int gcb89_parse_anim_curve(const char *text, int *out)
{
    long v;
    if (strcmp(text, "linear") == 0) { *out = GCB89_ANIM_CURVE_LINEAR; return 1; }
    if (strcmp(text, "ease_in") == 0) { *out = GCB89_ANIM_CURVE_EASE_IN; return 1; }
    if (strcmp(text, "ease_out") == 0) { *out = GCB89_ANIM_CURVE_EASE_OUT; return 1; }
    if (strcmp(text, "smoothstep") == 0 || strcmp(text, "smooth") == 0) {
        *out = GCB89_ANIM_CURVE_SMOOTHSTEP; return 1;
    }
    if (strcmp(text, "snap") == 0) { *out = GCB89_ANIM_CURVE_SNAP; return 1; }
    if (!gcb89_parse_long(text, &v)) return 0;
    *out = (int)v; return 1;
}

static int gcb89_parse_anim_scale_target(const char *text, int *out)
{
    long v;
    if (strcmp(text, "neutral") == 0) { *out = GCB89_ANIM_SCALE_NEUTRAL; return 1; }
    if (strcmp(text, "micro") == 0 || strcmp(text, "legacy_micro") == 0) {
        *out = GCB89_ANIM_SCALE_MICRO; return 1;
    }
    if (strcmp(text, "maxi") == 0 || strcmp(text, "legacy_maxi") == 0) {
        *out = GCB89_ANIM_SCALE_MAXI; return 1;
    }
    if (strcmp(text, "custom") == 0) { *out = GCB89_ANIM_SCALE_CUSTOM; return 1; }
    if (strcmp(text, "current") == 0) { *out = GCB89_ANIM_SCALE_CURRENT; return 1; }
    if (!gcb89_parse_long(text, &v)) return 0;
    *out = (int)v; return 1;
}

static int gcb89_parse_anim_return(const char *text, int *out)
{
    long v;
    if (strcmp(text, "none") == 0 || strcmp(text, "hold") == 0) {
        *out = GCB89_ANIM_RETURN_NONE; return 1;
    }
    if (strcmp(text, "start") == 0 || strcmp(text, "previous") == 0) {
        *out = GCB89_ANIM_RETURN_START; return 1;
    }
    if (strcmp(text, "neutral") == 0) { *out = GCB89_ANIM_RETURN_NEUTRAL; return 1; }
    if (!gcb89_parse_long(text, &v)) return 0;
    *out = (int)v; return 1;
}

static int gcb89_parse_anim_retrigger(const char *text, int *out)
{
    long v;
    if (strcmp(text, "restart") == 0 || strcmp(text, "from_current") == 0) {
        *out = GCB89_ANIM_RETRIGGER_RESTART; return 1;
    }
    if (strcmp(text, "ignore") == 0) { *out = GCB89_ANIM_RETRIGGER_IGNORE; return 1; }
    if (!gcb89_parse_long(text, &v)) return 0;
    *out = (int)v; return 1;
}

static int gcb89_animation_event_index(const char *section)
{
    const char *name;
    if (strncmp(section, "event.", 6U) == 0) name = section + 6;
    else if (strncmp(section, "animation.event.", 16U) == 0) name = section + 16;
    else return -1;
    if (strcmp(name, "aim_enter") == 0) return GCB89_ANIM_EVENT_AIM_ENTER;
    if (strcmp(name, "aim_exit") == 0) return GCB89_ANIM_EVENT_AIM_EXIT;
    if (strcmp(name, "fire") == 0) return GCB89_ANIM_EVENT_FIRE;
    if (strcmp(name, "hit") == 0) return GCB89_ANIM_EVENT_HIT;
    if (strcmp(name, "disabled") == 0) return GCB89_ANIM_EVENT_DISABLED;
    if (strcmp(name, "enabled") == 0) return GCB89_ANIM_EVENT_ENABLED;
    if (strcmp(name, "custom1") == 0) return GCB89_ANIM_EVENT_CUSTOM1;
    if (strcmp(name, "custom2") == 0) return GCB89_ANIM_EVENT_CUSTOM2;
    return -1;
}

static void gcb89_init_animation_recipe(GCB89_AnimationRecipe *recipe)
{
    int i;
    memset(recipe, 0, sizeof(*recipe));
    recipe->auto_input_events = 1;
    for (i = 0; i < GCB89_ANIM_EVENT_COUNT; ++i) {
        recipe->events[i].scale_target = GCB89_ANIM_SCALE_NEUTRAL;
        recipe->events[i].scale_fx = GC89_FX_ONE;
        recipe->events[i].alpha_fx = GC89_FX_ONE;
        recipe->events[i].thickness_scale_fx = GC89_FX_ONE;
        recipe->events[i].dot_scale_fx = GC89_FX_ONE;
        recipe->events[i].attack_curve = GCB89_ANIM_CURVE_SMOOTHSTEP;
        recipe->events[i].return_curve = GCB89_ANIM_CURVE_SMOOTHSTEP;
        recipe->events[i].return_mode = GCB89_ANIM_RETURN_START;
        recipe->events[i].repeat_count = 1;
        recipe->events[i].retrigger_mode = GCB89_ANIM_RETRIGGER_RESTART;
    }
}

static int gcb89_apply_animation_event_key(GCB89_RecipePresetData *preset,
                                            int event_index,
                                            const char *key,
                                            const char *value)
{
    GCB89_AnimationEventRecipe *e;
    long lv;
    int iv;
    if (event_index < 0 || event_index >= GCB89_ANIM_EVENT_COUNT) return 0;
    e = &preset->animation_recipe.events[event_index];
    if (strcmp(key, "enabled") == 0) {
        if (!gcb89_parse_bool(value, &iv)) return 0;
        e->enabled = iv;
        return 1;
    }
    if (strcmp(key, "scale_target") == 0 || strcmp(key, "target") == 0) {
        if (!gcb89_parse_anim_scale_target(value, &iv)) return 0;
        e->scale_target = iv;
        return 1;
    }
    if (strcmp(key, "scale_percent") == 0 || strcmp(key, "scale_fx") == 0) {
        if (!gcb89_parse_fx_value(key, value, &e->scale_fx)) return 0;
        e->scale_target = GCB89_ANIM_SCALE_CUSTOM; return 1;
    }
    if (strcmp(key, "rotation_deg") == 0 || strcmp(key, "rotation_deg_fx") == 0) {
        if (strcmp(key, "rotation_deg") == 0) {
            if (!gcb89_parse_long(value, &lv)) return 0;
            e->rotation_deg_fx = GC89_FX_FROM_INT(lv); return 1;
        }
        return gcb89_parse_fx_value(key, value, &e->rotation_deg_fx);
    }
    if (strcmp(key, "offset_x_px") == 0 || strcmp(key, "offset_x_fx") == 0)
        return gcb89_parse_fx_value(key, value, &e->offset_x_fx);
    if (strcmp(key, "offset_y_px") == 0 || strcmp(key, "offset_y_fx") == 0)
        return gcb89_parse_fx_value(key, value, &e->offset_y_fx);
    if (strcmp(key, "alpha_percent") == 0 || strcmp(key, "alpha_fx") == 0)
        return gcb89_parse_fx_value(key, value, &e->alpha_fx);
    if (strcmp(key, "thickness_percent") == 0 || strcmp(key, "thickness_scale_fx") == 0)
        return gcb89_parse_fx_value(key, value, &e->thickness_scale_fx);
    if (strcmp(key, "dot_percent") == 0 || strcmp(key, "dot_scale_fx") == 0)
        return gcb89_parse_fx_value(key, value, &e->dot_scale_fx);
    if (strcmp(key, "attack_ticks") == 0 || strcmp(key, "hold_ticks") == 0 ||
        strcmp(key, "return_ticks") == 0) {
        if (!gcb89_parse_long(value, &lv) || lv < 0 || lv > 65535L) return 0;
        if (strcmp(key, "attack_ticks") == 0) e->attack_ticks = (unsigned long)lv;
        else if (strcmp(key, "hold_ticks") == 0) e->hold_ticks = (unsigned long)lv;
        else e->return_ticks = (unsigned long)lv;
        return 1;
    }
    if (strcmp(key, "attack_curve") == 0 || strcmp(key, "curve") == 0) {
        if (!gcb89_parse_anim_curve(value, &iv)) return 0;
        e->attack_curve = iv;
        return 1;
    }
    if (strcmp(key, "return_curve") == 0) {
        if (!gcb89_parse_anim_curve(value, &iv)) return 0;
        e->return_curve = iv;
        return 1;
    }
    if (strcmp(key, "return_mode") == 0 || strcmp(key, "return_to") == 0) {
        if (!gcb89_parse_anim_return(value, &iv)) return 0;
        e->return_mode = iv;
        return 1;
    }
    if (strcmp(key, "repeat_count") == 0) {
        if (!gcb89_parse_long(value, &lv) || lv < 1 || lv > 32L) return 0;
        e->repeat_count = (int)lv; return 1;
    }
    if (strcmp(key, "retrigger") == 0) {
        if (!gcb89_parse_anim_retrigger(value, &iv)) return 0;
        e->retrigger_mode = iv;
        return 1;
    }
    return 0;
}

static int gcb89_apply_style_key(GCB89_RecipePresetData *preset,
                                 const char *key,
                                 const char *value)
{
    long lv;
    int iv;
    if (strcmp(key, "use_aim_variant") == 0) {
        if (!gcb89_parse_bool(value, &iv)) return 0;
        preset->style.use_aim_variant = iv; return 1;
    }
    if (strcmp(key, "use_fire_variant") == 0) {
        if (!gcb89_parse_bool(value, &iv)) return 0;
        preset->style.use_fire_variant = iv; return 1;
    }
    if (strcmp(key, "use_hit_variant") == 0) {
        if (!gcb89_parse_bool(value, &iv)) return 0;
        preset->style.use_hit_variant = iv; return 1;
    }
    if (strcmp(key, "color_change_enabled") == 0) {
        if (!gcb89_parse_bool(value, &iv)) return 0;
        preset->style.color_change_enabled = iv; return 1;
    }
    if (strcmp(key, "spread_multiplier_fx") == 0 ||
        strcmp(key, "spread_multiplier_percent") == 0) {
        return gcb89_parse_fx_value(key, value, &preset->style.spread_multiplier_fx);
    }
    if (strcmp(key, "center_offset_x_px") == 0 || strcmp(key, "center_offset_x_fx") == 0) {
        return gcb89_parse_fx_value(key, value, &preset->style.center_offset_x_fx);
    }
    if (strcmp(key, "center_offset_y_px") == 0 || strcmp(key, "center_offset_y_fx") == 0) {
        return gcb89_parse_fx_value(key, value, &preset->style.center_offset_y_fx);
    }
    if (strcmp(key, "reserved") == 0) {
        if (!gcb89_parse_long(value, &lv)) return 0;
        return 1;
    }
    return 0;
}

static int gcb89_apply_animation_key(GCB89_RecipePresetData *preset,
                                     const char *key,
                                     const char *value)
{
    if (strcmp(key, "micro_scale_fx") == 0 || strcmp(key, "micro_scale_percent") == 0) {
        return gcb89_parse_fx_value(key, value, &preset->animation.micro_scale_fx);
    }
    if (strcmp(key, "neutral_scale_fx") == 0 || strcmp(key, "neutral_scale_percent") == 0) {
        return gcb89_parse_fx_value(key, value, &preset->animation.neutral_scale_fx);
    }
    if (strcmp(key, "maxi_scale_fx") == 0 || strcmp(key, "maxi_scale_percent") == 0) {
        return gcb89_parse_fx_value(key, value, &preset->animation.maxi_scale_fx);
    }
    if (strcmp(key, "speed_fx_per_tick") == 0 || strcmp(key, "speed_percent") == 0) {
        return gcb89_parse_fx_value(key, value, &preset->animation.speed_fx_per_tick);
    }
    return 0;
}

static int gcb89_apply_preset_key(GCB89_RecipePresetData *preset,
                                  const char *key,
                                  const char *value)
{
    long lv;
    if (strcmp(key, "id") == 0) {
        if (!gcb89_parse_long(value, &lv)) return 0;
        preset->id = (int)lv; return 1;
    }
    if (strcmp(key, "name") == 0) {
        gcb89_copy_text(preset->name, sizeof(preset->name), value); return 1;
    }
    if (strcmp(key, "category") == 0) {
        gcb89_copy_text(preset->category, sizeof(preset->category), value); return 1;
    }
    return 0;
}

static int gcb89_parse_section_line(GCB89_RecipePresetData *preset,
                                    const char *section,
                                    const char *key,
                                    const char *value)
{
    if (strcmp(section, "preset") == 0) {
        return gcb89_apply_preset_key(preset, key, value);
    }
    if (strcmp(section, "style") == 0) {
        return gcb89_apply_style_key(preset, key, value);
    }
    if (strcmp(section, "animation") == 0) {
        int iv;
        if (strcmp(key, "auto_input_events") == 0) {
            if (!gcb89_parse_bool(value, &iv)) return 0;
            preset->animation_recipe.auto_input_events = iv;
            return 1;
        }
        return gcb89_apply_animation_key(preset, key, value);
    }
    {
        int event_index;
        event_index = gcb89_animation_event_index(section);
        if (event_index >= 0)
            return gcb89_apply_animation_event_key(preset, event_index, key, value);
    }
    if (gcb89_section_variant(preset, section)) {
        return gcb89_apply_variant_key(preset, section, key, value);
    }
    return 1;
}

static int gcb89_load_preset_fragment(const char *filename,
                                      GCB89_RecipePresetData *preset,
                                      int depth);

static int gcb89_load_fragment_includes(void *fp,
                                        const char *filename,
                                        GCB89_RecipePresetData *preset,
                                        int depth)
{
    char line[512];
    char section[64];
    char child[GCB89_RECIPE_MAX_PATH];
    char *p;
    char *eq;
    char *key;
    char *value;
    size_t n;
    section[0] = '\0';
    while (gcb89_io_read_line(fp, line, (int)sizeof(line))) {
        p = gcb89_trim(line);
        if (*p == '\0' || *p == ';' || *p == '#') continue;
        n = strlen(p);
        if (p[0] == '[' && n >= 2U && p[n - 1U] == ']') {
            p[n - 1U] = '\0';
            gcb89_copy_text(section, sizeof(section), gcb89_trim(p + 1));
            continue;
        }
        if (strcmp(section, "include") != 0) continue;
        eq = strchr(p, '=');
        if (!eq) continue;
        *eq = '\0';
        key = gcb89_trim(p);
        value = gcb89_trim(eq + 1);
        if (strncmp(key, "include", 7U) != 0) continue;
        if (!gcb89_join_path(filename, value, child, sizeof(child))) {
            gcb89_set_error("include path too long"); return 0;
        }
        if (!gcb89_load_preset_fragment(child, preset, depth + 1)) return 0;
    }
    return 1;
}

static int gcb89_load_preset_fragment(const char *filename,
                                      GCB89_RecipePresetData *preset,
                                      int depth)
{
    void *fp;
    char line[512];
    char section[64];
    char *p;
    char *eq;
    char *key;
    char *value;
    size_t n;

    if (depth > GCB89_RECIPE_MAX_INCLUDE_DEPTH) {
        gcb89_set_error("preset include depth exceeded"); return 0;
    }
    fp = gcb89_io_open_read(filename);
    if (!fp) {
        gcb89_set_error_path("cannot open preset/part: ", filename); return 0;
    }
    if (!gcb89_load_fragment_includes(fp, filename, preset, depth)) {
        gcb89_io_close(fp); return 0;
    }
    gcb89_io_close(fp);
    fp = gcb89_io_open_read(filename);
    if (!fp) {
        gcb89_set_error_path("cannot reopen preset/part: ", filename); return 0;
    }
    section[0] = '\0';
    while (gcb89_io_read_line(fp, line, (int)sizeof(line))) {
        p = gcb89_trim(line);
        if (*p == '\0' || *p == ';' || *p == '#') continue;
        n = strlen(p);
        if (p[0] == '[' && n >= 2U && p[n - 1U] == ']') {
            p[n - 1U] = '\0';
            gcb89_copy_text(section, sizeof(section), gcb89_trim(p + 1));
            continue;
        }
        if (strcmp(section, "include") == 0) continue;
        eq = strchr(p, '=');
        if (!eq) continue;
        *eq = '\0';
        key = gcb89_trim(p);
        value = gcb89_trim(eq + 1);
        if (!gcb89_parse_section_line(preset, section, key, value)) {
            gcb89_io_close(fp);
            gcb89_set_error_path("bad preset key/value in: ", filename);
            return 0;
        }
    }
    gcb89_io_close(fp);
    return 1;
}

static int gcb89_store_preset_file(const char *filename)
{
    GCB89_RecipePresetData temp;
    int id;
    memset(&temp, 0, sizeof(temp));
    gcb89_init_animation_recipe(&temp.animation_recipe);
    temp.id = -1;
    if (!gcb89_load_preset_fragment(filename, &temp, 0)) return 0;
    id = temp.id;
    if (id < 0 || id >= GCB89_RECIPE_MAX_PRESETS) {
        gcb89_set_error_path("preset id out of range in: ", filename); return 0;
    }
    if (temp.name[0] == '\0' || temp.category[0] == '\0') {
        gcb89_set_error_path("preset missing name/category in: ", filename); return 0;
    }
    if (g_recipe_presets[id].present) {
        gcb89_set_error_path("duplicate preset id in: ", filename); return 0;
    }
    temp.present = 1;
    g_recipe_presets[id] = temp;
    if (id + 1 > g_recipe_preset_count) g_recipe_preset_count = id + 1;
    return 1;
}

static int gcb89_load_list(const char *filename)
{
    void *fp;
    char line[512];
    char section[64];
    char child[GCB89_RECIPE_MAX_PATH];
    char *p;
    char *eq;
    char *key;
    char *value;
    size_t n;
    fp = gcb89_io_open_read(filename);
    if (!fp) { gcb89_set_error_path("cannot open preset list: ", filename); return 0; }
    section[0] = '\0';
    while (gcb89_io_read_line(fp, line, (int)sizeof(line))) {
        p = gcb89_trim(line);
        if (*p == '\0' || *p == ';' || *p == '#') continue;
        n = strlen(p);
        if (p[0] == '[' && n >= 2U && p[n - 1U] == ']') {
            p[n - 1U] = '\0';
            gcb89_copy_text(section, sizeof(section), gcb89_trim(p + 1));
            continue;
        }
        if (strcmp(section, "presets") != 0) continue;
        eq = strchr(p, '=');
        if (!eq) continue;
        *eq = '\0';
        key = gcb89_trim(p);
        value = gcb89_trim(eq + 1);
        if (strncmp(key, "preset", 6U) != 0) continue;
        if (!gcb89_join_path(filename, value, child, sizeof(child))) {
            gcb89_io_close(fp); gcb89_set_error("preset path too long"); return 0;
        }
        if (!gcb89_store_preset_file(child)) { gcb89_io_close(fp); return 0; }
    }
    gcb89_io_close(fp);
    return 1;
}

static int gcb89_load_assets(const char *filename)
{
    void *fp;
    char line[512];
    char section[64];
    char *p;
    char *eq;
    char *key;
    char *value;
    size_t n;
    long id;
    int slot;
    fp = gcb89_io_open_read(filename);
    if (!fp) { gcb89_set_error_path("cannot open asset catalog: ", filename); return 0; }
    section[0] = '\0';
    while (gcb89_io_read_line(fp, line, (int)sizeof(line))) {
        p = gcb89_trim(line);
        if (*p == '\0' || *p == ';' || *p == '#') continue;
        n = strlen(p);
        if (p[0] == '[' && n >= 2U && p[n - 1U] == ']') {
            p[n - 1U] = '\0';
            gcb89_copy_text(section, sizeof(section), gcb89_trim(p + 1));
            continue;
        }
        if (strcmp(section, "assets") != 0) continue;
        eq = strchr(p, '=');
        if (!eq) continue;
        *eq = '\0';
        key = gcb89_trim(p);
        value = gcb89_trim(eq + 1);
        if (!gcb89_parse_long(key, &id)) continue;
        if (g_recipe_asset_count >= GCB89_RECIPE_MAX_ASSETS) {
            gcb89_io_close(fp); gcb89_set_error("asset capacity exceeded"); return 0;
        }
        slot = g_recipe_asset_count++;
        g_recipe_assets[slot].present = 1;
        g_recipe_assets[slot].image_id = (int)id;
        gcb89_copy_text(g_recipe_assets[slot].filename,
                        sizeof(g_recipe_assets[slot].filename), value);
    }
    gcb89_io_close(fp);
    return 1;
}

void gcb89_recipe_reset(void)
{
    memset(g_recipe_presets, 0, sizeof(g_recipe_presets));
    memset(g_recipe_assets, 0, sizeof(g_recipe_assets));
    g_recipe_preset_count = 0;
    g_recipe_asset_count = 0;
    g_recipe_loaded = 0;
    g_recipe_root[0] = '\0';
    g_recipe_error[0] = '\0';
}

int gcb89_recipe_load_root(const char *root_ini)
{
    void *fp;
    char line[512];
    char section[64];
    char child[GCB89_RECIPE_MAX_PATH];
    char *p;
    char *eq;
    char *key;
    char *value;
    size_t n;
    int i;

    gcb89_recipe_reset();
    if (!root_ini || root_ini[0] == '\0') {
        gcb89_set_error("empty recipe root path"); return 0;
    }
    fp = gcb89_io_open_read(root_ini);
    if (!fp) { gcb89_set_error_path("cannot open recipe root: ", root_ini); return 0; }
    gcb89_copy_text(g_recipe_root, sizeof(g_recipe_root), root_ini);
    section[0] = '\0';
    while (gcb89_io_read_line(fp, line, (int)sizeof(line))) {
        p = gcb89_trim(line);
        if (*p == '\0' || *p == ';' || *p == '#') continue;
        n = strlen(p);
        if (p[0] == '[' && n >= 2U && p[n - 1U] == ']') {
            p[n - 1U] = '\0';
            gcb89_copy_text(section, sizeof(section), gcb89_trim(p + 1));
            continue;
        }
        eq = strchr(p, '=');
        if (!eq) continue;
        *eq = '\0';
        key = gcb89_trim(p);
        value = gcb89_trim(eq + 1);
        if (strcmp(section, "catalog") == 0 && strcmp(key, "assets") == 0) {
            if (!gcb89_join_path(root_ini, value, child, sizeof(child))) {
                gcb89_io_close(fp); gcb89_set_error("asset path too long"); return 0;
            }
            if (!gcb89_load_assets(child)) { gcb89_io_close(fp); return 0; }
        } else if (strcmp(section, "lists") == 0 && strncmp(key, "list", 4U) == 0) {
            if (!gcb89_join_path(root_ini, value, child, sizeof(child))) {
                gcb89_io_close(fp); gcb89_set_error("list path too long"); return 0;
            }
            if (!gcb89_load_list(child)) { gcb89_io_close(fp); return 0; }
        }
    }
    gcb89_io_close(fp);
    if (g_recipe_preset_count <= 0) {
        gcb89_set_error("recipe root loaded zero presets"); return 0;
    }
    for (i = 0; i < g_recipe_preset_count; ++i) {
        if (!g_recipe_presets[i].present) {
            gcb89_set_error("preset IDs must be contiguous from zero"); return 0;
        }
    }
    g_recipe_loaded = 1;
    return 1;
}

int gcb89_recipe_is_loaded(void)
{
    return g_recipe_loaded;
}

const char *gcb89_recipe_last_error(void)
{
    return g_recipe_error;
}

const char *gcb89_recipe_root_path(void)
{
    return g_recipe_root;
}

static int gcb89_recipe_try_default(void)
{
    if (g_recipe_loaded) return 1;
    if (gcb89_recipe_load_root("recipes/gcrosshair.ini")) return 1;
    if (gcb89_recipe_load_root("gcrosshair.ini")) return 1;
    return 0;
}

/* Internal bridge used by gcrosshair_base89.c. */
int gcb89_recipe__ensure(void)
{
    return gcb89_recipe_try_default();
}

int gcb89_recipe__count(void)
{
    return g_recipe_preset_count;
}

int gcb89_recipe__valid(int preset_id)
{
    return g_recipe_loaded && preset_id >= 0 &&
           preset_id < g_recipe_preset_count &&
           g_recipe_presets[preset_id].present;
}

const char *gcb89_recipe__name(int preset_id)
{
    if (!gcb89_recipe__valid(preset_id)) return "invalid";
    return g_recipe_presets[preset_id].name;
}

const char *gcb89_recipe__category(int preset_id)
{
    if (!gcb89_recipe__valid(preset_id)) return "invalid";
    return g_recipe_presets[preset_id].category;
}

const GC89_Style *gcb89_recipe__style(int preset_id)
{
    if (!gcb89_recipe__valid(preset_id)) return 0;
    return &g_recipe_presets[preset_id].style;
}

const GCB89_AnimationPreset *gcb89_recipe__animation(int preset_id)
{
    if (!gcb89_recipe__valid(preset_id)) return 0;
    return &g_recipe_presets[preset_id].animation;
}

const GCB89_AnimationRecipe *gcb89_recipe__animation_recipe(int preset_id)
{
    if (!gcb89_recipe__valid(preset_id)) return 0;
    return &g_recipe_presets[preset_id].animation_recipe;
}

const char *gcb89_recipe__asset_filename(int image_id)
{
    int i;
    if (!g_recipe_loaded) return "";
    for (i = 0; i < g_recipe_asset_count; ++i) {
        if (g_recipe_assets[i].present && g_recipe_assets[i].image_id == image_id) {
            return g_recipe_assets[i].filename;
        }
    }
    return "";
}
