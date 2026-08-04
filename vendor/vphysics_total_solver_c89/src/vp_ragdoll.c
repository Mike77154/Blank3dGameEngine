#include "vp_ragdoll.h"

vpJointId vpRagdollLinkHinge(vpWorld* w,
    vpBodyId parent, vpBodyId child,
    vpVec3 parentLocalAnchor, vpVec3 childLocalAnchor,
    vpVec3 parentLocalAxis, vpVec3 childLocalAxis)
{
    /* Hinge is "ball + axis alignment + optional motor". */
    return vpJointCreateHinge(w, parent, child, parentLocalAnchor, childLocalAnchor, parentLocalAxis, childLocalAxis);
}

vpJointId vpRagdollLinkWeld(vpWorld* w,
    vpBodyId a, vpBodyId b,
    vpVec3 localA, vpVec3 localB,
    vpVec3 axisA, vpVec3 axisB,
    vpVec3 upA, vpVec3 upB)
{
    return vpJointCreateWeld(w, a, b, localA, localB, axisA, axisB, upA, upB);
}
