#include "rawmix.h"

#include <stddef.h>
#include <string.h>

#define RM_PHASE_ONE_Q12 ((rm_u16)4096)
#define RM_PHASE_MASK_Q12 ((rm_u16)4095)

#define RM_SOURCE_KIND_NONE   ((rm_u8)0)
#define RM_SOURCE_KIND_BUFFER ((rm_u8)1)
#define RM_SOURCE_KIND_STREAM ((rm_u8)2)

static int rm_valid_bus_id(rm_u16 bus_id);
static int rm_valid_group_id(rm_u16 group_id);

static rm_s16 rm_clamp_s16(rm_s32 v)
{
    if (v > 32767) {
        return (rm_s16)32767;
    }
    if (v < -32768) {
        return (rm_s16)-32768;
    }
    return (rm_s16)v;
}

static rm_s32 rm_shr_round_s32(rm_s32 v, rm_u16 shift)
{
    rm_s32 bias;

    if (shift == 0U) {
        return v;
    }

    bias = (rm_s32)1 << (shift - 1U);
    if (v >= 0) {
        return (v + bias) >> shift;
    }
    return (v - bias) >> shift;
}

static rm_u32 rm_abs_s16_u32(rm_s16 v)
{
    if (v < 0) {
        return (rm_u32)(-(rm_s32)v);
    }
    return (rm_u32)v;
}

static rm_u32 rm_abs_s32_u32(rm_s32 v)
{
    if (v < 0) {
        return (rm_u32)(-(v + 1)) + 1U;
    }
    return (rm_u32)v;
}

static rm_s16 rm_mul_q15(rm_s16 a, rm_s16 b)
{
    rm_s32 t;

    t = (rm_s32)a * (rm_s32)b;
    if (t >= 0) {
        t += 16384;
    } else {
        t -= 16384;
    }
    t >>= 15;
    return rm_clamp_s16(t);
}

static rm_s16 rm_pan_scale_left(rm_s16 pan_q15)
{
    if (pan_q15 > 0) {
        return (rm_s16)(32767 - pan_q15);
    }
    return (rm_s16)32767;
}

static rm_s16 rm_pan_scale_right(rm_s16 pan_q15)
{
    if (pan_q15 < 0) {
        return (rm_s16)(32767 + pan_q15);
    }
    return (rm_s16)32767;
}

static void rm_update_pair_gains(rm_s16 gain_q15,
                                 rm_s16 pan_q15,
                                 rm_s16 *out_left,
                                 rm_s16 *out_right)
{
    rm_s16 lscale;
    rm_s16 rscale;

    lscale = rm_pan_scale_left(pan_q15);
    rscale = rm_pan_scale_right(pan_q15);

    *out_left = rm_mul_q15(gain_q15, lscale);
    *out_right = rm_mul_q15(gain_q15, rscale);
}

static rm_s16 rm_step_towards_s16(rm_s16 current, rm_s16 target, rm_u32 remaining)
{
    rm_s32 delta;
    rm_s32 step;
    rm_s32 next;

    if (remaining <= 1U) {
        return target;
    }

    delta = (rm_s32)target - (rm_s32)current;
    if (delta == 0) {
        return target;
    }

    step = delta / (rm_s32)remaining;
    if (step == 0) {
        step = (delta > 0) ? 1 : -1;
    }

    next = (rm_s32)current + step;
    if ((delta > 0 && next > target) || (delta < 0 && next < target)) {
        next = target;
    }
    return rm_clamp_s16(next);
}

static rm_s16 rm_smooth_towards_s16(rm_s16 current, rm_s16 target, rm_u16 frames)
{
    rm_s32 delta;
    rm_s32 step;

    if (frames <= 1U) {
        return target;
    }

    delta = (rm_s32)target - (rm_s32)current;
    if (delta == 0) {
        return target;
    }

    step = delta / (rm_s32)frames;
    if (step == 0) {
        step = (delta > 0) ? 1 : -1;
    }

    return rm_clamp_s16((rm_s32)current + step);
}

static void rm_pair_control_reset(rm_s16 *io_gain,
                                  rm_s16 *io_pan,
                                  rm_s16 *out_left,
                                  rm_s16 *out_right,
                                  rm_s16 *io_target_gain,
                                  rm_s16 *io_target_pan,
                                  rm_u32 *io_ramp_frames)
{
    *io_gain = RM_Q15_ONE;
    *io_pan = RM_PAN_CENTER;
    *io_target_gain = RM_Q15_ONE;
    *io_target_pan = RM_PAN_CENTER;
    *io_ramp_frames = 0U;
    rm_update_pair_gains(*io_gain, *io_pan, out_left, out_right);
}

static void rm_pair_control_set(rm_s16 *io_gain,
                                rm_s16 *io_pan,
                                rm_s16 *out_left,
                                rm_s16 *out_right,
                                rm_s16 *io_target_gain,
                                rm_s16 *io_target_pan,
                                rm_u32 *io_ramp_frames,
                                rm_s16 gain_q15,
                                rm_s16 pan_q15)
{
    *io_gain = gain_q15;
    *io_pan = pan_q15;
    *io_target_gain = gain_q15;
    *io_target_pan = pan_q15;
    *io_ramp_frames = 0U;
    rm_update_pair_gains(*io_gain, *io_pan, out_left, out_right);
}

static void rm_pair_control_ramp(rm_s16 *io_gain,
                                 rm_s16 *io_pan,
                                 rm_s16 *out_left,
                                 rm_s16 *out_right,
                                 rm_s16 *io_target_gain,
                                 rm_s16 *io_target_pan,
                                 rm_u32 *io_ramp_frames,
                                 rm_s16 gain_q15,
                                 rm_s16 pan_q15,
                                 rm_u32 frames)
{
    if (frames == 0U) {
        rm_pair_control_set(io_gain,
                            io_pan,
                            out_left,
                            out_right,
                            io_target_gain,
                            io_target_pan,
                            io_ramp_frames,
                            gain_q15,
                            pan_q15);
        return;
    }

    *io_target_gain = gain_q15;
    *io_target_pan = pan_q15;
    *io_ramp_frames = frames;
    rm_update_pair_gains(*io_gain, *io_pan, out_left, out_right);
}

static void rm_pair_control_step(rm_s16 *io_gain,
                                 rm_s16 *io_pan,
                                 rm_s16 *out_left,
                                 rm_s16 *out_right,
                                 rm_s16 *io_target_gain,
                                 rm_s16 *io_target_pan,
                                 rm_u32 *io_ramp_frames)
{
    if (*io_ramp_frames == 0U) {
        return;
    }

    *io_gain = rm_step_towards_s16(*io_gain, *io_target_gain, *io_ramp_frames);
    *io_pan = rm_step_towards_s16(*io_pan, *io_target_pan, *io_ramp_frames);

    (*io_ramp_frames)--;
    if (*io_ramp_frames == 0U) {
        *io_gain = *io_target_gain;
        *io_pan = *io_target_pan;
    }

    rm_update_pair_gains(*io_gain, *io_pan, out_left, out_right);
}


static rm_s16 rm_clamp_unit_q15(rm_s32 v)
{
    if (v < 0) {
        return RM_Q15_ZERO;
    }
    if (v > 32767) {
        return RM_Q15_ONE;
    }
    return (rm_s16)v;
}

static int rm_valid_resampler(rm_u16 resampler)
{
    return resampler == (rm_u16)RM_RESAMPLER_DEFAULT ||
           resampler == (rm_u16)RM_RESAMPLER_NEAREST ||
           resampler == (rm_u16)RM_RESAMPLER_LINEAR ||
           resampler == (rm_u16)RM_RESAMPLER_CUBIC;
}

static rm_u16 rm_resolve_resampler(rm_u16 requested, rm_u16 fallback)
{
    if (requested == (rm_u16)RM_RESAMPLER_DEFAULT) {
        requested = fallback;
    }
    if (!rm_valid_resampler(requested) || requested == (rm_u16)RM_RESAMPLER_DEFAULT) {
        requested = (rm_u16)RM_RESAMPLER_LINEAR;
    }
    return requested;
}

static int rm_valid_bus_fx_slot(rm_u16 slot)
{
    return slot < (rm_u16)RAWMIX_MAX_BUS_FX;
}

static int rm_valid_send_mode(rm_u16 mode)
{
    return mode == (rm_u16)RM_SEND_PRE_FADER || mode == (rm_u16)RM_SEND_POST_FADER;
}

static void rm_reset_meter_state(rm_meter_state *meter)
{
    if (meter == NULL) {
        return;
    }
    memset(meter, 0, sizeof(*meter));
}

static void rm_reset_bus_send_state(rm_bus_send_state *send)
{
    if (send == NULL) {
        return;
    }
    send->active = 0U;
    send->mode = (rm_u8)RM_SEND_PRE_FADER;
    send->gain_q15 = RM_Q15_ZERO;
}

static rm_s16 rm_s32_abs_to_meter_q15(rm_s32 sample)
{
    rm_u32 abs_value;

    abs_value = rm_abs_s32_u32(sample);
    if (abs_value > 32767U) {
        abs_value = 32767U;
    }
    return (rm_s16)abs_value;
}

static void rm_update_meter_pair(rm_meter_state *meter, rm_s32 left, rm_s32 right)
{
    rm_s16 abs_left;
    rm_s16 abs_right;

    if (meter == NULL) {
        return;
    }

    abs_left = rm_s32_abs_to_meter_q15(left);
    abs_right = rm_s32_abs_to_meter_q15(right);

    if (abs_left > meter->peak_left_q15) {
        meter->peak_left_q15 = abs_left;
    }
    if (abs_right > meter->peak_right_q15) {
        meter->peak_right_q15 = abs_right;
    }

    meter->env_left_q15 = rm_smooth_towards_s16(meter->env_left_q15, abs_left, 16U);
    meter->env_right_q15 = rm_smooth_towards_s16(meter->env_right_q15, abs_right, 16U);

    if (rm_abs_s32_u32(left) > 32767U) {
        meter->clip_events++;
    }
    if (rm_abs_s32_u32(right) > 32767U) {
        meter->clip_events++;
    }
}

static int rm_any_bus_solo_active(const rm_engine *engine)
{
    rm_u16 i;

    for (i = 0U; i < (rm_u16)RAWMIX_MAX_BUSES; ++i) {
        if (engine->buses[i].solo) {
            return 1;
        }
    }
    return 0;
}

static int rm_any_group_solo_active(const rm_engine *engine)
{
    rm_u16 i;

    for (i = 0U; i < (rm_u16)RAWMIX_MAX_GROUPS; ++i) {
        if (engine->groups[i].solo) {
            return 1;
        }
    }
    return 0;
}

static int rm_group_allows_audio(const rm_engine *engine, rm_u16 group_id)
{
    const rm_group_state *group;

    if (!rm_valid_group_id(group_id)) {
        group_id = RM_GROUP_DEFAULT;
    }
    group = &engine->groups[group_id];

    if (group->mute) {
        return 0;
    }
    if (rm_any_group_solo_active(engine) && !group->solo) {
        return 0;
    }
    return 1;
}

static int rm_bus_allows_output(const rm_engine *engine, rm_u16 bus_id)
{
    const rm_bus_state *bus;

    if (!rm_valid_bus_id(bus_id)) {
        bus_id = RM_BUS_DEFAULT;
    }
    bus = &engine->buses[bus_id];

    if (bus->mute) {
        return 0;
    }
    if (rm_any_bus_solo_active(engine) && !bus->solo) {
        return 0;
    }
    return 1;
}

static void rm_reset_bus_fx_state(rm_bus_fx_state *fx)
{
    if (fx == NULL) {
        return;
    }

    memset(fx, 0, sizeof(*fx));
    fx->type = (rm_u8)RM_BUS_FX_NONE;
    fx->active = 0U;
    fx->wet_q15 = RM_Q15_ONE;
    fx->output_gain_q15 = RM_Q15_ONE;
}

static void rm_reset_bus_state(rm_bus_state *bus)
{
    rm_u16 i;

    if (bus == NULL) {
        return;
    }

    bus->active = 1U;
    bus->mute = 0U;
    bus->solo = 0U;
    bus->reserved0 = 0U;
    rm_pair_control_reset(&bus->gain_q15,
                          &bus->pan_q15,
                          &bus->left_gain_q15,
                          &bus->right_gain_q15,
                          &bus->target_gain_q15,
                          &bus->target_pan_q15,
                          &bus->ramp_frames_remaining);
    rm_reset_meter_state(&bus->meter);
    for (i = 0U; i < (rm_u16)RAWMIX_MAX_BUS_FX; ++i) {
        rm_reset_bus_fx_state(&bus->fx[i]);
    }
}

static void rm_reset_group_state(rm_group_state *group)
{
    if (group == NULL) {
        return;
    }

    group->active = 1U;
    group->mute = 0U;
    group->solo = 0U;
    group->reserved0 = 0U;
    rm_pair_control_reset(&group->gain_q15,
                          &group->pan_q15,
                          &group->left_gain_q15,
                          &group->right_gain_q15,
                          &group->target_gain_q15,
                          &group->target_pan_q15,
                          &group->ramp_frames_remaining);
}

static void rm_reset_limiter_state(rm_limiter_state *limiter)
{
    if (limiter == NULL) {
        return;
    }

    memset(limiter, 0, sizeof(*limiter));
    limiter->active = 0U;
    limiter->attack_frames = 4U;
    limiter->release_frames = 64U;
    limiter->lookahead_frames = 0U;
    limiter->delay_write_frame = 0U;
    limiter->delay_count_frames = 0U;
    limiter->threshold_q15 = (rm_s16)30000;
    limiter->output_gain_q15 = RM_Q15_ONE;
    limiter->current_gain_q15 = RM_Q15_ONE;
}

