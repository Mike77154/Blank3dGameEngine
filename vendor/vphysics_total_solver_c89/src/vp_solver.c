#include "vp_solver.h"
#include "vp_config.h"

/* --- low-level helpers --- */
static void vp_update_inertia(vpBody* b)
{
    b->invInertiaWorld = vpMat33_inertia_world_from_quat_diag(b->rot, b->invInertiaDiagLocal);
}

static vpVec3 vp_point_velocity(vpBody* b, vpVec3 r)
{
    return vpVec3_add(b->v_lin, vpVec3_cross(b->w_ang, r));
}


#if VP_ENABLE_SPLIT_IMPULSE
static vpVec3 vp_point_velocity_bias(vpBody* b, vpVec3 r)
{
    return vpVec3_add(b->v_bias, vpVec3_cross(b->w_bias, r));
}
#endif

static void vp_wake_pair(vpBody* A, vpBody* B)
{
    /* Solver impulses must wake sleeping bodies, but must not erase the
     * accumulated sleep counter of bodies that are already awake. Resting
     * contacts generate tiny corrective impulses every frame; resetting the
     * counter here prevents them from ever reaching sleep. */
    if (A->asleep) { A->asleep = 0; A->sleepCounter = 0; }
    if (B->asleep) { B->asleep = 0; B->sleepCounter = 0; }
}

static void vp_apply_impulse_linear(vpBody* b, vpVec3 P, vp_u8 sign)
{
    if (b->invMass == 0) return;
    if (sign) {
        b->v_lin.x -= vp_fx_mul(P.x, b->invMass);
        b->v_lin.y -= vp_fx_mul(P.y, b->invMass);
        b->v_lin.z -= vp_fx_mul(P.z, b->invMass);
    } else {
        b->v_lin.x += vp_fx_mul(P.x, b->invMass);
        b->v_lin.y += vp_fx_mul(P.y, b->invMass);
        b->v_lin.z += vp_fx_mul(P.z, b->invMass);
    }
}

static void vp_apply_impulse_angular(vpBody* b, vpVec3 L, vp_u8 sign)
{
    vpVec3 dW;
    if (b->invMass == 0) return;
    dW = vpMat33_mul_vec3(b->invInertiaWorld, L);
    if (sign) { b->w_ang.x -= dW.x; b->w_ang.y -= dW.y; b->w_ang.z -= dW.z; }
    else      { b->w_ang.x += dW.x; b->w_ang.y += dW.y; b->w_ang.z += dW.z; }
}

static void vp_apply_impulse_at_point(vpBody* b, vpVec3 P, vpVec3 r, vp_u8 sign)
{
    vpVec3 rxP;
    if (b->invMass == 0) return;
    vp_update_inertia(b);
    vp_apply_impulse_linear(b, P, sign);
    rxP = vpVec3_cross(r, P);
    vp_apply_impulse_angular(b, rxP, sign);
}

#if VP_ENABLE_SPLIT_IMPULSE
static void vp_apply_impulse_linear_bias(vpBody* b, vpVec3 P, vp_u8 sign)
{
    vpVec3 dv;
    if (b->invMass == 0) return;
    dv = vpVec3_scale(P, b->invMass);
    if (sign) b->v_bias = vpVec3_sub(b->v_bias, dv);
    else      b->v_bias = vpVec3_add(b->v_bias, dv);

#if (VP_SPLIT_IMPULSE_MAX > 0)
    {
        vp_fx m = (vp_fx)VP_SPLIT_IMPULSE_MAX;
        b->v_bias.x = vp_fx_clamp(b->v_bias.x, -m, m);
        b->v_bias.y = vp_fx_clamp(b->v_bias.y, -m, m);
        b->v_bias.z = vp_fx_clamp(b->v_bias.z, -m, m);
    }
#endif
}

static void vp_apply_impulse_angular_bias(vpBody* b, vpVec3 tau, vp_u8 sign)
{
    vpVec3 dw;
    if (b->invMass == 0) return;
    vp_update_inertia(b);
    dw = vpMat33_mul_vec3(b->invInertiaWorld, tau);
    if (sign) b->w_bias = vpVec3_sub(b->w_bias, dw);
    else      b->w_bias = vpVec3_add(b->w_bias, dw);

#if (VP_SPLIT_IMPULSE_MAX > 0)
    {
        vp_fx m = (vp_fx)VP_SPLIT_IMPULSE_MAX;
        b->w_bias.x = vp_fx_clamp(b->w_bias.x, -m, m);
        b->w_bias.y = vp_fx_clamp(b->w_bias.y, -m, m);
        b->w_bias.z = vp_fx_clamp(b->w_bias.z, -m, m);
    }
#endif
}

static void vp_apply_impulse_at_point_bias(vpBody* b, vpVec3 P, vpVec3 r, vp_u8 sign)
{
    vpVec3 rxP;
    if (b->invMass == 0) return;
    vp_update_inertia(b);
    vp_apply_impulse_linear_bias(b, P, sign);
    rxP = vpVec3_cross(r, P);
    vp_apply_impulse_angular_bias(b, rxP, sign);
}
#endif /* VP_ENABLE_SPLIT_IMPULSE */

static vp_fx vp_effective_mass(vpBody* A, vpBody* B, vpVec3 rA, vpVec3 rB, vpVec3 axis)
{
    vp_fx k;
    vpVec3 raXa, rbXa, tmp;

    raXa = vpVec3_cross(rA, axis);
    rbXa = vpVec3_cross(rB, axis);

    k = A->invMass + B->invMass;

    tmp = vpMat33_mul_vec3(A->invInertiaWorld, raXa);
    k += vpVec3_dot(raXa, tmp);

    tmp = vpMat33_mul_vec3(B->invInertiaWorld, rbXa);
    k += vpVec3_dot(rbXa, tmp);

    return k;
}

static vp_fx vp_effective_mass_angular(vpBody* A, vpBody* B, vpVec3 axis)
{
    vp_fx k;
    vpVec3 tmp;
    k = 0;
    tmp = vpMat33_mul_vec3(A->invInertiaWorld, axis);
    k += vpVec3_dot(axis, tmp);
    tmp = vpMat33_mul_vec3(B->invInertiaWorld, axis);
    k += vpVec3_dot(axis, tmp);
    return k;
}

static int vp_check_break_joint(vpJoint* j, vp_fx impulse, vp_fx dt)
{
    vp_fx force;
    if (!j->breakable) return 0;
    if (dt == 0) return 0;
    force = vp_fx_div(vp_fx_abs(impulse), dt);
    if (force > j->maxForce) { j->broken = 1; return 1; }
    return 0;
}


