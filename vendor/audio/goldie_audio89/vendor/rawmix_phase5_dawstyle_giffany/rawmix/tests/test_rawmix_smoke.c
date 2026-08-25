#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rawmix.h"

#define TEST_FRAMES 16U
#define LOOP_FRAMES 8U

static rm_s16 g_src_mono[TEST_FRAMES];
static rm_s16 g_src_loud[LOOP_FRAMES];
static rm_s16 g_out_stereo[TEST_FRAMES * 2U];
static rm_s16 g_out_mono[TEST_FRAMES];
static rm_s16 g_out_mono_alt[TEST_FRAMES];

typedef struct test_stream_state_s {
    const rm_s16 *data;
    rm_u32 frame_count;
    rm_u32 cursor;
} test_stream_state;

static int expect(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", msg);
        return 0;
    }
    return 1;
}

static rm_result test_stream_next(void *user,
                                  rm_s16 *out_interleaved_frame,
                                  rm_u16 channels,
                                  rm_u16 *out_end_of_stream)
{
    test_stream_state *state;

    state = (test_stream_state *)user;
    if (out_end_of_stream == NULL || out_interleaved_frame == NULL || channels == 0U) {
        return RM_ERR_INVALID_ARG;
    }

    if (state->cursor >= state->frame_count) {
        *out_end_of_stream = 1U;
        return RM_OK;
    }

    out_interleaved_frame[0] = state->data[state->cursor];
    if (channels > 1U) {
        out_interleaved_frame[1] = state->data[state->cursor];
    }
    state->cursor++;
    *out_end_of_stream = 0U;
    return RM_OK;
}

static int test_basic_and_capture(void)
{
    rm_engine engine;
    rm_engine_config cfg;
    rm_voice_params params;
    rm_buffer buffer;
    rm_voice_handle handle;
    rm_u32 avail;
    rm_u32 frames_read;
    rm_u32 i;

    for (i = 0U; i < TEST_FRAMES; ++i) {
        g_src_mono[i] = (rm_s16)2000;
    }
    memset(g_out_stereo, 0, sizeof(g_out_stereo));

    rm_engine_config_init(&cfg);
    cfg.sample_rate = 48000U;
    cfg.channels = 2U;
    cfg.max_voices = 4U;
    cfg.monitor_gain_q15 = RM_Q15_HALF;

    if (!expect(rm_engine_init(&engine, &cfg) == RM_OK, "engine init")) {
        return 0;
    }

    rm_voice_params_init(&params);
    params.gain_q15 = RM_Q15_ONE;
    params.pan_q15 = RM_PAN_CENTER;

    buffer.samples = g_src_mono;
    buffer.frame_count = TEST_FRAMES;
    buffer.sample_rate = 48000U;
    buffer.channels = 1U;
    buffer.format = (rm_u16)RM_SAMPLE_S16;
    buffer.interleaved = 1U;
    buffer.reserved = 0U;

    if (!expect(rm_engine_play_buffer(&engine, &buffer, &params, &handle) == RM_OK, "play buffer")) {
        return 0;
    }

    if (!expect(rm_engine_render_s16(&engine, g_out_stereo, TEST_FRAMES) == RM_OK, "render")) {
        return 0;
    }

    if (!expect(g_out_stereo[0] > 0, "non-zero output")) {
        return 0;
    }

    if (!expect(rm_engine_is_voice_active(&engine, handle) == 0, "voice finished")) {
        return 0;
    }

    if (!expect(rm_engine_process_duplex_s16(&engine, g_src_mono, 1U, g_out_stereo, 2U, TEST_FRAMES) == RM_OK, "duplex")) {
        return 0;
    }

    avail = rm_engine_capture_available(&engine);
    if (!expect(avail == TEST_FRAMES, "capture ring frame count")) {
        return 0;
    }

    frames_read = 0U;
    if (!expect(rm_engine_capture_read_s16(&engine, g_out_stereo, 2U, TEST_FRAMES, &frames_read) == RM_OK, "capture read")) {
        return 0;
    }
    if (!expect(frames_read == TEST_FRAMES, "capture read count")) {
        return 0;
    }

    return 1;
}

