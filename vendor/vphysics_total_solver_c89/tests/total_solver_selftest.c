#include <stdio.h>
#include "vp_api.h"

#define RAW_WORLD_BYTES 1200000UL
#define TOTAL_BYTES 131072UL
#define OBJECT_COUNT 3

static vp_u8 g_world_raw[RAW_WORLD_BYTES + VP_ARENA_ALIGNMENT];
static vp_u8 g_total_raw[TOTAL_BYTES + VP_ARENA_ALIGNMENT];

typedef struct TestObject {
    vp_u8 used;
    vp_u32 parent;
    vpTransform world;
} TestObject;

typedef struct TestState {
    TestObject object[OBJECT_COUNT];
    vpBodyId ball;
    vpBodyId sensorA;
    vpBodyId sensorB;
    vp_fx radius;
    vp_u32 composeCalls;
    vp_u32 legacyPreCalls;
    vp_u32 legacyPostCalls;
    vp_u32 collisionCalls;
} TestState;

static int read_world(void* user, vp_u32 id, vpTransform* out)
{
    TestState* state = (TestState*)user;
    if (id == 0 || id > OBJECT_COUNT || !state->object[id - 1].used) return 0;
    *out = state->object[id - 1].world;
    return 1;
}

static int write_world(void* user, vp_u32 id, const vpTransform* value)
{
    TestState* state = (TestState*)user;
    if (id == 0 || id > OBJECT_COUNT || !state->object[id - 1].used) return 0;
    state->object[id - 1].world = *value;
    return 1;
}

static int get_parent(void* user, vp_u32 id, vp_u32* outParent)
{
    TestState* state = (TestState*)user;
    if (id == 0 || id > OBJECT_COUNT || !state->object[id - 1].used) return 0;
    *outParent = state->object[id - 1].parent;
    return 1;
}

