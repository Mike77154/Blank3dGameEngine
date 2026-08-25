#include "gveh_tank_tracks.h"

void gveh_tank_tracks_default(gveh_tank_tracks_cfg *c)
{
    c->drive_force = gveh_fx_from_int(180000);
    c->brake_force = gveh_fx_from_int(52000);
    c->pivot_torque = gveh_fx_from_int(55000);
    c->yaw_torque = gveh_fx_from_int(36000);
    c->mud_drag = GVEH_FX_ONE / 5;
    c->track_grip = GVEH_FX_ONE;
    c->track_health_max = gveh_fx_from_int(100);
    c->flags = 0;
}

void gveh_tank_tracks_heavy(gveh_tank_tracks_cfg *c)
{
    gveh_tank_tracks_default(c);
    c->drive_force = gveh_fx_from_int(260000);
    c->brake_force = gveh_fx_from_int(76000);
    c->pivot_torque = gveh_fx_from_int(73000);
    c->yaw_torque = gveh_fx_from_int(50000);
    c->mud_drag = GVEH_FX_ONE / 4;
}

void gveh_tank_tracks_ww2(gveh_tank_tracks_cfg *c)
{
    gveh_tank_tracks_default(c);
    c->drive_force = gveh_fx_from_int(150000);
    c->brake_force = gveh_fx_from_int(42000);
    c->pivot_torque = gveh_fx_from_int(38000);
    c->yaw_torque = gveh_fx_from_int(28000);
    c->mud_drag = GVEH_FX_ONE / 3;
}

void gveh_tank_tracks_scout(gveh_tank_tracks_cfg *c)
{
    gveh_tank_tracks_default(c);
    c->drive_force = gveh_fx_from_int(165000);
    c->brake_force = gveh_fx_from_int(46000);
    c->pivot_torque = gveh_fx_from_int(43000);
    c->yaw_torque = gveh_fx_from_int(42000);
    c->mud_drag = GVEH_FX_ONE / 8;
}

void gveh_tank_tracks_clear(gveh_tank_tracks_state *s, const gveh_tank_tracks_cfg *c)
{
    s->left_power = 0;
    s->right_power = 0;
    s->left_health = c->track_health_max;
    s->right_health = c->track_health_max;
    s->slip = 0;
    s->squeal = 0;
    s->pivoting = 0;
}

void gveh_tank_tracks_damage(gveh_tank_tracks_state *s, gveh_fx left, gveh_fx right)
{
    s->left_health -= left;
    s->right_health -= right;
    if (s->left_health < 0) s->left_health = 0;
    if (s->right_health < 0) s->right_health = 0;
}

void gveh_tank_tracks_apply(const gveh_tank_tracks_cfg *c, gveh_tank_tracks_state *s, gveh_body *body, gveh_basis basis, const gveh_input *in, gveh_fx grounded_ratio, gveh_fx dt)
{
    gveh_fx left_health;
    gveh_fx right_health;
    gveh_fx left;
    gveh_fx right;
    gveh_fx avg;
    gveh_fx diff;
    gveh_fx fmag;
    gveh_fx bmag;
    gveh_fx yaw;
    gveh_vec3 fwd_force;
    gveh_fx side;
    gveh_vec3 dragv;

    (void)dt;
    if (grounded_ratio <= 0) return;
    left_health = gveh_fx_div(s->left_health, c->track_health_max + 1);
    right_health = gveh_fx_div(s->right_health, c->track_health_max + 1);
    left = in->throttle + in->steer;
    right = in->throttle - in->steer;
    left = gveh_fx_clamp(left, -GVEH_FX_ONE, GVEH_FX_ONE);
    right = gveh_fx_clamp(right, -GVEH_FX_ONE, GVEH_FX_ONE);
    left = gveh_fx_mul(left, left_health);
    right = gveh_fx_mul(right, right_health);
    s->left_power = left;
    s->right_power = right;
    avg = (left + right) / 2;
    diff = left - right;
    fmag = gveh_fx_mul(c->drive_force, avg);
    fmag = gveh_fx_mul(fmag, grounded_ratio);
    if (in->brake > 0) {
        bmag = gveh_fx_mul(c->brake_force, in->brake);
        if (gveh_v3_dot(body->vel, basis.fwd) > 0) fmag -= bmag;
        else fmag += bmag;
    }
    fwd_force = gveh_v3_scale(basis.fwd, fmag);
    gveh_body_add_force(body, fwd_force);
    yaw = gveh_fx_mul(c->yaw_torque, diff);
    if (gveh_fx_abs(avg) < GVEH_FX_ONE / 6 && gveh_fx_abs(diff) > GVEH_FX_ONE / 4) {
        yaw = gveh_fx_mul(c->pivot_torque, diff);
        s->pivoting = 1;
    } else {
        s->pivoting = 0;
    }
    yaw = gveh_fx_mul(yaw, grounded_ratio);
    gveh_body_add_yaw_torque(body, yaw);
    side = gveh_v3_dot(body->vel, basis.right);
    s->slip = gveh_fx_abs(side);
    s->squeal = gveh_fx_abs(diff) + (gveh_fx_abs(side) / 4);
    dragv = gveh_v3_scale(basis.right, -gveh_fx_mul(side, c->track_grip));
    gveh_body_add_force(body, dragv);
    if (gveh_fx_abs(avg) > GVEH_FX_ONE / 2) {
        gveh_body_add_force(body, gveh_v3_scale(body->vel, -c->mud_drag));
    }
}
