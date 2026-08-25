#include "goldie_audio89.h"
#include <stdio.h>
#include <string.h>

static goldie_audio89 g;
static short tone_a[256 * 2];
static short tone_b[256 * 2];
static short outbuf[256 * 2];

static int peak_abs(const short *p, unsigned int n)
{
    unsigned int i;
    int peak;
    int v;
    peak = 0;
    for (i = 0U; i < n; ++i) {
        v = (int)p[i];
        if (v < 0) v = -v;
        if (v > peak) peak = v;
    }
    return peak;
}

int main(void)
{
    goldie_audio89_config cfg;
    goldie_audio89_console_handle master, music, rhythm, drums;
    goldie_audio89_console_handle ambience, fauna, birds;
    goldie_audio89_pcm_view a, b;
    goldie_audio89_voice_handle va, vb;
    goldie_audio89_stats gs;
    rm_engine_stats ms;
    unsigned int i;
    int rc;
    int peak_both;
    int peak_ambience;

    for (i = 0U; i < 256U; ++i) {
        tone_a[i * 2U] = 7000;
        tone_a[i * 2U + 1U] = 7000;
        tone_b[i * 2U] = 5000;
        tone_b[i * 2U + 1U] = 5000;
    }
    a.samples = tone_a; a.frame_count = 256UL; a.sample_rate = 48000UL; a.channels = 2U;
    b.samples = tone_b; b.frame_count = 256UL; b.sample_rate = 48000UL; b.channels = 2U;

    goldie_audio89_config_init(&cfg);
    cfg.backend = KNM_BACKEND_NULL;
    cfg.max_consoles = 16U;
    rc = goldie_audio89_init(&g, &cfg);
    if (rc != GOLDIE_AUDIO89_OK) return 1;
    master = goldie_audio89_master_console(&g);

    if (goldie_audio89_console_create(&g, master, "Music", &music) != GOLDIE_AUDIO89_OK) return 2;
    if (goldie_audio89_console_create(&g, music, "Rhythm", &rhythm) != GOLDIE_AUDIO89_OK) return 3;
    if (goldie_audio89_console_create(&g, rhythm, "Drums", &drums) != GOLDIE_AUDIO89_OK) return 4;
    if (goldie_audio89_console_create(&g, master, "Ambience", &ambience) != GOLDIE_AUDIO89_OK) return 5;
    if (goldie_audio89_console_create(&g, ambience, "Biofauna", &fauna) != GOLDIE_AUDIO89_OK) return 6;
    if (goldie_audio89_console_create(&g, fauna, "Birds", &birds) != GOLDIE_AUDIO89_OK) return 7;

    if (goldie_audio89_console_play_pcm_s16(&g, drums, &a, 1, &va) != GOLDIE_AUDIO89_OK) return 8;
    if (goldie_audio89_console_play_pcm_s16(&g, birds, &b, 1, &vb) != GOLDIE_AUDIO89_OK) return 9;

    memset(outbuf, 0, sizeof(outbuf));
    if (goldie_audio89_render_s16(&g, outbuf, 256U) != GOLDIE_AUDIO89_OK) return 10;
    peak_both = peak_abs(outbuf, 512U);
    if (peak_both < 9000) return 11;

    /* If Music is a true parent console, muting it removes the entire Drums->Rhythm subtree. */
    if (goldie_audio89_console_set_mute(&g, music, 1) != GOLDIE_AUDIO89_OK) return 12;
    memset(outbuf, 0, sizeof(outbuf));
    if (goldie_audio89_render_s16(&g, outbuf, 256U) != GOLDIE_AUDIO89_OK) return 13;
    peak_ambience = peak_abs(outbuf, 512U);
    if (peak_ambience < 4000 || peak_ambience >= peak_both) return 14;

    if (goldie_audio89_console_set_mute(&g, music, 0) != GOLDIE_AUDIO89_OK) return 15;
    /* An FX on Music must process child output, not merely local voices. */
    if (goldie_audio89_console_fx_lowpass(&g, music, 0U, 1200UL, 32767, 32767) != GOLDIE_AUDIO89_OK) return 16;
    memset(outbuf, 0, sizeof(outbuf));
    if (goldie_audio89_render_s16(&g, outbuf, 256U) != GOLDIE_AUDIO89_OK) return 17;
    if (goldie_audio89_console_get_mixer_stats(&g, music, &ms) != GOLDIE_AUDIO89_OK) return 18;
    if (ms.bus_fx_frames == 0UL) return 19;

    goldie_audio89_get_stats(&g, &gs);
    printf("Matryoshka smoke: consoles=%u renders=%lu child_inputs=%lu failures=%lu music_fx_frames=%lu peaks=%d/%d\n",
           goldie_audio89_console_count(&g), gs.graph_renders, gs.child_channels_injected,
           gs.child_channel_failures, (unsigned long)ms.bus_fx_frames, peak_both, peak_ambience);

    /* Cycle protection. */
    rc = goldie_audio89_console_reparent(&g, music, drums);
    if (rc != GOLDIE_AUDIO89_ERR_CYCLE) return 20;

    goldie_audio89_shutdown(&g);
    return 0;
}