static rm_u16 rm_compute_step_q12(rm_u32 src_rate, rm_u32 dst_rate, rm_u16 pitch_q12)
{
    rm_u32 base_step_q12;
    rm_u32 q;
    rm_u32 r;
    rm_u32 scaled;

    if (dst_rate == 0U) {
        return RM_RATIO_ONE_Q12;
    }

    q = src_rate / dst_rate;
    r = src_rate % dst_rate;
    base_step_q12 = (q << 12) + ((r * 4096U) / dst_rate);
    if (base_step_q12 == 0U) {
        base_step_q12 = 1U;
    }

    scaled = (base_step_q12 * (rm_u32)pitch_q12 + 2048U) >> 12;
    if (scaled == 0U) {
        scaled = 1U;
    }
    if (scaled > 65535U) {
        scaled = 65535U;
    }
    return (rm_u16)scaled;
}

static rm_s32 rm_mul_s32_q15(rm_s32 v, rm_s16 gain_q15)
{
    int neg;
    rm_u32 av;
    rm_u32 ag;
    rm_u32 hi;
    rm_u32 lo;
    rm_u32 res;

    neg = 0;
    if (v < 0) {
        neg = !neg;
        av = (rm_u32)(-(v + 1)) + 1U;
    } else {
        av = (rm_u32)v;
    }

    if (gain_q15 < 0) {
        neg = !neg;
        ag = (rm_u32)(-(rm_s32)gain_q15);
    } else {
        ag = (rm_u32)gain_q15;
    }

    hi = av >> 15;
    lo = av & 0x7FFFU;
    res = hi * ag + ((lo * ag + 16384U) >> 15);
    if (res > 2147483647UL) {
        res = 2147483647UL;
    }

    if (neg) {
        if (res >= 2147483648UL) {
            return (rm_s32)-2147483647L - 1L;
        }
        return (rm_s32)(-(rm_s32)res);
    }
    return (rm_s32)res;
}

static rm_s32 rm_mul_s32_q12(rm_s32 v, rm_u16 gain_q12)
{
    int neg;
    rm_u32 av;
    rm_u32 ag;
    rm_u32 hi;
    rm_u32 lo;
    rm_u32 res;

    neg = 0;
    if (v < 0) {
        neg = !neg;
        av = (rm_u32)(-(v + 1)) + 1U;
    } else {
        av = (rm_u32)v;
    }

    ag = (rm_u32)gain_q12;
    hi = av >> 12;
    lo = av & 0xFFFU;
    res = hi * ag + ((lo * ag + 2048U) >> 12);
    if (res > 2147483647UL) {
        res = 2147483647UL;
    }

    if (neg) {
        if (res >= 2147483648UL) {
            return (rm_s32)-2147483647L - 1L;
        }
        return (rm_s32)(-(rm_s32)res);
    }
    return (rm_s32)res;
}

static rm_s32 rm_mul_s32_q14(rm_s32 v, rm_s16 gain_q14)
{
    int neg;
    rm_u32 av;
    rm_u32 ag;
    rm_u32 hi;
    rm_u32 lo;
    rm_u32 res;

    neg = 0;
    if (v < 0) {
        neg = !neg;
        av = (rm_u32)(-(v + 1)) + 1U;
    } else {
        av = (rm_u32)v;
    }

    if (gain_q14 < 0) {
        neg = !neg;
        ag = (rm_u32)(-(rm_s32)gain_q14);
    } else {
        ag = (rm_u32)gain_q14;
    }

    hi = av >> 14;
    lo = av & 0x3FFFU;
    res = hi * ag + ((lo * ag + 8192U) >> 14);
    if (res > 2147483647UL) {
        res = 2147483647UL;
    }

    if (neg) {
        if (res >= 2147483648UL) {
            return (rm_s32)-2147483647L - 1L;
        }
        return (rm_s32)(-(rm_s32)res);
    }
    return (rm_s32)res;
}

static rm_s16 rm_compute_lowpass_alpha_q15(rm_u32 cutoff_hz, rm_u32 sample_rate)
{
    rm_u32 denom;
    rm_u32 value;

    if (cutoff_hz == 0U) {
        return RM_Q15_ZERO;
    }
    if (sample_rate == 0U) {
        return RM_Q15_ONE;
    }

    denom = sample_rate + (cutoff_hz * 2U);
    if (denom == 0U) {
        return RM_Q15_ONE;
    }

    value = ((cutoff_hz * 2U) * 32767U) / denom;
    if (value == 0U) {
        value = 1U;
    }
    if (value > 32767U) {
        value = 32767U;
    }
    return (rm_s16)value;
}

static rm_s32 rm_wet_dry_mix_s32(rm_s32 dry_sample, rm_s32 wet_sample, rm_s16 wet_q15)
{
    rm_s16 wet;
    rm_s16 dry;

    wet = rm_clamp_unit_q15((rm_s32)wet_q15);
    dry = (rm_s16)(32767 - wet);
    return rm_mul_s32_q15(dry_sample, dry) + rm_mul_s32_q15(wet_sample, wet);
}

static rm_s32 rm_soft_clip_s32(rm_s32 sample, rm_s32 threshold)
{
    rm_u32 abs_value;
    rm_u32 clipped;

    if (threshold <= 0) {
        threshold = 1;
    }

    if (sample < 0) {
        abs_value = (rm_u32)(-(sample + 1)) + 1U;
    } else {
        abs_value = (rm_u32)sample;
    }

    if (abs_value <= (rm_u32)threshold) {
        return sample;
    }

    clipped = (rm_u32)threshold + ((abs_value - (rm_u32)threshold) >> 2);
    if (clipped > 32767U) {
        clipped = 32767U;
    }

    if (sample < 0) {
        return (rm_s32)(-(rm_s32)clipped);
    }
    return (rm_s32)clipped;
}

static void rm_bus_fx_process_lowpass(rm_bus_fx_state *fx,
                                      rm_u16 channels,
                                      rm_s32 *io_left,
                                      rm_s32 *io_right)
{
    rm_s16 alpha_q15;
    rm_s32 dry_left;
    rm_s32 dry_right;
    rm_s32 filtered_left;
    rm_s32 filtered_right;

    alpha_q15 = (rm_s16)fx->param1;
    dry_left = *io_left;
    dry_right = *io_right;

    filtered_left = fx->state_left + rm_mul_s32_q15(dry_left - fx->state_left, alpha_q15);
    fx->state_left = filtered_left;
    *io_left = rm_wet_dry_mix_s32(dry_left, filtered_left, fx->wet_q15);
    *io_left = rm_mul_s32_q15(*io_left, fx->output_gain_q15);

    if (channels > 1U && io_right != NULL) {
        filtered_right = fx->state_right + rm_mul_s32_q15(dry_right - fx->state_right, alpha_q15);
        fx->state_right = filtered_right;
        *io_right = rm_wet_dry_mix_s32(dry_right, filtered_right, fx->wet_q15);
        *io_right = rm_mul_s32_q15(*io_right, fx->output_gain_q15);
    }
}

static void rm_bus_fx_process_drive(rm_bus_fx_state *fx,
                                    rm_u16 channels,
                                    rm_s32 *io_left,
                                    rm_s32 *io_right)
{
    rm_s32 dry_left;
    rm_s32 dry_right;
    rm_s32 wet_left;
    rm_s32 wet_right;
    rm_s32 threshold;

    dry_left = *io_left;
    dry_right = *io_right;
    threshold = (rm_s32)fx->param1;
    if (threshold <= 0) {
        threshold = 24576;
    }

    wet_left = rm_mul_s32_q12(dry_left, (rm_u16)fx->param0);
    wet_left = rm_soft_clip_s32(wet_left, threshold);
    *io_left = rm_wet_dry_mix_s32(dry_left, wet_left, fx->wet_q15);
    *io_left = rm_mul_s32_q15(*io_left, fx->output_gain_q15);

    if (channels > 1U && io_right != NULL) {
        wet_right = rm_mul_s32_q12(dry_right, (rm_u16)fx->param0);
        wet_right = rm_soft_clip_s32(wet_right, threshold);
        *io_right = rm_wet_dry_mix_s32(dry_right, wet_right, fx->wet_q15);
        *io_right = rm_mul_s32_q15(*io_right, fx->output_gain_q15);
    }
}

static rm_s32 rm_biquad_step_channel(rm_bus_fx_state *fx, rm_s32 in_sample, int right_channel)
{
    rm_s32 z1;
    rm_s32 z2;
    rm_s32 y;
    rm_s32 new_z1;
    rm_s32 new_z2;

    if (right_channel) {
        z1 = fx->state_right;
        z2 = fx->state2_right;
    } else {
        z1 = fx->state_left;
        z2 = fx->state2_left;
    }

    y = rm_mul_s32_q14(in_sample, fx->b0_q14) + z1;
    new_z1 = rm_mul_s32_q14(in_sample, fx->b1_q14) - rm_mul_s32_q14(y, fx->a1_q14) + z2;
    new_z2 = rm_mul_s32_q14(in_sample, fx->b2_q14) - rm_mul_s32_q14(y, fx->a2_q14);

    if (right_channel) {
        fx->state_right = new_z1;
        fx->state2_right = new_z2;
    } else {
        fx->state_left = new_z1;
        fx->state2_left = new_z2;
    }

    return y;
}

static void rm_bus_fx_process_biquad(rm_bus_fx_state *fx,
                                     rm_u16 channels,
                                     rm_s32 *io_left,
                                     rm_s32 *io_right)
{
    rm_s32 dry_left;
    rm_s32 dry_right;
    rm_s32 wet_left;
    rm_s32 wet_right;

    dry_left = *io_left;
    dry_right = *io_right;

    wet_left = rm_biquad_step_channel(fx, dry_left, 0);
    *io_left = rm_wet_dry_mix_s32(dry_left, wet_left, fx->wet_q15);
    *io_left = rm_mul_s32_q15(*io_left, fx->output_gain_q15);

    if (channels > 1U && io_right != NULL) {
        wet_right = rm_biquad_step_channel(fx, dry_right, 1);
        *io_right = rm_wet_dry_mix_s32(dry_right, wet_right, fx->wet_q15);
        *io_right = rm_mul_s32_q15(*io_right, fx->output_gain_q15);
    }
}

static void rm_apply_limiter(rm_engine *engine, rm_s32 *io_left, rm_s32 *io_right)
{
    rm_limiter_state *limiter;
    rm_u32 peak;
    rm_s16 target_gain_q15;
    rm_u16 smooth_frames;
    rm_s32 threshold;
    int active_frame;
    rm_s32 in_left;
    rm_s32 in_right;
    rm_s32 delayed_left;
    rm_s32 delayed_right;
    rm_u16 write_pos;
    rm_u16 base;

    if (engine == NULL || io_left == NULL) {
        return;
    }

    limiter = &engine->limiter;
    if (!limiter->active) {
        limiter->current_gain_q15 = RM_Q15_ONE;
        engine->master_meter.gain_reduction_q15 = RM_Q15_ZERO;
        return;
    }

    threshold = (rm_s32)limiter->threshold_q15;
    if (threshold < 0) {
        threshold = -threshold;
    }
    if (threshold <= 0) {
        threshold = 1;
    }

    in_left = *io_left;
    in_right = (engine->channels == 2U && io_right != NULL) ? *io_right : in_left;

    peak = rm_abs_s32_u32(in_left);
    if (engine->channels == 2U && io_right != NULL) {
        rm_u32 right_peak;
        right_peak = rm_abs_s32_u32(in_right);
        if (right_peak > peak) {
            peak = right_peak;
        }
    }

    if (peak > (rm_u32)threshold) {
        target_gain_q15 = (rm_s16)(((rm_u32)threshold * 32767U) / peak);
    } else {
        target_gain_q15 = RM_Q15_ONE;
    }

    if (target_gain_q15 < limiter->current_gain_q15) {
        smooth_frames = limiter->attack_frames;
    } else {
        smooth_frames = limiter->release_frames;
    }
    limiter->current_gain_q15 = rm_smooth_towards_s16(limiter->current_gain_q15,
                                                      target_gain_q15,
                                                      smooth_frames);

    delayed_left = in_left;
    delayed_right = in_right;
    if (limiter->lookahead_frames > 0U) {
        if (limiter->lookahead_frames > (rm_u16)RAWMIX_LIMITER_MAX_LOOKAHEAD_FRAMES) {
            limiter->lookahead_frames = (rm_u16)RAWMIX_LIMITER_MAX_LOOKAHEAD_FRAMES;
        }
        write_pos = limiter->delay_write_frame;
        if (write_pos >= limiter->lookahead_frames) {
            write_pos = 0U;
        }
        if (limiter->delay_count_frames >= limiter->lookahead_frames) {
            base = (rm_u16)(write_pos * 2U);
            delayed_left = limiter->delay_line[base];
            delayed_right = limiter->delay_line[base + 1U];
        }
        base = (rm_u16)(write_pos * 2U);
        limiter->delay_line[base] = in_left;
        limiter->delay_line[base + 1U] = in_right;
        write_pos++;
        if (write_pos >= limiter->lookahead_frames) {
            write_pos = 0U;
        }
        limiter->delay_write_frame = write_pos;
        if (limiter->delay_count_frames < limiter->lookahead_frames) {
            limiter->delay_count_frames++;
        }
    }

    *io_left = rm_mul_s32_q15(delayed_left, limiter->current_gain_q15);
    *io_left = rm_mul_s32_q15(*io_left, limiter->output_gain_q15);
    *io_left = rm_soft_clip_s32(*io_left, threshold);

    if (engine->channels == 2U && io_right != NULL) {
        *io_right = rm_mul_s32_q15(delayed_right, limiter->current_gain_q15);
        *io_right = rm_mul_s32_q15(*io_right, limiter->output_gain_q15);
        *io_right = rm_soft_clip_s32(*io_right, threshold);
    }

    active_frame = (peak > (rm_u32)threshold) || (limiter->current_gain_q15 != RM_Q15_ONE);
    if (active_frame) {
        engine->stats.limiter_frames++;
    }

    engine->master_meter.gain_reduction_q15 = (rm_s16)(RM_Q15_ONE - limiter->current_gain_q15);
}

