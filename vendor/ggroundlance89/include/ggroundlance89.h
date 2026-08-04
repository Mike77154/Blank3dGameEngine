#ifndef GGROUNDLANCE89_H
#define GGROUNDLANCE89_H

/*
 * ggroundlance89.h
 * Ground-hugging continuous charge / lance traversal controller.
 * C89, fixed-point, no dynamic allocation.
 * CC0-1.0 / public domain dedication.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GGL_FX_TYPE
#define GGL_FX_TYPE signed long
#endif

typedef GGL_FX_TYPE ggl_fx;

#define GGL_FX_SHIFT 16
#define GGL_FX_ONE   ((ggl_fx)65536L)
#define GGL_FX_HALF  ((ggl_fx)32768L)
#define GGL_FX_FROM_INT(v) ((ggl_fx)((v) * 65536L))
#define GGL_FX_TO_INT(v)   ((long)((v) / 65536L))

typedef struct ggl_vec3 {
    ggl_fx x;
    ggl_fx y;
    ggl_fx z;
} ggl_vec3;

typedef enum ggl_state {
    GGL_STATE_IDLE = 0,
    GGL_STATE_CHARGING,
    GGL_STATE_RECOVERY,
    GGL_STATE_DONE,
    GGL_STATE_CANCELLED
} ggl_state;

typedef enum ggl_style {
    GGL_STYLE_LANCE = 0,
    GGL_STYLE_PEGASUS,
    GGL_STYLE_RAM,
    GGL_STYLE_SKIM_HOP
} ggl_style;

typedef enum ggl_contact_policy {
    GGL_CONTACT_STOP = 0,
    GGL_CONTACT_BOUNCE,
    GGL_CONTACT_PIERCE
} ggl_contact_policy;

typedef struct ggl_config {
    ggl_fx speed;
    ggl_fx acceleration;
    ggl_fx max_speed;
    ggl_fx arrival_radius;
    ggl_fx ground_clearance;
    ggl_fx max_step_up;
    ggl_fx max_step_down;
    ggl_fx hop_height;
    unsigned long hop_period_ticks;
    ggl_fx impact_impulse;
    unsigned long max_charge_ticks;
    unsigned long recovery_ticks;
    unsigned int pierce_contacts;
    unsigned char track_target;
    unsigned char require_ground;
    unsigned char snap_to_ground;
    ggl_style style;
    ggl_contact_policy contact_policy;
} ggl_config;

typedef struct ggl_input {
    ggl_vec3 position;
    ggl_vec3 velocity;
    ggl_vec3 target_position;
    ggl_fx ground_height;
    ggl_vec3 ground_normal;
    unsigned char target_valid;
    unsigned char ground_valid;
    unsigned char trigger;
    unsigned char cancel;
    unsigned char contact;
    ggl_vec3 contact_normal;
} ggl_input;

typedef struct ggl_output {
    ggl_vec3 desired_position;
    ggl_vec3 desired_velocity;
    ggl_vec3 facing_direction;
    ggl_vec3 impact_direction;
    ggl_fx impact_impulse;
    ggl_style style;
    ggl_state state;
    unsigned char write_position;
    unsigned char write_velocity;
    unsigned char attack_active;
    unsigned char impact_event;
    unsigned char finished;
} ggl_output;

typedef struct ggl_controller {
    ggl_config cfg;
    ggl_state state;
    ggl_vec3 start_position;
    ggl_vec3 target_snapshot;
    ggl_vec3 last_direction;
    ggl_fx current_speed;
    ggl_fx last_ground_height;
    unsigned long state_ticks;
    unsigned long charge_ticks;
    unsigned int contact_count;
    unsigned char has_target;
    unsigned char has_ground;
} ggl_controller;

/* Optional provider adapter. */
typedef int  (*ggl_read_motion_fn)(void *user, ggl_vec3 *position, ggl_vec3 *velocity);
typedef int  (*ggl_read_target_fn)(void *user, ggl_vec3 *target_position);
typedef int  (*ggl_query_ground_fn)(void *user, ggl_fx x, ggl_fx z, ggl_fx *height, ggl_vec3 *normal);
typedef int  (*ggl_read_contact_fn)(void *user, ggl_vec3 *normal);
typedef void (*ggl_write_motion_fn)(void *user, const ggl_vec3 *position, const ggl_vec3 *velocity, const ggl_vec3 *facing);
typedef void (*ggl_impact_fn)(void *user, ggl_style style, const ggl_vec3 *direction, ggl_fx impulse);

typedef struct ggl_provider {
    void *user;
    ggl_read_motion_fn read_motion;
    ggl_read_target_fn read_target;
    ggl_query_ground_fn query_ground;
    ggl_read_contact_fn read_contact;
    ggl_write_motion_fn write_motion;
    ggl_impact_fn on_impact;
} ggl_provider;

void ggl_default_config(ggl_config *cfg);
void ggl_init(ggl_controller *ctl, const ggl_config *cfg);
void ggl_reset(ggl_controller *ctl);
void ggl_step(ggl_controller *ctl, const ggl_input *in, ggl_output *out);
int ggl_step_provider(ggl_controller *ctl, const ggl_provider *provider, int trigger, int cancel, ggl_output *out);

ggl_fx ggl_fx_mul(ggl_fx a, ggl_fx b);
ggl_fx ggl_fx_div(ggl_fx a, ggl_fx b);
ggl_fx ggl_vec3_length_approx(const ggl_vec3 *v);
void ggl_vec3_normalize_approx(const ggl_vec3 *v, ggl_vec3 *out);

#ifdef __cplusplus
}
#endif

#endif
