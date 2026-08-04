#include "ccs_heightfield.h"
#include "ccs_triangle.h"

static int clamp_int(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static int aabb_overlap(const ccs_aabb* a, const ccs_aabb* b)
{
    if (a->max.x < b->min.x || a->min.x > b->max.x) return 0;
    if (a->max.y < b->min.y || a->min.y > b->max.y) return 0;
    if (a->max.z < b->min.z || a->min.z > b->max.z) return 0;
    return 1;
}

static ccs_fixed hf_height(const ccs_shape_heightfield* hf, int i, int j)
{
    int idx;
    if (!hf || !hf->heights) return 0;
    if (i < 0) i = 0;
    if (j < 0) j = 0;
    if (i >= hf->width) i = hf->width - 1;
    if (j >= hf->depth) j = hf->depth - 1;
    idx = i + j * hf->width;
    return hf->heights[idx];
}

void ccs_heightfield_init(
    ccs_shape_heightfield* hf,
    ccs_vec3 origin,
    int width,
    int depth,
    ccs_fixed cell_size_x,
    ccs_fixed cell_size_z,
    const ccs_fixed* heights,
    ccs_u32 flags
) {
    int i;
    int n;

    if (!hf)
        return;

    hf->header.type = CCS_SHAPE_HEIGHTFIELD;
    hf->header.flags = flags;

    hf->origin = origin;
    hf->width = width;
    hf->depth = depth;
    hf->cell_size_x = cell_size_x;
    hf->cell_size_z = cell_size_z;
    hf->heights = heights;

    hf->min_h = 0;
    hf->max_h = 0;

    n = 0;
    if (heights && width > 0 && depth > 0)
        n = width * depth;

    if (n > 0) {
        ccs_fixed mn;
        ccs_fixed mx;
        mn = heights[0];
        mx = heights[0];
        for (i = 1; i < n; ++i) {
            if (heights[i] < mn) mn = heights[i];
            if (heights[i] > mx) mx = heights[i];
        }
        hf->min_h = mn;
        hf->max_h = mx;
    }
}

static void hf_cell_triangles(
    const ccs_shape_heightfield* hf,
    int i,
    int j,
    ccs_triangle* out0,
    ccs_triangle* out1
) {
    ccs_fixed x0;
    ccs_fixed x1;
    ccs_fixed z0;
    ccs_fixed z1;

    ccs_fixed y00;
    ccs_fixed y10;
    ccs_fixed y01;
    ccs_fixed y11;

    ccs_vec3 p00;
    ccs_vec3 p10;
    ccs_vec3 p01;
    ccs_vec3 p11;

    x0 = hf->origin.x + ccs_fixed_mul(hf->cell_size_x, ccs_fixed_from_int((ccs_i32)i));
    x1 = hf->origin.x + ccs_fixed_mul(hf->cell_size_x, ccs_fixed_from_int((ccs_i32)(i + 1)));
    z0 = hf->origin.z + ccs_fixed_mul(hf->cell_size_z, ccs_fixed_from_int((ccs_i32)j));
    z1 = hf->origin.z + ccs_fixed_mul(hf->cell_size_z, ccs_fixed_from_int((ccs_i32)(j + 1)));

    y00 = hf->origin.y + hf_height(hf, i, j);
    y10 = hf->origin.y + hf_height(hf, i + 1, j);
    y01 = hf->origin.y + hf_height(hf, i, j + 1);
    y11 = hf->origin.y + hf_height(hf, i + 1, j + 1);

    p00 = ccs_vec3_make(x0, y00, z0);
    p10 = ccs_vec3_make(x1, y10, z0);
    p01 = ccs_vec3_make(x0, y01, z1);
    p11 = ccs_vec3_make(x1, y11, z1);

    /* T0: p00, p10, p01 */
    out0->a = p00;
    out0->b = p10;
    out0->c = p01;

    /* T1: p10, p11, p01 */
    out1->a = p10;
    out1->b = p11;
    out1->c = p01;
}

static void hf_cell_range_for_aabb(
    const ccs_shape_heightfield* hf,
    const ccs_aabb* query,
    int* out_i0,
    int* out_i1,
    int* out_j0,
    int* out_j1
) {
    ccs_fixed lx0;
    ccs_fixed lx1;
    ccs_fixed lz0;
    ccs_fixed lz1;

    int i0;
    int i1;
    int j0;
    int j1;

    if (!hf || !query) {
        *out_i0 = *out_i1 = 0;
        *out_j0 = *out_j1 = 0;
        return;
    }

    lx0 = query->min.x - hf->origin.x;
    lx1 = query->max.x - hf->origin.x;
    lz0 = query->min.z - hf->origin.z;
    lz1 = query->max.z - hf->origin.z;

    if (hf->cell_size_x != 0) {
        i0 = ccs_fixed_to_int_floor(ccs_fixed_div(lx0, hf->cell_size_x));
        i1 = ccs_fixed_to_int_floor(ccs_fixed_div(lx1, hf->cell_size_x));
    } else {
        i0 = 0; i1 = 0;
    }

    if (hf->cell_size_z != 0) {
        j0 = ccs_fixed_to_int_floor(ccs_fixed_div(lz0, hf->cell_size_z));
        j1 = ccs_fixed_to_int_floor(ccs_fixed_div(lz1, hf->cell_size_z));
    } else {
        j0 = 0; j1 = 0;
    }

    if (hf->width < 2) {
        i0 = i1 = 0;
    } else {
        i0 = clamp_int(i0, 0, hf->width - 2);
        i1 = clamp_int(i1, 0, hf->width - 2);
    }

    if (hf->depth < 2) {
        j0 = j1 = 0;
    } else {
        j0 = clamp_int(j0, 0, hf->depth - 2);
        j1 = clamp_int(j1, 0, hf->depth - 2);
    }

    if (i0 > i1) { int tmp = i0; i0 = i1; i1 = tmp; }
    if (j0 > j1) { int tmp = j0; j0 = j1; j1 = tmp; }

    *out_i0 = i0;
    *out_i1 = i1;
    *out_j0 = j0;
    *out_j1 = j1;
}

static int hf_collide_triangles(
    const ccs_shape_heightfield* hf,
    const ccs_aabb* query_aabb,
    int (*tri_contact_fn)(const ccs_triangle*, const void*, ccs_contact*),
    const void* shape,
    ccs_contact* out
) {
    ccs_aabb hf_aabb;
    int i0, i1, j0, j1;
    int i, j;
    int found;
    ccs_fixed best_pen;

    if (!hf || !query_aabb || !tri_contact_fn || !shape || !out)
        return 0;

    hf_aabb = ccs_shape_compute_aabb(hf);
    if (!aabb_overlap(&hf_aabb, query_aabb))
        return 0;

    hf_cell_range_for_aabb(hf, query_aabb, &i0, &i1, &j0, &j1);

    found = 0;
    best_pen = 0;

    for (j = j0; j <= j1; ++j) {
        for (i = i0; i <= i1; ++i) {
            ccs_triangle t0;
            ccs_triangle t1;
            ccs_contact c;

            hf_cell_triangles(hf, i, j, &t0, &t1);

            if (tri_contact_fn(&t0, shape, &c)) {
                if (!found || c.penetration > best_pen) {
                    found = 1;
                    best_pen = c.penetration;
                    *out = c;
                }
            }

            if (tri_contact_fn(&t1, shape, &c)) {
                if (!found || c.penetration > best_pen) {
                    found = 1;
                    best_pen = c.penetration;
                    *out = c;
                }
            }
        }
    }

    return found;
}

/* wrappers so we can pass generic pointer */
static int tri_sphere_wrap(const ccs_triangle* tri, const void* shape, ccs_contact* out)
{
    return ccs_triangle_sphere_contact(tri, (const ccs_sphere*)shape, out);
}

static int tri_capsule_wrap(const ccs_triangle* tri, const void* shape, ccs_contact* out)
{
    return ccs_triangle_capsule_contact(tri, (const ccs_capsule*)shape, out);
}

static int tri_obb_wrap(const ccs_triangle* tri, const void* shape, ccs_contact* out)
{
    return ccs_triangle_obb_contact(tri, (const ccs_obb*)shape, out);
}

int ccs_heightfield_collide_sphere(const ccs_shape_heightfield* hf, const ccs_sphere* s, ccs_contact* out)
{
    ccs_aabb q;
    ccs_vec3 r;

    if (!hf || !s || !out)
        return 0;

    r.x = s->radius; r.y = s->radius; r.z = s->radius;
    q.min = ccs_vec3_sub(s->center, r);
    q.max = ccs_vec3_add(s->center, r);

    return hf_collide_triangles(hf, &q, tri_sphere_wrap, s, out);
}

int ccs_heightfield_collide_capsule(const ccs_shape_heightfield* hf, const ccs_capsule* c, ccs_contact* out)
{
    ccs_aabb q;
    ccs_vec3 a, b;
    ccs_vec3 mn, mx;
    ccs_vec3 r;

    if (!hf || !c || !out)
        return 0;

    ccs_capsule_endpoints(c, &a, &b);
    mn = ccs_vec3_min(a, b);
    mx = ccs_vec3_max(a, b);

    r.x = c->radius; r.y = c->radius; r.z = c->radius;

    q.min = ccs_vec3_sub(mn, r);
    q.max = ccs_vec3_add(mx, r);

    return hf_collide_triangles(hf, &q, tri_capsule_wrap, c, out);
}

int ccs_heightfield_collide_obb(const ccs_shape_heightfield* hf, const ccs_obb* o, ccs_contact* out)
{
    ccs_aabb q;

    if (!hf || !o || !out)
        return 0;

    /* build query AABB for obb */
    {
        ccs_vec3 ax0;
        ccs_vec3 ax1;
        ccs_vec3 ax2;
        ccs_vec3 e;

        ax0.x = ccs_fixed_abs(o->axis[0].x);
        ax0.y = ccs_fixed_abs(o->axis[0].y);
        ax0.z = ccs_fixed_abs(o->axis[0].z);

        ax1.x = ccs_fixed_abs(o->axis[1].x);
        ax1.y = ccs_fixed_abs(o->axis[1].y);
        ax1.z = ccs_fixed_abs(o->axis[1].z);

        ax2.x = ccs_fixed_abs(o->axis[2].x);
        ax2.y = ccs_fixed_abs(o->axis[2].y);
        ax2.z = ccs_fixed_abs(o->axis[2].z);

        e.x = ccs_fixed_mul(ax0.x, o->half.x)
            + ccs_fixed_mul(ax1.x, o->half.y)
            + ccs_fixed_mul(ax2.x, o->half.z);

        e.y = ccs_fixed_mul(ax0.y, o->half.x)
            + ccs_fixed_mul(ax1.y, o->half.y)
            + ccs_fixed_mul(ax2.y, o->half.z);

        e.z = ccs_fixed_mul(ax0.z, o->half.x)
            + ccs_fixed_mul(ax1.z, o->half.y)
            + ccs_fixed_mul(ax2.z, o->half.z);

        q.min = ccs_vec3_sub(o->center, e);
        q.max = ccs_vec3_add(o->center, e);
    }

    return hf_collide_triangles(hf, &q, tri_obb_wrap, o, out);
}

int ccs_heightfield_raycast(const ccs_shape_heightfield* hf, const ccs_ray* ray, ccs_raycast_hit* out)
{
    ccs_aabb hf_aabb;
    ccs_aabb q;
    ccs_vec3 p0;
    ccs_vec3 p1;
    int i0, i1, j0, j1;
    int i, j;
    int found;
    ccs_fixed best_t;

    if (!hf || !ray || !out)
        return 0;

    out->hit = 0;

    hf_aabb = ccs_shape_compute_aabb(hf);

    /* Build a rough query AABB from ray segment [tmin,tmax] */
    p0 = ccs_vec3_add(ray->origin, ccs_vec3_scale(ray->dir, ray->tmin));
    p1 = ccs_vec3_add(ray->origin, ccs_vec3_scale(ray->dir, ray->tmax));
    q.min = ccs_vec3_min(p0, p1);
    q.max = ccs_vec3_max(p0, p1);

    if (!aabb_overlap(&hf_aabb, &q))
        return 0;

    hf_cell_range_for_aabb(hf, &q, &i0, &i1, &j0, &j1);

    found = 0;
    best_t = ray->tmax;

    for (j = j0; j <= j1; ++j) {
        for (i = i0; i <= i1; ++i) {
            ccs_triangle t0;
            ccs_triangle t1;
            ccs_raycast_hit h;

            hf_cell_triangles(hf, i, j, &t0, &t1);

            if (ccs_raycast_triangle(ray, &t0, &h)) {
                if (!found || h.t < best_t) {
                    found = 1;
                    best_t = h.t;
                    *out = h;
                }
            }

            if (ccs_raycast_triangle(ray, &t1, &h)) {
                if (!found || h.t < best_t) {
                    found = 1;
                    best_t = h.t;
                    *out = h;
                }
            }
        }
    }

    return found;
}
