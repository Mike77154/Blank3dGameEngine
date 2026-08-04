#include "weapon_synth_sound_engine89.h"

static gv89_s16 wsse89_sat16(gv89_s32 value)
{
    if (value > 32767) return 32767;
    if (value < -32768) return -32768;
    return (gv89_s16)value;
}


static gv89_s16 wsse89_clamp_pan(gv89_s16 pan)
{
    if (pan < -32767) return -32767;
    return pan;
}

static gv89_s16 wsse89_clamp_gain(gv89_s16 gain)
{
    if (gain < 0) return 0;
    return gain;
}

static void wsse89_pan_mono(gv89_s16 sample, gv89_s16 gain_q15,
                            gv89_s16 pan_q15, gv89_s32 *left,
                            gv89_s32 *right)
{
    gv89_s32 value;
    gv89_s32 left_gain;
    gv89_s32 right_gain;
    value = ((gv89_s32)sample * (gv89_s32)wsse89_clamp_gain(gain_q15)) >> 15;
    pan_q15 = wsse89_clamp_pan(pan_q15);
    left_gain = pan_q15 > 0 ? 32767 - pan_q15 : 32767;
    right_gain = pan_q15 < 0 ? 32767 + pan_q15 : 32767;
    *left += (value * left_gain) >> 15;
    *right += (value * right_gain) >> 15;
}

static void wsse89_pan_stereo(gv89_s16 in_left, gv89_s16 in_right,
                              gv89_s16 gain_q15, gv89_s16 pan_q15,
                              gv89_s32 *left, gv89_s32 *right)
{
    gv89_s32 left_gain;
    gv89_s32 right_gain;
    gv89_s32 gain;
    pan_q15 = wsse89_clamp_pan(pan_q15);
    gain = wsse89_clamp_gain(gain_q15);
    left_gain = pan_q15 > 0 ? 32767 - pan_q15 : 32767;
    right_gain = pan_q15 < 0 ? 32767 + pan_q15 : 32767;
    *left += ((((gv89_s32)in_left * gain) >> 15) * left_gain) >> 15;
    *right += ((((gv89_s32)in_right * gain) >> 15) * right_gain) >> 15;
}

static wsse89_magazine_voice *wsse89_alloc_magazine(wsse89_context *ctx)
{
    gv89_u16 i;
    gv89_u16 oldest_index;
    gv89_u32 oldest_stamp;
    oldest_index = 0U;
    oldest_stamp = 0xFFFFFFFFU;
    for (i = 0U; i < WSSE89_MAGAZINE_VOICE_CAPACITY; ++i) {
        if (!ctx->magazine_voices[i].active ||
            !wmag89_is_active(&ctx->magazine_voices[i].synth))
            return &ctx->magazine_voices[i];
        if (ctx->magazine_voices[i].age_stamp < oldest_stamp) {
            oldest_stamp = ctx->magazine_voices[i].age_stamp;
            oldest_index = i;
        }
    }
    return &ctx->magazine_voices[oldest_index];
}

static wsse89_rocket_whistle_voice *wsse89_find_whistle(wsse89_context *ctx,
                                                         gv89_u32 key)
{
    gv89_u16 i;
    if (key == 0U) return 0;
    for (i = 0U; i < WSSE89_ROCKET_WHISTLE_VOICE_CAPACITY; ++i) {
        if (ctx->rocket_whistle_voices[i].active &&
            ctx->rocket_whistle_voices[i].instance_key == key)
            return &ctx->rocket_whistle_voices[i];
    }
    return 0;
}

static wsse89_rocket_whistle_voice *wsse89_alloc_whistle(wsse89_context *ctx,
                                                          gv89_u32 key)
{
    gv89_u16 i;
    gv89_u16 oldest_index;
    gv89_u32 oldest_stamp;
    wsse89_rocket_whistle_voice *voice;
    voice = wsse89_find_whistle(ctx, key);
    if (voice != 0) return voice;
    oldest_index = 0U;
    oldest_stamp = 0xFFFFFFFFU;
    for (i = 0U; i < WSSE89_ROCKET_WHISTLE_VOICE_CAPACITY; ++i) {
        if (!ctx->rocket_whistle_voices[i].active ||
            !gwh89_is_active(&ctx->rocket_whistle_voices[i].synth))
            return &ctx->rocket_whistle_voices[i];
        if (ctx->rocket_whistle_voices[i].age_stamp < oldest_stamp) {
            oldest_stamp = ctx->rocket_whistle_voices[i].age_stamp;
            oldest_index = i;
        }
    }
    return &ctx->rocket_whistle_voices[oldest_index];
}

static wsse89_rocket_spin_voice *wsse89_find_spin(wsse89_context *ctx,
                                                    gv89_u32 key)
{
    gv89_u16 i;
    if (key == 0U) return 0;
    for (i = 0U; i < WSSE89_ROCKET_SPIN_VOICE_CAPACITY; ++i) {
        if (ctx->rocket_spin_voices[i].active &&
            ctx->rocket_spin_voices[i].instance_key == key)
            return &ctx->rocket_spin_voices[i];
    }
    return 0;
}

static wsse89_rocket_spin_voice *wsse89_alloc_spin(wsse89_context *ctx,
                                                    gv89_u32 key)
{
    gv89_u16 i;
    gv89_u16 oldest_index;
    gv89_u32 oldest_stamp;
    wsse89_rocket_spin_voice *voice;
    voice = wsse89_find_spin(ctx, key);
    if (voice != 0) return voice;
    oldest_index = 0U;
    oldest_stamp = 0xFFFFFFFFU;
    for (i = 0U; i < WSSE89_ROCKET_SPIN_VOICE_CAPACITY; ++i) {
        if (!ctx->rocket_spin_voices[i].active ||
            !grs89_is_active(&ctx->rocket_spin_voices[i].synth))
            return &ctx->rocket_spin_voices[i];
        if (ctx->rocket_spin_voices[i].age_stamp < oldest_stamp) {
            oldest_stamp = ctx->rocket_spin_voices[i].age_stamp;
            oldest_index = i;
        }
    }
    return &ctx->rocket_spin_voices[oldest_index];
}

static wsse89_gatling_voice *wsse89_find_gatling(wsse89_context *ctx,
                                                  gv89_u32 key)
{
    gv89_u16 i;
    if (key == 0U) return 0;
    for (i = 0U; i < WSSE89_GATLING_VOICE_CAPACITY; ++i) {
        if (ctx->gatling_voices[i].active &&
            ctx->gatling_voices[i].instance_key == key)
            return &ctx->gatling_voices[i];
    }
    return 0;
}

static wsse89_gatling_voice *wsse89_alloc_gatling(wsse89_context *ctx,
                                                   gv89_u32 key)
{
    gv89_u16 i;
    gv89_u16 oldest_index;
    gv89_u32 oldest_stamp;
    wsse89_gatling_voice *voice;
    voice = wsse89_find_gatling(ctx, key);
    if (voice != 0) return voice;
    oldest_index = 0U;
    oldest_stamp = 0xFFFFFFFFU;
    for (i = 0U; i < WSSE89_GATLING_VOICE_CAPACITY; ++i) {
        if (!ctx->gatling_voices[i].active)
            return &ctx->gatling_voices[i];
        if (ctx->gatling_voices[i].age_stamp < oldest_stamp) {
            oldest_stamp = ctx->gatling_voices[i].age_stamp;
            oldest_index = i;
        }
    }
    return &ctx->gatling_voices[oldest_index];
}

