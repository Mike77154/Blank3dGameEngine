#include "gshotgunsequence89.h"

static gss89_s16 gss89_sat16(gss89_s32 x)
{
    if (x > 32767) return 32767;
    if (x < -32768) return -32768;
    return (gss89_s16)x;
}

static gss89_s32 gss89_mul_q15(gss89_s32 a, gss89_s32 b)
{
    return (a * b) >> 15;
}

static gss89_u32 gss89_ms_to_frames(gss89_u32 rate, gss89_u32 ms)
{
    return (rate * ms + 999U) / 1000U;
}

static gss89_u32 gss89_next_seed(gss89_context *ctx, gss89_u32 seed)
{
    if (seed != 0U) return seed;
    ctx->seed_counter = ctx->seed_counter * 1664525U + 1013904223U;
    return ctx->seed_counter;
}


static gss89_u16 gss89_clamp_u16(gss89_s32 value)
{
    if (value < 0) return 0U;
    if (value > 65535) return 65535U;
    return (gss89_u16)value;
}

static gss89_s16 gss89_clamp_q15(gss89_s32 value)
{
    if (value < 0) return 0;
    if (value > 32767) return 32767;
    return (gss89_s16)value;
}

static gss89_u16 gss89_scale_ms(gss89_u16 value,
                                  gss89_u32 numerator,
                                  gss89_u32 denominator)
{
    gss89_u32 scaled;
    if (denominator == 0U) return value;
    scaled = ((gss89_u32)value * numerator + (denominator / 2U)) / denominator;
    if (scaled > 65535U) scaled = 65535U;
    return (gss89_u16)scaled;
}

static gss89_u16 gss89_map_pump_ms(gss89_u16 old_ms,
                                    gss89_u32 target_rear_ms,
                                    gss89_u32 target_forward_ms)
{
    gss89_s32 mapped;
    gss89_s32 old_delta;
    gss89_u32 target_span;

    /*
     * chuecka89 v2.4 perceptual transient anchors:
     *   rear "chic" peak    ~= 87 ms
     *   forward "chic" peak ~= 231 ms
     * Map those onto gpump's rear-stop and battery "chuk" events.
     */
    target_span = target_forward_ms - target_rear_ms;
    old_delta = (gss89_s32)old_ms - 87;
    mapped = (gss89_s32)target_rear_ms
           + (old_delta * (gss89_s32)target_span) / 144;
    return gss89_clamp_u16(mapped);
}

static void gss89_scale_pump_voice_time(ch89_voice *voice,
                                         gss89_u32 numerator,
                                         gss89_u32 denominator)
{
    if (voice == 0) return;
    voice->envelope.delay_ms = gss89_scale_ms(voice->envelope.delay_ms,
                                               numerator, denominator);
    voice->envelope.attack_ms = gss89_scale_ms(voice->envelope.attack_ms,
                                                numerator, denominator);
    voice->envelope.decay_ms = gss89_scale_ms(voice->envelope.decay_ms,
                                               numerator, denominator);
    voice->envelope.sustain_ms = gss89_scale_ms(voice->envelope.sustain_ms,
                                                 numerator, denominator);
    voice->envelope.release_ms = gss89_scale_ms(voice->envelope.release_ms,
                                                 numerator, denominator);
    voice->eq_sweep_ms = gss89_scale_ms(voice->eq_sweep_ms,
                                         numerator, denominator);
}

static void gss89_brighten_pump_voice(ch89_voice *voice)
{
    static const gss89_s32 factor_q15[CH89_EQ_BANDS] = {
        8192, 14746, 24576, 32767, 40959, 49151
    };
    gss89_u16 i;
    gss89_s32 value;

    if (voice == 0) return;
    for (i = 0U; i < CH89_EQ_BANDS; ++i) {
        value = gss89_mul_q15((gss89_s32)voice->eq_start_q15[i],
                              factor_q15[i]);
        voice->eq_start_q15[i] = gss89_clamp_q15(value);
        value = gss89_mul_q15((gss89_s32)voice->eq_end_q15[i],
                              factor_q15[i]);
        voice->eq_end_q15[i] = gss89_clamp_q15(value);
    }

    /* Keep the pump layer crisp and metallic, not dark like shell insertion. */
    voice->envelope.color = CH89_NOISE_BRIGHT;
    voice->distortion_q15 = (gss89_s16)(((gss89_s32)voice->distortion_q15 * 3) / 4);
}

