#include "ccs_triangle.h"

/* ------------------------------------------------------------
   Internal helpers (copied/adapted from narrow-phase patterns)
   ------------------------------------------------------------ */

static ccs_fixed clamp01(ccs_fixed t)
{
    if (t < 0) return 0;
    if (t > CCS_FIXED_ONE) return CCS_FIXED_ONE;
    return t;
}



/* Returns dist_sq between segments; outputs closest points if requested */
static ccs_fixed closest_segment_segment(
    ccs_vec3 p1, ccs_vec3 q1,
    ccs_vec3 p2, ccs_vec3 q2,
    ccs_vec3* out_c1, ccs_vec3* out_c2)
{
    ccs_vec3 d1;
    ccs_vec3 d2;
    ccs_vec3 r;

    ccs_fixed a;
    ccs_fixed e;
    ccs_fixed f;

    ccs_fixed s;
    ccs_fixed t;

    ccs_fixed c;
    ccs_fixed b;
    ccs_fixed denom;

    d1 = ccs_vec3_sub(q1, p1);
    d2 = ccs_vec3_sub(q2, p2);
    r  = ccs_vec3_sub(p1, p2);

    a = ccs_vec3_dot(d1, d1);
    e = ccs_vec3_dot(d2, d2);
    f = ccs_vec3_dot(d2, r);

    if (a <= 0 && e <= 0) {
        /* both degenerate */
        if (out_c1) *out_c1 = p1;
        if (out_c2) *out_c2 = p2;
        return ccs_vec3_len_sq(ccs_vec3_sub(p1, p2));
    }

    if (a <= 0) {
        /* first degenerate */
        s = 0;
        t = clamp01(ccs_fixed_div(f, e));
    } else {
        c = ccs_vec3_dot(d1, r);
        if (e <= 0) {
            /* second degenerate */
            t = 0;
            s = clamp01(ccs_fixed_div(-c, a));
        } else {
            b = ccs_vec3_dot(d1, d2);
            denom = ccs_fixed_mul(a, e) - ccs_fixed_mul(b, b);

            if (denom != 0)
                s = clamp01(ccs_fixed_div((ccs_fixed_mul(b, f) - ccs_fixed_mul(c, e)), denom));
            else
                s = 0;

            t = (ccs_fixed_mul(b, s) + f);
            if (t < 0) {
                t = 0;
                s = clamp01(ccs_fixed_div(-c, a));
            } else if (t > e) {
                t = CCS_FIXED_ONE;
                s = clamp01(ccs_fixed_div((b - c), a));
            } else {
                t = ccs_fixed_div(t, e);
            }
        }
    }

    {
        ccs_vec3 c1;
        ccs_vec3 c2;
        c1 = ccs_vec3_add(p1, ccs_vec3_scale(d1, s));
        c2 = ccs_vec3_add(p2, ccs_vec3_scale(d2, t));

        if (out_c1) *out_c1 = c1;
        if (out_c2) *out_c2 = c2;

        return ccs_vec3_len_sq(ccs_vec3_sub(c1, c2));
    }
}

static int point_in_triangle(ccs_vec3 p, ccs_vec3 a, ccs_vec3 b, ccs_vec3 c, ccs_vec3 n)
{
    ccs_vec3 ab;
    ccs_vec3 bc;
    ccs_vec3 ca;

    ccs_vec3 ap;
    ccs_vec3 bp;
    ccs_vec3 cp;

    ccs_vec3 c0;
    ccs_vec3 c1;
    ccs_vec3 c2;

    ccs_fixed d0;
    ccs_fixed d1;
    ccs_fixed d2;

    ab = ccs_vec3_sub(b, a);
    bc = ccs_vec3_sub(c, b);
    ca = ccs_vec3_sub(a, c);

    ap = ccs_vec3_sub(p, a);
    bp = ccs_vec3_sub(p, b);
    cp = ccs_vec3_sub(p, c);

    c0 = ccs_vec3_cross(ab, ap);
    c1 = ccs_vec3_cross(bc, bp);
    c2 = ccs_vec3_cross(ca, cp);

    d0 = ccs_vec3_dot(c0, n);
    d1 = ccs_vec3_dot(c1, n);
    d2 = ccs_vec3_dot(c2, n);

    if ((d0 >= 0 && d1 >= 0 && d2 >= 0) || (d0 <= 0 && d1 <= 0 && d2 <= 0))
        return 1;

    return 0;
}

