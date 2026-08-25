#ifndef GVEH_WINGMAN_H
#define GVEH_WINGMAN_H

#include "gveh_ai.h"

#define GVEH_WINGMAN_MAX 4
#define GVEH_WINGMAN_FLAG_ENABLED 1u
#define GVEH_WINGMAN_CMD_FORM    0
#define GVEH_WINGMAN_CMD_ATTACK  1
#define GVEH_WINGMAN_CMD_COVER   2
#define GVEH_WINGMAN_CMD_HOLD    3
#define GVEH_WINGMAN_CMD_RESCUE  4

typedef struct gveh_wingman_cfg_s {
    gveh_u16 flags;
    gveh_i16 count;
    gveh_i16 order_delay;
    gveh_fx attack_skill;
    gveh_fx cover_skill;
    gveh_fx morale_k;
} gveh_wingman_cfg;

typedef struct gveh_wingman_unit_s {
    gveh_i16 hp;
    gveh_i16 ammo;
    gveh_i16 command;
    gveh_i16 cooldown;
    gveh_i16 morale;
} gveh_wingman_unit;

typedef struct gveh_wingman_state_s {
    gveh_wingman_unit unit[GVEH_WINGMAN_MAX];
    gveh_i16 active_count;
    gveh_i16 squad_score;
    gveh_i16 last_ack;
} gveh_wingman_state;

void gveh_wingman_default(gveh_wingman_cfg *cfg);
void gveh_wingman_arcade(gveh_wingman_cfg *cfg);
void gveh_wingman_sim(gveh_wingman_cfg *cfg);
void gveh_wingman_clear(gveh_wingman_state *st, const gveh_wingman_cfg *cfg);
void gveh_wingman_step(gveh_wingman_state *st, const gveh_wingman_cfg *cfg, const gveh_input *in, gveh_i16 locked, gveh_i16 threat);

#endif
