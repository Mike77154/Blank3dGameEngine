#ifndef VP_WORLD_H
#define VP_WORLD_H

#include "vp_types.h"
#include "vp_fixed.h"
#include "vp_math3.h"
#include "vp_body.h"
#include "vp_contact.h"
#include "vp_joint.h"
#include "vp_events.h"
#include "vp_callbacks.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vpWorldDesc {
    vp_u16 maxBodies;
    vp_u16 maxContacts; /* solver contacts (after manifold reduction) */
    vp_u16 maxJoints;
    vp_u16 solverIters;
} vpWorldDesc;

/* internal: pair cache for manifold persistence + contact events */
typedef struct vpPairCachePoint {
    vpVec3 localA;
    vpVec3 localB;
    vp_u32 key;
} vpPairCachePoint;

typedef struct vpPairCacheEntry {
    vp_u32 pairKey; /* (minId<<16)|maxId */
    vp_u8  used;
    vp_u8  active;  /* active last frame? */
    vp_u16 bodyA;   /* min id */
    vp_u16 bodyB;   /* max id */
    vp_u32 lastSeenStep;

    vpPairCachePoint p[VP_MANIFOLD_MAX_POINTS];
} vpPairCacheEntry;

#if VP_ENABLE_ISLANDS
typedef struct vpLink {
    vp_u16 next;   /* index into link array, 0xFFFF == none */
    vp_u16 other;  /* other body id (dynamic only), 0 == static/kinematic */
    vp_u16 kind;   /* 0 = contact, 1 = joint */
    vp_u16 index;  /* contact index or joint index */
} vpLink;
#endif

struct vpWorld {
    vp_u16 maxBodies;
    vp_u16 maxContacts;
    vp_u16 maxJoints;
    vp_u16 solverIters;

    vpVec3 gravity;

    vp_fx defaultFriction;
    vp_fx defaultRestitution;

    vp_u8 waterEnabled;
    vp_fx waterY;
    vp_fx waterDensity;

    vpBody*    bodies;
    vpContact* contacts;     /* solver contacts (finalized) */
    vpContact* contactsRaw;  /* raw candidates (as added by host) */
    vpJoint*   joints;

    vp_u16 contactCount;
    vp_u16 contactRawCount;

    vp_u16* contactSort;    /* [maxContacts] scratch for manifold build */
    vp_u8*  asleepPrev;     /* [0..maxBodies] sleep state snapshot (events) */

    vpContactCacheEntry* contactCache;
    vp_u32 contactCacheMask;

    vpPairCacheEntry* pairCache;
    vp_u32 pairCacheMask;

    vp_u32 stepId;
    vp_u32 nextContactKey;

    /* events (cleared each step) */
    vpEvent* events;
    vp_u16 eventCount;

    /* optional callbacks */
    vpCallbacks callbacks;
    vp_u8 callbacksEnabled;

#if VP_ENABLE_ISLANDS
    /* island adjacency + scratch */
    vp_u16* bodyFirstLink;   /* [0..maxBodies] */
    vpLink* links;           /* [0..2*(maxContacts+maxJoints)-1] */
    vp_u16  linkCount;
    vp_u16  linkCap;

    vp_u8*  bodyVisited;     /* [0..maxBodies] */
    vp_u8*  contactVisited;  /* [0..maxContacts-1] (current step contacts) */
    vp_u8*  jointVisited;    /* [0..maxJoints-1] */

    vp_u16* islandBodyStack;   /* [maxBodies] */
    vp_u16* islandBodies;      /* [maxBodies] */
    vp_u16* islandContacts;    /* [maxContacts] */
    vp_u16* islandJoints;      /* [maxJoints] */
#endif

    vpBody staticBody;
};

/* memory */
vp_u32   vpWorldMemSize(const vpWorldDesc* desc);
vpWorld* vpWorldInit(void* mem, vp_u32 memBytes, const vpWorldDesc* desc);

/* globals */
void vpWorldSetGravity(vpWorld* w, vpVec3 g);
void vpWorldSetDefaultMaterial(vpWorld* w, vp_fx friction, vp_fx restitution);

/* callbacks */
void vpWorldSetCallbacks(vpWorld* w, const vpCallbacks* cb);

/* water */
void vpWorldEnableWater(vpWorld* w, int enabled);
void vpWorldSetWaterPlane(vpWorld* w, vp_fx waterY, vp_fx density);

/* step */
void vpWorldStep(vpWorld* w, vp_fx dt);

/* optional host-driven CCD substepping:
   requires callbacks.bodySweepTOI != NULL.
   maxSubSteps clamps worst-case iteration. */
void vpWorldStepCCD(vpWorld* w, vp_fx dt, vp_u16 maxSubSteps);

/* bodies */
vpBodyId vpBodyCreateDynamic(vpWorld* w, vp_fx mass);
vpBodyId vpBodyCreateKinematic(vpWorld* w);

void vpBodySetPose(vpWorld* w, vpBodyId b, vpVec3 p, vpQuat q);
void vpBodySetVel (vpWorld* w, vpBodyId b, vpVec3 v, vpVec3 w_ang);
void vpBodySetInertiaInvDiag(vpWorld* w, vpBodyId b, vpVec3 invI_diag_local);

void vpBodyAddForce  (vpWorld* w, vpBodyId b, vpVec3 F);
void vpBodyAddTorque (vpWorld* w, vpBodyId b, vpVec3 T);
void vpBodyAddImpulse(vpWorld* w, vpBodyId b, vpVec3 J, vpVec3 worldPoint);

