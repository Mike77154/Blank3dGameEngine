#ifndef MNK_CORE_INTERNAL_H
#define MNK_CORE_INTERNAL_H

#include "knm_hwr_audio.h"

#include <limits.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef short knm_s16;

#define KNM_STREAM_SAMPLE_CAPACITY (KNM_MAX_FRAMES_PER_BUFFER * KNM_MAX_CHANNELS)

#if !defined(KNM_AUDIO_ENABLE_WINMM)
#define KNM_AUDIO_ENABLE_WINMM 0
#endif
#if !defined(KNM_AUDIO_ENABLE_WASAPI)
#define KNM_AUDIO_ENABLE_WASAPI 0
#endif
#if !defined(KNM_AUDIO_ENABLE_ALSA)
#define KNM_AUDIO_ENABLE_ALSA 0
#endif
#if !defined(KNM_AUDIO_ENABLE_PULSEAUDIO)
#define KNM_AUDIO_ENABLE_PULSEAUDIO 0
#endif
#if !defined(KNM_AUDIO_ENABLE_PIPEWIRE)
#define KNM_AUDIO_ENABLE_PIPEWIRE 0
#endif
#if !defined(KNM_AUDIO_ENABLE_JACK)
#define KNM_AUDIO_ENABLE_JACK 0
#endif
#if !defined(KNM_AUDIO_ENABLE_COREAUDIO)
#define KNM_AUDIO_ENABLE_COREAUDIO 0
#endif
#if !defined(KNM_AUDIO_ENABLE_AAUDIO)
#define KNM_AUDIO_ENABLE_AAUDIO 0
#endif
#if !defined(KNM_AUDIO_ENABLE_OPENSL_ES)
#define KNM_AUDIO_ENABLE_OPENSL_ES 0
#endif
#if !defined(KNM_AUDIO_ENABLE_SNDIO)
#define KNM_AUDIO_ENABLE_SNDIO 0
#endif
#if !defined(KNM_AUDIO_ENABLE_WEBAUDIO)
#define KNM_AUDIO_ENABLE_WEBAUDIO 0
#endif
#if !defined(KNM_AUDIO_ENABLE_HAIKU)
#define KNM_AUDIO_ENABLE_HAIKU 0
#endif

typedef struct knm_ring {
    volatile knm_uint32 lock_state;
    unsigned long read_pos;
    unsigned long write_pos;
    unsigned long used_frames;
    unsigned long overruns;
    unsigned int capacity_frames;
    knm_fix16 data[KNM_MAX_CAPTURE_RING_FRAMES * KNM_MAX_CHANNELS];
} knm_ring;

struct knm_backend_vtbl;
typedef struct knm_backend_vtbl knm_backend_vtbl;

struct knm_backend_vtbl {
    int backend_id;
    const char *name;
    int supports_input;
    int supports_output;
    int supports_duplex;
    int supports_enumeration;
    int (*probe)(void);
    unsigned int (*device_count)(void);
    int (*device_info)(unsigned int index, knm_device_info *out_info);
    int (*open)(knm_device *dev);
    int (*start)(knm_device *dev);
    int (*stop)(knm_device *dev);
    void (*close)(knm_device *dev);
};

struct knm_device {
    int in_use;
    int opened;
    int running;
    int backend_id;
    int last_error;
    int opened_device_valid;
    unsigned long status_flags;
    unsigned long output_latency_frames;
    unsigned long input_latency_frames;
    const knm_backend_vtbl *backend;

    knm_audio_config requested_cfg;
    knm_audio_config cfg;
    knm_device_info opened_device;
    knm_stream_stats stats;
    knm_audio_callback callback;
    void *user_data;

    knm_ring capture_ring;

    knm_fix16 input_scratch[KNM_STREAM_SAMPLE_CAPACITY];
    knm_fix16 output_scratch[KNM_STREAM_SAMPLE_CAPACITY];

    knm_s16 s16_input_scratch[KNM_STREAM_SAMPLE_CAPACITY];
    knm_s16 s16_output_scratch[KNM_STREAM_SAMPLE_CAPACITY];

    knm_s16 queue_input[KNM_MAX_STREAM_BUFFERS][KNM_STREAM_SAMPLE_CAPACITY];
    knm_s16 queue_output[KNM_MAX_STREAM_BUFFERS][KNM_STREAM_SAMPLE_CAPACITY];

    unsigned char backend_state[KNM_BACKEND_STATE_BYTES];
};

void mnk_zero(void *ptr, unsigned long size_bytes);
void mnk_copy_text(char *dst, unsigned int dst_capacity, const char *src);
void mnk_set_last_error(knm_device *dev, int rc);
void mnk_set_status_flag(knm_device *dev, unsigned long flag);
void mnk_clear_status_flags(knm_device *dev);
void mnk_clear_status_flags_mask(knm_device *dev, unsigned long flags);
void mnk_note_input_overflow(knm_device *dev, unsigned long count);
void mnk_note_backend_xrun(knm_device *dev);

