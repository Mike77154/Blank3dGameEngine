#include "genwinconfigc89.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static int gwc89_ascii_lower(int c)
{
    if (c >= 'A' && c <= 'Z') return c + ('a' - 'A');
    return c;
}

static int gwc89_eq(const char *a, const char *b)
{
    if (!a || !b) return 0;
    while (*a && *b) {
        if (gwc89_ascii_lower((unsigned char)*a) !=
            gwc89_ascii_lower((unsigned char)*b)) return 0;
        ++a; ++b;
    }
    return *a == '\0' && *b == '\0';
}

static void gwc89_copy(char *dst, unsigned int cap, const char *src)
{
    unsigned int i;
    if (!dst || cap == 0U) return;
    if (!src) src = "";
    i = 0U;
    while (src[i] && i + 1U < cap) { dst[i] = src[i]; ++i; }
    dst[i] = '\0';
}

static void gwc89_status_set(gwc89_state *state, const char *text)
{
    if (!state) return;
    gwc89_copy(state->status, sizeof(state->status), text);
}

static char *gwc89_trim(char *s)
{
    char *end;
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') ++s;
    end = s + strlen(s);
    while (end > s && (end[-1] == ' ' || end[-1] == '\t' ||
           end[-1] == '\r' || end[-1] == '\n')) --end;
    *end = '\0';
    return s;
}

static int gwc89_parse_bool(const char *s, int fallback)
{
    if (!s) return fallback;
    if (gwc89_eq(s, "true") || gwc89_eq(s, "yes") || gwc89_eq(s, "on")) return 1;
    if (gwc89_eq(s, "false") || gwc89_eq(s, "no") || gwc89_eq(s, "off")) return 0;
    return atoi(s) != 0;
}

static int gwc89_parse_size(const char *s, int *w, int *h)
{
    int a, b;
    char sep;
    if (!s || !w || !h) return 0;
    a = b = 0; sep = 0;
    if (sscanf(s, "%d %c %d", &a, &sep, &b) != 3) return 0;
    if (!(sep == 'x' || sep == 'X' || sep == ',' || sep == ':')) return 0;
    if (a < GWC89_MIN_SIZE || b < GWC89_MIN_SIZE ||
        a > GWC89_MAX_SIZE || b > GWC89_MAX_SIZE) return 0;
    *w = a; *h = b;
    return 1;
}

void gwc89_defaults(gwc89_state *state)
{
    if (!state) return;
    memset(state, 0, sizeof(*state));
    state->config.client_width = 960;
    state->config.client_height = 540;
    state->config.position_auto = 1;
    state->config.center_on_create = 1;
    state->config.resizable = 1;
    state->config.decorated = 1;
    state->config.visible = 1;
    state->config.always_on_top = 0;
    state->config.play_mode = GWC89_MODE_WINDOWED;
    state->config.monitor_index = 0;
    gwc89_copy(state->config.title, sizeof(state->config.title), "Blank3D");
    state->observed_client_width = state->config.client_width;
    state->observed_client_height = state->config.client_height;
    gwc89_status_set(state, "defaults");
}

void gwc89_set_provider(gwc89_state *state, const gwc89_provider *provider)
{
    if (!state) return;
    memset(&state->provider, 0, sizeof(state->provider));
    state->provider_bound = 0;
    if (provider) {
        state->provider = *provider;
        state->provider_bound = 1;
    }
}

int gwc89_mode_from_name(const char *name, int *out_mode)
{
    int mode;
    if (!name || !out_mode) return 0;
    mode = 0;
    if (gwc89_eq(name, "windowed") || gwc89_eq(name, "window")) mode = GWC89_MODE_WINDOWED;
    else if (gwc89_eq(name, "fullscreen") || gwc89_eq(name, "full")) mode = GWC89_MODE_FULLSCREEN;
    else if (gwc89_eq(name, "borderless")) mode = GWC89_MODE_BORDERLESS;
    else if (gwc89_eq(name, "borderless_fullscreen") ||
             gwc89_eq(name, "borderlessfullscreen") ||
             gwc89_eq(name, "desktop_fullscreen")) mode = GWC89_MODE_BORDERLESS_FULLSCREEN;
    if (!mode) return 0;
    *out_mode = mode;
    return 1;
}

