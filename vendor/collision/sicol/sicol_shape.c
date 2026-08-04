/* ============================================================
 * SICOL - Shape helpers
 * ============================================================ */

#include "sicol_shape.h"
#include "sicol_mesh.h"

static int shape_half_valid(const fx half[3])
{
    if (half[0] < 0 || half[1] < 0 || half[2] < 0) return 0;
    return 1;
}

static int shape_basis_valid(const fx axis[3][3])
{
    fx dot01;
    fx dot02;
    fx dot12;
    fx len0;
    fx len1;
    fx len2;

    len0 = fx_len3(axis[0]);
    len1 = fx_len3(axis[1]);
    len2 = fx_len3(axis[2]);
    if (FX_ABS(len0 - FX_ONE) > FX_FROM_RATIO(1, 8)) return 0;
    if (FX_ABS(len1 - FX_ONE) > FX_FROM_RATIO(1, 8)) return 0;
    if (FX_ABS(len2 - FX_ONE) > FX_FROM_RATIO(1, 8)) return 0;

    dot01 = fx_dot3(axis[0], axis[1]);
    dot02 = fx_dot3(axis[0], axis[2]);
    dot12 = fx_dot3(axis[1], axis[2]);
    if (FX_ABS(dot01) > FX_FROM_RATIO(1, 8)) return 0;
    if (FX_ABS(dot02) > FX_FROM_RATIO(1, 8)) return 0;
    if (FX_ABS(dot12) > FX_FROM_RATIO(1, 8)) return 0;
    return 1;
}

void sicol_shape_make_none(sicol_shape_t* s)
{
    if (!s) return;
    s->type = SICOL_SHAPE_NONE;
    fx_zero3(s->pos);
}

void sicol_shape_make_aabb(sicol_shape_t* s, const fx pos[3], const fx half[3])
{
    if (!s) return;
    s->type = SICOL_SHAPE_AABB;
    fx_copy3(s->pos, pos);
    fx_copy3(s->u.aabb.half, half);
}

void sicol_shape_make_obb(sicol_shape_t* s, const fx pos[3], const fx half[3], fx axis[3][3])
{
    int i, j;
    if (!s) return;
    s->type = SICOL_SHAPE_OBB;
    fx_copy3(s->pos, pos);
    fx_copy3(s->u.obb.half, half);

    for (i = 0; i < 3; ++i) {
        for (j = 0; j < 3; ++j) {
            s->u.obb.axis[i][j] = axis[i][j];
        }
    }
}

void sicol_shape_make_obb_identity(sicol_shape_t* s, const fx pos[3], const fx half[3])
{
    fx axis[3][3];
    fx_identity_mat3(axis);
    if (!s) return;
    s->type = SICOL_SHAPE_OBB;
    fx_copy3(s->pos, pos);
    fx_copy3(s->u.obb.half, half);
    fx_copy3(s->u.obb.axis[0], axis[0]);
    fx_copy3(s->u.obb.axis[1], axis[1]);
    fx_copy3(s->u.obb.axis[2], axis[2]);
}

void sicol_shape_make_plane(sicol_shape_t* s, const fx normal[3], fx d)
{
    fx n[3];
    if (!s) return;
    s->type = SICOL_SHAPE_PLANE;
    fx_zero3(s->pos);
    if (!fx_normalize3(n, normal)) {
        fx_set3(n, 0, FX_ONE, 0);
    }
    fx_copy3(s->u.plane.normal, n);
    s->u.plane.d = d;
}

void sicol_shape_make_ray(sicol_shape_t* s, const fx pos[3], const fx dir[3], fx length)
{
    fx d[3];
    if (!s) return;
    s->type = SICOL_SHAPE_RAY;
    fx_copy3(s->pos, pos);
    if (!fx_normalize3(d, dir)) {
        fx_set3(d, FX_ONE, 0, 0);
    }
    fx_copy3(s->u.ray.dir, d);
    s->u.ray.length = length;
}

void sicol_shape_make_sphere(sicol_shape_t* s, const fx pos[3], fx radius)
{
    if (!s) return;
    s->type = SICOL_SHAPE_SPHERE;
    fx_copy3(s->pos, pos);
    s->u.sphere.radius = radius;
}