static int test_priority_and_protection(void)
{
    rm_engine engine;
    rm_engine_config cfg;
    rm_voice_params params;
    rm_buffer buffer;
    rm_voice_handle low;
    rm_voice_handle hero;
    rm_voice_handle newcomer;
    rm_u32 i;

    for (i = 0U; i < LOOP_FRAMES; ++i) {
        g_src_mono[i] = (rm_s16)1000;
    }

    rm_engine_config_init(&cfg);
    cfg.sample_rate = 48000U;
    cfg.channels = 2U;
    cfg.max_voices = 2U;
    if (!expect(rm_engine_init(&engine, &cfg) == RM_OK, "priority engine init")) {
        return 0;
    }

    buffer.samples = g_src_mono;
    buffer.frame_count = LOOP_FRAMES;
    buffer.sample_rate = 48000U;
    buffer.channels = 1U;
    buffer.format = (rm_u16)RM_SAMPLE_S16;
    buffer.interleaved = 1U;
    buffer.reserved = 0U;

    rm_voice_params_init(&params);
    params.flags = (rm_u16)RM_VOICE_FLAG_LOOP;
    params.priority = 10U;
    if (!expect(rm_engine_play_buffer(&engine, &buffer, &params, &low) == RM_OK, "play low")) {
        return 0;
    }

    rm_voice_params_init(&params);
    params.flags = (rm_u16)(RM_VOICE_FLAG_LOOP | RM_VOICE_FLAG_PROTECTED);
    params.priority = 200U;
    if (!expect(rm_engine_play_buffer(&engine, &buffer, &params, &hero) == RM_OK, "play protected")) {
        return 0;
    }

    rm_voice_params_init(&params);
    params.flags = (rm_u16)RM_VOICE_FLAG_LOOP;
    params.priority = 20U;
    if (!expect(rm_engine_play_buffer(&engine, &buffer, &params, &newcomer) == RM_OK, "play newcomer")) {
        return 0;
    }

    if (!expect(rm_engine_is_voice_active(&engine, low) == 0, "low priority voice stolen")) {
        return 0;
    }
    if (!expect(rm_engine_is_voice_active(&engine, hero) == 1, "protected voice survived")) {
        return 0;
    }
    if (!expect(rm_engine_is_voice_active(&engine, newcomer) == 1, "newcomer active")) {
        return 0;
    }

    return 1;
}

static int test_bus_group_and_fade(void)
{
    rm_engine engine;
    rm_engine_config cfg;
    rm_voice_params params;
    rm_buffer buffer;
    rm_voice_handle handle;
    rm_u32 i;

    for (i = 0U; i < LOOP_FRAMES; ++i) {
        g_src_mono[i] = (rm_s16)2000;
    }
    memset(g_out_stereo, 0, sizeof(g_out_stereo));

    rm_engine_config_init(&cfg);
    cfg.sample_rate = 48000U;
    cfg.channels = 2U;
    cfg.max_voices = 4U;
    if (!expect(rm_engine_init(&engine, &cfg) == RM_OK, "bus engine init")) {
        return 0;
    }

    if (!expect(rm_engine_set_bus(&engine, 1U, RM_Q15_ZERO, RM_PAN_CENTER) == RM_OK, "set bus zero")) {
        return 0;
    }

    buffer.samples = g_src_mono;
    buffer.frame_count = LOOP_FRAMES;
    buffer.sample_rate = 48000U;
    buffer.channels = 1U;
    buffer.format = (rm_u16)RM_SAMPLE_S16;
    buffer.interleaved = 1U;
    buffer.reserved = 0U;

    rm_voice_params_init(&params);
    params.flags = (rm_u16)RM_VOICE_FLAG_LOOP;
    params.bus_id = 1U;
    params.group_id = 1U;
    if (!expect(rm_engine_play_buffer(&engine, &buffer, &params, &handle) == RM_OK, "play bus voice")) {
        return 0;
    }

    if (!expect(rm_engine_render_s16(&engine, g_out_stereo, 2U) == RM_OK, "render muted bus")) {
        return 0;
    }
    if (!expect(g_out_stereo[0] == 0 && g_out_stereo[1] == 0, "muted bus output zero")) {
        return 0;
    }

    if (!expect(rm_engine_ramp_bus(&engine, 1U, RM_Q15_ONE, RM_PAN_CENTER, 4U) == RM_OK, "ramp bus")) {
        return 0;
    }
    if (!expect(rm_engine_render_s16(&engine, g_out_stereo, 4U) == RM_OK, "render bus ramp")) {
        return 0;
    }
    if (!expect(g_out_stereo[0] < g_out_stereo[6], "bus ramp grows")) {
        return 0;
    }

    if (!expect(rm_engine_set_group(&engine, 1U, RM_Q15_ZERO, RM_PAN_CENTER) == RM_OK, "group mute")) {
        return 0;
    }
    if (!expect(rm_engine_render_s16(&engine, g_out_stereo, 2U) == RM_OK, "render group mute")) {
        return 0;
    }
    if (!expect(g_out_stereo[0] == 0 && g_out_stereo[1] == 0, "group mute output zero")) {
        return 0;
    }

    if (!expect(rm_engine_set_group(&engine, 1U, RM_Q15_ONE, RM_PAN_CENTER) == RM_OK, "group restore")) {
        return 0;
    }
    if (!expect(rm_engine_fade_out_voice(&engine, handle, 4U) == RM_OK, "fade voice")) {
        return 0;
    }
    if (!expect(rm_engine_render_s16(&engine, g_out_stereo, 6U) == RM_OK, "render fade")) {
        return 0;
    }
    if (!expect(rm_engine_is_voice_active(&engine, handle) == 0, "voice faded out")) {
        return 0;
    }

    return 1;
}

