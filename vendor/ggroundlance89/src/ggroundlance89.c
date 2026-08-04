#include "ggroundlance89.h"

#include <limits.h>

static ggl_fx ggl_abs(ggl_fx v)
{
    return v < 0 ? -v : v;
}

static ggl_fx ggl_max3(ggl_fx a, ggl_fx b, ggl_fx c)
{
    ggl_fx m;
    m = a > b ? a : b;
    return m > c ? m : c;
}

static ggl_fx ggl_min3(ggl_fx a, ggl_fx b, ggl_fx c)
{
    ggl_fx m;
    m = a < b ? a : b;
    return m < c ? m : c;
}

static ggl_vec3 ggl_vec3_scale(const ggl_vec3 *v, ggl_fx s)
{
    ggl_vec3 r;
    r.x = ggl_fx_mul(v->x, s);
    r.y = ggl_fx_mul(v->y, s);
    r.z = ggl_fx_mul(v->z, s);
    return r;
}

static ggl_fx ggl_clamp_delta(ggl_fx current, ggl_fx target, ggl_fx up, ggl_fx down)
{
    ggl_fx delta;
    delta = target - current;
    if (delta > up) return current + up;
    if (delta < -down) return current - down;
    return target;
}

static ggl_fx ggl_triangle_wave(unsigned long tick, unsigned long period)
{
    unsigned long p;
    unsigned long half;
    ggl_fx phase;

    if (period < 2UL) return 0;
    p = tick % period;
    half = period / 2UL;
    if (half == 0UL) return 0;
    if (p <= half) {
        phase = (ggl_fx)((p * 65536UL) / half);
    } else {
        phase = (ggl_fx)(((period - p) * 65536UL) / half);
    }
    if (phase < 0) phase = 0;
    if (phase > GGL_FX_ONE) phase = GGL_FX_ONE;
    return phase;
}

static void ggl_zero_output(ggl_output *out)
{
    out->desired_position.x = 0;
    out->desired_position.y = 0;
    out->desired_position.z = 0;
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
    out->style = GGL_STYLE_LANCE;
    out->state = GGL_STATE_IDLE;
    out->write_position = 0;
    out->write_velocity = 0;
    out->attack_active = 0;
    out->impact_event = 0;
    out->finished = 0;
}

static unsigned long ggl_abs_u(ggl_fx value)
{
    if (value < 0)
        return (unsigned long)(-(value + 1L)) + 1UL;
    return (unsigned long)value;
}

static ggl_fx ggl_from_magnitude(unsigned long magnitude, int negative)
{
    unsigned long negative_limit;
    negative_limit = (unsigned long)LONG_MAX + 1UL;
    if (negative) {
        if (magnitude >= negative_limit)
            return (ggl_fx)(-LONG_MAX - 1L);
        return (ggl_fx)(-(long)magnitude);
    }
    if (magnitude > (unsigned long)LONG_MAX)
        return (ggl_fx)LONG_MAX;
    return (ggl_fx)magnitude;
}

static int ggl_add_magnitude(unsigned long *accumulator,
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

ggl_fx ggl_fx_mul(ggl_fx a, ggl_fx b)
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
    ua = ggl_abs_u(a);
    ub = ggl_abs_u(b);
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
            return ggl_from_magnitude(limit, negative);
        magnitude = whole_a * whole_b * 65536UL;
    }

    if (whole_a != 0UL) {
        if (frac_b > (limit - magnitude) / whole_a)
            return ggl_from_magnitude(limit, negative);
        term = whole_a * frac_b;
        if (!ggl_add_magnitude(&magnitude, term, limit))
            return ggl_from_magnitude(limit, negative);
    }
    if (whole_b != 0UL) {
        if (frac_a > (limit - magnitude) / whole_b)
            return ggl_from_magnitude(limit, negative);
        term = whole_b * frac_a;
        if (!ggl_add_magnitude(&magnitude, term, limit))
            return ggl_from_magnitude(limit, negative);
    }

    /* 16-bit by 16-bit is exactly representable in 32-bit unsigned long. */
    term = (frac_a * frac_b) / 65536UL;
    (void)ggl_add_magnitude(&magnitude, term, limit);
    return ggl_from_magnitude(magnitude, negative);
}

