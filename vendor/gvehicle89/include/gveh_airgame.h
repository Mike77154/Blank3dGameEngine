#ifndef GVEH_AIRGAME_H
#define GVEH_AIRGAME_H

#include "gveh_ai.h"
#include "gveh_fx.h"
#include "gveh_body.h"

#define GVEH_AIRGAME_FLAG_ENABLED      1u
#define GVEH_AIRGAME_FLAG_LOCKON       2u
#define GVEH_AIRGAME_FLAG_RESCUE       4u
#define GVEH_AIRGAME_FLAG_FUEL         8u
#define GVEH_AIRGAME_FLAG_ARCADE_AMMO  16u

typedef struct gveh_airgame_cfg_s {
    gveh_u16 flags;
    gveh_fx fuel_max;
    gveh_fx fuel_burn_idle;
    gveh_fx fuel_burn_power;
    gveh_i16 cannon_max;
    gveh_i16 missile_max;
    gveh_i16 rocket_max;
    gveh_i16 fire_cooldown;
    gveh_i16 missile_cooldown;
    gveh_i16 lock_time;
    gveh_fx lock_cone;
    gveh_fx winch_rate;
    gveh_fx armor_max;
} gveh_airgame_cfg;

typedef struct gveh_airgame_state_s {
    gveh_fx fuel;
    gveh_fx armor;
    gveh_i16 cannon;
    gveh_i16 missiles;
    gveh_i16 rockets;
    gveh_i16 fire_timer;
    gveh_i16 missile_timer;
    gveh_i16 lock_timer;
    gveh_i16 locked;
    gveh_i16 cargo_count;
    gveh_fx winch_len;
    gveh_i16 empty;
} gveh_airgame_state;

void gveh_airgame_default(gveh_airgame_cfg *cfg);
void gveh_airgame_arcade(gveh_airgame_cfg *cfg);
void gveh_airgame_strike(gveh_airgame_cfg *cfg);
void gveh_airgame_clear(gveh_airgame_state *st, const gveh_airgame_cfg *cfg);
void gveh_airgame_step(gveh_airgame_state *st, const gveh_airgame_cfg *cfg, const gveh_input *in, gveh_body *body, gveh_basis basis, gveh_fx engine_power, gveh_fx dt, gveh_fx_queue *fxq);

#endif