static int test_stream_and_offset(void)
{
    rm_engine engine;
    rm_engine_config cfg;
    rm_voice_params params;
    rm_voice_handle handle;
    rm_stream_desc stream;
    test_stream_state state;
    rm_s16 stream_data[6];
    rm_u32 i;

    for (i = 0U; i < 6U; ++i) {
        stream_data[i] = (rm_s16)(1000 + (rm_s16)(i * 500U));
    }
    memset(g_out_mono, 0, sizeof(g_out_mono));

    state.data = stream_data;
    state.frame_count = 6U;
    state.cursor = 0U;

    stream.on_next = test_stream_next;
    stream.user = &state;
    stream.sample_rate = 48000U;
    stream.channels = 1U;
    stream.reserved = 0U;

    rm_engine_config_init(&cfg);
    cfg.sample_rate = 48000U;
    cfg.channels = 1U;
    cfg.max_voices = 2U;
    if (!expect(rm_engine_init(&engine, &cfg) == RM_OK, "stream engine init")) {
        return 0;
    }

    rm_voice_params_init(&params);
    params.start_delay_frames = 2U;
    params.start_frame_offset = 1U;
    if (!expect(rm_engine_play_stream(&engine, &stream, &params, &handle) == RM_OK, "play stream")) {
        return 0;
    }

    if (!expect(rm_engine_render_s16(&engine, g_out_mono, 8U) == RM_OK, "render stream")) {
        return 0;
    }
    if (!expect(g_out_mono[0] == 0 && g_out_mono[1] == 0, "stream delay applied")) {
        return 0;
    }
    if (!expect(g_out_mono[2] >= 1500, "stream start offset applied")) {
        return 0;
    }
    if (!expect(rm_engine_is_voice_active(&engine, handle) == 0, "stream finished")) {
        return 0;
    }

    return 1;
}

static int test_clipping_counter(void)
{
    rm_engine engine;
    rm_engine_config cfg;
    rm_voice_params params;
    rm_buffer buffer;
    rm_engine_stats stats;
    rm_u32 i;

    for (i = 0U; i < LOOP_FRAMES; ++i) {
        g_src_loud[i] = (rm_s16)32767;
    }

    rm_engine_config_init(&cfg);
    cfg.sample_rate = 48000U;
    cfg.channels = 1U;
    cfg.max_voices = 4U;
    if (!expect(rm_engine_init(&engine, &cfg) == RM_OK, "clip engine init")) {
        return 0;
    }

    buffer.samples = g_src_loud;
    buffer.frame_count = LOOP_FRAMES;
    buffer.sample_rate = 48000U;
    buffer.channels = 1U;
    buffer.format = (rm_u16)RM_SAMPLE_S16;
    buffer.interleaved = 1U;
    buffer.reserved = 0U;

    rm_voice_params_init(&params);
    params.flags = (rm_u16)RM_VOICE_FLAG_LOOP;
    if (!expect(rm_engine_play_buffer(&engine, &buffer, &params, NULL) == RM_OK, "clip play 1")) {
        return 0;
    }
    if (!expect(rm_engine_play_buffer(&engine, &buffer, &params, NULL) == RM_OK, "clip play 2")) {
        return 0;
    }

    if (!expect(rm_engine_render_s16(&engine, g_out_mono, 1U) == RM_OK, "clip render")) {
        return 0;
    }

    rm_engine_get_stats(&engine, &stats);
    if (!expect(stats.clipping_events > 0U, "clipping counted")) {
        return 0;
    }

    return 1;
}

