#include "ccs_narrow.h"
#include "ccs_sat.h" /* OBB vs OBB + manifold */

/* ============================================================
   Internal helpers
   ============================================================ */

static ccs_fixed ccs_clamp01(ccs_fixed t)
{
    if (t < 0) return 0;
    if (t > CCS_FIXED_ONE) return CCS_FIXED_ONE;
    return t;
}

static ccs_vec3 ccs_clamp_point_aabb(ccs_vec3 p, ccs_vec3 bmin, ccs_vec3 bmax)
{
    ccs_vec3 q;
    q.x = ccs_fixed_clamp(p.x, bmin.x, bmax.x);
    q.y = ccs_fixed_clamp(p.y, bmin.y, bmax.y);
    q.z = ccs_fixed_clamp(p.z, bmin.z, bmax.z);
    return q;
}

static ccs_vec3 ccs_closest_point_on_segment(ccs_vec3 a, ccs_vec3 b, ccs_vec3 p, ccs_fixed* out_t)
{
    ccs_vec3 ab;
    ccs_vec3 ap;
    ccs_fixed denom;
    ccs_fixed t;

    ab = ccs_vec3_sub(b, a);
    ap = ccs_vec3_sub(p, a);

    denom = ccs_vec3_dot(ab, ab);
    if (denom <= 0) {
        if (out_t) *out_t = 0;
        return a;
    }

    t = ccs_fixed_div(ccs_vec3_dot(ap, ab), denom);
    t = ccs_clamp01(t);

    if (out_t) *out_t = t;

    return ccs_vec3_add(a, ccs_vec3_scale(ab, t));
}

/* Closest points between two segments (Ericson). Returns squared distance. */
static ccs_fixed ccs_closest_segment_segment(
    ccs_vec3 p1, ccs_vec3 q1,
    ccs_vec3 p2, ccs_vec3 q2,
    ccs_vec3* c1, ccs_vec3* c2,
    ccs_fixed* s_out, ccs_fixed* t_out
) {
    ccs_vec3 d1;
    ccs_vec3 d2;
    ccs_vec3 r;
    ccs_fixed a;
    ccs_fixed e;
    ccs_fixed f;

    ccs_fixed s;
    ccs_fixed t;

    ccs_fixed b;
    ccs_fixed c;

    ccs_fixed denom;

    d1 = ccs_vec3_sub(q1, p1);
    d2 = ccs_vec3_sub(q2, p2);
    r  = ccs_vec3_sub(p1, p2);

    a = ccs_vec3_dot(d1, d1); /* squared length d1 */
    e = ccs_vec3_dot(d2, d2); /* squared length d2 */
    f = ccs_vec3_dot(d2, r);

    /* default */
    s = 0;
    t = 0;

    if (a <= 0 && e <= 0) {
        /* both segments degenerate */
        if (c1) *c1 = p1;
        if (c2) *c2 = p2;
        if (s_out) *s_out = 0;
        if (t_out) *t_out = 0;
        return ccs_vec3_len_sq(ccs_vec3_sub(p1, p2));
    }

    if (a <= 0) {
        /* first degenerate */
        s = 0;
        t = ccs_fixed_div(f, e);
        t = ccs_clamp01(t);
    } else {
        c = ccs_vec3_dot(d1, r);
        if (e <= 0) {
            /* second degenerate */
            t = 0;
            s = ccs_fixed_div(-c, a);
            s = ccs_clamp01(s);
        } else {
            b = ccs_vec3_dot(d1, d2);
            denom = ccs_fixed_mul(a, e) - ccs_fixed_mul(b, b);

            if (denom != 0) {
                s = ccs_fixed_div((ccs_fixed_mul(b, f) - ccs_fixed_mul(c, e)), denom);
                s = ccs_clamp01(s);
            } else {
                s = 0;
            }

            t = ccs_fixed_div((ccs_fixed_mul(b, s) + f), e);

            if (t < 0) {
                t = 0;
                s = ccs_fixed_div(-c, a);
                s = ccs_clamp01(s);
            } else if (t > CCS_FIXED_ONE) {
                t = CCS_FIXED_ONE;
                s = ccs_fixed_div((b - c), a);
                s = ccs_clamp01(s);
            }
        }
    }

    if (c1) *c1 = ccs_vec3_add(p1, ccs_vec3_scale(d1, s));
    if (c2) *c2 = ccs_vec3_add(p2, ccs_vec3_scale(d2, t));
    if (s_out) *s_out = s;
    if (t_out) *t_out = t;

    return ccs_vec3_len_sq(ccs_vec3_sub(
        ccs_vec3_add(p1, ccs_vec3_scale(d1, s)),
        ccs_vec3_add(p2, ccs_vec3_scale(d2, t))
    ));
}

