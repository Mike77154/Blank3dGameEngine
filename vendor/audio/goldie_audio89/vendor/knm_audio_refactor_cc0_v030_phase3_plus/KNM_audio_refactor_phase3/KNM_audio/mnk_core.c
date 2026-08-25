#include "mnk_core_internal.h"

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sched.h>
#endif

#define MNK_BACKEND_COUNT ((unsigned int)(sizeof(g_knm_backends) / sizeof(g_knm_backends[0])))

static knm_device g_knm_devices[KNM_MAX_DEVICES];

static const knm_backend_vtbl *g_knm_backends[] = {
    &knm_backend_null_vtbl,
#if KNM_AUDIO_ENABLE_WINMM
    &knm_backend_winmm_vtbl,
#endif
#if KNM_AUDIO_ENABLE_WASAPI
    &knm_backend_wasapi_vtbl,
#endif
#if KNM_AUDIO_ENABLE_ALSA
    &knm_backend_alsa_vtbl,
#endif
#if KNM_AUDIO_ENABLE_PULSEAUDIO
    &knm_backend_pulseaudio_vtbl,
#endif
#if KNM_AUDIO_ENABLE_PIPEWIRE
    &knm_backend_pipewire_vtbl,
#endif
#if KNM_AUDIO_ENABLE_JACK
    &knm_backend_jack_vtbl,
#endif
#if KNM_AUDIO_ENABLE_COREAUDIO
    &knm_backend_coreaudio_vtbl,
#endif
#if KNM_AUDIO_ENABLE_AAUDIO
    &knm_backend_aaudio_vtbl,
#endif
#if KNM_AUDIO_ENABLE_OPENSL_ES
    &knm_backend_opensl_es_vtbl,
#endif
#if KNM_AUDIO_ENABLE_SNDIO
    &knm_backend_sndio_vtbl,
#endif
#if KNM_AUDIO_ENABLE_WEBAUDIO
    &knm_backend_webaudio_vtbl,
#endif
#if KNM_AUDIO_ENABLE_HAIKU
    &knm_backend_haiku_vtbl
#endif
};

void mnk_copy_text(char *dst, unsigned int dst_capacity, const char *src)
{
    unsigned int i;

    if (dst == (char *)0 || dst_capacity == 0U) {
        return;
    }
    if (src == (const char *)0) {
        dst[0] = '\0';
        return;
    }

    for (i = 0U; i + 1U < dst_capacity && src[i] != '\0'; ++i) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

static int mnk_text_is_empty(const char *text)
{
    return (text == (const char *)0 || text[0] == '\0') ? 1 : 0;
}

static int mnk_text_equal(const char *a, const char *b)
{
    unsigned int i;

    if (a == b) {
        return 1;
    }
    if (a == (const char *)0 || b == (const char *)0) {
        return 0;
    }

    i = 0U;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return 0;
        }
        ++i;
    }
    return (a[i] == b[i]) ? 1 : 0;
}

static void mnk_assign_opened_device_info(knm_device *dev);

static void mnk_fill_default_device_info(const knm_backend_vtbl *vtbl,
                                         unsigned int index,
                                         knm_device_info *out_info)
{
    mnk_zero((void *)out_info, (unsigned long)sizeof(*out_info));
    out_info->backend = vtbl->backend_id;
    out_info->device_index = index;
    out_info->is_default = (index == 0U) ? 1 : 0;
    out_info->supports_input = vtbl->supports_input;
    out_info->supports_output = vtbl->supports_output;
    out_info->supports_duplex = vtbl->supports_duplex;
    out_info->max_input_channels = vtbl->supports_input ? KNM_MAX_CHANNELS : 0U;
    out_info->max_output_channels = vtbl->supports_output ? KNM_MAX_CHANNELS : 0U;
    out_info->default_sample_rate = 48000UL;
    mnk_copy_text(out_info->device_id, (unsigned int)sizeof(out_info->device_id), "default");
    mnk_copy_text(out_info->name, (unsigned int)sizeof(out_info->name), vtbl->name);
}

