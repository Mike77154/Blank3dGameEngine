#include "ccs_sat.h"

/* ============================================================
   Configuration
   ============================================================ */

#ifndef CCS_SAT_CROSS_EPS
/* Si |Ai x Bj|^2 es muy pequeño, evitamos usarlo como eje candidato. */
#define CCS_SAT_CROSS_EPS ((ccs_fixed)(CCS_FIXED_ONE / 4096))
#endif

#ifndef CCS_SAT_PLANE_SLOP
/* tolerancia al filtrar puntos detrás del plano de referencia */
#define CCS_SAT_PLANE_SLOP ((ccs_fixed)(CCS_FIXED_ONE / 256))
#endif

/* ============================================================
   Small helpers
   ============================================================ */

static ccs_fixed ccs_abs(ccs_fixed v)
{
    return ccs_fixed_abs(v);
}

static ccs_fixed obb_half_i(const ccs_obb* o, int i)
{
    if (i == 0) return o->half.x;
    if (i == 1) return o->half.y;
    return o->half.z;
}

static ccs_vec3 obb_axis_i(const ccs_obb* o, int i)
{
    return o->axis[i];
}

static int face_id_from_axis(int axis_index, int sign)
{
    /* mapping: axis 0 -> {+0,-1}, axis 1 -> {+2,-3}, axis 2 -> {+4,-5} */
    if (sign >= 0)
        return axis_index * 2;
    else
        return axis_index * 2 + 1;
}

static int hash_local_point(ccs_vec3 p_local)
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

    /* pack into 30 bits (10 per axis) */
    return (qx & 1023) | ((qy & 1023) << 10) | ((qz & 1023) << 20);
}

/* ============================================================
   SAT result
   ============================================================ */

typedef struct {
    ccs_vec3  normal;      /* unit normal A->B */
    ccs_fixed penetration; /* >=0 */

    int axis_type;         /* 0: face on A, 1: face on B, 2: cross */
    int axis_index;        /* face axis index (0..2) */
    int cross_i;
    int cross_j;
} sat_result;

/* ============================================================
   SAT core: find best axis and penetration
   ============================================================ */

