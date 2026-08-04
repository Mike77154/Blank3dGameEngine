#include "gautmove89.h"

static GMoveFx89 gautmove89_min_fx(GMoveFx89 a, GMoveFx89 b)
{
    if (a < b) {
        return a;
    }
    return b;
}

static GMoveFx89 gautmove89_effective_speed(
    GMoveFx89 distance,
    const GAutMoveConfig89 *config
)
{
    GMoveFx89 speed;
    GMoveFx89 ratio;

    speed = config->speed;
    if (speed < 0L) {
        speed = -speed;
    }

    if (config->speed_mode != GAUTMOVE89_SPEED_SLOWDOWN) {
        return speed;
    }

    if (config->slowdown_distance <= 0L) {
        return speed;
    }

    if (distance >= config->slowdown_distance) {
        return speed;
    }

    ratio = gmove89_fx_div(distance, config->slowdown_distance);
    ratio = gmove89_fx_clamp(ratio, 0L, GMOVE89_FX_ONE);
    return gmove89_fx_mul(speed, ratio);
}

void gautmove89_config_default(GAutMoveConfig89 *config)
{
    if (config == 0) {
        return;
    }

    config->speed = GMOVE89_FX_FROM_INT(1);
    config->stop_distance = 0L;
    config->slowdown_distance = GMOVE89_FX_FROM_INT(2);
    config->speed_mode = GAUTMOVE89_SPEED_CONSTANT;
}

void gautmove89_step_to_point(
    GMoveVec3_89 current_position,
    GMoveVec3_89 target_position,
    const GAutMoveConfig89 *config,
    GMoveFx89 delta_time,
    GMoveMotion89 *out_motion
)
{
    GMoveVec3_89 to_target;
    GMoveVec3_89 direction;
    GMoveFx89 distance;
    GMoveFx89 remaining;
    GMoveFx89 speed;
    GMoveFx89 step_distance;

    gmove89_motion_clear(out_motion);
    if (config == 0 || out_motion == 0 || delta_time < 0L) {
        return;
    }

    to_target = gmove89_vec3_sub(target_position, current_position);
    distance = gmove89_vec3_length_approx(to_target);
    remaining = gmove89_fx_sub(distance, config->stop_distance);

    out_motion->target_position = current_position;
    out_motion->distance_remaining = remaining;
    out_motion->valid = GMOVE89_TRUE;

    if (remaining <= 0L) {
        out_motion->reached = GMOVE89_TRUE;
        out_motion->distance_remaining = 0L;
        return;
    }

    direction = gmove89_vec3_normalize_approx(to_target);
    speed = gautmove89_effective_speed(distance, config);
    step_distance = gmove89_fx_mul(speed, delta_time);
    step_distance = gautmove89_min_fx(step_distance, remaining);

    out_motion->desired_direction = direction;
    out_motion->delta = gmove89_vec3_scale(direction, step_distance);
    out_motion->target_position = gmove89_vec3_add(
        current_position,
        out_motion->delta
    );
    out_motion->has_translation = GMOVE89_TRUE;

    if (step_distance >= remaining) {
        out_motion->reached = GMOVE89_TRUE;
        out_motion->distance_remaining = 0L;
    } else {
        out_motion->distance_remaining = gmove89_fx_sub(
            remaining,
            step_distance
        );
    }
}

void gautmove89_step_away_from_point(
    GMoveVec3_89 current_position,
    GMoveVec3_89 threat_position,
    GMoveVec3_89 fallback_direction,
    const GAutMoveConfig89 *config,
    GMoveFx89 safe_distance,
    GMoveFx89 delta_time,
    GMoveMotion89 *out_motion
)
{
    GMoveVec3_89 away;
    GMoveVec3_89 direction;
    GMoveFx89 distance;
    GMoveFx89 remaining;
    GMoveFx89 speed;
    GMoveFx89 step_distance;

    gmove89_motion_clear(out_motion);
    if (config == 0 || out_motion == 0 || delta_time < 0L) {
        return;
    }

    away = gmove89_vec3_sub(current_position, threat_position);
    distance = gmove89_vec3_length_approx(away);
    remaining = gmove89_fx_sub(safe_distance, distance);

    out_motion->target_position = current_position;
    out_motion->distance_remaining = remaining;
    out_motion->valid = GMOVE89_TRUE;

    if (remaining <= 0L) {
        out_motion->reached = GMOVE89_TRUE;
        out_motion->distance_remaining = 0L;
        return;
    }

    if (gmove89_vec3_is_near_zero(away, 16L)) {
        direction = gmove89_vec3_normalize_approx(fallback_direction);
        if (gmove89_vec3_is_near_zero(direction, 16L)) {
            direction = gmove89_vec3(GMOVE89_FX_ONE, 0L, 0L);
        }
    } else {
        direction = gmove89_vec3_normalize_approx(away);
    }

    speed = gautmove89_effective_speed(remaining, config);
    step_distance = gmove89_fx_mul(speed, delta_time);
    step_distance = gautmove89_min_fx(step_distance, remaining);

    out_motion->desired_direction = direction;
    out_motion->delta = gmove89_vec3_scale(direction, step_distance);
    out_motion->target_position = gmove89_vec3_add(
        current_position,
        out_motion->delta
    );
    out_motion->has_translation = GMOVE89_TRUE;

    if (step_distance >= remaining) {
        out_motion->reached = GMOVE89_TRUE;
        out_motion->distance_remaining = 0L;
    } else {
        out_motion->distance_remaining = gmove89_fx_sub(
            remaining,
            step_distance
        );
    }
}

