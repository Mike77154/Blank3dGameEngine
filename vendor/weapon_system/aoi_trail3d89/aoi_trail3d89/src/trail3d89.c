#include "trail3d89.h"
#include <string.h>
#include <limits.h>

static int t3d89_abs_i(int v)
{
    if (v < 0) {
        return -v;
    }
    return v;
}

static int t3d89_max_i(int a, int b)
{
    if (a > b) {
        return a;
    }
    return b;
}

static int t3d89_abs_safe_i(int v)
{
    if (v == INT_MIN) {
        return INT_MAX;
    }
    if (v < 0) {
        return -v;
    }
    return v;
}

static int t3d89_add_sat_i(int a, int b)
{
    if (b > 0 && a > (INT_MAX - b)) {
        return INT_MAX;
    }
    if (b < 0 && a < (INT_MIN - b)) {
        return INT_MIN;
    }
    return a + b;
}

static int t3d89_mul_sat_i(int a, int b)
{
    if (a == 0 || b == 0) {
        return 0;
    }
    if (a == -1 && b == INT_MIN) {
        return INT_MAX;
    }
    if (b == -1 && a == INT_MIN) {
        return INT_MAX;
    }
    if (a > 0) {
        if (b > 0) {
            if (a > (INT_MAX / b)) {
                return INT_MAX;
            }
        } else {
            if (b < (INT_MIN / a)) {
                return INT_MIN;
            }
        }
    } else {
        if (b > 0) {
            if (a < (INT_MIN / b)) {
                return INT_MIN;
            }
        } else {
            if (a < (INT_MAX / b)) {
                return INT_MAX;
            }
        }
    }
    return a * b;
}

/* Exact Q8.8 multiply without requiring a 64-bit C type. */
static int t3d89_fp_mul_i(int a, int b)
{
    int whole;
    int remainder;
    int part_whole;
    int part_fraction;

    whole = a / T3D89_FP_ONE;
    remainder = a % T3D89_FP_ONE;
    part_whole = t3d89_mul_sat_i(whole, b);
    part_fraction = t3d89_mul_sat_i(remainder, b) / T3D89_FP_ONE;
    return t3d89_add_sat_i(part_whole, part_fraction);
}

static void t3d89_transform_vector_internal(
    const t3d89_transform *transform,
    int x,
    int y,
    int z,
    int *out_x,
    int *out_y,
    int *out_z
)
{
    int sx;
    int sy;
    int sz;
    int rx;
    int ry;
    int rz;

    sx = t3d89_fp_mul_i(x, transform->sx);
    sy = t3d89_fp_mul_i(y, transform->sy);
    sz = t3d89_fp_mul_i(z, transform->sz);

    rx = t3d89_fp_mul_i(transform->m00, sx);
    rx = t3d89_add_sat_i(rx, t3d89_fp_mul_i(transform->m01, sy));
    rx = t3d89_add_sat_i(rx, t3d89_fp_mul_i(transform->m02, sz));

    ry = t3d89_fp_mul_i(transform->m10, sx);
    ry = t3d89_add_sat_i(ry, t3d89_fp_mul_i(transform->m11, sy));
    ry = t3d89_add_sat_i(ry, t3d89_fp_mul_i(transform->m12, sz));

    rz = t3d89_fp_mul_i(transform->m20, sx);
    rz = t3d89_add_sat_i(rz, t3d89_fp_mul_i(transform->m21, sy));
    rz = t3d89_add_sat_i(rz, t3d89_fp_mul_i(transform->m22, sz));

    *out_x = rx;
    *out_y = ry;
    *out_z = rz;
}

static void t3d89_transform_point_internal(
    const t3d89_transform *transform,
    int x,
    int y,
    int z,
    int *out_x,
    int *out_y,
    int *out_z
)
{
    int rx;
    int ry;
    int rz;

    t3d89_transform_vector_internal(transform, x, y, z, &rx, &ry, &rz);
    *out_x = t3d89_add_sat_i(transform->tx, rx);
    *out_y = t3d89_add_sat_i(transform->ty, ry);
    *out_z = t3d89_add_sat_i(transform->tz, rz);
}


