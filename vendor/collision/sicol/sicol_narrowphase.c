/* ============================================================
 * SICOL - Narrowphase
 * ============================================================ */

#include "sicol_narrowphase.h"
#include "sicol_raycast.h"
#include "sicol_gjk.h"
#include "sicol_mesh.h"

static void np_contact_zero(sicol_contact_t* out)
{
    if (!out) return;
    fx_zero3(out->normal);
    fx_zero3(out->point);
    out->penetration = 0;
    out->feature_a = 0;
    out->feature_b = 0;
    out->count = 0;
}

static void np_fill_contact(
    sicol_contact_t* out,
    const fx normal[3],
    const fx point[3],
    fx penetration
)
{
    if (!out) return;
    fx_copy3(out->normal, normal);
    fx_copy3(out->point, point);
    out->penetration = penetration;
    out->feature_a = 0;
    out->feature_b = 0;
    out->count = 1;
}

static void np_closest_point_aabb(const sicol_shape_t* box, const fx p[3], fx out[3])
{
    int i;
    for (i = 0; i < 3; ++i) {
        fx min_v = box->pos[i] - box->u.aabb.half[i];
        fx max_v = box->pos[i] + box->u.aabb.half[i];
        out[i] = fx_clamp(p[i], min_v, max_v);
    }
}

static void np_closest_point_obb(const sicol_shape_t* box, const fx p[3], fx out[3])
{
    fx d[3];
    fx local[3];
    int i;
    fx_sub3(d, p, box->pos);
    fx_basis_to_local3(local, box->u.obb.axis, d);

    for (i = 0; i < 3; ++i) {
        local[i] = fx_clamp(local[i], -box->u.obb.half[i], box->u.obb.half[i]);
    }

    fx_basis_to_world3(out, box->u.obb.axis, local);
    fx_add3(out, out, box->pos);
}

static void np_closest_point_triangle(
    const fx p[3],
    const fx a[3],
    const fx b[3],
    const fx c[3],
    fx out[3]
)
{
    fx ab[3], ac[3], ap[3], bp[3], cp[3], bc[3];
    fx d1, d2, d3, d4, d5, d6;
    int64_t va, vb, vc;
    fx v, w;

    fx_sub3(ab, b, a);
    fx_sub3(ac, c, a);
    fx_sub3(ap, p, a);
    d1 = fx_dot3(ab, ap);
    d2 = fx_dot3(ac, ap);
    if (d1 <= 0 && d2 <= 0) {
        fx_copy3(out, a);
        return;
    }

    fx_sub3(bp, p, b);
    d3 = fx_dot3(ab, bp);
    d4 = fx_dot3(ac, bp);
    if (d3 >= 0 && d4 <= d3) {
        fx_copy3(out, b);
        return;
    }

    vc = (int64_t)d1 * (int64_t)d4 - (int64_t)d3 * (int64_t)d2;
    if (vc <= 0 && d1 >= 0 && d3 <= 0) {
        v = FX_DIV(d1, d1 - d3);
        fx_madd3(out, a, ab, v);
        return;
    }

    fx_sub3(cp, p, c);
    d5 = fx_dot3(ab, cp);
    d6 = fx_dot3(ac, cp);
    if (d6 >= 0 && d5 <= d6) {
        fx_copy3(out, c);
        return;
    }

    vb = (int64_t)d5 * (int64_t)d2 - (int64_t)d1 * (int64_t)d6;
    if (vb <= 0 && d2 >= 0 && d6 <= 0) {
        w = FX_DIV(d2, d2 - d6);
        fx_madd3(out, a, ac, w);
        return;
    }

    va = (int64_t)d3 * (int64_t)d6 - (int64_t)d5 * (int64_t)d4;
    if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0) {
        fx_sub3(bc, c, b);
        w = FX_DIV(d4 - d3, (d4 - d3) + (d5 - d6));
        fx_madd3(out, b, bc, w);
        return;
    }

    {
        int64_t denom = va + vb + vc;
        fx vv = (denom != 0) ? (fx)((vb << FX_SHIFT) / denom) : 0;
        fx ww = (denom != 0) ? (fx)((vc << FX_SHIFT) / denom) : 0;
        out[0] = a[0] + FX_MUL(ab[0], vv) + FX_MUL(ac[0], ww);
        out[1] = a[1] + FX_MUL(ab[1], vv) + FX_MUL(ac[1], ww);
        out[2] = a[2] + FX_MUL(ab[2], vv) + FX_MUL(ac[2], ww);
    }
}

static void np_swap_shapes(
    const sicol_shape_t** a,
    const sicol_shape_t** b,
    int* swapped
)
{
    const sicol_shape_t* tmp = *a;
    *a = *b;
    *b = tmp;
    *swapped = 1;
}

static void np_swap_contact(sicol_contact_t* out)
{
    int tmp_feature;
    if (!out) return;
    out->normal[0] = -out->normal[0];
    out->normal[1] = -out->normal[1];
    out->normal[2] = -out->normal[2];
    tmp_feature = out->feature_a;
    out->feature_a = out->feature_b;
    out->feature_b = tmp_feature;
}

static void np_mesh_triangle_world_shape(
    const sicol_shape_t* mesh_shape,
    int triangle_index,
    sicol_shape_t* out_triangle
)
{
    fx a[3];
    fx b[3];
    fx c[3];

    if (!mesh_shape || !out_triangle || !mesh_shape->u.mesh.mesh) {
        if (out_triangle) sicol_shape_make_none(out_triangle);
        return;
    }

    if (!sicol_shape_get_mesh_triangle_points(mesh_shape, triangle_index, a, b, c)) {
        sicol_shape_make_none(out_triangle);
        return;
    }

    sicol_shape_make_triangle(out_triangle, a, b, c);
}