int mnk_validate_config(const knm_audio_config *cfg);

void mnk_ring_init(knm_ring *ring, unsigned int capacity_frames);
unsigned int mnk_ring_available(const knm_device *dev);
unsigned int mnk_ring_read(knm_device *dev, knm_fix16 *dst, unsigned int frames);
void mnk_ring_write(knm_device *dev, const knm_fix16 *src, unsigned int frames);

void mnk_zero_frames(knm_fix16 *dst, unsigned int frames, unsigned int channels);
void mnk_zero_s16(knm_s16 *dst, unsigned int frames, unsigned int channels);

void mnk_s16_to_fix(knm_fix16 *dst, const knm_s16 *src, unsigned int samples);
void mnk_fix_to_s16(knm_s16 *dst, const knm_fix16 *src, unsigned int samples);

void mnk_run_input_callback(knm_device *dev, const knm_fix16 *input, unsigned int frames);
void mnk_run_output_callback(knm_device *dev, knm_fix16 *output, unsigned int frames);

void mnk_submit_capture_fix(knm_device *dev, const knm_fix16 *src, unsigned int frames);
void mnk_capture_from_s16(knm_device *dev, const knm_s16 *src, unsigned int frames);
void mnk_render_to_s16(knm_device *dev, knm_s16 *dst, unsigned int frames);

int mnk_open_device(knm_device **out_device,
                    const knm_audio_config *cfg,
                    knm_audio_callback callback,
                    void *user_data);
int mnk_start_device(knm_device *dev);
int mnk_stop_device(knm_device *dev);
void mnk_close_device(knm_device *dev);
int mnk_is_running_device(const knm_device *dev);
int mnk_service_device(knm_device *dev, unsigned int frames);
int mnk_backend_id_of_device(const knm_device *dev);
int mnk_get_device_config(const knm_device *dev, knm_audio_config *out_actual);
int mnk_get_requested_device_config(const knm_device *dev, knm_audio_config *out_requested);
int mnk_get_opened_device_info(const knm_device *dev, knm_device_info *out_info);
unsigned long mnk_output_latency_frames_device(const knm_device *dev);
unsigned long mnk_input_latency_frames_device(const knm_device *dev);
int mnk_last_error_device(const knm_device *dev);
unsigned long mnk_status_flags_device(const knm_device *dev);
int mnk_get_stream_stats(const knm_device *dev, knm_stream_stats *out_stats);
void mnk_reset_stream_stats(knm_device *dev);
unsigned int mnk_capture_available_device(const knm_device *dev);
unsigned int mnk_capture_read_device(knm_device *dev,
                                     knm_fix16 *dst_frames,
                                     unsigned int frames_to_read);
unsigned long mnk_capture_overruns_device(const knm_device *dev);

const knm_backend_vtbl *mnk_backend_by_id(int backend_id);
const char *mnk_backend_name_from_id(int backend_id);
int mnk_backend_compiled(int backend_id);
int mnk_backend_available(int backend_id);
unsigned int mnk_backend_count(void);
int mnk_backend_info_at(unsigned int index, knm_backend_info *out_info);
unsigned int mnk_device_count_for_backend(int backend_id);
int mnk_device_info_for_backend(int backend_id, unsigned int index, knm_device_info *out_info);
int mnk_find_device(int backend_id, const char *device_id, knm_device_info *out_info);
unsigned int mnk_default_backend_order_copy(int *out_backends, unsigned int capacity);

extern const knm_backend_vtbl knm_backend_null_vtbl;
extern const knm_backend_vtbl knm_backend_winmm_vtbl;
extern const knm_backend_vtbl knm_backend_wasapi_vtbl;
extern const knm_backend_vtbl knm_backend_alsa_vtbl;
extern const knm_backend_vtbl knm_backend_pulseaudio_vtbl;
extern const knm_backend_vtbl knm_backend_pipewire_vtbl;
extern const knm_backend_vtbl knm_backend_jack_vtbl;
extern const knm_backend_vtbl knm_backend_coreaudio_vtbl;
extern const knm_backend_vtbl knm_backend_aaudio_vtbl;
extern const knm_backend_vtbl knm_backend_opensl_es_vtbl;
extern const knm_backend_vtbl knm_backend_sndio_vtbl;
extern const knm_backend_vtbl knm_backend_webaudio_vtbl;
extern const knm_backend_vtbl knm_backend_haiku_vtbl;

#ifdef __cplusplus
}
#endif

#endif /* MNK_CORE_INTERNAL_H */
