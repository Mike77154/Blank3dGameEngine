#include "gveh_tag.h"
#include <string.h>

static void gveh_copy_name(char dst[32], const char *src)
{
    gveh_i32 i;
    i = 0;
    while (i < 31 && src[i] != 0) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}

void gveh_profile_refresh_module_flags(gveh_profile *p)
{
    gveh_u16 m;

    m = 0u;
    if (p->wheel_count > 0) m = (gveh_u16)(m | GVEH_MODULE_CAR);
    if (p->buoy_count > 0 || (p->class_flags & GVEH_CLASS_WATER) != 0u) m = (gveh_u16)(m | GVEH_MODULE_WATER);
    if ((p->class_flags & (GVEH_CLASS_AIRPLANE | GVEH_CLASS_ROTORCRAFT | GVEH_CLASS_VTOL)) != 0u) m = (gveh_u16)(m | GVEH_MODULE_AIR);
    if ((p->class_flags & GVEH_CLASS_SPACECRAFT) != 0u) m = (gveh_u16)(m | GVEH_MODULE_SPACE);
    if ((p->class_flags & (GVEH_CLASS_TRACKED | GVEH_CLASS_TANKSIM)) != 0u || p->tank4.enabled != 0) m = (gveh_u16)(m | GVEH_MODULE_TANK);
    if ((p->class_flags & (GVEH_CLASS_ARCADE | GVEH_CLASS_HALOLIKE | GVEH_CLASS_RALLY)) != 0u) m = (gveh_u16)(m | GVEH_MODULE_ARCADE);
    p->module_flags = m;
}

void gveh_profile_clear(gveh_profile *p)
{
    gveh_i32 i;
    memset(p, 0, sizeof(*p));
    gveh_copy_name(p->name, "vehicle");
    p->module_flags = 0u;
    p->collision_radius = gveh_fx_from_int(2);
    p->mass = gveh_fx_from_int(1200);
    p->gravity = gveh_fx_from_int(24);
    p->max_speed = gveh_fx_from_int(55);
    p->velocity_retention = gveh_fx_from_int(94) / 100;
    p->steer_rate = GVEH_FX_ONE;
    p->yaw_power = GVEH_FX_ONE * 3;
    p->slide_power = GVEH_FX_ONE;
    p->air_yaw_power = 0;
    gveh_susp_default(&p->susp);
    gveh_tire_curve_sport(&p->tire);
    gveh_drivetrain_sport(&p->drive);
    gveh_assist_gt(&p->assists);
    gveh_nitro_default(&p->nitro);
    gveh_kudos_default(&p->kudos);
    gveh_skill_default(&p->skill);
    gveh_tuning_clear(&p->tuning);
    gveh_driver_profile_clear(&p->driver_profile);
    gveh_riskboost_default(&p->riskboost);
    gveh_takedown_default(&p->takedown);
    gveh_aftertouch_default(&p->aftertouch);
    gveh_crashbreaker_default(&p->crashbreaker);
    gveh_airframe_default(&p->airframe);
    gveh_rotorcraft_default(&p->rotorcraft);
    gveh_airassist_default(&p->airassist);
    gveh_airgame_default(&p->airgame);
    gveh_air_damage_default(&p->air_damage);
    gveh_avionics_default(&p->avionics);
    gveh_wingman_default(&p->wingman);
    gveh_air_mission_default(&p->air_mission);
    gveh_spacecraft_default(&p->spacecraft);
    gveh_tank4_default(&p->tank4);
    p->tank4.enabled = 0;
    gveh_camera_default(&p->camera);
    i = 0;
    while (i < GVEH_MAX_WHEELS) { gveh_wheel_clear(&p->wheels[i]); i++; }
    i = 0;
    while (i < GVEH_MAX_MASSPTS) { gveh_masspoint_clear(&p->masspoints[i]); i++; }
    i = 0;
    while (i < GVEH_MAX_BUOYS) { gveh_buoy_clear(&p->buoys[i]); i++; }
    p->wheel_count = 4;
    p->wheels[0].local_pos = gveh_v3(-GVEH_FX_ONE, 0, gveh_fx_from_int(2));
    p->wheels[1].local_pos = gveh_v3(GVEH_FX_ONE, 0, gveh_fx_from_int(2));
    p->wheels[2].local_pos = gveh_v3(-GVEH_FX_ONE, 0, -gveh_fx_from_int(2));
    p->wheels[3].local_pos = gveh_v3(GVEH_FX_ONE, 0, -gveh_fx_from_int(2));
    p->wheels[0].steer_max = GVEH_FX_ONE;
    p->wheels[1].steer_max = GVEH_FX_ONE;
    p->wheels[0].drive_bias = GVEH_FX_ONE / 2;
    p->wheels[1].drive_bias = GVEH_FX_ONE / 2;
    p->wheels[2].drive_bias = GVEH_FX_ONE / 2;
    p->wheels[3].drive_bias = GVEH_FX_ONE / 2;
    p->wheels[0].brake_bias = GVEH_FX_ONE / 2;
    p->wheels[1].brake_bias = GVEH_FX_ONE / 2;
    p->wheels[2].brake_bias = GVEH_FX_ONE / 2;
    p->wheels[3].brake_bias = GVEH_FX_ONE / 2;
    gveh_profile_refresh_module_flags(p);
}

