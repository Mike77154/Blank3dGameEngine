#include "ccs_plane.h"

void ccs_plane_init(ccs_shape_plane* p, ccs_vec3 point, ccs_vec3 normal, ccs_u32 flags)
{
    if (!p) return;

    p->header.type = CCS_SHAPE_PLANE;
    p->header.flags = flags;

    p->point = point;
    p->normal = normal;
    if (!ccs_vec3_normalize_safe(&p->normal))
        p->normal = ccs_vec3_axis_y();

    p->dist = ccs_vec3_dot(p->normal, p->point);
}

void ccs_halfspace_init(ccs_shape_halfspace* p, ccs_vec3 point, ccs_vec3 normal, ccs_u32 flags)
{
    if (!p) return;

    p->header.type = CCS_SHAPE_HALFSPACE;
    p->header.flags = flags;

    p->point = point;
    p->normal = normal;
    if (!ccs_vec3_normalize_safe(&p->normal))
        p->normal = ccs_vec3_axis_y();

    p->dist = ccs_vec3_dot(p->normal, p->point);
}

/* ------------------------------------------------------------
   Plane collisions (two-sided)
   ------------------------------------------------------------ */

int ccs_plane_collide_sphere(const ccs_shape_plane* p, const ccs_sphere* s, ccs_contact* out)
{
    ccs_fixed sd;
    ccs_fixed ad;

    if (!p || !s || !out)
        return 0;

    sd = ccs_vec3_dot(p->normal, s->center) - p->dist;
    ad = ccs_fixed_abs(sd);

    if (ad > s->radius)
        return 0;

    out->penetration = s->radius - ad;
    out->normal = (sd >= 0) ? p->normal : ccs_vec3_neg(p->normal);
    return 1;
}

int ccs_plane_collide_capsule(const ccs_shape_plane* p, const ccs_capsule* c, ccs_contact* out)
{
    ccs_vec3 a;
    ccs_vec3 b;
    ccs_fixed d0;
    ccs_fixed d1;
    ccs_fixed dist;

    if (!p || !c || !out)
        return 0;

    ccs_capsule_endpoints(c, &a, &b);
    d0 = ccs_vec3_dot(p->normal, a) - p->dist;
    d1 = ccs_vec3_dot(p->normal, b) - p->dist;

    /* segment crosses plane => distance 0 */
    if ((d0 >= 0 && d1 <= 0) || (d0 <= 0 && d1 >= 0)) {
        dist = 0;
    } else {
        ccs_fixed ad0;
        ccs_fixed ad1;
        ad0 = ccs_fixed_abs(d0);
        ad1 = ccs_fixed_abs(d1);
        dist = (ad0 < ad1) ? ad0 : ad1;
    }

    if (dist > c->radius)
        return 0;

    out->penetration = c->radius - dist;

    /* orient using capsule center signed distance */
    {
        ccs_fixed sc;
        sc = ccs_vec3_dot(p->normal, c->center) - p->dist;
        out->normal = (sc >= 0) ? p->normal : ccs_vec3_neg(p->normal);
    }

    return 1;
}

int ccs_plane_collide_obb(const ccs_shape_plane* p, const ccs_obb* o, ccs_contact* out)
{
    ccs_fixed sc;
    ccs_fixed asc;
    ccs_fixed r;

    if (!p || !o || !out)
        return 0;

    sc = ccs_vec3_dot(p->normal, o->center) - p->dist;
    asc = ccs_fixed_abs(sc);

    r = ccs_fixed_mul(ccs_fixed_abs(ccs_vec3_dot(p->normal, o->axis[0])), o->half.x)
      + ccs_fixed_mul(ccs_fixed_abs(ccs_vec3_dot(p->normal, o->axis[1])), o->half.y)
      + ccs_fixed_mul(ccs_fixed_abs(ccs_vec3_dot(p->normal, o->axis[2])), o->half.z);

    if (asc > r)
        return 0;

    out->penetration = r - asc;
    out->normal = (sc >= 0) ? p->normal : ccs_vec3_neg(p->normal);
    return 1;
}

/* ------------------------------------------------------------
   Halfspace collisions (one-sided). Normal points OUT of solid.
   Constraint to satisfy: signed_dist >= radius_proj
   ------------------------------------------------------------ */

