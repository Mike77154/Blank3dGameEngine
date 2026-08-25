#ifndef GVEH_TAG_H
#define GVEH_TAG_H

#include "gveh_body.h"
#include "gveh_wheel.h"
#include "gveh_susp.h"
#include "gveh_tire.h"
#include "gveh_drivetrain.h"
#include "gveh_masspoint.h"
#include "gveh_watercraft.h"
#include "gveh_assist.h"
#include "gveh_camera.h"
#include "gveh_ai.h"
#include "gveh_nitro.h"
#include "gveh_kudos.h"
#include "gveh_skill.h"
#include "gveh_tuning.h"
#include "gveh_driver_profile.h"
#include "gveh_riskboost.h"
#include "gveh_takedown.h"
#include "gveh_aftertouch.h"
#include "gveh_crashbreaker.h"
#include "gveh_airframe.h"
#include "gveh_rotorcraft.h"
#include "gveh_airassist.h"
#include "gveh_airgame.h"
#include "gveh_air_damage.h"
#include "gveh_avionics.h"
#include "gveh_wingman.h"
#include "gveh_air_mission.h"
#include "gveh_spacecraft.h"
#include "gveh_tank4.h"

#define GVEH_CLASS_ARCADE      1u
#define GVEH_CLASS_SIMLITE     2u
#define GVEH_CLASS_RALLY       4u
#define GVEH_CLASS_HALOLIKE    8u
#define GVEH_CLASS_WATER       16u
#define GVEH_CLASS_TRACKED     32u
#define GVEH_CLASS_HOVER       64u
#define GVEH_CLASS_AIRPLANE    128u
#define GVEH_CLASS_ROTORCRAFT  256u
#define GVEH_CLASS_SPACECRAFT   512u
#define GVEH_CLASS_VTOL         1024u
#define GVEH_CLASS_TANKSIM      2048u

#define GVEH_MODULE_CAR       1u
#define GVEH_MODULE_WATER     2u
#define GVEH_MODULE_AIR       4u
#define GVEH_MODULE_SPACE     8u
#define GVEH_MODULE_TANK      16u
#define GVEH_MODULE_ARCADE    32u

typedef struct gveh_profile_s {
    char name[32];
    gveh_u16 class_flags;
    gveh_u16 module_flags;
    gveh_fx collision_radius;
    gveh_fx mass;
    gveh_fx gravity;
    gveh_fx max_speed;
    gveh_fx velocity_retention;
    gveh_fx steer_rate;
    gveh_fx yaw_power;
    gveh_fx slide_power;
    gveh_fx air_yaw_power;
    gveh_susp susp;
    gveh_tire_curve tire;
    gveh_drivetrain drive;
    gveh_assist assists;
    gveh_nitro_cfg nitro;
    gveh_kudos_cfg kudos;
    gveh_skill_cfg skill;
    gveh_tuning tuning;
    gveh_driver_profile driver_profile;
    gveh_riskboost_cfg riskboost;
    gveh_takedown_cfg takedown;
    gveh_aftertouch_cfg aftertouch;
    gveh_crashbreaker_cfg crashbreaker;
    gveh_airframe_cfg airframe;
    gveh_rotorcraft_cfg rotorcraft;
    gveh_airassist_cfg airassist;
    gveh_airgame_cfg airgame;
    gveh_air_damage_cfg air_damage;
    gveh_avionics_cfg avionics;
    gveh_wingman_cfg wingman;
    gveh_air_mission_cfg air_mission;
    gveh_spacecraft_cfg spacecraft;
    gveh_tank4_cfg tank4;
    gveh_camera camera;
    gveh_wheel wheels[GVEH_MAX_WHEELS];
    gveh_u8 wheel_count;
    gveh_masspoint masspoints[GVEH_MAX_MASSPTS];
    gveh_u8 masspoint_count;
    gveh_buoy buoys[GVEH_MAX_BUOYS];
    gveh_u8 buoy_count;
    gveh_seat seats[GVEH_MAX_SEATS];
    gveh_u8 seat_count;
} gveh_profile;

void gveh_profile_clear(gveh_profile *p);
void gveh_profile_refresh_module_flags(gveh_profile *p);
void gveh_profile_warthog89(gveh_profile *p);
void gveh_profile_gt_sport89(gveh_profile *p);
void gveh_profile_rally89(gveh_profile *p);
void gveh_profile_daytona89(gveh_profile *p);
void gveh_profile_wave_jetski89(gveh_profile *p);
void gveh_profile_tank_lite89(gveh_profile *p);
void gveh_profile_thunder_chopper89(gveh_profile *p);
void gveh_profile_strike_chopper89(gveh_profile *p);
void gveh_profile_sim_heli89(gveh_profile *p);
void gveh_profile_ace_fighter89(gveh_profile *p);
void gveh_profile_fs_lightplane89(gveh_profile *p);
void gveh_profile_comanche_voxel89(gveh_profile *p);
void gveh_profile_longbow_campaign89(gveh_profile *p);
void gveh_profile_gunship_flight89(gveh_profile *p);
void gveh_profile_rogue_xwing89(gveh_profile *p);
void gveh_profile_tie_interceptor89(gveh_profile *p);
void gveh_profile_battlefront_bomber89(gveh_profile *p);
void gveh_profile_tank4_m1a1_89(gveh_profile *p);
void gveh_profile_tank4_t72_89(gveh_profile *p);
void gveh_profile_tank4_tiger_89(gveh_profile *p);
void gveh_profile_tank4_sherman_89(gveh_profile *p);
void gveh_profile_tank4_destroyer_89(gveh_profile *p);
void gveh_profile_tank4_scout_89(gveh_profile *p);

#endif