static int sat_obb_obb_best_axis(const ccs_obb* a, const ccs_obb* b, sat_result* out)
{
    ccs_vec3 d;
    ccs_fixed best_pen;
    ccs_vec3 best_n;
    int best_type;
    int best_axis;
    int best_ci;
    int best_cj;

    int i;
    int j;

    if (!a || !b || !out)
        return 0;

    d = ccs_vec3_sub(b->center, a->center);

    best_pen = (ccs_fixed)CCS_I32_MAX;
    best_n = ccs_vec3_axis_x();
    best_type = 0;
    best_axis = 0;
    best_ci = 0;
    best_cj = 0;

    /* --------------------------------------------------------
       Face axes of A
       -------------------------------------------------------- */
    for (i = 0; i < 3; ++i) {
        ccs_vec3 axis;
        ccs_fixed dist;
        ccs_fixed ra;
        ccs_fixed rb;
        ccs_fixed overlap;
        ccs_fixed sign;

        axis = obb_axis_i(a, i);
        dist = ccs_abs(ccs_vec3_dot(d, axis));

        ra = obb_half_i(a, i);

        /* rb = sum( b.half[j] * |dot(axis, b.axis[j])| ) */
        rb = 0;
        for (j = 0; j < 3; ++j) {
            ccs_fixed proj;
            proj = ccs_abs(ccs_vec3_dot(axis, obb_axis_i(b, j)));
            rb += ccs_fixed_mul(obb_half_i(b, j), proj);
        }

        overlap = ra + rb - dist;
        if (overlap <= 0)
            return 0;

        if (overlap < best_pen) {
            best_pen = overlap;

            sign = ccs_vec3_dot(d, axis);
            best_n = (sign >= 0) ? axis : ccs_vec3_neg(axis);

            best_type = 0;
            best_axis = i;
        }
    }

    /* --------------------------------------------------------
       Face axes of B
       -------------------------------------------------------- */
    for (j = 0; j < 3; ++j) {
        ccs_vec3 axis;
        ccs_fixed dist;
        ccs_fixed ra;
        ccs_fixed rb;
        ccs_fixed overlap;
        ccs_fixed sign;

        axis = obb_axis_i(b, j);
        dist = ccs_abs(ccs_vec3_dot(d, axis));

        rb = obb_half_i(b, j);

        ra = 0;
        for (i = 0; i < 3; ++i) {
            ccs_fixed proj;
            proj = ccs_abs(ccs_vec3_dot(axis, obb_axis_i(a, i)));
            ra += ccs_fixed_mul(obb_half_i(a, i), proj);
        }

        overlap = ra + rb - dist;
        if (overlap <= 0)
            return 0;

        if (overlap < best_pen) {
            best_pen = overlap;

            sign = ccs_vec3_dot(d, axis);
            best_n = (sign >= 0) ? axis : ccs_vec3_neg(axis);

            best_type = 1;
            best_axis = j;
        }
    }

    /* --------------------------------------------------------
       Cross axes Ai x Bj
       -------------------------------------------------------- */
    {
        ccs_fixed eps;
        ccs_fixed eps2;

        int k;

        eps = CCS_SAT_CROSS_EPS;
        eps2 = ccs_fixed_mul(eps, eps);

        for (i = 0; i < 3; ++i) {
            for (j = 0; j < 3; ++j) {
                ccs_vec3 axis;
                ccs_fixed len_sq;
                ccs_fixed len;

                ccs_fixed dist_un;
                ccs_fixed ra_un;
                ccs_fixed rb_un;
                ccs_fixed overlap_un;
                ccs_fixed overlap;

                axis = ccs_vec3_cross(obb_axis_i(a, i), obb_axis_i(b, j));
                len_sq = ccs_vec3_len_sq(axis);

                if (len_sq <= eps2)
                    continue;

                len = CCS_FIXED_SQRT(len_sq);
                if (len == 0)
                    continue;

                /* unnormalized projections (scaled by |axis|) */
                dist_un = ccs_abs(ccs_vec3_dot(d, axis));

                ra_un = 0;
                for (k = 0; k < 3; ++k) {
                    ccs_fixed proj;
                    proj = ccs_abs(ccs_vec3_dot(axis, obb_axis_i(a, k)));
                    ra_un += ccs_fixed_mul(obb_half_i(a, k), proj);
                }

                rb_un = 0;
                for (k = 0; k < 3; ++k) {
                    ccs_fixed proj;
                    proj = ccs_abs(ccs_vec3_dot(axis, obb_axis_i(b, k)));
                    rb_un += ccs_fixed_mul(obb_half_i(b, k), proj);
                }

                overlap_un = ra_un + rb_un - dist_un;
                if (overlap_un <= 0)
                    return 0;

                overlap = ccs_fixed_div(overlap_un, len);

                if (overlap < best_pen) {
                    ccs_vec3 n;
                    ccs_fixed sign;

                    n = axis;
                    ccs_vec3_normalize_safe(&n);

                    sign = ccs_vec3_dot(d, n);
                    best_n = (sign >= 0) ? n : ccs_vec3_neg(n);

                    best_pen = overlap;
                    best_type = 2;
                    best_ci = i;
                    best_cj = j;
                }
            }
        }
    }

    out->normal = best_n;
    out->penetration = best_pen;
    out->axis_type = best_type;
    out->axis_index = best_axis;
    out->cross_i = best_ci;
    out->cross_j = best_cj;

    return 1;
}

/* ============================================================
   Polygon clipping against plane: dot(n, v) <= offset
   ============================================================ */

