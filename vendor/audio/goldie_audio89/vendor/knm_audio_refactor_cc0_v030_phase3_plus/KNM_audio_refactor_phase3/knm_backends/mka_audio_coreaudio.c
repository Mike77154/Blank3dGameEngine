#include "../KNM_audio/mnk_core_internal.h"

#if KNM_AUDIO_ENABLE_COREAUDIO && defined(__APPLE__)

#include <AudioToolbox/AudioToolbox.h>

typedef struct mka_coreaudio_state {
    AudioStreamBasicDescription format;
    AudioQueueRef out_queue;
    AudioQueueRef in_queue;
    AudioQueueBufferRef out_buffers[KNM_MAX_STREAM_BUFFERS];
    AudioQueueBufferRef in_buffers[KNM_MAX_STREAM_BUFFERS];
    int out_allocated[KNM_MAX_STREAM_BUFFERS];
    int in_allocated[KNM_MAX_STREAM_BUFFERS];
} mka_coreaudio_state;

static mka_coreaudio_state *mka_coreaudio(knm_device *dev)
{
    return (mka_coreaudio_state *)(void *)dev->backend_state;
}

static int mka_coreaudio_probe(void)
{
    return 1;
}


static void mka_coreaudio_fill_format(knm_device *dev, AudioStreamBasicDescription *fmt);
static void mka_coreaudio_output_cb(void *userData,
                                    AudioQueueRef queue,
                                    AudioQueueBufferRef buffer);
static void mka_coreaudio_input_cb(void *userData,
                                   AudioQueueRef queue,
                                   AudioQueueBufferRef buffer,
                                   const AudioTimeStamp *startTime,
                                   UInt32 numPackets,
                                   const AudioStreamPacketDescription *packetDesc);
static int mka_coreaudio_open(knm_device *dev);
static int mka_coreaudio_start(knm_device *dev);
static int mka_coreaudio_stop(knm_device *dev);
static void mka_coreaudio_close(knm_device *dev);

static void mka_coreaudio_fill_format(knm_device *dev, AudioStreamBasicDescription *fmt)
{
    mnk_zero((void *)fmt, (unsigned long)sizeof(*fmt));
    fmt->mSampleRate = (Float64)dev->cfg.sample_rate;
    fmt->mFormatID = kAudioFormatLinearPCM;
    fmt->mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    fmt->mBitsPerChannel = 16;
    fmt->mChannelsPerFrame = (UInt32)dev->cfg.channels;
    fmt->mBytesPerFrame = (UInt32)(dev->cfg.channels * sizeof(knm_s16));
    fmt->mFramesPerPacket = 1;
    fmt->mBytesPerPacket = fmt->mBytesPerFrame;
}

static void mka_coreaudio_output_cb(void *userData,
                                    AudioQueueRef queue,
                                    AudioQueueBufferRef buffer)
{
    knm_device *dev;
    unsigned int frames;
    unsigned int bytes;

    (void)queue;

    dev = (knm_device *)userData;
    frames = dev->cfg.frames_per_buffer;
    bytes = frames * dev->cfg.channels * (unsigned int)sizeof(knm_s16);

    mnk_render_to_s16(dev, (knm_s16 *)buffer->mAudioData, frames);
    buffer->mAudioDataByteSize = (UInt32)bytes;

    if (dev->running) {
        AudioQueueEnqueueBuffer(queue, buffer, 0U, (const AudioStreamPacketDescription *)0);
    }
}

static void mka_coreaudio_input_cb(void *userData,
                                   AudioQueueRef queue,
                                   AudioQueueBufferRef buffer,
                                   const AudioTimeStamp *startTime,
                                   UInt32 numPackets,
                                   const AudioStreamPacketDescription *packetDesc)
{
    knm_device *dev;
    unsigned int frames;
    unsigned int bytes;

    (void)startTime;
    (void)numPackets;
    (void)packetDesc;

    dev = (knm_device *)userData;
    bytes = (unsigned int)buffer->mAudioDataByteSize;
    frames = bytes / (dev->cfg.channels * (unsigned int)sizeof(knm_s16));

    if (frames > 0U) {
        mnk_capture_from_s16(dev, (const knm_s16 *)buffer->mAudioData, frames);
    }

    buffer->mAudioDataByteSize = (UInt32)(dev->cfg.frames_per_buffer * dev->cfg.channels * sizeof(knm_s16));
    if (dev->running) {
        AudioQueueEnqueueBuffer(queue, buffer, 0U, (const AudioStreamPacketDescription *)0);
    }
}