void sicol_shape_make_capsule(sicol_shape_t* s, const fx pos[3], const fx axis[3], fx half_segment, fx radius)
{
    fx n[3];
    if (!s) return;
    s->type = SICOL_SHAPE_CAPSULE;
    fx_copy3(s->pos, pos);
    if (!fx_normalize3(n, axis)) {
        fx_set3(n, 0, FX_ONE, 0);
    }
    fx_copy3(s->u.capsule.axis, n);
    s->u.capsule.half_segment = half_segment;
    s->u.capsule.radius = radius;
}

void sicol_shape_make_segment(sicol_shape_t* s, const fx from[3], const fx to[3])
{
    if (!s) return;
    s->type = SICOL_SHAPE_SEGMENT;
    fx_copy3(s->pos, from);
    fx_copy3(s->u.segment.to, to);
}

void sicol_shape_make_convex(sicol_shape_t* s, const fx pos[3], fx verts[][3], int count, fx axis[3][3])
{
    int i;
    if (!s) return;
    s->type = SICOL_SHAPE_CONVEX;
    fx_copy3(s->pos, pos);
    s->u.convex.count = count;
    for (i = 0; i < SICOL_CONVEX_MAX_VERTS; ++i) {
        if (i < count && verts) {
            fx_copy3(s->u.convex.verts[i], verts[i]);
        } else {
            fx_zero3(s->u.convex.verts[i]);
        }
    }
    if (axis) {
        fx_copy3(s->u.convex.axis[0], axis[0]);
        fx_copy3(s->u.convex.axis[1], axis[1]);
        fx_copy3(s->u.convex.axis[2], axis[2]);
    } else {
        fx_identity_mat3(s->u.convex.axis);
    }
}

void sicol_shape_make_convex_identity(sicol_shape_t* s, const fx pos[3], fx verts[][3], int count)
{
    fx axis[3][3];
    fx_identity_mat3(axis);
    sicol_shape_make_convex(s, pos, verts, count, axis);
}

void sicol_shape_make_triangle(sicol_shape_t* s, const fx a[3], const fx b[3], const fx c[3])
{
    fx centroid[3];
    if (!s) return;
    s->type = SICOL_SHAPE_TRIANGLE;
    centroid[0] = (a[0] + b[0] + c[0]) / 3;
    centroid[1] = (a[1] + b[1] + c[1]) / 3;
    centroid[2] = (a[2] + b[2] + c[2]) / 3;
    fx_copy3(s->pos, centroid);
    fx_sub3(s->u.triangle.verts[0], a, centroid);
    fx_sub3(s->u.triangle.verts[1], b, centroid);
    fx_sub3(s->u.triangle.verts[2], c, centroid);
}

void sicol_shape_make_mesh_oriented(sicol_shape_t* s, const fx pos[3], const sicol_mesh_t* mesh, fx axis[3][3])
{
    if (!s) return;
    s->type = SICOL_SHAPE_MESH;
    fx_copy3(s->pos, pos);
    s->u.mesh.mesh = mesh;
    if (axis) {
        fx_copy3(s->u.mesh.axis[0], axis[0]);
        fx_copy3(s->u.mesh.axis[1], axis[1]);
        fx_copy3(s->u.mesh.axis[2], axis[2]);
    } else {
        fx_identity_mat3(s->u.mesh.axis);
    }
}

void sicol_shape_make_mesh(sicol_shape_t* s, const fx pos[3], const sicol_mesh_t* mesh)
{
    fx axis[3][3];
    fx_identity_mat3(axis);
    sicol_shape_make_mesh_oriented(s, pos, mesh, axis);
}

