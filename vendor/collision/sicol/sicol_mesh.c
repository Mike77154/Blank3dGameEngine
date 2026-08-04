/* ============================================================
 * SICOL - Triangle mesh + BVH
 * ============================================================ */

#include "sicol_mesh.h"

#define SICOL_MESH_LEAF_TRIANGLES 4
#define SICOL_MESH_RAY_EPS FX_FROM_RATIO(1, 4096)

static void mesh_bounds_reset(fx out_min[3], fx out_max[3])
{
    out_min[0] = out_min[1] = out_min[2] = FX_FROM_INT(32767);
    out_max[0] = out_max[1] = out_max[2] = -FX_FROM_INT(32767);
}

static void mesh_bounds_expand_point(fx io_min[3], fx io_max[3], const fx p[3])
{
    io_min[0] = FX_MIN(io_min[0], p[0]);
    io_min[1] = FX_MIN(io_min[1], p[1]);
    io_min[2] = FX_MIN(io_min[2], p[2]);
    io_max[0] = FX_MAX(io_max[0], p[0]);
    io_max[1] = FX_MAX(io_max[1], p[1]);
    io_max[2] = FX_MAX(io_max[2], p[2]);
}

static void mesh_bounds_expand_box(fx io_min[3], fx io_max[3], const fx bmin[3], const fx bmax[3])
{
    io_min[0] = FX_MIN(io_min[0], bmin[0]);
    io_min[1] = FX_MIN(io_min[1], bmin[1]);
    io_min[2] = FX_MIN(io_min[2], bmin[2]);
    io_max[0] = FX_MAX(io_max[0], bmax[0]);
    io_max[1] = FX_MAX(io_max[1], bmax[1]);
    io_max[2] = FX_MAX(io_max[2], bmax[2]);
}

static int mesh_aabb_overlap(const fx amin[3], const fx amax[3], const fx bmin[3], const fx bmax[3])
{
    if (amax[0] < bmin[0] || amin[0] > bmax[0]) return 0;
    if (amax[1] < bmin[1] || amin[1] > bmax[1]) return 0;
    if (amax[2] < bmin[2] || amin[2] > bmax[2]) return 0;
    return 1;
}

static int mesh_ray_vs_aabb(
    const fx origin[3],
    const fx dir[3],
    fx length,
    const fx box_min[3],
    const fx box_max[3],
    fx* out_tmin,
    fx* out_tmax
)
{
    fx tmin = 0;
    fx tmax = length;
    int i;

    for (i = 0; i < 3; ++i) {
        if (FX_ABS(dir[i]) > FX_EPSILON) {
            fx inv_d = FX_DIV(FX_ONE, dir[i]);
            fx t0 = FX_MUL(box_min[i] - origin[i], inv_d);
            fx t1 = FX_MUL(box_max[i] - origin[i], inv_d);
            if (t0 > t1) {
                fx tmp = t0;
                t0 = t1;
                t1 = tmp;
            }
            if (t0 > tmin) tmin = t0;
            if (t1 < tmax) tmax = t1;
            if (tmin > tmax) return 0;
        } else {
            if (origin[i] < box_min[i] || origin[i] > box_max[i]) return 0;
        }
    }

    if (tmax < 0 || tmin > length) return 0;
    if (out_tmin) *out_tmin = tmin;
    if (out_tmax) *out_tmax = tmax;
    return 1;
}

static int mesh_triangle_refresh(sicol_mesh_triangle_t* tri)
{
    fx e0[3];
    fx e1[3];
    fx normal[3];

    if (!tri) return 0;

    tri->centroid[0] = (tri->v[0][0] + tri->v[1][0] + tri->v[2][0]) / 3;
    tri->centroid[1] = (tri->v[0][1] + tri->v[1][1] + tri->v[2][1]) / 3;
    tri->centroid[2] = (tri->v[0][2] + tri->v[1][2] + tri->v[2][2]) / 3;

    mesh_bounds_reset(tri->min, tri->max);
    mesh_bounds_expand_point(tri->min, tri->max, tri->v[0]);
    mesh_bounds_expand_point(tri->min, tri->max, tri->v[1]);
    mesh_bounds_expand_point(tri->min, tri->max, tri->v[2]);

    fx_sub3(e0, tri->v[1], tri->v[0]);
    fx_sub3(e1, tri->v[2], tri->v[0]);
    fx_cross3(normal, e0, e1);
    if (!fx_normalize3(tri->normal, normal)) {
        fx_set3(tri->normal, 0, FX_ONE, 0);
        return 0;
    }
    return 1;
}

