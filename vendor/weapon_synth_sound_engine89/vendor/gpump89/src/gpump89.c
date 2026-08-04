#include "gpump89.h"
#include "gpump89_grains.inc"

static gpump89_i32 gpump89_clamp_q15(gpump89_i32 v)
{
    if (v < 0) return 0;
    if (v > GPUMP89_Q15_ONE) return GPUMP89_Q15_ONE;
    return v;
}

static gpump89_i32 gpump89_mul_q15(gpump89_i32 a, gpump89_i32 b)
{
    return (a * b) / 32768L;
}

static gpump89_i32 gpump89_sat_i16(gpump89_i32 v)
{
    if (v > 32767L) return 32767L;
    if (v < -32768L) return -32768L;
    return v;
}

static gpump89_i32 gpump89_soft_clip(gpump89_i32 v)
{
    gpump89_i32 sign;
    gpump89_i32 a;
    sign = 1;
    if (v < 0) {
        sign = -1;
        a = -v;
    } else {
        a = v;
    }
    if (a > 24576L) {
        a = 24576L + (a - 24576L) / 4L;
    }
    if (a > 32767L) a = 32767L;
    return sign < 0 ? -a : a;
}

static gpump89_u32 gpump89_ms_to_frames(gpump89_u32 rate, gpump89_u32 ms)
{
    return (rate / 1000UL) * ms + ((rate % 1000UL) * ms) / 1000UL;
}

static gpump89_u32 gpump89_permille(gpump89_u32 total, gpump89_u32 value)
{
    return (total / 1000UL) * value
         + ((total % 1000UL) * value) / 1000UL;
}

static gpump89_u32 gpump89_random(gpump89_context *ctx)
{
    ctx->rng = ctx->rng * 1664525UL + 1013904223UL;
    return ctx->rng;
}

static gpump89_i32 gpump89_rand_signed(gpump89_context *ctx)
{
    return (gpump89_i32)((gpump89_random(ctx) >> 16) & 65535UL) - 32768L;
}

static void gpump89_clear_voices(gpump89_context *ctx)
{
    int i;
    for (i = 0; i < GPUMP89_MAX_VOICES; ++i) {
        ctx->voices[i].samples = 0;
        ctx->voices[i].length = 0UL;
        ctx->voices[i].position_q16 = 0UL;
        ctx->voices[i].step_q16 = 0UL;
        ctx->voices[i].gain_q15 = 0L;
        ctx->voices[i].active = 0U;
    }
}

static int gpump89_builtin_grain(int stage, gpump89_grain *g)
{
    if (g == 0) return 0;
    g->sample_rate = GPUMP89_GRAIN_RATE;
    switch (stage) {
    case GPUMP89_STAGE_UNLOCK:
        g->samples = gpump89_grain_unlock;
        g->length = (gpump89_u32)(sizeof(gpump89_grain_unlock) / sizeof(gpump89_grain_unlock[0]));
        return 1;
    case GPUMP89_STAGE_FRICTION_REAR:
        g->samples = gpump89_grain_friction_rear;
        g->length = (gpump89_u32)(sizeof(gpump89_grain_friction_rear) / sizeof(gpump89_grain_friction_rear[0]));
        return 1;
    case GPUMP89_STAGE_EXTRACTOR:
        g->samples = gpump89_grain_extractor;
        g->length = (gpump89_u32)(sizeof(gpump89_grain_extractor) / sizeof(gpump89_grain_extractor[0]));
        return 1;
    case GPUMP89_STAGE_REAR_STOP:
        g->samples = gpump89_grain_rear;
        g->length = (gpump89_u32)(sizeof(gpump89_grain_rear) / sizeof(gpump89_grain_rear[0]));
        return 1;
    case GPUMP89_STAGE_SHELL_EJECT:
        g->samples = gpump89_grain_shell;
        g->length = (gpump89_u32)(sizeof(gpump89_grain_shell) / sizeof(gpump89_grain_shell[0]));
        return 1;
    case GPUMP89_STAGE_FRICTION_FORWARD:
        g->samples = gpump89_grain_friction_forward;
        g->length = (gpump89_u32)(sizeof(gpump89_grain_friction_forward) / sizeof(gpump89_grain_friction_forward[0]));
        return 1;
    case GPUMP89_STAGE_CARRIER:
        g->samples = gpump89_grain_carrier;
        g->length = (gpump89_u32)(sizeof(gpump89_grain_carrier) / sizeof(gpump89_grain_carrier[0]));
        return 1;
    case GPUMP89_STAGE_BATTERY:
        g->samples = gpump89_grain_battery;
        g->length = (gpump89_u32)(sizeof(gpump89_grain_battery) / sizeof(gpump89_grain_battery[0]));
        return 1;
    case GPUMP89_STAGE_LOCK:
        g->samples = gpump89_grain_lock;
        g->length = (gpump89_u32)(sizeof(gpump89_grain_lock) / sizeof(gpump89_grain_lock[0]));
        return 1;
    default:
        return 0;
    }
}

