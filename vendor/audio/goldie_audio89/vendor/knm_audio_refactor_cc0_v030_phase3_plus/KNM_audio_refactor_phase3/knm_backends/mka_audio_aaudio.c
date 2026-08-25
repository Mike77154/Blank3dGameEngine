#include "../KNM_audio/mnk_core_internal.h"

#if KNM_AUDIO_ENABLE_AAUDIO && defined(__ANDROID__)

#include <aaudio/AAudio.h>

typedef struct mka_aaudio_state {
    AAudioStream *out_stream;
    AAudioStream *in_stream;
} mka_aaudio_state;

static mka_aaudio_state *mka_aaudio(knm_device *dev)
{
    return (mka_aaudio_state *)(void *)dev->backend_state;
}

static int mka_aaudio_probe(void)
{
    return 1;
}


static aaudio_data_callback_result_t mka_aaudio_output_cb(AAudioStream *stream,
                                                          void *userData,
                                                          void *audioData,
                                                          int32_t numFrames);
static aaudio_data_callback_result_t mka_aaudio_input_cb(AAudioStream *stream,
                                                         void *userData,
                                                         void *audioData,
                                                         int32_t numFrames);
static int mka_aaudio_open_stream(knm_device *dev, AAudioStream **out_stream, aaudio_direction_t direction);
static int mka_aaudio_open(knm_device *dev);
static int mka_aaudio_start(knm_device *dev);
static int mka_aaudio_stop(knm_device *dev);
static void mka_aaudio_close(knm_device *dev);

static aaudio_data_callback_result_t mka_aaudio_output_cb(AAudioStream *stream,
                                                          void *userData,
                                                          void *audioData,
                                                          int32_t numFrames)
{
    knm_device *dev;

    (void)stream;
    dev = (knm_device *)userData;

    if (!dev->running) {
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    mnk_render_to_s16(dev, (knm_s16 *)audioData, (unsigned int)numFrames);
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

static aaudio_data_callback_result_t mka_aaudio_input_cb(AAudioStream *stream,
                                                         void *userData,
                                                         void *audioData,
                                                         int32_t numFrames)
{
    knm_device *dev;

    (void)stream;
    dev = (knm_device *)userData;

    if (!dev->running) {
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    mnk_capture_from_s16(dev, (const knm_s16 *)audioData, (unsigned int)numFrames);
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

static int mka_aaudio_open_stream(knm_device *dev, AAudioStream **out_stream, aaudio_direction_t direction)
{
    AAudioStreamBuilder *builder;
    aaudio_result_t rc;

    builder = (AAudioStreamBuilder *)0;
    rc = AAudio_createStreamBuilder(&builder);
    if (rc != AAUDIO_OK || builder == (AAudioStreamBuilder *)0) {
        return KNM_DEVICE_ERROR;
    }

    AAudioStreamBuilder_setDirection(builder, direction);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setSampleRate(builder, (int32_t)dev->cfg.sample_rate);
    AAudioStreamBuilder_setChannelCount(builder, (int32_t)dev->cfg.channels);
    AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_SHARED);
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setFramesPerDataCallback(builder, (int32_t)dev->cfg.frames_per_buffer);

    if (direction == AAUDIO_DIRECTION_OUTPUT) {
        AAudioStreamBuilder_setDataCallback(builder, mka_aaudio_output_cb, (void *)dev);
    } else {
        AAudioStreamBuilder_setDataCallback(builder, mka_aaudio_input_cb, (void *)dev);
    }

    rc = AAudioStreamBuilder_openStream(builder, out_stream);
    AAudioStreamBuilder_delete(builder);

    if (rc != AAUDIO_OK || *out_stream == (AAudioStream *)0) {
        return KNM_DEVICE_ERROR;
    }

    AAudioStream_setBufferSizeInFrames(*out_stream,
                                       (int32_t)(dev->cfg.frames_per_buffer * dev->cfg.buffer_count));
    return KNM_OK;
}

static int mka_aaudio_open(knm_device *dev)
{
    mka_aaudio_state *st;
    int rc;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }
    if ((unsigned long)sizeof(mka_aaudio_state) > (unsigned long)KNM_BACKEND_STATE_BYTES) {
        return KNM_LIMIT_EXCEEDED;
    }

    st = mka_aaudio(dev);
    mnk_zero((void *)st, (unsigned long)sizeof(*st));

    if (dev->cfg.enable_output) {
        rc = mka_aaudio_open_stream(dev, &st->out_stream, AAUDIO_DIRECTION_OUTPUT);
        if (rc != KNM_OK) {
            mka_aaudio_close(dev);
            return rc;
        }
    }
    if (dev->cfg.enable_input) {
        rc = mka_aaudio_open_stream(dev, &st->in_stream, AAUDIO_DIRECTION_INPUT);
        if (rc != KNM_OK) {
            mka_aaudio_close(dev);
            return rc;
        }
    }

    return KNM_OK;
}

static int mka_aaudio_start(knm_device *dev)
{
    mka_aaudio_state *st;
    aaudio_result_t rc;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }

    st = mka_aaudio(dev);
    dev->running = 1;

    if (st->in_stream != (AAudioStream *)0) {
        rc = AAudioStream_requestStart(st->in_stream);
        if (rc != AAUDIO_OK) {
            dev->running = 0;
            return KNM_DEVICE_ERROR;
        }
    }

    if (st->out_stream != (AAudioStream *)0) {
        rc = AAudioStream_requestStart(st->out_stream);
        if (rc != AAUDIO_OK) {
            if (st->in_stream != (AAudioStream *)0) {
                AAudioStream_requestStop(st->in_stream);
            }
            dev->running = 0;
            return KNM_DEVICE_ERROR;
        }
    }

    return KNM_OK;
}

static int mka_aaudio_stop(knm_device *dev)
{
    mka_aaudio_state *st;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }

    st = mka_aaudio(dev);
    dev->running = 0;

    if (st->out_stream != (AAudioStream *)0) {
        AAudioStream_requestStop(st->out_stream);
    }
    if (st->in_stream != (AAudioStream *)0) {
        AAudioStream_requestStop(st->in_stream);
    }

    return KNM_OK;
}