/* Distance between segment and AABB by interval enumeration (deterministic). */
static ccs_fixed ccs_closest_segment_aabb(
    ccs_vec3 p0,
    ccs_vec3 p1,
    ccs_vec3 bmin,
    ccs_vec3 bmax,
    ccs_vec3* out_seg,
    ccs_vec3* out_box
) {
    ccs_vec3 d;
    ccs_fixed ts[8];
    int n;
    int i;

    ccs_fixed best_t;
    ccs_fixed best_dist_sq;

    d = ccs_vec3_sub(p1, p0);

    n = 0;
    ts[n++] = 0;
    ts[n++] = CCS_FIXED_ONE;

    /* gather boundary t for each axis */
    {
        ccs_fixed t1;
        ccs_fixed t2;

        if (d.x != 0) {
            t1 = ccs_fixed_div((bmin.x - p0.x), d.x);
            t2 = ccs_fixed_div((bmax.x - p0.x), d.x);
            if (t1 > 0 && t1 < CCS_FIXED_ONE) ts[n++] = t1;
            if (t2 > 0 && t2 < CCS_FIXED_ONE) ts[n++] = t2;
        }
        if (d.y != 0) {
            t1 = ccs_fixed_div((bmin.y - p0.y), d.y);
            t2 = ccs_fixed_div((bmax.y - p0.y), d.y);
            if (t1 > 0 && t1 < CCS_FIXED_ONE) ts[n++] = t1;
            if (t2 > 0 && t2 < CCS_FIXED_ONE) ts[n++] = t2;
        }
        if (d.z != 0) {
            t1 = ccs_fixed_div((bmin.z - p0.z), d.z);
            t2 = ccs_fixed_div((bmax.z - p0.z), d.z);
            if (t1 > 0 && t1 < CCS_FIXED_ONE) ts[n++] = t1;
            if (t2 > 0 && t2 < CCS_FIXED_ONE) ts[n++] = t2;
        }
    }

    /* sort ascending (insertion sort) */
    for (i = 1; i < n; ++i) {
        ccs_fixed key;
        int j;
        key = ts[i];
        j = i - 1;
        while (j >= 0 && ts[j] > key) {
            ts[j + 1] = ts[j];
            j--;
        }
        ts[j + 1] = key;
    }

    best_t = 0;
    best_dist_sq = (ccs_fixed)CCS_I32_MAX;

    /* evaluate intervals */
    for (i = 0; i < n - 1; ++i) {
        ccs_fixed t0;
        ccs_fixed t1;
        ccs_fixed tm;
        ccs_vec3 pm;

        int clamp_x;
        int clamp_y;
        int clamp_z;

        ccs_fixed A;
        ccs_fixed B;

        ccs_fixed cand_t[3];
        int cand_n;
        int k;

        t0 = ts[i];
        t1 = ts[i + 1];
        if (t1 < t0) continue;

        tm = (t0 + t1) / 2;
        pm = ccs_vec3_add(p0, ccs_vec3_scale(d, tm));

        clamp_x = (pm.x < bmin.x) ? -1 : ((pm.x > bmax.x) ? 1 : 0);
        clamp_y = (pm.y < bmin.y) ? -1 : ((pm.y > bmax.y) ? 1 : 0);
        clamp_z = (pm.z < bmin.z) ? -1 : ((pm.z > bmax.z) ? 1 : 0);

        /* coefficients for f(t) = sum (p0_i + d_i t - c_i)^2 over clamped axes */
        A = 0;
        B = 0;

        if (clamp_x != 0) {
            ccs_fixed ci;
            ccs_fixed bi;
            ci = (clamp_x < 0) ? bmin.x : bmax.x;
            bi = p0.x - ci;
            A += ccs_fixed_mul(d.x, d.x);
            B += (ccs_fixed)(2) * ccs_fixed_mul(d.x, bi);
        }
        if (clamp_y != 0) {
            ccs_fixed ci;
            ccs_fixed bi;
            ci = (clamp_y < 0) ? bmin.y : bmax.y;
            bi = p0.y - ci;
            A += ccs_fixed_mul(d.y, d.y);
            B += (ccs_fixed)(2) * ccs_fixed_mul(d.y, bi);
        }
        if (clamp_z != 0) {
            ccs_fixed ci;
            ccs_fixed bi;
            ci = (clamp_z < 0) ? bmin.z : bmax.z;
            bi = p0.z - ci;
            A += ccs_fixed_mul(d.z, d.z);
            B += (ccs_fixed)(2) * ccs_fixed_mul(d.z, bi);
        }

        cand_n = 0;
        cand_t[cand_n++] = t0;
        cand_t[cand_n++] = t1;

        if (A != 0) {
            ccs_fixed t_star;
            /* t* = -B / (2A) */
            t_star = ccs_fixed_div(-B, (ccs_fixed)(2) * A);
            if (t_star < t0) t_star = t0;
            if (t_star > t1) t_star = t1;
            cand_t[cand_n++] = t_star;
        }

        for (k = 0; k < cand_n; ++k) {
            ccs_fixed t;
            ccs_vec3 p;
            ccs_vec3 q;
            ccs_vec3 diff;
            ccs_fixed dist_sq;

            t = cand_t[k];
            p = ccs_vec3_add(p0, ccs_vec3_scale(d, t));
            q = ccs_clamp_point_aabb(p, bmin, bmax);
            diff = ccs_vec3_sub(p, q);
            dist_sq = ccs_vec3_len_sq(diff);

            if (dist_sq < best_dist_sq) {
                best_dist_sq = dist_sq;
                best_t = t;
            }
        }
    }

    {
        ccs_vec3 p;
        ccs_vec3 q;

        p = ccs_vec3_add(p0, ccs_vec3_scale(d, best_t));
        q = ccs_clamp_point_aabb(p, bmin, bmax);

        if (out_seg) *out_seg = p;
        if (out_box) *out_box = q;

        return ccs_vec3_len_sq(ccs_vec3_sub(p, q));
    }
}