static void gss89_shape_pump_harmonic(ch89_gesture *gesture,
                                       gss89_u32 cycle_ms)
{
    gss89_u32 target_rear_ms;
    gss89_u32 target_forward_ms;
    gss89_u32 target_span;
    gss89_u32 max_end_ms;
    gss89_u32 end_ms;
    gss89_u16 i;
    ch89_stroke *stroke;

    if (gesture == 0 || cycle_ms == 0U) return;

    target_rear_ms = (cycle_ms * 400U) / 1000U;
    target_forward_ms = (cycle_ms * 900U) / 1000U;
    target_span = target_forward_ms - target_rear_ms;
    max_end_ms = 0U;

    for (i = 0U; i < gesture->stroke_count; ++i) {
        stroke = &gesture->strokes[i];
        stroke->start_ms = gss89_map_pump_ms(stroke->start_ms,
                                              target_rear_ms,
                                              target_forward_ms);
        gss89_scale_pump_voice_time(&stroke->sh, target_span, 144U);
        gss89_scale_pump_voice_time(&stroke->eckt, target_span, 144U);
        gss89_brighten_pump_voice(&stroke->sh);
        gss89_brighten_pump_voice(&stroke->eckt);

        end_ms = (gss89_u32)stroke->start_ms
               + (gss89_u32)stroke->sh.envelope.delay_ms
               + (gss89_u32)stroke->sh.envelope.attack_ms
               + (gss89_u32)stroke->sh.envelope.decay_ms
               + (gss89_u32)stroke->sh.envelope.sustain_ms
               + (gss89_u32)stroke->sh.envelope.release_ms;
        if (end_ms > max_end_ms) max_end_ms = end_ms;

        end_ms = (gss89_u32)stroke->start_ms
               + (gss89_u32)stroke->eckt.envelope.delay_ms
               + (gss89_u32)stroke->eckt.envelope.attack_ms
               + (gss89_u32)stroke->eckt.envelope.decay_ms
               + (gss89_u32)stroke->eckt.envelope.sustain_ms
               + (gss89_u32)stroke->eckt.envelope.release_ms;
        if (end_ms > max_end_ms) max_end_ms = end_ms;
    }

    /* Dry, close and bright: this harmonic should feel welded to gpump. */
    gesture->chorus_wet_q15 = 0;
    gesture->reverb_wet_q15 = 0;
    gesture->output_gain_q15 = 32767;
    max_end_ms += 28U;
    if (max_end_ms > 65535U) max_end_ms = 65535U;
    gesture->duration_ms = (gss89_u16)max_end_ms;
}

static sp89_preset gss89_shotpumpkin_for_gpump(int gpump_preset)
{
    switch (gpump_preset) {
    case GPUMP89_PRESET_TIGHT_DRY:
        return SP89_PRESET_OILED;
    case GPUMP89_PRESET_LOOSE_SERVICE:
        return SP89_PRESET_WORN;
    case GPUMP89_PRESET_HEAVY:
        return SP89_PRESET_HEAVY;
    case GPUMP89_PRESET_CINEMATIC_DRY:
        return SP89_PRESET_CINEMATIC;
    case GPUMP89_PRESET_WITH_SHELL:
    default:
        return SP89_PRESET_REALISTIC;
    }
}

static void gss89_prepare_shotpumpkin(gss89_context *ctx,
                                       sp89_preset preset,
                                       gss89_u32 cycle_ms,
                                       gss89_u32 seed)
{
    sp89_config cfg;
    gss89_u32 pull_ms;
    gss89_u32 gap_ms;
    gss89_u32 pump_ms;

    sp89_config_preset(&cfg, ctx->sample_rate, preset);

    /* Align the continuous texture with gpump's physical stage anchors:
       rear stop ~= 40%, forward friction ~= 54%, lock ~= 93%. */
    pull_ms = (cycle_ms * 400U + 500U) / 1000U;
    gap_ms = (cycle_ms * 140U + 500U) / 1000U;
    pump_ms = (cycle_ms * 390U + 500U) / 1000U;
    if (pull_ms < 1U) pull_ms = 1U;
    if (gap_ms < 1U) gap_ms = 1U;
    if (pump_ms < 1U) pump_ms = 1U;

    cfg.pull_ms = pull_ms;
    cfg.gap_ms = gap_ms;
    cfg.pump_ms = pump_ms;
    cfg.seed = seed;

    /* The world/room stack owns ambience; keep this mechanical layer close. */
    cfg.chorus_mix_q15 = 700U;
    cfg.reverb_mix_q15 = 0U;
    cfg.reverb_feedback_q15 = 0U;

    sp89_init(&ctx->shotpumpkin, &cfg);
    sp89_trigger_cycle(&ctx->shotpumpkin);
}


static const char *gss89_model_names[GSS89_SHOTGUN_MODEL_COUNT] = {
    "balanced",
    "remington_870",
    "mossberg_500_590",
    "benelli_nova",
    "winchester_sxp"
};

const char *gss89_shotgun_model_name(gss89_shotgun_model model)
{
    if ((int)model < 0 || model >= GSS89_SHOTGUN_MODEL_COUNT) return "invalid";
    return gss89_model_names[(int)model];
}

