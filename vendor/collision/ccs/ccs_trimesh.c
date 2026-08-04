#include "ccs_trimesh.h"
#include "ccs_triangle.h"
static ccs_vec3 vec3_abs_comp(ccs_vec3 v)
{
    ccs_vec3 out;
    out.x = ccs_fixed_abs(v.x);
    out.y = ccs_fixed_abs(v.y);
    out.z = ccs_fixed_abs(v.z);
    return out;
}


/* Local helper: compute triangle AABB in local space */
static ccs_aabb tri_aabb_local(const ccs_shape_trimesh* m, int tri_idx)
{
    ccs_aabb a;
    ccs_vec3 v0;
    ccs_vec3 v1;
    ccs_vec3 v2;
    int i0;
    int i1;
    int i2;

    i0 = (int)m->indices[tri_idx * 3 + 0];
    i1 = (int)m->indices[tri_idx * 3 + 1];
    i2 = (int)m->indices[tri_idx * 3 + 2];

    if (i0 < 0) i0 = 0;
    if (i1 < 0) i1 = 0;
    if (i2 < 0) i2 = 0;

    if (i0 >= m->vertex_count) i0 = m->vertex_count - 1;
    if (i1 >= m->vertex_count) i1 = m->vertex_count - 1;
    if (i2 >= m->vertex_count) i2 = m->vertex_count - 1;

    v0 = m->vertices[i0];
    v1 = m->vertices[i1];
    v2 = m->vertices[i2];

    a.min = ccs_vec3_min(v0, ccs_vec3_min(v1, v2));
    a.max = ccs_vec3_max(v0, ccs_vec3_max(v1, v2));

    return a;
}

static ccs_triangle tri_world(const ccs_shape_trimesh* m, int tri_idx)
{
    ccs_triangle t;
    int i0;
    int i1;
    int i2;

    i0 = (int)m->indices[tri_idx * 3 + 0];
    i1 = (int)m->indices[tri_idx * 3 + 1];
    i2 = (int)m->indices[tri_idx * 3 + 2];

    if (i0 < 0) i0 = 0;
    if (i1 < 0) i1 = 0;
    if (i2 < 0) i2 = 0;

    if (i0 >= m->vertex_count) i0 = m->vertex_count - 1;
    if (i1 >= m->vertex_count) i1 = m->vertex_count - 1;
    if (i2 >= m->vertex_count) i2 = m->vertex_count - 1;

    t.a = ccs_vec3_add(m->vertices[i0], m->center);
    t.b = ccs_vec3_add(m->vertices[i1], m->center);
    t.c = ccs_vec3_add(m->vertices[i2], m->center);

    return t;
}

static int aabb_overlap(const ccs_aabb* a, const ccs_aabb* b)
{
    if (a->max.x < b->min.x || a->min.x > b->max.x) return 0;
    if (a->max.y < b->min.y || a->min.y > b->max.y) return 0;
    if (a->max.z < b->min.z || a->min.z > b->max.z) return 0;
    return 1;
}

void ccs_trimesh_init(
    ccs_shape_trimesh* m,
    const ccs_vec3* vertices,
    int vertex_count,
    const ccs_u32* indices,
    int tri_count,
    ccs_vec3 center,
    ccs_u32 flags
) {
    int i;

    if (!m)
        return;

    m->header.type = CCS_SHAPE_TRIMESH;
    m->header.flags = flags;

    m->vertices = vertices;
    m->vertex_count = vertex_count;
    m->indices = indices;
    m->tri_count = tri_count;
    m->center = center;

    m->tri_aabbs = 0;
    m->tree = 0;

    /* compute local AABB from vertices */
    if (vertices && vertex_count > 0) {
        ccs_vec3 mn;
        ccs_vec3 mx;

        mn = vertices[0];
        mx = vertices[0];

        for (i = 1; i < vertex_count; ++i) {
            mn = ccs_vec3_min(mn, vertices[i]);
            mx = ccs_vec3_max(mx, vertices[i]);
        }

        m->local_aabb.min = mn;
        m->local_aabb.max = mx;
    } else {
        m->local_aabb.min = ccs_vec3_zero();
        m->local_aabb.max = ccs_vec3_zero();
    }
}

int ccs_trimesh_build_aabbtree(
    ccs_shape_trimesh* m,
    CCS_AABBTree* tree,
    ccs_aabb* tri_aabbs,
    int* tri_ids,
    int tri_count
) {
    int i;

    if (!m || !tree || !tri_aabbs || !tri_ids)
        return 0;

    if (tri_count <= 0)
        return 0;

    if (m->tri_count < tri_count)
        tri_count = m->tri_count;

    for (i = 0; i < tri_count; ++i) {
        tri_aabbs[i] = tri_aabb_local(m, i);
        tri_ids[i] = i;
    }

    if (!ccs_aabbtree_build(tree, tri_aabbs, tri_ids, tri_count))
        return 0;

    m->tri_aabbs = tri_aabbs;
    m->tree = tree;

    return 1;
}

