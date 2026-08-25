#include <stdio.h>
#include "goldie_audio89.h"
#include "mnk_core_internal.h"

#define TEST_SRC_RATE 44100UL
#define TEST_DST_RATE 48000UL
#define TEST_FRAMES 44100UL
#define TEST_OUT_FRAMES 49000UL

static short g_src[TEST_FRAMES * 2UL];
static short g_dst[TEST_OUT_FRAMES * 2UL];

static void make_test_wave(void)
{
    unsigned long i;
    int k;
    int v;
    for (i = 0UL; i < TEST_FRAMES; ++i) {
        k = (int)(i % 100UL);
        if (k < 25) v = k * 1000;
        else if (k < 75) v = 25000 - ((k - 25) * 1000);
        else v = -25000 + ((k - 75) * 1000);
        g_src[i * 2UL] = (short)v;
        g_src[i * 2UL + 1UL] = (short)v;
    }
}

int main(void)
{
    goldie_audio89 g;
    goldie_audio89_config cfg;
    goldie_audio89_config actual;
    goldie_audio89_pcm_view pcm;
    rm_voice_handle voice;
    unsigned long i;
    int crossings;
    int rc;

    make_test_wave();
    goldie_audio89_config_init(&cfg);
    cfg.backend = KNM_BACKEND_NULL;
    cfg.sample_rate = TEST_DST_RATE;
    cfg.channels = 2U;

    rc = goldie_audio89_init(&g, &cfg);
    if (rc != GOLDIE_AUDIO89_OK) return 1;
    rc = goldie_audio89_get_hardware_config(&g, &actual);
    if (rc != GOLDIE_AUDIO89_OK) return 2;
    if (actual.sample_rate != TEST_DST_RATE || actual.channels != 2U) return 3;

    pcm.samples = g_src;
    pcm.frame_count = TEST_FRAMES;
    pcm.sample_rate = TEST_SRC_RATE;
    pcm.channels = 2U;
    rc = goldie_audio89_play_pcm_s16(&g, &pcm, 0, &voice);
    if (rc != GOLDIE_AUDIO89_OK) return 4;

    mnk_render_to_s16(g.device, g_dst, (unsigned int)TEST_OUT_FRAMES);

    crossings = 0;
    for (i = 1UL; i < TEST_DST_RATE; ++i) {
        if (g_dst[(i - 1UL) * 2UL] < 0 && g_dst[i * 2UL] >= 0) ++crossings;
    }
    if (crossings < 438 || crossings > 442) return 5;
    if (g.mixer.stats.voices_finished != 1UL) return 6;

    printf("Goldie rate/bridge smoke: %d crossings, negotiated %lu Hz/%u ch, latency %lu frames\n",
           crossings, actual.sample_rate, actual.channels,
           goldie_audio89_output_latency_frames(&g));
    goldie_audio89_shutdown(&g);
    return 0;
}