/* ============================================================
   Sphere vs Sphere
   ============================================================ */

int ccs_narrow_sphere_sphere(
    const ccs_sphere* a,
    const ccs_sphere* b,
    ccs_contact* out
) {
    ccs_vec3 delta;
    ccs_fixed dist_sq;
    ccs_fixed radius_sum;
    ccs_fixed radius_sum_sq;
    ccs_fixed dist;

    if (!a || !b || !out)
        return 0;

    delta = ccs_vec3_sub(b->center, a->center);
    dist_sq = ccs_vec3_len_sq(delta);

    radius_sum = a->radius + b->radius;
    radius_sum_sq = ccs_fixed_mul(radius_sum, radius_sum);

    if (dist_sq > radius_sum_sq)
        return 0;

    dist = CCS_FIXED_SQRT(dist_sq);

    if (dist == 0) {
        out->normal = ccs_vec3_axis_x();
        out->penetration = radius_sum;
    } else {
        out->normal = ccs_vec3_scale(delta, ccs_fixed_div(CCS_FIXED_ONE, dist));
        out->penetration = radius_sum - dist;
    }

    return 1;
}

/* ============================================================
   Box vs Box (AABB SAT)
   ============================================================ */

int ccs_narrow_box_box(
    const ccs_box* a,
    const ccs_box* b,
    ccs_contact* out
) {
    ccs_vec3 delta;
    ccs_vec3 overlap;

    if (!a || !b || !out)
        return 0;

    delta = ccs_vec3_sub(b->center, a->center);

    overlap.x = a->half_extents.x + b->half_extents.x - ccs_fixed_abs(delta.x);
    overlap.y = a->half_extents.y + b->half_extents.y - ccs_fixed_abs(delta.y);
    overlap.z = a->half_extents.z + b->half_extents.z - ccs_fixed_abs(delta.z);

    if (overlap.x <= 0 || overlap.y <= 0 || overlap.z <= 0)
        return 0;

    /* find minimum penetration axis */
    if (overlap.x < overlap.y && overlap.x < overlap.z) {
        out->penetration = overlap.x;
        out->normal = ccs_vec3_make((delta.x < 0) ? -CCS_FIXED_ONE : CCS_FIXED_ONE, 0, 0);
    } else if (overlap.y < overlap.z) {
        out->penetration = overlap.y;
        out->normal = ccs_vec3_make(0, (delta.y < 0) ? -CCS_FIXED_ONE : CCS_FIXED_ONE, 0);
    } else {
        out->penetration = overlap.z;
        out->normal = ccs_vec3_make(0, 0, (delta.z < 0) ? -CCS_FIXED_ONE : CCS_FIXED_ONE);
    }

    return 1;
}

/* ============================================================
   Sphere vs AABB (Sphere=A, Box=B): normal A->B
   ============================================================ */

