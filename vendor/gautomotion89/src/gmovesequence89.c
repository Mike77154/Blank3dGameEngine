#include "gmovesequence89.h"

static void gmovesequence89_advance(GMoveSequenceState89 *state)
{
    state->index += 1UL;
    state->step_elapsed = 0L;
    state->step_started = GMOVE89_FALSE;
    gmovepattern89_reset(&state->pattern_state);
}

void gmovesequence89_reset(GMoveSequenceState89 *state)
{
    if (state == 0) {
        return;
    }

    state->index = 0UL;
    state->step_elapsed = 0L;
    state->step_started = GMOVE89_FALSE;
    state->done = GMOVE89_FALSE;
    gmovepattern89_reset(&state->pattern_state);
}

int gmovesequence89_update(
    GMoveSequenceState89 *state,
    const GMoveSequenceStep89 *steps,
    unsigned long step_count,
    const GMoveProvider89 *provider,
    GMoveVec3_89 current_position,
    GMoveVec3_89 current_forward,
    GMoveFx89 delta_time,
    GMoveMotion89 *out_motion
)
{
    const GMoveSequenceStep89 *step;
    GMoveVec3_89 target_position;
    int resolved;

    gmove89_motion_clear(out_motion);
    if (state == 0 || steps == 0 || out_motion == 0) {
        return GMOVE89_FALSE;
    }

    if (state->done || state->index >= step_count) {
        state->done = GMOVE89_TRUE;
        out_motion->valid = GMOVE89_TRUE;
        out_motion->reached = GMOVE89_TRUE;
        return GMOVE89_TRUE;
    }

    step = &steps[state->index];

    if (step->op == GMOVESEQ89_END) {
        state->done = GMOVE89_TRUE;
        out_motion->valid = GMOVE89_TRUE;
        out_motion->reached = GMOVE89_TRUE;
        return GMOVE89_TRUE;
    }

    state->step_elapsed = gmove89_fx_add(
        state->step_elapsed,
        delta_time
    );

    if (step->op == GMOVESEQ89_WAIT) {
        out_motion->valid = GMOVE89_TRUE;
        if (state->step_elapsed >= step->duration) {
            gmovesequence89_advance(state);
        }
        return GMOVE89_TRUE;
    }

    resolved = gmove89_target_resolve(
        provider,
        &step->target,
        &target_position
    );
    if (!resolved) {
        return GMOVE89_FALSE;
    }

    if (step->op == GMOVESEQ89_MOVE_TO) {
        gautmove89_step_to_point(
            current_position,
            target_position,
            &step->automove,
            delta_time,
            out_motion
        );
        if (out_motion->reached) {
            gmovesequence89_advance(state);
        }
        return GMOVE89_TRUE;
    }

    if (step->op == GMOVESEQ89_MOVE_AWAY) {
        gautmove89_step_away_from_point(
            current_position,
            target_position,
            current_forward,
            &step->automove,
            step->safe_distance,
            delta_time,
            out_motion
        );
        if (out_motion->reached) {
            gmovesequence89_advance(state);
        }
        return GMOVE89_TRUE;
    }

    if (step->op == GMOVESEQ89_ROTATE_TO) {
        gautmove89_rotate_toward_point(
            current_position,
            current_forward,
            target_position,
            step->turn_fraction_per_second,
            delta_time,
            out_motion
        );
        if (out_motion->reached ||
            (step->duration > 0L &&
             state->step_elapsed >= step->duration)) {
            gmovesequence89_advance(state);
        }
        return GMOVE89_TRUE;
    }

    if (step->op == GMOVESEQ89_PATTERN) {
        if (!state->step_started) {
            gmovepattern89_begin(
                &state->pattern_state,
                current_position
            );
            state->step_started = GMOVE89_TRUE;
        }

        gmovepattern89_step(
            &state->pattern_state,
            &step->pattern,
            current_position,
            target_position,
            delta_time,
            out_motion
        );

        if (out_motion->reached ||
            (step->duration > 0L &&
             state->step_elapsed >= step->duration)) {
            gmovesequence89_advance(state);
        }
        return GMOVE89_TRUE;
    }

    return GMOVE89_FALSE;
}


int gmovesequence89_update_provider(
    GMoveSequenceState89 *state,
    const GMoveSequenceStep89 *steps,
    unsigned long step_count,
    const GMoveProvider89 *provider,
    GMoveId89 moving_entity,
    GMoveFx89 delta_time,
    GMoveMotion89 *out_motion
)
{
    GMoveVec3_89 current_position;
    GMoveVec3_89 current_forward;

    gmove89_motion_clear(out_motion);
    if (provider == 0 || provider->get_position == 0) {
        return GMOVE89_FALSE;
    }

    if (!provider->get_position(
            provider->user,
            moving_entity,
            &current_position)) {
        return GMOVE89_FALSE;
    }

    current_forward = gmove89_vec3(GMOVE89_FX_ONE, 0L, 0L);
    if (provider->get_forward != 0) {
        provider->get_forward(
            provider->user,
            moving_entity,
            &current_forward
        );
    }

    return gmovesequence89_update(
        state,
        steps,
        step_count,
        provider,
        current_position,
        current_forward,
        delta_time,
        out_motion
    );
}
