#include "../KNM_audio/mnk_core_internal.h"

#if KNM_AUDIO_ENABLE_PIPEWIRE && defined(__linux__)

#include <pipewire/pipewire.h>
#include <pipewire/thread-loop.h>
#include <spa/param/audio/raw.h>
#include <spa/param/audio/format-utils.h>

typedef struct mka_pipewire_state {
    struct pw_thread_loop *loop;
    struct pw_stream *out_stream;
    struct pw_stream *in_stream;
    struct spa_audio_info_raw info;
    int loop_started;
} mka_pipewire_state;

static mka_pipewire_state *mka_pipewire(knm_device *dev)
{
    return (mka_pipewire_state *)(void *)dev->backend_state;
}

static int mka_pipewire_probe(void)
{
    return 1;
}


static void mka_pipewire_fill_info(knm_device *dev, struct spa_audio_info_raw *info);
static void mka_pipewire_process_output(void *userdata);
static void mka_pipewire_process_input(void *userdata);
static int mka_pipewire_connect_output(knm_device *dev);
static int mka_pipewire_connect_input(knm_device *dev);
static int mka_pipewire_open(knm_device *dev);
static int mka_pipewire_start(knm_device *dev);
static int mka_pipewire_stop(knm_device *dev);
static void mka_pipewire_close(knm_device *dev);

static void mka_pipewire_fill_info(knm_device *dev, struct spa_audio_info_raw *info)
{
    unsigned int i;

    info->format = SPA_AUDIO_FORMAT_S16_LE;
    info->channels = dev->cfg.channels;
    info->rate = (uint32_t)dev->cfg.sample_rate;

    for (i = 0U; i < SPA_AUDIO_MAX_CHANNELS; ++i) {
        info->position[i] = SPA_AUDIO_CHANNEL_UNKNOWN;
    }

    if (dev->cfg.channels == 1U) {
        info->position[0] = SPA_AUDIO_CHANNEL_MONO;
    } else {
        info->position[0] = SPA_AUDIO_CHANNEL_FL;
        info->position[1] = SPA_AUDIO_CHANNEL_FR;
    }
}

static void mka_pipewire_process_output(void *userdata)
{
    knm_device *dev;
    mka_pipewire_state *st;
    struct pw_buffer *buf;
    struct spa_buffer *spa_buf;
    struct spa_data *d;
    unsigned int frames;
    unsigned int bytes_per_frame;

    dev = (knm_device *)userdata;
    st = mka_pipewire(dev);
    if (st->out_stream == (struct pw_stream *)0) {
        return;
    }

    buf = pw_stream_dequeue_buffer(st->out_stream);
    if (buf == (struct pw_buffer *)0) {
        return;
    }

    spa_buf = buf->buffer;
    d = &spa_buf->datas[0];
    if (d->data == (void *)0) {
        pw_stream_queue_buffer(st->out_stream, buf);
        return;
    }

    bytes_per_frame = dev->cfg.channels * (unsigned int)sizeof(knm_s16);
    frames = (unsigned int)(d->maxsize / bytes_per_frame);
    if (frames > 0U) {
        mnk_render_to_s16(dev, (knm_s16 *)d->data, frames);
        d->chunk->offset = 0;
        d->chunk->size = frames * bytes_per_frame;
        d->chunk->stride = bytes_per_frame;
    }

    pw_stream_queue_buffer(st->out_stream, buf);
}