const char *gwc89_mode_name(int mode)
{
    if (mode == GWC89_MODE_FULLSCREEN) return "fullscreen";
    if (mode == GWC89_MODE_BORDERLESS) return "borderless";
    if (mode == GWC89_MODE_BORDERLESS_FULLSCREEN) return "borderless_fullscreen";
    return "windowed";
}

int gwc89_load_text(gwc89_state *state, const char *text)
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
        int w, h, mode;
        li = 0U;
        while (*p && *p != '\n' && li + 1U < sizeof(line)) line[li++] = *p++;
        while (*p && *p != '\n') ++p;
        if (*p == '\n') ++p;
        line[li] = '\0';
        comment = strchr(line, ';'); if (comment) *comment = '\0';
        comment = strchr(line, '#'); if (comment) *comment = '\0';
        s = gwc89_trim(line);
        if (!*s) continue;
        if (*s == '[') {
            char *end = strchr(s, ']');
            if (end) { *end = '\0'; gwc89_copy(section, sizeof(section), gwc89_trim(s + 1)); }
            continue;
        }
        if (!gwc89_eq(section, "window")) continue;
        eq = strchr(s, '=');
        if (!eq) continue;
        *eq = '\0';
        key = gwc89_trim(s);
        value = gwc89_trim(eq + 1);
        if (gwc89_eq(key, "windowsize") || gwc89_eq(key, "window_size")) {
            if (gwc89_parse_size(value, &w, &h)) (void)gwc89_set_client_size(state, w, h);
        } else if (gwc89_eq(key, "playmode") || gwc89_eq(key, "mode")) {
            if (gwc89_mode_from_name(value, &mode)) (void)gwc89_set_play_mode(state, mode);
        } else if (gwc89_eq(key, "resizable")) (void)gwc89_set_resizable(state, gwc89_parse_bool(value, state->config.resizable));
        else if (gwc89_eq(key, "decorated")) (void)gwc89_set_decorated(state, gwc89_parse_bool(value, state->config.decorated));
        else if (gwc89_eq(key, "visible")) (void)gwc89_set_visible(state, gwc89_parse_bool(value, state->config.visible));
        else if (gwc89_eq(key, "alwaysontop") || gwc89_eq(key, "always_on_top")) (void)gwc89_set_topmost(state, gwc89_parse_bool(value, state->config.always_on_top));
        else if (gwc89_eq(key, "center") || gwc89_eq(key, "centered")) (void)gwc89_set_center_on_create(state, gwc89_parse_bool(value, state->config.center_on_create));
        else if (gwc89_eq(key, "title")) (void)gwc89_set_title(state, value);
        else if (gwc89_eq(key, "monitor")) (void)gwc89_set_monitor_index(state, atoi(value));
        else if (gwc89_eq(key, "position")) {
            int x, y; char sep;
            if (sscanf(value, "%d %c %d", &x, &sep, &y) == 3 && (sep == ',' || sep == ':' || sep == 'x' || sep == 'X'))
                (void)gwc89_set_position(state, x, y);
        }
    }
    state->config.revision++;
    gwc89_status_set(state, "GeneralConfig [Window] loaded");
    return 1;
}

int gwc89_set_client_size(gwc89_state *state, int width, int height)
{
    if (!state || width < GWC89_MIN_SIZE || height < GWC89_MIN_SIZE ||
        width > GWC89_MAX_SIZE || height > GWC89_MAX_SIZE) return 0;
    state->config.client_width = width;
    state->config.client_height = height;
    state->config.revision++;
    return 1;
}