static gsgeq89_profile_id gss89_eq_profile_for_model(gss89_shotgun_model model)
{
    switch (model) {
    case GSS89_SHOTGUN_MODEL_REMINGTON_870:
        return GSGEQ89_PROFILE_REMINGTON_870;
    case GSS89_SHOTGUN_MODEL_MOSSBERG_500_590:
        return GSGEQ89_PROFILE_MOSSBERG_500_590;
    case GSS89_SHOTGUN_MODEL_BENELLI_NOVA:
        return GSGEQ89_PROFILE_BENELLI_NOVA;
    case GSS89_SHOTGUN_MODEL_WINCHESTER_SXP:
        return GSGEQ89_PROFILE_WINCHESTER_SXP;
    case GSS89_SHOTGUN_MODEL_BALANCED:
    default:
        return GSGEQ89_PROFILE_BALANCED;
    }
}

static void gss89_apply_model_mix(gss89_context *ctx,
                                   gss89_shotgun_model model)
{
    gss89_mix mix;
    if (ctx == 0) return;
    gss89_mix_default(&mix);
    switch (model) {
    case GSS89_SHOTGUN_MODEL_REMINGTON_870:
        mix.pump_primary_q15 = 25500;
        mix.pump_chuecka_q15 = 20500;
        mix.pump_chuecka_delay_ms = 20U;
        mix.pump_shotpumpkin_q15 = 8200;
        mix.pump_shotpumpkin_delay_ms = 6U;
        mix.pump_klek_q15 = 22000;
        mix.pump_tik_q15 = 5200;
        mix.master_q15 = 27800;
        ctx->shotpumpkin_preset = SP89_PRESET_OILED;
        break;
    case GSS89_SHOTGUN_MODEL_MOSSBERG_500_590:
        mix.pump_primary_q15 = 22200;
        mix.pump_chuecka_q15 = 28200;
        mix.pump_chuecka_delay_ms = 22U;
        mix.pump_shotpumpkin_q15 = 12300;
        mix.pump_shotpumpkin_delay_ms = 8U;
        mix.pump_klek_q15 = 27400;
        mix.pump_tik_q15 = 9800;
        mix.master_q15 = 27000;
        ctx->shotpumpkin_preset = SP89_PRESET_WORN;
        break;
    case GSS89_SHOTGUN_MODEL_BENELLI_NOVA:
        mix.pump_primary_q15 = 25200;
        mix.pump_chuecka_q15 = 19000;
        mix.pump_chuecka_delay_ms = 20U;
        mix.pump_shotpumpkin_q15 = 7600;
        mix.pump_shotpumpkin_delay_ms = 7U;
        mix.pump_klek_q15 = 23600;
        mix.pump_tik_q15 = 6500;
        mix.master_q15 = 27800;
        ctx->shotpumpkin_preset = SP89_PRESET_HEAVY;
        break;
    case GSS89_SHOTGUN_MODEL_WINCHESTER_SXP:
        mix.pump_primary_q15 = 23600;
        mix.pump_chuecka_q15 = 26600;
        mix.pump_chuecka_delay_ms = 15U;
        mix.pump_shotpumpkin_q15 = 8600;
        mix.pump_shotpumpkin_delay_ms = 4U;
        mix.pump_klek_q15 = 28600;
        mix.pump_tik_q15 = 8600;
        mix.master_q15 = 27000;
        ctx->shotpumpkin_preset = SP89_PRESET_OILED;
        break;
    case GSS89_SHOTGUN_MODEL_BALANCED:
    default:
        ctx->shotpumpkin_preset = SP89_PRESET_REALISTIC;
        break;
    }
    ctx->mix = mix;
    ctx->shotgun_model = model;
    gsgeq89_init(&ctx->pump_master, gss89_eq_profile_for_model(model));
    ctx->pump_master_enabled = 1U;
}

void gss89_set_shotgun_model(gss89_context *ctx, gss89_shotgun_model model)
{
    if (ctx == 0) return;
    if ((int)model < 0 || model >= GSS89_SHOTGUN_MODEL_COUNT) return;
    gss89_apply_model_mix(ctx, model);
}

void gss89_enable_pump_master(gss89_context *ctx, int enabled)
{
    if (ctx == 0) return;
    ctx->pump_master_enabled = enabled ? 1U : 0U;
    gsgeq89_reset(&ctx->pump_master);
}

void gss89_set_pump_master_profile(gss89_context *ctx,
                                    gsgeq89_profile_id profile)
{
    if (ctx == 0) return;
    gsgeq89_init(&ctx->pump_master, profile);
    ctx->pump_master_enabled = 1U;
}

void gss89_set_pump_master_custom(gss89_context *ctx,
                                   const gsgeq89_preset *preset)
{
    if (ctx == 0 || preset == 0) return;
    gsgeq89_set_preset(&ctx->pump_master, preset);
    ctx->pump_master_enabled = 1U;
}

void gss89_set_tek_provider(gss89_context *ctx,
                            gss89_tek_trigger_fn trigger_fn,
                            gss89_tek_process_fn process_fn,
                            gss89_tek_active_fn active_fn,
                            void *user)
{
    if (ctx == 0) return;
    ctx->tek_trigger = trigger_fn;
    ctx->tek_process = process_fn;
    ctx->tek_active = active_fn;
    ctx->tek_user = user;
}