static void mka_pipewire_process_input(void *userdata)
{
    knm_device *dev;
    mka_pipewire_state *st;
    struct pw_buffer *buf;
    struct spa_buffer *spa_buf;
    struct spa_data *d;
    unsigned int frames;
    unsigned int bytes_per_frame;

    dev = (knm_device *)userdata;
    st = mka_pipewire(dev);
    if (st->in_stream == (struct pw_stream *)0) {
        return;
    }

    buf = pw_stream_dequeue_buffer(st->in_stream);
    if (buf == (struct pw_buffer *)0) {
        return;
    }

    spa_buf = buf->buffer;
    d = &spa_buf->datas[0];
    bytes_per_frame = dev->cfg.channels * (unsigned int)sizeof(knm_s16);

    if (d->data != (void *)0 && d->chunk != (struct spa_chunk *)0 && d->chunk->size > 0U) {
        frames = (unsigned int)(d->chunk->size / bytes_per_frame);
        mnk_capture_from_s16(dev, (const knm_s16 *)((unsigned char *)d->data + d->chunk->offset), frames);
    }

    pw_stream_queue_buffer(st->in_stream, buf);
}

static int mka_pipewire_connect_output(knm_device *dev)
{
    mka_pipewire_state *st;
    struct pw_stream_events events;
    struct spa_pod_builder b;
    struct spa_pod *params[1];
    unsigned char buffer[256];
    int flags;

    st = mka_pipewire(dev);

    mnk_zero((void *)&events, (unsigned long)sizeof(events));
    events.version = PW_VERSION_STREAM_EVENTS;
    events.process = mka_pipewire_process_output;

    st->out_stream = pw_stream_new_simple(pw_thread_loop_get_loop(st->loop),
                                          "knm-output",
                                          pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio",
                                                            PW_KEY_MEDIA_CATEGORY, "Playback",
                                                            PW_KEY_MEDIA_ROLE, "Game",
                                                            (const char *)0),
                                          &events,
                                          (void *)dev);
    if (st->out_stream == (struct pw_stream *)0) {
        return KNM_DEVICE_ERROR;
    }

    b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &st->info);

    flags = PW_STREAM_FLAG_AUTOCONNECT |
            PW_STREAM_FLAG_MAP_BUFFERS |
            PW_STREAM_FLAG_RT_PROCESS;

    if (pw_stream_connect(st->out_stream,
                          PW_DIRECTION_OUTPUT,
                          PW_ID_ANY,
                          (unsigned int)flags,
                          (const struct spa_pod **)params,
                          1U) < 0) {
        return KNM_DEVICE_ERROR;
    }

    return KNM_OK;
}

static int mka_pipewire_connect_input(knm_device *dev)
{
    mka_pipewire_state *st;
    struct pw_stream_events events;
    struct spa_pod_builder b;
    struct spa_pod *params[1];
    unsigned char buffer[256];
    int flags;

    st = mka_pipewire(dev);

    mnk_zero((void *)&events, (unsigned long)sizeof(events));
    events.version = PW_VERSION_STREAM_EVENTS;
    events.process = mka_pipewire_process_input;

    st->in_stream = pw_stream_new_simple(pw_thread_loop_get_loop(st->loop),
                                         "knm-input",
                                         pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio",
                                                           PW_KEY_MEDIA_CATEGORY, "Capture",
                                                           PW_KEY_MEDIA_ROLE, "Game",
                                                           (const char *)0),
                                         &events,
                                         (void *)dev);
    if (st->in_stream == (struct pw_stream *)0) {
        return KNM_DEVICE_ERROR;
    }

    b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &st->info);

    flags = PW_STREAM_FLAG_AUTOCONNECT |
            PW_STREAM_FLAG_MAP_BUFFERS |
            PW_STREAM_FLAG_RT_PROCESS;

    if (pw_stream_connect(st->in_stream,
                          PW_DIRECTION_INPUT,
                          PW_ID_ANY,
                          (unsigned int)flags,
                          (const struct spa_pod **)params,
                          1U) < 0) {
        return KNM_DEVICE_ERROR;
    }

    return KNM_OK;
}

