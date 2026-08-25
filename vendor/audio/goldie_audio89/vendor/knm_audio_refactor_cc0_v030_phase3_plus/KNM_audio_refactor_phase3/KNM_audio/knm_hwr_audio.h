#ifndef KNM_HWR_AUDIO_H
#define KNM_HWR_AUDIO_H

/*
    SPDX-License-Identifier: CC0-1.0

    knm_hwr_audio.h
    ------------------------------------------------------------
    KNM hardware audio layer.

    Goals:
      - C89-friendly core API
      - fixed-point internal sample format (signed Q16.16)
      - no heap allocation inside the shared core
      - backend-pluggable architecture
      - stable 32-bit public knm_fix16 ABI

    Threading contract:
      - knm_audio_open/start/stop/close are not reentrant on the same device.
      - callback execution is owned by the selected backend.
      - the callback must not block, allocate, sleep, do I/O, or call control APIs.
      - capture queries are safe against the internal capture ring synchronization.

    Notes:
      - define KNM_AUDIO_ENABLE_<BACKEND>=1 in your build to compile
        and register the corresponding backend on the target platform.
      - compile only the enabled backend translation units plus the null
        backend; disabled backends do not need stub object files.
      - the public/core stays plain C; the Haiku backend is C++ only.
      - an empty cfg.device_id selects the backend default device.
      - knm_audio_service() is intended for the null backend and deterministic
        tests; real host backends usually return KNM_NOT_SUPPORTED.
*/

#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef KNM_MAX_DEVICES
#define KNM_MAX_DEVICES 4
#endif

#ifndef KNM_MAX_CHANNELS
#define KNM_MAX_CHANNELS 2
#endif

#ifndef KNM_MAX_STREAM_BUFFERS
#define KNM_MAX_STREAM_BUFFERS 4
#endif

#ifndef KNM_MAX_FRAMES_PER_BUFFER
#define KNM_MAX_FRAMES_PER_BUFFER 1024
#endif

#ifndef KNM_MAX_CAPTURE_RING_FRAMES
#define KNM_MAX_CAPTURE_RING_FRAMES 16384
#endif

#ifndef KNM_DEFAULT_CAPTURE_RING_FRAMES
#define KNM_DEFAULT_CAPTURE_RING_FRAMES 4096
#endif

#ifndef KNM_BACKEND_STATE_BYTES
#define KNM_BACKEND_STATE_BYTES 8192
#endif

#ifndef KNM_AUDIO_DEVICE_ID_BYTES
#define KNM_AUDIO_DEVICE_ID_BYTES 64
#endif

#ifndef KNM_AUDIO_DEVICE_NAME_BYTES
#define KNM_AUDIO_DEVICE_NAME_BYTES 64
#endif

#define KNM_AUDIO_VERSION_MAJOR 0
#define KNM_AUDIO_VERSION_MINOR 3
#define KNM_AUDIO_VERSION_PATCH 0
#define KNM_AUDIO_VERSION_HEX   0x000300UL
#define KNM_AUDIO_VERSION_STRING "0.3.0"

/*
    Public Q16.16 sample type.
    This must stay exactly 32 bits across ILP32/LP64 targets.
*/
#if (UINT_MAX == 0xFFFFFFFFUL)
typedef int knm_int32;
typedef unsigned int knm_uint32;
#elif (ULONG_MAX == 0xFFFFFFFFUL)
typedef long knm_int32;
typedef unsigned long knm_uint32;
#else
#error "KNM_audio requires a native 32-bit integer type for knm_fix16."
#endif

typedef knm_int32 knm_fix16;
typedef struct knm_device knm_device;

#define KNM_FIX16_BITS 32
#define KNM_FIX16_MIN ((knm_fix16)(-2147483647L - 1L))
#define KNM_FIX16_MAX ((knm_fix16)2147483647L)

enum knm_result {
    KNM_OK = 0,
    KNM_ERROR = -1,
    KNM_BAD_ARGUMENT = -2,
    KNM_NO_SLOT = -3,
    KNM_BACKEND_UNAVAILABLE = -4,
    KNM_DEVICE_ERROR = -5,
    KNM_LIMIT_EXCEEDED = -6,
    KNM_NOT_RUNNING = -7,
    KNM_NOT_SUPPORTED = -8,
    KNM_INVALID_STATE = -9,
    KNM_BACKEND_NOT_COMPILED = -10,
    KNM_PERMISSION_DENIED = -11,
    KNM_FORMAT_UNSUPPORTED = -12,
    KNM_BUFFER_OVERFLOW = -13,
    KNM_BUFFER_UNDERFLOW = -14,
    KNM_DEVICE_NOT_FOUND = -15
};

enum knm_backend {
    KNM_BACKEND_DEFAULT = 0,
    KNM_BACKEND_NULL = 1,
    KNM_BACKEND_WINMM = 2,
    KNM_BACKEND_WASAPI = 3,
    KNM_BACKEND_ALSA = 4,
    KNM_BACKEND_PULSEAUDIO = 5,
    KNM_BACKEND_PIPEWIRE = 6,
    KNM_BACKEND_JACK = 7,
    KNM_BACKEND_COREAUDIO = 8,
    KNM_BACKEND_AAUDIO = 9,
    KNM_BACKEND_OPENSL_ES = 10,
    KNM_BACKEND_SNDIO = 11,
    KNM_BACKEND_WEBAUDIO = 12,
    KNM_BACKEND_HAIKU = 13
};