static int rm_apply_bus_fx_chain(rm_bus_state *bus,
                                 rm_u16 channels,
                                 rm_s32 *io_left,
                                 rm_s32 *io_right)
{
    rm_u16 i;
    int used_fx;
    rm_bus_fx_state *fx;

    if (bus == NULL || io_left == NULL) {
        return 0;
    }

    used_fx = 0;
    for (i = 0U; i < (rm_u16)RAWMIX_MAX_BUS_FX; ++i) {
        fx = &bus->fx[i];
        if (!fx->active || fx->type == (rm_u8)RM_BUS_FX_NONE) {
            continue;
        }

        used_fx = 1;
        if (fx->type == (rm_u8)RM_BUS_FX_LOWPASS) {
            rm_bus_fx_process_lowpass(fx, channels, io_left, io_right);
        } else if (fx->type == (rm_u8)RM_BUS_FX_DRIVE) {
            rm_bus_fx_process_drive(fx, channels, io_left, io_right);
        } else if (fx->type == (rm_u8)RM_BUS_FX_BIQUAD) {
            rm_bus_fx_process_biquad(fx, channels, io_left, io_right);
        }
    }

    return used_fx;
}

static int rm_valid_channels(rm_u16 channels)
{
    return channels == 1U || channels == 2U;
}

static int rm_valid_bus_id(rm_u16 bus_id)
{
    return bus_id < (rm_u16)RAWMIX_MAX_BUSES;
}

static int rm_valid_group_id(rm_u16 group_id)
{
    return group_id < (rm_u16)RAWMIX_MAX_GROUPS;
}

static rm_result rm_validate_handle(const rm_engine *engine,
                                    rm_voice_handle handle,
                                    rm_voice_state **out_voice)
{
    rm_voice_state *voice;

    if (engine == NULL) {
        return RM_ERR_INVALID_ARG;
    }
    if ((rm_u32)handle.slot >= (rm_u32)engine->max_voices) {
        return RM_ERR_BAD_HANDLE;
    }

    voice = (rm_voice_state *)&engine->voices[handle.slot];
    if (!voice->active) {
        return RM_ERR_BAD_HANDLE;
    }
    if (voice->generation != handle.generation) {
        return RM_ERR_BAD_HANDLE;
    }

    if (out_voice != NULL) {
        *out_voice = voice;
    }
    return RM_OK;
}

static void rm_clear_voice(rm_voice_state *voice)
{
    rm_u16 generation;

    if (voice == NULL) {
        return;
    }

    generation = voice->generation;

    memset(voice, 0, sizeof(*voice));
    voice->generation = generation;
    voice->source_kind = RM_SOURCE_KIND_NONE;
    voice->bus_id = RM_BUS_DEFAULT;
    voice->group_id = RM_GROUP_DEFAULT;
    voice->resampler = (rm_u16)RM_RESAMPLER_LINEAR;
    voice->step_q12 = RM_RATIO_ONE_Q12;
    rm_pair_control_reset(&voice->gain_q15,
                          &voice->pan_q15,
                          &voice->left_gain_q15,
                          &voice->right_gain_q15,
                          &voice->target_gain_q15,
                          &voice->target_pan_q15,
                          &voice->ramp_frames_remaining);
}

static rm_u32 rm_voice_loudness_score(const rm_voice_state *voice)
{
    return rm_abs_s16_u32(voice->left_gain_q15) + rm_abs_s16_u32(voice->right_gain_q15);
}

static int rm_is_better_steal_candidate(const rm_voice_state *candidate,
                                        const rm_voice_state *current_best)
{
    rm_u32 candidate_loudness;
    rm_u32 best_loudness;

    if (current_best == NULL) {
        return 1;
    }

    if (candidate->priority != current_best->priority) {
        return candidate->priority < current_best->priority;
    }

    candidate_loudness = rm_voice_loudness_score(candidate);
    best_loudness = rm_voice_loudness_score(current_best);
    if (candidate_loudness != best_loudness) {
        return candidate_loudness < best_loudness;
    }

    return candidate->start_serial < current_best->start_serial;
}

static rm_result rm_find_voice_slot(rm_engine *engine,
                                    rm_u16 incoming_priority,
                                    rm_u16 incoming_flags,
                                    rm_u32 *out_stolen,
                                    rm_u16 *out_slot)
{
    rm_u16 i;
    const rm_voice_state *best;
    const rm_voice_state *voice;
    rm_u16 best_slot;

    (void)incoming_priority;

    if (out_stolen != NULL) {
        *out_stolen = 0U;
    }
    if (out_slot == NULL) {
        return RM_ERR_INVALID_ARG;
    }

    for (i = 0U; i < engine->max_voices; ++i) {
        if (!engine->voices[i].active) {
            *out_slot = i;
            return RM_OK;
        }
    }

    best = NULL;
    best_slot = 0U;

    for (i = 0U; i < engine->max_voices; ++i) {
        voice = &engine->voices[i];
        if ((voice->flags & (rm_u16)RM_VOICE_FLAG_PROTECTED) != 0U) {
            continue;
        }
        if (rm_is_better_steal_candidate(voice, best)) {
            best = voice;
            best_slot = i;
        }
    }

    if (best == NULL && (incoming_flags & (rm_u16)RM_VOICE_FLAG_PROTECTED) != 0U) {
        for (i = 0U; i < engine->max_voices; ++i) {
            voice = &engine->voices[i];
            if (rm_is_better_steal_candidate(voice, best)) {
                best = voice;
                best_slot = i;
            }
        }
    }

    if (best == NULL) {
        return RM_ERR_NO_FREE_VOICE;
    }

    if (out_stolen != NULL) {
        *out_stolen = 1U;
    }
    *out_slot = best_slot;
    return RM_OK;
}

static rm_s16 rm_interp_pair_s16(rm_s16 a, rm_s16 b, rm_u16 frac_q12)
{
    rm_s32 mix;

    mix = (rm_s32)a * (rm_s32)(4096U - frac_q12) + (rm_s32)b * (rm_s32)frac_q12;
    mix = rm_shr_round_s32(mix, 12U);
    return rm_clamp_s16(mix);
}

static rm_s16 rm_interp_cubic_s16(rm_s16 y0,
                                  rm_s16 y1,
                                  rm_s16 y2,
                                  rm_s16 y3,
                                  rm_u16 frac_q12)
{
    rm_s32 t;
    rm_s32 t2;
    rm_s32 t3;
    rm_s32 c1_2;
    rm_s32 c2_2;
    rm_s32 c3_2;
    rm_s32 sum2;

    t = (rm_s32)frac_q12;
    t2 = rm_shr_round_s32(t * t, 12U);
    t3 = rm_shr_round_s32(t2 * t, 12U);

    c1_2 = (rm_s32)y2 - (rm_s32)y0;
    c2_2 = ((rm_s32)y0 << 1) - ((rm_s32)y1 * 5L) + ((rm_s32)y2 << 2) - (rm_s32)y3;
    c3_2 = (3L * ((rm_s32)y1 - (rm_s32)y2)) + (rm_s32)y3 - (rm_s32)y0;

    sum2 = ((rm_s32)y1 << 1);
    sum2 += rm_shr_round_s32(c1_2 * t, 12U);
    sum2 += rm_shr_round_s32(c2_2 * t2, 12U);
    sum2 += rm_shr_round_s32(c3_2 * t3, 12U);

    return rm_clamp_s16(rm_shr_round_s32(sum2, 1U));
}

static rm_u32 rm_resolve_buffer_frame_index(const rm_voice_state *voice, rm_s32 frame_index)
{
    rm_s32 loop_start;
    rm_s32 loop_end;
    rm_s32 loop_len;

    if (voice == NULL || voice->frame_count == 0U) {
        return 0U;
    }

    loop_start = (rm_s32)voice->loop_start_frame;
    loop_end = (rm_s32)voice->loop_end_frame;
    if (loop_end > (rm_s32)voice->frame_count) {
        loop_end = (rm_s32)voice->frame_count;
    }

    if ((voice->flags & (rm_u16)RM_VOICE_FLAG_LOOP) != 0U && loop_start < loop_end) {
        loop_len = loop_end - loop_start;
        if (loop_len > 0) {
            while (frame_index < loop_start) {
                frame_index += loop_len;
            }
            while (frame_index >= loop_end) {
                frame_index -= loop_len;
            }
            return (rm_u32)frame_index;
        }
    }

    if (frame_index < 0) {
        return 0U;
    }
    if (frame_index >= (rm_s32)voice->frame_count) {
        return voice->frame_count - 1U;
    }
    return (rm_u32)frame_index;
}

static rm_s16 rm_buffer_sample_at(const rm_voice_state *voice,
                                  rm_s32 frame_index,
                                  rm_u16 channel_index)
{
    rm_u32 resolved_index;
    rm_u32 sample_offset;

    if (voice == NULL || voice->data == NULL || voice->frame_count == 0U) {
        return 0;
    }
    if (channel_index >= (rm_u16)voice->src_channels) {
        channel_index = 0U;
    }

    resolved_index = rm_resolve_buffer_frame_index(voice, frame_index);
    sample_offset = resolved_index * (rm_u32)voice->src_channels + (rm_u32)channel_index;
    return voice->data[sample_offset];
}

static rm_s16 rm_resample_buffer_channel(const rm_voice_state *voice, rm_u16 channel_index)
{
    rm_u16 mode;
    rm_s32 base_index;
    rm_u16 frac_q12;
    rm_s16 y0;
    rm_s16 y1;
    rm_s16 y2;
    rm_s16 y3;

    mode = voice->resampler;
    if (mode == (rm_u16)RM_RESAMPLER_DEFAULT || !rm_valid_resampler(mode)) {
        mode = (rm_u16)RM_RESAMPLER_LINEAR;
    }

    base_index = (rm_s32)voice->pos_frame;
    frac_q12 = voice->pos_frac_q12;

    if (mode == (rm_u16)RM_RESAMPLER_NEAREST) {
        if (frac_q12 >= 2048U) {
            base_index += 1;
        }
        return rm_buffer_sample_at(voice, base_index, channel_index);
    }

    y1 = rm_buffer_sample_at(voice, base_index, channel_index);
    y2 = rm_buffer_sample_at(voice, base_index + 1, channel_index);

    if (mode == (rm_u16)RM_RESAMPLER_CUBIC) {
        y0 = rm_buffer_sample_at(voice, base_index - 1, channel_index);
        y3 = rm_buffer_sample_at(voice, base_index + 2, channel_index);
        return rm_interp_cubic_s16(y0, y1, y2, y3, frac_q12);
    }

    return rm_interp_pair_s16(y1, y2, frac_q12);
}

static rm_s16 rm_resample_stream_channel(const rm_voice_state *voice, rm_u16 channel_index)
{
    rm_s16 y0;
    rm_s16 y1;
    rm_s16 y2;
    rm_s16 y3;
    rm_u16 mode;

    if (channel_index > 1U) {
        channel_index = 0U;
    }

    y0 = voice->stream_prev_frame[channel_index];
    y1 = voice->stream_curr[channel_index];
    y2 = voice->stream_have_next ? voice->stream_next_frame[channel_index] : y1;
    y3 = voice->stream_have_next2 ? voice->stream_next2_frame[channel_index] : y2;
    mode = voice->resampler;
    if (mode == (rm_u16)RM_RESAMPLER_NEAREST) {
        return (voice->pos_frac_q12 >= 2048U) ? y2 : y1;
    }
    if (mode == (rm_u16)RM_RESAMPLER_CUBIC) {
        return rm_interp_cubic_s16(y0, y1, y2, y3, voice->pos_frac_q12);
    }
    return rm_interp_pair_s16(y1, y2, voice->pos_frac_q12);
}

static rm_result rm_stream_pull_frame(rm_voice_state *voice,
                                      rm_s16 *out_frame,
                                      rm_u16 *out_got_frame)
{
    rm_result result;
    rm_u16 eos;

    if (out_got_frame != NULL) {
        *out_got_frame = 0U;
    }

    if (voice == NULL || voice->stream_next == NULL || out_frame == NULL) {
        return RM_ERR_INVALID_ARG;
    }

    eos = 0U;
    out_frame[0] = 0;
    out_frame[1] = 0;
    result = voice->stream_next(voice->stream_user,
                                out_frame,
                                (rm_u16)voice->src_channels,
                                &eos);
    if (result != RM_OK) {
        return result;
    }

    if (eos != 0U) {
        return RM_OK;
    }

    if (voice->src_channels == 1U) {
        out_frame[1] = out_frame[0];
    }

    if (out_got_frame != NULL) {
        *out_got_frame = 1U;
    }
    return RM_OK;
}

static rm_result rm_stream_discard_frames(rm_voice_state *voice, rm_u32 frames)
{
    rm_s16 temp[2];
    rm_u16 got;
    rm_result result;

    while (frames > 0U) {
        result = rm_stream_pull_frame(voice, temp, &got);
        if (result != RM_OK) {
            return result;
        }
        if (got == 0U) {
            return RM_ERR_EMPTY;
        }
        frames--;
    }

    return RM_OK;
}

