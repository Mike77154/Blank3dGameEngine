#include "../KNM_audio/mnk_core_internal.h"

#if KNM_AUDIO_ENABLE_SNDIO && (defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__) || defined(__DragonFly__))

#include <pthread.h>
#include <unistd.h>
#include <sndio.h>

typedef struct mka_sndio_state {
    struct sio_hdl *playback;
    struct sio_hdl *capture;
    pthread_t thread;
    int thread_started;
    int stop_requested;
} mka_sndio_state;

static mka_sndio_state *mka_sndio(knm_device *dev)
{
    return (mka_sndio_state *)(void *)dev->backend_state;
}

static int mka_sndio_probe(void)
{
    return 1;
}


static int mka_sndio_configure(struct sio_hdl *hdl, knm_device *dev, int mode);
static void *mka_sndio_thread_proc(void *param);
static int mka_sndio_open(knm_device *dev);
static int mka_sndio_start(knm_device *dev);
static int mka_sndio_stop(knm_device *dev);
static void mka_sndio_close(knm_device *dev);

static int mka_sndio_configure(struct sio_hdl *hdl, knm_device *dev, int mode)
{
    struct sio_par par;

    sio_initpar(&par);
    par.bits = 16U;
    par.bps = SIO_BPS(16U);
    par.sig = 1U;
    par.le = SIO_LE_NATIVE;
    par.rate = (unsigned int)dev->cfg.sample_rate;
    par.round = dev->cfg.frames_per_buffer;
    par.appbufsz = dev->cfg.frames_per_buffer * dev->cfg.buffer_count;
    par.xrun = SIO_SYNC;

    if (mode & SIO_PLAY) {
        par.pchan = dev->cfg.channels;
    }
    if (mode & SIO_REC) {
        par.rchan = dev->cfg.channels;
    }

    if (!sio_setpar(hdl, &par)) {
        return KNM_DEVICE_ERROR;
    }
    if (!sio_getpar(hdl, &par)) {
        return KNM_DEVICE_ERROR;
    }
    if (mode & SIO_PLAY) {
        if (par.pchan != dev->cfg.channels) {
            return KNM_DEVICE_ERROR;
        }
    }
    if (mode & SIO_REC) {
        if (par.rchan != dev->cfg.channels) {
            return KNM_DEVICE_ERROR;
        }
    }

    return KNM_OK;
}

static void *mka_sndio_thread_proc(void *param)
{
    knm_device *dev;
    mka_sndio_state *st;
    unsigned int bytes;

    dev = (knm_device *)param;
    st = mka_sndio(dev);
    bytes = dev->cfg.frames_per_buffer * dev->cfg.channels * (unsigned int)sizeof(knm_s16);

    while (!st->stop_requested) {
        if (st->capture != (struct sio_hdl *)0) {
            if (sio_read(st->capture, dev->s16_input_scratch, bytes) == bytes) {
                mnk_capture_from_s16(dev, dev->s16_input_scratch, dev->cfg.frames_per_buffer);
            } else {
                usleep(1000U);
            }
        }

        if (st->playback != (struct sio_hdl *)0) {
            mnk_render_to_s16(dev, dev->s16_output_scratch, dev->cfg.frames_per_buffer);
            if (sio_write(st->playback, dev->s16_output_scratch, bytes) == 0U) {
                usleep(1000U);
            }
        } else {
            usleep(1000U);
        }
    }

    return (void *)0;
}