static int test_bus_fx_lowpass_and_drive(void)
{
    rm_engine engine;
    rm_engine_config cfg;
    rm_voice_params params;
    rm_buffer buffer;
    rm_engine_stats stats;
    rm_u32 i;

    for (i = 0U; i < LOOP_FRAMES; ++i) {
        g_src_loud[i] = (rm_s16)30000;
    }
    memset(g_out_stereo, 0, sizeof(g_out_stereo));

    rm_engine_config_init(&cfg);
    cfg.sample_rate = 48000U;
    cfg.channels = 2U;
    cfg.max_voices = 4U;
    if (!expect(rm_engine_init(&engine, &cfg) == RM_OK, "fx lowpass engine init")) {
        return 0;
    }

    if (!expect(rm_engine_bus_fx_set_lowpass(&engine,
                                             2U,
                                             0U,
                                             200U,
                                             RM_Q15_ONE,
                                             RM_Q15_ONE) == RM_OK,
                "set lowpass")) {
        return 0;
    }

    buffer.samples = g_src_loud;
    buffer.frame_count = LOOP_FRAMES;
    buffer.sample_rate = 48000U;
    buffer.channels = 1U;
    buffer.format = (rm_u16)RM_SAMPLE_S16;
    buffer.interleaved = 1U;
    buffer.reserved = 0U;

    rm_voice_params_init(&params);
    params.bus_id = 2U;
    if (!expect(rm_engine_play_buffer(&engine, &buffer, &params, NULL) == RM_OK, "play lowpass voice")) {
        return 0;
    }
    if (!expect(rm_engine_render_s16(&engine, g_out_stereo, LOOP_FRAMES) == RM_OK, "render lowpass")) {
        return 0;
    }
    if (!expect(g_out_stereo[0] < g_out_stereo[(LOOP_FRAMES - 1U) * 2U], "lowpass smooths attack")) {
        return 0;
    }

    rm_engine_get_stats(&engine, &stats);
    if (!expect(stats.bus_fx_frames > 0U, "bus fx stats ticked")) {
        return 0;
    }

    rm_engine_config_init(&cfg);
    cfg.sample_rate = 48000U;
    cfg.channels = 1U;
    cfg.max_voices = 4U;
    if (!expect(rm_engine_init(&engine, &cfg) == RM_OK, "fx drive engine init")) {
        return 0;
    }

    if (!expect(rm_engine_bus_fx_set_drive(&engine,
                                           3U,
                                           0U,
                                           (rm_u16)(RM_RATIO_ONE_Q12 * 2U),
                                           (rm_s16)8000,
                                           RM_Q15_ONE,
                                           RM_Q15_ONE) == RM_OK,
                "set drive")) {
        return 0;
    }

    buffer.samples = g_src_loud;
    buffer.frame_count = LOOP_FRAMES;
    buffer.sample_rate = 48000U;
    buffer.channels = 1U;
    buffer.format = (rm_u16)RM_SAMPLE_S16;
    buffer.interleaved = 1U;
    buffer.reserved = 0U;

    rm_voice_params_init(&params);
    params.flags = (rm_u16)RM_VOICE_FLAG_LOOP;
    params.bus_id = 3U;
    params.gain_q15 = RM_Q15_ONE;
    if (!expect(rm_engine_play_buffer(&engine, &buffer, &params, NULL) == RM_OK, "drive play 1")) {
        return 0;
    }
    if (!expect(rm_engine_play_buffer(&engine, &buffer, &params, NULL) == RM_OK, "drive play 2")) {
        return 0;
    }
    if (!expect(rm_engine_render_s16(&engine, g_out_mono, 1U) == RM_OK, "drive render")) {
        return 0;
    }

    rm_engine_get_stats(&engine, &stats);
    if (!expect(stats.clipping_events == 0U, "drive avoided hard clipping")) {
        return 0;
    }
    if (!expect(g_out_mono[0] > 8000 && g_out_mono[0] < 32767, "drive shaped bus output")) {
        return 0;
    }

    return 1;
}

static int test_resampler_modes(void)
{
    rm_engine engine;
    rm_engine_config cfg;
    rm_voice_params params;
    rm_buffer buffer;
    rm_engine_stats stats;
    rm_s16 shape[5];

    shape[0] = (rm_s16)0;
    shape[1] = (rm_s16)30000;
    shape[2] = (rm_s16)0;
    shape[3] = (rm_s16)-30000;
    shape[4] = (rm_s16)0;
    memset(g_out_mono, 0, sizeof(g_out_mono));
    memset(g_out_mono_alt, 0, sizeof(g_out_mono_alt));

    buffer.samples = shape;
    buffer.frame_count = 5U;
    buffer.sample_rate = 24000U;
    buffer.channels = 1U;
    buffer.format = (rm_u16)RM_SAMPLE_S16;
    buffer.interleaved = 1U;
    buffer.reserved = 0U;

    rm_engine_config_init(&cfg);
    cfg.sample_rate = 48000U;
    cfg.channels = 1U;
    cfg.max_voices = 2U;
    if (!expect(rm_engine_init(&engine, &cfg) == RM_OK, "resampler linear init")) {
        return 0;
    }

    rm_voice_params_init(&params);
    params.resampler = (rm_u16)RM_RESAMPLER_LINEAR;
    if (!expect(rm_engine_play_buffer(&engine, &buffer, &params, NULL) == RM_OK, "play linear")) {
        return 0;
    }
    if (!expect(rm_engine_render_s16(&engine, g_out_mono, 6U) == RM_OK, "render linear")) {
        return 0;
    }

    rm_engine_config_init(&cfg);
    cfg.sample_rate = 48000U;
    cfg.channels = 1U;
    cfg.max_voices = 2U;
    if (!expect(rm_engine_init(&engine, &cfg) == RM_OK, "resampler cubic init")) {
        return 0;
    }

    rm_voice_params_init(&params);
    params.resampler = (rm_u16)RM_RESAMPLER_CUBIC;
    if (!expect(rm_engine_play_buffer(&engine, &buffer, &params, NULL) == RM_OK, "play cubic")) {
        return 0;
    }
    if (!expect(rm_engine_render_s16(&engine, g_out_mono_alt, 6U) == RM_OK, "render cubic")) {
        return 0;
    }

    if (!expect(g_out_mono[1] != g_out_mono_alt[1], "cubic differs from linear")) {
        return 0;
    }
    if (!expect(g_out_mono_alt[1] > g_out_mono[1], "cubic has stronger midpoint")) {
        return 0;
    }

    rm_engine_get_stats(&engine, &stats);
    if (!expect(stats.resampler_hq_frames > 0U, "hq resampler stats ticked")) {
        return 0;
    }

    return 1;
}


