#ifndef VP_JOINT_H
#define VP_JOINT_H

#include "vp_math3.h"
#include "vp_config.h"

typedef vp_u16 vpBodyId;
typedef vp_u16 vpJointId;

enum {
    VP_JOINT_NONE = 0,
    VP_JOINT_DISTANCE = 1,
    VP_JOINT_BALL = 2,
    VP_JOINT_SPRING = 3,
    VP_JOINT_HINGE = 4,
    VP_JOINT_SLIDER = 5,
    VP_JOINT_KEEP_UPRIGHT = 6,
    VP_JOINT_PHYSGUN = 7,
    VP_JOINT_ROPE = 8,   /* max-length only */
    VP_JOINT_WELD = 9    /* lock pos+orientation */
};

typedef struct vpJoint {
    vp_u8 used;
    vp_u8 type;
    vp_u8 broken;
    vp_u8 brokenReported; /* for event emission */

    vp_u16 bodyA;
    vp_u16 bodyB;

    /* anchors (local) */
    vpVec3 localA;
    vpVec3 localB;

    /* axes/up (local) */
    vpVec3 localAxisA;
    vpVec3 localAxisB;

    vpVec3 localUpA;
    vpVec3 localUpB;   /* used by weld/twist */

    /* rope/distance */
    vp_fx minLen;
    vp_fx maxLen;

    /* spring */
    vp_fx restLen;
    vp_fx stiffness;
    vp_fx damping;

    /* keep-upright world up */
    vpVec3 worldUp;

    /* slider limits along axis */
    vp_u8  limitEnabled;
    vp_fx  limitMin;
    vp_fx  limitMax;

    /* motor (hinge: angular, slider: linear) */
    vp_u8  motorEnabled;
    vp_fx  motorTargetVel;
    vp_fx  motorMax; /* maxTorque or maxForce (world units) */

    /* physgun target in world */
    vpVec3 targetWorld;

    /* breakable */
    vp_u8 breakable;
    vp_fx maxForce;

    /* warm-start lambdas (up to 8) */
    vp_fx lambda[8];
} vpJoint;

#endif /* VP_JOINT_H */