static int np_support_pair_exact(
    const sicol_shape_t* a,
    const sicol_shape_t* b,
    sicol_contact_t* out,
    sicol_gjk_cache_t* cache
)
{
    sicol_epa_result_t epa;
    sicol_contact_t local_contact;
    sicol_contact_t* dst = out ? out : &local_contact;

    if (!a || !b) return 0;
    if (!sicol_shape_is_support_mapped(a) || !sicol_shape_is_support_mapped(b)) return 0;

    if (!(cache ? sicol_gjk_penetration_cached_query(a, b, cache, &epa)
                : sicol_gjk_penetration_query(a, b, &epa))) {
        return 0;
    }

    np_fill_contact(dst, epa.normal, epa.point, epa.depth);
    if (a->type == SICOL_SHAPE_TRIANGLE) dst->feature_a = 1;
    if (b->type == SICOL_SHAPE_TRIANGLE) dst->feature_b = 1;
    return 1;
}

static int np_mesh_vs_shape(
    const sicol_shape_t* mesh_shape,
    const sicol_shape_t* other,
    sicol_contact_t* out
)
{
    int candidate_indices[SICOL_MESH_MAX_TRIANGLES];
    fx other_min[3];
    fx other_max[3];
    fx query_min[3];
    fx query_max[3];
    sicol_contact_t best_contact;
    int best_hit = 0;
    int count;
    int i;

    if (!mesh_shape || !other || !mesh_shape->u.mesh.mesh) return 0;

    sicol_shape_compute_aabb(other, other_min, other_max);
    sicol_shape_mesh_world_aabb_to_local_aabb(mesh_shape, other_min, other_max, query_min, query_max);

    count = sicol_mesh_query_local_aabb(mesh_shape->u.mesh.mesh,
                                        query_min,
                                        query_max,
                                        candidate_indices,
                                        SICOL_MESH_MAX_TRIANGLES);
    for (i = 0; i < count; ++i) {
        sicol_shape_t tri_shape;
        sicol_contact_t contact;
        np_mesh_triangle_world_shape(mesh_shape, candidate_indices[i], &tri_shape);
        if (sicol_shape_is_support_mapped(other)) {
            if (!np_support_pair_exact(&tri_shape, other, &contact, (sicol_gjk_cache_t*)0) &&
                !sicol_narrowphase_test(&tri_shape, other, &contact)) {
                continue;
            }
        } else if (!sicol_narrowphase_test(&tri_shape, other, &contact)) {
            continue;
        }
        contact.feature_a = candidate_indices[i] + 1;
        if (!best_hit || contact.penetration > best_contact.penetration) {
            best_contact = contact;
            best_hit = 1;
        }
    }

    if (best_hit && out) {
        *out = best_contact;
    }
    return best_hit;
}

static int np_aabb_vs_aabb(
    const sicol_shape_t* a,
    const sicol_shape_t* b,
    sicol_contact_t* out
)
{
    fx amin;
    fx amax;
    fx bmin;
    fx bmax;
    fx overlap;
    fx min_overlap = 0;
    int axis = -1;
    fx normal[3];
    fx point[3];

    amin = a->pos[0] - a->u.aabb.half[0];
    amax = a->pos[0] + a->u.aabb.half[0];
    bmin = b->pos[0] - b->u.aabb.half[0];
    bmax = b->pos[0] + b->u.aabb.half[0];
    if (amax < bmin || amin > bmax) return 0;
    overlap = FX_MIN(amax, bmax) - FX_MAX(amin, bmin);
    min_overlap = overlap;
    axis = 0;

    amin = a->pos[1] - a->u.aabb.half[1];
    amax = a->pos[1] + a->u.aabb.half[1];
    bmin = b->pos[1] - b->u.aabb.half[1];
    bmax = b->pos[1] + b->u.aabb.half[1];
    if (amax < bmin || amin > bmax) return 0;
    overlap = FX_MIN(amax, bmax) - FX_MAX(amin, bmin);
    if (overlap < min_overlap) {
        min_overlap = overlap;
        axis = 1;
    }

    amin = a->pos[2] - a->u.aabb.half[2];
    amax = a->pos[2] + a->u.aabb.half[2];
    bmin = b->pos[2] - b->u.aabb.half[2];
    bmax = b->pos[2] + b->u.aabb.half[2];
    if (amax < bmin || amin > bmax) return 0;
    overlap = FX_MIN(amax, bmax) - FX_MAX(amin, bmin);
    if (overlap < min_overlap) {
        min_overlap = overlap;
        axis = 2;
    }

    fx_zero3(normal);
    fx_copy3(point, a->pos);
    if (axis >= 0) {
        normal[axis] = (b->pos[axis] >= a->pos[axis]) ? FX_ONE : -FX_ONE;
        point[axis] = (a->pos[axis] + b->pos[axis]) / 2;
    }

    np_fill_contact(out, normal, point, min_overlap);
    return 1;
}

static int np_sphere_vs_sphere(
    const sicol_shape_t* a,
    const sicol_shape_t* b,
    sicol_contact_t* out
)
{
    fx delta[3];
    fx dist_sq;
    fx dist;
    fx radius_sum;
    fx normal[3];
    fx point[3];

    fx_sub3(delta, b->pos, a->pos);
    dist_sq = fx_len_sq3(delta);
    radius_sum = a->u.sphere.radius + b->u.sphere.radius;

    if (dist_sq > FX_MUL(radius_sum, radius_sum)) return 0;

    dist = fx_sqrt(dist_sq);
    if (dist > FX_EPSILON) {
        normal[0] = FX_DIV(delta[0], dist);
        normal[1] = FX_DIV(delta[1], dist);
        normal[2] = FX_DIV(delta[2], dist);
    } else {
        fx_set3(normal, FX_ONE, 0, 0);
    }

    fx_madd3(point, a->pos, normal, a->u.sphere.radius);
    np_fill_contact(out, normal, point, radius_sum - dist);
    return 1;
}

