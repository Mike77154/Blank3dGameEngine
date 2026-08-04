#include "vp_world.h"
#include "vp_solver.h"
#include "vp_config.h"
#include <stddef.h>

/* memset without stdlib */
static void vp_memzero(void* p, vp_u32 bytes)
{
    vp_u8* b = (vp_u8*)p;
    vp_u32 i;
    for (i = 0; i < bytes; ++i) b[i] = 0;
}

static vp_u32 vp_align_size(vp_u32 value)
{
    vp_u32 a = (vp_u32)VP_ARENA_ALIGNMENT;
    vp_u32 mask = a - 1U;
    return (value + mask) & ~mask;
}

static vp_u8* vp_align_ptr(vp_u8* p)
{
    size_t v = (size_t)p;
    size_t a = (size_t)VP_ARENA_ALIGNMENT;
    size_t mask = a - (size_t)1;
    v = (v + mask) & ~mask;
    return (vp_u8*)v;
}

static void vp_push_event(vpWorld* w, vpEventType type, vp_u16 a, vp_u16 b, vp_u16 jointId, vp_u32 pairKey)
{
    if (!w->events) return;
    if (w->eventCount >= (vp_u16)VP_EVENT_QUEUE_SIZE) return;
    {
        vpEvent* e = &w->events[w->eventCount++];
        e->type = (vp_u8)type;
        e->_pad = 0;
        e->bodyA = a;
        e->bodyB = b;
        e->jointId = jointId;
        e->pairKey = pairKey;
    }
}

vp_u32 vpWorldMemSize(const vpWorldDesc* desc)
{
    vp_u32 bytes = (vp_u32)VP_ARENA_ALIGNMENT - 1U;
#define VP_ADD_SEGMENT(type, count) \
    do { bytes = vp_align_size(bytes); bytes += (vp_u32)sizeof(type) * (vp_u32)(count); } while (0)

    VP_ADD_SEGMENT(vpWorld, 1);
    VP_ADD_SEGMENT(vpBody, desc->maxBodies);
    VP_ADD_SEGMENT(vpContact, desc->maxContacts);
    VP_ADD_SEGMENT(vpContact, desc->maxContacts);
    VP_ADD_SEGMENT(vpJoint, desc->maxJoints);
    VP_ADD_SEGMENT(vpContactCacheEntry, VP_CONTACT_CACHE_SIZE);
    VP_ADD_SEGMENT(vpPairCacheEntry, VP_PAIR_CACHE_SIZE);
    VP_ADD_SEGMENT(vpEvent, VP_EVENT_QUEUE_SIZE);
    VP_ADD_SEGMENT(vp_u16, desc->maxContacts);
    VP_ADD_SEGMENT(vp_u8, desc->maxBodies + 1U);
#if VP_ENABLE_ISLANDS
    VP_ADD_SEGMENT(vp_u16, desc->maxBodies + 1U);
    VP_ADD_SEGMENT(vpLink, 2U * (vp_u32)(desc->maxContacts + desc->maxJoints));
    VP_ADD_SEGMENT(vp_u8, desc->maxBodies + 1U);
    VP_ADD_SEGMENT(vp_u8, desc->maxContacts);
    VP_ADD_SEGMENT(vp_u8, desc->maxJoints);
    VP_ADD_SEGMENT(vp_u16, desc->maxBodies);
    VP_ADD_SEGMENT(vp_u16, desc->maxBodies);
    VP_ADD_SEGMENT(vp_u16, desc->maxContacts);
    VP_ADD_SEGMENT(vp_u16, desc->maxJoints);
#endif
#undef VP_ADD_SEGMENT
    return bytes;
}

