#include <stdio.h>
#include "../KNM_audio/knm_hwr_audio.h"

static void constant_cb(void *user_data,
                        const knm_fix16 *input,
                        knm_fix16 *output,
                        unsigned int frames,
                        unsigned int channels)
{
    unsigned int i;
    knm_fix16 value;
    (void)input;
    value = *(knm_fix16 *)user_data;
    for (i = 0U; i < frames * channels; ++i) {
        output[i] = value;
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

    if (knm_audio_open(&dev, &cfg, constant_cb, &value) != KNM_OK) {
        return 1;
    }
    if (knm_audio_service(dev, cfg.frames_per_buffer * 2U) != KNM_OK) {
        knm_audio_close(dev);
        return 2;
    }
    if (knm_audio_get_stats(dev, &stats) != KNM_OK) {
        knm_audio_close(dev);
        return 3;
    }

    printf("rendered=%lu callbacks=%lu\n",
           stats.rendered_frames,
           stats.callback_calls);
    knm_audio_close(dev);
    return 0;
}
