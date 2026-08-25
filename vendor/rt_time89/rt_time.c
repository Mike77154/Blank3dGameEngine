/* rt_time.c - protocol89 monotonic/simulation time core. */
#include "rt_time.h"
#include <string.h>

static rt_u32 rt_sat_add_u32(rt_u32 a, rt_u32 b)
{
    if (a > UINT_MAX - b) return UINT_MAX;
    return a + b;
}

static rt_u32 rt_mul_q16_u32(rt_u32 value, rt_fixed scale_q16)
{
    rt_u32 whole;
    rt_u32 frac;
    rt_u32 a;
    rt_u32 b;
    rt_u32 lo;
    rt_u32 hi;

    if (scale_q16 <= 0 || value == 0U) return 0U;
    whole = ((rt_u32)scale_q16) >> 16;
    frac = ((rt_u32)scale_q16) & 65535U;

    if (whole != 0U && value > UINT_MAX / whole) a = UINT_MAX;
    else a = value * whole;

    hi = (value >> 16) * frac;
    lo = ((value & 65535U) * frac + 32768U) >> 16;
    if (hi > UINT_MAX - lo) b = UINT_MAX;
    else b = hi + lo;

    return rt_sat_add_u32(a, b);
}

/* floor(a*b/c), saturating, without a 64-bit intermediate. */
static rt_u32 rt_muldiv_u32(rt_u32 a, rt_u32 b, rt_u32 c)
{
    rt_u32 q;
    rt_u32 r;
    rt_u32 bit;
    rt_u32 add_q;
    rt_u32 add_r;

    if (c == 0U || a == 0U || b == 0U) return 0U;

    q = 0U;
    r = 0U;
    bit = 1U;
    add_q = a / c;
    add_r = a % c;

    while (bit != 0U && bit <= b) {
        if ((b & bit) != 0U) {
            if (add_q != 0U && q > UINT_MAX - add_q) return UINT_MAX;
            q += add_q;
            if (r > UINT_MAX - add_r) return UINT_MAX;
            r += add_r;
            if (r >= c) {
                r -= c;
                if (q == UINT_MAX) return UINT_MAX;
                q += 1U;
            }
        }

        if (bit > UINT_MAX / 2U) break;
        bit <<= 1;

        if (add_q > UINT_MAX / 2U) add_q = UINT_MAX;
        else add_q <<= 1;

        if (add_r >= c - add_r) {
            add_r = add_r - (c - add_r);
            if (add_q == UINT_MAX) return UINT_MAX;
            add_q += 1U;
        } else {
            add_r += add_r;
        }
    }
    return q;
}

static int rt_time_validate_source(const rt_time_source *src)
{
    if (!src) return 0;
    if (!src->get_ticks) return 0;
    if (src->ticks_per_second == 0U) return 0;
    return 1;
}

static rt_fixed rt_time_sanitize_scale(rt_fixed value)
{
    if (value < 0) return 0;
    return value;
}

static void rt_time_prepare_context(rt_time_context *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->scale_q16 = RT_TIME_FP_ONE;
    ctx->max_step_ms = RT_TIME_DEFAULT_MAX_STEP_MS;
    ctx->clamp_enabled = 1;
    ctx->last_status = RT_TIME_ERR_NOT_INITIALIZED;
}

void rt_time_config_default(rt_time_config *cfg)
{
    if (!cfg) return;
    cfg->scale_q16 = RT_TIME_FP_ONE;
    cfg->max_step_ms = RT_TIME_DEFAULT_MAX_STEP_MS;
    cfg->paused = 0;
    cfg->clamp_enabled = 1;
}

rt_time_status rt_time_init_ex(rt_time_context *ctx,
                               const rt_time_source *src,
                               const rt_time_config *cfg)
{
    rt_time_config local_cfg;
    rt_ticks t;

    if (!ctx) return RT_TIME_ERR_NULL_CONTEXT;
    rt_time_prepare_context(ctx);

    if (!src) {
        ctx->last_status = RT_TIME_ERR_NULL_SOURCE;
        return ctx->last_status;
    }
    if (!src->get_ticks) {
        ctx->last_status = RT_TIME_ERR_NULL_CALLBACK;
        return ctx->last_status;
    }
    if (src->ticks_per_second == 0U) {
        ctx->last_status = RT_TIME_ERR_BAD_FREQUENCY;
        return ctx->last_status;
    }

    if (cfg) local_cfg = *cfg;
    else rt_time_config_default(&local_cfg);

    ctx->src = *src;
    ctx->scale_q16 = rt_time_sanitize_scale(local_cfg.scale_q16);
    ctx->paused = local_cfg.paused != 0;
    ctx->max_step_ms = local_cfg.max_step_ms;
    ctx->clamp_enabled =
        (local_cfg.clamp_enabled != 0) && (local_cfg.max_step_ms != 0U);

    t = ctx->src.get_ticks(ctx->src.user);
    ctx->start_ticks = t;
    ctx->last_ticks = t;
    ctx->initialized = 1;
    ctx->last_status = RT_TIME_OK;
    return RT_TIME_OK;
}