vpWorld* vpWorldInit(void* mem, vp_u32 memBytes, const vpWorldDesc* desc)
{
    vp_u32 need = vpWorldMemSize(desc);
    vp_u8* p;
    vpWorld* w;

    if (memBytes < need) return (vpWorld*)0;
    p = vp_align_ptr((vp_u8*)mem);

    w = (vpWorld*)p;
    p += sizeof(vpWorld);

    w->maxBodies = desc->maxBodies;
    w->maxContacts = desc->maxContacts;
    w->maxJoints = desc->maxJoints;
    w->solverIters = (desc->solverIters != 0) ? desc->solverIters : (vp_u16)VP_SOLVER_ITERS;

    w->gravity = vpVec3_make(0, -vp_fx_from_int(10), 0);

    w->defaultFriction = vp_fx_from_int(1); /* 1.0 */
    w->defaultRestitution = 0;

    w->waterEnabled = 0;
    w->waterY = 0;
    w->waterDensity = (vp_fx)VP_WATER_DENSITY_DEFAULT;

    p = vp_align_ptr(p);
    w->bodies = (vpBody*)p;
    p += sizeof(vpBody) * desc->maxBodies;

    p = vp_align_ptr(p);
    w->contacts = (vpContact*)p;
    p += sizeof(vpContact) * desc->maxContacts;

    p = vp_align_ptr(p);
    w->contactsRaw = (vpContact*)p;
    p += sizeof(vpContact) * desc->maxContacts;

    p = vp_align_ptr(p);
    w->joints = (vpJoint*)p;
    p += sizeof(vpJoint) * desc->maxJoints;

    p = vp_align_ptr(p);
    w->contactCache = (vpContactCacheEntry*)p;
    p += sizeof(vpContactCacheEntry) * (vp_u32)VP_CONTACT_CACHE_SIZE;
    w->contactCacheMask = (vp_u32)VP_CONTACT_CACHE_SIZE - 1U;

    p = vp_align_ptr(p);
    w->pairCache = (vpPairCacheEntry*)p;
    p += sizeof(vpPairCacheEntry) * (vp_u32)VP_PAIR_CACHE_SIZE;
    w->pairCacheMask = (vp_u32)VP_PAIR_CACHE_SIZE - 1U;

    p = vp_align_ptr(p);
    w->events = (vpEvent*)p;
    p += sizeof(vpEvent) * (vp_u32)VP_EVENT_QUEUE_SIZE;

    p = vp_align_ptr(p);
    w->contactSort = (vp_u16*)p;
    p += sizeof(vp_u16) * (vp_u32)desc->maxContacts;

    p = vp_align_ptr(p);
    w->asleepPrev = (vp_u8*)p;
    p += sizeof(vp_u8) * (vp_u32)(desc->maxBodies + 1);

#if VP_ENABLE_ISLANDS
    p = vp_align_ptr(p);
    w->bodyFirstLink = (vp_u16*)p;
    p += sizeof(vp_u16) * (vp_u32)(desc->maxBodies + 1);

    p = vp_align_ptr(p);
    w->links = (vpLink*)p;
    w->linkCap = (vp_u16)(2U * (desc->maxContacts + desc->maxJoints));
    p += sizeof(vpLink) * (vp_u32)w->linkCap;

    p = vp_align_ptr(p);
    w->bodyVisited = (vp_u8*)p;
    p += sizeof(vp_u8) * (vp_u32)(desc->maxBodies + 1);

    p = vp_align_ptr(p);
    w->contactVisited = (vp_u8*)p;
    p += sizeof(vp_u8) * (vp_u32)desc->maxContacts;

    p = vp_align_ptr(p);
    w->jointVisited = (vp_u8*)p;
    p += sizeof(vp_u8) * (vp_u32)desc->maxJoints;

    p = vp_align_ptr(p);
    w->islandBodyStack = (vp_u16*)p;
    p += sizeof(vp_u16) * (vp_u32)desc->maxBodies;

    p = vp_align_ptr(p);
    w->islandBodies = (vp_u16*)p;
    p += sizeof(vp_u16) * (vp_u32)desc->maxBodies;

    p = vp_align_ptr(p);
    w->islandContacts = (vp_u16*)p;
    p += sizeof(vp_u16) * (vp_u32)desc->maxContacts;

    p = vp_align_ptr(p);
    w->islandJoints = (vp_u16*)p;
    p += sizeof(vp_u16) * (vp_u32)desc->maxJoints;

    w->linkCount = 0;
#endif

    w->contactCount = 0;
    w->contactRawCount = 0;

    w->stepId = 0;
    w->nextContactKey = 1;

    w->eventCount = 0;

    w->callbacksEnabled = 0;
    vp_memzero(&w->callbacks, (vp_u32)sizeof(vpCallbacks));

    vp_memzero(w->bodies, (vp_u32)sizeof(vpBody) * (vp_u32)desc->maxBodies);
    vp_memzero(w->contacts, (vp_u32)sizeof(vpContact) * (vp_u32)desc->maxContacts);
    vp_memzero(w->contactsRaw, (vp_u32)sizeof(vpContact) * (vp_u32)desc->maxContacts);
    vp_memzero(w->joints, (vp_u32)sizeof(vpJoint) * (vp_u32)desc->maxJoints);
    vp_memzero(w->contactCache, (vp_u32)sizeof(vpContactCacheEntry) * (vp_u32)VP_CONTACT_CACHE_SIZE);
    vp_memzero(w->pairCache, (vp_u32)sizeof(vpPairCacheEntry) * (vp_u32)VP_PAIR_CACHE_SIZE);
    vp_memzero(w->events, (vp_u32)sizeof(vpEvent) * (vp_u32)VP_EVENT_QUEUE_SIZE);
    vp_memzero(w->contactSort, (vp_u32)sizeof(vp_u16) * (vp_u32)desc->maxContacts);
    vp_memzero(w->asleepPrev, (vp_u32)sizeof(vp_u8) * (vp_u32)(desc->maxBodies + 1));

#if VP_ENABLE_ISLANDS
    vp_memzero(w->bodyFirstLink, (vp_u32)sizeof(vp_u16) * (vp_u32)(desc->maxBodies + 1));
    vp_memzero(w->links, (vp_u32)sizeof(vpLink) * (vp_u32)w->linkCap);
    vp_memzero(w->bodyVisited, (vp_u32)sizeof(vp_u8) * (vp_u32)(desc->maxBodies + 1));
    vp_memzero(w->contactVisited, (vp_u32)sizeof(vp_u8) * (vp_u32)desc->maxContacts);
    vp_memzero(w->jointVisited, (vp_u32)sizeof(vp_u8) * (vp_u32)desc->maxJoints);
    vp_memzero(w->islandBodyStack, (vp_u32)sizeof(vp_u16) * (vp_u32)desc->maxBodies);
    vp_memzero(w->islandBodies, (vp_u32)sizeof(vp_u16) * (vp_u32)desc->maxBodies);
    vp_memzero(w->islandContacts, (vp_u32)sizeof(vp_u16) * (vp_u32)desc->maxContacts);
    vp_memzero(w->islandJoints, (vp_u32)sizeof(vp_u16) * (vp_u32)desc->maxJoints);
#endif

    /* static world body id=0 */
    vp_memzero(&w->staticBody, (vp_u32)sizeof(vpBody));
    w->staticBody.used = 1;
    w->staticBody.isKinematic = 1;
    w->staticBody.invMass = 0;
    w->staticBody.pos = vpVec3_make(0,0,0);
    w->staticBody.rot = vpQuat_identity();
    w->staticBody.prevPos = w->staticBody.pos;
    w->staticBody.prevRot = w->staticBody.rot;

    return w;
}

