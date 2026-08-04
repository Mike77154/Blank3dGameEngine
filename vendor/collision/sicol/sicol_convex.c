/* ============================================================
 * SICOL - Support mapping helpers
 * ============================================================ */

#include "sicol_convex.h"

static void convex_safe_dir(const fx in_dir[3], fx out_dir[3])
{
    fx_copy3(out_dir, in_dir);
    if (fx_len_sq3(out_dir) <= FX_EPSILON) {
        fx_set3(out_dir, FX_ONE, 0, 0);
    }
}

int sicol_shape_is_support_mapped(const sicol_shape_t* s)
{
    if (!s) return 0;
    switch (s->type) {
    case SICOL_SHAPE_AABB:
    case SICOL_SHAPE_OBB:
    case SICOL_SHAPE_SPHERE:
    case SICOL_SHAPE_CAPSULE:
    case SICOL_SHAPE_SEGMENT:
    case SICOL_SHAPE_CONVEX:
    case SICOL_SHAPE_TRIANGLE:
        return 1;
    default:
        break;
    }
    return 0;
}

void sicol_shape_center_point(const sicol_shape_t* s, fx out[3])
{
    fx a[3];
    fx b[3];

    if (!s || !out) return;

    switch (s->type) {
    case SICOL_SHAPE_SEGMENT:
        sicol_shape_get_segment_points(s, a, b);
        out[0] = (a[0] + b[0]) / 2;
        out[1] = (a[1] + b[1]) / 2;
        out[2] = (a[2] + b[2]) / 2;
        break;

    case SICOL_SHAPE_TRIANGLE:
        fx_copy3(out, s->pos);
        break;

    default:
        fx_copy3(out, s->pos);
        break;
    }
}

int sicol_shape_support_point(const sicol_shape_t* s, const fx dir_in[3], fx out[3])
{
    fx dir[3];
    int i;

    if (!s || !out) return 0;
    convex_safe_dir(dir_in, dir);

    switch (s->type) {
    case SICOL_SHAPE_AABB:
        out[0] = s->pos[0] + ((dir[0] >= 0) ? s->u.aabb.half[0] : -s->u.aabb.half[0]);
        out[1] = s->pos[1] + ((dir[1] >= 0) ? s->u.aabb.half[1] : -s->u.aabb.half[1]);
        out[2] = s->pos[2] + ((dir[2] >= 0) ? s->u.aabb.half[2] : -s->u.aabb.half[2]);
        return 1;

    case SICOL_SHAPE_OBB:
    {
        fx point[3];
        fx axis_scale[3];
        fx_copy3(point, s->pos);
        for (i = 0; i < 3; ++i) {
            fx sign = (fx_dot3(dir, s->u.obb.axis[i]) >= 0) ? s->u.obb.half[i] : -s->u.obb.half[i];
            fx_scale3(axis_scale, s->u.obb.axis[i], sign);
            fx_add3(point, point, axis_scale);
        }
        fx_copy3(out, point);
        return 1;
    }

    case SICOL_SHAPE_SPHERE:
    {
        fx n[3];
        if (!fx_normalize3(n, dir)) {
            fx_set3(n, FX_ONE, 0, 0);
        }
        fx_madd3(out, s->pos, n, s->u.sphere.radius);
        return 1;
    }

    case SICOL_SHAPE_CAPSULE:
    {
        fx n[3];
        fx segment_point[3];
        fx axis_scaled[3];
        if (!fx_normalize3(n, dir)) {
            fx_set3(n, FX_ONE, 0, 0);
        }
        if (fx_dot3(dir, s->u.capsule.axis) >= 0) {
            fx_scale3(axis_scaled, s->u.capsule.axis, s->u.capsule.half_segment);
        } else {
            fx_scale3(axis_scaled, s->u.capsule.axis, -s->u.capsule.half_segment);
        }
        fx_add3(segment_point, s->pos, axis_scaled);
        fx_madd3(out, segment_point, n, s->u.capsule.radius);
        return 1;
    }

    case SICOL_SHAPE_SEGMENT:
    {
        fx a[3];
        fx b[3];
        sicol_shape_get_segment_points(s, a, b);
        if (fx_dot3(a, dir) > fx_dot3(b, dir)) {
            fx_copy3(out, a);
        } else {
            fx_copy3(out, b);
        }
        return 1;
    }

    case SICOL_SHAPE_CONVEX:
    {
        fx local_dir[3];
        fx best_dot;
        int best_i = 0;
        fx_basis_to_local3(local_dir, s->u.convex.axis, dir);
        best_dot = fx_dot3(local_dir, s->u.convex.verts[0]);
        for (i = 1; i < s->u.convex.count; ++i) {
            fx d = fx_dot3(local_dir, s->u.convex.verts[i]);
            if (d > best_dot) {
                best_dot = d;
                best_i = i;
            }
        }
        fx_basis_to_world3(out, s->u.convex.axis, s->u.convex.verts[best_i]);
        fx_add3(out, out, s->pos);
        return 1;
    }

    case SICOL_SHAPE_TRIANGLE:
    {
        fx best_point[3];
        fx best_dot;
        fx world_v[3];
        int best_i = 0;

        fx_add3(best_point, s->pos, s->u.triangle.verts[0]);
        best_dot = fx_dot3(best_point, dir);
        for (i = 1; i < 3; ++i) {
            fx_add3(world_v, s->pos, s->u.triangle.verts[i]);
            if (fx_dot3(world_v, dir) > best_dot) {
                best_dot = fx_dot3(world_v, dir);
                best_i = i;
            }
        }
        fx_add3(out, s->pos, s->u.triangle.verts[best_i]);
        return 1;
    }

    default:
        break;
    }

    fx_zero3(out);
    return 0;
}
