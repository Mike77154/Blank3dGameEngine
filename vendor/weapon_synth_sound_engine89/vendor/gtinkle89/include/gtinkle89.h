#ifndef GTINKLE89_H
#define GTINKLE89_H

/*
    gtinkle89 - procedural shell-casing impact synthesizer
    C89, fixed-point, no heap, no floating-point types in the core.

    The core assumes:
      - 8-bit unsigned char
      - 16-bit short
      - 32-bit int
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GT89_MAX_VOICES
#define GT89_MAX_VOICES 12
#endif

#ifndef GT89_MAX_MODES
#define GT89_MAX_MODES 6
#endif

#ifndef GT89_MAX_FLOOR_MODES
#define GT89_MAX_FLOOR_MODES 2
#endif

#define GT89_PROFILE_MODES 6
#define GT89_PROFILE_FLOOR_MODES 2
#define GT89_RUNTIME_MODES (GT89_MAX_MODES + GT89_MAX_FLOOR_MODES)

#if GT89_MAX_VOICES < 1
#error GT89_MAX_VOICES must be at least 1
#endif

#if GT89_MAX_MODES < 1
#error GT89_MAX_MODES must be at least 1
#endif

#if GT89_MAX_FLOOR_MODES < 0
#error GT89_MAX_FLOOR_MODES cannot be negative
#endif

#ifndef GT89_ENABLE_FM
#define GT89_ENABLE_FM 1
#endif

#ifndef GT89_ENABLE_INTERNAL_BOUNCES
#define GT89_ENABLE_INTERNAL_BOUNCES 1
#endif

#define GT89_VERSION_MAJOR 1
#define GT89_VERSION_MINOR 3
#define GT89_VERSION_PATCH 0
#define GT89_PROFILE_ABI 2
#define GT89_BUILTIN_ROOT_HZ 7040U /* A8 at A4=440 Hz */

#define GT89_Q15_ONE 32767
#define GT89_PAN_LEFT   (-127)
#define GT89_PAN_CENTER 0
#define GT89_PAN_RIGHT  127
#define GT89_AUTO_U16   0
#define GT89_AUTO_U8    255

typedef signed char gt89_s8;
typedef unsigned char gt89_u8;
typedef signed short gt89_s16;
typedef unsigned short gt89_u16;
typedef signed int gt89_s32;
typedef unsigned int gt89_u32;

typedef enum gt89_shell_type {
    GT89_SHELL_RIMFIRE_BRASS = 0,
    GT89_SHELL_PISTOL_BRASS,
    GT89_SHELL_MAGNUM_BRASS,
    GT89_SHELL_RIFLE_BRASS,
    GT89_SHELL_STEEL_CASE,
    GT89_SHELL_SHOTGUN_BRASS,
    GT89_SHELL_SHOTGUN_PLASTIC,
    GT89_SHELL_COUNT
} gt89_shell_type;

typedef enum gt89_surface_type {
    GT89_SURFACE_CONCRETE = 0,
    GT89_SURFACE_TILE,
    GT89_SURFACE_WOOD,
    GT89_SURFACE_METAL,
    GT89_SURFACE_DIRT,
    GT89_SURFACE_GENERIC,
    GT89_SURFACE_COUNT
} gt89_surface_type;

typedef enum gt89_hit_region {
    GT89_HIT_RANDOM = 0,
    GT89_HIT_SIDE,
    GT89_HIT_MOUTH,
    GT89_HIT_BASE,
    GT89_HIT_RIM,
    GT89_HIT_REGION_COUNT
} gt89_hit_region;

typedef struct gt89_mode_profile {
    gt89_u16 hz;
    gt89_u16 amplitude_q15;
    gt89_u8 decay_shift;
    gt89_u8 reserved;
} gt89_mode_profile;

typedef struct gt89_shell_profile {
    gt89_mode_profile mode[GT89_PROFILE_MODES];
    gt89_u8 mode_count;
    gt89_u8 transient_decay_shift;
    gt89_u16 transient_amplitude_q15;
    gt89_u16 click_amplitude_q15;
    gt89_u16 fm_carrier_hz;
    gt89_u16 fm_modulator_hz;
    gt89_u16 fm_amplitude_q15;
    gt89_u16 fm_index_phase;
    gt89_u8 fm_amplitude_decay_shift;
    gt89_u8 fm_index_decay_shift;
    gt89_u16 nominal_duration_ms;
} gt89_shell_profile;

typedef struct gt89_surface_profile {
    gt89_u16 mode_gain_q15;
    gt89_u16 transient_gain_q15;
    gt89_mode_profile floor_mode[GT89_PROFILE_FLOOR_MODES];
    gt89_u16 bounce_gain_q15;
    gt89_u16 bounce_time_q15;
    gt89_u8 max_bounces;
    gt89_u8 hardness;
    gt89_s8 decay_shift_delta;
    gt89_u8 reserved;
} gt89_surface_profile;

typedef struct gt89_impact_params {
    gt89_u8 velocity;          /* normal impact velocity/impulse: 1..255 */
    gt89_u8 angular_velocity;  /* spin/tumble amount: 0..255 */
    gt89_u8 bounce_count;      /* 0 = one contact; 255 = surface automatic */
    gt89_u8 variation;
    gt89_s8 pan;
    gt89_u8 hit_region;        /* gt89_hit_region */
    gt89_u16 first_bounce_ms;  /* 0 = automatic */
    gt89_u16 bounce_gain_q15;  /* 0 = surface automatic */
    gt89_u16 bounce_time_q15;  /* 0 = surface automatic */
} gt89_impact_params;

