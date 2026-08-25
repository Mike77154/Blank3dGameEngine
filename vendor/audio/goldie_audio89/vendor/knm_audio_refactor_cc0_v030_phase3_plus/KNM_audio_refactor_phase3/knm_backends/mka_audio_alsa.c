#include "../KNM_audio/mnk_core_internal.h"

#if KNM_AUDIO_ENABLE_ALSA && defined(__linux__)

#include <pthread.h>
#include <unistd.h>
#include <alsa/asoundlib.h>

typedef struct mka_alsa_state {
    snd_pcm_t *playback;
    snd_pcm_t *capture;
    pthread_t thread;
    int thread_started;
    int stop_requested;
} mka_alsa_state;

static mka_alsa_state *mka_alsa(knm_device *dev)
{
    return (mka_alsa_state *)(void *)dev->backend_state;
}

static int mka_alsa_probe(void)
{
    return 1;
}

static const char *mka_alsa_selected_device_id(const knm_device *dev)
{
    if (dev != (const knm_device *)0 && dev->requested_cfg.device_id[0] != '\0') {
        return dev->requested_cfg.device_id;
    }
    return "default";
}

static void mka_alsa_capture_negotiated(snd_pcm_t *pcm,
                                        knm_device *dev,
                                        snd_pcm_stream_t stream);
static int mka_alsa_configure_pcm(snd_pcm_t *pcm,
                                  knm_device *dev,
                                  snd_pcm_stream_t stream);
static void *mka_alsa_thread_proc(void *param);
static int mka_alsa_open(knm_device *dev);
static int mka_alsa_start(knm_device *dev);
static int mka_alsa_stop(knm_device *dev);
static void mka_alsa_close(knm_device *dev);

static void mka_alsa_capture_negotiated(snd_pcm_t *pcm,
                                        knm_device *dev,
                                        snd_pcm_stream_t stream)
{
    snd_pcm_hw_params_t *hw;
    snd_pcm_uframes_t period_frames;
    snd_pcm_uframes_t buffer_frames;
    unsigned int rate;
    unsigned int channels;
    int dir;
    unsigned int buffer_count;

    if (pcm == (snd_pcm_t *)0 || dev == (knm_device *)0) {
        return;
    }

    snd_pcm_hw_params_alloca(&hw);
    if (snd_pcm_hw_params_current(pcm, hw) < 0) {
        return;
    }

    rate = (unsigned int)dev->cfg.sample_rate;
    dir = 0;
    if (snd_pcm_hw_params_get_rate(hw, &rate, &dir) >= 0) {
        dev->cfg.sample_rate = (unsigned long)rate;
    }

    channels = dev->cfg.channels;
    if (snd_pcm_hw_params_get_channels(hw, &channels) >= 0) {
        dev->cfg.channels = channels;
    }

    period_frames = (snd_pcm_uframes_t)dev->cfg.frames_per_buffer;
    dir = 0;
    if (snd_pcm_hw_params_get_period_size(hw, &period_frames, &dir) >= 0 && period_frames > 0) {
        dev->cfg.frames_per_buffer = (unsigned int)period_frames;
    }

    buffer_frames = (snd_pcm_uframes_t)(dev->cfg.frames_per_buffer * dev->cfg.buffer_count);
    if (snd_pcm_hw_params_get_buffer_size(hw, &buffer_frames) >= 0 && buffer_frames > 0) {
        buffer_count = (unsigned int)(buffer_frames / (snd_pcm_uframes_t)dev->cfg.frames_per_buffer);
        if (((snd_pcm_uframes_t)buffer_count * (snd_pcm_uframes_t)dev->cfg.frames_per_buffer) < buffer_frames) {
            buffer_count += 1U;
        }
        if (buffer_count == 0U) {
            buffer_count = 1U;
        }
        dev->cfg.buffer_count = buffer_count;
        if (stream == SND_PCM_STREAM_PLAYBACK) {
            dev->output_latency_frames = (unsigned long)buffer_frames;
        } else {
            dev->input_latency_frames = (unsigned long)buffer_frames;
        }
    }
}

