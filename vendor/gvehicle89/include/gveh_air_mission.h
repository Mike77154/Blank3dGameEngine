#ifndef GVEH_AIR_MISSION_H
#define GVEH_AIR_MISSION_H

#include "gveh_ai.h"

#define GVEH_AIRMISSION_FLAG_ENABLED    1u
#define GVEH_AIRMISSION_FLAG_MEDALS     2u
#define GVEH_AIRMISSION_FLAG_DYNAMIC    4u

#define GVEH_AIRMISSION_NONE     0
#define GVEH_AIRMISSION_BRONZE   1
#define GVEH_AIRMISSION_SILVER   2
#define GVEH_AIRMISSION_GOLD     3

typedef struct gveh_air_mission_cfg_s {
    gveh_u16 flags;
    gveh_i16 destroy_need;
    gveh_i16 rescue_need;
    gveh_i16 recon_need;
    gveh_i16 protect_need;
    gveh_i16 bronze_score;
    gveh_i16 silver_score;
    gveh_i16 gold_score;
} gveh_air_mission_cfg;

typedef struct gveh_air_mission_state_s {
    gveh_i16 destroyed;
    gveh_i16 rescued;
    gveh_i16 reconned;
    gveh_i16 protected_count;
    gveh_i16 score;
    gveh_i16 medal;
    gveh_i16 complete;
    gveh_i16 campaign_push;
} gveh_air_mission_state;

void gveh_air_mission_default(gveh_air_mission_cfg *cfg);
void gveh_air_mission_strike(gveh_air_mission_cfg *cfg);
void gveh_air_mission_ace(gveh_air_mission_cfg *cfg);
void gveh_air_mission_clear(gveh_air_mission_state *st);
void gveh_air_mission_step(gveh_air_mission_state *st, const gveh_air_mission_cfg *cfg, const gveh_input *in, gveh_i16 locked, gveh_i16 fired, gveh_i16 cargo, gveh_i16 takedown);

#endif
