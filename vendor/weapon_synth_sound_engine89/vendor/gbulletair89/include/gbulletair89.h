#ifndef GBULLETAIR89_H
#define GBULLETAIR89_H

#ifdef __cplusplus
extern "C" {
#endif

#define GBA89_VERSION_MAJOR    1
#define GBA89_VERSION_MINOR    2
#define GBA89_SAMPLE_RATE      48000UL
#define GBA89_CHORUS_SIZE      256
#define GBA89_REVERB_SIZE      2048
#define GBA89_Q15_ONE          32767
#define GBA89_Q12_ONE          4096

#define GBA89_PRESET_SUBSONIC_WHIZ      0
#define GBA89_PRESET_TRANSONIC_ZIP      1
#define GBA89_PRESET_SUPERSONIC_SNAP    2
#define GBA89_PRESET_CLOSE_RIFLE_PASS   3
#define GBA89_PRESET_HEAVY_ROUND_PASS   4
#define GBA89_PRESET_DISTANT_CRACK      5
#define GBA89_PRESET_INDOOR_PASS        6
#define GBA89_PRESET_GAME_WHIZZ         7
#define GBA89_PRESET_COUNT              8

typedef signed short gba89_s16;
typedef unsigned short gba89_u16;
typedef signed long gba89_s32;
typedef unsigned long gba89_u32;

typedef struct gba89_config_tag {
    gba89_u16 attack_ms;
    gba89_u16 decay_ms;
    gba89_u16 sustain_ms;
    gba89_u16 release_ms;
    gba89_s16 sustain_q15;
    gba89_s16 level_q15;

    gba89_s16 hp_start_q15;
    gba89_s16 hp_end_q15;
    gba89_s16 lp_start_q15;
    gba89_s16 lp_end_q15;
    gba89_s16 body_lp_q15;

    gba89_u16 drive_q8_8;
    gba89_s16 flutter_q15;

    gba89_s16 eq_sub_q12;
    gba89_s16 eq_low_q12;
    gba89_s16 eq_mid_q12;
    gba89_s16 eq_presence_q12;
    gba89_s16 eq_air_q12;
    gba89_s16 eq_ultra_q12;

    gba89_s16 chorus_mix_q15;
    gba89_u16 chorus_base_samples;
    gba89_u16 chorus_depth_samples;
    gba89_u16 chorus_rate_step;

    gba89_s16 reverb_mix_q15;
    gba89_s16 reverb_feedback_q15;
    gba89_u16 reverb_tail_ms;

    gba89_s16 crack_level_q15;
    gba89_u16 crack_delay_ms;
    gba89_u16 crack_samples;

    gba89_s16 pan_start_q15;
    gba89_s16 pan_end_q15;
} gba89_config;

typedef struct gba89_state_tag {
    gba89_config cfg;
    gba89_u32 rng;
    gba89_u32 age;
    gba89_u32 source_samples;
    gba89_u32 total_samples;
    gba89_u32 attack_samples;
    gba89_u32 decay_samples;
    gba89_u32 sustain_samples;
    gba89_u32 release_samples;
    gba89_u32 crack_at_samples;
    int active;

    gba89_s32 hp_memory;
    gba89_s32 lp_memory_1;
    gba89_s32 lp_memory_2;
    gba89_s32 body_lp_memory_1;
    gba89_s32 body_lp_memory_2;
    gba89_s32 flutter_memory;

    gba89_s32 eq_lp_1;
    gba89_s32 eq_lp_2;
    gba89_s32 eq_lp_3;
    gba89_s32 eq_lp_4;
    gba89_s32 eq_lp_5;

    gba89_s16 chorus_buffer[GBA89_CHORUS_SIZE];
    gba89_u16 chorus_index;
    gba89_u16 chorus_phase;

    gba89_s16 reverb_buffer[GBA89_REVERB_SIZE];
    gba89_u16 reverb_index;
} gba89_state;

void gba89_init(gba89_state *state, gba89_u32 seed);
const gba89_config *gba89_get_preset(int preset_index);
void gba89_trigger(gba89_state *state, const gba89_config *config);
void gba89_trigger_preset(gba89_state *state, int preset_index);
void gba89_stop(gba89_state *state);
int gba89_is_active(const gba89_state *state);
void gba89_render_stereo(gba89_state *state, gba89_s16 *interleaved, gba89_u32 frames);
void gba89_render_mono(gba89_state *state, gba89_s16 *mono, gba89_u32 frames);

#ifdef __cplusplus
}
#endif

#endif