int sicol_shape_validate(const sicol_shape_t* s)
{
    fx len0;

    if (!s) return 0;

    switch (s->type) {
    case SICOL_SHAPE_NONE:
        return 0;

    case SICOL_SHAPE_AABB:
        return shape_half_valid(s->u.aabb.half);

    case SICOL_SHAPE_OBB:
        if (!shape_half_valid(s->u.obb.half)) return 0;
        return shape_basis_valid(s->u.obb.axis);

    case SICOL_SHAPE_PLANE:
        len0 = fx_len3(s->u.plane.normal);
        return (FX_ABS(len0 - FX_ONE) <= FX_FROM_RATIO(1, 8));

    case SICOL_SHAPE_RAY:
        if (s->u.ray.length < 0) return 0;
        len0 = fx_len3(s->u.ray.dir);
        return (FX_ABS(len0 - FX_ONE) <= FX_FROM_RATIO(1, 8));

    case SICOL_SHAPE_SPHERE:
        return (s->u.sphere.radius >= 0);

    case SICOL_SHAPE_CAPSULE:
        if (s->u.capsule.radius < 0) return 0;
        if (s->u.capsule.half_segment < 0) return 0;
        len0 = fx_len3(s->u.capsule.axis);
        return (FX_ABS(len0 - FX_ONE) <= FX_FROM_RATIO(1, 8));

    case SICOL_SHAPE_SEGMENT:
        return 1;

    case SICOL_SHAPE_CONVEX:
        if (s->u.convex.count < 1 || s->u.convex.count > SICOL_CONVEX_MAX_VERTS) return 0;
        return shape_basis_valid(s->u.convex.axis);

    case SICOL_SHAPE_TRIANGLE:
    {
        fx a[3];
        fx b[3];
        fx c[3];
        fx ab[3];
        fx ac[3];
        fx n[3];
        sicol_shape_get_triangle_points(s, a, b, c);
        fx_sub3(ab, b, a);
        fx_sub3(ac, c, a);
        fx_cross3(n, ab, ac);
        return (fx_len_sq3(n) > FX_EPSILON);
    }

    case SICOL_SHAPE_MESH:
        return (s->u.mesh.mesh && s->u.mesh.mesh->triangle_count > 0 && shape_basis_valid(s->u.mesh.axis));

    default:
        break;
    }

    return 0;
}

void sicol_shape_translate(sicol_shape_t* s, const fx delta[3])
{
    if (!s || !delta) return;
    fx_add3(s->pos, s->pos, delta);

    if (s->type == SICOL_SHAPE_SEGMENT) {
        fx_add3(s->u.segment.to, s->u.segment.to, delta);
    }

    if (s->type == SICOL_SHAPE_PLANE) {
        s->u.plane.d -= fx_dot3(s->u.plane.normal, delta);
    }
}



static void shape_build_basis_from_axis(const fx axis_in[3], fx out_basis[3][3])
{
    fx axis[3];
    fx helper[3];
    fx side[3];
    fx forward[3];

    if (!fx_normalize3(axis, axis_in)) {
        fx_identity_mat3(out_basis);
        return;
    }

    if (FX_ABS(axis[1]) < FX_FROM_RATIO(9, 10)) {
        fx_set3(helper, 0, FX_ONE, 0);
    } else {
        fx_set3(helper, FX_ONE, 0, 0);
    }

    fx_cross3(side, helper, axis);
    if (!fx_normalize3(side, side)) {
        fx_set3(side, FX_ONE, 0, 0);
    }
    fx_cross3(forward, axis, side);
    if (!fx_normalize3(forward, forward)) {
        fx_set3(forward, 0, 0, FX_ONE);
    }

    fx_copy3(out_basis[0], side);
    fx_copy3(out_basis[1], axis);
    fx_copy3(out_basis[2], forward);
}

static void shape_orthonormalize_basis(fx basis[3][3])
{
    fx x[3];
    fx y[3];
    fx z[3];
    fx proj[3];
    fx tmp[3];

    fx_copy3(x, basis[0]);
    if (!fx_normalize3(x, x)) {
        fx_set3(x, FX_ONE, 0, 0);
    }

    fx_copy3(y, basis[1]);
    fx_scale3(proj, x, fx_dot3(y, x));
    fx_sub3(y, y, proj);
    if (!fx_normalize3(y, y)) {
        if (FX_ABS(x[0]) < FX_FROM_RATIO(9, 10)) {
            fx_set3(tmp, FX_ONE, 0, 0);
        } else {
            fx_set3(tmp, 0, FX_ONE, 0);
        }
        fx_scale3(proj, x, fx_dot3(tmp, x));
        fx_sub3(y, tmp, proj);
        if (!fx_normalize3(y, y)) {
            fx_set3(y, 0, FX_ONE, 0);
        }
    }

    fx_cross3(z, x, y);
    if (!fx_normalize3(z, z)) {
        fx_set3(z, 0, 0, FX_ONE);
    }

    fx_copy3(basis[0], x);
    fx_copy3(basis[1], y);
    fx_copy3(basis[2], z);
}