static int clip_poly_plane(
    const ccs_vec3* in_pts,
    int in_count,
    ccs_vec3 n,
    ccs_fixed offset,
    ccs_vec3* out_pts,
    int out_cap
) {
    int out_count;
    int i;
    ccs_vec3 prev;
    ccs_fixed prev_dist;
    int prev_in;

    if (!in_pts || in_count <= 0 || !out_pts || out_cap <= 0)
        return 0;

    out_count = 0;

    prev = in_pts[in_count - 1];
    prev_dist = offset - ccs_vec3_dot(n, prev);
    prev_in = (prev_dist >= 0);

    for (i = 0; i < in_count; ++i) {
        ccs_vec3 curr;
        ccs_fixed curr_dist;
        int curr_in;

        curr = in_pts[i];
        curr_dist = offset - ccs_vec3_dot(n, curr);
        curr_in = (curr_dist >= 0);

        if (curr_in) {
            if (!prev_in) {
                /* entering: add intersection */
                ccs_vec3 dir;
                ccs_fixed t;

                dir = ccs_vec3_sub(curr, prev);
                /* t = prev_dist / (prev_dist - curr_dist) */
                t = ccs_fixed_div(prev_dist, (prev_dist - curr_dist));

                if (out_count < out_cap)
                    out_pts[out_count++] = ccs_vec3_add(prev, ccs_vec3_scale(dir, t));
            }

            /* add curr */
            if (out_count < out_cap)
                out_pts[out_count++] = curr;
        } else {
            if (prev_in) {
                /* leaving: add intersection */
                ccs_vec3 dir;
                ccs_fixed t;

                dir = ccs_vec3_sub(curr, prev);
                t = ccs_fixed_div(prev_dist, (prev_dist - curr_dist));

                if (out_count < out_cap)
                    out_pts[out_count++] = ccs_vec3_add(prev, ccs_vec3_scale(dir, t));
            }
        }

        prev = curr;
        prev_dist = curr_dist;
        prev_in = curr_in;
    }

    return out_count;
}

/* ============================================================
   Face vertices for OBB
   ============================================================ */

static void obb_face_vertices(const ccs_obb* o, int axis_index, int sign, ccs_vec3 out4[4])
{
    int u;
    int v;
    ccs_fixed hu;
    ccs_fixed hv;
    ccs_fixed ha;

    ccs_vec3 ax;
    ccs_vec3 au;
    ccs_vec3 av;
    ccs_vec3 fc;

    if (!o || !out4)
        return;

    /* choose other axes */
    if (axis_index == 0) { u = 1; v = 2; }
    else if (axis_index == 1) { u = 0; v = 2; }
    else { u = 0; v = 1; }

    ha = obb_half_i(o, axis_index);
    hu = obb_half_i(o, u);
    hv = obb_half_i(o, v);

    ax = obb_axis_i(o, axis_index);
    au = obb_axis_i(o, u);
    av = obb_axis_i(o, v);

    fc = o->center;
    fc = ccs_vec3_add(fc, ccs_vec3_scale(ax, (sign >= 0) ? ha : -ha));

    au = ccs_vec3_scale(au, hu);
    av = ccs_vec3_scale(av, hv);

    /* consistent winding */
    out4[0] = ccs_vec3_add(ccs_vec3_add(fc, au), av);
    out4[1] = ccs_vec3_add(ccs_vec3_sub(fc, au), av);
    out4[2] = ccs_vec3_sub(ccs_vec3_sub(fc, au), av);
    out4[3] = ccs_vec3_sub(ccs_vec3_add(fc, au), av);
}

/* ============================================================
   Reduce contact candidates to <=4 with stable extremes
   ============================================================ */

typedef struct {
    ccs_vec3 p;
    ccs_fixed depth;
} contact_candidate;

static int add_unique_index(int* idxs, int count, int idx)
{
    int i;
    for (i = 0; i < count; ++i)
        if (idxs[i] == idx)
            return count;
    idxs[count] = idx;
    return count + 1;
}

