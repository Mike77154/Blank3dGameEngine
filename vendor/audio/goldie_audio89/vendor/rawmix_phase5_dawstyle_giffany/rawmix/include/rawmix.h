#ifndef RAWMIX_H
#define RAWMIX_H

/*
    rawmix - host-driven fixed-point PCM mixer core
    ------------------------------------------------
    Goals:
    - C89-compatible public API
    - no malloc/new in the library core
    - no floating point in the library core
    - fixed-size voice pools
    - host-driven realtime processing for raw PCM input/output
*/

#ifdef __cplusplus
extern "C" {
#endif

#include <limits.h>

#if USHRT_MAX != 0xFFFFU
#error "rawmix requires 16-bit short"
#endif

#if UINT_MAX == 0xFFFFFFFFUL
typedef signed int rm_s32;
typedef unsigned int rm_u32;
#elif ULONG_MAX == 0xFFFFFFFFUL
typedef signed long rm_s32;
typedef unsigned long rm_u32;
#else
#error "rawmix requires a 32-bit int or long"
#endif

typedef signed short rm_s16;
typedef unsigned short rm_u16;
typedef unsigned char rm_u8;

enum rm_result_e {
    RM_OK = 0,
    RM_ERR_INVALID_ARG = -1,
    RM_ERR_BAD_STATE = -2,
    RM_ERR_NO_FREE_VOICE = -3,
    RM_ERR_BAD_HANDLE = -4,
    RM_ERR_UNSUPPORTED = -5,
    RM_ERR_EMPTY = -6,
    RM_ERR_QUEUE_FULL = -7
};
typedef enum rm_result_e rm_result;

enum rm_sample_format_e {
    RM_SAMPLE_S16 = 1
};
typedef enum rm_sample_format_e rm_sample_format;

enum rm_channel_layout_e {
    RM_CHANNEL_MONO = 1,
    RM_CHANNEL_STEREO = 2
};
typedef enum rm_channel_layout_e rm_channel_layout;

enum rm_resampler_mode_e {
    RM_RESAMPLER_DEFAULT = 0,
    RM_RESAMPLER_NEAREST = 1,
    RM_RESAMPLER_LINEAR = 2,
    RM_RESAMPLER_CUBIC = 3,
    RM_RESAMPLER_HQ = 3
};
typedef enum rm_resampler_mode_e rm_resampler_mode;

enum rm_bus_fx_type_e {
    RM_BUS_FX_NONE = 0,
    RM_BUS_FX_LOWPASS = 1,
    RM_BUS_FX_DRIVE = 2,
    RM_BUS_FX_BIQUAD = 3
};
typedef enum rm_bus_fx_type_e rm_bus_fx_type;

enum rm_send_mode_e {
    RM_SEND_PRE_FADER = 0,
    RM_SEND_POST_FADER = 1
};
typedef enum rm_send_mode_e rm_send_mode;

enum rm_automation_target_e {
    RM_AUTOMATION_NONE = 0,
    RM_AUTOMATION_MASTER_GAIN = 1,
    RM_AUTOMATION_HEADROOM = 2,
    RM_AUTOMATION_BUS_SET = 3,
    RM_AUTOMATION_GROUP_SET = 4,
    RM_AUTOMATION_VOICE_SET = 5,
    RM_AUTOMATION_BUS_MUTE = 6,
    RM_AUTOMATION_BUS_SOLO = 7,
    RM_AUTOMATION_GROUP_MUTE = 8,
    RM_AUTOMATION_GROUP_SOLO = 9,
    RM_AUTOMATION_BUS_SEND = 10
};
typedef enum rm_automation_target_e rm_automation_target;

typedef struct rm_biquad_desc_s {
    rm_s16 b0_q14;
    rm_s16 b1_q14;
    rm_s16 b2_q14;
    rm_s16 a1_q14;
    rm_s16 a2_q14;
    rm_s16 wet_q15;
    rm_s16 output_gain_q15;
} rm_biquad_desc;

enum rm_voice_flags_e {
    RM_VOICE_FLAG_NONE = 0,
    RM_VOICE_FLAG_LOOP = 1,
    RM_VOICE_FLAG_PROTECTED = 2
};
typedef enum rm_voice_flags_e rm_voice_flags;

#define RM_Q15_ONE   ((rm_s16)32767)
#define RM_Q15_ZERO  ((rm_s16)0)
#define RM_Q15_HALF  ((rm_s16)16384)
#define RM_PAN_LEFT  ((rm_s16)-32767)
#define RM_PAN_CENTER ((rm_s16)0)
#define RM_PAN_RIGHT ((rm_s16)32767)
#define RM_RATIO_ONE_Q12 ((rm_u16)4096)

#define RM_BUS_DEFAULT ((rm_u16)0)
#define RM_GROUP_DEFAULT ((rm_u16)0)

#ifndef RAWMIX_MAX_VOICES
#define RAWMIX_MAX_VOICES 32
#endif

#ifndef RAWMIX_CAPTURE_RING_FRAMES
#define RAWMIX_CAPTURE_RING_FRAMES 4096
#endif

#ifndef RAWMIX_MAX_BUSES
#define RAWMIX_MAX_BUSES 8
#endif

#ifndef RAWMIX_MAX_GROUPS
#define RAWMIX_MAX_GROUPS 8
#endif

#ifndef RAWMIX_MAX_BUS_FX
#define RAWMIX_MAX_BUS_FX 4
#endif

#ifndef RAWMIX_MAX_AUTOMATION_EVENTS
#define RAWMIX_MAX_AUTOMATION_EVENTS 64
#endif

#ifndef RAWMIX_LIMITER_MAX_LOOKAHEAD_FRAMES
#define RAWMIX_LIMITER_MAX_LOOKAHEAD_FRAMES 256
#endif

typedef struct rm_buffer_s {
    const void *samples;
    rm_u32 frame_count;
    rm_u32 sample_rate;
    rm_u16 channels;
    rm_u16 format;
    rm_u16 interleaved;
    rm_u16 reserved;
} rm_buffer;

typedef struct rm_stream_desc_s rm_stream_desc;
typedef rm_result (*rm_stream_next_proc)(void *user,
                                         rm_s16 *out_interleaved_frame,
                                         rm_u16 channels,
                                         rm_u16 *out_end_of_stream);

struct rm_stream_desc_s {
    rm_stream_next_proc on_next;
    void *user;
    rm_u32 sample_rate;
    rm_u16 channels;
    rm_u16 reserved;
};

typedef struct rm_voice_params_s {
    rm_s16 gain_q15;
    rm_s16 pan_q15;
    rm_u16 pitch_q12;
    rm_u16 flags;
    rm_u16 priority;
    rm_u16 bus_id;
    rm_u16 group_id;
    rm_u16 resampler;
    rm_u32 loop_start_frame;
    rm_u32 loop_end_frame;
    rm_u32 start_delay_frames;
    rm_u32 start_frame_offset;
    rm_u32 fade_in_frames;
} rm_voice_params;

typedef struct rm_voice_handle_s {
    rm_u16 slot;
    rm_u16 generation;
} rm_voice_handle;

typedef struct rm_engine_config_s {
    rm_u32 sample_rate;
    rm_u16 channels;
    rm_u16 max_voices;
    rm_s16 master_gain_q15;
    rm_s16 headroom_q15;
    rm_s16 monitor_gain_q15;
    rm_s16 monitor_pan_q15;
    rm_u16 capture_enabled;
    rm_u16 default_resampler;
} rm_engine_config;

typedef struct rm_engine_stats_s {
    rm_u32 voices_started;
    rm_u32 voices_finished;
    rm_u32 voices_stolen;
    rm_u32 capture_frames_pushed;
    rm_u32 capture_frames_read;
    rm_u32 capture_frames_dropped;
    rm_u32 clipping_events;
    rm_u32 render_calls;
    rm_u32 duplex_calls;
    rm_u32 bus_fx_frames;
    rm_u32 resampler_hq_frames;
    rm_u32 limiter_frames;
    rm_u32 automation_events_applied;
    rm_u32 send_frames;
} rm_engine_stats;

typedef struct rm_meter_state_s {
    rm_s16 peak_left_q15;
    rm_s16 peak_right_q15;
    rm_s16 env_left_q15;
    rm_s16 env_right_q15;
    rm_s16 gain_reduction_q15;
    rm_s16 reserved0;
    rm_u32 clip_events;
} rm_meter_state;

typedef struct rm_bus_send_state_s {
    rm_u8 active;
    rm_u8 mode;
    rm_s16 gain_q15;
} rm_bus_send_state;

typedef struct rm_bus_fx_state_s {
    rm_u8 type;
    rm_u8 active;
    rm_u16 reserved0;
    rm_u32 param0;
    rm_u32 param1;
    rm_s16 wet_q15;
    rm_s16 output_gain_q15;
    rm_s16 b0_q14;
    rm_s16 b1_q14;
    rm_s16 b2_q14;
    rm_s16 a1_q14;
    rm_s16 a2_q14;
    rm_s16 reserved1;
    rm_s32 state_left;
    rm_s32 state_right;
    rm_s32 state2_left;
    rm_s32 state2_right;
} rm_bus_fx_state;

typedef struct rm_bus_state_s {
    rm_u8 active;
    rm_u8 mute;
    rm_u8 solo;
    rm_u8 reserved0;
    rm_s16 gain_q15;
    rm_s16 pan_q15;
    rm_s16 left_gain_q15;
    rm_s16 right_gain_q15;
    rm_s16 target_gain_q15;
    rm_s16 target_pan_q15;
    rm_u32 ramp_frames_remaining;
    rm_meter_state meter;
    rm_bus_fx_state fx[RAWMIX_MAX_BUS_FX];
} rm_bus_state;

typedef struct rm_group_state_s {
    rm_u8 active;
    rm_u8 mute;
    rm_u8 solo;
    rm_u8 reserved0;
    rm_s16 gain_q15;
    rm_s16 pan_q15;
    rm_s16 left_gain_q15;
    rm_s16 right_gain_q15;
    rm_s16 target_gain_q15;
    rm_s16 target_pan_q15;
    rm_u32 ramp_frames_remaining;
} rm_group_state;

typedef struct rm_limiter_state_s {
    rm_u8 active;
    rm_u8 reserved0;
    rm_u16 attack_frames;
    rm_u16 release_frames;
    rm_u16 lookahead_frames;
    rm_u16 delay_write_frame;
    rm_u16 delay_count_frames;
    rm_u16 reserved1;
    rm_s16 threshold_q15;
    rm_s16 output_gain_q15;
    rm_s16 current_gain_q15;
    rm_s16 reserved2;
    rm_s32 delay_line[RAWMIX_LIMITER_MAX_LOOKAHEAD_FRAMES * 2];
} rm_limiter_state;

typedef struct rm_voice_state_s {
    rm_u8 active;
    rm_u8 src_channels;
    rm_u8 source_kind;
    rm_u8 stop_when_silent;
    rm_u16 generation;
    rm_u16 priority;
    rm_u16 bus_id;
    rm_u16 group_id;
    rm_u16 resampler;
    rm_u16 reserved_fx0;
    rm_u32 start_serial;
    const rm_s16 *data;
    rm_u32 frame_count;
    rm_u32 sample_rate;
    rm_u32 loop_start_frame;
    rm_u32 loop_end_frame;
    rm_u32 pos_frame;
    rm_u16 pos_frac_q12;
    rm_u16 step_q12;
    rm_s16 gain_q15;
    rm_s16 pan_q15;
    rm_s16 left_gain_q15;
    rm_s16 right_gain_q15;
    rm_s16 target_gain_q15;
    rm_s16 target_pan_q15;
    rm_u32 ramp_frames_remaining;
    rm_u16 flags;
    rm_u16 reserved0;
    rm_u32 start_delay_frames;
    rm_stream_next_proc stream_next;
    void *stream_user;
    rm_s16 stream_prev_frame[2];
    rm_s16 stream_curr[2];
    rm_s16 stream_next_frame[2];
    rm_s16 stream_next2_frame[2];
    rm_u8 stream_have_next;
    rm_u8 stream_have_next2;
    rm_u8 stream_eos;
    rm_u8 reserved1;
} rm_voice_state;

typedef struct rm_automation_event_s {
    rm_u16 sample_offset;
    rm_u8 type;
    rm_u8 reserved0;
    rm_u16 target_id;
    rm_u16 aux_id;
    rm_voice_handle handle;
    rm_s16 value0_q15;
    rm_s16 value1_q15;
    rm_u16 value2_u16;
    rm_u16 reserved1;
    rm_u32 value3_u32;
} rm_automation_event;

typedef struct rm_bus_mix_snapshot_s {
    rm_s16 gain_q15;
    rm_s16 pan_q15;
    rm_u8 mute;
    rm_u8 solo;
} rm_bus_mix_snapshot;

typedef struct rm_group_mix_snapshot_s {
    rm_s16 gain_q15;
    rm_s16 pan_q15;
    rm_u8 mute;
    rm_u8 solo;
} rm_group_mix_snapshot;

typedef struct rm_mix_snapshot_s {
    rm_s16 master_gain_q15;
    rm_s16 headroom_q15;
    rm_s16 monitor_gain_q15;
    rm_s16 monitor_pan_q15;
    rm_u8 limiter_enabled;
    rm_u8 reserved0;
    rm_u16 limiter_attack_frames;
    rm_u16 limiter_release_frames;
    rm_u16 limiter_lookahead_frames;
    rm_s16 limiter_threshold_q15;
    rm_s16 limiter_output_gain_q15;
    rm_bus_mix_snapshot buses[RAWMIX_MAX_BUSES];
    rm_group_mix_snapshot groups[RAWMIX_MAX_GROUPS];
    rm_bus_send_state sends[RAWMIX_MAX_BUSES][RAWMIX_MAX_BUSES];
} rm_mix_snapshot;

typedef struct rm_engine_s {
    rm_u32 sample_rate;
    rm_u16 channels;
    rm_u16 max_voices;
    rm_s16 master_gain_q15;
    rm_s16 headroom_q15;
    rm_s16 monitor_gain_q15;
    rm_s16 monitor_pan_q15;
    rm_s16 monitor_left_gain_q15;
    rm_s16 monitor_right_gain_q15;
    rm_u16 capture_enabled;
    rm_u16 default_resampler;
    rm_u16 capture_write_frame;
    rm_u16 capture_read_frame;
    rm_u16 capture_count_frames;
    rm_u16 automation_count;
    rm_u32 start_serial_counter;
    rm_engine_stats stats;
    rm_meter_state master_meter;
    rm_limiter_state limiter;
    rm_bus_state buses[RAWMIX_MAX_BUSES];
    rm_group_state groups[RAWMIX_MAX_GROUPS];
    rm_bus_send_state sends[RAWMIX_MAX_BUSES][RAWMIX_MAX_BUSES];
    rm_voice_state voices[RAWMIX_MAX_VOICES];
    rm_automation_event automation_queue[RAWMIX_MAX_AUTOMATION_EVENTS];
    rm_s16 capture_ring[RAWMIX_CAPTURE_RING_FRAMES * 2];
} rm_engine;

void rm_engine_config_init(rm_engine_config *cfg);
rm_result rm_engine_init(rm_engine *engine, const rm_engine_config *cfg);
void rm_engine_reset(rm_engine *engine);
void rm_engine_get_stats(const rm_engine *engine, rm_engine_stats *out_stats);

rm_result rm_engine_set_master_gain(rm_engine *engine, rm_s16 gain_q15);
rm_result rm_engine_set_default_resampler(rm_engine *engine, rm_u16 resampler);
rm_result rm_engine_set_headroom(rm_engine *engine, rm_s16 gain_q15);
rm_result rm_engine_set_monitor(rm_engine *engine, rm_s16 gain_q15, rm_s16 pan_q15);

rm_result rm_engine_set_bus(rm_engine *engine, rm_u16 bus_id, rm_s16 gain_q15, rm_s16 pan_q15);
rm_result rm_engine_ramp_bus(rm_engine *engine,
                             rm_u16 bus_id,
                             rm_s16 gain_q15,
                             rm_s16 pan_q15,
                             rm_u32 frames);
rm_result rm_engine_set_bus_mute(rm_engine *engine, rm_u16 bus_id, rm_u16 mute_on);
rm_result rm_engine_set_bus_solo(rm_engine *engine, rm_u16 bus_id, rm_u16 solo_on);
rm_result rm_engine_set_bus_send(rm_engine *engine,
                                 rm_u16 src_bus_id,
                                 rm_u16 dst_bus_id,
                                 rm_s16 gain_q15,
                                 rm_u16 mode);
rm_result rm_engine_clear_bus_send(rm_engine *engine, rm_u16 src_bus_id, rm_u16 dst_bus_id);
rm_result rm_engine_bus_fx_clear(rm_engine *engine, rm_u16 bus_id, rm_u16 slot);
rm_result rm_engine_bus_fx_set_lowpass(rm_engine *engine,
                                       rm_u16 bus_id,
                                       rm_u16 slot,
                                       rm_u32 cutoff_hz,
                                       rm_s16 wet_q15,
                                       rm_s16 output_gain_q15);
rm_result rm_engine_bus_fx_set_drive(rm_engine *engine,
                                     rm_u16 bus_id,
                                     rm_u16 slot,
                                     rm_u16 drive_q12,
                                     rm_s16 threshold_q15,
                                     rm_s16 wet_q15,
                                     rm_s16 output_gain_q15);
rm_result rm_engine_bus_fx_set_biquad(rm_engine *engine,
                                      rm_u16 bus_id,
                                      rm_u16 slot,
                                      const rm_biquad_desc *desc);
rm_result rm_engine_set_limiter(rm_engine *engine,
                                rm_s16 threshold_q15,
                                rm_u16 attack_frames,
                                rm_u16 release_frames,
                                rm_s16 output_gain_q15);
rm_result rm_engine_set_limiter_ex(rm_engine *engine,
                                   rm_s16 threshold_q15,
                                   rm_u16 attack_frames,
                                   rm_u16 release_frames,
                                   rm_s16 output_gain_q15,
                                   rm_u16 lookahead_frames);
rm_result rm_engine_clear_limiter(rm_engine *engine);
rm_u16 rm_engine_get_latency_frames(const rm_engine *engine);
rm_result rm_engine_get_bus_meter(const rm_engine *engine, rm_u16 bus_id, rm_meter_state *out_meter);
rm_result rm_engine_get_master_meter(const rm_engine *engine, rm_meter_state *out_meter);
rm_result rm_engine_set_group(rm_engine *engine, rm_u16 group_id, rm_s16 gain_q15, rm_s16 pan_q15);
rm_result rm_engine_ramp_group(rm_engine *engine,
                               rm_u16 group_id,
                               rm_s16 gain_q15,
                               rm_s16 pan_q15,
                               rm_u32 frames);
rm_result rm_engine_set_group_mute(rm_engine *engine, rm_u16 group_id, rm_u16 mute_on);
rm_result rm_engine_set_group_solo(rm_engine *engine, rm_u16 group_id, rm_u16 solo_on);
rm_result rm_engine_stop_group(rm_engine *engine, rm_u16 group_id);
rm_result rm_engine_capture_snapshot(const rm_engine *engine, rm_mix_snapshot *out_snapshot);
rm_result rm_engine_apply_snapshot(rm_engine *engine,
                                   const rm_mix_snapshot *snapshot,
                                   rm_u32 ramp_frames);
rm_result rm_engine_queue_automation(rm_engine *engine, const rm_automation_event *event_desc);
rm_result rm_engine_queue_automationv(rm_engine *engine,
                                      const rm_automation_event *event_descs,
                                      rm_u16 count);
void rm_engine_clear_automation(rm_engine *engine);

rm_result rm_engine_play_buffer(rm_engine *engine,
                                const rm_buffer *buffer,
                                const rm_voice_params *params,
                                rm_voice_handle *out_handle);
rm_result rm_engine_play_stream(rm_engine *engine,
                                const rm_stream_desc *stream,
                                const rm_voice_params *params,
                                rm_voice_handle *out_handle);
rm_result rm_engine_stop_voice(rm_engine *engine, rm_voice_handle handle);
rm_result rm_engine_set_voice_gain(rm_engine *engine, rm_voice_handle handle, rm_s16 gain_q15);
rm_result rm_engine_set_voice_pan(rm_engine *engine, rm_voice_handle handle, rm_s16 pan_q15);
rm_result rm_engine_set_voice_pitch(rm_engine *engine, rm_voice_handle handle, rm_u16 pitch_q12);
rm_result rm_engine_set_voice_bus(rm_engine *engine, rm_voice_handle handle, rm_u16 bus_id);
rm_result rm_engine_set_voice_group(rm_engine *engine, rm_voice_handle handle, rm_u16 group_id);
rm_result rm_engine_ramp_voice(rm_engine *engine,
                               rm_voice_handle handle,
                               rm_s16 gain_q15,
                               rm_s16 pan_q15,
                               rm_u32 frames);
rm_result rm_engine_fade_out_voice(rm_engine *engine, rm_voice_handle handle, rm_u32 frames);
int rm_engine_is_voice_active(const rm_engine *engine, rm_voice_handle handle);

rm_result rm_engine_render_s16(rm_engine *engine, rm_s16 *output_interleaved, rm_u32 frames);
rm_result rm_engine_process_duplex_s16(rm_engine *engine,
                                       const rm_s16 *input_interleaved,
                                       rm_u16 input_channels,
                                       rm_s16 *output_interleaved,
                                       rm_u16 output_channels,
                                       rm_u32 frames);
rm_u32 rm_engine_capture_available(const rm_engine *engine);
rm_result rm_engine_capture_read_s16(rm_engine *engine,
                                     rm_s16 *dst_interleaved,
                                     rm_u16 dst_channels,
                                     rm_u32 max_frames,
                                     rm_u32 *out_frames_read);

void rm_voice_params_init(rm_voice_params *params);

#ifdef __cplusplus
}
#endif

#endif
