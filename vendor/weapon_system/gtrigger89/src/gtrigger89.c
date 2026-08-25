#include "gtrigger89.h"
#include <string.h>

#define GTRIGGER89_Q16_ONE 65536L

static unsigned int gtrigger89_add_clamped(unsigned int value,
                                            unsigned int add,
                                            unsigned int max_value)
{
    if (value >= max_value) return max_value;
    if (add >= max_value - value) return max_value;
    return value + add;
}

void gtrigger89_init(gtrigger89_state *state)
{
    if (!state) return;
    memset(state, 0, sizeof(*state));
    state->active_model = GTRIGGER89_MODEL_NORMAL;
    state->initialized = 1;
}

void gtrigger89_reset(gtrigger89_state *state)
{
    gtrigger89_init(state);
}

void gtrigger89_update(gtrigger89_state *state,
                       const gtrigger89_config *config,
                       int raw_down,
                       unsigned int dt_ms,
                       gtrigger89_output *out)
{
    int raw_pressed;
    int raw_released;
    unsigned int limit;
    if (!state || !config || !out) return;
    if (!state->initialized) gtrigger89_init(state);
    memset(out, 0, sizeof(*out));
    raw_down = raw_down ? 1 : 0;
    raw_pressed = raw_down && !state->previous_raw_down;
    raw_released = !raw_down && state->previous_raw_down;

    if (state->active_model != config->model) {
        if (state->active_model == GTRIGGER89_MODEL_SPINUP &&
            (state->elapsed_ms > 0U || state->armed))
            out->spin_end = 1;
        state->elapsed_ms = 0U;
        state->armed = 0;
        state->charging = 0;
        state->active_model = config->model;
        raw_pressed = raw_down ? 1 : 0;
        raw_released = 0;
    }

    if (config->model == GTRIGGER89_MODEL_NORMAL) {
        out->trigger_down = raw_down;
        out->trigger_pressed = raw_pressed;
        out->trigger_released = raw_released;
    } else if (config->model == GTRIGGER89_MODEL_SPINUP) {
        limit = config->spinup_ms;
        if (limit == 0U) limit = 1U;
        if (raw_pressed) {
            state->elapsed_ms = 0U;
            state->armed = 0;
            out->spin_begin = 1;
        }
        if (raw_down && !state->armed) {
            state->elapsed_ms = gtrigger89_add_clamped(state->elapsed_ms,
                                                       dt_ms, limit);
            if (state->elapsed_ms >= limit) {
                state->armed = 1;
                out->trigger_down = 1;
                out->trigger_pressed = 1;
            }
        } else if (raw_down && state->armed) {
            out->trigger_down = 1;
        }
        if (!raw_down && (raw_released || state->elapsed_ms > 0U ||
                          state->armed)) {
            if (state->armed) out->trigger_released = 1;
            out->spin_end = 1;
            state->elapsed_ms = 0U;
            state->armed = 0;
        }
    } else if (config->model == GTRIGGER89_MODEL_CHARGE_RELEASE) {
        limit = config->charge_max_ms;
        if (limit == 0U) limit = 1U;
        if (raw_pressed) {
            state->charging = 1;
            state->elapsed_ms = 0U;
            out->charge_begin = 1;
        }
        if (raw_down && state->charging)
            state->elapsed_ms = gtrigger89_add_clamped(state->elapsed_ms,
                                                       dt_ms, limit);
        if (raw_released && state->charging) {
            out->charge_release = 1;
            out->charge_elapsed_ms = state->elapsed_ms;
            out->charge_ratio_q16 =
                (long)(((unsigned long)state->elapsed_ms *
                        (unsigned long)GTRIGGER89_Q16_ONE) /
                       (unsigned long)limit);
            if (out->charge_ratio_q16 > GTRIGGER89_Q16_ONE)
                out->charge_ratio_q16 = GTRIGGER89_Q16_ONE;
            out->trigger_down = 1;
            out->trigger_pressed = 1;
            state->charging = 0;
            state->elapsed_ms = 0U;
        }
    } else {
        /* Buster-style policy: press is an ordinary shot, hold accumulates a
           charge duration, release publishes that duration. The host decides
           whether the release duration activates a second shot. */
        limit = config->charge_max_ms;
        if (limit == 0U) limit = 1U;
        if (raw_pressed) {
            state->charging = 1;
            state->elapsed_ms = 0U;
            out->charge_begin = 1;
            out->trigger_down = 1;
            out->trigger_pressed = 1;
        }
        if (raw_down && state->charging) {
            state->elapsed_ms = gtrigger89_add_clamped(state->elapsed_ms,
                                                       dt_ms, limit);
            out->trigger_down = 1;
        }
        if (raw_released && state->charging) {
            out->charge_release = 1;
            out->charge_elapsed_ms = state->elapsed_ms;
            out->charge_ratio_q16 =
                (long)(((unsigned long)state->elapsed_ms *
                        (unsigned long)GTRIGGER89_Q16_ONE) /
                       (unsigned long)limit);
            if (out->charge_ratio_q16 > GTRIGGER89_Q16_ONE)
                out->charge_ratio_q16 = GTRIGGER89_Q16_ONE;
            /* No synthetic release fire here: morethanone89 gates it. */
            state->charging = 0;
            state->elapsed_ms = 0U;
        }
    }
    state->previous_raw_down = raw_down;
}
