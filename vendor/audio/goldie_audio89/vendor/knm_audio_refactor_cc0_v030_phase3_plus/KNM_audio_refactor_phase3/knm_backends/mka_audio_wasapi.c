#include "../KNM_audio/mnk_core_internal.h"

#if KNM_AUDIO_ENABLE_WASAPI && (defined(_WIN32) || defined(_WIN64))

#define COBJMACROS
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <objbase.h>

typedef struct mka_wasapi_state {
    int com_initialized;
    HANDLE thread;
    DWORD thread_id;
    HANDLE stop_event;
    HANDLE out_event;
    HANDLE in_event;

    IMMDeviceEnumerator *enumerator;
    IMMDevice *out_device;
    IMMDevice *in_device;

    IAudioClient *out_client;
    IAudioClient *in_client;
    IAudioRenderClient *render_client;
    IAudioCaptureClient *capture_client;

    WAVEFORMATEX fmt;
    UINT32 out_buffer_frames;
    UINT32 in_buffer_frames;
} mka_wasapi_state;

static mka_wasapi_state *mka_wasapi(knm_device *dev)
{
    return (mka_wasapi_state *)(void *)dev->backend_state;
}

static int mka_wasapi_probe(void)
{
    return 1;
}


static REFERENCE_TIME mka_wasapi_buffer_time(const knm_device *dev);
static void mka_wasapi_fill_wave_format(knm_device *dev, WAVEFORMATEX *fmt);
static int mka_wasapi_create_enumerator(mka_wasapi_state *st);
static int mka_wasapi_open_output(knm_device *dev);
static int mka_wasapi_open_input(knm_device *dev);
static void mka_wasapi_render_once(knm_device *dev, UINT32 frames);
static void mka_wasapi_service_output(knm_device *dev);
static void mka_wasapi_service_input(knm_device *dev);
static DWORD WINAPI mka_wasapi_thread_proc(LPVOID param);
static int mka_wasapi_open(knm_device *dev);
static int mka_wasapi_start(knm_device *dev);
static int mka_wasapi_stop(knm_device *dev);
static void mka_wasapi_close(knm_device *dev);

static REFERENCE_TIME mka_wasapi_buffer_time(const knm_device *dev)
{
    unsigned long frames;
    unsigned long secs_q;
    unsigned long secs_r;
    unsigned long ticks;

    frames = (unsigned long)(dev->cfg.frames_per_buffer * dev->cfg.buffer_count);
    secs_q = 10000000UL / dev->cfg.sample_rate;
    secs_r = 10000000UL % dev->cfg.sample_rate;
    ticks = (frames * secs_q) + ((frames * secs_r) / dev->cfg.sample_rate);
    return (REFERENCE_TIME)ticks;
}

static void mka_wasapi_fill_wave_format(knm_device *dev, WAVEFORMATEX *fmt)
{
    mnk_zero((void *)fmt, (unsigned long)sizeof(*fmt));
    fmt->wFormatTag = WAVE_FORMAT_PCM;
    fmt->nChannels = (WORD)dev->cfg.channels;
    fmt->nSamplesPerSec = (DWORD)dev->cfg.sample_rate;
    fmt->wBitsPerSample = 16;
    fmt->nBlockAlign = (WORD)(fmt->nChannels * 2U);
    fmt->nAvgBytesPerSec = fmt->nSamplesPerSec * (DWORD)fmt->nBlockAlign;
    fmt->cbSize = 0;
}

static int mka_wasapi_create_enumerator(mka_wasapi_state *st)
{
    HRESULT hr;

    hr = CoInitializeEx((LPVOID)0, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE) {
        if (SUCCEEDED(hr)) {
            st->com_initialized = 1;
        }
    } else {
        return KNM_DEVICE_ERROR;
    }

    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator,
                          (IUnknown *)0,
                          CLSCTX_ALL,
                          &IID_IMMDeviceEnumerator,
                          (void **)&st->enumerator);
    if (FAILED(hr)) {
        return KNM_DEVICE_ERROR;
    }
    return KNM_OK;
}

