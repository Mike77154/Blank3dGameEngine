#include "ccs_dispatch.h"

#include "ccs_narrow.h"
#include "ccs_sat.h"

/* new shape modules (optional) */
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

#include <stddef.h>

static int is_valid(const void* s)
{
    return ccs_shape_is_valid(s);
}

static void invert_contact(ccs_contact* c)
{
    c->normal = ccs_vec3_neg(c->normal);
}

static void invert_manifold(ccs_manifold* m)
{
    int i;
    for (i = 0; i < m->count; ++i) {
        m->contacts[i].normal = ccs_vec3_neg(m->contacts[i].normal);
    }
}

static void to_sphere(const ccs_shape_sphere* s, ccs_sphere* out)
{
    out->center = s->center;
    out->radius = s->radius;
}

static void to_box(const ccs_shape_box* b, ccs_box* out)
{
    out->center = b->center;
    out->half_extents = b->half_extents;
}

static void to_capsule(const ccs_shape_capsule* c, ccs_capsule* out)
{
    out->center = c->center;
    out->axis = c->axis;
    out->half_height = c->half_height;
    out->radius = c->radius;
}

static void to_obb(const ccs_shape_obb* o, ccs_obb* out)
{
    out->center = o->center;
    out->half = o->half_extents;
    out->axis[0] = o->axis[0];
    out->axis[1] = o->axis[1];
    out->axis[2] = o->axis[2];
}

static void aabb_to_obb(const ccs_box* b, ccs_obb* out)
{
    out->center = b->center;
    out->half = b->half_extents;
    out->axis[0] = ccs_vec3_axis_x();
    out->axis[1] = ccs_vec3_axis_y();
    out->axis[2] = ccs_vec3_axis_z();
}

int ccs_dispatch_test(const void* a, const void* b)
{
    ccs_contact dummy;
    if (!is_valid(a) || !is_valid(b))
        return 0;
    return ccs_dispatch_collide(a, b, &dummy);
}

