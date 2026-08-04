#ifndef VP_MATH3_H
#define VP_MATH3_H

#include "vp_fixed.h"

#ifndef VP_HEADER_FN
#if defined(__GNUC__) || defined(__clang__)
#define VP_HEADER_FN static __attribute__((unused))
#else
#define VP_HEADER_FN static
#endif
#endif

/* vec3 */
typedef struct vpVec3 { vp_fx x,y,z; } vpVec3;

VP_HEADER_FN vpVec3 vpVec3_make(vp_fx x,vp_fx y,vp_fx z){ vpVec3 v; v.x=x; v.y=y; v.z=z; return v; }
VP_HEADER_FN vpVec3 vpVec3_add(vpVec3 a,vpVec3 b){ return vpVec3_make(a.x+b.x,a.y+b.y,a.z+b.z); }
VP_HEADER_FN vpVec3 vpVec3_sub(vpVec3 a,vpVec3 b){ return vpVec3_make(a.x-b.x,a.y-b.y,a.z-b.z); }
VP_HEADER_FN vpVec3 vpVec3_scale(vpVec3 v,vp_fx s){ return vpVec3_make(vp_fx_mul(v.x,s),vp_fx_mul(v.y,s),vp_fx_mul(v.z,s)); }
VP_HEADER_FN vp_fx  vpVec3_dot(vpVec3 a,vpVec3 b){ return vp_fx_mul(a.x,b.x)+vp_fx_mul(a.y,b.y)+vp_fx_mul(a.z,b.z); }
VP_HEADER_FN vpVec3 vpVec3_cross(vpVec3 a,vpVec3 b)
{
    return vpVec3_make(
        vp_fx_mul(a.y,b.z)-vp_fx_mul(a.z,b.y),
        vp_fx_mul(a.z,b.x)-vp_fx_mul(a.x,b.z),
        vp_fx_mul(a.x,b.y)-vp_fx_mul(a.y,b.x)
    );
}
VP_HEADER_FN vp_fx  vpVec3_len2(vpVec3 v){ return vpVec3_dot(v,v); }
VP_HEADER_FN vp_fx  vpVec3_len(vpVec3 v){ return vp_fx_sqrt(vpVec3_len2(v)); }

VP_HEADER_FN vpVec3 vpVec3_normalize(vpVec3 v)
{
    vp_fx len = vpVec3_len(v);
    if (len <= 0) return v;
    return vpVec3_scale(v, vp_fx_div(VP_FX_ONE, len));
}

VP_HEADER_FN void vpVec3_tangent_basis(vpVec3 n, vpVec3* t1, vpVec3* t2)
{
    vpVec3 a, c;
    if (vp_fx_abs(n.y) < (VP_FX_ONE>>1)) a = vpVec3_make(0, VP_FX_ONE, 0);
    else a = vpVec3_make(VP_FX_ONE, 0, 0);
    c = vpVec3_cross(n, a);
    c = vpVec3_normalize(c);
    *t1 = c;
    *t2 = vpVec3_cross(n, c);
}

/* quat */
typedef struct vpQuat { vp_fx x,y,z,w; } vpQuat;

VP_HEADER_FN vpQuat vpQuat_identity(void){ vpQuat q; q.x=q.y=q.z=0; q.w=VP_FX_ONE; return q; }

VP_HEADER_FN vpQuat vpQuat_mul(vpQuat a, vpQuat b)
{
    vpQuat r;
    r.w = vp_fx_mul(a.w,b.w) - vp_fx_mul(a.x,b.x) - vp_fx_mul(a.y,b.y) - vp_fx_mul(a.z,b.z);
    r.x = vp_fx_mul(a.w,b.x) + vp_fx_mul(a.x,b.w) + vp_fx_mul(a.y,b.z) - vp_fx_mul(a.z,b.y);
    r.y = vp_fx_mul(a.w,b.y) - vp_fx_mul(a.x,b.z) + vp_fx_mul(a.y,b.w) + vp_fx_mul(a.z,b.x);
    r.z = vp_fx_mul(a.w,b.z) + vp_fx_mul(a.x,b.y) - vp_fx_mul(a.y,b.x) + vp_fx_mul(a.z,b.w);
    return r;
}

