#include <stdio.h>
#include "gveh.h"

static void plot_point(unsigned char img[128][128], gveh_fx x, gveh_fx z, unsigned char v)
{
    int px;
    int pz;
    px = 64 + gveh_fx_to_int(x);
    pz = 110 - gveh_fx_to_int(z);
    if (px < 0 || px >= 128 || pz < 0 || pz >= 128) return;
    img[pz][px] = v;
}

static void write_ppm(const char *path, unsigned char img[128][128])
{
    FILE *f;
    int y;
    int x;
    unsigned char p;
    f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P6\n128 128\n255\n");
    y = 0;
    while (y < 128) {
        x = 0;
        while (x < 128) {
            p = img[y][x];
            if (p == 0) { fputc(20, f); fputc(20, f); fputc(24, f); }
            else if (p == 1) { fputc(230, f); fputc(230, f); fputc(230, f); }
            else if (p == 2) { fputc(230, f); fputc(150, f); fputc(60, f); }
            else { fputc(90, f); fputc(180, f); fputc(255, f); }
            x++;
        }
        y++;
    }
    fclose(f);
}

int main(int argc, char **argv)
{
    gveh_runtime rt;
    gveh_profile prof;
    gveh_vehicle veh;
    gveh_input in;
    gveh_i32 i;
    gveh_fx dt;
    FILE *csv;
    unsigned char img[128][128];
    int y;
    int x;
    int mode;

    mode = 0;
    if (argc > 1) {
        if (argv[1][0] == 'g') mode = 1;
        if (argv[1][0] == 'r') mode = 2;
        if (argv[1][0] == 'd') mode = 3;
        if (argv[1][0] == 'w') mode = 4;
        if (argv[1][0] == 't') mode = 5;
        if (argv[1][0] == 'h') mode = 6;
        if (argv[1][0] == 's') mode = 7;
        if (argv[1][0] == 'm') mode = 8;
        if (argv[1][0] == 'a') mode = 9;
        if (argv[1][0] == 'f') mode = 10;
        if (argv[1][0] == 'c') mode = 11;
        if (argv[1][0] == 'l') mode = 12;
        if (argv[1][0] == 'u') mode = 13;
        if (argv[1][0] == 'x') mode = 14;
        if (argv[1][0] == 'i') mode = 15;
        if (argv[1][0] == 'b') mode = 16;
        if (argv[1][0] == '1') mode = 17;
        if (argv[1][0] == '2') mode = 18;
        if (argv[1][0] == '3') mode = 19;
        if (argv[1][0] == '4') mode = 20;
        if (argv[1][0] == '5') mode = 21;
        if (argv[1][0] == '6') mode = 22;
    }

    gveh_runtime_init(&rt);
    if (mode == 1) gveh_profile_gt_sport89(&prof);
    else if (mode == 2) gveh_profile_rally89(&prof);
    else if (mode == 3) gveh_profile_daytona89(&prof);
    else if (mode == 4) gveh_profile_wave_jetski89(&prof);
    else if (mode == 5) gveh_profile_tank_lite89(&prof);
    else if (mode == 6) gveh_profile_thunder_chopper89(&prof);
    else if (mode == 7) gveh_profile_strike_chopper89(&prof);
    else if (mode == 8) gveh_profile_sim_heli89(&prof);
    else if (mode == 9) gveh_profile_ace_fighter89(&prof);
    else if (mode == 10) gveh_profile_fs_lightplane89(&prof);
    else if (mode == 11) gveh_profile_comanche_voxel89(&prof);
    else if (mode == 12) gveh_profile_longbow_campaign89(&prof);
    else if (mode == 13) gveh_profile_gunship_flight89(&prof);
    else if (mode == 14) gveh_profile_rogue_xwing89(&prof);
    else if (mode == 15) gveh_profile_tie_interceptor89(&prof);
    else if (mode == 16) gveh_profile_battlefront_bomber89(&prof);
    else if (mode == 17) gveh_profile_tank4_m1a1_89(&prof);
    else if (mode == 18) gveh_profile_tank4_t72_89(&prof);
    else if (mode == 19) gveh_profile_tank4_tiger_89(&prof);
    else if (mode == 20) gveh_profile_tank4_sherman_89(&prof);
    else if (mode == 21) gveh_profile_tank4_destroyer_89(&prof);
    else if (mode == 22) gveh_profile_tank4_scout_89(&prof);
    else gveh_profile_warthog89(&prof);

    if ((prof.class_flags & GVEH_CLASS_SPACECRAFT) != 0u) {
        gveh_vehicle_init(&veh, &prof, gveh_v3(0, gveh_fx_from_int(18), 0));
        veh.body.vel.z = gveh_fx_from_int(60);
    } else if ((prof.class_flags & (GVEH_CLASS_AIRPLANE | GVEH_CLASS_ROTORCRAFT)) != 0u) {
        gveh_vehicle_init(&veh, &prof, gveh_v3(0, gveh_fx_from_int(12), 0));
        if ((prof.class_flags & GVEH_CLASS_AIRPLANE) != 0u) veh.body.vel.z = gveh_fx_from_int(24);
    } else {
        gveh_vehicle_init(&veh, &prof, gveh_v3(0, gveh_fx_from_int(2), 0));
    }
    dt = GVEH_FX_ONE / 30;
    csv = fopen("demo_vehicle89.csv", "w");
    if (!csv) return 2;
    fprintf(csv, "tick,name,pos_x,pos_y,pos_z,vel_x,vel_y,vel_z,yaw,grounded,fx_count,kudos,combo,nitro,riskboost,takedowns,skill_driver,crashbreaker,airspeed,air_lift,rotor_rpm,rotor_lift,fuel,cannon,missiles,lock,cargo,av_lock,threat,engine_dead,ctrl_loss,mission_score,medal,wing_score,shield,laser,heat,tank_left,tank_right,turret_yaw,turret_elev,tank_ammo,tank_fired,tank_pen,tank_effective,tank_spall,crew_shock,crew_alive,module_last,platoon_order,platoon_score\n");

    y = 0;
    while (y < 128) { x = 0; while (x < 128) { img[y][x] = 0; x++; } y++; }

    i = 0;
    while (i < 900) {
        gveh_input_clear(&in);
        if ((prof.class_flags & GVEH_CLASS_TANKSIM) != 0u) {
            in.throttle = GVEH_FX_ONE / 2;
            if (i > 60 && i < 260) in.steer = GVEH_FX_ONE / 2;
            if (i >= 260 && i < 430) in.steer = -GVEH_FX_ONE / 3;
            if (i > 120 && i < 760) in.tank_turret = GVEH_FX_ONE / 2;
            if (i > 360 && i < 520) in.tank_gun = -GVEH_FX_ONE / 3;
            if ((i % 120) < 50) in.tank_range = GVEH_FX_ONE;
            if ((i % 180) == 20) in.tank_ammo_next = GVEH_FX_ONE;
            if ((i % 150) == 40) in.tank_order = GVEH_FX_ONE;
            if ((i % 100) == 70) in.fire = GVEH_FX_ONE;
            if (i > 600 && i < 660) in.tank_smoke = GVEH_FX_ONE;
            if (i > 700) { in.brake = GVEH_FX_ONE / 2; in.throttle = GVEH_FX_ONE / 4; }
            if (i > 500 && i < 680) in.risk = GVEH_FX_ONE;
            in.tank_face = gveh_fx_from_int((i / 220) & 3);
        } else if ((prof.class_flags & GVEH_CLASS_SPACECRAFT) != 0u) {
            in.throttle = GVEH_FX_ONE;
            if (i > 60 && i < 220) { in.air_roll = GVEH_FX_ONE / 2; in.air_yaw = GVEH_FX_ONE / 4; }
            if (i >= 220 && i < 420) { in.air_roll = -GVEH_FX_ONE / 2; in.air_yaw = -GVEH_FX_ONE / 5; }
            if (i > 380 && i < 520) in.air_pitch = GVEH_FX_ONE / 3;
            if (i > 90 && i < 190) in.nitro = GVEH_FX_ONE;
            if (i > 240 && i < 620) in.lock_on = GVEH_FX_ONE;
            if ((i % 34) < 16) in.fire = GVEH_FX_ONE;
            if (i > 500 && i < 650) in.brake = GVEH_FX_ONE;
            if (i > 680 && i < 760) in.handbrake = GVEH_FX_ONE;
            if (i > 300 && i < 460) in.aggression = GVEH_FX_ONE;
        } else if ((prof.class_flags & GVEH_CLASS_AIRPLANE) != 0u) {
            in.throttle = GVEH_FX_ONE;
            if (i > 80 && i < 260) { in.air_roll = GVEH_FX_ONE / 2; in.air_yaw = GVEH_FX_ONE / 5; }
            if (i >= 260 && i < 430) { in.air_roll = -GVEH_FX_ONE / 2; in.air_yaw = -GVEH_FX_ONE / 5; }
            if (i > 430 && i < 520) in.air_pitch = GVEH_FX_ONE / 3;
            if (i > 520 && i < 620) in.air_pitch = -GVEH_FX_ONE / 4;
            if (i > 160 && i < 230) in.nitro = GVEH_FX_ONE;
            if (i > 300 && i < 520) in.lock_on = GVEH_FX_ONE;
            if ((i % 60) < 20) in.fire = GVEH_FX_ONE;
            if (i > 640) in.alt_hold = GVEH_FX_ONE;
        } else if ((prof.class_flags & GVEH_CLASS_ROTORCRAFT) != 0u) {
            in.collective = GVEH_FX_ONE / 2 + GVEH_FX_ONE / 8;
            in.throttle = GVEH_FX_ONE / 2;
            if (i > 80 && i < 260) { in.air_roll = GVEH_FX_ONE / 3; in.air_yaw = GVEH_FX_ONE / 3; }
            if (i >= 260 && i < 430) { in.air_roll = -GVEH_FX_ONE / 3; in.air_yaw = -GVEH_FX_ONE / 4; }
            if (i > 430 && i < 560) in.air_pitch = GVEH_FX_ONE / 2;
            if (i > 160 && i < 230) in.nitro = GVEH_FX_ONE;
            if (i > 300 && i < 520) in.lock_on = GVEH_FX_ONE;
            if ((i % 75) < 22) in.fire = GVEH_FX_ONE;
            if (i > 560 && i < 620) in.winch = GVEH_FX_ONE;
            if (i > 640) in.alt_hold = GVEH_FX_ONE;
        } else {
            in.throttle = GVEH_FX_ONE;
            if (i > 80 && i < 320) in.steer = GVEH_FX_ONE / 2;
            if (i >= 320 && i < 520) in.steer = -GVEH_FX_ONE / 3;
            if (i >= 520 && i < 650) in.handbrake = GVEH_FX_ONE;
            if (i > 130 && i < 210) in.nitro = GVEH_FX_ONE;
            if (i > 300 && i < 390) in.risk = GVEH_FX_ONE;
            if (i > 560 && i < 610) { in.aggression = GVEH_FX_ONE; in.risk = GVEH_FX_ONE; }
            if (i > 650 && i < 700) { in.aftertouch_x = GVEH_FX_ONE / 2; in.aftertouch_z = GVEH_FX_ONE / 3; }
            if (i == 720) in.crashbreak = GVEH_FX_ONE;
            if (i >= 760) { in.throttle = GVEH_FX_ONE / 3; in.brake = GVEH_FX_ONE / 2; }
        }
        gveh_vehicle_step(&rt, &veh, &in, dt);
        gveh_debug_print(csv, &veh, i);
        if (i % 3 == 0) plot_point(img, veh.body.pos.x, veh.body.pos.z, (unsigned char)(1 + mode));
        i++;
    }
    fclose(csv);
    write_ppm("demo_vehicle89.ppm", img);
    printf("gvehicle89 demo ok: %s\n", prof.name);
    return 0;
}