static int np_aabb_vs_sphere(
    const sicol_shape_t* a,
    const sicol_shape_t* b,
    sicol_contact_t* out
)
{
    fx closest[3];
    fx delta[3];
    fx dist_sq;
    fx dist;
    fx normal[3];
    fx point[3];

    np_closest_point_aabb(a, b->pos, closest);
    fx_sub3(delta, b->pos, closest);
    dist_sq = fx_len_sq3(delta);

    if (dist_sq > FX_MUL(b->u.sphere.radius, b->u.sphere.radius)) return 0;

    dist = fx_sqrt(dist_sq);

    if (dist > FX_EPSILON) {
        normal[0] = FX_DIV(delta[0], dist);
        normal[1] = FX_DIV(delta[1], dist);
        normal[2] = FX_DIV(delta[2], dist);
    } else {
        fx_set3(normal, FX_ONE, 0, 0);
    }

    fx_madd3(point, closest, normal, dist / 2);
    np_fill_contact(out, normal, point, b->u.sphere.radius - dist);
    return 1;
}

static int np_obb_vs_sphere(
    const sicol_shape_t* a,
    const sicol_shape_t* b,
    sicol_contact_t* out
)
{
    fx closest[3];
    fx delta[3];
    fx dist_sq;
    fx dist;
    fx normal[3];
    fx point[3];

    np_closest_point_obb(a, b->pos, closest);
    fx_sub3(delta, b->pos, closest);
    dist_sq = fx_len_sq3(delta);

    if (dist_sq > FX_MUL(b->u.sphere.radius, b->u.sphere.radius)) return 0;

    dist = fx_sqrt(dist_sq);

    if (dist > FX_EPSILON) {
        normal[0] = FX_DIV(delta[0], dist);
        normal[1] = FX_DIV(delta[1], dist);
        normal[2] = FX_DIV(delta[2], dist);
    } else {
        fx_set3(normal, a->u.obb.axis[0][0], a->u.obb.axis[0][1], a->u.obb.axis[0][2]);
    }

    fx_madd3(point, closest, normal, dist / 2);
    np_fill_contact(out, normal, point, b->u.sphere.radius - dist);
    return 1;
}

static int np_plane_vs_sphere(
    const sicol_shape_t* plane,
    const sicol_shape_t* sphere,
    sicol_contact_t* out
)
{
    fx signed_dist;
    fx point[3];
    signed_dist = fx_dot3(plane->u.plane.normal, sphere->pos) + plane->u.plane.d;
    if (signed_dist > sphere->u.sphere.radius) return 0;

    fx_madd3(point, sphere->pos, plane->u.plane.normal, -signed_dist);
    np_fill_contact(out, plane->u.plane.normal, point, sphere->u.sphere.radius - signed_dist);
    return 1;
}

static int np_plane_vs_aabb(
    const sicol_shape_t* plane,
    const sicol_shape_t* box,
    sicol_contact_t* out
)
{
    fx radius;
    fx signed_dist;
    fx point[3];

    radius =
        FX_ABS(FX_MUL(plane->u.plane.normal[0], box->u.aabb.half[0])) +
        FX_ABS(FX_MUL(plane->u.plane.normal[1], box->u.aabb.half[1])) +
        FX_ABS(FX_MUL(plane->u.plane.normal[2], box->u.aabb.half[2]));

    signed_dist = fx_dot3(plane->u.plane.normal, box->pos) + plane->u.plane.d;
    if (signed_dist > radius) return 0;

    fx_madd3(point, box->pos, plane->u.plane.normal, -signed_dist);
    np_fill_contact(out, plane->u.plane.normal, point, radius - signed_dist);
    return 1;
}

static int np_plane_vs_obb(
    const sicol_shape_t* plane,
    const sicol_shape_t* box,
    sicol_contact_t* out
)
{
    fx radius;
    fx signed_dist;
    fx point[3];

    radius =
        FX_ABS(FX_MUL(box->u.obb.half[0], fx_dot3(plane->u.plane.normal, box->u.obb.axis[0]))) +
        FX_ABS(FX_MUL(box->u.obb.half[1], fx_dot3(plane->u.plane.normal, box->u.obb.axis[1]))) +
        FX_ABS(FX_MUL(box->u.obb.half[2], fx_dot3(plane->u.plane.normal, box->u.obb.axis[2])));

    signed_dist = fx_dot3(plane->u.plane.normal, box->pos) + plane->u.plane.d;
    if (signed_dist > radius) return 0;

    fx_madd3(point, box->pos, plane->u.plane.normal, -signed_dist);
    np_fill_contact(out, plane->u.plane.normal, point, radius - signed_dist);
    return 1;
}

static int np_plane_vs_capsule(
    const sicol_shape_t* plane,
    const sicol_shape_t* capsule,
    sicol_contact_t* out
)
{
    fx projected = FX_ABS(FX_MUL(capsule->u.capsule.half_segment,
                                 fx_dot3(plane->u.plane.normal, capsule->u.capsule.axis)));
    fx signed_dist = fx_dot3(plane->u.plane.normal, capsule->pos) + plane->u.plane.d;
    fx reach = projected + capsule->u.capsule.radius;
    fx point[3];

    if (signed_dist > reach) return 0;
    fx_madd3(point, capsule->pos, plane->u.plane.normal, -signed_dist);
    np_fill_contact(out, plane->u.plane.normal, point, reach - signed_dist);
    return 1;
}

