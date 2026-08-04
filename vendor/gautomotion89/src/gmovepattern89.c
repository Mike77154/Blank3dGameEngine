#include "gmovepattern89.h"

static GMoveVec3_89 gmovepattern89_default_side(
    GMoveVec3_89 direction
)
{
    GMoveVec3_89 world_up;
    GMoveVec3_89 side;

    world_up = gmove89_vec3(0L, GMOVE89_FX_ONE, 0L);
    side = gmove89_vec3_cross(world_up, direction);
    side = gmove89_vec3_normalize_approx(side);

    if (gmove89_vec3_is_near_zero(side, 64L)) {
        side = gmove89_vec3(GMOVE89_FX_ONE, 0L, 0L);
    }

    return side;
}

static GMoveVec3_89 gmovepattern89_axis_or_default(
    GMoveVec3_89 axis,
    GMoveVec3_89 fallback
)
{
    axis = gmove89_vec3_normalize_approx(axis);
    if (gmove89_vec3_is_near_zero(axis, 64L)) {
        return gmove89_vec3_normalize_approx(fallback);
    }
    return axis;
}

static void gmovepattern89_finish_motion(
    GMoveVec3_89 current_position,
    GMoveVec3_89 desired_position,
    int completed,
    GMoveMotion89 *out_motion
)
{
    GMoveVec3_89 delta;

    delta = gmove89_vec3_sub(desired_position, current_position);

    out_motion->target_position = desired_position;
    out_motion->delta = delta;
    out_motion->desired_direction = gmove89_vec3_normalize_approx(delta);
    out_motion->distance_remaining = gmove89_vec3_length_approx(delta);
    out_motion->has_translation = GMOVE89_TRUE;
    out_motion->reached = completed;
    out_motion->valid = GMOVE89_TRUE;
}

void gmovepattern89_config_default(GMovePatternConfig89 *config)
{
    if (config == 0) {
        return;
    }

    config->type = GMOVEPATTERN89_ZIGZAG_TO;
    config->speed = GMOVE89_FX_FROM_INT(1);
    config->amplitude = GMOVE89_FX_FROM_INT(1);
    config->radius = GMOVE89_FX_FROM_INT(1);
    config->frequency = GMOVE89_FX_ONE;
    config->stop_distance = 0L;
    config->axis_a = gmove89_vec3_zero();
    config->axis_b = gmove89_vec3_zero();
}

void gmovepattern89_begin(
    GMovePatternState89 *state,
    GMoveVec3_89 origin
)
{
    if (state == 0) {
        return;
    }

    state->origin = origin;
    state->phase = 0L;
    state->travel = 0L;
    state->elapsed = 0L;
    state->active = GMOVE89_TRUE;
    state->completed = GMOVE89_FALSE;
}

void gmovepattern89_reset(GMovePatternState89 *state)
{
    if (state == 0) {
        return;
    }

    state->origin = gmove89_vec3_zero();
    state->phase = 0L;
    state->travel = 0L;
    state->elapsed = 0L;
    state->active = GMOVE89_FALSE;
    state->completed = GMOVE89_FALSE;
}

