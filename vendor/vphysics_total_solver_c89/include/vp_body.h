#ifndef VP_BODY_H
#define VP_BODY_H

#include "vp_math3.h"
#include "vp_config.h"

/* vpBody is intentionally POD: no malloc, no hidden pointers. */
typedef struct vpBody {
    vp_u8 used;
    vp_u8 isKinematic;

    vp_u8 asleep;
    vp_u8 kinematicMoved; /* set internally when pose changes (for island seeding) */

    vp_u16 sleepCounter;

    /* transforms */
    vpVec3 pos;
    vpQuat rot;

    /* previous transforms (for kinematic velocity derivation) */
    vpVec3 prevPos;
    vpQuat prevRot;

    /* velocities */
    vpVec3 v_lin;
    vpVec3 w_ang;

#if VP_ENABLE_SPLIT_IMPULSE
    /* split impulse (bias velocities used only for penetration correction) */
    vpVec3 v_bias;
    vpVec3 w_bias;
#endif

    /* accumulators */
    vpVec3 force;
    vpVec3 torque;

    /* inverse mass / inertia */
    vp_fx  invMass;
    vpVec3 invInertiaDiagLocal;
    vpMat33 invInertiaWorld;

    /* damping */
    vp_fx linearDamp;
    vp_fx angularDamp;

    /* buoyancy/water */
    vp_u8 buoyEnabled;
    vp_fx buoyVolume;
    vp_fx buoyHalfHeight;
    vp_fx waterLinDrag;
    vp_fx waterAngDrag;

    /* optional user tag */
    vp_u32 userData;
} vpBody;

#endif /* VP_BODY_H */
