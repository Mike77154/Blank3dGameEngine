#include "../KNM_audio/mnk_core_internal.h"

#if KNM_AUDIO_ENABLE_HAIKU && defined(__HAIKU__)

#include <SoundPlayer.h>
#include <MediaDefs.h>
#include <MediaRoster.h>
#include <string.h>

typedef struct mka_haiku_state {
    BSoundPlayer *player;
    media_raw_audio_format format;
} mka_haiku_state;

static mka_haiku_state *mka_haiku(knm_device *dev)
{
    return (mka_haiku_state *)(void *)dev->backend_state;
}

static int mka_haiku_probe(void)
{
    return 1;
}


static void mka_haiku_fill_buffer(void *cookie,
                                  void *buffer,
                                  size_t size,
                                  const media_raw_audio_format &format);
static int mka_haiku_open(knm_device *dev);
static int mka_haiku_start(knm_device *dev);
static int mka_haiku_stop(knm_device *dev);
static void mka_haiku_close(knm_device *dev);

static void mka_haiku_fill_buffer(void *cookie,
                                  void *buffer,
                                  size_t size,
                                  const media_raw_audio_format &format)
{
    knm_device *dev;
    unsigned int frames;
    unsigned int bytes_per_frame;

    (void)format;

    dev = (knm_device *)cookie;
    if (!dev->running) {
        memset(buffer, 0, size);
        return;
    }

    bytes_per_frame = dev->cfg.channels * (unsigned int)sizeof(knm_s16);
    if (bytes_per_frame == 0U) {
        memset(buffer, 0, size);
        return;
    }

    frames = (unsigned int)(size / bytes_per_frame);
    mnk_render_to_s16(dev, (knm_s16 *)buffer, frames);
}

static int mka_haiku_open(knm_device *dev)
{
    mka_haiku_state *st;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }
    if (dev->cfg.enable_input) {
        return KNM_NOT_SUPPORTED;
    }
    if ((unsigned long)sizeof(mka_haiku_state) > (unsigned long)KNM_BACKEND_STATE_BYTES) {
        return KNM_LIMIT_EXCEEDED;
    }

    st = mka_haiku(dev);
    memset(st, 0, sizeof(*st));
    memset(&st->format, 0, sizeof(st->format));

    st->format.frame_rate = dev->cfg.sample_rate;
    st->format.channel_count = dev->cfg.channels;
    st->format.format = media_raw_audio_format::B_AUDIO_SHORT;
#if B_HOST_IS_LENDIAN
    st->format.byte_order = B_MEDIA_LITTLE_ENDIAN;
#else
    st->format.byte_order = B_MEDIA_BIG_ENDIAN;
#endif
    st->format.buffer_size = dev->cfg.frames_per_buffer * dev->cfg.buffer_count *
                             dev->cfg.channels * sizeof(knm_s16);

    st->player = new BSoundPlayer(&st->format,
                                  "KNM_audio",
                                  mka_haiku_fill_buffer,
                                  0,
                                  (void *)dev);
    if (st->player == 0) {
        return KNM_DEVICE_ERROR;
    }
    if (st->player->InitCheck() != B_OK) {
        delete st->player;
        st->player = 0;
        return KNM_DEVICE_ERROR;
    }

    st->player->SetVolume(1);
    return KNM_OK;
}

static int mka_haiku_start(knm_device *dev)
{
    mka_haiku_state *st;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }

    st = mka_haiku(dev);
    dev->running = 1;
    st->player->Start();
    st->player->SetHasData(true);
    return KNM_OK;
}

static int mka_haiku_stop(knm_device *dev)
{
    mka_haiku_state *st;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }

    st = mka_haiku(dev);
    dev->running = 0;
    if (st->player != 0) {
        st->player->SetHasData(false);
        st->player->Stop();
    }
    return KNM_OK;
}

static void mka_haiku_close(knm_device *dev)
{
    mka_haiku_state *st;

    if (dev == (knm_device *)0) {
        return;
    }

    st = mka_haiku(dev);
    if (st->player != 0) {
        st->player->SetHasData(false);
        st->player->Stop();
        delete st->player;
        st->player = 0;
    }
}

extern "C" const knm_backend_vtbl knm_backend_haiku_vtbl = {
    KNM_BACKEND_HAIKU,
    "haiku",
    mka_haiku_probe,
    mka_haiku_open,
    mka_haiku_start,
    mka_haiku_stop,
    mka_haiku_close
};

#else

static int mka_haiku_probe(void)
{
    return 0;
}

static int mka_haiku_open(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static int mka_haiku_start(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static int mka_haiku_stop(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static void mka_haiku_close(knm_device *dev)
{
    (void)dev;
}

extern "C" const knm_backend_vtbl knm_backend_haiku_vtbl = {
    KNM_BACKEND_HAIKU,
    "haiku",
    mka_haiku_probe,
    mka_haiku_open,
    mka_haiku_start,
    mka_haiku_stop,
    mka_haiku_close
};

#endif