static void wsse89_process_auxiliary(wsse89_context *ctx,
                                     gv89_s16 *left, gv89_s16 *right)
{
    gv89_u16 i;
    gv89_s16 mono;
    gwh89_s16 stereo[2];
    gv89_s32 aux_left;
    gv89_s32 aux_right;
    aux_left = 0;
    aux_right = 0;
    for (i = 0U; i < WSSE89_MAGAZINE_VOICE_CAPACITY; ++i) {
        wsse89_magazine_voice *voice;
        voice = &ctx->magazine_voices[i];
        if (!voice->active) continue;
        if (!wmag89_is_active(&voice->synth)) {
            voice->active = 0U;
            continue;
        }
        mono = (gv89_s16)wmag89_process_sample(&voice->synth);
        wsse89_pan_mono(mono, voice->gain_q15, voice->pan_q15,
                        &aux_left, &aux_right);
        if (!wmag89_is_active(&voice->synth)) voice->active = 0U;
    }
    for (i = 0U; i < WSSE89_ROCKET_WHISTLE_VOICE_CAPACITY; ++i) {
        wsse89_rocket_whistle_voice *voice;
        voice = &ctx->rocket_whistle_voices[i];
        if (!voice->active) continue;
        if (!gwh89_is_active(&voice->synth)) {
            voice->active = 0U;
            continue;
        }
        stereo[0] = 0;
        stereo[1] = 0;
        gwh89_render_stereo(&voice->synth, stereo, 1);
        wsse89_pan_stereo(stereo[0], stereo[1], voice->gain_q15,
                          voice->pan_q15, &aux_left, &aux_right);
        if (!gwh89_is_active(&voice->synth)) voice->active = 0U;
    }
    for (i = 0U; i < WSSE89_ROCKET_SPIN_VOICE_CAPACITY; ++i) {
        wsse89_rocket_spin_voice *voice;
        voice = &ctx->rocket_spin_voices[i];
        if (!voice->active) continue;
        if (!grs89_is_active(&voice->synth)) {
            voice->active = 0U;
            continue;
        }
        mono = (gv89_s16)grs89_process_sample(&voice->synth);
        wsse89_pan_mono(mono, voice->gain_q15, voice->pan_q15,
                        &aux_left, &aux_right);
        if (!grs89_is_active(&voice->synth)) voice->active = 0U;
    }
    for (i = 0U; i < WSSE89_GATLING_VOICE_CAPACITY; ++i) {
        wsse89_gatling_voice *voice;
        ggm89_s16 motor_sample;
        ggtr89_i16 rotator_sample;
        short whistle_sample;
        gv89_s32 mixed;
        voice = &ctx->gatling_voices[i];
        if (!voice->active) continue;
        motor_sample = 0;
        rotator_sample = 0;
        whistle_sample = 0;
        ggm89_render_mono(&voice->motor, &motor_sample, 1U);
        ggtr89_render(&voice->rotator, &rotator_sample, 1);
        if (ggw89_is_active(&voice->whistle))
            whistle_sample = ggw89_process(&voice->whistle);
        mixed = ((gv89_s32)motor_sample * voice->motor_gain_q15) >> 15;
        mixed += ((gv89_s32)rotator_sample * voice->rotator_gain_q15) >> 15;
        mixed += ((gv89_s32)whistle_sample * voice->whistle_gain_q15) >> 15;
        mono = wsse89_sat16(mixed);
        wsse89_pan_mono(mono, voice->gain_q15, voice->pan_q15,
                        &aux_left, &aux_right);
        if (ggm89_get_mode(&voice->motor) == GGM89_STATE_OFF &&
            !ggtr89_is_active(&voice->rotator) &&
            !ggw89_is_active(&voice->whistle)) {
            voice->active = 0U;
            voice->firing = 0U;
        }
    }
    *left = wsse89_sat16((gv89_s32)*left + aux_left);
    *right = wsse89_sat16((gv89_s32)*right + aux_right);
}

static void wsse89_invalid_handle(gv89_handle *handle)
{
    if (handle == 0) return;
    handle->index = GV89_INVALID_INDEX;
    handle->generation = 0U;
}

static gv89_u32 wsse89_next_seed(wsse89_context *ctx, gv89_u32 requested)
{
    if (requested != 0U) return requested;
    ctx->seed_counter = ctx->seed_counter * 1664525U + 1013904223U;
    if (ctx->seed_counter == 0U) ctx->seed_counter = 1U;
    return ctx->seed_counter;
}

void wsse89_config_defaults(wsse89_config *config)
{
    if (config == 0) return;
    config->sample_rate = 44100U;
    config->logical_voice_capacity = GV89_RECOMMENDED_LOGICAL_VOICES;
    config->physical_voice_limit = GV89_RECOMMENDED_PHYSICAL_VOICES;
    config->seed = 0x47534658U;
    config->expansion_mask = GSSEXP89_ALL;
}

void wsse89_weapon_fire_defaults(wsse89_weapon_fire_event *event)
{
    if (event == 0) return;
    event->pan_q15 = 0;
    event->distance_q15 = 1800U;
    event->occlusion_q15 = 32767U;
    event->focus_q15 = 32767U;
    event->gain_q15 = 28600;
    event->priority_bias = 180;
    event->instance_key = 0U;
    event->instance_limit = 16U;
    event->pressure_energy_q15 = 0U;
}


void wsse89_magazine_defaults(wsse89_magazine_event *event)
{
    if (event == 0) return;
    event->preset = WMAG89_PRESET_PISTOL_POLYMER;
    event->action = WMAG89_ACTION_INSERT;
    event->velocity_q15 = 30000U;
    event->gain_q15 = 21000;
    event->pan_q15 = 0;
    event->instance_key = 0U;
}

void wsse89_rocket_whistle_defaults(wsse89_rocket_whistle_start_event *event)
{
    if (event == 0) return;
    event->preset = GWH89_PRESET_RPG7_SUSTAINED;
    event->radial_velocity_mps = 55;
    event->distance_gain_q15 = 24500U;
    event->pitch_scale_q15 = 32767U;
    event->auto_hold_ms = 1200U;
    event->gain_q15 = 15500;
    event->pan_q15 = 0;
    event->instance_key = 0U;
}

void wsse89_rocket_whistle_fx_defaults(wsse89_rocket_whistle_fx_event *event)
{
    gv89_u16 i;
    if (event == 0) return;
    event->instance_key = 0U;
    event->primary_eq_mode = 0U;
    event->output_eq_mode = 0U;
    event->reverb_mode = 0U;
    event->reserved = 0U;
    for (i = 0U; i < GWH89_EQ_BANDS; ++i)
        event->primary_eq_gain_q15[i] = 32767U;
    for (i = 0U; i < GWH89_OUTPUT_EQ_BANDS; ++i)
        event->output_eq_gain_q15[i] = 32767U;
    event->reverb_wet_q15 = 2400U;
    event->reverb_feedback_q15 = 17400U;
    event->reverb_damping_q15 = 7600U;
    event->reverb_tail_ms = 220U;
}