void gmovepattern89_step(
    GMovePatternState89 *state,
    const GMovePatternConfig89 *config,
    GMoveVec3_89 current_position,
    GMoveVec3_89 target_position,
    GMoveFx89 delta_time,
    GMoveMotion89 *out_motion
)
{
    GMoveVec3_89 route;
    GMoveVec3_89 direction;
    GMoveVec3_89 side;
    GMoveVec3_89 up_axis;
    GMoveVec3_89 base;
    GMoveVec3_89 desired;
    GMoveVec3_89 offset_a;
    GMoveVec3_89 offset_b;
    GMoveFx89 route_length;
    GMoveFx89 max_travel;
    GMoveFx89 wave;
    GMoveFx89 wave_b;
    GMoveFx89 advance;

    gmove89_motion_clear(out_motion);
    if (state == 0 || config == 0 || out_motion == 0) {
        return;
    }

    if (!state->active) {
        gmovepattern89_begin(state, current_position);
    }

    state->elapsed = gmove89_fx_add(state->elapsed, delta_time);
    state->phase = gmove89_wrap_turn(
        gmove89_fx_add(
            state->phase,
            gmove89_fx_mul(config->frequency, delta_time)
        )
    );

    if (config->type == GMOVEPATTERN89_PINGPONG) {
        side = gmovepattern89_axis_or_default(
            config->axis_a,
            gmove89_vec3(GMOVE89_FX_ONE, 0L, 0L)
        );
        wave = gmove89_sin_turn(state->phase);
        desired = gmove89_vec3_add(
            state->origin,
            gmove89_vec3_scale(
                side,
                gmove89_fx_mul(config->amplitude, wave)
            )
        );
        gmovepattern89_finish_motion(
            current_position,
            desired,
            GMOVE89_FALSE,
            out_motion
        );
        return;
    }

    if (config->type == GMOVEPATTERN89_ORBIT) {
        side = gmovepattern89_axis_or_default(
            config->axis_a,
            gmove89_vec3(GMOVE89_FX_ONE, 0L, 0L)
        );
        up_axis = gmovepattern89_axis_or_default(
            config->axis_b,
            gmove89_vec3(0L, 0L, GMOVE89_FX_ONE)
        );
        wave = gmove89_cos_turn(state->phase);
        wave_b = gmove89_sin_turn(state->phase);

        offset_a = gmove89_vec3_scale(
            side,
            gmove89_fx_mul(config->radius, wave)
        );
        offset_b = gmove89_vec3_scale(
            up_axis,
            gmove89_fx_mul(config->radius, wave_b)
        );
        desired = gmove89_vec3_add(
            target_position,
            gmove89_vec3_add(offset_a, offset_b)
        );
        gmovepattern89_finish_motion(
            current_position,
            desired,
            GMOVE89_FALSE,
            out_motion
        );
        return;
    }

    route = gmove89_vec3_sub(target_position, state->origin);
    route_length = gmove89_vec3_length_approx(route);
    max_travel = gmove89_fx_sub(route_length, config->stop_distance);

    if (max_travel <= 0L) {
        state->completed = GMOVE89_TRUE;
        gmovepattern89_finish_motion(
            current_position,
            state->origin,
            GMOVE89_TRUE,
            out_motion
        );
        return;
    }

    direction = gmove89_vec3_normalize_approx(route);
    advance = gmove89_fx_mul(config->speed, delta_time);
    if (advance < 0L) {
        advance = -advance;
    }

    state->travel = gmove89_fx_add(state->travel, advance);
    if (state->travel >= max_travel) {
        state->travel = max_travel;
        state->completed = GMOVE89_TRUE;
    }

    base = gmove89_vec3_add(
        state->origin,
        gmove89_vec3_scale(direction, state->travel)
    );

    side = gmovepattern89_default_side(direction);
    side = gmovepattern89_axis_or_default(config->axis_a, side);
    up_axis = gmovepattern89_axis_or_default(
        config->axis_b,
        gmove89_vec3_cross(direction, side)
    );

    if (state->completed) {
        desired = base;
    } else if (config->type == GMOVEPATTERN89_ZIGZAG_TO) {
        wave = gmove89_triangle_turn(state->phase);
        desired = gmove89_vec3_add(
            base,
            gmove89_vec3_scale(
                side,
                gmove89_fx_mul(config->amplitude, wave)
            )
        );
    } else if (config->type == GMOVEPATTERN89_HELIX_TO) {
        wave = gmove89_cos_turn(state->phase);
        wave_b = gmove89_sin_turn(state->phase);
        offset_a = gmove89_vec3_scale(
            side,
            gmove89_fx_mul(config->radius, wave)
        );
        offset_b = gmove89_vec3_scale(
            up_axis,
            gmove89_fx_mul(config->radius, wave_b)
        );
        desired = gmove89_vec3_add(
            base,
            gmove89_vec3_add(offset_a, offset_b)
        );
    } else {
        wave = gmove89_sin_turn(state->phase);
        desired = gmove89_vec3_add(
            base,
            gmove89_vec3_scale(
                side,
                gmove89_fx_mul(config->amplitude, wave)
            )
        );
    }

    gmovepattern89_finish_motion(
        current_position,
        desired,
        state->completed,
        out_motion
    );
}