static int np_triangle_vs_sphere(
    const sicol_shape_t* tri,
    const sicol_shape_t* sphere,
    sicol_contact_t* out
)
{
    fx a[3];
    fx b[3];
    fx c[3];
    fx closest[3];
    fx delta[3];
    fx dist_sq;
    fx dist;
    fx ab[3];
    fx ac[3];
    fx normal[3];

    sicol_shape_get_triangle_points(tri, a, b, c);
    np_closest_point_triangle(sphere->pos, a, b, c, closest);
    fx_sub3(delta, sphere->pos, closest);
    dist_sq = fx_len_sq3(delta);
    if (dist_sq > FX_MUL(sphere->u.sphere.radius, sphere->u.sphere.radius)) return 0;

    dist = fx_sqrt(dist_sq);
    if (dist > FX_EPSILON) {
        normal[0] = FX_DIV(delta[0], dist);
        normal[1] = FX_DIV(delta[1], dist);
        normal[2] = FX_DIV(delta[2], dist);
    } else {
        fx_sub3(ab, b, a);
        fx_sub3(ac, c, a);
        fx_cross3(normal, ab, ac);
        if (!fx_normalize3(normal, normal)) {
            fx_set3(normal, FX_ONE, 0, 0);
        }
    }

    np_fill_contact(out, normal, closest, sphere->u.sphere.radius - dist);
    return 1;
}

static void np_triangle_normal(const fx a[3], const fx b[3], const fx c[3], fx out_normal[3])
{
    fx ab[3];
    fx ac[3];
    fx_sub3(ab, b, a);
    fx_sub3(ac, c, a);
    fx_cross3(out_normal, ab, ac);
    if (!fx_normalize3(out_normal, out_normal)) {
        fx_set3(out_normal, FX_ONE, 0, 0);
    }
}

static void np_segment_triangle_closest_points(
    const fx s0[3],
    const fx s1[3],
    const fx a[3],
    const fx b[3],
    const fx c[3],
    fx out_seg[3],
    fx out_tri[3],
    fx out_normal[3],
    fx* out_dist_sq
)
{
    fx best_seg[3];
    fx best_tri[3];
    fx best_normal[3];
    fx best_dist_sq;
    fx tri_normal[3];
    fx seg_dir[3];
    fx temp_seg[3];
    fx temp_tri[3];
    fx delta[3];
    fx d0;
    fx d1;
    fx denom;
    fx t;
    fx cp[3];
    fx mid[3];

    np_triangle_normal(a, b, c, tri_normal);
    best_dist_sq = -1;

    fx_sub3(seg_dir, s1, s0);
    d0 = fx_dot3(tri_normal, s0) - fx_dot3(tri_normal, a);
    d1 = fx_dot3(tri_normal, s1) - fx_dot3(tri_normal, a);
    if ((d0 <= 0 && d1 >= 0) || (d1 <= 0 && d0 >= 0)) {
        denom = d0 - d1;
        if (FX_ABS(denom) > FX_EPSILON) {
            t = FX_DIV(d0, denom);
            if (t >= 0 && t <= FX_ONE) {
                fx_madd3(temp_seg, s0, seg_dir, t);
                np_closest_point_triangle(temp_seg, a, b, c, cp);
                fx_sub3(delta, temp_seg, cp);
                if (fx_len_sq3(delta) <= FX_EPSILON) {
                    fx_copy3(best_seg, temp_seg);
                    fx_copy3(best_tri, cp);
                    fx_copy3(best_normal, tri_normal);
                    best_dist_sq = 0;
                }
            }
        }
    }

    np_closest_point_triangle(s0, a, b, c, cp);
    fx_sub3(delta, s0, cp);
    if (best_dist_sq < 0 || fx_len_sq3(delta) < best_dist_sq) {
        best_dist_sq = fx_len_sq3(delta);
        fx_copy3(best_seg, s0);
        fx_copy3(best_tri, cp);
        fx_copy3(best_normal, tri_normal);
    }

    np_closest_point_triangle(s1, a, b, c, cp);
    fx_sub3(delta, s1, cp);
    if (best_dist_sq < 0 || fx_len_sq3(delta) < best_dist_sq) {
        best_dist_sq = fx_len_sq3(delta);
        fx_copy3(best_seg, s1);
        fx_copy3(best_tri, cp);
        fx_copy3(best_normal, tri_normal);
    }

    fx_closest_points_segment_segment3(temp_seg, temp_tri, s0, s1, a, b, 0, 0);
    fx_sub3(delta, temp_seg, temp_tri);
    if (best_dist_sq < 0 || fx_len_sq3(delta) < best_dist_sq) {
        best_dist_sq = fx_len_sq3(delta);
        fx_copy3(best_seg, temp_seg);
        fx_copy3(best_tri, temp_tri);
        fx_copy3(best_normal, tri_normal);
    }

    fx_closest_points_segment_segment3(temp_seg, temp_tri, s0, s1, b, c, 0, 0);
    fx_sub3(delta, temp_seg, temp_tri);
    if (best_dist_sq < 0 || fx_len_sq3(delta) < best_dist_sq) {
        best_dist_sq = fx_len_sq3(delta);
        fx_copy3(best_seg, temp_seg);
        fx_copy3(best_tri, temp_tri);
        fx_copy3(best_normal, tri_normal);
    }

    fx_closest_points_segment_segment3(temp_seg, temp_tri, s0, s1, c, a, 0, 0);
    fx_sub3(delta, temp_seg, temp_tri);
    if (best_dist_sq < 0 || fx_len_sq3(delta) < best_dist_sq) {
        best_dist_sq = fx_len_sq3(delta);
        fx_copy3(best_seg, temp_seg);
        fx_copy3(best_tri, temp_tri);
        fx_copy3(best_normal, tri_normal);
    }

    if (best_dist_sq <= FX_EPSILON) {
        mid[0] = (s0[0] + s1[0]) / 2;
        mid[1] = (s0[1] + s1[1]) / 2;
        mid[2] = (s0[2] + s1[2]) / 2;
        fx_sub3(delta, mid, best_tri);
        if (fx_dot3(delta, best_normal) < 0) {
            fx_neg3(best_normal, best_normal);
        }
    } else {
        fx_sub3(delta, best_seg, best_tri);
        if (!fx_normalize3(best_normal, delta)) {
            np_triangle_normal(a, b, c, best_normal);
        }
    }

    fx_copy3(out_seg, best_seg);
    fx_copy3(out_tri, best_tri);
    fx_copy3(out_normal, best_normal);
    if (out_dist_sq) *out_dist_sq = best_dist_sq;
}

