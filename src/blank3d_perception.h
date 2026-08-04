#ifndef BLANK3D_PERCEPTION_H
#define BLANK3D_PERCEPTION_H

#include "../vendor/gamlib3d/gamlib3d_transform.h"
#include "../vendor/soquete3d/soquete3d.h"
#include "3d_npc_eyes.h"
#include "enlightenerai.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef B3D_PERCEPTION_MAX_AGENTS
#define B3D_PERCEPTION_MAX_AGENTS 32
#endif

#define B3D_TRUTH_SOURCE_SOCKETER    1UL
#define B3D_TRUTH_SOURCE_EYES        2UL
#define B3D_TRUTH_SOURCE_ENLIGHTENER 4UL
#define B3D_TRUTH_SOURCE_HEARING     8UL
#define B3D_TRUTH_SOURCE_MEMORY     16UL

#define B3D_TRUTH_TIER_NONE        0
#define B3D_TRUTH_TIER_ABSOLUTE    1
#define B3D_TRUTH_TIER_PERCEIVED   2
#define B3D_TRUTH_TIER_ENLIGHTENED 3

#define B3D_EYE_SHAPE_CONE    1
#define B3D_EYE_SHAPE_SPHERE  2
#define B3D_EYE_SHAPE_BOX     3
#define B3D_EYE_SHAPE_FRUSTUM 4

typedef int (*Blank3DPerceptionRaycastFn)(
    void *user,
    const Vec3 *from,
    const Vec3 *to,
    unsigned long block_mask
);

typedef struct Blank3DPerceptionConfigTag {
    int eye_shape;
    g3d_fix view_range;
    int horizontal_fov_degrees;
    int vertical_fov_degrees;
    int require_line_of_sight;
    g3d_fix hearing_range;
    g3d_fix target_radius;
    g3d_fix target_half_height;
    unsigned long see_mask;
    unsigned long block_mask;
} Blank3DPerceptionConfig;

typedef struct Blank3DPerceptionAgentTag {
    int active;
    soq3d_key self_key;
    soq3d_key target_key;
    EAI_EntityId eai_self_id;
    EAI_EntityId eai_target_id;
    int self_alive;
    int target_alive;

    Blank3DPerceptionConfig config;
    tdne_sensor eyes;
    tdne_result eye_result;

    int target_known;
    int target_visible;
    int target_partial;
    int target_occluded;
    int target_out_of_range;
    int target_out_of_shape;
    int target_heard;
    int target_remembered;
    int target_inferred;
    int fallback_active;

    unsigned long truth_sources;
    unsigned int truth_count;
    int truth_tier;
    int action_confidence;

    Vec3 self_position;
    Vec3 absolute_target_position;
    Vec3 best_target_position;
    Vec3 last_seen_position;
    Vec3 last_heard_position;
    g3d_fix target_distance;
    g3d_fix target_flat_distance;
    g3d_fix visible_time;
    g3d_fix lost_time;
    int alertness_percent;
    int suspicion_percent;
} Blank3DPerceptionAgent;

typedef struct Blank3DPerceptionWorldTag {
    EAI_Context enlightener;
    EAI_WorldOps world_ops;
    Blank3DPerceptionRaycastFn raycast;
    void *raycast_user;
    EAI_EntityId player_entity_id;
    Blank3DPerceptionAgent agents[B3D_PERCEPTION_MAX_AGENTS];
    int initialized;
} Blank3DPerceptionWorld;

void blank3d_perception_config_defaults(Blank3DPerceptionConfig *config);
void blank3d_perception_world_init(Blank3DPerceptionWorld *world,
                                   Blank3DPerceptionRaycastFn raycast,
                                   void *raycast_user);
int blank3d_perception_agent_init(Blank3DPerceptionWorld *world,
                                  int agent_index,
                                  soq3d_key self_key,
                                  soq3d_key target_key,
                                  unsigned char self_team,
                                  unsigned char target_team);
void blank3d_perception_agent_disable(Blank3DPerceptionWorld *world,
                                      int agent_index);
Blank3DPerceptionAgent *blank3d_perception_agent(
    Blank3DPerceptionWorld *world, int agent_index);
const Blank3DPerceptionAgent *blank3d_perception_agent_const(
    const Blank3DPerceptionWorld *world, int agent_index);

int blank3d_perception_set_eye_shape(Blank3DPerceptionAgent *agent,
                                     const char *shape_name);
const char *blank3d_perception_eye_shape_name(int eye_shape);
void blank3d_perception_set_view_range(Blank3DPerceptionAgent *agent,
                                       g3d_fix range);
void blank3d_perception_set_fov(Blank3DPerceptionAgent *agent,
                                int horizontal_degrees,
                                int vertical_degrees);
void blank3d_perception_set_hearing_range(Blank3DPerceptionAgent *agent,
                                          g3d_fix range);
void blank3d_perception_set_line_of_sight(Blank3DPerceptionAgent *agent,
                                          int required);
void blank3d_perception_set_target_key(Blank3DPerceptionAgent *agent,
                                       soq3d_key target_key,
                                       EAI_EntityId eai_target_id);

void blank3d_perception_sync_player(Blank3DPerceptionWorld *world,
                                    const soq3d_context *locator,
                                    soq3d_stamp stamp,
                                    soq3d_key player_key,
                                    int player_alive);
void blank3d_perception_sync_agent(Blank3DPerceptionWorld *world,
                                   int agent_index,
                                   const soq3d_context *locator,
                                   soq3d_stamp stamp,
                                   int self_alive,
                                   int target_alive);
void blank3d_perception_update(Blank3DPerceptionWorld *world,
                               g3d_fix dt_q12);
int blank3d_perception_emit_sound(Blank3DPerceptionWorld *world,
                                  const Vec3 *position,
                                  g3d_fix radius,
                                  g3d_fix strength,
                                  EAI_EntityId source_id);

int blank3d_perception_has_source(const Blank3DPerceptionAgent *agent,
                                  unsigned long source_mask);
int blank3d_perception_truth_tier_from_name(const char *name);
const char *blank3d_perception_truth_tier_name(int tier);

#ifdef __cplusplus
}
#endif

#endif /* BLANK3D_PERCEPTION_H */