static void mnk_normalize_negotiated_config(knm_device *dev)
{
    unsigned int min_ring;

    if (dev == (knm_device *)0) {
        return;
    }

    if (dev->cfg.sample_rate == 0UL) {
        dev->cfg.sample_rate = dev->requested_cfg.sample_rate;
    }
    if (dev->cfg.channels == 0U) {
        dev->cfg.channels = dev->requested_cfg.channels;
    }
    if (dev->cfg.frames_per_buffer == 0U) {
        dev->cfg.frames_per_buffer = dev->requested_cfg.frames_per_buffer;
    }
    if (dev->cfg.buffer_count == 0U) {
        dev->cfg.buffer_count = dev->requested_cfg.buffer_count;
    }
    dev->cfg.enable_input = dev->requested_cfg.enable_input;
    dev->cfg.enable_output = dev->requested_cfg.enable_output;
    dev->cfg.backend = dev->backend_id;
    dev->cfg.prefer_low_latency = dev->requested_cfg.prefer_low_latency;

    if (dev->cfg.enable_input) {
        if (dev->cfg.capture_ring_frames == 0U) {
            dev->cfg.capture_ring_frames = dev->requested_cfg.capture_ring_frames;
        }
        min_ring = dev->cfg.frames_per_buffer * dev->cfg.buffer_count;
        if (dev->cfg.capture_ring_frames < min_ring) {
            dev->cfg.capture_ring_frames = min_ring;
        }
        if (dev->cfg.capture_ring_frames > KNM_MAX_CAPTURE_RING_FRAMES) {
            dev->cfg.capture_ring_frames = KNM_MAX_CAPTURE_RING_FRAMES;
        }
    } else {
        dev->cfg.capture_ring_frames = 0U;
    }

    dev->output_latency_frames = (unsigned long)dev->cfg.frames_per_buffer * (unsigned long)dev->cfg.buffer_count;
    dev->input_latency_frames = dev->cfg.enable_input ? dev->output_latency_frames : 0UL;
}

void mnk_zero(void *ptr, unsigned long size_bytes)
{
    memset(ptr, 0, (size_t)size_bytes);
}

void mnk_set_last_error(knm_device *dev, int rc)
{
    if (dev != (knm_device *)0) {
        dev->last_error = rc;
    }
}

void mnk_set_status_flag(knm_device *dev, unsigned long flag)
{
    if (dev != (knm_device *)0) {
        dev->status_flags |= flag;
    }
}

void mnk_clear_status_flags(knm_device *dev)
{
    if (dev != (knm_device *)0) {
        dev->status_flags = 0UL;
    }
}

void mnk_clear_status_flags_mask(knm_device *dev, unsigned long flags)
{
    if (dev == (knm_device *)0) {
        return;
    }
    if (flags == 0UL) {
        dev->status_flags = 0UL;
        return;
    }
    dev->status_flags &= ~flags;
}

void mnk_note_input_overflow(knm_device *dev, unsigned long count)
{
    if (dev == (knm_device *)0) {
        return;
    }
    dev->stats.input_overflows += count;
    mnk_set_status_flag(dev, (unsigned long)KNM_STATUS_INPUT_OVERFLOW);
    mnk_set_last_error(dev, KNM_BUFFER_OVERFLOW);
}

void mnk_note_backend_xrun(knm_device *dev)
{
    if (dev == (knm_device *)0) {
        return;
    }
    dev->stats.backend_xruns += 1UL;
    mnk_set_status_flag(dev, (unsigned long)KNM_STATUS_BACKEND_XRUN);
    mnk_set_last_error(dev, KNM_DEVICE_ERROR);
}

static void mnk_ring_pause(unsigned int spins)
{
#if defined(_WIN32) || defined(_WIN64)
    if ((spins & 31U) == 31U) {
        Sleep(0U);
    }
#else
    if ((spins & 31U) == 31U) {
        (void)sched_yield();
    }
#endif
}

static void mnk_ring_lock(knm_ring *ring)
{
    unsigned int spins;

    if (ring == (knm_ring *)0) {
        return;
    }

    spins = 0U;
#if defined(_WIN32) || defined(_WIN64)
    while (InterlockedExchange((volatile LONG *)&ring->lock_state, 1L) != 0L) {
        ++spins;
        mnk_ring_pause(spins);
    }
#elif defined(__GNUC__) || defined(__clang__)
    while (__sync_lock_test_and_set(&ring->lock_state, 1U) != 0U) {
        ++spins;
        mnk_ring_pause(spins);
    }
#else
#error "KNM_audio needs atomic lock primitives for ring synchronization on this compiler."
#endif
}

static void mnk_ring_unlock(knm_ring *ring)
{
    if (ring == (knm_ring *)0) {
        return;
    }

#if defined(_WIN32) || defined(_WIN64)
    InterlockedExchange((volatile LONG *)&ring->lock_state, 0L);
#elif defined(__GNUC__) || defined(__clang__)
    __sync_synchronize();
    __sync_lock_release(&ring->lock_state);
#endif
}

const knm_backend_vtbl *mnk_backend_by_id(int backend_id)
{
    unsigned int i;
    for (i = 0U; i < MNK_BACKEND_COUNT; ++i) {
        if (g_knm_backends[i]->backend_id == backend_id) {
            return g_knm_backends[i];
        }
    }
    return (const knm_backend_vtbl *)0;
}

int mnk_backend_compiled(int backend_id)
{
    return mnk_backend_by_id(backend_id) != (const knm_backend_vtbl *)0 ? 1 : 0;
}

int mnk_backend_available(int backend_id)
{
    const knm_backend_vtbl *vtbl;

    vtbl = mnk_backend_by_id(backend_id);
    if (vtbl == (const knm_backend_vtbl *)0) {
        return 0;
    }
    if (vtbl->probe == (int (*)(void))0) {
        return 0;
    }
    return vtbl->probe() ? 1 : 0;
}

