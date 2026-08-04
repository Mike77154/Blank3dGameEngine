#include "ccs_raycast.h"

#include "ccs_shapes.h"
#include "ccs_math.h"
#include "ccs_narrow.h"

/* optional shape modules */
#if CCS_ENABLE_PLANE
#include "ccs_plane.h"
#endif

#if CCS_ENABLE_TRIMESH
#include "ccs_trimesh.h"
#endif

#if CCS_ENABLE_HEIGHTFIELD
#include "ccs_heightfield.h"
#endif

#if CCS_ENABLE_CONVEX
#include "ccs_convex.h"
#endif

#if CCS_ENABLE_COMPOUND
#include "ccs_compound.h"
#endif

/* ============================================================
   Helpers
   ============================================================ */

ccs_ray ccs_ray_make(ccs_vec3 origin, ccs_vec3 dir, ccs_fixed tmin, ccs_fixed tmax)
{
    ccs_ray r;
    r.origin = origin;
    r.dir    = dir;
    r.tmin   = tmin;
    r.tmax   = tmax;
    return r;
}


static ccs_fixed clamp01(ccs_fixed t)
{
    if (t < 0) return 0;
    if (t > CCS_FIXED_ONE) return CCS_FIXED_ONE;
    return t;
}

/* ============================================================
   Sphere raycast
   ============================================================ */

static int raycast_sphere(const ccs_ray* ray, const ccs_shape_sphere* s, ccs_raycast_hit* out)
{
    ccs_vec3 m;
    ccs_fixed b;
    ccs_fixed c;
    ccs_fixed discr;
    ccs_fixed t;

    out->hit = 0;

    /* m = o - c */
    m = ccs_vec3_sub(ray->origin, s->center);

    b = ccs_vec3_dot(m, ray->dir);
    c = ccs_vec3_dot(m, m) - ccs_fixed_mul(s->radius, s->radius);

    /* Ray origin outside sphere and pointing away? */
    if (c > 0 && b > 0)
        return 0;

    discr = ccs_fixed_mul(b, b) - c;
    if (discr < 0)
        return 0;

    t = -b - CCS_FIXED_SQRT(discr);

    if (t < ray->tmin)
        t = ray->tmin;

    if (t > ray->tmax)
        return 0;

    out->hit = 1;
    out->t = t;
    out->point = ccs_vec3_add(ray->origin, ccs_vec3_scale(ray->dir, t));
    out->normal = ccs_vec3_sub(out->point, s->center);
    ccs_vec3_normalize_safe(&out->normal);
    out->feature_id = 0;

    return 1;
}

/* ============================================================
   AABB raycast
   ============================================================ */

