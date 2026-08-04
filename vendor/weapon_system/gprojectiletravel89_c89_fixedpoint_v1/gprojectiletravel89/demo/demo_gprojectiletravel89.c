#include <stdio.h>
#include "../src/gprojectiletravel89.h"

static void on_end(void *ctx, const GPT89_State *state)
{
    (void)ctx;
    printf("projectile slot=%d ended reason=%d elapsed=%lu distance=%ld pos_z=%ld\n",
           state->slot,
           state->end_reason,
           state->elapsed_ms,
           state->traveled_fx,
           state->position.z);
}

int main(void)
{
    GPT89_Pool pool;
    GPT89_Desc desc;
    int slot;
    int frame;

    gpt89_pool_init(&pool);
    gpt89_set_end_hook(&pool, on_end, 0);
    gpt89_desc_defaults(&desc);
    desc.projectile_id = 3;
    desc.speed_fx = gpt89_fx_from_int(40);
    desc.max_distance_fx = gpt89_fx_from_int(24);
    desc.life_ms = 900UL;

    slot = gpt89_spawn(&pool, &desc);
    if (slot < 0) return 1;

    for (frame = 0; frame < 120 && pool.active_count > 0; frame++) {
        gpt89_update_all(&pool, 16UL);
    }
    return 0;
}