/* ============================================================
   Triangle normal
   ============================================================ */

ccs_vec3 ccs_triangle_normal(const ccs_triangle* t)
{
    ccs_vec3 ab;
    ccs_vec3 ac;
    ccs_vec3 n;

    if (!t)
        return ccs_vec3_axis_y();

    ab = ccs_vec3_sub(t->b, t->a);
    ac = ccs_vec3_sub(t->c, t->a);
    n = ccs_vec3_cross(ab, ac);

    if (!ccs_vec3_normalize_safe(&n))
        n = ccs_vec3_axis_y();

    return n;
}

/* ============================================================
   Closest point on triangle (Ericson)
   ============================================================ */

ccs_vec3 ccs_triangle_closest_point(const ccs_triangle* t, ccs_vec3 p)
{
    ccs_vec3 a;
    ccs_vec3 b;
    ccs_vec3 c;

    ccs_vec3 ab;
    ccs_vec3 ac;
    ccs_vec3 ap;

    ccs_fixed d1;
    ccs_fixed d2;

    ccs_vec3 bp;
    ccs_fixed d3;
    ccs_fixed d4;

    ccs_vec3 cp;
    ccs_fixed d5;
    ccs_fixed d6;

    ccs_fixed vc;
    ccs_fixed vb;
    ccs_fixed va;

    if (!t)
        return p;

    a = t->a;
    b = t->b;
    c = t->c;

    ab = ccs_vec3_sub(b, a);
    ac = ccs_vec3_sub(c, a);
    ap = ccs_vec3_sub(p, a);

    d1 = ccs_vec3_dot(ab, ap);
    d2 = ccs_vec3_dot(ac, ap);
    if (d1 <= 0 && d2 <= 0)
        return a;

    bp = ccs_vec3_sub(p, b);
    d3 = ccs_vec3_dot(ab, bp);
    d4 = ccs_vec3_dot(ac, bp);
    if (d3 >= 0 && d4 <= d3)
        return b;

    vc = ccs_fixed_mul(d1, d4) - ccs_fixed_mul(d3, d2);
    if (vc <= 0 && d1 >= 0 && d3 <= 0) {
        ccs_fixed v;
        v = ccs_fixed_div(d1, (d1 - d3));
        return ccs_vec3_add(a, ccs_vec3_scale(ab, v));
    }

    cp = ccs_vec3_sub(p, c);
    d5 = ccs_vec3_dot(ab, cp);
    d6 = ccs_vec3_dot(ac, cp);
    if (d6 >= 0 && d5 <= d6)
        return c;

    vb = ccs_fixed_mul(d5, d2) - ccs_fixed_mul(d1, d6);
    if (vb <= 0 && d2 >= 0 && d6 <= 0) {
        ccs_fixed w;
        w = ccs_fixed_div(d2, (d2 - d6));
        return ccs_vec3_add(a, ccs_vec3_scale(ac, w));
    }

    va = ccs_fixed_mul(d3, d6) - ccs_fixed_mul(d5, d4);
    if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0) {
        ccs_vec3 bc;
        ccs_fixed w;
        bc = ccs_vec3_sub(c, b);
        w = ccs_fixed_div((d4 - d3), ((d4 - d3) + (d5 - d6)));
        return ccs_vec3_add(b, ccs_vec3_scale(bc, w));
    }

    {
        ccs_fixed denom;
        ccs_fixed v;
        ccs_fixed w;
        denom = va + vb + vc;
        if (denom == 0)
            return a;

        v = ccs_fixed_div(vb, denom);
        w = ccs_fixed_div(vc, denom);
        return ccs_vec3_add(a, ccs_vec3_add(ccs_vec3_scale(ab, v), ccs_vec3_scale(ac, w)));
    }
}

/* ============================================================
   Triangle vs Sphere
   ============================================================ */