static int mka_coreaudio_open(knm_device *dev)
{
    mka_coreaudio_state *st;
    unsigned int i;
    UInt32 bytes;
    OSStatus os;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }
    if ((unsigned long)sizeof(mka_coreaudio_state) > (unsigned long)KNM_BACKEND_STATE_BYTES) {
        return KNM_LIMIT_EXCEEDED;
    }

    st = mka_coreaudio(dev);
    mnk_zero((void *)st, (unsigned long)sizeof(*st));

    mka_coreaudio_fill_format(dev, &st->format);
    bytes = (UInt32)(dev->cfg.frames_per_buffer * dev->cfg.channels * sizeof(knm_s16));

    if (dev->cfg.enable_output) {
        os = AudioQueueNewOutput(&st->format,
                                 mka_coreaudio_output_cb,
                                 (void *)dev,
                                 (CFRunLoopRef)0,
                                 (CFStringRef)0,
                                 0U,
                                 &st->out_queue);
        if (os != noErr) {
            mka_coreaudio_close(dev);
            return KNM_DEVICE_ERROR;
        }

        for (i = 0U; i < dev->cfg.buffer_count; ++i) {
            os = AudioQueueAllocateBuffer(st->out_queue, bytes, &st->out_buffers[i]);
            if (os != noErr) {
                mka_coreaudio_close(dev);
                return KNM_DEVICE_ERROR;
            }
            st->out_allocated[i] = 1;
        }
    }

    if (dev->cfg.enable_input) {
        os = AudioQueueNewInput(&st->format,
                                mka_coreaudio_input_cb,
                                (void *)dev,
                                (CFRunLoopRef)0,
                                (CFStringRef)0,
                                0U,
                                &st->in_queue);
        if (os != noErr) {
            mka_coreaudio_close(dev);
            return KNM_DEVICE_ERROR;
        }

        for (i = 0U; i < dev->cfg.buffer_count; ++i) {
            os = AudioQueueAllocateBuffer(st->in_queue, bytes, &st->in_buffers[i]);
            if (os != noErr) {
                mka_coreaudio_close(dev);
                return KNM_DEVICE_ERROR;
            }
            st->in_allocated[i] = 1;
            st->in_buffers[i]->mAudioDataByteSize = bytes;
            os = AudioQueueEnqueueBuffer(st->in_queue, st->in_buffers[i], 0U,
                                         (const AudioStreamPacketDescription *)0);
            if (os != noErr) {
                mka_coreaudio_close(dev);
                return KNM_DEVICE_ERROR;
            }
        }
    }

    return KNM_OK;
}

static int mka_coreaudio_start(knm_device *dev)
{
    mka_coreaudio_state *st;
    unsigned int i;
    UInt32 bytes;
    OSStatus os;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }

    st = mka_coreaudio(dev);
    bytes = (UInt32)(dev->cfg.frames_per_buffer * dev->cfg.channels * sizeof(knm_s16));
    dev->running = 1;

    if (st->out_queue != (AudioQueueRef)0) {
        for (i = 0U; i < dev->cfg.buffer_count; ++i) {
            mnk_render_to_s16(dev, (knm_s16 *)st->out_buffers[i]->mAudioData, dev->cfg.frames_per_buffer);
            st->out_buffers[i]->mAudioDataByteSize = bytes;
            os = AudioQueueEnqueueBuffer(st->out_queue, st->out_buffers[i], 0U,
                                         (const AudioStreamPacketDescription *)0);
            if (os != noErr) {
                dev->running = 0;
                return KNM_DEVICE_ERROR;
            }
        }

        os = AudioQueueStart(st->out_queue, (const AudioTimeStamp *)0);
        if (os != noErr) {
            dev->running = 0;
            return KNM_DEVICE_ERROR;
        }
    }

    if (st->in_queue != (AudioQueueRef)0) {
        os = AudioQueueStart(st->in_queue, (const AudioTimeStamp *)0);
        if (os != noErr) {
            if (st->out_queue != (AudioQueueRef)0) {
                AudioQueueStop(st->out_queue, 1);
            }
            dev->running = 0;
            return KNM_DEVICE_ERROR;
        }
    }

    return KNM_OK;
}

static int mka_coreaudio_stop(knm_device *dev)
{
    mka_coreaudio_state *st;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }

    st = mka_coreaudio(dev);
    dev->running = 0;

    if (st->out_queue != (AudioQueueRef)0) {
        AudioQueueStop(st->out_queue, 1);
    }
    if (st->in_queue != (AudioQueueRef)0) {
        AudioQueueStop(st->in_queue, 1);
    }

    return KNM_OK;
}

static void mka_coreaudio_close(knm_device *dev)
{
    mka_coreaudio_state *st;

    if (dev == (knm_device *)0) {
        return;
    }

    st = mka_coreaudio(dev);

    if (st->out_queue != (AudioQueueRef)0) {
        AudioQueueDispose(st->out_queue, 1);
        st->out_queue = (AudioQueueRef)0;
    }
    if (st->in_queue != (AudioQueueRef)0) {
        AudioQueueDispose(st->in_queue, 1);
        st->in_queue = (AudioQueueRef)0;
    }
}

const knm_backend_vtbl knm_backend_coreaudio_vtbl = {
    KNM_BACKEND_COREAUDIO,
    "coreaudio",
    1,
    1,
    1,
    0,
    mka_coreaudio_probe,
    (unsigned int (*)(void))0,
    (int (*)(unsigned int, knm_device_info *))0,
    mka_coreaudio_open,
    mka_coreaudio_start,
    mka_coreaudio_stop,
    mka_coreaudio_close
};

#else

static int mka_coreaudio_probe(void)
{
    return 0;
}

static int mka_coreaudio_open(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static int mka_coreaudio_start(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static int mka_coreaudio_stop(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static void mka_coreaudio_close(knm_device *dev)
{
    (void)dev;
}

const knm_backend_vtbl knm_backend_coreaudio_vtbl = {
    KNM_BACKEND_COREAUDIO,
    "coreaudio",
    1,
    1,
    1,
    0,
    mka_coreaudio_probe,
    (unsigned int (*)(void))0,
    (int (*)(unsigned int, knm_device_info *))0,
    mka_coreaudio_open,
    mka_coreaudio_start,
    mka_coreaudio_stop,
    mka_coreaudio_close
};

#endif
