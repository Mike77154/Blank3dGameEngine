#include "gwinvrbs89.h"
#include <string.h>
#include <stdlib.h>

static const char *gw_actions[] = {
    "window", "window_size", "window_mode", "window_center",
    "window_position", "window_title", "window_resizable",
    "window_decorated", "window_visible", "window_topmost",
    "window_monitor", "window_create", "window_destroy",
    "window_refresh", "window_apply"
};
static const char *gw_conditions[] = {
    "window_is_fullscreen", "window_is_windowed", "window_is_borderless",
    "window_is_created", "window_is_visible", "window_is_resizable",
    "window_is_decorated", "window_is_topmost"
};
static int eq(const char *a, const char *b) { return a && b && strcmp(a, b) == 0; }
int gwvrb89_action_count(void) { return (int)(sizeof(gw_actions) / sizeof(gw_actions[0])); }
const char *gwvrb89_action_name(int n) { return n >= 0 && n < gwvrb89_action_count() ? gw_actions[n] : (const char *)0; }
int gwvrb89_condition_count(void) { return (int)(sizeof(gw_conditions) / sizeof(gw_conditions[0])); }
const char *gwvrb89_condition_name(int n) { return n >= 0 && n < gwvrb89_condition_count() ? gw_conditions[n] : (const char *)0; }

int gwvrb89_perform(gwc89_state *state, const char *name, const char **argv, int argc)
{
    int mode;
    if (!state || !name) return GWVRB89_ERROR;
    if ((eq(name, "window") || eq(name, "window_size")) && argc >= 2) {
        if (!gwc89_set_client_size(state, atoi(argv[0]), atoi(argv[1]))) return GWVRB89_ERROR;
        if (state->created && !gwc89_apply(state)) return GWVRB89_ERROR;
        return GWVRB89_HANDLED;
    }
    if (eq(name, "window_mode") && argc >= 1) {
        if (!gwc89_mode_from_name(argv[0], &mode) || !gwc89_set_play_mode(state, mode)) return GWVRB89_ERROR;
        if (state->created && !gwc89_apply(state)) return GWVRB89_ERROR;
        return GWVRB89_HANDLED;
    }
    if (eq(name, "window_center")) {
        if (!gwc89_set_center_on_create(state, 1)) return GWVRB89_ERROR;
        if (state->created && !gwc89_apply(state)) return GWVRB89_ERROR;
        return GWVRB89_HANDLED;
    }
    if (eq(name, "window_position") && argc >= 2) {
        if (!gwc89_set_position(state, atoi(argv[0]), atoi(argv[1]))) return GWVRB89_ERROR;
        if (state->created && !gwc89_apply(state)) return GWVRB89_ERROR;
        return GWVRB89_HANDLED;
    }
    if (eq(name, "window_title") && argc >= 1) {
        if (!gwc89_set_title(state, argv[0])) return GWVRB89_ERROR;
        if (state->created && !gwc89_apply(state)) return GWVRB89_ERROR;
        return GWVRB89_HANDLED;
    }
    if (eq(name, "window_resizable") && argc >= 1) {
        if (!gwc89_set_resizable(state, atoi(argv[0]) != 0)) return GWVRB89_ERROR;
        if (state->created && !gwc89_apply(state)) return GWVRB89_ERROR;
        return GWVRB89_HANDLED;
    }
    if (eq(name, "window_decorated") && argc >= 1) {
        if (!gwc89_set_decorated(state, atoi(argv[0]) != 0)) return GWVRB89_ERROR;
        if (state->created && !gwc89_apply(state)) return GWVRB89_ERROR;
        return GWVRB89_HANDLED;
    }
    if (eq(name, "window_visible") && argc >= 1) {
        if (!gwc89_set_visible(state, atoi(argv[0]) != 0)) return GWVRB89_ERROR;
        if (state->created && !gwc89_apply(state)) return GWVRB89_ERROR;
        return GWVRB89_HANDLED;
    }
    if (eq(name, "window_topmost") && argc >= 1) {
        if (!gwc89_set_topmost(state, atoi(argv[0]) != 0)) return GWVRB89_ERROR;
        if (state->created && !gwc89_apply(state)) return GWVRB89_ERROR;
        return GWVRB89_HANDLED;
    }
    if (eq(name, "window_monitor") && argc >= 1) {
        if (!gwc89_set_monitor_index(state, atoi(argv[0]))) return GWVRB89_ERROR;
        if (state->created && !gwc89_apply(state)) return GWVRB89_ERROR;
        return GWVRB89_HANDLED;
    }
    if (eq(name, "window_create"))
        return gwc89_create(state) ? GWVRB89_HANDLED : GWVRB89_ERROR;
    if (eq(name, "window_destroy"))
        return gwc89_destroy(state) ? GWVRB89_HANDLED : GWVRB89_ERROR;
    if (eq(name, "window_refresh"))
        return gwc89_refresh(state) ? GWVRB89_HANDLED : GWVRB89_ERROR;
    if (eq(name, "window_apply"))
        return gwc89_apply(state) ? GWVRB89_HANDLED : GWVRB89_ERROR;
    return GWVRB89_UNHANDLED;
}

int gwvrb89_query(const gwc89_state *state, const char *name, int *out_truth)
{
    if (!state || !name || !out_truth) return GWVRB89_ERROR;
    if (eq(name, "window_is_fullscreen"))
        *out_truth = state->config.play_mode == GWC89_MODE_FULLSCREEN ||
                     state->config.play_mode == GWC89_MODE_BORDERLESS_FULLSCREEN;
    else if (eq(name, "window_is_windowed"))
        *out_truth = state->config.play_mode == GWC89_MODE_WINDOWED;
    else if (eq(name, "window_is_borderless"))
        *out_truth = state->config.play_mode == GWC89_MODE_BORDERLESS ||
                     state->config.play_mode == GWC89_MODE_BORDERLESS_FULLSCREEN;
    else if (eq(name, "window_is_created"))
        *out_truth = state->created != 0;
    else if (eq(name, "window_is_visible"))
        *out_truth = state->config.visible != 0;
    else if (eq(name, "window_is_resizable"))
        *out_truth = state->config.resizable != 0;
    else if (eq(name, "window_is_decorated"))
        *out_truth = state->config.decorated != 0;
    else if (eq(name, "window_is_topmost"))
        *out_truth = state->config.always_on_top != 0;
    else
        return GWVRB89_UNHANDLED;
    return GWVRB89_HANDLED;
}
