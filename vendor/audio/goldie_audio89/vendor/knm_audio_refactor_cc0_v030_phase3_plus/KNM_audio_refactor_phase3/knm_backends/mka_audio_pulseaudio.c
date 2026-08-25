#include "../KNM_audio/mnk_core_internal.h"

#if KNM_AUDIO_ENABLE_PULSEAUDIO && defined(__linux__)

#include <pthread.h>
#include <unistd.h>
#include <pulse/simple.h>
#include <pulse/error.h>

typedef struct mka_pulse_state {
    pa_simple *playback;
    pa_simple *capture;
    pthread_t thread;
    int thread_started;
    int stop_requested;
    pa_sample_spec spec;
    pa_buffer_attr playback_attr;
    pa_buffer_attr capture_attr;
} mka_pulse_state;

static mka_pulse_state *mka_pulse(knm_device *dev)
{
    return (mka_pulse_state *)(void *)dev->backend_state;
}

static int mka_pulse_probe(void)
{
    return 1;
}


static void *mka_pulse_thread_proc(void *param);
static int mka_pulse_open(knm_device *dev);
static int mka_pulse_start(knm_device *dev);
static int mka_pulse_stop(knm_device *dev);
static void mka_pulse_close(knm_device *dev);

static void *mka_pulse_thread_proc(void *param)
{
    knm_device *dev;
    mka_pulse_state *st;
    int error;

    dev = (knm_device *)param;
    st = mka_pulse(dev);
    error = 0;

    while (!st->stop_requested) {
        if (st->capture != (pa_simple *)0) {
            if (pa_simple_read(st->capture,
                               dev->s16_input_scratch,
                               (size_t)(dev->cfg.frames_per_buffer * dev->cfg.channels * sizeof(knm_s16)),
                               &error) < 0) {
                usleep(1000U);
            } else {
                mnk_capture_from_s16(dev, dev->s16_input_scratch, dev->cfg.frames_per_buffer);
            }
        }

        if (st->playback != (pa_simple *)0) {
            mnk_render_to_s16(dev, dev->s16_output_scratch, dev->cfg.frames_per_buffer);
            if (pa_simple_write(st->playback,
                                dev->s16_output_scratch,
                                (size_t)(dev->cfg.frames_per_buffer * dev->cfg.channels * sizeof(knm_s16)),
                                &error) < 0) {
                usleep(1000U);
            }
        } else {
            usleep(1000U);
        }
    }

    return (void *)0;
}

static int mka_pulse_open(knm_device *dev)
{
    mka_pulse_state *st;
    unsigned int bytes_per_frame;
    unsigned int target_bytes;
    int error;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }
    if ((unsigned long)sizeof(mka_pulse_state) > (unsigned long)KNM_BACKEND_STATE_BYTES) {
        return KNM_LIMIT_EXCEEDED;
    }

    st = mka_pulse(dev);
    mnk_zero((void *)st, (unsigned long)sizeof(*st));
    error = 0;

    st->spec.format = PA_SAMPLE_S16LE;
    st->spec.rate = (uint32_t)dev->cfg.sample_rate;
    st->spec.channels = (uint8_t)dev->cfg.channels;

    bytes_per_frame = dev->cfg.channels * (unsigned int)sizeof(knm_s16);
    target_bytes = dev->cfg.frames_per_buffer * dev->cfg.buffer_count * bytes_per_frame;

    st->playback_attr.maxlength = (uint32_t)-1;
    st->playback_attr.tlength = target_bytes;
    st->playback_attr.prebuf = (uint32_t)-1;
    st->playback_attr.minreq = (uint32_t)(dev->cfg.frames_per_buffer * bytes_per_frame);
    st->playback_attr.fragsize = (uint32_t)-1;

    st->capture_attr.maxlength = (uint32_t)-1;
    st->capture_attr.tlength = (uint32_t)-1;
    st->capture_attr.prebuf = (uint32_t)-1;
    st->capture_attr.minreq = (uint32_t)-1;
    st->capture_attr.fragsize = (uint32_t)(dev->cfg.frames_per_buffer * bytes_per_frame);

    if (dev->cfg.enable_output) {
        st->playback = pa_simple_new((const char *)0,
                                     "KNM_audio",
                                     PA_STREAM_PLAYBACK,
                                     (const char *)0,
                                     "playback",
                                     &st->spec,
                                     (const pa_channel_map *)0,
                                     &st->playback_attr,
                                     &error);
        if (st->playback == (pa_simple *)0) {
            mka_pulse_close(dev);
            return KNM_DEVICE_ERROR;
        }
    }

    if (dev->cfg.enable_input) {
        st->capture = pa_simple_new((const char *)0,
                                    "KNM_audio",
                                    PA_STREAM_RECORD,
                                    (const char *)0,
                                    "capture",
                                    &st->spec,
                                    (const pa_channel_map *)0,
                                    &st->capture_attr,
                                    &error);
        if (st->capture == (pa_simple *)0) {
            mka_pulse_close(dev);
            return KNM_DEVICE_ERROR;
        }
    }

    return KNM_OK;
}

