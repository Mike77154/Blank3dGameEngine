/* ============================================================
 * SICOL - Pair shape casts / TOI helpers
 * ============================================================ */

#include "sicol_cast.h"
#include "sicol_convex.h"
#include "sicol_narrowphase.h"
#include "sicol_raycast.h"
#include "sicol_mesh.h"

#define SICOL_CAST_PAIR_TOL FX_FROM_RATIO(1, 1024)
#define SICOL_CAST_PAIR_BISECT 18
#define SICOL_CAST_PAIR_FALLBACK_STEPS 48

static void cast_zero_result(sicol_shape_cast_result_t* out)
{
    if (!out) return;
    out->hit = 0;
    out->fraction = 0;
    out->safe_fraction = 0;
    fx_zero3(out->normal);
    fx_zero3(out->point);
    out->iterations = 0;
}

static void cast_shape_at_fraction(sicol_shape_t* out, const sicol_shape_t* in_shape, const fx delta[3], fx fraction)
{
    fx step[3];
    *out = *in_shape;
    fx_scale3(step, delta, fraction);
    sicol_shape_translate(out, step);
}

static int cast_is_overlap(const sicol_shape_t* a, const sicol_shape_t* b, sicol_contact_t* out_contact)
{
    return sicol_narrowphase_test(a, b, out_contact);
}

static int cast_support_vs_plane(
    const sicol_shape_t* moving,
    const fx delta[3],
    const sicol_shape_t* plane,
    sicol_shape_cast_result_t* out
)
{
    fx neg_n[3];
    fx support[3];
    fx signed_dist;
    fx closing;
    fx fraction;
    fx moved_point[3];

    if (!moving || !delta || !plane || !out) return 0;
    if (!sicol_shape_is_support_mapped(moving)) return 0;
    if (plane->type != SICOL_SHAPE_PLANE) return 0;

    fx_neg3(neg_n, plane->u.plane.normal);
    sicol_shape_support_point(moving, neg_n, support);
    signed_dist = fx_dot3(plane->u.plane.normal, support) + plane->u.plane.d;
    if (signed_dist <= 0) {
        out->hit = 1;
        out->fraction = 0;
        out->safe_fraction = 0;
        fx_copy3(out->normal, plane->u.plane.normal);
        fx_copy3(out->point, support);
        out->iterations = 0;
        return 1;
    }

    closing = -fx_dot3(plane->u.plane.normal, delta);
    if (closing <= FX_EPSILON) return 0;

    fraction = FX_DIV(signed_dist, closing);
    if (fraction < 0 || fraction > FX_ONE) return 0;

    fx_madd3(moved_point, support, delta, fraction);
    out->hit = 1;
    out->fraction = fraction;
    out->safe_fraction = (fraction > SICOL_CAST_PAIR_TOL) ? (fraction - SICOL_CAST_PAIR_TOL) : 0;
    fx_copy3(out->normal, plane->u.plane.normal);
    fx_copy3(out->point, moved_point);
    out->iterations = 1;
    return 1;
}