enum knm_status_flag {
    KNM_STATUS_NONE = 0,
    KNM_STATUS_INPUT_OVERFLOW = 1,
    KNM_STATUS_OUTPUT_UNDERFLOW = 2,
    KNM_STATUS_BACKEND_XRUN = 4
};

typedef struct knm_backend_info {
    int backend;
    char name[32];
    int compiled;
    int available;
    int supports_input;
    int supports_output;
    int supports_duplex;
    int supports_enumeration;
    unsigned int device_count;
} knm_backend_info;

typedef struct knm_device_info {
    int backend;
    unsigned int device_index;
    char device_id[KNM_AUDIO_DEVICE_ID_BYTES];
    char name[KNM_AUDIO_DEVICE_NAME_BYTES];
    int is_default;
    int supports_input;
    int supports_output;
    int supports_duplex;
    unsigned int max_input_channels;
    unsigned int max_output_channels;
    unsigned long default_sample_rate;
} knm_device_info;

typedef struct knm_stream_stats {
    unsigned long starts;
    unsigned long stops;
    unsigned long callback_calls;
    unsigned long input_callbacks;
    unsigned long output_callbacks;
    unsigned long rendered_frames;
    unsigned long captured_frames;
    unsigned long capture_reads;
    unsigned long capture_read_frames;
    unsigned long input_overflows;
    unsigned long backend_xruns;
} knm_stream_stats;

typedef struct knm_audio_config {
    unsigned long sample_rate;
    unsigned int channels;
    unsigned int frames_per_buffer;
    unsigned int buffer_count;
    unsigned int capture_ring_frames;
    int enable_output;
    int enable_input;
    int backend;
    int prefer_low_latency;
    char device_id[KNM_AUDIO_DEVICE_ID_BYTES];
} knm_audio_config;

typedef void (*knm_audio_callback)(
    void *user_data,
    const knm_fix16 *input,
    knm_fix16 *output,
    unsigned int frames,
    unsigned int channels
);

#define KNM_FIX_ZERO ((knm_fix16)0L)
#define KNM_FIX_HALF ((knm_fix16)32768L)
#define KNM_FIX_ONE  ((knm_fix16)65536L)

void knm_audio_config_init(knm_audio_config *cfg);

unsigned long knm_audio_version(void);
const char *knm_audio_version_string(void);

const char *knm_result_string(int code);
const char *knm_backend_name(int backend);
int knm_audio_backend_compiled(int backend);
int knm_audio_backend_available(int backend);
unsigned int knm_audio_backend_count(void);
int knm_audio_backend_info(unsigned int index, knm_backend_info *out_info);
unsigned int knm_audio_device_count(int backend);
int knm_audio_device_info(int backend, unsigned int index, knm_device_info *out_info);
int knm_audio_find_device(int backend, const char *device_id, knm_device_info *out_info);
unsigned int knm_audio_default_backend_order(int *out_backends, unsigned int capacity);

knm_fix16 knm_fix_from_int(int value);
knm_fix16 knm_fix_from_ratio(int numerator, int denominator);
knm_fix16 knm_fix_mul(knm_fix16 a, knm_fix16 b);
knm_fix16 knm_fix_abs(knm_fix16 value);
knm_fix16 knm_fix_clamp_unit(knm_fix16 value);

int knm_audio_open(knm_device **out_device,
                   const knm_audio_config *cfg,
                   knm_audio_callback callback,
                   void *user_data);
int knm_audio_start(knm_device *dev);
int knm_audio_stop(knm_device *dev);
void knm_audio_close(knm_device *dev);
int knm_audio_is_running(const knm_device *dev);
int knm_audio_service(knm_device *dev, unsigned int frames);

int knm_audio_backend(const knm_device *dev);
int knm_audio_get_config(const knm_device *dev, knm_audio_config *out_actual);
int knm_audio_get_requested_config(const knm_device *dev, knm_audio_config *out_requested);
int knm_audio_get_opened_device_info(const knm_device *dev, knm_device_info *out_info);
unsigned long knm_audio_output_latency_frames(const knm_device *dev);
unsigned long knm_audio_input_latency_frames(const knm_device *dev);
int knm_audio_last_error(const knm_device *dev);
const char *knm_audio_last_error_string(const knm_device *dev);
unsigned long knm_audio_status_flags(const knm_device *dev);
void knm_audio_clear_status(knm_device *dev, unsigned long flags);
int knm_audio_get_stats(const knm_device *dev, knm_stream_stats *out_stats);
void knm_audio_reset_stats(knm_device *dev);
unsigned int knm_audio_capture_available(const knm_device *dev);
unsigned int knm_audio_capture_read(knm_device *dev,
                                    knm_fix16 *dst_frames,
                                    unsigned int frames_to_read);
unsigned long knm_audio_capture_overruns(const knm_device *dev);

#ifdef __cplusplus
}
#endif

#endif /* KNM_HWR_AUDIO_H */
