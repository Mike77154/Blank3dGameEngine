#ifndef GTRIGGER89_H
#define GTRIGGER89_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum gtrigger89_model_e {
    GTRIGGER89_MODEL_NORMAL = 0,
    GTRIGGER89_MODEL_SPINUP = 1,
    GTRIGGER89_MODEL_CHARGE_RELEASE = 2,
    GTRIGGER89_MODEL_PRESS_CHARGE_RELEASE = 3
} gtrigger89_model;

typedef struct gtrigger89_config_s {
    gtrigger89_model model;
    unsigned int spinup_ms;
    unsigned int charge_max_ms;
} gtrigger89_config;

typedef struct gtrigger89_state_s {
    int initialized;
    int previous_raw_down;
    gtrigger89_model active_model;
    unsigned int elapsed_ms;
    int armed;
    int charging;
} gtrigger89_state;

typedef struct gtrigger89_output_s {
    int trigger_down;
    int trigger_pressed;
    int trigger_released;
    int spin_begin;
    int spin_end;
    int charge_begin;
    int charge_release;
    unsigned int charge_elapsed_ms;
    long charge_ratio_q16;
} gtrigger89_output;

void gtrigger89_init(gtrigger89_state *state);
void gtrigger89_reset(gtrigger89_state *state);
void gtrigger89_update(gtrigger89_state *state,
                       const gtrigger89_config *config,
                       int raw_down,
                       unsigned int dt_ms,
                       gtrigger89_output *out);

#ifdef __cplusplus
}
#endif

#endif
