#ifndef GOLDIE_AUDIO89_H
#define GOLDIE_AUDIO89_H

/* Goldie Audio System 89 v0.3.0
   True Matryoshka submix runtime.
   C89, fixed/static ownership, no heap in Goldie runtime.

   Every Goldie console owns an independent rawmix engine. A child console is
   rendered first; its processed PCM output is injected as one input channel
   (a transient protected rawmix voice) into its parent console. Rendering is
   bottom-up until MASTER reaches KNM_audio.
*/

#include "rawmix.h"
#include "knm_hwr_audio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GOLDIE_AUDIO89_VERSION_MAJOR 0
#define GOLDIE_AUDIO89_VERSION_MINOR 3
#define GOLDIE_AUDIO89_VERSION_PATCH 0
#define GOLDIE_AUDIO89_ERROR_CAP 256

#ifndef GOLDIE_AUDIO89_MAX_CONSOLES
#define GOLDIE_AUDIO89_MAX_CONSOLES 64
#endif
#ifndef GOLDIE_AUDIO89_CONSOLE_NAME_CAP
#define GOLDIE_AUDIO89_CONSOLE_NAME_CAP 32
#endif

#define GOLDIE_AUDIO89_INVALID_SLOT ((unsigned short)0xFFFFU)
#define GOLDIE_AUDIO89_Q15_UNITY ((short)32767)
#define GOLDIE_AUDIO89_PAN_CENTER ((short)0)

enum goldie_audio89_result {
    GOLDIE_AUDIO89_OK = 0,
    GOLDIE_AUDIO89_ERR_ARGUMENT = -1,
    GOLDIE_AUDIO89_ERR_STATE = -2,
    GOLDIE_AUDIO89_ERR_MIXER = -3,
    GOLDIE_AUDIO89_ERR_HARDWARE = -4,
    GOLDIE_AUDIO89_ERR_FILE = -5,
    GOLDIE_AUDIO89_ERR_WAV = -6,
    GOLDIE_AUDIO89_ERR_PCM = -7,
    GOLDIE_AUDIO89_ERR_CAPACITY = -8,
    GOLDIE_AUDIO89_ERR_MP3 = -9,
    GOLDIE_AUDIO89_ERR_UNSUPPORTED = -10,
    GOLDIE_AUDIO89_ERR_CONSOLE = -11,
    GOLDIE_AUDIO89_ERR_CYCLE = -12,
    GOLDIE_AUDIO89_ERR_BUSY = -13
};

typedef struct goldie_audio89_console_handle {
    unsigned short slot;
    unsigned short generation;
} goldie_audio89_console_handle;

typedef struct goldie_audio89_voice_handle {
    unsigned short console_slot;
    unsigned short console_generation;
    rm_voice_handle raw;
} goldie_audio89_voice_handle;

typedef struct goldie_audio89_config {
    unsigned long sample_rate;
    unsigned int channels;
    unsigned int frames_per_buffer;
    unsigned int buffer_count;
    unsigned int max_voices_per_console;
    unsigned int max_consoles;
    int backend;
    int prefer_low_latency;
} goldie_audio89_config;

typedef struct goldie_audio89_pcm_view {
    const short *samples;
    unsigned long frame_count;
    unsigned long sample_rate;
    unsigned int channels;
} goldie_audio89_pcm_view;

typedef struct goldie_audio89_console_state {
    unsigned char active;
    unsigned char reserved0;
    unsigned short generation;
    unsigned short parent_slot;
    unsigned short parent_generation;
    unsigned short depth;
    unsigned short reserved1;
    char name[GOLDIE_AUDIO89_CONSOLE_NAME_CAP];
    rm_engine mixer;
    rm_s16 output[KNM_MAX_FRAMES_PER_BUFFER * KNM_MAX_CHANNELS];
} goldie_audio89_console_state;

typedef struct goldie_audio89_stats {
    unsigned long graph_renders;
    unsigned long child_channels_injected;
    unsigned long child_channel_failures;
} goldie_audio89_stats;

typedef struct goldie_audio89 {
    knm_device *device;
    goldie_audio89_config config;
    int initialized;
    int running;
    int paused;
    char error[GOLDIE_AUDIO89_ERROR_CAP];
    goldie_audio89_console_state consoles[GOLDIE_AUDIO89_MAX_CONSOLES];
    unsigned short render_order[GOLDIE_AUDIO89_MAX_CONSOLES];
    unsigned int console_limit;
    unsigned int console_count;
    unsigned int render_count;
    goldie_audio89_stats stats;
} goldie_audio89;

void goldie_audio89_config_init(goldie_audio89_config *cfg);
int goldie_audio89_init(goldie_audio89 *g, const goldie_audio89_config *cfg);
int goldie_audio89_start(goldie_audio89 *g);
int goldie_audio89_pause(goldie_audio89 *g);
int goldie_audio89_resume(goldie_audio89 *g);
int goldie_audio89_stop_device(goldie_audio89 *g);
void goldie_audio89_shutdown(goldie_audio89 *g);

/* Console graph. MASTER is slot 0. Each non-master console has one parent. */
goldie_audio89_console_handle goldie_audio89_master_console(const goldie_audio89 *g);
goldie_audio89_console_handle goldie_audio89_invalid_console(void);
int goldie_audio89_console_is_valid(const goldie_audio89 *g, goldie_audio89_console_handle c);
int goldie_audio89_console_create(goldie_audio89 *g, goldie_audio89_console_handle parent,
                                  const char *name, goldie_audio89_console_handle *out_console);
int goldie_audio89_console_destroy(goldie_audio89 *g, goldie_audio89_console_handle c);
int goldie_audio89_console_reparent(goldie_audio89 *g, goldie_audio89_console_handle c,
                                    goldie_audio89_console_handle new_parent);
