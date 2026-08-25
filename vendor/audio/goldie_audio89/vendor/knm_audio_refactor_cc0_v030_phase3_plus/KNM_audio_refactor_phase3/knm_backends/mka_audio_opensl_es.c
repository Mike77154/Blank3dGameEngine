#include "../KNM_audio/mnk_core_internal.h"

#if KNM_AUDIO_ENABLE_OPENSL_ES && defined(__ANDROID__)

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

typedef struct mka_opensl_state {
    SLObjectItf engine_object;
    SLEngineItf engine_engine;
    SLObjectItf output_mix_object;

    SLObjectItf player_object;
    SLPlayItf player_play;
    SLAndroidSimpleBufferQueueItf player_queue;

    SLObjectItf recorder_object;
    SLRecordItf recorder_record;
    SLAndroidSimpleBufferQueueItf recorder_queue;

    unsigned int next_output;
    unsigned int next_input;
} mka_opensl_state;

static mka_opensl_state *mka_opensl(knm_device *dev)
{
    return (mka_opensl_state *)(void *)dev->backend_state;
}

static int mka_opensl_probe(void)
{
    return 1;
}


static void mka_opensl_queue_output(knm_device *dev, unsigned int index);
static void mka_opensl_player_cb(SLAndroidSimpleBufferQueueItf queue, void *context);
static void mka_opensl_recorder_cb(SLAndroidSimpleBufferQueueItf queue, void *context);
static int mka_opensl_create_engine(knm_device *dev);
static int mka_opensl_create_player(knm_device *dev);
static int mka_opensl_create_recorder(knm_device *dev);
static int mka_opensl_open(knm_device *dev);
static int mka_opensl_start(knm_device *dev);
static int mka_opensl_stop(knm_device *dev);
static void mka_opensl_close(knm_device *dev);

static SLuint32 mka_opensl_channel_mask(unsigned int channels)
{
    if (channels == 1U) {
        return SL_SPEAKER_FRONT_CENTER;
    }
    return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
}

static void mka_opensl_queue_output(knm_device *dev, unsigned int index)
{
    mka_opensl_state *st;
    unsigned int bytes;

    st = mka_opensl(dev);
    bytes = dev->cfg.frames_per_buffer * dev->cfg.channels * (unsigned int)sizeof(knm_s16);

    mnk_render_to_s16(dev, dev->queue_output[index], dev->cfg.frames_per_buffer);
    (*st->player_queue)->Enqueue(st->player_queue, dev->queue_output[index], (SLuint32)bytes);
}

static void mka_opensl_player_cb(SLAndroidSimpleBufferQueueItf queue, void *context)
{
    knm_device *dev;
    mka_opensl_state *st;
    unsigned int index;

    (void)queue;

    dev = (knm_device *)context;
    st = mka_opensl(dev);

    if (!dev->running) {
        return;
    }

    index = st->next_output;
    mka_opensl_queue_output(dev, index);
    st->next_output = (index + 1U) % dev->cfg.buffer_count;
}

static void mka_opensl_recorder_cb(SLAndroidSimpleBufferQueueItf queue, void *context)
{
    knm_device *dev;
    mka_opensl_state *st;
    unsigned int index;
    unsigned int bytes;

    (void)queue;

    dev = (knm_device *)context;
    st = mka_opensl(dev);

    if (!dev->running) {
        return;
    }

    index = st->next_input;
    bytes = dev->cfg.frames_per_buffer * dev->cfg.channels * (unsigned int)sizeof(knm_s16);

    mnk_capture_from_s16(dev, dev->queue_input[index], dev->cfg.frames_per_buffer);
    (*st->recorder_queue)->Enqueue(st->recorder_queue, dev->queue_input[index], (SLuint32)bytes);

    st->next_input = (index + 1U) % dev->cfg.buffer_count;
}