static rm_result rm_stream_prime_voice(rm_voice_state *voice)
{
    rm_u16 got;
    rm_result result;

    result = rm_stream_pull_frame(voice, voice->stream_curr, &got);
    if (result != RM_OK) {
        return result;
    }
    if (got == 0U) {
        return RM_ERR_EMPTY;
    }

    voice->stream_prev_frame[0] = voice->stream_curr[0];
    voice->stream_prev_frame[1] = voice->stream_curr[1];

    result = rm_stream_pull_frame(voice, voice->stream_next_frame, &got);
    if (result != RM_OK) {
        return result;
    }

    if (got == 0U) {
        voice->stream_next_frame[0] = voice->stream_curr[0];
        voice->stream_next_frame[1] = voice->stream_curr[1];
        voice->stream_next2_frame[0] = voice->stream_next_frame[0];
        voice->stream_next2_frame[1] = voice->stream_next_frame[1];
        voice->stream_have_next = 0U;
        voice->stream_have_next2 = 0U;
        voice->stream_eos = 1U;
        return RM_OK;
    }

    voice->stream_have_next = 1U;
    result = rm_stream_pull_frame(voice, voice->stream_next2_frame, &got);
    if (result != RM_OK) {
        return result;
    }

    if (got == 0U) {
        voice->stream_next2_frame[0] = voice->stream_next_frame[0];
        voice->stream_next2_frame[1] = voice->stream_next_frame[1];
        voice->stream_have_next2 = 0U;
        voice->stream_eos = 1U;
    } else {
        voice->stream_have_next2 = 1U;
        voice->stream_eos = 0U;
    }

    return RM_OK;
}

static int rm_voice_advance_buffer(rm_voice_state *voice)
{
    rm_u32 frame_step;
    rm_u16 frac_step;
    rm_u32 carry;
    rm_u32 loop_end;
    rm_u32 loop_start;
    rm_u32 loop_len;

    frame_step = (rm_u32)(voice->step_q12 >> 12);
    frac_step = (rm_u16)(voice->step_q12 & RM_PHASE_MASK_Q12);

    voice->pos_frame += frame_step;
    carry = (rm_u32)voice->pos_frac_q12 + (rm_u32)frac_step;
    voice->pos_frame += (carry >> 12);
    voice->pos_frac_q12 = (rm_u16)(carry & RM_PHASE_MASK_Q12);

    if ((voice->flags & (rm_u16)RM_VOICE_FLAG_LOOP) != 0U) {
        loop_start = voice->loop_start_frame;
        loop_end = voice->loop_end_frame;
        if (loop_end > voice->frame_count) {
            loop_end = voice->frame_count;
        }
        if (loop_start >= loop_end) {
            loop_start = 0U;
            loop_end = voice->frame_count;
        }
        if (voice->pos_frame >= loop_end) {
            loop_len = loop_end - loop_start;
            if (loop_len == 0U) {
                voice->pos_frame = loop_start;
                voice->pos_frac_q12 = 0U;
            } else {
                while (voice->pos_frame >= loop_end) {
                    voice->pos_frame -= loop_len;
                }
                if (voice->pos_frame < loop_start) {
                    voice->pos_frame = loop_start;
                }
            }
        }
        return 1;
    }

    if (voice->pos_frame >= voice->frame_count) {
        return 0;
    }
    return 1;
}

static int rm_voice_advance_stream(rm_voice_state *voice)
{
    rm_u32 frame_step;
    rm_u16 frac_step;
    rm_u32 carry;
    rm_u32 shifts;
    rm_s16 fetched[2];
    rm_u16 got;
    rm_result result;

    frame_step = (rm_u32)(voice->step_q12 >> 12);
    frac_step = (rm_u16)(voice->step_q12 & RM_PHASE_MASK_Q12);

    voice->pos_frame += frame_step;
    carry = (rm_u32)voice->pos_frac_q12 + (rm_u32)frac_step;
    voice->pos_frame += (carry >> 12);
    voice->pos_frac_q12 = (rm_u16)(carry & RM_PHASE_MASK_Q12);

    shifts = voice->pos_frame;
    while (shifts > 0U) {
        if (voice->stream_have_next == 0U) {
            return 0;
        }

        voice->stream_prev_frame[0] = voice->stream_curr[0];
        voice->stream_prev_frame[1] = voice->stream_curr[1];
        voice->stream_curr[0] = voice->stream_next_frame[0];
        voice->stream_curr[1] = voice->stream_next_frame[1];

        if (voice->stream_have_next2 != 0U) {
            voice->stream_next_frame[0] = voice->stream_next2_frame[0];
            voice->stream_next_frame[1] = voice->stream_next2_frame[1];
            voice->stream_have_next = 1U;

            result = rm_stream_pull_frame(voice, fetched, &got);
            if (result != RM_OK || got == 0U) {
                voice->stream_next2_frame[0] = voice->stream_next_frame[0];
                voice->stream_next2_frame[1] = voice->stream_next_frame[1];
                voice->stream_have_next2 = 0U;
                voice->stream_eos = 1U;
            } else {
                voice->stream_next2_frame[0] = fetched[0];
                voice->stream_next2_frame[1] = fetched[1];
                voice->stream_have_next2 = 1U;
                voice->stream_eos = 0U;
            }
        } else {
            voice->stream_next_frame[0] = voice->stream_curr[0];
            voice->stream_next_frame[1] = voice->stream_curr[1];
            voice->stream_next2_frame[0] = voice->stream_next_frame[0];
            voice->stream_next2_frame[1] = voice->stream_next_frame[1];
            voice->stream_have_next = 0U;
            voice->stream_have_next2 = 0U;
            voice->stream_eos = 1U;
        }

        shifts--;
    }

    voice->pos_frame = 0U;
    return 1;
}

static int rm_voice_advance(rm_voice_state *voice)
{
    if (voice->source_kind == RM_SOURCE_KIND_STREAM) {
        return rm_voice_advance_stream(voice);
    }
    return rm_voice_advance_buffer(voice);
}

static void rm_capture_push_frame(rm_engine *engine, rm_s16 left, rm_s16 right)
{
    rm_u16 base;

    if (!engine->capture_enabled) {
        return;
    }

    if (engine->capture_count_frames == (rm_u16)RAWMIX_CAPTURE_RING_FRAMES) {
        engine->capture_read_frame = (rm_u16)((engine->capture_read_frame + 1U) % (rm_u16)RAWMIX_CAPTURE_RING_FRAMES);
        engine->capture_count_frames--;
        engine->stats.capture_frames_dropped++;
    }

    base = (rm_u16)(engine->capture_write_frame * 2U);
    engine->capture_ring[base] = left;
    engine->capture_ring[base + 1U] = right;
    engine->capture_write_frame = (rm_u16)((engine->capture_write_frame + 1U) % (rm_u16)RAWMIX_CAPTURE_RING_FRAMES);
    engine->capture_count_frames++;
    engine->stats.capture_frames_pushed++;
}

static void rm_mix_input_frame(const rm_engine *engine,
                               const rm_s16 *input,
                               rm_u16 input_channels,
                               rm_s32 *io_left,
                               rm_s32 *io_right,
                               rm_s16 *out_capture_left,
                               rm_s16 *out_capture_right)
{
    rm_s16 l;
    rm_s16 r;
    rm_s16 mono;

    l = 0;
    r = 0;
    mono = 0;

    if (input != NULL) {
        if (input_channels == 1U) {
            l = input[0];
            r = input[0];
        } else if (input_channels >= 2U) {
            l = input[0];
            r = input[1];
        }
    }

    *out_capture_left = l;
    *out_capture_right = r;

    if (engine->channels == 1U) {
        mono = (rm_s16)(((rm_s32)l + (rm_s32)r) / 2);
        *io_left += (rm_s32)rm_mul_q15(mono, engine->monitor_gain_q15);
    } else {
        *io_left += (rm_s32)rm_mul_q15(l, engine->monitor_left_gain_q15);
        *io_right += (rm_s32)rm_mul_q15(r, engine->monitor_right_gain_q15);
    }
}

static void rm_mix_voice_frame(rm_voice_state *voice,
                               const rm_group_state *group,
                               rm_u16 engine_channels,
                               rm_s32 *io_left,
                               rm_s32 *io_right)
{
    rm_s16 sample_mono;
    rm_s16 sample_left;
    rm_s16 sample_right;
    rm_s16 group_gain_q15;
    rm_s16 effective_left_gain;
    rm_s16 effective_right_gain;

    if (!voice->active) {
        return;
    }

    sample_mono = 0;
    sample_left = 0;
    sample_right = 0;

    if (voice->source_kind == RM_SOURCE_KIND_BUFFER) {
        if (voice->data == NULL || voice->frame_count == 0U) {
            return;
        }

        if (voice->pos_frame >= voice->frame_count) {
            voice->active = 0U;
            return;
        }

        if (voice->src_channels == 1U) {
            sample_mono = rm_resample_buffer_channel(voice, 0U);
        } else {
            sample_left = rm_resample_buffer_channel(voice, 0U);
            sample_right = rm_resample_buffer_channel(voice, 1U);
        }
    } else if (voice->source_kind == RM_SOURCE_KIND_STREAM) {
        if (voice->src_channels == 1U) {
            sample_mono = rm_resample_stream_channel(voice, 0U);
        } else {
            sample_left = rm_resample_stream_channel(voice, 0U);
            sample_right = rm_resample_stream_channel(voice, 1U);
        }
    } else {
        return;
    }

    if (engine_channels == 1U) {
        if (voice->src_channels == 2U) {
            sample_mono = (rm_s16)(((rm_s32)sample_left + (rm_s32)sample_right) / 2);
        }
        group_gain_q15 = group != NULL ? group->gain_q15 : RM_Q15_ONE;
        *io_left += (rm_s32)rm_mul_q15(sample_mono, rm_mul_q15(voice->gain_q15, group_gain_q15));
        return;
    }

    effective_left_gain = voice->left_gain_q15;
    effective_right_gain = voice->right_gain_q15;
    if (group != NULL) {
        effective_left_gain = rm_mul_q15(effective_left_gain, group->left_gain_q15);
        effective_right_gain = rm_mul_q15(effective_right_gain, group->right_gain_q15);
    }

    if (voice->src_channels == 1U) {
        *io_left += (rm_s32)rm_mul_q15(sample_mono, effective_left_gain);
        *io_right += (rm_s32)rm_mul_q15(sample_mono, effective_right_gain);
    } else {
        *io_left += (rm_s32)rm_mul_q15(sample_left, effective_left_gain);
        *io_right += (rm_s32)rm_mul_q15(sample_right, effective_right_gain);
    }
}

static void rm_step_all_ramps(rm_engine *engine)
{
    rm_u16 i;

    for (i = 0U; i < (rm_u16)RAWMIX_MAX_BUSES; ++i) {
        rm_pair_control_step(&engine->buses[i].gain_q15,
                             &engine->buses[i].pan_q15,
                             &engine->buses[i].left_gain_q15,
                             &engine->buses[i].right_gain_q15,
                             &engine->buses[i].target_gain_q15,
                             &engine->buses[i].target_pan_q15,
                             &engine->buses[i].ramp_frames_remaining);
    }

    for (i = 0U; i < (rm_u16)RAWMIX_MAX_GROUPS; ++i) {
        rm_pair_control_step(&engine->groups[i].gain_q15,
                             &engine->groups[i].pan_q15,
                             &engine->groups[i].left_gain_q15,
                             &engine->groups[i].right_gain_q15,
                             &engine->groups[i].target_gain_q15,
                             &engine->groups[i].target_pan_q15,
                             &engine->groups[i].ramp_frames_remaining);
    }
}

static void rm_reset_block_meters(rm_engine *engine)
{
    rm_u16 i;

    engine->master_meter.peak_left_q15 = RM_Q15_ZERO;
    engine->master_meter.peak_right_q15 = RM_Q15_ZERO;
    engine->master_meter.clip_events = 0U;
    engine->master_meter.gain_reduction_q15 = RM_Q15_ZERO;

    for (i = 0U; i < (rm_u16)RAWMIX_MAX_BUSES; ++i) {
        engine->buses[i].meter.peak_left_q15 = RM_Q15_ZERO;
        engine->buses[i].meter.peak_right_q15 = RM_Q15_ZERO;
        engine->buses[i].meter.clip_events = 0U;
    }
}