unsigned int mnk_backend_count(void)
{
    return MNK_BACKEND_COUNT;
}

int mnk_backend_info_at(unsigned int index, knm_backend_info *out_info)
{
    const knm_backend_vtbl *vtbl;

    if (out_info == (knm_backend_info *)0) {
        return KNM_BAD_ARGUMENT;
    }
    if (index >= MNK_BACKEND_COUNT) {
        return KNM_LIMIT_EXCEEDED;
    }

    vtbl = g_knm_backends[index];
    mnk_zero((void *)out_info, (unsigned long)sizeof(*out_info));
    out_info->backend = vtbl->backend_id;
    mnk_copy_text(out_info->name, (unsigned int)sizeof(out_info->name), vtbl->name);
    out_info->compiled = 1;
    out_info->available = (vtbl->probe != (int (*)(void))0 && vtbl->probe()) ? 1 : 0;
    out_info->supports_input = vtbl->supports_input;
    out_info->supports_output = vtbl->supports_output;
    out_info->supports_duplex = vtbl->supports_duplex;
    out_info->supports_enumeration = vtbl->supports_enumeration;
    out_info->device_count = mnk_device_count_for_backend(vtbl->backend_id);
    return KNM_OK;
}

unsigned int mnk_device_count_for_backend(int backend_id)
{
    const knm_backend_vtbl *vtbl;

    vtbl = mnk_backend_by_id(backend_id);
    if (vtbl == (const knm_backend_vtbl *)0) {
        return 0U;
    }
    if (vtbl->probe == (int (*)(void))0 || !vtbl->probe()) {
        return 0U;
    }
    if (vtbl->device_count != (unsigned int (*)(void))0) {
        return vtbl->device_count();
    }
    return 1U;
}

int mnk_device_info_for_backend(int backend_id, unsigned int index, knm_device_info *out_info)
{
    const knm_backend_vtbl *vtbl;

    if (out_info == (knm_device_info *)0) {
        return KNM_BAD_ARGUMENT;
    }
    vtbl = mnk_backend_by_id(backend_id);
    if (vtbl == (const knm_backend_vtbl *)0) {
        return KNM_BACKEND_NOT_COMPILED;
    }
    if (vtbl->probe == (int (*)(void))0 || !vtbl->probe()) {
        return KNM_BACKEND_UNAVAILABLE;
    }
    if (index >= mnk_device_count_for_backend(backend_id)) {
        return KNM_LIMIT_EXCEEDED;
    }
    if (vtbl->device_info != (int (*)(unsigned int, knm_device_info *))0) {
        return vtbl->device_info(index, out_info);
    }
    mnk_fill_default_device_info(vtbl, index, out_info);
    return KNM_OK;
}

int mnk_find_device(int backend_id, const char *device_id, knm_device_info *out_info)
{
    unsigned int count;
    unsigned int i;
    knm_device_info info;
    const knm_backend_vtbl *vtbl;
    int rc;

    if (device_id == (const char *)0 || device_id[0] == '\0') {
        return KNM_BAD_ARGUMENT;
    }

    vtbl = mnk_backend_by_id(backend_id);
    if (vtbl == (const knm_backend_vtbl *)0) {
        return KNM_BACKEND_NOT_COMPILED;
    }
    if (vtbl->probe == (int (*)(void))0 || !vtbl->probe()) {
        return KNM_BACKEND_UNAVAILABLE;
    }

    count = mnk_device_count_for_backend(backend_id);
    for (i = 0U; i < count; ++i) {
        rc = mnk_device_info_for_backend(backend_id, i, &info);
        if (rc == KNM_OK && mnk_text_equal(info.device_id, device_id)) {
            if (out_info != (knm_device_info *)0) {
                *out_info = info;
            }
            return KNM_OK;
        }
    }
    return KNM_DEVICE_NOT_FOUND;
}

const char *mnk_backend_name_from_id(int backend_id)
{
    const knm_backend_vtbl *vtbl;

    if (backend_id == KNM_BACKEND_DEFAULT) {
        return "default";
    }

    vtbl = mnk_backend_by_id(backend_id);
    if (vtbl == (const knm_backend_vtbl *)0) {
        return "unknown";
    }
    return vtbl->name;
}