ggl_fx ggl_fx_div(ggl_fx a, ggl_fx b)
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
    ua = ggl_abs_u(a);
    ub = ggl_abs_u(b);
    limit = negative
          ? (unsigned long)LONG_MAX + 1UL
          : (unsigned long)LONG_MAX;

    whole = ua / ub;
    remainder = ua % ub;
    if (whole > limit / 65536UL)
        return ggl_from_magnitude(limit, negative);
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
    (void)ggl_add_magnitude(&magnitude, fraction, limit);
    return ggl_from_magnitude(magnitude, negative);
}

ggl_fx ggl_vec3_length_approx(const ggl_vec3 *v)
{
    ggl_fx ax;
    ggl_fx ay;
    ggl_fx az;
    ggl_fx hi;
    ggl_fx lo;
    ggl_fx mid;

    ax = ggl_abs(v->x);
    ay = ggl_abs(v->y);
    az = ggl_abs(v->z);
    hi = ggl_max3(ax, ay, az);
    lo = ggl_min3(ax, ay, az);
    mid = ax + ay + az - hi - lo;
    return hi + (mid * 3L) / 8L + (lo * 3L) / 16L;
}

void ggl_vec3_normalize_approx(const ggl_vec3 *v, ggl_vec3 *out)
{
    ggl_fx len;
    len = ggl_vec3_length_approx(v);
    if (len <= 0) {
        out->x = 0;
        out->y = 0;
        out->z = 0;
        return;
    }
    out->x = ggl_fx_div(v->x, len);
    out->y = ggl_fx_div(v->y, len);
    out->z = ggl_fx_div(v->z, len);
}

void ggl_default_config(ggl_config *cfg)
{
    if (cfg == 0) return;
    cfg->speed = GGL_FX_FROM_INT(8);
    cfg->acceleration = GGL_FX_FROM_INT(1);
    cfg->max_speed = GGL_FX_FROM_INT(20);
    cfg->arrival_radius = GGL_FX_FROM_INT(1);
    cfg->ground_clearance = 0;
    cfg->max_step_up = GGL_FX_FROM_INT(1);
    cfg->max_step_down = GGL_FX_FROM_INT(2);
    cfg->hop_height = GGL_FX_ONE / 4L;
    cfg->hop_period_ticks = 8UL;
    cfg->impact_impulse = GGL_FX_FROM_INT(20);
    cfg->max_charge_ticks = 120UL;
    cfg->recovery_ticks = 10UL;
    cfg->pierce_contacts = 1U;
    cfg->track_target = 0U;
    cfg->require_ground = 1U;
    cfg->snap_to_ground = 1U;
    cfg->style = GGL_STYLE_LANCE;
    cfg->contact_policy = GGL_CONTACT_STOP;
}

void ggl_init(ggl_controller *ctl, const ggl_config *cfg)
{
    ggl_config local_cfg;
    if (ctl == 0) return;
    if (cfg == 0) {
        ggl_default_config(&local_cfg);
        ctl->cfg = local_cfg;
    } else {
        ctl->cfg = *cfg;
    }
    ggl_reset(ctl);
}

void ggl_reset(ggl_controller *ctl)
{
    if (ctl == 0) return;
    ctl->state = GGL_STATE_IDLE;
    ctl->start_position.x = 0;
    ctl->start_position.y = 0;
    ctl->start_position.z = 0;
    ctl->target_snapshot.x = 0;
    ctl->target_snapshot.y = 0;
    ctl->target_snapshot.z = 0;
    ctl->last_direction.x = 0;
    ctl->last_direction.y = 0;
    ctl->last_direction.z = 0;
    ctl->current_speed = 0;
    ctl->last_ground_height = 0;
    ctl->state_ticks = 0UL;
    ctl->charge_ticks = 0UL;
    ctl->contact_count = 0U;
    ctl->has_target = 0U;
    ctl->has_ground = 0U;
}