static int mka_wasapi_open_output(knm_device *dev)
{
    mka_wasapi_state *st;
    HRESULT hr;
    DWORD flags;
    REFERENCE_TIME duration;

    st = mka_wasapi(dev);
    duration = mka_wasapi_buffer_time(dev);
    flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK |
            AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM |
            AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY;

    hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(st->enumerator, eRender, eConsole, &st->out_device);
    if (FAILED(hr)) {
        return KNM_DEVICE_ERROR;
    }

    hr = IMMDevice_Activate(st->out_device,
                            &IID_IAudioClient,
                            CLSCTX_ALL,
                            (PROPVARIANT *)0,
                            (void **)&st->out_client);
    if (FAILED(hr)) {
        return KNM_DEVICE_ERROR;
    }

    mka_wasapi_fill_wave_format(dev, &st->fmt);

    hr = IAudioClient_Initialize(st->out_client,
                                 AUDCLNT_SHAREMODE_SHARED,
                                 flags,
                                 duration,
                                 0,
                                 &st->fmt,
                                 (LPCGUID)0);
    if (FAILED(hr)) {
        return KNM_DEVICE_ERROR;
    }

    hr = IAudioClient_GetBufferSize(st->out_client, &st->out_buffer_frames);
    if (FAILED(hr)) {
        return KNM_DEVICE_ERROR;
    }

    st->out_event = CreateEvent((LPSECURITY_ATTRIBUTES)0, FALSE, FALSE, (LPCTSTR)0);
    if (st->out_event == (HANDLE)0) {
        return KNM_DEVICE_ERROR;
    }

    hr = IAudioClient_SetEventHandle(st->out_client, st->out_event);
    if (FAILED(hr)) {
        return KNM_DEVICE_ERROR;
    }

    hr = IAudioClient_GetService(st->out_client,
                                 &IID_IAudioRenderClient,
                                 (void **)&st->render_client);
    if (FAILED(hr)) {
        return KNM_DEVICE_ERROR;
    }

    return KNM_OK;
}

static int mka_wasapi_open_input(knm_device *dev)
{
    mka_wasapi_state *st;
    HRESULT hr;
    DWORD flags;
    REFERENCE_TIME duration;

    st = mka_wasapi(dev);
    duration = mka_wasapi_buffer_time(dev);
    flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK |
            AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM |
            AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY;

    hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(st->enumerator, eCapture, eConsole, &st->in_device);
    if (FAILED(hr)) {
        return KNM_DEVICE_ERROR;
    }

    hr = IMMDevice_Activate(st->in_device,
                            &IID_IAudioClient,
                            CLSCTX_ALL,
                            (PROPVARIANT *)0,
                            (void **)&st->in_client);
    if (FAILED(hr)) {
        return KNM_DEVICE_ERROR;
    }

    mka_wasapi_fill_wave_format(dev, &st->fmt);

    hr = IAudioClient_Initialize(st->in_client,
                                 AUDCLNT_SHAREMODE_SHARED,
                                 flags,
                                 duration,
                                 0,
                                 &st->fmt,
                                 (LPCGUID)0);
    if (FAILED(hr)) {
        return KNM_DEVICE_ERROR;
    }

    hr = IAudioClient_GetBufferSize(st->in_client, &st->in_buffer_frames);
    if (FAILED(hr)) {
        return KNM_DEVICE_ERROR;
    }

    st->in_event = CreateEvent((LPSECURITY_ATTRIBUTES)0, FALSE, FALSE, (LPCTSTR)0);
    if (st->in_event == (HANDLE)0) {
        return KNM_DEVICE_ERROR;
    }

    hr = IAudioClient_SetEventHandle(st->in_client, st->in_event);
    if (FAILED(hr)) {
        return KNM_DEVICE_ERROR;
    }

    hr = IAudioClient_GetService(st->in_client,
                                 &IID_IAudioCaptureClient,
                                 (void **)&st->capture_client);
    if (FAILED(hr)) {
        return KNM_DEVICE_ERROR;
    }

    return KNM_OK;
}

static void mka_wasapi_render_once(knm_device *dev, UINT32 frames)
{
    mka_wasapi_state *st;
    BYTE *data;
    HRESULT hr;

    st = mka_wasapi(dev);
    if (frames == 0U) {
        return;
    }

    hr = IAudioRenderClient_GetBuffer(st->render_client, frames, &data);
    if (FAILED(hr)) {
        return;
    }

    mnk_render_to_s16(dev, (knm_s16 *)data, (unsigned int)frames);

    hr = IAudioRenderClient_ReleaseBuffer(st->render_client, frames, 0U);
    (void)hr;
}

