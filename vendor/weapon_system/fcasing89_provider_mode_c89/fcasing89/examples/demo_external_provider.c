#include <stdio.h>
#include "fcasing89.h"

typedef struct DemoProviderState {
    fc89_u32 gravity_calls;
    fc89_u32 move_calls;
    fc89_u32 rotate_calls;
    fc89_u32 scale_calls;
    fc89_u32 collision_calls;
    fc89_u32 hits;
} DemoProviderState;

static int demo_gravity(
    void *user,
    const FCasing89ProviderGravityQuery *query,
    FCasing89Vec3 *out_acceleration)
{
    DemoProviderState *state;
    state = (DemoProviderState *)user;
    state->gravity_calls++;
    *out_acceleration = query->fallback_acceleration;
    return 1;
}

static int demo_transform(
    void *user,
    const FCasing89ProviderTransformQuery *query,
    FCasing89Vec3 *out_value)
{
    DemoProviderState *state;
    state = (DemoProviderState *)user;

    if (query->operation == FCASING89_TRANSFORM_MOVE) {
        state->move_calls++;
        *out_value = fcasing89_vec3_add(query->current, query->delta);
        return 1;
    }
    if (query->operation == FCASING89_TRANSFORM_ROTATE) {
        state->rotate_calls++;
        *out_value = fcasing89_vec3_add(query->current, query->delta);
        return 1;
    }

    state->scale_calls++;
    *out_value = fcasing89_vec3_mul_q(query->current, query->delta);
    return 1;
}

static int demo_collision(
    void *user,
    const FCasing89ProviderCollisionQuery *query,
    FCasing89ProviderCollisionResult *out_result)
{
    DemoProviderState *state;
    fc89_i32 floor_y;
    fc89_i32 wall_x;

    state = (DemoProviderState *)user;
    state->collision_calls++;
    floor_y = query->radius;
    wall_x = FCASING89_TO_FIX(24);

    if (out_result->position.y <= floor_y && query->velocity.y <= 0L) {
        out_result->position.y = floor_y;
        out_result->normal = fcasing89_vec3(0L, FCASING89_FIX_ONE, 0L);
        out_result->flags = FCASING89_COLLISION_HIT;
        state->hits++;
    } else if (out_result->position.x >= wall_x && query->velocity.x > 0L) {
        out_result->position.x = wall_x;
        out_result->normal = fcasing89_vec3(-FCASING89_FIX_ONE, 0L, 0L);
        out_result->flags = FCASING89_COLLISION_HIT;
        state->hits++;
    } else if (out_result->position.x <= -wall_x && query->velocity.x < 0L) {
        out_result->position.x = -wall_x;
        out_result->normal = fcasing89_vec3(FCASING89_FIX_ONE, 0L, 0L);
        out_result->flags = FCASING89_COLLISION_HIT;
        state->hits++;
    }
    return 1;
}

int main(void)
{
    FCasing89System sys;
    FCasing89Config cfg;
    FCasing89Provider provider;
    FCasing89Spawn spawn;
    FCasing89RenderItem items[8];
    DemoProviderState state;
    int frame;
    int count;

    state.gravity_calls = 0UL;
    state.move_calls = 0UL;
    state.rotate_calls = 0UL;
    state.scale_calls = 0UL;
    state.collision_calls = 0UL;
    state.hits = 0UL;

    fcasing89_default_config(&cfg);
    cfg.default_mode = FCASING89_MODE_SIM;
    fcasing89_init(&sys, &cfg, 12345UL);

    fcasing89_provider_init(&provider);
    provider.user = &state;
    provider.enabled_mask = FCASING89_PROVIDER_ALL;
    provider.gravity = demo_gravity;
    provider.move = demo_transform;
    provider.rotate = demo_transform;
    provider.scale = demo_transform;
    provider.collision = demo_collision;
    fcasing89_set_provider(&sys, &provider);

    spawn.origin = fcasing89_vec3(
        FCASING89_TO_FIX(0),
        FCASING89_TO_FIX(18),
        FCASING89_TO_FIX(0));
    spawn.forward = fcasing89_vec3(0L, 0L, FCASING89_FIX_ONE);
    spawn.right = fcasing89_vec3(FCASING89_FIX_ONE, 0L, 0L);
    spawn.up = fcasing89_vec3(0L, FCASING89_FIX_ONE, 0L);
    spawn.profile_id = FCASING89_PROFILE_RIFLE;
    spawn.count = 1U;
    spawn.importance = 255U;
    spawn.flags = FCASING89_FLAG_FORCE_SIM | FCASING89_FLAG_CUSTOM_SCALE;
    spawn.seed_bias = 0U;
    spawn.local_side_bias = 0L;
    spawn.local_up_bias = 0L;
    spawn.local_back_bias = 0L;
    spawn.scale = fcasing89_vec3(
        FCASING89_FIX_ONE,
        FCASING89_FIX_ONE,
        FCASING89_FIX_HALF);

    fcasing89_begin_frame(&sys);
    fcasing89_emit(&sys, &spawn);
    for (frame = 0; frame < 30; frame++) {
        fcasing89_begin_frame(&sys);
        fcasing89_update(&sys, 16U);
    }

    count = fcasing89_collect_render_items(&sys, items, 8);
    printf("provider_mask=%lu render=%d calls=%lu/%lu/%lu/%lu/%lu hits=%lu\n",
        fcasing89_get_provider_mask(&sys),
        count,
        state.gravity_calls,
        state.move_calls,
        state.rotate_calls,
        state.scale_calls,
        state.collision_calls,
        state.hits);
    if (count > 0) {
        printf("scale_q8=(%ld,%ld,%ld) state=%u bounces=%u\n",
            items[0].scale.x,
            items[0].scale.y,
            items[0].scale.z,
            (unsigned)items[0].state,
            (unsigned)items[0].bounce_count);
    }
    return 0;
}
