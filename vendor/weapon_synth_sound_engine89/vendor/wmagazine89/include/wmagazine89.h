#ifndef WMAGAZINE89_H
#define WMAGAZINE89_H

/*
    wmagazine89 - procedural weapon magazine handling synthesis
    Strict C89 core, fixed point only, caller-owned state, no heap.

    The core deliberately includes no system headers.
    Assumptions checked by typedefs below:
      - unsigned int is at least 32 bits
      - signed int is at least 32 bits
      - signed short is at least 16 bits
*/

typedef char wmag89_require_int32[(sizeof(int) >= 4) ? 1 : -1];
typedef char wmag89_require_uint32[(sizeof(unsigned int) >= 4) ? 1 : -1];
typedef char wmag89_require_short16[(sizeof(short) >= 2) ? 1 : -1];

#define WMAG89_Q15_ONE              32767
#define WMAG89_EQ_BANDS             6
#define WMAG89_MODES                4
#define WMAG89_CHORUS_SAMPLES       512
#define WMAG89_REVERB_COMB1         1103
#define WMAG89_REVERB_COMB2         1277
#define WMAG89_REVERB_COMB3         1423
#define WMAG89_REVERB_ALLPASS       331
#define WMAG89_MAX_EVENTS           12

#define WMAG89_PRESET_PISTOL_METAL       0
#define WMAG89_PRESET_PISTOL_POLYMER     1
#define WMAG89_PRESET_SMG_STEEL          2
#define WMAG89_PRESET_RIFLE_ALUMINUM     3
#define WMAG89_PRESET_RIFLE_POLYMER      4
#define WMAG89_PRESET_SNIPER_BOX         5
#define WMAG89_PRESET_DRUM_HEAVY         6
#define WMAG89_PRESET_COUNT              7

#define WMAG89_ACTION_INSERT             0
#define WMAG89_ACTION_REMOVE             1
#define WMAG89_ACTION_SEAT_TAP           2
#define WMAG89_ACTION_TUG_CHECK          3
#define WMAG89_ACTION_RATTLE             4
#define WMAG89_ACTION_COUNT              5

#define WMAG89_EVENT_BURST               0
#define WMAG89_EVENT_SCRAPE              1
#define WMAG89_EVENT_CATCH               2
#define WMAG89_EVENT_SPRING              3
#define WMAG89_EVENT_THUMP               4

#define WMAG89_OK                        0
#define WMAG89_ERR_NULL                 -1
#define WMAG89_ERR_RANGE                -2

/* All user-facing values are integers or Q15 fixed point. */
typedef struct WMag89Event {
    int start_ms;
    int duration_ms;
    int gain_q15;
    int type;
    int color;
} WMag89Event;

typedef struct WMag89Mode {
    unsigned int phase;
    unsigned int increment;
    int amplitude_q15;
    int decay_q15;
    int gain_q15;
} WMag89Mode;

typedef struct WMag89State {
    int sample_rate;
    unsigned int rng;

    int preset;
    int action;
    int active;
    int velocity_q15;
    int sample_cursor;
    int event_count;
    unsigned int fired_mask;
    WMag89Event events[WMAG89_MAX_EVENTS];

    int time_scale_q15;
    int source_low_gain_q15;
    int source_mid_gain_q15;
    int source_high_gain_q15;
    int roughness_q15;
    int body_gain_q15;
    int spring_gain_q15;
    int tail_gain_q15;

    int source_lp_low;
    int source_lp_mid;
    int source_lp_high;
    int source_lp_air;
    int source_alpha_low;
    int source_alpha_mid;
    int source_alpha_high;
    int source_alpha_air;
    int rough_state;

    WMag89Mode modes[WMAG89_MODES];

    int eq_gain_q15[WMAG89_EQ_BANDS];
    int eq_lp[5];
    int eq_alpha[5];

    int distortion_drive_q8;
    int distortion_mix_q15;

    short chorus_buffer[WMAG89_CHORUS_SAMPLES];
    int chorus_write;
    unsigned int chorus_phase;
    int chorus_base_samples;
    int chorus_depth_samples;
    int chorus_mix_q15;

    short reverb_comb1[WMAG89_REVERB_COMB1];
    short reverb_comb2[WMAG89_REVERB_COMB2];
    short reverb_comb3[WMAG89_REVERB_COMB3];
    short reverb_allpass[WMAG89_REVERB_ALLPASS];
    int reverb_pos1;
    int reverb_pos2;
    int reverb_pos3;
    int reverb_pos_ap;
    int reverb_feedback_q15;
    int reverb_damping_q15;
    int reverb_damp1;
    int reverb_damp2;
    int reverb_damp3;
    int reverb_mix_q15;

    int output_gain_q15;
    int source_finished_sample;
} WMag89State;

/* Initialization and reset. State memory must be supplied by the caller. */
int wmag89_init(WMag89State *state, int sample_rate, unsigned int seed);
void wmag89_reset(WMag89State *state);

/* Preset and effect control. */
int wmag89_set_preset(WMag89State *state, int preset);
int wmag89_set_eq_gain(WMag89State *state, int band, int gain_q15);
void wmag89_set_distortion(WMag89State *state, int drive_q8, int mix_q15);
void wmag89_set_chorus(WMag89State *state, int base_ms, int depth_ms, int mix_q15);
void wmag89_set_reverb(WMag89State *state, int feedback_q15,
                       int damping_q15, int mix_q15);
void wmag89_set_output_gain(WMag89State *state, int gain_q15);

/* One-shot control and rendering. */
int wmag89_trigger(WMag89State *state, int action, int velocity_q15);
int wmag89_trigger_custom(WMag89State *state, const WMag89Event *events,
                          int event_count, int velocity_q15);
int wmag89_process_sample(WMag89State *state);
void wmag89_process_block(WMag89State *state, short *output, int frames);
int wmag89_is_active(const WMag89State *state);

/* Lightweight metadata. */
const char *wmag89_preset_name(int preset);
const char *wmag89_action_name(int action);

#endif