static int mka_pulse_start(knm_device *dev)
{
    mka_pulse_state *st;
    int rc;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }

    st = mka_pulse(dev);
    st->stop_requested = 0;
    dev->running = 1;

    rc = pthread_create(&st->thread, (pthread_attr_t *)0, mka_pulse_thread_proc, (void *)dev);
    if (rc != 0) {
        dev->running = 0;
        return KNM_DEVICE_ERROR;
    }
    st->thread_started = 1;

    return KNM_OK;
}

static int mka_pulse_stop(knm_device *dev)
{
    mka_pulse_state *st;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }

    st = mka_pulse(dev);
    dev->running = 0;
    st->stop_requested = 1;

    if (st->thread_started) {
        pthread_join(st->thread, (void **)0);
        st->thread_started = 0;
    }

    return KNM_OK;
}

static void mka_pulse_close(knm_device *dev)
{
    mka_pulse_state *st;
    int error;

    if (dev == (knm_device *)0) {
        return;
    }

    st = mka_pulse(dev);
    error = 0;
    st->stop_requested = 1;

    if (st->thread_started) {
        pthread_join(st->thread, (void **)0);
        st->thread_started = 0;
    }

    if (st->playback != (pa_simple *)0) {
        pa_simple_drain(st->playback, &error);
        pa_simple_free(st->playback);
        st->playback = (pa_simple *)0;
    }
    if (st->capture != (pa_simple *)0) {
        pa_simple_free(st->capture);
        st->capture = (pa_simple *)0;
    }
}

const knm_backend_vtbl knm_backend_pulseaudio_vtbl = {
    KNM_BACKEND_PULSEAUDIO,
    "pulseaudio",
    1,
    1,
    1,
    0,
    mka_pulse_probe,
    (unsigned int (*)(void))0,
    (int (*)(unsigned int, knm_device_info *))0,
    mka_pulse_open,
    mka_pulse_start,
    mka_pulse_stop,
    mka_pulse_close
};

#else

static int mka_pulse_probe(void)
{
    return 0;
}

static int mka_pulse_open(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static int mka_pulse_start(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static int mka_pulse_stop(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static void mka_pulse_close(knm_device *dev)
{
    (void)dev;
}

const knm_backend_vtbl knm_backend_pulseaudio_vtbl = {
    KNM_BACKEND_PULSEAUDIO,
    "pulseaudio",
    1,
    1,
    1,
    0,
    mka_pulse_probe,
    (unsigned int (*)(void))0,
    (int (*)(unsigned int, knm_device_info *))0,
    mka_pulse_open,
    mka_pulse_start,
    mka_pulse_stop,
    mka_pulse_close
};

#endif
