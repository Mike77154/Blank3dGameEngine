#include "ccs_shapes.h"

/* ============================================================
   Compute AABB from shape data
   ============================================================ */

static ccs_vec3 ccs_vec3_abs_comp(ccs_vec3 v)
{
    ccs_vec3 out;
    out.x = ccs_fixed_abs(v.x);
    out.y = ccs_fixed_abs(v.y);
    out.z = ccs_fixed_abs(v.z);
    return out;
}

static ccs_aabb ccs_aabb_shift(ccs_aabb a, ccs_vec3 t)
{
    ccs_aabb out;
    out.min = ccs_vec3_add(a.min, t);
    out.max = ccs_vec3_add(a.max, t);
    return out;
}

static ccs_aabb ccs_aabb_union(ccs_aabb a, ccs_aabb b)
{
    ccs_aabb out;
    out.min = ccs_vec3_min(a.min, b.min);
    out.max = ccs_vec3_max(a.max, b.max);
    return out;
}

ccs_aabb ccs_shape_compute_aabb(const void* shape)
{
    const ccs_shape_header* h;
    ccs_aabb aabb;

    h = (const ccs_shape_header*)shape;

    switch (h ? h->type : CCS_SHAPE_NONE) {
    case CCS_SHAPE_SPHERE:
    {
        const ccs_shape_sphere* s;
        ccs_vec3 r;

        s = (const ccs_shape_sphere*)shape;
        r.x = s->radius;
        r.y = s->radius;
        r.z = s->radius;

        aabb.min = ccs_vec3_sub(s->center, r);
        aabb.max = ccs_vec3_add(s->center, r);
        break;
    }

    case CCS_SHAPE_BOX:
    {
        const ccs_shape_box* b;

        b = (const ccs_shape_box*)shape;
        aabb.min = ccs_vec3_sub(b->center, b->half_extents);
        aabb.max = ccs_vec3_add(b->center, b->half_extents);
        break;
    }

    case CCS_SHAPE_CAPSULE:
    {
        const ccs_shape_capsule* c;
        ccs_vec3 off;
        ccs_vec3 a;
        ccs_vec3 b;
        ccs_vec3 r;
        ccs_vec3 minv;
        ccs_vec3 maxv;

        c = (const ccs_shape_capsule*)shape;
        off = ccs_vec3_scale(c->axis, c->half_height);
        a = ccs_vec3_sub(c->center, off);
        b = ccs_vec3_add(c->center, off);

        minv = ccs_vec3_min(a, b);
        maxv = ccs_vec3_max(a, b);

        r.x = c->radius;
        r.y = c->radius;
        r.z = c->radius;

        aabb.min = ccs_vec3_sub(minv, r);
        aabb.max = ccs_vec3_add(maxv, r);
        break;
    }

    case CCS_SHAPE_OBB:
    {
        const ccs_shape_obb* o;
        ccs_vec3 ax0;
        ccs_vec3 ax1;
        ccs_vec3 ax2;
        ccs_vec3 e;

        o = (const ccs_shape_obb*)shape;

        ax0 = ccs_vec3_abs_comp(o->axis[0]);
        ax1 = ccs_vec3_abs_comp(o->axis[1]);
        ax2 = ccs_vec3_abs_comp(o->axis[2]);

        /* world extents = sum(|axis_i| * half_i) */
        e.x = ccs_fixed_mul(ax0.x, o->half_extents.x)
            + ccs_fixed_mul(ax1.x, o->half_extents.y)
            + ccs_fixed_mul(ax2.x, o->half_extents.z);

        e.y = ccs_fixed_mul(ax0.y, o->half_extents.x)
            + ccs_fixed_mul(ax1.y, o->half_extents.y)
            + ccs_fixed_mul(ax2.y, o->half_extents.z);

        e.z = ccs_fixed_mul(ax0.z, o->half_extents.x)
            + ccs_fixed_mul(ax1.z, o->half_extents.y)
            + ccs_fixed_mul(ax2.z, o->half_extents.z);

        aabb.min = ccs_vec3_sub(o->center, e);
        aabb.max = ccs_vec3_add(o->center, e);
        break;
    }

    case CCS_SHAPE_PLANE:
    case CCS_SHAPE_HALFSPACE:
    {
        ccs_vec3 p;
        ccs_vec3 e;
        ccs_fixed he;

        he = ccs_fixed_from_int((ccs_i32)CCS_PLANE_AABB_HALF_EXTENTS);
        e.x = he; e.y = he; e.z = he;

        if (h->type == CCS_SHAPE_PLANE) {
            p = ((const ccs_shape_plane*)shape)->point;
        } else {
            p = ((const ccs_shape_halfspace*)shape)->point;
        }

        aabb.min = ccs_vec3_sub(p, e);
        aabb.max = ccs_vec3_add(p, e);
        break;
    }

    case CCS_SHAPE_TRIMESH:
    {
        const ccs_shape_trimesh* m;
        m = (const ccs_shape_trimesh*)shape;
        aabb = ccs_aabb_shift(m->local_aabb, m->center);
        break;
    }

    case CCS_SHAPE_HEIGHTFIELD:
    {
        const ccs_shape_heightfield* hf;
        ccs_vec3 mn;
        ccs_vec3 mx;
        ccs_fixed w;
        ccs_fixed d;

        hf = (const ccs_shape_heightfield*)shape;

        w = 0;
        d = 0;
        if (hf->width > 1)
            w = ccs_fixed_mul(hf->cell_size_x, ccs_fixed_from_int((ccs_i32)(hf->width - 1)));
        if (hf->depth > 1)
            d = ccs_fixed_mul(hf->cell_size_z, ccs_fixed_from_int((ccs_i32)(hf->depth - 1)));

        mn.x = hf->origin.x;
        mn.z = hf->origin.z;
        mx.x = hf->origin.x + w;
        mx.z = hf->origin.z + d;

        /* allow negative cell size */
        if (mn.x > mx.x) {
            ccs_fixed tmp = mn.x; mn.x = mx.x; mx.x = tmp;
        }
        if (mn.z > mx.z) {
            ccs_fixed tmp = mn.z; mn.z = mx.z; mx.z = tmp;
        }

        mn.y = hf->origin.y + hf->min_h;
        mx.y = hf->origin.y + hf->max_h;

        aabb.min = mn;
        aabb.max = mx;
        break;
    }

    case CCS_SHAPE_CONVEX:
    {
        const ccs_shape_convex* cv;
        cv = (const ccs_shape_convex*)shape;
        aabb = ccs_aabb_shift(cv->local_aabb, cv->center);
        break;
    }

    case CCS_SHAPE_COMPOUND:
    {
        const ccs_shape_compound* cp;
        int i;
        ccs_aabb out;
        int have;

        cp = (const ccs_shape_compound*)shape;

        have = 0;
        out.min = ccs_vec3_zero();
        out.max = ccs_vec3_zero();

        for (i = 0; i < cp->child_count; ++i) {
            const ccs_compound_child* ch;
            ccs_aabb ca;
            ccs_vec3 shift;

            ch = &cp->children[i];
            if (!ch->shape)
                continue;

            ca = ccs_shape_compute_aabb(ch->shape);
            shift = ccs_vec3_add(cp->center, ch->offset);
            ca = ccs_aabb_shift(ca, shift);

            if (!have) {
                out = ca;
                have = 1;
            } else {
                out = ccs_aabb_union(out, ca);
            }
        }

        if (!have) {
            out.min = cp->center;
            out.max = cp->center;
        }

        aabb = out;
        break;
    }

    default:
        aabb.min = ccs_vec3_zero();
        aabb.max = ccs_vec3_zero();
        break;
    }

    return aabb;
}