void vpWorldSetGravity(vpWorld* w, vpVec3 g) { w->gravity = g; }

void vpWorldSetDefaultMaterial(vpWorld* w, vp_fx friction, vp_fx restitution)
{
    w->defaultFriction = friction;
    w->defaultRestitution = restitution;
}

void vpWorldSetCallbacks(vpWorld* w, const vpCallbacks* cb)
{
    if (!cb) {
        w->callbacksEnabled = 0;
        vp_memzero(&w->callbacks, (vp_u32)sizeof(vpCallbacks));
        return;
    }
    w->callbacks = *cb;
    w->callbacksEnabled = 1;
}

void vpWorldEnableWater(vpWorld* w, int enabled) { w->waterEnabled = (vp_u8)(enabled ? 1 : 0); }

void vpWorldSetWaterPlane(vpWorld* w, vp_fx waterY, vp_fx density)
{
    w->waterY = waterY;
    w->waterDensity = density;
}

vpBody* vpWorldGetBody(vpWorld* w, vpBodyId id)
{
    if (id == 0) return &w->staticBody;
    if ((vp_u16)(id - 1) >= w->maxBodies) return (vpBody*)0;
    return &w->bodies[id - 1];
}

vpJoint* vpWorldGetJoint(vpWorld* w, vpJointId id)
{
    if (id == 0) return (vpJoint*)0;
    if ((vp_u16)(id - 1) >= w->maxJoints) return (vpJoint*)0;
    return &w->joints[id - 1];
}

/* buoyancy + drag */
static void vp_apply_buoyancy(vpWorld* w, vpBody* b, vp_fx dt)
{
    vp_fx twoH, depth, sub;
    vp_fx gmag, Fb;
    vpVec3 F;
    vp_fx k;

    if (!w->waterEnabled) return;
    if (!b->buoyEnabled) return;
    if (b->invMass == 0) return;

    twoH = b->buoyHalfHeight + b->buoyHalfHeight;
    if (twoH <= 0) return;

    depth = w->waterY - b->pos.y;
    sub = vp_fx_div((depth + b->buoyHalfHeight), twoH);
    sub = vp_fx_clamp(sub, 0, VP_FX_ONE);

    gmag = vp_fx_abs(w->gravity.y);
    Fb = vp_fx_mul(vp_fx_mul(w->waterDensity, b->buoyVolume), vp_fx_mul(gmag, sub));

    F = vpVec3_make(0, Fb, 0);
    b->force = vpVec3_add(b->force, F);

    k = vp_fx_mul(b->waterLinDrag, sub);
    k = vp_fx_mul(k, dt);
    if (k > VP_FX_ONE) k = VP_FX_ONE;
    b->v_lin.x -= vp_fx_mul(b->v_lin.x, k);
    b->v_lin.y -= vp_fx_mul(b->v_lin.y, k);
    b->v_lin.z -= vp_fx_mul(b->v_lin.z, k);

    k = vp_fx_mul(b->waterAngDrag, sub);
    k = vp_fx_mul(k, dt);
    if (k > VP_FX_ONE) k = VP_FX_ONE;
    b->w_ang.x -= vp_fx_mul(b->w_ang.x, k);
    b->w_ang.y -= vp_fx_mul(b->w_ang.y, k);
    b->w_ang.z -= vp_fx_mul(b->w_ang.z, k);
}

