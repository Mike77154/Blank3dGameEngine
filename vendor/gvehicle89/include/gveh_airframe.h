#ifndef GVEH_AIRFRAME_H
#define GVEH_AIRFRAME_H

#include "gveh_body.h"
#include "gveh_ai.h"

#define GVEH_AIRFRAME_FLAG_ENABLED      1u
#define GVEH_AIRFRAME_FLAG_ARCADE       2u
#define GVEH_AIRFRAME_FLAG_STALL        4u
#define GVEH_AIRFRAME_FLAG_AFTERBURNER  8u

typedef struct gveh_airframe_cfg_s {
    gveh_u16 flags;
    gveh_fx thrust;
    gveh_fx lift_k;
    gveh_fx drag_k;
    gveh_fx side_drag_k;
    gveh_fx pitch_power;
    gveh_fx roll_power;
    gveh_fx yaw_power;
    gveh_fx min_fly_speed;
    gveh_fx stall_speed;
    gveh_fx arcade_lift;
    gveh_fx afterburner_thrust;
    gveh_fx g_limit;
} gveh_airframe_cfg;

typedef struct gveh_airframe_state_s {
    gveh_fx airspeed;
    gveh_fx aoa;
    gveh_fx lift;
    gveh_fx drag;
    gveh_fx throttle_lag;
    gveh_i16 stalled;
    gveh_i16 overspeed;
} gveh_airframe_state;

void gveh_airframe_default(gveh_airframe_cfg *cfg);
void gveh_airframe_ace(gveh_airframe_cfg *cfg);
void gveh_airframe_lightplane(gveh_airframe_cfg *cfg);
void gveh_airframe_clear(gveh_airframe_state *st);
void gveh_airframe_apply(const gveh_airframe_cfg *cfg, gveh_airframe_state *st, gveh_body *body, gveh_basis basis, const gveh_input *in, gveh_fx dt);

#endif