static void gss89_model_pump_params(gss89_shotgun_model model,
                                     gpump89_params *params,
                                     int *base_preset)
{
    int preset;
    preset = GPUMP89_PRESET_WITH_SHELL;
    switch (model) {
    case GSS89_SHOTGUN_MODEL_REMINGTON_870:
        preset = GPUMP89_PRESET_WITH_SHELL;
        break;
    case GSS89_SHOTGUN_MODEL_MOSSBERG_500_590:
        preset = GPUMP89_PRESET_LOOSE_SERVICE;
        break;
    case GSS89_SHOTGUN_MODEL_BENELLI_NOVA:
        preset = GPUMP89_PRESET_HEAVY;
        break;
    case GSS89_SHOTGUN_MODEL_WINCHESTER_SXP:
        preset = GPUMP89_PRESET_TIGHT_DRY;
        break;
    case GSS89_SHOTGUN_MODEL_BALANCED:
    default:
        preset = GPUMP89_PRESET_WITH_SHELL;
        break;
    }
    gpump89_preset(params, preset, GSS89_SAMPLE_RATE);
    params->shell_enabled = 1UL;
    switch (model) {
    case GSS89_SHOTGUN_MODEL_REMINGTON_870:
        params->cycle_ms = 500UL;
        params->force_q15 = 29600L;
        params->slide_q15 = 8300L;
        params->wear_q15 = 2400L;
        params->variation_q15 = 1000L;
        params->master_q15 = 28000L;
        break;
    case GSS89_SHOTGUN_MODEL_MOSSBERG_500_590:
        params->cycle_ms = 470UL;
        params->force_q15 = 27900L;
        params->slide_q15 = 11200L;
        params->wear_q15 = 15000L;
        params->variation_q15 = 2200L;
        params->master_q15 = 27800L;
        break;
    case GSS89_SHOTGUN_MODEL_BENELLI_NOVA:
        params->cycle_ms = 530UL;
        params->force_q15 = 31000L;
        params->slide_q15 = 7700L;
        params->wear_q15 = 2800L;
        params->variation_q15 = 1000L;
        params->master_q15 = 27000L;
        break;
    case GSS89_SHOTGUN_MODEL_WINCHESTER_SXP:
        params->cycle_ms = 365UL;
        params->force_q15 = 30000L;
        params->slide_q15 = 7500L;
        params->wear_q15 = 2200L;
        params->variation_q15 = 800L;
        params->master_q15 = 27800L;
        break;
    case GSS89_SHOTGUN_MODEL_BALANCED:
    default:
        break;
    }
    if (base_preset != 0) *base_preset = preset;
}

static gss89_u16 gss89_model_klek_permille(gss89_shotgun_model model)
{
    switch (model) {
    case GSS89_SHOTGUN_MODEL_MOSSBERG_500_590: return 590U;
    case GSS89_SHOTGUN_MODEL_BENELLI_NOVA: return 650U;
    case GSS89_SHOTGUN_MODEL_WINCHESTER_SXP: return 885U;
    case GSS89_SHOTGUN_MODEL_REMINGTON_870: return 625U;
    case GSS89_SHOTGUN_MODEL_BALANCED:
    default: return 625U;
    }
}

static gkl89_preset_id gss89_model_klek_preset(gss89_shotgun_model model,
                                                int gpump_preset)
{
    if (model == GSS89_SHOTGUN_MODEL_MOSSBERG_500_590) {
        return GKL89_PRESET_STEEL_KLEK;
    }
    if (model == GSS89_SHOTGUN_MODEL_WINCHESTER_SXP) {
        return GKL89_PRESET_REAR_STOP;
    }
    if (gpump_preset == GPUMP89_PRESET_HEAVY) return GKL89_PRESET_REAR_STOP;
    return GKL89_PRESET_CARRIER_KLEK;
}

void gss89_mix_default(gss89_mix *mix)
{
    if (mix == 0) return;
    mix->report_q15 = 29200;
    mix->report_body_q15 = 8200;
    mix->report_gas_q15 = 7200;
    mix->report_crack_q15 = 4200;
    mix->report_thump_q15 = 8600;
    mix->report_tail_q15 = 8200;

    /* Balanced master: structural mass first, bright articulation second. */
    mix->pump_primary_q15 = 21000;
    mix->pump_chuecka_q15 = 22400;
    mix->pump_chuecka_delay_ms = 22U;
    mix->pump_shotpumpkin_q15 = 9200;
    mix->pump_shotpumpkin_delay_ms = 7U;
    mix->pump_klek_q15 = 21000;
    mix->pump_tik_q15 = 6200;

    mix->general_chuecka_q15 = 25500;
    mix->foley_q15 = 22500;
    mix->master_q15 = 27600;
}

static void gss89_disarm_report_autotrigger(gpaah89_state *state)
{
    int i;
    if (state == 0) return;
    for (i = 0; i < GPAAH89_NOISE_OSCS; ++i) {
        state->env[i].stage = 0U;
        state->env[i].pos = 0U;
        state->env[i].level_q15 = 0;
    }
    state->tail_pos = 0U;
    state->trigger_pos = 0U;
    state->active = 0U;
}

