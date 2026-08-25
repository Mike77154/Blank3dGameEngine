#include "genvidverbs89.h"
#include <string.h>
#include <stdlib.h>

static const char *acts[] = {
    "video_resolution", "video_output_size", "screen_scale", "video_vsync",
    "video_filter", "video_keep_aspect", "video_center_output", "video_apply"
};
static const char *conds[] = {
    "video_integer_scale", "video_letterboxed", "video_is_native",
    "video_is_stretched", "video_is_overscan", "video_vsync_enabled",
    "video_filter_nearest", "video_filter_linear"
};
static int eq(const char *a, const char *b) { return a && b && strcmp(a, b) == 0; }
int gvvrb89_action_count(void) { return (int)(sizeof(acts) / sizeof(acts[0])); }
const char *gvvrb89_action_name(int n) { return n >= 0 && n < gvvrb89_action_count() ? acts[n] : (const char *)0; }
int gvvrb89_condition_count(void) { return (int)(sizeof(conds) / sizeof(conds[0])); }
const char *gvvrb89_condition_name(int n) { return n >= 0 && n < gvvrb89_condition_count() ? conds[n] : (const char *)0; }

int gvvrb89_perform(gvc89_state *state, const char *name, const char **argv, int argc)
{
    int mode;
    int fixed;
    if (!state || !name) return GVVRB89_ERROR;
    if (eq(name, "video_resolution") && argc >= 2) {
        if (!gvc89_set_resolution(state, atoi(argv[0]), atoi(argv[1]))) return GVVRB89_ERROR;
        return gvc89_apply(state) ? GVVRB89_HANDLED : GVVRB89_ERROR;
    }
    if (eq(name, "video_output_size") && argc >= 2) {
        if (!gvc89_set_output_size(state, atoi(argv[0]), atoi(argv[1]))) return GVVRB89_ERROR;
        return gvc89_apply(state) ? GVVRB89_HANDLED : GVVRB89_ERROR;
    }
    if (eq(name, "screen_scale") && argc >= 1) {
        if (!gvc89_scale_from_name(argv[0], &mode, &fixed) ||
            !gvc89_set_scale_mode(state, mode, fixed)) return GVVRB89_ERROR;
        return gvc89_apply(state) ? GVVRB89_HANDLED : GVVRB89_ERROR;
    }
    if (eq(name, "video_vsync") && argc >= 1) {
        if (!gvc89_set_vsync(state, atoi(argv[0]) != 0)) return GVVRB89_ERROR;
        return gvc89_apply(state) ? GVVRB89_HANDLED : GVVRB89_ERROR;
    }
    if (eq(name, "video_filter") && argc >= 1) {
        if (!gvc89_filter_from_name(argv[0], &mode) || !gvc89_set_filter(state, mode)) return GVVRB89_ERROR;
        return gvc89_apply(state) ? GVVRB89_HANDLED : GVVRB89_ERROR;
    }
    if (eq(name, "video_keep_aspect") && argc >= 1) {
        if (!gvc89_set_keep_aspect(state, atoi(argv[0]) != 0)) return GVVRB89_ERROR;
        return gvc89_apply(state) ? GVVRB89_HANDLED : GVVRB89_ERROR;
    }
    if (eq(name, "video_center_output") && argc >= 1) {
        if (!gvc89_set_center_output(state, atoi(argv[0]) != 0)) return GVVRB89_ERROR;
        return gvc89_apply(state) ? GVVRB89_HANDLED : GVVRB89_ERROR;
    }
    if (eq(name, "video_apply"))
        return gvc89_apply(state) ? GVVRB89_HANDLED : GVVRB89_ERROR;
    return GVVRB89_UNHANDLED;
}

int gvvrb89_query(const gvc89_state *state, const char *name, int *out_truth)
{
    if (!state || !name || !out_truth) return GVVRB89_ERROR;
    if (eq(name, "video_integer_scale"))
        *out_truth = state->config.scale_mode == GVC89_SCALE_INTEGER;
    else if (eq(name, "video_letterboxed"))
        *out_truth = state->presentation.width < state->output_width ||
                     state->presentation.height < state->output_height;
    else if (eq(name, "video_is_native"))
        *out_truth = state->config.scale_mode == GVC89_SCALE_NATIVE;
    else if (eq(name, "video_is_stretched"))
        *out_truth = state->config.scale_mode == GVC89_SCALE_STRETCH;
    else if (eq(name, "video_is_overscan"))
        *out_truth = state->config.scale_mode == GVC89_SCALE_OVERSCAN;
    else if (eq(name, "video_vsync_enabled"))
        *out_truth = state->config.vsync != 0;
    else if (eq(name, "video_filter_nearest"))
        *out_truth = state->config.filter_mode == GVC89_FILTER_NEAREST;
    else if (eq(name, "video_filter_linear"))
        *out_truth = state->config.filter_mode == GVC89_FILTER_LINEAR;
    else
        return GVVRB89_UNHANDLED;
    return GVVRB89_HANDLED;
}