static void rm_apply_single_automation_event(rm_engine *engine,
                                             const rm_automation_event *event_desc)
{
    rm_voice_state *voice;
    int applied;

    if (engine == NULL || event_desc == NULL) {
        return;
    }

    applied = 1;
    switch (event_desc->type) {
        case RM_AUTOMATION_MASTER_GAIN:
            engine->master_gain_q15 = event_desc->value0_q15;
            break;

        case RM_AUTOMATION_HEADROOM:
            engine->headroom_q15 = event_desc->value0_q15;
            break;

        case RM_AUTOMATION_BUS_SET:
            if (!rm_valid_bus_id(event_desc->target_id)) {
                applied = 0;
                break;
            }
            if (event_desc->value3_u32 == 0U) {
                rm_pair_control_set(&engine->buses[event_desc->target_id].gain_q15,
                                    &engine->buses[event_desc->target_id].pan_q15,
                                    &engine->buses[event_desc->target_id].left_gain_q15,
                                    &engine->buses[event_desc->target_id].right_gain_q15,
                                    &engine->buses[event_desc->target_id].target_gain_q15,
                                    &engine->buses[event_desc->target_id].target_pan_q15,
                                    &engine->buses[event_desc->target_id].ramp_frames_remaining,
                                    event_desc->value0_q15,
                                    event_desc->value1_q15);
            } else {
                rm_pair_control_ramp(&engine->buses[event_desc->target_id].gain_q15,
                                     &engine->buses[event_desc->target_id].pan_q15,
                                     &engine->buses[event_desc->target_id].left_gain_q15,
                                     &engine->buses[event_desc->target_id].right_gain_q15,
                                     &engine->buses[event_desc->target_id].target_gain_q15,
                                     &engine->buses[event_desc->target_id].target_pan_q15,
                                     &engine->buses[event_desc->target_id].ramp_frames_remaining,
                                     event_desc->value0_q15,
                                     event_desc->value1_q15,
                                     event_desc->value3_u32);
            }
            break;

        case RM_AUTOMATION_GROUP_SET:
            if (!rm_valid_group_id(event_desc->target_id)) {
                applied = 0;
                break;
            }
            if (event_desc->value3_u32 == 0U) {
                rm_pair_control_set(&engine->groups[event_desc->target_id].gain_q15,
                                    &engine->groups[event_desc->target_id].pan_q15,
                                    &engine->groups[event_desc->target_id].left_gain_q15,
                                    &engine->groups[event_desc->target_id].right_gain_q15,
                                    &engine->groups[event_desc->target_id].target_gain_q15,
                                    &engine->groups[event_desc->target_id].target_pan_q15,
                                    &engine->groups[event_desc->target_id].ramp_frames_remaining,
                                    event_desc->value0_q15,
                                    event_desc->value1_q15);
            } else {
                rm_pair_control_ramp(&engine->groups[event_desc->target_id].gain_q15,
                                     &engine->groups[event_desc->target_id].pan_q15,
                                     &engine->groups[event_desc->target_id].left_gain_q15,
                                     &engine->groups[event_desc->target_id].right_gain_q15,
                                     &engine->groups[event_desc->target_id].target_gain_q15,
                                     &engine->groups[event_desc->target_id].target_pan_q15,
                                     &engine->groups[event_desc->target_id].ramp_frames_remaining,
                                     event_desc->value0_q15,
                                     event_desc->value1_q15,
                                     event_desc->value3_u32);
            }
            break;

        case RM_AUTOMATION_VOICE_SET:
            if (rm_validate_handle(engine, event_desc->handle, &voice) != RM_OK) {
                applied = 0;
                break;
            }
            voice->stop_when_silent = 0U;
            if (event_desc->value3_u32 == 0U) {
                rm_pair_control_set(&voice->gain_q15,
                                    &voice->pan_q15,
                                    &voice->left_gain_q15,
                                    &voice->right_gain_q15,
                                    &voice->target_gain_q15,
                                    &voice->target_pan_q15,
                                    &voice->ramp_frames_remaining,
                                    event_desc->value0_q15,
                                    event_desc->value1_q15);
            } else {
                rm_pair_control_ramp(&voice->gain_q15,
                                     &voice->pan_q15,
                                     &voice->left_gain_q15,
                                     &voice->right_gain_q15,
                                     &voice->target_gain_q15,
                                     &voice->target_pan_q15,
                                     &voice->ramp_frames_remaining,
                                     event_desc->value0_q15,
                                     event_desc->value1_q15,
                                     event_desc->value3_u32);
            }
            break;

        case RM_AUTOMATION_BUS_MUTE:
            if (!rm_valid_bus_id(event_desc->target_id)) {
                applied = 0;
                break;
            }
            engine->buses[event_desc->target_id].mute = event_desc->value2_u16 ? 1U : 0U;
            break;

        case RM_AUTOMATION_BUS_SOLO:
            if (!rm_valid_bus_id(event_desc->target_id)) {
                applied = 0;
                break;
            }
            engine->buses[event_desc->target_id].solo = event_desc->value2_u16 ? 1U : 0U;
            break;

        case RM_AUTOMATION_GROUP_MUTE:
            if (!rm_valid_group_id(event_desc->target_id)) {
                applied = 0;
                break;
            }
            engine->groups[event_desc->target_id].mute = event_desc->value2_u16 ? 1U : 0U;
            break;

        case RM_AUTOMATION_GROUP_SOLO:
            if (!rm_valid_group_id(event_desc->target_id)) {
                applied = 0;
                break;
            }
            engine->groups[event_desc->target_id].solo = event_desc->value2_u16 ? 1U : 0U;
            break;

        case RM_AUTOMATION_BUS_SEND:
            if (!rm_valid_bus_id(event_desc->target_id) ||
                !rm_valid_bus_id(event_desc->aux_id) ||
                event_desc->aux_id <= event_desc->target_id ||
                !rm_valid_send_mode(event_desc->value2_u16)) {
                applied = 0;
                break;
            }
            engine->sends[event_desc->target_id][event_desc->aux_id].active =
                event_desc->value0_q15 == RM_Q15_ZERO ? 0U : 1U;
            engine->sends[event_desc->target_id][event_desc->aux_id].mode =
                (rm_u8)event_desc->value2_u16;
            engine->sends[event_desc->target_id][event_desc->aux_id].gain_q15 =
                event_desc->value0_q15;
            break;

        default:
            applied = 0;
            break;
    }

    if (applied) {
        engine->stats.automation_events_applied++;
    }
}

static void rm_apply_automation_for_offset(rm_engine *engine, rm_u16 sample_offset)
{
    rm_u16 i;

    for (i = 0U; i < engine->automation_count; ++i) {
        if (engine->automation_queue[i].sample_offset == sample_offset) {
            rm_apply_single_automation_event(engine, &engine->automation_queue[i]);
        }
    }
}

static void rm_consume_automation_events(rm_engine *engine, rm_u32 frames)
{
    rm_u16 read_i;
    rm_u16 write_i;

    write_i = 0U;
    for (read_i = 0U; read_i < engine->automation_count; ++read_i) {
        if (engine->automation_queue[read_i].sample_offset < frames) {
            continue;
        }
        if (write_i != read_i) {
            engine->automation_queue[write_i] = engine->automation_queue[read_i];
        }
        engine->automation_queue[write_i].sample_offset =
            (rm_u16)(engine->automation_queue[write_i].sample_offset - frames);
        write_i++;
    }
    engine->automation_count = write_i;
}

static void rm_process_frames(rm_engine *engine,
                              const rm_s16 *input_interleaved,
                              rm_u16 input_channels,
                              rm_s16 *output_interleaved,
                              rm_u16 output_channels,
                              rm_u32 frames,
                              int duplex_mode)
{
    rm_u32 f;
    rm_u16 i;
    rm_u16 j;
    rm_s32 mix_left;
    rm_s32 mix_right;
    rm_s16 cap_left;
    rm_s16 cap_right;
    const rm_s16 *in_frame;
    rm_s16 out_left;
    rm_s16 out_right;
    rm_s32 bus_mix_left[RAWMIX_MAX_BUSES];
    rm_s32 bus_mix_right[RAWMIX_MAX_BUSES];
    rm_voice_state *voice;
    rm_u16 bus_id;
    const rm_group_state *group;
    rm_s32 mono_out;
    rm_s32 bus_signal_left;
    rm_s32 bus_signal_right;
    rm_s32 bus_output_left;
    rm_s32 bus_output_right;
    rm_s32 send_left;
    rm_s32 send_right;

    rm_reset_block_meters(engine);

    for (f = 0U; f < frames; ++f) {
        rm_apply_automation_for_offset(engine, (rm_u16)f);
        rm_step_all_ramps(engine);

        for (i = 0U; i < (rm_u16)RAWMIX_MAX_BUSES; ++i) {
            bus_mix_left[i] = 0;
            bus_mix_right[i] = 0;
        }

        mix_left = 0;
        mix_right = 0;
        cap_left = 0;
        cap_right = 0;

        in_frame = NULL;
        if (duplex_mode && input_interleaved != NULL) {
            in_frame = &input_interleaved[f * (rm_u32)input_channels];
            rm_mix_input_frame(engine,
                               in_frame,
                               input_channels,
                               &mix_left,
                               &mix_right,
                               &cap_left,
                               &cap_right);
            rm_capture_push_frame(engine, cap_left, cap_right);
        }

        for (i = 0U; i < engine->max_voices; ++i) {
            voice = &engine->voices[i];
            if (!voice->active) {
                continue;
            }

            if (voice->start_delay_frames > 0U) {
                voice->start_delay_frames--;
                continue;
            }

            rm_pair_control_step(&voice->gain_q15,
                                 &voice->pan_q15,
                                 &voice->left_gain_q15,
                                 &voice->right_gain_q15,
                                 &voice->target_gain_q15,
                                 &voice->target_pan_q15,
                                 &voice->ramp_frames_remaining);

            if (voice->stop_when_silent &&
                voice->gain_q15 == RM_Q15_ZERO &&
                voice->ramp_frames_remaining == 0U) {
                rm_clear_voice(voice);
                voice->generation++;
                continue;
            }

            bus_id = voice->bus_id;
            if (!rm_valid_bus_id(bus_id)) {
                bus_id = RM_BUS_DEFAULT;
            }
            group = rm_valid_group_id(voice->group_id) ? &engine->groups[voice->group_id] : &engine->groups[RM_GROUP_DEFAULT];

            if (voice->resampler == (rm_u16)RM_RESAMPLER_CUBIC) {
                engine->stats.resampler_hq_frames++;
            }

            if (rm_group_allows_audio(engine, voice->group_id)) {
                rm_mix_voice_frame(voice,
                                   group,
                                   engine->channels,
                                   &bus_mix_left[bus_id],
                                   &bus_mix_right[bus_id]);
            }

            if (!rm_voice_advance(voice)) {
                rm_clear_voice(voice);
                voice->generation++;
                engine->stats.voices_finished++;
            }
        }

        for (i = 0U; i < (rm_u16)RAWMIX_MAX_BUSES; ++i) {
            if (!engine->buses[i].active) {
                continue;
            }

            bus_signal_left = bus_mix_left[i];
            bus_signal_right = bus_mix_right[i];

            if (rm_apply_bus_fx_chain(&engine->buses[i],
                                      engine->channels,
                                      &bus_signal_left,
                                      &bus_signal_right)) {
                engine->stats.bus_fx_frames++;
            }

            for (j = (rm_u16)(i + 1U); j < (rm_u16)RAWMIX_MAX_BUSES; ++j) {
                if (!engine->sends[i][j].active || engine->sends[i][j].mode != (rm_u8)RM_SEND_PRE_FADER) {
                    continue;
                }
                send_left = rm_mul_s32_q15(bus_signal_left, engine->sends[i][j].gain_q15);
                send_right = engine->channels == 2U ? rm_mul_s32_q15(bus_signal_right, engine->sends[i][j].gain_q15) : 0;
                bus_mix_left[j] += send_left;
                if (engine->channels == 2U) {
                    bus_mix_right[j] += send_right;
                }
                engine->stats.send_frames++;
            }

            bus_output_left = 0;
            bus_output_right = 0;
            if (rm_bus_allows_output(engine, i)) {
                if (engine->channels == 1U) {
                    bus_output_left = rm_mul_s32_q15(bus_signal_left, engine->buses[i].gain_q15);
                } else {
                    bus_output_left = rm_mul_s32_q15(bus_signal_left, engine->buses[i].left_gain_q15);
                    bus_output_right = rm_mul_s32_q15(bus_signal_right, engine->buses[i].right_gain_q15);
                }
            }

            if (engine->channels == 1U) {
                rm_update_meter_pair(&engine->buses[i].meter, bus_output_left, bus_output_left);
            } else {
                rm_update_meter_pair(&engine->buses[i].meter, bus_output_left, bus_output_right);
            }

            for (j = (rm_u16)(i + 1U); j < (rm_u16)RAWMIX_MAX_BUSES; ++j) {
                if (!engine->sends[i][j].active || engine->sends[i][j].mode != (rm_u8)RM_SEND_POST_FADER) {
                    continue;
                }
                send_left = rm_mul_s32_q15(bus_output_left, engine->sends[i][j].gain_q15);
                send_right = engine->channels == 2U ? rm_mul_s32_q15(bus_output_right, engine->sends[i][j].gain_q15) : 0;
                bus_mix_left[j] += send_left;
                if (engine->channels == 2U) {
                    bus_mix_right[j] += send_right;
                }
                engine->stats.send_frames++;
            }

            mix_left += bus_output_left;
            if (engine->channels == 2U) {
                mix_right += bus_output_right;
            }
        }

        mix_left = rm_mul_s32_q15(mix_left, engine->master_gain_q15);
        mix_left = rm_mul_s32_q15(mix_left, engine->headroom_q15);

        if (engine->channels == 2U) {
            mix_right = rm_mul_s32_q15(mix_right, engine->master_gain_q15);
            mix_right = rm_mul_s32_q15(mix_right, engine->headroom_q15);
        }

        rm_apply_limiter(engine,
                         &mix_left,
                         engine->channels == 2U ? &mix_right : NULL);

        if (mix_left > 32767 || mix_left < -32768) {
            engine->stats.clipping_events++;
        }
        if (engine->channels == 2U && (mix_right > 32767 || mix_right < -32768)) {
            engine->stats.clipping_events++;
        }

        if (engine->channels == 1U) {
            rm_update_meter_pair(&engine->master_meter, mix_left, mix_left);
        } else {
            rm_update_meter_pair(&engine->master_meter, mix_left, mix_right);
        }

        out_left = rm_clamp_s16(mix_left);
        out_right = rm_clamp_s16(mix_right);

        if (output_interleaved != NULL) {
            if (output_channels == 1U) {
                if (engine->channels == 1U) {
                    output_interleaved[f] = out_left;
                } else {
                    mono_out = ((rm_s32)out_left + (rm_s32)out_right) / 2;
                    output_interleaved[f] = rm_clamp_s16(mono_out);
                }
            } else {
                output_interleaved[f * (rm_u32)output_channels] = out_left;
                output_interleaved[f * (rm_u32)output_channels + 1U] = (engine->channels == 2U) ? out_right : out_left;
            }
        }
    }

    rm_consume_automation_events(engine, frames);
}


void rm_engine_config_init(rm_engine_config *cfg)
{
    if (cfg == NULL) {
        return;
    }

    cfg->sample_rate = 48000U;
    cfg->channels = 2U;
    cfg->max_voices = RAWMIX_MAX_VOICES;
    cfg->master_gain_q15 = RM_Q15_ONE;
    cfg->headroom_q15 = RM_Q15_ONE;
    cfg->monitor_gain_q15 = RM_Q15_ZERO;
    cfg->monitor_pan_q15 = RM_PAN_CENTER;
    cfg->capture_enabled = 1U;
    cfg->default_resampler = (rm_u16)RM_RESAMPLER_LINEAR;
}

