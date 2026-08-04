/* ============================================================
 * SICOL - GJK / EPA / Convex shape cast
 * ============================================================ */

#include "sicol_gjk.h"

#define SICOL_GJK_DIST_TOL       FX_FROM_RATIO(1, 1024)
#define SICOL_GJK_DOT_TOL        FX_FROM_RATIO(1, 4096)
#define SICOL_GJK_CAST_TOL       FX_FROM_RATIO(1, 1024)
#define SICOL_GJK_CAST_MAX_ITERS 24
#define SICOL_GJK_CAST_BISECT    16
#define SICOL_EPA_TOL            FX_FROM_RATIO(1, 1024)
#define SICOL_EPA_MAX_VERTS      64
#define SICOL_EPA_MAX_FACES      128
#define SICOL_EPA_MAX_EDGES      256

typedef struct {
    fx p[3];
    fx a[3];
    fx b[3];
} gjk_vertex_t;

typedef struct {
    int count;
    gjk_vertex_t v[4];
} gjk_simplex_t;

typedef struct {
    int a;
    int b;
    int c;
    fx normal[3];
    fx dist;
    int active;
} epa_face_t;

void sicol_gjk_cache_reset(sicol_gjk_cache_t* cache)
{
    int i;
    if (!cache) return;
    cache->valid = 0;
    cache->simplex_count = 0;
    fx_zero3(cache->last_dir);
    fx_zero3(cache->last_closest);
    for (i = 0; i < 4; ++i) {
        fx_zero3(cache->p[i]);
        fx_zero3(cache->a[i]);
        fx_zero3(cache->b[i]);
    }
}

static int gjk_cache_to_simplex(const sicol_gjk_cache_t* cache, gjk_simplex_t* simplex)
{
    int i;
    if (!cache || !simplex) return 0;
    if (!cache->valid) return 0;
    if (cache->simplex_count < 1 || cache->simplex_count > 4) return 0;
    simplex->count = cache->simplex_count;
    for (i = 0; i < cache->simplex_count; ++i) {
        fx_copy3(simplex->v[i].p, cache->p[i]);
        fx_copy3(simplex->v[i].a, cache->a[i]);
        fx_copy3(simplex->v[i].b, cache->b[i]);
    }
    return 1;
}

static void gjk_cache_store(sicol_gjk_cache_t* cache, const gjk_simplex_t* simplex, const fx dir[3], const fx closest[3])
{
    int i;
    if (!cache || !simplex) return;
    cache->valid = 1;
    cache->simplex_count = simplex->count;
    fx_copy3(cache->last_dir, dir);
    fx_copy3(cache->last_closest, closest);
    for (i = 0; i < simplex->count && i < 4; ++i) {
        fx_copy3(cache->p[i], simplex->v[i].p);
        fx_copy3(cache->a[i], simplex->v[i].a);
        fx_copy3(cache->b[i], simplex->v[i].b);
    }
    for (; i < 4; ++i) {
        fx_zero3(cache->p[i]);
        fx_zero3(cache->a[i]);
        fx_zero3(cache->b[i]);
    }
}

typedef struct {
    int a;
    int b;
} epa_edge_t;

static fx gjk_div64(int64_t num, int64_t den)
{
    if (den == 0) return 0;
    return (fx)((num << FX_SHIFT) / den);
}

static int gjk_point_equal(const fx a[3], const fx b[3])
{
    if (FX_ABS(a[0] - b[0]) > SICOL_GJK_DIST_TOL) return 0;
    if (FX_ABS(a[1] - b[1]) > SICOL_GJK_DIST_TOL) return 0;
    if (FX_ABS(a[2] - b[2]) > SICOL_GJK_DIST_TOL) return 0;
    return 1;
}

static void gjk_support_vertex(
    const sicol_shape_t* a,
    const sicol_shape_t* b,
    const fx dir[3],
    gjk_vertex_t* out
)
{
    fx neg_dir[3];
    sicol_shape_support_point(a, dir, out->a);
    fx_neg3(neg_dir, dir);
    sicol_shape_support_point(b, neg_dir, out->b);
    fx_sub3(out->p, out->a, out->b);
}

static int gjk_simplex_has_duplicate(const gjk_simplex_t* simplex, const gjk_vertex_t* v)
{
    int i;
    for (i = 0; i < simplex->count; ++i) {
        if (gjk_point_equal(simplex->v[i].p, v->p)) return 1;
    }
    return 0;
}