static int np_triangle_vs_capsule(
    const sicol_shape_t* tri,
    const sicol_shape_t* capsule,
    sicol_contact_t* out
)
{
    fx a[3];
    fx b[3];
    fx c[3];
    fx s0[3];
    fx s1[3];
    fx seg_pt[3];
    fx tri_pt[3];
    fx normal[3];
    fx dist_sq;
    fx dist;
    fx radius_sq;

    sicol_shape_get_triangle_points(tri, a, b, c);
    sicol_shape_get_capsule_segment(capsule, s0, s1);
    np_segment_triangle_closest_points(s0, s1, a, b, c, seg_pt, tri_pt, normal, &dist_sq);
    radius_sq = FX_MUL(capsule->u.capsule.radius, capsule->u.capsule.radius);
    if (dist_sq > radius_sq) return 0;

    dist = fx_sqrt(dist_sq);
    np_fill_contact(out, normal, tri_pt, capsule->u.capsule.radius - dist);
    return 1;
}

static void np_capsule_segment(const sicol_shape_t* c, fx out_a[3], fx out_b[3])
{
    sicol_shape_get_capsule_segment(c, out_a, out_b);
}

static int np_capsule_vs_sphere(
    const sicol_shape_t* capsule,
    const sicol_shape_t* sphere,
    sicol_contact_t* out
)
{
    fx a[3];
    fx b[3];
    fx closest[3];
    fx delta[3];
    fx dist_sq;
    fx dist;
    fx radius_sum;
    fx normal[3];
    fx point[3];

    np_capsule_segment(capsule, a, b);
    fx_closest_point_segment3(closest, a, b, sphere->pos);
    fx_sub3(delta, sphere->pos, closest);
    dist_sq = fx_len_sq3(delta);
    radius_sum = capsule->u.capsule.radius + sphere->u.sphere.radius;

    if (dist_sq > FX_MUL(radius_sum, radius_sum)) return 0;
    dist = fx_sqrt(dist_sq);

    if (dist > FX_EPSILON) {
        normal[0] = FX_DIV(delta[0], dist);
        normal[1] = FX_DIV(delta[1], dist);
        normal[2] = FX_DIV(delta[2], dist);
    } else {
        fx_copy3(normal, capsule->u.capsule.axis);
    }

    fx_madd3(point, closest, normal, capsule->u.capsule.radius);
    np_fill_contact(out, normal, point, radius_sum - dist);
    return 1;
}

static int np_capsule_vs_capsule(
    const sicol_shape_t* a,
    const sicol_shape_t* b,
    sicol_contact_t* out
)
{
    fx a0[3];
    fx a1[3];
    fx b0[3];
    fx b1[3];
    fx pa[3];
    fx pb[3];
    fx delta[3];
    fx dist_sq;
    fx dist;
    fx radius_sum;
    fx normal[3];
    fx point[3];

    np_capsule_segment(a, a0, a1);
    np_capsule_segment(b, b0, b1);

    fx_closest_points_segment_segment3(pa, pb, a0, a1, b0, b1, 0, 0);
    fx_sub3(delta, pb, pa);
    dist_sq = fx_len_sq3(delta);
    radius_sum = a->u.capsule.radius + b->u.capsule.radius;

    if (dist_sq > FX_MUL(radius_sum, radius_sum)) return 0;
    dist = fx_sqrt(dist_sq);

    if (dist > FX_EPSILON) {
        normal[0] = FX_DIV(delta[0], dist);
        normal[1] = FX_DIV(delta[1], dist);
        normal[2] = FX_DIV(delta[2], dist);
    } else {
        fx_copy3(normal, a->u.capsule.axis);
    }

    fx_madd3(point, pa, normal, a->u.capsule.radius);
    np_fill_contact(out, normal, point, radius_sum - dist);
    return 1;
}