static void ggl_begin(ggl_controller *ctl, const ggl_input *in)
{
    ctl->state = GGL_STATE_CHARGING;
    ctl->start_position = in->position;
    ctl->target_snapshot = in->target_position;
    ctl->has_target = in->target_valid;
    ctl->last_ground_height = in->ground_height;
    ctl->has_ground = in->ground_valid;
    ctl->current_speed = ctl->cfg.speed;
    ctl->state_ticks = 0UL;
    ctl->charge_ticks = 0UL;
    ctl->contact_count = 0U;
}

void ggl_step(ggl_controller *ctl, const ggl_input *in, ggl_output *out)
{
    ggl_vec3 target;
    ggl_vec3 flat_delta;
    ggl_vec3 dir;
    ggl_vec3 desired_velocity;
    ggl_fx dist;
    ggl_fx y_target;
    ggl_fx hop;
    ggl_fx wave;
    ggl_fx travel_speed;
    unsigned int limit;

    if (out == 0) return;
    ggl_zero_output(out);
    if (ctl == 0 || in == 0) return;

    if (ctl->state == GGL_STATE_IDLE) {
        if (in->trigger && in->target_valid &&
            (!ctl->cfg.require_ground || in->ground_valid)) {
            ggl_begin(ctl, in);
        }
    }

    if (in->cancel && ctl->state == GGL_STATE_CHARGING) {
        ctl->state = GGL_STATE_CANCELLED;
        ctl->state_ticks = 0UL;
    }

    if (ctl->state == GGL_STATE_CHARGING) {
        if (ctl->cfg.track_target && in->target_valid) {
            target = in->target_position;
            ctl->target_snapshot = target;
            ctl->has_target = 1U;
        } else {
            target = ctl->target_snapshot;
        }

        if (!ctl->has_target ||
            (ctl->cfg.require_ground && !in->ground_valid) ||
            ctl->charge_ticks >= ctl->cfg.max_charge_ticks) {
            ctl->state = GGL_STATE_RECOVERY;
            ctl->state_ticks = 0UL;
        } else {
            flat_delta.x = target.x - in->position.x;
            flat_delta.y = 0;
            flat_delta.z = target.z - in->position.z;
            dist = ggl_vec3_length_approx(&flat_delta);

            if (dist <= ctl->cfg.arrival_radius) {
                ctl->state = GGL_STATE_RECOVERY;
                ctl->state_ticks = 0UL;
            } else {
                ggl_vec3_normalize_approx(&flat_delta, &dir);
                ctl->last_direction = dir;
                ctl->current_speed += ctl->cfg.acceleration;
                if (ctl->current_speed > ctl->cfg.max_speed) {
                    ctl->current_speed = ctl->cfg.max_speed;
                }
                travel_speed = ctl->current_speed;
                if (travel_speed > dist) travel_speed = dist;
                desired_velocity = ggl_vec3_scale(&dir, travel_speed);

                out->desired_position = in->position;
                out->desired_position.x += desired_velocity.x;
                out->desired_position.z += desired_velocity.z;

                if (in->ground_valid) {
                    y_target = in->ground_height + ctl->cfg.ground_clearance;
                    if (ctl->has_ground) {
                        y_target = ggl_clamp_delta(ctl->last_ground_height + ctl->cfg.ground_clearance,
                                                   y_target,
                                                   ctl->cfg.max_step_up,
                                                   ctl->cfg.max_step_down);
                    }
                    ctl->last_ground_height = y_target - ctl->cfg.ground_clearance;
                    ctl->has_ground = 1U;
                } else {
                    y_target = in->position.y;
                }

                hop = 0;
                if (ctl->cfg.style == GGL_STYLE_PEGASUS ||
                    ctl->cfg.style == GGL_STYLE_SKIM_HOP) {
                    wave = ggl_triangle_wave(ctl->charge_ticks, ctl->cfg.hop_period_ticks);
                    hop = ggl_fx_mul(ctl->cfg.hop_height, wave);
                }

                out->desired_position.y = y_target + hop;
                desired_velocity.y = out->desired_position.y - in->position.y;
                out->desired_velocity = desired_velocity;
                out->facing_direction = dir;
                out->write_position = ctl->cfg.snap_to_ground ? 1U : 0U;
                out->write_velocity = 1U;
                out->attack_active = 1U;
                out->style = ctl->cfg.style;

                if (in->contact) {
                    out->impact_event = 1U;
                    out->impact_direction = dir;
                    out->impact_impulse = ctl->cfg.impact_impulse;
                    ctl->contact_count++;

                    if (ctl->cfg.contact_policy == GGL_CONTACT_STOP) {
                        ctl->state = GGL_STATE_RECOVERY;
                        ctl->state_ticks = 0UL;
                    } else if (ctl->cfg.contact_policy == GGL_CONTACT_BOUNCE) {
                        out->desired_velocity.x = -out->desired_velocity.x / 2L;
                        out->desired_velocity.z = -out->desired_velocity.z / 2L;
                        ctl->state = GGL_STATE_RECOVERY;
                        ctl->state_ticks = 0UL;
                    } else {
                        limit = ctl->cfg.pierce_contacts;
                        if (limit == 0U || ctl->contact_count >= limit) {
                            ctl->state = GGL_STATE_RECOVERY;
                            ctl->state_ticks = 0UL;
                        }
                    }
                }

                if (ctl->state == GGL_STATE_CHARGING) {
                    ctl->charge_ticks++;
                    ctl->state_ticks++;
                }
            }
        }
    }

    if (ctl->state == GGL_STATE_RECOVERY) {
        if (ctl->state_ticks >= ctl->cfg.recovery_ticks) {
            ctl->state = GGL_STATE_DONE;
            ctl->state_ticks = 0UL;
        } else {
            ctl->state_ticks++;
        }
    }

    if (ctl->state == GGL_STATE_DONE || ctl->state == GGL_STATE_CANCELLED) {
        out->finished = 1U;
    }

    out->state = ctl->state;
    out->style = ctl->cfg.style;
}