static void vp_apply_damping(vpBody* b, vp_fx dt)
{
    vp_fx k;
    if (b->invMass == 0) return;

    k = vp_fx_mul(b->linearDamp, dt);
    if (k > VP_FX_ONE) k = VP_FX_ONE;
    b->v_lin.x -= vp_fx_mul(b->v_lin.x, k);
    b->v_lin.y -= vp_fx_mul(b->v_lin.y, k);
    b->v_lin.z -= vp_fx_mul(b->v_lin.z, k);

    k = vp_fx_mul(b->angularDamp, dt);
    if (k > VP_FX_ONE) k = VP_FX_ONE;
    b->w_ang.x -= vp_fx_mul(b->w_ang.x, k);
    b->w_ang.y -= vp_fx_mul(b->w_ang.y, k);
    b->w_ang.z -= vp_fx_mul(b->w_ang.z, k);
}

static void vp_reset_step_transients(vpWorld* w)
{
    vp_u16 i;
    vpBody* b;

    /* snapshot sleep state for events */
    w->asleepPrev[0] = 0;
    for (i = 0; i < w->maxBodies; ++i) {
        b = &w->bodies[i];
        w->asleepPrev[i + 1] = (vp_u8)(b->used ? b->asleep : 0);
    }

    /* clear events */
    w->eventCount = 0;

    /* reset kinematicMoved and split impulse velocities */
    for (i = 0; i < w->maxBodies; ++i) {
        b = &w->bodies[i];
        if (!b->used) continue;

        b->kinematicMoved = 0;

#if VP_ENABLE_SPLIT_IMPULSE
        b->v_bias = vpVec3_make(0,0,0);
        b->w_bias = vpVec3_make(0,0,0);
#endif
    }
}

static void vp_update_kinematics(vpWorld* w, vp_fx dt)
{
    vp_u16 i;
    for (i = 0; i < w->maxBodies; ++i) {
        vpBody* b = &w->bodies[i];
        vpVec3 dp;
        vp_fx invDt;

        if (!b->used) continue;
        if (!b->isKinematic) continue;

        dp = vpVec3_sub(b->pos, b->prevPos);
        if (dp.x != 0 || dp.y != 0 || dp.z != 0) b->kinematicMoved = 1;

        if (dt != 0) {
            invDt = vp_fx_div(VP_FX_ONE, dt);
            b->v_lin = vpVec3_scale(dp, invDt);
            b->w_ang = vpQuat_delta_to_angvel(b->prevRot, b->rot, dt);
            if (b->w_ang.x != 0 || b->w_ang.y != 0 || b->w_ang.z != 0) b->kinematicMoved = 1;
        } else {
            b->v_lin = vpVec3_make(0,0,0);
            b->w_ang = vpVec3_make(0,0,0);
        }
    }
}

#if VP_ENABLE_ISLANDS
static vp_u8 vp_is_dynamic(const vpBody* b)
{
    return (vp_u8)((b->used) && (b->invMass != 0));
}

static void vp_islands_clear(vpWorld* w)
{
    vp_u16 i;
    for (i = 0; i <= w->maxBodies; ++i) w->bodyFirstLink[i] = (vp_u16)0xFFFF;
    for (i = 0; i < w->maxBodies + 1; ++i) w->bodyVisited[i] = 0;
    for (i = 0; i < w->contactCount; ++i) w->contactVisited[i] = 0;
    for (i = 0; i < w->maxJoints; ++i) w->jointVisited[i] = 0;
    w->linkCount = 0;
}

static void vp_link_add(vpWorld* w, vp_u16 fromBody, vp_u16 otherBody, vp_u16 kind, vp_u16 index)
{
    vp_u16 li;
    if (w->linkCount >= w->linkCap) return;
    li = w->linkCount++;
    w->links[li].other = otherBody;
    w->links[li].kind = kind;
    w->links[li].index = index;
    w->links[li].next = w->bodyFirstLink[fromBody];
    w->bodyFirstLink[fromBody] = li;
}

