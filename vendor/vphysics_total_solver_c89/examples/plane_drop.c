/* plane_drop.c - legacy callback path, kept compatible with Total Solver.
   Strict C89, fixed point, static arena.
*/
#include <stdio.h>
#include "vp_api.h"

#define WORLD_ARENA_BYTES 1048576UL
static vp_u8 g_world_mem[WORLD_ARENA_BYTES];

typedef struct App {
    vpBodyId ball;
    vp_fx radius;
} App;

static void preStep(void* user, vpWorld* w, vp_fx dt)
{
    App* app = (App*)user;
    vpBody* b = vpWorldGetBody(w, app->ball);
    VP_UNUSED(dt);
    vpContactsBegin(w);
    if (b && b->used && b->pos.y < app->radius) {
        vpVec3 p = b->pos;
        p.y = app->radius;
        vpContactsAddEx(w, 0, app->ball, p,
            vpVec3_make(0, VP_FX_ONE, 0),
            app->radius - b->pos.y,
            (vp_fx)(VP_FX_ONE * 4L / 5L), 0,
            VP_CONTACT_FLAG_NONE);
    }
}

int main(void)
{
    vpWorldDesc desc;
    vpWorld* w;
    vpBodyId ball;
    vpCallbacks cb;
    App app;
    vpBody* b;
    vp_u16 step;

    desc.maxBodies = 8;
    desc.maxContacts = 32;
    desc.maxJoints = 8;
    desc.solverIters = 10;
    if (vpWorldMemSize(&desc) > WORLD_ARENA_BYTES) return 1;
    w = vpWorldInit(g_world_mem, WORLD_ARENA_BYTES, &desc);
    if (!w) return 2;

    ball = vpBodyCreateDynamic(w, vp_fx_from_int(1));
    vpBodySetPose(w, ball, vpVec3_make(0, vp_fx_from_int(5), 0), vpQuat_identity());
    vpBodySetInertiaInvDiag(w, ball, vpVec3_make(VP_FX_ONE,VP_FX_ONE,VP_FX_ONE));
    vpBodySetDamping(w, ball, (vp_fx)(VP_FX_ONE / 20), (vp_fx)(VP_FX_ONE / 20));

    app.ball = ball;
    app.radius = VP_FX_HALF;
    cb.user = &app;
    cb.preStep = preStep;
    cb.postStep = 0;
    cb.bodySweepTOI = 0;
    vpWorldSetCallbacks(w, &cb);

    for (step = 0; step < 180; ++step) vpWorldStep(w, (vp_fx)(VP_FX_ONE / 60));
    b = vpWorldGetBody(w, ball);
    if (!b) return 3;
    printf("y_q16=%ld vy_q16=%ld asleep=%u contacts=%u\n",
        (long)b->pos.y, (long)b->v_lin.y,
        (unsigned)b->asleep, (unsigned)w->contactCount);
    return 0;
}