static int mesh_triangle_build(sicol_mesh_triangle_t* tri, const sicol_triangle_verts_t* src)
{
    if (!tri || !src) return 0;
    fx_copy3(tri->v[0], src->v0);
    fx_copy3(tri->v[1], src->v1);
    fx_copy3(tri->v[2], src->v2);
    return mesh_triangle_refresh(tri);
}

static int mesh_choose_split_axis(const sicol_mesh_t* mesh, int start, int count)
{
    fx cmin[3];
    fx cmax[3];
    int i;
    int axis = 0;
    fx best_extent;

    mesh_bounds_reset(cmin, cmax);
    for (i = 0; i < count; ++i) {
        const sicol_mesh_triangle_t* tri = &mesh->triangles[mesh->indices[start + i]];
        mesh_bounds_expand_point(cmin, cmax, tri->centroid);
    }

    best_extent = cmax[0] - cmin[0];
    if ((cmax[1] - cmin[1]) > best_extent) {
        best_extent = cmax[1] - cmin[1];
        axis = 1;
    }
    if ((cmax[2] - cmin[2]) > best_extent) {
        axis = 2;
    }
    return axis;
}

static void mesh_sort_indices(sicol_mesh_t* mesh, int start, int count, int axis)
{
    int i;
    for (i = start + 1; i < start + count; ++i) {
        int key = mesh->indices[i];
        fx key_value = mesh->triangles[key].centroid[axis];
        int j = i - 1;
        while (j >= start && mesh->triangles[mesh->indices[j]].centroid[axis] > key_value) {
            mesh->indices[j + 1] = mesh->indices[j];
            --j;
        }
        mesh->indices[j + 1] = key;
    }
}

static int mesh_build_node(sicol_mesh_t* mesh, int start, int count)
{
    int node_index;
    int i;
    int axis;
    int split;
    sicol_mesh_bvh_node_t* node;

    if (!mesh || count <= 0) return -1;
    if (mesh->node_count >= SICOL_MESH_MAX_BVH_NODES) return -1;

    node_index = mesh->node_count++;
    node = &mesh->nodes[node_index];
    node->left = -1;
    node->right = -1;
    node->start = start;
    node->count = count;
    mesh_bounds_reset(node->min, node->max);

    for (i = 0; i < count; ++i) {
        const sicol_mesh_triangle_t* tri = &mesh->triangles[mesh->indices[start + i]];
        mesh_bounds_expand_box(node->min, node->max, tri->min, tri->max);
    }

    if (count <= SICOL_MESH_LEAF_TRIANGLES) {
        return node_index;
    }

    axis = mesh_choose_split_axis(mesh, start, count);
    mesh_sort_indices(mesh, start, count, axis);
    split = count / 2;
    if (split <= 0 || split >= count) {
        return node_index;
    }

    node->left = mesh_build_node(mesh, start, split);
    node->right = mesh_build_node(mesh, start + split, count - split);
    if (node->left >= 0 || node->right >= 0) {
        node->count = 0;
    }
    return node_index;
}

static int mesh_ray_vs_triangle(
    const fx origin[3],
    const fx dir[3],
    fx length,
    const sicol_mesh_triangle_t* tri,
    fx* out_t,
    fx out_point[3],
    fx out_normal[3]
)
{
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

    if (!tri) return 0;

    fx_sub3(edge1, tri->v[1], tri->v[0]);
    fx_sub3(edge2, tri->v[2], tri->v[0]);
    fx_cross3(pvec, dir, edge2);
    det = fx_dot3(edge1, pvec);
    if (FX_ABS(det) <= SICOL_MESH_RAY_EPS) return 0;

    inv_det = FX_DIV(FX_ONE, det);
    fx_sub3(tvec, origin, tri->v[0]);
    u = FX_MUL(fx_dot3(tvec, pvec), inv_det);
    if (u < 0 || u > FX_ONE) return 0;

    fx_cross3(qvec, tvec, edge1);
    v = FX_MUL(fx_dot3(dir, qvec), inv_det);
    if (v < 0 || (u + v) > FX_ONE) return 0;

    t = FX_MUL(fx_dot3(edge2, qvec), inv_det);
    if (t < 0 || t > length) return 0;

    if (out_t) *out_t = t;
    if (out_point) fx_madd3(out_point, origin, dir, t);
    if (out_normal) {
        fx_copy3(out_normal, tri->normal);
        if (fx_dot3(out_normal, dir) > 0) {
            fx_neg3(out_normal, out_normal);
        }
    }
    return 1;
}