static int cast_sphere_vs_sphere(
    const sicol_shape_t* moving,
    const fx delta[3],
    const sicol_shape_t* target,
    sicol_shape_cast_result_t* out
)
{
    fx rel[3];
    fx rsum;
    fx a;
    fx b;
    fx c;
    fx disc;
    fx sqrt_disc;
    fx denom;
    fx fraction;
    fx center[3];
    fx normal[3];
    fx point[3];

    if (!moving || !delta || !target || !out) return 0;
    if (moving->type != SICOL_SHAPE_SPHERE || target->type != SICOL_SHAPE_SPHERE) return 0;

    fx_sub3(rel, moving->pos, target->pos);
    rsum = moving->u.sphere.radius + target->u.sphere.radius;
    a = fx_dot3(delta, delta);
    c = fx_dot3(rel, rel) - FX_MUL(rsum, rsum);

    if (c <= 0) {
        if (!fx_normalize3(normal, rel)) {
            fx_set3(normal, FX_ONE, 0, 0);
        }
        fx_madd3(point, moving->pos, normal, -moving->u.sphere.radius);
        out->hit = 1;
        out->fraction = 0;
        out->safe_fraction = 0;
        fx_copy3(out->normal, normal);
        fx_copy3(out->point, point);
        out->iterations = 0;
        return 1;
    }

    if (a <= FX_EPSILON) return 0;

    b = FX_MUL(FX_FROM_INT(2), fx_dot3(rel, delta));
    disc = FX_MUL(b, b) - FX_MUL(FX_FROM_INT(4), FX_MUL(a, c));
    if (disc < 0) return 0;

    sqrt_disc = fx_sqrt(disc);
    denom = FX_MUL(FX_FROM_INT(2), a);
    if (denom == 0) return 0;

    fraction = FX_DIV(-b - sqrt_disc, denom);
    if (fraction < 0 || fraction > FX_ONE) return 0;

    fx_madd3(center, moving->pos, delta, fraction);
    fx_sub3(normal, center, target->pos);
    if (!fx_normalize3(normal, normal)) {
        fx_set3(normal, FX_ONE, 0, 0);
    }
    fx_madd3(point, center, normal, -moving->u.sphere.radius);

    out->hit = 1;
    out->fraction = fraction;
    out->safe_fraction = (fraction > SICOL_CAST_PAIR_TOL) ? (fraction - SICOL_CAST_PAIR_TOL) : 0;
    fx_copy3(out->normal, normal);
    fx_copy3(out->point, point);
    out->iterations = 1;
    return 1;
}

static int cast_ray_vs_aabb_local(
    const fx origin[3],
    const fx dir[3],
    fx length,
    const fx half[3],
    sicol_hit_t* out
)
{
    fx tmin = 0;
    fx tmax = length;
    fx normal_axis[3] = {0, 0, 0};
    fx slab_min;
    fx slab_max;
    fx inv_d;
    fx t0;
    fx t1;
    fx point[3];
    int i;

    for (i = 0; i < 3; ++i) {
        slab_min = -half[i];
        slab_max = half[i];

        if (FX_ABS(dir[i]) > FX_EPSILON) {
            inv_d = FX_DIV(FX_ONE, dir[i]);
            t0 = FX_MUL(slab_min - origin[i], inv_d);
            t1 = FX_MUL(slab_max - origin[i], inv_d);
            if (t0 > t1) {
                fx tmp = t0;
                t0 = t1;
                t1 = tmp;
            }
            if (t0 > tmin) {
                tmin = t0;
                fx_zero3(normal_axis);
                normal_axis[i] = (dir[i] > 0) ? -FX_ONE : FX_ONE;
            }
            if (t1 < tmax) tmax = t1;
            if (tmin > tmax) return 0;
        } else {
            if (origin[i] < slab_min || origin[i] > slab_max) return 0;
        }
    }

    if (tmin < 0 || tmin > length) return 0;
    fx_madd3(point, origin, dir, tmin);
    if (out) {
        out->t = tmin;
        fx_copy3(out->point, point);
        fx_copy3(out->normal, normal_axis);
        out->shape_index = -1;
    }
    return 1;
}