static int gpump89_get_grain(gpump89_context *ctx, int stage, gpump89_grain *g)
{
    if (ctx->provider != 0) {
        if (ctx->provider(ctx->provider_user, stage, g)) {
            if (g->samples != 0 && g->length > 1UL && g->sample_rate > 0UL) {
                return 1;
            }
        }
    }
    return gpump89_builtin_grain(stage, g);
}

static gpump89_i32 gpump89_stage_gain(const gpump89_context *ctx,
                                      int stage,
                                      gpump89_i32 intensity_q15)
{
    gpump89_i32 base;
    gpump89_i32 wear;
    wear = ctx->params.wear_q15;
    switch (stage) {
    case GPUMP89_STAGE_UNLOCK: base = 15500L; break;
    case GPUMP89_STAGE_FRICTION_REAR: base = ctx->params.slide_q15; break;
    case GPUMP89_STAGE_EXTRACTOR: base = 10500L; break;
    case GPUMP89_STAGE_REAR_STOP: base = 28000L; break;
    case GPUMP89_STAGE_SHELL_EJECT: base = 11500L; break;
    case GPUMP89_STAGE_FRICTION_FORWARD: base = ctx->params.slide_q15 * 9L / 10L; break;
    case GPUMP89_STAGE_CARRIER: base = 11000L; break;
    case GPUMP89_STAGE_BATTERY: base = 30500L; break;
    case GPUMP89_STAGE_LOCK: base = 12500L; break;
    default: base = 0L; break;
    }
    if (stage == GPUMP89_STAGE_FRICTION_REAR
        || stage == GPUMP89_STAGE_FRICTION_FORWARD) {
        base += wear / 7L;
    } else if (stage == GPUMP89_STAGE_EXTRACTOR
               || stage == GPUMP89_STAGE_CARRIER
               || stage == GPUMP89_STAGE_LOCK) {
        base += wear / 10L;
    }
    base = gpump89_mul_q15(base, intensity_q15);
    base = gpump89_mul_q15(base, 18000L + ctx->params.force_q15 / 2L);
    return gpump89_clamp_q15(base);
}

static void gpump89_start_voice(gpump89_context *ctx,
                                int stage,
                                gpump89_i32 intensity_q15,
                                gpump89_u32 duration_frames)
{
    gpump89_grain g;
    gpump89_u32 step;
    gpump89_i32 gain;
    gpump89_i32 jitter;
    gpump89_i32 vary;
    int i;

    if (!gpump89_get_grain(ctx, stage, &g)) return;

    step = (g.sample_rate << 16) / ctx->params.sample_rate;
    if (duration_frames > 0UL) {
        step = (g.length << 16) / duration_frames;
        if (step == 0UL) step = 1UL;
    }

    vary = ctx->params.variation_q15;
    jitter = gpump89_mul_q15(gpump89_rand_signed(ctx), vary);
    if (jitter > 4096L) jitter = 4096L;
    if (jitter < -4096L) jitter = -4096L;
    step = (gpump89_u32)((gpump89_i32)step
           + ((gpump89_i32)step * jitter) / 131072L);
    if (step == 0UL) step = 1UL;

    gain = gpump89_stage_gain(ctx, stage, intensity_q15);
    jitter = gpump89_mul_q15(gpump89_rand_signed(ctx), vary);
    gain += gpump89_mul_q15(gain, jitter / 8L);
    gain = gpump89_clamp_q15(gain);

    for (i = 0; i < GPUMP89_MAX_VOICES; ++i) {
        if (!ctx->voices[i].active) {
            ctx->voices[i].samples = g.samples;
            ctx->voices[i].length = g.length;
            ctx->voices[i].position_q16 = 0UL;
            ctx->voices[i].step_q16 = step;
            ctx->voices[i].gain_q15 = gain;
            ctx->voices[i].active = 1U;
            return;
        }
    }
}

static void gpump89_add_event(gpump89_context *ctx,
                              gpump89_u32 frame,
                              int stage,
                              gpump89_i32 gain_q15,
                              gpump89_u32 duration_frames)
{
    gpump89_event *e;
    if (ctx->event_count >= GPUMP89_MAX_EVENTS) return;
    e = &ctx->events[ctx->event_count++];
    e->frame = frame;
    e->stage = (gpump89_u8)stage;
    e->gain_q15 = gain_q15;
    e->duration_frames = duration_frames;
}

