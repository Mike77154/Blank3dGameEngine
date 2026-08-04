#include "../src/gprojectiletravel89.h"

static int last_reason;

static void ended(void *ctx, const GPT89_State *state)
{
    (void)ctx;
    last_reason = state->end_reason;
}

static int test_range(void)
{
    GPT89_Pool pool;
    GPT89_Desc desc;
    int slot;
    gpt89_pool_init(&pool);
    gpt89_set_end_hook(&pool, ended, 0);
    gpt89_desc_defaults(&desc);
    desc.speed_fx = gpt89_fx_from_int(40);
    desc.max_distance_fx = gpt89_fx_from_int(24);
    desc.life_ms = 900UL;
    slot = gpt89_spawn(&pool, &desc);
    if (slot < 0) return 1;
    while (pool.active_count > 0) gpt89_update_all(&pool, 16UL);
    return last_reason == GPT89_END_MAX_DISTANCE ? 0 : 2;
}

static int test_timeout(void)
{
    GPT89_Pool pool;
    GPT89_Desc desc;
    int slot;
    gpt89_pool_init(&pool);
    gpt89_set_end_hook(&pool, ended, 0);
    gpt89_desc_defaults(&desc);
    desc.speed_fx = gpt89_fx_from_int(1);
    desc.max_distance_fx = gpt89_fx_from_int(100);
    desc.life_ms = 20UL;
    slot = gpt89_spawn(&pool, &desc);
    if (slot < 0) return 3;
    gpt89_update_all(&pool, 32UL);
    return last_reason == GPT89_END_TIMEOUT ? 0 : 4;
}

static int test_destination(void)
{
    GPT89_Pool pool;
    GPT89_Desc desc;
    int slot;
    gpt89_pool_init(&pool);
    gpt89_set_end_hook(&pool, ended, 0);
    gpt89_desc_defaults(&desc);
    desc.speed_fx = gpt89_fx_from_int(10);
    desc.max_distance_fx = gpt89_fx_from_int(100);
    desc.life_ms = 10000UL;
    desc.has_destination = 1;
    desc.destination = gpt89_v3(0L, 0L, gpt89_fx_from_int(2));
    desc.arrival_radius_fx = 0L;
    slot = gpt89_spawn(&pool, &desc);
    if (slot < 0) return 5;
    while (pool.active_count > 0) gpt89_update_all(&pool, 16UL);
    return last_reason == GPT89_END_DESTINATION ? 0 : 6;
}

int main(void)
{
    int result;
    result = test_range();
    if (result) return result;
    result = test_timeout();
    if (result) return result;
    return test_destination();
}