static int gjk_triangle_barycentric(
    const fx a[3], const fx b[3], const fx c[3], const fx q[3], fx out_bc[3]
)
{
    fx v0[3], v1[3], v2[3];
    fx d00, d01, d11, d20, d21;
    int64_t denom;
    fx v, w;

    fx_sub3(v0, b, a);
    fx_sub3(v1, c, a);
    fx_sub3(v2, q, a);

    d00 = fx_dot3(v0, v0);
    d01 = fx_dot3(v0, v1);
    d11 = fx_dot3(v1, v1);
    d20 = fx_dot3(v2, v0);
    d21 = fx_dot3(v2, v1);

    denom = (int64_t)d00 * (int64_t)d11 - (int64_t)d01 * (int64_t)d01;
    if (denom == 0) {
        out_bc[0] = FX_ONE;
        out_bc[1] = 0;
        out_bc[2] = 0;
        return 0;
    }

    v = gjk_div64((int64_t)d11 * (int64_t)d20 - (int64_t)d01 * (int64_t)d21, denom);
    w = gjk_div64((int64_t)d00 * (int64_t)d21 - (int64_t)d01 * (int64_t)d20, denom);
    out_bc[1] = v;
    out_bc[2] = w;
    out_bc[0] = FX_ONE - v - w;
    return 1;
}

static void gjk_weighted_point(
    fx out[3],
    const fx p0[3], fx w0,
    const fx p1[3], fx w1,
    const fx p2[3], fx w2
)
{
    out[0] = FX_MUL(p0[0], w0) + FX_MUL(p1[0], w1) + FX_MUL(p2[0], w2);
    out[1] = FX_MUL(p0[1], w0) + FX_MUL(p1[1], w1) + FX_MUL(p2[1], w2);
    out[2] = FX_MUL(p0[2], w0) + FX_MUL(p1[2], w1) + FX_MUL(p2[2], w2);
}

static int gjk_reduce_segment(gjk_simplex_t* simplex, fx closest[3])
{
    fx ab[3], ao[3], denom, t;
    fx_sub3(ab, simplex->v[1].p, simplex->v[0].p);
    fx_neg3(ao, simplex->v[0].p);
    denom = fx_len_sq3(ab);
    if (denom <= FX_EPSILON) {
        simplex->count = 1;
        fx_copy3(closest, simplex->v[0].p);
        return (fx_len_sq3(closest) <= SICOL_GJK_DIST_TOL);
    }
    t = FX_DIV(fx_dot3(ao, ab), denom);
    t = fx_clamp(t, 0, FX_ONE);
    fx_madd3(closest, simplex->v[0].p, ab, t);
    if (t <= SICOL_GJK_DIST_TOL) {
        simplex->count = 1;
    } else if (t >= FX_ONE - SICOL_GJK_DIST_TOL) {
        simplex->v[0] = simplex->v[1];
        simplex->count = 1;
    } else {
        simplex->count = 2;
    }
    return (fx_len_sq3(closest) <= SICOL_GJK_DIST_TOL);
}

