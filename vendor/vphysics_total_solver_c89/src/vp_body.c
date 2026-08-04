#include "vp_world.h"
#include "vp_config.h"

static vpBodyId vp_alloc_body(vpWorld* w)
{
    vp_u16 i;
    for (i = 0; i < w->maxBodies; ++i) {
        vpBody* b = &w->bodies[i];
        if (!b->used) {
            b->used = 1;
            b->isKinematic = 0;

            b->asleep = 0;
            b->kinematicMoved = 0;
            b->sleepCounter = 0;

            b->pos = vpVec3_make(0,0,0);
            b->rot = vpQuat_identity();
            b->prevPos = b->pos;
            b->prevRot = b->rot;

            b->v_lin = vpVec3_make(0,0,0);
            b->w_ang = vpVec3_make(0,0,0);

#if VP_ENABLE_SPLIT_IMPULSE
            b->v_bias = vpVec3_make(0,0,0);
            b->w_bias = vpVec3_make(0,0,0);
#endif

            b->force = vpVec3_make(0,0,0);
            b->torque = vpVec3_make(0,0,0);

            b->invInertiaDiagLocal = vpVec3_make(0,0,0);
            b->invInertiaWorld = vpMat33_inertia_world_from_quat_diag(b->rot, b->invInertiaDiagLocal);

            b->linearDamp = (vp_fx)VP_DEFAULT_LINEAR_DAMP;
            b->angularDamp = (vp_fx)VP_DEFAULT_ANGULAR_DAMP;

            b->buoyEnabled = 0;
            b->buoyVolume = 0;
            b->buoyHalfHeight = 0;
            b->waterLinDrag = (vp_fx)VP_WATER_LINEAR_DRAG_DEFAULT;
            b->waterAngDrag = (vp_fx)VP_WATER_ANGULAR_DRAG_DEFAULT;

            b->userData = 0;

            return (vpBodyId)(i + 1);
        }
    }
    return (vpBodyId)0;
}

vpBodyId vpBodyCreateDynamic(vpWorld* w, vp_fx mass)
{
    vpBodyId id = vp_alloc_body(w);
    vpBody* b;
    if (!id) return 0;
    b = vpWorldGetBody(w, id);
    b->isKinematic = 0;
    b->invMass = (mass != 0) ? vp_fx_div(VP_FX_ONE, mass) : 0;

    /* default inertia: user should set proper values */
    b->invInertiaDiagLocal = vpVec3_make(b->invMass, b->invMass, b->invMass);
    b->invInertiaWorld = vpMat33_inertia_world_from_quat_diag(b->rot, b->invInertiaDiagLocal);
    return id;
}

vpBodyId vpBodyCreateKinematic(vpWorld* w)
{
    vpBodyId id = vp_alloc_body(w);
    vpBody* b;
    if (!id) return 0;
    b = vpWorldGetBody(w, id);
    b->isKinematic = 1;
    b->invMass = 0;
    b->invInertiaDiagLocal = vpVec3_make(0,0,0);
    b->invInertiaWorld = vpMat33_inertia_world_from_quat_diag(b->rot, b->invInertiaDiagLocal);
    return id;
}

void vpBodySetPose(vpWorld* w, vpBodyId bid, vpVec3 p, vpQuat q)
{
    vpBody* b = vpWorldGetBody(w, bid);
    if (!b) return;

    b->pos = p;
    b->rot = q;
    b->invInertiaWorld = vpMat33_inertia_world_from_quat_diag(b->rot, b->invInertiaDiagLocal);

    /* if this is a kinematic body, we derive v/w from prev->current in vpWorldStep().
       so DO NOT overwrite prevPos/prevRot here. */
    if (!b->isKinematic) {
        b->prevPos = b->pos;
        b->prevRot = b->rot;
    } else {
        b->kinematicMoved = 1;
    }

    b->asleep = 0;
    b->sleepCounter = 0;
}

