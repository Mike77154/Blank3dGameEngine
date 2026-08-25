#include "../KNM_audio/mnk_core_internal.h"

#if KNM_AUDIO_ENABLE_WINMM && (defined(_WIN32) || defined(_WIN64))

#define COBJMACROS
#include <windows.h>
#include <mmsystem.h>

typedef struct mka_winmm_state {
    int out_opened;
    int in_opened;
    HANDLE thread;
    HANDLE stop_event;
    HANDLE out_event;
    HANDLE in_event;
    DWORD thread_id;
    HWAVEOUT out_handle;
    HWAVEIN in_handle;
    WAVEFORMATEX fmt;
    WAVEHDR out_headers[KNM_MAX_STREAM_BUFFERS];
    WAVEHDR in_headers[KNM_MAX_STREAM_BUFFERS];
    int out_prepared[KNM_MAX_STREAM_BUFFERS];
    int in_prepared[KNM_MAX_STREAM_BUFFERS];
    int out_queued[KNM_MAX_STREAM_BUFFERS];
    int in_queued[KNM_MAX_STREAM_BUFFERS];
} mka_winmm_state;

static mka_winmm_state *mka_winmm(knm_device *dev)
{
    return (mka_winmm_state *)(void *)dev->backend_state;
}

static int mka_winmm_probe(void)
{
    return 1;
}


static int mka_winmm_prepare_format(knm_device *dev);
static int mka_winmm_prepare_output_headers(knm_device *dev);
static int mka_winmm_prepare_input_headers(knm_device *dev);
static void mka_winmm_unprepare_output_headers(knm_device *dev);
static void mka_winmm_unprepare_input_headers(knm_device *dev);
static int mka_winmm_queue_output_buffer(knm_device *dev, unsigned int index);
static int mka_winmm_queue_input_buffer(knm_device *dev, unsigned int index);
static void mka_winmm_service_output(knm_device *dev);
static void mka_winmm_service_input(knm_device *dev);
static DWORD WINAPI mka_winmm_thread_proc(LPVOID param);
static int mka_winmm_open(knm_device *dev);
static int mka_winmm_start(knm_device *dev);
static int mka_winmm_stop(knm_device *dev);
static void mka_winmm_close(knm_device *dev);

static int mka_winmm_prepare_format(knm_device *dev)
{
    mka_winmm_state *st;

    st = mka_winmm(dev);
    mnk_zero((void *)&st->fmt, (unsigned long)sizeof(st->fmt));
    st->fmt.wFormatTag = WAVE_FORMAT_PCM;
    st->fmt.nChannels = (WORD)dev->cfg.channels;
    st->fmt.nSamplesPerSec = (DWORD)dev->cfg.sample_rate;
    st->fmt.wBitsPerSample = 16;
    st->fmt.nBlockAlign = (WORD)(st->fmt.nChannels * 2U);
    st->fmt.nAvgBytesPerSec = st->fmt.nSamplesPerSec * (DWORD)st->fmt.nBlockAlign;
    st->fmt.cbSize = 0;
    return KNM_OK;
}

static int mka_winmm_prepare_output_headers(knm_device *dev)
{
    mka_winmm_state *st;
    unsigned int i;
    unsigned long bytes;
    MMRESULT mmr;

    st = mka_winmm(dev);
    bytes = (unsigned long)(dev->cfg.frames_per_buffer * dev->cfg.channels * sizeof(knm_s16));

    for (i = 0U; i < dev->cfg.buffer_count; ++i) {
        mnk_zero((void *)&st->out_headers[i], (unsigned long)sizeof(WAVEHDR));
        st->out_headers[i].lpData = (LPSTR)dev->queue_output[i];
        st->out_headers[i].dwBufferLength = (DWORD)bytes;
        mmr = waveOutPrepareHeader(st->out_handle, &st->out_headers[i], (UINT)sizeof(WAVEHDR));
        if (mmr != MMSYSERR_NOERROR) {
            return KNM_DEVICE_ERROR;
        }
        st->out_prepared[i] = 1;
        st->out_queued[i] = 0;
    }
    return KNM_OK;
}

