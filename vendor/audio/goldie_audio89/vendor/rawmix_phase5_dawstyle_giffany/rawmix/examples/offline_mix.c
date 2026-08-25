#include <stdio.h>
#include "rawmix.h"
#include "example_wav.h"

#define ENGINE_SR 48000U
#define OUTPUT_SECONDS 3U
#define OUTPUT_FRAMES (ENGINE_SR * OUTPUT_SECONDS)
#define BLOCK_FRAMES 256U
#define BED_SR 22050U
#define BED_FRAMES (BED_SR * 2U)
#define LEAD_SR 44100U
#define LEAD_FRAMES (LEAD_SR * 1U)

static rm_s16 g_bed_mono[BED_FRAMES];
static rm_s16 g_lead_stereo[LEAD_FRAMES * 2U];
static rm_s16 g_output[OUTPUT_FRAMES * 2U];

static rm_u32 calc_step_q16(rm_u32 hz, rm_u32 sample_rate)
{
    return (hz * 65536U) / sample_rate;
}

static rm_s16 triangle_from_phase(rm_u32 phase_q16)
{
    rm_u32 p;
    rm_s32 tri;

    p = (phase_q16 >> 16) & 0xFFFFU;
    if (p < 32768U) {
        tri = (rm_s32)p;
    } else {
        tri = (rm_s32)(65535U - p);
    }
    tri -= 16384;
    tri *= 2;
    return (rm_s16)tri;
}

static rm_s16 square_from_phase(rm_u32 phase_q16, rm_s16 amplitude)
{
    rm_u32 p;
    p = (phase_q16 >> 16) & 0xFFFFU;
    if (p < 32768U) {
        return amplitude;
    }
    return (rm_s16)(-amplitude);
}

static void build_sources(void)
{
    rm_u32 i;
    rm_u32 phase_a;
    rm_u32 phase_b;
    rm_u32 phase_c;
    rm_u32 step_a;
    rm_u32 step_b;
    rm_u32 step_c;
    rm_s16 mono;
    rm_s16 left;
    rm_s16 right;

    phase_a = 0U;
    phase_b = 0U;
    phase_c = 0U;
    step_a = calc_step_q16(110U, BED_SR);
    step_b = calc_step_q16(660U, LEAD_SR);
    step_c = calc_step_q16(880U, LEAD_SR);

    for (i = 0U; i < BED_FRAMES; ++i) {
        mono = triangle_from_phase(phase_a);
        g_bed_mono[i] = (rm_s16)(mono / 2);
        phase_a += step_a;
    }

    for (i = 0U; i < LEAD_FRAMES; ++i) {
        left = triangle_from_phase(phase_b);
        right = square_from_phase(phase_c, (rm_s16)12000);
        g_lead_stereo[i * 2U] = (rm_s16)(left / 2);
        g_lead_stereo[i * 2U + 1U] = right;
        phase_b += step_b;
        phase_c += step_c;
    }
}