static int gjk_reduce_triangle(gjk_simplex_t* simplex, fx closest[3])
{
    gjk_vertex_t A = simplex->v[0];
    gjk_vertex_t B = simplex->v[1];
    gjk_vertex_t C = simplex->v[2];
    fx ab[3], ac[3], ap[3], bp[3], cp[3], bc[3];
    fx d1, d2, d3, d4, d5, d6;
    int64_t va, vb, vc;
    fx v, w;

    fx_sub3(ab, B.p, A.p);
    fx_sub3(ac, C.p, A.p);
    fx_neg3(ap, A.p);
    d1 = fx_dot3(ab, ap);
    d2 = fx_dot3(ac, ap);
    if (d1 <= 0 && d2 <= 0) {
        simplex->v[0] = A;
        simplex->count = 1;
        fx_copy3(closest, A.p);
        return (fx_len_sq3(closest) <= SICOL_GJK_DIST_TOL);
    }

    fx_neg3(bp, B.p);
    d3 = fx_dot3(ab, bp);
    d4 = fx_dot3(ac, bp);
    if (d3 >= 0 && d4 <= d3) {
        simplex->v[0] = B;
        simplex->count = 1;
        fx_copy3(closest, B.p);
        return (fx_len_sq3(closest) <= SICOL_GJK_DIST_TOL);
    }

    vc = (int64_t)d1 * (int64_t)d4 - (int64_t)d3 * (int64_t)d2;
    if (vc <= 0 && d1 >= 0 && d3 <= 0) {
        v = FX_DIV(d1, d1 - d3);
        fx_madd3(closest, A.p, ab, v);
        simplex->v[0] = A;
        simplex->v[1] = B;
        simplex->count = 2;
        return (fx_len_sq3(closest) <= SICOL_GJK_DIST_TOL);
    }

    fx_neg3(cp, C.p);
    d5 = fx_dot3(ab, cp);
    d6 = fx_dot3(ac, cp);
    if (d6 >= 0 && d5 <= d6) {
        simplex->v[0] = C;
        simplex->count = 1;
        fx_copy3(closest, C.p);
        return (fx_len_sq3(closest) <= SICOL_GJK_DIST_TOL);
    }

    vb = (int64_t)d5 * (int64_t)d2 - (int64_t)d1 * (int64_t)d6;
    if (vb <= 0 && d2 >= 0 && d6 <= 0) {
        w = FX_DIV(d2, d2 - d6);
        fx_madd3(closest, A.p, ac, w);
        simplex->v[0] = A;
        simplex->v[1] = C;
        simplex->count = 2;
        return (fx_len_sq3(closest) <= SICOL_GJK_DIST_TOL);
    }

    va = (int64_t)d3 * (int64_t)d6 - (int64_t)d5 * (int64_t)d4;
    if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0) {
        fx_sub3(bc, C.p, B.p);
        w = FX_DIV(d4 - d3, (d4 - d3) + (d5 - d6));
        fx_madd3(closest, B.p, bc, w);
        simplex->v[0] = B;
        simplex->v[1] = C;
        simplex->count = 2;
        return (fx_len_sq3(closest) <= SICOL_GJK_DIST_TOL);
    }

    {
        int64_t denom = va + vb + vc;
        fx vv = gjk_div64(vb, denom);
        fx ww = gjk_div64(vc, denom);
        closest[0] = A.p[0] + FX_MUL(ab[0], vv) + FX_MUL(ac[0], ww);
        closest[1] = A.p[1] + FX_MUL(ab[1], vv) + FX_MUL(ac[1], ww);
        closest[2] = A.p[2] + FX_MUL(ab[2], vv) + FX_MUL(ac[2], ww);
        simplex->v[0] = A;
        simplex->v[1] = B;
        simplex->v[2] = C;
        simplex->count = 3;
    }

    return (fx_len_sq3(closest) <= SICOL_GJK_DIST_TOL);
}

static int gjk_origin_outside_face(
    const gjk_vertex_t* A,
    const gjk_vertex_t* B,
    const gjk_vertex_t* C,
    const gjk_vertex_t* D,
    fx out_normal[3]
)
{
    fx ab[3], ac[3], ad[3], ao[3];
    fx_sub3(ab, B->p, A->p);
    fx_sub3(ac, C->p, A->p);
    fx_sub3(ad, D->p, A->p);
    fx_cross3(out_normal, ab, ac);
    if (fx_dot3(out_normal, ad) > 0) {
        fx_neg3(out_normal, out_normal);
    }
    fx_neg3(ao, A->p);
    return (fx_dot3(out_normal, ao) > SICOL_GJK_DIST_TOL);
}

static int gjk_reduce_tetrahedron(gjk_simplex_t* simplex, fx closest[3])
{
    int best_face = -1;
    fx best_dist_sq = 0;
    int face_indices[4][3] = {{0,1,2},{0,3,1},{0,2,3},{1,3,2}};
    int opp_index[4] = {3,2,1,0};
    int i;
    fx n[3];

    for (i = 0; i < 4; ++i) {
        int ia = face_indices[i][0];
        int ib = face_indices[i][1];
        int ic = face_indices[i][2];
        int id = opp_index[i];
        if (gjk_origin_outside_face(&simplex->v[ia], &simplex->v[ib], &simplex->v[ic], &simplex->v[id], n)) {
            gjk_simplex_t face_simplex;
            fx face_closest[3];
            fx dist_sq;
            face_simplex.count = 3;
            face_simplex.v[0] = simplex->v[ia];
            face_simplex.v[1] = simplex->v[ib];
            face_simplex.v[2] = simplex->v[ic];
            gjk_reduce_triangle(&face_simplex, face_closest);
            dist_sq = fx_len_sq3(face_closest);
            if (best_face < 0 || dist_sq < best_dist_sq) {
                best_face = i;
                best_dist_sq = dist_sq;
                *simplex = face_simplex;
                fx_copy3(closest, face_closest);
            }
        }
    }

    if (best_face < 0) {
        fx_zero3(closest);
        return 1;
    }
    return (fx_len_sq3(closest) <= SICOL_GJK_DIST_TOL);
}

static int gjk_reduce_simplex(gjk_simplex_t* simplex, fx closest[3])
{
    switch (simplex->count) {
    case 1:
        fx_copy3(closest, simplex->v[0].p);
        return (fx_len_sq3(closest) <= SICOL_GJK_DIST_TOL);
    case 2:
        return gjk_reduce_segment(simplex, closest);
    case 3:
        return gjk_reduce_triangle(simplex, closest);
    case 4:
        return gjk_reduce_tetrahedron(simplex, closest);
    default:
        break;
    }
    fx_zero3(closest);
    return 0;
}