static int reduce_contacts_stable(
    const contact_candidate* cands,
    int cand_count,
    ccs_vec3 u,
    ccs_vec3 v,
    contact_candidate* out,
    int out_cap
) {
    int min_u;
    int max_u;
    int min_v;
    int max_v;
    int i;

    ccs_fixed min_u_val;
    ccs_fixed max_u_val;
    ccs_fixed min_v_val;
    ccs_fixed max_v_val;

    int chosen[8];
    int chosen_count;

    if (!cands || cand_count <= 0 || !out || out_cap <= 0)
        return 0;

    if (cand_count <= out_cap) {
        for (i = 0; i < cand_count; ++i)
            out[i] = cands[i];
        return cand_count;
    }

    min_u = 0; max_u = 0; min_v = 0; max_v = 0;
    min_u_val = ccs_vec3_dot(cands[0].p, u);
    max_u_val = min_u_val;
    min_v_val = ccs_vec3_dot(cands[0].p, v);
    max_v_val = min_v_val;

    for (i = 1; i < cand_count; ++i) {
        ccs_fixed du;
        ccs_fixed dv;
        du = ccs_vec3_dot(cands[i].p, u);
        dv = ccs_vec3_dot(cands[i].p, v);

        if (du < min_u_val) { min_u_val = du; min_u = i; }
        if (du > max_u_val) { max_u_val = du; max_u = i; }
        if (dv < min_v_val) { min_v_val = dv; min_v = i; }
        if (dv > max_v_val) { max_v_val = dv; max_v = i; }
    }

    chosen_count = 0;
    chosen_count = add_unique_index(chosen, chosen_count, min_u);
    chosen_count = add_unique_index(chosen, chosen_count, max_u);
    chosen_count = add_unique_index(chosen, chosen_count, min_v);
    chosen_count = add_unique_index(chosen, chosen_count, max_v);

    /* if still less than out_cap, fill by deepest penetration */
    while (chosen_count < out_cap) {
        int best_i;
        ccs_fixed best_depth;

        best_i = -1;
        best_depth = -1;

        for (i = 0; i < cand_count; ++i) {
            int already;
            int k;
            already = 0;
            for (k = 0; k < chosen_count; ++k)
                if (chosen[k] == i) { already = 1; break; }
            if (already)
                continue;

            if (cands[i].depth > best_depth) {
                best_depth = cands[i].depth;
                best_i = i;
            }
        }

        if (best_i < 0)
            break;

        chosen[chosen_count++] = best_i;
    }

    if (chosen_count > out_cap)
        chosen_count = out_cap;

    for (i = 0; i < chosen_count; ++i) {
        out[i] = cands[chosen[i]];
    }

    return chosen_count;
}

/* ============================================================
   Build face manifold (reference -> incident)
   normal_ref points from reference to incident
   ============================================================ */

