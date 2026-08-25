#include <stdio.h>
#include <string.h>
#include "blank3d_kinverbs.h"

#define T_OWNER 101UL

static int fail(const char *msg)
{
    fprintf(stderr, "FAIL: %s\n", msg);
    return 1;
}

static void sync_world(Blank3DKinVerbs *kin, Transform *actor, int with_wall)
{
    blank3d_kinverbs_begin_sync(kin);
    (void)blank3d_kinverbs_add_world_box(
        kin,
        G3D_FIX_FROM_INT(-10), G3D_FIX_FROM_INT(-2), G3D_FIX_FROM_INT(-10),
        G3D_FIX_FROM_INT(20), G3D_FIX_FROM_INT(2), G3D_FIX_FROM_INT(20),
        GK3D_FLAG_ACTIVE | GK3D_FLAG_ENABLED | GK3D_FLAG_SOLID);
    if (with_wall) {
        (void)blank3d_kinverbs_add_world_box(
            kin,
            G3D_FIX_FROM_INT(-2), G3D_FIX_FROM_INT(0), -G3D_FIX_ONE,
            G3D_FIX_FROM_INT(4), G3D_FIX_FROM_INT(3), G3D_FIX_ONE / 4L,
            GK3D_FLAG_ACTIVE | GK3D_FLAG_ENABLED | GK3D_FLAG_SOLID);
    }
    (void)blank3d_kinverbs_sync_actor(
        kin, T_OWNER, 7, actor,
        G3D_FIX_ONE / 2L, G3D_FIX_FROM_INT(2), G3D_FIX_ONE / 2L, 1);
    blank3d_kinverbs_end_sync(kin);
}

int main(void)
{
    Blank3DKinVerbs kin;
    Transform actor;
    gverb89_result r;
    int rc;

    blank3d_kinverbs_init(&kin);
    if (!kin.initialized) return fail("kinverbs init");
    transform_init(&actor);

    sync_world(&kin, &actor, 0);
    memset(&r, 0, sizeof(r));
    rc = blank3d_kinverbs_query(&kin, T_OWNER, &actor,
                                "is_on_floor", 0L, "", 0, &r);
    if (rc != GVERB89_HANDLED || !r.truth) return fail("is_on_floor should be true");

    if (!blank3d_kinverbs_can_game_verb(&kin, T_OWNER,
            MBV89_GAME_WALK_FORWARD, (mbv89_fixed)32768L))
        return fail("walk forward should be free without wall");

    memset(&r, 0, sizeof(r));
    rc = blank3d_kinverbs_query(&kin, T_OWNER, &actor,
                                "can_walk_forward", 32768L, "", 1, &r);
    if (rc != GVERB89_HANDLED || !r.truth)
        return fail("can_walk_forward condition should be true");

    sync_world(&kin, &actor, 1);
    if (blank3d_kinverbs_can_game_verb(&kin, T_OWNER,
            MBV89_GAME_WALK_FORWARD, (mbv89_fixed)32768L))
        return fail("walk forward should preflight-block at wall");

    memset(&r, 0, sizeof(r));
    rc = blank3d_kinverbs_query(&kin, T_OWNER, &actor,
                                "can_walk_forward", 32768L, "", 1, &r);
    if (rc != GVERB89_HANDLED || r.truth)
        return fail("can_walk_forward condition should be false at wall");

    printf("OK: 3DKin -> gameverbs contextual locomotion bridge\n");
    return 0;
}
