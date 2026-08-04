#include "blank3d_perception.h"

#include <string.h>

static EAI_Fixed b3d_q12_to_q16(g3d_fix value)
{
    if (value > (g3d_fix)134217727L) return EAI_FX_MAX;
    if (value < (g3d_fix)(-134217728L)) return EAI_FX_MIN;
    return (EAI_Fixed)(value * 16L);
}

static g3d_fix b3d_q16_to_q12(EAI_Fixed value)
{
    return (g3d_fix)(value / 16L);
}

static long b3d_q12_to_units(g3d_fix value)
{
    return (long)(value / G3D_FIX_ONE);
}

static g3d_fix b3d_units_to_q12(long value)
{
    if (value > 524287L) value = 524287L;
    if (value < -524288L) value = -524288L;
    return (g3d_fix)(value * G3D_FIX_ONE);
}

static EAI_Vec3 b3d_vec3_to_eai(Vec3 value)
{
    EAI_Vec3 result;
    result.x = b3d_q12_to_q16(value.x);
    result.y = b3d_q12_to_q16(value.y);
    result.z = b3d_q12_to_q16(value.z);
    return result;
}

static Vec3 b3d_eai_to_vec3(EAI_Vec3 value)
{
    Vec3 result;
    result.x = b3d_q16_to_q12(value.x);
    result.y = b3d_q16_to_q12(value.y);
    result.z = b3d_q16_to_q12(value.z);
    return result;
}

static Vec3 b3d_pose_position_q12(const soq3d_pose *pose)
{
    Vec3 result;
    result.x = (g3d_fix)(pose->position.x / 16L);
    result.y = (g3d_fix)(pose->position.y / 16L);
    result.z = (g3d_fix)(pose->position.z / 16L);
    return result;
}

static Vec3 b3d_pose_forward_q12(const soq3d_pose *pose)
{
    Vec3 result;
    result.x = (g3d_fix)(pose->basis.m02 / 16L);
    result.y = (g3d_fix)(pose->basis.m12 / 16L);
    result.z = (g3d_fix)(pose->basis.m22 / 16L);
    if (gamlib_vec3_length(&result) <= G3D_FIX_EPSILON)
        result = gamlib_vec3(0, 0, G3D_FIX_ONE);
    else
        gamlib_vec3_normalize(&result, &result);
    return result;
}

static tdne_vec3 b3d_vec3_to_tdne(Vec3 value)
{
    tdne_vec3 result;
    result.x = (tdne_i32)b3d_q12_to_units(value.x);
    result.y = (tdne_i32)b3d_q12_to_units(value.y);
    result.z = (tdne_i32)b3d_q12_to_units(value.z);
    return result;
}

static Vec3 b3d_tdne_to_vec3(tdne_vec3 value)
{
    Vec3 result;
    result.x = b3d_units_to_q12((long)value.x);
    result.y = b3d_units_to_q12((long)value.y);
    result.z = b3d_units_to_q12((long)value.z);
    return result;
}

static int b3d_eai_raycast_world(void *user,
                                 const EAI_Vec3 *from,
                                 const EAI_Vec3 *to,
                                 EAI_U32 mask)
{
    Blank3DPerceptionWorld *world;
    Vec3 a;
    Vec3 b;
    world = (Blank3DPerceptionWorld *)user;
    if (!world || !world->raycast || !from || !to) return 0;
    a = b3d_eai_to_vec3(*from);
    b = b3d_eai_to_vec3(*to);
    return world->raycast(world->raycast_user, &a, &b,
                          (unsigned long)mask);
}

static int b3d_tdne_raycast(void *world_user,
                            const tdne_vec3 *from,
                            const tdne_vec3 *to,
                            tdne_u32 block_mask,
                            tdne_ray_hit *out_hit)
{
    Blank3DPerceptionWorld *world;
    Vec3 a;
    Vec3 b;
    int blocked;
    world = (Blank3DPerceptionWorld *)world_user;
    if (out_hit) memset(out_hit, 0, sizeof(*out_hit));
    if (!world || !world->raycast || !from || !to) return 0;
    a = b3d_tdne_to_vec3(*from);
    b = b3d_tdne_to_vec3(*to);
    blocked = world->raycast(world->raycast_user, &a, &b,
                             (unsigned long)block_mask);
    if (out_hit) out_hit->hit = blocked ? TDNE_TRUE : TDNE_FALSE;
    return blocked;
}