static int mka_alsa_configure_pcm(snd_pcm_t *pcm,
                                  knm_device *dev,
                                  snd_pcm_stream_t stream)
{
    snd_pcm_hw_params_t *hw;
    snd_pcm_sw_params_t *sw;
    snd_pcm_uframes_t period_frames;
    snd_pcm_uframes_t buffer_frames;
    unsigned int rate;
    int dir;
    int rc;

    (void)stream;

    snd_pcm_hw_params_alloca(&hw);
    snd_pcm_sw_params_alloca(&sw);

    rc = snd_pcm_hw_params_any(pcm, hw);
    if (rc < 0) {
        return KNM_DEVICE_ERROR;
    }
    rc = snd_pcm_hw_params_set_access(pcm, hw, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (rc < 0) {
        return KNM_DEVICE_ERROR;
    }
    rc = snd_pcm_hw_params_set_format(pcm, hw, SND_PCM_FORMAT_S16_LE);
    if (rc < 0) {
        return KNM_DEVICE_ERROR;
    }
    rc = snd_pcm_hw_params_set_channels(pcm, hw, dev->cfg.channels);
    if (rc < 0) {
        return KNM_DEVICE_ERROR;
    }

    rate = (unsigned int)dev->cfg.sample_rate;
    dir = 0;
    rc = snd_pcm_hw_params_set_rate_near(pcm, hw, &rate, &dir);
    if (rc < 0) {
        return KNM_DEVICE_ERROR;
    }

    period_frames = (snd_pcm_uframes_t)dev->cfg.frames_per_buffer;
    dir = 0;
    rc = snd_pcm_hw_params_set_period_size_near(pcm, hw, &period_frames, &dir);
    if (rc < 0) {
        return KNM_DEVICE_ERROR;
    }

    buffer_frames = (snd_pcm_uframes_t)(dev->cfg.frames_per_buffer * dev->cfg.buffer_count);
    dir = 0;
    rc = snd_pcm_hw_params_set_buffer_size_near(pcm, hw, &buffer_frames);
    if (rc < 0) {
        return KNM_DEVICE_ERROR;
    }

    rc = snd_pcm_hw_params(pcm, hw);
    if (rc < 0) {
        return KNM_DEVICE_ERROR;
    }

    rc = snd_pcm_sw_params_current(pcm, sw);
    if (rc < 0) {
        return KNM_DEVICE_ERROR;
    }
    rc = snd_pcm_sw_params_set_avail_min(pcm, sw, period_frames);
    if (rc < 0) {
        return KNM_DEVICE_ERROR;
    }
    rc = snd_pcm_sw_params_set_start_threshold(pcm, sw, period_frames);
    if (rc < 0) {
        return KNM_DEVICE_ERROR;
    }
    rc = snd_pcm_sw_params(pcm, sw);
    if (rc < 0) {
        return KNM_DEVICE_ERROR;
    }

    rc = snd_pcm_prepare(pcm);
    if (rc < 0) {
        return KNM_DEVICE_ERROR;
    }

    mka_alsa_capture_negotiated(pcm, dev, stream);
    return KNM_OK;
}

static void *mka_alsa_thread_proc(void *param)
{
    knm_device *dev;
    mka_alsa_state *st;
    snd_pcm_sframes_t frames;
    int rc;

    dev = (knm_device *)param;
    st = mka_alsa(dev);

    while (!st->stop_requested) {
        if (st->capture != (snd_pcm_t *)0) {
            frames = snd_pcm_readi(st->capture, dev->s16_input_scratch, dev->cfg.frames_per_buffer);
            if (frames < 0) {
                mnk_note_backend_xrun(dev);
                rc = snd_pcm_recover(st->capture, (int)frames, 1);
                if (rc < 0) {
                    usleep(1000U);
                }
            } else if (frames > 0) {
                mnk_capture_from_s16(dev, dev->s16_input_scratch, (unsigned int)frames);
            }
        }

        if (st->playback != (snd_pcm_t *)0) {
            mnk_render_to_s16(dev, dev->s16_output_scratch, dev->cfg.frames_per_buffer);
            frames = snd_pcm_writei(st->playback, dev->s16_output_scratch, dev->cfg.frames_per_buffer);
            if (frames < 0) {
                mnk_note_backend_xrun(dev);
                rc = snd_pcm_recover(st->playback, (int)frames, 1);
                if (rc < 0) {
                    usleep(1000U);
                }
            }
        } else {
            usleep(1000U);
        }
    }

    return (void *)0;
}

static int mka_alsa_open(knm_device *dev)
{
    mka_alsa_state *st;
    int rc;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }
    if ((unsigned long)sizeof(mka_alsa_state) > (unsigned long)KNM_BACKEND_STATE_BYTES) {
        return KNM_LIMIT_EXCEEDED;
    }

    st = mka_alsa(dev);
    mnk_zero((void *)st, (unsigned long)sizeof(*st));

    if (dev->cfg.enable_output) {
        rc = snd_pcm_open(&st->playback, mka_alsa_selected_device_id(dev), SND_PCM_STREAM_PLAYBACK, 0);
        if (rc < 0) {
            mka_alsa_close(dev);
            return dev->requested_cfg.device_id[0] != '\0' ? KNM_DEVICE_NOT_FOUND : KNM_DEVICE_ERROR;
        }
        rc = mka_alsa_configure_pcm(st->playback, dev, SND_PCM_STREAM_PLAYBACK);
        if (rc != KNM_OK) {
            mka_alsa_close(dev);
            return rc;
        }
    }

    if (dev->cfg.enable_input) {
        rc = snd_pcm_open(&st->capture, mka_alsa_selected_device_id(dev), SND_PCM_STREAM_CAPTURE, 0);
        if (rc < 0) {
            mka_alsa_close(dev);
            return dev->requested_cfg.device_id[0] != '\0' ? KNM_DEVICE_NOT_FOUND : KNM_DEVICE_ERROR;
        }
        rc = mka_alsa_configure_pcm(st->capture, dev, SND_PCM_STREAM_CAPTURE);
        if (rc != KNM_OK) {
            mka_alsa_close(dev);
            return rc;
        }
    }

    return KNM_OK;
}