void sicol_shape_get_basis(const sicol_shape_t* s, fx out_basis[3][3])
{
    if (!out_basis) return;
    fx_identity_mat3(out_basis);
    if (!s) return;

    switch (s->type) {
    case SICOL_SHAPE_OBB:
        fx_copy3(out_basis[0], s->u.obb.axis[0]);
        fx_copy3(out_basis[1], s->u.obb.axis[1]);
        fx_copy3(out_basis[2], s->u.obb.axis[2]);
        break;
    case SICOL_SHAPE_CONVEX:
        fx_copy3(out_basis[0], s->u.convex.axis[0]);
        fx_copy3(out_basis[1], s->u.convex.axis[1]);
        fx_copy3(out_basis[2], s->u.convex.axis[2]);
        break;
    case SICOL_SHAPE_CAPSULE:
        shape_build_basis_from_axis(s->u.capsule.axis, out_basis);
        break;
    case SICOL_SHAPE_MESH:
        fx_copy3(out_basis[0], s->u.mesh.axis[0]);
        fx_copy3(out_basis[1], s->u.mesh.axis[1]);
        fx_copy3(out_basis[2], s->u.mesh.axis[2]);
        break;
    default:
        fx_identity_mat3(out_basis);
        break;
    }
}

void sicol_shape_apply_angular_velocity(sicol_shape_t* s, const fx omega[3], fx dt)
{
    fx delta[3];
    fx tmp[3];
    fx center[3];
    fx a[3];
    fx b[3];
    fx rel_a[3];
    fx rel_b[3];
    int i;

    if (!s || !omega || dt == 0) return;
    fx_scale3(delta, omega, dt);
    if (fx_len_sq3(delta) <= FX_EPSILON) return;

    switch (s->type) {
    case SICOL_SHAPE_OBB:
        for (i = 0; i < 3; ++i) {
            fx_cross3(tmp, delta, s->u.obb.axis[i]);
            fx_add3(s->u.obb.axis[i], s->u.obb.axis[i], tmp);
        }
        shape_orthonormalize_basis(s->u.obb.axis);
        break;

    case SICOL_SHAPE_CONVEX:
        for (i = 0; i < 3; ++i) {
            fx_cross3(tmp, delta, s->u.convex.axis[i]);
            fx_add3(s->u.convex.axis[i], s->u.convex.axis[i], tmp);
        }
        shape_orthonormalize_basis(s->u.convex.axis);
        break;

    case SICOL_SHAPE_CAPSULE:
        fx_cross3(tmp, delta, s->u.capsule.axis);
        fx_add3(s->u.capsule.axis, s->u.capsule.axis, tmp);
        if (!fx_normalize3(s->u.capsule.axis, s->u.capsule.axis)) {
            fx_set3(s->u.capsule.axis, 0, FX_ONE, 0);
        }
        break;

    case SICOL_SHAPE_SEGMENT:
        sicol_shape_get_segment_points(s, a, b);
        center[0] = (a[0] + b[0]) / 2;
        center[1] = (a[1] + b[1]) / 2;
        center[2] = (a[2] + b[2]) / 2;
        fx_sub3(rel_a, a, center);
        fx_sub3(rel_b, b, center);
        fx_cross3(tmp, delta, rel_a);
        fx_add3(rel_a, rel_a, tmp);
        fx_cross3(tmp, delta, rel_b);
        fx_add3(rel_b, rel_b, tmp);
        fx_add3(s->pos, center, rel_a);
        fx_add3(s->u.segment.to, center, rel_b);
        break;

    case SICOL_SHAPE_RAY:
        fx_cross3(tmp, delta, s->u.ray.dir);
        fx_add3(s->u.ray.dir, s->u.ray.dir, tmp);
        if (!fx_normalize3(s->u.ray.dir, s->u.ray.dir)) {
            fx_set3(s->u.ray.dir, FX_ONE, 0, 0);
        }
        break;

    case SICOL_SHAPE_PLANE:
        fx_cross3(tmp, delta, s->u.plane.normal);
        fx_add3(s->u.plane.normal, s->u.plane.normal, tmp);
        if (!fx_normalize3(s->u.plane.normal, s->u.plane.normal)) {
            fx_set3(s->u.plane.normal, 0, FX_ONE, 0);
        }
        s->u.plane.d = -fx_dot3(s->u.plane.normal, s->pos);
        break;

    case SICOL_SHAPE_MESH:
        for (i = 0; i < 3; ++i) {
            fx_cross3(tmp, delta, s->u.mesh.axis[i]);
            fx_add3(s->u.mesh.axis[i], s->u.mesh.axis[i], tmp);
        }
        shape_orthonormalize_basis(s->u.mesh.axis);
        break;

    default:
        break;
    }
}