static int gss89_init_report_layers(gss89_context *ctx, gss89_u32 seed)
{
    gpaah89_preset report_preset;
    gwb89_preset body_preset;
    gmg89_preset gas_preset;
    gbc89_preset crack_preset;
    glt89_preset tail_preset;
    gct89_preset thump_preset;

    if (!gpaah89_get_preset(GPAAH89_PRESET_SHOTGUN, &report_preset)) return 0;
    if (!gwb89_get_preset(GWB89_PRESET_SHOTGUN, &body_preset)) return 0;
    if (!gmg89_get_preset(GMG89_PRESET_SHOTGUN, &gas_preset)) return 0;
    if (!gbc89_get_preset(GBC89_PRESET_NEAR, &crack_preset)) return 0;
    if (!glt89_get_preset(GLT89_PRESET_WAREHOUSE, &tail_preset)) return 0;
    if (!gct89_get_preset(GCT89_PRESET_SHOTGUN, &thump_preset)) return 0;

    body_preset.dry_q15 = 0;
    body_preset.wet_q15 = 29500;
    tail_preset.dry_q15 = 0;
    tail_preset.wet_q15 = 23800;
    tail_preset.input_gain_q15 = 17500;

    if (!gpaah89_init(&ctx->report, ctx->sample_rate, &report_preset, seed + 1U)) return 0;
    /* gpaah89 intentionally auto-triggers on init; the sequencer starts armed but silent. */
    gss89_disarm_report_autotrigger(&ctx->report);
    if (!gwb89_init(&ctx->body, ctx->sample_rate, &body_preset, seed + 2U)) return 0;
    if (!gmg89_init(&ctx->gas, ctx->sample_rate, &gas_preset, seed + 3U)) return 0;
    if (!gbc89_init(&ctx->crack, ctx->sample_rate, &crack_preset, seed + 4U)) return 0;
    if (!glt89_init(&ctx->tail, ctx->sample_rate, &tail_preset)) return 0;
    if (!gct89_init(&ctx->thump, ctx->sample_rate, &thump_preset, seed + 5U)) return 0;
    return 1;
}

gss89_result gss89_init(gss89_context *ctx,
                         gss89_u32 sample_rate,
                         gss89_u32 seed)
{
    gss89_u16 i;
    if (ctx == 0) return GSS89_BAD_ARGUMENT;
    if (sample_rate != GSS89_SAMPLE_RATE) return GSS89_UNSUPPORTED_RATE;

    ctx->sample_rate = sample_rate;
    ctx->seed_counter = seed == 0U ? 0x47535331U : seed;
    gss89_mix_default(&ctx->mix);

    gpump89_init(&ctx->pump, sample_rate, ctx->seed_counter + 11U);
    ctx->shotpumpkin_preset = SP89_PRESET_REALISTIC;
    ctx->shotgun_model = GSS89_SHOTGUN_MODEL_BALANCED;
    ctx->pump_master_enabled = 1U;
    gsgeq89_init(&ctx->pump_master, GSGEQ89_PROFILE_BALANCED);
    ctx->tek_trigger = 0;
    ctx->tek_process = 0;
    ctx->tek_active = 0;
    ctx->tek_user = 0;
    ctx->shotpumpkin_delay_frames = 0U;
    {
        sp89_config cfg;
        sp89_config_preset(&cfg, sample_rate, ctx->shotpumpkin_preset);
        cfg.chorus_mix_q15 = 700U;
        cfg.reverb_mix_q15 = 0U;
        cfg.reverb_feedback_q15 = 0U;
        sp89_init(&ctx->shotpumpkin, &cfg);
    }
    if (ch89_init(&ctx->chuecka_synth, sample_rate, ctx->seed_counter + 12U) != CH89_OK) {
        return GSS89_DEPENDENCY_ERROR;
    }
    gkl89_init(&ctx->klek, ctx->seed_counter + 13U);
    gwf89_init(&ctx->foley, ctx->seed_counter + 14U);
    gwf89_set_room_send(&ctx->foley, 2500);
    ctx->pump_tik_delay_frames = 0U;
    ctx->pump_tik_seed = 0U;
    ctx->pump_tik_pending = 0U;
    ctx->foley_role = 0U;

    if (!gss89_init_report_layers(ctx, ctx->seed_counter + 20U)) {
        return GSS89_DEPENDENCY_ERROR;
    }

    for (i = 0U; i < GSS89_TEXTURE_VOICES; ++i) {
        ctx->texture[i].pcm = 0;
        ctx->texture[i].capacity = 0U;
        ctx->texture[i].frames = 0U;
        ctx->texture[i].position = 0U;
        ctx->texture[i].delay_frames = 0U;
        ctx->texture[i].active = 0U;
        ctx->texture[i].role = GSS89_TEXTURE_GENERAL;
    }
    return GSS89_OK;
}