void blank3d_perception_config_defaults(Blank3DPerceptionConfig *config)
{
    if (!config) return;
    memset(config, 0, sizeof(*config));
    config->eye_shape = B3D_EYE_SHAPE_CONE;
    config->view_range = G3D_FIX_FROM_INT(28);
    config->horizontal_fov_degrees = 100;
    config->vertical_fov_degrees = 70;
    config->require_line_of_sight = 1;
    config->hearing_range = G3D_FIX_FROM_INT(22);
    config->target_radius = G3D_FIX_FROM_INT(1);
    config->target_half_height = G3D_FIX_FROM_INT(1);
    config->see_mask = TDNE_MASK_ALL;
    config->block_mask = 1UL;
}

void blank3d_perception_world_init(Blank3DPerceptionWorld *world,
                                   Blank3DPerceptionRaycastFn raycast,
                                   void *raycast_user)
{
    int i;
    if (!world) return;
    memset(world, 0, sizeof(*world));
    world->raycast = raycast;
    world->raycast_user = raycast_user;
    world->world_ops.raycast_world = b3d_eai_raycast_world;
    world->world_ops.raycast_entity = 0;
    world->world_ops.edge_cost = 0;
    eai_init(&world->enlightener, &world->world_ops, world);
    world->player_entity_id = eai_entity_create(&world->enlightener);
    if (world->player_entity_id != EAI_INVALID_ID)
        eai_entity_set_team(&world->enlightener,
                            world->player_entity_id, 1u);
    for (i = 0; i < B3D_PERCEPTION_MAX_AGENTS; ++i)
        world->agents[i].eai_self_id = EAI_INVALID_ID;
    world->initialized = 1;
}

int blank3d_perception_agent_init(Blank3DPerceptionWorld *world,
                                  int agent_index,
                                  soq3d_key self_key,
                                  soq3d_key target_key,
                                  unsigned char self_team,
                                  unsigned char target_team)
{
    Blank3DPerceptionAgent *agent;
    EAI_EntityId id;
    if (!world || !world->initialized || agent_index < 0 ||
        agent_index >= B3D_PERCEPTION_MAX_AGENTS) return 0;
    agent = &world->agents[agent_index];
    if (agent->eai_self_id != EAI_INVALID_ID)
        eai_entity_destroy(&world->enlightener, agent->eai_self_id);
    memset(agent, 0, sizeof(*agent));
    blank3d_perception_config_defaults(&agent->config);
    id = eai_entity_create(&world->enlightener);
    if (id == EAI_INVALID_ID) {
        agent->eai_self_id = EAI_INVALID_ID;
        return 0;
    }
    agent->active = 1;
    agent->self_key = self_key;
    agent->target_key = target_key;
    agent->eai_self_id = id;
    agent->eai_target_id = world->player_entity_id;
    eai_entity_set_team(&world->enlightener, id, self_team);
    eai_targeting_set_hostile(&world->enlightener,
                              (EAI_U8)self_team,
                              (EAI_U8)target_team, EAI_TRUE);
    return 1;
}

void blank3d_perception_agent_disable(Blank3DPerceptionWorld *world,
                                      int agent_index)
{
    Blank3DPerceptionAgent *agent;
    if (!world || agent_index < 0 ||
        agent_index >= B3D_PERCEPTION_MAX_AGENTS) return;
    agent = &world->agents[agent_index];
    if (agent->eai_self_id != EAI_INVALID_ID)
        eai_entity_destroy(&world->enlightener, agent->eai_self_id);
    memset(agent, 0, sizeof(*agent));
    agent->eai_self_id = EAI_INVALID_ID;
}

Blank3DPerceptionAgent *blank3d_perception_agent(
    Blank3DPerceptionWorld *world, int agent_index)
{
    if (!world || agent_index < 0 ||
        agent_index >= B3D_PERCEPTION_MAX_AGENTS) return 0;
    return &world->agents[agent_index];
}

const Blank3DPerceptionAgent *blank3d_perception_agent_const(
    const Blank3DPerceptionWorld *world, int agent_index)
{
    if (!world || agent_index < 0 ||
        agent_index >= B3D_PERCEPTION_MAX_AGENTS) return 0;
    return &world->agents[agent_index];
}