/* Candidate triangle iterator.
   If BVH exists, returns list of tri indices in out_ids and count.
   Else returns -1 count to indicate brute force.
*/
static int gather_candidates(
    const ccs_shape_trimesh* m,
    const ccs_aabb* query_world,
    int* out_ids,
    int max_ids
) {
    ccs_aabb q_local;

    if (!m || !query_world || !out_ids || max_ids <= 0)
        return 0;

    if (!m->tree || !m->tri_aabbs)
        return -1; /* brute */

    /* world -> local */
    q_local.min = ccs_vec3_sub(query_world->min, m->center);
    q_local.max = ccs_vec3_sub(query_world->max, m->center);

    return ccs_aabbtree_query_aabb(m->tree, &q_local, out_ids, max_ids);
}

static int collide_triangles_sphere(
    const ccs_shape_trimesh* m,
    const ccs_sphere* s,
    const ccs_aabb* query_aabb,
    ccs_contact* out
) {
    int i;
    int best;
    ccs_fixed best_pen;
    int ids[CCS_QUERY_MAX_CANDIDATES];
    int count;

    best = 0;
    best_pen = 0;

    count = gather_candidates(m, query_aabb, ids, CCS_QUERY_MAX_CANDIDATES);

    if (count == -1) {
        /* brute */
        for (i = 0; i < m->tri_count; ++i) {
            ccs_triangle tri;
            ccs_contact c;
            tri = tri_world(m, i);
            if (ccs_triangle_sphere_contact(&tri, s, &c)) {
                if (c.penetration > best_pen) {
                    best_pen = c.penetration;
                    *out = c;
                    best = 1;
                }
            }
        }
        return best;
    }

    /* BVH candidates */
    for (i = 0; i < count; ++i) {
        int tid;
        ccs_aabb ta_local;
        ccs_aabb ta_world;
        ccs_triangle tri;
        ccs_contact c;

        tid = ids[i];
        if (tid < 0 || tid >= m->tri_count)
            continue;

        /* optional quick AABB reject using stored tri_aabbs */
        ta_local = m->tri_aabbs[tid];
        ta_world.min = ccs_vec3_add(ta_local.min, m->center);
        ta_world.max = ccs_vec3_add(ta_local.max, m->center);
        if (!aabb_overlap(&ta_world, query_aabb))
            continue;

        tri = tri_world(m, tid);
        if (ccs_triangle_sphere_contact(&tri, s, &c)) {
            if (c.penetration > best_pen) {
                best_pen = c.penetration;
                *out = c;
                best = 1;
            }
        }
    }

    return best;
}

static int collide_triangles_capsule(
    const ccs_shape_trimesh* m,
    const ccs_capsule* cap,
    const ccs_aabb* query_aabb,
    ccs_contact* out
) {
    int i;
    int best;
    ccs_fixed best_pen;
    int ids[CCS_QUERY_MAX_CANDIDATES];
    int count;

    best = 0;
    best_pen = 0;

    count = gather_candidates(m, query_aabb, ids, CCS_QUERY_MAX_CANDIDATES);

    if (count == -1) {
        for (i = 0; i < m->tri_count; ++i) {
            ccs_triangle tri;
            ccs_contact c;
            tri = tri_world(m, i);
            if (ccs_triangle_capsule_contact(&tri, cap, &c)) {
                if (c.penetration > best_pen) {
                    best_pen = c.penetration;
                    *out = c;
                    best = 1;
                }
            }
        }
        return best;
    }

    for (i = 0; i < count; ++i) {
        int tid;
        ccs_aabb ta_local;
        ccs_aabb ta_world;
        ccs_triangle tri;
        ccs_contact c;

        tid = ids[i];
        if (tid < 0 || tid >= m->tri_count)
            continue;

        ta_local = m->tri_aabbs[tid];
        ta_world.min = ccs_vec3_add(ta_local.min, m->center);
        ta_world.max = ccs_vec3_add(ta_local.max, m->center);
        if (!aabb_overlap(&ta_world, query_aabb))
            continue;

        tri = tri_world(m, tid);
        if (ccs_triangle_capsule_contact(&tri, cap, &c)) {
            if (c.penetration > best_pen) {
                best_pen = c.penetration;
                *out = c;
                best = 1;
            }
        }
    }

    return best;
}