gss89_result gss89_bind_texture_buffer(gss89_context *ctx,
                                        gss89_u16 voice_index,
                                        gss89_s16 *pcm,
                                        gss89_u32 frame_capacity)
{
    if (ctx == 0 || pcm == 0) return GSS89_BAD_ARGUMENT;
    if (voice_index >= GSS89_TEXTURE_VOICES) return GSS89_BAD_ARGUMENT;
    ctx->texture[voice_index].pcm = pcm;
    ctx->texture[voice_index].capacity = frame_capacity;
    ctx->texture[voice_index].frames = 0U;
    ctx->texture[voice_index].position = 0U;
    ctx->texture[voice_index].active = 0U;
    return GSS89_OK;
}

void gss89_set_mix(gss89_context *ctx, const gss89_mix *mix)
{
    if (ctx == 0 || mix == 0) return;
    ctx->mix = *mix;
}

void gss89_set_shotpumpkin_preset(gss89_context *ctx, sp89_preset preset)
{
    if (ctx == 0) return;
    if ((int)preset < 0 || preset >= SP89_PRESET_COUNT) return;
    ctx->shotpumpkin_preset = preset;
}

void gss89_trigger_report(gss89_context *ctx, gss89_u32 seed)
{
    gss89_u32 s;
    if (ctx == 0) return;
    s = gss89_next_seed(ctx, seed);
    gpaah89_trigger(&ctx->report, s + 1U);
    gwb89_trigger(&ctx->body, 30000, s + 2U);
    gmg89_trigger(&ctx->gas, 30000, s + 3U);
    gbc89_trigger(&ctx->crack, 18U, 28700, s + 4U);
    gct89_trigger(&ctx->thump, 28200, s + 5U);
}

static gss89_texture_voice *gss89_acquire_texture_voice(gss89_context *ctx)
{
    gss89_u16 i;
    gss89_texture_voice *oldest;
    oldest = 0;
    for (i = 0U; i < GSS89_TEXTURE_VOICES; ++i) {
        if (ctx->texture[i].pcm == 0 || ctx->texture[i].capacity == 0U) continue;
        if (!ctx->texture[i].active) return &ctx->texture[i];
        if (oldest == 0 || ctx->texture[i].position > oldest->position) {
            oldest = &ctx->texture[i];
        }
    }
    return oldest;
}

static gss89_result gss89_render_texture(gss89_context *ctx,
                                         ch89_preset preset,
                                         gss89_u16 repetitions,
                                         gss89_u16 intensity_q15,
                                         gss89_u32 seed,
                                         gss89_u8 role,
                                         gss89_u16 delay_ms,
                                         gss89_u32 pump_cycle_ms)
{
    ch89_gesture gesture;
    ch89_u32 required;
    ch89_u32 written;
    gss89_texture_voice *voice;

    voice = gss89_acquire_texture_voice(ctx);
    if (voice == 0) return GSS89_TEXTURE_BUFFER_MISSING;
    if (ch89_make_preset(preset, repetitions, intensity_q15, &gesture) != CH89_OK) {
        return GSS89_DEPENDENCY_ERROR;
    }
    if (role == GSS89_TEXTURE_PUMP_HARMONIC) {
        gss89_shape_pump_harmonic(&gesture, pump_cycle_ms);
    }
    required = ch89_required_frames(&gesture, ctx->sample_rate);
    if (required > voice->capacity) return GSS89_TEXTURE_BUFFER_SMALL;

    ch89_reset(&ctx->chuecka_synth, gss89_next_seed(ctx, seed));
    written = 0U;
    if (ch89_render(&ctx->chuecka_synth, &gesture,
                    voice->pcm, voice->capacity, &written) != CH89_OK) {
        return GSS89_DEPENDENCY_ERROR;
    }
    voice->frames = written;
    voice->position = 0U;
    voice->delay_frames = gss89_ms_to_frames(ctx->sample_rate, delay_ms);
    voice->active = 1U;
    voice->role = role;
    return GSS89_OK;
}