static int cast_sphere_vs_aabb_like(
    const sicol_shape_t* moving,
    const fx delta[3],
    const sicol_shape_t* target,
    sicol_shape_cast_result_t* out,
    int use_obb
)
{
    fx len;
    fx dir[3];
    sicol_hit_t hit;
    fx expanded_half[3];
    fx local_origin[3];
    fx local_dir[3];
    fx world_point[3];
    fx world_normal[3];
    fx point[3];

    if (!moving || !delta || !target || !out) return 0;
    if (moving->type != SICOL_SHAPE_SPHERE) return 0;

    len = fx_len3(delta);
    if (len <= FX_EPSILON) return 0;
    if (!fx_normalize3(dir, delta)) return 0;

    expanded_half[0] = (use_obb ? target->u.obb.half[0] : target->u.aabb.half[0]) + moving->u.sphere.radius;
    expanded_half[1] = (use_obb ? target->u.obb.half[1] : target->u.aabb.half[1]) + moving->u.sphere.radius;
    expanded_half[2] = (use_obb ? target->u.obb.half[2] : target->u.aabb.half[2]) + moving->u.sphere.radius;

    if (use_obb) {
        fx rel_origin[3];
        fx_sub3(rel_origin, moving->pos, target->pos);
        fx_basis_to_local3(local_origin, target->u.obb.axis, rel_origin);
        fx_basis_to_local3(local_dir, target->u.obb.axis, dir);
        if (!cast_ray_vs_aabb_local(local_origin, local_dir, len, expanded_half, &hit)) return 0;
        fx_basis_to_world3(world_point, target->u.obb.axis, hit.point);
        fx_basis_to_world3(world_normal, target->u.obb.axis, hit.normal);
        fx_add3(world_point, world_point, target->pos);
    } else {
        fx_sub3(local_origin, moving->pos, target->pos);
        if (!cast_ray_vs_aabb_local(local_origin, dir, len, expanded_half, &hit)) return 0;
        fx_add3(world_point, target->pos, hit.point);
        fx_copy3(world_normal, hit.normal);
    }

    out->hit = 1;
    out->fraction = FX_DIV(hit.t, len);
    out->safe_fraction = (out->fraction > SICOL_CAST_PAIR_TOL) ? (out->fraction - SICOL_CAST_PAIR_TOL) : 0;
    fx_copy3(out->normal, world_normal);
    fx_madd3(point, world_point, world_normal, -moving->u.sphere.radius);
    fx_copy3(out->point, point);
    out->iterations = 1;
    return 1;
}

static int cast_fallback_bisect(
    const sicol_shape_t* moving,
    const fx delta[3],
    const sicol_shape_t* target,
    sicol_shape_cast_result_t* out
)
{
    fx lo;
    fx hi;
    fx mid;
    fx step_fraction;
    sicol_shape_t probe;
    sicol_contact_t contact;
    int i;
    int found;

    if (!moving || !delta || !target || !out) return 0;

    if (cast_is_overlap(moving, target, &contact)) {
        out->hit = 1;
        out->fraction = 0;
        out->safe_fraction = 0;
        fx_copy3(out->normal, contact.normal);
        fx_copy3(out->point, contact.point);
        out->iterations = 0;
        return 1;
    }

    found = 0;
    lo = 0;
    hi = 0;

    for (i = 1; i <= SICOL_CAST_PAIR_FALLBACK_STEPS; ++i) {
        step_fraction = FX_DIV(FX_FROM_INT(i), FX_FROM_INT(SICOL_CAST_PAIR_FALLBACK_STEPS));
        cast_shape_at_fraction(&probe, moving, delta, step_fraction);
        if (cast_is_overlap(&probe, target, &contact)) {
            hi = step_fraction;
            lo = FX_DIV(FX_FROM_INT(i - 1), FX_FROM_INT(SICOL_CAST_PAIR_FALLBACK_STEPS));
            found = 1;
            break;
        }
    }
    if (!found) return 0;

    for (i = 0; i < SICOL_CAST_PAIR_BISECT; ++i) {
        mid = (lo + hi) / 2;
        cast_shape_at_fraction(&probe, moving, delta, mid);
        if (cast_is_overlap(&probe, target, &contact)) {
            hi = mid;
        } else {
            lo = mid;
        }
    }

    cast_shape_at_fraction(&probe, moving, delta, hi);
    if (!cast_is_overlap(&probe, target, &contact)) {
        return 0;
    }

    out->hit = 1;
    out->fraction = hi;
    out->safe_fraction = lo;
    fx_copy3(out->normal, contact.normal);
    fx_copy3(out->point, contact.point);
    out->iterations = SICOL_CAST_PAIR_BISECT;
    return 1;
}

static int cast_choose_earlier(
    sicol_shape_cast_result_t* io_best,
    int* io_hit,
    const sicol_shape_cast_result_t* candidate
)
{
    if (!io_best || !io_hit || !candidate || !candidate->hit) return 0;
    if (!(*io_hit) || candidate->fraction < io_best->fraction) {
        *io_best = *candidate;
        *io_hit = 1;
        return 1;
    }
    return 0;
}