static int build_face_manifold(
    const ccs_obb* ref,
    const ccs_obb* inc,
    ccs_vec3 normal_ref,
    int ref_axis_index,
    ccs_manifold* out
) {
    int ref_sign;
    int ref_face;

    int u;
    int v;

    ccs_vec3 ref_axis;
    ccs_fixed ref_plane_offset;

    /* side planes */
    ccs_vec3 plane_n[4];
    ccs_fixed plane_off[4];

    ccs_vec3 inc_face_verts[4];
    int inc_axis_index;
    int inc_sign;

    ccs_vec3 poly1[8];
    ccs_vec3 poly2[8];
    int poly_count;

    contact_candidate cands[8];
    int cand_count;

    contact_candidate reduced[CCS_MAX_CONTACTS];
    int red_count;

    int i;

    if (!ref || !inc || !out)
        return 0;

    /* determine sign of reference face (+/-) */
    ref_axis = obb_axis_i(ref, ref_axis_index);
    ref_sign = (ccs_vec3_dot(normal_ref, ref_axis) >= 0) ? 1 : -1;
    ref_face = face_id_from_axis(ref_axis_index, ref_sign);

    /* choose in-plane axes */
    if (ref_axis_index == 0) { u = 1; v = 2; }
    else if (ref_axis_index == 1) { u = 0; v = 2; }
    else { u = 0; v = 1; }

    /* reference plane offset: dot(n, ref_center + n * half_i) */
    {
        ccs_vec3 p;
        ccs_fixed ha;

        ha = obb_half_i(ref, ref_axis_index);
        p = ccs_vec3_add(ref->center, ccs_vec3_scale(ref_axis, (ref_sign >= 0) ? ha : -ha));
        ref_plane_offset = ccs_vec3_dot(normal_ref, p);
    }

    /* build side planes: dot(n, x) <= offset */
    plane_n[0] = obb_axis_i(ref, u);
    plane_off[0] = ccs_vec3_dot(plane_n[0], ref->center) + obb_half_i(ref, u);

    plane_n[1] = ccs_vec3_neg(obb_axis_i(ref, u));
    plane_off[1] = ccs_vec3_dot(plane_n[1], ref->center) + obb_half_i(ref, u);

    plane_n[2] = obb_axis_i(ref, v);
    plane_off[2] = ccs_vec3_dot(plane_n[2], ref->center) + obb_half_i(ref, v);

    plane_n[3] = ccs_vec3_neg(obb_axis_i(ref, v));
    plane_off[3] = ccs_vec3_dot(plane_n[3], ref->center) + obb_half_i(ref, v);

    /* incident face: choose axis with max abs dot */
    {
        ccs_fixed best_abs;
        int best_i;
        ccs_fixed best_dot;

        best_abs = -1;
        best_i = 0;
        best_dot = 0;

        for (i = 0; i < 3; ++i) {
            ccs_fixed d;
            ccs_fixed ad;
            d = ccs_vec3_dot(normal_ref, obb_axis_i(inc, i));
            ad = ccs_abs(d);
            if (ad > best_abs) {
                best_abs = ad;
                best_i = i;
                best_dot = d;
            }
        }

        inc_axis_index = best_i;
        /* pick face normal opposite normal_ref */
        inc_sign = (best_dot > 0) ? -1 : 1;
    }

    obb_face_vertices(inc, inc_axis_index, inc_sign, inc_face_verts);

    /* start polygon */
    for (i = 0; i < 4; ++i)
        poly1[i] = inc_face_verts[i];
    poly_count = 4;

    /* clip against 4 side planes */
    for (i = 0; i < 4; ++i) {
        poly_count = clip_poly_plane(poly1, poly_count, plane_n[i], plane_off[i], poly2, 8);
        if (poly_count <= 0)
            break;
        /* swap */
        {
            int k;
            for (k = 0; k < poly_count; ++k)
                poly1[k] = poly2[k];
        }
    }

    if (poly_count <= 0)
        return 0;

    /* build candidates behind reference plane */
    cand_count = 0;
    for (i = 0; i < poly_count && cand_count < 8; ++i) {
        ccs_fixed depth;
        depth = ref_plane_offset - ccs_vec3_dot(normal_ref, poly1[i]);
        if (depth >= -CCS_SAT_PLANE_SLOP) {
            if (depth < 0) depth = 0;
            cands[cand_count].depth = depth;
            /* project to reference plane */
            cands[cand_count].p = ccs_vec3_add(poly1[i], ccs_vec3_scale(normal_ref, depth));
            cand_count++;
        }
    }

    if (cand_count <= 0)
        return 0;

    /* reduce to <=4 */
    red_count = reduce_contacts_stable(cands, cand_count, obb_axis_i(ref, u), obb_axis_i(ref, v), reduced, CCS_MAX_CONTACTS);

    if (red_count <= 0)
        return 0;

    out->count = red_count;
    for (i = 0; i < red_count; ++i) {
        ccs_vec3 p_local;
        int h;

        out->contacts[i].point = reduced[i].p;
        out->contacts[i].normal = normal_ref;
        out->contacts[i].penetration = reduced[i].depth;

        /* stable feature id: reference face + quantized local point */
        p_local = ccs_obb_to_local_point(ref, reduced[i].p);
        h = hash_local_point(p_local);
        out->contacts[i].feature_id = ((ref_face & 15) << 28) ^ (h & 0x0FFFFFFF);

        out->contacts[i].normal_impulse = 0;
        out->contacts[i].tangent_impulse1 = 0;
        out->contacts[i].tangent_impulse2 = 0;
    }

    return 1;
}

/* ============================================================
   Edge-edge contact for cross axis
   ============================================================ */

static ccs_fixed sign_from_dot(ccs_vec3 dir, ccs_vec3 axis)
{
    return (ccs_vec3_dot(dir, axis) >= 0) ? CCS_FIXED_ONE : -CCS_FIXED_ONE;
}