static int mka_opensl_create_engine(knm_device *dev)
{
    mka_opensl_state *st;
    SLresult slrc;

    st = mka_opensl(dev);

    slrc = slCreateEngine(&st->engine_object, 0U, (const SLEngineOption *)0,
                          0U, (const SLInterfaceID *)0, (const SLboolean *)0);
    if (slrc != SL_RESULT_SUCCESS) {
        return KNM_DEVICE_ERROR;
    }

    slrc = (*st->engine_object)->Realize(st->engine_object, SL_BOOLEAN_FALSE);
    if (slrc != SL_RESULT_SUCCESS) {
        return KNM_DEVICE_ERROR;
    }

    slrc = (*st->engine_object)->GetInterface(st->engine_object, SL_IID_ENGINE, &st->engine_engine);
    if (slrc != SL_RESULT_SUCCESS) {
        return KNM_DEVICE_ERROR;
    }

    slrc = (*st->engine_engine)->CreateOutputMix(st->engine_engine,
                                                 &st->output_mix_object,
                                                 0U,
                                                 (const SLInterfaceID *)0,
                                                 (const SLboolean *)0);
    if (slrc != SL_RESULT_SUCCESS) {
        return KNM_DEVICE_ERROR;
    }

    slrc = (*st->output_mix_object)->Realize(st->output_mix_object, SL_BOOLEAN_FALSE);
    if (slrc != SL_RESULT_SUCCESS) {
        return KNM_DEVICE_ERROR;
    }

    return KNM_OK;
}

static int mka_opensl_create_player(knm_device *dev)
{
    mka_opensl_state *st;
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq;
    SLDataFormat_PCM format_pcm;
    SLDataSource audio_src;
    SLDataLocator_OutputMix loc_outmix;
    SLDataSink audio_sink;
    SLInterfaceID ids[1];
    SLboolean req[1];
    SLresult slrc;

    st = mka_opensl(dev);

    loc_bufq.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    loc_bufq.numBuffers = dev->cfg.buffer_count;

    format_pcm.formatType = SL_DATAFORMAT_PCM;
    format_pcm.numChannels = dev->cfg.channels;
    format_pcm.samplesPerSec = (SLuint32)(dev->cfg.sample_rate * 1000UL);
    format_pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
    format_pcm.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
    format_pcm.channelMask = mka_opensl_channel_mask(dev->cfg.channels);
    format_pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;

    audio_src.pLocator = &loc_bufq;
    audio_src.pFormat = &format_pcm;

    loc_outmix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
    loc_outmix.outputMix = st->output_mix_object;

    audio_sink.pLocator = &loc_outmix;
    audio_sink.pFormat = (void *)0;

    ids[0] = SL_IID_ANDROIDSIMPLEBUFFERQUEUE;
    req[0] = SL_BOOLEAN_TRUE;

    slrc = (*st->engine_engine)->CreateAudioPlayer(st->engine_engine,
                                                   &st->player_object,
                                                   &audio_src,
                                                   &audio_sink,
                                                   1U,
                                                   ids,
                                                   req);
    if (slrc != SL_RESULT_SUCCESS) {
        return KNM_DEVICE_ERROR;
    }

    slrc = (*st->player_object)->Realize(st->player_object, SL_BOOLEAN_FALSE);
    if (slrc != SL_RESULT_SUCCESS) {
        return KNM_DEVICE_ERROR;
    }

    slrc = (*st->player_object)->GetInterface(st->player_object, SL_IID_PLAY, &st->player_play);
    if (slrc != SL_RESULT_SUCCESS) {
        return KNM_DEVICE_ERROR;
    }

    slrc = (*st->player_object)->GetInterface(st->player_object,
                                              SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                                              &st->player_queue);
    if (slrc != SL_RESULT_SUCCESS) {
        return KNM_DEVICE_ERROR;
    }

    slrc = (*st->player_queue)->RegisterCallback(st->player_queue, mka_opensl_player_cb, (void *)dev);
    if (slrc != SL_RESULT_SUCCESS) {
        return KNM_DEVICE_ERROR;
    }

    return KNM_OK;
}

