#include <stdio.h>
#include <string.h>
#include "gvoice89.h"

#define TEST_VOICES 256
#define TEST_FRAMES 4096

typedef struct test_tone_s {
    gv89_u32 pos;
    gv89_u32 length;
    gv89_u32 phase;
    gv89_u32 step;
    gv89_s16 amp;
} test_tone;

static gv89_s16 tone_process(void *user)
{
    test_tone *tone;
    gv89_s32 env;
    gv89_s32 sample;
    tone = (test_tone *)user;
    if (tone->pos >= tone->length) return 0;
    env = (gv89_s32)(((tone->length - tone->pos) * 32767U) / tone->length);
    sample = (tone->phase & 0x80000000U) ? tone->amp : -tone->amp;
    tone->phase += tone->step;
    tone->pos++;
    return (gv89_s16)((sample * env) >> 15);
}

static int tone_active(const void *user)
{
    const test_tone *tone;
    tone = (const test_tone *)user;
    return tone->pos < tone->length;
}

static void tone_stop(void *user)
{
    test_tone *tone;
    tone = (test_tone *)user;
    tone->pos = tone->length;
}

static void tone_advance(void *user, gv89_u32 frames)
{
    test_tone *tone;
    gv89_u32 left;
    tone = (test_tone *)user;
    left = tone->length - tone->pos;
    if (frames > left) frames = left;
    tone->phase += tone->step * frames;
    tone->pos += frames;
}

static gv89_u16 tone_level(const void *user)
{
    const test_tone *tone;
    gv89_u32 remain;
    tone = (const test_tone *)user;
    if (tone->pos >= tone->length) return 0U;
    remain = tone->length - tone->pos;
    return (gv89_u16)((remain * 32767U) / tone->length);
}

int main(void)
{
    gv89_context ctx;
    gv89_voice voices[TEST_VOICES];
    test_tone tones[TEST_VOICES];
    gv89_provider_ex provider;
    gv89_request request;
    gv89_voice_params params;
    gv89_handle handle;
    gv89_s16 output[TEST_FRAMES * 2];
    gv89_stats stats;
    gv89_u16 i;
    gv89_result result;

    memset(&ctx, 0, sizeof(ctx));
    memset(voices, 0, sizeof(voices));
    memset(tones, 0, sizeof(tones));
    memset(output, 0, sizeof(output));
    result = gv89_init(&ctx, voices, TEST_VOICES, 44100U);
    if (result != GV89_OK) return 1;
    if (gv89_set_physical_limit(&ctx, 64U) != GV89_OK) return 2;
    gv89_set_group_rule(&ctx, 1U, 256U, 8U,
                         GV89_STEAL_PRIORITY_AUDIBILITY);
    gv89_request_default(&request);
    gv89_voice_params_default(&params);
    request.group_id = 1U;
    request.virtual_behavior = GV89_VIRTUAL_ADVANCE;
    request.instance_limit = 0U;
    request.minimum_physical_ms = 5U;
    provider.base.process_mono = tone_process;
    provider.base.is_active = tone_active;
    provider.base.stop = tone_stop;
    provider.advance_frames = tone_advance;
    provider.restart = 0;
    provider.estimated_level_q15 = tone_level;
    provider.physical_state_changed = 0;

    for (i = 0U; i < TEST_VOICES; ++i) {
        tones[i].pos = 0U;
        tones[i].length = 22050U;
        tones[i].phase = (gv89_u32)i * 1234567U;
        tones[i].step = 1000000U + (gv89_u32)i * 17011U;
        tones[i].amp = 3000;
        provider.base.user = &tones[i];
        request.priority = (gv89_u16)(100U + (i & 31U));
        params.pan_q15 = (gv89_s16)(((gv89_s32)(i & 31U) * 2048) - 31744);
        params.audibility_q15 = (gv89_u16)(8192U + (i * 97U) % 24575U);
        result = gv89_start_ex(&ctx, &request, &provider, &params, &handle);
        if (result != GV89_OK) return 3;
    }
    gv89_force_rebalance(&ctx);
    if (gv89_active_count(&ctx) != 256U) return 4;
    if (gv89_physical_count(&ctx) != 64U) return 5;
    if (gv89_virtual_count(&ctx) != 192U) return 6;
    gv89_render_stereo(&ctx, output, TEST_FRAMES, 0);
    gv89_get_stats(&ctx, &stats);
    if (stats.peak_logical != 256U) return 7;
    if (stats.peak_physical != 64U) return 8;
    if (stats.promotions < 64U) return 9;
    printf("PASS logical=%u physical=%u virtual=%u voice_bytes=%lu ctx_bytes=%lu\n",
           (unsigned)gv89_active_count(&ctx),
           (unsigned)gv89_physical_count(&ctx),
           (unsigned)gv89_virtual_count(&ctx),
           (unsigned long)gv89_voice_bytes(),
           (unsigned long)gv89_context_bytes());
    return 0;
}
