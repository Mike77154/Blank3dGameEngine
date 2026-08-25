#include "goldie_audio89.h"
#include <stdio.h>

#define MV_FRAMES 2400

static short g_a[MV_FRAMES];
static short g_b[MV_FRAMES * 2];
static short g_c[MV_FRAMES];
static short g_d[MV_FRAMES * 2];

static void fill_sources(void)
{
    int i;
    short a;
    short b;
    short c;
    short d;
    for (i = 0; i < MV_FRAMES; ++i) {
        a = ((i / 11) & 1) ? (short)1200 : (short)-1200;
        b = ((i / 17) & 1) ? (short)1000 : (short)-1000;
        c = ((i / 23) & 1) ? (short)800 : (short)-800;
        d = ((i / 31) & 1) ? (short)600 : (short)-600;
        g_a[i] = a;
        g_b[i * 2] = b;
        g_b[i * 2 + 1] = (short)-b;
        g_c[i] = c;
        g_d[i * 2] = d;
        g_d[i * 2 + 1] = d;
    }
}

static int play_one(goldie_audio89 *g,
                    const short *samples,
                    unsigned long frames,
                    unsigned long rate,
                    unsigned int channels,
                    short pan,
                    rm_voice_handle *out_voice)
{
    goldie_audio89_pcm_view pcm;
    int rc;
    pcm.samples = samples;
    pcm.frame_count = frames;
    pcm.sample_rate = rate;
    pcm.channels = channels;
    rc = goldie_audio89_play_pcm_s16(g, &pcm, 0, out_voice);
    if (rc != GOLDIE_AUDIO89_OK) return rc;
    rc = goldie_audio89_set_voice_gain_q15(g, *out_voice, (short)8192);
    if (rc != GOLDIE_AUDIO89_OK) return rc;
    return goldie_audio89_set_voice_pan_q15(g, *out_voice, pan);
}

int main(void)
{
    goldie_audio89 g;
    goldie_audio89_config cfg;
    rm_voice_handle voices[4];
    rm_engine_stats stats;
    int rc;
    int i;
    int active;

    fill_sources();
    goldie_audio89_config_init(&cfg);
    cfg.backend = KNM_BACKEND_NULL;
    cfg.sample_rate = 48000UL;
    cfg.channels = 2U;
    cfg.frames_per_buffer = 256U;
    cfg.buffer_count = 4U;

    rc = goldie_audio89_init(&g, &cfg);
    if (rc != GOLDIE_AUDIO89_OK) return 1;

    rc = play_one(&g, g_a, MV_FRAMES, 22050UL, 1U, (short)-18000, &voices[0]);
    if (rc != GOLDIE_AUDIO89_OK) return 2;
    rc = play_one(&g, g_b, MV_FRAMES, 44100UL, 2U, (short)-6000, &voices[1]);
    if (rc != GOLDIE_AUDIO89_OK) return 3;
    rc = play_one(&g, g_c, MV_FRAMES, 32000UL, 1U, (short)6000, &voices[2]);
    if (rc != GOLDIE_AUDIO89_OK) return 4;
    rc = play_one(&g, g_d, MV_FRAMES, 48000UL, 2U, (short)18000, &voices[3]);
    if (rc != GOLDIE_AUDIO89_OK) return 5;

    rc = goldie_audio89_start(&g);
    if (rc != GOLDIE_AUDIO89_OK) return 6;

    for (i = 0; i < 32; ++i) {
        rc = goldie_audio89_service(&g, 256U);
        if (rc != GOLDIE_AUDIO89_OK) return 7;
    }

    if (goldie_audio89_get_mixer_stats(&g, &stats) != GOLDIE_AUDIO89_OK) return 8;
    active = 0;
    for (i = 0; i < 4; ++i) {
        if (goldie_audio89_is_voice_active(&g, voices[i])) ++active;
    }

    printf("Goldie multivoice smoke: started=%lu finished=%lu active=%d render_calls=%lu clips=%lu hq_frames=%lu\n",
           (unsigned long)stats.voices_started,
           (unsigned long)stats.voices_finished,
           active,
           (unsigned long)stats.render_calls,
           (unsigned long)stats.clipping_events,
           (unsigned long)stats.resampler_hq_frames);

    goldie_audio89_shutdown(&g);
    if (stats.voices_started != 4UL) return 9;
    if (stats.voices_finished != 4UL) return 10;
    if (active != 0) return 11;
    if (stats.clipping_events != 0UL) return 12;
    if (stats.resampler_hq_frames == 0UL) return 13;
    return 0;
}