void gpump89_params_default(gpump89_params *p, gpump89_u32 sample_rate)
{
    if (p == 0) return;
    p->sample_rate = sample_rate < 8000UL ? 8000UL : sample_rate;
    p->cycle_ms = 440UL;
    p->force_q15 = 28000L;
    p->slide_q15 = 9800L;
    p->wear_q15 = 3000L;
    p->variation_q15 = 1400L;
    p->master_q15 = 28500L;
    p->shell_enabled = 0UL;
    p->seed = 0x47503252UL;
}

int gpump89_preset(gpump89_params *p, int preset_id, gpump89_u32 sample_rate)
{
    gpump89_params_default(p, sample_rate);
    if (p == 0) return 0;
    switch (preset_id) {
    case GPUMP89_PRESET_TIGHT_DRY:
        p->cycle_ms = 410UL;
        p->force_q15 = 27800L;
        p->slide_q15 = 8200L;
        p->wear_q15 = 1800L;
        p->variation_q15 = 900L;
        p->master_q15 = 29400L;
        p->seed = 0x87008702UL;
        break;
    case GPUMP89_PRESET_LOOSE_SERVICE:
        p->cycle_ms = 485UL;
        p->force_q15 = 26800L;
        p->slide_q15 = 10800L;
        p->wear_q15 = 18800L;
        p->variation_q15 = 2600L;
        p->master_q15 = 28600L;
        p->seed = 0x50005002UL;
        break;
    case GPUMP89_PRESET_HEAVY:
        p->cycle_ms = 535UL;
        p->force_q15 = 32200L;
        p->slide_q15 = 9000L;
        p->wear_q15 = 4200L;
        p->variation_q15 = 1100L;
        p->master_q15 = 27000L;
        p->seed = 0x59005902UL;
        break;
    case GPUMP89_PRESET_WITH_SHELL:
        p->cycle_ms = 455UL;
        p->force_q15 = 29200L;
        p->slide_q15 = 8800L;
        p->wear_q15 = 3500L;
        p->variation_q15 = 1500L;
        p->master_q15 = 28000L;
        p->shell_enabled = 1UL;
        p->seed = 0x53484C32UL;
        break;
    case GPUMP89_PRESET_CINEMATIC_DRY:
        p->cycle_ms = 610UL;
        p->force_q15 = 32767L;
        p->slide_q15 = 10300L;
        p->wear_q15 = 6200L;
        p->variation_q15 = 1800L;
        p->master_q15 = 28600L;
        p->seed = 0x43494E32UL;
        break;
    default:
        return 0;
    }
    return 1;
}

void gpump89_init(gpump89_context *ctx,
                  gpump89_u32 sample_rate,
                  gpump89_u32 seed)
{
    if (ctx == 0) return;
    gpump89_params_default(&ctx->params, sample_rate);
    ctx->params.seed = seed;
    ctx->cursor = 0UL;
    ctx->total_frames = 0UL;
    ctx->rng = seed;
    ctx->event_count = 0U;
    ctx->event_index = 0U;
    ctx->active = 0U;
    ctx->provider = 0;
    ctx->provider_user = 0;
    gpump89_clear_voices(ctx);
}

void gpump89_set_provider(gpump89_context *ctx,
                          gpump89_grain_provider provider,
                          void *user)
{
    if (ctx == 0) return;
    ctx->provider = provider;
    ctx->provider_user = user;
}