static void edge_endpoints_for_axis(const ccs_obb* o, int axis_index, ccs_vec3 n_dir, ccs_vec3* out_p, ccs_vec3* out_q)
{
    int u;
    int v;
    ccs_fixed su;
    ccs_fixed sv;

    ccs_vec3 base;
    ccs_fixed ha;

    if (!o || !out_p || !out_q)
        return;

    if (axis_index == 0) { u = 1; v = 2; }
    else if (axis_index == 1) { u = 0; v = 2; }
    else { u = 0; v = 1; }

    su = sign_from_dot(n_dir, obb_axis_i(o, u));
    sv = sign_from_dot(n_dir, obb_axis_i(o, v));

    base = o->center;
    base = ccs_vec3_add(base, ccs_vec3_scale(obb_axis_i(o, u), (su >= 0) ? obb_half_i(o, u) : -obb_half_i(o, u)));
    base = ccs_vec3_add(base, ccs_vec3_scale(obb_axis_i(o, v), (sv >= 0) ? obb_half_i(o, v) : -obb_half_i(o, v)));

    ha = obb_half_i(o, axis_index);

    *out_p = ccs_vec3_sub(base, ccs_vec3_scale(obb_axis_i(o, axis_index), ha));
    *out_q = ccs_vec3_add(base, ccs_vec3_scale(obb_axis_i(o, axis_index), ha));
}

static ccs_fixed closest_segment_segment(
    ccs_vec3 p1, ccs_vec3 q1,
    ccs_vec3 p2, ccs_vec3 q2,
    ccs_vec3* c1, ccs_vec3* c2
) {
    /* reuse algorithm from narrow.c, simplified here */
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

    a = ccs_vec3_dot(d1, d1);
    e = ccs_vec3_dot(d2, d2);
    f = ccs_vec3_dot(d2, r);

    s = 0;
    t = 0;

    if (a <= 0 && e <= 0) {
        if (c1) *c1 = p1;
        if (c2) *c2 = p2;
        return ccs_vec3_len_sq(ccs_vec3_sub(p1, p2));
    }

    if (a <= 0) {
        s = 0;
        t = ccs_fixed_div(f, e);
        if (t < 0) t = 0;
        if (t > CCS_FIXED_ONE) t = CCS_FIXED_ONE;
    } else {
        c = ccs_vec3_dot(d1, r);
        if (e <= 0) {
            t = 0;
            s = ccs_fixed_div(-c, a);
            if (s < 0) s = 0;
            if (s > CCS_FIXED_ONE) s = CCS_FIXED_ONE;
        } else {
            b = ccs_vec3_dot(d1, d2);
            denom = ccs_fixed_mul(a, e) - ccs_fixed_mul(b, b);
            if (denom != 0)
                s = ccs_fixed_div((ccs_fixed_mul(b, f) - ccs_fixed_mul(c, e)), denom);
            else
                s = 0;
            if (s < 0) s = 0;
            if (s > CCS_FIXED_ONE) s = CCS_FIXED_ONE;

            t = ccs_fixed_div((ccs_fixed_mul(b, s) + f), e);
            if (t < 0) {
                t = 0;
                s = ccs_fixed_div(-c, a);
                if (s < 0) s = 0;
                if (s > CCS_FIXED_ONE) s = CCS_FIXED_ONE;
            } else if (t > CCS_FIXED_ONE) {
                t = CCS_FIXED_ONE;
                s = ccs_fixed_div((b - c), a);
                if (s < 0) s = 0;
                if (s > CCS_FIXED_ONE) s = CCS_FIXED_ONE;
            }
        }
    }

    if (c1) *c1 = ccs_vec3_add(p1, ccs_vec3_scale(d1, s));
    if (c2) *c2 = ccs_vec3_add(p2, ccs_vec3_scale(d2, t));

    return ccs_vec3_len_sq(ccs_vec3_sub(
        ccs_vec3_add(p1, ccs_vec3_scale(d1, s)),
        ccs_vec3_add(p2, ccs_vec3_scale(d2, t))
    ));
}