/* forward decl (C89): used before definition */
static void vp_solve_axis_alignment(vpBody* A, vpBody* B, vpVec3 axisA, vpVec3 axisB, vp_fx dt, vp_fx beta, vp_fx* lam0, vp_fx* lam1, vpJoint* j);
/* --- Warm-start --- */
static void vp_warmstart_contacts(vpWorld* w)
{
    vp_u16 i;
    for (i = 0; i < w->contactCount; ++i) {
        vpContact* c = &w->contacts[i];
        vpBody* A = vpWorldGetBody(w, (vpBodyId)c->bodyA);
        vpBody* B = vpWorldGetBody(w, (vpBodyId)c->bodyB);
        vpVec3 rA, rB, t1, t2, P;

        if (!A || !B) continue;
        if (c->flags & VP_CONTACT_FLAG_SENSOR) continue;
        if (A->asleep && B->asleep) continue;
        vp_update_inertia(A); vp_update_inertia(B);

        rA = vpVec3_sub(c->p, A->pos);
        rB = vpVec3_sub(c->p, B->pos);

        if (c->lambdaN != 0) {
            P = vpVec3_scale(c->n, c->lambdaN);
            vp_apply_impulse_at_point(A, P, rA, 1);
            vp_apply_impulse_at_point(B, P, rB, 0);
        }

        vpVec3_tangent_basis(c->n, &t1, &t2);

        if (c->lambdaT1 != 0) {
            P = vpVec3_scale(t1, c->lambdaT1);
            vp_apply_impulse_at_point(A, P, rA, 1);
            vp_apply_impulse_at_point(B, P, rB, 0);
        }
        if (c->lambdaT2 != 0) {
            P = vpVec3_scale(t2, c->lambdaT2);
            vp_apply_impulse_at_point(A, P, rA, 1);
            vp_apply_impulse_at_point(B, P, rB, 0);
        }
    }
}

static void vp_warmstart_joint_ball(vpWorld* w, vpJoint* j, vpVec3 rA, vpVec3 rB)
{
    vpBody* A = vpWorldGetBody(w, (vpBodyId)j->bodyA);
    vpBody* B = vpWorldGetBody(w, (vpBodyId)j->bodyB);
    vpVec3 ax, P;

    if (!A || !B) return;

    ax = vpVec3_make(VP_FX_ONE,0,0); P = vpVec3_scale(ax, j->lambda[0]);
    vp_apply_impulse_at_point(A, P, rA, 1); vp_apply_impulse_at_point(B, P, rB, 0);
    ax = vpVec3_make(0,VP_FX_ONE,0); P = vpVec3_scale(ax, j->lambda[1]);
    vp_apply_impulse_at_point(A, P, rA, 1); vp_apply_impulse_at_point(B, P, rB, 0);
    ax = vpVec3_make(0,0,VP_FX_ONE); P = vpVec3_scale(ax, j->lambda[2]);
    vp_apply_impulse_at_point(A, P, rA, 1); vp_apply_impulse_at_point(B, P, rB, 0);
}

static void vp_warmstart_joints(vpWorld* w)
{
    vp_u16 i;
    for (i = 0; i < w->maxJoints; ++i) {
        vpJoint* j = &w->joints[i];
        vpBody* A;
        vpBody* B;
        vpVec3 rA, rB, pA, pB, d, n, P;
        vp_fx len;

        if (!j->used || j->broken) continue;

        A = vpWorldGetBody(w, (vpBodyId)j->bodyA);
        B = vpWorldGetBody(w, (vpBodyId)j->bodyB);
        if (!A || !B) continue;
        if (A->asleep && B->asleep) continue;

        vp_update_inertia(A); vp_update_inertia(B);

        rA = vpQuat_rotate_vec3(A->rot, j->localA);
        rB = vpQuat_rotate_vec3(B->rot, j->localB);

        if (j->type == VP_JOINT_BALL) {
            vp_warmstart_joint_ball(w, j, rA, rB);
        } else if (j->type == VP_JOINT_HINGE || j->type == VP_JOINT_SLIDER) {
            /* positional part warm-start as ball/axes used */
            vp_warmstart_joint_ball(w, j, rA, rB);
        } else if (j->type == VP_JOINT_DISTANCE || j->type == VP_JOINT_SPRING || j->type == VP_JOINT_PHYSGUN) {
            pA = vpVec3_add(A->pos, rA);
            if (j->type == VP_JOINT_PHYSGUN) pB = j->targetWorld;
            else pB = vpVec3_add(B->pos, rB);

            d = vpVec3_sub(pB, pA);
            len = vpVec3_len(d);
            if (len <= 0) continue;
            n = vpVec3_scale(d, vp_fx_div(VP_FX_ONE, len));
            P = vpVec3_scale(n, j->lambda[0]);
            vp_apply_impulse_at_point(A, P, rA, 1);
            if (j->type != VP_JOINT_PHYSGUN) vp_apply_impulse_at_point(B, P, rB, 0);
        } else if (j->type == VP_JOINT_KEEP_UPRIGHT) {
            /* no warm start */
        }
    }
}

/* --- Contacts solve --- */
static void vp_prepare_contact_restitution(vpWorld* w, vpContact* c)
{
    vpBody* A;
    vpBody* B;
    vpVec3 rA;
    vpVec3 rB;
    vpVec3 vA;
    vpVec3 vB;
    vpVec3 dv;
    vp_fx vn;
    if (!w || !c) return;
    c->restitutionTarget = 0;
    if (c->flags & VP_CONTACT_FLAG_SENSOR) return;
    A = vpWorldGetBody(w, (vpBodyId)c->bodyA);
    B = vpWorldGetBody(w, (vpBodyId)c->bodyB);
    if (!A || !B) return;
    rA = vpVec3_sub(c->p, A->pos);
    rB = vpVec3_sub(c->p, B->pos);
    vA = vp_point_velocity(A, rA);
    vB = vp_point_velocity(B, rB);
    dv = vpVec3_sub(vB, vA);
    vn = vpVec3_dot(dv, c->n);
    if (vn < -(vp_fx)VP_RESTITUTION_VEL_THRESHOLD)
        c->restitutionTarget = vp_fx_mul(c->restitution, (vp_fx)(-vn));
}