int mnk_validate_config(const knm_audio_config *cfg)
{
    if (cfg == (const knm_audio_config *)0) {
        return KNM_BAD_ARGUMENT;
    }
    if (!cfg->enable_input && !cfg->enable_output) {
        return KNM_BAD_ARGUMENT;
    }
    if (cfg->sample_rate == 0UL) {
        return KNM_BAD_ARGUMENT;
    }
    if (cfg->channels == 0U || cfg->channels > KNM_MAX_CHANNELS) {
        return KNM_LIMIT_EXCEEDED;
    }
    if (cfg->frames_per_buffer == 0U || cfg->frames_per_buffer > KNM_MAX_FRAMES_PER_BUFFER) {
        return KNM_LIMIT_EXCEEDED;
    }
    if (cfg->buffer_count < 2U || cfg->buffer_count > KNM_MAX_STREAM_BUFFERS) {
        return KNM_LIMIT_EXCEEDED;
    }
    if (cfg->enable_input) {
        if (cfg->capture_ring_frames == 0U ||
            cfg->capture_ring_frames > KNM_MAX_CAPTURE_RING_FRAMES) {
            return KNM_LIMIT_EXCEEDED;
        }
    }
    return KNM_OK;
}

void mnk_ring_init(knm_ring *ring, unsigned int capacity_frames)
{
    if (ring == (knm_ring *)0) {
        return;
    }
    ring->lock_state = 0U;
    ring->read_pos = 0UL;
    ring->write_pos = 0UL;
    ring->used_frames = 0UL;
    ring->overruns = 0UL;
    ring->capacity_frames = capacity_frames;
}

unsigned int mnk_ring_available(const knm_device *dev)
{
    unsigned int available;
    knm_ring *ring;

    if (dev == (const knm_device *)0) {
        return 0U;
    }

    ring = (knm_ring *)(void *)&dev->capture_ring;
    mnk_ring_lock(ring);
    available = (unsigned int)ring->used_frames;
    mnk_ring_unlock(ring);
    return available;
}

static void mnk_copy_fix(knm_fix16 *dst, const knm_fix16 *src, unsigned int samples)
{
    unsigned int i;
    for (i = 0U; i < samples; ++i) {
        dst[i] = src[i];
    }
}

unsigned int mnk_ring_read(knm_device *dev, knm_fix16 *dst, unsigned int frames)
{
    unsigned int channels;
    unsigned int capacity;
    unsigned int to_read;
    unsigned int i;
    unsigned long frame_index;
    unsigned int sample_base;
    knm_ring *ring;

    if (dev == (knm_device *)0 || dst == (knm_fix16 *)0) {
        return 0U;
    }

    ring = &dev->capture_ring;
    channels = dev->cfg.channels;
    capacity = ring->capacity_frames;
    if (capacity == 0U) {
        return 0U;
    }

    mnk_ring_lock(ring);
    to_read = frames;
    if ((unsigned long)to_read > ring->used_frames) {
        to_read = (unsigned int)ring->used_frames;
    }

    for (i = 0U; i < to_read; ++i) {
        frame_index = ring->read_pos;
        sample_base = (unsigned int)frame_index * channels;
        mnk_copy_fix(dst + (i * channels), ring->data + sample_base, channels);
        frame_index += 1UL;
        if (frame_index >= (unsigned long)capacity) {
            frame_index = 0UL;
        }
        ring->read_pos = frame_index;
    }

    ring->used_frames -= (unsigned long)to_read;
    mnk_ring_unlock(ring);
    return to_read;
}

void mnk_ring_write(knm_device *dev, const knm_fix16 *src, unsigned int frames)
{
    unsigned int channels;
    unsigned int capacity;
    unsigned int i;
    unsigned long frame_index;
    unsigned int sample_base;
    knm_ring *ring;

    if (dev == (knm_device *)0 || src == (const knm_fix16 *)0) {
        return;
    }

    ring = &dev->capture_ring;
    channels = dev->cfg.channels;
    capacity = ring->capacity_frames;
    if (capacity == 0U) {
        return;
    }

    mnk_ring_lock(ring);
    for (i = 0U; i < frames; ++i) {
        if (ring->used_frames >= (unsigned long)capacity) {
            ring->read_pos += 1UL;
            if (ring->read_pos >= (unsigned long)capacity) {
                ring->read_pos = 0UL;
            }
            ring->overruns += 1UL;
            ring->used_frames = (unsigned long)capacity;
            mnk_note_input_overflow(dev, 1UL);
        } else {
            ring->used_frames += 1UL;
        }

        frame_index = ring->write_pos;
        sample_base = (unsigned int)frame_index * channels;
        mnk_copy_fix(ring->data + sample_base, src + (i * channels), channels);

        frame_index += 1UL;
        if (frame_index >= (unsigned long)capacity) {
            frame_index = 0UL;
        }
        ring->write_pos = frame_index;
    }
    mnk_ring_unlock(ring);
}

void mnk_zero_frames(knm_fix16 *dst, unsigned int frames, unsigned int channels)
{
    unsigned int i;
    unsigned int samples;

    samples = frames * channels;
    for (i = 0U; i < samples; ++i) {
        dst[i] = (knm_fix16)0L;
    }
}

void mnk_zero_s16(knm_s16 *dst, unsigned int frames, unsigned int channels)
{
    unsigned int i;
    unsigned int samples;

    samples = frames * channels;
    for (i = 0U; i < samples; ++i) {
        dst[i] = (knm_s16)0;
    }
}