int ggl_step_provider(ggl_controller *ctl, const ggl_provider *provider, int trigger, int cancel, ggl_output *out)
{
    ggl_input in;
    ggl_vec3 normal;
    ggl_vec3 next_normal;
    ggl_fx height;
    ggl_fx next_height;
    ggl_fx hop_offset;
    int ok;

    if (ctl == 0 || provider == 0 || out == 0) return 0;
    if (provider->read_motion == 0 || provider->read_target == 0 || provider->query_ground == 0) return 0;

    ok = provider->read_motion(provider->user, &in.position, &in.velocity);
    if (!ok) return 0;
    in.target_valid = provider->read_target(provider->user, &in.target_position) ? 1U : 0U;
    in.ground_valid = provider->query_ground(provider->user, in.position.x, in.position.z, &height, &normal) ? 1U : 0U;
    in.ground_height = height;
    in.ground_normal = normal;
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

    ggl_step(ctl, &in, out);

    if (out->write_position &&
        provider->query_ground(provider->user,
                               out->desired_position.x,
                               out->desired_position.z,
                               &next_height,
                               &next_normal)) {
        hop_offset = out->desired_position.y - (in.ground_height + ctl->cfg.ground_clearance);
        if (hop_offset < 0) hop_offset = 0;
        out->desired_position.y = next_height + ctl->cfg.ground_clearance + hop_offset;
        out->desired_velocity.y = out->desired_position.y - in.position.y;
        ctl->last_ground_height = next_height;
        ctl->has_ground = 1U;
    }

    if ((out->write_position || out->write_velocity) && provider->write_motion != 0) {
        provider->write_motion(provider->user,
                               &out->desired_position,
                               &out->desired_velocity,
                               &out->facing_direction);
    }
    if (out->impact_event && provider->on_impact != 0) {
        provider->on_impact(provider->user, out->style, &out->impact_direction, out->impact_impulse);
    }
    return 1;
}