static int mka_pipewire_open(knm_device *dev)
{
    mka_pipewire_state *st;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }
    if ((unsigned long)sizeof(mka_pipewire_state) > (unsigned long)KNM_BACKEND_STATE_BYTES) {
        return KNM_LIMIT_EXCEEDED;
    }

    st = mka_pipewire(dev);
    mnk_zero((void *)st, (unsigned long)sizeof(*st));

    pw_init((int *)0, (char ***)0);

    st->loop = pw_thread_loop_new("knm-pipewire", (const struct spa_dict *)0);
    if (st->loop == (struct pw_thread_loop *)0) {
        mka_pipewire_close(dev);
        return KNM_DEVICE_ERROR;
    }

    mka_pipewire_fill_info(dev, &st->info);
    return KNM_OK;
}

static int mka_pipewire_start(knm_device *dev)
{
    mka_pipewire_state *st;
    int rc;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }

    st = mka_pipewire(dev);

    rc = pw_thread_loop_start(st->loop);
    if (rc < 0) {
        return KNM_DEVICE_ERROR;
    }
    st->loop_started = 1;

    if (dev->cfg.enable_output) {
        rc = mka_pipewire_connect_output(dev);
        if (rc != KNM_OK) {
            (void)mka_pipewire_stop(dev);
            return rc;
        }
    }

    if (dev->cfg.enable_input) {
        rc = mka_pipewire_connect_input(dev);
        if (rc != KNM_OK) {
            (void)mka_pipewire_stop(dev);
            return rc;
        }
    }

    dev->running = 1;
    return KNM_OK;
}

static int mka_pipewire_stop(knm_device *dev)
{
    mka_pipewire_state *st;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }

    st = mka_pipewire(dev);
    dev->running = 0;

    if (st->out_stream != (struct pw_stream *)0) {
        pw_stream_destroy(st->out_stream);
        st->out_stream = (struct pw_stream *)0;
    }
    if (st->in_stream != (struct pw_stream *)0) {
        pw_stream_destroy(st->in_stream);
        st->in_stream = (struct pw_stream *)0;
    }
    if (st->loop_started) {
        pw_thread_loop_stop(st->loop);
        st->loop_started = 0;
    }

    return KNM_OK;
}

static void mka_pipewire_close(knm_device *dev)
{
    mka_pipewire_state *st;

    if (dev == (knm_device *)0) {
        return;
    }

    st = mka_pipewire(dev);

    if (st->out_stream != (struct pw_stream *)0) {
        pw_stream_destroy(st->out_stream);
        st->out_stream = (struct pw_stream *)0;
    }
    if (st->in_stream != (struct pw_stream *)0) {
        pw_stream_destroy(st->in_stream);
        st->in_stream = (struct pw_stream *)0;
    }
    if (st->loop_started) {
        pw_thread_loop_stop(st->loop);
        st->loop_started = 0;
    }
    if (st->loop != (struct pw_thread_loop *)0) {
        pw_thread_loop_destroy(st->loop);
        st->loop = (struct pw_thread_loop *)0;
    }

    pw_deinit();
}

const knm_backend_vtbl knm_backend_pipewire_vtbl = {
    KNM_BACKEND_PIPEWIRE,
    "pipewire",
    1,
    1,
    1,
    0,
    mka_pipewire_probe,
    (unsigned int (*)(void))0,
    (int (*)(unsigned int, knm_device_info *))0,
    mka_pipewire_open,
    mka_pipewire_start,
    mka_pipewire_stop,
    mka_pipewire_close
};

#else

static int mka_pipewire_probe(void)
{
    return 0;
}

static int mka_pipewire_open(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static int mka_pipewire_start(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static int mka_pipewire_stop(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static void mka_pipewire_close(knm_device *dev)
{
    (void)dev;
}

const knm_backend_vtbl knm_backend_pipewire_vtbl = {
    KNM_BACKEND_PIPEWIRE,
    "pipewire",
    1,
    1,
    1,
    0,
    mka_pipewire_probe,
    (unsigned int (*)(void))0,
    (int (*)(unsigned int, knm_device_info *))0,
    mka_pipewire_open,
    mka_pipewire_start,
    mka_pipewire_stop,
    mka_pipewire_close
};

#endif