static int gjk_internal(
    const sicol_shape_t* a,
    const sicol_shape_t* b,
    sicol_gjk_cache_t* cache,
    sicol_gjk_result_t* out,
    gjk_simplex_t* out_simplex
)
{
    gjk_simplex_t simplex;
    gjk_vertex_t w;
    fx center_a[3], center_b[3], dir[3], closest[3], prev_dist_sq;
    int iter;
    int warm_started;

    if (!a || !b) return 0;
    if (!sicol_shape_is_support_mapped(a) || !sicol_shape_is_support_mapped(b)) return 0;

    warm_started = 0;
    simplex.count = 0;

    sicol_shape_center_point(a, center_a);
    sicol_shape_center_point(b, center_b);
    fx_sub3(dir, center_b, center_a);
    if (fx_len_sq3(dir) <= FX_EPSILON) {
        fx_set3(dir, FX_ONE, 0, 0);
    }

    if (cache && gjk_cache_to_simplex(cache, &simplex)) {
        if (gjk_reduce_simplex(&simplex, closest)) {
            if (out) {
                out->intersect = 1;
                out->distance = 0;
                fx_zero3(out->closest);
                fx_zero3(out->normal);
                out->iterations = 0;
            }
            if (out_simplex) *out_simplex = simplex;
            gjk_cache_store(cache, &simplex, dir, closest);
            return 1;
        }
        if (fx_len_sq3(closest) > FX_EPSILON) {
            fx_neg3(dir, closest);
        } else if (fx_len_sq3(cache->last_dir) > FX_EPSILON) {
            fx_copy3(dir, cache->last_dir);
        }
        warm_started = 1;
    } else if (cache && fx_len_sq3(cache->last_dir) > FX_EPSILON) {
        fx_copy3(dir, cache->last_dir);
    }

    if (!warm_started) {
        gjk_support_vertex(a, b, dir, &simplex.v[0]);
        simplex.count = 1;
        fx_copy3(closest, simplex.v[0].p);
    }

    if (fx_len_sq3(closest) <= SICOL_GJK_DIST_TOL) {
        if (out) {
            out->intersect = 1;
            out->distance = 0;
            fx_zero3(out->closest);
            fx_zero3(out->normal);
            out->iterations = 1;
        }
        if (out_simplex) *out_simplex = simplex;
        gjk_cache_store(cache, &simplex, dir, closest);
        return 1;
    }

    prev_dist_sq = fx_len_sq3(closest);
    for (iter = 0; iter < SICOL_GJK_MAX_ITERS; ++iter) {
        fx search_dir[3], support_dot, closest_dot, dist_sq;
        fx_neg3(search_dir, closest);
        gjk_support_vertex(a, b, search_dir, &w);
        if (gjk_simplex_has_duplicate(&simplex, &w)) break;
        support_dot = fx_dot3(w.p, search_dir);
        closest_dot = fx_dot3(closest, search_dir);
        if (support_dot - closest_dot <= SICOL_GJK_DOT_TOL) break;
        if (simplex.count < 4) {
            simplex.v[simplex.count] = w;
            ++simplex.count;
        } else {
            simplex.v[3] = w;
        }
        if (gjk_reduce_simplex(&simplex, closest)) {
            if (out) {
                out->intersect = 1;
                out->distance = 0;
                fx_zero3(out->closest);
                fx_zero3(out->normal);
                out->iterations = iter + 1;
            }
            if (out_simplex) *out_simplex = simplex;
            gjk_cache_store(cache, &simplex, search_dir, closest);
            return 1;
        }
        dist_sq = fx_len_sq3(closest);
        if (prev_dist_sq - dist_sq <= SICOL_GJK_DIST_TOL) {
            fx_copy3(dir, search_dir);
            break;
        }
        prev_dist_sq = dist_sq;
        fx_copy3(dir, search_dir);
    }

    if (out) {
        out->intersect = 0;
        out->distance = fx_len3(closest);
        fx_copy3(out->closest, closest);
        if (!fx_normalize3(out->normal, closest)) {
            fx_zero3(out->normal);
        }
        out->iterations = iter + 1;
    }
    if (out_simplex) *out_simplex = simplex;
    gjk_cache_store(cache, &simplex, dir, closest);
    return 1;
}