void gveh_profile_warthog89(gveh_profile *p)
{
    gveh_profile_clear(p);
    gveh_copy_name(p->name, "warthog89");
    p->class_flags = GVEH_CLASS_HALOLIKE | GVEH_CLASS_ARCADE;
    p->mass = gveh_fx_from_int(1600);
    p->max_speed = gveh_fx_from_int(48);
    p->yaw_power = GVEH_FX_ONE * 5;
    p->slide_power = GVEH_FX_ONE * 2;
    p->air_yaw_power = GVEH_FX_ONE;
    p->susp.rest_len = gveh_fx_from_int(15) / 10;
    p->susp.spring_k = GVEH_FX_ONE * 9;
    p->susp.damper_bump = GVEH_FX_ONE * 2;
    p->susp.damper_rebound = GVEH_FX_ONE;
    gveh_tire_curve_arcade(&p->tire);
    gveh_drivetrain_arcade(&p->drive);
    gveh_assist_halo(&p->assists);
    gveh_tuning_street(&p->tuning);
    p->nitro.force = gveh_fx_from_int(1800);
    p->riskboost.boost_force = gveh_fx_from_int(1600);
    p->wheels[2].steer_max = GVEH_FX_ONE / 2;
    p->wheels[3].steer_max = GVEH_FX_ONE / 2;
    p->masspoint_count = 6;
    p->masspoints[0].local_pos = gveh_v3(-gveh_fx_from_int(12)/10, -GVEH_FX_HALF, gveh_fx_from_int(2));
    p->masspoints[1].local_pos = gveh_v3(gveh_fx_from_int(12)/10, -GVEH_FX_HALF, gveh_fx_from_int(2));
    p->masspoints[2].local_pos = gveh_v3(-gveh_fx_from_int(12)/10, -GVEH_FX_HALF, -gveh_fx_from_int(2));
    p->masspoints[3].local_pos = gveh_v3(gveh_fx_from_int(12)/10, -GVEH_FX_HALF, -gveh_fx_from_int(2));
    p->masspoints[4].local_pos = gveh_v3(0, 0, gveh_fx_from_int(3));
    p->masspoints[5].local_pos = gveh_v3(0, 0, -gveh_fx_from_int(3));
    p->seat_count = 3;
    p->seats[0].local_pos = gveh_v3(0, GVEH_FX_ONE, GVEH_FX_HALF);
    p->seats[1].local_pos = gveh_v3(-GVEH_FX_ONE, GVEH_FX_ONE, -GVEH_FX_HALF);
    p->seats[2].local_pos = gveh_v3(GVEH_FX_ONE, GVEH_FX_ONE, -GVEH_FX_HALF);
    gveh_tuning_apply(p, &p->tuning);
}