int blank3d_perception_set_eye_shape(Blank3DPerceptionAgent *agent,
                                     const char *shape_name)
{
    if (!agent || !shape_name) return 0;
    if (strcmp(shape_name, "cone") == 0) agent->config.eye_shape = B3D_EYE_SHAPE_CONE;
    else if (strcmp(shape_name, "sphere") == 0) agent->config.eye_shape = B3D_EYE_SHAPE_SPHERE;
    else if (strcmp(shape_name, "box") == 0) agent->config.eye_shape = B3D_EYE_SHAPE_BOX;
    else if (strcmp(shape_name, "frustum") == 0) agent->config.eye_shape = B3D_EYE_SHAPE_FRUSTUM;
    else return 0;
    return 1;
}

const char *blank3d_perception_eye_shape_name(int eye_shape)
{
    if (eye_shape == B3D_EYE_SHAPE_SPHERE) return "sphere";
    if (eye_shape == B3D_EYE_SHAPE_BOX) return "box";
    if (eye_shape == B3D_EYE_SHAPE_FRUSTUM) return "frustum";
    return "cone";
}

void blank3d_perception_set_view_range(Blank3DPerceptionAgent *agent,
                                       g3d_fix range)
{
    if (!agent) return;
    agent->config.view_range = range < 0 ? 0 : range;
}

void blank3d_perception_set_fov(Blank3DPerceptionAgent *agent,
                                int horizontal_degrees,
                                int vertical_degrees)
{
    if (!agent) return;
    if (horizontal_degrees < 1) horizontal_degrees = 1;
    if (horizontal_degrees > 179) horizontal_degrees = 179;
    if (vertical_degrees < 1) vertical_degrees = 1;
    if (vertical_degrees > 179) vertical_degrees = 179;
    agent->config.horizontal_fov_degrees = horizontal_degrees;
    agent->config.vertical_fov_degrees = vertical_degrees;
}

void blank3d_perception_set_hearing_range(Blank3DPerceptionAgent *agent,
                                          g3d_fix range)
{
    if (!agent) return;
    agent->config.hearing_range = range < 0 ? 0 : range;
}

void blank3d_perception_set_line_of_sight(Blank3DPerceptionAgent *agent,
                                          int required)
{
    if (!agent) return;
    agent->config.require_line_of_sight = required ? 1 : 0;
}

void blank3d_perception_set_target_key(Blank3DPerceptionAgent *agent,
                                       soq3d_key target_key,
                                       EAI_EntityId eai_target_id)
{
    if (!agent) return;
    if (agent->target_key != target_key ||
        agent->eai_target_id != eai_target_id) {
        agent->target_known = 0;
        agent->target_visible = 0;
        agent->target_partial = 0;
        agent->target_occluded = 0;
        agent->target_out_of_range = 0;
        agent->target_out_of_shape = 0;
        agent->target_heard = 0;
        agent->target_remembered = 0;
        agent->target_inferred = 0;
        agent->fallback_active = 0;
        agent->truth_sources = 0UL;
        agent->truth_count = 0U;
        agent->truth_tier = B3D_TRUTH_TIER_NONE;
        agent->action_confidence = 0;
        agent->visible_time = 0;
        agent->lost_time = 0;
        agent->alertness_percent = 0;
        agent->suspicion_percent = 0;
        memset(&agent->eye_result, 0, sizeof(agent->eye_result));
        memset(&agent->last_seen_position, 0,
               sizeof(agent->last_seen_position));
        memset(&agent->last_heard_position, 0,
               sizeof(agent->last_heard_position));
    }
    agent->target_key = target_key;
    agent->eai_target_id = eai_target_id;
}

void blank3d_perception_sync_player(Blank3DPerceptionWorld *world,
                                    const soq3d_context *locator,
                                    soq3d_stamp stamp,
                                    soq3d_key player_key,
                                    int player_alive)
{
    soq3d_pose pose;
    Vec3 position;
    Vec3 forward;
    EAI_Vec3 epos;
    EAI_Vec3 efwd;
    if (!world || !locator || world->player_entity_id == EAI_INVALID_ID) return;
    if (soq3d_get_thing(locator, player_key, stamp, &pose) != SOQ3D_OK) return;
    position = b3d_pose_position_q12(&pose);
    forward = b3d_pose_forward_q12(&pose);
    epos = b3d_vec3_to_eai(position);
    efwd = b3d_vec3_to_eai(forward);
    eai_entity_set_position(&world->enlightener,
                            world->player_entity_id, &epos);
    eai_entity_set_forward(&world->enlightener,
                           world->player_entity_id, &efwd);
    if (player_alive)
        world->enlightener.entities[world->player_entity_id].flags &=
            (EAI_U8)~EAI_ENTITY_FLAG_DISABLED;
    else
        world->enlightener.entities[world->player_entity_id].flags |=
            EAI_ENTITY_FLAG_DISABLED;
}