static int mka_sndio_open(knm_device *dev)
{
    mka_sndio_state *st;
    int rc;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }
    if ((unsigned long)sizeof(mka_sndio_state) > (unsigned long)KNM_BACKEND_STATE_BYTES) {
        return KNM_LIMIT_EXCEEDED;
    }

    st = mka_sndio(dev);
    mnk_zero((void *)st, (unsigned long)sizeof(*st));

    if (dev->cfg.enable_output) {
        st->playback = sio_open(SIO_DEVANY, SIO_PLAY, 0);
        if (st->playback == (struct sio_hdl *)0) {
            mka_sndio_close(dev);
            return KNM_DEVICE_ERROR;
        }
        rc = mka_sndio_configure(st->playback, dev, SIO_PLAY);
        if (rc != KNM_OK) {
            mka_sndio_close(dev);
            return rc;
        }
    }

    if (dev->cfg.enable_input) {
        st->capture = sio_open(SIO_DEVANY, SIO_REC, 0);
        if (st->capture == (struct sio_hdl *)0) {
            mka_sndio_close(dev);
            return KNM_DEVICE_ERROR;
        }
        rc = mka_sndio_configure(st->capture, dev, SIO_REC);
        if (rc != KNM_OK) {
            mka_sndio_close(dev);
            return rc;
        }
    }

    return KNM_OK;
}

static int mka_sndio_start(knm_device *dev)
{
    mka_sndio_state *st;
    int rc;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }

    st = mka_sndio(dev);

    if (st->playback != (struct sio_hdl *)0 && !sio_start(st->playback)) {
        return KNM_DEVICE_ERROR;
    }
    if (st->capture != (struct sio_hdl *)0 && !sio_start(st->capture)) {
        if (st->playback != (struct sio_hdl *)0) {
            sio_stop(st->playback);
        }
        return KNM_DEVICE_ERROR;
    }

    st->stop_requested = 0;
    dev->running = 1;

    rc = pthread_create(&st->thread, (pthread_attr_t *)0, mka_sndio_thread_proc, (void *)dev);
    if (rc != 0) {
        dev->running = 0;
        return KNM_DEVICE_ERROR;
    }

    st->thread_started = 1;
    return KNM_OK;
}

static int mka_sndio_stop(knm_device *dev)
{
    mka_sndio_state *st;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }

    st = mka_sndio(dev);
    dev->running = 0;
    st->stop_requested = 1;

    if (st->thread_started) {
        pthread_join(st->thread, (void **)0);
        st->thread_started = 0;
    }

    if (st->playback != (struct sio_hdl *)0) {
        sio_stop(st->playback);
    }
    if (st->capture != (struct sio_hdl *)0) {
        sio_stop(st->capture);
    }

    return KNM_OK;
}

static void mka_sndio_close(knm_device *dev)
{
    mka_sndio_state *st;

    if (dev == (knm_device *)0) {
        return;
    }

    st = mka_sndio(dev);
    st->stop_requested = 1;

    if (st->thread_started) {
        pthread_join(st->thread, (void **)0);
        st->thread_started = 0;
    }

    if (st->playback != (struct sio_hdl *)0) {
        sio_close(st->playback);
        st->playback = (struct sio_hdl *)0;
    }
    if (st->capture != (struct sio_hdl *)0) {
        sio_close(st->capture);
        st->capture = (struct sio_hdl *)0;
    }
}

const knm_backend_vtbl knm_backend_sndio_vtbl = {
    KNM_BACKEND_SNDIO,
    "sndio",
    1,
    1,
    1,
    0,
    mka_sndio_probe,
    (unsigned int (*)(void))0,
    (int (*)(unsigned int, knm_device_info *))0,
    mka_sndio_open,
    mka_sndio_start,
    mka_sndio_stop,
    mka_sndio_close
};

#else

static int mka_sndio_probe(void)
{
    return 0;
}

static int mka_sndio_open(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static int mka_sndio_start(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static int mka_sndio_stop(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static void mka_sndio_close(knm_device *dev)
{
    (void)dev;
}

const knm_backend_vtbl knm_backend_sndio_vtbl = {
    KNM_BACKEND_SNDIO,
    "sndio",
    1,
    1,
    1,
    0,
    mka_sndio_probe,
    (unsigned int (*)(void))0,
    (int (*)(unsigned int, knm_device_info *))0,
    mka_sndio_open,
    mka_sndio_start,
    mka_sndio_stop,
    mka_sndio_close
};

#endif
