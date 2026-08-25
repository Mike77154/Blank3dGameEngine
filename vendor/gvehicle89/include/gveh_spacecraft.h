#ifndef GVEH_SPACECRAFT_H
#define GVEH_SPACECRAFT_H

#include "gveh_body.h"
#include "gveh_ai.h"
#include "gveh_fx.h"

#define GVEH_SPACE_FLAG_ENABLED       1u
#define GVEH_SPACE_FLAG_ARCADE        2u
#define GVEH_SPACE_FLAG_SHIELDS       4u
#define GVEH_SPACE_FLAG_ENERGY        8u
#define GVEH_SPACE_FLAG_INERTIA_DAMP  16u
#define GVEH_SPACE_FLAG_SFOILS        32u

typedef struct gveh_spacecraft_cfg_s {
    gveh_u16 flags;
    gveh_fx thrust;
    gveh_fx boost_thrust;
    gveh_fx pitch_power;
    gveh_fx roll_power;
    gveh_fx yaw_power;
    gveh_fx damp_k;
    gveh_fx shield_max;
    gveh_fx shield_recharge;
    gveh_fx laser_max;
    gveh_fx laser_recharge;
    gveh_fx laser_cost;
    gveh_fx heat_decay;
} gveh_spacecraft_cfg;

typedef struct gveh_spacecraft_state_s {
    gveh_fx shield;
    gveh_fx laser;
    gveh_fx heat;
    gveh_fx energy_engine;
    gveh_fx energy_weapon;
    gveh_fx energy_shield;
    gveh_i16 overheated;
    gveh_i16 sfoils_open;
    gveh_i16 fired;
} gveh_spacecraft_state;

void gveh_spacecraft_default(gveh_spacecraft_cfg *cfg);
void gveh_spacecraft_xwing(gveh_spacecraft_cfg *cfg);
void gveh_spacecraft_tie(gveh_spacecraft_cfg *cfg);
void gveh_spacecraft_bomber(gveh_spacecraft_cfg *cfg);
void gveh_spacecraft_clear(gveh_spacecraft_state *st, const gveh_spacecraft_cfg *cfg);
void gveh_spacecraft_apply(gveh_spacecraft_state *st, const gveh_spacecraft_cfg *cfg, gveh_body *body, gveh_basis basis, const gveh_input *in, gveh_fx dt, gveh_fx_queue *fxq);

#endif
