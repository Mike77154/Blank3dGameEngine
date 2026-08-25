#include "gloco89.h"
#include <stdio.h>
#include <string.h>

typedef struct TestStatsTag {
    GLOCO_FX stamina[GLOCO_MAX_ACTORS];
    int hard_stop_events;
} TestStats;

static int get_stamina(void *user, int actor_id, GLOCO_FX *out)
{
    TestStats *s = (TestStats *)user;
    if (!s || !out || actor_id < 0 || actor_id >= GLOCO_MAX_ACTORS)
        return GLOCO_PROVIDER_FALLBACK;
    *out = s->stamina[actor_id];
    return GLOCO_PROVIDER_HANDLED;
}

static int set_stamina(void *user, int actor_id, GLOCO_FX value)
{
    TestStats *s = (TestStats *)user;
    if (!s || actor_id < 0 || actor_id >= GLOCO_MAX_ACTORS)
        return GLOCO_PROVIDER_FALLBACK;
    s->stamina[actor_id] = value;
    return GLOCO_PROVIDER_HANDLED;
}

static int consume_stamina(void *user, int actor_id, GLOCO_FX amount,
                           GLOCO_FX *out)
{
    TestStats *s = (TestStats *)user;
    GLOCO_FX v;
    if (!s || !out || actor_id < 0 || actor_id >= GLOCO_MAX_ACTORS)
        return GLOCO_PROVIDER_FALLBACK;
    v = s->stamina[actor_id] - amount;
    if (v < 0) v = 0;
    s->stamina[actor_id] = v;
    *out = v;
    return GLOCO_PROVIDER_HANDLED;
}

static void event_cb(void *user, int actor_id, int event_id, int value)
{
    TestStats *s = (TestStats *)user;
    (void)actor_id;
    (void)value;
    if (s && event_id == GLOCO_EVENT_HARD_STOP) s->hard_stop_events += 1;
}

int main(void)
{
    GLOCO_Context ctx;
    GLOCO_StatsProvider stats;
    GLOCO_Input input;
    GLOCO_Vec3 pos;
    GLOCO_Vec3 facing;
    TestStats store;
    int actor;
    int i;
    memset(&store, 0, sizeof(store));
    gloco_init(&ctx);
    gloco_stats_provider_init(&stats);
    stats.user = &store;
    stats.get_stamina = get_stamina;
    stats.set_stamina = set_stamina;
    stats.consume_stamina = consume_stamina;
    gloco_set_stats_provider(&ctx, &stats);
    gloco_set_callbacks(&ctx, 0, event_cb, &store);
    pos = gloco_v3(0, 0, 0);
    facing = gloco_v3(0, 0, GLOCO_FX_ONE);
    actor = gloco_actor_create(&ctx, 0, &pos, &facing);
    if (actor < 0) return 1;
    memset(&input, 0, sizeof(input));
    input.basis_fwd = facing;
    input.basis_right = gloco_v3(GLOCO_FX_ONE, 0, 0);
    input.move_z = 256;
    input.buttons = GLOCO_INPUT_RUN;
    input.speed_override = gloco_fx_from_int(3);
    if (gloco_update_actor(&ctx, actor, &input, 16) != GLOCO_OK) return 2;
    if (ctx.actors[actor].vel.z <= 0) return 3;
    input.move_z = 0;
    input.buttons = GLOCO_INPUT_HARD_STOP;
    input.speed_override = 0;
    for (i = 0; i < 8; ++i)
        if (gloco_update_actor(&ctx, actor, &input, 16) != GLOCO_OK) return 4;
    if (store.hard_stop_events != 1) return 5;
    input.buttons = 0;
    if (gloco_update_actor(&ctx, actor, &input, 16) != GLOCO_OK) return 6;
    input.buttons = GLOCO_INPUT_HARD_STOP;
    if (gloco_update_actor(&ctx, actor, &input, 16) != GLOCO_OK) return 7;
    if (store.hard_stop_events != 2) return 8;
    if (store.stamina[actor] != ctx.actors[actor].stamina) return 9;
    printf("OK: gloco89 engine-ready providers/latch\n");
    return 0;
}