static int np_obb_vs_obb(
    const sicol_shape_t* a,
    const sicol_shape_t* b,
    sicol_contact_t* out
)
{
    fx R[3][3];
    fx AbsR[3][3];
    fx t_world[3];
    fx t[3];
    fx ra;
    fx rb;
    fx val;
    fx pen;
    fx min_pen = 0;
    fx best_axis[3];
    fx axis_candidate[3];
    int best_init = 0;
    int i, j;

    fx_sub3(t_world, b->pos, a->pos);
    fx_basis_to_local3(t, a->u.obb.axis, t_world);

    for (i = 0; i < 3; ++i) {
        for (j = 0; j < 3; ++j) {
            R[i][j] = fx_dot3(a->u.obb.axis[i], b->u.obb.axis[j]);
            AbsR[i][j] = FX_ABS(R[i][j]) + FX_EPSILON;
        }
    }

    for (i = 0; i < 3; ++i) {
        ra = a->u.obb.half[i];
        rb = FX_MUL(b->u.obb.half[0], AbsR[i][0]) +
             FX_MUL(b->u.obb.half[1], AbsR[i][1]) +
             FX_MUL(b->u.obb.half[2], AbsR[i][2]);
        val = FX_ABS(t[i]);
        if (val > ra + rb) return 0;
        pen = ra + rb - val;
        if (!best_init || pen < min_pen) {
            min_pen = pen;
            fx_copy3(best_axis, a->u.obb.axis[i]);
            if (t[i] < 0) fx_neg3(best_axis, best_axis);
            best_init = 1;
        }
    }

    for (j = 0; j < 3; ++j) {
        ra = FX_MUL(a->u.obb.half[0], AbsR[0][j]) +
             FX_MUL(a->u.obb.half[1], AbsR[1][j]) +
             FX_MUL(a->u.obb.half[2], AbsR[2][j]);
        rb = b->u.obb.half[j];
        val = FX_ABS(fx_dot3(t_world, b->u.obb.axis[j]));
        if (val > ra + rb) return 0;
        pen = ra + rb - val;
        if (pen < min_pen) {
            min_pen = pen;
            fx_copy3(best_axis, b->u.obb.axis[j]);
            if (fx_dot3(t_world, best_axis) < 0) fx_neg3(best_axis, best_axis);
        }
    }

    for (i = 0; i < 3; ++i) {
        for (j = 0; j < 3; ++j) {
            ra = FX_MUL(a->u.obb.half[(i + 1) % 3], AbsR[(i + 2) % 3][j]) +
                 FX_MUL(a->u.obb.half[(i + 2) % 3], AbsR[(i + 1) % 3][j]);
            rb = FX_MUL(b->u.obb.half[(j + 1) % 3], AbsR[i][(j + 2) % 3]) +
                 FX_MUL(b->u.obb.half[(j + 2) % 3], AbsR[i][(j + 1) % 3]);
            val = FX_ABS(FX_MUL(t[(i + 2) % 3], R[(i + 1) % 3][j]) -
                         FX_MUL(t[(i + 1) % 3], R[(i + 2) % 3][j]));
            if (val > ra + rb) return 0;
            pen = ra + rb - val;
            fx_cross3(axis_candidate, a->u.obb.axis[i], b->u.obb.axis[j]);
            if (fx_normalize3(axis_candidate, axis_candidate) && pen < min_pen) {
                if (fx_dot3(t_world, axis_candidate) < 0) fx_neg3(axis_candidate, axis_candidate);
                min_pen = pen;
                fx_copy3(best_axis, axis_candidate);
            }
        }
    }

    np_fill_contact(out, best_axis, a->pos, min_pen);
    return 1;
}


static int np_segment_to_ray(
    const sicol_shape_t* seg,
    sicol_shape_t* ray,
    fx* out_length
)
{
    fx a[3];
    fx b[3];
    fx dir[3];
    fx len;

    if (!seg || seg->type != SICOL_SHAPE_SEGMENT || !ray) return 0;
    sicol_shape_get_segment_points(seg, a, b);
    fx_sub3(dir, b, a);
    len = fx_len3(dir);
    if (len <= FX_EPSILON) return 0;
    sicol_shape_make_ray(ray, a, dir, len);
    if (out_length) *out_length = len;
    return 1;
}

static int np_segment_vs_sphere(
    const sicol_shape_t* seg,
    const sicol_shape_t* sphere,
    sicol_contact_t* out
)
{
    fx a[3];
    fx b[3];
    fx closest[3];
    fx delta[3];
    fx dist_sq;
    fx dist;
    fx normal[3];
    fx point[3];

    sicol_shape_get_segment_points(seg, a, b);
    fx_closest_point_segment3(closest, a, b, sphere->pos);
    fx_sub3(delta, sphere->pos, closest);
    dist_sq = fx_len_sq3(delta);
    if (dist_sq > FX_MUL(sphere->u.sphere.radius, sphere->u.sphere.radius)) return 0;

    dist = fx_sqrt(dist_sq);
    if (dist > FX_EPSILON) {
        normal[0] = FX_DIV(delta[0], dist);
        normal[1] = FX_DIV(delta[1], dist);
        normal[2] = FX_DIV(delta[2], dist);
    } else {
        fx_sub3(normal, b, a);
        if (!fx_normalize3(normal, normal)) {
            fx_set3(normal, FX_ONE, 0, 0);
        }
    }

    fx_copy3(point, closest);
    np_fill_contact(out, normal, point, sphere->u.sphere.radius - dist);
    return 1;
}

static int np_segment_vs_capsule(
    const sicol_shape_t* seg,
    const sicol_shape_t* capsule,
    sicol_contact_t* out
)
{
    fx sa[3];
    fx sb[3];
    fx ca[3];
    fx cb[3];
    fx pa[3];
    fx pb[3];
    fx delta[3];
    fx dist_sq;
    fx dist;
    fx normal[3];
    fx point[3];

    sicol_shape_get_segment_points(seg, sa, sb);
    sicol_shape_get_capsule_segment(capsule, ca, cb);
    fx_closest_points_segment_segment3(pa, pb, sa, sb, ca, cb, 0, 0);
    fx_sub3(delta, pb, pa);
    dist_sq = fx_len_sq3(delta);
    if (dist_sq > FX_MUL(capsule->u.capsule.radius, capsule->u.capsule.radius)) return 0;

    dist = fx_sqrt(dist_sq);
    if (dist > FX_EPSILON) {
        normal[0] = FX_DIV(delta[0], dist);
        normal[1] = FX_DIV(delta[1], dist);
        normal[2] = FX_DIV(delta[2], dist);
    } else {
        fx_copy3(normal, capsule->u.capsule.axis);
    }

    fx_copy3(point, pa);
    np_fill_contact(out, normal, point, capsule->u.capsule.radius - dist);
    return 1;
}