static void b3d_init_sensor(Blank3DPerceptionAgent *agent,
                            Vec3 origin,
                            Vec3 forward,
                            Vec3 right,
                            Vec3 up)
{
    tdne_vec3 td_origin;
    tdne_vec3 td_forward;
    tdne_vec3 td_right;
    tdne_vec3 td_up;
    long range;
    long half;
    td_origin = b3d_vec3_to_tdne(origin);
    td_forward = b3d_vec3_to_tdne(forward);
    td_right = b3d_vec3_to_tdne(right);
    td_up = b3d_vec3_to_tdne(up);
    range = b3d_q12_to_units(agent->config.view_range);
    if (range < 0L) range = 0L;
    if (agent->config.eye_shape == B3D_EYE_SHAPE_SPHERE) {
        tdne_sensor_init_sphere(&agent->eyes, td_origin, range);
    } else if (agent->config.eye_shape == B3D_EYE_SHAPE_BOX) {
        half = range;
        tdne_sensor_init_box(&agent->eyes, td_origin,
            tdne_vec3_make(half, half, half));
    } else if (agent->config.eye_shape == B3D_EYE_SHAPE_FRUSTUM) {
        tdne_sensor_init_frustum(&agent->eyes, td_origin, td_forward,
            td_right, td_up, 0L, range,
            agent->config.horizontal_fov_degrees,
            agent->config.vertical_fov_degrees);
    } else {
        tdne_sensor_init_cone(&agent->eyes, td_origin, td_forward,
            range, agent->config.horizontal_fov_degrees);
    }
    tdne_sensor_set_masks(&agent->eyes,
                          (tdne_u32)agent->config.see_mask,
                          (tdne_u32)agent->config.block_mask);
    tdne_sensor_set_line_of_sight(&agent->eyes,
                                  agent->config.require_line_of_sight);
}

void blank3d_perception_sync_agent(Blank3DPerceptionWorld *world,
                                   int agent_index,
                                   const soq3d_context *locator,
                                   soq3d_stamp stamp,
                                   int self_alive,
                                   int target_alive)
{
    Blank3DPerceptionAgent *agent;
    soq3d_pose self_pose;
    soq3d_pose target_pose;
    Vec3 forward;
    Vec3 right;
    Vec3 up;
    Vec3 delta;
    tdne_target target;
    EAI_Vec3 epos;
    EAI_Vec3 efwd;
    EAI_Fixed view_cos;
    int eye_visible;
    if (!world || !locator || agent_index < 0 ||
        agent_index >= B3D_PERCEPTION_MAX_AGENTS) return;
    agent = &world->agents[agent_index];
    if (!agent->active || agent->eai_self_id == EAI_INVALID_ID) return;

    agent->self_alive = self_alive ? 1 : 0;
    agent->target_alive = target_alive ? 1 : 0;
    agent->target_known = 0;
    agent->target_visible = 0;
    agent->target_partial = 0;
    agent->target_occluded = 0;
    agent->target_out_of_range = 0;
    agent->target_out_of_shape = 0;
    memset(&agent->eye_result, 0, sizeof(agent->eye_result));

    if (soq3d_get_thing(locator, agent->self_key, stamp,
                        &self_pose) != SOQ3D_OK) return;
    if (soq3d_get_thing(locator, agent->target_key, stamp,
                        &target_pose) != SOQ3D_OK) return;

    agent->target_known = 1;
    agent->self_position = b3d_pose_position_q12(&self_pose);
    agent->absolute_target_position = b3d_pose_position_q12(&target_pose);
    forward = b3d_pose_forward_q12(&self_pose);
    right.x = (g3d_fix)(self_pose.basis.m00 / 16L);
    right.y = (g3d_fix)(self_pose.basis.m10 / 16L);
    right.z = (g3d_fix)(self_pose.basis.m20 / 16L);
    up.x = (g3d_fix)(self_pose.basis.m01 / 16L);
    up.y = (g3d_fix)(self_pose.basis.m11 / 16L);
    up.z = (g3d_fix)(self_pose.basis.m21 / 16L);

    gamlib_vec3_sub(&delta, &agent->absolute_target_position,
                    &agent->self_position);
    agent->target_distance = gamlib_vec3_length(&delta);
    delta.y = 0;
    agent->target_flat_distance = gamlib_vec3_length(&delta);

    b3d_init_sensor(agent, agent->self_position, forward, right, up);
    tdne_target_init(&target,
        b3d_vec3_to_tdne(agent->absolute_target_position),
        b3d_q12_to_units(agent->config.target_radius),
        (tdne_u32)agent->config.see_mask, 0);
    tdne_target_use_vertical3(&target,
        b3d_q12_to_units(agent->config.target_half_height), TDNE_AXIS_Y);
    eye_visible = tdne_eval_target(&agent->eyes, &target,
        b3d_tdne_raycast, world, &agent->eye_result);
    agent->target_visible = eye_visible &&
        agent->eye_result.visibility == TDNE_VIS_VISIBLE;
    agent->target_partial = eye_visible &&
        agent->eye_result.visibility == TDNE_VIS_PARTIAL;
    agent->target_occluded =
        agent->eye_result.visibility == TDNE_VIS_OCCLUDED;
    agent->target_out_of_range =
        agent->eye_result.visibility == TDNE_VIS_OUT_OF_RANGE;
    agent->target_out_of_shape =
        agent->eye_result.visibility == TDNE_VIS_OUT_OF_SHAPE;

    epos = b3d_vec3_to_eai(agent->self_position);
    efwd = b3d_vec3_to_eai(forward);
    eai_entity_set_position(&world->enlightener,
                            agent->eai_self_id, &epos);
    eai_entity_set_forward(&world->enlightener,
                           agent->eai_self_id, &efwd);
    view_cos = (EAI_Fixed)((tdne_cos_deg(
        agent->config.horizontal_fov_degrees / 2) * 64L));
    eai_entity_set_view(&world->enlightener, agent->eai_self_id,
                        b3d_q12_to_q16(agent->config.view_range), view_cos);
    eai_entity_set_hearing(&world->enlightener, agent->eai_self_id,
                           b3d_q12_to_q16(agent->config.hearing_range));
    if (self_alive)
        world->enlightener.entities[agent->eai_self_id].flags &=
            (EAI_U8)~EAI_ENTITY_FLAG_DISABLED;
    else
        world->enlightener.entities[agent->eai_self_id].flags |=
            EAI_ENTITY_FLAG_DISABLED;
}