void sicol_shape_get_capsule_segment(const sicol_shape_t* s, fx out_a[3], fx out_b[3])
{
    fx axis_scaled[3];
    if (!s || s->type != SICOL_SHAPE_CAPSULE) {
        fx_zero3(out_a);
        fx_zero3(out_b);
        return;
    }

    fx_scale3(axis_scaled, s->u.capsule.axis, s->u.capsule.half_segment);
    fx_sub3(out_a, s->pos, axis_scaled);
    fx_add3(out_b, s->pos, axis_scaled);
}

void sicol_shape_get_segment_points(const sicol_shape_t* s, fx out_a[3], fx out_b[3])
{
    if (!s || s->type != SICOL_SHAPE_SEGMENT) {
        fx_zero3(out_a);
        fx_zero3(out_b);
        return;
    }
    fx_copy3(out_a, s->pos);
    fx_copy3(out_b, s->u.segment.to);
}

void sicol_shape_get_triangle_points(const sicol_shape_t* s, fx out_a[3], fx out_b[3], fx out_c[3])
{
    if (!s || s->type != SICOL_SHAPE_TRIANGLE) {
        fx_zero3(out_a);
        fx_zero3(out_b);
        fx_zero3(out_c);
        return;
    }
    fx_add3(out_a, s->pos, s->u.triangle.verts[0]);
    fx_add3(out_b, s->pos, s->u.triangle.verts[1]);
    fx_add3(out_c, s->pos, s->u.triangle.verts[2]);
}

void sicol_shape_mesh_local_to_world(const sicol_shape_t* s, const fx local[3], fx out_world[3])
{
    fx world_v[3];
    if (!out_world) return;
    if (!s || s->type != SICOL_SHAPE_MESH || !local) {
        fx_zero3(out_world);
        return;
    }
    fx_basis_to_world3(world_v, s->u.mesh.axis, local);
    fx_add3(out_world, world_v, s->pos);
}

void sicol_shape_mesh_world_to_local(const sicol_shape_t* s, const fx world[3], fx out_local[3])
{
    fx rel[3];
    if (!out_local) return;
    if (!s || s->type != SICOL_SHAPE_MESH || !world) {
        fx_zero3(out_local);
        return;
    }
    fx_sub3(rel, world, s->pos);
    fx_basis_to_local3(out_local, s->u.mesh.axis, rel);
}

void sicol_shape_mesh_world_aabb_to_local_aabb(const sicol_shape_t* s, const fx world_min[3], const fx world_max[3], fx out_local_min[3], fx out_local_max[3])
{
    fx corner[3];
    fx local_corner[3];
    int xi;
    int yi;
    int zi;
    int first = 1;

    if (!out_local_min || !out_local_max) return;
    if (!s || s->type != SICOL_SHAPE_MESH || !world_min || !world_max) {
        fx_zero3(out_local_min);
        fx_zero3(out_local_max);
        return;
    }

    for (xi = 0; xi < 2; ++xi) {
        corner[0] = xi ? world_max[0] : world_min[0];
        for (yi = 0; yi < 2; ++yi) {
            corner[1] = yi ? world_max[1] : world_min[1];
            for (zi = 0; zi < 2; ++zi) {
                corner[2] = zi ? world_max[2] : world_min[2];
                sicol_shape_mesh_world_to_local(s, corner, local_corner);
                if (first) {
                    fx_copy3(out_local_min, local_corner);
                    fx_copy3(out_local_max, local_corner);
                    first = 0;
                } else {
                    out_local_min[0] = FX_MIN(out_local_min[0], local_corner[0]);
                    out_local_min[1] = FX_MIN(out_local_min[1], local_corner[1]);
                    out_local_min[2] = FX_MIN(out_local_min[2], local_corner[2]);
                    out_local_max[0] = FX_MAX(out_local_max[0], local_corner[0]);
                    out_local_max[1] = FX_MAX(out_local_max[1], local_corner[1]);
                    out_local_max[2] = FX_MAX(out_local_max[2], local_corner[2]);
                }
            }
        }
    }
}