static void vp_solve_contact_normal(vpWorld* w, vpContact* c, vp_fx dt, vp_fx beta, vp_fx slop)
{
    vpBody* A = vpWorldGetBody(w, (vpBodyId)c->bodyA);
    vpBody* B = vpWorldGetBody(w, (vpBodyId)c->bodyB);
    vpVec3 rA, rB;
    vpVec3 vA, vB, dv;
    vp_fx vn;
    vp_fx k, invK;
    vp_fx dLambda, lambdaOld, lambdaNew;
    vpVec3 P;

    vp_fx restitution;

#if VP_ENABLE_SPLIT_IMPULSE
    vp_fx bias;
#endif

    if (!A || !B) return;
    if (c->flags & VP_CONTACT_FLAG_SENSOR) return;

    vp_update_inertia(A); vp_update_inertia(B);

    rA = vpVec3_sub(c->p, A->pos);
    rB = vpVec3_sub(c->p, B->pos);

    vA = vp_point_velocity(A, rA);
    vB = vp_point_velocity(B, rB);
    dv = vpVec3_sub(vB, vA);
    vn = vpVec3_dot(dv, c->n);

    /* Restitution target is captured once before warm-start/iterations.
     * Recomputing it from the changing vn each iteration collapses a bounce
     * back to zero normal velocity. */
    restitution = c->restitutionTarget;

    k = vp_effective_mass(A, B, rA, rB, c->n);
    if (k == 0) return;
    invK = vp_fx_div(VP_FX_ONE, k);

#if VP_ENABLE_SPLIT_IMPULSE
    /* --- velocity impulse (no Baumgarte) --- */
    lambdaOld = c->lambdaN;
    dLambda = vp_fx_mul(invK, (-vn + restitution));
    lambdaNew = lambdaOld + dLambda;
    if (lambdaNew < 0) lambdaNew = 0;
    dLambda = lambdaNew - lambdaOld;
    c->lambdaN = lambdaNew;

    if (dLambda != 0) {
        P = vpVec3_scale(c->n, dLambda);
        vp_apply_impulse_at_point(A, P, rA, 1);
        vp_apply_impulse_at_point(B, P, rB, 0);
        vp_wake_pair(A,B);
    }

    /* --- split impulse (position correction) --- */
    bias = 0;
    if (dt != 0) {
        if (c->penetration > slop) {
            bias = vp_fx_div(vp_fx_mul(beta, (c->penetration - slop)), dt);
        } else if ((c->flags & VP_CONTACT_FLAG_SPECULATIVE) && c->penetration > -slop) {
            /* keep a small separation band (speculative contacts) */
            vp_fx b = c->penetration + slop; /* in (0..slop] */
            if (b > 0) bias = vp_fx_div(vp_fx_mul(beta, b), dt);
        }
    }

    if (bias != 0 || c->lambdaBias != 0) {
        vp_fx vnBias;
        vA = vp_point_velocity_bias(A, rA);
        vB = vp_point_velocity_bias(B, rB);
        dv = vpVec3_sub(vB, vA);
        vnBias = vpVec3_dot(dv, c->n);

        lambdaOld = c->lambdaBias;
        dLambda = vp_fx_mul(invK, (-vnBias + bias));
        lambdaNew = lambdaOld + dLambda;
        if (lambdaNew < 0) lambdaNew = 0;
        dLambda = lambdaNew - lambdaOld;
        c->lambdaBias = lambdaNew;

        if (dLambda != 0) {
            P = vpVec3_scale(c->n, dLambda);
            vp_apply_impulse_at_point_bias(A, P, rA, 1);
            vp_apply_impulse_at_point_bias(B, P, rB, 0);
        }
    }
#else
    /* classic (Baumgarte) */
    {
        vp_fx bias = 0;
        if (dt != 0) {
            if (c->penetration > slop) {
                bias = vp_fx_div(vp_fx_mul(beta, (c->penetration - slop)), dt);
            } else if ((c->flags & VP_CONTACT_FLAG_SPECULATIVE) && c->penetration > -slop) {
                vp_fx b = c->penetration + slop;
                if (b > 0) bias = vp_fx_div(vp_fx_mul(beta, b), dt);
            }
        }

        lambdaOld = c->lambdaN;
        dLambda = vp_fx_mul(invK, (-vn + bias + restitution));
        lambdaNew = lambdaOld + dLambda;
        if (lambdaNew < 0) lambdaNew = 0;
        dLambda = lambdaNew - lambdaOld;
        c->lambdaN = lambdaNew;

        if (dLambda != 0) {
            P = vpVec3_scale(c->n, dLambda);
            vp_apply_impulse_at_point(A, P, rA, 1);
            vp_apply_impulse_at_point(B, P, rB, 0);
            vp_wake_pair(A,B);
        }
    }
#endif
}


static void vp_solve_contact_friction(vpWorld* w, vpContact* c)
{
    vpBody* A = vpWorldGetBody(w, (vpBodyId)c->bodyA);
    vpBody* B = vpWorldGetBody(w, (vpBodyId)c->bodyB);
    vpVec3 rA, rB, velA, velB, relV, t1, t2;
    vp_fx vt1, vt2;
    vp_fx k1, k2, invK1, invK2;
    vp_fx maxF;
    vp_fx lambdaOld, lambdaNew, dLambda;
    vpVec3 P;

    if (!A || !B) return;
    if (c->flags & VP_CONTACT_FLAG_SENSOR) return;
    vp_update_inertia(A); vp_update_inertia(B);

    rA = vpVec3_sub(c->p, A->pos);
    rB = vpVec3_sub(c->p, B->pos);
    velA = vp_point_velocity(A, rA);
    velB = vp_point_velocity(B, rB);
    relV = vpVec3_sub(velB, velA);

    vpVec3_tangent_basis(c->n, &t1, &t2);
    vt1 = vpVec3_dot(relV, t1);
    vt2 = vpVec3_dot(relV, t2);

    k1 = vp_effective_mass(A,B,rA,rB,t1);
    k2 = vp_effective_mass(A,B,rA,rB,t2);
    invK1 = (k1 != 0) ? vp_fx_div(VP_FX_ONE, k1) : 0;
    invK2 = (k2 != 0) ? vp_fx_div(VP_FX_ONE, k2) : 0;

    maxF = vp_fx_mul(c->friction, c->lambdaN);

    lambdaOld = c->lambdaT1;
    dLambda = vp_fx_mul(invK1, (vp_fx)(-vt1));
    lambdaNew = lambdaOld + dLambda;
    lambdaNew = vp_fx_clamp(lambdaNew, -maxF, maxF);
    dLambda = lambdaNew - lambdaOld;
    c->lambdaT1 = lambdaNew;

    if (dLambda != 0) {
        P = vpVec3_scale(t1, dLambda);
        vp_apply_impulse_at_point(A, P, rA, 1);
        vp_apply_impulse_at_point(B, P, rB, 0);
        vp_wake_pair(A,B);
    }

    lambdaOld = c->lambdaT2;
    dLambda = vp_fx_mul(invK2, (vp_fx)(-vt2));
    lambdaNew = lambdaOld + dLambda;
    lambdaNew = vp_fx_clamp(lambdaNew, -maxF, maxF);
    dLambda = lambdaNew - lambdaOld;
    c->lambdaT2 = lambdaNew;

    if (dLambda != 0) {
        P = vpVec3_scale(t2, dLambda);
        vp_apply_impulse_at_point(A, P, rA, 1);
        vp_apply_impulse_at_point(B, P, rB, 0);
        vp_wake_pair(A,B);
    }
}