int ccs_triangle_sphere_contact(const ccs_triangle* tri, const ccs_sphere* s, ccs_contact* out)
{
    ccs_vec3 closest;
    ccs_vec3 delta;
    ccs_fixed dist_sq;
    ccs_fixed r2;

    if (!tri || !s || !out)
        return 0;

    closest = ccs_triangle_closest_point(tri, s->center);
    delta = ccs_vec3_sub(s->center, closest);
    dist_sq = ccs_vec3_len_sq(delta);

    r2 = ccs_fixed_mul(s->radius, s->radius);
    if (dist_sq > r2)
        return 0;

    if (dist_sq == 0) {
        /* center exactly on triangle: pick triangle normal oriented toward sphere center */
        ccs_vec3 n;
        ccs_fixed side;

        n = ccs_triangle_normal(tri);
        side = ccs_vec3_dot(n, ccs_vec3_sub(s->center, tri->a));
        if (side < 0)
            n = ccs_vec3_neg(n);

        out->normal = n;
        out->penetration = s->radius;
    } else {
        ccs_fixed dist;
        dist = CCS_FIXED_SQRT(dist_sq);

        out->normal = ccs_vec3_scale(delta, ccs_fixed_div(CCS_FIXED_ONE, dist));
        out->penetration = s->radius - dist;
    }

    return 1;
}

/* ============================================================
   Triangle vs Capsule
   ============================================================ */

int ccs_triangle_capsule_contact(const ccs_triangle* tri, const ccs_capsule* c, ccs_contact* out)
{
    ccs_vec3 s0;
    ccs_vec3 s1;
    ccs_vec3 n;
    ccs_fixed d;
    ccs_fixed dist_sq;
    ccs_fixed r2;

    ccs_vec3 closest_seg;
    ccs_vec3 closest_tri;

    if (!tri || !c || !out)
        return 0;

    ccs_capsule_endpoints(c, &s0, &s1);

    /* First: segment-plane intersection check */
    {
        ccs_vec3 ab;
        ccs_vec3 ac;
        ccs_vec3 nn;
        ccs_vec3 dir;
        ccs_fixed denom;
        ccs_fixed dd;
        ccs_fixed t;

        ab = ccs_vec3_sub(tri->b, tri->a);
        ac = ccs_vec3_sub(tri->c, tri->a);
        nn = ccs_vec3_cross(ab, ac);

        dir = ccs_vec3_sub(s1, s0);
        denom = ccs_vec3_dot(nn, dir);
        if (denom != 0) {
            dd = ccs_vec3_dot(nn, tri->a);
            t = ccs_fixed_div((dd - ccs_vec3_dot(nn, s0)), denom);
            if (t >= 0 && t <= CCS_FIXED_ONE) {
                ccs_vec3 p;
                p = ccs_vec3_add(s0, ccs_vec3_scale(dir, t));

                /* need a stable normal for inside test */
                n = nn;
                ccs_vec3_normalize_safe(&n);

                if (point_in_triangle(p, tri->a, tri->b, tri->c, n)) {
                    /* segment intersects triangle => distance 0 */
                    closest_seg = p;
                    closest_tri = p;
                    dist_sq = 0;
                    goto compute_contact;
                }
            }
        }
    }

    /* Otherwise: compute min distance among endpoints and edges */
    dist_sq = (ccs_fixed)CCS_I32_MAX;
    closest_seg = s0;
    closest_tri = tri->a;

    {
        ccs_vec3 cp0;
        ccs_vec3 cp1;
        ccs_fixed ds0;
        ccs_fixed ds1;

        cp0 = ccs_triangle_closest_point(tri, s0);
        ds0 = ccs_vec3_len_sq(ccs_vec3_sub(s0, cp0));
        if (ds0 < dist_sq) {
            dist_sq = ds0;
            closest_seg = s0;
            closest_tri = cp0;
        }

        cp1 = ccs_triangle_closest_point(tri, s1);
        ds1 = ccs_vec3_len_sq(ccs_vec3_sub(s1, cp1));
        if (ds1 < dist_sq) {
            dist_sq = ds1;
            closest_seg = s1;
            closest_tri = cp1;
        }
    }

    /* segment vs triangle edges */
    {
        ccs_vec3 ea0;
        ccs_vec3 ea1;
        ccs_vec3 cA;
        ccs_vec3 cB;
        ccs_fixed ds;

        ea0 = tri->a;
        ea1 = tri->b;
        ds = closest_segment_segment(s0, s1, ea0, ea1, &cA, &cB);
        if (ds < dist_sq) {
            dist_sq = ds;
            closest_seg = cA;
            closest_tri = cB;
        }

        ea0 = tri->b;
        ea1 = tri->c;
        ds = closest_segment_segment(s0, s1, ea0, ea1, &cA, &cB);
        if (ds < dist_sq) {
            dist_sq = ds;
            closest_seg = cA;
            closest_tri = cB;
        }

        ea0 = tri->c;
        ea1 = tri->a;
        ds = closest_segment_segment(s0, s1, ea0, ea1, &cA, &cB);
        if (ds < dist_sq) {
            dist_sq = ds;
            closest_seg = cA;
            closest_tri = cB;
        }
    }

compute_contact:
    r2 = ccs_fixed_mul(c->radius, c->radius);
    if (dist_sq > r2)
        return 0;

    if (dist_sq == 0) {
        n = ccs_triangle_normal(tri);
        /* orient normal toward capsule center */
        d = ccs_vec3_dot(n, ccs_vec3_sub(c->center, tri->a));
        if (d < 0)
            n = ccs_vec3_neg(n);
        out->normal = n;
        out->penetration = c->radius;
    } else {
        ccs_vec3 delta;
        ccs_fixed dist;
        delta = ccs_vec3_sub(closest_seg, closest_tri);
        dist = CCS_FIXED_SQRT(dist_sq);
        out->normal = ccs_vec3_scale(delta, ccs_fixed_div(CCS_FIXED_ONE, dist));
        out->penetration = c->radius - dist;
    }

    return 1;
}

