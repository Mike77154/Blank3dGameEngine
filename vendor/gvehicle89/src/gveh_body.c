#include "gveh_body.h"

void gveh_body_clear(gveh_body *b)
{
    b->pos = gveh_v3(0,0,0);
    b->vel = gveh_v3(0,0,0);
    b->yaw = 0; b->pitch = 0; b->roll = 0;
    b->yaw_vel = 0; b->pitch_vel = 0; b->roll_vel = 0;
    b->mass = GVEH_FX_ONE;
    b->inv_mass = GVEH_FX_ONE;
    b->inertia_yaw = GVEH_FX_ONE;
    b->inertia_pitch = GVEH_FX_ONE;
    b->inertia_roll = GVEH_FX_ONE;
    b->force = gveh_v3(0,0,0);
    b->torque = gveh_v3(0,0,0);
}

void gveh_body_set_mass(gveh_body *b, gveh_fx mass)
{
    if (mass <= 0) mass = GVEH_FX_ONE;
    b->mass = mass;
    b->inv_mass = gveh_fx_div(GVEH_FX_ONE, mass);
    b->inertia_yaw = mass;
    b->inertia_pitch = mass;
    b->inertia_roll = mass;
}

void gveh_body_add_force(gveh_body *b, gveh_vec3 f)
{
    b->force = gveh_v3_add(b->force, f);
}

void gveh_body_add_force_at(gveh_body *b, gveh_vec3 f, gveh_vec3 world_point)
{
    gveh_vec3 r;
    gveh_vec3 tq;
    b->force = gveh_v3_add(b->force, f);
    r = gveh_v3_sub(world_point, b->pos);
    tq = gveh_v3_cross(r, f);
    b->torque = gveh_v3_add(b->torque, tq);
}

void gveh_body_add_yaw_torque(gveh_body *b, gveh_fx t)
{
    b->torque.y += t;
}

void gveh_body_step_damped(gveh_body *b, gveh_fx dt, gveh_fx velocity_retention)
{
    gveh_vec3 acc;
    gveh_fx yaw_acc;
    gveh_fx pitch_acc;
    gveh_fx roll_acc;
    gveh_fx damping;
    gveh_fx idt;

    idt = dt * 8;
    acc.x = gveh_fx_div(b->force.x, b->mass);
    acc.y = gveh_fx_div(b->force.y, b->mass);
    acc.z = gveh_fx_div(b->force.z, b->mass);
    b->vel = gveh_v3_add(b->vel, gveh_v3_scale(acc, idt));
    b->pos = gveh_v3_add(b->pos, gveh_v3_scale(b->vel, idt));

    yaw_acc = gveh_fx_div(b->torque.y, b->inertia_yaw);
    pitch_acc = gveh_fx_div(b->torque.x, b->inertia_pitch);
    roll_acc = gveh_fx_div(b->torque.z, b->inertia_roll);

    b->yaw_vel += gveh_fx_to_int(gveh_fx_mul(yaw_acc, idt));
    b->pitch_vel += gveh_fx_to_int(gveh_fx_mul(pitch_acc, idt));
    b->roll_vel += gveh_fx_to_int(gveh_fx_mul(roll_acc, idt));

    damping = gveh_fx_clamp(velocity_retention, 0, GVEH_FX_ONE);
    b->vel = gveh_v3_scale(b->vel, damping);
    b->yaw_vel = (b->yaw_vel * 245) / 256;
    b->pitch_vel = (b->pitch_vel * 235) / 256;
    b->roll_vel = (b->roll_vel * 235) / 256;

    b->yaw = (b->yaw + b->yaw_vel) & 255;
    b->pitch = (b->pitch + b->pitch_vel) & 255;
    b->roll = (b->roll + b->roll_vel) & 255;

    b->force = gveh_v3(0,0,0);
    b->torque = gveh_v3(0,0,0);
}

void gveh_body_step(gveh_body *b, gveh_fx dt)
{
    gveh_body_step_damped(b, dt, gveh_fx_from_int(94) / 100);
}