static int np_plane_vs_segment(
    const sicol_shape_t* plane,
    const sicol_shape_t* seg,
    sicol_contact_t* out
)
{
    fx a[3];
    fx b[3];
    fx ab[3];
    fx point[3];
    fx d0;
    fx d1;
    fx denom;
    fx t;
    fx penetration;

    sicol_shape_get_segment_points(seg, a, b);
    d0 = fx_dot3(plane->u.plane.normal, a) + plane->u.plane.d;
    d1 = fx_dot3(plane->u.plane.normal, b) + plane->u.plane.d;

    if (d0 > 0 && d1 > 0) return 0;

    if ((d0 <= 0 && d1 >= 0) || (d1 <= 0 && d0 >= 0)) {
        fx_sub3(ab, b, a);
        denom = d0 - d1;
        if (FX_ABS(denom) > FX_EPSILON) {
            t = FX_DIV(d0, denom);
        } else {
            t = 0;
        }
        t = fx_clamp(t, 0, FX_ONE);
        fx_madd3(point, a, ab, t);
        np_fill_contact(out, plane->u.plane.normal, point, 0);
        return 1;
    }

    if (d0 < d1) {
        fx_copy3(point, a);
        penetration = -d0;
    } else {
        fx_copy3(point, b);
        penetration = -d1;
    }

    if (penetration < 0) penetration = 0;
    np_fill_contact(out, plane->u.plane.normal, point, penetration);
    return 1;
}

static int np_segment_vs_aabb(
    const sicol_shape_t* seg,
    const sicol_shape_t* box,
    sicol_contact_t* out
)
{
    sicol_shape_t ray;
    sicol_hit_t hit;
    fx normal[3];

    if (!np_segment_to_ray(seg, &ray, 0)) return 0;
    if (!sicol_raycast(&ray, box, &hit)) return 0;
    fx_neg3(normal, hit.normal);
    np_fill_contact(out, normal, hit.point, 0);
    return 1;
}

static int np_segment_vs_obb(
    const sicol_shape_t* seg,
    const sicol_shape_t* box,
    sicol_contact_t* out
)
{
    sicol_shape_t ray;
    sicol_hit_t hit;
    fx normal[3];

    if (!np_segment_to_ray(seg, &ray, 0)) return 0;
    if (!sicol_raycast(&ray, box, &hit)) return 0;
    fx_neg3(normal, hit.normal);
    np_fill_contact(out, normal, hit.point, 0);
    return 1;
}