int ccs_narrow_sphere_box(
    const ccs_sphere* s,
    const ccs_box* b,
    ccs_contact* out
) {
    ccs_vec3 bmin;
    ccs_vec3 bmax;
    ccs_vec3 closest;
    ccs_vec3 delta;
    ccs_fixed dist_sq;
    ccs_fixed r2;
    ccs_fixed dist;

    if (!s || !b || !out)
        return 0;

    bmin = ccs_vec3_sub(b->center, b->half_extents);
    bmax = ccs_vec3_add(b->center, b->half_extents);

    closest = ccs_clamp_point_aabb(s->center, bmin, bmax);

    /* normal A->B : from sphere center to closest point on box */
    delta = ccs_vec3_sub(closest, s->center);
    dist_sq = ccs_vec3_len_sq(delta);

    r2 = ccs_fixed_mul(s->radius, s->radius);

    if (dist_sq > r2)
        return 0;

    dist = CCS_FIXED_SQRT(dist_sq);

    if (dist == 0) {
        /* sphere center inside box: push towards nearest face */
        ccs_vec3 local;
        ccs_fixed dxp;
        ccs_fixed dxn;
        ccs_fixed dyp;
        ccs_fixed dyn;
        ccs_fixed dzp;
        ccs_fixed dzn;
        ccs_fixed best;
        ccs_vec3 n;

        local = ccs_vec3_sub(s->center, b->center);

        dxp = b->half_extents.x - local.x;
        dxn = b->half_extents.x + local.x;
        dyp = b->half_extents.y - local.y;
        dyn = b->half_extents.y + local.y;
        dzp = b->half_extents.z - local.z;
        dzn = b->half_extents.z + local.z;

        best = dxp;
        n = ccs_vec3_make(CCS_FIXED_ONE, 0, 0);
        if (dxn < best) { best = dxn; n = ccs_vec3_make(-CCS_FIXED_ONE, 0, 0); }
        if (dyp < best) { best = dyp; n = ccs_vec3_make(0, CCS_FIXED_ONE, 0); }
        if (dyn < best) { best = dyn; n = ccs_vec3_make(0, -CCS_FIXED_ONE, 0); }
        if (dzp < best) { best = dzp; n = ccs_vec3_make(0, 0, CCS_FIXED_ONE); }
        if (dzn < best) { best = dzn; n = ccs_vec3_make(0, 0, -CCS_FIXED_ONE); }

        out->normal = n; /* already A->B */
        out->penetration = s->radius + best;
    } else {
        out->normal = ccs_vec3_scale(delta, ccs_fixed_div(CCS_FIXED_ONE, dist));
        out->penetration = s->radius - dist;
    }

    return 1;
}

/* ============================================================
   Sphere vs OBB (Sphere=A, OBB=B): normal A->B
   ============================================================ */

int ccs_narrow_sphere_obb(
    const ccs_sphere* s,
    const ccs_obb* b,
    ccs_contact* out
) {
    ccs_vec3 p_local;
    ccs_vec3 bmin;
    ccs_vec3 bmax;
    ccs_vec3 q_local;
    ccs_vec3 delta_local;
    ccs_fixed dist_sq;
    ccs_fixed r2;
    ccs_fixed dist;

    if (!s || !b || !out)
        return 0;

    p_local = ccs_obb_to_local_point(b, s->center);

    bmin = ccs_vec3_neg(b->half);
    bmax = b->half;

    q_local = ccs_clamp_point_aabb(p_local, bmin, bmax);

    delta_local = ccs_vec3_sub(q_local, p_local); /* A->B in local */
    dist_sq = ccs_vec3_len_sq(delta_local);

    r2 = ccs_fixed_mul(s->radius, s->radius);
    if (dist_sq > r2)
        return 0;

    dist = CCS_FIXED_SQRT(dist_sq);

    if (dist == 0) {
        /* center inside: choose nearest face */
        ccs_fixed dxp;
        ccs_fixed dxn;
        ccs_fixed dyp;
        ccs_fixed dyn;
        ccs_fixed dzp;
        ccs_fixed dzn;
        ccs_fixed best;
        ccs_vec3 n_local;

        dxp = b->half.x - p_local.x;
        dxn = b->half.x + p_local.x;
        dyp = b->half.y - p_local.y;
        dyn = b->half.y + p_local.y;
        dzp = b->half.z - p_local.z;
        dzn = b->half.z + p_local.z;

        best = dxp;
        n_local = ccs_vec3_make(CCS_FIXED_ONE, 0, 0);
        if (dxn < best) { best = dxn; n_local = ccs_vec3_make(-CCS_FIXED_ONE, 0, 0); }
        if (dyp < best) { best = dyp; n_local = ccs_vec3_make(0, CCS_FIXED_ONE, 0); }
        if (dyn < best) { best = dyn; n_local = ccs_vec3_make(0, -CCS_FIXED_ONE, 0); }
        if (dzp < best) { best = dzp; n_local = ccs_vec3_make(0, 0, CCS_FIXED_ONE); }
        if (dzn < best) { best = dzn; n_local = ccs_vec3_make(0, 0, -CCS_FIXED_ONE); }

        out->normal = ccs_obb_to_world_dir(b, n_local);
        out->penetration = s->radius + best;
    } else {
        out->normal = ccs_obb_to_world_dir(b, ccs_vec3_scale(delta_local, ccs_fixed_div(CCS_FIXED_ONE, dist)));
        out->penetration = s->radius - dist;
    }

    return 1;
}