void gveh_profile_gt_sport89(gveh_profile *p)
{
    gveh_profile_clear(p);
    gveh_copy_name(p->name, "gt_sport89");
    p->class_flags = GVEH_CLASS_SIMLITE;
    p->mass = gveh_fx_from_int(1180);
    p->max_speed = gveh_fx_from_int(76);
    p->yaw_power = GVEH_FX_ONE * 3;
    p->slide_power = GVEH_FX_ONE;
    p->susp.spring_k = GVEH_FX_ONE * 16;
    p->susp.damper_bump = GVEH_FX_ONE * 5;
    p->susp.damper_rebound = GVEH_FX_ONE * 4;
    gveh_tire_curve_sport(&p->tire);
    gveh_drivetrain_sport(&p->drive);
    gveh_assist_gt(&p->assists);
    gveh_tuning_street(&p->tuning);
    p->skill.steer_help = GVEH_FX_ONE / 5;
    gveh_tuning_apply(p, &p->tuning);
}

void gveh_profile_rally89(gveh_profile *p)
{
    gveh_profile_clear(p);
    gveh_copy_name(p->name, "rally89");
    p->class_flags = GVEH_CLASS_RALLY | GVEH_CLASS_ARCADE;
    p->mass = gveh_fx_from_int(1260);
    p->max_speed = gveh_fx_from_int(64);
    p->yaw_power = GVEH_FX_ONE * 4;
    p->slide_power = GVEH_FX_ONE * 3;
    p->susp.rest_len = gveh_fx_from_int(13) / 10;
    p->susp.spring_k = GVEH_FX_ONE * 11;
    gveh_tire_curve_rally(&p->tire);
    gveh_drivetrain_arcade(&p->drive);
    gveh_assist_arcade(&p->assists);
    gveh_tuning_drift(&p->tuning);
    p->kudos.drift_points = 12;
    p->riskboost.drift_gain = gveh_fx_from_int(25);
    gveh_driver_profile_arcade(&p->driver_profile);
    gveh_tuning_apply(p, &p->tuning);
}

void gveh_profile_daytona89(gveh_profile *p)
{
    gveh_profile_clear(p);
    gveh_copy_name(p->name, "daytona89");
    p->class_flags = GVEH_CLASS_ARCADE;
    p->mass = gveh_fx_from_int(1500);
    p->max_speed = gveh_fx_from_int(88);
    p->yaw_power = GVEH_FX_ONE * 4;
    p->slide_power = GVEH_FX_ONE * 4;
    gveh_tire_curve_arcade(&p->tire);
    gveh_drivetrain_arcade(&p->drive);
    gveh_assist_arcade(&p->assists);
    gveh_tuning_drag(&p->tuning);
    p->riskboost.signal_gain = gveh_fx_from_int(35);
    gveh_driver_profile_arcade(&p->driver_profile);
    gveh_tuning_apply(p, &p->tuning);
}

void gveh_profile_wave_jetski89(gveh_profile *p)
{
    gveh_profile_clear(p);
    gveh_copy_name(p->name, "wave_jetski89");
    p->class_flags = GVEH_CLASS_WATER | GVEH_CLASS_ARCADE;
    p->mass = gveh_fx_from_int(360);
    p->max_speed = gveh_fx_from_int(40);
    p->yaw_power = GVEH_FX_ONE * 5;
    p->slide_power = GVEH_FX_ONE * 2;
    p->wheel_count = 0;
    p->buoy_count = 4;
    p->buoys[0].local_pos = gveh_v3(-GVEH_FX_HALF, 0, GVEH_FX_ONE);
    p->buoys[1].local_pos = gveh_v3(GVEH_FX_HALF, 0, GVEH_FX_ONE);
    p->buoys[2].local_pos = gveh_v3(-GVEH_FX_HALF, 0, -GVEH_FX_ONE);
    p->buoys[3].local_pos = gveh_v3(GVEH_FX_HALF, 0, -GVEH_FX_ONE);
    gveh_drivetrain_arcade(&p->drive);
    gveh_assist_arcade(&p->assists);
    p->nitro.force = gveh_fx_from_int(900);
    p->riskboost.boost_force = gveh_fx_from_int(700);
}