static int test_biquad_and_limiter(void)
{
    rm_engine engine;
    rm_engine_config cfg;
    rm_voice_params params;
    rm_buffer buffer;
    rm_engine_stats stats;
    rm_biquad_desc biquad;
    rm_u32 i;

    for (i = 0U; i < LOOP_FRAMES; ++i) {
        g_src_loud[i] = (rm_s16)30000;
    }

    rm_engine_config_init(&cfg);
    cfg.sample_rate = 48000U;
    cfg.channels = 1U;
    cfg.max_voices = 4U;
    if (!expect(rm_engine_init(&engine, &cfg) == RM_OK, "biquad engine init")) {
        return 0;
    }

    biquad.b0_q14 = (rm_s16)236;
    biquad.b1_q14 = (rm_s16)472;
    biquad.b2_q14 = (rm_s16)236;
    biquad.a1_q14 = (rm_s16)-26755;
    biquad.a2_q14 = (rm_s16)11315;
    biquad.wet_q15 = RM_Q15_ONE;
    biquad.output_gain_q15 = RM_Q15_ONE;
    if (!expect(rm_engine_bus_fx_set_biquad(&engine, 4U, 0U, &biquad) == RM_OK, "set biquad")) {
        return 0;
    }

    buffer.samples = g_src_loud;
    buffer.frame_count = LOOP_FRAMES;
    buffer.sample_rate = 48000U;
    buffer.channels = 1U;
    buffer.format = (rm_u16)RM_SAMPLE_S16;
    buffer.interleaved = 1U;
    buffer.reserved = 0U;

    rm_voice_params_init(&params);
    params.bus_id = 4U;
    if (!expect(rm_engine_play_buffer(&engine, &buffer, &params, NULL) == RM_OK, "play biquad voice")) {
        return 0;
    }
    if (!expect(rm_engine_render_s16(&engine, g_out_mono, LOOP_FRAMES) == RM_OK, "render biquad")) {
        return 0;
    }
    if (!expect(g_out_mono[0] < g_out_mono[LOOP_FRAMES - 1U], "biquad smooths attack")) {
        return 0;
    }

    rm_engine_get_stats(&engine, &stats);
    if (!expect(stats.bus_fx_frames > 0U, "biquad bus fx stats ticked")) {
        return 0;
    }

    rm_engine_config_init(&cfg);
    cfg.sample_rate = 48000U;
    cfg.channels = 1U;
    cfg.max_voices = 4U;
    if (!expect(rm_engine_init(&engine, &cfg) == RM_OK, "limiter engine init")) {
        return 0;
    }
    if (!expect(rm_engine_set_limiter(&engine,
                                      (rm_s16)12000,
                                      1U,
                                      8U,
                                      RM_Q15_ONE) == RM_OK,
                "set limiter")) {
        return 0;
    }

    for (i = 0U; i < LOOP_FRAMES; ++i) {
        g_src_loud[i] = (rm_s16)32767;
    }

    buffer.samples = g_src_loud;
    buffer.frame_count = LOOP_FRAMES;
    buffer.sample_rate = 48000U;
    buffer.channels = 1U;
    buffer.format = (rm_u16)RM_SAMPLE_S16;
    buffer.interleaved = 1U;
    buffer.reserved = 0U;

    rm_voice_params_init(&params);
    params.flags = (rm_u16)RM_VOICE_FLAG_LOOP;
    if (!expect(rm_engine_play_buffer(&engine, &buffer, &params, NULL) == RM_OK, "limiter play 1")) {
        return 0;
    }
    if (!expect(rm_engine_play_buffer(&engine, &buffer, &params, NULL) == RM_OK, "limiter play 2")) {
        return 0;
    }
    if (!expect(rm_engine_render_s16(&engine, g_out_mono, 1U) == RM_OK, "limiter render")) {
        return 0;
    }

    rm_engine_get_stats(&engine, &stats);
    if (!expect(stats.limiter_frames > 0U, "limiter stats ticked")) {
        return 0;
    }
    if (!expect(stats.clipping_events == 0U, "limiter prevented clipping")) {
        return 0;
    }
    if (!expect(g_out_mono[0] >= 10000 && g_out_mono[0] <= 14000, "limiter ceiling in range")) {
        return 0;
    }

    return 1;
}

