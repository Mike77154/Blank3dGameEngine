#include "ccs_convex.h"

static void convex_local_aabb(ccs_shape_convex* cv)
{
    int i;

    if (!cv || !cv->vertices || cv->vertex_count <= 0) {
        if (cv) {
            cv->local_aabb.min = ccs_vec3_zero();
            cv->local_aabb.max = ccs_vec3_zero();
        }
        return;
    }

    cv->local_aabb.min = cv->vertices[0];
    cv->local_aabb.max = cv->vertices[0];

    for (i = 1; i < cv->vertex_count; ++i) {
        cv->local_aabb.min = ccs_vec3_min(cv->local_aabb.min, cv->vertices[i]);
        cv->local_aabb.max = ccs_vec3_max(cv->local_aabb.max, cv->vertices[i]);
    }
}

void ccs_convex_init(
    ccs_shape_convex* cv,
    const ccs_vec3* vertices,
    int vertex_count,
    const ccs_vec3* plane_normals,
    const ccs_fixed* plane_ds,
    int plane_count,
    ccs_vec3 center,
    ccs_u32 flags
) {
    if (!cv)
        return;

    cv->header.type = CCS_SHAPE_CONVEX;
    cv->header.flags = flags;

    cv->center = center;

    cv->vertices = vertices;
    cv->vertex_count = vertex_count;

    cv->plane_normals = plane_normals;
    cv->plane_ds = plane_ds;
    cv->plane_count = plane_count;

    convex_local_aabb(cv);
}

static void project_convex(const ccs_shape_convex* cv, ccs_vec3 axis, ccs_fixed* out_min, ccs_fixed* out_max)
{
    int i;
    ccs_fixed mn;
    ccs_fixed mx;

    mn = 0;
    mx = 0;

    if (!cv || !cv->vertices || cv->vertex_count <= 0) {
        *out_min = 0;
        *out_max = 0;
        return;
    }

    {
        ccs_vec3 v0;
        v0 = ccs_vec3_add(cv->vertices[0], cv->center);
        mn = mx = ccs_vec3_dot(axis, v0);
    }

    for (i = 1; i < cv->vertex_count; ++i) {
        ccs_fixed p;
        ccs_vec3 v;
        v = ccs_vec3_add(cv->vertices[i], cv->center);
        p = ccs_vec3_dot(axis, v);
        if (p < mn) mn = p;
        if (p > mx) mx = p;
    }

    *out_min = mn;
    *out_max = mx;
}

static void project_sphere(const ccs_sphere* s, ccs_vec3 axis, ccs_fixed* out_min, ccs_fixed* out_max)
{
    ccs_fixed c;
    c = ccs_vec3_dot(axis, s->center);
    *out_min = c - s->radius;
    *out_max = c + s->radius;
}

static void project_capsule(const ccs_capsule* c, ccs_vec3 axis, ccs_fixed* out_min, ccs_fixed* out_max)
{
    ccs_vec3 a;
    ccs_vec3 b;
    ccs_fixed p0;
    ccs_fixed p1;
    ccs_fixed mn;
    ccs_fixed mx;

    ccs_capsule_endpoints(c, &a, &b);
    p0 = ccs_vec3_dot(axis, a);
    p1 = ccs_vec3_dot(axis, b);

    mn = (p0 < p1 ? p0 : p1);
    mx = (p0 > p1 ? p0 : p1);

    *out_min = mn - c->radius;
    *out_max = mx + c->radius;
}

static void project_obb(const ccs_obb* o, ccs_vec3 axis, ccs_fixed* out_min, ccs_fixed* out_max)
{
    ccs_fixed c;
    ccs_fixed r;

    c = ccs_vec3_dot(axis, o->center);

    r = ccs_fixed_mul(ccs_fixed_abs(ccs_vec3_dot(axis, o->axis[0])), o->half.x)
      + ccs_fixed_mul(ccs_fixed_abs(ccs_vec3_dot(axis, o->axis[1])), o->half.y)
      + ccs_fixed_mul(ccs_fixed_abs(ccs_vec3_dot(axis, o->axis[2])), o->half.z);

    *out_min = c - r;
    *out_max = c + r;
}

static int sat_test_axis(
    ccs_vec3 axis,
    ccs_fixed a_min,
    ccs_fixed a_max,
    ccs_fixed b_min,
    ccs_fixed b_max,
    ccs_fixed* inout_best_overlap,
    ccs_vec3* inout_best_axis
) {
    ccs_fixed overlap;

    /* Disjoint? */
    if (a_max < b_min || b_max < a_min)
        return 0;

    /* overlap length */
    overlap = (a_max < b_max ? a_max : b_max) - (a_min > b_min ? a_min : b_min);

    if (overlap < *inout_best_overlap) {
        *inout_best_overlap = overlap;
        *inout_best_axis = axis;
    }

    return 1;
}