void gveh_profile_tank_lite89(gveh_profile *p)
{
    gveh_profile_clear(p);
    gveh_copy_name(p->name, "tank_lite89");
    p->class_flags = GVEH_CLASS_TRACKED | GVEH_CLASS_ARCADE;
    p->mass = gveh_fx_from_int(6000);
    p->max_speed = gveh_fx_from_int(30);
    p->yaw_power = GVEH_FX_ONE * 8;
    p->slide_power = GVEH_FX_ONE / 2;
    p->wheels[0].steer_max = 0;
    p->wheels[1].steer_max = 0;
    p->wheels[2].steer_max = 0;
    p->wheels[3].steer_max = 0;
    gveh_drivetrain_arcade(&p->drive);
    gveh_assist_arcade(&p->assists);
    p->nitro.force = gveh_fx_from_int(500);
    p->riskboost.boost_force = gveh_fx_from_int(500);
}


void gveh_profile_thunder_chopper89(gveh_profile *p)
{
    gveh_profile_clear(p);
    gveh_copy_name(p->name, "thunder_chopper89");
    p->class_flags = GVEH_CLASS_ROTORCRAFT | GVEH_CLASS_ARCADE;
    p->wheel_count = 0;
    p->mass = gveh_fx_from_int(2200);
    p->gravity = gveh_fx_from_int(24);
    p->max_speed = gveh_fx_from_int(64);
    p->yaw_power = GVEH_FX_ONE * 5;
    p->air_yaw_power = GVEH_FX_ONE * 3;
    gveh_rotorcraft_thunder(&p->rotorcraft);
    gveh_airassist_arcade(&p->airassist);
    gveh_airgame_arcade(&p->airgame);
    gveh_air_damage_arcade(&p->air_damage);
    gveh_avionics_arcade(&p->avionics);
    gveh_air_mission_ace(&p->air_mission);
    p->nitro.force = gveh_fx_from_int(900);
    p->riskboost.boost_force = gveh_fx_from_int(600);
    p->camera.dist = gveh_fx_from_int(10);
    p->camera.height = gveh_fx_from_int(4);
    p->camera.lag = GVEH_FX_ONE / 4;
    p->masspoint_count = 3;
    p->masspoints[0].local_pos = gveh_v3(0, -GVEH_FX_HALF, gveh_fx_from_int(2));
    p->masspoints[1].local_pos = gveh_v3(-GVEH_FX_ONE, -GVEH_FX_HALF, -GVEH_FX_ONE);
    p->masspoints[2].local_pos = gveh_v3(GVEH_FX_ONE, -GVEH_FX_HALF, -GVEH_FX_ONE);
}

void gveh_profile_strike_chopper89(gveh_profile *p)
{
    gveh_profile_clear(p);
    gveh_copy_name(p->name, "strike_chopper89");
    p->class_flags = GVEH_CLASS_ROTORCRAFT | GVEH_CLASS_SIMLITE;
    p->wheel_count = 0;
    p->mass = gveh_fx_from_int(4200);
    p->gravity = gveh_fx_from_int(24);
    p->max_speed = gveh_fx_from_int(46);
    p->yaw_power = GVEH_FX_ONE * 3;
    gveh_rotorcraft_strike(&p->rotorcraft);
    gveh_airassist_arcade(&p->airassist);
    gveh_airgame_strike(&p->airgame);
    gveh_air_damage_arcade(&p->air_damage);
    gveh_avionics_arcade(&p->avionics);
    gveh_wingman_arcade(&p->wingman);
    gveh_air_mission_strike(&p->air_mission);
    p->skill.gain_air = GVEH_FX_ONE / 4;
    p->camera.dist = gveh_fx_from_int(13);
    p->camera.height = gveh_fx_from_int(7);
    p->camera.lag = GVEH_FX_ONE / 5;
    p->masspoint_count = 4;
    p->masspoints[0].local_pos = gveh_v3(-GVEH_FX_ONE, -GVEH_FX_ONE, GVEH_FX_ONE);
    p->masspoints[1].local_pos = gveh_v3(GVEH_FX_ONE, -GVEH_FX_ONE, GVEH_FX_ONE);
    p->masspoints[2].local_pos = gveh_v3(-GVEH_FX_ONE, -GVEH_FX_ONE, -GVEH_FX_ONE);
    p->masspoints[3].local_pos = gveh_v3(GVEH_FX_ONE, -GVEH_FX_ONE, -GVEH_FX_ONE);
}