int ccs_dispatch_collide(const void* a, const void* b, ccs_contact* out)
{
    const ccs_shape_header* ha;
    const ccs_shape_header* hb;

    if (!out)
        return 0;
    if (!is_valid(a) || !is_valid(b))
        return 0;

    ha = (const ccs_shape_header*)a;
    hb = (const ccs_shape_header*)b;

    /* Compound first (so it can recurse) */
#if CCS_ENABLE_COMPOUND
    if (ha->type == CCS_SHAPE_COMPOUND) {
        return ccs_compound_collide((const ccs_shape_compound*)a, b, out);
    }
    if (hb->type == CCS_SHAPE_COMPOUND) {
        if (!ccs_compound_collide((const ccs_shape_compound*)b, a, out))
            return 0;
        invert_contact(out);
        return 1;
    }
#endif

    /* --------------------
       Sphere as A
       -------------------- */
    if (ha->type == CCS_SHAPE_SPHERE) {
        ccs_sphere sa;
        to_sphere((const ccs_shape_sphere*)a, &sa);

        if (hb->type == CCS_SHAPE_SPHERE) {
            ccs_sphere sb;
            to_sphere((const ccs_shape_sphere*)b, &sb);
            return ccs_narrow_sphere_sphere(&sa, &sb, out);
        }
        if (hb->type == CCS_SHAPE_BOX) {
            ccs_box bb;
            to_box((const ccs_shape_box*)b, &bb);
            return ccs_narrow_sphere_box(&sa, &bb, out);
        }
        if (hb->type == CCS_SHAPE_CAPSULE) {
            ccs_capsule cb;
            to_capsule((const ccs_shape_capsule*)b, &cb);
            return ccs_narrow_capsule_sphere(&cb, &sa, out);
        }
        if (hb->type == CCS_SHAPE_OBB) {
            ccs_obb ob;
            to_obb((const ccs_shape_obb*)b, &ob);
            return ccs_narrow_sphere_obb(&sa, &ob, out);
        }

        /* new targets */
#if CCS_ENABLE_PLANE
        if (hb->type == CCS_SHAPE_PLANE) {
            if (!ccs_plane_collide_sphere((const ccs_shape_plane*)b, &sa, out))
                return 0;
            invert_contact(out);
            return 1;
        }
#endif
#if CCS_ENABLE_PLANE
        if (hb->type == CCS_SHAPE_HALFSPACE) {
            if (!ccs_halfspace_collide_sphere((const ccs_shape_halfspace*)b, &sa, out))
                return 0;
            invert_contact(out);
            return 1;
        }
#endif
#if CCS_ENABLE_TRIMESH
        if (hb->type == CCS_SHAPE_TRIMESH) {
            if (!ccs_trimesh_collide_sphere((const ccs_shape_trimesh*)b, &sa, out))
                return 0;
            invert_contact(out);
            return 1;
        }
#endif
#if CCS_ENABLE_HEIGHTFIELD
        if (hb->type == CCS_SHAPE_HEIGHTFIELD) {
            if (!ccs_heightfield_collide_sphere((const ccs_shape_heightfield*)b, &sa, out))
                return 0;
            invert_contact(out);
            return 1;
        }
#endif
#if CCS_ENABLE_CONVEX
        if (hb->type == CCS_SHAPE_CONVEX) {
            if (!ccs_convex_collide_sphere((const ccs_shape_convex*)b, &sa, out))
                return 0;
            invert_contact(out);
            return 1;
        }
#endif

        return 0;
    }

    /* --------------------
       Box (AABB) as A
       -------------------- */
    if (ha->type == CCS_SHAPE_BOX) {
        ccs_box ba;
        to_box((const ccs_shape_box*)a, &ba);

        if (hb->type == CCS_SHAPE_SPHERE) {
            ccs_sphere sb;
            to_sphere((const ccs_shape_sphere*)b, &sb);
            if (!ccs_narrow_sphere_box(&sb, &ba, out))
                return 0;
            invert_contact(out);
            return 1;
        }
        if (hb->type == CCS_SHAPE_BOX) {
            ccs_box bb;
            to_box((const ccs_shape_box*)b, &bb);
            return ccs_narrow_box_box(&ba, &bb, out);
        }
        if (hb->type == CCS_SHAPE_CAPSULE) {
            ccs_capsule cb;
            ccs_obb obb_a;
            to_capsule((const ccs_shape_capsule*)b, &cb);
            aabb_to_obb(&ba, &obb_a);
            if (!ccs_narrow_capsule_obb(&cb, &obb_a, out))
                return 0;
            invert_contact(out);
            return 1;
        }
        if (hb->type == CCS_SHAPE_OBB) {
            ccs_obb oa;
            ccs_obb ob;
            aabb_to_obb(&ba, &oa);
            to_obb((const ccs_shape_obb*)b, &ob);
            return ccs_narrow_obb_obb(&oa, &ob, out);
        }

        /* new targets */
#if CCS_ENABLE_PLANE
        if (hb->type == CCS_SHAPE_PLANE) {
            ccs_obb oa;
            aabb_to_obb(&ba, &oa);
            if (!ccs_plane_collide_obb((const ccs_shape_plane*)b, &oa, out))
                return 0;
            invert_contact(out);
            return 1;
        }
#endif
#if CCS_ENABLE_PLANE
        if (hb->type == CCS_SHAPE_HALFSPACE) {
            ccs_obb oa;
            aabb_to_obb(&ba, &oa);
            if (!ccs_halfspace_collide_obb((const ccs_shape_halfspace*)b, &oa, out))
                return 0;
            invert_contact(out);
            return 1;
        }
#endif
#if CCS_ENABLE_TRIMESH
        if (hb->type == CCS_SHAPE_TRIMESH) {
            ccs_obb oa;
            aabb_to_obb(&ba, &oa);
            if (!ccs_trimesh_collide_obb((const ccs_shape_trimesh*)b, &oa, out))
                return 0;
            invert_contact(out);
            return 1;
        }
#endif
#if CCS_ENABLE_HEIGHTFIELD
        if (hb->type == CCS_SHAPE_HEIGHTFIELD) {
            ccs_obb oa;
            aabb_to_obb(&ba, &oa);
            if (!ccs_heightfield_collide_obb((const ccs_shape_heightfield*)b, &oa, out))
                return 0;
            invert_contact(out);
            return 1;
        }
#endif
#if CCS_ENABLE_CONVEX
        if (hb->type == CCS_SHAPE_CONVEX) {
            ccs_obb oa;
            aabb_to_obb(&ba, &oa);
            if (!ccs_convex_collide_obb((const ccs_shape_convex*)b, &oa, out))
                return 0;
            invert_contact(out);
            return 1;
        }
#endif

        return 0;
    }

    /* --------------------
       Capsule as A
       -------------------- */
    if (ha->type == CCS_SHAPE_CAPSULE) {
        ccs_capsule ca;
        to_capsule((const ccs_shape_capsule*)a, &ca);

        if (hb->type == CCS_SHAPE_SPHERE) {
            ccs_sphere sb;
            to_sphere((const ccs_shape_sphere*)b, &sb);
            return ccs_narrow_capsule_sphere(&ca, &sb, out);
        }
        if (hb->type == CCS_SHAPE_BOX) {
            ccs_box bb;
            ccs_obb obb_b;
            to_box((const ccs_shape_box*)b, &bb);
            aabb_to_obb(&bb, &obb_b);
            return ccs_narrow_capsule_obb(&ca, &obb_b, out);
        }
        if (hb->type == CCS_SHAPE_CAPSULE) {
            ccs_capsule cb;
            to_capsule((const ccs_shape_capsule*)b, &cb);
            return ccs_narrow_capsule_capsule(&ca, &cb, out);
        }
        if (hb->type == CCS_SHAPE_OBB) {
            ccs_obb ob;
            to_obb((const ccs_shape_obb*)b, &ob);
            return ccs_narrow_capsule_obb(&ca, &ob, out);
        }

        /* new targets */
#if CCS_ENABLE_PLANE
        if (hb->type == CCS_SHAPE_PLANE) {
            if (!ccs_plane_collide_capsule((const ccs_shape_plane*)b, &ca, out))
                return 0;
            invert_contact(out);
            return 1;
        }
#endif
#if CCS_ENABLE_PLANE
        if (hb->type == CCS_SHAPE_HALFSPACE) {
            if (!ccs_halfspace_collide_capsule((const ccs_shape_halfspace*)b, &ca, out))
                return 0;
            invert_contact(out);
            return 1;
        }
#endif
#if CCS_ENABLE_TRIMESH
        if (hb->type == CCS_SHAPE_TRIMESH) {
            if (!ccs_trimesh_collide_capsule((const ccs_shape_trimesh*)b, &ca, out))
                return 0;
            invert_contact(out);
            return 1;
        }
#endif
#if CCS_ENABLE_HEIGHTFIELD
        if (hb->type == CCS_SHAPE_HEIGHTFIELD) {
            if (!ccs_heightfield_collide_capsule((const ccs_shape_heightfield*)b, &ca, out))
                return 0;
            invert_contact(out);
            return 1;
        }
#endif
#if CCS_ENABLE_CONVEX
        if (hb->type == CCS_SHAPE_CONVEX) {
            if (!ccs_convex_collide_capsule((const ccs_shape_convex*)b, &ca, out))
                return 0;
            invert_contact(out);
            return 1;
        }
#endif

        return 0;
    }

    /* --------------------
       OBB as A
       -------------------- */
    if (ha->type == CCS_SHAPE_OBB) {
        ccs_obb oa;
        to_obb((const ccs_shape_obb*)a, &oa);

        if (hb->type == CCS_SHAPE_SPHERE) {
            ccs_sphere sb;
            to_sphere((const ccs_shape_sphere*)b, &sb);
            if (!ccs_narrow_sphere_obb(&sb, &oa, out))
                return 0;
            invert_contact(out);
            return 1;
        }
        if (hb->type == CCS_SHAPE_BOX) {
            ccs_box bb;
            ccs_obb ob;
            to_box((const ccs_shape_box*)b, &bb);
            aabb_to_obb(&bb, &ob);
            return ccs_narrow_obb_obb(&oa, &ob, out);
        }
        if (hb->type == CCS_SHAPE_CAPSULE) {
            ccs_capsule cb;
            to_capsule((const ccs_shape_capsule*)b, &cb);
            if (!ccs_narrow_capsule_obb(&cb, &oa, out))
                return 0;
            invert_contact(out);
            return 1;
        }
        if (hb->type == CCS_SHAPE_OBB) {
            ccs_obb ob;
            to_obb((const ccs_shape_obb*)b, &ob);
            return ccs_narrow_obb_obb(&oa, &ob, out);
        }

        /* new targets */
#if CCS_ENABLE_PLANE
        if (hb->type == CCS_SHAPE_PLANE) {
            if (!ccs_plane_collide_obb((const ccs_shape_plane*)b, &oa, out))
                return 0;
            invert_contact(out);
            return 1;
        }
#endif
#if CCS_ENABLE_PLANE
        if (hb->type == CCS_SHAPE_HALFSPACE) {
            if (!ccs_halfspace_collide_obb((const ccs_shape_halfspace*)b, &oa, out))
                return 0;
            invert_contact(out);
            return 1;
        }
#endif
#if CCS_ENABLE_TRIMESH
        if (hb->type == CCS_SHAPE_TRIMESH) {
            if (!ccs_trimesh_collide_obb((const ccs_shape_trimesh*)b, &oa, out))
                return 0;
            invert_contact(out);
            return 1;
        }
#endif
#if CCS_ENABLE_HEIGHTFIELD
        if (hb->type == CCS_SHAPE_HEIGHTFIELD) {
            if (!ccs_heightfield_collide_obb((const ccs_shape_heightfield*)b, &oa, out))
                return 0;
            invert_contact(out);
            return 1;
        }
#endif
#if CCS_ENABLE_CONVEX
        if (hb->type == CCS_SHAPE_CONVEX) {
            if (!ccs_convex_collide_obb((const ccs_shape_convex*)b, &oa, out))
                return 0;
            invert_contact(out);
            return 1;
        }
#endif

        return 0;
    }

    /* --------------------
       Plane as A
       -------------------- */
#if CCS_ENABLE_PLANE
    if (ha->type == CCS_SHAPE_PLANE) {
        const ccs_shape_plane* p;
        p = (const ccs_shape_plane*)a;

        if (hb->type == CCS_SHAPE_SPHERE) {
            ccs_sphere sb;
            to_sphere((const ccs_shape_sphere*)b, &sb);
            return ccs_plane_collide_sphere(p, &sb, out);
        }
        if (hb->type == CCS_SHAPE_CAPSULE) {
            ccs_capsule cb;
            to_capsule((const ccs_shape_capsule*)b, &cb);
            return ccs_plane_collide_capsule(p, &cb, out);
        }
        if (hb->type == CCS_SHAPE_BOX) {
            ccs_box bb;
            ccs_obb ob;
            to_box((const ccs_shape_box*)b, &bb);
            aabb_to_obb(&bb, &ob);
            return ccs_plane_collide_obb(p, &ob, out);
        }
        if (hb->type == CCS_SHAPE_OBB) {
            ccs_obb ob;
            to_obb((const ccs_shape_obb*)b, &ob);
            return ccs_plane_collide_obb(p, &ob, out);
        }
        return 0;
    }
#endif

    /* --------------------
       Halfspace as A
       -------------------- */
#if CCS_ENABLE_PLANE
    if (ha->type == CCS_SHAPE_HALFSPACE) {
        const ccs_shape_halfspace* p;
        p = (const ccs_shape_halfspace*)a;

        if (hb->type == CCS_SHAPE_SPHERE) {
            ccs_sphere sb;
            to_sphere((const ccs_shape_sphere*)b, &sb);
            return ccs_halfspace_collide_sphere(p, &sb, out);
        }
        if (hb->type == CCS_SHAPE_CAPSULE) {
            ccs_capsule cb;
            to_capsule((const ccs_shape_capsule*)b, &cb);
            return ccs_halfspace_collide_capsule(p, &cb, out);
        }
        if (hb->type == CCS_SHAPE_BOX) {
            ccs_box bb;
            ccs_obb ob;
            to_box((const ccs_shape_box*)b, &bb);
            aabb_to_obb(&bb, &ob);
            return ccs_halfspace_collide_obb(p, &ob, out);
        }
        if (hb->type == CCS_SHAPE_OBB) {
            ccs_obb ob;
            to_obb((const ccs_shape_obb*)b, &ob);
            return ccs_halfspace_collide_obb(p, &ob, out);
        }
#if CCS_ENABLE_CONVEX
        if (hb->type == CCS_SHAPE_CONVEX) {
            const ccs_shape_convex* cv;
            int i;
            ccs_fixed min_sd;
            int have;

            cv = (const ccs_shape_convex*)b;
            have = 0;
            min_sd = 0;

            for (i = 0; i < cv->vertex_count; ++i) {
                ccs_vec3 v;
                ccs_fixed sd;
                v = ccs_vec3_add(cv->vertices[i], cv->center);
                sd = ccs_vec3_dot(p->normal, v) - p->dist;
                if (!have || sd < min_sd) {
                    min_sd = sd;
                    have = 1;
                }
            }

            if (!have)
                return 0;

            if (min_sd >= 0)
                return 0;

            out->normal = p->normal;
            out->penetration = -min_sd;
            return 1;
        }
#endif
        return 0;
    }
#endif

    /* --------------------
       Trimesh as A
       -------------------- */
#if CCS_ENABLE_TRIMESH
    if (ha->type == CCS_SHAPE_TRIMESH) {
        const ccs_shape_trimesh* m;
        m = (const ccs_shape_trimesh*)a;

        if (hb->type == CCS_SHAPE_SPHERE) {
            ccs_sphere sb;
            to_sphere((const ccs_shape_sphere*)b, &sb);
            return ccs_trimesh_collide_sphere(m, &sb, out);
        }
        if (hb->type == CCS_SHAPE_CAPSULE) {
            ccs_capsule cb;
            to_capsule((const ccs_shape_capsule*)b, &cb);
            return ccs_trimesh_collide_capsule(m, &cb, out);
        }
        if (hb->type == CCS_SHAPE_BOX) {
            ccs_box bb;
            ccs_obb ob;
            to_box((const ccs_shape_box*)b, &bb);
            aabb_to_obb(&bb, &ob);
            return ccs_trimesh_collide_obb(m, &ob, out);
        }
        if (hb->type == CCS_SHAPE_OBB) {
            ccs_obb ob;
            to_obb((const ccs_shape_obb*)b, &ob);
            return ccs_trimesh_collide_obb(m, &ob, out);
        }
        return 0;
    }
#endif

    /* --------------------
       Heightfield as A
       -------------------- */
#if CCS_ENABLE_HEIGHTFIELD
    if (ha->type == CCS_SHAPE_HEIGHTFIELD) {
        const ccs_shape_heightfield* hf;
        hf = (const ccs_shape_heightfield*)a;

        if (hb->type == CCS_SHAPE_SPHERE) {
            ccs_sphere sb;
            to_sphere((const ccs_shape_sphere*)b, &sb);
            return ccs_heightfield_collide_sphere(hf, &sb, out);
        }
        if (hb->type == CCS_SHAPE_CAPSULE) {
            ccs_capsule cb;
            to_capsule((const ccs_shape_capsule*)b, &cb);
            return ccs_heightfield_collide_capsule(hf, &cb, out);
        }
        if (hb->type == CCS_SHAPE_BOX) {
            ccs_box bb;
            ccs_obb ob;
            to_box((const ccs_shape_box*)b, &bb);
            aabb_to_obb(&bb, &ob);
            return ccs_heightfield_collide_obb(hf, &ob, out);
        }
        if (hb->type == CCS_SHAPE_OBB) {
            ccs_obb ob;
            to_obb((const ccs_shape_obb*)b, &ob);
            return ccs_heightfield_collide_obb(hf, &ob, out);
        }
        return 0;
    }
#endif

    /* --------------------
       Convex as A
       -------------------- */
#if CCS_ENABLE_CONVEX
    if (ha->type == CCS_SHAPE_CONVEX) {
        const ccs_shape_convex* cv;
        cv = (const ccs_shape_convex*)a;

        if (hb->type == CCS_SHAPE_SPHERE) {
            ccs_sphere sb;
            to_sphere((const ccs_shape_sphere*)b, &sb);
            return ccs_convex_collide_sphere(cv, &sb, out);
        }
        if (hb->type == CCS_SHAPE_CAPSULE) {
            ccs_capsule cb;
            to_capsule((const ccs_shape_capsule*)b, &cb);
            return ccs_convex_collide_capsule(cv, &cb, out);
        }
        if (hb->type == CCS_SHAPE_BOX) {
            ccs_box bb;
            ccs_obb ob;
            to_box((const ccs_shape_box*)b, &bb);
            aabb_to_obb(&bb, &ob);
            return ccs_convex_collide_obb(cv, &ob, out);
        }
        if (hb->type == CCS_SHAPE_OBB) {
            ccs_obb ob;
            to_obb((const ccs_shape_obb*)b, &ob);
            return ccs_convex_collide_obb(cv, &ob, out);
        }
#if CCS_ENABLE_PLANE
        if (hb->type == CCS_SHAPE_HALFSPACE) {
            const ccs_shape_halfspace* hs;
            int i;
            ccs_fixed min_sd;
            int have;

            hs = (const ccs_shape_halfspace*)b;
            have = 0;
            min_sd = 0;

            for (i = 0; i < cv->vertex_count; ++i) {
                ccs_vec3 v;
                ccs_fixed sd;
                v = ccs_vec3_add(cv->vertices[i], cv->center);
                sd = ccs_vec3_dot(hs->normal, v) - hs->dist;
                if (!have || sd < min_sd) {
                    min_sd = sd;
                    have = 1;
                }
            }

            if (!have)
                return 0;

            if (min_sd >= 0)
                return 0;

            out->normal = ccs_vec3_neg(hs->normal); /* convex -> halfspace */
            out->penetration = -min_sd;
            return 1;
        }
#endif
        return 0;
    }
#endif

    return 0;
}

