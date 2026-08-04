/* total_solver_demo.c
   Strict C89, fixed-point, static arenas, no malloc/free, no float/double.
*/
#include <stdio.h>
#include "vp_api.h"

#define WORLD_ARENA_BYTES 1048576UL
#define TOTAL_ARENA_BYTES 131072UL
#define EXT_OBJECTS 4

typedef struct DemoObject {
    vp_u8 used;
    vpTransform world;
} DemoObject;

typedef struct Demo {
    DemoObject objects[EXT_OBJECTS];
    vpBodyId ball;
    vp_fx radius;
    vp_u32 customComposeCalls;
} Demo;

static vp_u8 g_world_mem[WORLD_ARENA_BYTES];
static vp_u8 g_total_mem[TOTAL_ARENA_BYTES];

static int demo_read_world(void* user, vp_u32 id, vpTransform* out)
{
    Demo* d = (Demo*)user;
    if (id == 0 || id > EXT_OBJECTS || !d->objects[id - 1].used) return 0;
    *out = d->objects[id - 1].world;
    return 1;
}

static int demo_write_world(void* user, vp_u32 id, const vpTransform* value)
{
    Demo* d = (Demo*)user;
    if (id == 0 || id > EXT_OBJECTS || !d->objects[id - 1].used) return 0;
    d->objects[id - 1].world = *value;
    return 1;
}

static void demo_compose(void* user, vpTransform* out,
    const vpTransform* parent, const vpTransform* local)
{
    Demo* d = (Demo*)user;
    vpVec3 scaled;
    vpVec3 rotated;
    d->customComposeCalls += 1;
    scaled.x = vp_fx_mul(local->position.x, parent->scale.x);
    scaled.y = vp_fx_mul(local->position.y, parent->scale.y);
    scaled.z = vp_fx_mul(local->position.z, parent->scale.z);
    rotated = vpQuat_rotate_vec3(parent->rotation, scaled);
    out->position = vpVec3_add(parent->position, rotated);
    out->rotation = vpQuat_normalize(vpQuat_mul(parent->rotation, local->rotation));
    out->scale.x = vp_fx_mul(parent->scale.x, local->scale.x);
    out->scale.y = vp_fx_mul(parent->scale.y, local->scale.y);
    out->scale.z = vp_fx_mul(parent->scale.z, local->scale.z);
}

static void demo_collisions(void* user, vpTotalSolver* solver, vpWorld* world, vp_fx dt)
{
    Demo* d = (Demo*)user;
    vpBody* b = vpWorldGetBody(world, d->ball);
    VP_UNUSED(solver);
    VP_UNUSED(dt);
    if (b && b->used && b->pos.y < d->radius) {
        vpVec3 point = b->pos;
        point.y = d->radius;
        vpContactsAddEx(world, 0, d->ball, point,
            vpVec3_make(0, VP_FX_ONE, 0),
            d->radius - b->pos.y,
            (vp_fx)(VP_FX_ONE * 4L / 5L), 0,
            VP_CONTACT_FLAG_NONE);
    }
}

int main(void)
{
    vpWorldDesc wd;
    vpTotalSolverDesc td;
    vpWorld* world;
    vpTotalSolver* total;
    vpTransformProvider tp;
    vpTransformMathProvider mp;
    vpCollisionProvider cp;
    vpTransformNodeId bodyNode;
    vpTransformNodeId socketNode;
    vpTransformConstraintId follow;
    vpTransform offset;
    vpBody* body;
    Demo demo;
    vp_u16 i;

    wd.maxBodies = 16;
    wd.maxContacts = 64;
    wd.maxJoints = 16;
    wd.solverIters = 12;
    if (vpWorldMemSize(&wd) > WORLD_ARENA_BYTES) return 10;
    world = vpWorldInit(g_world_mem, WORLD_ARENA_BYTES, &wd);
    if (!world) return 11;

    td.maxTransformNodes = 16;
    td.maxTransformConstraints = 16;
    td.transformIterations = 2;
    if (vpTotalSolverMemSize(&td) > TOTAL_ARENA_BYTES) return 12;
    total = vpTotalSolverInit(g_total_mem, TOTAL_ARENA_BYTES, world, &td);
    if (!total) return 13;

    for (i = 0; i < EXT_OBJECTS; ++i) {
        demo.objects[i].used = 0;
        demo.objects[i].world = vpTransformIdentity();
    }
    demo.customComposeCalls = 0;
    demo.radius = VP_FX_HALF;

    demo.ball = vpBodyCreateDynamic(world, vp_fx_from_int(1));
    vpBodySetPose(world, demo.ball,
        vpVec3_make(0, vp_fx_from_int(5), 0), vpQuat_identity());
    vpBodySetInertiaInvDiag(world, demo.ball,
        vpVec3_make(VP_FX_ONE, VP_FX_ONE, VP_FX_ONE));

    demo.objects[0].used = 1; /* external body representation */
    demo.objects[0].world.position = vpVec3_make(0, vp_fx_from_int(5), 0);
    demo.objects[1].used = 1; /* attached socket */

    tp.user = &demo;
    tp.readLocal = 0;
    tp.readWorld = demo_read_world;
    tp.writeLocal = 0;
    tp.writeWorld = demo_write_world;
    tp.getParent = 0;
    vpTotalSolverSetTransformProvider(total, &tp);

    mp.user = &demo;
    mp.capabilities = VP_TRANSFORM_MATH_COMPOSE;
    mp.compose = demo_compose;
    mp.inverse = 0;
    mp.transformPoint = 0;
    mp.rotateVector = 0;
    mp.quatMul = 0;
    mp.quatNormalize = 0;
    mp.blend = 0;
    mp.lookAt = 0;
    vpTotalSolverSetMathProvider(total, &mp);

    cp.user = &demo;
    cp.beginStep = 0;
    cp.generateContacts = demo_collisions;
    cp.endStep = 0;
    cp.bodySweepTOI = 0;
    vpTotalSolverSetCollisionProvider(total, &cp);

    bodyNode = vpTransformNodeCreate(total, 1);
    socketNode = vpTransformNodeCreate(total, 2);
    vpTransformNodeBindBody(total, bodyNode, demo.ball, VP_TRANSFORM_AUTH_PHYSICS);

    follow = vpTransformConstraintCreate(total,
        VP_TRANSFORM_CONSTRAINT_FOLLOW,
        VP_TRANSFORM_PHASE_POST_PHYSICS,
        socketNode, bodyNode, 100);
    offset = vpTransformIdentity();
    offset.position = vpVec3_make(vp_fx_from_int(1), 0, 0);
    vpTransformConstraintSetOffset(total, follow, &offset);

    for (i = 0; i < 180; ++i) {
        vpTotalSolverStep(total, (vp_fx)(VP_FX_ONE / 60));
    }

    body = vpWorldGetBody(world, demo.ball);
    if (!body) return 14;
    printf("ball_y_q16=%ld socket_x_q16=%ld contacts=%u math_calls=%lu\n",
        (long)body->pos.y,
        (long)demo.objects[1].world.position.x,
        (unsigned)world->contactCount,
        (unsigned long)demo.customComposeCalls);

    if (body->pos.y < (vp_fx)(demo.radius - (VP_FX_ONE >> 5))) return 20;
    if (demo.objects[1].world.position.x < (vp_fx)(VP_FX_ONE - (VP_FX_ONE >> 4))) return 21;
    if (demo.customComposeCalls == 0) return 22;
    return 0;
}
