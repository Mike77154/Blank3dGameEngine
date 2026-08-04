#include "vp_world.h"
#include "vp_config.h"

static vpJointId vp_alloc_joint(vpWorld* w)
{
    vp_u16 i;
    for (i = 0; i < w->maxJoints; ++i) {
        vpJoint* j = &w->joints[i];
        if (!j->used) {
            vp_u16 k;
            j->used = 1;
            j->broken = 0;
            j->brokenReported = 0;
            j->breakable = 0;
            j->maxForce = 0;
            j->minLen = 0; j->maxLen = 0;
            j->restLen = 0; j->stiffness = 0; j->damping = 0;
            j->limitEnabled = 0; j->limitMin = 0; j->limitMax = 0;
            j->motorEnabled = 0; j->motorTargetVel = 0; j->motorMax = 0;
            j->targetWorld = vpVec3_make(0,0,0);
            j->localAxisA = vpVec3_make(VP_FX_ONE,0,0);
            j->localAxisB = vpVec3_make(VP_FX_ONE,0,0);
            j->localUpA = vpVec3_make(0,VP_FX_ONE,0);
            j->worldUp = vpVec3_make(0,VP_FX_ONE,0);
            for (k = 0; k < 8; ++k) j->lambda[k] = 0;
            j->localUpB = vpVec3_make(0,VP_FX_ONE,0);
            return (vpJointId)(i + 1);
        }
    }
    return (vpJointId)0;
}

vpJointId vpJointCreateDistance(vpWorld* w, vpBodyId a, vpBodyId b, vpVec3 localA, vpVec3 localB, vp_fx minLen, vp_fx maxLen)
{
    vpJointId id = vp_alloc_joint(w);
    vpJoint* j;
    if (!id) return 0;
    j = vpWorldGetJoint(w, id);
    j->type = (vp_u8)VP_JOINT_DISTANCE;
    j->bodyA = (vp_u16)a; j->bodyB = (vp_u16)b;
    j->localA = localA; j->localB = localB;
    j->minLen = minLen; j->maxLen = maxLen;
    return id;
}

vpJointId vpJointCreateRope(vpWorld* w, vpBodyId a, vpBodyId b, vpVec3 localA, vpVec3 localB, vp_fx maxLen)
{
    vpJointId id = vp_alloc_joint(w);
    vpJoint* j;
    if (!id) return 0;
    j = vpWorldGetJoint(w, id);
    j->type = (vp_u8)VP_JOINT_ROPE;
    j->bodyA = (vp_u16)a; j->bodyB = (vp_u16)b;
    j->localA = localA; j->localB = localB;
    j->minLen = 0;
    j->maxLen = maxLen;
    return id;
}

vpJointId vpJointCreateBall(vpWorld* w, vpBodyId a, vpBodyId b, vpVec3 localA, vpVec3 localB)
{
    vpJointId id = vp_alloc_joint(w);
    vpJoint* j;
    if (!id) return 0;
    j = vpWorldGetJoint(w, id);
    j->type = (vp_u8)VP_JOINT_BALL;
    j->bodyA = (vp_u16)a; j->bodyB = (vp_u16)b;
    j->localA = localA; j->localB = localB;
    return id;
}

vpJointId vpJointCreateSpring(vpWorld* w, vpBodyId a, vpBodyId b, vpVec3 localA, vpVec3 localB, vp_fx restLen, vp_fx stiffness, vp_fx damping)
{
    vpJointId id = vp_alloc_joint(w);
    vpJoint* j;
    if (!id) return 0;
    j = vpWorldGetJoint(w, id);
    j->type = (vp_u8)VP_JOINT_SPRING;
    j->bodyA = (vp_u16)a; j->bodyB = (vp_u16)b;
    j->localA = localA; j->localB = localB;
    j->restLen = restLen;
    j->stiffness = stiffness;
    j->damping = damping;
    return id;
}

vpJointId vpJointCreateHinge(vpWorld* w, vpBodyId a, vpBodyId b, vpVec3 localA, vpVec3 localB, vpVec3 localAxisA, vpVec3 localAxisB)
{
    vpJointId id = vp_alloc_joint(w);
    vpJoint* j;
    if (!id) return 0;
    j = vpWorldGetJoint(w, id);
    j->type = (vp_u8)VP_JOINT_HINGE;
    j->bodyA = (vp_u16)a; j->bodyB = (vp_u16)b;
    j->localA = localA; j->localB = localB;
    j->localAxisA = vpVec3_normalize(localAxisA);
    j->localAxisB = vpVec3_normalize(localAxisB);
    return id;
}

vpJointId vpJointCreateSlider(vpWorld* w, vpBodyId a, vpBodyId b, vpVec3 localA, vpVec3 localB, vpVec3 localAxisA, vpVec3 localAxisB)
{
    vpJointId id = vp_alloc_joint(w);
    vpJoint* j;
    if (!id) return 0;
    j = vpWorldGetJoint(w, id);
    j->type = (vp_u8)VP_JOINT_SLIDER;
    j->bodyA = (vp_u16)a; j->bodyB = (vp_u16)b;
    j->localA = localA; j->localB = localB;
    j->localAxisA = vpVec3_normalize(localAxisA);
    j->localAxisB = vpVec3_normalize(localAxisB);
    return id;
}