static gss89_result gss89_trigger_pump_params(gss89_context *ctx,
                                                const gpump89_params *source,
                                                int gpump_preset,
                                                gss89_u32 seed)
{
    gpump89_params params;
    gss89_u32 s;
    gss89_u32 articulation_ms;
    gss89_result r;
    if (ctx == 0 || source == 0) return GSS89_BAD_ARGUMENT;
    params = *source;
    s = gss89_next_seed(ctx, seed);
    params.seed = s + 1U;
    gpump89_start_cycle(&ctx->pump, &params);

    /* A user-selected preset wins; REALISTIC automatically follows gpump class. */
    gss89_prepare_shotpumpkin(ctx,
        ctx->shotpumpkin_preset == SP89_PRESET_REALISTIC
            ? gss89_shotpumpkin_for_gpump(gpump_preset)
            : ctx->shotpumpkin_preset,
        params.cycle_ms, s + 5U);
    ctx->shotpumpkin_delay_frames = gss89_ms_to_frames(
        ctx->sample_rate, ctx->mix.pump_shotpumpkin_delay_ms);

    r = gss89_render_texture(ctx,
                             CH89_PRESET_SHOTGUN_PUMP,
                             1U,
                             32767U,
                             s + 2U,
                             GSS89_TEXTURE_PUMP_HARMONIC,
                             ctx->mix.pump_chuecka_delay_ms,
                             params.cycle_ms);
    if (r != GSS89_OK) return r;

    articulation_ms = (params.cycle_ms
        * (gss89_u32)gss89_model_klek_permille(ctx->shotgun_model)) / 1000U;
    if (!gkl89_trigger(&ctx->klek,
                       gss89_model_klek_preset(ctx->shotgun_model,
                                              gpump_preset),
                       s + 3U,
                       (gss89_u16)articulation_ms)) {
        return GSS89_DEPENDENCY_ERROR;
    }

    /* Optional external tek provider supersedes the built-in Foley particle. */
    if (ctx->tek_trigger != 0 && ctx->tek_process != 0) {
        ctx->tek_trigger(ctx->tek_user, s + 4U,
                         (gss89_u16)(articulation_ms + 11U),
                         ctx->shotgun_model);
        ctx->pump_tik_pending = 0U;
    } else {
        ctx->pump_tik_delay_frames = gss89_ms_to_frames(
            ctx->sample_rate, articulation_ms + 11U);
        ctx->pump_tik_seed = s + 4U;
        ctx->pump_tik_pending = 1U;
    }
    return GSS89_OK;
}

gss89_result gss89_trigger_pump(gss89_context *ctx,
                                 int gpump_preset,
                                 gss89_u32 seed)
{
    gpump89_params params;
    if (ctx == 0) return GSS89_BAD_ARGUMENT;
    if (!gpump89_preset(&params, gpump_preset, ctx->sample_rate)) {
        return GSS89_BAD_ARGUMENT;
    }
    return gss89_trigger_pump_params(ctx, &params, gpump_preset, seed);
}

gss89_result gss89_trigger_model_pump(gss89_context *ctx,
                                       gss89_shotgun_model model,
                                       gss89_u32 seed)
{
    gpump89_params params;
    int base_preset;
    if (ctx == 0) return GSS89_BAD_ARGUMENT;
    if ((int)model < 0 || model >= GSS89_SHOTGUN_MODEL_COUNT) {
        return GSS89_BAD_ARGUMENT;
    }
    gss89_set_shotgun_model(ctx, model);
    gss89_model_pump_params(model, &params, &base_preset);
    return gss89_trigger_pump_params(ctx, &params, base_preset, seed);
}

gss89_result gss89_trigger_chuecka(gss89_context *ctx,
                                    ch89_preset preset,
                                    gss89_u16 repetitions,
                                    gss89_u16 intensity_q15,
                                    gss89_u32 seed)
{
    if (ctx == 0) return GSS89_BAD_ARGUMENT;
    return gss89_render_texture(ctx, preset, repetitions, intensity_q15,
                                seed, GSS89_TEXTURE_GENERAL, 0U, 0U);
}

void gss89_trigger_foley(gss89_context *ctx,
                         gwf89_preset_id preset,
                         gss89_u32 seed)
{
    if (ctx == 0) return;
    ctx->pump_tik_pending = 0U;
    ctx->foley_role = 0U;
    gwf89_trigger_ex(&ctx->foley, preset, gss89_next_seed(ctx, seed),
                     GWF89_VARIANT_AUTO, GWF89_SPEED_NORMAL);
}

static gss89_s32 gss89_process_textures(gss89_context *ctx)
{
    gss89_u16 i;
    gss89_s32 mix;
    gss89_s32 gain;
    gss89_texture_voice *v;
    mix = 0;
    for (i = 0U; i < GSS89_TEXTURE_VOICES; ++i) {
        v = &ctx->texture[i];
        if (!v->active) continue;
        if (v->delay_frames > 0U) {
            v->delay_frames--;
            continue;
        }
        if (v->position >= v->frames) {
            v->active = 0U;
            continue;
        }
        gain = v->role == GSS89_TEXTURE_PUMP_HARMONIC
             ? ctx->mix.pump_chuecka_q15
             : ctx->mix.general_chuecka_q15;
        mix += gss89_mul_q15((gss89_s32)v->pcm[v->position], gain);
        v->position++;
        if (v->position >= v->frames) v->active = 0U;
    }
    return mix;
}