/* --- Joints solve --- */
static void vp_solve_joint_ball(vpWorld* w, vpJoint* j, vp_fx dt, vp_fx beta)
{
    vpBody* A = vpWorldGetBody(w, (vpBodyId)j->bodyA);
    vpBody* B = vpWorldGetBody(w, (vpBodyId)j->bodyB);
    vpVec3 rA, rB, pA, pB, err;
    vpVec3 ax[3];
    vp_u16 k;

    if (!A || !B) return;
    vp_update_inertia(A); vp_update_inertia(B);

    rA = vpQuat_rotate_vec3(A->rot, j->localA);
    rB = vpQuat_rotate_vec3(B->rot, j->localB);
    pA = vpVec3_add(A->pos, rA);
    pB = vpVec3_add(B->pos, rB);
    err = vpVec3_sub(pB, pA);

    ax[0] = vpVec3_make(VP_FX_ONE,0,0);
    ax[1] = vpVec3_make(0,VP_FX_ONE,0);
    ax[2] = vpVec3_make(0,0,VP_FX_ONE);

    for (k = 0; k < 3; ++k) {
        vpVec3 velA, velB, relV;
        vp_fx vax, bias, eff, invEff, dLambda;
        vpVec3 P;
        vp_fx* lam = &j->lambda[k];

        velA = vp_point_velocity(A, rA);
        velB = vp_point_velocity(B, rB);
        relV = vpVec3_sub(velB, velA);
        vax = vpVec3_dot(relV, ax[k]);

        if (k == 0) bias = vp_fx_div(vp_fx_mul(beta, err.x), dt);
        else if (k == 1) bias = vp_fx_div(vp_fx_mul(beta, err.y), dt);
        else bias = vp_fx_div(vp_fx_mul(beta, err.z), dt);

        eff = vp_effective_mass(A,B,rA,rB,ax[k]);
        invEff = (eff != 0) ? vp_fx_div(VP_FX_ONE, eff) : 0;

        dLambda = vp_fx_mul(invEff, ((vp_fx)(-vax) - bias));
        *lam += dLambda;

        if (vp_check_break_joint(j, dLambda, dt)) return;

        if (dLambda != 0) {
            P = vpVec3_scale(ax[k], dLambda);
            vp_apply_impulse_at_point(A, P, rA, 1);
            vp_apply_impulse_at_point(B, P, rB, 0);
            vp_wake_pair(A,B);
        }
    }
}

static void vp_solve_joint_distance(vpWorld* w, vpJoint* j, vp_fx dt, vp_fx beta)
{
    vpBody* A = vpWorldGetBody(w, (vpBodyId)j->bodyA);
    vpBody* B = vpWorldGetBody(w, (vpBodyId)j->bodyB);
    vpVec3 rA, rB, pA, pB, d, n;
    vp_fx len, target, err;
    vpVec3 velA, velB, relV;
    vp_fx vn, bias;
    vp_fx k, invK;
    vp_fx lambdaOld, lambdaNew, dLambda;
    vpVec3 P;

    if (!A || !B) return;
    vp_update_inertia(A); vp_update_inertia(B);

    rA = vpQuat_rotate_vec3(A->rot, j->localA);
    rB = vpQuat_rotate_vec3(B->rot, j->localB);
    pA = vpVec3_add(A->pos, rA);
    pB = vpVec3_add(B->pos, rB);
    d = vpVec3_sub(pB, pA);
    len = vpVec3_len(d);
    if (len <= 0) return;
    n = vpVec3_scale(d, vp_fx_div(VP_FX_ONE, len));

    target = len;
    if (j->maxLen > 0 && len > j->maxLen) target = j->maxLen;
    else if (j->minLen > 0 && len < j->minLen) target = j->minLen;
    else return;

    err = len - target;
    bias = vp_fx_div(vp_fx_mul(beta, err), dt);

    velA = vp_point_velocity(A, rA);
    velB = vp_point_velocity(B, rB);
    relV = vpVec3_sub(velB, velA);
    vn = vpVec3_dot(relV, n);

    k = vp_effective_mass(A,B,rA,rB,n);
    invK = (k != 0) ? vp_fx_div(VP_FX_ONE, k) : 0;

    lambdaOld = j->lambda[0];
    dLambda = vp_fx_mul(invK, ((vp_fx)(-vn) - bias));
    lambdaNew = lambdaOld + dLambda;

    if (j->maxLen > 0 && len > j->maxLen) { if (lambdaNew < 0) lambdaNew = 0; }
    if (j->minLen > 0 && len < j->minLen) { if (lambdaNew > 0) lambdaNew = 0; }

    dLambda = lambdaNew - lambdaOld;
    j->lambda[0] = lambdaNew;

    if (vp_check_break_joint(j, dLambda, dt)) return;

    if (dLambda != 0) {
        P = vpVec3_scale(n, dLambda);
        vp_apply_impulse_at_point(A, P, rA, 1);
        vp_apply_impulse_at_point(B, P, rB, 0);
        vp_wake_pair(A,B);
    }
}

static void vp_solve_joint_spring(vpWorld* w, vpJoint* j, vp_fx dt)
{
    vpBody* A = vpWorldGetBody(w, (vpBodyId)j->bodyA);
    vpBody* B = vpWorldGetBody(w, (vpBodyId)j->bodyB);
    vpVec3 rA, rB, pA, pB, d, n;
    vp_fx len, x;
    vpVec3 velA, velB, relV;
    vp_fx vn, Fs;
    vp_fx k, invK;
    vp_fx dLambda;
    vpVec3 P;

    if (!A || !B) return;
    vp_update_inertia(A); vp_update_inertia(B);

    rA = vpQuat_rotate_vec3(A->rot, j->localA);
    rB = vpQuat_rotate_vec3(B->rot, j->localB);
    pA = vpVec3_add(A->pos, rA);
    pB = vpVec3_add(B->pos, rB);
    d = vpVec3_sub(pB, pA);
    len = vpVec3_len(d);
    if (len <= 0) return;
    n = vpVec3_scale(d, vp_fx_div(VP_FX_ONE, len));

    x = len - j->restLen;

    velA = vp_point_velocity(A, rA);
    velB = vp_point_velocity(B, rB);
    relV = vpVec3_sub(velB, velA);
    vn = vpVec3_dot(relV, n);

    Fs = (vp_fx)(-vp_fx_mul(j->stiffness, x) - vp_fx_mul(j->damping, vn));
    dLambda = vp_fx_mul(Fs, dt);

    k = vp_effective_mass(A,B,rA,rB,n);
    invK = (k != 0) ? vp_fx_div(VP_FX_ONE, k) : 0;
    dLambda = vp_fx_mul(dLambda, invK);

    if (vp_check_break_joint(j, dLambda, dt)) return;

    if (dLambda != 0) {
        P = vpVec3_scale(n, dLambda);
        vp_apply_impulse_at_point(A, P, rA, 1);
        vp_apply_impulse_at_point(B, P, rB, 0);
        vp_wake_pair(A,B);
    }
}