vpJointId vpJointCreateKeepUpright(vpWorld* w, vpBodyId a, vpVec3 localUpA, vpVec3 worldUp, vp_fx stiffness)
{
    vpJointId id = vp_alloc_joint(w);
    vpJoint* j;
    if (!id) return 0;
    j = vpWorldGetJoint(w, id);
    j->type = (vp_u8)VP_JOINT_KEEP_UPRIGHT;
    j->bodyA = (vp_u16)a; j->bodyB = 0;
    j->localUpA = vpVec3_normalize(localUpA);
    j->worldUp = vpVec3_normalize(worldUp);
    j->stiffness = stiffness;
    return id;
}

vpJointId vpJointCreateWeld(vpWorld* w, vpBodyId a, vpBodyId b,
    vpVec3 localA, vpVec3 localB,
    vpVec3 localAxisA, vpVec3 localAxisB,
    vpVec3 localUpA, vpVec3 localUpB)
{
    vpJointId id = vp_alloc_joint(w);
    vpJoint* j;
    if (!id) return 0;
    j = vpWorldGetJoint(w, id);
    j->type = (vp_u8)VP_JOINT_WELD;
    j->bodyA = (vp_u16)a; j->bodyB = (vp_u16)b;
    j->localA = localA; j->localB = localB;
    j->localAxisA = vpVec3_normalize(localAxisA);
    j->localAxisB = vpVec3_normalize(localAxisB);
    j->localUpA = vpVec3_normalize(localUpA);
    j->localUpB = vpVec3_normalize(localUpB);
    return id;
}

void vpHingeEnableMotor(vpWorld* w, vpJointId jid, int enabled, vp_fx targetAngVel, vp_fx maxTorque)
{
    vpJoint* j = vpWorldGetJoint(w, jid);
    VP_UNUSED(w);
    if (!j) return;
    if (j->type != VP_JOINT_HINGE) return;
    j->motorEnabled = (vp_u8)(enabled ? 1 : 0);
    j->motorTargetVel = targetAngVel;
    j->motorMax = maxTorque;
}

void vpSliderEnableMotor(vpWorld* w, vpJointId jid, int enabled, vp_fx targetLinVel, vp_fx maxForce)
{
    vpJoint* j = vpWorldGetJoint(w, jid);
    VP_UNUSED(w);
    if (!j) return;
    if (j->type != VP_JOINT_SLIDER) return;
    j->motorEnabled = (vp_u8)(enabled ? 1 : 0);
    j->motorTargetVel = targetLinVel;
    j->motorMax = maxForce;
}

void vpSliderSetLimits(vpWorld* w, vpJointId jid, int enabled, vp_fx minPos, vp_fx maxPos)
{
    vpJoint* j = vpWorldGetJoint(w, jid);
    VP_UNUSED(w);
    if (!j) return;
    if (j->type != VP_JOINT_SLIDER) return;
    j->limitEnabled = (vp_u8)(enabled ? 1 : 0);
    j->limitMin = minPos;
    j->limitMax = maxPos;
}

vpJointId vpPhysGunGrab(vpWorld* w, vpBodyId a, vpVec3 localGrabPoint, vpVec3 initialTarget)
{
    vpJointId id = vp_alloc_joint(w);
    vpJoint* j;
    if (!id) return 0;
    j = vpWorldGetJoint(w, id);
    j->type = (vp_u8)VP_JOINT_PHYSGUN;
    j->bodyA = (vp_u16)a; j->bodyB = 0;
    j->localA = localGrabPoint;
    j->targetWorld = initialTarget;
    j->stiffness = (vp_fx)VP_PHYSGUN_STIFFNESS_DEFAULT;
    j->damping = (vp_fx)VP_PHYSGUN_DAMPING_DEFAULT;
    j->motorMax = (vp_fx)VP_PHYSGUN_MAX_FORCE_DEFAULT; /* reuse motorMax as maxForce */
    return id;
}

void vpPhysGunSetTarget(vpWorld* w, vpJointId jid, vpVec3 targetWorld)
{
    vpJoint* j = vpWorldGetJoint(w, jid);
    VP_UNUSED(w);
    if (!j || j->type != VP_JOINT_PHYSGUN) return;
    j->targetWorld = targetWorld;
}

void vpPhysGunSetParams(vpWorld* w, vpJointId jid, vp_fx stiffness, vp_fx damping, vp_fx maxForce)
{
    vpJoint* j = vpWorldGetJoint(w, jid);
    VP_UNUSED(w);
    if (!j || j->type != VP_JOINT_PHYSGUN) return;
    j->stiffness = stiffness;
    j->damping = damping;
    j->motorMax = maxForce;
}

void vpPhysGunRelease(vpWorld* w, vpJointId jid)
{
    vpJoint* j = vpWorldGetJoint(w, jid);
    VP_UNUSED(w);
    if (!j) return;
    j->used = 0;
}

void vpJointSetBreakable(vpWorld* w, vpJointId jid, vp_fx maxForce)
{
    vpJoint* j = vpWorldGetJoint(w, jid);
    VP_UNUSED(w);
    if (!j) return;
    j->breakable = 1;
    j->maxForce = maxForce;
}