/* ============================================================
   Shape helpers
   ============================================================ */

int ccs_shape_is_valid(const void* shape)
{
    const ccs_shape_header* h;

    if (!shape)
        return 0;

    h = (const ccs_shape_header*)shape;

    return (h->type > CCS_SHAPE_NONE &&
            h->type < CCS_SHAPE_COUNT);
}

ccs_vec3 ccs_shape_get_center(const void* shape)
{
    const ccs_shape_header* h;

    if (!shape)
        return ccs_vec3_zero();

    h = (const ccs_shape_header*)shape;

    switch (h->type) {
    case CCS_SHAPE_SPHERE:
        return ((const ccs_shape_sphere*)shape)->center;

    case CCS_SHAPE_BOX:
        return ((const ccs_shape_box*)shape)->center;

    case CCS_SHAPE_CAPSULE:
        return ((const ccs_shape_capsule*)shape)->center;

    case CCS_SHAPE_OBB:
        return ((const ccs_shape_obb*)shape)->center;

    case CCS_SHAPE_PLANE:
        return ((const ccs_shape_plane*)shape)->point;

    case CCS_SHAPE_HALFSPACE:
        return ((const ccs_shape_halfspace*)shape)->point;

    case CCS_SHAPE_TRIMESH:
        return ((const ccs_shape_trimesh*)shape)->center;

    case CCS_SHAPE_HEIGHTFIELD:
        return ((const ccs_shape_heightfield*)shape)->origin;

    case CCS_SHAPE_CONVEX:
        return ((const ccs_shape_convex*)shape)->center;

    case CCS_SHAPE_COMPOUND:
        return ((const ccs_shape_compound*)shape)->center;

    default:
        return ccs_vec3_zero();
    }
}

