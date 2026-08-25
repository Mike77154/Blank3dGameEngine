#ifndef GVEH_DRIVETRAIN_H
#define GVEH_DRIVETRAIN_H

#include "gveh_types.h"

typedef struct gveh_engine_s {
    gveh_fx idle_rpm;
    gveh_fx redline_rpm;
    gveh_fx rpm;
    gveh_fx inertia;
    gveh_i16 torque_rpm[16];
    gveh_i16 torque_nm[16];
    gveh_fx engine_brake;
} gveh_engine;

typedef struct gveh_gearbox_s {
    gveh_fx gear_ratio[8];
    gveh_fx final_drive;
    gveh_i8 current_gear;
    gveh_u8 gear_count;
    gveh_fx shift_up_rpm;
    gveh_fx shift_down_rpm;
} gveh_gearbox;

typedef struct gveh_drivetrain_s {
    gveh_engine engine;
    gveh_gearbox gearbox;
    gveh_fx drive_torque;
    gveh_fx brake_torque;
    gveh_fx clutch;
} gveh_drivetrain;

void gveh_drivetrain_sport(gveh_drivetrain *d);
void gveh_drivetrain_arcade(gveh_drivetrain *d);
void gveh_drivetrain_update(gveh_drivetrain *d, gveh_fx throttle, gveh_fx brake, gveh_fx avg_wheel_speed, gveh_fx dt);

#endif
