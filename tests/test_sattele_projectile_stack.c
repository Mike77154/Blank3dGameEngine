#include "satellaborner89.h"
#include "telesearcher89.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int sat_target(void *user, int owner, int requested,
                      sat89_target *out)
{
    (void)user; (void)owner; (void)requested;
    memset(out, 0, sizeof(*out));
    out->valid = 1;
    out->actor_id = 42;
    out->position.x = 5L * 4096L;
    out->position.y = 0L;
    out->position.z = 10L * 4096L;
    return 1;
}

static int tele_target(void *user, int owner, int requested,
                       ts89_target *out)
{
    (void)user; (void)owner;
    memset(out, 0, sizeof(*out));
    out->valid = 1;
    out->actor_id = requested >= 0 ? requested : 42;
    out->position.x = 5L * TS89_ONE;
    out->position.y = 0L;
    out->position.z = 10L * TS89_ONE;
    return 1;
}

int main(void)
{
    sat89_request sq;
    sat89_result sr;
    ts89_request tq;
    ts89_result tr;
    memset(&sq, 0, sizeof(sq));
    sq.owner_actor_id = 1;
    sq.requested_target_id = SAT89_TARGET_NONE;
    sq.target_offset.y = 12L * 4096L;
    assert(satellaborner89_resolve(&sq, sat_target, 0, &sr));
    assert(sr.target_found && sr.target_actor_id == 42);
    assert(sr.spawn_origin.x == 5L * 4096L);
    assert(sr.spawn_origin.y == 12L * 4096L);
    assert(sr.spawn_origin.z == 10L * 4096L);

    memset(&tq, 0, sizeof(tq));
    tq.owner_actor_id = 1;
    tq.target_actor_id = sr.target_actor_id;
    tq.position.x = sr.spawn_origin.x;
    tq.position.y = sr.spawn_origin.y;
    tq.position.z = sr.spawn_origin.z;
    tq.velocity.z = 20L * TS89_ONE;
    tq.speed_fx = 20L * TS89_ONE;
    tq.gain_fx = TS89_ONE;
    tq.target_offset.y = (TS89_ONE * 8L) / 10L;
    assert(telesearcher89_step(&tq, tele_target, 0, &tr));
    assert(tr.target_found && tr.target_actor_id == 42);
    assert(tr.velocity.y < 0L);
    puts("Satellaborner89 + Telesearcher89 composition: OK");
    return 0;
}