void gveh_profile_sim_heli89(gveh_profile *p)
{
    gveh_profile_clear(p);
    gveh_copy_name(p->name, "sim_heli89");
    p->class_flags = GVEH_CLASS_ROTORCRAFT | GVEH_CLASS_SIMLITE;
    p->wheel_count = 0;
    p->mass = gveh_fx_from_int(5200);
    p->gravity = gveh_fx_from_int(24);
    p->max_speed = gveh_fx_from_int(38);
    gveh_rotorcraft_sim(&p->rotorcraft);
    gveh_airassist_sim(&p->airassist);
    gveh_airgame_strike(&p->airgame);
    gveh_air_damage_sim(&p->air_damage);
    gveh_avionics_sim(&p->avionics);
    gveh_wingman_sim(&p->wingman);
    gveh_air_mission_strike(&p->air_mission);
    p->airgame.fuel_burn_power = GVEH_FX_ONE / 28;
    p->camera.dist = gveh_fx_from_int(15);
    p->camera.height = gveh_fx_from_int(5);
}

void gveh_profile_ace_fighter89(gveh_profile *p)
{
    gveh_profile_clear(p);
    gveh_copy_name(p->name, "ace_fighter89");
    p->class_flags = GVEH_CLASS_AIRPLANE | GVEH_CLASS_ARCADE;
    p->wheel_count = 0;
    p->mass = gveh_fx_from_int(9000);
    p->gravity = gveh_fx_from_int(24);
    p->max_speed = gveh_fx_from_int(140);
    gveh_airframe_ace(&p->airframe);
    gveh_airassist_arcade(&p->airassist);
    gveh_airgame_arcade(&p->airgame);
    gveh_air_damage_arcade(&p->air_damage);
    gveh_avionics_arcade(&p->avionics);
    gveh_wingman_arcade(&p->wingman);
    gveh_air_mission_ace(&p->air_mission);
    p->nitro.force = gveh_fx_from_int(2500);
    p->riskboost.boost_force = gveh_fx_from_int(1800);
    p->camera.dist = gveh_fx_from_int(18);
    p->camera.height = gveh_fx_from_int(5);
    p->camera.lag = GVEH_FX_ONE / 5;
}

void gveh_profile_fs_lightplane89(gveh_profile *p)
{
    gveh_profile_clear(p);
    gveh_copy_name(p->name, "fs_lightplane89");
    p->class_flags = GVEH_CLASS_AIRPLANE | GVEH_CLASS_SIMLITE;
    p->wheel_count = 0;
    p->mass = gveh_fx_from_int(1100);
    p->gravity = gveh_fx_from_int(24);
    p->max_speed = gveh_fx_from_int(72);
    gveh_airframe_lightplane(&p->airframe);
    gveh_airassist_sim(&p->airassist);
    gveh_airgame_strike(&p->airgame);
    gveh_air_damage_sim(&p->air_damage);
    gveh_avionics_sim(&p->avionics);
    gveh_air_mission_ace(&p->air_mission);
    p->airgame.cannon_max = 0;
    p->airgame.missile_max = 0;
    p->airgame.rocket_max = 0;
    p->camera.dist = gveh_fx_from_int(14);
    p->camera.height = gveh_fx_from_int(4);
}