static int mka_opensl_create_recorder(knm_device *dev)
{
    mka_opensl_state *st;
    SLDataLocator_IODevice loc_dev;
    SLDataSource audio_src;
    SLDataLocator_AndroidSimpleBufferQueue loc_bq;
    SLDataFormat_PCM format_pcm;
    SLDataSink audio_sink;
    SLInterfaceID ids[1];
    SLboolean req[1];
    SLresult slrc;

    st = mka_opensl(dev);

    loc_dev.locatorType = SL_DATALOCATOR_IODEVICE;
    loc_dev.deviceType = SL_IODEVICE_AUDIOINPUT;
    loc_dev.deviceID = SL_DEFAULTDEVICEID_AUDIOINPUT;
    loc_dev.device = (SLObjectItf)0;

    audio_src.pLocator = &loc_dev;
    audio_src.pFormat = (void *)0;

    loc_bq.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    loc_bq.numBuffers = dev->cfg.buffer_count;

    format_pcm.formatType = SL_DATAFORMAT_PCM;
    format_pcm.numChannels = dev->cfg.channels;
    format_pcm.samplesPerSec = (SLuint32)(dev->cfg.sample_rate * 1000UL);
    format_pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
    format_pcm.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
    format_pcm.channelMask = mka_opensl_channel_mask(dev->cfg.channels);
    format_pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;

    audio_sink.pLocator = &loc_bq;
    audio_sink.pFormat = &format_pcm;

    ids[0] = SL_IID_ANDROIDSIMPLEBUFFERQUEUE;
    req[0] = SL_BOOLEAN_TRUE;

    slrc = (*st->engine_engine)->CreateAudioRecorder(st->engine_engine,
                                                     &st->recorder_object,
                                                     &audio_src,
                                                     &audio_sink,
                                                     1U,
                                                     ids,
                                                     req);
    if (slrc != SL_RESULT_SUCCESS) {
        return KNM_DEVICE_ERROR;
    }

    slrc = (*st->recorder_object)->Realize(st->recorder_object, SL_BOOLEAN_FALSE);
    if (slrc != SL_RESULT_SUCCESS) {
        return KNM_DEVICE_ERROR;
    }

    slrc = (*st->recorder_object)->GetInterface(st->recorder_object, SL_IID_RECORD, &st->recorder_record);
    if (slrc != SL_RESULT_SUCCESS) {
        return KNM_DEVICE_ERROR;
    }

    slrc = (*st->recorder_object)->GetInterface(st->recorder_object,
                                                SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                                                &st->recorder_queue);
    if (slrc != SL_RESULT_SUCCESS) {
        return KNM_DEVICE_ERROR;
    }

    slrc = (*st->recorder_queue)->RegisterCallback(st->recorder_queue, mka_opensl_recorder_cb, (void *)dev);
    if (slrc != SL_RESULT_SUCCESS) {
        return KNM_DEVICE_ERROR;
    }

    return KNM_OK;
}

static int mka_opensl_open(knm_device *dev)
{
    mka_opensl_state *st;
    int rc;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }
    if ((unsigned long)sizeof(mka_opensl_state) > (unsigned long)KNM_BACKEND_STATE_BYTES) {
        return KNM_LIMIT_EXCEEDED;
    }

    st = mka_opensl(dev);
    mnk_zero((void *)st, (unsigned long)sizeof(*st));

    rc = mka_opensl_create_engine(dev);
    if (rc != KNM_OK) {
        mka_opensl_close(dev);
        return rc;
    }

    if (dev->cfg.enable_output) {
        rc = mka_opensl_create_player(dev);
        if (rc != KNM_OK) {
            mka_opensl_close(dev);
            return rc;
        }
    }

    if (dev->cfg.enable_input) {
        rc = mka_opensl_create_recorder(dev);
        if (rc != KNM_OK) {
            mka_opensl_close(dev);
            return rc;
        }
    }

    return KNM_OK;
}