void rm_voice_params_init(rm_voice_params *params)
{
    if (params == NULL) {
        return;
    }
    params->gain_q15 = RM_Q15_ONE;
    params->pan_q15 = RM_PAN_CENTER;
    params->pitch_q12 = RM_RATIO_ONE_Q12;
    params->flags = (rm_u16)RM_VOICE_FLAG_NONE;
    params->priority = 128U;
    params->bus_id = RM_BUS_DEFAULT;
    params->group_id = RM_GROUP_DEFAULT;
    params->resampler = (rm_u16)RM_RESAMPLER_DEFAULT;
    params->loop_start_frame = 0U;
    params->loop_end_frame = 0U;
    params->start_delay_frames = 0U;
    params->start_frame_offset = 0U;
    params->fade_in_frames = 0U;
}

rm_result rm_engine_init(rm_engine *engine, const rm_engine_config *cfg)
{
    rm_u16 i;
    rm_u16 j;
    rm_engine_config local_cfg;

    if (engine == NULL) {
        return RM_ERR_INVALID_ARG;
    }

    if (cfg == NULL) {
        rm_engine_config_init(&local_cfg);
        cfg = &local_cfg;
    }

    if (cfg->sample_rate == 0U) {
        return RM_ERR_INVALID_ARG;
    }
    if (!rm_valid_channels(cfg->channels)) {
        return RM_ERR_UNSUPPORTED;
    }
    if (cfg->max_voices == 0U || cfg->max_voices > RAWMIX_MAX_VOICES) {
        return RM_ERR_INVALID_ARG;
    }
    if (!rm_valid_resampler(cfg->default_resampler)) {
        return RM_ERR_INVALID_ARG;
    }

    memset(engine, 0, sizeof(*engine));
    engine->sample_rate = cfg->sample_rate;
    engine->channels = cfg->channels;
    engine->max_voices = cfg->max_voices;
    engine->master_gain_q15 = cfg->master_gain_q15;
    engine->headroom_q15 = cfg->headroom_q15;
    engine->monitor_gain_q15 = cfg->monitor_gain_q15;
    engine->monitor_pan_q15 = cfg->monitor_pan_q15;
    engine->capture_enabled = cfg->capture_enabled ? 1U : 0U;
    engine->default_resampler = rm_resolve_resampler(cfg->default_resampler,
                                                     (rm_u16)RM_RESAMPLER_LINEAR);
    rm_update_pair_gains(engine->monitor_gain_q15,
                         engine->monitor_pan_q15,
                         &engine->monitor_left_gain_q15,
                         &engine->monitor_right_gain_q15);
    rm_reset_meter_state(&engine->master_meter);
    rm_reset_limiter_state(&engine->limiter);
    engine->automation_count = 0U;

    for (i = 0U; i < (rm_u16)RAWMIX_MAX_BUSES; ++i) {
        rm_reset_bus_state(&engine->buses[i]);
        for (j = 0U; j < (rm_u16)RAWMIX_MAX_BUSES; ++j) {
            rm_reset_bus_send_state(&engine->sends[i][j]);
        }
    }
    for (i = 0U; i < (rm_u16)RAWMIX_MAX_GROUPS; ++i) {
        rm_reset_group_state(&engine->groups[i]);
    }
    for (i = 0U; i < engine->max_voices; ++i) {
        engine->voices[i].generation = 1U;
        rm_clear_voice(&engine->voices[i]);
    }

    return RM_OK;
}


void rm_engine_reset(rm_engine *engine)
{
    rm_engine_config cfg;
    rm_u8 limiter_active;
    rm_s16 limiter_threshold_q15;
    rm_u16 limiter_attack_frames;
    rm_u16 limiter_release_frames;
    rm_s16 limiter_output_gain_q15;
    rm_u16 limiter_lookahead_frames;

    if (engine == NULL) {
        return;
    }

    limiter_active = engine->limiter.active;
    limiter_threshold_q15 = engine->limiter.threshold_q15;
    limiter_attack_frames = engine->limiter.attack_frames;
    limiter_release_frames = engine->limiter.release_frames;
    limiter_output_gain_q15 = engine->limiter.output_gain_q15;
    limiter_lookahead_frames = engine->limiter.lookahead_frames;

    cfg.sample_rate = engine->sample_rate;
    cfg.channels = engine->channels;
    cfg.max_voices = engine->max_voices;
    cfg.master_gain_q15 = engine->master_gain_q15;
    cfg.headroom_q15 = engine->headroom_q15;
    cfg.monitor_gain_q15 = engine->monitor_gain_q15;
    cfg.monitor_pan_q15 = engine->monitor_pan_q15;
    cfg.capture_enabled = engine->capture_enabled;
    cfg.default_resampler = engine->default_resampler;
    rm_engine_init(engine, &cfg);
    if (limiter_active) {
        rm_engine_set_limiter_ex(engine,
                                 limiter_threshold_q15,
                                 limiter_attack_frames,
                                 limiter_release_frames,
                                 limiter_output_gain_q15,
                                 limiter_lookahead_frames);
    }
}

void rm_engine_get_stats(const rm_engine *engine, rm_engine_stats *out_stats)
{
    if (engine == NULL || out_stats == NULL) {
        return;
    }
    *out_stats = engine->stats;
}

rm_result rm_engine_set_master_gain(rm_engine *engine, rm_s16 gain_q15)
{
    if (engine == NULL) {
        return RM_ERR_INVALID_ARG;
    }
    engine->master_gain_q15 = gain_q15;
    return RM_OK;
}

rm_result rm_engine_set_default_resampler(rm_engine *engine, rm_u16 resampler)
{
    if (engine == NULL || !rm_valid_resampler(resampler)) {
        return RM_ERR_INVALID_ARG;
    }

    engine->default_resampler = rm_resolve_resampler(resampler,
                                                     (rm_u16)RM_RESAMPLER_LINEAR);
    return RM_OK;
}

rm_result rm_engine_set_headroom(rm_engine *engine, rm_s16 gain_q15)
{
    if (engine == NULL) {
        return RM_ERR_INVALID_ARG;
    }
    engine->headroom_q15 = gain_q15;
    return RM_OK;
}

rm_result rm_engine_set_monitor(rm_engine *engine, rm_s16 gain_q15, rm_s16 pan_q15)
{
    if (engine == NULL) {
        return RM_ERR_INVALID_ARG;
    }
    engine->monitor_gain_q15 = gain_q15;
    engine->monitor_pan_q15 = pan_q15;
    rm_update_pair_gains(engine->monitor_gain_q15,
                         engine->monitor_pan_q15,
                         &engine->monitor_left_gain_q15,
                         &engine->monitor_right_gain_q15);
    return RM_OK;
}

rm_result rm_engine_set_limiter(rm_engine *engine,
                                rm_s16 threshold_q15,
                                rm_u16 attack_frames,
                                rm_u16 release_frames,
                                rm_s16 output_gain_q15)
{
    return rm_engine_set_limiter_ex(engine,
                                    threshold_q15,
                                    attack_frames,
                                    release_frames,
                                    output_gain_q15,
                                    0U);
}

rm_result rm_engine_set_limiter_ex(rm_engine *engine,
                                   rm_s16 threshold_q15,
                                   rm_u16 attack_frames,
                                   rm_u16 release_frames,
                                   rm_s16 output_gain_q15,
                                   rm_u16 lookahead_frames)
{
    rm_s32 threshold;

    if (engine == NULL) {
        return RM_ERR_INVALID_ARG;
    }

    threshold = threshold_q15;
    if (threshold < 0) {
        threshold = -threshold;
    }
    if (threshold == 0) {
        return RM_ERR_INVALID_ARG;
    }
    if (lookahead_frames > (rm_u16)RAWMIX_LIMITER_MAX_LOOKAHEAD_FRAMES) {
        return RM_ERR_INVALID_ARG;
    }

    rm_reset_limiter_state(&engine->limiter);
    engine->limiter.active = 1U;
    engine->limiter.threshold_q15 = rm_clamp_s16(threshold);
    engine->limiter.attack_frames = attack_frames == 0U ? 1U : attack_frames;
    engine->limiter.release_frames = release_frames == 0U ? 1U : release_frames;
    engine->limiter.output_gain_q15 = output_gain_q15;
    engine->limiter.lookahead_frames = lookahead_frames;
    engine->limiter.current_gain_q15 = RM_Q15_ONE;
    return RM_OK;
}

rm_result rm_engine_clear_limiter(rm_engine *engine)
{
    if (engine == NULL) {
        return RM_ERR_INVALID_ARG;
    }

    rm_reset_limiter_state(&engine->limiter);
    return RM_OK;
}

rm_u16 rm_engine_get_latency_frames(const rm_engine *engine)
{
    if (engine == NULL || !engine->limiter.active) {
        return 0U;
    }
    return engine->limiter.lookahead_frames;
}

rm_result rm_engine_get_bus_meter(const rm_engine *engine, rm_u16 bus_id, rm_meter_state *out_meter)
{
    if (engine == NULL || out_meter == NULL || !rm_valid_bus_id(bus_id)) {
        return RM_ERR_INVALID_ARG;
    }
    *out_meter = engine->buses[bus_id].meter;
    return RM_OK;
}

rm_result rm_engine_get_master_meter(const rm_engine *engine, rm_meter_state *out_meter)
{
    if (engine == NULL || out_meter == NULL) {
        return RM_ERR_INVALID_ARG;
    }
    *out_meter = engine->master_meter;
    return RM_OK;
}

rm_result rm_engine_set_bus(rm_engine *engine, rm_u16 bus_id, rm_s16 gain_q15, rm_s16 pan_q15)
{
    if (engine == NULL || !rm_valid_bus_id(bus_id)) {
        return RM_ERR_INVALID_ARG;
    }

    rm_pair_control_set(&engine->buses[bus_id].gain_q15,
                        &engine->buses[bus_id].pan_q15,
                        &engine->buses[bus_id].left_gain_q15,
                        &engine->buses[bus_id].right_gain_q15,
                        &engine->buses[bus_id].target_gain_q15,
                        &engine->buses[bus_id].target_pan_q15,
                        &engine->buses[bus_id].ramp_frames_remaining,
                        gain_q15,
                        pan_q15);
    return RM_OK;
}

rm_result rm_engine_ramp_bus(rm_engine *engine,
                             rm_u16 bus_id,
                             rm_s16 gain_q15,
                             rm_s16 pan_q15,
                             rm_u32 frames)
{
    if (engine == NULL || !rm_valid_bus_id(bus_id)) {
        return RM_ERR_INVALID_ARG;
    }

    rm_pair_control_ramp(&engine->buses[bus_id].gain_q15,
                         &engine->buses[bus_id].pan_q15,
                         &engine->buses[bus_id].left_gain_q15,
                         &engine->buses[bus_id].right_gain_q15,
                         &engine->buses[bus_id].target_gain_q15,
                         &engine->buses[bus_id].target_pan_q15,
                         &engine->buses[bus_id].ramp_frames_remaining,
                         gain_q15,
                         pan_q15,
                         frames);
    return RM_OK;
}

rm_result rm_engine_set_bus_mute(rm_engine *engine, rm_u16 bus_id, rm_u16 mute_on)
{
    if (engine == NULL || !rm_valid_bus_id(bus_id)) {
        return RM_ERR_INVALID_ARG;
    }
    engine->buses[bus_id].mute = mute_on ? 1U : 0U;
    return RM_OK;
}

rm_result rm_engine_set_bus_solo(rm_engine *engine, rm_u16 bus_id, rm_u16 solo_on)
{
    if (engine == NULL || !rm_valid_bus_id(bus_id)) {
        return RM_ERR_INVALID_ARG;
    }
    engine->buses[bus_id].solo = solo_on ? 1U : 0U;
    return RM_OK;
}

rm_result rm_engine_set_bus_send(rm_engine *engine,
                                 rm_u16 src_bus_id,
                                 rm_u16 dst_bus_id,
                                 rm_s16 gain_q15,
                                 rm_u16 mode)
{
    if (engine == NULL || !rm_valid_bus_id(src_bus_id) || !rm_valid_bus_id(dst_bus_id)) {
        return RM_ERR_INVALID_ARG;
    }
    if (dst_bus_id <= src_bus_id || !rm_valid_send_mode(mode)) {
        return RM_ERR_UNSUPPORTED;
    }

    engine->sends[src_bus_id][dst_bus_id].active = gain_q15 == RM_Q15_ZERO ? 0U : 1U;
    engine->sends[src_bus_id][dst_bus_id].mode = (rm_u8)mode;
    engine->sends[src_bus_id][dst_bus_id].gain_q15 = gain_q15;
    return RM_OK;
}

rm_result rm_engine_clear_bus_send(rm_engine *engine, rm_u16 src_bus_id, rm_u16 dst_bus_id)
{
    if (engine == NULL || !rm_valid_bus_id(src_bus_id) || !rm_valid_bus_id(dst_bus_id)) {
        return RM_ERR_INVALID_ARG;
    }
    if (dst_bus_id <= src_bus_id) {
        return RM_ERR_UNSUPPORTED;
    }

    rm_reset_bus_send_state(&engine->sends[src_bus_id][dst_bus_id]);
    return RM_OK;
}

rm_result rm_engine_bus_fx_clear(rm_engine *engine, rm_u16 bus_id, rm_u16 slot)
{
    if (engine == NULL || !rm_valid_bus_id(bus_id) || !rm_valid_bus_fx_slot(slot)) {
        return RM_ERR_INVALID_ARG;
    }

    rm_reset_bus_fx_state(&engine->buses[bus_id].fx[slot]);
    return RM_OK;
}