static fx gjk_tetra_volume(const gjk_vertex_t* a, const gjk_vertex_t* b, const gjk_vertex_t* c, const gjk_vertex_t* d)
{
    fx ab[3], ac[3], ad[3], n[3];
    fx_sub3(ab, b->p, a->p);
    fx_sub3(ac, c->p, a->p);
    fx_sub3(ad, d->p, a->p);
    fx_cross3(n, ab, ac);
    return FX_ABS(fx_dot3(n, ad));
}

static int gjk_add_unique_vertex(gjk_vertex_t* verts, int* count, const gjk_vertex_t* v)
{
    int i;
    for (i = 0; i < *count; ++i) {
        if (gjk_point_equal(verts[i].p, v->p)) return 0;
    }
    if (*count >= SICOL_EPA_MAX_VERTS) return 0;
    verts[*count] = *v;
    ++(*count);
    return 1;
}

static int epa_seed_from_simplex(
    const sicol_shape_t* a,
    const sicol_shape_t* b,
    const gjk_simplex_t* simplex,
    gjk_vertex_t* verts,
    int* out_count
)
{
    int i;
    int count = 0;
    fx dirs[10][3];
    int dir_count = 0;
    gjk_vertex_t w;

    for (i = 0; i < simplex->count; ++i) {
        gjk_add_unique_vertex(verts, &count, &simplex->v[i]);
    }
    if (count >= 4 && gjk_tetra_volume(&verts[0], &verts[1], &verts[2], &verts[3]) > SICOL_GJK_DIST_TOL) {
        *out_count = 4;
        return 1;
    }

    if (count == 3) {
        fx ab[3], ac[3], n[3];
        fx_sub3(ab, verts[1].p, verts[0].p);
        fx_sub3(ac, verts[2].p, verts[0].p);
        fx_cross3(n, ab, ac);
        if (fx_len_sq3(n) > SICOL_GJK_DIST_TOL) {
            fx_copy3(dirs[dir_count++], n);
            fx_neg3(dirs[dir_count], n);
            ++dir_count;
        }
    } else if (count == 2) {
        fx seg[3], u[3], v[3];
        fx_sub3(seg, verts[1].p, verts[0].p);
        fx_set3(u, FX_ONE, 0, 0);
        fx_cross3(v, seg, u);
        if (fx_len_sq3(v) <= SICOL_GJK_DIST_TOL) {
            fx_set3(u, 0, FX_ONE, 0);
            fx_cross3(v, seg, u);
        }
        if (fx_len_sq3(v) <= SICOL_GJK_DIST_TOL) {
            fx_set3(u, 0, 0, FX_ONE);
            fx_cross3(v, seg, u);
        }
        if (fx_len_sq3(v) > SICOL_GJK_DIST_TOL) {
            fx_copy3(dirs[dir_count++], v);
            fx_neg3(dirs[dir_count], v);
            ++dir_count;
            fx_cross3(u, seg, v);
            fx_copy3(dirs[dir_count++], u);
            fx_neg3(dirs[dir_count], u);
            ++dir_count;
        }
    }

    fx_set3(dirs[dir_count], FX_ONE, 0, 0); ++dir_count;
    fx_set3(dirs[dir_count], -FX_ONE, 0, 0); ++dir_count;
    fx_set3(dirs[dir_count], 0, FX_ONE, 0); ++dir_count;
    fx_set3(dirs[dir_count], 0, -FX_ONE, 0); ++dir_count;
    fx_set3(dirs[dir_count], 0, 0, FX_ONE); ++dir_count;
    fx_set3(dirs[dir_count], 0, 0, -FX_ONE); ++dir_count;

    for (i = 0; i < dir_count && count < 4; ++i) {
        gjk_support_vertex(a, b, dirs[i], &w);
        gjk_add_unique_vertex(verts, &count, &w);
    }

    if (count >= 4) {
        int ia, ib, ic, id;
        for (ia = 0; ia < count - 3; ++ia) {
            for (ib = ia + 1; ib < count - 2; ++ib) {
                for (ic = ib + 1; ic < count - 1; ++ic) {
                    for (id = ic + 1; id < count; ++id) {
                        if (gjk_tetra_volume(&verts[ia], &verts[ib], &verts[ic], &verts[id]) > SICOL_GJK_DIST_TOL) {
                            gjk_vertex_t tetra[4];
                            tetra[0] = verts[ia];
                            tetra[1] = verts[ib];
                            tetra[2] = verts[ic];
                            tetra[3] = verts[id];
                            verts[0] = tetra[0];
                            verts[1] = tetra[1];
                            verts[2] = tetra[2];
                            verts[3] = tetra[3];
                            *out_count = 4;
                            return 1;
                        }
                    }
                }
            }
        }
    }

    *out_count = count;
    return 0;
}

