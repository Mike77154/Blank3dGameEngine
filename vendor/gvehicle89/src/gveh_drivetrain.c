#include "gveh_drivetrain.h"
#include "gveh_math.h"

static gveh_fx gveh_engine_torque(gveh_engine *e, gveh_fx rpm)
{
    gveh_i32 i;
    gveh_fx r;
    gveh_fx r0;
    gveh_fx r1;
    gveh_fx t0;
    gveh_fx t1;
    gveh_fx k;

    r = rpm / 100;
    if (r <= e->torque_rpm[0]) return gveh_fx_from_int(e->torque_nm[0]);
    i = 0;
    while (i < 15) {
        if (r <= e->torque_rpm[i + 1]) {
            r0 = gveh_fx_from_int(e->torque_rpm[i]);
            r1 = gveh_fx_from_int(e->torque_rpm[i + 1]);
            t0 = gveh_fx_from_int(e->torque_nm[i]);
            t1 = gveh_fx_from_int(e->torque_nm[i + 1]);
            k = gveh_fx_div(gveh_fx_from_int(gveh_fx_to_int(r) - e->torque_rpm[i]), r1 - r0);
            return gveh_fx_lerp(t0, t1, k);
        }
        i++;
    }
    return gveh_fx_from_int(e->torque_nm[15]);
}

static void gveh_engine_curve(gveh_engine *e, gveh_i32 style)
{
    static const gveh_i16 rpm[16] = {8,12,16,20,24,28,32,36,40,45,50,55,60,65,70,75};
    static const gveh_i16 sport[16] = {70,95,130,170,205,230,245,255,260,255,245,225,200,160,110,60};
    static const gveh_i16 flat[16] = {170,190,205,215,220,225,225,225,220,215,210,205,195,185,175,160};
    gveh_i32 i;
    i = 0;
    while (i < 16) {
        e->torque_rpm[i] = rpm[i];
        if (style == 0) e->torque_nm[i] = sport[i];
        else e->torque_nm[i] = flat[i];
        i++;
    }
}

void gveh_drivetrain_sport(gveh_drivetrain *d)
{
    gveh_i32 i;
    d->engine.idle_rpm = gveh_fx_from_int(900);
    d->engine.redline_rpm = gveh_fx_from_int(7600);
    d->engine.rpm = d->engine.idle_rpm;
    d->engine.inertia = GVEH_FX_ONE;
    d->engine.engine_brake = GVEH_FX_ONE * 2;
    gveh_engine_curve(&d->engine, 0);
    i = 0;
    while (i < 8) { d->gearbox.gear_ratio[i] = GVEH_FX_ONE; i++; }
    d->gearbox.gear_ratio[0] = -gveh_fx_from_int(3);
    d->gearbox.gear_ratio[1] = gveh_fx_from_int(3);
    d->gearbox.gear_ratio[2] = gveh_fx_from_int(2);
    d->gearbox.gear_ratio[3] = gveh_fx_from_int(16) / 10;
    d->gearbox.gear_ratio[4] = gveh_fx_from_int(13) / 10;
    d->gearbox.gear_ratio[5] = GVEH_FX_ONE;
    d->gearbox.gear_ratio[6] = gveh_fx_from_int(8) / 10;
    d->gearbox.gear_ratio[7] = gveh_fx_from_int(6) / 10;
    d->gearbox.final_drive = gveh_fx_from_int(4);
    d->gearbox.current_gear = 1;
    d->gearbox.gear_count = 7;
    d->gearbox.shift_up_rpm = gveh_fx_from_int(6900);
    d->gearbox.shift_down_rpm = gveh_fx_from_int(2100);
    d->drive_torque = 0;
    d->brake_torque = 0;
    d->clutch = GVEH_FX_ONE;
}

void gveh_drivetrain_arcade(gveh_drivetrain *d)
{
    gveh_drivetrain_sport(d);
    gveh_engine_curve(&d->engine, 1);
    d->gearbox.gear_count = 4;
    d->gearbox.gear_ratio[1] = gveh_fx_from_int(3);
    d->gearbox.gear_ratio[2] = gveh_fx_from_int(2);
    d->gearbox.gear_ratio[3] = GVEH_FX_ONE;
    d->gearbox.gear_ratio[4] = gveh_fx_from_int(7) / 10;
    d->engine.engine_brake = GVEH_FX_ONE / 2;
}

void gveh_drivetrain_update(gveh_drivetrain *d, gveh_fx throttle, gveh_fx brake, gveh_fx avg_wheel_speed, gveh_fx dt)
{
    gveh_fx gear;
    gveh_fx ratio;
    gveh_fx wheel_rpm;
    gveh_fx torque;
    gveh_i8 cg;
    (void)dt;

    throttle = gveh_fx_clamp(throttle, 0, GVEH_FX_ONE);
    brake = gveh_fx_clamp(brake, 0, GVEH_FX_ONE);
    cg = d->gearbox.current_gear;
    /* Gear 0 is the reverse ratio installed by both stock drivetrains.
       Earlier revisions clamped it away, leaving that authored gear
       unreachable.  Keep automatic shifting only in forward gears. */
    if (cg < 0) cg = 0;
    if (cg > (gveh_i8)d->gearbox.gear_count)
        cg = (gveh_i8)d->gearbox.gear_count;

    gear = d->gearbox.gear_ratio[(int)cg];
    ratio = gveh_fx_mul(gear, d->gearbox.final_drive);
    wheel_rpm = gveh_fx_abs(avg_wheel_speed) * 45;
    d->engine.rpm = d->engine.idle_rpm +
                    gveh_fx_mul(wheel_rpm, gveh_fx_abs(ratio));
    if (cg > 0) {
        if (d->engine.rpm > d->gearbox.shift_up_rpm &&
            cg < (gveh_i8)d->gearbox.gear_count)
            d->gearbox.current_gear++;
        if (d->engine.rpm < d->gearbox.shift_down_rpm && cg > 1)
            d->gearbox.current_gear--;
    }
    if (d->engine.rpm > d->engine.redline_rpm) d->engine.rpm = d->engine.redline_rpm;

    torque = gveh_engine_torque(&d->engine, d->engine.rpm);
    d->drive_torque = gveh_fx_mul(gveh_fx_mul(torque, ratio), throttle) / 6;
    d->brake_torque = gveh_fx_mul(gveh_fx_from_int(800), brake);
}