/* --- WELD joint: lock position + orientation ---
   Position: 3 linear constraints (ball style).
   Orientation: axis alignment (2 DOF) + twist alignment around axis (1 DOF).
*/
static void vp_solve_joint_weld(vpWorld* w, vpJoint* j, vp_fx dt, vp_fx beta)
{
    vpBody* A = vpWorldGetBody(w, (vpBodyId)j->bodyA);
    vpBody* B = vpWorldGetBody(w, (vpBodyId)j->bodyB);

    if (!A || !B) return;
    if (j->broken) return;

    /* Reuse ball solver (expects lambda0..2) -> in this code, ball solver uses j->lambda[0..2]. */
    vp_solve_joint_ball(w, j, dt, beta);

    vp_update_inertia(A); vp_update_inertia(B);

    /* Axis alignment uses j->lambda[3..4] */
    {
        vpVec3 axisA = vpQuat_rotate_vec3(A->rot, j->localAxisA);
        vpVec3 axisB = vpQuat_rotate_vec3(B->rot, j->localAxisB);
        vp_solve_axis_alignment(A, B, axisA, axisB, dt, beta, &j->lambda[3], &j->lambda[4], j);
    }

    /* Twist alignment around axis using projected up vectors -> j->lambda[5] */
    {
        vpVec3 axis = vpVec3_normalize(vpQuat_rotate_vec3(A->rot, j->localAxisA));
        vpVec3 upA = vpQuat_rotate_vec3(A->rot, j->localUpA);
        vpVec3 upB = vpQuat_rotate_vec3(B->rot, j->localUpB);
        vpVec3 projA, projB;
        vp_fx dotA, dotB;
        vpVec3 crossAB;
        vp_fx twistErr;
        vp_fx bias;
        vp_fx wrel;
        vp_fx eff, invEff;
        vp_fx dLambda;
        vp_fx old, neu;
        vpVec3 L;

        /* project ups onto plane perpendicular to axis */
        dotA = vpVec3_dot(upA, axis);
        dotB = vpVec3_dot(upB, axis);
        projA = vpVec3_sub(upA, vpVec3_scale(axis, dotA));
        projB = vpVec3_sub(upB, vpVec3_scale(axis, dotB));

        projA = vpVec3_normalize(projA);
        projB = vpVec3_normalize(projB);

        crossAB = vpVec3_cross(projA, projB);
        /* signed error around axis ~ dot(axis, cross(projA, projB)) */
        twistErr = vpVec3_dot(axis, crossAB);

        /* bias tries to drive twistErr -> 0 */
        bias = vp_fx_div(vp_fx_mul(beta, twistErr), dt);

        wrel = vpVec3_dot(vpVec3_sub(B->w_ang, A->w_ang), axis);

        eff = vp_effective_mass_angular(A, B, axis);
        invEff = (eff != 0) ? vp_fx_div(VP_FX_ONE, eff) : 0;

        dLambda = vp_fx_mul(invEff, ((vp_fx)(-wrel) - bias));

        old = j->lambda[5];
        neu = old + dLambda;
        j->lambda[5] = neu;
        dLambda = neu - old;

        if (vp_check_break_joint(j, dLambda, dt)) return;

        if (dLambda != 0) {
            L = vpVec3_scale(axis, dLambda);
            vp_apply_impulse_angular(A, L, 1);
            vp_apply_impulse_angular(B, L, 0);
            vp_wake_pair(A,B);
        }
    }
}

static void vp_solve_joint_rope(vpWorld* w, vpJoint* j, vp_fx dt, vp_fx beta)
{
    /* Rope is distance with only maxLen active. We'll call distance solver directly:
       it checks minLen/maxLen and exits if within range. For rope, minLen = 0. */
    vp_solve_joint_distance(w, j, dt, beta);
}

/* axis alignment (2 angular DOF) */
static void vp_solve_axis_alignment(vpBody* A, vpBody* B, vpVec3 axisA, vpVec3 axisB, vp_fx dt, vp_fx beta, vp_fx* lam0, vp_fx* lam1, vpJoint* j)
{
    vpVec3 errv;
    vpVec3 t1, t2;
    vpVec3 ax[2];
    vp_fx* lams[2];
    vp_u16 k;

    axisA = vpVec3_normalize(axisA);
    axisB = vpVec3_normalize(axisB);

    errv = vpVec3_cross(axisA, axisB);

    vpVec3_tangent_basis(axisA, &t1, &t2);
    ax[0] = t1; ax[1] = t2;
    lams[0] = lam0; lams[1] = lam1;

    for (k = 0; k < 2; ++k) {
        vp_fx ecomp = vpVec3_dot(errv, ax[k]);
        vp_fx bias = vp_fx_div(vp_fx_mul(beta, ecomp), dt);
        vp_fx wrel = vpVec3_dot(vpVec3_sub(B->w_ang, A->w_ang), ax[k]);
        vp_fx eff = vp_effective_mass_angular(A, B, ax[k]);
        vp_fx invEff = (eff != 0) ? vp_fx_div(VP_FX_ONE, eff) : 0;
        vp_fx dLambda = vp_fx_mul(invEff, ((vp_fx)(-wrel) - bias));
        *lams[k] += dLambda;

        if (vp_check_break_joint(j, dLambda, dt)) return;

        if (dLambda != 0) {
            vp_apply_impulse_angular(A, vpVec3_scale(ax[k], dLambda), 1);
            vp_apply_impulse_angular(B, vpVec3_scale(ax[k], dLambda), 0);
        }
    }
}

static void vp_solve_hinge_motor(vpBody* A, vpBody* B, vpVec3 axis, vp_fx dt, vpJoint* j)
{
    vp_fx wrel, eff, invEff, dLambda;
    vp_fx maxImpulse;
    vp_fx old, neu;

    if (!j->motorEnabled) return;

    axis = vpVec3_normalize(axis);

    wrel = vpVec3_dot(vpVec3_sub(B->w_ang, A->w_ang), axis);
    eff = vp_effective_mass_angular(A, B, axis);
    invEff = (eff != 0) ? vp_fx_div(VP_FX_ONE, eff) : 0;

    dLambda = vp_fx_mul(invEff, (j->motorTargetVel - wrel));

    /* clamp by max torque -> max impulse */
    maxImpulse = vp_fx_mul(j->motorMax, dt);

    old = j->lambda[5];
    neu = old + dLambda;
    neu = vp_fx_clamp(neu, -maxImpulse, maxImpulse);
    dLambda = neu - old;
    j->lambda[5] = neu;

    if (vp_check_break_joint(j, dLambda, dt)) return;

    if (dLambda != 0) {
        vp_apply_impulse_angular(A, vpVec3_scale(axis, dLambda), 1);
        vp_apply_impulse_angular(B, vpVec3_scale(axis, dLambda), 0);
    }
}