void mnk_s16_to_fix(knm_fix16 *dst, const knm_s16 *src, unsigned int samples)
{
    unsigned int i;
    for (i = 0U; i < samples; ++i) {
        dst[i] = (knm_fix16)(((int)src[i]) << 1);
    }
}

void mnk_fix_to_s16(knm_s16 *dst, const knm_fix16 *src, unsigned int samples)
{
    unsigned int i;
    knm_fix16 v;

    for (i = 0U; i < samples; ++i) {
        v = src[i];
        if (v > (knm_fix16)65534L) {
            v = (knm_fix16)65534L;
        }
        if (v < (knm_fix16)-65536L) {
            v = (knm_fix16)-65536L;
        }
        dst[i] = (knm_s16)(v >> 1);
    }
}

void mnk_run_input_callback(knm_device *dev, const knm_fix16 *input, unsigned int frames)
{
    if (dev == (knm_device *)0) {
        return;
    }
    if (dev->callback != (knm_audio_callback)0) {
        dev->stats.callback_calls += 1UL;
        dev->stats.input_callbacks += 1UL;
        dev->callback(dev->user_data, input, (knm_fix16 *)0, frames, dev->cfg.channels);
    }
}

void mnk_run_output_callback(knm_device *dev, knm_fix16 *output, unsigned int frames)
{
    const knm_fix16 *input_ptr;
    unsigned int from_ring;
    unsigned int channels;

    if (dev == (knm_device *)0 || output == (knm_fix16 *)0) {
        return;
    }

    channels = dev->cfg.channels;
    input_ptr = (const knm_fix16 *)0;
    mnk_zero_frames(output, frames, channels);

    if (dev->cfg.enable_input) {
        from_ring = mnk_ring_read(dev, dev->input_scratch, frames);
        if (from_ring < frames) {
            mnk_zero_frames(dev->input_scratch + (from_ring * channels), frames - from_ring, channels);
        }
        input_ptr = dev->input_scratch;
    }

    if (dev->callback != (knm_audio_callback)0) {
        dev->stats.callback_calls += 1UL;
        dev->stats.output_callbacks += 1UL;
        dev->callback(dev->user_data, input_ptr, output, frames, channels);
    }
}

void mnk_submit_capture_fix(knm_device *dev, const knm_fix16 *src, unsigned int frames)
{
    if (dev == (knm_device *)0 || src == (const knm_fix16 *)0 || frames == 0U) {
        return;
    }

    dev->stats.captured_frames += (unsigned long)frames;
    mnk_ring_write(dev, src, frames);
    if (!dev->cfg.enable_output) {
        mnk_run_input_callback(dev, src, frames);
    }
}

void mnk_capture_from_s16(knm_device *dev, const knm_s16 *src, unsigned int frames)
{
    unsigned int remaining;
    unsigned int offset_frames;
    unsigned int chunk_frames;
    unsigned int chunk_samples;

    if (dev == (knm_device *)0 || src == (const knm_s16 *)0) {
        return;
    }

    remaining = frames;
    offset_frames = 0U;
    while (remaining > 0U) {
        chunk_frames = remaining;
        if (chunk_frames > KNM_MAX_FRAMES_PER_BUFFER) {
            chunk_frames = KNM_MAX_FRAMES_PER_BUFFER;
        }
        chunk_samples = chunk_frames * dev->cfg.channels;
        mnk_s16_to_fix(dev->input_scratch, src + (offset_frames * dev->cfg.channels), chunk_samples);
        mnk_submit_capture_fix(dev, dev->input_scratch, chunk_frames);
        remaining -= chunk_frames;
        offset_frames += chunk_frames;
    }
}

void mnk_render_to_s16(knm_device *dev, knm_s16 *dst, unsigned int frames)
{
    unsigned int remaining;
    unsigned int offset_frames;
    unsigned int chunk_frames;
    unsigned int chunk_samples;

    if (dev == (knm_device *)0 || dst == (knm_s16 *)0) {
        return;
    }

    remaining = frames;
    offset_frames = 0U;
    while (remaining > 0U) {
        chunk_frames = remaining;
        if (chunk_frames > KNM_MAX_FRAMES_PER_BUFFER) {
            chunk_frames = KNM_MAX_FRAMES_PER_BUFFER;
        }
        chunk_samples = chunk_frames * dev->cfg.channels;
        mnk_run_output_callback(dev, dev->output_scratch, chunk_frames);
        dev->stats.rendered_frames += (unsigned long)chunk_frames;
        mnk_fix_to_s16(dst + (offset_frames * dev->cfg.channels), dev->output_scratch, chunk_samples);
        remaining -= chunk_frames;
        offset_frames += chunk_frames;
    }
}

