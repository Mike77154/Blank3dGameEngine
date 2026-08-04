#include "ccs_compound.h"

#include "ccs_dispatch.h"
#include "ccs_raycast.h"

static ccs_vec3 add_shift(ccs_vec3 a, ccs_vec3 b)
{
    return ccs_vec3_add(a, b);
}

typedef union {
    ccs_shape_sphere sphere;
    ccs_shape_box box;
    ccs_shape_capsule capsule;
    ccs_shape_obb obb;
    ccs_shape_plane plane;
    ccs_shape_halfspace halfspace;
    ccs_shape_trimesh trimesh;
    ccs_shape_heightfield heightfield;
    ccs_shape_convex convex;
    ccs_shape_compound compound;
} ccs_shape_any;

static const void* copy_child_shifted(const ccs_shape* child, ccs_vec3 shift, ccs_shape_any* tmp)
{
    const ccs_shape_header* h;

    if (!child || !tmp)
        return 0;

    h = (const ccs_shape_header*)child;

    switch (h->type) {
    case CCS_SHAPE_SPHERE:
        tmp->sphere = *(const ccs_shape_sphere*)child;
        tmp->sphere.center = add_shift(tmp->sphere.center, shift);
        return &tmp->sphere;

    case CCS_SHAPE_BOX:
        tmp->box = *(const ccs_shape_box*)child;
        tmp->box.center = add_shift(tmp->box.center, shift);
        return &tmp->box;

    case CCS_SHAPE_CAPSULE:
        tmp->capsule = *(const ccs_shape_capsule*)child;
        tmp->capsule.center = add_shift(tmp->capsule.center, shift);
        return &tmp->capsule;

    case CCS_SHAPE_OBB:
        tmp->obb = *(const ccs_shape_obb*)child;
        tmp->obb.center = add_shift(tmp->obb.center, shift);
        return &tmp->obb;

    case CCS_SHAPE_PLANE:
        tmp->plane = *(const ccs_shape_plane*)child;
        tmp->plane.point = add_shift(tmp->plane.point, shift);
        tmp->plane.dist = ccs_vec3_dot(tmp->plane.normal, tmp->plane.point);
        return &tmp->plane;

    case CCS_SHAPE_HALFSPACE:
        tmp->halfspace = *(const ccs_shape_halfspace*)child;
        tmp->halfspace.point = add_shift(tmp->halfspace.point, shift);
        tmp->halfspace.dist = ccs_vec3_dot(tmp->halfspace.normal, tmp->halfspace.point);
        return &tmp->halfspace;

    case CCS_SHAPE_TRIMESH:
        tmp->trimesh = *(const ccs_shape_trimesh*)child;
        tmp->trimesh.center = add_shift(tmp->trimesh.center, shift);
        return &tmp->trimesh;

    case CCS_SHAPE_HEIGHTFIELD:
        tmp->heightfield = *(const ccs_shape_heightfield*)child;
        tmp->heightfield.origin = add_shift(tmp->heightfield.origin, shift);
        return &tmp->heightfield;

    case CCS_SHAPE_CONVEX:
        tmp->convex = *(const ccs_shape_convex*)child;
        tmp->convex.center = add_shift(tmp->convex.center, shift);
        return &tmp->convex;

    case CCS_SHAPE_COMPOUND:
        tmp->compound = *(const ccs_shape_compound*)child;
        tmp->compound.center = add_shift(tmp->compound.center, shift);
        return &tmp->compound;

    default:
        return 0;
    }
}

void ccs_compound_init(ccs_shape_compound* cp, ccs_vec3 center, ccs_u32 flags)
{
    int i;
    if (!cp) return;

    cp->header.type = CCS_SHAPE_COMPOUND;
    cp->header.flags = flags;
    cp->center = center;
    cp->child_count = 0;

    for (i = 0; i < CCS_COMPOUND_MAX_CHILDREN; ++i) {
        cp->children[i].shape = 0;
        cp->children[i].offset = ccs_vec3_zero();
    }
}

int ccs_compound_add_child(ccs_shape_compound* cp, const ccs_shape* child_shape, ccs_vec3 offset)
{
    int idx;
    if (!cp || !child_shape)
        return 0;

    if (cp->child_count >= CCS_COMPOUND_MAX_CHILDREN)
        return 0;

    idx = cp->child_count;
    cp->child_count++;

    cp->children[idx].shape = child_shape;
    cp->children[idx].offset = offset;

    return 1;
}

int ccs_compound_collide(const ccs_shape_compound* cp, const void* other_shape, ccs_contact* out)
{
    int i;
    int found;
    ccs_fixed best_pen;

    if (!cp || !other_shape || !out)
        return 0;

    found = 0;
    best_pen = 0;

    for (i = 0; i < cp->child_count; ++i) {
        const ccs_compound_child* ch;
        ccs_shape_any tmp;
        const void* child_world;
        ccs_contact c;
        ccs_vec3 shift;

        ch = &cp->children[i];
        if (!ch->shape)
            continue;

        shift = ccs_vec3_add(cp->center, ch->offset);
        child_world = copy_child_shifted(ch->shape, shift, &tmp);
        if (!child_world)
            continue;

        if (ccs_dispatch_collide(child_world, other_shape, &c)) {
            if (!found || c.penetration > best_pen) {
                found = 1;
                best_pen = c.penetration;
                *out = c;
            }
        }
    }

    return found;
}

int ccs_compound_raycast(const ccs_shape_compound* cp, const ccs_ray* ray, ccs_raycast_hit* out)
{
    int i;
    int found;
    ccs_fixed best_t;

    if (!cp || !ray || !out)
        return 0;

    out->hit = 0;

    found = 0;
    best_t = ray->tmax;

    for (i = 0; i < cp->child_count; ++i) {
        const ccs_compound_child* ch;
        ccs_shape_any tmp;
        const void* child_world;
        ccs_raycast_hit h;
        ccs_vec3 shift;

        ch = &cp->children[i];
        if (!ch->shape)
            continue;

        shift = ccs_vec3_add(cp->center, ch->offset);
        child_world = copy_child_shifted(ch->shape, shift, &tmp);
        if (!child_world)
            continue;

        if (ccs_raycast_shape(ray, child_world, &h)) {
            if (!found || h.t < best_t) {
                found = 1;
                best_t = h.t;
                *out = h;
            }
        }
    }

    return found;
}