/* ============================================================
   Manifold dispatch
   ============================================================ */


/* ============================================================
   Manifold dispatch
   ============================================================ */

int ccs_dispatch_collide_manifold(const void* a, const void* b, ccs_manifold* out)
{
    const ccs_shape_header* ha;
    const ccs_shape_header* hb;

    if (!out)
        return 0;

    out->count = 0;

    if (!is_valid(a) || !is_valid(b))
        return 0;

    ha = (const ccs_shape_header*)a;
    hb = (const ccs_shape_header*)b;

    /* Fast paths (available manifold generators) */

    /* Box as A */
    if (ha->type == CCS_SHAPE_BOX) {
        ccs_box ba;
        to_box((const ccs_shape_box*)a, &ba);

        if (hb->type == CCS_SHAPE_BOX) {
            ccs_box bb;
            to_box((const ccs_shape_box*)b, &bb);
            return ccs_narrow_box_box_manifold(&ba, &bb, out);
        }

        if (hb->type == CCS_SHAPE_OBB) {
            ccs_obb oa;
            ccs_obb ob;
            aabb_to_obb(&ba, &oa);
            to_obb((const ccs_shape_obb*)b, &ob);
            return ccs_narrow_obb_obb_manifold(&oa, &ob, out);
        }

        if (hb->type == CCS_SHAPE_SPHERE) {
            ccs_sphere sb;
            ccs_obb oa;
            to_sphere((const ccs_shape_sphere*)b, &sb);
            aabb_to_obb(&ba, &oa);
            if (!ccs_narrow_sphere_obb_manifold(&sb, &oa, out))
                return 0;
            invert_manifold(out); /* box -> sphere */
            return 1;
        }

        if (hb->type == CCS_SHAPE_CAPSULE) {
            ccs_capsule cb;
            ccs_obb oa;
            to_capsule((const ccs_shape_capsule*)b, &cb);
            aabb_to_obb(&ba, &oa);
            if (!ccs_narrow_capsule_obb_manifold(&cb, &oa, out))
                return 0;
            invert_manifold(out); /* box -> capsule */
            return 1;
        }
    }

    /* Sphere as A */
    if (ha->type == CCS_SHAPE_SPHERE) {
        ccs_sphere sa;
        to_sphere((const ccs_shape_sphere*)a, &sa);

        if (hb->type == CCS_SHAPE_OBB) {
            ccs_obb ob;
            to_obb((const ccs_shape_obb*)b, &ob);
            return ccs_narrow_sphere_obb_manifold(&sa, &ob, out);
        }

        if (hb->type == CCS_SHAPE_BOX) {
            ccs_box bb;
            ccs_obb ob;
            to_box((const ccs_shape_box*)b, &bb);
            aabb_to_obb(&bb, &ob);
            return ccs_narrow_sphere_obb_manifold(&sa, &ob, out);
        }

        if (hb->type == CCS_SHAPE_CAPSULE) {
            ccs_capsule cb;
            to_capsule((const ccs_shape_capsule*)b, &cb);
            if (!ccs_narrow_capsule_sphere_manifold(&cb, &sa, out))
                return 0;
            invert_manifold(out); /* sphere -> capsule */
            return 1;
        }
    }

    /* Capsule as A */
    if (ha->type == CCS_SHAPE_CAPSULE) {
        ccs_capsule ca;
        to_capsule((const ccs_shape_capsule*)a, &ca);

        if (hb->type == CCS_SHAPE_SPHERE) {
            ccs_sphere sb;
            to_sphere((const ccs_shape_sphere*)b, &sb);
            return ccs_narrow_capsule_sphere_manifold(&ca, &sb, out);
        }
        if (hb->type == CCS_SHAPE_CAPSULE) {
            ccs_capsule cb;
            to_capsule((const ccs_shape_capsule*)b, &cb);
            return ccs_narrow_capsule_capsule_manifold(&ca, &cb, out);
        }
        if (hb->type == CCS_SHAPE_OBB) {
            ccs_obb ob;
            to_obb((const ccs_shape_obb*)b, &ob);
            return ccs_narrow_capsule_obb_manifold(&ca, &ob, out);
        }
        if (hb->type == CCS_SHAPE_BOX) {
            ccs_box bb;
            ccs_obb ob;
            to_box((const ccs_shape_box*)b, &bb);
            aabb_to_obb(&bb, &ob);
            return ccs_narrow_capsule_obb_manifold(&ca, &ob, out);
        }
    }

    /* OBB as A */
    if (ha->type == CCS_SHAPE_OBB) {
        ccs_obb oa;
        to_obb((const ccs_shape_obb*)a, &oa);

        if (hb->type == CCS_SHAPE_OBB) {
            ccs_obb ob;
            to_obb((const ccs_shape_obb*)b, &ob);
            return ccs_narrow_obb_obb_manifold(&oa, &ob, out);
        }
        if (hb->type == CCS_SHAPE_BOX) {
            ccs_box bb;
            ccs_obb ob;
            to_box((const ccs_shape_box*)b, &bb);
            aabb_to_obb(&bb, &ob);
            return ccs_narrow_obb_obb_manifold(&oa, &ob, out);
        }
        if (hb->type == CCS_SHAPE_SPHERE) {
            ccs_sphere sb;
            to_sphere((const ccs_shape_sphere*)b, &sb);
            if (!ccs_narrow_sphere_obb_manifold(&sb, &oa, out))
                return 0;
            invert_manifold(out); /* obb -> sphere */
            return 1;
        }
        if (hb->type == CCS_SHAPE_CAPSULE) {
            ccs_capsule cb;
            to_capsule((const ccs_shape_capsule*)b, &cb);
            if (!ccs_narrow_capsule_obb_manifold(&cb, &oa, out))
                return 0;
            invert_manifold(out); /* obb -> capsule */
            return 1;
        }
    }

    /* Fallback: generate single-contact manifold from ccs_dispatch_collide */
    {
        ccs_contact c;
        if (!ccs_dispatch_collide(a, b, &c))
            return 0;

        out->count = 1;
        out->contacts[0].normal = c.normal;
        out->contacts[0].penetration = c.penetration;
        out->contacts[0].point = ccs_vec3_scale(
            ccs_vec3_add(ccs_shape_get_center(a), ccs_shape_get_center(b)),
            CCS_FIXED_HALF
        );
        out->contacts[0].feature_id = 0;

        return 1;
    }
}

unsigned int ccs_dispatch_shape_caps(ccs_shape_type type)
{
    /* Only primitives have dedicated manifold generators. */
    switch (type) {
    case CCS_SHAPE_SPHERE:
    case CCS_SHAPE_BOX:
    case CCS_SHAPE_CAPSULE:
    case CCS_SHAPE_OBB:
        return CCS_SHAPE_SUPPORTS_MANIFOLD;
    default:
        return 0;
    }
}