static void external_compose(void* user, vpTransform* out,
    const vpTransform* parent, const vpTransform* local)
{
    TestState* state = (TestState*)user;
    vpVec3 scaled;
    vpVec3 rotated;
    state->composeCalls += 1;
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

static void legacy_pre(void* user, vpWorld* world, vp_fx dt)
{
    TestState* state = (TestState*)user;
    VP_UNUSED(world);
    VP_UNUSED(dt);
    state->legacyPreCalls += 1;
}

static void legacy_post(void* user, vpWorld* world, vp_fx dt)
{
    TestState* state = (TestState*)user;
    VP_UNUSED(world);
    VP_UNUSED(dt);
    state->legacyPostCalls += 1;
}

static void collision_generate(void* user, vpTotalSolver* solver, vpWorld* world, vp_fx dt)
{
    TestState* state = (TestState*)user;
    vpBody* body = vpWorldGetBody(world, state->ball);
    VP_UNUSED(solver);
    VP_UNUSED(dt);
    state->collisionCalls += 1;
    if (body && body->used && body->pos.y < state->radius) {
        vpVec3 point = body->pos;
        point.y = state->radius;
        /* Normal points from body A (world body 0) toward body B (ball). */
        vpContactsAddEx(world, 0, state->ball, point,
            vpVec3_make(0, VP_FX_ONE, 0),
            state->radius - body->pos.y,
            (vp_fx)(VP_FX_ONE * 4L / 5L), 0,
            VP_CONTACT_FLAG_NONE);
    }
}

int main(void)
{
    vpWorldDesc worldDesc;
    vpTotalSolverDesc totalDesc;
    vpWorld* world;
    vpTotalSolver* total;
    vpCallbacks legacy;
    vpTransformProvider transformProvider;
    vpTransformMathProvider mathProvider;
    vpCollisionProvider collisionProvider;
    vpTransformNodeId bodyNode;
    vpTransformNodeId childNode;
    vpTransformConstraintId follow;
    vpTransform offset;
    vpTransform testTransform;
    vpVec3 point;
    vpVec3 transformed;
    TestState state;
    vpBody* body;
    vp_u32 worldNeed;
    vp_u32 totalNeed;
    vp_u32 beforeCCD;
    vp_u16 i;

    worldDesc.maxBodies = 16;
    worldDesc.maxContacts = 64;
    worldDesc.maxJoints = 16;
    worldDesc.solverIters = 12;
    worldNeed = vpWorldMemSize(&worldDesc);
    if (worldNeed > RAW_WORLD_BYTES) return 10;
    world = vpWorldInit(g_world_raw + 1, worldNeed, &worldDesc);
    if (!world) return 11;
    if (((unsigned long)world & (VP_ARENA_ALIGNMENT - 1UL)) != 0) return 12;
    if (((unsigned long)world->bodies & (VP_ARENA_ALIGNMENT - 1UL)) != 0) return 13;
    if (((unsigned long)world->contacts & (VP_ARENA_ALIGNMENT - 1UL)) != 0) return 14;
    if (((unsigned long)world->joints & (VP_ARENA_ALIGNMENT - 1UL)) != 0) return 15;

    totalDesc.maxTransformNodes = 16;
    totalDesc.maxTransformConstraints = 16;
    totalDesc.transformIterations = 2;
    totalNeed = vpTotalSolverMemSize(&totalDesc);
    if (totalNeed > TOTAL_BYTES) return 16;
    total = vpTotalSolverInit(g_total_raw + 1, totalNeed, world, &totalDesc);
    if (!total) return 17;

    for (i = 0; i < OBJECT_COUNT; ++i) {
        state.object[i].used = 0;
        state.object[i].parent = 0;
        state.object[i].world = vpTransformIdentity();
    }
    state.composeCalls = 0;
    state.legacyPreCalls = 0;
    state.legacyPostCalls = 0;
    state.collisionCalls = 0;
    state.radius = VP_FX_HALF;

    state.ball = vpBodyCreateDynamic(world, vp_fx_from_int(1));
    state.sensorA = vpBodyCreateKinematic(world);
    state.sensorB = vpBodyCreateKinematic(world);
    if (!state.ball || !state.sensorA || !state.sensorB) return 18;
    vpBodySetPose(world, state.ball,
        vpVec3_make(0, vp_fx_from_int(5), 0), vpQuat_identity());
    vpBodySetInertiaInvDiag(world, state.ball,
        vpVec3_make(VP_FX_ONE, VP_FX_ONE, VP_FX_ONE));

    state.object[0].used = 1;
    state.object[0].world.position = vpVec3_make(0, vp_fx_from_int(5), 0);
    state.object[1].used = 1;
    state.object[1].parent = 1;

    legacy.user = &state;
    legacy.preStep = legacy_pre;
    legacy.postStep = legacy_post;
    legacy.bodySweepTOI = 0;
    vpWorldSetCallbacks(world, &legacy);

    transformProvider.user = &state;
    transformProvider.readLocal = 0;
    transformProvider.readWorld = read_world;
    transformProvider.writeLocal = 0;
    transformProvider.writeWorld = write_world;
    transformProvider.getParent = get_parent;
    vpTotalSolverSetTransformProvider(total, &transformProvider);

    mathProvider.user = &state;
    mathProvider.capabilities = VP_TRANSFORM_MATH_COMPOSE;
    mathProvider.compose = external_compose;
    mathProvider.inverse = 0;
    mathProvider.transformPoint = 0;
    mathProvider.rotateVector = 0;
    mathProvider.quatMul = 0;
    mathProvider.quatNormalize = 0;
    mathProvider.blend = 0;
    mathProvider.lookAt = 0;
    vpTotalSolverSetMathProvider(total, &mathProvider);

    collisionProvider.user = &state;
    collisionProvider.beginStep = 0;
    collisionProvider.generateContacts = collision_generate;
    collisionProvider.endStep = 0;
    collisionProvider.bodySweepTOI = 0;
    vpTotalSolverSetCollisionProvider(total, &collisionProvider);

    bodyNode = vpTransformNodeCreate(total, 1);
    childNode = vpTransformNodeCreate(total, 2);
    if (!bodyNode || !childNode) return 19;
    vpTransformNodeBindBody(total, bodyNode, state.ball, VP_TRANSFORM_AUTH_PHYSICS);

    follow = vpTransformConstraintCreate(total,
        VP_TRANSFORM_CONSTRAINT_FOLLOW,
        VP_TRANSFORM_PHASE_POST_PHYSICS,
        childNode, bodyNode, 100);
    if (!follow) return 20;
    offset = vpTransformIdentity();
    offset.position = vpVec3_make(vp_fx_from_int(1), 0, 0);
    vpTransformConstraintSetOffset(total, follow, &offset);

    /* A contact appended before the step must survive Total Solver attachment. */
    vpContactsAddEx(world, state.sensorA, state.sensorB,
        vpVec3_make(0,0,0), vpVec3_make(VP_FX_ONE,0,0),
        0, VP_FX_ONE, 0, VP_CONTACT_FLAG_SENSOR);
    vpTotalSolverStep(total, (vp_fx)(VP_FX_ONE / 60));
    {
        vp_u16 eventIndex;
        int foundSensorBegin = 0;
        for (eventIndex = 0; eventIndex < vpWorldGetEventCount(world); ++eventIndex) {
            const vpEvent* event = vpWorldGetEvent(world, eventIndex);
            if (event && event->type == VP_EVENT_CONTACT_BEGIN &&
                event->bodyA == state.sensorA && event->bodyB == state.sensorB)
                foundSensorBegin = 1;
        }
        if (!foundSensorBegin) return 32;
    }
    for (i = 1; i < 180; ++i)
        vpTotalSolverStep(total, (vp_fx)(VP_FX_ONE / 60));

    body = vpWorldGetBody(world, state.ball);
    if (!body) return 21;
    if (body->pos.y < (state.radius - (VP_FX_ONE >> 5))) return 22;
    if (vp_fx_abs(body->v_lin.y) > (vp_fx)VP_SLEEP_LIN_THRESHOLD) return 23;
    if (state.object[1].world.position.x < (VP_FX_ONE - (VP_FX_ONE >> 4))) return 24;
    if (state.composeCalls == 0) return 25;
    if (state.legacyPreCalls != 180 || state.legacyPostCalls != 180) return 26;
    if (state.collisionCalls != 180) return 27;
    if (total->providerErrorCount != 0 || total->cycleCount != 0) return 28;

    /* Public math service must fall back for operations not supplied by provider. */
    testTransform = vpTransformIdentity();
    testTransform.position = vpVec3_make(vp_fx_from_int(2), 0, 0);
    point = vpVec3_make(vp_fx_from_int(3), 0, 0);
    vpTotalSolverMathTransformPoint(total, &transformed, &testTransform, &point);
    if (transformed.x != vp_fx_from_int(5)) return 29;

    /* No sweep callback: Total Solver must preserve uniform CCD substepping. */
    beforeCCD = world->stepId;
    vpTotalSolverStepCCD(total, (vp_fx)(VP_FX_ONE / 60), 4);
    if (world->stepId != beforeCCD + 4) return 30;

    vpTotalSolverDetach(total);
    if (!world->callbacksEnabled || world->callbacks.preStep != legacy_pre) return 31;

    printf("total_solver_selftest: OK y=%ld compose=%lu legacy=%lu collision=%lu\n",
        (long)body->pos.y,
        (unsigned long)state.composeCalls,
        (unsigned long)state.legacyPreCalls,
        (unsigned long)state.collisionCalls);
    return 0;
}