static void mka_wasapi_service_output(knm_device *dev)
{
    mka_wasapi_state *st;
    UINT32 padding;
    UINT32 frames;
    HRESULT hr;

    st = mka_wasapi(dev);
    if (st->out_client == (IAudioClient *)0 || st->render_client == (IAudioRenderClient *)0) {
        return;
    }

    hr = IAudioClient_GetCurrentPadding(st->out_client, &padding);
    if (FAILED(hr)) {
        return;
    }

    if (padding >= st->out_buffer_frames) {
        return;
    }

    frames = st->out_buffer_frames - padding;
    mka_wasapi_render_once(dev, frames);
}

static void mka_wasapi_service_input(knm_device *dev)
{
    mka_wasapi_state *st;
    UINT32 packet_frames;
    BYTE *data;
    DWORD flags;
    HRESULT hr;

    st = mka_wasapi(dev);
    if (st->in_client == (IAudioClient *)0 || st->capture_client == (IAudioCaptureClient *)0) {
        return;
    }

    hr = IAudioCaptureClient_GetNextPacketSize(st->capture_client, &packet_frames);
    while (SUCCEEDED(hr) && packet_frames > 0U) {
        hr = IAudioCaptureClient_GetBuffer(st->capture_client,
                                           &data,
                                           &packet_frames,
                                           &flags,
                                           (UINT64 *)0,
                                           (UINT64 *)0);
        if (FAILED(hr)) {
            return;
        }

        if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
            mnk_zero_s16(dev->s16_input_scratch, (unsigned int)packet_frames, dev->cfg.channels);
            mnk_capture_from_s16(dev, dev->s16_input_scratch, (unsigned int)packet_frames);
        } else {
            mnk_capture_from_s16(dev, (const knm_s16 *)data, (unsigned int)packet_frames);
        }

        hr = IAudioCaptureClient_ReleaseBuffer(st->capture_client, packet_frames);
        if (FAILED(hr)) {
            return;
        }

        hr = IAudioCaptureClient_GetNextPacketSize(st->capture_client, &packet_frames);
    }
}

static DWORD WINAPI mka_wasapi_thread_proc(LPVOID param)
{
    knm_device *dev;
    mka_wasapi_state *st;
    HANDLE waits[3];
    DWORD count;
    DWORD rc;

    dev = (knm_device *)param;
    st = mka_wasapi(dev);

    CoInitializeEx((LPVOID)0, COINIT_MULTITHREADED);

    count = 0UL;
    waits[count++] = st->stop_event;
    if (st->out_event != (HANDLE)0) {
        waits[count++] = st->out_event;
    }
    if (st->in_event != (HANDLE)0) {
        waits[count++] = st->in_event;
    }

    for (;;) {
        rc = WaitForMultipleObjects(count, waits, FALSE, INFINITE);
        if (rc == WAIT_OBJECT_0) {
            break;
        }
        if (st->out_event != (HANDLE)0) {
            mka_wasapi_service_output(dev);
        }
        if (st->in_event != (HANDLE)0) {
            mka_wasapi_service_input(dev);
        }
    }

    CoUninitialize();
    return 0UL;
}

static int mka_wasapi_open(knm_device *dev)
{
    mka_wasapi_state *st;
    int rc;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }
    if ((unsigned long)sizeof(mka_wasapi_state) > (unsigned long)KNM_BACKEND_STATE_BYTES) {
        return KNM_LIMIT_EXCEEDED;
    }

    st = mka_wasapi(dev);
    mnk_zero((void *)st, (unsigned long)sizeof(*st));

    rc = mka_wasapi_create_enumerator(st);
    if (rc != KNM_OK) {
        mka_wasapi_close(dev);
        return rc;
    }

    st->stop_event = CreateEvent((LPSECURITY_ATTRIBUTES)0, TRUE, FALSE, (LPCTSTR)0);
    if (st->stop_event == (HANDLE)0) {
        mka_wasapi_close(dev);
        return KNM_DEVICE_ERROR;
    }

    if (dev->cfg.enable_output) {
        rc = mka_wasapi_open_output(dev);
        if (rc != KNM_OK) {
            mka_wasapi_close(dev);
            return rc;
        }
    }
    if (dev->cfg.enable_input) {
        rc = mka_wasapi_open_input(dev);
        if (rc != KNM_OK) {
            mka_wasapi_close(dev);
            return rc;
        }
    }

    return KNM_OK;
}

