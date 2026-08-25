#include "satellaborner89.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int target_provider(void *user, int owner, int requested,
                           sat89_target *out)
{
    (void)user;
    (void)owner;
    memset(out, 0, sizeof(*out));
    out->valid = 1;
    out->actor_id = requested >= 0 ? requested : 77;
    out->position.x = 10L * 4096L;
    out->position.y = 2L * 4096L;
    out->position.z = -3L * 4096L;
    return 1;
}

int main(void)
{
    sat89_request q;
    sat89_result r;
    memset(&q, 0, sizeof(q));
    q.owner_actor_id = 1;
    q.requested_target_id = SAT89_TARGET_NONE;
    q.original_origin.z = 4096L;
    q.target_offset.y = 5L * 4096L;
    assert(satellaborner89_resolve(&q, target_provider, 0, &r));
    assert(r.valid && r.target_found);
    assert(r.target_actor_id == 77);
    assert(r.spawn_origin.x == 10L * 4096L);
    assert(r.spawn_origin.y == 7L * 4096L);
    assert(r.spawn_origin.z == -3L * 4096L);
    puts("Satellaborner89: OK");
    return 0;
}
