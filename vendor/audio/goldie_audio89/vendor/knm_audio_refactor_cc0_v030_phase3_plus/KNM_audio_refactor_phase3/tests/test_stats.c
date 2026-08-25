#include <assert.h>
#include "../KNM_audio/knm_hwr_audio.h"

static void duplex_cb(void *u,
                      const knm_fix16 *in,
                      knm_fix16 *out,
                      unsigned int f,
                      unsigned int c)
{
    unsigned int i;
    unsigned long *seen;
    seen = (unsigned long *)u;
    if (in != (const knm_fix16 *)0) {
        *seen += 1UL;
    }
    for (i = 0U; i < f * c; ++i) {
        out[i] = (in != (const knm_fix16 *)0) ? in[i] : 0;
    }
}

int main(void)
{
    knm_audio_config cfg;
    knm_device *dev;
    knm_stream_stats stats;
    unsigned long seen;

    knm_audio_config_init(&cfg);
    cfg.backend = KNM_BACKEND_NULL;
    cfg.enable_input = 1;
    cfg.capture_ring_frames = cfg.frames_per_buffer * cfg.buffer_count;
    seen = 0UL;

    assert(knm_audio_open(&dev, &cfg, duplex_cb, &seen) == KNM_OK);
    assert(knm_audio_service(dev, cfg.frames_per_buffer) == KNM_OK);
    assert(knm_audio_get_stats(dev, &stats) == KNM_OK);
    assert(stats.captured_frames == (unsigned long)cfg.frames_per_buffer);
    assert(stats.rendered_frames == (unsigned long)cfg.frames_per_buffer);
    assert(stats.output_callbacks == 1UL);
    assert(seen == 1UL);
    knm_audio_reset_stats(dev);
    assert(knm_audio_get_stats(dev, &stats) == KNM_OK);
    assert(stats.callback_calls == 0UL);
    knm_audio_close(dev);
    return 0;
}