static int test_stream_hq_resampler(void)
{
    rm_engine engine;
    rm_engine_config cfg;
    rm_voice_params params;
    rm_stream_desc stream;
    rm_engine_stats stats;
    rm_s16 shape[5];
    test_stream_state state;

    shape[0] = (rm_s16)0;
    shape[1] = (rm_s16)30000;
    shape[2] = (rm_s16)0;
    shape[3] = (rm_s16)-30000;
    shape[4] = (rm_s16)0;
    memset(g_out_mono, 0, sizeof(g_out_mono));
    memset(g_out_mono_alt, 0, sizeof(g_out_mono_alt));

    stream.on_next = test_stream_next;
    stream.user = &state;
    stream.sample_rate = 24000U;
    stream.channels = 1U;
    stream.reserved = 0U;

    state.data = shape;
    state.frame_count = 5U;
    state.cursor = 0U;

    rm_engine_config_init(&cfg);
    cfg.sample_rate = 48000U;
    cfg.channels = 1U;
    cfg.max_voices = 2U;
    if (!expect(rm_engine_init(&engine, &cfg) == RM_OK, "stream linear init")) {
        return 0;
    }

    rm_voice_params_init(&params);
    params.resampler = (rm_u16)RM_RESAMPLER_LINEAR;
    if (!expect(rm_engine_play_stream(&engine, &stream, &params, NULL) == RM_OK, "play stream linear")) {
        return 0;
    }
    if (!expect(rm_engine_render_s16(&engine, g_out_mono, 6U) == RM_OK, "render stream linear")) {
        return 0;
    }

    state.data = shape;
    state.frame_count = 5U;
    state.cursor = 0U;

    rm_engine_config_init(&cfg);
    cfg.sample_rate = 48000U;
    cfg.channels = 1U;
    cfg.max_voices = 2U;
    if (!expect(rm_engine_init(&engine, &cfg) == RM_OK, "stream cubic init")) {
        return 0;
    }

    rm_voice_params_init(&params);
    params.resampler = (rm_u16)RM_RESAMPLER_CUBIC;
    if (!expect(rm_engine_play_stream(&engine, &stream, &params, NULL) == RM_OK, "play stream cubic")) {
        return 0;
    }
    if (!expect(rm_engine_render_s16(&engine, g_out_mono_alt, 6U) == RM_OK, "render stream cubic")) {
        return 0;
    }

    if (!expect(g_out_mono[1] != g_out_mono_alt[1], "stream cubic differs from linear")) {
        return 0;
    }
    if (!expect(g_out_mono_alt[1] > g_out_mono[1], "stream cubic midpoint stronger")) {
        return 0;
    }

    rm_engine_get_stats(&engine, &stats);
    if (!expect(stats.resampler_hq_frames > 0U, "stream hq stats ticked")) {
        return 0;
    }

    return 1;
}


static int test_bus_send_and_meter(void)
{
    rm_engine engine;
    rm_engine_config cfg;
    rm_voice_params params;
    rm_buffer buffer;
    rm_meter_state meter;
    rm_u32 i;

    for (i = 0U; i < LOOP_FRAMES; ++i) {
        g_src_mono[i] = (rm_s16)1500;
    }
    memset(g_out_stereo, 0, sizeof(g_out_stereo));

    rm_engine_config_init(&cfg);
    cfg.sample_rate = 48000U;
    cfg.channels = 2U;
    cfg.max_voices = 4U;
    if (!expect(rm_engine_init(&engine, &cfg) == RM_OK, "send init")) {
        return 0;
    }

    if (!expect(rm_engine_set_bus_send(&engine, 0U, 1U, RM_Q15_ONE, (rm_u16)RM_SEND_PRE_FADER) == RM_OK,
                "set pre send")) {
        return 0;
    }

    buffer.samples = g_src_mono;
    buffer.frame_count = LOOP_FRAMES;
    buffer.sample_rate = 48000U;
    buffer.channels = 1U;
    buffer.format = (rm_u16)RM_SAMPLE_S16;
    buffer.interleaved = 1U;
    buffer.reserved = 0U;

    rm_voice_params_init(&params);
    params.flags = (rm_u16)RM_VOICE_FLAG_LOOP;
    if (!expect(rm_engine_play_buffer(&engine, &buffer, &params, NULL) == RM_OK, "play send voice")) {
        return 0;
    }

    if (!expect(rm_engine_render_s16(&engine, g_out_stereo, 1U) == RM_OK, "render send")) {
        return 0;
    }
    if (!expect(g_out_stereo[0] > 2500, "send added extra level")) {
        return 0;
    }

    if (!expect(rm_engine_get_bus_meter(&engine, 1U, &meter) == RM_OK, "bus meter")) {
        return 0;
    }
    if (!expect(meter.peak_left_q15 > 0, "bus meter peak")) {
        return 0;
    }

    return 1;
}