static const int *mnk_default_backend_order(void)
{
#if defined(_WIN32) || defined(_WIN64)
    static const int order_win[] = {
        KNM_BACKEND_WASAPI,
        KNM_BACKEND_WINMM,
        KNM_BACKEND_NULL,
        0
    };
    return order_win;
#elif defined(__ANDROID__)
    static const int order_android[] = {
        KNM_BACKEND_AAUDIO,
        KNM_BACKEND_OPENSL_ES,
        KNM_BACKEND_NULL,
        0
    };
    return order_android;
#elif defined(__APPLE__)
    static const int order_apple[] = {
        KNM_BACKEND_COREAUDIO,
        KNM_BACKEND_NULL,
        0
    };
    return order_apple;
#elif defined(__EMSCRIPTEN__)
    static const int order_web[] = {
        KNM_BACKEND_WEBAUDIO,
        KNM_BACKEND_NULL,
        0
    };
    return order_web;
#elif defined(__HAIKU__)
    static const int order_haiku[] = {
        KNM_BACKEND_HAIKU,
        KNM_BACKEND_NULL,
        0
    };
    return order_haiku;
#elif defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__) || defined(__DragonFly__)
    static const int order_bsd[] = {
        KNM_BACKEND_SNDIO,
        KNM_BACKEND_JACK,
        KNM_BACKEND_NULL,
        0
    };
    return order_bsd;
#else
    static const int order_linux[] = {
        KNM_BACKEND_PIPEWIRE,
        KNM_BACKEND_PULSEAUDIO,
        KNM_BACKEND_ALSA,
        KNM_BACKEND_JACK,
        KNM_BACKEND_NULL,
        0
    };
    return order_linux;
#endif
}

unsigned int mnk_default_backend_order_copy(int *out_backends, unsigned int capacity)
{
    const int *order;
    unsigned int count;

    order = mnk_default_backend_order();
    count = 0U;
    while (order[count] != 0) {
        if (out_backends != (int *)0 && count < capacity) {
            out_backends[count] = order[count];
        }
        ++count;
    }
    return count;
}

static void mnk_assign_opened_device_info(knm_device *dev)
{
    knm_device_info info;
    int rc;

    if (dev == (knm_device *)0 || dev->backend == (const knm_backend_vtbl *)0) {
        return;
    }

    dev->opened_device_valid = 0;
    mnk_zero((void *)&info, (unsigned long)sizeof(info));

    if (!mnk_text_is_empty(dev->requested_cfg.device_id)) {
        rc = mnk_find_device(dev->backend_id, dev->requested_cfg.device_id, &info);
        if (rc == KNM_OK) {
            dev->opened_device = info;
            dev->opened_device_valid = 1;
            return;
        }
    }

    rc = mnk_device_info_for_backend(dev->backend_id, 0U, &info);
    if (rc == KNM_OK) {
        dev->opened_device = info;
        dev->opened_device_valid = 1;
        if (!mnk_text_is_empty(dev->requested_cfg.device_id) &&
            !mnk_text_equal(dev->requested_cfg.device_id, info.device_id)) {
            mnk_copy_text(dev->opened_device.device_id,
                          (unsigned int)sizeof(dev->opened_device.device_id),
                          dev->requested_cfg.device_id);
            dev->opened_device.is_default = 0;
        }
        return;
    }

    mnk_fill_default_device_info(dev->backend, 0U, &dev->opened_device);
    if (!mnk_text_is_empty(dev->requested_cfg.device_id)) {
        mnk_copy_text(dev->opened_device.device_id,
                      (unsigned int)sizeof(dev->opened_device.device_id),
                      dev->requested_cfg.device_id);
        dev->opened_device.is_default = 0;
    }
    dev->opened_device.backend = dev->backend_id;
    dev->opened_device.default_sample_rate = dev->cfg.sample_rate;
    dev->opened_device_valid = 1;
}

static int mnk_try_backend_open(knm_device *dev, int backend_id)
{
    const knm_backend_vtbl *vtbl;
    int rc;

    vtbl = mnk_backend_by_id(backend_id);
    if (vtbl == (const knm_backend_vtbl *)0) {
        return KNM_BACKEND_NOT_COMPILED;
    }
    if (vtbl->probe == (int (*)(void))0 || !vtbl->probe()) {
        return KNM_BACKEND_UNAVAILABLE;
    }

    dev->backend = vtbl;
    dev->backend_id = backend_id;
    mnk_zero((void *)dev->backend_state, (unsigned long)sizeof(dev->backend_state));

    rc = vtbl->open(dev);
    if (rc != KNM_OK) {
        if (vtbl->close != (void (*)(knm_device *))0) {
            vtbl->close(dev);
        }
        dev->backend = (const knm_backend_vtbl *)0;
        dev->backend_id = KNM_BACKEND_NULL;
        mnk_zero((void *)dev->backend_state, (unsigned long)sizeof(dev->backend_state));
        return rc;
    }

    mnk_normalize_negotiated_config(dev);
    mnk_ring_init(&dev->capture_ring, dev->cfg.capture_ring_frames);
    mnk_assign_opened_device_info(dev);
    return KNM_OK;
}