static int mka_winmm_prepare_input_headers(knm_device *dev)
{
    mka_winmm_state *st;
    unsigned int i;
    unsigned long bytes;
    MMRESULT mmr;

    st = mka_winmm(dev);
    bytes = (unsigned long)(dev->cfg.frames_per_buffer * dev->cfg.channels * sizeof(knm_s16));

    for (i = 0U; i < dev->cfg.buffer_count; ++i) {
        mnk_zero((void *)&st->in_headers[i], (unsigned long)sizeof(WAVEHDR));
        st->in_headers[i].lpData = (LPSTR)dev->queue_input[i];
        st->in_headers[i].dwBufferLength = (DWORD)bytes;
        mmr = waveInPrepareHeader(st->in_handle, &st->in_headers[i], (UINT)sizeof(WAVEHDR));
        if (mmr != MMSYSERR_NOERROR) {
            return KNM_DEVICE_ERROR;
        }
        st->in_prepared[i] = 1;
        st->in_queued[i] = 0;
    }
    return KNM_OK;
}

static void mka_winmm_unprepare_output_headers(knm_device *dev)
{
    mka_winmm_state *st;
    unsigned int i;

    st = mka_winmm(dev);
    for (i = 0U; i < dev->cfg.buffer_count; ++i) {
        if (st->out_prepared[i]) {
            waveOutUnprepareHeader(st->out_handle, &st->out_headers[i], (UINT)sizeof(WAVEHDR));
            st->out_prepared[i] = 0;
            st->out_queued[i] = 0;
        }
    }
}

static void mka_winmm_unprepare_input_headers(knm_device *dev)
{
    mka_winmm_state *st;
    unsigned int i;

    st = mka_winmm(dev);
    for (i = 0U; i < dev->cfg.buffer_count; ++i) {
        if (st->in_prepared[i]) {
            waveInUnprepareHeader(st->in_handle, &st->in_headers[i], (UINT)sizeof(WAVEHDR));
            st->in_prepared[i] = 0;
            st->in_queued[i] = 0;
        }
    }
}

static int mka_winmm_queue_output_buffer(knm_device *dev, unsigned int index)
{
    mka_winmm_state *st;
    unsigned int samples;
    MMRESULT mmr;

    st = mka_winmm(dev);
    samples = dev->cfg.frames_per_buffer * dev->cfg.channels;

    mnk_render_to_s16(dev, dev->queue_output[index], dev->cfg.frames_per_buffer);

    st->out_headers[index].dwBufferLength = (DWORD)(samples * sizeof(knm_s16));
    st->out_headers[index].dwBytesRecorded = 0;
    st->out_headers[index].dwFlags &= (DWORD)(~WHDR_DONE);

    mmr = waveOutWrite(st->out_handle, &st->out_headers[index], (UINT)sizeof(WAVEHDR));
    if (mmr != MMSYSERR_NOERROR) {
        return KNM_DEVICE_ERROR;
    }
    st->out_queued[index] = 1;
    return KNM_OK;
}

static int mka_winmm_queue_input_buffer(knm_device *dev, unsigned int index)
{
    mka_winmm_state *st;
    MMRESULT mmr;

    st = mka_winmm(dev);
    st->in_headers[index].dwBytesRecorded = 0;
    st->in_headers[index].dwFlags &= (DWORD)(~WHDR_DONE);

    mmr = waveInAddBuffer(st->in_handle, &st->in_headers[index], (UINT)sizeof(WAVEHDR));
    if (mmr != MMSYSERR_NOERROR) {
        return KNM_DEVICE_ERROR;
    }
    st->in_queued[index] = 1;
    return KNM_OK;
}

static void mka_winmm_service_output(knm_device *dev)
{
    mka_winmm_state *st;
    unsigned int i;

    st = mka_winmm(dev);
    for (i = 0U; i < dev->cfg.buffer_count; ++i) {
        if (st->out_queued[i] && (st->out_headers[i].dwFlags & WHDR_DONE)) {
            st->out_queued[i] = 0;
            if (dev->running) {
                (void)mka_winmm_queue_output_buffer(dev, i);
            }
        }
    }
}

