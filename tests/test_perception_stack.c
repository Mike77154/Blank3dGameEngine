#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "blank3d_perception.h"

static int no_block(void *user, const Vec3 *from, const Vec3 *to,
                    unsigned long mask)
{
    (void)user;
    (void)from;
    (void)to;
    (void)mask;
    return 0;
}

static soq3d_pose pose_at(int x, int y, int z, int face_back)
{
    soq3d_pose pose;
    pose = soq3d_pose_identity();
    pose.position.x = x * SOQ3D_FX_ONE;
    pose.position.y = y * SOQ3D_FX_ONE;
    pose.position.z = z * SOQ3D_FX_ONE;
    if (face_back) {
        pose.basis.m00 = -SOQ3D_FX_ONE;
        pose.basis.m22 = -SOQ3D_FX_ONE;
    }
    return pose;
}

static void publish_pair(soq3d_context *locator, soq3d_stamp stamp,
                         soq3d_key enemy_key, soq3d_key player_key,
                         int enemy_back)
{
    soq3d_pose enemy_pose;
    soq3d_pose player_pose;
    soq3d_begin_frame(locator, stamp);
    enemy_pose = pose_at(0, 0, 0, enemy_back);
    player_pose = pose_at(0, 0, 10, 0);
    assert(soq3d_publish_thing(locator, enemy_key, &enemy_pose) == SOQ3D_OK);
    assert(soq3d_publish_thing(locator, player_key, &player_pose) == SOQ3D_OK);
}

int main(void)
{
    soq3d_context locator;
    soq3d_key enemy_key;
    soq3d_key player_key;
    Blank3DPerceptionWorld world;
    Blank3DPerceptionAgent *agent;
    Vec3 sound_position;

    soq3d_init(&locator);
    enemy_key = soq3d_key_from_cstr("enemy.0");
    player_key = soq3d_key_from_cstr("player");
    blank3d_perception_world_init(&world, no_block, 0);
    assert(blank3d_perception_agent_init(&world, 0, enemy_key,
                                         player_key, 2u, 1u));
    agent = blank3d_perception_agent(&world, 0);
    assert(agent != 0);
    blank3d_perception_set_view_range(agent, G3D_FIX_FROM_INT(30));
    blank3d_perception_set_fov(agent, 100, 70);

    publish_pair(&locator, 1U, enemy_key, player_key, 0);
    blank3d_perception_sync_player(&world, &locator, 1U, player_key, 1);
    blank3d_perception_sync_agent(&world, 0, &locator, 1U, 1, 1);
    blank3d_perception_update(&world, G3D_FIX_ONE / 60);
    assert(agent->target_known);
    assert(agent->target_visible || agent->target_partial);
    assert(blank3d_perception_has_source(agent,
            B3D_TRUTH_SOURCE_SOCKETER));
    assert(blank3d_perception_has_source(agent,
            B3D_TRUTH_SOURCE_EYES));
    assert(agent->truth_tier >= B3D_TRUTH_TIER_PERCEIVED);

    publish_pair(&locator, 2U, enemy_key, player_key, 1);
    blank3d_perception_sync_player(&world, &locator, 2U, player_key, 1);
    blank3d_perception_sync_agent(&world, 0, &locator, 2U, 1, 1);
    blank3d_perception_update(&world, G3D_FIX_ONE / 60);
    assert(agent->target_known);
    assert(!agent->target_visible && !agent->target_partial);
    assert(blank3d_perception_has_source(agent,
            B3D_TRUTH_SOURCE_SOCKETER));
    assert(agent->target_remembered || agent->fallback_active);

    sound_position = gamlib_vec3(0, 0, G3D_FIX_FROM_INT(10));
    assert(blank3d_perception_emit_sound(&world, &sound_position,
        G3D_FIX_FROM_INT(30), G3D_FIX_ONE, world.player_entity_id));
    blank3d_perception_update(&world, G3D_FIX_ONE / 60);
    assert(agent->target_heard || agent->target_remembered);
    assert(agent->target_known);

    printf("Sensory truth stack: Socketer + Eyes + Enlightener fallback OK.\n");
    return 0;
}