void rt_time_init(rt_time_context *ctx, const rt_time_source *src)
{
    (void)rt_time_init_ex(ctx, src, (const rt_time_config *)0);
}

void rt_time_reset(rt_time_context *ctx)
{
    rt_ticks t;
    if (!ctx) return;
    if (!rt_time_validate_source(&ctx->src)) {
        ctx->raw_delta_ms = 0U;
        ctx->delta_ms = 0U;
        ctx->initialized = 0;
        ctx->last_status = RT_TIME_ERR_NOT_INITIALIZED;
        return;
    }
    t = ctx->src.get_ticks(ctx->src.user);
    ctx->start_ticks = t;
    ctx->last_ticks = t;
    ctx->raw_now_ms = 0U;
    ctx->raw_delta_ms = 0U;
    ctx->now_ms = 0U;
    ctx->delta_ms = 0U;
    ctx->frame_count = 0U;
    ctx->clock_backsteps = 0U;
    ctx->initialized = 1;
    ctx->last_status = RT_TIME_OK;
}

rt_u32 rt_time_ticks_to_ms(const rt_time_context *ctx, rt_ticks ticks)
{
    if (!ctx || ctx->src.ticks_per_second == 0U) return 0U;
    return rt_muldiv_u32(ticks, 1000U, ctx->src.ticks_per_second);
}

rt_ticks rt_time_ms_to_ticks(const rt_time_context *ctx, rt_u32 ms)
{
    if (!ctx || ctx->src.ticks_per_second == 0U) return 0U;
    return rt_muldiv_u32(ms, ctx->src.ticks_per_second, 1000U);
}

void rt_time_update(rt_time_context *ctx)
{
    rt_ticks now_ticks;
    rt_u32 diff_ticks;
    rt_u32 raw_dt;
    rt_u32 dt;

    if (!ctx) return;
    if (!ctx->initialized || !rt_time_validate_source(&ctx->src)) {
        ctx->raw_delta_ms = 0U;
        ctx->delta_ms = 0U;
        ctx->initialized = 0;
        ctx->last_status = RT_TIME_ERR_NOT_INITIALIZED;
        return;
    }

    now_ticks = ctx->src.get_ticks(ctx->src.user);
    diff_ticks = now_ticks - ctx->last_ticks;
    ctx->last_ticks = now_ticks;
    ctx->frame_count = rt_sat_add_u32(ctx->frame_count, 1U);

    if (diff_ticks > 2147483647U) {
        raw_dt = 0U;
        ctx->clock_backsteps =
            rt_sat_add_u32(ctx->clock_backsteps, 1U);
    } else {
        raw_dt = rt_time_ticks_to_ms(ctx, diff_ticks);
    }

    ctx->raw_delta_ms = raw_dt;
    ctx->raw_now_ms = rt_sat_add_u32(ctx->raw_now_ms, raw_dt);

    if (ctx->paused) {
        ctx->delta_ms = 0U;
        ctx->last_status = RT_TIME_OK;
        return;
    }

    dt = raw_dt;
    if (ctx->clamp_enabled && ctx->max_step_ms != 0U &&
        dt > ctx->max_step_ms)
        dt = ctx->max_step_ms;

    dt = rt_mul_q16_u32(dt, ctx->scale_q16);
    ctx->delta_ms = dt;
    ctx->now_ms = rt_sat_add_u32(ctx->now_ms, dt);
    ctx->last_status = RT_TIME_OK;
}

void rt_time_set_paused(rt_time_context *ctx, int paused)
{
    if (!ctx) return;
    ctx->paused = paused != 0;
    ctx->last_status = RT_TIME_OK;
}

void rt_time_toggle_paused(rt_time_context *ctx)
{
    if (!ctx) return;
    ctx->paused = !ctx->paused;
    ctx->last_status = RT_TIME_OK;
}

void rt_time_set_scale(rt_time_context *ctx, rt_fixed scale_q16)
{
    if (!ctx) return;
    ctx->scale_q16 = rt_time_sanitize_scale(scale_q16);
    ctx->last_status = RT_TIME_OK;
}

void rt_time_set_max_step_ms(rt_time_context *ctx, rt_u32 max_step_ms)
{
    if (!ctx) return;
    ctx->max_step_ms = max_step_ms;
    ctx->clamp_enabled = max_step_ms != 0U;
    ctx->last_status = RT_TIME_OK;
}