void gveh_profile_comanche_voxel89(gveh_profile *p)
{
    gveh_profile_clear(p);
    gveh_copy_name(p->name, "comanche_voxel89");
    p->class_flags = GVEH_CLASS_ROTORCRAFT | GVEH_CLASS_ARCADE;
    p->wheel_count = 0;
    p->mass = gveh_fx_from_int(3600);
    p->gravity = gveh_fx_from_int(23);
    p->max_speed = gveh_fx_from_int(58);
    gveh_rotorcraft_thunder(&p->rotorcraft);
    p->rotorcraft.main_lift = gveh_fx_from_int(19500);
    p->rotorcraft.body_drag = GVEH_FX_ONE / 7;
    gveh_airassist_arcade(&p->airassist);
    gveh_airgame_arcade(&p->airgame);
    gveh_air_damage_arcade(&p->air_damage);
    gveh_avionics_arcade(&p->avionics);
    gveh_air_mission_strike(&p->air_mission);
    p->airgame.missile_max = 10;
    p->airgame.rocket_max = 24;
    p->camera.dist = gveh_fx_from_int(12);
    p->camera.height = gveh_fx_from_int(5);
}

void gveh_profile_longbow_campaign89(gveh_profile *p)
{
    gveh_profile_clear(p);
    gveh_copy_name(p->name, "longbow_campaign89");
    p->class_flags = GVEH_CLASS_ROTORCRAFT | GVEH_CLASS_SIMLITE;
    p->wheel_count = 0;
    p->mass = gveh_fx_from_int(5350);
    p->gravity = gveh_fx_from_int(24);
    p->max_speed = gveh_fx_from_int(42);
    gveh_rotorcraft_sim(&p->rotorcraft);
    gveh_airassist_sim(&p->airassist);
    gveh_airgame_strike(&p->airgame);
    gveh_air_damage_sim(&p->air_damage);
    gveh_avionics_sim(&p->avionics);
    gveh_wingman_sim(&p->wingman);
    gveh_air_mission_strike(&p->air_mission);
    p->airgame.missile_max = 16;
    p->airgame.rocket_max = 38;
    p->camera.dist = gveh_fx_from_int(16);
    p->camera.height = gveh_fx_from_int(5);
}

void gveh_profile_gunship_flight89(gveh_profile *p)
{
    gveh_profile_longbow_campaign89(p);
    gveh_copy_name(p->name, "gunship_flight89");
    p->wingman.count = 3;
    p->air_mission.destroy_need = 4;
    p->air_mission.rescue_need = 1;
    p->air_mission.protect_need = 1;
}

void gveh_profile_rogue_xwing89(gveh_profile *p)
{
    gveh_profile_clear(p);
    gveh_copy_name(p->name, "rogue_xwing89");
    p->class_flags = GVEH_CLASS_SPACECRAFT | GVEH_CLASS_ARCADE;
    p->wheel_count = 0;
    p->mass = gveh_fx_from_int(9000);
    p->gravity = 0;
    p->max_speed = gveh_fx_from_int(180);
    gveh_spacecraft_xwing(&p->spacecraft);
    gveh_avionics_space(&p->avionics);
    gveh_wingman_arcade(&p->wingman);
    gveh_air_mission_ace(&p->air_mission);
    gveh_airgame_arcade(&p->airgame);
    p->airgame.fuel_max = gveh_fx_from_int(9999);
    p->airgame.fuel_burn_idle = 0;
    p->airgame.fuel_burn_power = 0;
    p->camera.dist = gveh_fx_from_int(20);
    p->camera.height = gveh_fx_from_int(4);
}

void gveh_profile_tie_interceptor89(gveh_profile *p)
{
    gveh_profile_rogue_xwing89(p);
    gveh_copy_name(p->name, "tie_interceptor89");
    gveh_spacecraft_tie(&p->spacecraft);
    p->wingman.count = 2;
    p->camera.dist = gveh_fx_from_int(18);
}

void gveh_profile_battlefront_bomber89(gveh_profile *p)
{
    gveh_profile_rogue_xwing89(p);
    gveh_copy_name(p->name, "battlefront_bomber89");
    gveh_spacecraft_bomber(&p->spacecraft);
    p->air_mission.destroy_need = 6;
    p->air_mission.protect_need = 1;
    p->airgame.missile_max = 20;
    p->camera.dist = gveh_fx_from_int(24);
}