int goldie_audio89_console_parent(const goldie_audio89 *g, goldie_audio89_console_handle c,
                                  goldie_audio89_console_handle *out_parent);
const char *goldie_audio89_console_name(const goldie_audio89 *g, goldie_audio89_console_handle c);
unsigned int goldie_audio89_console_count(const goldie_audio89 *g);
unsigned int goldie_audio89_console_capacity(const goldie_audio89 *g);
unsigned int goldie_audio89_console_child_count(const goldie_audio89 *g, goldie_audio89_console_handle c);
unsigned int goldie_audio89_console_active_voice_count(const goldie_audio89 *g, goldie_audio89_console_handle c);

/* Full-console controls. These act on the combined signal of local voices + child console outputs. */
int goldie_audio89_console_set_gain_q15(goldie_audio89 *g, goldie_audio89_console_handle c, short gain_q15);
int goldie_audio89_console_set_pan_q15(goldie_audio89 *g, goldie_audio89_console_handle c, short pan_q15);
int goldie_audio89_console_set_mute(goldie_audio89 *g, goldie_audio89_console_handle c, int mute_on);
int goldie_audio89_console_fx_clear(goldie_audio89 *g, goldie_audio89_console_handle c, unsigned int slot);
int goldie_audio89_console_fx_lowpass(goldie_audio89 *g, goldie_audio89_console_handle c, unsigned int slot,
                                      unsigned long cutoff_hz, short wet_q15, short output_gain_q15);
int goldie_audio89_console_fx_drive(goldie_audio89 *g, goldie_audio89_console_handle c, unsigned int slot,
                                    unsigned int drive_q12, short threshold_q15, short wet_q15, short output_gain_q15);
int goldie_audio89_console_set_limiter(goldie_audio89 *g, goldie_audio89_console_handle c,
                                       short threshold_q15, unsigned int attack_frames,
                                       unsigned int release_frames, short output_gain_q15,
                                       unsigned int lookahead_frames);
int goldie_audio89_console_clear_limiter(goldie_audio89 *g, goldie_audio89_console_handle c);
int goldie_audio89_console_get_meter(const goldie_audio89 *g, goldie_audio89_console_handle c,
                                     rm_meter_state *out_meter);
int goldie_audio89_console_get_mixer_stats(const goldie_audio89 *g, goldie_audio89_console_handle c,
                                           rm_engine_stats *out_stats);

/* Sources live inside a console. */
int goldie_audio89_console_play_pcm_s16(goldie_audio89 *g, goldie_audio89_console_handle c,
                                        const goldie_audio89_pcm_view *pcm, int loop,
                                        goldie_audio89_voice_handle *out_voice);
int goldie_audio89_voice_stop(goldie_audio89 *g, goldie_audio89_voice_handle voice);
int goldie_audio89_voice_set_gain_q15(goldie_audio89 *g, goldie_audio89_voice_handle voice, short gain_q15);
int goldie_audio89_voice_set_pan_q15(goldie_audio89 *g, goldie_audio89_voice_handle voice, short pan_q15);
int goldie_audio89_voice_is_active(const goldie_audio89 *g, goldie_audio89_voice_handle voice);
int goldie_audio89_stop_all(goldie_audio89 *g);

/* Offline/block render of the full Matryoshka graph. Do not call concurrently with a running device. */
int goldie_audio89_render_s16(goldie_audio89 *g, short *dst_interleaved, unsigned int frames);
void goldie_audio89_get_stats(const goldie_audio89 *g, goldie_audio89_stats *out_stats);

/* Compatibility helpers: operate on MASTER only. */
int goldie_audio89_play_pcm_s16(goldie_audio89 *g, const goldie_audio89_pcm_view *pcm,
                                int loop, rm_voice_handle *out_voice);
int goldie_audio89_stop_voice(goldie_audio89 *g, rm_voice_handle voice);
int goldie_audio89_set_master_gain_q15(goldie_audio89 *g, short gain_q15);
int goldie_audio89_set_voice_gain_q15(goldie_audio89 *g, rm_voice_handle voice, short gain_q15);
int goldie_audio89_set_voice_pan_q15(goldie_audio89 *g, rm_voice_handle voice, short pan_q15);
int goldie_audio89_is_voice_active(const goldie_audio89 *g, rm_voice_handle voice);
int goldie_audio89_get_mixer_stats(const goldie_audio89 *g, rm_engine_stats *out_stats);

int goldie_audio89_decode_wav_memory(const void *wav_bytes, unsigned long wav_byte_count,
                                     short *pcm_dst, unsigned long pcm_sample_capacity,
                                     goldie_audio89_pcm_view *out_pcm);
int goldie_audio89_decode_wav_file(const char *path, unsigned char *file_workspace,
                                   unsigned long file_workspace_bytes, short *pcm_dst,
                                   unsigned long pcm_sample_capacity, goldie_audio89_pcm_view *out_pcm);
int goldie_audio89_decode_mp3_file(const char *path, short *pcm_dst,
                                   unsigned long pcm_sample_capacity, goldie_audio89_pcm_view *out_pcm,
                                   char *error_text, unsigned int error_text_capacity);

int goldie_audio89_service(goldie_audio89 *g, unsigned int frames);
int goldie_audio89_backend(const goldie_audio89 *g);
int goldie_audio89_get_hardware_config(const goldie_audio89 *g, goldie_audio89_config *out_cfg);
unsigned long goldie_audio89_output_latency_frames(const goldie_audio89 *g);
const char *goldie_audio89_last_error(const goldie_audio89 *g);
const char *goldie_audio89_result_string(int result);

#ifdef __cplusplus
}
#endif
#endif