static int cast_validate_fraction_hit(
    const sicol_shape_t* moving,
    const fx delta[3],
    const sicol_shape_t* target,
    fx fraction
)
{
    sicol_shape_t probe;
    sicol_contact_t contact;
    if (!moving || !delta || !target) return 0;
    if (fraction < 0 || fraction > FX_ONE) return 0;
    cast_shape_at_fraction(&probe, moving, delta, fraction);
    return cast_is_overlap(&probe, target, &contact);
}

static int cast_triangle_point_inside(
    const fx p[3],
    const fx a[3],
    const fx b[3],
    const fx c[3],
    const fx normal[3]
)
{
    fx edge[3];
    fx rel[3];
    fx cross[3];
    fx d;
    const fx tol = -FX_FROM_RATIO(1, 1024);

    fx_sub3(edge, b, a);
    fx_sub3(rel, p, a);
    fx_cross3(cross, edge, rel);
    d = fx_dot3(cross, normal);
    if (d < tol) return 0;

    fx_sub3(edge, c, b);
    fx_sub3(rel, p, b);
    fx_cross3(cross, edge, rel);
    d = fx_dot3(cross, normal);
    if (d < tol) return 0;

    fx_sub3(edge, a, c);
    fx_sub3(rel, p, c);
    fx_cross3(cross, edge, rel);
    d = fx_dot3(cross, normal);
    if (d < tol) return 0;

    return 1;
}

static int cast_sphere_vs_triangle_face(
    const sicol_shape_t* moving,
    const fx delta[3],
    const sicol_shape_t* tri_shape,
    sicol_shape_cast_result_t* out
)
{
    fx a[3], b[3], c[3];
    fx ab[3], ac[3], normal[3];
    fx dist0;
    fx vn;
    int best_hit = 0;
    sicol_shape_cast_result_t best;
    int sign_i;
    sicol_contact_t overlap_contact;

    if (!moving || !delta || !tri_shape || !out) return 0;
    if (moving->type != SICOL_SHAPE_SPHERE || tri_shape->type != SICOL_SHAPE_TRIANGLE) return 0;

    if (cast_is_overlap(moving, tri_shape, &overlap_contact)) {
        out->hit = 1;
        out->fraction = 0;
        out->safe_fraction = 0;
        fx_copy3(out->normal, overlap_contact.normal);
        fx_copy3(out->point, overlap_contact.point);
        out->iterations = 0;
        return 1;
    }

    sicol_shape_get_triangle_points(tri_shape, a, b, c);
    fx_sub3(ab, b, a);
    fx_sub3(ac, c, a);
    fx_cross3(normal, ab, ac);
    if (!fx_normalize3(normal, normal)) return 0;

    dist0 = fx_dot3(normal, moving->pos) - fx_dot3(normal, a);
    vn = fx_dot3(normal, delta);
    if (FX_ABS(vn) <= FX_EPSILON) return 0;

    cast_zero_result(&best);
    for (sign_i = -1; sign_i <= 1; sign_i += 2) {
        fx signed_radius = (sign_i > 0) ? moving->u.sphere.radius : -moving->u.sphere.radius;
        fx fraction = FX_DIV(signed_radius - dist0, vn);
        fx center[3];
        fx point[3];
        sicol_shape_cast_result_t candidate;
        if (fraction < 0 || fraction > FX_ONE) continue;
        fx_madd3(center, moving->pos, delta, fraction);
        fx_madd3(point, center, normal, -signed_radius);
        if (!cast_triangle_point_inside(point, a, b, c, normal)) continue;
        candidate.hit = 1;
        candidate.fraction = fraction;
        candidate.safe_fraction = (fraction > SICOL_CAST_PAIR_TOL) ? (fraction - SICOL_CAST_PAIR_TOL) : 0;
        if (sign_i > 0) {
            fx_copy3(candidate.normal, normal);
        } else {
            fx_neg3(candidate.normal, normal);
        }
        fx_copy3(candidate.point, point);
        candidate.iterations = 1;
        cast_choose_earlier(&best, &best_hit, &candidate);
    }

    if (best_hit) {
        *out = best;
    }
    return best_hit;
}

