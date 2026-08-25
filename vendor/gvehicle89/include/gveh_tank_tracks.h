#ifndef GVEH_TANK_TRACKS_H
#define GVEH_TANK_TRACKS_H

#include "gveh_body.h"
#include "gveh_ai.h"

typedef struct gveh_tank_tracks_cfg_s {
    gveh_fx drive_force;
    gveh_fx brake_force;
    gveh_fx pivot_torque;
    gveh_fx yaw_torque;
    gveh_fx mud_drag;
    gveh_fx track_grip;
    gveh_fx track_health_max;
    gveh_u16 flags;
} gveh_tank_tracks_cfg;

typedef struct gveh_tank_tracks_state_s {
    gveh_fx left_power;
    gveh_fx right_power;
    gveh_fx left_health;
    gveh_fx right_health;
    gveh_fx slip;
    gveh_fx squeal;
    gveh_i16 pivoting;
} gveh_tank_tracks_state;

void gveh_tank_tracks_default(gveh_tank_tracks_cfg *c);
void gveh_tank_tracks_heavy(gveh_tank_tracks_cfg *c);
void gveh_tank_tracks_ww2(gveh_tank_tracks_cfg *c);
void gveh_tank_tracks_scout(gveh_tank_tracks_cfg *c);
void gveh_tank_tracks_clear(gveh_tank_tracks_state *s, const gveh_tank_tracks_cfg *c);
void gveh_tank_tracks_damage(gveh_tank_tracks_state *s, gveh_fx left, gveh_fx right);
void gveh_tank_tracks_apply(const gveh_tank_tracks_cfg *c, gveh_tank_tracks_state *s, gveh_body *body, gveh_basis basis, const gveh_input *in, gveh_fx grounded_ratio, gveh_fx dt);

#endif
