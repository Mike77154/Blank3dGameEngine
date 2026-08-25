#include "blank3d_goldie_audio89.h"

#include <stdio.h>
#include <string.h>

static unsigned long g_render_calls;

static int test_weapon_render(void *user, short *dst, unsigned int frames)
{
    unsigned int i;
    int phase;
    (void)user;
    if (!dst) return 0;
    for (i = 0U; i < frames; ++i) {
        phase = (int)((i + (unsigned int)g_render_calls * frames) & 31U);
        dst[i * 2U] = (short)(phase < 16 ? 7200 : -7200);
        dst[i * 2U + 1U] = (short)(phase < 16 ? 6800 : -6800);
    }
    ++g_render_calls;
    return 1;
}

static int peak_of(const short *pcm, unsigned int samples)
{
    unsigned int i;
    int peak;
    int v;
    peak = 0;
    for (i = 0U; i < samples; ++i) {
        v = (int)pcm[i];
        if (v < 0) v = -v;
        if (v > peak) peak = v;
    }
    return peak;
}

int main(void)
{
    Blank3DGoldieAudio89 host;
    short pcm[512U * 2U];
    int peak_on;
    int peak_muted;
    goldie_audio89_stats stats;
    memset(&host, 0, sizeof(host));
    memset(pcm, 0, sizeof(pcm));
    g_render_calls = 0UL;
    if (!blank3d_goldie_audio89_init(&host, (void *)0,
                                      test_weapon_render, 44100UL)) {
        fprintf(stderr, "init: %s\n", blank3d_goldie_audio89_status(&host));
        return 1;
    }
    if (goldie_audio89_console_count(&host.runtime) != B3D_AUDIO_BUS_COUNT)
        return 2;
    if (!blank3d_goldie_audio89_render(&host, pcm, 512U)) return 3;
    peak_on = peak_of(pcm, 1024U);
    if (peak_on <= 0) return 4;
    if (!blank3d_goldie_audio89_set_bus_mute(&host,
                                              B3D_AUDIO_BUS_WEAPONS, 1)) return 5;
    memset(pcm, 0, sizeof(pcm));
    if (!blank3d_goldie_audio89_render(&host, pcm, 512U)) return 6;
    /* MASTER has a 32-frame lookahead limiter, so one render may contain
       the bounded delayed tail from the previously audible block. */
    memset(pcm, 0, sizeof(pcm));
    if (!blank3d_goldie_audio89_render(&host, pcm, 512U)) return 7;
    peak_muted = peak_of(pcm, 1024U);
    if (peak_muted != 0) return 8;
    if (!blank3d_goldie_audio89_set_bus_mute(&host,
                                              B3D_AUDIO_BUS_WEAPONS, 0)) return 9;
    goldie_audio89_get_stats(&host.runtime, &stats);
    printf("Goldie host PASS: buses=%u weapon_peak=%d muted_peak=%d renders=%lu child_inputs=%lu callback_blocks=%lu\n",
           goldie_audio89_console_count(&host.runtime), peak_on, peak_muted,
           stats.graph_renders, stats.child_channels_injected, g_render_calls);
    blank3d_goldie_audio89_shutdown(&host);
    return 0;
}