/* ============================================================
   OBB vs OBB (SAT)
   ============================================================ */

int ccs_narrow_obb_obb(
    const ccs_obb* a,
    const ccs_obb* b,
    ccs_contact* out
) {
    return ccs_sat_obb_obb(a, b, out);
}

/* ============================================================
   Capsule vs Sphere (Capsule=A, Sphere=B): normal A->B
   ============================================================ */

int ccs_narrow_capsule_sphere(
    const ccs_capsule* a,
    const ccs_sphere* b,
    ccs_contact* out
) {
    ccs_vec3 ca;
    ccs_vec3 cb;
    ccs_vec3 pA;
    ccs_fixed t;
    ccs_vec3 d;
    ccs_fixed dist_sq;
    ccs_fixed r;
    ccs_fixed r2;
    ccs_fixed dist;

    if (!a || !b || !out)
        return 0;

    ccs_capsule_endpoints(a, &ca, &cb);

    /* closest point on capsule segment to sphere center */
    pA = ccs_closest_point_on_segment(ca, cb, b->center, &t);

    d = ccs_vec3_sub(b->center, pA); /* from capsule to sphere center */
    dist_sq = ccs_vec3_len_sq(d);

    r = a->radius + b->radius;
    r2 = ccs_fixed_mul(r, r);

    if (dist_sq > r2)
        return 0;

    dist = CCS_FIXED_SQRT(dist_sq);

    if (dist == 0) {
        out->normal = ccs_vec3_axis_x();
        out->penetration = r;
    } else {
        /* normal A->B should point from capsule to sphere:
           from pA (capsule axis) to sphere center.
           But we need normal from capsule surface to sphere.
           Same direction.
        */
        out->normal = ccs_vec3_scale(d, ccs_fixed_div(CCS_FIXED_ONE, dist));
        out->penetration = r - dist;
    }

    return 1;
}

/* ============================================================
   Capsule vs Capsule (A vs B): normal A->B
   ============================================================ */

int ccs_narrow_capsule_capsule(
    const ccs_capsule* a,
    const ccs_capsule* b,
    ccs_contact* out
) {
    ccs_vec3 a0;
    ccs_vec3 a1;
    ccs_vec3 b0;
    ccs_vec3 b1;
    ccs_vec3 pA;
    ccs_vec3 pB;
    ccs_vec3 d;
    ccs_fixed dist_sq;
    ccs_fixed r;
    ccs_fixed r2;
    ccs_fixed dist;

    if (!a || !b || !out)
        return 0;

    ccs_capsule_endpoints(a, &a0, &a1);
    ccs_capsule_endpoints(b, &b0, &b1);

    dist_sq = ccs_closest_segment_segment(a0, a1, b0, b1, &pA, &pB, 0, 0);

    d = ccs_vec3_sub(pB, pA);

    r = a->radius + b->radius;
    r2 = ccs_fixed_mul(r, r);

    if (dist_sq > r2)
        return 0;

    dist = CCS_FIXED_SQRT(dist_sq);

    if (dist == 0) {
        out->normal = ccs_vec3_axis_x();
        out->penetration = r;
    } else {
        out->normal = ccs_vec3_scale(d, ccs_fixed_div(CCS_FIXED_ONE, dist));
        out->penetration = r - dist;
    }

    return 1;
}

/* ============================================================
   Capsule vs OBB (A capsule, B obb): normal A->B
   ============================================================ */

