#include <stdio.h>
#include <string.h>
#include "rawmix.h"
#include "example_wav.h"

#define ENGINE_SR 48000U
#define OUTPUT_SECONDS 3U
#define OUTPUT_FRAMES (ENGINE_SR * OUTPUT_SECONDS)
#define LOOP_FRAMES (ENGINE_SR / 2U)

static rm_s16 g_loop[LOOP_FRAMES];
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

static void build_source(void)
{
    rm_u32 i;
    rm_u32 phase;
    rm_u32 step;

    phase = 0U;
    step = calc_step_q16(220U, ENGINE_SR);
    for (i = 0U; i < LOOP_FRAMES; ++i) {
        g_loop[i] = (rm_s16)(triangle_from_phase(phase) / 2);
        phase += step;
    }
}

int main(int argc, char **argv)
{
    rm_engine engine;
    rm_engine_config cfg;
    rm_voice_params params;
    rm_buffer buffer;
    rm_automation_event ev[4];
    const char *out_path;

    (void)argv;

    out_path = "daw_style_showcase.wav";
    if (argc > 1) {
        out_path = argv[1];
    }

    build_source();
    memset(g_output, 0, sizeof(g_output));
    memset(ev, 0, sizeof(ev));

    rm_engine_config_init(&cfg);
    cfg.sample_rate = ENGINE_SR;
    cfg.channels = 2U;
    cfg.max_voices = 8U;
    cfg.default_resampler = (rm_u16)RM_RESAMPLER_CUBIC;
    if (rm_engine_init(&engine, &cfg) != RM_OK) {
        fprintf(stderr, "rm_engine_init failed\n");
        return 1;
    }

    if (rm_engine_set_limiter_ex(&engine,
                                 (rm_s16)26000,
                                 2U,
                                 64U,
                                 RM_Q15_ONE,
                                 32U) != RM_OK) {
        fprintf(stderr, "rm_engine_set_limiter_ex failed\n");
        return 1;
    }

    if (rm_engine_set_bus_send(&engine, 0U, 1U, RM_Q15_HALF, (rm_u16)RM_SEND_PRE_FADER) != RM_OK) {
        fprintf(stderr, "rm_engine_set_bus_send failed\n");
        return 1;
    }
    if (rm_engine_bus_fx_set_lowpass(&engine, 1U, 0U, 1800U, RM_Q15_ONE, (rm_s16)24576) != RM_OK) {
        fprintf(stderr, "rm_engine_bus_fx_set_lowpass failed\n");
        return 1;
    }
    if (rm_engine_bus_fx_set_drive(&engine,
                                   1U,
                                   1U,
                                   (rm_u16)(RM_RATIO_ONE_Q12 * 2U),
                                   (rm_s16)18000,
                                   (rm_s16)14000,
                                   RM_Q15_ONE) != RM_OK) {
        fprintf(stderr, "rm_engine_bus_fx_set_drive failed\n");
        return 1;
    }

    buffer.samples = g_loop;
    buffer.frame_count = LOOP_FRAMES;
    buffer.sample_rate = ENGINE_SR;
    buffer.channels = 1U;
    buffer.format = (rm_u16)RM_SAMPLE_S16;
    buffer.interleaved = 1U;
    buffer.reserved = 0U;

    rm_voice_params_init(&params);
    params.flags = (rm_u16)RM_VOICE_FLAG_LOOP;
    params.gain_q15 = (rm_s16)24576;
    params.bus_id = 0U;
    params.resampler = (rm_u16)RM_RESAMPLER_CUBIC;
    if (rm_engine_play_buffer(&engine, &buffer, &params, NULL) != RM_OK) {
        fprintf(stderr, "rm_engine_play_buffer failed\n");
        return 1;
    }

    ev[0].sample_offset = (rm_u16)ENGINE_SR;
    ev[0].type = (rm_u8)RM_AUTOMATION_BUS_SET;
    ev[0].target_id = 0U;
    ev[0].value0_q15 = RM_Q15_ONE;
    ev[0].value1_q15 = RM_PAN_LEFT;

    ev[1].sample_offset = (rm_u16)(ENGINE_SR * 2U);
    ev[1].type = (rm_u8)RM_AUTOMATION_BUS_SET;
    ev[1].target_id = 0U;
    ev[1].value0_q15 = RM_Q15_ONE;
    ev[1].value1_q15 = RM_PAN_RIGHT;

    ev[2].sample_offset = (rm_u16)(ENGINE_SR + (ENGINE_SR / 2U));
    ev[2].type = (rm_u8)RM_AUTOMATION_BUS_MUTE;
    ev[2].target_id = 1U;
    ev[2].value2_u16 = 1U;

    ev[3].sample_offset = (rm_u16)((ENGINE_SR * 2U) + (ENGINE_SR / 4U));
    ev[3].type = (rm_u8)RM_AUTOMATION_BUS_MUTE;
    ev[3].target_id = 1U;
    ev[3].value2_u16 = 0U;

    if (rm_engine_queue_automationv(&engine, ev, 4U) != RM_OK) {
        fprintf(stderr, "rm_engine_queue_automationv failed\n");
        return 1;
    }

    if (rm_engine_render_s16(&engine, g_output, OUTPUT_FRAMES) != RM_OK) {
        fprintf(stderr, "rm_engine_render_s16 failed\n");
        return 1;
    }

    if (!example_wav_write_s16(out_path, g_output, OUTPUT_FRAMES, 2U, ENGINE_SR)) {
        fprintf(stderr, "failed to write %s\n", out_path);
        return 1;
    }

    printf("wrote %s (latency %u frames)\n", out_path, (unsigned)rm_engine_get_latency_frames(&engine));
    return 0;
}
