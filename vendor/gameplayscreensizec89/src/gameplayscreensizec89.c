#include "gameplayscreensizec89.h"
#include <string.h>

static void gpss89_copy(char *dst, unsigned int cap, const char *src)
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

static void gpss89_status_set(gpss89_state *state, const char *text)
{
    if (!state) return;
    gpss89_copy(state->status, sizeof(state->status), text);
}

void gpss89_defaults(gpss89_state *state)
{
    if (!state) return;
    memset(state, 0, sizeof(*state));
    state->config.camera_draw_width = 960;
    state->config.camera_draw_height = 540;
    state->config.scene_screen_width = 1920;
    state->config.scene_screen_height = 1080;
    state->config.clamp_camera_to_scene = 1;
    gpss89_status_set(state, "defaults");
}

void gpss89_set_provider(gpss89_state *state, const gpss89_provider *provider)
{
    if (!state) return;
    memset(&state->provider, 0, sizeof(state->provider));
    state->provider_bound = 0;
    if (provider) {
        state->provider = *provider;
        state->provider_bound = 1;
    }
}

int gpss89_set_camera_draw_size(gpss89_state *state, int width, int height)
{
    if (!state || width < GPSS89_MIN_SIZE || height < GPSS89_MIN_SIZE ||
        width > GPSS89_MAX_SIZE || height > GPSS89_MAX_SIZE) return 0;
    state->config.camera_draw_width = width;
    state->config.camera_draw_height = height;
    state->config.revision++;
    gpss89_clamp_camera(state);
    return 1;
}

int gpss89_set_scene_screen_size(gpss89_state *state, int width, int height)
{
    if (!state || width < GPSS89_MIN_SIZE || height < GPSS89_MIN_SIZE ||
        width > GPSS89_MAX_SIZE || height > GPSS89_MAX_SIZE) return 0;
    state->config.scene_screen_width = width;
    state->config.scene_screen_height = height;
    state->config.revision++;
    gpss89_clamp_camera(state);
    return 1;
}

int gpss89_set_camera_position(gpss89_state *state, int x, int y)
{
    if (!state) return 0;
    state->config.camera_x = x;
    state->config.camera_y = y;
    state->config.revision++;
    gpss89_clamp_camera(state);
    return 1;
}

int gpss89_set_clamp(gpss89_state *state, int enabled)
{
    if (!state) return 0;
    state->config.clamp_camera_to_scene = enabled ? 1 : 0;
    state->config.revision++;
    gpss89_clamp_camera(state);
    return 1;
}

void gpss89_clamp_camera(gpss89_state *state)
{
    int max_x;
    int max_y;
    if (!state || !state->config.clamp_camera_to_scene) return;
    max_x = state->config.scene_screen_width - state->config.camera_draw_width;
    max_y = state->config.scene_screen_height - state->config.camera_draw_height;
    if (max_x < 0) max_x = 0;
    if (max_y < 0) max_y = 0;
    if (state->config.camera_x < 0) state->config.camera_x = 0;
    if (state->config.camera_y < 0) state->config.camera_y = 0;
    if (state->config.camera_x > max_x) state->config.camera_x = max_x;
    if (state->config.camera_y > max_y) state->config.camera_y = max_y;
}

int gpss89_camera_fits_scene(const gpss89_state *state)
{
    if (!state) return 0;
    return state->config.camera_draw_width <= state->config.scene_screen_width &&
           state->config.camera_draw_height <= state->config.scene_screen_height;
}

int gpss89_apply(gpss89_state *state)
{
    if (!state || !state->provider_bound || !state->provider.apply_gameplay_screen)
        return 0;
    gpss89_clamp_camera(state);
    if (!state->provider.apply_gameplay_screen(state->provider.user, &state->config)) {
        gpss89_status_set(state, "gameplay screen provider apply failed");
        return 0;
    }
    gpss89_status_set(state, "gameplay screen applied");
    return 1;
}

const char *gpss89_status(const gpss89_state *state)
{
    return state ? state->status : "gameplay screen unavailable";
}
