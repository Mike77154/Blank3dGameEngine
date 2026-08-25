#include "generalvideoconfigc89.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static int gvc89_lower(int c)
{
    if (c >= 'A' && c <= 'Z') return c + ('a' - 'A');
    return c;
}

static int gvc89_eq(const char *a, const char *b)
{
    if (!a || !b) return 0;
    while (*a && *b) {
        if (gvc89_lower((unsigned char)*a) !=
            gvc89_lower((unsigned char)*b)) return 0;
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

static void gvc89_copy(char *dst, unsigned int cap, const char *src)
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

static void gvc89_status_set(gvc89_state *state, const char *text)
{
    if (!state) return;
    gvc89_copy(state->status, sizeof(state->status), text);
}

static char *gvc89_trim(char *s)
{
    char *end;
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') ++s;
    end = s + strlen(s);
    while (end > s && (end[-1] == ' ' || end[-1] == '\t' ||
           end[-1] == '\r' || end[-1] == '\n')) --end;
    *end = '\0';
    return s;
}

static int gvc89_bool(const char *s, int fallback)
{
    if (!s) return fallback;
    if (gvc89_eq(s, "true") || gvc89_eq(s, "yes") || gvc89_eq(s, "on"))
        return 1;
    if (gvc89_eq(s, "false") || gvc89_eq(s, "no") || gvc89_eq(s, "off"))
        return 0;
    return atoi(s) != 0;
}

static int gvc89_size(const char *s, int *w, int *h)
{
    int a;
    int b;
    char sep;
    if (!s || !w || !h) return 0;
    a = 0;
    b = 0;
    sep = 0;
    if (sscanf(s, "%d %c %d", &a, &sep, &b) != 3) return 0;
    if (!(sep == 'x' || sep == 'X' || sep == ',' || sep == ':')) return 0;
    if (a < GVC89_MIN_RESOLUTION || b < GVC89_MIN_RESOLUTION ||
        a > GVC89_MAX_RESOLUTION || b > GVC89_MAX_RESOLUTION) return 0;
    *w = a;
    *h = b;
    return 1;
}

void gvc89_defaults(gvc89_state *state)
{
    if (!state) return;
    memset(state, 0, sizeof(*state));
    state->config.resolution_width = 960;
    state->config.resolution_height = 540;
    state->config.scale_mode = GVC89_SCALE_FIT;
    state->config.fixed_scale = 1;
    state->config.keep_aspect = 1;
    state->config.center_output = 1;
    state->config.filter_mode = GVC89_FILTER_NEAREST;
    state->config.vsync = 1;
    state->output_width = 960;
    state->output_height = 540;
    (void)gvc89_compute_presentation(state);
    gvc89_status_set(state, "defaults");
}

void gvc89_set_provider(gvc89_state *state, const gvc89_provider *provider)
{
    if (!state) return;
    memset(&state->provider, 0, sizeof(state->provider));
    state->provider_bound = 0;
    if (provider) {
        state->provider = *provider;
        state->provider_bound = 1;
    }
}

int gvc89_scale_from_name(const char *name, int *out_mode, int *out_fixed)
{
    int mode;
    int fixed;
    const char *x;
    int n;
    if (!name || !out_mode || !out_fixed) return 0;
    mode = 0;
    fixed = 1;
    if (gvc89_eq(name, "native") || gvc89_eq(name, "1:1"))
        mode = GVC89_SCALE_NATIVE;
    else if (gvc89_eq(name, "fit") || gvc89_eq(name, "letterbox"))
        mode = GVC89_SCALE_FIT;
    else if (gvc89_eq(name, "stretch"))
        mode = GVC89_SCALE_STRETCH;
    else if (gvc89_eq(name, "integer") || gvc89_eq(name, "integer_scale"))
        mode = GVC89_SCALE_INTEGER;
    else if (gvc89_eq(name, "overscan") || gvc89_eq(name, "crop"))
        mode = GVC89_SCALE_OVERSCAN;
    else {
        n = atoi(name);
        x = strchr(name, 'x');
        if (!x) x = strchr(name, 'X');
        if (n >= 1 && n <= 16 && x && x[1] == '\0') {
            mode = GVC89_SCALE_FIXED;
            fixed = n;
        }
    }
    if (!mode) return 0;
    *out_mode = mode;
    *out_fixed = fixed;
    return 1;
}

const char *gvc89_scale_name(int mode)
{
    if (mode == GVC89_SCALE_NATIVE) return "native";
    if (mode == GVC89_SCALE_STRETCH) return "stretch";
    if (mode == GVC89_SCALE_INTEGER) return "integer";
    if (mode == GVC89_SCALE_OVERSCAN) return "overscan";
    if (mode == GVC89_SCALE_FIXED) return "fixed";
    return "fit";
}

int gvc89_filter_from_name(const char *name, int *out_mode)
{
    if (!name || !out_mode) return 0;
    if (gvc89_eq(name, "nearest") || gvc89_eq(name, "point")) {
        *out_mode = GVC89_FILTER_NEAREST;
        return 1;
    }
    if (gvc89_eq(name, "linear") || gvc89_eq(name, "bilinear")) {
        *out_mode = GVC89_FILTER_LINEAR;
        return 1;
    }
    return 0;
}

int gvc89_set_resolution(gvc89_state *state, int width, int height)
{
    if (!state || width < GVC89_MIN_RESOLUTION || height < GVC89_MIN_RESOLUTION ||
        width > GVC89_MAX_RESOLUTION || height > GVC89_MAX_RESOLUTION) return 0;
    state->config.resolution_width = width;
    state->config.resolution_height = height;
    state->config.revision++;
    return gvc89_compute_presentation(state);
}

int gvc89_set_output_size(gvc89_state *state, int width, int height)
{
    if (!state || width <= 0 || height <= 0) return 0;
    state->output_width = width;
    state->output_height = height;
    return gvc89_compute_presentation(state);
}

int gvc89_set_scale_mode(gvc89_state *state, int mode, int fixed_scale)
{
    if (!state || mode < GVC89_SCALE_NATIVE || mode > GVC89_SCALE_FIXED) return 0;
    if (mode == GVC89_SCALE_FIXED && (fixed_scale < 1 || fixed_scale > 16)) return 0;
    state->config.scale_mode = mode;
    state->config.fixed_scale = fixed_scale;
    state->config.revision++;
    return gvc89_compute_presentation(state);
}

int gvc89_set_filter(gvc89_state *state, int mode)
{
    if (!state || (mode != GVC89_FILTER_NEAREST && mode != GVC89_FILTER_LINEAR)) return 0;
    state->config.filter_mode = mode;
    state->config.revision++;
    return 1;
}

int gvc89_set_vsync(gvc89_state *state, int enabled)
{
    if (!state) return 0;
    state->config.vsync = enabled ? 1 : 0;
    state->config.revision++;
    return 1;
}

int gvc89_set_keep_aspect(gvc89_state *state, int enabled)
{
    if (!state) return 0;
    state->config.keep_aspect = enabled ? 1 : 0;
    state->config.revision++;
    return gvc89_compute_presentation(state);
}

int gvc89_set_center_output(gvc89_state *state, int enabled)
{
    if (!state) return 0;
    state->config.center_output = enabled ? 1 : 0;
    state->config.revision++;
    return gvc89_compute_presentation(state);
}

int gvc89_compute_presentation(gvc89_state *state)
{
    int rw;
    int rh;
    int ow;
    int oh;
    int w;
    int h;
    int x;
    int y;
    int k;
    if (!state) return 0;
    rw = state->config.resolution_width;
    rh = state->config.resolution_height;
    ow = state->output_width;
    oh = state->output_height;
    if (rw <= 0 || rh <= 0 || ow <= 0 || oh <= 0) return 0;
    w = rw;
    h = rh;
    if (state->config.scale_mode == GVC89_SCALE_STRETCH || !state->config.keep_aspect) {
        w = ow;
        h = oh;
    } else if (state->config.scale_mode == GVC89_SCALE_NATIVE) {
        w = rw;
        h = rh;
    } else if (state->config.scale_mode == GVC89_SCALE_INTEGER) {
        k = ow / rw;
        if (oh / rh < k) k = oh / rh;
        if (k < 1) {
            if (ow * rh <= oh * rw) {
                w = ow;
                h = (rh * ow) / rw;
            } else {
                h = oh;
                w = (rw * oh) / rh;
            }
        } else {
            w = rw * k;
            h = rh * k;
        }
    } else if (state->config.scale_mode == GVC89_SCALE_FIXED) {
        k = state->config.fixed_scale;
        w = rw * k;
        h = rh * k;
    } else if (state->config.scale_mode == GVC89_SCALE_OVERSCAN) {
        if (ow * rh >= oh * rw) {
            w = ow;
            h = (rh * ow + rw - 1) / rw;
        } else {
            h = oh;
            w = (rw * oh + rh - 1) / rh;
        }
    } else {
        if (ow * rh <= oh * rw) {
            w = ow;
            h = (rh * ow) / rw;
        } else {
            h = oh;
            w = (rw * oh) / rh;
        }
    }
    x = state->config.center_output ? (ow - w) / 2 : 0;
    y = state->config.center_output ? (oh - h) / 2 : 0;
    state->presentation.x = x;
    state->presentation.y = y;
    state->presentation.width = w;
    state->presentation.height = h;
    return 1;
}

int gvc89_load_text(gvc89_state *state, const char *text)
{
    char line[256];
    char section[32];
    unsigned int li;
    const char *p;
    if (!state || !text) return 0;
    section[0] = '\0';
    p = text;
    while (*p) {
        char *s;
        char *eq;
        char *comment;
        char *key;
        char *value;
        int w;
        int h;
        int mode;
        int fixed;
        li = 0U;
        while (*p && *p != '\n' && li + 1U < sizeof(line)) line[li++] = *p++;
        while (*p && *p != '\n') ++p;
        if (*p == '\n') ++p;
        line[li] = '\0';
        comment = strchr(line, ';');
        if (comment) *comment = '\0';
        comment = strchr(line, '#');
        if (comment) *comment = '\0';
        s = gvc89_trim(line);
        if (!*s) continue;
        if (*s == '[') {
            char *end = strchr(s, ']');
            if (end) {
                *end = '\0';
                gvc89_copy(section, sizeof(section), gvc89_trim(s + 1));
            }
            continue;
        }
        if (!gvc89_eq(section, "video")) continue;
        eq = strchr(s, '=');
        if (!eq) continue;
        *eq = '\0';
        key = gvc89_trim(s);
        value = gvc89_trim(eq + 1);
        if (gvc89_eq(key, "resolution")) {
            if (gvc89_size(value, &w, &h)) (void)gvc89_set_resolution(state, w, h);
        } else if (gvc89_eq(key, "screenscale") || gvc89_eq(key, "screen_scale")) {
            if (gvc89_scale_from_name(value, &mode, &fixed))
                (void)gvc89_set_scale_mode(state, mode, fixed);
        } else if (gvc89_eq(key, "keepaspect") || gvc89_eq(key, "keep_aspect")) {
            (void)gvc89_set_keep_aspect(state, gvc89_bool(value, state->config.keep_aspect));
        } else if (gvc89_eq(key, "centeroutput") || gvc89_eq(key, "center_output")) {
            (void)gvc89_set_center_output(state, gvc89_bool(value, state->config.center_output));
        } else if (gvc89_eq(key, "vsync")) {
            state->config.vsync = gvc89_bool(value, state->config.vsync);
        } else if (gvc89_eq(key, "filter") || gvc89_eq(key, "scalefilter")) {
            if (gvc89_filter_from_name(value, &mode)) (void)gvc89_set_filter(state, mode);
        }
    }
    state->config.revision++;
    (void)gvc89_compute_presentation(state);
    gvc89_status_set(state, "GeneralConfig [Video] loaded");
    return 1;
}

int gvc89_apply(gvc89_state *state)
{
    if (!state || !state->provider_bound || !state->provider.apply_video) return 0;
    (void)gvc89_compute_presentation(state);
    if (!state->provider.apply_video(state->provider.user,
                                     &state->config,
                                     &state->presentation,
                                     state->output_width,
                                     state->output_height)) {
        gvc89_status_set(state, "video provider apply failed");
        return 0;
    }
    gvc89_status_set(state, "video config applied");
    return 1;
}

const char *gvc89_status(const gvc89_state *state)
{
    return state ? state->status : "video config unavailable";
}