static void vp_solve_slider_limit(vpBody* A, vpBody* B, vpVec3 rA, vpVec3 rB, vpVec3 axis, vp_fx dt, vp_fx beta, vpJoint* j)
{
    vpVec3 pA, pB, d;
    vp_fx s, err, bias;
    vpVec3 velA, velB, relV;
    vp_fx vax;
    vp_fx eff, invEff, dLambda;
    vp_fx old, neu;
    vpVec3 P;

    if (!j->limitEnabled) return;

    axis = vpVec3_normalize(axis);

    pA = vpVec3_add(A->pos, rA);
    pB = vpVec3_add(B->pos, rB);
    d = vpVec3_sub(pB, pA);
    s = vpVec3_dot(d, axis);

    if (s < j->limitMin) {
        err = s - j->limitMin; /* negative */
    } else if (s > j->limitMax) {
        err = s - j->limitMax; /* positive */
    } else {
        return;
    }

    bias = vp_fx_div(vp_fx_mul(beta, err), dt);

    velA = vp_point_velocity(A, rA);
    velB = vp_point_velocity(B, rB);
    relV = vpVec3_sub(velB, velA);
    vax = vpVec3_dot(relV, axis);

    eff = vp_effective_mass(A,B,rA,rB,axis);
    invEff = (eff != 0) ? vp_fx_div(VP_FX_ONE, eff) : 0;

    dLambda = vp_fx_mul(invEff, ((vp_fx)(-vax) - bias));

    old = j->lambda[4];
    neu = old + dLambda;

    /* clamp direction: if below min, only push positive; if above max, only push negative */
    if (s < j->limitMin) {
        if (neu < 0) neu = 0;
    } else {
        if (neu > 0) neu = 0;
    }

    dLambda = neu - old;
    j->lambda[4] = neu;

    if (vp_check_break_joint(j, dLambda, dt)) return;

    if (dLambda != 0) {
        P = vpVec3_scale(axis, dLambda);
        vp_apply_impulse_at_point(A, P, rA, 1);
        vp_apply_impulse_at_point(B, P, rB, 0);
        vp_wake_pair(A,B);
    }
}

static void vp_solve_slider_motor(vpBody* A, vpBody* B, vpVec3 rA, vpVec3 rB, vpVec3 axis, vp_fx dt, vpJoint* j)
{
    vpVec3 velA, velB, relV;
    vp_fx vax;
    vp_fx eff, invEff, dLambda;
    vp_fx maxImpulse;
    vp_fx old, neu;
    vpVec3 P;

    if (!j->motorEnabled) return;

    axis = vpVec3_normalize(axis);

    velA = vp_point_velocity(A, rA);
    velB = vp_point_velocity(B, rB);
    relV = vpVec3_sub(velB, velA);
    vax = vpVec3_dot(relV, axis);

    eff = vp_effective_mass(A,B,rA,rB,axis);
    invEff = (eff != 0) ? vp_fx_div(VP_FX_ONE, eff) : 0;

    dLambda = vp_fx_mul(invEff, (j->motorTargetVel - vax));

    maxImpulse = vp_fx_mul(j->motorMax, dt);

    old = j->lambda[5];
    neu = old + dLambda;
    neu = vp_fx_clamp(neu, -maxImpulse, maxImpulse);
    dLambda = neu - old;
    j->lambda[5] = neu;

    if (vp_check_break_joint(j, dLambda, dt)) return;

    if (dLambda != 0) {
        P = vpVec3_scale(axis, dLambda);
        vp_apply_impulse_at_point(A, P, rA, 1);
        vp_apply_impulse_at_point(B, P, rB, 0);
        vp_wake_pair(A,B);
    }
}

static void vp_solve_joint_hinge(vpWorld* w, vpJoint* j, vp_fx dt, vp_fx beta)
{
    vpBody* A = vpWorldGetBody(w, (vpBodyId)j->bodyA);
    vpBody* B = vpWorldGetBody(w, (vpBodyId)j->bodyB);
    vpVec3 axisA, axisB;

    if (!A || !B) return;
    if (j->broken) return;

    vp_solve_joint_ball(w, j, dt, beta);

    vp_update_inertia(A); vp_update_inertia(B);

    axisA = vpQuat_rotate_vec3(A->rot, j->localAxisA);
    axisB = vpQuat_rotate_vec3(B->rot, j->localAxisB);

    /* use lambdas 3,4 for axis alignment */
    vp_solve_axis_alignment(A, B, axisA, axisB, dt, beta, &j->lambda[3], &j->lambda[4], j);

    /* motor along hinge axis (lambda[5]) */
    vp_solve_hinge_motor(A, B, axisA, dt, j);
}

static void vp_solve_joint_slider(vpWorld* w, vpJoint* j, vp_fx dt, vp_fx beta)
{
    vpBody* A = vpWorldGetBody(w, (vpBodyId)j->bodyA);
    vpBody* B = vpWorldGetBody(w, (vpBodyId)j->bodyB);
    vpVec3 rA, rB, pA, pB, err;
    vpVec3 axisA, axisB;
    vpVec3 t1, t2;
    vp_u16 k;

    if (!A || !B) return;
    if (j->broken) return;

    vp_update_inertia(A); vp_update_inertia(B);

    rA = vpQuat_rotate_vec3(A->rot, j->localA);
    rB = vpQuat_rotate_vec3(B->rot, j->localB);
    pA = vpVec3_add(A->pos, rA);
    pB = vpVec3_add(B->pos, rB);
    err = vpVec3_sub(pB, pA);

    axisA = vpVec3_normalize(vpQuat_rotate_vec3(A->rot, j->localAxisA));
    axisB = vpVec3_normalize(vpQuat_rotate_vec3(B->rot, j->localAxisB));

    /* constrain perpendicular position error to zero (2 linear constraints) */
    vpVec3_tangent_basis(axisA, &t1, &t2);
    {
        vpVec3 ax[2];
        ax[0] = t1; ax[1] = t2;
        for (k = 0; k < 2; ++k) {
            vp_fx ecomp = vpVec3_dot(err, ax[k]);
            vp_fx bias = vp_fx_div(vp_fx_mul(beta, ecomp), dt);
            vpVec3 velA = vp_point_velocity(A, rA);
            vpVec3 velB = vp_point_velocity(B, rB);
            vpVec3 relV = vpVec3_sub(velB, velA);
            vp_fx vax = vpVec3_dot(relV, ax[k]);
            vp_fx eff = vp_effective_mass(A,B,rA,rB,ax[k]);
            vp_fx invEff = (eff != 0) ? vp_fx_div(VP_FX_ONE, eff) : 0;
            vp_fx dLambda = vp_fx_mul(invEff, ((vp_fx)(-vax) - bias));

            j->lambda[k] += dLambda; /* lambda[0..1] */

            if (vp_check_break_joint(j, dLambda, dt)) return;

            if (dLambda != 0) {
                vpVec3 P = vpVec3_scale(ax[k], dLambda);
                vp_apply_impulse_at_point(A, P, rA, 1);
                vp_apply_impulse_at_point(B, P, rB, 0);
                vp_wake_pair(A,B);
            }
        }
    }

    /* axis alignment using lambda[2..3] */
    vp_solve_axis_alignment(A, B, axisA, axisB, dt, beta, &j->lambda[2], &j->lambda[3], j);

    /* limits along axis using lambda[4], motor uses lambda[5] */
    vp_solve_slider_limit(A, B, rA, rB, axisA, dt, beta, j);
    vp_solve_slider_motor(A, B, rA, rB, axisA, dt, j);
}

