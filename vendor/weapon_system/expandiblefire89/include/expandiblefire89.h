#ifndef EXPANDIBLEFIRE89_H
#define EXPANDIBLEFIRE89_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * expandiblefire89
 * ----------------
 * Renderer/engine-agnostic expanding projectile behaviour.
 *
 * Numeric protocol: signed Q20.12 in long values (4096 == 1.0).
 * Storage is caller-owned. No heap, no FP scalar types, no platform API.
 */
typedef long ef89_fx;

typedef struct ef89_vec3_s {
    ef89_fx x;
    ef89_fx y;
    ef89_fx z;
} ef89_vec3;

#define EF89_ONE 4096L

typedef struct ef89_config_s {
    ef89_fx scale_start_fx;
    ef89_fx scale_end_fx;
    ef89_fx growth_distance_fx;
    ef89_fx kill_distance_fx;
} ef89_config;

typedef struct ef89_state_s {
    int active;
    ef89_vec3 spawn_position;
    ef89_vec3 previous_position;
    ef89_fx distance_travelled_fx;
    ef89_fx scale_fx;
} ef89_state;

typedef struct ef89_result_s {
    int alive;
    ef89_fx distance_travelled_fx;
    ef89_fx progress_fx;
    ef89_fx scale_fx;
} ef89_result;

void expandiblefire89_config_default(ef89_config *config);
void expandiblefire89_init(ef89_state *state, ef89_vec3 spawn_position,
                           const ef89_config *config);
int expandiblefire89_step(ef89_state *state, const ef89_config *config,
                          ef89_vec3 current_position, ef89_result *result);
ef89_fx expandiblefire89_apply_scale(ef89_fx base_value,
                                     ef89_fx scale_fx);

#ifdef __cplusplus
}
#endif

#endif