void vpBodySetVel(vpWorld* w, vpBodyId bid, vpVec3 v, vpVec3 w_ang)
{
    vpBody* b = vpWorldGetBody(w, bid);
    if (!b) return;
    b->v_lin = v;
    b->w_ang = w_ang;
    b->asleep = 0;
    b->sleepCounter = 0;
}

void vpBodySetInertiaInvDiag(vpWorld* w, vpBodyId bid, vpVec3 invI_diag_local)
{
    vpBody* b = vpWorldGetBody(w, bid);
    if (!b) return;
    b->invInertiaDiagLocal = invI_diag_local;
    b->invInertiaWorld = vpMat33_inertia_world_from_quat_diag(b->rot, b->invInertiaDiagLocal);
}

void vpBodyAddForce(vpWorld* w, vpBodyId bid, vpVec3 F)
{
    vpBody* b = vpWorldGetBody(w, bid);
    if (!b || b->invMass == 0) return;
    b->force = vpVec3_add(b->force, F);
    b->asleep = 0;
    b->sleepCounter = 0;
}

void vpBodyAddTorque(vpWorld* w, vpBodyId bid, vpVec3 T)
{
    vpBody* b = vpWorldGetBody(w, bid);
    if (!b || b->invMass == 0) return;
    b->torque = vpVec3_add(b->torque, T);
    b->asleep = 0;
    b->sleepCounter = 0;
}

void vpBodyAddImpulse(vpWorld* w, vpBodyId bid, vpVec3 J, vpVec3 worldPoint)
{
    vpBody* b = vpWorldGetBody(w, bid);
    vpVec3 r, dw;
    if (!b || b->invMass == 0) return;

    b->v_lin.x += vp_fx_mul(J.x, b->invMass);
    b->v_lin.y += vp_fx_mul(J.y, b->invMass);
    b->v_lin.z += vp_fx_mul(J.z, b->invMass);

    r = vpVec3_sub(worldPoint, b->pos);
    b->invInertiaWorld = vpMat33_inertia_world_from_quat_diag(b->rot, b->invInertiaDiagLocal);
    dw = vpMat33_mul_vec3(b->invInertiaWorld, vpVec3_cross(r, J));
    b->w_ang = vpVec3_add(b->w_ang, dw);

    b->asleep = 0;
    b->sleepCounter = 0;
}

void vpBodySetDamping(vpWorld* w, vpBodyId bid, vp_fx linearDamp, vp_fx angularDamp)
{
    vpBody* b = vpWorldGetBody(w, bid);
    VP_UNUSED(w);
    if (!b) return;
    b->linearDamp = linearDamp;
    b->angularDamp = angularDamp;
}

void vpBodyEnableBuoyancy(vpWorld* w, vpBodyId bid, int enabled)
{
    vpBody* b = vpWorldGetBody(w, bid);
    VP_UNUSED(w);
    if (!b) return;
    b->buoyEnabled = (vp_u8)(enabled ? 1 : 0);
}

void vpBodySetBuoyancyParams(vpWorld* w, vpBodyId bid, vp_fx volume, vp_fx halfHeight)
{
    vpBody* b = vpWorldGetBody(w, bid);
    VP_UNUSED(w);
    if (!b) return;
    b->buoyVolume = volume;
    b->buoyHalfHeight = halfHeight;
}

void vpBodySetWaterDrag(vpWorld* w, vpBodyId bid, vp_fx linearDrag, vp_fx angularDrag)
{
    vpBody* b = vpWorldGetBody(w, bid);
    VP_UNUSED(w);
    if (!b) return;
    b->waterLinDrag = linearDrag;
    b->waterAngDrag = angularDrag;
}

void vpBodySetUserData(vpWorld* w, vpBodyId bid, vp_u32 userData)
{
    vpBody* b = vpWorldGetBody(w, bid);
    VP_UNUSED(w);
    if (!b) return;
    b->userData = userData;
}

vp_u32 vpBodyGetUserData(vpWorld* w, vpBodyId bid)
{
    vpBody* b = vpWorldGetBody(w, bid);
    VP_UNUSED(w);
    if (!b) return 0;
    return b->userData;
}