static int collide_triangles_obb(
    const ccs_shape_trimesh* m,
    const ccs_obb* obb,
    const ccs_aabb* query_aabb,
    ccs_contact* out
) {
    int i;
    int best;
    ccs_fixed best_pen;
    int ids[CCS_QUERY_MAX_CANDIDATES];
    int count;

    best = 0;
    best_pen = 0;

    count = gather_candidates(m, query_aabb, ids, CCS_QUERY_MAX_CANDIDATES);

    if (count == -1) {
        for (i = 0; i < m->tri_count; ++i) {
            ccs_triangle tri;
            ccs_contact c;
            tri = tri_world(m, i);
            if (ccs_triangle_obb_contact(&tri, obb, &c)) {
                if (c.penetration > best_pen) {
                    best_pen = c.penetration;
                    *out = c;
                    best = 1;
                }
            }
        }
        return best;
    }

    for (i = 0; i < count; ++i) {
        int tid;
        ccs_aabb ta_local;
        ccs_aabb ta_world;
        ccs_triangle tri;
        ccs_contact c;

        tid = ids[i];
        if (tid < 0 || tid >= m->tri_count)
            continue;

        ta_local = m->tri_aabbs[tid];
        ta_world.min = ccs_vec3_add(ta_local.min, m->center);
        ta_world.max = ccs_vec3_add(ta_local.max, m->center);
        if (!aabb_overlap(&ta_world, query_aabb))
            continue;

        tri = tri_world(m, tid);
        if (ccs_triangle_obb_contact(&tri, obb, &c)) {
            if (c.penetration > best_pen) {
                best_pen = c.penetration;
                *out = c;
                best = 1;
            }
        }
    }

    return best;
}

int ccs_trimesh_collide_sphere(const ccs_shape_trimesh* m, const ccs_sphere* s, ccs_contact* out)
{
    ccs_aabb query;
    ccs_vec3 r;

    if (!m || !s || !out)
        return 0;

    r.x = s->radius;
    r.y = s->radius;
    r.z = s->radius;

    query.min = ccs_vec3_sub(s->center, r);
    query.max = ccs_vec3_add(s->center, r);

    return collide_triangles_sphere(m, s, &query, out);
}

int ccs_trimesh_collide_capsule(const ccs_shape_trimesh* m, const ccs_capsule* c, ccs_contact* out)
{
    ccs_aabb query;
    ccs_vec3 a;
    ccs_vec3 b;
    ccs_vec3 mn;
    ccs_vec3 mx;
    ccs_vec3 r;

    if (!m || !c || !out)
        return 0;

    ccs_capsule_endpoints(c, &a, &b);
    mn = ccs_vec3_min(a, b);
    mx = ccs_vec3_max(a, b);

    r.x = c->radius;
    r.y = c->radius;
    r.z = c->radius;

    query.min = ccs_vec3_sub(mn, r);
    query.max = ccs_vec3_add(mx, r);

    return collide_triangles_capsule(m, c, &query, out);
}

int ccs_trimesh_collide_obb(const ccs_shape_trimesh* m, const ccs_obb* o, ccs_contact* out)
{
    ccs_aabb query;

    if (!m || !o || !out)
        return 0;


    /* build query AABB for obb */
    {
        ccs_vec3 ax0;
        ccs_vec3 ax1;
        ccs_vec3 ax2;
        ccs_vec3 e;

        ax0 = vec3_abs_comp(o->axis[0]);
        ax1 = vec3_abs_comp(o->axis[1]);
        ax2 = vec3_abs_comp(o->axis[2]);

        e.x = ccs_fixed_mul(ax0.x, o->half.x)
            + ccs_fixed_mul(ax1.x, o->half.y)
            + ccs_fixed_mul(ax2.x, o->half.z);

        e.y = ccs_fixed_mul(ax0.y, o->half.x)
            + ccs_fixed_mul(ax1.y, o->half.y)
            + ccs_fixed_mul(ax2.y, o->half.z);

        e.z = ccs_fixed_mul(ax0.z, o->half.x)
            + ccs_fixed_mul(ax1.z, o->half.y)
            + ccs_fixed_mul(ax2.z, o->half.z);

        query.min = ccs_vec3_sub(o->center, e);
        query.max = ccs_vec3_add(o->center, e);
    }

    return collide_triangles_obb(m, o, &query, out);
}

int ccs_trimesh_raycast(const ccs_shape_trimesh* m, const ccs_ray* ray, ccs_raycast_hit* out)
{
    int i;
    int found;
    ccs_fixed best_t;

    if (!m || !ray || !out)
        return 0;

    out->hit = 0;

    found = 0;
    best_t = ray->tmax;

    /* TODO: BVH ray traversal (for now brute) */
    for (i = 0; i < m->tri_count; ++i) {
        ccs_triangle tri;
        ccs_raycast_hit h;

        tri = tri_world(m, i);
        if (ccs_raycast_triangle(ray, &tri, &h)) {
            if (!found || h.t < best_t) {
                found = 1;
                best_t = h.t;
                *out = h;
            }
        }
    }

    return found;
}
