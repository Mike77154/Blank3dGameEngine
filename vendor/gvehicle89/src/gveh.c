#include "gveh.h"
#include "vehiclephysics89.h"
#include "carmovement89.h"
#include "tankmovement89.h"
#include "watermovement89.h"
#include "airmovement89.h"
#include "rotorcraftmovement89.h"
#include "spacemovement89.h"
#include <string.h>

static gveh_i32 gveh_try_movement_provider(gveh_runtime *rt,
                                            gveh_vehicle *v,
                                            const gveh_input *input,
                                            gveh_basis basis,
                                            gveh_fx dt,
                                            gveh_u32 module)
{
    vehicleprovider89_movement_request request;
    request.runtime = rt;
    request.vehicle = v;
    request.input = input;
    request.basis = basis;
    request.dt = dt;
    request.module = module;
    return vehicleprovider89_movement_try(&v->movement_provider, &request);
}

static gveh_i32 gveh_try_physics_provider(gveh_runtime *rt,
                                           gveh_vehicle *v,
                                           const gveh_input *input,
                                           gveh_basis basis,
                                           gveh_fx dt,
                                           gveh_u32 phase)
{
    vehicleprovider89_physics_request request;
    request.runtime = rt;
    request.vehicle = v;
    request.input = input;
    request.basis = basis;
    request.dt = dt;
    request.phase = phase;
    return vehicleprovider89_physics_try(&v->physics_provider, &request);
}

void gveh_runtime_init(gveh_runtime *rt)
{
    gveh_surface_defaults(rt->surfaces);
    rt->world.user = 0;
    rt->world.ground_probe = gveh_demo_ground_probe;
    rt->world.sphere_probe = gveh_demo_sphere_probe;
    rt->world.water_sample = gveh_demo_water_sample;
    rt->tick = 0;
}

void gveh_runtime_set_world(gveh_runtime *rt, gveh_world_i world)
{
    rt->world = world;
}

void gveh_vehicle_init(gveh_vehicle *v, const gveh_profile *profile, gveh_vec3 pos)
{
    memset(v, 0, sizeof(*v));
    vehicleprovider89_movement_clear(&v->movement_provider);
    vehicleprovider89_physics_clear(&v->physics_provider);
    v->profile = *profile;
    gveh_profile_refresh_module_flags(&v->profile);
    gveh_body_clear(&v->body);
    gveh_body_set_mass(&v->body, profile->mass);
    v->body.pos = pos;
    v->camera = profile->camera;
    v->camera.pos = gveh_v3(pos.x, pos.y + gveh_fx_from_int(3), pos.z - gveh_fx_from_int(8));
    gveh_fx_clear(&v->fxq);
    gveh_nitro_init(&v->nitro, &v->profile.nitro);
    gveh_kudos_clear(&v->kudos);
    gveh_skill_clear(&v->skill);
    gveh_riskboost_clear(&v->riskboost);
    gveh_takedown_clear(&v->takedown);
    gveh_aftertouch_clear(&v->aftertouch);
    gveh_crashbreaker_clear(&v->crashbreaker, &v->profile.crashbreaker);
    gveh_airframe_clear(&v->airframe);
    gveh_rotorcraft_clear(&v->rotorcraft);
    gveh_airgame_clear(&v->airgame, &v->profile.airgame);
    gveh_air_damage_clear(&v->air_damage, &v->profile.air_damage);
    gveh_avionics_clear(&v->avionics);
    gveh_wingman_clear(&v->wingman, &v->profile.wingman);
    gveh_air_mission_clear(&v->air_mission);
    gveh_spacecraft_clear(&v->spacecraft, &v->profile.spacecraft);
    gveh_tank4_clear(&v->tank4, &v->profile.tank4);
}


void gveh_vehicle_set_movement_provider(
    gveh_vehicle *v, const vehicleprovider89_movement *provider)
{
    if (!v) return;
    if (provider) v->movement_provider = *provider;
    else vehicleprovider89_movement_clear(&v->movement_provider);
}