VP_HEADER_FN vpQuat vpQuat_normalize(vpQuat q)
{
    vp_fx len2 = vp_fx_mul(q.x,q.x)+vp_fx_mul(q.y,q.y)+vp_fx_mul(q.z,q.z)+vp_fx_mul(q.w,q.w);
    vp_fx len = vp_fx_sqrt(len2);
    if (len <= 0) return q;
    {
        vp_fx inv = vp_fx_div(VP_FX_ONE, len);
        q.x = vp_fx_mul(q.x, inv);
        q.y = vp_fx_mul(q.y, inv);
        q.z = vp_fx_mul(q.z, inv);
        q.w = vp_fx_mul(q.w, inv);
    }
    return q;
}

/* conjugate (inverse for unit quats) */
VP_HEADER_FN vpQuat vpQuat_conjugate(vpQuat q){ q.x = -q.x; q.y = -q.y; q.z = -q.z; return q; }
VP_HEADER_FN vpQuat vpQuat_neg(vpQuat q){ q.x=-q.x; q.y=-q.y; q.z=-q.z; q.w=-q.w; return q; }

VP_HEADER_FN vpQuat vpQuat_integrate(vpQuat q, vpVec3 w_ang, vp_fx dt)
{
    vpQuat omega, dq;
    vp_fx halfdt = vp_fx_mul(dt, VP_FX_HALF);
    omega.x = vp_fx_mul(w_ang.x, halfdt);
    omega.y = vp_fx_mul(w_ang.y, halfdt);
    omega.z = vp_fx_mul(w_ang.z, halfdt);
    omega.w = 0;
    dq = vpQuat_mul(omega, q);
    q.x += dq.x; q.y += dq.y; q.z += dq.z; q.w += dq.w;
    return vpQuat_normalize(q);
}

/* mat33 */
typedef struct vpMat33 {
    vp_fx m00,m01,m02;
    vp_fx m10,m11,m12;
    vp_fx m20,m21,m22;
} vpMat33;

VP_HEADER_FN vpMat33 vpMat33_from_quat(vpQuat q)
{
    vpMat33 R;
    vp_fx two = vp_fx_from_int(2);
    vp_fx xx = vp_fx_mul(q.x,q.x);
    vp_fx yy = vp_fx_mul(q.y,q.y);
    vp_fx zz = vp_fx_mul(q.z,q.z);
    vp_fx xy = vp_fx_mul(q.x,q.y);
    vp_fx xz = vp_fx_mul(q.x,q.z);
    vp_fx yz = vp_fx_mul(q.y,q.z);
    vp_fx wx = vp_fx_mul(q.w,q.x);
    vp_fx wy = vp_fx_mul(q.w,q.y);
    vp_fx wz = vp_fx_mul(q.w,q.z);

    R.m00 = VP_FX_ONE - vp_fx_mul(two, (yy+zz));
    R.m11 = VP_FX_ONE - vp_fx_mul(two, (xx+zz));
    R.m22 = VP_FX_ONE - vp_fx_mul(two, (xx+yy));

    R.m01 = vp_fx_mul(two, (xy - wz));
    R.m10 = vp_fx_mul(two, (xy + wz));

    R.m02 = vp_fx_mul(two, (xz + wy));
    R.m20 = vp_fx_mul(two, (xz - wy));

    R.m12 = vp_fx_mul(two, (yz - wx));
    R.m21 = vp_fx_mul(two, (yz + wx));
    return R;
}