int sicol_narrowphase_test_cached(
    const sicol_shape_t* a_in,
    const sicol_shape_t* b_in,
    sicol_contact_t* out,
    sicol_gjk_cache_t* cache
)
{
    const sicol_shape_t* a = a_in;
    const sicol_shape_t* b = b_in;
    int swapped = 0;
    sicol_shape_t a_obb;

    if (!a || !b) return 0;
    np_contact_zero(out);

    if (a->type == SICOL_SHAPE_AABB && b->type == SICOL_SHAPE_AABB) {
        return np_aabb_vs_aabb(a, b, out);
    }

    if (a->type == SICOL_SHAPE_SPHERE && b->type == SICOL_SHAPE_SPHERE) {
        return np_sphere_vs_sphere(a, b, out);
    }

    if (a->type == SICOL_SHAPE_AABB && b->type == SICOL_SHAPE_OBB) {
        sicol_shape_make_obb_identity(&a_obb, a->pos, a->u.aabb.half);
        return np_obb_vs_obb(&a_obb, b, out);
    }

    if (a->type == SICOL_SHAPE_OBB && b->type == SICOL_SHAPE_AABB) {
        sicol_shape_make_obb_identity(&a_obb, b->pos, b->u.aabb.half);
        return np_obb_vs_obb(a, &a_obb, out);
    }

    if (a->type == SICOL_SHAPE_OBB && b->type == SICOL_SHAPE_OBB) {
        return np_obb_vs_obb(a, b, out);
    }

    if (a->type == SICOL_SHAPE_AABB && b->type == SICOL_SHAPE_SPHERE) {
        return np_aabb_vs_sphere(a, b, out);
    }

    if (a->type == SICOL_SHAPE_SPHERE && b->type == SICOL_SHAPE_AABB) {
        np_swap_shapes(&a, &b, &swapped);
        if (!np_aabb_vs_sphere(a, b, out)) return 0;
        if (swapped) np_swap_contact(out);
        return 1;
    }

    if (a->type == SICOL_SHAPE_OBB && b->type == SICOL_SHAPE_SPHERE) {
        return np_obb_vs_sphere(a, b, out);
    }

    if (a->type == SICOL_SHAPE_SPHERE && b->type == SICOL_SHAPE_OBB) {
        np_swap_shapes(&a, &b, &swapped);
        if (!np_obb_vs_sphere(a, b, out)) return 0;
        if (swapped) np_swap_contact(out);
        return 1;
    }

    if (a->type == SICOL_SHAPE_PLANE && b->type == SICOL_SHAPE_SPHERE) {
        return np_plane_vs_sphere(a, b, out);
    }

    if (a->type == SICOL_SHAPE_SPHERE && b->type == SICOL_SHAPE_PLANE) {
        np_swap_shapes(&a, &b, &swapped);
        if (!np_plane_vs_sphere(a, b, out)) return 0;
        if (swapped) np_swap_contact(out);
        return 1;
    }

    if (a->type == SICOL_SHAPE_PLANE && b->type == SICOL_SHAPE_AABB) {
        return np_plane_vs_aabb(a, b, out);
    }

    if (a->type == SICOL_SHAPE_AABB && b->type == SICOL_SHAPE_PLANE) {
        np_swap_shapes(&a, &b, &swapped);
        if (!np_plane_vs_aabb(a, b, out)) return 0;
        if (swapped) np_swap_contact(out);
        return 1;
    }

    if (a->type == SICOL_SHAPE_PLANE && b->type == SICOL_SHAPE_OBB) {
        return np_plane_vs_obb(a, b, out);
    }

    if (a->type == SICOL_SHAPE_OBB && b->type == SICOL_SHAPE_PLANE) {
        np_swap_shapes(&a, &b, &swapped);
        if (!np_plane_vs_obb(a, b, out)) return 0;
        if (swapped) np_swap_contact(out);
        return 1;
    }

    if (a->type == SICOL_SHAPE_CAPSULE && b->type == SICOL_SHAPE_SPHERE) {
        return np_capsule_vs_sphere(a, b, out);
    }

    if (a->type == SICOL_SHAPE_SPHERE && b->type == SICOL_SHAPE_CAPSULE) {
        np_swap_shapes(&a, &b, &swapped);
        if (!np_capsule_vs_sphere(a, b, out)) return 0;
        if (swapped) np_swap_contact(out);
        return 1;
    }

    if (a->type == SICOL_SHAPE_CAPSULE && b->type == SICOL_SHAPE_CAPSULE) {
        return np_capsule_vs_capsule(a, b, out);
    }

    if (a->type == SICOL_SHAPE_PLANE && b->type == SICOL_SHAPE_CAPSULE) {
        return np_plane_vs_capsule(a, b, out);
    }

    if (a->type == SICOL_SHAPE_CAPSULE && b->type == SICOL_SHAPE_PLANE) {
        np_swap_shapes(&a, &b, &swapped);
        if (!np_plane_vs_capsule(a, b, out)) return 0;
        if (swapped) np_swap_contact(out);
        return 1;
    }

    if (a->type == SICOL_SHAPE_SEGMENT && b->type == SICOL_SHAPE_SPHERE) {
        return np_segment_vs_sphere(a, b, out);
    }

    if (a->type == SICOL_SHAPE_SPHERE && b->type == SICOL_SHAPE_SEGMENT) {
        np_swap_shapes(&a, &b, &swapped);
        if (!np_segment_vs_sphere(a, b, out)) return 0;
        if (swapped) np_swap_contact(out);
        return 1;
    }

    if (a->type == SICOL_SHAPE_SEGMENT && b->type == SICOL_SHAPE_CAPSULE) {
        return np_segment_vs_capsule(a, b, out);
    }

    if (a->type == SICOL_SHAPE_CAPSULE && b->type == SICOL_SHAPE_SEGMENT) {
        np_swap_shapes(&a, &b, &swapped);
        if (!np_segment_vs_capsule(a, b, out)) return 0;
        if (swapped) np_swap_contact(out);
        return 1;
    }

    if (a->type == SICOL_SHAPE_PLANE && b->type == SICOL_SHAPE_SEGMENT) {
        return np_plane_vs_segment(a, b, out);
    }

    if (a->type == SICOL_SHAPE_SEGMENT && b->type == SICOL_SHAPE_PLANE) {
        np_swap_shapes(&a, &b, &swapped);
        if (!np_plane_vs_segment(a, b, out)) return 0;
        if (swapped) np_swap_contact(out);
        return 1;
    }

    if (a->type == SICOL_SHAPE_SEGMENT && b->type == SICOL_SHAPE_AABB) {
        return np_segment_vs_aabb(a, b, out);
    }

    if (a->type == SICOL_SHAPE_AABB && b->type == SICOL_SHAPE_SEGMENT) {
        np_swap_shapes(&a, &b, &swapped);
        if (!np_segment_vs_aabb(a, b, out)) return 0;
        if (swapped) np_swap_contact(out);
        return 1;
    }

    if (a->type == SICOL_SHAPE_SEGMENT && b->type == SICOL_SHAPE_OBB) {
        return np_segment_vs_obb(a, b, out);
    }

    if (a->type == SICOL_SHAPE_OBB && b->type == SICOL_SHAPE_SEGMENT) {
        np_swap_shapes(&a, &b, &swapped);
        if (!np_segment_vs_obb(a, b, out)) return 0;
        if (swapped) np_swap_contact(out);
        return 1;
    }

    if (a->type == SICOL_SHAPE_TRIANGLE && b->type == SICOL_SHAPE_SPHERE) {
        return np_triangle_vs_sphere(a, b, out);
    }

    if (a->type == SICOL_SHAPE_SPHERE && b->type == SICOL_SHAPE_TRIANGLE) {
        np_swap_shapes(&a, &b, &swapped);
        if (!np_triangle_vs_sphere(a, b, out)) return 0;
        if (swapped) np_swap_contact(out);
        return 1;
    }

    if (a->type == SICOL_SHAPE_TRIANGLE && b->type == SICOL_SHAPE_CAPSULE) {
        return np_triangle_vs_capsule(a, b, out);
    }

    if (a->type == SICOL_SHAPE_CAPSULE && b->type == SICOL_SHAPE_TRIANGLE) {
        np_swap_shapes(&a, &b, &swapped);
        if (!np_triangle_vs_capsule(a, b, out)) return 0;
        if (swapped) np_swap_contact(out);
        return 1;
    }

    if (a->type == SICOL_SHAPE_TRIANGLE && sicol_shape_is_support_mapped(b)) {
        return np_support_pair_exact(a, b, out, cache);
    }

    if (sicol_shape_is_support_mapped(a) && b->type == SICOL_SHAPE_TRIANGLE) {
        np_swap_shapes(&a, &b, &swapped);
        if (!np_support_pair_exact(a, b, out, cache)) return 0;
        if (swapped) np_swap_contact(out);
        return 1;
    }

    if (a->type == SICOL_SHAPE_MESH && b->type != SICOL_SHAPE_MESH) {
        return np_mesh_vs_shape(a, b, out);
    }

    if (a->type != SICOL_SHAPE_MESH && b->type == SICOL_SHAPE_MESH) {
        if (!np_mesh_vs_shape(b, a, out)) return 0;
        np_swap_contact(out);
        return 1;
    }

    if (sicol_shape_is_support_mapped(a) && sicol_shape_is_support_mapped(b)) {
        return np_support_pair_exact(a, b, out, cache);
    }

    return 0;
}


int sicol_narrowphase_test(
    const sicol_shape_t* a,
    const sicol_shape_t* b,
    sicol_contact_t* out
)
{
    return sicol_narrowphase_test_cached(a, b, out, (sicol_gjk_cache_t*)0);
}
