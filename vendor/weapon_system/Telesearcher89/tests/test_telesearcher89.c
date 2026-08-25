#include "telesearcher89.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int target_provider(void *user, int owner, int requested,
                           ts89_target *out)
{
    (void)user;
    (void)owner;
    memset(out, 0, sizeof(*out));
    out->valid = 1;
    out->actor_id = requested >= 0 ? requested : 88;
    out->position.x = 10L * TS89_ONE;
    out->position.y = 0L;
    out->position.z = 0L;
    return 1;
}

int main(void)
{
    ts89_request q;
    ts89_result r;
    memset(&q, 0, sizeof(q));
    q.owner_actor_id = 1;
    q.target_actor_id = TS89_TARGET_NONE;
    q.velocity.z = 20L * TS89_ONE;
    q.speed_fx = 20L * TS89_ONE;
    q.gain_fx = TS89_ONE;
    q.flags = TS89_FLAG_REACQUIRE | TS89_FLAG_REQUIRE_TARGET;
    assert(telesearcher89_step(&q, target_provider, 0, &r));
    assert(r.target_found && r.target_actor_id == 88);
    assert(r.velocity.x > 19L * TS89_ONE);
    assert(r.velocity.z == 0L);
    puts("Telesearcher89: OK");
    return 0;
}