int gwc89_set_position(gwc89_state *state, int x, int y)
{
    if (!state) return 0;
    state->config.position_x = x;
    state->config.position_y = y;
    state->config.position_auto = 0;
    state->config.center_on_create = 0;
    state->config.revision++;
    return 1;
}

int gwc89_set_title(gwc89_state *state, const char *title)
{
    if (!state || !title) return 0;
    gwc89_copy(state->config.title, sizeof(state->config.title), title);
    state->config.revision++;
    return 1;
}

int gwc89_set_play_mode(gwc89_state *state, int play_mode)
{
    if (!state || play_mode < GWC89_MODE_WINDOWED || play_mode > GWC89_MODE_BORDERLESS_FULLSCREEN) return 0;
    state->config.play_mode = play_mode;
    state->config.revision++;
    return 1;
}

int gwc89_set_resizable(gwc89_state *state, int enabled)
{
    if (!state) return 0;
    state->config.resizable = enabled ? 1 : 0;
    state->config.revision++;
    return 1;
}

int gwc89_set_decorated(gwc89_state *state, int enabled)
{
    if (!state) return 0;
    state->config.decorated = enabled ? 1 : 0;
    state->config.revision++;
    return 1;
}

int gwc89_set_visible(gwc89_state *state, int enabled)
{
    if (!state) return 0;
    state->config.visible = enabled ? 1 : 0;
    state->config.revision++;
    return 1;
}

int gwc89_set_topmost(gwc89_state *state, int enabled)
{
    if (!state) return 0;
    state->config.always_on_top = enabled ? 1 : 0;
    state->config.revision++;
    return 1;
}

int gwc89_set_center_on_create(gwc89_state *state, int enabled)
{
    if (!state) return 0;
    state->config.center_on_create = enabled ? 1 : 0;
    if (enabled) state->config.position_auto = 1;
    state->config.revision++;
    return 1;
}

int gwc89_set_monitor_index(gwc89_state *state, int monitor_index)
{
    if (!state || monitor_index < 0 || monitor_index > 255) return 0;
    state->config.monitor_index = monitor_index;
    state->config.revision++;
    return 1;
}

int gwc89_set_observed_client_size(gwc89_state *state, int width, int height)
{
    if (!state || width <= 0 || height <= 0) return 0;
    state->observed_client_width = width;
    state->observed_client_height = height;
    return 1;
}

int gwc89_create(gwc89_state *state)
{
    if (!state) return 0;
    if (state->created) return 1;
    if (!state->provider_bound || !state->provider.create_window) return 0;
    if (!state->provider.create_window(state->provider.user, &state->config)) {
        gwc89_status_set(state, "window provider create failed"); return 0;
    }
    state->created = 1;
    (void)gwc89_refresh(state);
    gwc89_status_set(state, "window created by provider");
    return 1;
}

int gwc89_apply(gwc89_state *state)
{
    if (!state || !state->created || !state->provider.apply_window) return 0;
    if (!state->provider.apply_window(state->provider.user, &state->config)) {
        gwc89_status_set(state, "window provider apply failed"); return 0;
    }
    (void)gwc89_refresh(state);
    gwc89_status_set(state, "window config applied");
    return 1;
}

int gwc89_destroy(gwc89_state *state)
{
    if (!state) return 0;
    if (state->created && state->provider.destroy_window &&
        !state->provider.destroy_window(state->provider.user)) return 0;
    state->created = 0;
    gwc89_status_set(state, "window destroyed");
    return 1;
}

int gwc89_refresh(gwc89_state *state)
{
    int w, h;
    if (!state) return 0;
    if (!state->provider.query_client_size) return 1;
    w = h = 0;
    if (!state->provider.query_client_size(state->provider.user, &w, &h)) return 0;
    return gwc89_set_observed_client_size(state, w, h);
}

const char *gwc89_status(const gwc89_state *state)
{
    return state ? state->status : "genwinconfig unavailable";
}