static int cast_capsule_vs_triangle_face(
    const sicol_shape_t* moving,
    const fx delta[3],
    const sicol_shape_t* tri_shape,
    sicol_shape_cast_result_t* out
)
{
    fx seg_a[3], seg_b[3];
    fx samples[3][3];
    int i;
    int best_hit = 0;
    sicol_shape_cast_result_t best;

    if (!moving || !delta || !tri_shape || !out) return 0;
    if (moving->type != SICOL_SHAPE_CAPSULE || tri_shape->type != SICOL_SHAPE_TRIANGLE) return 0;

    sicol_shape_get_capsule_segment(moving, seg_a, seg_b);
    fx_copy3(samples[0], seg_a);
    fx_copy3(samples[1], moving->pos);
    fx_copy3(samples[2], seg_b);

    cast_zero_result(&best);
    for (i = 0; i < 3; ++i) {
        sicol_shape_t sphere_sample;
        sicol_shape_cast_result_t candidate;
        sicol_shape_make_sphere(&sphere_sample, samples[i], moving->u.capsule.radius);
        if (!cast_sphere_vs_triangle_face(&sphere_sample, delta, tri_shape, &candidate)) continue;
        cast_choose_earlier(&best, &best_hit, &candidate);
    }

    if (best_hit) {
        *out = best;
    }
    return best_hit;
}

static int cast_triangle_target_refined(
    const sicol_shape_t* moving,
    const fx delta[3],
    const sicol_shape_t* tri_shape,
    sicol_shape_cast_result_t* out
)
{
    sicol_shape_cast_result_t best;
    sicol_shape_cast_result_t candidate;
    sicol_gjk_cache_t cache;
    int best_hit = 0;

    if (!moving || !delta || !tri_shape || !out) return 0;

    cast_zero_result(&best);

    if (moving->type == SICOL_SHAPE_SPHERE) {
        if (cast_sphere_vs_triangle_face(moving, delta, tri_shape, &candidate)) {
            cast_choose_earlier(&best, &best_hit, &candidate);
        }
    } else if (moving->type == SICOL_SHAPE_CAPSULE) {
        if (cast_capsule_vs_triangle_face(moving, delta, tri_shape, &candidate)) {
            cast_choose_earlier(&best, &best_hit, &candidate);
        }
    } else if (sicol_shape_is_support_mapped(moving) && sicol_shape_is_support_mapped(tri_shape)) {
        sicol_gjk_cache_reset(&cache);
        if (sicol_shape_cast_exact_cached(moving, delta, tri_shape, &cache, &candidate) &&
            cast_validate_fraction_hit(moving, delta, tri_shape, candidate.fraction)) {
            cast_choose_earlier(&best, &best_hit, &candidate);
        }
    }

    if (cast_fallback_bisect(moving, delta, tri_shape, &candidate)) {
        cast_choose_earlier(&best, &best_hit, &candidate);
    }

    if (best_hit) {
        *out = best;
    }
    return best_hit;
}