static void mka_winmm_service_input(knm_device *dev)
{
    mka_winmm_state *st;
    unsigned int i;
    unsigned int frames;
    unsigned int bytes_per_frame;

    st = mka_winmm(dev);
    bytes_per_frame = dev->cfg.channels * (unsigned int)sizeof(knm_s16);

    for (i = 0U; i < dev->cfg.buffer_count; ++i) {
        if (st->in_queued[i] && (st->in_headers[i].dwFlags & WHDR_DONE)) {
            st->in_queued[i] = 0;
            frames = (unsigned int)(st->in_headers[i].dwBytesRecorded / (DWORD)bytes_per_frame);
            if (frames > 0U) {
                mnk_capture_from_s16(dev, dev->queue_input[i], frames);
            }
            if (dev->running) {
                (void)mka_winmm_queue_input_buffer(dev, i);
            }
        }
    }
}

static DWORD WINAPI mka_winmm_thread_proc(LPVOID param)
{
    knm_device *dev;
    mka_winmm_state *st;
    HANDLE waits[3];
    DWORD count;
    DWORD rc;

    dev = (knm_device *)param;
    st = mka_winmm(dev);

    count = 0UL;
    waits[count++] = st->stop_event;
    if (st->out_opened) {
        waits[count++] = st->out_event;
    }
    if (st->in_opened) {
        waits[count++] = st->in_event;
    }

    for (;;) {
        rc = WaitForMultipleObjects(count, waits, FALSE, INFINITE);
        if (rc == WAIT_OBJECT_0) {
            break;
        }
        if (st->out_opened) {
            mka_winmm_service_output(dev);
        }
        if (st->in_opened) {
            mka_winmm_service_input(dev);
        }
    }

    return 0UL;
}

static int mka_winmm_open(knm_device *dev)
{
    mka_winmm_state *st;
    MMRESULT mmr;
    int rc;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }
    if ((unsigned long)sizeof(mka_winmm_state) > (unsigned long)KNM_BACKEND_STATE_BYTES) {
        return KNM_LIMIT_EXCEEDED;
    }

    st = mka_winmm(dev);
    mnk_zero((void *)st, (unsigned long)sizeof(*st));

    rc = mka_winmm_prepare_format(dev);
    if (rc != KNM_OK) {
        return rc;
    }

    st->stop_event = CreateEvent((LPSECURITY_ATTRIBUTES)0, TRUE, FALSE, (LPCTSTR)0);
    if (st->stop_event == (HANDLE)0) {
        mka_winmm_close(dev);
        return KNM_DEVICE_ERROR;
    }

    if (dev->cfg.enable_output) {
        st->out_event = CreateEvent((LPSECURITY_ATTRIBUTES)0, FALSE, FALSE, (LPCTSTR)0);
        if (st->out_event == (HANDLE)0) {
            mka_winmm_close(dev);
            return KNM_DEVICE_ERROR;
        }

        mmr = waveOutOpen(&st->out_handle,
                          WAVE_MAPPER,
                          &st->fmt,
                          (DWORD_PTR)st->out_event,
                          0UL,
                          CALLBACK_EVENT);
        if (mmr != MMSYSERR_NOERROR) {
            mka_winmm_close(dev);
            return KNM_DEVICE_ERROR;
        }
        st->out_opened = 1;

        rc = mka_winmm_prepare_output_headers(dev);
        if (rc != KNM_OK) {
            mka_winmm_close(dev);
            return rc;
        }
    }

    if (dev->cfg.enable_input) {
        st->in_event = CreateEvent((LPSECURITY_ATTRIBUTES)0, FALSE, FALSE, (LPCTSTR)0);
        if (st->in_event == (HANDLE)0) {
            mka_winmm_close(dev);
            return KNM_DEVICE_ERROR;
        }

        mmr = waveInOpen(&st->in_handle,
                         WAVE_MAPPER,
                         &st->fmt,
                         (DWORD_PTR)st->in_event,
                         0UL,
                         CALLBACK_EVENT);
        if (mmr != MMSYSERR_NOERROR) {
            mka_winmm_close(dev);
            return KNM_DEVICE_ERROR;
        }
        st->in_opened = 1;

        rc = mka_winmm_prepare_input_headers(dev);
        if (rc != KNM_OK) {
            mka_winmm_close(dev);
            return rc;
        }
    }

    return KNM_OK;
}