void rt_time_set_clamp_enabled(rt_time_context *ctx, int enabled)
{
    if (!ctx) return;
    ctx->clamp_enabled = (enabled != 0) && (ctx->max_step_ms != 0U);
    ctx->last_status = RT_TIME_OK;
}

rt_u32 rt_time_raw_now_ms(const rt_time_context *ctx)
{
    return ctx ? ctx->raw_now_ms : 0U;
}
rt_u32 rt_time_raw_delta_ms(const rt_time_context *ctx)
{
    return ctx ? ctx->raw_delta_ms : 0U;
}
rt_u32 rt_time_now_ms(const rt_time_context *ctx)
{
    return ctx ? ctx->now_ms : 0U;
}
rt_u32 rt_time_delta_ms(const rt_time_context *ctx)
{
    return ctx ? ctx->delta_ms : 0U;
}

rt_fixed rt_time_ms_to_q16_seconds(rt_u32 ms)
{
    rt_u32 whole;
    rt_u32 rem;
    rt_u32 q;
    whole = ms / 1000U;
    rem = ms % 1000U;
    if (whole >= 32767U) return INT_MAX;
    q = whole * 65536U;
    q += (rem * 65536U) / 1000U;
    if (q > (rt_u32)INT_MAX) return INT_MAX;
    return (rt_fixed)q;
}

rt_u32 rt_time_q16_seconds_to_ms(rt_fixed seconds_q16)
{
    rt_u32 value;
    rt_u32 whole;
    rt_u32 frac;
    rt_u32 ms;
    if (seconds_q16 <= 0) return 0U;
    value = (rt_u32)seconds_q16;
    whole = value >> 16;
    frac = value & 65535U;
    if (whole > UINT_MAX / 1000U) return UINT_MAX;
    ms = whole * 1000U;
    return rt_sat_add_u32(ms, (frac * 1000U + 32768U) >> 16);
}

rt_fixed rt_time_raw_now_q16(const rt_time_context *ctx)
{
    return rt_time_ms_to_q16_seconds(rt_time_raw_now_ms(ctx));
}
rt_fixed rt_time_raw_delta_q16(const rt_time_context *ctx)
{
    return rt_time_ms_to_q16_seconds(rt_time_raw_delta_ms(ctx));
}
rt_fixed rt_time_now_q16(const rt_time_context *ctx)
{
    return rt_time_ms_to_q16_seconds(rt_time_now_ms(ctx));
}
rt_fixed rt_time_delta_q16(const rt_time_context *ctx)
{
    return rt_time_ms_to_q16_seconds(rt_time_delta_ms(ctx));
}

rt_u32 rt_time_frame_count(const rt_time_context *ctx)
{
    return ctx ? ctx->frame_count : 0U;
}
rt_u32 rt_time_clock_backsteps(const rt_time_context *ctx)
{
    return ctx ? ctx->clock_backsteps : 0U;
}
int rt_time_is_paused(const rt_time_context *ctx)
{
    return ctx ? (ctx->paused != 0) : 0;
}
int rt_time_is_valid(const rt_time_context *ctx)
{
    return ctx ? (ctx->initialized != 0) : 0;
}
rt_time_status rt_time_last_status(const rt_time_context *ctx)
{
    return ctx ? ctx->last_status : RT_TIME_ERR_NULL_CONTEXT;
}

const char *rt_time_status_string(rt_time_status status)
{
    switch (status) {
        case RT_TIME_OK: return "RT_TIME_OK";
        case RT_TIME_ERR_NULL_CONTEXT: return "RT_TIME_ERR_NULL_CONTEXT";
        case RT_TIME_ERR_NULL_SOURCE: return "RT_TIME_ERR_NULL_SOURCE";
        case RT_TIME_ERR_NULL_CALLBACK: return "RT_TIME_ERR_NULL_CALLBACK";
        case RT_TIME_ERR_BAD_FREQUENCY: return "RT_TIME_ERR_BAD_FREQUENCY";
        case RT_TIME_ERR_NOT_INITIALIZED: return "RT_TIME_ERR_NOT_INITIALIZED";
        default: return "RT_TIME_ERR_UNKNOWN";
    }
}

void rt_timer_init(rt_timer *t)
{
    if (!t) return;
    t->remaining_ms = 0U;
    t->period_ms = 0U;
    t->active = 0;
    t->looping = 0;
    t->pending = 0;
}

void rt_timer_start_ms(rt_timer *t, rt_u32 ms, int looping)
{
    if (!t) return;
    rt_timer_init(t);
    if (ms == 0U) {
        t->active = 1;
        t->pending = 1;
        return;
    }
    t->remaining_ms = ms;
    t->period_ms = ms;
    t->active = 1;
    t->looping = looping != 0;
}