static int cast_mesh_target(
    const sicol_shape_t* moving,
    const fx delta[3],
    const sicol_shape_t* target,
    sicol_shape_cast_result_t* out
)
{
    int candidate_indices[SICOL_MESH_MAX_TRIANGLES];
    fx start_min[3];
    fx start_max[3];
    fx moved_min[3];
    fx moved_max[3];
    fx swept_min[3];
    fx swept_max[3];
    fx query_min[3];
    fx query_max[3];
    int count;
    int i;
    int best_hit;
    sicol_shape_cast_result_t best;

    if (!moving || !delta || !target || !out) return 0;
    if (target->type != SICOL_SHAPE_MESH || !target->u.mesh.mesh) return 0;

    sicol_shape_compute_aabb(moving, start_min, start_max);
    swept_min[0] = start_min[0]; swept_min[1] = start_min[1]; swept_min[2] = start_min[2];
    swept_max[0] = start_max[0]; swept_max[1] = start_max[1]; swept_max[2] = start_max[2];
    moved_min[0] = start_min[0] + delta[0];
    moved_min[1] = start_min[1] + delta[1];
    moved_min[2] = start_min[2] + delta[2];
    moved_max[0] = start_max[0] + delta[0];
    moved_max[1] = start_max[1] + delta[1];
    moved_max[2] = start_max[2] + delta[2];
    swept_min[0] = FX_MIN(swept_min[0], moved_min[0]);
    swept_min[1] = FX_MIN(swept_min[1], moved_min[1]);
    swept_min[2] = FX_MIN(swept_min[2], moved_min[2]);
    swept_max[0] = FX_MAX(swept_max[0], moved_max[0]);
    swept_max[1] = FX_MAX(swept_max[1], moved_max[1]);
    swept_max[2] = FX_MAX(swept_max[2], moved_max[2]);

    sicol_shape_mesh_world_aabb_to_local_aabb(target, swept_min, swept_max, query_min, query_max);

    count = sicol_mesh_query_local_aabb(target->u.mesh.mesh,
                                        query_min,
                                        query_max,
                                        candidate_indices,
                                        SICOL_MESH_MAX_TRIANGLES);

    best_hit = 0;
    cast_zero_result(&best);
    for (i = 0; i < count; ++i) {
        sicol_triangle_verts_t tri;
        sicol_shape_t tri_shape;
        sicol_shape_cast_result_t tri_hit;
        fx a[3];
        fx b[3];
        fx c[3];

        if (!sicol_mesh_get_triangle(target->u.mesh.mesh, candidate_indices[i], &tri)) continue;
        sicol_shape_mesh_local_to_world(target, tri.v0, a);
        sicol_shape_mesh_local_to_world(target, tri.v1, b);
        sicol_shape_mesh_local_to_world(target, tri.v2, c);
        sicol_shape_make_triangle(&tri_shape, a, b, c);

        if (!cast_triangle_target_refined(moving, delta, &tri_shape, &tri_hit)) continue;
        if (!tri_hit.hit) continue;
        if (!best_hit || tri_hit.fraction < best.fraction) {
            best = tri_hit;
            best_hit = 1;
        }
    }

    if (best_hit) {
        *out = best;
    }
    return best_hit;
}

int sicol_shape_cast_pair(
    const sicol_shape_t* moving,
    const fx delta[3],
    const sicol_shape_t* target,
    sicol_gjk_cache_t* cache,
    sicol_shape_cast_result_t* out
)
{
    cast_zero_result(out);
    if (!moving || !delta || !target || !out) return 0;
    if (!sicol_shape_validate(moving) || !sicol_shape_validate(target)) return 0;

    if (target->type == SICOL_SHAPE_PLANE) {
        if (cast_support_vs_plane(moving, delta, target, out)) return 1;
    }

    if (moving->type == SICOL_SHAPE_SPHERE && target->type == SICOL_SHAPE_SPHERE) {
        if (cast_sphere_vs_sphere(moving, delta, target, out)) return 1;
    }

    if (moving->type == SICOL_SHAPE_SPHERE && target->type == SICOL_SHAPE_AABB) {
        if (cast_sphere_vs_aabb_like(moving, delta, target, out, 0)) return 1;
    }

    if (moving->type == SICOL_SHAPE_SPHERE && target->type == SICOL_SHAPE_OBB) {
        if (cast_sphere_vs_aabb_like(moving, delta, target, out, 1)) return 1;
    }

    if (target->type == SICOL_SHAPE_MESH) {
        if (cast_mesh_target(moving, delta, target, out)) return 1;
    }

    if (sicol_shape_is_support_mapped(moving) && sicol_shape_is_support_mapped(target)) {
        if (sicol_shape_cast_exact_cached(moving, delta, target, cache, out)) return 1;
    }

    return cast_fallback_bisect(moving, delta, target, out);
}