static int b3d_percent_from_q16(EAI_Fixed value)
{
    long result;
    if (value <= 0) return 0;
    result = ((long)value * 100L) / (long)EAI_FX_ONE;
    if (result > 100L) result = 100L;
    return (int)result;
}

void blank3d_perception_update(Blank3DPerceptionWorld *world,
                               g3d_fix dt_q12)
{
    int i;
    EAI_Fixed dt;
    if (!world || !world->initialized) return;
    dt = b3d_q12_to_q16(dt_q12);
    eai_update(&world->enlightener, dt);

    for (i = 0; i < B3D_PERCEPTION_MAX_AGENTS; ++i) {
        Blank3DPerceptionAgent *agent;
        EAI_Entity *self;
        EAI_EntityMemory *memory;
        int eye_truth;
        agent = &world->agents[i];
        if (!agent->active || agent->eai_self_id == EAI_INVALID_ID) continue;
        self = &world->enlightener.entities[agent->eai_self_id];
        memory = &self->memory;
        eye_truth = agent->target_visible || agent->target_partial;

        if (agent->eai_target_id != EAI_INVALID_ID &&
            agent->eai_target_id < EAI_MAX_ENTITIES) {
            world->enlightener.visibility[agent->eai_self_id]
                                         [agent->eai_target_id] =
                (EAI_U8)(eye_truth ? 1 : 0);
            if (eye_truth) {
                memory->target_id = agent->eai_target_id;
                memory->last_seen_entity = agent->eai_target_id;
                memory->last_seen_pos =
                    b3d_vec3_to_eai(agent->absolute_target_position);
                memory->target_visible_time = eai_fx_add_sat(
                    memory->target_visible_time, dt);
                memory->target_lost_time = EAI_FX_ZERO;
                memory->alertness = eai_clampf(eai_fx_add_sat(
                    memory->alertness, EAI_FX_FROM_RAW(9830)),
                    EAI_FX_ZERO, EAI_FX_ONE);
                memory->suspicion = eai_clampf(eai_fx_add_sat(
                    memory->suspicion, EAI_FX_FROM_RAW(6554)),
                    EAI_FX_ZERO, EAI_FX_ONE);
            }
        }

        agent->target_heard =
            memory->last_heard_source == agent->eai_target_id;
        agent->target_remembered =
            memory->last_seen_entity == agent->eai_target_id &&
            memory->target_lost_time <= EAI_INVESTIGATE_MEMORY_TIME;
        agent->target_inferred = !eye_truth &&
            (agent->target_remembered || agent->target_heard);
        agent->last_seen_position = b3d_eai_to_vec3(memory->last_seen_pos);
        agent->last_heard_position = b3d_eai_to_vec3(memory->last_heard_pos);
        agent->visible_time = b3d_q16_to_q12(memory->target_visible_time);
        agent->lost_time = b3d_q16_to_q12(memory->target_lost_time);
        agent->alertness_percent = b3d_percent_from_q16(memory->alertness);
        agent->suspicion_percent = b3d_percent_from_q16(memory->suspicion);

        agent->truth_sources = 0UL;
        agent->truth_count = 0U;
        agent->truth_tier = B3D_TRUTH_TIER_NONE;
        agent->action_confidence = 0;
        if (agent->target_known) {
            agent->truth_sources |= B3D_TRUTH_SOURCE_SOCKETER;
            agent->truth_count += 1U;
            agent->truth_tier = B3D_TRUTH_TIER_ABSOLUTE;
            agent->action_confidence = 250;
            agent->best_target_position = agent->absolute_target_position;
        }
        if (eye_truth) {
            agent->truth_sources |= B3D_TRUTH_SOURCE_EYES;
            agent->truth_count += 1U;
            agent->truth_tier = B3D_TRUTH_TIER_PERCEIVED;
            agent->action_confidence = agent->target_visible ? 850 : 700;
            agent->best_target_position = agent->absolute_target_position;
        }
        if (agent->target_remembered || agent->target_heard) {
            agent->truth_sources |= B3D_TRUTH_SOURCE_ENLIGHTENER;
            if (agent->target_remembered)
                agent->truth_sources |= B3D_TRUTH_SOURCE_MEMORY;
            if (agent->target_heard)
                agent->truth_sources |= B3D_TRUTH_SOURCE_HEARING;
            agent->truth_count += 1U;
            agent->truth_tier = B3D_TRUTH_TIER_ENLIGHTENED;
            if (!eye_truth) {
                if (agent->target_remembered) {
                    agent->best_target_position = agent->last_seen_position;
                    agent->action_confidence = 600;
                } else {
                    agent->best_target_position = agent->last_heard_position;
                    agent->action_confidence = 450;
                }
            } else {
                agent->action_confidence = 1000;
            }
        }
        agent->fallback_active = agent->target_known &&
            !eye_truth && !agent->target_remembered && !agent->target_heard;
    }
}

