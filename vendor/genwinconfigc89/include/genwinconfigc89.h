#ifndef GENWINCONFIGC89_H
#define GENWINCONFIGC89_H

/*
 * GenWinConfigC89
 * OS/window-system-agnostic window configuration and lifecycle state.
 * ISO C89, fixed-capacity, no heap, no floating point, no 64-bit types.
 * A platform provider owns the actual native window.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define GWC89_TITLE_CAP 128
#define GWC89_MIN_SIZE 64
#define GWC89_MAX_SIZE 16384

#define GWC89_MODE_WINDOWED 1
#define GWC89_MODE_FULLSCREEN 2
#define GWC89_MODE_BORDERLESS 3
#define GWC89_MODE_BORDERLESS_FULLSCREEN 4

typedef struct gwc89_config {
    int client_width;
    int client_height;
    int position_x;
    int position_y;
    int position_auto;
    int center_on_create;
    int resizable;
    int decorated;
    int visible;
    int always_on_top;
    int play_mode;
    int monitor_index;
    char title[GWC89_TITLE_CAP];
    unsigned int revision;
} gwc89_config;

typedef struct gwc89_provider {
    void *user;
    int (*create_window)(void *user, const gwc89_config *config);
    int (*apply_window)(void *user, const gwc89_config *config);
    int (*destroy_window)(void *user);
    int (*query_client_size)(void *user, int *out_width, int *out_height);
} gwc89_provider;

typedef struct gwc89_state {
    gwc89_config config;
    gwc89_provider provider;
    int provider_bound;
    int created;
    int observed_client_width;
    int observed_client_height;
    char status[96];
} gwc89_state;

void gwc89_defaults(gwc89_state *state);
void gwc89_set_provider(gwc89_state *state, const gwc89_provider *provider);
int gwc89_load_text(gwc89_state *state, const char *text);
int gwc89_set_client_size(gwc89_state *state, int width, int height);
int gwc89_set_position(gwc89_state *state, int x, int y);
int gwc89_set_title(gwc89_state *state, const char *title);
int gwc89_set_play_mode(gwc89_state *state, int play_mode);
int gwc89_set_resizable(gwc89_state *state, int enabled);
int gwc89_set_decorated(gwc89_state *state, int enabled);
int gwc89_set_visible(gwc89_state *state, int enabled);
int gwc89_set_topmost(gwc89_state *state, int enabled);
int gwc89_set_center_on_create(gwc89_state *state, int enabled);
int gwc89_set_monitor_index(gwc89_state *state, int monitor_index);
int gwc89_set_observed_client_size(gwc89_state *state, int width, int height);
int gwc89_create(gwc89_state *state);
int gwc89_apply(gwc89_state *state);
int gwc89_destroy(gwc89_state *state);
int gwc89_refresh(gwc89_state *state);
int gwc89_mode_from_name(const char *name, int *out_mode);
const char *gwc89_mode_name(int mode);
const char *gwc89_status(const gwc89_state *state);

#ifdef __cplusplus
}
#endif

#endif