static int mka_opensl_start(knm_device *dev)
{
    mka_opensl_state *st;
    unsigned int i;
    unsigned int bytes;
    SLresult slrc;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }

    st = mka_opensl(dev);
    bytes = dev->cfg.frames_per_buffer * dev->cfg.channels * (unsigned int)sizeof(knm_s16);
    st->next_output = 0U;
    st->next_input = 0U;
    dev->running = 1;

    if (st->player_play != (SLPlayItf)0) {
        (*st->player_queue)->Clear(st->player_queue);
        for (i = 0U; i < dev->cfg.buffer_count; ++i) {
            mnk_render_to_s16(dev, dev->queue_output[i], dev->cfg.frames_per_buffer);
            (*st->player_queue)->Enqueue(st->player_queue, dev->queue_output[i], (SLuint32)bytes);
        }

        slrc = (*st->player_play)->SetPlayState(st->player_play, SL_PLAYSTATE_PLAYING);
        if (slrc != SL_RESULT_SUCCESS) {
            dev->running = 0;
            return KNM_DEVICE_ERROR;
        }
    }

    if (st->recorder_record != (SLRecordItf)0) {
        (*st->recorder_queue)->Clear(st->recorder_queue);
        for (i = 0U; i < dev->cfg.buffer_count; ++i) {
            mnk_zero_s16(dev->queue_input[i], dev->cfg.frames_per_buffer, dev->cfg.channels);
            (*st->recorder_queue)->Enqueue(st->recorder_queue, dev->queue_input[i], (SLuint32)bytes);
        }

        slrc = (*st->recorder_record)->SetRecordState(st->recorder_record, SL_RECORDSTATE_RECORDING);
        if (slrc != SL_RESULT_SUCCESS) {
            dev->running = 0;
            return KNM_DEVICE_ERROR;
        }
    }

    return KNM_OK;
}

static int mka_opensl_stop(knm_device *dev)
{
    mka_opensl_state *st;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }

    st = mka_opensl(dev);
    dev->running = 0;

    if (st->player_play != (SLPlayItf)0) {
        (*st->player_play)->SetPlayState(st->player_play, SL_PLAYSTATE_STOPPED);
    }
    if (st->player_queue != (SLAndroidSimpleBufferQueueItf)0) {
        (*st->player_queue)->Clear(st->player_queue);
    }

    if (st->recorder_record != (SLRecordItf)0) {
        (*st->recorder_record)->SetRecordState(st->recorder_record, SL_RECORDSTATE_STOPPED);
    }
    if (st->recorder_queue != (SLAndroidSimpleBufferQueueItf)0) {
        (*st->recorder_queue)->Clear(st->recorder_queue);
    }

    return KNM_OK;
}

static void mka_opensl_close(knm_device *dev)
{
    mka_opensl_state *st;

    if (dev == (knm_device *)0) {
        return;
    }

    st = mka_opensl(dev);

    if (st->player_object != (SLObjectItf)0) {
        (*st->player_object)->Destroy(st->player_object);
        st->player_object = (SLObjectItf)0;
        st->player_play = (SLPlayItf)0;
        st->player_queue = (SLAndroidSimpleBufferQueueItf)0;
    }
    if (st->recorder_object != (SLObjectItf)0) {
        (*st->recorder_object)->Destroy(st->recorder_object);
        st->recorder_object = (SLObjectItf)0;
        st->recorder_record = (SLRecordItf)0;
        st->recorder_queue = (SLAndroidSimpleBufferQueueItf)0;
    }
    if (st->output_mix_object != (SLObjectItf)0) {
        (*st->output_mix_object)->Destroy(st->output_mix_object);
        st->output_mix_object = (SLObjectItf)0;
    }
    if (st->engine_object != (SLObjectItf)0) {
        (*st->engine_object)->Destroy(st->engine_object);
        st->engine_object = (SLObjectItf)0;
        st->engine_engine = (SLEngineItf)0;
    }
}

const knm_backend_vtbl knm_backend_opensl_es_vtbl = {
    KNM_BACKEND_OPENSL_ES,
    "opensl_es",
    1,
    1,
    1,
    0,
    mka_opensl_probe,
    (unsigned int (*)(void))0,
    (int (*)(unsigned int, knm_device_info *))0,
    mka_opensl_open,
    mka_opensl_start,
    mka_opensl_stop,
    mka_opensl_close
};

#else

static int mka_opensl_probe(void)
{
    return 0;
}

static int mka_opensl_open(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static int mka_opensl_start(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static int mka_opensl_stop(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static void mka_opensl_close(knm_device *dev)
{
    (void)dev;
}

const knm_backend_vtbl knm_backend_opensl_es_vtbl = {
    KNM_BACKEND_OPENSL_ES,
    "opensl_es",
    1,
    1,
    1,
    0,
    mka_opensl_probe,
    (unsigned int (*)(void))0,
    (int (*)(unsigned int, knm_device_info *))0,
    mka_opensl_open,
    mka_opensl_start,
    mka_opensl_stop,
    mka_opensl_close
};

#endif
