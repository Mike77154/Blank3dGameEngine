#ifndef GVEH_AIR_DAMAGE_H
#define GVEH_AIR_DAMAGE_H

#include "gveh_body.h"
#include "gveh_airgame.h"

#define GVEH_AIRDAMAGE_FLAG_ENABLED     1u
#define GVEH_AIRDAMAGE_FLAG_ARCADE      2u
#define GVEH_AIRDAMAGE_FLAG_AUTOROTATE  4u
#define GVEH_AIRDAMAGE_FLAG_FUEL_LEAK   8u

#define GVEH_AIRDAMAGE_ENGINE 0
#define GVEH_AIRDAMAGE_ROTOR  1
#define GVEH_AIRDAMAGE_WING_L 2
#define GVEH_AIRDAMAGE_WING_R 3
#define GVEH_AIRDAMAGE_TAIL   4
#define GVEH_AIRDAMAGE_FUEL   5
#define GVEH_AIRDAMAGE_COUNT  6

typedef struct gveh_air_damage_cfg_s {
    gveh_u16 flags;
    gveh_fx part_hp[GVEH_AIRDAMAGE_COUNT];
    gveh_fx impact_threshold;
    gveh_fx leak_rate;
    gveh_fx fire_rate;
    gveh_fx control_loss_k;
    gveh_fx autorotate_collective;
} gveh_air_damage_cfg;

typedef struct gveh_air_damage_state_s {
    gveh_fx part_hp[GVEH_AIRDAMAGE_COUNT];
    gveh_fx fuel_leak;
    gveh_fx fire;
    gveh_fx control_loss;
    gveh_fx last_damage;
    gveh_i16 engine_dead;
    gveh_i16 rotor_bad;
    gveh_i16 tail_bad;
    gveh_i16 autorotate;
} gveh_air_damage_state;

void gveh_air_damage_default(gveh_air_damage_cfg *cfg);
void gveh_air_damage_arcade(gveh_air_damage_cfg *cfg);
void gveh_air_damage_sim(gveh_air_damage_cfg *cfg);
void gveh_air_damage_clear(gveh_air_damage_state *st, const gveh_air_damage_cfg *cfg);
void gveh_air_damage_step(gveh_air_damage_state *st, const gveh_air_damage_cfg *cfg, gveh_body *body, gveh_airgame_state *airgame, gveh_fx impact, gveh_fx hazard, gveh_fx dt);

#endif