static int epa_make_face(epa_face_t* face, int a, int b, int c, gjk_vertex_t* verts)
{
    fx ab[3], ac[3], n[3], dist;
    face->a = a;
    face->b = b;
    face->c = c;
    face->active = 1;
    fx_sub3(ab, verts[b].p, verts[a].p);
    fx_sub3(ac, verts[c].p, verts[a].p);
    fx_cross3(n, ab, ac);
    if (!fx_normalize3(n, n)) return 0;
    dist = fx_dot3(n, verts[a].p);
    if (dist < 0) {
        int tmp = face->b;
        face->b = face->c;
        face->c = tmp;
        fx_neg3(n, n);
        dist = -dist;
    }
    fx_copy3(face->normal, n);
    face->dist = dist;
    return 1;
}

static void epa_add_edge(epa_edge_t* edges, int* edge_count, int a, int b)
{
    int i;
    for (i = 0; i < *edge_count; ++i) {
        if (edges[i].a == b && edges[i].b == a) {
            edges[i] = edges[*edge_count - 1];
            --(*edge_count);
            return;
        }
    }
    if (*edge_count < SICOL_EPA_MAX_EDGES) {
        edges[*edge_count].a = a;
        edges[*edge_count].b = b;
        ++(*edge_count);
    }
}

static int gjk_penetration_fallback(const sicol_shape_t* a, const sicol_shape_t* b, sicol_epa_result_t* out)
{
    fx amin[3], amax[3], bmin[3], bmax[3];
    fx overlap;
    fx best_overlap = 0;
    fx center_a[3], center_b[3];
    int axis = -1;
    int i;

    sicol_shape_compute_aabb(a, amin, amax);
    sicol_shape_compute_aabb(b, bmin, bmax);
    for (i = 0; i < 3; ++i) {
        if (amax[i] < bmin[i] || amin[i] > bmax[i]) return 0;
        overlap = FX_MIN(amax[i], bmax[i]) - FX_MAX(amin[i], bmin[i]);
        if (axis < 0 || overlap < best_overlap) {
            axis = i;
            best_overlap = overlap;
        }
    }

    sicol_shape_center_point(a, center_a);
    sicol_shape_center_point(b, center_b);
    out->hit = 1;
    out->depth = best_overlap;
    fx_zero3(out->normal);
    fx_zero3(out->point);
    if (axis >= 0) {
        out->normal[axis] = (center_b[axis] >= center_a[axis]) ? FX_ONE : -FX_ONE;
        out->point[0] = (center_a[0] + center_b[0]) / 2;
        out->point[1] = (center_a[1] + center_b[1]) / 2;
        out->point[2] = (center_a[2] + center_b[2]) / 2;
    }
    return 1;
}

int sicol_gjk_distance_query(const sicol_shape_t* a, const sicol_shape_t* b, sicol_gjk_result_t* out)
{
    return gjk_internal(a, b, (sicol_gjk_cache_t*)0, out, 0);
}

int sicol_gjk_distance_cached_query(
    const sicol_shape_t* a,
    const sicol_shape_t* b,
    sicol_gjk_cache_t* cache,
    sicol_gjk_result_t* out
)
{
    return gjk_internal(a, b, cache, out, 0);
}

int sicol_gjk_penetration_query(const sicol_shape_t* a, const sicol_shape_t* b, sicol_epa_result_t* out)
{
    return sicol_gjk_penetration_cached_query(a, b, (sicol_gjk_cache_t*)0, out);
}