static int mka_wasapi_start(knm_device *dev)
{
    mka_wasapi_state *st;
    HRESULT hr;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }

    st = mka_wasapi(dev);
    ResetEvent(st->stop_event);
    dev->running = 1;

    if (st->out_client != (IAudioClient *)0) {
        mka_wasapi_render_once(dev, st->out_buffer_frames);
    }

    st->thread = CreateThread((LPSECURITY_ATTRIBUTES)0,
                              0UL,
                              mka_wasapi_thread_proc,
                              (LPVOID)dev,
                              0UL,
                              &st->thread_id);
    if (st->thread == (HANDLE)0) {
        dev->running = 0;
        return KNM_DEVICE_ERROR;
    }

    if (st->in_client != (IAudioClient *)0) {
        hr = IAudioClient_Start(st->in_client);
        if (FAILED(hr)) {
            (void)mka_wasapi_stop(dev);
            return KNM_DEVICE_ERROR;
        }
    }
    if (st->out_client != (IAudioClient *)0) {
        hr = IAudioClient_Start(st->out_client);
        if (FAILED(hr)) {
            (void)mka_wasapi_stop(dev);
            return KNM_DEVICE_ERROR;
        }
    }

    return KNM_OK;
}

static int mka_wasapi_stop(knm_device *dev)
{
    mka_wasapi_state *st;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }

    st = mka_wasapi(dev);
    dev->running = 0;

    if (st->in_client != (IAudioClient *)0) {
        IAudioClient_Stop(st->in_client);
    }
    if (st->out_client != (IAudioClient *)0) {
        IAudioClient_Stop(st->out_client);
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

static void mka_wasapi_close(knm_device *dev)
{
    mka_wasapi_state *st;

    if (dev == (knm_device *)0) {
        return;
    }

    st = mka_wasapi(dev);

    if (st->thread != (HANDLE)0) {
        SetEvent(st->stop_event);
        WaitForSingleObject(st->thread, INFINITE);
        CloseHandle(st->thread);
        st->thread = (HANDLE)0;
    }

    if (st->render_client != (IAudioRenderClient *)0) {
        IAudioRenderClient_Release(st->render_client);
        st->render_client = (IAudioRenderClient *)0;
    }
    if (st->capture_client != (IAudioCaptureClient *)0) {
        IAudioCaptureClient_Release(st->capture_client);
        st->capture_client = (IAudioCaptureClient *)0;
    }
    if (st->out_client != (IAudioClient *)0) {
        IAudioClient_Stop(st->out_client);
        IAudioClient_Release(st->out_client);
        st->out_client = (IAudioClient *)0;
    }
    if (st->in_client != (IAudioClient *)0) {
        IAudioClient_Stop(st->in_client);
        IAudioClient_Release(st->in_client);
        st->in_client = (IAudioClient *)0;
    }
    if (st->out_device != (IMMDevice *)0) {
        IMMDevice_Release(st->out_device);
        st->out_device = (IMMDevice *)0;
    }
    if (st->in_device != (IMMDevice *)0) {
        IMMDevice_Release(st->in_device);
        st->in_device = (IMMDevice *)0;
    }
    if (st->enumerator != (IMMDeviceEnumerator *)0) {
        IMMDeviceEnumerator_Release(st->enumerator);
        st->enumerator = (IMMDeviceEnumerator *)0;
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

    if (st->com_initialized) {
        CoUninitialize();
        st->com_initialized = 0;
    }
}

const knm_backend_vtbl knm_backend_wasapi_vtbl = {
    KNM_BACKEND_WASAPI,
    "wasapi",
    1,
    1,
    1,
    0,
    mka_wasapi_probe,
    (unsigned int (*)(void))0,
    (int (*)(unsigned int, knm_device_info *))0,
    mka_wasapi_open,
    mka_wasapi_start,
    mka_wasapi_stop,
    mka_wasapi_close
};

#else

static int mka_wasapi_probe(void)
{
    return 0;
}

static int mka_wasapi_open(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static int mka_wasapi_start(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static int mka_wasapi_stop(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static void mka_wasapi_close(knm_device *dev)
{
    (void)dev;
}

const knm_backend_vtbl knm_backend_wasapi_vtbl = {
    KNM_BACKEND_WASAPI,
    "wasapi",
    1,
    1,
    1,
    0,
    mka_wasapi_probe,
    (unsigned int (*)(void))0,
    (int (*)(unsigned int, knm_device_info *))0,
    mka_wasapi_open,
    mka_wasapi_start,
    mka_wasapi_stop,
    mka_wasapi_close
};

#endif
