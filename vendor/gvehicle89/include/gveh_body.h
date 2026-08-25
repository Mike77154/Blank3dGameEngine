#ifndef GVEH_BODY_H
#define GVEH_BODY_H

#include "gveh_math.h"

typedef struct gveh_body_s {
    gveh_vec3 pos;
    gveh_vec3 vel;
    gveh_i32 yaw;
    gveh_i32 pitch;
    gveh_i32 roll;
    gveh_i32 yaw_vel;
    gveh_i32 pitch_vel;
    gveh_i32 roll_vel;
    gveh_fx mass;
    gveh_fx inv_mass;
    gveh_fx inertia_yaw;
    gveh_fx inertia_pitch;
    gveh_fx inertia_roll;
    gveh_vec3 force;
    gveh_vec3 torque;
} gveh_body;

void gveh_body_clear(gveh_body *b);
void gveh_body_set_mass(gveh_body *b, gveh_fx mass);
void gveh_body_add_force(gveh_body *b, gveh_vec3 f);
void gveh_body_add_force_at(gveh_body *b, gveh_vec3 f, gveh_vec3 world_point);
void gveh_body_add_yaw_torque(gveh_body *b, gveh_fx t);
void gveh_body_step_damped(gveh_body *b, gveh_fx dt, gveh_fx velocity_retention);
void gveh_body_step(gveh_body *b, gveh_fx dt);

#endif