typedef struct gt89_runtime_mode {
    gt89_u16 phase;
    gt89_u16 increment;
    gt89_u16 envelope_q15;
    gt89_u16 amplitude_q15;
    gt89_u8 decay_shift;
    gt89_u8 reserved;
} gt89_runtime_mode;

typedef struct gt89_voice {
    gt89_runtime_mode mode[GT89_RUNTIME_MODES];
    gt89_u16 fm_carrier_phase;
    gt89_u16 fm_carrier_increment;
    gt89_u16 fm_modulator_phase;
    gt89_u16 fm_modulator_increment;
    gt89_u16 fm_envelope_q15;
    gt89_u16 fm_index_envelope_q15;
    gt89_u16 fm_amplitude_q15;
    gt89_u16 fm_index_phase;
    gt89_u16 transient_envelope_q15;
    gt89_u16 transient_amplitude_q15;
    gt89_u16 click_amplitude_q15;
    gt89_u16 noise_previous;
    gt89_u16 age_samples_low;
    gt89_u16 age_samples_high;
    gt89_u16 duration_samples_low;
    gt89_u16 duration_samples_high;
    gt89_u16 bounce_countdown_low;
    gt89_u16 bounce_countdown_high;
    gt89_u16 bounce_interval_low;
    gt89_u16 bounce_interval_high;
    gt89_u16 bounce_gain_q15;
    gt89_u16 bounce_time_q15;
    gt89_u16 rng;
    gt89_s8 pan;
    gt89_u8 mode_count;
    gt89_u8 fm_amplitude_decay_shift;
    gt89_u8 fm_index_decay_shift;
    gt89_u8 transient_decay_shift;
    gt89_u8 click_samples_left;
    gt89_u8 bounce_remaining;
    gt89_u8 angular_velocity;
    gt89_u8 hardness;
    gt89_u8 hit_region;
    gt89_u8 active;
} gt89_voice;

typedef struct gt89_context {
    gt89_voice voice[GT89_MAX_VOICES];
    gt89_u32 sample_rate;
    gt89_u32 trigger_counter;
    gt89_u16 master_gain_q15;
    gt89_u16 seed;
    gt89_u8 initialized;
    gt89_u8 reserved;
} gt89_context;

/* Returns 1 on success, 0 if the integer model is unsupported. */
int gt89_init(gt89_context *ctx, gt89_u32 sample_rate, gt89_u16 seed);
void gt89_reset(gt89_context *ctx);
void gt89_set_master_gain(gt89_context *ctx, gt89_u16 gain_q15);
void gt89_default_impact_params(gt89_impact_params *params);

/* Backward-compatible one-contact trigger. */
int gt89_trigger(gt89_context *ctx,
                 gt89_shell_type shell,
                 gt89_surface_type surface,
                 gt89_u8 velocity,
                 gt89_s8 pan,
                 gt89_u8 variation);

/* Physics-aware event: region, spin and optional internal bounce train. */
int gt89_trigger_ex(gt89_context *ctx,
                    gt89_shell_type shell,
                    gt89_surface_type surface,
                    const gt89_impact_params *params);

/* Convenience preset for an ejected/falling casing that bounces and settles. */
int gt89_trigger_drop(gt89_context *ctx,
                      gt89_shell_type shell,
                      gt89_surface_type surface,
                      gt89_u8 velocity,
                      gt89_u8 angular_velocity,
                      gt89_s8 pan,
                      gt89_u8 variation);

/* Legacy one-contact custom trigger retained for source compatibility. */
int gt89_trigger_custom(gt89_context *ctx,
                        const gt89_shell_profile *shell,
                        const gt89_surface_profile *surface,
                        gt89_u8 velocity,
                        gt89_s8 pan,
                        gt89_u8 variation);

int gt89_trigger_custom_ex(gt89_context *ctx,
                           const gt89_shell_profile *shell,
                           const gt89_surface_profile *surface,
                           const gt89_impact_params *params);

void gt89_render_mono_i16(gt89_context *ctx, gt89_s16 *output, gt89_u32 frames);
void gt89_render_stereo_i16(gt89_context *ctx, gt89_s16 *output_interleaved, gt89_u32 frames);
void gt89_mix_mono_i16(gt89_context *ctx, gt89_s16 *output, gt89_u32 frames);
void gt89_mix_stereo_i16(gt89_context *ctx, gt89_s16 *output_interleaved, gt89_u32 frames);

const gt89_shell_profile *gt89_get_shell_profile(gt89_shell_type shell);
const gt89_surface_profile *gt89_get_surface_profile(gt89_surface_type surface);
const char *gt89_shell_name(gt89_shell_type shell);
const char *gt89_surface_name(gt89_surface_type surface);
const char *gt89_hit_region_name(gt89_hit_region region);

gt89_u32 gt89_active_voice_count(const gt89_context *ctx);
gt89_u32 gt89_context_bytes(void);

#ifdef __cplusplus
}
#endif

#endif /* GTINKLE89_H */