int ccs_halfspace_collide_sphere(const ccs_shape_halfspace* p, const ccs_sphere* s, ccs_contact* out)
{
    ccs_fixed sd;

    if (!p || !s || !out)
        return 0;

    sd = ccs_vec3_dot(p->normal, s->center) - p->dist;

    if (sd >= s->radius)
        return 0;

    out->penetration = s->radius - sd;
    out->normal = p->normal;
    return 1;
}

int ccs_halfspace_collide_capsule(const ccs_shape_halfspace* p, const ccs_capsule* c, ccs_contact* out)
{
    ccs_vec3 a;
    ccs_vec3 b;
    ccs_fixed d0;
    ccs_fixed d1;
    ccs_fixed dmin;

    if (!p || !c || !out)
        return 0;

    ccs_capsule_endpoints(c, &a, &b);
    d0 = ccs_vec3_dot(p->normal, a) - p->dist;
    d1 = ccs_vec3_dot(p->normal, b) - p->dist;

    dmin = (d0 < d1) ? d0 : d1;

    if (dmin >= c->radius)
        return 0;

    out->penetration = c->radius - dmin;
    out->normal = p->normal;
    return 1;
}

int ccs_halfspace_collide_obb(const ccs_shape_halfspace* p, const ccs_obb* o, ccs_contact* out)
{
    ccs_fixed sd;
    ccs_fixed r;

    if (!p || !o || !out)
        return 0;

    sd = ccs_vec3_dot(p->normal, o->center) - p->dist;

    r = ccs_fixed_mul(ccs_fixed_abs(ccs_vec3_dot(p->normal, o->axis[0])), o->half.x)
      + ccs_fixed_mul(ccs_fixed_abs(ccs_vec3_dot(p->normal, o->axis[1])), o->half.y)
      + ccs_fixed_mul(ccs_fixed_abs(ccs_vec3_dot(p->normal, o->axis[2])), o->half.z);

    if (sd >= r)
        return 0;

    out->penetration = r - sd;
    out->normal = p->normal;
    return 1;
}

/* ------------------------------------------------------------
   Raycast
   ------------------------------------------------------------ */

int ccs_plane_raycast(const ccs_shape_plane* p, const ccs_ray* ray, ccs_raycast_hit* out)
{
    ccs_fixed denom;
    ccs_fixed t;
    ccs_vec3 n;

    if (!p || !ray || !out)
        return 0;

    out->hit = 0;

    denom = ccs_vec3_dot(p->normal, ray->dir);
    if (denom == 0)
        return 0;

    t = ccs_fixed_div((p->dist - ccs_vec3_dot(p->normal, ray->origin)), denom);
    if (t < ray->tmin || t > ray->tmax)
        return 0;

    n = p->normal;
    if (ccs_vec3_dot(n, ray->dir) > 0)
        n = ccs_vec3_neg(n);

    out->hit = 1;
    out->t = t;
    out->point = ccs_vec3_add(ray->origin, ccs_vec3_scale(ray->dir, t));
    out->normal = n;
    out->feature_id = 0;

    return 1;
}

int ccs_halfspace_raycast(const ccs_shape_halfspace* p, const ccs_ray* ray, ccs_raycast_hit* out)
{
    ccs_fixed denom;
    ccs_fixed t;
    ccs_fixed s0;

    if (!p || !ray || !out)
        return 0;

    out->hit = 0;

    /* start must be outside solid */
    s0 = ccs_vec3_dot(p->normal, ray->origin) - p->dist;
    if (s0 <= 0)
        return 0;

    denom = ccs_vec3_dot(p->normal, ray->dir);
    if (denom == 0)
        return 0;

    /* must be moving toward solid (opposite normal) */
    if (denom >= 0)
        return 0;

    t = ccs_fixed_div((p->dist - ccs_vec3_dot(p->normal, ray->origin)), denom);
    if (t < ray->tmin || t > ray->tmax)
        return 0;

    out->hit = 1;
    out->t = t;
    out->point = ccs_vec3_add(ray->origin, ccs_vec3_scale(ray->dir, t));
    out->normal = p->normal; /* outward */
    out->feature_id = 0;
    return 1;
}