int blank3d_perception_emit_sound(Blank3DPerceptionWorld *world,
                                  const Vec3 *position,
                                  g3d_fix radius,
                                  g3d_fix strength,
                                  EAI_EntityId source_id)
{
    EAI_Vec3 pos;
    if (!world || !position) return 0;
    pos = b3d_vec3_to_eai(*position);
    return eai_perception_emit_sound(&world->enlightener, &pos,
        b3d_q12_to_q16(radius), b3d_q12_to_q16(strength), source_id);
}

int blank3d_perception_has_source(const Blank3DPerceptionAgent *agent,
                                  unsigned long source_mask)
{
    return agent && (agent->truth_sources & source_mask) == source_mask;
}

int blank3d_perception_truth_tier_from_name(const char *name)
{
    if (!name) return B3D_TRUTH_TIER_NONE;
    if (strcmp(name, "absolute") == 0 || strcmp(name, "socketer") == 0 ||
        strcmp(name, "socket") == 0 || strcmp(name, "t0") == 0)
        return B3D_TRUTH_TIER_ABSOLUTE;
    if (strcmp(name, "perceived") == 0 || strcmp(name, "eyes") == 0 ||
        strcmp(name, "visual") == 0 || strcmp(name, "t1") == 0)
        return B3D_TRUTH_TIER_PERCEIVED;
    if (strcmp(name, "enlightened") == 0 ||
        strcmp(name, "enlightener") == 0 ||
        strcmp(name, "interpreted") == 0 || strcmp(name, "t2") == 0)
        return B3D_TRUTH_TIER_ENLIGHTENED;
    return B3D_TRUTH_TIER_NONE;
}

const char *blank3d_perception_truth_tier_name(int tier)
{
    if (tier >= B3D_TRUTH_TIER_ENLIGHTENED) return "enlightened";
    if (tier >= B3D_TRUTH_TIER_PERCEIVED) return "perceived";
    if (tier >= B3D_TRUTH_TIER_ABSOLUTE) return "absolute";
    return "none";
}