int ccs_narrow_capsule_obb(
    const ccs_capsule* a,
    const ccs_obb* b,
    ccs_contact* out
) {
    ccs_vec3 a0;
    ccs_vec3 a1;
    ccs_vec3 p0_local;
    ccs_vec3 p1_local;
    ccs_vec3 seg_closest;
    ccs_vec3 box_closest;
    ccs_fixed dist_sq;
    ccs_fixed r2;
    ccs_fixed dist;
    ccs_vec3 n_local;

    ccs_vec3 bmin;
    ccs_vec3 bmax;

    if (!a || !b || !out)
        return 0;

    ccs_capsule_endpoints(a, &a0, &a1);

    /* segment into OBB local space */
    p0_local = ccs_obb_to_local_point(b, a0);
    p1_local = ccs_obb_to_local_point(b, a1);

    bmin = ccs_vec3_neg(b->half);
    bmax = b->half;

    dist_sq = ccs_closest_segment_aabb(p0_local, p1_local, bmin, bmax, &seg_closest, &box_closest);

    r2 = ccs_fixed_mul(a->radius, a->radius);
    if (dist_sq > r2)
        return 0;

    dist = CCS_FIXED_SQRT(dist_sq);

    if (dist == 0) {
        /* segment intersects box: choose nearest face from seg_closest */
        ccs_fixed dxp;
        ccs_fixed dxn;
        ccs_fixed dyp;
        ccs_fixed dyn;
        ccs_fixed dzp;
        ccs_fixed dzn;
        ccs_fixed best;

        dxp = b->half.x - seg_closest.x;
        dxn = b->half.x + seg_closest.x;
        dyp = b->half.y - seg_closest.y;
        dyn = b->half.y + seg_closest.y;
        dzp = b->half.z - seg_closest.z;
        dzn = b->half.z + seg_closest.z;

        best = dxp;
        n_local = ccs_vec3_make(CCS_FIXED_ONE, 0, 0);
        if (dxn < best) { best = dxn; n_local = ccs_vec3_make(-CCS_FIXED_ONE, 0, 0); }
        if (dyp < best) { best = dyp; n_local = ccs_vec3_make(0, CCS_FIXED_ONE, 0); }
        if (dyn < best) { best = dyn; n_local = ccs_vec3_make(0, -CCS_FIXED_ONE, 0); }
        if (dzp < best) { best = dzp; n_local = ccs_vec3_make(0, 0, CCS_FIXED_ONE); }
        if (dzn < best) { best = dzn; n_local = ccs_vec3_make(0, 0, -CCS_FIXED_ONE); }

        out->normal = ccs_obb_to_world_dir(b, n_local);
        out->penetration = a->radius + best;
    } else {
        n_local = ccs_vec3_scale(ccs_vec3_sub(box_closest, seg_closest), ccs_fixed_div(CCS_FIXED_ONE, dist));
        out->normal = ccs_obb_to_world_dir(b, n_local);
        out->penetration = a->radius - dist;
    }

    return 1;
}

/* ============================================================
   Manifold helpers
   ============================================================ */

static int ccs_feature_hash_point_local(ccs_vec3 p_local)
{
    /* Quantize to 1/32 units: q = floor(p * 32). */
    ccs_fixed k32;
    int qx;
    int qy;
    int qz;

    k32 = ccs_fixed_from_int(32);

    qx = (int)ccs_fixed_to_int_floor(ccs_fixed_mul(p_local.x, k32));
    qy = (int)ccs_fixed_to_int_floor(ccs_fixed_mul(p_local.y, k32));
    qz = (int)ccs_fixed_to_int_floor(ccs_fixed_mul(p_local.z, k32));

    /* pack into 30 bits (10 each) */
    return (qx & 1023) | ((qy & 1023) << 10) | ((qz & 1023) << 20);
}

/* ============================================================
   Box vs Box manifold (AABB)
   ============================================================ */

int ccs_narrow_box_box_manifold(
    const ccs_box* a,
    const ccs_box* b,
    ccs_manifold* out
) {
    ccs_obb oa;
    ccs_obb ob;

    if (!a || !b || !out)
        return 0;

    oa.center = a->center;
    oa.half = a->half_extents;
    ccs_obb_set_identity(&oa);

    ob.center = b->center;
    ob.half = b->half_extents;
    ccs_obb_set_identity(&ob);

    return ccs_sat_obb_obb_manifold(&oa, &ob, out);
}

int ccs_narrow_obb_obb_manifold(
    const ccs_obb* a,
    const ccs_obb* b,
    ccs_manifold* out
) {
    return ccs_sat_obb_obb_manifold(a, b, out);
}

