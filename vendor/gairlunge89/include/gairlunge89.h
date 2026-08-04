#ifndef GAIRLUNGE89_H
#define GAIRLUNGE89_H

/*
 * gairlunge89.h
 * Mid-air committed lunge / dive assault controller.
 * C89, fixed-point, no dynamic allocation.
 * CC0-1.0 / public domain dedication.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GAL_FX_TYPE
#define GAL_FX_TYPE signed long
#endif

typedef GAL_FX_TYPE gal_fx;

#define GAL_FX_SHIFT 16
#define GAL_FX_ONE   ((gal_fx)65536L)
#define GAL_FX_HALF  ((gal_fx)32768L)
#define GAL_FX_FROM_INT(v) ((gal_fx)((v) * 65536L))
#define GAL_FX_TO_INT(v)   ((long)((v) / 65536L))

typedef struct gal_vec3 {
    gal_fx x;
    gal_fx y;
    gal_fx z;
} gal_vec3;

typedef enum gal_state {
    GAL_STATE_IDLE = 0,
    GAL_STATE_ACTIVE,
    GAL_STATE_RECOVERY,
    GAL_STATE_DONE,
    GAL_STATE_CANCELLED
} gal_state;

typedef enum gal_intent {
    GAL_INTENT_PUSH = 0,
    GAL_INTENT_RAM,
    GAL_INTENT_CLAW,
    GAL_INTENT_PIERCE
} gal_intent;

typedef enum gal_target_policy {
    GAL_TARGET_SNAPSHOT = 0,
    GAL_TARGET_TRACK_EACH_STEP
} gal_target_policy;

typedef struct gal_config {
    gal_fx launch_speed;
    gal_fx acceleration;
    gal_fx max_speed;
    gal_fx steering;             /* 0..GAL_FX_ONE. */
    gal_fx arrival_radius;
    gal_fx impact_impulse;
    gal_fx velocity_keep;        /* 0..GAL_FX_ONE. Preserves old velocity. */
    unsigned long max_active_ticks;
    unsigned long recovery_ticks;
    unsigned int pierce_contacts;
    unsigned char require_airborne;
    unsigned char finish_on_ground;
    unsigned char cancel_on_target_loss;
    gal_target_policy target_policy;
    gal_intent intent;
} gal_config;

typedef struct gal_input {
    gal_vec3 position;
    gal_vec3 velocity;
    gal_vec3 target_position;
    unsigned char target_valid;
    unsigned char airborne;
    unsigned char trigger;
    unsigned char cancel;
    unsigned char contact;
    gal_vec3 contact_normal;
} gal_input;

typedef struct gal_output {
    gal_vec3 desired_velocity;
    gal_vec3 facing_direction;
    gal_vec3 impact_direction;
    gal_fx impact_impulse;
    gal_intent intent;
    gal_state state;
    unsigned char write_velocity;
    unsigned char attack_active;
    unsigned char impact_event;
    unsigned char finished;
} gal_output;

typedef struct gal_controller {
    gal_config cfg;
    gal_state state;
    gal_vec3 target_snapshot;
    gal_vec3 last_direction;
    gal_fx current_speed;
    unsigned long state_ticks;
    unsigned int contact_count;
    unsigned char has_target;
} gal_controller;

/* Optional provider adapter. */
typedef int  (*gal_read_motion_fn)(void *user, gal_vec3 *position, gal_vec3 *velocity, int *airborne);
typedef int  (*gal_read_target_fn)(void *user, gal_vec3 *target_position);
typedef void (*gal_write_velocity_fn)(void *user, const gal_vec3 *velocity, const gal_vec3 *facing);
typedef int  (*gal_read_contact_fn)(void *user, gal_vec3 *normal);
typedef void (*gal_impact_fn)(void *user, gal_intent intent, const gal_vec3 *direction, gal_fx impulse);

typedef struct gal_provider {
    void *user;
    gal_read_motion_fn read_motion;
    gal_read_target_fn read_target;
    gal_write_velocity_fn write_velocity;
    gal_read_contact_fn read_contact;
    gal_impact_fn on_impact;
} gal_provider;

void gal_default_config(gal_config *cfg);
void gal_init(gal_controller *ctl, const gal_config *cfg);
void gal_reset(gal_controller *ctl);
void gal_step(gal_controller *ctl, const gal_input *in, gal_output *out);
int gal_step_provider(gal_controller *ctl, const gal_provider *provider, int trigger, int cancel, gal_output *out);

/* Fixed-point helpers exposed for integration. */
gal_fx gal_fx_mul(gal_fx a, gal_fx b);
gal_fx gal_fx_div(gal_fx a, gal_fx b);
gal_fx gal_vec3_length_approx(const gal_vec3 *v);
void gal_vec3_normalize_approx(const gal_vec3 *v, gal_vec3 *out);

#ifdef __cplusplus
}
#endif

#endif