static void mka_aaudio_close(knm_device *dev)
{
    mka_aaudio_state *st;

    if (dev == (knm_device *)0) {
        return;
    }

    st = mka_aaudio(dev);

    if (st->out_stream != (AAudioStream *)0) {
        AAudioStream_close(st->out_stream);
        st->out_stream = (AAudioStream *)0;
    }
    if (st->in_stream != (AAudioStream *)0) {
        AAudioStream_close(st->in_stream);
        st->in_stream = (AAudioStream *)0;
    }
}

const knm_backend_vtbl knm_backend_aaudio_vtbl = {
    KNM_BACKEND_AAUDIO,
    "aaudio",
    1,
    1,
    1,
    0,
    mka_aaudio_probe,
    (unsigned int (*)(void))0,
    (int (*)(unsigned int, knm_device_info *))0,
    mka_aaudio_open,
    mka_aaudio_start,
    mka_aaudio_stop,
    mka_aaudio_close
};

#else

static int mka_aaudio_probe(void)
{
    return 0;
}

static int mka_aaudio_open(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static int mka_aaudio_start(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static int mka_aaudio_stop(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static void mka_aaudio_close(knm_device *dev)
{
    (void)dev;
}

const knm_backend_vtbl knm_backend_aaudio_vtbl = {
    KNM_BACKEND_AAUDIO,
    "aaudio",
    1,
    1,
    1,
    0,
    mka_aaudio_probe,
    (unsigned int (*)(void))0,
    (int (*)(unsigned int, knm_device_info *))0,
    mka_aaudio_open,
    mka_aaudio_start,
    mka_aaudio_stop,
    mka_aaudio_close
};

#endif
