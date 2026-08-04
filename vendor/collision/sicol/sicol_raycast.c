/* ============================================================
 * SICOL - Raycast
 * ============================================================ */

#include "sicol_raycast.h"
#include "sicol_gjk.h"
#include "sicol_mesh.h"

#define SICOL_RAYCAST_CAPSULE_BISECT 18

static void rc_fill_hit(sicol_hit_t* out, fx t, const fx point[3], const fx normal[3])
{
    if (!out) return;
    out->t = t;
    fx_copy3(out->point, point);
    fx_copy3(out->normal, normal);
}

static void rc_ray_point(const sicol_shape_t* ray, fx t, fx out[3])
{
    fx_madd3(out, ray->pos, ray->u.ray.dir, t);
}

static int ray_vs_aabb_local(
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
        slab_max =  half[i];

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
    rc_fill_hit(out, tmin, point, normal_axis);
    return 1;
}

static int ray_vs_aabb(
    const sicol_shape_t* ray,
    const sicol_shape_t* box,
    sicol_hit_t* out
)
{
    fx local_origin[3];
    fx point[3];
    if (!ray || !box) return 0;
    fx_sub3(local_origin, ray->pos, box->pos);

    if (!ray_vs_aabb_local(local_origin, ray->u.ray.dir, ray->u.ray.length, box->u.aabb.half, out)) {
        return 0;
    }

    if (out) {
        fx_add3(point, box->pos, out->point);
        fx_copy3(out->point, point);
    }

    return 1;
}

static int ray_vs_obb(
    const sicol_shape_t* ray,
    const sicol_shape_t* box,
    sicol_hit_t* out
)
{
    fx rel_origin[3];
    fx local_origin[3];
    fx local_dir[3];
    sicol_hit_t local_hit;
    fx world_normal[3];
    fx world_point[3];

    fx_sub3(rel_origin, ray->pos, box->pos);
    fx_basis_to_local3(local_origin, box->u.obb.axis, rel_origin);
    fx_basis_to_local3(local_dir, box->u.obb.axis, ray->u.ray.dir);

    if (!ray_vs_aabb_local(local_origin, local_dir, ray->u.ray.length, box->u.obb.half, &local_hit)) {
        return 0;
    }

    if (out) {
        fx_basis_to_world3(world_point, box->u.obb.axis, local_hit.point);
        fx_add3(world_point, world_point, box->pos);
        fx_basis_to_world3(world_normal, box->u.obb.axis, local_hit.normal);
        rc_fill_hit(out, local_hit.t, world_point, world_normal);
    }

    return 1;
}

static int ray_vs_plane(
    const sicol_shape_t* ray,
    const sicol_shape_t* plane,
    sicol_hit_t* out
)
{
    fx denom;
    fx numer;
    fx t;
    fx point[3];

    denom = fx_dot3(plane->u.plane.normal, ray->u.ray.dir);
    if (FX_ABS(denom) <= FX_EPSILON) return 0;

    numer = -(fx_dot3(plane->u.plane.normal, ray->pos) + plane->u.plane.d);
    t = FX_DIV(numer, denom);

    if (t < 0 || t > ray->u.ray.length) return 0;

    fx_madd3(point, ray->pos, ray->u.ray.dir, t);
    rc_fill_hit(out, t, point, plane->u.plane.normal);
    return 1;
}

static int ray_vs_sphere(
    const sicol_shape_t* ray,
    const sicol_shape_t* sphere,
    sicol_hit_t* out
)
{
    fx m[3];
    fx b;
    fx c;
    fx discr;
    fx sqrt_discr;
    fx t;
    fx point[3];
    fx normal[3];

    fx_sub3(m, ray->pos, sphere->pos);

    b = fx_dot3(m, ray->u.ray.dir);
    c = fx_dot3(m, m) - FX_MUL(sphere->u.sphere.radius, sphere->u.sphere.radius);

    if (c > 0 && b > 0) return 0;

    discr = FX_MUL(b, b) - c;
    if (discr < 0) return 0;

    sqrt_discr = fx_sqrt(discr);
    t = -b - sqrt_discr;
    if (t < 0) t = 0;
    if (t > ray->u.ray.length) return 0;

    fx_madd3(point, ray->pos, ray->u.ray.dir, t);
    fx_sub3(normal, point, sphere->pos);
    if (!fx_normalize3(normal, normal)) {
        fx_set3(normal, FX_ONE, 0, 0);
    }
    rc_fill_hit(out, t, point, normal);
    return 1;
}

