#include "mnk_core_internal.h"

#define KNM_FIX16_INT_MAX 32767
#define KNM_FIX16_INT_MIN (-32768)

static knm_fix16 knm_fix_apply_sign(knm_uint32 magnitude, int negative)
{
    if (negative) {
        if (magnitude > 0x80000000UL) {
            return KNM_FIX16_MIN;
        }
        if (magnitude == 0x80000000UL) {
            return KNM_FIX16_MIN;
        }
        return (knm_fix16)(-(knm_int32)magnitude);
    }

    if (magnitude > 0x7FFFFFFFUL) {
        return KNM_FIX16_MAX;
    }
    return (knm_fix16)magnitude;
}

static knm_uint32 knm_abs_u32(knm_fix16 value, int *negative)
{
    if (value < 0) {
        *negative = 1;
        if (value == KNM_FIX16_MIN) {
            return 0x80000000UL;
        }
        return (knm_uint32)(-value);
    }

    *negative = 0;
    return (knm_uint32)value;
}

static knm_uint32 knm_mul_u32_shift16(knm_uint32 a, knm_uint32 b, int *overflow)
{
    knm_uint32 a0;
    knm_uint32 a1;
    knm_uint32 b0;
    knm_uint32 b1;
    knm_uint32 p0;
    knm_uint32 p1;
    knm_uint32 p2;
    knm_uint32 p3;
    knm_uint32 w1;
    knm_uint32 w2;
    knm_uint32 w3;
    knm_uint32 carry;

    a0 = a & 0xFFFFUL;
    a1 = a >> 16;
    b0 = b & 0xFFFFUL;
    b1 = b >> 16;

    p0 = a0 * b0;
    p1 = a0 * b1;
    p2 = a1 * b0;
    p3 = a1 * b1;

    w1 = (p0 >> 16) + (p1 & 0xFFFFUL) + (p2 & 0xFFFFUL);
    carry = w1 >> 16;
    w1 &= 0xFFFFUL;

    w2 = (p1 >> 16) + (p2 >> 16) + (p3 & 0xFFFFUL) + carry;
    carry = w2 >> 16;
    w2 &= 0xFFFFUL;

    w3 = (p3 >> 16) + carry;
    if (overflow != (int *)0) {
        *overflow = (w3 != 0UL) ? 1 : 0;
    }

    return w1 | (w2 << 16);
}

void knm_audio_config_init(knm_audio_config *cfg)
{
    if (cfg == (knm_audio_config *)0) {
        return;
    }

    cfg->sample_rate = 48000UL;
    cfg->channels = 2U;
    cfg->frames_per_buffer = 256U;
    cfg->buffer_count = 4U;
    cfg->capture_ring_frames = 0U;
    cfg->enable_output = 1;
    cfg->enable_input = 0;
    cfg->backend = KNM_BACKEND_DEFAULT;
    cfg->prefer_low_latency = 1;
    cfg->device_id[0] = '\0';
}

unsigned long knm_audio_version(void)
{
    return KNM_AUDIO_VERSION_HEX;
}

const char *knm_audio_version_string(void)
{
    return KNM_AUDIO_VERSION_STRING;
}

const char *knm_result_string(int code)
{
    switch (code) {
        case KNM_OK: return "ok";
        case KNM_ERROR: return "generic error";
        case KNM_BAD_ARGUMENT: return "bad argument";
        case KNM_NO_SLOT: return "no static device slot available";
        case KNM_BACKEND_UNAVAILABLE: return "backend unavailable at runtime";
        case KNM_DEVICE_ERROR: return "device error";
        case KNM_LIMIT_EXCEEDED: return "compile-time limit exceeded";
        case KNM_NOT_RUNNING: return "device not running";
        case KNM_NOT_SUPPORTED: return "feature not supported by selected backend";
        case KNM_INVALID_STATE: return "invalid device state";
        case KNM_BACKEND_NOT_COMPILED: return "backend not compiled into this build";
        case KNM_PERMISSION_DENIED: return "permission denied";
        case KNM_FORMAT_UNSUPPORTED: return "audio format unsupported by backend";
        case KNM_BUFFER_OVERFLOW: return "audio input overflow";
        case KNM_BUFFER_UNDERFLOW: return "audio output underflow";
        case KNM_DEVICE_NOT_FOUND: return "requested device was not found";
        default: return "unknown result";
    }
}

const char *knm_backend_name(int backend)
{
    return mnk_backend_name_from_id(backend);
}

int knm_audio_backend_compiled(int backend)
{
    return mnk_backend_compiled(backend);
}

int knm_audio_backend_available(int backend)
{
    return mnk_backend_available(backend);
}

unsigned int knm_audio_backend_count(void)
{
    return mnk_backend_count();
}

int knm_audio_backend_info(unsigned int index, knm_backend_info *out_info)
{
    return mnk_backend_info_at(index, out_info);
}

unsigned int knm_audio_device_count(int backend)
{
    return mnk_device_count_for_backend(backend);
}

int knm_audio_device_info(int backend, unsigned int index, knm_device_info *out_info)
{
    return mnk_device_info_for_backend(backend, index, out_info);
}

int knm_audio_find_device(int backend, const char *device_id, knm_device_info *out_info)
{
    return mnk_find_device(backend, device_id, out_info);
}

unsigned int knm_audio_default_backend_order(int *out_backends, unsigned int capacity)
{
    return mnk_default_backend_order_copy(out_backends, capacity);
}

knm_fix16 knm_fix_from_int(int value)
{
    if (value > KNM_FIX16_INT_MAX) {
        return KNM_FIX16_MAX;
    }
    if (value < KNM_FIX16_INT_MIN) {
        return KNM_FIX16_MIN;
    }
    return (knm_fix16)(value * 65536L);
}

