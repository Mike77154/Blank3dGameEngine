#include <stdio.h>

#include "gautmove89.h"

#define WORLD_ENTITY_COUNT 3

typedef struct DemoWorld89 {
    GMoveVec3_89 position[WORLD_ENTITY_COUNT];
    GMoveVec3_89 forward[WORLD_ENTITY_COUNT];
} DemoWorld89;

static int world_get_position(
    void *user,
    GMoveId89 entity_id,
    GMoveVec3_89 *out_position
)
{
    DemoWorld89 *world;

    world = (DemoWorld89 *)user;
    if (entity_id < 0L || entity_id >= WORLD_ENTITY_COUNT) {
        return GMOVE89_FALSE;
    }

    *out_position = world->position[entity_id];
    return GMOVE89_TRUE;
}

static int world_get_forward(
    void *user,
    GMoveId89 entity_id,
    GMoveVec3_89 *out_forward
)
{
    DemoWorld89 *world;

    world = (DemoWorld89 *)user;
    if (entity_id < 0L || entity_id >= WORLD_ENTITY_COUNT) {
        return GMOVE89_FALSE;
    }

    *out_forward = world->forward[entity_id];
    return GMOVE89_TRUE;
}

static int world_get_socket_position(
    void *user,
    GMoveId89 entity_id,
    GMoveId89 socket_id,
    GMoveVec3_89 *out_position
)
{
    DemoWorld89 *world;
    GMoveVec3_89 offset;

    world = (DemoWorld89 *)user;
    if (entity_id < 0L || entity_id >= WORLD_ENTITY_COUNT) {
        return GMOVE89_FALSE;
    }

    offset = gmove89_vec3_zero();
    if (socket_id == 1L) {
        offset.y = GMOVE89_FX_FROM_INT(2);
    }

    *out_position = gmove89_vec3_add(
        world->position[entity_id],
        offset
    );
    return GMOVE89_TRUE;
}

static int world_apply_motion(
    void *user,
    GMoveId89 entity_id,
    const GMoveMotion89 *motion
)
{
    DemoWorld89 *world;

    world = (DemoWorld89 *)user;
    if (entity_id < 0L || entity_id >= WORLD_ENTITY_COUNT) {
        return GMOVE89_FALSE;
    }

    if (motion->has_translation) {
        world->position[entity_id] = motion->target_position;
    }
    if (motion->has_rotation) {
        world->forward[entity_id] = motion->desired_forward;
    }
    return GMOVE89_TRUE;
}

int main(void)
{
    DemoWorld89 world;
    GMoveProvider89 provider;
    GMoveTarget89 target;
    GAutMoveConfig89 config;
    GMoveMotion89 motion;
    GMoveFx89 dt;
    int frame;
    int i;

    for (i = 0; i < WORLD_ENTITY_COUNT; ++i) {
        world.position[i] = gmove89_vec3_zero();
        world.forward[i] = gmove89_vec3(GMOVE89_FX_ONE, 0L, 0L);
    }

    world.position[1] = gmove89_vec3(
        GMOVE89_FX_FROM_INT(12),
        0L,
        GMOVE89_FX_FROM_INT(4)
    );

    provider.user = &world;
    provider.get_position = world_get_position;
    provider.get_forward = world_get_forward;
    provider.get_socket_position = world_get_socket_position;
    provider.apply_motion = world_apply_motion;

    target.type = GMOVE89_TARGET_SOCKET;
    target.entity_id = 1L;
    target.socket_id = 1L;
    target.point = gmove89_vec3_zero();

    gautmove89_config_default(&config);
    config.speed = GMOVE89_FX_FROM_INT(6);
    config.stop_distance = GMOVE89_FX_FROM_INT(1);

    dt = GMOVE89_FX_ONE / 20L;

    for (frame = 0; frame < 100; ++frame) {
        if (!gautmove89_step_to_target(
                &provider,
                0L,
                &target,
                &config,
                dt,
                &motion)) {
            puts("provider error");
            return 1;
        }

        gmove89_apply_motion(&provider, 0L, &motion);
        if (motion.reached) {
            break;
        }
    }

    printf(
        "entity 0 reached socket area at (%ld,%ld,%ld)\n",
        GMOVE89_FX_TO_INT(world.position[0].x),
        GMOVE89_FX_TO_INT(world.position[0].y),
        GMOVE89_FX_TO_INT(world.position[0].z)
    );

    return motion.reached ? 0 : 1;
}