static fx rc_point_segment_dist_sq(const fx p[3], const fx a[3], const fx b[3], fx closest[3])
{
    fx_closest_point_segment3(closest, a, b, p);
    return fx_distance_sq3(p, closest);
}

static int ray_vs_capsule(
    const sicol_shape_t* ray,
    const sicol_shape_t* capsule,
    sicol_hit_t* out
)
{
    fx seg_a[3];
    fx seg_b[3];
    fx ray_end[3];
    fx pa[3];
    fx pb[3];
    fx s;
    fx t_seg;
    fx dist_sq;
    fx radius_sq;
    fx lo;
    fx hi;
    fx mid;
    fx point[3];
    fx closest[3];
    fx normal[3];
    fx t_hit;
    int i;

    sicol_shape_get_capsule_segment(capsule, seg_a, seg_b);
    fx_madd3(ray_end, ray->pos, ray->u.ray.dir, ray->u.ray.length);
    fx_closest_points_segment_segment3(pa, pb, ray->pos, ray_end, seg_a, seg_b, &s, &t_seg);

    radius_sq = FX_MUL(capsule->u.capsule.radius, capsule->u.capsule.radius);
    dist_sq = fx_distance_sq3(pa, pb);
    if (dist_sq > radius_sq) return 0;

    dist_sq = rc_point_segment_dist_sq(ray->pos, seg_a, seg_b, closest);
    if (dist_sq <= radius_sq) {
        fx_sub3(normal, ray->pos, closest);
        if (!fx_normalize3(normal, normal)) {
            fx_copy3(normal, capsule->u.capsule.axis);
        }
        rc_fill_hit(out, 0, ray->pos, normal);
        return 1;
    }

    hi = FX_MUL(ray->u.ray.length, s);
    if (hi < 0) hi = 0;
    if (hi > ray->u.ray.length) hi = ray->u.ray.length;
    lo = 0;

    for (i = 0; i < SICOL_RAYCAST_CAPSULE_BISECT; ++i) {
        mid = (lo + hi) / 2;
        rc_ray_point(ray, mid, point);
        dist_sq = rc_point_segment_dist_sq(point, seg_a, seg_b, closest);
        if (dist_sq <= radius_sq) {
            hi = mid;
        } else {
            lo = mid;
        }
    }

    t_hit = hi;
    rc_ray_point(ray, t_hit, point);
    rc_point_segment_dist_sq(point, seg_a, seg_b, closest);
    fx_sub3(normal, point, closest);
    if (!fx_normalize3(normal, normal)) {
        fx_copy3(normal, capsule->u.capsule.axis);
    }
    rc_fill_hit(out, t_hit, point, normal);
    return 1;
}

static int ray_vs_triangle(
    const sicol_shape_t* ray,
    const sicol_shape_t* tri,
    sicol_hit_t* out
)
{
    fx a[3];
    fx b[3];
    fx c[3];
    fx edge1[3];
    fx edge2[3];
    fx pvec[3];
    fx tvec[3];
    fx qvec[3];
    fx det;
    fx inv_det;
    fx u;
    fx v;
    fx t;
    fx point[3];
    fx normal[3];

    sicol_shape_get_triangle_points(tri, a, b, c);
    fx_sub3(edge1, b, a);
    fx_sub3(edge2, c, a);
    fx_cross3(pvec, ray->u.ray.dir, edge2);
    det = fx_dot3(edge1, pvec);
    if (FX_ABS(det) <= FX_FROM_RATIO(1, 4096)) return 0;

    inv_det = FX_DIV(FX_ONE, det);
    fx_sub3(tvec, ray->pos, a);
    u = FX_MUL(fx_dot3(tvec, pvec), inv_det);
    if (u < 0 || u > FX_ONE) return 0;

    fx_cross3(qvec, tvec, edge1);
    v = FX_MUL(fx_dot3(ray->u.ray.dir, qvec), inv_det);
    if (v < 0 || (u + v) > FX_ONE) return 0;

    t = FX_MUL(fx_dot3(edge2, qvec), inv_det);
    if (t < 0 || t > ray->u.ray.length) return 0;

    fx_madd3(point, ray->pos, ray->u.ray.dir, t);
    fx_cross3(normal, edge1, edge2);
    if (!fx_normalize3(normal, normal)) return 0;
    if (fx_dot3(normal, ray->u.ray.dir) > 0) {
        fx_neg3(normal, normal);
    }
    rc_fill_hit(out, t, point, normal);
    return 1;
}

