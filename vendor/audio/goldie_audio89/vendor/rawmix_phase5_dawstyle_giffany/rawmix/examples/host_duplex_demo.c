#include <stdio.h>
#include "rawmix.h"
#include "example_wav.h"

#define ENGINE_SR 48000U
#define BLOCK_FRAMES 128U
#define TOTAL_FRAMES (ENGINE_SR * 2U)
#define MIC_FRAMES TOTAL_FRAMES
#define BED_FRAMES (ENGINE_SR / 2U)

static rm_s16 g_mic_input[MIC_FRAMES];
static rm_s16 g_duplex_output[TOTAL_FRAMES * 2U];
static rm_s16 g_capture_copy[TOTAL_FRAMES * 2U];
static rm_s16 g_bed[BED_FRAMES];

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

static void build_signals(void)
{
    rm_u32 i;
    rm_u32 mic_phase;
    rm_u32 bed_phase;
    rm_u32 mic_step;
    rm_u32 bed_step;

    mic_phase = 0U;
    bed_phase = 0U;
    mic_step = calc_step_q16(220U, ENGINE_SR);
    bed_step = calc_step_q16(55U, ENGINE_SR);

    for (i = 0U; i < MIC_FRAMES; ++i) {
        g_mic_input[i] = (rm_s16)(triangle_from_phase(mic_phase) / 3);
        mic_phase += mic_step;
    }
    for (i = 0U; i < BED_FRAMES; ++i) {
        g_bed[i] = (rm_s16)(triangle_from_phase(bed_phase) / 4);
        bed_phase += bed_step;
    }
}

int main(int argc, char **argv)
{
    rm_engine engine;
    rm_engine_config cfg;
    rm_voice_params params;
    rm_buffer buffer;
    rm_biquad_desc bed_biquad;
    rm_u32 pos;
    rm_u32 left;
    rm_u32 chunk;
    rm_u32 frames_read;
    const char *out_path;
    const char *capture_path;

    out_path = "host_duplex_demo.wav";
    capture_path = "host_duplex_capture.wav";
    if (argc > 1) {
        out_path = argv[1];
    }
    if (argc > 2) {
        capture_path = argv[2];
    }

    build_signals();

    rm_engine_config_init(&cfg);
    cfg.sample_rate = ENGINE_SR;
    cfg.channels = 2U;
    cfg.max_voices = 8U;
    cfg.master_gain_q15 = RM_Q15_ONE;
    cfg.monitor_gain_q15 = (rm_s16)16384;
    cfg.monitor_pan_q15 = (rm_s16)10000;
    cfg.capture_enabled = 1U;
    cfg.default_resampler = (rm_u16)RM_RESAMPLER_CUBIC;

    if (rm_engine_init(&engine, &cfg) != RM_OK) {
        fprintf(stderr, "rm_engine_init failed\n");
        return 1;
    }

    if (rm_engine_set_limiter(&engine,
                              (rm_s16)26000,
                              2U,
                              64U,
                              RM_Q15_ONE) != RM_OK) {
        fprintf(stderr, "rm_engine_set_limiter failed\n");
        return 1;
    }

    bed_biquad.b0_q14 = (rm_s16)236;
    bed_biquad.b1_q14 = (rm_s16)472;
    bed_biquad.b2_q14 = (rm_s16)236;
    bed_biquad.a1_q14 = (rm_s16)-26755;
    bed_biquad.a2_q14 = (rm_s16)11315;
    bed_biquad.wet_q15 = (rm_s16)24576;
    bed_biquad.output_gain_q15 = RM_Q15_ONE;

    if (rm_engine_bus_fx_set_biquad(&engine, 1U, 0U, &bed_biquad) != RM_OK) {
        fprintf(stderr, "rm_engine_bus_fx_set_biquad failed\n");
        return 1;
    }

    rm_voice_params_init(&params);
    params.gain_q15 = (rm_s16)22000;
    params.flags = (rm_u16)RM_VOICE_FLAG_LOOP;
    params.loop_end_frame = BED_FRAMES;
    params.bus_id = 1U;
    params.group_id = 1U;
    params.resampler = (rm_u16)RM_RESAMPLER_CUBIC;

    buffer.samples = g_bed;
    buffer.frame_count = BED_FRAMES;
    buffer.sample_rate = ENGINE_SR;
    buffer.channels = 1U;
    buffer.format = (rm_u16)RM_SAMPLE_S16;
    buffer.interleaved = 1U;
    buffer.reserved = 0U;

    if (rm_engine_play_buffer(&engine, &buffer, &params, NULL) != RM_OK) {
        fprintf(stderr, "rm_engine_play_buffer failed\n");
        return 1;
    }

    pos = 0U;
    left = TOTAL_FRAMES;
    while (left > 0U) {
        chunk = left > BLOCK_FRAMES ? BLOCK_FRAMES : left;
        if (rm_engine_process_duplex_s16(&engine,
                                         &g_mic_input[pos],
                                         1U,
                                         &g_duplex_output[pos * 2U],
                                         2U,
                                         chunk) != RM_OK) {
            fprintf(stderr, "rm_engine_process_duplex_s16 failed\n");
            return 1;
        }
        pos += chunk;
        left -= chunk;
    }

    frames_read = 0U;
    while (rm_engine_capture_available(&engine) > 0U && frames_read < TOTAL_FRAMES) {
        rm_u32 got;
        got = 0U;
        if (rm_engine_capture_read_s16(&engine,
                                       &g_capture_copy[frames_read * 2U],
                                       2U,
                                       TOTAL_FRAMES - frames_read,
                                       &got) == RM_ERR_EMPTY) {
            break;
        }
        frames_read += got;
    }

    if (!example_wav_write_s16(out_path, g_duplex_output, TOTAL_FRAMES, 2U, ENGINE_SR)) {
        fprintf(stderr, "failed to write %s\n", out_path);
        return 1;
    }

    if (!example_wav_write_s16(capture_path, g_capture_copy, frames_read, 2U, ENGINE_SR)) {
        fprintf(stderr, "failed to write %s\n", capture_path);
        return 1;
    }

    printf("wrote %s and %s\n", out_path, capture_path);
    return 0;
}
