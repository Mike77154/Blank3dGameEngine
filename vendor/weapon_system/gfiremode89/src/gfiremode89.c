#include "gfiremode89.h"

#include <string.h>

static unsigned short gfm89_sub_ms(unsigned short value, unsigned short dt_ms)
{
    if (dt_ms >= value) return 0u;
    return (unsigned short)(value - dt_ms);
}

static char gfm89_norm_char(char c)
{
    if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
    if (c == '-' || c == ' ' || c == '.') c = '_';
    return c;
}

static int gfm89_streq(const char *a, const char *b)
{
    int i;
    char ca;
    char cb;
    if (!a || !b) return 0;
    for (i = 0; ; i++) {
        ca = gfm89_norm_char(a[i]);
        cb = gfm89_norm_char(b[i]);
        if (ca != cb) return 0;
        if (ca == '\0') return 1;
    }
}

void gfm89_init(GFM89_State *state)
{
    if (!state) return;
    memset(state, 0, sizeof(*state));
    state->mode = GFM89_FIRE_SEMI;
    state->burst_count = 3;
}

int gfm89_configure(GFM89_State *state,
                    int mode,
                    int burst_count,
                    unsigned short shot_interval_ms)
{
    if (!state) return GFM89_BAD_ARG;
    if (mode < GFM89_FIRE_SEMI || mode > GFM89_FIRE_BURST) {
        return GFM89_BAD_ARG;
    }
    if (burst_count <= 0) burst_count = 3;
    state->mode = mode;
    state->burst_count = burst_count;
    state->shot_interval_ms = shot_interval_ms;
    state->trigger_latched = 0;
    state->burst_left = 0;
    state->pending_request = 0;
    state->cooldown_ms_left = 0u;
    return GFM89_OK;
}

void gfm89_reset(GFM89_State *state)
{
    if (!state) return;
    state->trigger_latched = 0;
    state->burst_left = 0;
    state->pending_request = 0;
    state->cooldown_ms_left = 0u;
}

void gfm89_update(GFM89_State *state, unsigned short dt_ms)
{
    if (!state) return;
    state->cooldown_ms_left = gfm89_sub_ms(state->cooldown_ms_left, dt_ms);
}

static int gfm89_trigger_wants_fire(GFM89_State *state, int trigger_flags)
{
    if (state->mode == GFM89_FIRE_AUTO) {
        return (trigger_flags & GFM89_TRIGGER_DOWN) ? 1 : 0;
    }

    if (state->mode == GFM89_FIRE_HOLD_ONCE) {
        if ((trigger_flags & GFM89_TRIGGER_DOWN) && !state->trigger_latched) {
            state->trigger_latched = 1;
            return 1;
        }
        return 0;
    }

    if (state->mode == GFM89_FIRE_BURST) {
        if ((trigger_flags & GFM89_TRIGGER_PRESSED) && state->burst_left <= 0) {
            state->burst_left = state->burst_count;
        }
        if ((trigger_flags & GFM89_TRIGGER_DOWN) && state->burst_left > 0) {
            return 1;
        }
        return 0;
    }

    return (trigger_flags & GFM89_TRIGGER_PRESSED) ? 1 : 0;
}

int gfm89_request(GFM89_State *state,
                  int trigger_flags,
                  unsigned short dt_ms)
{
    int wants_fire;
    if (!state) return GFM89_BAD_ARG;

    gfm89_update(state, dt_ms);

    if (trigger_flags & GFM89_TRIGGER_RELEASED) {
        state->trigger_latched = 0;
    }

    if (state->pending_request) return GFM89_PENDING;

    wants_fire = gfm89_trigger_wants_fire(state, trigger_flags);
    if (!wants_fire) return GFM89_TRIGGER_LOCKED;
    if (state->cooldown_ms_left > 0u) return GFM89_COOLDOWN;

    state->pending_request = 1;
    return GFM89_FIRE_REQUEST;
}

int gfm89_commit_fire(GFM89_State *state)
{
    if (!state) return GFM89_BAD_ARG;
    if (!state->pending_request) return GFM89_NO_PENDING;

    state->pending_request = 0;
    state->cooldown_ms_left = state->shot_interval_ms;
    if (state->mode == GFM89_FIRE_BURST && state->burst_left > 0) {
        state->burst_left--;
    }
    return GFM89_OK;
}

int gfm89_reject_fire(GFM89_State *state)
{
    if (!state) return GFM89_BAD_ARG;
    if (!state->pending_request) return GFM89_NO_PENDING;
    state->pending_request = 0;
    return GFM89_OK;
}

void gfm89_release_latch(GFM89_State *state)
{
    if (!state) return;
    state->trigger_latched = 0;
}

void gfm89_cancel_burst(GFM89_State *state)
{
    if (!state) return;
    state->burst_left = 0;
}

int gfm89_is_ready(const GFM89_State *state)
{
    if (!state) return 0;
    return (!state->pending_request && state->cooldown_ms_left == 0u) ? 1 : 0;
}

int gfm89_is_pending(const GFM89_State *state)
{
    if (!state) return 0;
    return state->pending_request ? 1 : 0;
}

int gfm89_burst_remaining(const GFM89_State *state)
{
    if (!state) return 0;
    return state->burst_left;
}

unsigned short gfm89_cooldown_remaining(const GFM89_State *state)
{
    if (!state) return 0u;
    return state->cooldown_ms_left;
}

int gfm89_mode_from_name(const char *name)
{
    if (gfm89_streq(name, "auto") ||
        gfm89_streq(name, "automatic") ||
        gfm89_streq(name, "continuous")) return GFM89_FIRE_AUTO;
    if (gfm89_streq(name, "hold_once") ||
        gfm89_streq(name, "once_per_hold") ||
        gfm89_streq(name, "shotgun")) return GFM89_FIRE_HOLD_ONCE;
    if (gfm89_streq(name, "burst") ||
        gfm89_streq(name, "rafaga")) return GFM89_FIRE_BURST;
    return GFM89_FIRE_SEMI;
}

const char *gfm89_mode_name(int mode)
{
    if (mode == GFM89_FIRE_AUTO) return "auto";
    if (mode == GFM89_FIRE_HOLD_ONCE) return "hold_once";
    if (mode == GFM89_FIRE_BURST) return "burst";
    return "semi";
}