void ccs_shape_set_center(void* shape, ccs_vec3 center)
{
    ccs_shape_header* h;

    if (!shape)
        return;

    h = (ccs_shape_header*)shape;

    switch (h->type) {
    case CCS_SHAPE_SPHERE:
        ((ccs_shape_sphere*)shape)->center = center;
        break;

    case CCS_SHAPE_BOX:
        ((ccs_shape_box*)shape)->center = center;
        break;

    case CCS_SHAPE_CAPSULE:
        ((ccs_shape_capsule*)shape)->center = center;
        break;

    case CCS_SHAPE_OBB:
        ((ccs_shape_obb*)shape)->center = center;
        break;

    case CCS_SHAPE_PLANE:
    {
        ccs_shape_plane* p;
        p = (ccs_shape_plane*)shape;
        p->point = center;
        p->dist = ccs_vec3_dot(p->normal, p->point);
        break;
    }

    case CCS_SHAPE_HALFSPACE:
    {
        ccs_shape_halfspace* p;
        p = (ccs_shape_halfspace*)shape;
        p->point = center;
        p->dist = ccs_vec3_dot(p->normal, p->point);
        break;
    }

    case CCS_SHAPE_TRIMESH:
        ((ccs_shape_trimesh*)shape)->center = center;
        break;

    case CCS_SHAPE_HEIGHTFIELD:
        ((ccs_shape_heightfield*)shape)->origin = center;
        break;

    case CCS_SHAPE_CONVEX:
        ((ccs_shape_convex*)shape)->center = center;
        break;

    case CCS_SHAPE_COMPOUND:
        ((ccs_shape_compound*)shape)->center = center;
        break;

    default:
        break;
    }
}

/* ============================================================
   AABB helpers
   ============================================================ */

ccs_vec3 ccs_aabb_center(ccs_aabb aabb)
{
    return ccs_vec3_scale(
        ccs_vec3_add(aabb.min, aabb.max),
        CCS_FIXED_HALF
    );
}

ccs_vec3 ccs_aabb_extents(ccs_aabb aabb)
{
    return ccs_vec3_scale(
        ccs_vec3_sub(aabb.max, aabb.min),
        CCS_FIXED_HALF
    );
}

int ccs_aabb_contains_point(ccs_aabb aabb, ccs_vec3 p)
{
    if (p.x < aabb.min.x || p.x > aabb.max.x) return 0;
    if (p.y < aabb.min.y || p.y > aabb.max.y) return 0;
    if (p.z < aabb.min.z || p.z > aabb.max.z) return 0;
    return 1;
}

/* ============================================================
   Flag helpers
   ============================================================ */

int ccs_shape_is_static(const void* shape)
{
    const ccs_shape_header* h;

    if (!shape)
        return 0;

    h = (const ccs_shape_header*)shape;
    return (h->flags & (ccs_u32)CCS_SHAPE_FLAG_STATIC) != 0u;
}

int ccs_shape_is_trigger(const void* shape)
{
    const ccs_shape_header* h;

    if (!shape)
        return 0;

    h = (const ccs_shape_header*)shape;
    return (h->flags & (ccs_u32)CCS_SHAPE_FLAG_TRIGGER) != 0u;
}
