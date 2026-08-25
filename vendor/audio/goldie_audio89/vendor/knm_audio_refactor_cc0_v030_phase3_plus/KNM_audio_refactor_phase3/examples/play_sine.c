#include "../KNM_audio/knm_hwr_audio.h"

static void demo_callback(void *user_data,
                          const knm_fix16 *input,
                          knm_fix16 *output,
                          unsigned int frames,
                          unsigned int channels)
{
    unsigned int i;
    knm_fix16 *phase;
    (void)input;
    phase = (knm_fix16 *)user_data;
    for (i = 0U; i < frames * channels; ++i) {
        output[i] = *phase;
        *phase += 1024;
        if (*phase > KNM_FIX_ONE) {
            *phase = -KNM_FIX_ONE;
        }
    }
}

int main(void)
{
    knm_audio_config cfg;
    knm_device *dev;
    knm_fix16 phase;
    knm_audio_config_init(&cfg);
    cfg.backend = KNM_BACKEND_NULL;
    phase = 0;
    if (knm_audio_open(&dev, &cfg, demo_callback, &phase) != KNM_OK) {
        return 1;
    }
    if (knm_audio_service(dev, cfg.frames_per_buffer) != KNM_OK) {
        knm_audio_close(dev);
        return 2;
    }
    knm_audio_close(dev);
    return 0;
}
