#ifndef GVEH_SKILL_H
#define GVEH_SKILL_H

#include "gveh_ai.h"

#define GVEH_SKILL_FLAG_HELP_STEER   1u
#define GVEH_SKILL_FLAG_HELP_GRIP    2u
#define GVEH_SKILL_FLAG_HELP_EJECT   4u

typedef struct gveh_skill_cfg_s {
    gveh_fx max_skill;
    gveh_fx gain_drive;
    gveh_fx gain_drift;
    gveh_fx gain_air;
    gveh_fx steer_help;
    gveh_fx throttle_smooth;
    gveh_u16 flags;
} gveh_skill_cfg;

typedef struct gveh_skill_state_s {
    gveh_fx driver;
    gveh_fx drift;
    gveh_fx stunt;
    gveh_input prev_input;
} gveh_skill_state;

void gveh_skill_default(gveh_skill_cfg *cfg);
void gveh_skill_clear(gveh_skill_state *st);
void gveh_skill_apply_input(gveh_skill_state *st, const gveh_skill_cfg *cfg, const gveh_input *in, gveh_input *out_input, gveh_fx speed_abs);
void gveh_skill_step(gveh_skill_state *st, const gveh_skill_cfg *cfg, gveh_fx speed_abs, gveh_fx side_abs, gveh_fx grounded_ratio, gveh_fx dt);

#endif