void wsse89_rocket_spin_defaults(wsse89_rocket_spin_start_event *event)
{
    if (event == 0) return;
    event->preset = GRS89_PRESET_RPG7_SUSTAINER;
    event->sustain_ms = 1900U;
    event->release_ms = 240U;
    event->gain_q15 = 6200;
    event->pan_q15 = 0;
    event->instance_key = 0U;
}

void wsse89_gatling_defaults(wsse89_gatling_start_event *event)
{
    if (event == 0) return;
    event->motor_preset = GGM89_PRESET_M134D;
    event->rotator_preset = GGTR89_PRESET_MEDIUM;
    event->gain_q15 = 26000;
    event->motor_gain_q15 = 7600;
    event->rotator_gain_q15 = 8200;
    event->whistle_gain_q15 = 5200;
    event->pan_q15 = 0;
    event->instance_key = 0U;
}

int wsse89_init(wsse89_context *ctx,
                const wsse89_config *config,
                const wsse89_storage *storage)
{
    if (ctx == 0 || config == 0 || storage == 0) return 0;
    if (config->sample_rate == 0U || config->sample_rate > 48000U) return 0;
    if (config->logical_voice_capacity == 0U || storage->logical_voices == 0) return 0;
    if (gwv89_init_ex(&ctx->handler, storage->logical_voices,
                      config->logical_voice_capacity,
                      config->physical_voice_limit,
                      config->sample_rate) != GV89_OK) return 0;
    if (!gssr89_init(&ctx->reports, storage->report_voices,
                     storage->report_capacity, config->sample_rate)) return 0;
    if (!gsse89_init(&ctx->base,
                     storage->casing_voices, storage->casing_capacity,
                     storage->fire_voices, storage->fire_capacity,
                     config->sample_rate)) return 0;
    if (!gsso89_init(&ctx->ordnance,
                     storage->bullet_voices, storage->bullet_capacity,
                     storage->grenade_voices, storage->grenade_capacity,
                     storage->rocket_voices, storage->rocket_capacity,
                     config->sample_rate)) return 0;
    if (gssexp89_init(&ctx->expansion, config->sample_rate,
                      config->seed + 100U,
                      &storage->expansion_memory) != WSOUND89_OK) return 0;
    gssexp89_enable(&ctx->expansion, config->expansion_mask);
    if (!gssw89_init(&ctx->world,
                     storage->projectile_voices, storage->projectile_capacity,
                     storage->impact_voices, storage->impact_capacity,
                     storage->ricochet_voices, storage->ricochet_capacity,
                     config->sample_rate, config->seed + 200U,
                     &storage->world_memory)) return 0;
    ctx->sample_rate = config->sample_rate;
    ctx->seed_counter = config->seed == 0U ? 1U : config->seed;
    if (wsoundmetrics89_init(&ctx->metrics, config->sample_rate) != WSOUND89_OK) return 0;
    {
        gv89_u16 i;
        for (i = 0U; i < WSSE89_MAGAZINE_VOICE_CAPACITY; ++i) {
            if (wmag89_init(&ctx->magazine_voices[i].synth,
                            (int)config->sample_rate,
                            config->seed + 300U + (gv89_u32)i) != WMAG89_OK)
                return 0;
            ctx->magazine_voices[i].instance_key = 0U;
            ctx->magazine_voices[i].age_stamp = 0U;
            ctx->magazine_voices[i].pan_q15 = 0;
            ctx->magazine_voices[i].gain_q15 = 0;
            ctx->magazine_voices[i].active = 0U;
        }
        for (i = 0U; i < WSSE89_ROCKET_WHISTLE_VOICE_CAPACITY; ++i) {
            gwh89_init(&ctx->rocket_whistle_voices[i].synth,
                       (gwh89_s32)config->sample_rate,
                       config->seed + 400U + (gv89_u32)i);
            ctx->rocket_whistle_voices[i].instance_key = 0U;
            ctx->rocket_whistle_voices[i].age_stamp = 0U;
            ctx->rocket_whistle_voices[i].pan_q15 = 0;
            ctx->rocket_whistle_voices[i].gain_q15 = 0;
            ctx->rocket_whistle_voices[i].active = 0U;
        }
        for (i = 0U; i < WSSE89_ROCKET_SPIN_VOICE_CAPACITY; ++i) {
            grs89_params params;
            grs89_params_preset(&params, GRS89_PRESET_RPG7_SUSTAINER);
            params.seed = config->seed + 500U + (gv89_u32)i;
            if (!grs89_init(&ctx->rocket_spin_voices[i].synth,
                            config->sample_rate, &params)) return 0;
            ctx->rocket_spin_voices[i].instance_key = 0U;
            ctx->rocket_spin_voices[i].age_stamp = 0U;
            ctx->rocket_spin_voices[i].pan_q15 = 0;
            ctx->rocket_spin_voices[i].gain_q15 = 0;
            ctx->rocket_spin_voices[i].active = 0U;
        }
        for (i = 0U; i < WSSE89_GATLING_VOICE_CAPACITY; ++i) {
            ggm89_config motor_cfg;
            ggtr89_config rotator_cfg;
            ggm89_config_preset(&motor_cfg, GGM89_PRESET_M134D);
            if (!ggm89_init(&ctx->gatling_voices[i].motor, &motor_cfg,
                            config->sample_rate)) return 0;
            ggtr89_config_preset(&rotator_cfg, (ggtr89_i32)config->sample_rate,
                                 GGTR89_PRESET_MEDIUM);
            if (!ggtr89_init(&ctx->gatling_voices[i].rotator,
                             &rotator_cfg)) return 0;
            ggw89_init(&ctx->gatling_voices[i].whistle,
                       (unsigned long)config->sample_rate);
            ctx->gatling_voices[i].instance_key = 0U;
            ctx->gatling_voices[i].age_stamp = 0U;
            ctx->gatling_voices[i].pan_q15 = 0;
            ctx->gatling_voices[i].gain_q15 = 0;
            ctx->gatling_voices[i].motor_gain_q15 = 0;
            ctx->gatling_voices[i].rotator_gain_q15 = 0;
            ctx->gatling_voices[i].whistle_gain_q15 = 0;
            ctx->gatling_voices[i].active = 0U;
            ctx->gatling_voices[i].firing = 0U;
        }
    }
    ctx->auxiliary_age_counter = 1U;
    ctx->has_last_dna = 0U;
    ctx->initialized = 1U;
    return 1;
}