int sicol_gjk_penetration_cached_query(
    const sicol_shape_t* a,
    const sicol_shape_t* b,
    sicol_gjk_cache_t* cache,
    sicol_epa_result_t* out
)
{
    sicol_gjk_result_t gjk;
    gjk_simplex_t simplex;
    gjk_vertex_t verts[SICOL_EPA_MAX_VERTS];
    epa_face_t faces[SICOL_EPA_MAX_FACES];
    int vert_count;
    int face_count = 0;
    int i;

    if (!out) return 0;
    out->hit = 0;
    out->depth = 0;
    fx_zero3(out->normal);
    fx_zero3(out->point);
    out->iterations = 0;

    if (!gjk_internal(a, b, cache, &gjk, &simplex)) return 0;
    if (!gjk.intersect) return 0;
    if (!epa_seed_from_simplex(a, b, &simplex, verts, &vert_count)) {
        return gjk_penetration_fallback(a, b, out);
    }

    if (!epa_make_face(&faces[face_count++], 0, 1, 2, verts)) return gjk_penetration_fallback(a, b, out);
    if (!epa_make_face(&faces[face_count++], 0, 3, 1, verts)) return gjk_penetration_fallback(a, b, out);
    if (!epa_make_face(&faces[face_count++], 0, 2, 3, verts)) return gjk_penetration_fallback(a, b, out);
    if (!epa_make_face(&faces[face_count++], 1, 3, 2, verts)) return gjk_penetration_fallback(a, b, out);

    for (i = 0; i < SICOL_EPA_MAX_ITERS; ++i) {
        int best_face = -1;
        fx best_dist = 0;
        gjk_vertex_t support;
        fx support_dist;
        int f;

        for (f = 0; f < face_count; ++f) {
            if (!faces[f].active) continue;
            if (best_face < 0 || faces[f].dist < best_dist) {
                best_face = f;
                best_dist = faces[f].dist;
            }
        }
        if (best_face < 0) return 0;

        gjk_support_vertex(a, b, faces[best_face].normal, &support);
        support_dist = fx_dot3(faces[best_face].normal, support.p);
        if (support_dist - faces[best_face].dist <= SICOL_EPA_TOL) {
            epa_face_t* face = &faces[best_face];
            fx q[3], bc[3], pa[3], pb[3];
            q[0] = FX_MUL(face->normal[0], face->dist);
            q[1] = FX_MUL(face->normal[1], face->dist);
            q[2] = FX_MUL(face->normal[2], face->dist);
            if (!gjk_triangle_barycentric(verts[face->a].p, verts[face->b].p, verts[face->c].p, q, bc)) {
                bc[0] = FX_FROM_RATIO(1, 3);
                bc[1] = FX_FROM_RATIO(1, 3);
                bc[2] = FX_ONE - bc[0] - bc[1];
            }
            gjk_weighted_point(pa, verts[face->a].a, bc[0], verts[face->b].a, bc[1], verts[face->c].a, bc[2]);
            gjk_weighted_point(pb, verts[face->a].b, bc[0], verts[face->b].b, bc[1], verts[face->c].b, bc[2]);
            out->hit = 1;
            out->depth = face->dist;
            fx_copy3(out->normal, face->normal);
            out->point[0] = (pa[0] + pb[0]) / 2;
            out->point[1] = (pa[1] + pb[1]) / 2;
            out->point[2] = (pa[2] + pb[2]) / 2;
            out->iterations = i + 1;
            if (cache) {
                fx_copy3(cache->last_dir, face->normal);
            }
            return 1;
        }

        if (vert_count >= SICOL_EPA_MAX_VERTS) return 0;
        verts[vert_count] = support;

        {
            epa_edge_t edges[SICOL_EPA_MAX_EDGES];
            int edge_count = 0;
            int new_index = vert_count;
            ++vert_count;

            for (f = 0; f < face_count; ++f) {
                if (!faces[f].active) continue;
                if (fx_dot3(faces[f].normal, support.p) - faces[f].dist > SICOL_EPA_TOL) {
                    faces[f].active = 0;
                    epa_add_edge(edges, &edge_count, faces[f].a, faces[f].b);
                    epa_add_edge(edges, &edge_count, faces[f].b, faces[f].c);
                    epa_add_edge(edges, &edge_count, faces[f].c, faces[f].a);
                }
            }

            for (f = 0; f < edge_count; ++f) {
                if (face_count >= SICOL_EPA_MAX_FACES) return 0;
                if (epa_make_face(&faces[face_count], edges[f].a, edges[f].b, new_index, verts)) {
                    ++face_count;
                }
            }
        }
    }

    return gjk_penetration_fallback(a, b, out);
}

static void gjk_shape_at_fraction(sicol_shape_t* out, const sicol_shape_t* in_shape, const fx delta[3], fx fraction)
{
    fx step[3];
    *out = *in_shape;
    fx_scale3(step, delta, fraction);
    sicol_shape_translate(out, step);
}

static int gjk_cast_is_hit_cached(
    const sicol_shape_t* a,
    const sicol_shape_t* b,
    sicol_gjk_cache_t* cache
)
{
    sicol_gjk_result_t gjk;
    if (!sicol_gjk_distance_cached_query(a, b, cache, &gjk)) return 0;
    return (gjk.intersect || gjk.distance <= SICOL_GJK_CAST_TOL);
}

int sicol_shape_cast_exact(
    const sicol_shape_t* moving,
    const fx delta[3],
    const sicol_shape_t* target,
    sicol_shape_cast_result_t* out
)
{
    return sicol_shape_cast_exact_cached(moving, delta, target, (sicol_gjk_cache_t*)0, out);
}