rm_result rm_engine_bus_fx_set_lowpass(rm_engine *engine,
                                       rm_u16 bus_id,
                                       rm_u16 slot,
                                       rm_u32 cutoff_hz,
                                       rm_s16 wet_q15,
                                       rm_s16 output_gain_q15)
{
    rm_bus_fx_state *fx;

    if (engine == NULL || !rm_valid_bus_id(bus_id) || !rm_valid_bus_fx_slot(slot) || cutoff_hz == 0U) {
        return RM_ERR_INVALID_ARG;
    }

    fx = &engine->buses[bus_id].fx[slot];
    rm_reset_bus_fx_state(fx);
    fx->type = (rm_u8)RM_BUS_FX_LOWPASS;
    fx->active = 1U;
    fx->param0 = cutoff_hz;
    fx->param1 = (rm_u32)(rm_u16)rm_compute_lowpass_alpha_q15(cutoff_hz, engine->sample_rate);
    fx->wet_q15 = rm_clamp_unit_q15((rm_s32)wet_q15);
    fx->output_gain_q15 = output_gain_q15;
    return RM_OK;
}

rm_result rm_engine_bus_fx_set_drive(rm_engine *engine,
                                     rm_u16 bus_id,
                                     rm_u16 slot,
                                     rm_u16 drive_q12,
                                     rm_s16 threshold_q15,
                                     rm_s16 wet_q15,
                                     rm_s16 output_gain_q15)
{
    rm_bus_fx_state *fx;
    rm_s32 threshold;

    if (engine == NULL || !rm_valid_bus_id(bus_id) || !rm_valid_bus_fx_slot(slot)) {
        return RM_ERR_INVALID_ARG;
    }

    threshold = threshold_q15;
    if (threshold < 0) {
        threshold = -threshold;
    }
    if (threshold == 0) {
        return RM_ERR_INVALID_ARG;
    }
    if (drive_q12 == 0U) {
        drive_q12 = RM_RATIO_ONE_Q12;
    }

    fx = &engine->buses[bus_id].fx[slot];
    rm_reset_bus_fx_state(fx);
    fx->type = (rm_u8)RM_BUS_FX_DRIVE;
    fx->active = 1U;
    fx->param0 = drive_q12;
    fx->param1 = (rm_u32)threshold;
    fx->wet_q15 = rm_clamp_unit_q15((rm_s32)wet_q15);
    fx->output_gain_q15 = output_gain_q15;
    return RM_OK;
}

rm_result rm_engine_bus_fx_set_biquad(rm_engine *engine,
                                      rm_u16 bus_id,
                                      rm_u16 slot,
                                      const rm_biquad_desc *desc)
{
    rm_bus_fx_state *fx;

    if (engine == NULL || desc == NULL || !rm_valid_bus_id(bus_id) || !rm_valid_bus_fx_slot(slot)) {
        return RM_ERR_INVALID_ARG;
    }

    if (desc->b0_q14 == 0 && desc->b1_q14 == 0 && desc->b2_q14 == 0) {
        return RM_ERR_INVALID_ARG;
    }

    fx = &engine->buses[bus_id].fx[slot];
    rm_reset_bus_fx_state(fx);
    fx->type = (rm_u8)RM_BUS_FX_BIQUAD;
    fx->active = 1U;
    fx->b0_q14 = desc->b0_q14;
    fx->b1_q14 = desc->b1_q14;
    fx->b2_q14 = desc->b2_q14;
    fx->a1_q14 = desc->a1_q14;
    fx->a2_q14 = desc->a2_q14;
    fx->wet_q15 = rm_clamp_unit_q15((rm_s32)desc->wet_q15);
    fx->output_gain_q15 = desc->output_gain_q15;
    return RM_OK;
}

rm_result rm_engine_set_group(rm_engine *engine, rm_u16 group_id, rm_s16 gain_q15, rm_s16 pan_q15)
{
    if (engine == NULL || !rm_valid_group_id(group_id)) {
        return RM_ERR_INVALID_ARG;
    }

    rm_pair_control_set(&engine->groups[group_id].gain_q15,
                        &engine->groups[group_id].pan_q15,
                        &engine->groups[group_id].left_gain_q15,
                        &engine->groups[group_id].right_gain_q15,
                        &engine->groups[group_id].target_gain_q15,
                        &engine->groups[group_id].target_pan_q15,
                        &engine->groups[group_id].ramp_frames_remaining,
                        gain_q15,
                        pan_q15);
    return RM_OK;
}

rm_result rm_engine_ramp_group(rm_engine *engine,
                               rm_u16 group_id,
                               rm_s16 gain_q15,
                               rm_s16 pan_q15,
                               rm_u32 frames)
{
    if (engine == NULL || !rm_valid_group_id(group_id)) {
        return RM_ERR_INVALID_ARG;
    }

    rm_pair_control_ramp(&engine->groups[group_id].gain_q15,
                         &engine->groups[group_id].pan_q15,
                         &engine->groups[group_id].left_gain_q15,
                         &engine->groups[group_id].right_gain_q15,
                         &engine->groups[group_id].target_gain_q15,
                         &engine->groups[group_id].target_pan_q15,
                         &engine->groups[group_id].ramp_frames_remaining,
                         gain_q15,
                         pan_q15,
                         frames);
    return RM_OK;
}

rm_result rm_engine_set_group_mute(rm_engine *engine, rm_u16 group_id, rm_u16 mute_on)
{
    if (engine == NULL || !rm_valid_group_id(group_id)) {
        return RM_ERR_INVALID_ARG;
    }
    engine->groups[group_id].mute = mute_on ? 1U : 0U;
    return RM_OK;
}

rm_result rm_engine_set_group_solo(rm_engine *engine, rm_u16 group_id, rm_u16 solo_on)
{
    if (engine == NULL || !rm_valid_group_id(group_id)) {
        return RM_ERR_INVALID_ARG;
    }
    engine->groups[group_id].solo = solo_on ? 1U : 0U;
    return RM_OK;
}

rm_result rm_engine_stop_group(rm_engine *engine, rm_u16 group_id)
{
    rm_u16 i;
    rm_voice_state *voice;

    if (engine == NULL || !rm_valid_group_id(group_id)) {
        return RM_ERR_INVALID_ARG;
    }

    for (i = 0U; i < engine->max_voices; ++i) {
        voice = &engine->voices[i];
        if (!voice->active || voice->group_id != group_id) {
            continue;
        }
        rm_clear_voice(voice);
        voice->generation++;
    }

    return RM_OK;
}

rm_result rm_engine_capture_snapshot(const rm_engine *engine, rm_mix_snapshot *out_snapshot)
{
    rm_u16 i;
    rm_u16 j;

    if (engine == NULL || out_snapshot == NULL) {
        return RM_ERR_INVALID_ARG;
    }

    memset(out_snapshot, 0, sizeof(*out_snapshot));
    out_snapshot->master_gain_q15 = engine->master_gain_q15;
    out_snapshot->headroom_q15 = engine->headroom_q15;
    out_snapshot->monitor_gain_q15 = engine->monitor_gain_q15;
    out_snapshot->monitor_pan_q15 = engine->monitor_pan_q15;
    out_snapshot->limiter_enabled = engine->limiter.active;
    out_snapshot->limiter_attack_frames = engine->limiter.attack_frames;
    out_snapshot->limiter_release_frames = engine->limiter.release_frames;
    out_snapshot->limiter_lookahead_frames = engine->limiter.lookahead_frames;
    out_snapshot->limiter_threshold_q15 = engine->limiter.threshold_q15;
    out_snapshot->limiter_output_gain_q15 = engine->limiter.output_gain_q15;

    for (i = 0U; i < (rm_u16)RAWMIX_MAX_BUSES; ++i) {
        out_snapshot->buses[i].gain_q15 = engine->buses[i].gain_q15;
        out_snapshot->buses[i].pan_q15 = engine->buses[i].pan_q15;
        out_snapshot->buses[i].mute = engine->buses[i].mute;
        out_snapshot->buses[i].solo = engine->buses[i].solo;
        for (j = 0U; j < (rm_u16)RAWMIX_MAX_BUSES; ++j) {
            out_snapshot->sends[i][j] = engine->sends[i][j];
        }
    }

    for (i = 0U; i < (rm_u16)RAWMIX_MAX_GROUPS; ++i) {
        out_snapshot->groups[i].gain_q15 = engine->groups[i].gain_q15;
        out_snapshot->groups[i].pan_q15 = engine->groups[i].pan_q15;
        out_snapshot->groups[i].mute = engine->groups[i].mute;
        out_snapshot->groups[i].solo = engine->groups[i].solo;
    }

    return RM_OK;
}

rm_result rm_engine_apply_snapshot(rm_engine *engine,
                                   const rm_mix_snapshot *snapshot,
                                   rm_u32 ramp_frames)
{
    rm_u16 i;
    rm_u16 j;
    rm_result result;

    if (engine == NULL || snapshot == NULL) {
        return RM_ERR_INVALID_ARG;
    }

    engine->master_gain_q15 = snapshot->master_gain_q15;
    engine->headroom_q15 = snapshot->headroom_q15;
    rm_engine_set_monitor(engine, snapshot->monitor_gain_q15, snapshot->monitor_pan_q15);

    for (i = 0U; i < (rm_u16)RAWMIX_MAX_BUSES; ++i) {
        if (ramp_frames == 0U) {
            rm_engine_set_bus(engine, i, snapshot->buses[i].gain_q15, snapshot->buses[i].pan_q15);
        } else {
            rm_engine_ramp_bus(engine, i, snapshot->buses[i].gain_q15, snapshot->buses[i].pan_q15, ramp_frames);
        }
        engine->buses[i].mute = snapshot->buses[i].mute;
        engine->buses[i].solo = snapshot->buses[i].solo;
        for (j = 0U; j < (rm_u16)RAWMIX_MAX_BUSES; ++j) {
            if (snapshot->sends[i][j].active) {
                result = rm_engine_set_bus_send(engine,
                                                i,
                                                j,
                                                snapshot->sends[i][j].gain_q15,
                                                snapshot->sends[i][j].mode);
                if (result != RM_OK && result != RM_ERR_UNSUPPORTED) {
                    return result;
                }
            } else if (j > i) {
                rm_engine_clear_bus_send(engine, i, j);
            }
        }
    }

    for (i = 0U; i < (rm_u16)RAWMIX_MAX_GROUPS; ++i) {
        if (ramp_frames == 0U) {
            rm_engine_set_group(engine, i, snapshot->groups[i].gain_q15, snapshot->groups[i].pan_q15);
        } else {
            rm_engine_ramp_group(engine, i, snapshot->groups[i].gain_q15, snapshot->groups[i].pan_q15, ramp_frames);
        }
        engine->groups[i].mute = snapshot->groups[i].mute;
        engine->groups[i].solo = snapshot->groups[i].solo;
    }

    if (snapshot->limiter_enabled) {
        return rm_engine_set_limiter_ex(engine,
                                        snapshot->limiter_threshold_q15,
                                        snapshot->limiter_attack_frames,
                                        snapshot->limiter_release_frames,
                                        snapshot->limiter_output_gain_q15,
                                        snapshot->limiter_lookahead_frames);
    }

    return rm_engine_clear_limiter(engine);
}

rm_result rm_engine_queue_automation(rm_engine *engine, const rm_automation_event *event_desc)
{
    if (engine == NULL || event_desc == NULL) {
        return RM_ERR_INVALID_ARG;
    }
    if (engine->automation_count >= (rm_u16)RAWMIX_MAX_AUTOMATION_EVENTS) {
        return RM_ERR_QUEUE_FULL;
    }

    engine->automation_queue[engine->automation_count] = *event_desc;
    engine->automation_count++;
    return RM_OK;
}

rm_result rm_engine_queue_automationv(rm_engine *engine,
                                      const rm_automation_event *event_descs,
                                      rm_u16 count)
{
    rm_u16 i;
    rm_result result;

    if (engine == NULL || event_descs == NULL) {
        return RM_ERR_INVALID_ARG;
    }

    for (i = 0U; i < count; ++i) {
        result = rm_engine_queue_automation(engine, &event_descs[i]);
        if (result != RM_OK) {
            return result;
        }
    }
    return RM_OK;
}

void rm_engine_clear_automation(rm_engine *engine)
{
    if (engine == NULL) {
        return;
    }
    engine->automation_count = 0U;
}

static rm_result rm_prepare_voice_common(rm_engine *engine,
                                         const rm_voice_params *params,
                                         rm_voice_handle *out_handle,
                                         rm_u16 *out_slot,
                                         rm_u32 *out_stolen)
{
    rm_result result;
    rm_voice_state *voice;
    rm_u16 slot;

    if (engine == NULL || params == NULL || out_slot == NULL || out_stolen == NULL) {
        return RM_ERR_INVALID_ARG;
    }
    if (!rm_valid_bus_id(params->bus_id) ||
        !rm_valid_group_id(params->group_id) ||
        !rm_valid_resampler(params->resampler)) {
        return RM_ERR_INVALID_ARG;
    }

    result = rm_find_voice_slot(engine,
                                params->priority,
                                params->flags,
                                out_stolen,
                                &slot);
    if (result != RM_OK) {
        return result;
    }

    voice = &engine->voices[slot];
    if (*out_stolen != 0U) {
        engine->stats.voices_stolen++;
        voice->generation++;
    }

    rm_clear_voice(voice);
    voice->active = 1U;
    voice->priority = params->priority;
    voice->bus_id = params->bus_id;
    voice->group_id = params->group_id;
    voice->resampler = rm_resolve_resampler(params->resampler, engine->default_resampler);
    voice->flags = params->flags;
    voice->start_serial = ++engine->start_serial_counter;
    voice->start_delay_frames = params->start_delay_frames;

    if (params->fade_in_frames > 0U) {
        voice->gain_q15 = RM_Q15_ZERO;
        voice->pan_q15 = params->pan_q15;
        voice->target_gain_q15 = params->gain_q15;
        voice->target_pan_q15 = params->pan_q15;
        voice->ramp_frames_remaining = params->fade_in_frames;
        rm_update_pair_gains(voice->gain_q15,
                             voice->pan_q15,
                             &voice->left_gain_q15,
                             &voice->right_gain_q15);
    } else {
        rm_pair_control_set(&voice->gain_q15,
                            &voice->pan_q15,
                            &voice->left_gain_q15,
                            &voice->right_gain_q15,
                            &voice->target_gain_q15,
                            &voice->target_pan_q15,
                            &voice->ramp_frames_remaining,
                            params->gain_q15,
                            params->pan_q15);
    }

    if (out_handle != NULL) {
        out_handle->slot = slot;
        out_handle->generation = voice->generation;
    }

    *out_slot = slot;
    return RM_OK;
}