static int raycast_aabb(const ccs_ray* ray, const ccs_shape_box* b, ccs_raycast_hit* out)
{
    ccs_vec3 minv;
    ccs_vec3 maxv;

    ccs_fixed tmin;
    ccs_fixed tmax;

    out->hit = 0;

    minv = ccs_vec3_sub(b->center, b->half_extents);
    maxv = ccs_vec3_add(b->center, b->half_extents);

    tmin = ray->tmin;
    tmax = ray->tmax;

    /* X slab */
    if (ray->dir.x == 0) {
        if (ray->origin.x < minv.x || ray->origin.x > maxv.x)
            return 0;
    } else {
        ccs_fixed inv;
        ccs_fixed t0;
        ccs_fixed t1;
        inv = ccs_fixed_div(CCS_FIXED_ONE, ray->dir.x);
        t0 = ccs_fixed_mul((minv.x - ray->origin.x), inv);
        t1 = ccs_fixed_mul((maxv.x - ray->origin.x), inv);
        if (t0 > t1) { ccs_fixed tmp = t0; t0 = t1; t1 = tmp; }
        if (t0 > tmin) tmin = t0;
        if (t1 < tmax) tmax = t1;
        if (tmax < tmin) return 0;
    }

    /* Y slab */
    if (ray->dir.y == 0) {
        if (ray->origin.y < minv.y || ray->origin.y > maxv.y)
            return 0;
    } else {
        ccs_fixed inv;
        ccs_fixed t0;
        ccs_fixed t1;
        inv = ccs_fixed_div(CCS_FIXED_ONE, ray->dir.y);
        t0 = ccs_fixed_mul((minv.y - ray->origin.y), inv);
        t1 = ccs_fixed_mul((maxv.y - ray->origin.y), inv);
        if (t0 > t1) { ccs_fixed tmp = t0; t0 = t1; t1 = tmp; }
        if (t0 > tmin) tmin = t0;
        if (t1 < tmax) tmax = t1;
        if (tmax < tmin) return 0;
    }

    /* Z slab */
    if (ray->dir.z == 0) {
        if (ray->origin.z < minv.z || ray->origin.z > maxv.z)
            return 0;
    } else {
        ccs_fixed inv;
        ccs_fixed t0;
        ccs_fixed t1;
        inv = ccs_fixed_div(CCS_FIXED_ONE, ray->dir.z);
        t0 = ccs_fixed_mul((minv.z - ray->origin.z), inv);
        t1 = ccs_fixed_mul((maxv.z - ray->origin.z), inv);
        if (t0 > t1) { ccs_fixed tmp = t0; t0 = t1; t1 = tmp; }
        if (t0 > tmin) tmin = t0;
        if (t1 < tmax) tmax = t1;
        if (tmax < tmin) return 0;
    }

    if (tmin < ray->tmin)
        tmin = ray->tmin;
    if (tmin > ray->tmax)
        return 0;

    out->hit = 1;
    out->t = tmin;
    out->point = ccs_vec3_add(ray->origin, ccs_vec3_scale(ray->dir, tmin));

    /* estimate normal by closest face */
    {
        ccs_vec3 p;
        ccs_vec3 rel;
        ccs_fixed dx;
        ccs_fixed dy;
        ccs_fixed dz;

        p = out->point;
        rel = ccs_vec3_sub(p, b->center);

        dx = ccs_fixed_abs(ccs_fixed_abs(rel.x) - b->half_extents.x);
        dy = ccs_fixed_abs(ccs_fixed_abs(rel.y) - b->half_extents.y);
        dz = ccs_fixed_abs(ccs_fixed_abs(rel.z) - b->half_extents.z);

        if (dx <= dy && dx <= dz) {
            out->normal = ccs_vec3_make((rel.x >= 0) ? CCS_FIXED_ONE : -CCS_FIXED_ONE, 0, 0);
        } else if (dy <= dx && dy <= dz) {
            out->normal = ccs_vec3_make(0, (rel.y >= 0) ? CCS_FIXED_ONE : -CCS_FIXED_ONE, 0);
        } else {
            out->normal = ccs_vec3_make(0, 0, (rel.z >= 0) ? CCS_FIXED_ONE : -CCS_FIXED_ONE);
        }
    }

    out->feature_id = 0;

    return 1;
}

/* ============================================================
   Capsule raycast (segment + radius, using closest approach)
   ============================================================ */

static int raycast_capsule(const ccs_ray* ray, const ccs_shape_capsule* c, ccs_raycast_hit* out)
{
    ccs_capsule cap;
    ccs_vec3 a;
    ccs_vec3 b;

    out->hit = 0;

    cap.center = c->center;
    cap.axis = c->axis;
    cap.half_height = c->half_height;
    cap.radius = c->radius;

    ccs_capsule_endpoints(&cap, &a, &b);

    /*
        NOTE:
        This is a conservative approximation: we raycast a sphere at the closest point
        on the capsule segment to the ray origin. It is NOT an exact capsule raycast,
        but it is deterministic and avoids heavy math for fixed-point.

        For higher-quality capsule raycasts, implement a proper ray vs capsule solver.
    */
    {
        ccs_vec3 d;
        ccs_vec3 m;
        ccs_fixed dd;
        ccs_fixed md;
        ccs_fixed t;
        ccs_vec3 closest;
        ccs_shape_sphere sph;

        d = ccs_vec3_sub(b, a);
        m = ccs_vec3_sub(ray->origin, a);

        dd = ccs_vec3_dot(d, d);
        if (dd <= 0) {
            /* Degenerate capsule -> sphere at center */
            sph.header.type = CCS_SHAPE_SPHERE;
            sph.header.flags = 0;
            sph.center = c->center;
            sph.radius = c->radius;
            return raycast_sphere(ray, &sph, out);
        }

        md = ccs_vec3_dot(m, d);
        t = ccs_fixed_div(md, dd);
        t = clamp01(t);

        closest = ccs_vec3_add(a, ccs_vec3_scale(d, t));

        sph.header.type = CCS_SHAPE_SPHERE;
        sph.header.flags = 0;
        sph.center = closest;
        sph.radius = c->radius;
        return raycast_sphere(ray, &sph, out);
    }
}