static int test_bus_solo_group_mute_and_snapshot(void)
{
    rm_engine engine;
    rm_engine_config cfg;
    rm_voice_params params;
    rm_buffer buffer;
    rm_mix_snapshot snapshot;
    rm_u32 i;

    for (i = 0U; i < LOOP_FRAMES; ++i) {
        g_src_mono[i] = (rm_s16)1800;
    }
    memset(g_out_stereo, 0, sizeof(g_out_stereo));

    rm_engine_config_init(&cfg);
    cfg.sample_rate = 48000U;
    cfg.channels = 2U;
    cfg.max_voices = 4U;
    if (!expect(rm_engine_init(&engine, &cfg) == RM_OK, "solo init")) {
        return 0;
    }

    buffer.samples = g_src_mono;
    buffer.frame_count = LOOP_FRAMES;
    buffer.sample_rate = 48000U;
    buffer.channels = 1U;
    buffer.format = (rm_u16)RM_SAMPLE_S16;
    buffer.interleaved = 1U;
    buffer.reserved = 0U;

    rm_voice_params_init(&params);
    params.flags = (rm_u16)RM_VOICE_FLAG_LOOP;
    params.bus_id = 1U;
    params.group_id = 1U;
    if (!expect(rm_engine_play_buffer(&engine, &buffer, &params, NULL) == RM_OK, "play solo voice")) {
        return 0;
    }

    if (!expect(rm_engine_set_bus_solo(&engine, 2U, 1U) == RM_OK, "solo other bus")) {
        return 0;
    }
    if (!expect(rm_engine_render_s16(&engine, g_out_stereo, 1U) == RM_OK, "render other solo")) {
        return 0;
    }
    if (!expect(g_out_stereo[0] == 0 && g_out_stereo[1] == 0, "other bus solo mutes")) {
        return 0;
    }

    if (!expect(rm_engine_set_bus_solo(&engine, 2U, 0U) == RM_OK, "clear other solo")) {
        return 0;
    }
    if (!expect(rm_engine_set_bus_solo(&engine, 1U, 1U) == RM_OK, "solo voice bus")) {
        return 0;
    }
    if (!expect(rm_engine_render_s16(&engine, g_out_stereo, 1U) == RM_OK, "render voice solo")) {
        return 0;
    }
    if (!expect(g_out_stereo[0] > 0, "soloed bus audible")) {
        return 0;
    }

    if (!expect(rm_engine_set_group_mute(&engine, 1U, 1U) == RM_OK, "mute group")) {
        return 0;
    }
    if (!expect(rm_engine_render_s16(&engine, g_out_stereo, 1U) == RM_OK, "render muted group")) {
        return 0;
    }
    if (!expect(g_out_stereo[0] == 0 && g_out_stereo[1] == 0, "group mute works")) {
        return 0;
    }

    if (!expect(rm_engine_capture_snapshot(&engine, &snapshot) == RM_OK, "capture snapshot")) {
        return 0;
    }
    if (!expect(rm_engine_set_group_mute(&engine, 1U, 0U) == RM_OK, "unmute group")) {
        return 0;
    }
    if (!expect(rm_engine_set_bus_solo(&engine, 1U, 0U) == RM_OK, "clear bus solo")) {
        return 0;
    }
    if (!expect(rm_engine_apply_snapshot(&engine, &snapshot, 0U) == RM_OK, "apply snapshot")) {
        return 0;
    }
    if (!expect(engine.groups[1].mute == 1U, "snapshot restored group mute")) {
        return 0;
    }
    if (!expect(engine.buses[1].solo == 1U, "snapshot restored bus solo")) {
        return 0;
    }

    return 1;
}

