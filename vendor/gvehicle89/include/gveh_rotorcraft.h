#ifndef GVEH_ROTORCRAFT_H
#define GVEH_ROTORCRAFT_H

#include "gveh_body.h"
#include "gveh_ai.h"

#define GVEH_ROTOR_FLAG_ENABLED       1u
#define GVEH_ROTOR_FLAG_ARCADE        2u
#define GVEH_ROTOR_FLAG_STRIKE        4u
#define GVEH_ROTOR_FLAG_ALT_HOLD      8u

typedef struct gveh_rotorcraft_cfg_s {
    gveh_u16 flags;
    gveh_fx main_lift;
    gveh_fx hover_collective;
    gveh_fx rotor_ramp;
    gveh_fx translational_lift;
    gveh_fx cyclic_force;
    gveh_fx pitch_power;
    gveh_fx roll_power;
    gveh_fx yaw_power;
    gveh_fx body_drag;
    gveh_fx altitude_hold_k;
    gveh_fx target_altitude;
} gveh_rotorcraft_cfg;

typedef struct gveh_rotorcraft_state_s {
    gveh_fx rotor_rpm;
    gveh_fx lift;
    gveh_fx forward_flow;
    gveh_fx altitude_error;
    gveh_i16 low_rotor;
} gveh_rotorcraft_state;

void gveh_rotorcraft_default(gveh_rotorcraft_cfg *cfg);
void gveh_rotorcraft_thunder(gveh_rotorcraft_cfg *cfg);
void gveh_rotorcraft_strike(gveh_rotorcraft_cfg *cfg);
void gveh_rotorcraft_sim(gveh_rotorcraft_cfg *cfg);
void gveh_rotorcraft_clear(gveh_rotorcraft_state *st);
void gveh_rotorcraft_apply(const gveh_rotorcraft_cfg *cfg, gveh_rotorcraft_state *st, gveh_body *body, gveh_basis basis, const gveh_input *in, gveh_fx dt);

#endif
