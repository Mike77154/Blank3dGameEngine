#include "gairlunge89.h"

#include <limits.h>

static gal_fx gal_abs(gal_fx v)
{
    return v < 0 ? -v : v;
}

static gal_fx gal_clamp(gal_fx v, gal_fx lo, gal_fx hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static gal_fx gal_max3(gal_fx a, gal_fx b, gal_fx c)
{
    gal_fx m;
    m = a > b ? a : b;
    return m > c ? m : c;
}

static gal_fx gal_min3(gal_fx a, gal_fx b, gal_fx c)
{
    gal_fx m;
    m = a < b ? a : b;
    return m < c ? m : c;
}

static gal_vec3 gal_vec3_sub(const gal_vec3 *a, const gal_vec3 *b)
{
    gal_vec3 r;
    r.x = a->x - b->x;
    r.y = a->y - b->y;
    r.z = a->z - b->z;
    return r;
}

static gal_vec3 gal_vec3_scale(const gal_vec3 *v, gal_fx s)
{
    gal_vec3 r;
    r.x = gal_fx_mul(v->x, s);
    r.y = gal_fx_mul(v->y, s);
    r.z = gal_fx_mul(v->z, s);
    return r;
}

static gal_vec3 gal_vec3_lerp(const gal_vec3 *a, const gal_vec3 *b, gal_fx t)
{
    gal_vec3 r;
    gal_fx clamped;
    clamped = gal_clamp(t, 0, GAL_FX_ONE);
    r.x = a->x + gal_fx_mul(b->x - a->x, clamped);
    r.y = a->y + gal_fx_mul(b->y - a->y, clamped);
    r.z = a->z + gal_fx_mul(b->z - a->z, clamped);
    return r;
}

static void gal_zero_output(gal_output *out)
{
    out->desired_velocity.x = 0;
    out->desired_velocity.y = 0;
    out->desired_velocity.z = 0;
    out->facing_direction.x = 0;
    out->facing_direction.y = 0;
    out->facing_direction.z = 0;
    out->impact_direction.x = 0;
    out->impact_direction.y = 0;
    out->impact_direction.z = 0;
    out->impact_impulse = 0;
    out->intent = GAL_INTENT_RAM;
    out->state = GAL_STATE_IDLE;
    out->write_velocity = 0;
    out->attack_active = 0;
    out->impact_event = 0;
    out->finished = 0;
}

static unsigned long gal_abs_u(gal_fx value)
{
    if (value < 0)
        return (unsigned long)(-(value + 1L)) + 1UL;
    return (unsigned long)value;
}

static gal_fx gal_from_magnitude(unsigned long magnitude, int negative)
{
    unsigned long negative_limit;
    negative_limit = (unsigned long)LONG_MAX + 1UL;
    if (negative) {
        if (magnitude >= negative_limit)
            return (gal_fx)(-LONG_MAX - 1L);
        return (gal_fx)(-(long)magnitude);
    }
    if (magnitude > (unsigned long)LONG_MAX)
        return (gal_fx)LONG_MAX;
    return (gal_fx)magnitude;
}

static int gal_add_magnitude(unsigned long *accumulator,
                               unsigned long term,
                               unsigned long limit)
{
    if (!accumulator) return 0;
    if (term > limit - *accumulator) {
        *accumulator = limit;
        return 0;
    }
    *accumulator += term;
    return 1;
}

gal_fx gal_fx_mul(gal_fx a, gal_fx b)
{
    unsigned long ua;
    unsigned long ub;
    unsigned long whole_a;
    unsigned long whole_b;
    unsigned long frac_a;
    unsigned long frac_b;
    unsigned long magnitude;
    unsigned long limit;
    unsigned long term;
    int negative;

    negative = (a < 0) != (b < 0);
    ua = gal_abs_u(a);
    ub = gal_abs_u(b);
    limit = negative
          ? (unsigned long)LONG_MAX + 1UL
          : (unsigned long)LONG_MAX;

    whole_a = ua / 65536UL;
    whole_b = ub / 65536UL;
    frac_a = ua % 65536UL;
    frac_b = ub % 65536UL;
    magnitude = 0UL;

    if (whole_a != 0UL && whole_b != 0UL) {
        if (whole_b > (limit / 65536UL) / whole_a)
            return gal_from_magnitude(limit, negative);
        magnitude = whole_a * whole_b * 65536UL;
    }

    if (whole_a != 0UL) {
        if (frac_b > (limit - magnitude) / whole_a)
            return gal_from_magnitude(limit, negative);
        term = whole_a * frac_b;
        if (!gal_add_magnitude(&magnitude, term, limit))
            return gal_from_magnitude(limit, negative);
    }
    if (whole_b != 0UL) {
        if (frac_a > (limit - magnitude) / whole_b)
            return gal_from_magnitude(limit, negative);
        term = whole_b * frac_a;
        if (!gal_add_magnitude(&magnitude, term, limit))
            return gal_from_magnitude(limit, negative);
    }

    /* 16-bit by 16-bit is exactly representable in 32-bit unsigned long. */
    term = (frac_a * frac_b) / 65536UL;
    (void)gal_add_magnitude(&magnitude, term, limit);
    return gal_from_magnitude(magnitude, negative);
}

gal_fx gal_fx_div(gal_fx a, gal_fx b)
{
    unsigned long ua;
    unsigned long ub;
    unsigned long whole;
    unsigned long remainder;
    unsigned long fraction;
    unsigned long magnitude;
    unsigned long limit;
    int negative;
    int bit;

    if (b == 0) return 0;
    negative = (a < 0) != (b < 0);
    ua = gal_abs_u(a);
    ub = gal_abs_u(b);
    limit = negative
          ? (unsigned long)LONG_MAX + 1UL
          : (unsigned long)LONG_MAX;

    whole = ua / ub;
    remainder = ua % ub;
    if (whole > limit / 65536UL)
        return gal_from_magnitude(limit, negative);
    magnitude = whole * 65536UL;

    /* Build the 16 fractional bits without evaluating remainder * 65536.
       That multiplication overflows a 32-bit signed long on MinGW32. */
    fraction = 0UL;
    for (bit = 0; bit < 16; ++bit) {
        fraction <<= 1;
        if (remainder >= ub - remainder) {
            remainder = remainder - (ub - remainder);
            fraction |= 1UL;
        } else {
            remainder += remainder;
        }
    }
    (void)gal_add_magnitude(&magnitude, fraction, limit);
    return gal_from_magnitude(magnitude, negative);
}

gal_fx gal_vec3_length_approx(const gal_vec3 *v)
{
    gal_fx ax;
    gal_fx ay;
    gal_fx az;
    gal_fx hi;
    gal_fx lo;
    gal_fx mid;

    ax = gal_abs(v->x);
    ay = gal_abs(v->y);
    az = gal_abs(v->z);
    hi = gal_max3(ax, ay, az);
    lo = gal_min3(ax, ay, az);
    mid = ax + ay + az - hi - lo;

    /* Stable approximation: hi + 3/8 mid + 3/16 lo. */
    return hi + (mid * 3L) / 8L + (lo * 3L) / 16L;
}

void gal_vec3_normalize_approx(const gal_vec3 *v, gal_vec3 *out)
{
    gal_fx len;
    len = gal_vec3_length_approx(v);
    if (len <= 0) {
        out->x = 0;
        out->y = 0;
        out->z = 0;
        return;
    }
    out->x = gal_fx_div(v->x, len);
    out->y = gal_fx_div(v->y, len);
    out->z = gal_fx_div(v->z, len);
}

void gal_default_config(gal_config *cfg)
{
    if (cfg == 0) return;
    cfg->launch_speed = GAL_FX_FROM_INT(12);
    cfg->acceleration = GAL_FX_FROM_INT(1);
    cfg->max_speed = GAL_FX_FROM_INT(22);
    cfg->steering = GAL_FX_FROM_INT(1);
    cfg->arrival_radius = GAL_FX_FROM_INT(1);
    cfg->impact_impulse = GAL_FX_FROM_INT(18);
    cfg->velocity_keep = GAL_FX_ONE / 4L;
    cfg->max_active_ticks = 45UL;
    cfg->recovery_ticks = 8UL;
    cfg->pierce_contacts = 1U;
    cfg->require_airborne = 1U;
    cfg->finish_on_ground = 1U;
    cfg->cancel_on_target_loss = 1U;
    cfg->target_policy = GAL_TARGET_TRACK_EACH_STEP;
    cfg->intent = GAL_INTENT_RAM;
}

void gal_init(gal_controller *ctl, const gal_config *cfg)
{
    gal_config local_cfg;
    if (ctl == 0) return;
    if (cfg == 0) {
        gal_default_config(&local_cfg);
        ctl->cfg = local_cfg;
    } else {
        ctl->cfg = *cfg;
    }
    gal_reset(ctl);
}

void gal_reset(gal_controller *ctl)
{
    if (ctl == 0) return;
    ctl->state = GAL_STATE_IDLE;
    ctl->target_snapshot.x = 0;
    ctl->target_snapshot.y = 0;
    ctl->target_snapshot.z = 0;
    ctl->last_direction.x = 0;
    ctl->last_direction.y = 0;
    ctl->last_direction.z = 0;
    ctl->current_speed = 0;
    ctl->state_ticks = 0UL;
    ctl->contact_count = 0U;
    ctl->has_target = 0U;
}

static void gal_begin(gal_controller *ctl, const gal_input *in)
{
    ctl->state = GAL_STATE_ACTIVE;
    ctl->state_ticks = 0UL;
    ctl->contact_count = 0U;
    ctl->current_speed = ctl->cfg.launch_speed;
    ctl->target_snapshot = in->target_position;
    ctl->has_target = in->target_valid;
}

void gal_step(gal_controller *ctl, const gal_input *in, gal_output *out)
{
    gal_vec3 target;
    gal_vec3 delta;
    gal_vec3 dir;
    gal_vec3 launch_velocity;
    gal_vec3 kept_velocity;
    gal_vec3 blended;
    gal_vec3 blended_dir;
    gal_fx dist;
    gal_fx blended_len;
    gal_fx speed_next;
    gal_fx travel_speed;
    unsigned int pierce_limit;

    if (out == 0) return;
    gal_zero_output(out);
    if (ctl == 0 || in == 0) return;

    if (ctl->state == GAL_STATE_IDLE) {
        if (in->trigger && in->target_valid &&
            (!ctl->cfg.require_airborne || in->airborne)) {
            gal_begin(ctl, in);
        }
    }

    if (in->cancel && ctl->state == GAL_STATE_ACTIVE) {
        ctl->state = GAL_STATE_CANCELLED;
        ctl->state_ticks = 0UL;
    }

    if (ctl->state == GAL_STATE_ACTIVE) {
        if (ctl->cfg.target_policy == GAL_TARGET_TRACK_EACH_STEP && in->target_valid) {
            target = in->target_position;
            ctl->target_snapshot = target;
            ctl->has_target = 1U;
        } else {
            target = ctl->target_snapshot;
        }

        if (!ctl->has_target && ctl->cfg.cancel_on_target_loss) {
            ctl->state = GAL_STATE_CANCELLED;
            ctl->state_ticks = 0UL;
        } else {
            delta = gal_vec3_sub(&target, &in->position);
            dist = gal_vec3_length_approx(&delta);
            gal_vec3_normalize_approx(&delta, &dir);

            if (dist <= ctl->cfg.arrival_radius || ctl->state_ticks >= ctl->cfg.max_active_ticks) {
                ctl->state = GAL_STATE_RECOVERY;
                ctl->state_ticks = 0UL;
            } else if (ctl->cfg.finish_on_ground && !in->airborne && ctl->state_ticks > 0UL) {
                ctl->state = GAL_STATE_RECOVERY;
                ctl->state_ticks = 0UL;
            } else {
                speed_next = ctl->current_speed + ctl->cfg.acceleration;
                if (speed_next > ctl->cfg.max_speed) speed_next = ctl->cfg.max_speed;
                ctl->current_speed = speed_next;

                travel_speed = ctl->current_speed;
                if (travel_speed > dist) travel_speed = dist;
                launch_velocity = gal_vec3_scale(&dir, travel_speed);
                kept_velocity = gal_vec3_scale(&in->velocity, ctl->cfg.velocity_keep);
                blended.x = launch_velocity.x + kept_velocity.x;
                blended.y = launch_velocity.y + kept_velocity.y;
                blended.z = launch_velocity.z + kept_velocity.z;
                blended = gal_vec3_lerp(&in->velocity, &blended, ctl->cfg.steering);
                blended_len = gal_vec3_length_approx(&blended);
                if (blended_len > dist && dist > 0) {
                    gal_vec3_normalize_approx(&blended, &blended_dir);
                    blended = gal_vec3_scale(&blended_dir, dist);
                }

                ctl->last_direction = dir;
                out->desired_velocity = blended;
                out->facing_direction = dir;
                out->write_velocity = 1U;
                out->attack_active = 1U;
                out->intent = ctl->cfg.intent;

                if (in->contact) {
                    out->impact_event = 1U;
                    out->impact_direction = dir;
                    out->impact_impulse = ctl->cfg.impact_impulse;
                    ctl->contact_count++;
                    pierce_limit = ctl->cfg.pierce_contacts;
                    if (ctl->cfg.intent != GAL_INTENT_PIERCE ||
                        pierce_limit == 0U || ctl->contact_count >= pierce_limit) {
                        ctl->state = GAL_STATE_RECOVERY;
                        ctl->state_ticks = 0UL;
                    }
                }

                if (ctl->state == GAL_STATE_ACTIVE) ctl->state_ticks++;
            }
        }
    }

    if (ctl->state == GAL_STATE_RECOVERY) {
        if (ctl->state_ticks >= ctl->cfg.recovery_ticks) {
            ctl->state = GAL_STATE_DONE;
            ctl->state_ticks = 0UL;
        } else {
            ctl->state_ticks++;
        }
    }

    if (ctl->state == GAL_STATE_DONE || ctl->state == GAL_STATE_CANCELLED) {
        out->finished = 1U;
    }

    out->state = ctl->state;
    out->intent = ctl->cfg.intent;
}

int gal_step_provider(gal_controller *ctl, const gal_provider *provider, int trigger, int cancel, gal_output *out)
{
    gal_input in;
    int airborne;
    int ok;
    gal_vec3 normal;

    if (ctl == 0 || provider == 0 || out == 0) return 0;
    if (provider->read_motion == 0 || provider->read_target == 0) return 0;

    airborne = 0;
    ok = provider->read_motion(provider->user, &in.position, &in.velocity, &airborne);
    if (!ok) return 0;
    in.airborne = airborne ? 1U : 0U;
    in.target_valid = provider->read_target(provider->user, &in.target_position) ? 1U : 0U;
    in.trigger = trigger ? 1U : 0U;
    in.cancel = cancel ? 1U : 0U;
    in.contact = 0U;
    in.contact_normal.x = 0;
    in.contact_normal.y = 0;
    in.contact_normal.z = 0;

    if (provider->read_contact != 0 && provider->read_contact(provider->user, &normal)) {
        in.contact = 1U;
        in.contact_normal = normal;
    }

    gal_step(ctl, &in, out);

    if (out->write_velocity && provider->write_velocity != 0) {
        provider->write_velocity(provider->user, &out->desired_velocity, &out->facing_direction);
    }
    if (out->impact_event && provider->on_impact != 0) {
        provider->on_impact(provider->user, out->intent, &out->impact_direction, out->impact_impulse);
    }
    return 1;
}