int sicol_shape_get_mesh_triangle_points(const sicol_shape_t* s, int triangle_index, fx out_a[3], fx out_b[3], fx out_c[3])
{
    sicol_triangle_verts_t tri;
    if (!s || s->type != SICOL_SHAPE_MESH || !s->u.mesh.mesh) {
        fx_zero3(out_a);
        fx_zero3(out_b);
        fx_zero3(out_c);
        return 0;
    }
    if (!sicol_mesh_get_triangle(s->u.mesh.mesh, triangle_index, &tri)) {
        fx_zero3(out_a);
        fx_zero3(out_b);
        fx_zero3(out_c);
        return 0;
    }
    sicol_shape_mesh_local_to_world(s, tri.v0, out_a);
    sicol_shape_mesh_local_to_world(s, tri.v1, out_b);
    sicol_shape_mesh_local_to_world(s, tri.v2, out_c);
    return 1;
}

void sicol_shape_compute_aabb(const sicol_shape_t* s, fx out_min[3], fx out_max[3])
{
    int i;
    fx extent[3];
    fx a[3];
    fx b[3];
    fx c[3];
    fx ab[3];
    fx world_v[3];

    if (!s) {
        fx_zero3(out_min);
        fx_zero3(out_max);
        return;
    }

    switch (s->type) {
    case SICOL_SHAPE_AABB:
        for (i = 0; i < 3; ++i) {
            out_min[i] = s->pos[i] - s->u.aabb.half[i];
            out_max[i] = s->pos[i] + s->u.aabb.half[i];
        }
        break;

    case SICOL_SHAPE_OBB:
        extent[0] =
            FX_ABS(FX_MUL(s->u.obb.axis[0][0], s->u.obb.half[0])) +
            FX_ABS(FX_MUL(s->u.obb.axis[1][0], s->u.obb.half[1])) +
            FX_ABS(FX_MUL(s->u.obb.axis[2][0], s->u.obb.half[2]));
        extent[1] =
            FX_ABS(FX_MUL(s->u.obb.axis[0][1], s->u.obb.half[0])) +
            FX_ABS(FX_MUL(s->u.obb.axis[1][1], s->u.obb.half[1])) +
            FX_ABS(FX_MUL(s->u.obb.axis[2][1], s->u.obb.half[2]));
        extent[2] =
            FX_ABS(FX_MUL(s->u.obb.axis[0][2], s->u.obb.half[0])) +
            FX_ABS(FX_MUL(s->u.obb.axis[1][2], s->u.obb.half[1])) +
            FX_ABS(FX_MUL(s->u.obb.axis[2][2], s->u.obb.half[2]));
        for (i = 0; i < 3; ++i) {
            out_min[i] = s->pos[i] - extent[i];
            out_max[i] = s->pos[i] + extent[i];
        }
        break;

    case SICOL_SHAPE_SPHERE:
        for (i = 0; i < 3; ++i) {
            out_min[i] = s->pos[i] - s->u.sphere.radius;
            out_max[i] = s->pos[i] + s->u.sphere.radius;
        }
        break;

    case SICOL_SHAPE_CAPSULE:
        sicol_shape_get_capsule_segment(s, a, b);
        for (i = 0; i < 3; ++i) {
            out_min[i] = FX_MIN(a[i], b[i]) - s->u.capsule.radius;
            out_max[i] = FX_MAX(a[i], b[i]) + s->u.capsule.radius;
        }
        break;

    case SICOL_SHAPE_SEGMENT:
        sicol_shape_get_segment_points(s, a, b);
        for (i = 0; i < 3; ++i) {
            out_min[i] = FX_MIN(a[i], b[i]);
            out_max[i] = FX_MAX(a[i], b[i]);
        }
        break;

    case SICOL_SHAPE_RAY:
        fx_scale3(ab, s->u.ray.dir, s->u.ray.length);
        fx_add3(b, s->pos, ab);
        for (i = 0; i < 3; ++i) {
            out_min[i] = FX_MIN(s->pos[i], b[i]);
            out_max[i] = FX_MAX(s->pos[i], b[i]);
        }
        break;

    case SICOL_SHAPE_CONVEX:
        fx_basis_to_world3(world_v, s->u.convex.axis, s->u.convex.verts[0]);
        fx_add3(world_v, world_v, s->pos);
        fx_copy3(out_min, world_v);
        fx_copy3(out_max, world_v);
        for (i = 1; i < s->u.convex.count; ++i) {
            fx_basis_to_world3(world_v, s->u.convex.axis, s->u.convex.verts[i]);
            fx_add3(world_v, world_v, s->pos);
            out_min[0] = FX_MIN(out_min[0], world_v[0]);
            out_min[1] = FX_MIN(out_min[1], world_v[1]);
            out_min[2] = FX_MIN(out_min[2], world_v[2]);
            out_max[0] = FX_MAX(out_max[0], world_v[0]);
            out_max[1] = FX_MAX(out_max[1], world_v[1]);
            out_max[2] = FX_MAX(out_max[2], world_v[2]);
        }
        break;

    case SICOL_SHAPE_TRIANGLE:
        sicol_shape_get_triangle_points(s, a, b, c);
        fx_copy3(out_min, a);
        fx_copy3(out_max, a);
        out_min[0] = FX_MIN(out_min[0], b[0]);
        out_min[1] = FX_MIN(out_min[1], b[1]);
        out_min[2] = FX_MIN(out_min[2], b[2]);
        out_max[0] = FX_MAX(out_max[0], b[0]);
        out_max[1] = FX_MAX(out_max[1], b[1]);
        out_max[2] = FX_MAX(out_max[2], b[2]);
        out_min[0] = FX_MIN(out_min[0], c[0]);
        out_min[1] = FX_MIN(out_min[1], c[1]);
        out_min[2] = FX_MIN(out_min[2], c[2]);
        out_max[0] = FX_MAX(out_max[0], c[0]);
        out_max[1] = FX_MAX(out_max[1], c[1]);
        out_max[2] = FX_MAX(out_max[2], c[2]);
        break;

    case SICOL_SHAPE_MESH:
        if (s->u.mesh.mesh) {
            fx local_center[3];
            fx local_half[3];
            fx world_center[3];
            local_center[0] = (s->u.mesh.mesh->min[0] + s->u.mesh.mesh->max[0]) / 2;
            local_center[1] = (s->u.mesh.mesh->min[1] + s->u.mesh.mesh->max[1]) / 2;
            local_center[2] = (s->u.mesh.mesh->min[2] + s->u.mesh.mesh->max[2]) / 2;
            local_half[0] = (s->u.mesh.mesh->max[0] - s->u.mesh.mesh->min[0]) / 2;
            local_half[1] = (s->u.mesh.mesh->max[1] - s->u.mesh.mesh->min[1]) / 2;
            local_half[2] = (s->u.mesh.mesh->max[2] - s->u.mesh.mesh->min[2]) / 2;
            fx_basis_to_world3(world_center, s->u.mesh.axis, local_center);
            fx_add3(world_center, world_center, s->pos);
            extent[0] =
                FX_ABS(FX_MUL(s->u.mesh.axis[0][0], local_half[0])) +
                FX_ABS(FX_MUL(s->u.mesh.axis[1][0], local_half[1])) +
                FX_ABS(FX_MUL(s->u.mesh.axis[2][0], local_half[2]));
            extent[1] =
                FX_ABS(FX_MUL(s->u.mesh.axis[0][1], local_half[0])) +
                FX_ABS(FX_MUL(s->u.mesh.axis[1][1], local_half[1])) +
                FX_ABS(FX_MUL(s->u.mesh.axis[2][1], local_half[2]));
            extent[2] =
                FX_ABS(FX_MUL(s->u.mesh.axis[0][2], local_half[0])) +
                FX_ABS(FX_MUL(s->u.mesh.axis[1][2], local_half[1])) +
                FX_ABS(FX_MUL(s->u.mesh.axis[2][2], local_half[2]));
            for (i = 0; i < 3; ++i) {
                out_min[i] = world_center[i] - extent[i];
                out_max[i] = world_center[i] + extent[i];
            }
        } else {
            fx_zero3(out_min);
            fx_zero3(out_max);
        }
        break;

    case SICOL_SHAPE_PLANE:
        for (i = 0; i < 3; ++i) {
            out_min[i] = -FX_FROM_INT(32768);
            out_max[i] =  FX_FROM_INT(32768);
        }
        break;

    default:
        fx_zero3(out_min);
        fx_zero3(out_max);
        break;
    }
}