void gpump89_start_cycle(gpump89_context *ctx, const gpump89_params *p)
{
    gpump89_u32 cycle;
    gpump89_u32 rear_dur;
    gpump89_u32 forward_dur;
    gpump89_i32 normal;
    gpump89_i32 hard;

    if (ctx == 0 || p == 0) return;
    ctx->params = *p;
    if (ctx->params.sample_rate < 8000UL) ctx->params.sample_rate = 8000UL;
    ctx->params.force_q15 = gpump89_clamp_q15(ctx->params.force_q15);
    ctx->params.slide_q15 = gpump89_clamp_q15(ctx->params.slide_q15);
    ctx->params.wear_q15 = gpump89_clamp_q15(ctx->params.wear_q15);
    ctx->params.variation_q15 = gpump89_clamp_q15(ctx->params.variation_q15);
    ctx->params.master_q15 = gpump89_clamp_q15(ctx->params.master_q15);
    ctx->rng = ctx->params.seed;
    ctx->cursor = 0UL;
    ctx->event_count = 0U;
    ctx->event_index = 0U;
    gpump89_clear_voices(ctx);

    cycle = gpump89_ms_to_frames(ctx->params.sample_rate,
                                  ctx->params.cycle_ms);
    rear_dur = gpump89_permille(cycle, 285UL);
    forward_dur = gpump89_permille(cycle, 285UL);
    ctx->total_frames = cycle
        + gpump89_ms_to_frames(ctx->params.sample_rate, 110UL);

    normal = 30000L;
    hard = 32767L;

    gpump89_add_event(ctx, gpump89_permille(cycle, 25UL),
                      GPUMP89_STAGE_UNLOCK, normal, 0UL);
    gpump89_add_event(ctx, gpump89_permille(cycle, 70UL),
                      GPUMP89_STAGE_FRICTION_REAR, normal, rear_dur);
    gpump89_add_event(ctx, gpump89_permille(cycle, 235UL),
                      GPUMP89_STAGE_EXTRACTOR, normal, 0UL);
    gpump89_add_event(ctx, gpump89_permille(cycle, 400UL),
                      GPUMP89_STAGE_REAR_STOP, hard, 0UL);

    if (ctx->params.shell_enabled) {
        gpump89_add_event(ctx, gpump89_permille(cycle, 430UL),
                          GPUMP89_STAGE_SHELL_EJECT, normal, 0UL);
    }

    gpump89_add_event(ctx, gpump89_permille(cycle, 540UL),
                      GPUMP89_STAGE_FRICTION_FORWARD, normal, forward_dur);
    gpump89_add_event(ctx, gpump89_permille(cycle, 710UL),
                      GPUMP89_STAGE_CARRIER, normal, 0UL);
    gpump89_add_event(ctx, gpump89_permille(cycle, 900UL),
                      GPUMP89_STAGE_BATTERY, hard, 0UL);
    gpump89_add_event(ctx, gpump89_permille(cycle, 930UL),
                      GPUMP89_STAGE_LOCK, normal, 0UL);

    ctx->active = 1U;
}

void gpump89_trigger_stage(gpump89_context *ctx,
                           int stage,
                           gpump89_i32 intensity_q15,
                           gpump89_u32 duration_ms)
{
    gpump89_u32 frames;
    if (ctx == 0) return;
    if (stage < 0 || stage >= GPUMP89_STAGE_COUNT) return;
    intensity_q15 = gpump89_clamp_q15(intensity_q15);
    frames = duration_ms > 0UL
        ? gpump89_ms_to_frames(ctx->params.sample_rate, duration_ms)
        : 0UL;
    gpump89_start_voice(ctx, stage, intensity_q15, frames);
    ctx->active = 1U;
}

void gpump89_stop(gpump89_context *ctx)
{
    if (ctx == 0) return;
    ctx->active = 0U;
    ctx->event_count = 0U;
    ctx->event_index = 0U;
    gpump89_clear_voices(ctx);
}

gpump89_u32 gpump89_render_i16(gpump89_context *ctx,
                               gpump89_i16 *out,
                               gpump89_u32 frames)
{
    gpump89_u32 written;
    gpump89_i32 mix;
    int i;
    int any;

    if (ctx == 0 || out == 0 || frames == 0UL || !ctx->active) return 0UL;

    written = 0UL;
    while (written < frames && ctx->active) {
        while (ctx->event_index < ctx->event_count
            && ctx->events[ctx->event_index].frame <= ctx->cursor) {
            gpump89_event *e;
            e = &ctx->events[ctx->event_index];
            gpump89_start_voice(ctx, (int)e->stage,
                                e->gain_q15, e->duration_frames);
            ctx->event_index++;
        }

        mix = 0L;
        any = 0;
        for (i = 0; i < GPUMP89_MAX_VOICES; ++i) {
            gpump89_voice *v;
            gpump89_u32 index;
            gpump89_u32 frac;
            gpump89_i32 a;
            gpump89_i32 b;
            gpump89_i32 s;
            v = &ctx->voices[i];
            if (!v->active) continue;
            index = v->position_q16 >> 16;
            if (index >= v->length - 1UL) {
                v->active = 0U;
                continue;
            }
            frac = v->position_q16 & 65535UL;
            a = (gpump89_i32)v->samples[index] << 8;
            b = (gpump89_i32)v->samples[index + 1UL] << 8;
            s = a + ((b - a) * (gpump89_i32)frac) / 65536L;
            mix += gpump89_mul_q15(s, v->gain_q15);
            v->position_q16 += v->step_q16;
            any = 1;
        }

        mix = gpump89_mul_q15(mix, ctx->params.master_q15);
        mix = gpump89_soft_clip(mix);
        out[written++] = (gpump89_i16)gpump89_sat_i16(mix);
        ctx->cursor++;

        if (ctx->cursor >= ctx->total_frames
            && ctx->event_index >= ctx->event_count
            && !any) {
            ctx->active = 0U;
        }
    }
    return written;
}

int gpump89_is_active(const gpump89_context *ctx)
{
    return ctx != 0 && ctx->active != 0U;
}

gpump89_u32 gpump89_total_frames(const gpump89_context *ctx)
{
    return ctx == 0 ? 0UL : ctx->total_frames;
}