static void gveh_profile_tank4_base(gveh_profile *p)
{
    p->class_flags = GVEH_CLASS_TRACKED | GVEH_CLASS_TANKSIM | GVEH_CLASS_SIMLITE;
    p->wheel_count = 0;
    p->masspoint_count = 6;
    p->masspoints[0].local_pos = gveh_v3(-gveh_fx_from_int(14)/10, -GVEH_FX_HALF, gveh_fx_from_int(2));
    p->masspoints[1].local_pos = gveh_v3(gveh_fx_from_int(14)/10, -GVEH_FX_HALF, gveh_fx_from_int(2));
    p->masspoints[2].local_pos = gveh_v3(-gveh_fx_from_int(14)/10, -GVEH_FX_HALF, -gveh_fx_from_int(2));
    p->masspoints[3].local_pos = gveh_v3(gveh_fx_from_int(14)/10, -GVEH_FX_HALF, -gveh_fx_from_int(2));
    p->masspoints[4].local_pos = gveh_v3(0, 0, gveh_fx_from_int(3));
    p->masspoints[5].local_pos = gveh_v3(0, 0, -gveh_fx_from_int(3));
    p->camera.dist = gveh_fx_from_int(12);
    p->camera.height = gveh_fx_from_int(4);
    p->camera.lag = GVEH_FX_ONE / 5;
    p->nitro.force = 0;
    p->riskboost.boost_force = 0;
}

void gveh_profile_tank4_m1a1_89(gveh_profile *p)
{
    gveh_profile_clear(p);
    gveh_copy_name(p->name, "tank4_m1a1_89");
    gveh_profile_tank4_base(p);
    p->mass = gveh_fx_from_int(62000);
    p->gravity = gveh_fx_from_int(24);
    p->max_speed = gveh_fx_from_int(42);
    p->yaw_power = GVEH_FX_ONE * 9;
    gveh_tank4_modern(&p->tank4);
}

void gveh_profile_tank4_t72_89(gveh_profile *p)
{
    gveh_profile_clear(p);
    gveh_copy_name(p->name, "tank4_t72_89");
    gveh_profile_tank4_base(p);
    p->mass = gveh_fx_from_int(41000);
    p->max_speed = gveh_fx_from_int(38);
    gveh_tank4_t72(&p->tank4);
}

void gveh_profile_tank4_tiger_89(gveh_profile *p)
{
    gveh_profile_clear(p);
    gveh_copy_name(p->name, "tank4_tiger_89");
    gveh_profile_tank4_base(p);
    p->class_flags = GVEH_CLASS_TRACKED | GVEH_CLASS_TANKSIM;
    p->mass = gveh_fx_from_int(57000);
    p->max_speed = gveh_fx_from_int(28);
    gveh_tank4_tiger(&p->tank4);
}

void gveh_profile_tank4_sherman_89(gveh_profile *p)
{
    gveh_profile_clear(p);
    gveh_copy_name(p->name, "tank4_sherman_89");
    gveh_profile_tank4_base(p);
    p->class_flags = GVEH_CLASS_TRACKED | GVEH_CLASS_TANKSIM;
    p->mass = gveh_fx_from_int(33000);
    p->max_speed = gveh_fx_from_int(32);
    gveh_tank4_sherman(&p->tank4);
}

void gveh_profile_tank4_destroyer_89(gveh_profile *p)
{
    gveh_profile_tank4_tiger_89(p);
    gveh_copy_name(p->name, "tank4_destroyer_89");
    p->mass = gveh_fx_from_int(45000);
    gveh_tank4_destroyer(&p->tank4);
}

void gveh_profile_tank4_scout_89(gveh_profile *p)
{
    gveh_profile_clear(p);
    gveh_copy_name(p->name, "tank4_scout_89");
    gveh_profile_tank4_base(p);
    p->class_flags = GVEH_CLASS_TRACKED | GVEH_CLASS_TANKSIM | GVEH_CLASS_ARCADE;
    p->mass = gveh_fx_from_int(18000);
    p->max_speed = gveh_fx_from_int(55);
    p->velocity_retention = gveh_fx_from_int(94) / 100;
    gveh_tank4_scout(&p->tank4);
}