static void vp_build_adjacency(vpWorld* w)
{
    vp_u16 i;
    /* contacts */
    for (i = 0; i < w->contactCount; ++i) {
        vpContact* c = &w->contacts[i];
        vpBody* A = vpWorldGetBody(w, (vpBodyId)c->bodyA);
        vpBody* B = vpWorldGetBody(w, (vpBodyId)c->bodyB);
        vp_u8 dynA, dynB;

        if (!A || !B) continue;
        if (c->flags & VP_CONTACT_FLAG_SENSOR) continue;

        dynA = (vp_u8)(A->invMass != 0);
        dynB = (vp_u8)(B->invMass != 0);

        if (!dynA && !dynB) continue;

        if (dynA) vp_link_add(w, c->bodyA, dynB ? c->bodyB : 0, 0, i);
        if (dynB) vp_link_add(w, c->bodyB, dynA ? c->bodyA : 0, 0, i);
    }

    /* joints */
    for (i = 0; i < w->maxJoints; ++i) {
        vpJoint* j = &w->joints[i];
        vpBody* A;
        vpBody* B;
        vp_u8 dynA, dynB;

        if (!j->used || j->broken) continue;
        A = vpWorldGetBody(w, (vpBodyId)j->bodyA);
        B = vpWorldGetBody(w, (vpBodyId)j->bodyB);
        if (!A || !B) continue;

        dynA = (vp_u8)(A->invMass != 0);
        dynB = (vp_u8)(B->invMass != 0);
        if (!dynA && !dynB) continue;

        if (dynA) vp_link_add(w, j->bodyA, dynB ? j->bodyB : 0, 1, i);
        if (dynB) vp_link_add(w, j->bodyB, dynA ? j->bodyA : 0, 1, i);
    }
}

static void vp_seed_from_kinematics(vpWorld* w)
{
    vp_u16 i;
    /* contacts: wake dynamic bodies touched by moved kinematics */
    for (i = 0; i < w->contactCount; ++i) {
        vpContact* c = &w->contacts[i];
        vpBody* A = vpWorldGetBody(w, (vpBodyId)c->bodyA);
        vpBody* B = vpWorldGetBody(w, (vpBodyId)c->bodyB);

        if (!A || !B) continue;
        if (c->flags & VP_CONTACT_FLAG_SENSOR) continue;

        /* A dynamic, B kinematic moved */
        if (A->invMass != 0 && B->invMass == 0 && B->isKinematic && B->kinematicMoved) {
            A->asleep = 0;
            A->sleepCounter = 0;
        }
        if (B->invMass != 0 && A->invMass == 0 && A->isKinematic && A->kinematicMoved) {
            B->asleep = 0;
            B->sleepCounter = 0;
        }
    }

    /* joints: wake dynamic bodies connected to moved kinematics */
    for (i = 0; i < w->maxJoints; ++i) {
        vpJoint* j = &w->joints[i];
        vpBody* A;
        vpBody* B;

        if (!j->used || j->broken) continue;
        A = vpWorldGetBody(w, (vpBodyId)j->bodyA);
        B = vpWorldGetBody(w, (vpBodyId)j->bodyB);
        if (!A || !B) continue;

        if (A->invMass != 0 && B->invMass == 0 && B->isKinematic && B->kinematicMoved) {
            A->asleep = 0;
            A->sleepCounter = 0;
        }
        if (B->invMass != 0 && A->invMass == 0 && A->isKinematic && A->kinematicMoved) {
            B->asleep = 0;
            B->sleepCounter = 0;
        }
    }
}

static vp_u8 vp_body_below_sleep_threshold(const vpBody* b)
{
    if (!b->used) return 1;
    if (b->invMass == 0) return 1;
    if (vp_fx_abs(b->v_lin.x) >= (vp_fx)VP_SLEEP_LIN_THRESHOLD) return 0;
    if (vp_fx_abs(b->v_lin.y) >= (vp_fx)VP_SLEEP_LIN_THRESHOLD) return 0;
    if (vp_fx_abs(b->v_lin.z) >= (vp_fx)VP_SLEEP_LIN_THRESHOLD) return 0;
    if (vp_fx_abs(b->w_ang.x) >= (vp_fx)VP_SLEEP_ANG_THRESHOLD) return 0;
    if (vp_fx_abs(b->w_ang.y) >= (vp_fx)VP_SLEEP_ANG_THRESHOLD) return 0;
    if (vp_fx_abs(b->w_ang.z) >= (vp_fx)VP_SLEEP_ANG_THRESHOLD) return 0;
    return 1;
}
#endif /* VP_ENABLE_ISLANDS */

void vpWorldClearEvents(vpWorld* w)
{
    w->eventCount = 0;
}

vp_u16 vpWorldGetEventCount(const vpWorld* w)
{
    return w->eventCount;
}