VP_HEADER_FN vpVec3 vpMat33_mul_vec3(vpMat33 A, vpVec3 v)
{
    return vpVec3_make(
        vp_fx_mul(A.m00,v.x)+vp_fx_mul(A.m01,v.y)+vp_fx_mul(A.m02,v.z),
        vp_fx_mul(A.m10,v.x)+vp_fx_mul(A.m11,v.y)+vp_fx_mul(A.m12,v.z),
        vp_fx_mul(A.m20,v.x)+vp_fx_mul(A.m21,v.y)+vp_fx_mul(A.m22,v.z)
    );
}

VP_HEADER_FN vpMat33 vpMat33_inertia_world_from_quat_diag(vpQuat q, vpVec3 invI)
{
    vpMat33 R = vpMat33_from_quat(q);
    vpMat33 Iw;
    vpVec3 c0 = vpVec3_make(R.m00,R.m10,R.m20);
    vpVec3 c1 = vpVec3_make(R.m01,R.m11,R.m21);
    vpVec3 c2 = vpVec3_make(R.m02,R.m12,R.m22);
    vp_fx d0=invI.x, d1=invI.y, d2=invI.z;

    Iw.m00 = vp_fx_mul(d0, vp_fx_mul(c0.x,c0.x)) + vp_fx_mul(d1, vp_fx_mul(c1.x,c1.x)) + vp_fx_mul(d2, vp_fx_mul(c2.x,c2.x));
    Iw.m01 = vp_fx_mul(d0, vp_fx_mul(c0.x,c0.y)) + vp_fx_mul(d1, vp_fx_mul(c1.x,c1.y)) + vp_fx_mul(d2, vp_fx_mul(c2.x,c2.y));
    Iw.m02 = vp_fx_mul(d0, vp_fx_mul(c0.x,c0.z)) + vp_fx_mul(d1, vp_fx_mul(c1.x,c1.z)) + vp_fx_mul(d2, vp_fx_mul(c2.x,c2.z));

    Iw.m10 = Iw.m01;
    Iw.m11 = vp_fx_mul(d0, vp_fx_mul(c0.y,c0.y)) + vp_fx_mul(d1, vp_fx_mul(c1.y,c1.y)) + vp_fx_mul(d2, vp_fx_mul(c2.y,c2.y));
    Iw.m12 = vp_fx_mul(d0, vp_fx_mul(c0.y,c0.z)) + vp_fx_mul(d1, vp_fx_mul(c1.y,c1.z)) + vp_fx_mul(d2, vp_fx_mul(c2.y,c2.z));

    Iw.m20 = Iw.m02;
    Iw.m21 = Iw.m12;
    Iw.m22 = vp_fx_mul(d0, vp_fx_mul(c0.z,c0.z)) + vp_fx_mul(d1, vp_fx_mul(c1.z,c1.z)) + vp_fx_mul(d2, vp_fx_mul(c2.z,c2.z));
    return Iw;
}

VP_HEADER_FN vpVec3 vpQuat_rotate_vec3(vpQuat q, vpVec3 v)
{
    return vpMat33_mul_vec3(vpMat33_from_quat(q), v);
}

VP_HEADER_FN vpVec3 vpQuat_rotate_inv_vec3(vpQuat q, vpVec3 v)
{
    return vpMat33_mul_vec3(vpMat33_from_quat(vpQuat_conjugate(q)), v);
}

/* small-angle friendly angular velocity from qPrev->qCur over dt */
VP_HEADER_FN vpVec3 vpQuat_delta_to_angvel(vpQuat qPrev, vpQuat qCur, vp_fx dt)
{
    vpQuat dq = vpQuat_mul(qCur, vpQuat_conjugate(qPrev));
    if (dq.w < 0) dq = vpQuat_neg(dq);
    if (dt == 0) return vpVec3_make(0,0,0);
    {
        vp_fx two_over_dt = vp_fx_div(vp_fx_from_int(2), dt);
        return vpVec3_make(
            vp_fx_mul(dq.x, two_over_dt),
            vp_fx_mul(dq.y, two_over_dt),
            vp_fx_mul(dq.z, two_over_dt));
    }
}

#endif /* VP_MATH3_H */