void sicol_mesh_init(sicol_mesh_t* mesh)
{
    int i;
    if (!mesh) return;
    mesh->triangle_count = 0;
    mesh->node_count = 0;
    fx_zero3(mesh->min);
    fx_zero3(mesh->max);
    for (i = 0; i < SICOL_MESH_MAX_TRIANGLES; ++i) {
        mesh->indices[i] = i;
        fx_zero3(mesh->triangles[i].v[0]);
        fx_zero3(mesh->triangles[i].v[1]);
        fx_zero3(mesh->triangles[i].v[2]);
        fx_zero3(mesh->triangles[i].normal);
        fx_zero3(mesh->triangles[i].centroid);
        fx_zero3(mesh->triangles[i].min);
        fx_zero3(mesh->triangles[i].max);
    }
    for (i = 0; i < SICOL_MESH_MAX_BVH_NODES; ++i) {
        fx_zero3(mesh->nodes[i].min);
        fx_zero3(mesh->nodes[i].max);
        mesh->nodes[i].left = -1;
        mesh->nodes[i].right = -1;
        mesh->nodes[i].start = 0;
        mesh->nodes[i].count = 0;
    }
}

int sicol_mesh_build(sicol_mesh_t* mesh, const sicol_triangle_verts_t* tris, int count)
{
    int i;
    int valid_count;

    if (!mesh || !tris) return 0;
    if (count < 1 || count > SICOL_MESH_MAX_TRIANGLES) return 0;

    sicol_mesh_init(mesh);
    mesh_bounds_reset(mesh->min, mesh->max);

    valid_count = 0;
    for (i = 0; i < count; ++i) {
        if (mesh_triangle_build(&mesh->triangles[valid_count], &tris[i])) {
            mesh_bounds_expand_box(mesh->min, mesh->max,
                                   mesh->triangles[valid_count].min,
                                   mesh->triangles[valid_count].max);
            ++valid_count;
        }
    }

    if (valid_count <= 0) {
        sicol_mesh_init(mesh);
        return 0;
    }

    mesh->triangle_count = valid_count;
    for (i = 0; i < valid_count; ++i) {
        mesh->indices[i] = i;
    }
    mesh->node_count = 0;
    if (mesh_build_node(mesh, 0, valid_count) < 0) {
        sicol_mesh_init(mesh);
        return 0;
    }
    return 1;
}

int sicol_mesh_get_triangle(const sicol_mesh_t* mesh, int triangle_index, sicol_triangle_verts_t* out)
{
    if (!mesh || !out) return 0;
    if (triangle_index < 0 || triangle_index >= mesh->triangle_count) return 0;
    fx_copy3(out->v0, mesh->triangles[triangle_index].v[0]);
    fx_copy3(out->v1, mesh->triangles[triangle_index].v[1]);
    fx_copy3(out->v2, mesh->triangles[triangle_index].v[2]);
    return 1;
}

static void mesh_node_refit(sicol_mesh_t* mesh, int node_index)
{
    sicol_mesh_bvh_node_t* node;
    int i;
    if (!mesh || node_index < 0 || node_index >= mesh->node_count) return;
    node = &mesh->nodes[node_index];
    mesh_bounds_reset(node->min, node->max);
    if (node->left < 0 && node->right < 0) {
        for (i = 0; i < node->count; ++i) {
            int tri_index = mesh->indices[node->start + i];
            mesh_bounds_expand_box(node->min, node->max,
                                   mesh->triangles[tri_index].min,
                                   mesh->triangles[tri_index].max);
        }
    } else {
        if (node->left >= 0) {
            mesh_bounds_expand_box(node->min, node->max,
                                   mesh->nodes[node->left].min,
                                   mesh->nodes[node->left].max);
        }
        if (node->right >= 0) {
            mesh_bounds_expand_box(node->min, node->max,
                                   mesh->nodes[node->right].min,
                                   mesh->nodes[node->right].max);
        }
    }
}