const vpEvent* vpWorldGetEvent(const vpWorld* w, vp_u16 index)
{
    if (!w->events) return (const vpEvent*)0;
    if (index >= w->eventCount) return (const vpEvent*)0;
    return &w->events[index];
}

void vpWorldStep(vpWorld* w, vp_fx dt)
{
    vp_u16 i;
    vpBody* b;

    /* advance frame id and clear step-local buffers */
    w->stepId += 1;
    vp_reset_step_transients(w);

    /* optional host hook */
    if (w->callbacksEnabled && w->callbacks.preStep) {
        w->callbacks.preStep(w->callbacks.user, w, dt);
    }

    /* update kinematic velocities from prev pose */
    vp_update_kinematics(w, dt);

    /* buoyancy pre-force */
    for (i = 0; i < w->maxBodies; ++i) {
        b = &w->bodies[i];
        if (!b->used) continue;
        if (b->asleep) continue;
        vp_apply_buoyancy(w, b, dt);
    }

    /* integrate velocities */
    for (i = 0; i < w->maxBodies; ++i) {
        b = &w->bodies[i];
        if (!b->used) continue;
        if (b->asleep) continue;
        if (b->invMass == 0) continue;

        b->v_lin.x += vp_fx_mul((w->gravity.x + vp_fx_mul(b->force.x, b->invMass)), dt);
        b->v_lin.y += vp_fx_mul((w->gravity.y + vp_fx_mul(b->force.y, b->invMass)), dt);
        b->v_lin.z += vp_fx_mul((w->gravity.z + vp_fx_mul(b->force.z, b->invMass)), dt);

        b->invInertiaWorld = vpMat33_inertia_world_from_quat_diag(b->rot, b->invInertiaDiagLocal);
        {
            vpVec3 dw = vpMat33_mul_vec3(b->invInertiaWorld, b->torque);
            b->w_ang.x += vp_fx_mul(dw.x, dt);
            b->w_ang.y += vp_fx_mul(dw.y, dt);
            b->w_ang.z += vp_fx_mul(dw.z, dt);
        }

        /* per-body damping */
        vp_apply_damping(b, dt);

        b->force = vpVec3_make(0,0,0);
        b->torque = vpVec3_make(0,0,0);
    }

    /* finalize contacts (manifold reduction + key assignment + warmstart lookup) */
    vpContactsFinalize(w, dt);
    w->contactRawCount = 0; /* consume */

#if VP_ENABLE_ISLANDS
    vp_islands_clear(w);
    vp_seed_from_kinematics(w);
    vp_build_adjacency(w);

    /* solve each awake island */
    for (i = 1; i <= w->maxBodies; ++i) {
        vpBody* seed = vpWorldGetBody(w, (vpBodyId)i);
        vp_u16 stackN, islandBodyN, islandContactN, islandJointN;
        vp_u16 si;

        if (!seed) continue;
        if (!vp_is_dynamic(seed)) continue;
        if (seed->asleep) continue;
        if (w->bodyVisited[i]) continue;

        /* BFS/DFS stack */
        stackN = 0;
        islandBodyN = 0;
        islandContactN = 0;
        islandJointN = 0;

        w->islandBodyStack[stackN++] = i;
        w->bodyVisited[i] = 1;

        while (stackN) {
            vp_u16 bid = w->islandBodyStack[--stackN];
            vp_u16 link = w->bodyFirstLink[bid];
            vpBody* bb = vpWorldGetBody(w, (vpBodyId)bid);

            w->islandBodies[islandBodyN++] = bid;

            if (bb && bb->asleep) {
                bb->asleep = 0;
                bb->sleepCounter = 0;
            }

            while (link != (vp_u16)0xFFFF) {
                vpLink* L = &w->links[link];

                if (L->kind == 0) {
                    /* contact */
                    if (L->index < w->contactCount && !w->contactVisited[L->index]) {
                        w->contactVisited[L->index] = 1;
                        w->islandContacts[islandContactN++] = L->index;
                    }
                } else {
                    /* joint */
                    if (L->index < w->maxJoints && !w->jointVisited[L->index]) {
                        w->jointVisited[L->index] = 1;
                        w->islandJoints[islandJointN++] = L->index;
                    }
                }

                if (L->other != 0 && !w->bodyVisited[L->other]) {
                    vpBody* ob = vpWorldGetBody(w, (vpBodyId)L->other);
                    w->bodyVisited[L->other] = 1;
                    if (ob && ob->asleep) {
                        ob->asleep = 0;
                        ob->sleepCounter = 0;
                    }
                    w->islandBodyStack[stackN++] = L->other;
                }

                link = L->next;
            }
        }

        /* warm-start + solve for this island */
        vpSolverWarmStartSubset(w, dt, w->islandContacts, islandContactN, w->islandJoints, islandJointN);
        vpSolverSolveSubset(w, dt, w->islandContacts, islandContactN, w->islandJoints, islandJointN);

        /* integrate pose for bodies in this island */
        for (si = 0; si < islandBodyN; ++si) {
            vpBody* db = vpWorldGetBody(w, (vpBodyId)w->islandBodies[si]);
            if (!db) continue;
            if (db->asleep) continue;
            if (db->invMass == 0) continue;

#if VP_ENABLE_SPLIT_IMPULSE
            db->pos.x += vp_fx_mul((db->v_lin.x + db->v_bias.x), dt);
            db->pos.y += vp_fx_mul((db->v_lin.y + db->v_bias.y), dt);
            db->pos.z += vp_fx_mul((db->v_lin.z + db->v_bias.z), dt);
            db->rot = vpQuat_integrate(db->rot, vpVec3_add(db->w_ang, db->w_bias), dt);
#else
            db->pos.x += vp_fx_mul(db->v_lin.x, dt);
            db->pos.y += vp_fx_mul(db->v_lin.y, dt);
            db->pos.z += vp_fx_mul(db->v_lin.z, dt);
            db->rot = vpQuat_integrate(db->rot, db->w_ang, dt);
#endif
        }

        /* island sleep */
        {
            vp_u8 canSleep = 1;
            for (si = 0; si < islandBodyN; ++si) {
                vpBody* db = vpWorldGetBody(w, (vpBodyId)w->islandBodies[si]);
                if (!db) continue;
                if (!vp_body_below_sleep_threshold(db)) { canSleep = 0; break; }
            }

            if (canSleep) {
                for (si = 0; si < islandBodyN; ++si) {
                    vpBody* db = vpWorldGetBody(w, (vpBodyId)w->islandBodies[si]);
                    if (!db) continue;
                    if (db->invMass == 0) continue;
                    if (db->sleepCounter < 65535) db->sleepCounter += 1;
                    if (db->sleepCounter >= (vp_u16)VP_SLEEP_FRAMES) {
                        db->asleep = 1;
                        db->v_lin = vpVec3_make(0,0,0);
                        db->w_ang = vpVec3_make(0,0,0);
#if VP_ENABLE_SPLIT_IMPULSE
                        db->v_bias = vpVec3_make(0,0,0);
                        db->w_bias = vpVec3_make(0,0,0);
#endif
                    }
                }
            } else {
                for (si = 0; si < islandBodyN; ++si) {
                    vpBody* db = vpWorldGetBody(w, (vpBodyId)w->islandBodies[si]);
                    if (!db) continue;
                    if (db->invMass == 0) continue;
                    db->sleepCounter = 0;
                    db->asleep = 0;
                }
            }
        }
    }

#else /* no islands */
    vpSolverWarmStart(w, dt);
    vpSolverSolve(w, dt);

    /* integrate pose */
    for (i = 0; i < w->maxBodies; ++i) {
        b = &w->bodies[i];
        if (!b->used) continue;
        if (b->asleep) continue;
        if (b->invMass == 0) continue;

#if VP_ENABLE_SPLIT_IMPULSE
        b->pos.x += vp_fx_mul((b->v_lin.x + b->v_bias.x), dt);
        b->pos.y += vp_fx_mul((b->v_lin.y + b->v_bias.y), dt);
        b->pos.z += vp_fx_mul((b->v_lin.z + b->v_bias.z), dt);
        b->rot = vpQuat_integrate(b->rot, vpVec3_add(b->w_ang, b->w_bias), dt);
#else
        b->pos.x += vp_fx_mul(b->v_lin.x, dt);
        b->pos.y += vp_fx_mul(b->v_lin.y, dt);
        b->pos.z += vp_fx_mul(b->v_lin.z, dt);
        b->rot = vpQuat_integrate(b->rot, b->w_ang, dt);
#endif
    }

    /* per-body sleep */
    for (i = 0; i < w->maxBodies; ++i) {
        b = &w->bodies[i];
        if (!b->used) continue;
        if (b->invMass == 0) continue;

        if (vp_fx_abs(b->v_lin.x) < (vp_fx)VP_SLEEP_LIN_THRESHOLD &&
            vp_fx_abs(b->v_lin.y) < (vp_fx)VP_SLEEP_LIN_THRESHOLD &&
            vp_fx_abs(b->v_lin.z) < (vp_fx)VP_SLEEP_LIN_THRESHOLD &&
            vp_fx_abs(b->w_ang.x) < (vp_fx)VP_SLEEP_ANG_THRESHOLD &&
            vp_fx_abs(b->w_ang.y) < (vp_fx)VP_SLEEP_ANG_THRESHOLD &&
            vp_fx_abs(b->w_ang.z) < (vp_fx)VP_SLEEP_ANG_THRESHOLD) {

            if (b->sleepCounter < 65535) b->sleepCounter += 1;
            if (b->sleepCounter >= (vp_u16)VP_SLEEP_FRAMES) {
                b->asleep = 1;
                b->v_lin = vpVec3_make(0,0,0);
                b->w_ang = vpVec3_make(0,0,0);
#if VP_ENABLE_SPLIT_IMPULSE
                b->v_bias = vpVec3_make(0,0,0);
                b->w_bias = vpVec3_make(0,0,0);
#endif
            }
        } else {
            b->sleepCounter = 0;
            b->asleep = 0;
        }
    }
#endif

    /* commit contact lambdas to cache for warm-start next frame */
    vpContactsEnd(w);

    /* joint break events */
    for (i = 0; i < w->maxJoints; ++i) {
        vpJoint* j = &w->joints[i];
        if (!j->used) continue;
        if (j->broken && !j->brokenReported) {
            j->brokenReported = 1;
            vp_push_event(w, VP_EVENT_JOINT_BROKE, j->bodyA, j->bodyB, (vp_u16)(i + 1), 0);
        }
    }

    /* sleep/wake events (compare against snapshot) */
    for (i = 0; i < w->maxBodies; ++i) {
        vpBody* bb = &w->bodies[i];
        vp_u16 id = (vp_u16)(i + 1);
        if (!bb->used) continue;
        if (w->asleepPrev[id] && !bb->asleep) {
            vp_push_event(w, VP_EVENT_BODY_WAKE, id, 0, 0, 0);
        } else if (!w->asleepPrev[id] && bb->asleep) {
            vp_push_event(w, VP_EVENT_BODY_SLEEP, id, 0, 0, 0);
        }
    }

    /* update prev transforms for next step */
    for (i = 0; i < w->maxBodies; ++i) {
        vpBody* bb = &w->bodies[i];
        if (!bb->used) continue;
        bb->prevPos = bb->pos;
        bb->prevRot = bb->rot;
        bb->kinematicMoved = 0;
    }

    /* optional host hook */
    if (w->callbacksEnabled && w->callbacks.postStep) {
        w->callbacks.postStep(w->callbacks.user, w, dt);
    }
}

