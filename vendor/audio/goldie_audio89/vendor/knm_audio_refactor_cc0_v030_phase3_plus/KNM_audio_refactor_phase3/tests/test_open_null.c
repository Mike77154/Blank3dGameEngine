#include <assert.h>
#include <string.h>
#include "../KNM_audio/knm_hwr_audio.h"

static void cb(void *u, const knm_fix16 *in, knm_fix16 *out, unsigned int f, unsigned int c)
{
    unsigned int i;
    (void)u;
    (void)in;
    for (i = 0U; i < f * c; ++i) {
        out[i] = 0;
    }
}

int main(void)
{
    knm_audio_config cfg;
    knm_device *dev;
    knm_device_info info;

    assert(knm_audio_version() == KNM_AUDIO_VERSION_HEX);
    knm_audio_config_init(&cfg);
    cfg.backend = KNM_BACKEND_NULL;
    assert(knm_audio_open(&dev, &cfg, cb, 0) == KNM_OK);
    assert(knm_audio_backend(dev) == KNM_BACKEND_NULL);
    assert(knm_audio_get_config(dev, &cfg) == KNM_OK);
    assert(knm_audio_get_opened_device_info(dev, &info) == KNM_OK);
    assert(strcmp(info.device_id, "null.default") == 0);
    knm_audio_close(dev);
    return 0;
}