void wsse89_reset(wsse89_context *ctx)
{
    if (ctx == 0 || !ctx->initialized) return;
    gv89_stop_all(&ctx->handler.voices, 0U);
    gssr89_reset(&ctx->reports, &ctx->handler);
    gsse89_reset(&ctx->base, &ctx->handler);
    gsso89_reset(&ctx->ordnance, &ctx->handler);
    gssw89_reset(&ctx->world, &ctx->handler);
    wsoundmetrics89_reset(&ctx->metrics);
    {
        gv89_u16 i;
        for (i = 0U; i < WSSE89_MAGAZINE_VOICE_CAPACITY; ++i) {
            wmag89_reset(&ctx->magazine_voices[i].synth);
            ctx->magazine_voices[i].active = 0U;
            ctx->magazine_voices[i].instance_key = 0U;
        }
        for (i = 0U; i < WSSE89_ROCKET_WHISTLE_VOICE_CAPACITY; ++i) {
            gwh89_reset(&ctx->rocket_whistle_voices[i].synth);
            ctx->rocket_whistle_voices[i].active = 0U;
            ctx->rocket_whistle_voices[i].instance_key = 0U;
        }
        for (i = 0U; i < WSSE89_ROCKET_SPIN_VOICE_CAPACITY; ++i) {
            grs89_reset(&ctx->rocket_spin_voices[i].synth);
            ctx->rocket_spin_voices[i].active = 0U;
            ctx->rocket_spin_voices[i].instance_key = 0U;
        }
        for (i = 0U; i < WSSE89_GATLING_VOICE_CAPACITY; ++i) {
            ggm89_reset(&ctx->gatling_voices[i].motor);
            ggtr89_reset(&ctx->gatling_voices[i].rotator);
            ggw89_reset(&ctx->gatling_voices[i].whistle);
            ctx->gatling_voices[i].active = 0U;
            ctx->gatling_voices[i].firing = 0U;
            ctx->gatling_voices[i].instance_key = 0U;
        }
    }
    ctx->auxiliary_age_counter = 1U;
    ctx->has_last_dna = 0U;
}


static void wsse89_acoustic_source(wsse89_context *ctx, wsounda89_source source)
{
    wsounda89_path_params path;
    if (ctx == 0) return;
    path = ctx->world.acoustic.path;
    path.source = source;
    (void)gssw89_set_acoustic_path(&ctx->world, &path);
}
static int wsse89_gv_result(gv89_result result)
{
    return result == GV89_OK ? WSSE89_OK : WSSE89_EVOICE;
}