static void vp_solve_joint_keep_upright(vpWorld* w, vpJoint* j, vp_fx dt)
{
    vpBody* A = vpWorldGetBody(w, (vpBodyId)j->bodyA);
    vpBody* B = vpWorldGetBody(w, 0);
    vpVec3 upA, upW, axis;
    vp_fx angErr;
    vp_fx eff, invEff;
    vp_fx dLambda;
    vp_fx old, neu;
    vpVec3 L;

    VP_UNUSED(w);
    if (!A || !B) return;
    if (A->invMass == 0) return;

    vp_update_inertia(A); vp_update_inertia(B);

    upA = vpVec3_normalize(vpQuat_rotate_vec3(A->rot, j->localUpA));
    upW = vpVec3_normalize(j->worldUp);

    axis = vpVec3_cross(upA, upW);
    angErr = vpVec3_len(axis);
    if (angErr <= 0) return;

    axis = vpVec3_scale(axis, vp_fx_div(VP_FX_ONE, angErr));

    /* impulse proportional to error */
    dLambda = (vp_fx)(-vp_fx_mul(vp_fx_mul(j->stiffness, angErr), dt));

    eff = vp_effective_mass_angular(A, B, axis);
    invEff = (eff != 0) ? vp_fx_div(VP_FX_ONE, eff) : 0;
    dLambda = vp_fx_mul(dLambda, invEff);

    /* clamp using maxForce if provided */
    old = j->lambda[0];
    neu = old + dLambda;
    j->lambda[0] = neu;
    dLambda = neu - old;

    if (vp_check_break_joint(j, dLambda, dt)) return;

    L = vpVec3_scale(axis, dLambda);
    vp_apply_impulse_angular(A, L, 0);
    A->asleep = 0; A->sleepCounter = 0;
}

static void vp_solve_joint_physgun(vpWorld* w, vpJoint* j, vp_fx dt)
{
    vpBody* A = vpWorldGetBody(w, (vpBodyId)j->bodyA);
    vpBody* B = vpWorldGetBody(w, 0);
    vpVec3 rA, pA, d, n;
    vp_fx len;
    vpVec3 velA;
    vp_fx vn, Fs;
    vp_fx eff, invEff;
    vp_fx dLambda;
    vp_fx maxImpulse;
    vp_fx old, neu;
    vpVec3 P;

    VP_UNUSED(w);
    if (!A || !B) return;
    if (A->invMass == 0) return;

    vp_update_inertia(A); vp_update_inertia(B);

    rA = vpQuat_rotate_vec3(A->rot, j->localA);
    pA = vpVec3_add(A->pos, rA);

    d = vpVec3_sub(j->targetWorld, pA);
    len = vpVec3_len(d);
    if (len <= 0) return;
    n = vpVec3_scale(d, vp_fx_div(VP_FX_ONE, len));

    velA = vp_point_velocity(A, rA);
    vn = vpVec3_dot(velA, n);

    /* spring-damper force toward target:
       F = k*len - c*vn
       impulse ~ F*dt
    */
    Fs = (vp_fx)(vp_fx_mul(j->stiffness, len) - vp_fx_mul(j->damping, vn));
    dLambda = vp_fx_mul(Fs, dt);

    eff = vp_effective_mass(A,B,rA,vpVec3_make(0,0,0),n);
    invEff = (eff != 0) ? vp_fx_div(VP_FX_ONE, eff) : 0;
    dLambda = vp_fx_mul(dLambda, invEff);

    maxImpulse = vp_fx_mul(j->motorMax, dt); /* motorMax holds maxForce */
    old = j->lambda[0];
    neu = old + dLambda;
    neu = vp_fx_clamp(neu, -maxImpulse, maxImpulse);
    dLambda = neu - old;
    j->lambda[0] = neu;

    if (vp_check_break_joint(j, dLambda, dt)) return;

    if (dLambda != 0) {
        P = vpVec3_scale(n, dLambda);
        vp_apply_impulse_at_point(A, P, rA, 0);
        A->asleep = 0; A->sleepCounter = 0;
    }
}

void vpSolverWarmStart(vpWorld* w, vp_fx dt)
{
    vp_u16 i;
    VP_UNUSED(dt);
    for (i = 0; i < w->contactCount; ++i)
        vp_prepare_contact_restitution(w, &w->contacts[i]);
    vp_warmstart_contacts(w);
    vp_warmstart_joints(w);
}

void vpSolverSolve(vpWorld* w, vp_fx dt)
{
    vp_u16 iter;
    vp_u16 i;
    vp_fx slop = (vp_fx)VP_PENETRATION_SLOP;
    vp_fx beta = vp_fx_div(vp_fx_from_int(VP_BETA_NUM), vp_fx_from_int(VP_BETA_DEN));

    for (iter = 0; iter < w->solverIters; ++iter) {
        for (i = 0; i < w->contactCount; ++i) vp_solve_contact_normal(w, &w->contacts[i], dt, beta, slop);
        for (i = 0; i < w->contactCount; ++i) vp_solve_contact_friction(w, &w->contacts[i]);

        for (i = 0; i < w->maxJoints; ++i) {
            vpJoint* j = &w->joints[i];
            if (!j->used || j->broken) continue;

            if (j->type == VP_JOINT_DISTANCE) vp_solve_joint_distance(w, j, dt, beta);
            else if (j->type == VP_JOINT_SPRING) vp_solve_joint_spring(w, j, dt);
            else if (j->type == VP_JOINT_BALL) vp_solve_joint_ball(w, j, dt, beta);
            else if (j->type == VP_JOINT_HINGE) vp_solve_joint_hinge(w, j, dt, beta);
            else if (j->type == VP_JOINT_SLIDER) vp_solve_joint_slider(w, j, dt, beta);
            else if (j->type == VP_JOINT_KEEP_UPRIGHT) vp_solve_joint_keep_upright(w, j, dt);
            else if (j->type == VP_JOINT_PHYSGUN) vp_solve_joint_physgun(w, j, dt);
            else if (j->type == VP_JOINT_ROPE) vp_solve_joint_rope(w, j, dt, beta);
            else if (j->type == VP_JOINT_WELD) vp_solve_joint_weld(w, j, dt, beta);
        }
    }
}


