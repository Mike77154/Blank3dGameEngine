#include <assert.h>
#include "../KNM_audio/knm_hwr_audio.h"

static void constant_cb(void *u,
                        const knm_fix16 *in,
                        knm_fix16 *out,
                        unsigned int f,
                        unsigned int c)
{
    unsigned int i;
    knm_fix16 value;
    (void)in;
    value = *(knm_fix16 *)u;
    for (i = 0U; i < f * c; ++i) {
        out[i] = value;
    }
}

int main(void)
{
    knm_audio_config cfg;
    knm_device *dev;
    knm_stream_stats stats;
    knm_fix16 value;

    knm_audio_config_init(&cfg);
    cfg.backend = KNM_BACKEND_NULL;
    value = KNM_FIX_HALF;

    assert(knm_audio_open(&dev, &cfg, constant_cb, &value) == KNM_OK);
    assert(knm_audio_service(dev, cfg.frames_per_buffer) == KNM_OK);
    assert(knm_audio_service(dev, cfg.frames_per_buffer) == KNM_OK);
    assert(knm_audio_service(dev, cfg.frames_per_buffer) == KNM_OK);
    assert(knm_audio_get_stats(dev, &stats) == KNM_OK);
    assert(stats.rendered_frames == (unsigned long)(cfg.frames_per_buffer * 3U));
    assert(stats.callback_calls == 3UL);
    knm_audio_close(dev);
    return 0;
}