static int mka_winmm_start(knm_device *dev)
{
    mka_winmm_state *st;
    unsigned int i;
    MMRESULT mmr;
    int rc;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }

    st = mka_winmm(dev);
    ResetEvent(st->stop_event);
    if (st->out_event != (HANDLE)0) {
        ResetEvent(st->out_event);
    }
    if (st->in_event != (HANDLE)0) {
        ResetEvent(st->in_event);
    }

    dev->running = 1;

    st->thread = CreateThread((LPSECURITY_ATTRIBUTES)0,
                              0UL,
                              mka_winmm_thread_proc,
                              (LPVOID)dev,
                              0UL,
                              &st->thread_id);
    if (st->thread == (HANDLE)0) {
        dev->running = 0;
        return KNM_DEVICE_ERROR;
    }

    if (dev->cfg.enable_input) {
        for (i = 0U; i < dev->cfg.buffer_count; ++i) {
            rc = mka_winmm_queue_input_buffer(dev, i);
            if (rc != KNM_OK) {
                (void)mka_winmm_stop(dev);
                return rc;
            }
        }
        mmr = waveInStart(st->in_handle);
        if (mmr != MMSYSERR_NOERROR) {
            (void)mka_winmm_stop(dev);
            return KNM_DEVICE_ERROR;
        }
    }

    if (dev->cfg.enable_output) {
        for (i = 0U; i < dev->cfg.buffer_count; ++i) {
            rc = mka_winmm_queue_output_buffer(dev, i);
            if (rc != KNM_OK) {
                (void)mka_winmm_stop(dev);
                return rc;
            }
        }
    }

    return KNM_OK;
}

static int mka_winmm_stop(knm_device *dev)
{
    mka_winmm_state *st;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }

    st = mka_winmm(dev);
    dev->running = 0;

    if (st->in_opened) {
        waveInStop(st->in_handle);
        waveInReset(st->in_handle);
    }
    if (st->out_opened) {
        waveOutReset(st->out_handle);
    }
    if (st->stop_event != (HANDLE)0) {
        SetEvent(st->stop_event);
    }
    if (st->thread != (HANDLE)0) {
        WaitForSingleObject(st->thread, INFINITE);
        CloseHandle(st->thread);
        st->thread = (HANDLE)0;
    }

    return KNM_OK;
}

static void mka_winmm_close(knm_device *dev)
{
    mka_winmm_state *st;

    if (dev == (knm_device *)0) {
        return;
    }

    st = mka_winmm(dev);

    if (st->thread != (HANDLE)0) {
        SetEvent(st->stop_event);
        WaitForSingleObject(st->thread, INFINITE);
        CloseHandle(st->thread);
        st->thread = (HANDLE)0;
    }

    if (st->out_opened) {
        waveOutReset(st->out_handle);
        mka_winmm_unprepare_output_headers(dev);
        waveOutClose(st->out_handle);
        st->out_opened = 0;
    }
    if (st->in_opened) {
        waveInReset(st->in_handle);
        mka_winmm_unprepare_input_headers(dev);
        waveInClose(st->in_handle);
        st->in_opened = 0;
    }

    if (st->out_event != (HANDLE)0) {
        CloseHandle(st->out_event);
        st->out_event = (HANDLE)0;
    }
    if (st->in_event != (HANDLE)0) {
        CloseHandle(st->in_event);
        st->in_event = (HANDLE)0;
    }
    if (st->stop_event != (HANDLE)0) {
        CloseHandle(st->stop_event);
        st->stop_event = (HANDLE)0;
    }
}

const knm_backend_vtbl knm_backend_winmm_vtbl = {
    KNM_BACKEND_WINMM,
    "winmm",
    0,
    1,
    0,
    0,
    mka_winmm_probe,
    (unsigned int (*)(void))0,
    (int (*)(unsigned int, knm_device_info *))0,
    mka_winmm_open,
    mka_winmm_start,
    mka_winmm_stop,
    mka_winmm_close
};

#else

static int mka_winmm_probe(void)
{
    return 0;
}

static int mka_winmm_open(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static int mka_winmm_start(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static int mka_winmm_stop(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static void mka_winmm_close(knm_device *dev)
{
    (void)dev;
}

const knm_backend_vtbl knm_backend_winmm_vtbl = {
    KNM_BACKEND_WINMM,
    "winmm",
    0,
    1,
    0,
    0,
    mka_winmm_probe,
    (unsigned int (*)(void))0,
    (int (*)(unsigned int, knm_device_info *))0,
    mka_winmm_open,
    mka_winmm_start,
    mka_winmm_stop,
    mka_winmm_close
};

#endif