int wsse89_dispatch(wsse89_context *ctx,
                    const wsse89_event *event,
                    gv89_handle *out_handle)
{
    gv89_u32 seed;
    gv89_result gv_result;
    if (ctx == 0 || event == 0 || !ctx->initialized) return WSSE89_EINVAL;
    wsse89_invalid_handle(out_handle);
    seed = wsse89_next_seed(ctx, event->seed);
    switch (event->type) {
    case WSSE89_EVENT_REPORT:
        if (out_handle == 0) return WSSE89_EINVAL;
        wsse89_acoustic_source(ctx, WSOUNDA89_SOURCE_REPORT);
        gv_result = gssr89_play(&ctx->reports, &ctx->handler,
                                &event->data.report, seed, out_handle);
        if (gv_result == GV89_OK)
            gssw89_trigger_pressure(&ctx->world, WSOUNDA89_PRESSURE_REPORT,
                                    24500U, event->data.report.pan_q15, seed);
        return wsse89_gv_result(gv_result);
    case WSSE89_EVENT_CASING:
        if (out_handle == 0) return WSSE89_EINVAL;
        return wsse89_gv_result(gsse89_play_casing(&ctx->base, &ctx->handler,
                                                    &event->data.casing, seed,
                                                    out_handle));
    case WSSE89_EVENT_FIRE_START:
        if (out_handle == 0) return WSSE89_EINVAL;
        return wsse89_gv_result(gsse89_play_fire(&ctx->base, &ctx->handler,
                                                 &event->data.fire, seed,
                                                 out_handle));
    case WSSE89_EVENT_BULLET_PASS:
        if (out_handle == 0) return WSSE89_EINVAL;
        return wsse89_gv_result(gsso89_play_bullet(&ctx->ordnance, &ctx->handler,
                                                    &event->data.bullet, seed,
                                                    out_handle));
    case WSSE89_EVENT_GRENADE_BLAST:
        if (out_handle == 0) return WSSE89_EINVAL;
        wsse89_acoustic_source(ctx, WSOUNDA89_SOURCE_EXPLOSION);
        gv_result = gsso89_play_grenade(&ctx->ordnance, &ctx->handler,
                                        &event->data.grenade, seed, out_handle);
        if (gv_result == GV89_OK)
            gssw89_trigger_pressure(&ctx->world, WSOUNDA89_PRESSURE_GRENADE,
                                    (wsound89_u16)event->data.grenade.intensity_q15,
                                    event->data.grenade.common.pan_q15, seed);
        return wsse89_gv_result(gv_result);
    case WSSE89_EVENT_ROCKET_BLAST:
        if (out_handle == 0) return WSSE89_EINVAL;
        wsse89_acoustic_source(ctx, WSOUNDA89_SOURCE_EXPLOSION);
        gv_result = gsso89_play_rocket(&ctx->ordnance, &ctx->handler,
                                       &event->data.rocket, seed, out_handle);
        if (gv_result == GV89_OK)
            gssw89_trigger_pressure(&ctx->world, WSOUNDA89_PRESSURE_ROCKET,
                                    31000U, event->data.rocket.common.pan_q15, seed);
        return wsse89_gv_result(gv_result);
    case WSSE89_EVENT_PROJECTILE:
        if (out_handle == 0) return WSSE89_EINVAL;
        wsse89_acoustic_source(ctx, WSOUNDA89_SOURCE_PROJECTILE);
        return wsse89_gv_result(gssw89_play_projectile(&ctx->world, &ctx->handler,
                                                        &event->data.projectile,
                                                        seed, out_handle));
    case WSSE89_EVENT_IMPACT:
        if (out_handle == 0) return WSSE89_EINVAL;
        wsse89_acoustic_source(ctx, WSOUNDA89_SOURCE_IMPACT);
        gv_result = gssw89_play_impact(&ctx->world, &ctx->handler,
                                       &event->data.impact, seed, out_handle);
        if (gv_result == GV89_OK)
            gssw89_trigger_pressure(&ctx->world, WSOUNDA89_PRESSURE_IMPACT,
                                    event->data.impact.energy_q15,
                                    event->data.impact.common.pan_q15, seed);
        return wsse89_gv_result(gv_result);
    case WSSE89_EVENT_RICOCHET:
        if (out_handle == 0) return WSSE89_EINVAL;
        wsse89_acoustic_source(ctx, WSOUNDA89_SOURCE_PROJECTILE);
        return wsse89_gv_result(gssw89_play_ricochet(&ctx->world, &ctx->handler,
                                                      &event->data.ricochet, seed,
                                                      out_handle));
    case WSSE89_EVENT_STOP_HANDLE:
        gv_result = gv89_stop(&ctx->handler.voices, event->data.stop.handle,
                              event->data.stop.release_ms);
        return wsse89_gv_result(gv_result);
    case WSSE89_EVENT_SET_HANDLE_SPATIAL:
        gv_result = gwv89_set_event_spatial(&ctx->handler,
                                             event->data.spatial.handle,
                                             event->data.spatial.pan_q15,
                                             event->data.spatial.distance_q15,
                                             event->data.spatial.occlusion_q15,
                                             event->data.spatial.focus_q15);
        return wsse89_gv_result(gv_result);
    case WSSE89_EVENT_EXPANSION_SHOT:
        gssexp89_trigger_shot(&ctx->expansion,
                              event->data.expansion_shot.device,
                              event->data.expansion_shot.energy_q15,
                              event->data.expansion_shot.distance_q15,
                              seed);
        return WSSE89_OK;
    case WSSE89_EVENT_BELT_START:
        gssexp89_start_belt(&ctx->expansion, event->data.belt.rpm,
                            event->data.belt.tension_q15);
        return WSSE89_OK;
    case WSSE89_EVENT_BELT_STOP:
        gssexp89_stop_belt(&ctx->expansion);
        return WSSE89_OK;
    case WSSE89_EVENT_FRICTION_START:
        gssexp89_start_friction(&ctx->expansion,
                                event->data.friction.material,
                                event->data.friction.speed_q15,
                                event->data.friction.pressure_q15,
                                event->data.friction.roughness_q15);
        return WSSE89_OK;
    case WSSE89_EVENT_FRICTION_STOP:
        gssexp89_stop_friction(&ctx->expansion);
        return WSSE89_OK;
    case WSSE89_EVENT_AERO_START:
        gssexp89_start_aero(&ctx->expansion, event->data.aero.mode,
                            event->data.aero.speed_q15,
                            event->data.aero.size_q15,
                            event->data.aero.duration_frames);
        return WSSE89_OK;
    case WSSE89_EVENT_AERO_STOP:
        wsoundaero89_set_gate(&ctx->expansion.aero, 0);
        return WSSE89_OK;
    case WSSE89_EVENT_PARTICLES:
        return wsoundparticles89_trigger(&ctx->expansion.particles,
                                          event->data.particles.material,
                                          event->data.particles.density_q15,
                                          event->data.particles.energy_q15) == WSOUND89_OK ? WSSE89_OK : WSSE89_ESYNTH;
    case WSSE89_EVENT_AMMO:
        return wsoundammo89_trigger(&ctx->expansion.ammo,
                                     event->data.ammo.type,
                                     event->data.ammo.fill_q15,
                                     event->data.ammo.motion_q15) == WSOUND89_OK ? WSSE89_OK : WSSE89_ESYNTH;
    case WSSE89_EVENT_THERMAL_HEAT:
        wsoundthermal89_add_heat(&ctx->expansion.thermal,
                                 event->data.thermal.material,
                                 event->data.thermal.heat_q15);
        return WSSE89_OK;
    case WSSE89_EVENT_LISTENER_EXPOSE:
        wsoundlistener89_expose(&ctx->expansion.listener,
                                event->data.listener.energy_q15,
                                event->data.listener.distance_q15,
                                event->data.listener.protection);
        return WSSE89_OK;
    case WSSE89_EVENT_MASK_TRIGGER:
        wsoundmask89_trigger(&ctx->expansion.mask,
                             event->data.mask_energy_q15);
        return WSSE89_OK;
    case WSSE89_EVENT_SPATIAL_AZIMUTH:
        wsoundspatial89_set_azimuth(&ctx->expansion.spatial,
                                    event->data.azimuth.pan_q15,
                                    event->data.azimuth.itd_samples,
                                    event->data.azimuth.shadow_q15);
        return WSSE89_OK;
    case WSSE89_EVENT_PORTAL_PATH:
        return wsoundportal89_set_path(&ctx->expansion.portal,
                                        event->data.portal.delay_samples,
                                        event->data.portal.low_q15,
                                        event->data.portal.mid_q15,
                                        event->data.portal.high_q15,
                                        event->data.portal.opening_q15) == WSOUND89_OK ? WSSE89_OK : WSSE89_ESYNTH;
    case WSSE89_EVENT_OUTDOOR_MIX:
        gssexp89_set_outdoor_mix(&ctx->expansion,
                                  event->data.outdoor_mix.send_q15,
                                  event->data.outdoor_mix.wet_q15);
        return WSSE89_OK;
    case WSSE89_EVENT_ACTION_START:
        return gssw89_action_trigger(&ctx->world, event->data.action.action,
                                      event->data.action.speed_q16) == WSOUND89_OK ? WSSE89_OK : WSSE89_ESYNTH;
    case WSSE89_EVENT_RECEIVER_EXCITE:
        gssw89_excite_receiver(&ctx->world, event->data.receiver_impulse);
        return WSSE89_OK;
    case WSSE89_EVENT_ACOUSTIC_ENABLE:
        gssw89_enable_acoustic(&ctx->world, event->data.acoustic_enabled != 0U);
        return WSSE89_OK;
    case WSSE89_EVENT_ACOUSTIC_PROFILE:
        gssw89_set_acoustic_profile(&ctx->world, event->data.acoustic_profile);
        return WSSE89_OK;
    case WSSE89_EVENT_ACOUSTIC_PATH:
        return gssw89_set_acoustic_path(&ctx->world, &event->data.acoustic_path) == WSOUND89_OK ? WSSE89_OK : WSSE89_ESYNTH;
    case WSSE89_EVENT_ACOUSTIC_MATERIAL:
        return gssw89_set_acoustic_material(&ctx->world,
                                             event->data.acoustic_material.material,
                                             event->data.acoustic_material.thickness_q15) == WSOUND89_OK ? WSSE89_OK : WSSE89_ESYNTH;
    case WSSE89_EVENT_ACOUSTIC_SPACE:
        return gssw89_set_acoustic_space(&ctx->world, event->data.acoustic_space) == WSOUND89_OK ? WSSE89_OK : WSSE89_ESYNTH;
    case WSSE89_EVENT_ACOUSTIC_PORTAL:
        return gssw89_set_acoustic_portal(&ctx->world, &event->data.acoustic_portal) == WSOUND89_OK ? WSSE89_OK : WSSE89_ESYNTH;
    case WSSE89_EVENT_PRESSURE_TRIGGER:
        gssw89_trigger_pressure(&ctx->world, event->data.pressure.kind,
                                event->data.pressure.energy_q15,
                                event->data.pressure.pan_q15, seed);
        return WSSE89_OK;
    case WSSE89_EVENT_WEAPON_PROFILE:
        return gssw89_dna_set_profile(&ctx->world, &event->data.weapon_profile) == WSOUND89_OK ? WSSE89_OK : WSSE89_EINVAL;
    case WSSE89_EVENT_WEAPON_MODE:
        if (gssw89_dna_set_mode(&ctx->world, event->data.weapon_mode) != WSOUND89_OK) return WSSE89_EINVAL;
        gssw89_set_acoustic_profile(&ctx->world,
            event->data.weapon_mode == WSOUNDDNA89_MODE_REALISTIC ? WSOUNDA89_REALISTIC :
            (event->data.weapon_mode == WSOUNDDNA89_MODE_HYBRID ? WSOUNDA89_HYBRID : WSOUNDA89_CINEMATIC));
        if (gwv89_apply_mix_profile(&ctx->handler,
            event->data.weapon_mode == WSOUNDDNA89_MODE_REALISTIC ? GWV89_MIX_REALISTIC :
            (event->data.weapon_mode == WSOUNDDNA89_MODE_HYBRID ? GWV89_MIX_HYBRID : GWV89_MIX_CINEMATIC)) != GV89_OK)
            return WSSE89_ESYNTH;
        return WSSE89_OK;
    case WSSE89_EVENT_WEAPON_FIRE:
        {
            gssr89_params params;
            wsounddna89_shot shot;
            wsoundmuzzledevice89_type device;
            if (out_handle == 0) return WSSE89_EINVAL;
            if (gssw89_dna_next_profiled(&ctx->world, &shot) != WSOUND89_OK) return WSSE89_ESYNTH;
            gssr89_apply_dna(&params, &ctx->world.dna.profile, &shot, ctx->world.dna.mode);
            params.pan_q15 = event->data.weapon_fire.pan_q15;
            params.distance_q15 = event->data.weapon_fire.distance_q15;
            params.occlusion_q15 = event->data.weapon_fire.occlusion_q15;
            params.focus_q15 = event->data.weapon_fire.focus_q15;
            params.gain_q15 = event->data.weapon_fire.gain_q15;
            params.priority_bias = event->data.weapon_fire.priority_bias;
            params.instance_key = event->data.weapon_fire.instance_key;
            params.instance_limit = event->data.weapon_fire.instance_limit;
            wsse89_acoustic_source(ctx, WSOUNDA89_SOURCE_REPORT);
            gv_result = gssr89_play(&ctx->reports, &ctx->handler, &params, seed ^ shot.seed, out_handle);
            if (gv_result != GV89_OK) return wsse89_gv_result(gv_result);
            ctx->last_dna_shot = shot;
            ctx->has_last_dna = 1U;
            gssw89_trigger_pressure(&ctx->world, WSOUNDA89_PRESSURE_REPORT,
                event->data.weapon_fire.pressure_energy_q15 == 0U ? shot.pressure_q15 : event->data.weapon_fire.pressure_energy_q15,
                params.pan_q15, seed);
            gssw89_excite_receiver(&ctx->world, (wsound89_i16)shot.receiver_q15);
            device = ctx->world.dna.profile.suppressor_q15 > 8000U ? WSOUNDMUZZLEDEVICE89_SUPPRESSOR :
                     (ctx->world.dna.profile.muzzle_brake_q15 > 5000U ? WSOUNDMUZZLEDEVICE89_BRAKE : WSOUNDMUZZLEDEVICE89_BARE);
            gssexp89_trigger_shot(&ctx->expansion, device, shot.gas_q15, params.distance_q15, seed + 1U);
            return WSSE89_OK;
        }
    case WSSE89_EVENT_MAGAZINE_ACTION:
        {
            wsse89_magazine_voice *voice;
            gv89_u32 key;
            if (event->data.magazine.preset < 0 ||
                event->data.magazine.preset >= WMAG89_PRESET_COUNT ||
                event->data.magazine.action < 0 ||
                event->data.magazine.action >= WMAG89_ACTION_COUNT ||
                event->data.magazine.velocity_q15 > 32767U)
                return WSSE89_EINVAL;
            voice = wsse89_alloc_magazine(ctx);
            wmag89_reset(&voice->synth);
            if (wmag89_set_preset(&voice->synth,
                                   (int)event->data.magazine.preset) != WMAG89_OK)
                return WSSE89_ESYNTH;
            key = event->data.magazine.instance_key == 0U ? seed :
                  event->data.magazine.instance_key;
            voice->instance_key = key;
            voice->pan_q15 = wsse89_clamp_pan(event->data.magazine.pan_q15);
            voice->gain_q15 = wsse89_clamp_gain(event->data.magazine.gain_q15);
            voice->age_stamp = ctx->auxiliary_age_counter++;
            if (wmag89_trigger(&voice->synth,
                               (int)event->data.magazine.action,
                               (int)event->data.magazine.velocity_q15) != WMAG89_OK)
                return WSSE89_ESYNTH;
            voice->active = 1U;
            wsse89_acoustic_source(ctx, WSOUNDA89_SOURCE_MECHANISM);
            return WSSE89_OK;
        }
    case WSSE89_EVENT_ROCKET_WHISTLE_START:
        {
            wsse89_rocket_whistle_voice *voice;
            gv89_u32 key;
            if (event->data.rocket_whistle_start.preset < 0 ||
                event->data.rocket_whistle_start.preset >= GWH89_PRESET_COUNT ||
                event->data.rocket_whistle_start.distance_gain_q15 > 32767U ||
                event->data.rocket_whistle_start.pitch_scale_q15 < 8192U)
                return WSSE89_EINVAL;
            key = event->data.rocket_whistle_start.instance_key == 0U ? seed :
                  event->data.rocket_whistle_start.instance_key;
            voice = wsse89_alloc_whistle(ctx, key);
            gwh89_reset(&voice->synth);
            gwh89_trigger_preset(&voice->synth,
                                 (gwh89_s32)event->data.rocket_whistle_start.preset);
            gwh89_set_motion(&voice->synth,
                             (gwh89_s32)event->data.rocket_whistle_start.radial_velocity_mps,
                             (gwh89_s32)event->data.rocket_whistle_start.distance_gain_q15);
            gwh89_set_pitch_scale_q15(&voice->synth,
                                      (gwh89_s32)event->data.rocket_whistle_start.pitch_scale_q15);
            gwh89_set_auto_hold_ms(&voice->synth,
                                   (gwh89_s32)event->data.rocket_whistle_start.auto_hold_ms);
            voice->instance_key = key;
            voice->pan_q15 = wsse89_clamp_pan(event->data.rocket_whistle_start.pan_q15);
            voice->gain_q15 = wsse89_clamp_gain(event->data.rocket_whistle_start.gain_q15);
            voice->age_stamp = ctx->auxiliary_age_counter++;
            voice->active = 1U;
            wsse89_acoustic_source(ctx, WSOUNDA89_SOURCE_PROJECTILE);
            return WSSE89_OK;
        }
    case WSSE89_EVENT_ROCKET_WHISTLE_MOTION:
        {
            wsse89_rocket_whistle_voice *voice;
            voice = wsse89_find_whistle(ctx,
                        event->data.rocket_whistle_motion.instance_key);
            if (voice == 0) return WSSE89_EINVAL;
            if (event->data.rocket_whistle_motion.distance_gain_q15 > 32767U)
                return WSSE89_EINVAL;
            gwh89_set_motion(&voice->synth,
                             (gwh89_s32)event->data.rocket_whistle_motion.radial_velocity_mps,
                             (gwh89_s32)event->data.rocket_whistle_motion.distance_gain_q15);
            voice->pan_q15 = wsse89_clamp_pan(event->data.rocket_whistle_motion.pan_q15);
            voice->gain_q15 = wsse89_clamp_gain(event->data.rocket_whistle_motion.gain_q15);
            return WSSE89_OK;
        }
    case WSSE89_EVENT_ROCKET_WHISTLE_FX:
        {
            wsse89_rocket_whistle_voice *voice;
            gv89_u16 i;
            const wsse89_rocket_whistle_fx_event *fx;
            fx = &event->data.rocket_whistle_fx;
            voice = wsse89_find_whistle(ctx, fx->instance_key);
            if (voice == 0) return WSSE89_EINVAL;
            if (fx->primary_eq_mode > 2U || fx->output_eq_mode > 2U ||
                fx->reverb_mode > 2U || fx->reverb_wet_q15 > 32767U ||
                fx->reverb_feedback_q15 > 32767U ||
                fx->reverb_damping_q15 > 32767U)
                return WSSE89_EINVAL;
            if (fx->primary_eq_mode == 1U) {
                gwh89_set_eq_enabled(&voice->synth, 0);
            } else if (fx->primary_eq_mode == 2U) {
                gwh89_set_eq_enabled(&voice->synth, 1);
                for (i = 0U; i < GWH89_EQ_BANDS; ++i)
                    gwh89_set_eq_band_gain_q15(&voice->synth, (gwh89_s32)i,
                        (gwh89_s32)fx->primary_eq_gain_q15[i]);
            }
            if (fx->output_eq_mode == 1U) {
                gwh89_set_output_eq_enabled(&voice->synth, 0);
            } else if (fx->output_eq_mode == 2U) {
                gwh89_set_output_eq_enabled(&voice->synth, 1);
                for (i = 0U; i < GWH89_OUTPUT_EQ_BANDS; ++i)
                    gwh89_set_output_eq_band_gain_q15(&voice->synth,
                        (gwh89_s32)i,
                        (gwh89_s32)fx->output_eq_gain_q15[i]);
            }
            if (fx->reverb_mode == 1U) {
                gwh89_set_reverb_enabled(&voice->synth, 0);
            } else if (fx->reverb_mode == 2U) {
                gwh89_set_reverb(&voice->synth, 1,
                    (gwh89_s32)fx->reverb_wet_q15,
                    (gwh89_s32)fx->reverb_feedback_q15,
                    (gwh89_s32)fx->reverb_damping_q15,
                    (gwh89_s32)fx->reverb_tail_ms);
            }
            return WSSE89_OK;
        }
    case WSSE89_EVENT_ROCKET_WHISTLE_RELEASE:
        {
            wsse89_rocket_whistle_voice *voice;
            voice = wsse89_find_whistle(ctx,
                        event->data.rocket_whistle_control.instance_key);
            if (voice == 0) return WSSE89_EINVAL;
            gwh89_release(&voice->synth);
            return WSSE89_OK;
        }
    case WSSE89_EVENT_ROCKET_WHISTLE_STOP:
        {
            wsse89_rocket_whistle_voice *voice;
            voice = wsse89_find_whistle(ctx,
                        event->data.rocket_whistle_control.instance_key);
            if (voice == 0) return WSSE89_EINVAL;
            gwh89_reset(&voice->synth);
            voice->active = 0U;
            return WSSE89_OK;
        }
    case WSSE89_EVENT_ROCKET_SPIN_START:
        {
            wsse89_rocket_spin_voice *voice;
            grs89_params params;
            gv89_u32 key;
            if (event->data.rocket_spin_start.preset < 0 ||
                event->data.rocket_spin_start.preset > GRS89_PRESET_FAST_MISSILE)
                return WSSE89_EINVAL;
            key = event->data.rocket_spin_start.instance_key == 0U ? seed :
                  event->data.rocket_spin_start.instance_key;
            voice = wsse89_alloc_spin(ctx, key);
            grs89_params_preset(&params,
                                (int)event->data.rocket_spin_start.preset);
            params.seed = seed;
            if (event->data.rocket_spin_start.sustain_ms != 0U)
                params.sustain_ms = event->data.rocket_spin_start.sustain_ms;
            if (event->data.rocket_spin_start.release_ms != 0U)
                params.release_ms = event->data.rocket_spin_start.release_ms;
            if (!grs89_init(&voice->synth, ctx->sample_rate, &params))
                return WSSE89_ESYNTH;
            grs89_trigger(&voice->synth);
            voice->instance_key = key;
            voice->pan_q15 = wsse89_clamp_pan(event->data.rocket_spin_start.pan_q15);
            voice->gain_q15 = wsse89_clamp_gain(event->data.rocket_spin_start.gain_q15);
            voice->age_stamp = ctx->auxiliary_age_counter++;
            voice->active = 1U;
            wsse89_acoustic_source(ctx, WSOUNDA89_SOURCE_PROJECTILE);
            return WSSE89_OK;
        }
    case WSSE89_EVENT_ROCKET_SPIN_MOTION:
        {
            wsse89_rocket_spin_voice *voice;
            voice = wsse89_find_spin(ctx,
                    event->data.rocket_spin_motion.instance_key);
            if (voice == 0) return WSSE89_EINVAL;
            voice->pan_q15 = wsse89_clamp_pan(event->data.rocket_spin_motion.pan_q15);
            voice->gain_q15 = wsse89_clamp_gain(event->data.rocket_spin_motion.gain_q15);
            return WSSE89_OK;
        }
    case WSSE89_EVENT_ROCKET_SPIN_STOP:
        {
            wsse89_rocket_spin_voice *voice;
            voice = wsse89_find_spin(ctx,
                    event->data.rocket_spin_control.instance_key);
            if (voice == 0) return WSSE89_EINVAL;
            grs89_stop(&voice->synth);
            return WSSE89_OK;
        }
    case WSSE89_EVENT_GATLING_START:
        {
            wsse89_gatling_voice *voice;
            ggm89_config motor_cfg;
            ggtr89_config rotator_cfg;
            gv89_u32 key;
            if (event->data.gatling_start.motor_preset < 0 ||
                event->data.gatling_start.motor_preset >= GGM89_PRESET_COUNT ||
                event->data.gatling_start.rotator_preset < GGTR89_PRESET_LIGHT ||
                event->data.gatling_start.rotator_preset > GGTR89_PRESET_HEAVY)
                return WSSE89_EINVAL;
            key = event->data.gatling_start.instance_key == 0U ? seed :
                  event->data.gatling_start.instance_key;
            voice = wsse89_alloc_gatling(ctx, key);
            ggm89_config_preset(&motor_cfg,
                               (int)event->data.gatling_start.motor_preset);
            if (!ggm89_init(&voice->motor, &motor_cfg, ctx->sample_rate))
                return WSSE89_ESYNTH;
            ggtr89_config_preset(&rotator_cfg, (ggtr89_i32)ctx->sample_rate,
                                 (ggtr89_i32)event->data.gatling_start.rotator_preset);
            if (!ggtr89_init(&voice->rotator, &rotator_cfg))
                return WSSE89_ESYNTH;
            ggw89_init(&voice->whistle, (unsigned long)ctx->sample_rate);
            ggm89_start(&voice->motor);
            ggtr89_start(&voice->rotator);
            ggw89_trigger(&voice->whistle, GGW89_EVENT_SPIN_UP);
            voice->instance_key = key;
            voice->pan_q15 = wsse89_clamp_pan(event->data.gatling_start.pan_q15);
            voice->gain_q15 = wsse89_clamp_gain(event->data.gatling_start.gain_q15);
            voice->motor_gain_q15 = wsse89_clamp_gain(event->data.gatling_start.motor_gain_q15);
            voice->rotator_gain_q15 = wsse89_clamp_gain(event->data.gatling_start.rotator_gain_q15);
            voice->whistle_gain_q15 = wsse89_clamp_gain(event->data.gatling_start.whistle_gain_q15);
            voice->age_stamp = ctx->auxiliary_age_counter++;
            voice->active = 1U;
            voice->firing = 0U;
            wsse89_acoustic_source(ctx, WSOUNDA89_SOURCE_MECHANISM);
            return WSSE89_OK;
        }
    case WSSE89_EVENT_GATLING_FIRE_START:
        {
            wsse89_gatling_voice *voice;
            voice = wsse89_find_gatling(ctx,
                    event->data.gatling_fire.instance_key);
            if (voice == 0) return WSSE89_EINVAL;
            ggm89_set_firing_load_q15(&voice->motor,
                    wsse89_clamp_gain(event->data.gatling_fire.firing_load_q15));
            ggtr89_force_loop(&voice->rotator);
            ggw89_trigger_continuous(&voice->whistle, GGW89_EVENT_FIRE_LOOP);
            voice->firing = 1U;
            return WSSE89_OK;
        }
    case WSSE89_EVENT_GATLING_FIRE_STOP:
        {
            wsse89_gatling_voice *voice;
            voice = wsse89_find_gatling(ctx,
                    event->data.gatling_control.instance_key);
            if (voice == 0) return WSSE89_EINVAL;
            ggm89_set_firing_load_q15(&voice->motor, 0);
            ggm89_stop(&voice->motor);
            ggtr89_stop(&voice->rotator);
            ggw89_trigger(&voice->whistle, GGW89_EVENT_SPIN_DOWN);
            voice->firing = 0U;
            return WSSE89_OK;
        }
    case WSSE89_EVENT_GATLING_STOP:
        {
            wsse89_gatling_voice *voice;
            voice = wsse89_find_gatling(ctx,
                    event->data.gatling_control.instance_key);
            if (voice == 0) return WSSE89_EINVAL;
            ggm89_reset(&voice->motor);
            ggtr89_reset(&voice->rotator);
            ggw89_reset(&voice->whistle);
            voice->active = 0U;
            voice->firing = 0U;
            return WSSE89_OK;
        }
    case WSSE89_EVENT_METRICS_ENABLE:
        wsoundmetrics89_set_enabled(&ctx->metrics, event->data.metrics_enabled != 0U);
        return WSSE89_OK;
    case WSSE89_EVENT_METRICS_RESET:
        wsoundmetrics89_reset(&ctx->metrics);
        return WSSE89_OK;
    case WSSE89_EVENT_NONE:
        return WSSE89_OK;
    default:
        return WSSE89_EUNSUPPORTED;
    }
}

