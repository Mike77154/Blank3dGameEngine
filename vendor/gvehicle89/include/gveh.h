#ifndef GVEH_H
#define GVEH_H

#include "gveh_types.h"
#include "gveh_math.h"
#include "gveh_surface.h"
#include "gveh_world.h"
#include "gveh_contact.h"
#include "gveh_body.h"
#include "gveh_wheel.h"
#include "gveh_susp.h"
#include "gveh_tire.h"
#include "gveh_drivetrain.h"
#include "gveh_drive.h"
#include "gveh_masspoint.h"
#include "gveh_watercraft.h"
#include "gveh_assist.h"
#include "gveh_camera.h"
#include "gveh_ai.h"
#include "gveh_fx.h"
#include "gveh_tag.h"
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
#include "gveh_debug.h"
#include "vehicleprovider89.h"

typedef struct gveh_runtime_s {
    gveh_surface surfaces[GVEH_MAX_SURFACES];
    gveh_world_i world;
    gveh_i32 tick;
} gveh_runtime;

typedef struct gveh_vehicle_s {
    gveh_profile profile;
    gveh_body body;
    gveh_camera camera;
    gveh_fx_queue fxq;
    gveh_fx speed_forward;
    gveh_fx speed_side;
    gveh_fx prev_speed_forward;
    gveh_fx grounded_ratio;
    gveh_i32 grounded_count;
    gveh_nitro_state nitro;
    gveh_kudos_state kudos;
    gveh_skill_state skill;
    gveh_riskboost_state riskboost;
    gveh_takedown_state takedown;
    gveh_aftertouch_state aftertouch;
    gveh_crashbreaker_state crashbreaker;
    gveh_airframe_state airframe;
    gveh_rotorcraft_state rotorcraft;
    gveh_airgame_state airgame;
    gveh_air_damage_state air_damage;
    gveh_avionics_state avionics;
    gveh_wingman_state wingman;
    gveh_air_mission_state air_mission;
    gveh_spacecraft_state spacecraft;
    gveh_tank4_state tank4;
    vehicleprovider89_movement movement_provider;
    vehicleprovider89_physics physics_provider;
} gveh_vehicle;

void gveh_runtime_init(gveh_runtime *rt);
void gveh_runtime_set_world(gveh_runtime *rt, gveh_world_i world);
void gveh_vehicle_init(gveh_vehicle *v, const gveh_profile *profile, gveh_vec3 pos);
void gveh_vehicle_step(gveh_runtime *rt, gveh_vehicle *v, const gveh_input *input, gveh_fx dt);
void gveh_vehicle_set_movement_provider(
    gveh_vehicle *v, const vehicleprovider89_movement *provider);
void gveh_vehicle_set_physics_provider(
    gveh_vehicle *v, const vehicleprovider89_physics *provider);
void gveh_vehicle_clear_providers(gveh_vehicle *v);

#endif