void vpWorldStepCCD(vpWorld* w, vp_fx dt, vp_u16 maxSubSteps)
{
    vp_u16 step;
    vp_fx remaining = dt;

    if (maxSubSteps == 0) maxSubSteps = 1;

    /* If no sweep callback is provided, fall back to uniform sub-stepping. */
    if (!w->callbacksEnabled || !w->callbacks.bodySweepTOI) {
        vp_fx dtSub = vp_fx_div(dt, vp_fx_from_int((vp_i32)maxSubSteps));
        for (step = 0; step < maxSubSteps; ++step) vpWorldStep(w, dtSub);
        return;
    }

    for (step = 0; step < maxSubSteps && remaining > 0; ++step) {
        vp_fx minToi = VP_FX_ONE;
        vp_u16 i;

        /* Find earliest TOI among awake dynamic bodies (host-defined sweep). */
        for (i = 0; i < w->maxBodies; ++i) {
            vpBody* b = &w->bodies[i];
            vp_u16 id = (vp_u16)(i + 1);
            vp_fx toi;
            vpVec3 toPos;
            vpQuat toRot;

            if (!b->used) continue;
            if (b->invMass == 0) continue;
            if (b->asleep) continue;

            toPos.x = b->pos.x + vp_fx_mul(b->v_lin.x, remaining);
            toPos.y = b->pos.y + vp_fx_mul(b->v_lin.y, remaining);
            toPos.z = b->pos.z + vp_fx_mul(b->v_lin.z, remaining);

            toRot = vpQuat_integrate(b->rot, b->w_ang, remaining);

            toi = w->callbacks.bodySweepTOI(w->callbacks.user, (vpBodyId)id,
                b->pos, b->rot, toPos, toRot,
                0, 0, 0);

            if (toi < minToi) minToi = toi;
        }

        if (minToi < 0) minToi = 0;
        if (minToi > VP_FX_ONE) minToi = VP_FX_ONE;

        /* avoid zero dt */
        if (minToi == 0) {
            minToi = vp_fx_div(VP_FX_ONE, vp_fx_from_int(128));
        }

        {
            vp_fx dtSub = vp_fx_mul(remaining, minToi);
            if (dtSub <= 0) dtSub = remaining;
            if (dtSub > remaining) dtSub = remaining;

            vpWorldStep(w, dtSub);
            remaining -= dtSub;
        }
    }

    /* any leftover */
    if (remaining > 0) vpWorldStep(w, remaining);
}