int wsse89_event_sink(void *user,
                      const wsse89_event *event,
                      gv89_handle *out_handle)
{
    return wsse89_dispatch((wsse89_context *)user, event, out_handle);
}

const char *wsse89_event_name(wsse89_event_type type)
{
    static const char *names[WSSE89_EVENT_COUNT] = {
        "none", "report", "casing", "fire_start", "bullet_pass",
        "grenade_blast", "rocket_blast", "projectile", "impact",
        "ricochet", "stop_handle", "set_handle_spatial",
        "expansion_shot", "belt_start", "belt_stop", "friction_start",
        "friction_stop", "aero_start", "aero_stop", "particles", "ammo",
        "thermal_heat", "listener_expose", "mask_trigger",
        "spatial_azimuth", "portal_path", "outdoor_mix", "action_start",
        "receiver_excite", "acoustic_enable", "acoustic_profile",
        "acoustic_path", "acoustic_material", "acoustic_space",
        "acoustic_portal", "pressure_trigger", "weapon_profile",
        "weapon_mode", "weapon_fire", "metrics_enable", "metrics_reset",
        "magazine_action", "rocket_whistle_start",
        "rocket_whistle_motion", "rocket_whistle_fx",
        "rocket_whistle_release", "rocket_whistle_stop",
        "rocket_spin_start", "rocket_spin_motion", "rocket_spin_stop",
        "gatling_start", "gatling_fire_start", "gatling_fire_stop",
        "gatling_stop"
    };
    if ((int)type < 0 || type >= WSSE89_EVENT_COUNT) return "unknown";
    return names[(int)type];
}