int main(int argc, char **argv)
{
    rm_engine engine;
    rm_engine_config cfg;
    rm_voice_params params;
    rm_buffer buffer;
    rm_voice_handle handle;
    rm_biquad_desc bed_biquad;
    rm_u32 offset;
    rm_u32 frames_left;
    rm_u32 chunk;
    const char *out_path;

    (void)argc;
    (void)argv;

    out_path = "offline_mix.wav";
    if (argc > 1) {
        out_path = argv[1];
    }

    build_sources();

    rm_engine_config_init(&cfg);
    cfg.sample_rate = ENGINE_SR;
    cfg.channels = 2U;
    cfg.max_voices = 8U;
    cfg.master_gain_q15 = (rm_s16)29491;
    cfg.monitor_gain_q15 = RM_Q15_ZERO;
    cfg.capture_enabled = 1U;
    cfg.default_resampler = (rm_u16)RM_RESAMPLER_CUBIC;

    if (rm_engine_init(&engine, &cfg) != RM_OK) {
        fprintf(stderr, "rm_engine_init failed\n");
        return 1;
    }

    if (rm_engine_set_limiter(&engine,
                              (rm_s16)29000,
                              4U,
                              96U,
                              RM_Q15_ONE) != RM_OK) {
        fprintf(stderr, "rm_engine_set_limiter failed\n");
        return 1;
    }

    bed_biquad.b0_q14 = (rm_s16)236;
    bed_biquad.b1_q14 = (rm_s16)472;
    bed_biquad.b2_q14 = (rm_s16)236;
    bed_biquad.a1_q14 = (rm_s16)-26755;
    bed_biquad.a2_q14 = (rm_s16)11315;
    bed_biquad.wet_q15 = RM_Q15_ONE;
    bed_biquad.output_gain_q15 = RM_Q15_ONE;

    if (rm_engine_bus_fx_set_biquad(&engine, 1U, 0U, &bed_biquad) != RM_OK) {
        fprintf(stderr, "rm_engine_bus_fx_set_biquad failed\n");
        return 1;
    }
    if (rm_engine_bus_fx_set_drive(&engine,
                                   2U,
                                   0U,
                                   (rm_u16)(RM_RATIO_ONE_Q12 * 2U),
                                   (rm_s16)16000,
                                   (rm_s16)16384,
                                   RM_Q15_ONE) != RM_OK) {
        fprintf(stderr, "rm_engine_bus_fx_set_drive failed\n");
        return 1;
    }

    rm_voice_params_init(&params);
    params.gain_q15 = (rm_s16)19660;
    params.pan_q15 = (rm_s16)-18000;
    params.flags = (rm_u16)RM_VOICE_FLAG_LOOP;
    params.loop_start_frame = 0U;
    params.loop_end_frame = BED_FRAMES;
    params.bus_id = 1U;
    params.group_id = 1U;
    params.resampler = (rm_u16)RM_RESAMPLER_CUBIC;

    buffer.samples = g_bed_mono;
    buffer.frame_count = BED_FRAMES;
    buffer.sample_rate = BED_SR;
    buffer.channels = 1U;
    buffer.format = (rm_u16)RM_SAMPLE_S16;
    buffer.interleaved = 1U;
    buffer.reserved = 0U;

    if (rm_engine_play_buffer(&engine, &buffer, &params, &handle) != RM_OK) {
        fprintf(stderr, "rm_engine_play_buffer(bed) failed\n");
        return 1;
    }

    rm_voice_params_init(&params);
    params.gain_q15 = (rm_s16)24576;
    params.pan_q15 = RM_PAN_CENTER;
    params.pitch_q12 = RM_RATIO_ONE_Q12;
    params.bus_id = 2U;
    params.group_id = 2U;
    params.resampler = (rm_u16)RM_RESAMPLER_CUBIC;

    buffer.samples = g_lead_stereo;
    buffer.frame_count = LEAD_FRAMES;
    buffer.sample_rate = LEAD_SR;
    buffer.channels = 2U;
    buffer.format = (rm_u16)RM_SAMPLE_S16;
    buffer.interleaved = 1U;
    buffer.reserved = 0U;

    if (rm_engine_play_buffer(&engine, &buffer, &params, NULL) != RM_OK) {
        fprintf(stderr, "rm_engine_play_buffer(lead) failed\n");
        return 1;
    }

    offset = 0U;
    frames_left = OUTPUT_FRAMES;
    while (frames_left > 0U) {
        chunk = frames_left > BLOCK_FRAMES ? BLOCK_FRAMES : frames_left;
        if (rm_engine_render_s16(&engine, &g_output[offset * 2U], chunk) != RM_OK) {
            fprintf(stderr, "rm_engine_render_s16 failed\n");
            return 1;
        }
        offset += chunk;
        frames_left -= chunk;
    }

    if (!example_wav_write_s16(out_path, g_output, OUTPUT_FRAMES, 2U, ENGINE_SR)) {
        fprintf(stderr, "failed to write %s\n", out_path);
        return 1;
    }

    printf("wrote %s\n", out_path);
    return 0;
}