int mnk_open_device(knm_device **out_device,
                    const knm_audio_config *cfg,
                    knm_audio_callback callback,
                    void *user_data)
{
    knm_audio_config tmp;
    unsigned int i;
    knm_device *slot;
    int rc;
    const int *order;
    unsigned int min_ring;

    if (out_device == (knm_device **)0 || cfg == (const knm_audio_config *)0) {
        return KNM_BAD_ARGUMENT;
    }

    *out_device = (knm_device *)0;
    tmp = *cfg;

    if (tmp.enable_input) {
        if (tmp.capture_ring_frames == 0U) {
            tmp.capture_ring_frames = KNM_DEFAULT_CAPTURE_RING_FRAMES;
        }
        min_ring = tmp.frames_per_buffer * tmp.buffer_count;
        if (tmp.capture_ring_frames < min_ring) {
            tmp.capture_ring_frames = min_ring;
        }
    } else {
        tmp.capture_ring_frames = 0U;
    }

    rc = mnk_validate_config(&tmp);
    if (rc != KNM_OK) {
        return rc;
    }

    slot = (knm_device *)0;
    for (i = 0U; i < KNM_MAX_DEVICES; ++i) {
        if (!g_knm_devices[i].in_use) {
            slot = &g_knm_devices[i];
            break;
        }
    }
    if (slot == (knm_device *)0) {
        return KNM_NO_SLOT;
    }

    mnk_zero((void *)slot, (unsigned long)sizeof(*slot));
    slot->in_use = 1;
    slot->requested_cfg = tmp;
    slot->cfg = tmp;
    slot->callback = callback;
    slot->user_data = user_data;
    slot->backend_id = KNM_BACKEND_NULL;
    slot->last_error = KNM_OK;
    slot->opened_device_valid = 0;
    slot->output_latency_frames = tmp.frames_per_buffer * tmp.buffer_count;
    slot->input_latency_frames = tmp.enable_input ? tmp.frames_per_buffer * tmp.buffer_count : 0UL;
    mnk_ring_init(&slot->capture_ring, tmp.capture_ring_frames);

    if (tmp.backend == KNM_BACKEND_DEFAULT) {
        int first_error;

        first_error = KNM_BACKEND_UNAVAILABLE;
        order = mnk_default_backend_order();
        while (*order != 0) {
            rc = mnk_try_backend_open(slot, *order);
            if (rc == KNM_OK) {
                slot->opened = 1;
                *out_device = slot;
                return KNM_OK;
            }
            if (first_error == KNM_BACKEND_UNAVAILABLE && rc != KNM_BACKEND_UNAVAILABLE) {
                first_error = rc;
            }
            mnk_set_last_error(slot, rc);
            ++order;
        }

        mnk_zero((void *)slot, (unsigned long)sizeof(*slot));
        return first_error;
    }

    rc = mnk_try_backend_open(slot, tmp.backend);
    if (rc != KNM_OK) {
        mnk_set_last_error(slot, rc);
        mnk_zero((void *)slot, (unsigned long)sizeof(*slot));
        return rc;
    }

    slot->opened = 1;
    *out_device = slot;
    return KNM_OK;
}

int mnk_start_device(knm_device *dev)
{
    int rc;

    if (dev == (knm_device *)0 || !dev->opened || dev->backend == (const knm_backend_vtbl *)0) {
        return KNM_BAD_ARGUMENT;
    }
    if (dev->running) {
        return KNM_OK;
    }

    rc = dev->backend->start(dev);
    if (rc == KNM_OK) {
        dev->stats.starts += 1UL;
    }
    mnk_set_last_error(dev, rc);
    return rc;
}

int mnk_stop_device(knm_device *dev)
{
    int rc;

    if (dev == (knm_device *)0 || !dev->opened || dev->backend == (const knm_backend_vtbl *)0) {
        return KNM_BAD_ARGUMENT;
    }
    if (!dev->running) {
        mnk_set_last_error(dev, KNM_NOT_RUNNING);
        return KNM_NOT_RUNNING;
    }

    rc = dev->backend->stop(dev);
    if (rc == KNM_OK) {
        dev->stats.stops += 1UL;
    }
    mnk_set_last_error(dev, rc);
    return rc;
}

void mnk_close_device(knm_device *dev)
{
    if (dev == (knm_device *)0 || !dev->in_use) {
        return;
    }

    if (dev->running && dev->backend != (const knm_backend_vtbl *)0) {
        (void)dev->backend->stop(dev);
    }
    if (dev->backend != (const knm_backend_vtbl *)0) {
        dev->backend->close(dev);
    }

    mnk_zero((void *)dev, (unsigned long)sizeof(*dev));
}