gss89_s16 gss89_process_sample(gss89_context *ctx)
{
    gss89_s16 report;
    gss89_s16 pump;
    gss89_s16 shotpumpkin;
    gss89_s16 foley;
    gss89_s16 klek;
    gss89_s16 tek;
    gss89_s16 body;
    gss89_s16 gas;
    gss89_s16 crack;
    gss89_s16 thump;
    gss89_s16 tail;
    gss89_s16 feed;
    gss89_s32 texture_mix;
    gss89_s32 shot_mix;
    gss89_s32 pump_bus;
    gss89_s32 total;

    if (ctx == 0) return 0;

    report = 0;
    if (gpaah89_is_active(&ctx->report)) {
        gpaah89_render_mono(&ctx->report, &report, 1U);
    }
    body = gwb89_process_sample(&ctx->body, report);
    gas = gmg89_process_sample(&ctx->gas);
    crack = gbc89_process_sample(&ctx->crack);
    thump = gct89_process_sample(&ctx->thump);
    feed = gss89_sat16(((gss89_s32)report >> 1)
                     + ((gss89_s32)body >> 2)
                     + ((gss89_s32)gas >> 2)
                     + ((gss89_s32)crack >> 2));
    tail = glt89_process_sample(&ctx->tail, feed);

    shot_mix = gss89_mul_q15(report, ctx->mix.report_q15)
             + gss89_mul_q15(body, ctx->mix.report_body_q15)
             + gss89_mul_q15(gas, ctx->mix.report_gas_q15)
             + gss89_mul_q15(crack, ctx->mix.report_crack_q15)
             + gss89_mul_q15(thump, ctx->mix.report_thump_q15)
             + gss89_mul_q15(tail, ctx->mix.report_tail_q15);

    pump = 0;
    if (gpump89_is_active(&ctx->pump)) {
        gpump89_render_i16(&ctx->pump, &pump, 1U);
    }
    shotpumpkin = 0;
    if (ctx->shotpumpkin_delay_frames > 0U) {
        ctx->shotpumpkin_delay_frames--;
    } else if (sp89_is_active(&ctx->shotpumpkin)) {
        shotpumpkin = sp89_process(&ctx->shotpumpkin);
    }
    texture_mix = gss89_process_textures(ctx);
    klek = gkl89_process_sample(&ctx->klek);
    tek = 0;
    if (ctx->tek_process != 0) tek = ctx->tek_process(ctx->tek_user);

    if (ctx->pump_tik_pending) {
        if (ctx->pump_tik_delay_frames > 0U) {
            ctx->pump_tik_delay_frames--;
        } else {
            gwf89_trigger_ex(&ctx->foley, GWF89_SHOTGUN_SAFETY,
                             ctx->pump_tik_seed,
                             GWF89_VARIANT_AUTO, GWF89_SPEED_FAST);
            ctx->foley_role = 1U;
            ctx->pump_tik_pending = 0U;
        }
    }

    foley = 0;
    if (gwf89_is_active(&ctx->foley)) {
        foley = gwf89_process_sample(&ctx->foley);
    }

    pump_bus = gss89_mul_q15(pump, ctx->mix.pump_primary_q15)
             + gss89_mul_q15(shotpumpkin, ctx->mix.pump_shotpumpkin_q15)
             + texture_mix
             + gss89_mul_q15(klek, ctx->mix.pump_klek_q15)
             + gss89_mul_q15(tek, ctx->mix.pump_tik_q15)
             + gss89_mul_q15(foley,
                 ctx->foley_role ? ctx->mix.pump_tik_q15 : ctx->mix.foley_q15);
    if (ctx->pump_master_enabled) {
        pump_bus = (gss89_s32)gsgeq89_process_sample(
            &ctx->pump_master, gss89_sat16(pump_bus));
    }
    total = shot_mix + pump_bus;
    total = gss89_mul_q15(total, ctx->mix.master_q15);
    return gss89_sat16(total);
}

gss89_u32 gss89_render_mono(gss89_context *ctx,
                             gss89_s16 *output,
                             gss89_u32 frames)
{
    gss89_u32 i;
    if (ctx == 0 || output == 0) return 0U;
    for (i = 0U; i < frames; ++i) output[i] = gss89_process_sample(ctx);
    return frames;
}

int gss89_is_active(const gss89_context *ctx)
{
    gss89_u16 i;
    if (ctx == 0) return 0;
    if (gpaah89_is_active(&ctx->report)) return 1;
    if (gpump89_is_active(&ctx->pump)) return 1;
    if (ctx->shotpumpkin_delay_frames > 0U) return 1;
    if (sp89_is_active(&ctx->shotpumpkin)) return 1;
    if (gkl89_is_active(&ctx->klek)) return 1;
    if (ctx->tek_active != 0 && ctx->tek_active(ctx->tek_user)) return 1;
    if (ctx->pump_tik_pending) return 1;
    if (gwf89_is_active(&ctx->foley)) return 1;
    if (gwb89_is_active(&ctx->body)) return 1;
    if (gmg89_is_active(&ctx->gas)) return 1;
    if (gbc89_is_active(&ctx->crack)) return 1;
    if (glt89_is_active(&ctx->tail)) return 1;
    if (gct89_is_active(&ctx->thump)) return 1;
    for (i = 0U; i < GSS89_TEXTURE_VOICES; ++i) {
        if (ctx->texture[i].active) return 1;
    }
    return 0;
}

gss89_u32 gss89_context_bytes(void)
{
    return (gss89_u32)sizeof(gss89_context);
}