int sicol_mesh_set_triangle(sicol_mesh_t* mesh, int triangle_index, const sicol_triangle_verts_t* tri)
{
    if (!mesh || !tri) return 0;
    if (triangle_index < 0 || triangle_index >= mesh->triangle_count) return 0;
    fx_copy3(mesh->triangles[triangle_index].v[0], tri->v0);
    fx_copy3(mesh->triangles[triangle_index].v[1], tri->v1);
    fx_copy3(mesh->triangles[triangle_index].v[2], tri->v2);
    mesh_triangle_refresh(&mesh->triangles[triangle_index]);
    return sicol_mesh_refit(mesh);
}

int sicol_mesh_refit(sicol_mesh_t* mesh)
{
    int i;
    if (!mesh) return 0;
    if (mesh->triangle_count <= 0 || mesh->node_count <= 0) return 0;

    for (i = 0; i < mesh->triangle_count; ++i) {
        mesh_triangle_refresh(&mesh->triangles[i]);
    }

    for (i = mesh->node_count - 1; i >= 0; --i) {
        mesh_node_refit(mesh, i);
    }

    fx_copy3(mesh->min, mesh->nodes[0].min);
    fx_copy3(mesh->max, mesh->nodes[0].max);
    return 1;
}

int sicol_mesh_query_local_aabb(
    const sicol_mesh_t* mesh,
    const fx query_min[3],
    const fx query_max[3],
    int* out_triangle_indices,
    int max_results
)
{
    int stack[SICOL_MESH_MAX_BVH_NODES];
    int top;
    int count;

    if (!mesh || !out_triangle_indices || max_results <= 0) return 0;
    if (mesh->triangle_count <= 0 || mesh->node_count <= 0) return 0;

    top = 0;
    count = 0;
    stack[top++] = 0;

    while (top > 0) {
        int node_index = stack[--top];
        const sicol_mesh_bvh_node_t* node = &mesh->nodes[node_index];
        int i;
        if (!mesh_aabb_overlap(node->min, node->max, query_min, query_max)) continue;
        if (node->left < 0 && node->right < 0) {
            for (i = 0; i < node->count; ++i) {
                if (count < max_results) {
                    out_triangle_indices[count++] = mesh->indices[node->start + i];
                } else {
                    return count;
                }
            }
        } else {
            if (node->left >= 0) stack[top++] = node->left;
            if (node->right >= 0) stack[top++] = node->right;
        }
    }

    return count;
}

int sicol_mesh_raycast_local(
    const sicol_mesh_t* mesh,
    const fx origin[3],
    const fx dir[3],
    fx length,
    int* out_triangle_index,
    fx* out_t,
    fx out_point[3],
    fx out_normal[3]
)
{
    int stack[SICOL_MESH_MAX_BVH_NODES];
    int top;
    int hit;
    fx best_t;
    int best_index;
    fx best_point[3];
    fx best_normal[3];

    if (!mesh || !origin || !dir) return 0;
    if (mesh->triangle_count <= 0 || mesh->node_count <= 0) return 0;

    top = 0;
    hit = 0;
    best_t = length;
    best_index = -1;
    fx_zero3(best_point);
    fx_zero3(best_normal);
    stack[top++] = 0;

    while (top > 0) {
        int node_index = stack[--top];
        const sicol_mesh_bvh_node_t* node = &mesh->nodes[node_index];
        fx node_tmin;
        fx node_tmax;
        int i;

        if (!mesh_ray_vs_aabb(origin, dir, best_t, node->min, node->max, &node_tmin, &node_tmax)) {
            continue;
        }

        if (node->left < 0 && node->right < 0) {
            for (i = 0; i < node->count; ++i) {
                int tri_index = mesh->indices[node->start + i];
                fx tri_t;
                fx tri_point[3];
                fx tri_normal[3];
                if (mesh_ray_vs_triangle(origin, dir, best_t, &mesh->triangles[tri_index],
                                         &tri_t, tri_point, tri_normal)) {
                    hit = 1;
                    best_t = tri_t;
                    best_index = tri_index;
                    fx_copy3(best_point, tri_point);
                    fx_copy3(best_normal, tri_normal);
                }
            }
        } else {
            if (node->left >= 0) stack[top++] = node->left;
            if (node->right >= 0) stack[top++] = node->right;
        }
    }

    if (!hit) return 0;
    if (out_triangle_index) *out_triangle_index = best_index;
    if (out_t) *out_t = best_t;
    if (out_point) fx_copy3(out_point, best_point);
    if (out_normal) fx_copy3(out_normal, best_normal);
    return 1;
}