static int convex_sat_against(
    const ccs_shape_convex* cv,
    const void* shape,
    int shape_kind,
    ccs_contact* out
) {
    int i;
    ccs_fixed best_overlap;
    ccs_vec3 best_axis;
    ccs_vec3 center_b;

    if (!cv || !shape || !out)
        return 0;

    best_overlap = (ccs_fixed)CCS_I32_MAX;
    best_axis = ccs_vec3_axis_y();

    /* 1) convex face normals */
    for (i = 0; i < cv->plane_count; ++i) {
        ccs_vec3 axis;
        ccs_fixed a_min, a_max;
        ccs_fixed b_min, b_max;

        axis = cv->plane_normals[i];
        if (!ccs_vec3_normalize_safe(&axis))
            continue;

        project_convex(cv, axis, &a_min, &a_max);

        if (shape_kind == 0) {
            project_sphere((const ccs_sphere*)shape, axis, &b_min, &b_max);
        } else if (shape_kind == 1) {
            project_capsule((const ccs_capsule*)shape, axis, &b_min, &b_max);
        } else {
            project_obb((const ccs_obb*)shape, axis, &b_min, &b_max);
        }

        if (!sat_test_axis(axis, a_min, a_max, b_min, b_max, &best_overlap, &best_axis))
            return 0;
    }

    /* 2) additional axes for OBB (its basis) */
    if (shape_kind == 2) {
        const ccs_obb* o;
        o = (const ccs_obb*)shape;
        for (i = 0; i < 3; ++i) {
            ccs_vec3 axis;
            ccs_fixed a_min, a_max;
            ccs_fixed b_min, b_max;

            axis = o->axis[i];
            if (!ccs_vec3_normalize_safe(&axis))
                continue;

            project_convex(cv, axis, &a_min, &a_max);
            project_obb(o, axis, &b_min, &b_max);

            if (!sat_test_axis(axis, a_min, a_max, b_min, b_max, &best_overlap, &best_axis))
                return 0;
        }

        center_b = ((const ccs_obb*)shape)->center;
    } else if (shape_kind == 1) {
        center_b = ((const ccs_capsule*)shape)->center;
    } else {
        center_b = ((const ccs_sphere*)shape)->center;
    }

    /* Orient normal from convex -> shape */
    {
        ccs_vec3 delta;
        delta = ccs_vec3_sub(center_b, cv->center);
        if (ccs_vec3_dot(best_axis, delta) < 0)
            best_axis = ccs_vec3_neg(best_axis);
    }

    out->normal = best_axis;
    out->penetration = best_overlap;

    return 1;
}

int ccs_convex_collide_sphere(const ccs_shape_convex* cv, const ccs_sphere* s, ccs_contact* out)
{
    return convex_sat_against(cv, s, 0, out);
}

int ccs_convex_collide_capsule(const ccs_shape_convex* cv, const ccs_capsule* c, ccs_contact* out)
{
    return convex_sat_against(cv, c, 1, out);
}

int ccs_convex_collide_obb(const ccs_shape_convex* cv, const ccs_obb* o, ccs_contact* out)
{
    return convex_sat_against(cv, o, 2, out);
}

int ccs_convex_raycast(const ccs_shape_convex* cv, const ccs_ray* ray, ccs_raycast_hit* out)
{
    ccs_fixed t_enter;
    ccs_fixed t_exit;
    ccs_vec3 hit_normal;
    int hit_plane;
    int i;

    if (!cv || !ray || !out)
        return 0;

    out->hit = 0;

    t_enter = ray->tmin;
    t_exit = ray->tmax;
    hit_normal = ccs_vec3_axis_y();
    hit_plane = -1;

    for (i = 0; i < cv->plane_count; ++i) {
        ccs_vec3 n;
        ccs_fixed d_world;
        ccs_fixed dist0;
        ccs_fixed denom;

        n = cv->plane_normals[i];
        if (!ccs_vec3_normalize_safe(&n))
            continue;

        d_world = cv->plane_ds[i] + ccs_vec3_dot(n, cv->center);

        dist0 = ccs_vec3_dot(n, ray->origin) - d_world;
        denom = ccs_vec3_dot(n, ray->dir);

        if (denom == 0) {
            if (dist0 > 0)
                return 0; /* parallel outside */
            continue;
        }

        {
            ccs_fixed t;
            t = ccs_fixed_div(-dist0, denom);

            if (denom < 0) {
                /* entering */
                if (t > t_enter) {
                    t_enter = t;
                    hit_normal = n;
                    hit_plane = i;
                }
            } else {
                /* leaving */
                if (t < t_exit)
                    t_exit = t;
            }

            if (t_enter > t_exit)
                return 0;
        }
    }

    if (t_enter < ray->tmin || t_enter > ray->tmax)
        return 0;

    /* ensure normal opposes ray direction */
    if (ccs_vec3_dot(hit_normal, ray->dir) > 0)
        hit_normal = ccs_vec3_neg(hit_normal);

    out->hit = 1;
    out->t = t_enter;
    out->point = ccs_vec3_add(ray->origin, ccs_vec3_scale(ray->dir, t_enter));
    out->normal = hit_normal;
    out->feature_id = (hit_plane < 0 ? 0 : hit_plane);

    return 1;
}