/* Capsule vs Sphere manifold (1 point) */
int ccs_narrow_capsule_sphere_manifold(
    const ccs_capsule* a,
    const ccs_sphere* b,
    ccs_manifold* out
) {
    ccs_vec3 a0;
    ccs_vec3 a1;
    ccs_vec3 pA;
    ccs_fixed t;
    ccs_vec3 d;
    ccs_fixed dist_sq;
    ccs_fixed r;
    ccs_fixed dist;
    ccs_vec3 n;
    ccs_vec3 pa;
    ccs_vec3 pb;

    if (!a || !b || !out)
        return 0;

    ccs_manifold_clear(out);

    ccs_capsule_endpoints(a, &a0, &a1);
    pA = ccs_closest_point_on_segment(a0, a1, b->center, &t);

    d = ccs_vec3_sub(b->center, pA);
    dist_sq = ccs_vec3_len_sq(d);

    r = a->radius + b->radius;
    if (dist_sq > ccs_fixed_mul(r, r))
        return 0;

    dist = CCS_FIXED_SQRT(dist_sq);
    if (dist == 0) {
        n = ccs_vec3_axis_x();
    } else {
        n = ccs_vec3_scale(d, ccs_fixed_div(CCS_FIXED_ONE, dist));
    }

    pa = ccs_vec3_add(pA, ccs_vec3_scale(n, a->radius));
    pb = ccs_vec3_sub(b->center, ccs_vec3_scale(n, b->radius));

    out->count = 1;
    out->contacts[0].normal = n;
    out->contacts[0].penetration = r - dist;
    out->contacts[0].point = ccs_vec3_scale(ccs_vec3_add(pa, pb), CCS_FIXED_HALF);

    /* stable feature id: quantized capsule param + sphere */
    out->contacts[0].feature_id = (int)((t >> 10) & 0xFFFF) ^ 0x5150;

    return 1;
}

/* Capsule vs Capsule manifold (1 point) */
int ccs_narrow_capsule_capsule_manifold(
    const ccs_capsule* a,
    const ccs_capsule* b,
    ccs_manifold* out
) {
    ccs_vec3 a0;
    ccs_vec3 a1;
    ccs_vec3 b0;
    ccs_vec3 b1;
    ccs_vec3 pA;
    ccs_vec3 pB;
    ccs_vec3 d;
    ccs_fixed dist_sq;
    ccs_fixed r;
    ccs_fixed dist;
    ccs_vec3 n;
    ccs_vec3 pa;
    ccs_vec3 pb;

    if (!a || !b || !out)
        return 0;

    ccs_manifold_clear(out);

    ccs_capsule_endpoints(a, &a0, &a1);
    ccs_capsule_endpoints(b, &b0, &b1);

    dist_sq = ccs_closest_segment_segment(a0, a1, b0, b1, &pA, &pB, 0, 0);

    d = ccs_vec3_sub(pB, pA);
    r = a->radius + b->radius;

    if (dist_sq > ccs_fixed_mul(r, r))
        return 0;

    dist = CCS_FIXED_SQRT(dist_sq);
    if (dist == 0) {
        n = ccs_vec3_axis_x();
    } else {
        n = ccs_vec3_scale(d, ccs_fixed_div(CCS_FIXED_ONE, dist));
    }

    pa = ccs_vec3_add(pA, ccs_vec3_scale(n, a->radius));
    pb = ccs_vec3_sub(pB, ccs_vec3_scale(n, b->radius));

    out->count = 1;
    out->contacts[0].normal = n;
    out->contacts[0].penetration = r - dist;
    out->contacts[0].point = ccs_vec3_scale(ccs_vec3_add(pa, pb), CCS_FIXED_HALF);

    out->contacts[0].feature_id = 0xCACA;

    return 1;
}