int sicol_shape_cast_exact_cached(
    const sicol_shape_t* moving,
    const fx delta[3],
    const sicol_shape_t* target,
    sicol_gjk_cache_t* cache,
    sicol_shape_cast_result_t* out
)
{
    fx lambda = 0;
    fx safe_lambda = 0;
    int iter;
    sicol_shape_t probe;
    sicol_gjk_result_t gjk;
    sicol_epa_result_t epa;
    sicol_gjk_cache_t local_cache;
    sicol_gjk_cache_t bisect_cache;
    sicol_gjk_cache_t* use_cache;
    sicol_gjk_cache_t* bisect_use;

    if (!moving || !delta || !target || !out) return 0;
    if (!sicol_shape_is_support_mapped(moving) || !sicol_shape_is_support_mapped(target)) return 0;

    use_cache = cache;
    if (!use_cache) {
        sicol_gjk_cache_reset(&local_cache);
        use_cache = &local_cache;
    }
    sicol_gjk_cache_reset(&bisect_cache);
    bisect_use = &bisect_cache;

    out->hit = 0;
    out->fraction = 0;
    out->safe_fraction = 0;
    fx_zero3(out->normal);
    fx_zero3(out->point);
    out->iterations = 0;

    if (sicol_gjk_penetration_cached_query(moving, target, use_cache, &epa)) {
        out->hit = 1;
        out->fraction = 0;
        out->safe_fraction = 0;
        fx_copy3(out->normal, epa.normal);
        fx_copy3(out->point, epa.point);
        return 1;
    }

    for (iter = 0; iter < SICOL_GJK_CAST_MAX_ITERS; ++iter) {
        fx closing;
        fx step;
        gjk_shape_at_fraction(&probe, moving, delta, lambda);
        if (!sicol_gjk_distance_cached_query(&probe, target, use_cache, &gjk)) return 0;
        if (gjk.intersect || gjk.distance <= SICOL_GJK_CAST_TOL) {
            fx lo = safe_lambda;
            fx hi = lambda;
            int bi;
            sicol_gjk_cache_reset(bisect_use);
            for (bi = 0; bi < SICOL_GJK_CAST_BISECT; ++bi) {
                fx mid = (lo + hi) / 2;
                gjk_shape_at_fraction(&probe, moving, delta, mid);
                if (gjk_cast_is_hit_cached(&probe, target, bisect_use)) {
                    hi = mid;
                } else {
                    lo = mid;
                }
            }
            gjk_shape_at_fraction(&probe, moving, delta, hi);
            if (sicol_gjk_penetration_cached_query(&probe, target, bisect_use, &epa)) {
                fx_copy3(out->normal, epa.normal);
                fx_copy3(out->point, epa.point);
            } else if (sicol_gjk_distance_cached_query(&probe, target, bisect_use, &gjk)) {
                fx_copy3(out->normal, gjk.normal);
                fx_neg3(out->normal, out->normal);
                fx_copy3(out->point, probe.pos);
            } else {
                fx_zero3(out->normal);
                fx_copy3(out->point, probe.pos);
            }
            out->hit = 1;
            out->fraction = hi;
            out->safe_fraction = lo;
            out->iterations = iter + 1;
            return 1;
        }

        closing = -fx_dot3(gjk.normal, delta);
        if (closing <= FX_EPSILON) return 0;
        step = FX_DIV(gjk.distance, closing);
        if (step <= 0) step = FX_EPSILON;
        safe_lambda = lambda;
        lambda += step;
        if (lambda > FX_ONE) return 0;
    }

    return 0;
}

int sicol_gjk_raycast_query(
    const sicol_shape_t* ray,
    const sicol_shape_t* target,
    sicol_gjk_cache_t* cache,
    sicol_gjk_raycast_result_t* out
)
{
    sicol_shape_t point_shape;
    sicol_shape_cast_result_t cast;
    fx delta[3];

    if (!ray || !target || !out) return 0;
    if (ray->type != SICOL_SHAPE_RAY) return 0;
    if (!sicol_shape_is_support_mapped(target)) return 0;

    sicol_shape_make_sphere(&point_shape, ray->pos, 0);
    fx_scale3(delta, ray->u.ray.dir, ray->u.ray.length);

    out->hit = 0;
    out->fraction = 0;
    fx_zero3(out->point);
    fx_zero3(out->normal);
    out->iterations = 0;

    if (!sicol_shape_cast_exact_cached(&point_shape, delta, target, cache, &cast)) {
        return 0;
    }

    out->hit = cast.hit;
    out->fraction = cast.fraction;
    fx_madd3(out->point, ray->pos, ray->u.ray.dir, FX_MUL(ray->u.ray.length, cast.fraction));
    fx_copy3(out->normal, cast.normal);
    out->iterations = cast.iterations;
    return cast.hit;
}