void vpBodySetDamping(vpWorld* w, vpBodyId b, vp_fx linearDamp, vp_fx angularDamp);

void vpBodyEnableBuoyancy(vpWorld* w, vpBodyId b, int enabled);
void vpBodySetBuoyancyParams(vpWorld* w, vpBodyId b, vp_fx volume, vp_fx halfHeight);
void vpBodySetWaterDrag(vpWorld* w, vpBodyId b, vp_fx linearDrag, vp_fx angularDrag);

void  vpBodySetUserData(vpWorld* w, vpBodyId b, vp_u32 userData);
vp_u32 vpBodyGetUserData(vpWorld* w, vpBodyId b);

/* contacts (host supplies candidates; engine finalizes into manifolds internally) */
void vpContactsBegin(vpWorld* w);

void vpContactsAdd(vpWorld* w,
    vpBodyId a, vpBodyId b,
    vpVec3 p, vpVec3 n,
    vp_fx penetration,
    vp_fx friction,
    vp_fx restitution);

void vpContactsAddKeyed(vpWorld* w,
    vp_u32 key,
    vpBodyId a, vpBodyId b,
    vpVec3 p, vpVec3 n,
    vp_fx penetration,
    vp_fx friction,
    vp_fx restitution);

/* Extended variant: provide flags (sensor/speculative) */
void vpContactsAddEx(vpWorld* w,
    vpBodyId a, vpBodyId b,
    vpVec3 p, vpVec3 n,
    vp_fx penetration,
    vp_fx friction,
    vp_fx restitution,
    vp_u8 flags);

void vpContactsAddKeyedEx(vpWorld* w,
    vp_u32 key,
    vpBodyId a, vpBodyId b,
    vpVec3 p, vpVec3 n,
    vp_fx penetration,
    vp_fx friction,
    vp_fx restitution,
    vp_u8 flags);

/* Finalize raw candidates into solver contacts (manifold reduction + key assignment). */
void vpContactsFinalize(vpWorld* w, vp_fx dt);

/* Commit lambdas to cache (normally called by vpWorldStep). Kept for backwards compat. */
void vpContactsEnd(vpWorld* w);

/* joints */
vpJointId vpJointCreateDistance(vpWorld* w, vpBodyId a, vpBodyId b, vpVec3 localA, vpVec3 localB, vp_fx minLen, vp_fx maxLen);
vpJointId vpJointCreateRope(vpWorld* w, vpBodyId a, vpBodyId b, vpVec3 localA, vpVec3 localB, vp_fx maxLen);
vpJointId vpJointCreateBall(vpWorld* w, vpBodyId a, vpBodyId b, vpVec3 localA, vpVec3 localB);
vpJointId vpJointCreateSpring(vpWorld* w, vpBodyId a, vpBodyId b, vpVec3 localA, vpVec3 localB, vp_fx restLen, vp_fx stiffness, vp_fx damping);

vpJointId vpJointCreateHinge(vpWorld* w, vpBodyId a, vpBodyId b, vpVec3 localA, vpVec3 localB, vpVec3 localAxisA, vpVec3 localAxisB);
vpJointId vpJointCreateSlider(vpWorld* w, vpBodyId a, vpBodyId b, vpVec3 localA, vpVec3 localB, vpVec3 localAxisA, vpVec3 localAxisB);

vpJointId vpJointCreateKeepUpright(vpWorld* w, vpBodyId a, vpVec3 localUpA, vpVec3 worldUp, vp_fx stiffness);

/* Weld locks position+orientation. Provide:
   - local anchors
   - local axis and up vectors on both bodies (axis,up must be perpendicular-ish) */
vpJointId vpJointCreateWeld(vpWorld* w, vpBodyId a, vpBodyId b,
    vpVec3 localA, vpVec3 localB,
    vpVec3 localAxisA, vpVec3 localAxisB,
    vpVec3 localUpA, vpVec3 localUpB);

/* motors and limits */
void vpHingeEnableMotor(vpWorld* w, vpJointId j, int enabled, vp_fx targetAngVel, vp_fx maxTorque);
void vpSliderEnableMotor(vpWorld* w, vpJointId j, int enabled, vp_fx targetLinVel, vp_fx maxForce);
void vpSliderSetLimits(vpWorld* w, vpJointId j, int enabled, vp_fx minPos, vp_fx maxPos);

/* physgun */
vpJointId vpPhysGunGrab(vpWorld* w, vpBodyId a, vpVec3 localGrabPoint, vpVec3 initialTarget);
void      vpPhysGunSetTarget(vpWorld* w, vpJointId physgunJoint, vpVec3 targetWorld);
void      vpPhysGunSetParams(vpWorld* w, vpJointId physgunJoint, vp_fx stiffness, vp_fx damping, vp_fx maxForce);
void      vpPhysGunRelease(vpWorld* w, vpJointId physgunJoint);

void      vpJointSetBreakable(vpWorld* w, vpJointId j, vp_fx maxForce);

/* accessors */
vpBody*  vpWorldGetBody(vpWorld* w, vpBodyId id);
vpJoint* vpWorldGetJoint(vpWorld* w, vpJointId id);

/* events */
void       vpWorldClearEvents(vpWorld* w);
vp_u16     vpWorldGetEventCount(const vpWorld* w);
const vpEvent* vpWorldGetEvent(const vpWorld* w, vp_u16 index);

#ifdef __cplusplus
}
#endif
#endif /* VP_WORLD_H */