void wsse89_process_stereo_sample(wsse89_context *ctx,
                                  gv89_s16 *left,
                                  gv89_s16 *right)
{
    gv89_s16 dry_l;
    gv89_s16 dry_r;
    gv89_s16 mono;
    gv89_s16 exp_l;
    gv89_s16 exp_r;
    gv89_s16 world_l;
    gv89_s16 world_r;
    gv89_s32 side_l;
    gv89_s32 side_r;
    if (ctx == 0 || left == 0 || right == 0 || !ctx->initialized) return;
    dry_l = 0;
    dry_r = 0;
    gwv89_process_stereo_sample(&ctx->handler, &dry_l, &dry_r);
    wsse89_process_auxiliary(ctx, &dry_l, &dry_r);
    mono = (gv89_s16)(((gv89_s32)dry_l + (gv89_s32)dry_r) / 2);
    exp_l = 0;
    exp_r = 0;
    gssexp89_process_mono(&ctx->expansion, mono, &exp_l, &exp_r);
    side_l = (gv89_s32)dry_l - (gv89_s32)mono;
    side_r = (gv89_s32)dry_r - (gv89_s32)mono;
    exp_l = wsse89_sat16((gv89_s32)exp_l + side_l);
    exp_r = wsse89_sat16((gv89_s32)exp_r + side_r);
    gssw89_process_post_stereo(&ctx->world, exp_l, exp_r, &world_l, &world_r);
    *left = world_l;
    *right = world_r;
    wsoundmetrics89_push_stereo(&ctx->metrics, world_l, world_r);
}