int mnk_is_running_device(const knm_device *dev)
{
    if (dev == (const knm_device *)0) {
        return 0;
    }
    return dev->running;
}

int mnk_service_device(knm_device *dev, unsigned int frames)
{
    unsigned int remaining;
    unsigned int chunk_frames;
    unsigned int channels;

    if (dev == (knm_device *)0 || !dev->opened) {
        return KNM_BAD_ARGUMENT;
    }
    if (dev->backend_id != KNM_BACKEND_NULL) {
        return KNM_NOT_SUPPORTED;
    }

    if (frames == 0U) {
        frames = dev->cfg.frames_per_buffer;
    }
    if (frames == 0U) {
        return KNM_BAD_ARGUMENT;
    }

    channels = dev->cfg.channels;
    remaining = frames;
    while (remaining > 0U) {
        chunk_frames = remaining;
        if (chunk_frames > KNM_MAX_FRAMES_PER_BUFFER) {
            chunk_frames = KNM_MAX_FRAMES_PER_BUFFER;
        }

        if (dev->cfg.enable_input) {
            mnk_zero_frames(dev->input_scratch, chunk_frames, channels);
            mnk_submit_capture_fix(dev, dev->input_scratch, chunk_frames);
        }
        if (dev->cfg.enable_output) {
            mnk_run_output_callback(dev, dev->output_scratch, chunk_frames);
            dev->stats.rendered_frames += (unsigned long)chunk_frames;
        }
        remaining -= chunk_frames;
    }

    return KNM_OK;
}

int mnk_backend_id_of_device(const knm_device *dev)
{
    if (dev == (const knm_device *)0) {
        return KNM_BACKEND_DEFAULT;
    }
    return dev->backend_id;
}

int mnk_get_device_config(const knm_device *dev, knm_audio_config *out_actual)
{
    if (dev == (const knm_device *)0 || out_actual == (knm_audio_config *)0) {
        return KNM_BAD_ARGUMENT;
    }
    *out_actual = dev->cfg;
    return KNM_OK;
}

int mnk_get_requested_device_config(const knm_device *dev, knm_audio_config *out_requested)
{
    if (dev == (const knm_device *)0 || out_requested == (knm_audio_config *)0) {
        return KNM_BAD_ARGUMENT;
    }
    *out_requested = dev->requested_cfg;
    return KNM_OK;
}

int mnk_get_opened_device_info(const knm_device *dev, knm_device_info *out_info)
{
    if (dev == (const knm_device *)0 || out_info == (knm_device_info *)0) {
        return KNM_BAD_ARGUMENT;
    }
    if (!dev->opened_device_valid) {
        return KNM_INVALID_STATE;
    }
    *out_info = dev->opened_device;
    return KNM_OK;
}

unsigned long mnk_output_latency_frames_device(const knm_device *dev)
{
    if (dev == (const knm_device *)0) {
        return 0UL;
    }
    return dev->output_latency_frames;
}

unsigned long mnk_input_latency_frames_device(const knm_device *dev)
{
    if (dev == (const knm_device *)0) {
        return 0UL;
    }
    return dev->input_latency_frames;
}

int mnk_last_error_device(const knm_device *dev)
{
    if (dev == (const knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }
    return dev->last_error;
}

unsigned long mnk_status_flags_device(const knm_device *dev)
{
    if (dev == (const knm_device *)0) {
        return 0UL;
    }
    return dev->status_flags;
}

int mnk_get_stream_stats(const knm_device *dev, knm_stream_stats *out_stats)
{
    if (dev == (const knm_device *)0 || out_stats == (knm_stream_stats *)0) {
        return KNM_BAD_ARGUMENT;
    }
    *out_stats = dev->stats;
    return KNM_OK;
}

void mnk_reset_stream_stats(knm_device *dev)
{
    if (dev == (knm_device *)0) {
        return;
    }
    mnk_zero((void *)&dev->stats, (unsigned long)sizeof(dev->stats));
}

unsigned int mnk_capture_available_device(const knm_device *dev)
{
    return mnk_ring_available(dev);
}

unsigned int mnk_capture_read_device(knm_device *dev,
                                     knm_fix16 *dst_frames,
                                     unsigned int frames_to_read)
{
    unsigned int read_frames;

    read_frames = mnk_ring_read(dev, dst_frames, frames_to_read);
    if (dev != (knm_device *)0 && read_frames > 0U) {
        dev->stats.capture_reads += 1UL;
        dev->stats.capture_read_frames += (unsigned long)read_frames;
    }
    return read_frames;
}

unsigned long mnk_capture_overruns_device(const knm_device *dev)
{
    unsigned long overruns;
    knm_ring *ring;

    if (dev == (const knm_device *)0) {
        return 0UL;
    }

    ring = (knm_ring *)(void *)&dev->capture_ring;
    mnk_ring_lock(ring);
    overruns = ring->overruns;
    mnk_ring_unlock(ring);
    return overruns;
}
