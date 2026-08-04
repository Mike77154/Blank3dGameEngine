#ifndef VP_RAGDOLL_H
#define VP_RAGDOLL_H

#include "vp_world.h"

/* Convenience helpers (no malloc): ragdoll is "just constraints". */

/* Create a ragdoll hinge-like link:
   parent/child are body ids already created by the host.
   local anchors are joint pivots in each body local space.
   localAxis vectors define hinge axis in each body's local space.
   localUp vectors define twist reference (perpendicular to axis) for weld/twist.
   Returns a hinge joint id, preconfigured (motor off by default).
*/
vpJointId vpRagdollLinkHinge(vpWorld* w,
    vpBodyId parent, vpBodyId child,
    vpVec3 parentLocalAnchor, vpVec3 childLocalAnchor,
    vpVec3 parentLocalAxis, vpVec3 childLocalAxis);

/* Create a ragdoll "weld" (rigid link), useful for pelvis clusters etc. */
vpJointId vpRagdollLinkWeld(vpWorld* w,
    vpBodyId a, vpBodyId b,
    vpVec3 localA, vpVec3 localB,
    vpVec3 axisA, vpVec3 axisB,
    vpVec3 upA, vpVec3 upB);

#endif