void gautmove89_step_direction(
    GMoveVec3_89 current_position,
    GMoveVec3_89 direction,
    GMoveFx89 speed,
    GMoveFx89 delta_time,
    GMoveMotion89 *out_motion
)
{
    GMoveFx89 step_distance;

    gmove89_motion_clear(out_motion);
    if (out_motion == 0 || delta_time < 0L) {
        return;
    }

    direction = gmove89_vec3_normalize_approx(direction);
    if (speed < 0L) {
        speed = -speed;
        direction = gmove89_vec3_scale(direction, -GMOVE89_FX_ONE);
    }

    step_distance = gmove89_fx_mul(speed, delta_time);

    out_motion->desired_direction = direction;
    out_motion->delta = gmove89_vec3_scale(direction, step_distance);
    out_motion->target_position = gmove89_vec3_add(
        current_position,
        out_motion->delta
    );
    out_motion->has_translation = GMOVE89_TRUE;
    out_motion->valid = GMOVE89_TRUE;
}

void gautmove89_step_forward(
    GMoveVec3_89 current_position,
    GMoveVec3_89 current_forward,
    GMoveFx89 speed,
    GMoveFx89 delta_time,
    GMoveMotion89 *out_motion
)
{
    gautmove89_step_direction(
        current_position,
        current_forward,
        speed,
        delta_time,
        out_motion
    );
}

void gautmove89_rotate_toward_point(
    GMoveVec3_89 current_position,
    GMoveVec3_89 current_forward,
    GMoveVec3_89 target_position,
    GMoveFx89 turn_fraction_per_second,
    GMoveFx89 delta_time,
    GMoveMotion89 *out_motion
)
{
    GMoveVec3_89 desired;
    GMoveVec3_89 blended;
    GMoveFx89 blend;

    gmove89_motion_clear(out_motion);
    if (out_motion == 0 || delta_time < 0L) {
        return;
    }

    desired = gmove89_vec3_normalize_approx(
        gmove89_vec3_sub(target_position, current_position)
    );

    if (gmove89_vec3_is_near_zero(desired, 16L)) {
        out_motion->desired_forward = gmove89_vec3_normalize_approx(
            current_forward
        );
        out_motion->reached = GMOVE89_TRUE;
        out_motion->has_rotation = GMOVE89_TRUE;
        out_motion->valid = GMOVE89_TRUE;
        return;
    }

    if (turn_fraction_per_second <= 0L) {
        blended = desired;
    } else {
        blend = gmove89_fx_mul(turn_fraction_per_second, delta_time);
        blend = gmove89_fx_clamp(blend, 0L, GMOVE89_FX_ONE);

        blended = gmove89_vec3_add(
            gmove89_vec3_scale(
                gmove89_vec3_normalize_approx(current_forward),
                gmove89_fx_sub(GMOVE89_FX_ONE, blend)
            ),
            gmove89_vec3_scale(desired, blend)
        );
        blended = gmove89_vec3_normalize_approx(blended);
    }

    out_motion->desired_forward = blended;
    out_motion->has_rotation = GMOVE89_TRUE;
    out_motion->valid = GMOVE89_TRUE;

    if (gmove89_vec3_length_approx(
            gmove89_vec3_sub(blended, desired)
        ) <= 512L) {
        out_motion->reached = GMOVE89_TRUE;
    }
}

int gautmove89_step_to_target(
    const GMoveProvider89 *provider,
    GMoveId89 moving_entity,
    const GMoveTarget89 *target,
    const GAutMoveConfig89 *config,
    GMoveFx89 delta_time,
    GMoveMotion89 *out_motion
)
{
    GMoveVec3_89 current;
    GMoveVec3_89 destination;

    gmove89_motion_clear(out_motion);
    if (provider == 0 || provider->get_position == 0) {
        return GMOVE89_FALSE;
    }

    if (!provider->get_position(provider->user, moving_entity, &current)) {
        return GMOVE89_FALSE;
    }

    if (!gmove89_target_resolve(provider, target, &destination)) {
        return GMOVE89_FALSE;
    }

    gautmove89_step_to_point(
        current,
        destination,
        config,
        delta_time,
        out_motion
    );
    return out_motion->valid;
}

int gautmove89_step_away_from_target(
    const GMoveProvider89 *provider,
    GMoveId89 moving_entity,
    const GMoveTarget89 *target,
    const GAutMoveConfig89 *config,
    GMoveFx89 safe_distance,
    GMoveFx89 delta_time,
    GMoveMotion89 *out_motion
)
{
    GMoveVec3_89 current;
    GMoveVec3_89 threat;
    GMoveVec3_89 forward;

    gmove89_motion_clear(out_motion);
    if (provider == 0 || provider->get_position == 0) {
        return GMOVE89_FALSE;
    }

    if (!provider->get_position(provider->user, moving_entity, &current)) {
        return GMOVE89_FALSE;
    }

    if (!gmove89_target_resolve(provider, target, &threat)) {
        return GMOVE89_FALSE;
    }

    forward = gmove89_vec3(GMOVE89_FX_ONE, 0L, 0L);
    if (provider->get_forward != 0) {
        provider->get_forward(provider->user, moving_entity, &forward);
    }

    gautmove89_step_away_from_point(
        current,
        threat,
        forward,
        config,
        safe_distance,
        delta_time,
        out_motion
    );
    return out_motion->valid;
}