knm_fix16 knm_fix_from_ratio(int numerator, int denominator)
{
    knm_uint32 ua;
    knm_uint32 ub;
    knm_uint32 whole;
    knm_uint32 rem;
    knm_uint32 frac;
    int negative;
    int sign_a;
    int sign_b;
    unsigned int i;

    if (denominator == 0) {
        return (knm_fix16)0L;
    }

    ua = (numerator < 0) ? (knm_uint32)(0U - (knm_uint32)numerator) : (knm_uint32)numerator;
    ub = (denominator < 0) ? (knm_uint32)(0U - (knm_uint32)denominator) : (knm_uint32)denominator;
    sign_a = (numerator < 0) ? 1 : 0;
    sign_b = (denominator < 0) ? 1 : 0;
    negative = (sign_a != sign_b) ? 1 : 0;

    if (ub == 0U) {
        return (knm_fix16)0L;
    }

    whole = ua / ub;
    rem = ua % ub;
    if (whole > 32768UL) {
        return negative ? KNM_FIX16_MIN : KNM_FIX16_MAX;
    }
    if (!negative && whole >= 32768UL) {
        return KNM_FIX16_MAX;
    }

    frac = 0U;
    for (i = 0U; i < 16U; ++i) {
        frac <<= 1;
        rem <<= 1;
        if (rem >= ub) {
            rem -= ub;
            frac |= 1U;
        }
    }

    return knm_fix_apply_sign((whole << 16) | frac, negative);
}

knm_fix16 knm_fix_mul(knm_fix16 a, knm_fix16 b)
{
    knm_uint32 ua;
    knm_uint32 ub;
    knm_uint32 magnitude;
    int neg_a;
    int neg_b;
    int overflow;

    ua = knm_abs_u32(a, &neg_a);
    ub = knm_abs_u32(b, &neg_b);
    magnitude = knm_mul_u32_shift16(ua, ub, &overflow);
    if (overflow) {
        return (neg_a != neg_b) ? KNM_FIX16_MIN : KNM_FIX16_MAX;
    }
    return knm_fix_apply_sign(magnitude, (neg_a != neg_b) ? 1 : 0);
}

knm_fix16 knm_fix_abs(knm_fix16 value)
{
    if (value == KNM_FIX16_MIN) {
        return KNM_FIX16_MAX;
    }
    if (value < 0) {
        return (knm_fix16)(-value);
    }
    return value;
}

knm_fix16 knm_fix_clamp_unit(knm_fix16 value)
{
    if (value > KNM_FIX_ONE) {
        return KNM_FIX_ONE;
    }
    if (value < -KNM_FIX_ONE) {
        return -KNM_FIX_ONE;
    }
    return value;
}

int knm_audio_open(knm_device **out_device,
                   const knm_audio_config *cfg,
                   knm_audio_callback callback,
                   void *user_data)
{
    return mnk_open_device(out_device, cfg, callback, user_data);
}

int knm_audio_start(knm_device *dev)
{
    return mnk_start_device(dev);
}

int knm_audio_stop(knm_device *dev)
{
    return mnk_stop_device(dev);
}

void knm_audio_close(knm_device *dev)
{
    mnk_close_device(dev);
}

int knm_audio_is_running(const knm_device *dev)
{
    return mnk_is_running_device(dev);
}

int knm_audio_service(knm_device *dev, unsigned int frames)
{
    return mnk_service_device(dev, frames);
}

int knm_audio_backend(const knm_device *dev)
{
    return mnk_backend_id_of_device(dev);
}

int knm_audio_get_config(const knm_device *dev, knm_audio_config *out_actual)
{
    return mnk_get_device_config(dev, out_actual);
}

int knm_audio_get_requested_config(const knm_device *dev, knm_audio_config *out_requested)
{
    return mnk_get_requested_device_config(dev, out_requested);
}

int knm_audio_get_opened_device_info(const knm_device *dev, knm_device_info *out_info)
{
    return mnk_get_opened_device_info(dev, out_info);
}

unsigned long knm_audio_output_latency_frames(const knm_device *dev)
{
    return mnk_output_latency_frames_device(dev);
}

unsigned long knm_audio_input_latency_frames(const knm_device *dev)
{
    return mnk_input_latency_frames_device(dev);
}

int knm_audio_last_error(const knm_device *dev)
{
    return mnk_last_error_device(dev);
}

const char *knm_audio_last_error_string(const knm_device *dev)
{
    return knm_result_string(mnk_last_error_device(dev));
}

unsigned long knm_audio_status_flags(const knm_device *dev)
{
    return mnk_status_flags_device(dev);
}

void knm_audio_clear_status(knm_device *dev, unsigned long flags)
{
    mnk_clear_status_flags_mask(dev, flags);
}

int knm_audio_get_stats(const knm_device *dev, knm_stream_stats *out_stats)
{
    return mnk_get_stream_stats(dev, out_stats);
}

void knm_audio_reset_stats(knm_device *dev)
{
    mnk_reset_stream_stats(dev);
}

unsigned int knm_audio_capture_available(const knm_device *dev)
{
    return mnk_capture_available_device(dev);
}

unsigned int knm_audio_capture_read(knm_device *dev,
                                    knm_fix16 *dst_frames,
                                    unsigned int frames_to_read)
{
    return mnk_capture_read_device(dev, dst_frames, frames_to_read);
}

unsigned long knm_audio_capture_overruns(const knm_device *dev)
{
    return mnk_capture_overruns_device(dev);
}