/* ============================================================
   Triangle vs OBB
   ============================================================ */

int ccs_triangle_obb_contact(const ccs_triangle* tri, const ccs_obb* o, ccs_contact* out)
{
    ccs_vec3 v0;
    ccs_vec3 v1;
    ccs_vec3 v2;

    ccs_vec3 e0;
    ccs_vec3 e1;
    ccs_vec3 e2;

    ccs_vec3 axes[13];
    int axis_count;
    int i;

    ccs_fixed best_overlap;
    ccs_vec3 best_axis;

    if (!tri || !o || !out)
        return 0;

    /* Transform triangle into OBB local space => AABB at origin */
    v0 = ccs_obb_to_local_point(o, tri->a);
    v1 = ccs_obb_to_local_point(o, tri->b);
    v2 = ccs_obb_to_local_point(o, tri->c);

    e0 = ccs_vec3_sub(v1, v0);
    e1 = ccs_vec3_sub(v2, v1);
    e2 = ccs_vec3_sub(v0, v2);

    axis_count = 0;

    /* box face normals */
    axes[axis_count++] = ccs_vec3_axis_x();
    axes[axis_count++] = ccs_vec3_axis_y();
    axes[axis_count++] = ccs_vec3_axis_z();

    /* triangle normal */
    {
        ccs_vec3 n;
        n = ccs_vec3_cross(e0, ccs_vec3_sub(v2, v0));
        if (ccs_vec3_len_sq(n) > 0)
            axes[axis_count++] = n;
    }

    /* 9 cross axes (box axis x triangle edge) */
    {
        /* cross(X, e) = (0, -ez, ey) */
        axes[axis_count++] = ccs_vec3_make(0, -e0.z, e0.y);
        axes[axis_count++] = ccs_vec3_make(0, -e1.z, e1.y);
        axes[axis_count++] = ccs_vec3_make(0, -e2.z, e2.y);

        /* cross(Y, e) = (ez, 0, -ex) */
        axes[axis_count++] = ccs_vec3_make(e0.z, 0, -e0.x);
        axes[axis_count++] = ccs_vec3_make(e1.z, 0, -e1.x);
        axes[axis_count++] = ccs_vec3_make(e2.z, 0, -e2.x);

        /* cross(Z, e) = (-ey, ex, 0) */
        axes[axis_count++] = ccs_vec3_make(-e0.y, e0.x, 0);
        axes[axis_count++] = ccs_vec3_make(-e1.y, e1.x, 0);
        axes[axis_count++] = ccs_vec3_make(-e2.y, e2.x, 0);
    }

    best_overlap = (ccs_fixed)CCS_I32_MAX;
    best_axis = ccs_vec3_axis_y();

    for (i = 0; i < axis_count; ++i) {
        ccs_vec3 axis;
        ccs_fixed len_sq;
        ccs_fixed len;
        ccs_fixed inv_len;

        ccs_fixed tri_min;
        ccs_fixed tri_max;
        ccs_fixed p0;
        ccs_fixed p1;
        ccs_fixed p2;

        ccs_fixed r;

        ccs_fixed overlap;

        axis = axes[i];
        len_sq = ccs_vec3_len_sq(axis);
        if (len_sq == 0)
            continue;

        len = CCS_FIXED_SQRT(len_sq);
        if (len == 0)
            continue;
        inv_len = ccs_fixed_div(CCS_FIXED_ONE, len);
        axis = ccs_vec3_scale(axis, inv_len);

        p0 = ccs_vec3_dot(axis, v0);
        p1 = ccs_vec3_dot(axis, v1);
        p2 = ccs_vec3_dot(axis, v2);

        tri_min = p0;
        tri_max = p0;
        if (p1 < tri_min) tri_min = p1;
        if (p1 > tri_max) tri_max = p1;
        if (p2 < tri_min) tri_min = p2;
        if (p2 > tri_max) tri_max = p2;

        /* box interval is [-r, r] */
        r = ccs_fixed_mul(ccs_fixed_abs(axis.x), o->half.x)
          + ccs_fixed_mul(ccs_fixed_abs(axis.y), o->half.y)
          + ccs_fixed_mul(ccs_fixed_abs(axis.z), o->half.z);

        if (tri_min > r || tri_max < -r)
            return 0;

        /* overlap */
        overlap = (tri_max < r ? tri_max : r) - (tri_min > -r ? tri_min : -r);
        if (overlap < best_overlap) {
            best_overlap = overlap;
            best_axis = axis;
        }
    }

    /* Determine direction: want normal from triangle -> box (local box center is 0) */
    {
        ccs_vec3 tri_center;
        ccs_fixed s;

        tri_center = ccs_vec3_scale(ccs_vec3_add(ccs_vec3_add(v0, v1), v2), ccs_fixed_div(CCS_FIXED_ONE, ccs_fixed_from_int(3)));
        s = ccs_vec3_dot(best_axis, tri_center);
        if (s > 0)
            best_axis = ccs_vec3_neg(best_axis);

        /* Convert to world */
        out->normal = ccs_obb_to_world_dir(o, best_axis);
        ccs_vec3_normalize_safe(&out->normal);
        out->penetration = best_overlap;
    }

    return 1;
}

