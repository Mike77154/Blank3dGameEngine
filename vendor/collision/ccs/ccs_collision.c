#include "ccs_collision.h"

/* ============================================================
   Broadphase: AABB vs AABB
   ============================================================ */

static int ccs_aabb_overlap(const ccs_aabb* a, const ccs_aabb* b)
{
    if (a->max.x < b->min.x || a->min.x > b->max.x) return 0;
    if (a->max.y < b->min.y || a->min.y > b->max.y) return 0;
    if (a->max.z < b->min.z || a->min.z > b->max.z) return 0;
    return 1;
}

/* ============================================================
   Public collision entry point
   ============================================================ */

ccs_collision_result ccs_collide(
    const void* shape_a,
    const void* shape_b,
    ccs_u32 flags
)
{
    ccs_collision_result result;

    result.hit = 0;
    result.contact.normal = ccs_vec3_zero();
    result.contact.penetration = 0;

    if (!shape_a || !shape_b)
        return result;

    /* --------------------------------------------------------
       Broadphase
       -------------------------------------------------------- */

    if (!(flags & CCS_COLLIDE_SKIP_BROADPHASE)) {
        ccs_aabb aabb_a = ccs_shape_compute_aabb(shape_a);
        ccs_aabb aabb_b = ccs_shape_compute_aabb(shape_b);

        if (!ccs_aabb_overlap(&aabb_a, &aabb_b))
            return result;
    }

    /* --------------------------------------------------------
       Narrowphase via dispatch
       -------------------------------------------------------- */

    if (ccs_dispatch_collide(shape_a, shape_b, &result.contact)) {
        result.hit = 1;
    }

    return result;
}

/* ============================================================
   Boolean overlap query
   ============================================================ */

int ccs_overlap(
    const void* shape_a,
    const void* shape_b,
    ccs_u32 flags
)
{
    if (!shape_a || !shape_b)
        return 0;

    if (!(flags & CCS_COLLIDE_SKIP_BROADPHASE)) {
        ccs_aabb aabb_a = ccs_shape_compute_aabb(shape_a);
        ccs_aabb aabb_b = ccs_shape_compute_aabb(shape_b);

        if (!ccs_aabb_overlap(&aabb_a, &aabb_b))
            return 0;
    }

    return ccs_dispatch_test(shape_a, shape_b);
}

/* ============================================================
   Manifold collision entry
   ============================================================ */

ccs_collision_manifold_result ccs_collide_manifold(
    const void* shape_a,
    const void* shape_b,
    ccs_u32 flags
)
{
    ccs_collision_manifold_result result;

    result.hit = 0;
    result.manifold.count = 0;

    if (!shape_a || !shape_b)
        return result;

    if (!(flags & CCS_COLLIDE_SKIP_BROADPHASE)) {
        ccs_aabb aabb_a = ccs_shape_compute_aabb(shape_a);
        ccs_aabb aabb_b = ccs_shape_compute_aabb(shape_b);

        if (!ccs_aabb_overlap(&aabb_a, &aabb_b))
            return result;
    }

    if (ccs_dispatch_collide_manifold(shape_a, shape_b, &result.manifold)) {
        result.hit = 1;
        return result;
    }

    /* fallback: single contact -> manifold de 1 punto */
    if (flags & CCS_COLLIDE_PREFER_MANIFOLD) {
        ccs_contact contact;
        if (ccs_dispatch_collide(shape_a, shape_b, &contact)) {
            result.hit = 1;
            result.manifold.count = 1;
            result.manifold.contacts[0].normal = contact.normal;
            result.manifold.contacts[0].penetration = contact.penetration;
            {
                ccs_vec3 ca = ccs_shape_get_center(shape_a);
                ccs_vec3 cb = ccs_shape_get_center(shape_b);
                result.manifold.contacts[0].point = ccs_vec3_scale(
                    ccs_vec3_add(ca, cb),
                    CCS_FIXED_HALF
                );
            }
            result.manifold.contacts[0].feature_id = 0;
        }
    }

    return result;
}

/* ============================================================
   Shape capability passthrough
   ============================================================ */

unsigned int ccs_shape_caps(const void* shape)
{
    const ccs_shape_header* h;

    if (!shape)
        return 0;

    h = (const ccs_shape_header*)shape;
    return ccs_dispatch_shape_caps(h->type);
}

ccs_collision_manifold_result ccs_collide_manifold_warmstart(
    const void* shape_a,
    const void* shape_b,
    const ccs_manifold* prev,
    ccs_u32 flags
) {
    ccs_collision_manifold_result r;

    r = ccs_collide_manifold(shape_a, shape_b, flags);
    if (r.hit && prev)
        ccs_manifold_warm_start(&r.manifold, prev);
    return r;
}