gv89_u32 wsse89_render_stereo(wsse89_context *ctx,
                               gv89_s16 *interleaved_stereo,
                               gv89_u32 frames,
                               int accumulate)
{
    gv89_u32 i;
    gv89_s16 left;
    gv89_s16 right;
    if (ctx == 0 || interleaved_stereo == 0) return 0U;
    for (i = 0U; i < frames; ++i) {
        wsse89_process_stereo_sample(ctx, &left, &right);
        if (accumulate) {
            left = wsse89_sat16((gv89_s32)interleaved_stereo[i * 2U] + left);
            right = wsse89_sat16((gv89_s32)interleaved_stereo[i * 2U + 1U] + right);
        }
        interleaved_stereo[i * 2U] = left;
        interleaved_stereo[i * 2U + 1U] = right;
    }
    return frames;
}

wsound89_result wsse89_advance_action(wsse89_context *ctx,
                                      wsound89_u32 frames,
                                      wsoundaction89_event *events,
                                      wsound89_u16 capacity,
                                      wsound89_u16 *written)
{
    if (ctx == 0) return WSOUND89_EINVAL;
    return gssw89_action_advance(&ctx->world, frames, events, capacity, written);
}

int wsse89_get_last_dna(const wsse89_context *ctx, wsounddna89_shot *shot)
{
    if (ctx == 0 || shot == 0 || !ctx->has_last_dna) return 0;
    *shot = ctx->last_dna_shot;
    return 1;
}

wsound89_result wsse89_get_metrics(const wsse89_context *ctx,
                                    wsoundmetrics89_result *result)
{
    if (ctx == 0) return WSOUND89_EINVAL;
    return wsoundmetrics89_finish(&ctx->metrics, result);
}

wsound89_u16 wsse89_validate_metrics(const wsse89_context *ctx,
                                      wsoundmetrics89_target *target,
                                      wsoundmetrics89_result *result)
{
    if (ctx == 0 || target == 0 || result == 0) return 0xFFFFU;
    if (wsoundmetrics89_finish(&ctx->metrics, result) != WSOUND89_OK) return 0xFFFFU;
    wsoundmetrics89_target_defaults(&ctx->world.dna.profile, ctx->world.dna.mode, target);
    return wsoundmetrics89_validate(result, target);
}

void wsse89_get_stats(const wsse89_context *ctx, gv89_stats *stats)
{
    if (ctx == 0 || stats == 0) return;
    gwv89_get_stats(&ctx->handler, stats);
}

gv89_u32 wsse89_context_bytes(void) { return (gv89_u32)sizeof(wsse89_context); }