static int ray_vs_mesh(
    const sicol_shape_t* ray,
    const sicol_shape_t* mesh_shape,
    sicol_hit_t* out
)
{
    fx local_origin[3];
    fx local_dir[3];
    fx local_point[3];
    fx local_normal[3];
    fx world_point[3];
    fx world_normal[3];
    fx t;
    int tri_index;

    if (!mesh_shape->u.mesh.mesh) return 0;
    sicol_shape_mesh_world_to_local(mesh_shape, ray->pos, local_origin);
    fx_basis_to_local3(local_dir, mesh_shape->u.mesh.axis, ray->u.ray.dir);
    if (!fx_normalize3(local_dir, local_dir)) {
        return 0;
    }
    if (!sicol_mesh_raycast_local(mesh_shape->u.mesh.mesh,
                                  local_origin,
                                  local_dir,
                                  ray->u.ray.length,
                                  &tri_index,
                                  &t,
                                  local_point,
                                  local_normal)) {
        return 0;
    }

    if (out) {
        sicol_shape_mesh_local_to_world(mesh_shape, local_point, world_point);
        fx_basis_to_world3(world_normal, mesh_shape->u.mesh.axis, local_normal);
        if (!fx_normalize3(world_normal, world_normal)) {
            fx_copy3(world_normal, local_normal);
        }
        rc_fill_hit(out, t, world_point, world_normal);
        out->shape_index = tri_index;
    }
    return 1;
}


int sicol_raycast_gjk(
    const sicol_shape_t* ray,
    const sicol_shape_t* target,
    sicol_gjk_cache_t* cache,
    sicol_hit_t* out
)
{
    sicol_gjk_raycast_result_t gjk_hit;

    if (!ray || !target) return 0;
    if (ray->type != SICOL_SHAPE_RAY) return 0;
    if (!sicol_shape_is_support_mapped(target)) return 0;

    if (!sicol_gjk_raycast_query(ray, target, cache, &gjk_hit)) return 0;
    if (!gjk_hit.hit) return 0;

    if (out) {
        out->t = FX_MUL(ray->u.ray.length, gjk_hit.fraction);
        fx_copy3(out->point, gjk_hit.point);
        fx_copy3(out->normal, gjk_hit.normal);
        out->shape_index = -1;
    }
    return 1;
}

int sicol_raycast(
    const sicol_shape_t* ray,
    const sicol_shape_t* target,
    sicol_hit_t* out
)
{
    if (!ray || !target) return 0;
    if (ray->type != SICOL_SHAPE_RAY) return 0;

    if (out) {
        out->t = 0;
        fx_zero3(out->point);
        fx_zero3(out->normal);
        out->shape_index = -1;
    }

    switch (target->type) {
    case SICOL_SHAPE_AABB:
        return ray_vs_aabb(ray, target, out);

    case SICOL_SHAPE_OBB:
        return ray_vs_obb(ray, target, out);

    case SICOL_SHAPE_PLANE:
        return ray_vs_plane(ray, target, out);

    case SICOL_SHAPE_SPHERE:
        return ray_vs_sphere(ray, target, out);

    case SICOL_SHAPE_CAPSULE:
        return ray_vs_capsule(ray, target, out);

    case SICOL_SHAPE_TRIANGLE:
        return ray_vs_triangle(ray, target, out);

    case SICOL_SHAPE_MESH:
        return ray_vs_mesh(ray, target, out);

    case SICOL_SHAPE_CONVEX:
    {
        sicol_gjk_raycast_result_t gjk_hit;
        sicol_gjk_cache_t cache;
        sicol_gjk_cache_reset(&cache);
        if (!sicol_gjk_raycast_query(ray, target, &cache, &gjk_hit)) return 0;
        if (out) {
            rc_fill_hit(out, FX_MUL(ray->u.ray.length, gjk_hit.fraction), gjk_hit.point, gjk_hit.normal);
        }
        return 1;
    }

    default:
        if (sicol_shape_is_support_mapped(target)) {
            sicol_gjk_raycast_result_t gjk_hit;
            sicol_gjk_cache_t cache;
            sicol_gjk_cache_reset(&cache);
            if (!sicol_gjk_raycast_query(ray, target, &cache, &gjk_hit)) return 0;
            if (out) {
                rc_fill_hit(out, FX_MUL(ray->u.ray.length, gjk_hit.fraction), gjk_hit.point, gjk_hit.normal);
            }
            return 1;
        }
        break;
    }

    return 0;
}