/* ============================================================
   Raycast vs Triangle (plane hit + inside test)
   ============================================================ */

int ccs_raycast_triangle(const ccs_ray* ray, const ccs_triangle* tri, ccs_raycast_hit* out)
{
    ccs_vec3 ab;
    ccs_vec3 ac;
    ccs_vec3 n;
    ccs_fixed denom;
    ccs_fixed d;
    ccs_fixed t;

    if (!ray || !tri || !out)
        return 0;

    out->hit = 0;

    ab = ccs_vec3_sub(tri->b, tri->a);
    ac = ccs_vec3_sub(tri->c, tri->a);
    n = ccs_vec3_cross(ab, ac);

    denom = ccs_vec3_dot(n, ray->dir);
    if (denom == 0)
        return 0;

    d = ccs_vec3_dot(n, tri->a);
    t = ccs_fixed_div((d - ccs_vec3_dot(n, ray->origin)), denom);

    if (t < ray->tmin || t > ray->tmax)
        return 0;

    {
        ccs_vec3 p;
        ccs_vec3 nn;

        p = ccs_vec3_add(ray->origin, ccs_vec3_scale(ray->dir, t));

        nn = n;
        ccs_vec3_normalize_safe(&nn);

        if (!point_in_triangle(p, tri->a, tri->b, tri->c, nn))
            return 0;

        /* orient normal against ray direction (typical) */
        if (ccs_vec3_dot(nn, ray->dir) > 0)
            nn = ccs_vec3_neg(nn);

        out->hit = 1;
        out->t = t;
        out->point = p;
        out->normal = nn;
        out->feature_id = 0;
        return 1;
    }
}