/* ----------------------- Subset (island) solver ----------------------- */

void vpSolverWarmStartSubset(vpWorld* w, vp_fx dt,
    const vp_u16* contactIdx, vp_u16 contactCount,
    const vp_u16* jointIdx,   vp_u16 jointCount)
{
    vp_u16 i;

    VP_UNUSED(dt);

    for (i = 0; i < contactCount; ++i) {
        vp_u16 ci = contactIdx[i];
        if (ci < w->contactCount)
            vp_prepare_contact_restitution(w, &w->contacts[ci]);
    }

    /* contacts */
    for (i = 0; i < contactCount; ++i) {
        vp_u16 ci = contactIdx[i];
        vpContact* c;
        vpBody* A;
        vpBody* B;
        vpVec3 rA, rB;
        vpVec3 t1, t2;
        vpVec3 Pn, Pt1, Pt2, P;

        if (ci >= w->contactCount) continue;
        c = &w->contacts[ci];
        if (c->flags & VP_CONTACT_FLAG_SENSOR) continue;

        A = vpWorldGetBody(w, (vpBodyId)c->bodyA);
        B = vpWorldGetBody(w, (vpBodyId)c->bodyB);
        if (!A || !B) continue;

        vp_update_inertia(A); vp_update_inertia(B);

        rA = vpVec3_sub(c->p, A->pos);
        rB = vpVec3_sub(c->p, B->pos);

        vpVec3_tangent_basis(c->n, &t1, &t2);

        Pn  = vpVec3_scale(c->n,  c->lambdaN);
        Pt1 = vpVec3_scale(t1,    c->lambdaT1);
        Pt2 = vpVec3_scale(t2,    c->lambdaT2);
        P = vpVec3_add(Pn, vpVec3_add(Pt1, Pt2));

        vp_apply_impulse_at_point(A, P, rA, 1);
        vp_apply_impulse_at_point(B, P, rB, 0);
    }

    /* joints */
    for (i = 0; i < jointCount; ++i) {
        vp_u16 ji = jointIdx[i];
        vpJoint* j;
        vpBody* A;
        vpBody* B;

        if (ji >= w->maxJoints) continue;
        j = &w->joints[ji];
        if (!j->used || j->broken) continue;

        A = vpWorldGetBody(w, (vpBodyId)j->bodyA);
        B = vpWorldGetBody(w, (vpBodyId)j->bodyB);
        if (!A || !B) continue;

        /* Warm-start only the simple scalar constraints that the base solver stores in lambda[0]. */
        if (j->type == VP_JOINT_DISTANCE || j->type == VP_JOINT_ROPE ||
            j->type == VP_JOINT_SPRING || j->type == VP_JOINT_PHYSGUN) {

            vpVec3 worldA = vpVec3_add(A->pos, vpQuat_rotate_vec3(A->rot, j->localA));
            vpVec3 worldB = vpVec3_add(B->pos, vpQuat_rotate_vec3(B->rot, j->localB));
            vpVec3 dir = vpVec3_sub(worldB, worldA);
            vp_fx len = vpVec3_len(dir);
            if (len > 0) {
                vpVec3 n = vpVec3_scale(dir, vp_fx_div(VP_FX_ONE, len));
                vpVec3 rA = vpVec3_sub(worldA, A->pos);
                vpVec3 rB = vpVec3_sub(worldB, B->pos);
                vpVec3 P = vpVec3_scale(n, j->lambda[0]);
                vp_apply_impulse_at_point(A, P, rA, 1);
                vp_apply_impulse_at_point(B, P, rB, 0);
            }
        }
    }
}

void vpSolverSolveSubset(vpWorld* w, vp_fx dt,
    const vp_u16* contactIdx, vp_u16 contactCount,
    const vp_u16* jointIdx,   vp_u16 jointCount)
{
    vp_u16 iter;
    vp_fx beta = vp_fx_div(vp_fx_from_int((vp_i32)VP_BETA_NUM), vp_fx_from_int((vp_i32)VP_BETA_DEN));
    vp_fx slop = (vp_fx)VP_PENETRATION_SLOP;

    for (iter = 0; iter < w->solverIters; ++iter) {
        vp_u16 i;

        /* contacts: normal + (optional split impulse) */
        for (i = 0; i < contactCount; ++i) {
            vp_u16 ci = contactIdx[i];
            if (ci >= w->contactCount) continue;
            vp_solve_contact_normal(w, &w->contacts[ci], dt, beta, slop);
        }

        /* contacts: friction */
        for (i = 0; i < contactCount; ++i) {
            vp_u16 ci = contactIdx[i];
            if (ci >= w->contactCount) continue;
            vp_solve_contact_friction(w, &w->contacts[ci]);
        }

        /* joints */
        for (i = 0; i < jointCount; ++i) {
            vp_u16 ji = jointIdx[i];
            vpJoint* j;
            if (ji >= w->maxJoints) continue;
            j = &w->joints[ji];
            if (!j->used || j->broken) continue;
            if (j->type == VP_JOINT_DISTANCE) vp_solve_joint_distance(w, j, dt, beta);
            else if (j->type == VP_JOINT_SPRING) vp_solve_joint_spring(w, j, dt);
            else if (j->type == VP_JOINT_BALL) vp_solve_joint_ball(w, j, dt, beta);
            else if (j->type == VP_JOINT_HINGE) vp_solve_joint_hinge(w, j, dt, beta);
            else if (j->type == VP_JOINT_SLIDER) vp_solve_joint_slider(w, j, dt, beta);
            else if (j->type == VP_JOINT_KEEP_UPRIGHT) vp_solve_joint_keep_upright(w, j, dt);
            else if (j->type == VP_JOINT_PHYSGUN) vp_solve_joint_physgun(w, j, dt);
            else if (j->type == VP_JOINT_ROPE) vp_solve_joint_rope(w, j, dt, beta);
            else if (j->type == VP_JOINT_WELD) vp_solve_joint_weld(w, j, dt, beta);
        }
    }
}