void rt_timer_start(rt_timer *t, const rt_time_context *ctx,
                    rt_fixed seconds_q16, int looping)
{
    (void)ctx;
    rt_timer_start_ms(t, rt_time_q16_seconds_to_ms(seconds_q16), looping);
}

void rt_timer_stop(rt_timer *t)
{
    if (!t) return;
    rt_timer_init(t);
}

int rt_timer_update_count(rt_timer *t, const rt_time_context *ctx)
{
    rt_u32 dt;
    rt_u32 overshoot;
    rt_u32 fires;

    if (!t || !ctx || !t->active) return 0;
    if (t->pending) {
        t->pending = 0;
        t->active = 0;
        return 1;
    }
    dt = ctx->delta_ms;
    if (dt == 0U) return 0;
    if (dt < t->remaining_ms) {
        t->remaining_ms -= dt;
        return 0;
    }
    if (!t->looping || t->period_ms == 0U) {
        t->remaining_ms = 0U;
        t->active = 0;
        return 1;
    }

    overshoot = dt - t->remaining_ms;
    fires = 1U + overshoot / t->period_ms;
    t->remaining_ms = t->period_ms - (overshoot % t->period_ms);
    if (t->remaining_ms == 0U) t->remaining_ms = t->period_ms;
    if (fires > (rt_u32)INT_MAX) return INT_MAX;
    return (int)fires;
}

int rt_timer_update(rt_timer *t, const rt_time_context *ctx)
{
    return rt_timer_update_count(t, ctx) > 0;
}

rt_u32 rt_timer_remaining_ms(const rt_timer *t)
{
    return t ? t->remaining_ms : 0U;
}
rt_fixed rt_timer_remaining(const rt_timer *t)
{
    return rt_time_ms_to_q16_seconds(rt_timer_remaining_ms(t));
}
int rt_timer_is_active(const rt_timer *t)
{
    return t ? (t->active != 0) : 0;
}

void rt_fixed_step_init_ms(rt_fixed_step *fs, rt_u32 step_ms,
                           int max_steps_per_update)
{
    if (!fs) return;
    fs->step_ms = step_ms;
    fs->accumulator_ms = 0U;
    fs->alpha_q16 = 0;
    fs->max_steps_per_update =
        max_steps_per_update > 0 ? max_steps_per_update : 0;
    fs->total_steps = 0U;
    fs->dropped_updates = 0U;
}

void rt_fixed_step_init(rt_fixed_step *fs, rt_fixed step_seconds_q16,
                        int max_steps_per_update)
{
    rt_fixed_step_init_ms(fs, rt_time_q16_seconds_to_ms(step_seconds_q16),
                          max_steps_per_update);
}

void rt_fixed_step_reset(rt_fixed_step *fs)
{
    if (!fs) return;
    fs->accumulator_ms = 0U;
    fs->alpha_q16 = 0;
    fs->total_steps = 0U;
    fs->dropped_updates = 0U;
}

int rt_fixed_step_update(rt_fixed_step *fs, const rt_time_context *ctx)
{
    rt_u32 max_acc;
    rt_u32 steps_u;
    int steps;

    if (!fs || !ctx || fs->step_ms == 0U) return 0;
    fs->accumulator_ms =
        rt_sat_add_u32(fs->accumulator_ms, ctx->delta_ms);

    if (fs->max_steps_per_update > 0) {
        rt_u32 max_steps;
        max_steps = (rt_u32)fs->max_steps_per_update;
        if (fs->step_ms != 0U && max_steps > UINT_MAX / fs->step_ms)
            max_acc = UINT_MAX;
        else
            max_acc = fs->step_ms * max_steps;
        if (fs->accumulator_ms > max_acc) {
            fs->accumulator_ms = max_acc;
            fs->dropped_updates =
                rt_sat_add_u32(fs->dropped_updates, 1U);
        }
    }

    steps_u = fs->accumulator_ms / fs->step_ms;
    if (steps_u > (rt_u32)INT_MAX) steps = INT_MAX;
    else steps = (int)steps_u;
    fs->accumulator_ms -= (rt_u32)steps * fs->step_ms;
    fs->alpha_q16 = (rt_fixed)
        rt_muldiv_u32(fs->accumulator_ms, 65536U, fs->step_ms);
    fs->total_steps = rt_sat_add_u32(fs->total_steps, (rt_u32)steps);
    return steps;
}

rt_fixed rt_fixed_step_alpha(const rt_fixed_step *fs)
{
    return fs ? fs->alpha_q16 : 0;
}
rt_u32 rt_fixed_step_total_steps(const rt_fixed_step *fs)
{
    return fs ? fs->total_steps : 0U;
}
rt_u32 rt_fixed_step_dropped_updates(const rt_fixed_step *fs)
{
    return fs ? fs->dropped_updates : 0U;
}