/* ============================================================
   OBB raycast (transform ray to local, raycast AABB)
   ============================================================ */

static int raycast_obb(const ccs_ray* ray, const ccs_shape_obb* o, ccs_raycast_hit* out)
{
    ccs_ray local;
    ccs_shape_box box;
    ccs_raycast_hit h;

    out->hit = 0;

    local.origin = ccs_obb_to_local_point((const ccs_obb*)o, ray->origin);
    local.dir    = ccs_obb_to_local_dir((const ccs_obb*)o, ray->dir);
    local.tmin   = ray->tmin;
    local.tmax   = ray->tmax;

    box.header.type = CCS_SHAPE_BOX;
    box.header.flags = 0;
    box.center = ccs_vec3_zero();
    box.half_extents = o->half_extents;

    if (!raycast_aabb(&local, &box, &h))
        return 0;

    out->hit = 1;
    out->t = h.t;
    out->point = ccs_vec3_add(ray->origin, ccs_vec3_scale(ray->dir, h.t));

    /* transform normal back */
    out->normal = ccs_obb_to_world_dir((const ccs_obb*)o, h.normal);
    ccs_vec3_normalize_safe(&out->normal);

    out->feature_id = h.feature_id;

    return 1;
}

/* ============================================================
   Raycast dispatch
   ============================================================ */

int ccs_raycast_shape(const ccs_ray* ray, const void* shape, ccs_raycast_hit* out)
{
    const ccs_shape_header* h;

    if (!ray || !shape || !out)
        return 0;

    out->hit = 0;

    if (!ccs_shape_is_valid(shape))
        return 0;

    h = (const ccs_shape_header*)shape;

    switch (h->type) {
    case CCS_SHAPE_SPHERE:
        return raycast_sphere(ray, (const ccs_shape_sphere*)shape, out);

    case CCS_SHAPE_BOX:
        return raycast_aabb(ray, (const ccs_shape_box*)shape, out);

    case CCS_SHAPE_CAPSULE:
        return raycast_capsule(ray, (const ccs_shape_capsule*)shape, out);

    case CCS_SHAPE_OBB:
        return raycast_obb(ray, (const ccs_shape_obb*)shape, out);

#if CCS_ENABLE_PLANE
    case CCS_SHAPE_PLANE:
        return ccs_plane_raycast((const ccs_shape_plane*)shape, ray, out);

    case CCS_SHAPE_HALFSPACE:
        return ccs_halfspace_raycast((const ccs_shape_halfspace*)shape, ray, out);
#endif

#if CCS_ENABLE_TRIMESH
    case CCS_SHAPE_TRIMESH:
        return ccs_trimesh_raycast((const ccs_shape_trimesh*)shape, ray, out);
#endif

#if CCS_ENABLE_HEIGHTFIELD
    case CCS_SHAPE_HEIGHTFIELD:
        return ccs_heightfield_raycast((const ccs_shape_heightfield*)shape, ray, out);
#endif

#if CCS_ENABLE_CONVEX
    case CCS_SHAPE_CONVEX:
        return ccs_convex_raycast((const ccs_shape_convex*)shape, ray, out);
#endif

#if CCS_ENABLE_COMPOUND
    case CCS_SHAPE_COMPOUND:
        return ccs_compound_raycast((const ccs_shape_compound*)shape, ray, out);
#endif

    default:
        return 0;
    }
}
