#include "vp_debugdraw.h"
#include "vp_world.h"

void vpDebugDrawWorld(const vpWorld* world, const vpDebugDraw* dd)
{
    vpWorld* w;
    vp_u16 i;

    if (!world || !dd) return;
    if (!dd->line && !dd->point) return;

    /* cast away const for internal accessor (world is POD) */
    w = (vpWorld*)world;

    /* contacts */
    for (i = 0; i < w->contactCount; ++i) {
        const vpContact* c = &w->contacts[i];
        vpVec3 p0 = c->p;
        vpVec3 p1 = vpVec3_add(c->p, vpVec3_scale(c->n, vp_fx_from_int(1))); /* 1m normal */
        if (dd->point) dd->point(dd->user, p0, 0xFF00FFFFu);
        if (dd->line)  dd->line(dd->user, p0, p1, 0xFF00FFFFu);
    }

    /* joints */
    for (i = 0; i < w->maxJoints; ++i) {
        const vpJoint* j = &w->joints[i];
        const vpBody* A;
        const vpBody* B;
        vpVec3 worldA, worldB;

        if (!j->used || j->broken) continue;

        A = vpWorldGetBody(w, (vpBodyId)j->bodyA);
        B = vpWorldGetBody(w, (vpBodyId)j->bodyB);
        if (!A || !B) continue;

        worldA = vpVec3_add(A->pos, vpQuat_rotate_vec3(A->rot, j->localA));
        worldB = vpVec3_add(B->pos, vpQuat_rotate_vec3(B->rot, j->localB));

        if (dd->line) dd->line(dd->user, worldA, worldB, 0xFFFFFFFFu);
    }
}
