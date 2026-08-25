#ifndef GVEH_AIRASSIST_H
#define GVEH_AIRASSIST_H

#include "gveh_body.h"
#include "gveh_ai.h"
#include "gveh_airframe.h"
#include "gveh_rotorcraft.h"

#define GVEH_AIRASSIST_FLAG_ENABLED      1u
#define GVEH_AIRASSIST_FLAG_AUTOLEVEL    2u
#define GVEH_AIRASSIST_FLAG_STALL_GUARD  4u
#define GVEH_AIRASSIST_FLAG_LIMIT_BANK   8u
#define GVEH_AIRASSIST_FLAG_ARCADE_CAM   16u

typedef struct gveh_airassist_cfg_s {
    gveh_u16 flags;
    gveh_fx autolevel_pitch;
    gveh_fx autolevel_roll;
    gveh_fx stall_push;
    gveh_fx max_pitch_rate;
    gveh_fx max_roll_rate;
    gveh_fx max_yaw_rate;
    gveh_i16 max_bank256;
} gveh_airassist_cfg;

void gveh_airassist_default(gveh_airassist_cfg *cfg);
void gveh_airassist_arcade(gveh_airassist_cfg *cfg);
void gveh_airassist_sim(gveh_airassist_cfg *cfg);
void gveh_airassist_apply(const gveh_airassist_cfg *cfg, gveh_body *body, const gveh_airframe_state *air, const gveh_rotorcraft_state *rotor, const gveh_input *in);

#endif