static int test_automation_queue_sample_accuracy(void)
{
    rm_engine engine;
    rm_engine_config cfg;
    rm_voice_params params;
    rm_buffer buffer;
    rm_automation_event ev[2];
    rm_u32 i;

    for (i = 0U; i < LOOP_FRAMES; ++i) {
        g_src_mono[i] = (rm_s16)2000;
    }
    memset(g_out_stereo, 0, sizeof(g_out_stereo));

    rm_engine_config_init(&cfg);
    cfg.sample_rate = 48000U;
    cfg.channels = 2U;
    cfg.max_voices = 4U;
    if (!expect(rm_engine_init(&engine, &cfg) == RM_OK, "automation init")) {
        return 0;
    }

    if (!expect(rm_engine_set_bus(&engine, 1U, RM_Q15_ZERO, RM_PAN_CENTER) == RM_OK, "automation bus zero")) {
        return 0;
    }

    buffer.samples = g_src_mono;
    buffer.frame_count = LOOP_FRAMES;
    buffer.sample_rate = 48000U;
    buffer.channels = 1U;
    buffer.format = (rm_u16)RM_SAMPLE_S16;
    buffer.interleaved = 1U;
    buffer.reserved = 0U;

    rm_voice_params_init(&params);
    params.flags = (rm_u16)RM_VOICE_FLAG_LOOP;
    params.bus_id = 1U;
    if (!expect(rm_engine_play_buffer(&engine, &buffer, &params, NULL) == RM_OK, "play automation voice")) {
        return 0;
    }

    memset(ev, 0, sizeof(ev));
    ev[0].sample_offset = 2U;
    ev[0].type = (rm_u8)RM_AUTOMATION_BUS_SET;
    ev[0].target_id = 1U;
    ev[0].value0_q15 = RM_Q15_ONE;
    ev[0].value1_q15 = RM_PAN_CENTER;

    ev[1].sample_offset = 3U;
    ev[1].type = (rm_u8)RM_AUTOMATION_BUS_SET;
    ev[1].target_id = 1U;
    ev[1].value0_q15 = RM_Q15_ZERO;
    ev[1].value1_q15 = RM_PAN_CENTER;

    if (!expect(rm_engine_queue_automationv(&engine, ev, 2U) == RM_OK, "queue automation")) {
        return 0;
    }
    if (!expect(rm_engine_render_s16(&engine, g_out_stereo, 4U) == RM_OK, "render automation")) {
        return 0;
    }

    if (!expect(g_out_stereo[0] == 0 && g_out_stereo[2] == 0, "automation first frames silent")) {
        return 0;
    }
    if (!expect(g_out_stereo[4] > 0 && g_out_stereo[5] > 0, "automation turned bus on")) {
        return 0;
    }
    if (!expect(g_out_stereo[6] == 0 && g_out_stereo[7] == 0, "automation turned bus off")) {
        return 0;
    }

    return 1;
}

static int test_limiter_ex_and_master_meter(void)
{
    rm_engine engine;
    rm_engine_config cfg;
    rm_voice_params params;
    rm_buffer buffer;
    rm_meter_state meter;
    rm_u32 i;

    for (i = 0U; i < LOOP_FRAMES; ++i) {
        g_src_loud[i] = (rm_s16)30000;
    }
    memset(g_out_mono, 0, sizeof(g_out_mono));

    rm_engine_config_init(&cfg);
    cfg.sample_rate = 48000U;
    cfg.channels = 1U;
    cfg.max_voices = 2U;
    if (!expect(rm_engine_init(&engine, &cfg) == RM_OK, "limiter ex init")) {
        return 0;
    }

    if (!expect(rm_engine_set_limiter_ex(&engine, (rm_s16)12000, 1U, 8U, RM_Q15_ONE, 8U) == RM_OK,
                "set limiter ex")) {
        return 0;
    }
    if (!expect(rm_engine_get_latency_frames(&engine) == 8U, "lookahead latency")) {
        return 0;
    }

    buffer.samples = g_src_loud;
    buffer.frame_count = LOOP_FRAMES;
    buffer.sample_rate = 48000U;
    buffer.channels = 1U;
    buffer.format = (rm_u16)RM_SAMPLE_S16;
    buffer.interleaved = 1U;
    buffer.reserved = 0U;

    rm_voice_params_init(&params);
    if (!expect(rm_engine_play_buffer(&engine, &buffer, &params, NULL) == RM_OK, "play limiter voice")) {
        return 0;
    }
    if (!expect(rm_engine_render_s16(&engine, g_out_mono, 4U) == RM_OK, "render limiter")) {
        return 0;
    }
    if (!expect(rm_engine_get_master_meter(&engine, &meter) == RM_OK, "master meter")) {
        return 0;
    }
    if (!expect(meter.gain_reduction_q15 > 0, "limiter gain reduction meter")) {
        return 0;
    }

    return 1;
}


int main(void)
{
    if (!test_basic_and_capture()) {
        return 1;
    }
    if (!test_priority_and_protection()) {
        return 1;
    }
    if (!test_bus_group_and_fade()) {
        return 1;
    }
    if (!test_stream_and_offset()) {
        return 1;
    }
    if (!test_clipping_counter()) {
        return 1;
    }
    if (!test_bus_fx_lowpass_and_drive()) {
        return 1;
    }
    if (!test_resampler_modes()) {
        return 1;
    }
    if (!test_biquad_and_limiter()) {
        return 1;
    }
    if (!test_stream_hq_resampler()) {
        return 1;
    }
    if (!test_bus_send_and_meter()) {
        return 1;
    }
    if (!test_bus_solo_group_mute_and_snapshot()) {
        return 1;
    }
    if (!test_automation_queue_sample_accuracy()) {
        return 1;
    }
    if (!test_limiter_ex_and_master_meter()) {
        return 1;
    }

    printf("rawmix phase5 daw-style smoke test passed\n");
    return 0;
}