/* Capsule vs OBB manifold (1 point) */
int ccs_narrow_capsule_obb_manifold(
    const ccs_capsule* a,
    const ccs_obb* b,
    ccs_manifold* out
) {
    ccs_vec3 a0;
    ccs_vec3 a1;
    ccs_vec3 p0_local;
    ccs_vec3 p1_local;
    ccs_vec3 seg_closest;
    ccs_vec3 box_closest;
    ccs_fixed dist_sq;
    ccs_fixed dist;
    ccs_vec3 n_local;
    ccs_vec3 n_world;
    ccs_vec3 seg_world;
    ccs_vec3 box_world;
    ccs_vec3 pa;

    ccs_vec3 bmin;
    ccs_vec3 bmax;

    if (!a || !b || !out)
        return 0;

    ccs_manifold_clear(out);

    ccs_capsule_endpoints(a, &a0, &a1);

    p0_local = ccs_obb_to_local_point(b, a0);
    p1_local = ccs_obb_to_local_point(b, a1);

    bmin = ccs_vec3_neg(b->half);
    bmax = b->half;

    dist_sq = ccs_closest_segment_aabb(p0_local, p1_local, bmin, bmax, &seg_closest, &box_closest);

    if (dist_sq > ccs_fixed_mul(a->radius, a->radius))
        return 0;

    dist = CCS_FIXED_SQRT(dist_sq);

    if (dist == 0) {
        /* choose nearest face */
        ccs_fixed dxp;
        ccs_fixed dxn;
        ccs_fixed dyp;
        ccs_fixed dyn;
        ccs_fixed dzp;
        ccs_fixed dzn;
        ccs_fixed best;

        dxp = b->half.x - seg_closest.x;
        dxn = b->half.x + seg_closest.x;
        dyp = b->half.y - seg_closest.y;
        dyn = b->half.y + seg_closest.y;
        dzp = b->half.z - seg_closest.z;
        dzn = b->half.z + seg_closest.z;

        best = dxp;
        n_local = ccs_vec3_make(CCS_FIXED_ONE, 0, 0);
        if (dxn < best) { best = dxn; n_local = ccs_vec3_make(-CCS_FIXED_ONE, 0, 0); }
        if (dyp < best) { best = dyp; n_local = ccs_vec3_make(0, CCS_FIXED_ONE, 0); }
        if (dyn < best) { best = dyn; n_local = ccs_vec3_make(0, -CCS_FIXED_ONE, 0); }
        if (dzp < best) { best = dzp; n_local = ccs_vec3_make(0, 0, CCS_FIXED_ONE); }
        if (dzn < best) { best = dzn; n_local = ccs_vec3_make(0, 0, -CCS_FIXED_ONE); }

        out->contacts[0].penetration = a->radius + best;
    } else {
        n_local = ccs_vec3_scale(ccs_vec3_sub(box_closest, seg_closest), ccs_fixed_div(CCS_FIXED_ONE, dist));
        out->contacts[0].penetration = a->radius - dist;
    }

    n_world = ccs_obb_to_world_dir(b, n_local);

    seg_world = ccs_obb_to_world_point(b, seg_closest);
    box_world = ccs_obb_to_world_point(b, box_closest);

    pa = ccs_vec3_add(seg_world, ccs_vec3_scale(n_world, a->radius));

    out->count = 1;
    out->contacts[0].normal = n_world;
    out->contacts[0].point = ccs_vec3_scale(ccs_vec3_add(pa, box_world), CCS_FIXED_HALF);

    out->contacts[0].feature_id = ccs_feature_hash_point_local(box_closest) ^ 0xB0B0;

    return 1;
}

/* Sphere vs OBB manifold (1 point) */
int ccs_narrow_sphere_obb_manifold(
    const ccs_sphere* a,
    const ccs_obb* b,
    ccs_manifold* out
) {
    ccs_contact c;
    ccs_vec3 p_local;
    ccs_vec3 bmin;
    ccs_vec3 bmax;
    ccs_vec3 q_local;
    ccs_vec3 q_world;

    if (!a || !b || !out)
        return 0;

    ccs_manifold_clear(out);

    if (!ccs_narrow_sphere_obb(a, b, &c))
        return 0;

    p_local = ccs_obb_to_local_point(b, a->center);
    bmin = ccs_vec3_neg(b->half);
    bmax = b->half;

    q_local = ccs_clamp_point_aabb(p_local, bmin, bmax);

    /* If inside, push to face like contact computation */
    if (ccs_vec3_len_sq(ccs_vec3_sub(q_local, p_local)) == 0) {
        ccs_fixed dxp;
        ccs_fixed dxn;
        ccs_fixed dyp;
        ccs_fixed dyn;
        ccs_fixed dzp;
        ccs_fixed dzn;
        ccs_fixed best;

        dxp = b->half.x - p_local.x;
        dxn = b->half.x + p_local.x;
        dyp = b->half.y - p_local.y;
        dyn = b->half.y + p_local.y;
        dzp = b->half.z - p_local.z;
        dzn = b->half.z + p_local.z;

        best = dxp;
        q_local = p_local;
        q_local.x = b->half.x;
        if (dxn < best) { best = dxn; q_local = p_local; q_local.x = -b->half.x; }
        if (dyp < best) { best = dyp; q_local = p_local; q_local.y = b->half.y; }
        if (dyn < best) { best = dyn; q_local = p_local; q_local.y = -b->half.y; }
        if (dzp < best) { best = dzp; q_local = p_local; q_local.z = b->half.z; }
        if (dzn < best) { best = dzn; q_local = p_local; q_local.z = -b->half.z; }
    }

    q_world = ccs_obb_to_world_point(b, q_local);

    out->count = 1;
    out->contacts[0].normal = c.normal;
    out->contacts[0].penetration = c.penetration;

    /* point midway between sphere surface and box point */
    {
        ccs_vec3 pa;
        pa = ccs_vec3_add(a->center, ccs_vec3_scale(c.normal, a->radius));
        out->contacts[0].point = ccs_vec3_scale(ccs_vec3_add(pa, q_world), CCS_FIXED_HALF);
    }

    out->contacts[0].feature_id = ccs_feature_hash_point_local(q_local) ^ 0x5A5A;

    return 1;
}
