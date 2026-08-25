#include "gmplyss89.h"
#include <string.h>
#include <stdlib.h>

static const char *acts[] = {
    "camera_size_draw", "scene_screen_size", "camera_screen_pos",
    "camera_screen_clamp", "gameplay_screen_apply"
};
static const char *conds[] = {
    "camera_fits_scene", "camera_clamped", "camera_clamp_enabled"
};
static int eq(const char *a, const char *b) { return a && b && strcmp(a, b) == 0; }
int gmplyss89_action_count(void) { return (int)(sizeof(acts) / sizeof(acts[0])); }
const char *gmplyss89_action_name(int n) { return n >= 0 && n < gmplyss89_action_count() ? acts[n] : (const char *)0; }
int gmplyss89_condition_count(void) { return (int)(sizeof(conds) / sizeof(conds[0])); }
const char *gmplyss89_condition_name(int n) { return n >= 0 && n < gmplyss89_condition_count() ? conds[n] : (const char *)0; }

int gmplyss89_perform(gpss89_state *state, const char *name, const char **argv, int argc)
{
    if (!state || !name) return GMPLYSS89_ERROR;
    if (eq(name, "camera_size_draw") && argc >= 2) {
        if (!gpss89_set_camera_draw_size(state, atoi(argv[0]), atoi(argv[1]))) return GMPLYSS89_ERROR;
        return gpss89_apply(state) ? GMPLYSS89_HANDLED : GMPLYSS89_ERROR;
    }
    if (eq(name, "scene_screen_size") && argc >= 2) {
        if (!gpss89_set_scene_screen_size(state, atoi(argv[0]), atoi(argv[1]))) return GMPLYSS89_ERROR;
        return gpss89_apply(state) ? GMPLYSS89_HANDLED : GMPLYSS89_ERROR;
    }
    if (eq(name, "camera_screen_pos") && argc >= 2) {
        if (!gpss89_set_camera_position(state, atoi(argv[0]), atoi(argv[1]))) return GMPLYSS89_ERROR;
        return gpss89_apply(state) ? GMPLYSS89_HANDLED : GMPLYSS89_ERROR;
    }
    if (eq(name, "camera_screen_clamp") && argc >= 1) {
        if (!gpss89_set_clamp(state, atoi(argv[0]) != 0)) return GMPLYSS89_ERROR;
        return gpss89_apply(state) ? GMPLYSS89_HANDLED : GMPLYSS89_ERROR;
    }
    if (eq(name, "gameplay_screen_apply"))
        return gpss89_apply(state) ? GMPLYSS89_HANDLED : GMPLYSS89_ERROR;
    return GMPLYSS89_UNHANDLED;
}

int gmplyss89_query(const gpss89_state *state, const char *name, int *out_truth)
{
    int max_x;
    int max_y;
    if (!state || !name || !out_truth) return GMPLYSS89_ERROR;
    if (eq(name, "camera_fits_scene")) {
        *out_truth = gpss89_camera_fits_scene(state);
    } else if (eq(name, "camera_clamped")) {
        max_x = state->config.scene_screen_width - state->config.camera_draw_width;
        max_y = state->config.scene_screen_height - state->config.camera_draw_height;
        if (max_x < 0) max_x = 0;
        if (max_y < 0) max_y = 0;
        *out_truth = state->config.camera_x >= 0 && state->config.camera_y >= 0 &&
                     state->config.camera_x <= max_x && state->config.camera_y <= max_y;
    } else if (eq(name, "camera_clamp_enabled")) {
        *out_truth = state->config.clamp_camera_to_scene != 0;
    } else {
        return GMPLYSS89_UNHANDLED;
    }
    return GMPLYSS89_HANDLED;
}