rm_result rm_engine_play_buffer(rm_engine *engine,
                                const rm_buffer *buffer,
                                const rm_voice_params *params,
                                rm_voice_handle *out_handle)
{
    rm_voice_params local_params;
    rm_u16 slot;
    rm_u32 stolen;
    rm_voice_state *voice;
    rm_u32 loop_end;
    rm_result result;

    if (engine == NULL || buffer == NULL) {
        return RM_ERR_INVALID_ARG;
    }
    if (buffer->samples == NULL || buffer->frame_count == 0U || buffer->sample_rate == 0U) {
        return RM_ERR_INVALID_ARG;
    }
    if (buffer->format != (rm_u16)RM_SAMPLE_S16) {
        return RM_ERR_UNSUPPORTED;
    }
    if (!buffer->interleaved) {
        return RM_ERR_UNSUPPORTED;
    }
    if (!rm_valid_channels(buffer->channels)) {
        return RM_ERR_UNSUPPORTED;
    }

    if (params == NULL) {
        rm_voice_params_init(&local_params);
        params = &local_params;
    }

    if (params->start_frame_offset >= buffer->frame_count) {
        return RM_ERR_INVALID_ARG;
    }

    result = rm_prepare_voice_common(engine, params, out_handle, &slot, &stolen);
    if (result != RM_OK) {
        return result;
    }

    voice = &engine->voices[slot];
    voice->source_kind = RM_SOURCE_KIND_BUFFER;
    voice->src_channels = (rm_u8)buffer->channels;
    voice->data = (const rm_s16 *)buffer->samples;
    voice->frame_count = buffer->frame_count;
    voice->sample_rate = buffer->sample_rate;
    voice->pos_frame = params->start_frame_offset;
    voice->pos_frac_q12 = 0U;
    voice->step_q12 = rm_compute_step_q12(buffer->sample_rate,
                                          engine->sample_rate,
                                          params->pitch_q12 == 0U ? RM_RATIO_ONE_Q12 : params->pitch_q12);
    voice->loop_start_frame = params->loop_start_frame;

    loop_end = params->loop_end_frame;
    if (loop_end == 0U || loop_end > buffer->frame_count) {
        loop_end = buffer->frame_count;
    }
    voice->loop_end_frame = loop_end;

    engine->stats.voices_started++;
    return RM_OK;
}

rm_result rm_engine_play_stream(rm_engine *engine,
                                const rm_stream_desc *stream,
                                const rm_voice_params *params,
                                rm_voice_handle *out_handle)
{
    rm_voice_params local_params;
    rm_u16 slot;
    rm_u32 stolen;
    rm_voice_state *voice;
    rm_result result;

    if (engine == NULL || stream == NULL || stream->on_next == NULL) {
        return RM_ERR_INVALID_ARG;
    }
    if (stream->sample_rate == 0U || !rm_valid_channels(stream->channels)) {
        return RM_ERR_INVALID_ARG;
    }

    if (params == NULL) {
        rm_voice_params_init(&local_params);
        params = &local_params;
    }

    if ((params->flags & (rm_u16)RM_VOICE_FLAG_LOOP) != 0U) {
        return RM_ERR_UNSUPPORTED;
    }

    result = rm_prepare_voice_common(engine, params, out_handle, &slot, &stolen);
    if (result != RM_OK) {
        return result;
    }

    voice = &engine->voices[slot];
    voice->source_kind = RM_SOURCE_KIND_STREAM;
    voice->src_channels = (rm_u8)stream->channels;
    voice->sample_rate = stream->sample_rate;
    voice->frame_count = 0U;
    voice->data = NULL;
    voice->stream_next = stream->on_next;
    voice->stream_user = stream->user;
    voice->step_q12 = rm_compute_step_q12(stream->sample_rate,
                                          engine->sample_rate,
                                          params->pitch_q12 == 0U ? RM_RATIO_ONE_Q12 : params->pitch_q12);

    if (params->start_frame_offset > 0U) {
        result = rm_stream_discard_frames(voice, params->start_frame_offset);
        if (result != RM_OK) {
            rm_clear_voice(voice);
            voice->generation++;
            return result;
        }
    }

    result = rm_stream_prime_voice(voice);
    if (result != RM_OK) {
        rm_clear_voice(voice);
        voice->generation++;
        return result;
    }

    engine->stats.voices_started++;
    return RM_OK;
}

rm_result rm_engine_stop_voice(rm_engine *engine, rm_voice_handle handle)
{
    rm_voice_state *voice;
    rm_result result;

    if (engine == NULL) {
        return RM_ERR_INVALID_ARG;
    }
    result = rm_validate_handle(engine, handle, &voice);
    if (result != RM_OK) {
        return result;
    }
    rm_clear_voice(voice);
    voice->generation++;
    return RM_OK;
}

rm_result rm_engine_set_voice_gain(rm_engine *engine, rm_voice_handle handle, rm_s16 gain_q15)
{
    rm_voice_state *voice;
    rm_result result;

    result = rm_validate_handle(engine, handle, &voice);
    if (result != RM_OK) {
        return result;
    }

    voice->stop_when_silent = 0U;
    rm_pair_control_set(&voice->gain_q15,
                        &voice->pan_q15,
                        &voice->left_gain_q15,
                        &voice->right_gain_q15,
                        &voice->target_gain_q15,
                        &voice->target_pan_q15,
                        &voice->ramp_frames_remaining,
                        gain_q15,
                        voice->pan_q15);
    return RM_OK;
}

rm_result rm_engine_set_voice_pan(rm_engine *engine, rm_voice_handle handle, rm_s16 pan_q15)
{
    rm_voice_state *voice;
    rm_result result;

    result = rm_validate_handle(engine, handle, &voice);
    if (result != RM_OK) {
        return result;
    }

    voice->stop_when_silent = 0U;
    rm_pair_control_set(&voice->gain_q15,
                        &voice->pan_q15,
                        &voice->left_gain_q15,
                        &voice->right_gain_q15,
                        &voice->target_gain_q15,
                        &voice->target_pan_q15,
                        &voice->ramp_frames_remaining,
                        voice->gain_q15,
                        pan_q15);
    return RM_OK;
}

rm_result rm_engine_set_voice_pitch(rm_engine *engine, rm_voice_handle handle, rm_u16 pitch_q12)
{
    rm_voice_state *voice;
    rm_result result;

    result = rm_validate_handle(engine, handle, &voice);
    if (result != RM_OK) {
        return result;
    }

    if (pitch_q12 == 0U) {
        pitch_q12 = RM_RATIO_ONE_Q12;
    }
    voice->step_q12 = rm_compute_step_q12(voice->sample_rate, engine->sample_rate, pitch_q12);
    return RM_OK;
}

rm_result rm_engine_set_voice_bus(rm_engine *engine, rm_voice_handle handle, rm_u16 bus_id)
{
    rm_voice_state *voice;
    rm_result result;

    if (!rm_valid_bus_id(bus_id)) {
        return RM_ERR_INVALID_ARG;
    }

    result = rm_validate_handle(engine, handle, &voice);
    if (result != RM_OK) {
        return result;
    }

    voice->bus_id = bus_id;
    return RM_OK;
}

rm_result rm_engine_set_voice_group(rm_engine *engine, rm_voice_handle handle, rm_u16 group_id)
{
    rm_voice_state *voice;
    rm_result result;

    if (!rm_valid_group_id(group_id)) {
        return RM_ERR_INVALID_ARG;
    }

    result = rm_validate_handle(engine, handle, &voice);
    if (result != RM_OK) {
        return result;
    }

    voice->group_id = group_id;
    return RM_OK;
}

rm_result rm_engine_ramp_voice(rm_engine *engine,
                               rm_voice_handle handle,
                               rm_s16 gain_q15,
                               rm_s16 pan_q15,
                               rm_u32 frames)
{
    rm_voice_state *voice;
    rm_result result;

    result = rm_validate_handle(engine, handle, &voice);
    if (result != RM_OK) {
        return result;
    }

    voice->stop_when_silent = 0U;
    rm_pair_control_ramp(&voice->gain_q15,
                         &voice->pan_q15,
                         &voice->left_gain_q15,
                         &voice->right_gain_q15,
                         &voice->target_gain_q15,
                         &voice->target_pan_q15,
                         &voice->ramp_frames_remaining,
                         gain_q15,
                         pan_q15,
                         frames);
    return RM_OK;
}

rm_result rm_engine_fade_out_voice(rm_engine *engine, rm_voice_handle handle, rm_u32 frames)
{
    rm_voice_state *voice;
    rm_result result;

    result = rm_validate_handle(engine, handle, &voice);
    if (result != RM_OK) {
        return result;
    }

    voice->stop_when_silent = 1U;
    rm_pair_control_ramp(&voice->gain_q15,
                         &voice->pan_q15,
                         &voice->left_gain_q15,
                         &voice->right_gain_q15,
                         &voice->target_gain_q15,
                         &voice->target_pan_q15,
                         &voice->ramp_frames_remaining,
                         RM_Q15_ZERO,
                         voice->pan_q15,
                         frames);
    if (frames == 0U) {
        rm_clear_voice(voice);
        voice->generation++;
    }
    return RM_OK;
}

int rm_engine_is_voice_active(const rm_engine *engine, rm_voice_handle handle)
{
    rm_voice_state *voice;

    if (rm_validate_handle(engine, handle, &voice) != RM_OK) {
        return 0;
    }
    return voice->active ? 1 : 0;
}

rm_result rm_engine_render_s16(rm_engine *engine, rm_s16 *output_interleaved, rm_u32 frames)
{
    if (engine == NULL || output_interleaved == NULL || frames == 0U) {
        return RM_ERR_INVALID_ARG;
    }
    rm_process_frames(engine,
                      NULL,
                      0U,
                      output_interleaved,
                      engine->channels,
                      frames,
                      0);
    engine->stats.render_calls++;
    return RM_OK;
}

rm_result rm_engine_process_duplex_s16(rm_engine *engine,
                                       const rm_s16 *input_interleaved,
                                       rm_u16 input_channels,
                                       rm_s16 *output_interleaved,
                                       rm_u16 output_channels,
                                       rm_u32 frames)
{
    if (engine == NULL || output_interleaved == NULL || frames == 0U) {
        return RM_ERR_INVALID_ARG;
    }
    if (!rm_valid_channels(output_channels)) {
        return RM_ERR_UNSUPPORTED;
    }
    if (input_interleaved != NULL && !rm_valid_channels(input_channels)) {
        return RM_ERR_UNSUPPORTED;
    }

    rm_process_frames(engine,
                      input_interleaved,
                      input_channels,
                      output_interleaved,
                      output_channels,
                      frames,
                      input_interleaved != NULL ? 1 : 0);
    engine->stats.duplex_calls++;
    return RM_OK;
}

rm_u32 rm_engine_capture_available(const rm_engine *engine)
{
    if (engine == NULL) {
        return 0U;
    }
    return (rm_u32)engine->capture_count_frames;
}

rm_result rm_engine_capture_read_s16(rm_engine *engine,
                                     rm_s16 *dst_interleaved,
                                     rm_u16 dst_channels,
                                     rm_u32 max_frames,
                                     rm_u32 *out_frames_read)
{
    rm_u32 frames_to_read;
    rm_u32 i;
    rm_u16 base;
    rm_s16 left;
    rm_s16 right;

    if (out_frames_read != NULL) {
        *out_frames_read = 0U;
    }

    if (engine == NULL || dst_interleaved == NULL || max_frames == 0U) {
        return RM_ERR_INVALID_ARG;
    }
    if (!rm_valid_channels(dst_channels)) {
        return RM_ERR_UNSUPPORTED;
    }
    if (engine->capture_count_frames == 0U) {
        return RM_ERR_EMPTY;
    }

    frames_to_read = (rm_u32)engine->capture_count_frames;
    if (frames_to_read > max_frames) {
        frames_to_read = max_frames;
    }

    for (i = 0U; i < frames_to_read; ++i) {
        base = (rm_u16)(engine->capture_read_frame * 2U);
        left = engine->capture_ring[base];
        right = engine->capture_ring[base + 1U];
        if (dst_channels == 1U) {
            dst_interleaved[i] = (rm_s16)(((rm_s32)left + (rm_s32)right) / 2);
        } else {
            dst_interleaved[i * 2U] = left;
            dst_interleaved[i * 2U + 1U] = right;
        }
        engine->capture_read_frame = (rm_u16)((engine->capture_read_frame + 1U) % (rm_u16)RAWMIX_CAPTURE_RING_FRAMES);
        engine->capture_count_frames--;
    }

    engine->stats.capture_frames_read += frames_to_read;
    if (out_frames_read != NULL) {
        *out_frames_read = frames_to_read;
    }
    return RM_OK;
}