void gveh_vehicle_set_physics_provider(
    gveh_vehicle *v, const vehicleprovider89_physics *provider)
{
    if (!v) return;
    if (provider) v->physics_provider = *provider;
    else vehicleprovider89_physics_clear(&v->physics_provider);
}

void gveh_vehicle_clear_providers(gveh_vehicle *v)
{
    if (!v) return;
    vehicleprovider89_movement_clear(&v->movement_provider);
    vehicleprovider89_physics_clear(&v->physics_provider);
}


void gveh_vehicle_step(gveh_runtime *rt, gveh_vehicle *v, const gveh_input *input, gveh_fx dt)
{
    gveh_basis b;
    gveh_fx yaw_air;
    gveh_input driver_input;
    gveh_input sim_input;
    gveh_fx speed_abs;
    gveh_fx side_abs;
    gveh_fx nitro_force;
    gveh_fx risk_force;
    gveh_fx total_boost;
    gveh_fx engine_power;
    gveh_i32 takedown_now;
    gveh_i32 crashbreaker_now;
    gveh_fx new_speed;
    gveh_fx damage_impact;
    gveh_i16 fired_now;
    gveh_i32 external_tank;

    gveh_fx_clear(&v->fxq);
    external_tank = 0;
    b = gveh_basis_yaw_pitch_roll(v->body.yaw, v->body.pitch, v->body.roll);
    v->prev_speed_forward = v->speed_forward;
    v->speed_forward = gveh_v3_dot(v->body.vel, b.fwd);
    v->speed_side = gveh_v3_dot(v->body.vel, b.right);
    speed_abs = gveh_fx_abs(v->speed_forward);
    side_abs = gveh_fx_abs(v->speed_side);

    gveh_driver_profile_apply(&v->profile.driver_profile, input, &driver_input, rt->tick);
    gveh_skill_apply_input(&v->skill, &v->profile.skill, &driver_input, &sim_input, speed_abs);

    if (gveh_try_physics_provider(rt, v, &sim_input, b, dt,
            VEHICLEPROVIDER89_PHYS_GRAVITY) != VEHICLEPROVIDER89_HANDLED) {
        vehiclephysics89_apply_gravity(&v->body, v->profile.mass, v->profile.gravity);
    }

    if (v->airgame.empty != 0 || v->air_damage.engine_dead != 0) {
        sim_input.throttle = 0;
        sim_input.collective = 0;
        sim_input.nitro = 0;
    }
    if (v->air_damage.control_loss > 0) {
        sim_input.air_pitch = gveh_fx_mul(sim_input.air_pitch, GVEH_FX_ONE - v->air_damage.control_loss);
        sim_input.air_roll = gveh_fx_mul(sim_input.air_roll, GVEH_FX_ONE - v->air_damage.control_loss);
        sim_input.air_yaw = gveh_fx_mul(sim_input.air_yaw, GVEH_FX_ONE - v->air_damage.control_loss);
    }

#if GVEH_ENABLE_MODULE_CAR
    if ((v->profile.module_flags & GVEH_MODULE_CAR) != 0u) {
        if (gveh_try_movement_provider(rt, v, &sim_input, b, dt,
                VEHICLEPROVIDER89_MODULE_CAR) != VEHICLEPROVIDER89_HANDLED) {
            carmovement89_step(rt->surfaces, &rt->world, &v->profile, &v->body, b, &sim_input, dt, &v->grounded_count, &v->grounded_ratio, &v->fxq);
        }
    }
#endif
#if GVEH_ENABLE_MODULE_TANK
    if ((v->profile.module_flags & GVEH_MODULE_TANK) != 0u) {
        external_tank = gveh_try_movement_provider(rt, v, &sim_input, b, dt,
            VEHICLEPROVIDER89_MODULE_TANK) == VEHICLEPROVIDER89_HANDLED;
        if (!external_tank) {
            tankmovement89_step(&v->profile.tank4, &v->tank4, &v->body, b, &sim_input, v->grounded_ratio, speed_abs, dt, rt->tick, &v->fxq);
        }
    }
#endif
#if GVEH_ENABLE_MODULE_WATER
    if ((v->profile.module_flags & GVEH_MODULE_WATER) != 0u) {
        if (gveh_try_movement_provider(rt, v, &sim_input, b, dt,
                VEHICLEPROVIDER89_MODULE_WATER) != VEHICLEPROVIDER89_HANDLED) {
            watermovement89_step(&rt->world, &v->profile, &v->body, b, &sim_input, rt->tick, dt, &v->fxq);
        }
    }
#endif
    if (gveh_try_physics_provider(rt, v, &sim_input, b, dt,
            VEHICLEPROVIDER89_PHYS_MASSPOINTS) != VEHICLEPROVIDER89_HANDLED) {
        vehiclephysics89_apply_masspoints(&v->body, v->profile.masspoints, v->profile.masspoint_count, &rt->world, b, v->profile.gravity, dt);
    }
#if GVEH_ENABLE_MODULE_AIR
    if ((v->profile.module_flags & GVEH_MODULE_AIR) != 0u) {
        if (gveh_try_movement_provider(rt, v, &sim_input, b, dt,
                VEHICLEPROVIDER89_MODULE_AIR) != VEHICLEPROVIDER89_HANDLED) {
            airmovement89_step(&v->profile.airframe, &v->airframe, &v->body, b, &sim_input, dt);
            rotorcraftmovement89_step(&v->profile.rotorcraft, &v->rotorcraft, &v->body, b, &sim_input, dt);
            airmovement89_apply_assist(&v->profile.airassist, &v->body, &v->airframe, &v->rotorcraft, &sim_input);
        }
        engine_power = gveh_fx_abs(sim_input.throttle);
        if (gveh_fx_abs(sim_input.collective) > engine_power) engine_power = gveh_fx_abs(sim_input.collective);
        gveh_airgame_step(&v->airgame, &v->profile.airgame, &sim_input, &v->body, b, engine_power, dt, &v->fxq);
        gveh_avionics_step(&v->avionics, &v->profile.avionics, &sim_input, speed_abs, rt->tick, &v->fxq);
    }
#endif
#if GVEH_ENABLE_MODULE_SPACE
    if ((v->profile.module_flags & GVEH_MODULE_SPACE) != 0u) {
        if (gveh_try_movement_provider(rt, v, &sim_input, b, dt,
                VEHICLEPROVIDER89_MODULE_SPACE) != VEHICLEPROVIDER89_HANDLED) {
            spacemovement89_step(&v->spacecraft, &v->profile.spacecraft, &v->body, b, &sim_input, dt, &v->fxq);
        }
    }
#endif

#if GVEH_ENABLE_MODULE_ARCADE
    nitro_force = gveh_nitro_step(&v->nitro, &v->profile.nitro, sim_input.nitro, speed_abs, GVEH_FX_ONE, v->grounded_count, dt);
    risk_force = gveh_riskboost_step(&v->riskboost, &v->profile.riskboost, sim_input.nitro, speed_abs, side_abs, v->grounded_ratio, sim_input.risk, 0, dt);
#else
    nitro_force = 0;
    risk_force = 0;
#endif
    total_boost = nitro_force + risk_force;
    if (total_boost != 0) {
        gveh_body_add_force(&v->body, gveh_v3_scale(b.fwd, total_boost));
        gveh_fx_push(&v->fxq, GVEH_FX_EVENT_ENGINE, (gveh_i16)gveh_fx_to_int(total_boost / 64), v->body.pos.x, v->body.pos.y, v->body.pos.z);
    }

    gveh_aftertouch_apply(&v->aftertouch, &v->profile.aftertouch, &v->body, b, sim_input.aftertouch_x, sim_input.aftertouch_z);
    crashbreaker_now = gveh_crashbreaker_fire(&v->crashbreaker, &v->profile.crashbreaker, &v->body, b, sim_input.crashbreak);
    if (crashbreaker_now != 0) {
        gveh_fx_push(&v->fxq, GVEH_FX_EVENT_IMPACT, 99, v->body.pos.x, v->body.pos.y, v->body.pos.z);
    }

    if (v->grounded_count == 0) {
        yaw_air = gveh_fx_mul(sim_input.steer, v->profile.air_yaw_power * 40);
        gveh_body_add_yaw_torque(&v->body, yaw_air);
    }

    if (gveh_try_physics_provider(rt, v, &sim_input, b, dt,
            VEHICLEPROVIDER89_PHYS_LINEAR_DRAG) != VEHICLEPROVIDER89_HANDLED) {
        vehiclephysics89_apply_linear_drag(&v->body, v->speed_forward);
    }
    if (gveh_try_physics_provider(rt, v, &sim_input, b, dt,
            VEHICLEPROVIDER89_PHYS_INTEGRATE) != VEHICLEPROVIDER89_HANDLED) {
        vehiclephysics89_integrate_with_retention(&v->body, dt,
                                                   v->profile.velocity_retention);
    }
    if (!external_tank)
        tankmovement89_post_integrate(&v->body, v->profile.class_flags);
    b = gveh_basis_yaw_pitch_roll(v->body.yaw, v->body.pitch, v->body.roll);
    new_speed = gveh_v3_dot(v->body.vel, b.fwd);
    takedown_now = gveh_takedown_step(&v->takedown, &v->profile.takedown, v->speed_forward, new_speed, v->speed_side, sim_input.aggression);
    damage_impact = v->takedown.last_impact;
    if (v->spacecraft.fired != 0) fired_now = 1;
    else fired_now = (sim_input.fire > 0) ? 1 : 0;
    gveh_air_damage_step(&v->air_damage, &v->profile.air_damage, &v->body, &v->airgame, damage_impact, v->avionics.threat_warn > 0 ? GVEH_FX_ONE : 0, dt);
    gveh_wingman_step(&v->wingman, &v->profile.wingman, &sim_input, v->avionics.locked, v->avionics.threat_warn);
    gveh_air_mission_step(&v->air_mission, &v->profile.air_mission, &sim_input, v->avionics.locked, fired_now, v->airgame.cargo_count, takedown_now);
    if (takedown_now != 0) {
        gveh_riskboost_add(&v->riskboost, &v->profile.riskboost, v->profile.takedown.reward_boost);
        gveh_nitro_add(&v->nitro, &v->profile.nitro, v->profile.takedown.reward_boost / 2);
        gveh_fx_push(&v->fxq, GVEH_FX_EVENT_IMPACT, 80, v->body.pos.x, v->body.pos.y, v->body.pos.z);
    }
    gveh_aftertouch_trigger(&v->aftertouch, &v->profile.aftertouch, v->takedown.last_impact);
    gveh_crashbreaker_charge(&v->crashbreaker, &v->profile.crashbreaker, v->takedown.last_impact, sim_input.risk);
    v->speed_forward = new_speed;
    v->speed_side = gveh_v3_dot(v->body.vel, b.right);
    gveh_kudos_step(&v->kudos, &v->profile.kudos, v->speed_forward, v->speed_side, v->grounded_ratio, sim_input.risk, takedown_now, dt);
    gveh_skill_step(&v->skill, &v->profile.skill, gveh_fx_abs(v->speed_forward), gveh_fx_abs(v->speed_side), v->grounded_ratio, dt);
    gveh_camera_update(&v->camera, v->body.pos, b, v->speed_forward, dt);
    rt->tick++;
}