static int mka_alsa_start(knm_device *dev)
{
    mka_alsa_state *st;
    int rc;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }

    st = mka_alsa(dev);
    st->stop_requested = 0;
    dev->running = 1;

    rc = pthread_create(&st->thread, (pthread_attr_t *)0, mka_alsa_thread_proc, (void *)dev);
    if (rc != 0) {
        dev->running = 0;
        return KNM_DEVICE_ERROR;
    }
    st->thread_started = 1;
    return KNM_OK;
}

static int mka_alsa_stop(knm_device *dev)
{
    mka_alsa_state *st;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }

    st = mka_alsa(dev);
    dev->running = 0;
    st->stop_requested = 1;

    if (st->capture != (snd_pcm_t *)0) {
        snd_pcm_drop(st->capture);
    }
    if (st->playback != (snd_pcm_t *)0) {
        snd_pcm_drop(st->playback);
    }

    if (st->thread_started) {
        pthread_join(st->thread, (void **)0);
        st->thread_started = 0;
    }

    if (st->capture != (snd_pcm_t *)0) {
        snd_pcm_prepare(st->capture);
    }
    if (st->playback != (snd_pcm_t *)0) {
        snd_pcm_prepare(st->playback);
    }

    return KNM_OK;
}

static void mka_alsa_close(knm_device *dev)
{
    mka_alsa_state *st;

    if (dev == (knm_device *)0) {
        return;
    }

    st = mka_alsa(dev);
    st->stop_requested = 1;

    if (st->thread_started) {
        pthread_join(st->thread, (void **)0);
        st->thread_started = 0;
    }

    if (st->playback != (snd_pcm_t *)0) {
        snd_pcm_close(st->playback);
        st->playback = (snd_pcm_t *)0;
    }
    if (st->capture != (snd_pcm_t *)0) {
        snd_pcm_close(st->capture);
        st->capture = (snd_pcm_t *)0;
    }
}

const knm_backend_vtbl knm_backend_alsa_vtbl = {
    KNM_BACKEND_ALSA,
    "alsa",
    1,
    1,
    1,
    0,
    mka_alsa_probe,
    (unsigned int (*)(void))0,
    (int (*)(unsigned int, knm_device_info *))0,
    mka_alsa_open,
    mka_alsa_start,
    mka_alsa_stop,
    mka_alsa_close
};

#else

static int mka_alsa_probe(void)
{
    return 0;
}

static int mka_alsa_open(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static int mka_alsa_start(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static int mka_alsa_stop(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static void mka_alsa_close(knm_device *dev)
{
    (void)dev;
}

const knm_backend_vtbl knm_backend_alsa_vtbl = {
    KNM_BACKEND_ALSA,
    "alsa",
    1,
    1,
    1,
    0,
    mka_alsa_probe,
    (unsigned int (*)(void))0,
    (int (*)(unsigned int, knm_device_info *))0,
    mka_alsa_open,
    mka_alsa_start,
    mka_alsa_stop,
    mka_alsa_close
};

#endif
