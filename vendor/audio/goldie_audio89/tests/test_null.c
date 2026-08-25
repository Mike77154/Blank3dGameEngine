#include "goldie_audio89.h"
#include <stdio.h>

static goldie_audio89 g;
static short g_pcm[480 * 2];

int main(void)
{
    goldie_audio89_config cfg;
    goldie_audio89_pcm_view pcm;
    rm_voice_handle voice;
    rm_engine_stats stats;
    int i;
    int rc;
    for (i = 0; i < 480; ++i) {
        short s;
        s = ((i / 12) & 1) ? (short)8000 : (short)-8000;
        g_pcm[i * 2] = s;
        g_pcm[i * 2 + 1] = s;
    }
    goldie_audio89_config_init(&cfg);
    cfg.backend = KNM_BACKEND_NULL;
    rc = goldie_audio89_init(&g, &cfg);
    if (rc != GOLDIE_AUDIO89_OK) return 1;
    pcm.samples = g_pcm; pcm.frame_count = 480UL; pcm.sample_rate = 48000UL; pcm.channels = 2U;
    rc = goldie_audio89_play_pcm_s16(&g, &pcm, 0, &voice);
    if (rc != GOLDIE_AUDIO89_OK) return 2;
    rc = goldie_audio89_start(&g); if (rc != GOLDIE_AUDIO89_OK) return 3;
    rc = goldie_audio89_service(&g, 256U); if (rc != GOLDIE_AUDIO89_OK) return 4;
    rc = goldie_audio89_service(&g, 256U); if (rc != GOLDIE_AUDIO89_OK) return 5;
    rc = goldie_audio89_get_mixer_stats(&g, &stats); if (rc != GOLDIE_AUDIO89_OK) return 6;
    printf("Goldie null v0.3: backend=%s started=%lu renders=%lu finished=%lu\n",
           knm_backend_name(goldie_audio89_backend(&g)),
           (unsigned long)stats.voices_started,
           (unsigned long)stats.render_calls,
           (unsigned long)stats.voices_finished);
    goldie_audio89_shutdown(&g);
    return stats.voices_started >= 1UL && stats.render_calls >= 1UL ? 0 : 7;
}