static int t3d89_clamp_i(int v, int lo, int hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

static unsigned char t3d89_clamp_u8(int v)
{
    if (v < 0) {
        return (unsigned char)0;
    }
    if (v > 255) {
        return (unsigned char)255;
    }
    return (unsigned char)v;
}

static void t3d89_cross(int ax, int ay, int az, int bx, int by, int bz, int *rx, int *ry, int *rz)
{
    int sax;
    int say;
    int saz;
    int sbx;
    int sby;
    int sbz;

    sax = ax >> 4;
    say = ay >> 4;
    saz = az >> 4;
    sbx = bx >> 4;
    sby = by >> 4;
    sbz = bz >> 4;

    *rx = (say * sbz) - (saz * sby);
    *ry = (saz * sbx) - (sax * sbz);
    *rz = (sax * sby) - (say * sbx);
}

static int t3d89_dist2_shifted(int ax, int ay, int az, int bx, int by, int bz)
{
    int dx;
    int dy;
    int dz;
    int sx;
    int sy;
    int sz;

    dx = ax - bx;
    dy = ay - by;
    dz = az - bz;
    sx = dx >> 4;
    sy = dy >> 4;
    sz = dz >> 4;
    return sx * sx + sy * sy + sz * sz;
}

static int t3d89_len2_shifted_vec(int x, int y, int z)
{
    int sx;
    int sy;
    int sz;

    sx = x >> 4;
    sy = y >> 4;
    sz = z >> 4;
    return sx * sx + sy * sy + sz * sz;
}

static void t3d89_scale_to_len(int x, int y, int z, int len, int *rx, int *ry, int *rz)
{
    int ax;
    int ay;
    int az;
    int m;

    ax = t3d89_abs_i(x);
    ay = t3d89_abs_i(y);
    az = t3d89_abs_i(z);
    m = t3d89_max_i(ax, t3d89_max_i(ay, az));
    if (m <= 0) {
        *rx = len;
        *ry = 0;
        *rz = 0;
        return;
    }

    *rx = (x * len) / m;
    *ry = (y * len) / m;
    *rz = (z * len) / m;
}

static t3d89_color t3d89_lerp_color(t3d89_color a, t3d89_color b, int t_q8)
{
    t3d89_color out;
    int inv;

    t_q8 = t3d89_clamp_i(t_q8, 0, T3D89_FP_ONE);
    inv = T3D89_FP_ONE - t_q8;
    out.r = t3d89_clamp_u8(((int)a.r * inv + (int)b.r * t_q8) >> T3D89_FP_SHIFT);
    out.g = t3d89_clamp_u8(((int)a.g * inv + (int)b.g * t_q8) >> T3D89_FP_SHIFT);
    out.b = t3d89_clamp_u8(((int)a.b * inv + (int)b.b * t_q8) >> T3D89_FP_SHIFT);
    out.a = t3d89_clamp_u8(((int)a.a * inv + (int)b.a * t_q8) >> T3D89_FP_SHIFT);
    return out;
}

static int t3d89_lerp_int(int a, int b, int t_q8)
{
    int inv;

    t_q8 = t3d89_clamp_i(t_q8, 0, T3D89_FP_ONE);
    inv = T3D89_FP_ONE - t_q8;
    return (a * inv + b * t_q8) >> T3D89_FP_SHIFT;
}

static int t3d89_valid_id(int trail_id)
{
    if (trail_id < 0) {
        return 0;
    }
    if (trail_id >= T3D89_MAX_TRAILS) {
        return 0;
    }
    return 1;
}

static int t3d89_clamp_max_points(int v)
{
    if (v <= 0) {
        return T3D89_MAX_POINTS_PER_TRAIL;
    }
    if (v > T3D89_MAX_POINTS_PER_TRAIL) {
        return T3D89_MAX_POINTS_PER_TRAIL;
    }
    return v;
}

static int t3d89_point_index(const t3d89_trail *tr, int ordinal)
{
    int cap;
    int idx;

    cap = tr->desc.max_points;
    idx = tr->start + ordinal;
    while (idx >= cap) {
        idx -= cap;
    }
    return idx;
}

static t3d89_point *t3d89_point_at(t3d89_ctx *ctx, int trail_id, int ordinal)
{
    int idx;

    idx = t3d89_point_index(&ctx->trails[trail_id], ordinal);
    return &ctx->points[trail_id][idx];
}

static const t3d89_point *t3d89_point_at_const(const t3d89_ctx *ctx, int trail_id, int ordinal)
{
    int idx;

    idx = t3d89_point_index(&ctx->trails[trail_id], ordinal);
    return &ctx->points[trail_id][idx];
}

static void t3d89_age_one(t3d89_ctx *ctx, int trail_id, int dt_ticks)
{
    t3d89_trail *tr;
    int i;
    t3d89_point *p;

    tr = &ctx->trails[trail_id];
    if (!tr->active) {
        return;
    }

    i = 0;
    while (i < tr->count) {
        p = t3d89_point_at(ctx, trail_id, i);
        p->age += dt_ticks;
        i++;
    }

    while (tr->count > 0) {
        p = t3d89_point_at(ctx, trail_id, 0);
        if (p->age < tr->desc.life_ticks) {
            break;
        }
        tr->start++;
        if (tr->start >= tr->desc.max_points) {
            tr->start = 0;
        }
        tr->count--;
    }
}

static int t3d89_sampler_distance_pass(const t3d89_trail *tr, const t3d89_sample *sample)
{
    int dsq;
    int msq;
    int md;

    if (!tr->have_last) {
        return 1;
    }
    md = tr->desc.min_dist;
    if (md <= 0) {
        return 1;
    }
    dsq = t3d89_dist2_shifted(sample->x, sample->y, sample->z, tr->last_x, tr->last_y, tr->last_z);
    md = md >> 4;
    msq = md * md;
    if (dsq >= msq) {
        return 1;
    }
    return 0;
}

static int t3d89_sampler_time_pass(const t3d89_ctx *ctx, const t3d89_trail *tr)
{
    if (!tr->have_last) {
        return 1;
    }
    if (tr->desc.min_ticks <= 0) {
        return 1;
    }
    if ((ctx->tick - tr->last_emit_tick) >= tr->desc.min_ticks) {
        return 1;
    }
    return 0;
}

static int t3d89_sampler_curve_pass(const t3d89_trail *tr, const t3d89_sample *sample)
{
    int ex;
    int ey;
    int ez;
    int cd;
    int dsq;
    int csq;

    if (!tr->have_last) {
        return 1;
    }
    if (tr->count < 2) {
        return 0;
    }
    cd = tr->desc.curve_dist;
    if (cd <= 0) {
        return 0;
    }

    ex = tr->last_x + (tr->last_x - tr->prev_x);
    ey = tr->last_y + (tr->last_y - tr->prev_y);
    ez = tr->last_z + (tr->last_z - tr->prev_z);
    dsq = t3d89_dist2_shifted(sample->x, sample->y, sample->z, ex, ey, ez);
    cd = cd >> 4;
    csq = cd * cd;
    if (dsq >= csq) {
        return 1;
    }
    return 0;
}

static int t3d89_should_emit(const t3d89_ctx *ctx, const t3d89_trail *tr, const t3d89_sample *sample)
{
    int any;
    int pass;
    int needed;
    int got;

    if ((sample->flags & T3D89_EMIT_FORCE) != 0u) {
        return 1;
    }
    if ((tr->desc.sampler_mask & T3D89_SAMPLE_FORCE) != 0u) {
        return 1;
    }
    if (!tr->have_last) {
        return 1;
    }

    any = 0;
    needed = 0;
    got = 0;

    if ((tr->desc.sampler_mask & T3D89_SAMPLE_DISTANCE) != 0u) {
        needed++;
        pass = t3d89_sampler_distance_pass(tr, sample);
        if (pass) {
            got++;
            any = 1;
        }
    }
    if ((tr->desc.sampler_mask & T3D89_SAMPLE_TIME) != 0u) {
        needed++;
        pass = t3d89_sampler_time_pass(ctx, tr);
        if (pass) {
            got++;
            any = 1;
        }
    }
    if ((tr->desc.sampler_mask & T3D89_SAMPLE_CURVE) != 0u) {
        needed++;
        pass = t3d89_sampler_curve_pass(tr, sample);
        if (pass) {
            got++;
            any = 1;
        }
    }

    if (needed <= 0) {
        return 1;
    }
    if (tr->desc.sampler_mode == T3D89_SAMPLER_ALL) {
        if (got == needed) {
            return 1;
        }
        return 0;
    }
    return any;
}

static void t3d89_add_point(t3d89_ctx *ctx, int trail_id, const t3d89_sample *sample)
{
    t3d89_trail *tr;
    t3d89_point *p;
    int idx;
    int seg_dist;

    tr = &ctx->trails[trail_id];
    if (tr->count >= tr->desc.max_points) {
        tr->start++;
        if (tr->start >= tr->desc.max_points) {
            tr->start = 0;
        }
        tr->count--;
        tr->dropped_points++;
    }

    idx = tr->start + tr->count;
    while (idx >= tr->desc.max_points) {
        idx -= tr->desc.max_points;
    }
    p = &ctx->points[trail_id][idx];
    memset(p, 0, sizeof(*p));

    p->x = sample->x;
    p->y = sample->y;
    p->z = sample->z;
    p->a_x = sample->a_x;
    p->a_y = sample->a_y;
    p->a_z = sample->a_z;
    p->b_x = sample->b_x;
    p->b_y = sample->b_y;
    p->b_z = sample->b_z;
    p->right_x = sample->right_x;
    p->right_y = sample->right_y;
    p->right_z = sample->right_z;
    p->up_x = sample->up_x;
    p->up_y = sample->up_y;
    p->up_z = sample->up_z;
    p->width = sample->width;
    p->color = sample->color;
    p->flags = sample->flags;
    p->age = 0;

    if (sample->width <= 0) {
        p->width = tr->desc.width_head;
    }
    if ((sample->flags & T3D89_EMIT_HAS_COLOR) == 0u) {
        p->color = tr->desc.color_head;
    }

    if (tr->have_last) {
        seg_dist = t3d89_len2_shifted_vec(sample->x - tr->last_x, sample->y - tr->last_y, sample->z - tr->last_z);
        if (seg_dist > 0) {
            tr->total_uv_dist += tr->desc.min_dist;
        }
    }
    p->uv_dist = tr->total_uv_dist;

    tr->prev_x = tr->last_x;
    tr->prev_y = tr->last_y;
    tr->prev_z = tr->last_z;
    tr->last_x = sample->x;
    tr->last_y = sample->y;
    tr->last_z = sample->z;
    tr->last_emit_tick = ctx->tick;
    tr->have_last = 1;
    tr->count++;
}

static int t3d89_mesh_need(t3d89_mesh *mesh, int vertices, int indices)
{
    if (mesh == 0) {
        return 0;
    }
    if (mesh->vertices == 0) {
        return 0;
    }
    if (mesh->indices == 0) {
        return 0;
    }
    if ((mesh->vertex_count + vertices) > mesh->max_vertices) {
        mesh->overflowed = 1;
        return 0;
    }
    if ((mesh->index_count + indices) > mesh->max_indices) {
        mesh->overflowed = 1;
        return 0;
    }
    return 1;
}

static void t3d89_push_vertex(t3d89_mesh *mesh, int x, int y, int z, int u, int v, t3d89_color c)
{
    t3d89_vertex *out;

    out = &mesh->vertices[mesh->vertex_count];
    out->x = x;
    out->y = y;
    out->z = z;
    out->u = u;
    out->v = v;
    out->r = c.r;
    out->g = c.g;
    out->b = c.b;
    out->a = c.a;
    mesh->vertex_count++;
}

static void t3d89_push_tri(t3d89_mesh *mesh, int a, int b, int c)
{
    mesh->indices[mesh->index_count++] = (unsigned short)a;
    mesh->indices[mesh->index_count++] = (unsigned short)b;
    mesh->indices[mesh->index_count++] = (unsigned short)c;
}

static void t3d89_push_quad(t3d89_mesh *mesh, int a, int b, int c, int d)
{
    t3d89_push_tri(mesh, a, b, c);
    t3d89_push_tri(mesh, c, b, d);
}

static int t3d89_lod_stride(const t3d89_trail *tr, int verts_per_point)
{
    int budget;
    int stride;
    int estimate;

    stride = tr->desc.lod_min_stride;
    if (stride <= 0) {
        stride = 1;
    }
    budget = tr->desc.lod_vertex_budget;
    if (budget > 0) {
        estimate = tr->count * verts_per_point;
        while ((estimate / stride) > budget) {
            stride++;
        }
    }
    if (tr->desc.lod_max_stride > 0) {
        if (stride > tr->desc.lod_max_stride) {
            stride = tr->desc.lod_max_stride;
        }
    }
    if (stride < 1) {
        stride = 1;
    }
    return stride;
}

static int t3d89_age_t_q8(const t3d89_trail *tr, const t3d89_point *p)
{
    int t;

    if (tr->desc.life_ticks <= 0) {
        return T3D89_FP_ONE;
    }
    t = (p->age << T3D89_FP_SHIFT) / tr->desc.life_ticks;
    return t3d89_clamp_i(t, 0, T3D89_FP_ONE);
}

static int t3d89_point_width(const t3d89_trail *tr, const t3d89_point *p)
{
    int t;
    int w;

    t = t3d89_age_t_q8(tr, p);
    w = t3d89_lerp_int(tr->desc.width_head, tr->desc.width_tail, t);
    if ((p->flags & T3D89_EMIT_HAS_WIDTH) != 0u) {
        w = t3d89_lerp_int(p->width, tr->desc.width_tail, t);
    }
    if (w < 0) {
        w = 0;
    }
    return w;
}

static t3d89_color t3d89_point_color(const t3d89_trail *tr, const t3d89_point *p)
{
    int t;
    t3d89_color head;

    t = t3d89_age_t_q8(tr, p);
    head = tr->desc.color_head;
    if ((p->flags & T3D89_EMIT_HAS_COLOR) != 0u) {
        head = p->color;
    }
    return t3d89_lerp_color(head, tr->desc.color_tail, t);
}

static void t3d89_get_tangent(const t3d89_ctx *ctx, int trail_id, int ordinal, int stride, int *tx, int *ty, int *tz)
{
    const t3d89_trail *tr;
    const t3d89_point *a;
    const t3d89_point *b;
    int prev;
    int next;

    tr = &ctx->trails[trail_id];
    prev = ordinal - stride;
    next = ordinal + stride;
    if (prev < 0) {
        prev = ordinal;
    }
    if (next >= tr->count) {
        next = ordinal;
    }
    a = t3d89_point_at_const(ctx, trail_id, prev);
    b = t3d89_point_at_const(ctx, trail_id, next);
    *tx = b->x - a->x;
    *ty = b->y - a->y;
    *tz = b->z - a->z;
    if (*tx == 0 && *ty == 0 && *tz == 0) {
        *tx = T3D89_FP_ONE;
        *ty = 0;
        *tz = 0;
    }
}

static void t3d89_side_for_point(const t3d89_ctx *ctx, int trail_id, int ordinal, int stride, const t3d89_camera *camera, int side_mode, int width, int *sx, int *sy, int *sz)
{
    const t3d89_trail *tr;
    const t3d89_point *p;
    int tx;
    int ty;
    int tz;
    int ax;
    int ay;
    int az;
    int cx;
    int cy;
    int cz;
    int half;

    tr = &ctx->trails[trail_id];
    p = t3d89_point_at_const(ctx, trail_id, ordinal);
    half = width / 2;
    if (half <= 0) {
        half = 1;
    }

    if (side_mode == T3D89_MODE_ORIENTED_RIBBON && (p->right_x != 0 || p->right_y != 0 || p->right_z != 0)) {
        t3d89_scale_to_len(p->right_x, p->right_y, p->right_z, half, sx, sy, sz);
        return;
    }

    t3d89_get_tangent(ctx, trail_id, ordinal, stride, &tx, &ty, &tz);

    if (side_mode == T3D89_MODE_AXIS_RIBBON) {
        ax = tr->desc.axis_x;
        ay = tr->desc.axis_y;
        az = tr->desc.axis_z;
        if (ax == 0 && ay == 0 && az == 0) {
            ax = 0;
            ay = T3D89_FP_ONE;
            az = 0;
        }
        t3d89_cross(ax, ay, az, tx, ty, tz, &cx, &cy, &cz);
        t3d89_scale_to_len(cx, cy, cz, half, sx, sy, sz);
        return;
    }

    ax = camera->x - p->x;
    ay = camera->y - p->y;
    az = camera->z - p->z;
    if (ax == 0 && ay == 0 && az == 0) {
        ax = 0;
        ay = 0;
        az = T3D89_FP_ONE;
    }
    t3d89_cross(ax, ay, az, tx, ty, tz, &cx, &cy, &cz);
    if (cx == 0 && cy == 0 && cz == 0) {
        cx = camera->up_x;
        cy = camera->up_y;
        cz = camera->up_z;
        if (cx == 0 && cy == 0 && cz == 0) {
            cx = 0;
            cy = T3D89_FP_ONE;
            cz = 0;
        }
    }
    t3d89_scale_to_len(cx, cy, cz, half, sx, sy, sz);
}

static int t3d89_push_ribbon_vertices(t3d89_ctx *ctx, int trail_id, const t3d89_camera *camera, t3d89_mesh *mesh, int side_mode, int second_cross)
{
    t3d89_trail *tr;
    t3d89_point *p;
    int stride;
    int n;
    int i;
    int base;
    int sx;
    int sy;
    int sz;
    int width;
    int t_q8;
    int u;
    int v0;
    int v1;
    t3d89_color c;

    tr = &ctx->trails[trail_id];
    if (tr->count < 2) {
        return T3D89_ERR_EMPTY;
    }
    stride = t3d89_lod_stride(tr, 2);
    n = 0;
    i = 0;
    while (i < tr->count) {
        n++;
        i += stride;
    }
    if ((tr->count - 1) % stride != 0) {
        n++;
    }
    if (n < 2) {
        return T3D89_ERR_EMPTY;
    }
    if (!t3d89_mesh_need(mesh, n * 2, (n - 1) * 6)) {
        return T3D89_ERR_OVERFLOW;
    }

    base = mesh->vertex_count;
    n = 0;
    i = 0;
    while (i < tr->count) {
        p = t3d89_point_at(ctx, trail_id, i);
        width = t3d89_point_width(tr, p);
        t3d89_side_for_point(ctx, trail_id, i, stride, camera, side_mode, width, &sx, &sy, &sz);
        if (second_cross) {
            t3d89_get_tangent(ctx, trail_id, i, stride, &v0, &v1, &u);
            t3d89_cross(v0, v1, u, sx, sy, sz, &sx, &sy, &sz);
            t3d89_scale_to_len(sx, sy, sz, width / 2, &sx, &sy, &sz);
        }
        t_q8 = t3d89_age_t_q8(tr, p);
        c = t3d89_point_color(tr, p);
        if (tr->desc.uv_mode == T3D89_UV_BY_DISTANCE) {
            u = p->uv_dist;
            if (tr->desc.uv_tile_dist > 0) {
                u = p->uv_dist % tr->desc.uv_tile_dist;
            }
        } else {
            u = t_q8;
        }
        t3d89_push_vertex(mesh, p->x - sx, p->y - sy, p->z - sz, u, 0, c);
        t3d89_push_vertex(mesh, p->x + sx, p->y + sy, p->z + sz, u, T3D89_FP_ONE, c);
        n++;
        i += stride;
    }
    if ((tr->count - 1) % stride != 0) {
        i = tr->count - 1;
        p = t3d89_point_at(ctx, trail_id, i);
        width = t3d89_point_width(tr, p);
        t3d89_side_for_point(ctx, trail_id, i, stride, camera, side_mode, width, &sx, &sy, &sz);
        if (second_cross) {
            t3d89_get_tangent(ctx, trail_id, i, stride, &v0, &v1, &u);
            t3d89_cross(v0, v1, u, sx, sy, sz, &sx, &sy, &sz);
            t3d89_scale_to_len(sx, sy, sz, width / 2, &sx, &sy, &sz);
        }
        t_q8 = t3d89_age_t_q8(tr, p);
        c = t3d89_point_color(tr, p);
        if (tr->desc.uv_mode == T3D89_UV_BY_DISTANCE) {
            u = p->uv_dist;
            if (tr->desc.uv_tile_dist > 0) {
                u = p->uv_dist % tr->desc.uv_tile_dist;
            }
        } else {
            u = t_q8;
        }
        t3d89_push_vertex(mesh, p->x - sx, p->y - sy, p->z - sz, u, 0, c);
        t3d89_push_vertex(mesh, p->x + sx, p->y + sy, p->z + sz, u, T3D89_FP_ONE, c);
        n++;
    }

    i = 0;
    while (i < (n - 1)) {
        t3d89_push_quad(mesh, base + i * 2, base + i * 2 + 1, base + i * 2 + 2, base + i * 2 + 3);
        i++;
    }
    return T3D89_OK;
}

static int t3d89_build_cross(t3d89_ctx *ctx, int trail_id, const t3d89_camera *camera, t3d89_mesh *mesh)
{
    int rc;

    rc = t3d89_push_ribbon_vertices(ctx, trail_id, camera, mesh, T3D89_MODE_VIEW_RIBBON, 0);
    if (rc != T3D89_OK) {
        return rc;
    }
    rc = t3d89_push_ribbon_vertices(ctx, trail_id, camera, mesh, T3D89_MODE_VIEW_RIBBON, 1);
    return rc;
}

static int t3d89_build_tube_lite(t3d89_ctx *ctx, int trail_id, const t3d89_camera *camera, t3d89_mesh *mesh)
{
    t3d89_trail *tr;
    t3d89_point *p;
    int stride;
    int n;
    int sides;
    int i;
    int k;
    int base;
    int sx;
    int sy;
    int sz;
    int tx;
    int ty;
    int tz;
    int bx;
    int by;
    int bz;
    int width;
    int half;
    int u;
    t3d89_color c;

    tr = &ctx->trails[trail_id];
    if (tr->count < 2) {
        return T3D89_ERR_EMPTY;
    }
    sides = tr->desc.tube_sides;
    if (sides < 3) {
        sides = 4;
    }
    if (sides > 8) {
        sides = 8;
    }
    if (sides != 4) {
        sides = 4;
    }
    stride = t3d89_lod_stride(tr, sides);
    n = 0;
    i = 0;
    while (i < tr->count) {
        n++;
        i += stride;
    }
    if ((tr->count - 1) % stride != 0) {
        n++;
    }
    if (n < 2) {
        return T3D89_ERR_EMPTY;
    }
    if (!t3d89_mesh_need(mesh, n * sides, (n - 1) * sides * 6)) {
        return T3D89_ERR_OVERFLOW;
    }

    base = mesh->vertex_count;
    n = 0;
    i = 0;
    while (i < tr->count) {
        p = t3d89_point_at(ctx, trail_id, i);
        width = t3d89_point_width(tr, p);
        half = width / 2;
        t3d89_side_for_point(ctx, trail_id, i, stride, camera, T3D89_MODE_VIEW_RIBBON, width, &sx, &sy, &sz);
        t3d89_get_tangent(ctx, trail_id, i, stride, &tx, &ty, &tz);
        t3d89_cross(tx, ty, tz, sx, sy, sz, &bx, &by, &bz);
        t3d89_scale_to_len(bx, by, bz, half, &bx, &by, &bz);
        c = t3d89_point_color(tr, p);
        u = p->uv_dist;
        t3d89_push_vertex(mesh, p->x + sx, p->y + sy, p->z + sz, u, 0, c);
        t3d89_push_vertex(mesh, p->x + bx, p->y + by, p->z + bz, u, T3D89_FP_ONE / 4, c);
        t3d89_push_vertex(mesh, p->x - sx, p->y - sy, p->z - sz, u, T3D89_FP_ONE / 2, c);
        t3d89_push_vertex(mesh, p->x - bx, p->y - by, p->z - bz, u, (T3D89_FP_ONE * 3) / 4, c);
        n++;
        i += stride;
    }
    if ((tr->count - 1) % stride != 0) {
        i = tr->count - 1;
        p = t3d89_point_at(ctx, trail_id, i);
        width = t3d89_point_width(tr, p);
        half = width / 2;
        t3d89_side_for_point(ctx, trail_id, i, stride, camera, T3D89_MODE_VIEW_RIBBON, width, &sx, &sy, &sz);
        t3d89_get_tangent(ctx, trail_id, i, stride, &tx, &ty, &tz);
        t3d89_cross(tx, ty, tz, sx, sy, sz, &bx, &by, &bz);
        t3d89_scale_to_len(bx, by, bz, half, &bx, &by, &bz);
        c = t3d89_point_color(tr, p);
        u = p->uv_dist;
        t3d89_push_vertex(mesh, p->x + sx, p->y + sy, p->z + sz, u, 0, c);
        t3d89_push_vertex(mesh, p->x + bx, p->y + by, p->z + bz, u, T3D89_FP_ONE / 4, c);
        t3d89_push_vertex(mesh, p->x - sx, p->y - sy, p->z - sz, u, T3D89_FP_ONE / 2, c);
        t3d89_push_vertex(mesh, p->x - bx, p->y - by, p->z - bz, u, (T3D89_FP_ONE * 3) / 4, c);
        n++;
    }

    i = 0;
    while (i < (n - 1)) {
        k = 0;
        while (k < sides) {
            t3d89_push_quad(mesh, base + i * sides + k, base + i * sides + ((k + 1) % sides), base + (i + 1) * sides + k, base + (i + 1) * sides + ((k + 1) % sides));
            k++;
        }
        i++;
    }
    return T3D89_OK;
}

static int t3d89_build_socket_sweep(t3d89_ctx *ctx, int trail_id, t3d89_mesh *mesh)
{
    t3d89_trail *tr;
    t3d89_point *p;
    int stride;
    int n;
    int i;
    int base;
    int u;
    t3d89_color c;

    tr = &ctx->trails[trail_id];
    if (tr->count < 2) {
        return T3D89_ERR_EMPTY;
    }
    stride = t3d89_lod_stride(tr, 2);
    n = 0;
    i = 0;
    while (i < tr->count) {
        n++;
        i += stride;
    }
    if ((tr->count - 1) % stride != 0) {
        n++;
    }
    if (n < 2) {
        return T3D89_ERR_EMPTY;
    }
    if (!t3d89_mesh_need(mesh, n * 2, (n - 1) * 6)) {
        return T3D89_ERR_OVERFLOW;
    }

    base = mesh->vertex_count;
    n = 0;
    i = 0;
    while (i < tr->count) {
        p = t3d89_point_at(ctx, trail_id, i);
        c = t3d89_point_color(tr, p);
        u = p->uv_dist;
        t3d89_push_vertex(mesh, p->a_x, p->a_y, p->a_z, u, 0, c);
        t3d89_push_vertex(mesh, p->b_x, p->b_y, p->b_z, u, T3D89_FP_ONE, c);
        n++;
        i += stride;
    }
    if ((tr->count - 1) % stride != 0) {
        i = tr->count - 1;
        p = t3d89_point_at(ctx, trail_id, i);
        c = t3d89_point_color(tr, p);
        u = p->uv_dist;
        t3d89_push_vertex(mesh, p->a_x, p->a_y, p->a_z, u, 0, c);
        t3d89_push_vertex(mesh, p->b_x, p->b_y, p->b_z, u, T3D89_FP_ONE, c);
        n++;
    }

    i = 0;
    while (i < (n - 1)) {
        t3d89_push_quad(mesh, base + i * 2, base + i * 2 + 1, base + i * 2 + 2, base + i * 2 + 3);
        i++;
    }
    return T3D89_OK;
}

static int t3d89_build_beam_ab(t3d89_ctx *ctx, int trail_id, const t3d89_camera *camera, t3d89_mesh *mesh)
{
    t3d89_trail *tr;
    t3d89_point *p;
    int i;
    int stride;
    int n;
    int base;
    int ax;
    int ay;
    int az;
    int bx;
    int by;
    int bz;
    int vx;
    int vy;
    int vz;
    int tx;
    int ty;
    int tz;
    int sx;
    int sy;
    int sz;
    int half;
    int width;
    t3d89_color c;

    tr = &ctx->trails[trail_id];
    if (tr->count < 1) {
        return T3D89_ERR_EMPTY;
    }
    stride = t3d89_lod_stride(tr, 4);
    n = 0;
    i = 0;
    while (i < tr->count) {
        n++;
        i += stride;
    }
    if (!t3d89_mesh_need(mesh, n * 4, n * 6)) {
        return T3D89_ERR_OVERFLOW;
    }

    i = 0;
    while (i < tr->count) {
        p = t3d89_point_at(ctx, trail_id, i);
        ax = p->a_x;
        ay = p->a_y;
        az = p->a_z;
        bx = p->b_x;
        by = p->b_y;
        bz = p->b_z;
        tx = bx - ax;
        ty = by - ay;
        tz = bz - az;
        vx = camera->x - ((ax + bx) / 2);
        vy = camera->y - ((ay + by) / 2);
        vz = camera->z - ((az + bz) / 2);
        t3d89_cross(vx, vy, vz, tx, ty, tz, &sx, &sy, &sz);
        width = t3d89_point_width(tr, p);
        half = width / 2;
        t3d89_scale_to_len(sx, sy, sz, half, &sx, &sy, &sz);
        c = t3d89_point_color(tr, p);
        base = mesh->vertex_count;
        t3d89_push_vertex(mesh, ax - sx, ay - sy, az - sz, 0, 0, c);
        t3d89_push_vertex(mesh, ax + sx, ay + sy, az + sz, 0, T3D89_FP_ONE, c);
        t3d89_push_vertex(mesh, bx - sx, by - sy, bz - sz, T3D89_FP_ONE, 0, c);
        t3d89_push_vertex(mesh, bx + sx, by + sy, bz + sz, T3D89_FP_ONE, T3D89_FP_ONE, c);
        t3d89_push_quad(mesh, base, base + 1, base + 2, base + 3);
        i += stride;
    }
    return T3D89_OK;
}

void t3d89_init(t3d89_ctx *ctx)
{
    if (ctx == 0) {
        return;
    }
    memset(ctx, 0, sizeof(*ctx));
}

void t3d89_default_desc(t3d89_desc *desc)
{
    if (desc == 0) {
        return;
    }
    memset(desc, 0, sizeof(*desc));
    desc->mode = T3D89_MODE_VIEW_RIBBON;
    desc->max_points = T3D89_MAX_POINTS_PER_TRAIL;
    desc->life_ticks = 20;
    desc->sampler_mask = T3D89_SAMPLE_DISTANCE | T3D89_SAMPLE_TIME | T3D89_SAMPLE_CURVE;
    desc->sampler_mode = T3D89_SAMPLER_ANY;
    desc->min_ticks = 1;
    desc->min_dist = T3D89_FP_ONE / 8;
    desc->curve_dist = T3D89_FP_ONE / 4;
    desc->width_head = T3D89_FP_ONE / 4;
    desc->width_tail = 0;
    desc->color_head = t3d89_color_make(255, 255, 255, 255);
    desc->color_tail = t3d89_color_make(255, 255, 255, 0);
    desc->axis_x = 0;
    desc->axis_y = T3D89_FP_ONE;
    desc->axis_z = 0;
    desc->uv_mode = T3D89_UV_BY_DISTANCE;
    desc->uv_tile_dist = T3D89_FP_ONE;
    desc->tube_sides = 4;
    desc->lod_vertex_budget = 0;
    desc->lod_min_stride = 1;
    desc->lod_max_stride = 8;
}

t3d89_color t3d89_color_make(int r, int g, int b, int a)
{
    t3d89_color c;
    c.r = t3d89_clamp_u8(r);
    c.g = t3d89_clamp_u8(g);
    c.b = t3d89_clamp_u8(b);
    c.a = t3d89_clamp_u8(a);
    return c;
}

int t3d89_create(t3d89_ctx *ctx, const t3d89_desc *desc)
{
    int i;
    t3d89_desc d;

    if (ctx == 0 || desc == 0) {
        return T3D89_ERR_NULL;
    }
    d = *desc;
    d.max_points = t3d89_clamp_max_points(d.max_points);
    if (d.life_ticks <= 0) {
        return T3D89_ERR_BAD_DESC;
    }
    if (d.mode < T3D89_MODE_VIEW_RIBBON || d.mode > T3D89_MODE_SOCKET_SWEEP) {
        return T3D89_ERR_BAD_DESC;
    }

    i = 0;
    while (i < T3D89_MAX_TRAILS) {
        if (!ctx->trails[i].active) {
            memset(&ctx->trails[i], 0, sizeof(ctx->trails[i]));
            memset(ctx->points[i], 0, sizeof(ctx->points[i]));
            ctx->trails[i].active = 1;
            ctx->trails[i].enabled = 1;
            ctx->trails[i].desc = d;
            ctx->trails[i].last_emit_tick = ctx->tick;
            ctx->trails[i].provider_fn = 0;
            ctx->trails[i].provider_user = 0;
            ctx->active_trails++;
            return i;
        }
        i++;
    }
    return T3D89_ERR_FULL;
}

int t3d89_destroy(t3d89_ctx *ctx, int trail_id)
{
    if (ctx == 0) {
        return T3D89_ERR_NULL;
    }
    if (!t3d89_valid_id(trail_id)) {
        return T3D89_ERR_BAD_ID;
    }
    if (!ctx->trails[trail_id].active) {
        return T3D89_ERR_INACTIVE;
    }
    memset(&ctx->trails[trail_id], 0, sizeof(ctx->trails[trail_id]));
    memset(ctx->points[trail_id], 0, sizeof(ctx->points[trail_id]));
    if (ctx->active_trails > 0) {
        ctx->active_trails--;
    }
    return T3D89_OK;
}

int t3d89_reset(t3d89_ctx *ctx, int trail_id)
{
    t3d89_desc desc;
    int enabled;
    int provider_enabled;
    unsigned int provider_flags;
    t3d89_transform_provider_fn provider_fn;
    void *provider_user;
    t3d89_sample provider_local_sample;

    if (ctx == 0) {
        return T3D89_ERR_NULL;
    }
    if (!t3d89_valid_id(trail_id)) {
        return T3D89_ERR_BAD_ID;
    }
    if (!ctx->trails[trail_id].active) {
        return T3D89_ERR_INACTIVE;
    }
    desc = ctx->trails[trail_id].desc;
    enabled = ctx->trails[trail_id].enabled;
    provider_enabled = ctx->trails[trail_id].provider_enabled;
    provider_flags = ctx->trails[trail_id].provider_flags;
    provider_fn = ctx->trails[trail_id].provider_fn;
    provider_user = ctx->trails[trail_id].provider_user;
    provider_local_sample = ctx->trails[trail_id].provider_local_sample;
    memset(&ctx->trails[trail_id], 0, sizeof(ctx->trails[trail_id]));
    memset(ctx->points[trail_id], 0, sizeof(ctx->points[trail_id]));
    ctx->trails[trail_id].active = 1;
    ctx->trails[trail_id].enabled = enabled;
    ctx->trails[trail_id].desc = desc;
    ctx->trails[trail_id].last_emit_tick = ctx->tick;
    ctx->trails[trail_id].provider_enabled = provider_enabled;
    ctx->trails[trail_id].provider_flags = provider_flags;
    ctx->trails[trail_id].provider_fn = provider_fn;
    ctx->trails[trail_id].provider_user = provider_user;
    ctx->trails[trail_id].provider_local_sample = provider_local_sample;
    return T3D89_OK;
}

int t3d89_set_enabled(t3d89_ctx *ctx, int trail_id, int enabled)
{
    if (ctx == 0) {
        return T3D89_ERR_NULL;
    }
    if (!t3d89_valid_id(trail_id)) {
        return T3D89_ERR_BAD_ID;
    }
    if (!ctx->trails[trail_id].active) {
        return T3D89_ERR_INACTIVE;
    }
    ctx->trails[trail_id].enabled = enabled ? 1 : 0;
    return T3D89_OK;
}

void t3d89_transform_identity(t3d89_transform *transform)
{
    if (transform == 0) {
        return;
    }
    memset(transform, 0, sizeof(*transform));
    transform->m00 = T3D89_FP_ONE;
    transform->m11 = T3D89_FP_ONE;
    transform->m22 = T3D89_FP_ONE;
    transform->sx = T3D89_FP_ONE;
    transform->sy = T3D89_FP_ONE;
    transform->sz = T3D89_FP_ONE;
}

int t3d89_transform_sample(
    const t3d89_transform *transform,
    const t3d89_sample *local_sample,
    t3d89_sample *world_sample,
    unsigned int provider_flags
)
{
    int width_scale;

    if (transform == 0 || local_sample == 0 || world_sample == 0) {
        return T3D89_ERR_NULL;
    }

    *world_sample = *local_sample;

    t3d89_transform_point_internal(
        transform,
        local_sample->x,
        local_sample->y,
        local_sample->z,
        &world_sample->x,
        &world_sample->y,
        &world_sample->z
    );
    t3d89_transform_vector_internal(
        transform,
        local_sample->vx,
        local_sample->vy,
        local_sample->vz,
        &world_sample->vx,
        &world_sample->vy,
        &world_sample->vz
    );
    t3d89_transform_vector_internal(
        transform,
        local_sample->right_x,
        local_sample->right_y,
        local_sample->right_z,
        &world_sample->right_x,
        &world_sample->right_y,
        &world_sample->right_z
    );
    t3d89_transform_vector_internal(
        transform,
        local_sample->up_x,
        local_sample->up_y,
        local_sample->up_z,
        &world_sample->up_x,
        &world_sample->up_y,
        &world_sample->up_z
    );

    if ((local_sample->flags & T3D89_EMIT_HAS_AB) != 0u) {
        t3d89_transform_point_internal(
            transform,
            local_sample->a_x,
            local_sample->a_y,
            local_sample->a_z,
            &world_sample->a_x,
            &world_sample->a_y,
            &world_sample->a_z
        );
        t3d89_transform_point_internal(
            transform,
            local_sample->b_x,
            local_sample->b_y,
            local_sample->b_z,
            &world_sample->b_x,
            &world_sample->b_y,
            &world_sample->b_z
        );
    }

    if ((provider_flags & T3D89_PROVIDER_SCALE_WIDTH) != 0u &&
        (local_sample->flags & T3D89_EMIT_HAS_WIDTH) != 0u) {
        width_scale = t3d89_max_i(
            t3d89_abs_safe_i(transform->sx),
            t3d89_max_i(
                t3d89_abs_safe_i(transform->sy),
                t3d89_abs_safe_i(transform->sz)
            )
        );
        world_sample->width = t3d89_fp_mul_i(local_sample->width, width_scale);
    }

    return T3D89_OK;
}

int t3d89_bind_transform_provider(
    t3d89_ctx *ctx,
    int trail_id,
    t3d89_transform_provider_fn provider_fn,
    void *provider_user,
    const t3d89_sample *local_sample,
    unsigned int provider_flags
)
{
    t3d89_trail *tr;

    if (ctx == 0 || provider_fn == 0 || local_sample == 0) {
        return T3D89_ERR_NULL;
    }
    if (!t3d89_valid_id(trail_id)) {
        return T3D89_ERR_BAD_ID;
    }
    tr = &ctx->trails[trail_id];
    if (!tr->active) {
        return T3D89_ERR_INACTIVE;
    }

    tr->provider_fn = provider_fn;
    tr->provider_user = provider_user;
    tr->provider_local_sample = *local_sample;
    tr->provider_flags = provider_flags;
    tr->provider_enabled = 1;
    tr->provider_calls = 0;
    tr->provider_skips = 0;
    tr->provider_errors = 0;
    tr->provider_last_result = T3D89_OK;
    return T3D89_OK;
}

int t3d89_unbind_transform_provider(t3d89_ctx *ctx, int trail_id)
{
    t3d89_trail *tr;

    if (ctx == 0) {
        return T3D89_ERR_NULL;
    }
    if (!t3d89_valid_id(trail_id)) {
        return T3D89_ERR_BAD_ID;
    }
    tr = &ctx->trails[trail_id];
    if (!tr->active) {
        return T3D89_ERR_INACTIVE;
    }

    tr->provider_enabled = 0;
    tr->provider_flags = 0u;
    tr->provider_fn = 0;
    tr->provider_user = 0;
    t3d89_sample_clear(&tr->provider_local_sample);
    tr->provider_calls = 0;
    tr->provider_skips = 0;
    tr->provider_errors = 0;
    tr->provider_last_result = T3D89_OK;
    return T3D89_OK;
}

int t3d89_set_provider_enabled(t3d89_ctx *ctx, int trail_id, int enabled)
{
    t3d89_trail *tr;

    if (ctx == 0) {
        return T3D89_ERR_NULL;
    }
    if (!t3d89_valid_id(trail_id)) {
        return T3D89_ERR_BAD_ID;
    }
    tr = &ctx->trails[trail_id];
    if (!tr->active) {
        return T3D89_ERR_INACTIVE;
    }
    if (tr->provider_fn == 0) {
        return T3D89_ERR_PROVIDER;
    }
    tr->provider_enabled = enabled ? 1 : 0;
    return T3D89_OK;
}

int t3d89_set_provider_local_sample(
    t3d89_ctx *ctx,
    int trail_id,
    const t3d89_sample *local_sample
)
{
    t3d89_trail *tr;

    if (ctx == 0 || local_sample == 0) {
        return T3D89_ERR_NULL;
    }
    if (!t3d89_valid_id(trail_id)) {
        return T3D89_ERR_BAD_ID;
    }
    tr = &ctx->trails[trail_id];
    if (!tr->active) {
        return T3D89_ERR_INACTIVE;
    }
    if (tr->provider_fn == 0) {
        return T3D89_ERR_PROVIDER;
    }
    tr->provider_local_sample = *local_sample;
    return T3D89_OK;
}

int t3d89_step_provider(t3d89_ctx *ctx, int trail_id)
{
    t3d89_trail *tr;
    t3d89_transform transform;
    t3d89_sample world_sample;
    int provider_result;
    int rc;

    if (ctx == 0) {
        return T3D89_ERR_NULL;
    }
    if (!t3d89_valid_id(trail_id)) {
        return T3D89_ERR_BAD_ID;
    }
    tr = &ctx->trails[trail_id];
    if (!tr->active) {
        return T3D89_ERR_INACTIVE;
    }
    if (!tr->provider_enabled || !tr->enabled) {
        return T3D89_OK;
    }
    if (tr->provider_fn == 0) {
        tr->provider_errors++;
        tr->provider_last_result = T3D89_ERR_PROVIDER;
        return T3D89_ERR_PROVIDER;
    }

    t3d89_transform_identity(&transform);
    tr->provider_calls++;
    provider_result = tr->provider_fn(
        tr->provider_user,
        trail_id,
        ctx->tick,
        &transform
    );
    tr->provider_last_result = provider_result;

    if (provider_result == T3D89_PROVIDER_SKIP) {
        tr->provider_skips++;
        return T3D89_OK;
    }
    if (provider_result < T3D89_PROVIDER_SKIP) {
        tr->provider_errors++;
        return T3D89_ERR_PROVIDER;
    }

    rc = t3d89_transform_sample(
        &transform,
        &tr->provider_local_sample,
        &world_sample,
        tr->provider_flags
    );
    if (rc != T3D89_OK) {
        tr->provider_errors++;
        tr->provider_last_result = rc;
        return rc;
    }

    rc = t3d89_emit_sample(ctx, trail_id, &world_sample);
    if (rc != T3D89_OK) {
        tr->provider_errors++;
        tr->provider_last_result = rc;
        return rc;
    }
    return T3D89_OK;
}

int t3d89_update_providers(t3d89_ctx *ctx)
{
    int i;
    int rc;
    int first_error;
    t3d89_trail *tr;

    if (ctx == 0) {
        return T3D89_ERR_NULL;
    }

    first_error = T3D89_OK;
    i = 0;
    while (i < T3D89_MAX_TRAILS) {
        tr = &ctx->trails[i];
        if (tr->active && tr->provider_enabled &&
            (tr->provider_flags & T3D89_PROVIDER_AUTOSTEP) != 0u) {
            rc = t3d89_step_provider(ctx, i);
            if (rc != T3D89_OK && first_error == T3D89_OK) {
                first_error = rc;
            }
        }
        i++;
    }
    return first_error;
}

int t3d89_get_provider_last_result(const t3d89_ctx *ctx, int trail_id)
{
    if (ctx == 0) {
        return T3D89_ERR_NULL;
    }
    if (!t3d89_valid_id(trail_id)) {
        return T3D89_ERR_BAD_ID;
    }
    if (!ctx->trails[trail_id].active) {
        return T3D89_ERR_INACTIVE;
    }
    return ctx->trails[trail_id].provider_last_result;
}

int t3d89_get_provider_call_count(const t3d89_ctx *ctx, int trail_id)
{
    if (ctx == 0) {
        return T3D89_ERR_NULL;
    }
    if (!t3d89_valid_id(trail_id)) {
        return T3D89_ERR_BAD_ID;
    }
    if (!ctx->trails[trail_id].active) {
        return T3D89_ERR_INACTIVE;
    }
    return ctx->trails[trail_id].provider_calls;
}

int t3d89_get_provider_skip_count(const t3d89_ctx *ctx, int trail_id)
{
    if (ctx == 0) {
        return T3D89_ERR_NULL;
    }
    if (!t3d89_valid_id(trail_id)) {
        return T3D89_ERR_BAD_ID;
    }
    if (!ctx->trails[trail_id].active) {
        return T3D89_ERR_INACTIVE;
    }
    return ctx->trails[trail_id].provider_skips;
}

int t3d89_get_provider_error_count(const t3d89_ctx *ctx, int trail_id)
{
    if (ctx == 0) {
        return T3D89_ERR_NULL;
    }
    if (!t3d89_valid_id(trail_id)) {
        return T3D89_ERR_BAD_ID;
    }
    if (!ctx->trails[trail_id].active) {
        return T3D89_ERR_INACTIVE;
    }
    return ctx->trails[trail_id].provider_errors;
}

int t3d89_tick(t3d89_ctx *ctx, int dt_ticks)
{
    int i;

    if (ctx == 0) {
        return T3D89_ERR_NULL;
    }
    if (dt_ticks < 0) {
        dt_ticks = 0;
    }
    ctx->tick += dt_ticks;
    i = 0;
    while (i < T3D89_MAX_TRAILS) {
        t3d89_age_one(ctx, i, dt_ticks);
        i++;
    }
    return t3d89_update_providers(ctx);
}

int t3d89_emit_sample(t3d89_ctx *ctx, int trail_id, const t3d89_sample *sample)
{
    t3d89_trail *tr;

    if (ctx == 0 || sample == 0) {
        return T3D89_ERR_NULL;
    }
    if (!t3d89_valid_id(trail_id)) {
        return T3D89_ERR_BAD_ID;
    }
    tr = &ctx->trails[trail_id];
    if (!tr->active) {
        return T3D89_ERR_INACTIVE;
    }
    if (!tr->enabled) {
        return T3D89_OK;
    }
    if (!t3d89_should_emit(ctx, tr, sample)) {
        tr->rejected_samples++;
        return T3D89_OK;
    }
    t3d89_add_point(ctx, trail_id, sample);
    return T3D89_OK;
}

int t3d89_emit_point(t3d89_ctx *ctx, int trail_id, int x, int y, int z)
{
    t3d89_sample s;

    t3d89_sample_clear(&s);
    s.x = x;
    s.y = y;
    s.z = z;
    return t3d89_emit_sample(ctx, trail_id, &s);
}

int t3d89_emit_oriented(t3d89_ctx *ctx, int trail_id, int x, int y, int z, int right_x, int right_y, int right_z, int up_x, int up_y, int up_z)
{
    t3d89_sample s;

    t3d89_sample_clear(&s);
    s.x = x;
    s.y = y;
    s.z = z;
    s.right_x = right_x;
    s.right_y = right_y;
    s.right_z = right_z;
    s.up_x = up_x;
    s.up_y = up_y;
    s.up_z = up_z;
    s.flags = T3D89_EMIT_HAS_ORIENT;
    return t3d89_emit_sample(ctx, trail_id, &s);
}

int t3d89_emit_segment(t3d89_ctx *ctx, int trail_id, int ax, int ay, int az, int bx, int by, int bz)
{
    t3d89_sample s;

    t3d89_sample_clear(&s);
    s.x = (ax + bx) / 2;
    s.y = (ay + by) / 2;
    s.z = (az + bz) / 2;
    s.a_x = ax;
    s.a_y = ay;
    s.a_z = az;
    s.b_x = bx;
    s.b_y = by;
    s.b_z = bz;
    s.flags = T3D89_EMIT_HAS_AB;
    return t3d89_emit_sample(ctx, trail_id, &s);
}

int t3d89_build_mesh(t3d89_ctx *ctx, int trail_id, const t3d89_camera *camera, t3d89_mesh *mesh)
{
    t3d89_camera cam;
    t3d89_trail *tr;
    int rc;

    if (ctx == 0 || mesh == 0) {
        return T3D89_ERR_NULL;
    }
    if (!t3d89_valid_id(trail_id)) {
        return T3D89_ERR_BAD_ID;
    }
    if (!ctx->trails[trail_id].active) {
        return T3D89_ERR_INACTIVE;
    }
    if (mesh->vertices == 0 || mesh->indices == 0) {
        return T3D89_ERR_NULL;
    }
    if ((mesh->flags & T3D89_BUILD_APPEND) == 0u) {
        t3d89_mesh_clear(mesh);
    }

    cam.x = 0;
    cam.y = 0;
    cam.z = T3D89_FP_ONE * 8;
    cam.up_x = 0;
    cam.up_y = T3D89_FP_ONE;
    cam.up_z = 0;
    if (camera != 0) {
        cam = *camera;
    }
    if (cam.up_x == 0 && cam.up_y == 0 && cam.up_z == 0) {
        cam.up_y = T3D89_FP_ONE;
    }

    tr = &ctx->trails[trail_id];
    if (tr->count <= 0) {
        return T3D89_ERR_EMPTY;
    }

    rc = T3D89_ERR_EMPTY;
    if (tr->desc.mode == T3D89_MODE_VIEW_RIBBON) {
        rc = t3d89_push_ribbon_vertices(ctx, trail_id, &cam, mesh, T3D89_MODE_VIEW_RIBBON, 0);
    } else if (tr->desc.mode == T3D89_MODE_AXIS_RIBBON) {
        rc = t3d89_push_ribbon_vertices(ctx, trail_id, &cam, mesh, T3D89_MODE_AXIS_RIBBON, 0);
    } else if (tr->desc.mode == T3D89_MODE_ORIENTED_RIBBON) {
        rc = t3d89_push_ribbon_vertices(ctx, trail_id, &cam, mesh, T3D89_MODE_ORIENTED_RIBBON, 0);
    } else if (tr->desc.mode == T3D89_MODE_CROSS_RIBBON) {
        rc = t3d89_build_cross(ctx, trail_id, &cam, mesh);
    } else if (tr->desc.mode == T3D89_MODE_TUBE_LITE) {
        rc = t3d89_build_tube_lite(ctx, trail_id, &cam, mesh);
    } else if (tr->desc.mode == T3D89_MODE_BEAM_AB) {
        rc = t3d89_build_beam_ab(ctx, trail_id, &cam, mesh);
    } else if (tr->desc.mode == T3D89_MODE_SOCKET_SWEEP) {
        rc = t3d89_build_socket_sweep(ctx, trail_id, mesh);
    }
    return rc;
}

int t3d89_build_all(t3d89_ctx *ctx, const t3d89_camera *camera, t3d89_mesh *mesh)
{
    int i;
    int rc;
    int first_err;

    if (ctx == 0 || mesh == 0) {
        return T3D89_ERR_NULL;
    }
    t3d89_mesh_clear(mesh);
    mesh->flags |= T3D89_BUILD_APPEND;
    first_err = T3D89_OK;
    i = 0;
    while (i < T3D89_MAX_TRAILS) {
        if (ctx->trails[i].active && ctx->trails[i].count > 0) {
            rc = t3d89_build_mesh(ctx, i, camera, mesh);
            if (rc != T3D89_OK && rc != T3D89_ERR_EMPTY && first_err == T3D89_OK) {
                first_err = rc;
            }
        }
        i++;
    }
    mesh->flags &= ~T3D89_BUILD_APPEND;
    return first_err;
}

int t3d89_get_point_count(const t3d89_ctx *ctx, int trail_id)
{
    if (ctx == 0) {
        return T3D89_ERR_NULL;
    }
    if (!t3d89_valid_id(trail_id)) {
        return T3D89_ERR_BAD_ID;
    }
    if (!ctx->trails[trail_id].active) {
        return T3D89_ERR_INACTIVE;
    }
    return ctx->trails[trail_id].count;
}

int t3d89_get_dropped_count(const t3d89_ctx *ctx, int trail_id)
{
    if (ctx == 0) {
        return T3D89_ERR_NULL;
    }
    if (!t3d89_valid_id(trail_id)) {
        return T3D89_ERR_BAD_ID;
    }
    if (!ctx->trails[trail_id].active) {
        return T3D89_ERR_INACTIVE;
    }
    return ctx->trails[trail_id].dropped_points;
}

int t3d89_get_rejected_count(const t3d89_ctx *ctx, int trail_id)
{
    if (ctx == 0) {
        return T3D89_ERR_NULL;
    }
    if (!t3d89_valid_id(trail_id)) {
        return T3D89_ERR_BAD_ID;
    }
    if (!ctx->trails[trail_id].active) {
        return T3D89_ERR_INACTIVE;
    }
    return ctx->trails[trail_id].rejected_samples;
}

void t3d89_sample_clear(t3d89_sample *sample)
{
    if (sample == 0) {
        return;
    }
    memset(sample, 0, sizeof(*sample));
}

void t3d89_mesh_clear(t3d89_mesh *mesh)
{
    if (mesh == 0) {
        return;
    }
    mesh->vertex_count = 0;
    mesh->index_count = 0;
    mesh->overflowed = 0;
}

int t3d89_fp_from_int(int v)
{
    return v << T3D89_FP_SHIFT;
}

int t3d89_fp_to_int(int v)
{
    return v >> T3D89_FP_SHIFT;
}