static int build_edge_manifold(
    const ccs_obb* a,
    const ccs_obb* b,
    ccs_vec3 normal,
    int axis_a,
    int axis_b,
    ccs_fixed penetration,
    ccs_manifold* out
) {
    ccs_vec3 pa0;
    ccs_vec3 pa1;
    ccs_vec3 pb0;
    ccs_vec3 pb1;

    ccs_vec3 ca;
    ccs_vec3 cb;

    ccs_vec3 p;

    if (!a || !b || !out)
        return 0;

    /* Edge on A: support in +normal. Edge on B: support in -normal */
    edge_endpoints_for_axis(a, axis_a, normal, &pa0, &pa1);
    edge_endpoints_for_axis(b, axis_b, ccs_vec3_neg(normal), &pb0, &pb1);

    closest_segment_segment(pa0, pa1, pb0, pb1, &ca, &cb);

    p = ccs_vec3_scale(ccs_vec3_add(ca, cb), CCS_FIXED_HALF);

    out->count = 1;
    out->contacts[0].point = p;
    out->contacts[0].normal = normal;
    out->contacts[0].penetration = penetration;

    /* feature id: hash local point in A */
    {
        ccs_vec3 p_local;
        int h;
        p_local = ccs_obb_to_local_point(a, p);
        h = hash_local_point(p_local);
        out->contacts[0].feature_id = ((axis_a & 3) << 28) ^ (h & 0x0FFFFFFF) ^ 0x0E0E0E0E;
    }

    out->contacts[0].normal_impulse = 0;
    out->contacts[0].tangent_impulse1 = 0;
    out->contacts[0].tangent_impulse2 = 0;

    return 1;
}

/* ============================================================
   Public API
   ============================================================ */

int ccs_sat_obb_obb(
    const ccs_obb* a,
    const ccs_obb* b,
    ccs_contact* out
) {
    sat_result r;
    if (!out)
        return 0;

    if (!sat_obb_obb_best_axis(a, b, &r))
        return 0;

    out->normal = r.normal;
    out->penetration = r.penetration;
    return 1;
}

int ccs_sat_obb_obb_manifold(
    const ccs_obb* a,
    const ccs_obb* b,
    ccs_manifold* out
) {
    sat_result r;
    int hit;

    if (!out)
        return 0;

    ccs_manifold_clear(out);

    hit = sat_obb_obb_best_axis(a, b, &r);
    if (!hit)
        return 0;

    if (r.axis_type == 0) {
        /* reference = A, normal A->B */
        if (!build_face_manifold(a, b, r.normal, r.axis_index, out)) {
            /* fallback single point */
            out->count = 1;
            out->contacts[0].point = ccs_vec3_scale(ccs_vec3_add(a->center, b->center), CCS_FIXED_HALF);
            out->contacts[0].normal = r.normal;
            out->contacts[0].penetration = r.penetration;
            out->contacts[0].feature_id = 0;
        }
        return 1;
    }

    if (r.axis_type == 1) {
        /* reference = B, but manifold must output normal A->B.
           build_face_manifold expects normal_ref = reference->incident.
           Here reference->incident is B->A = - (A->B).
        */
        ccs_vec3 n_ref;
        int i;
        n_ref = ccs_vec3_neg(r.normal);

        if (!build_face_manifold(b, a, n_ref, r.axis_index, out)) {
            out->count = 1;
            out->contacts[0].point = ccs_vec3_scale(ccs_vec3_add(a->center, b->center), CCS_FIXED_HALF);
            out->contacts[0].normal = r.normal;
            out->contacts[0].penetration = r.penetration;
            out->contacts[0].feature_id = 0;
            return 1;
        }

        /* flip normals to A->B */
        for (i = 0; i < out->count; ++i) {
            out->contacts[i].normal = ccs_vec3_neg(out->contacts[i].normal);
        }

        return 1;
    }

    /* cross axis edge-edge */
    return build_edge_manifold(a, b, r.normal, r.cross_i, r.cross_j, r.penetration, out);
}
