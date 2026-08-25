#include "hurtbox3d.h"

#define HB3_EPSILON 1

static hb3_fx hb3_fx_abs(hb3_fx a)
{
    if (a < 0) {
        return -a;
    }
    return a;
}

static hb3_fx hb3_fx_max(hb3_fx a, hb3_fx b)
{
    return a > b ? a : b;
}

static hb3_fx hb3_fx_min(hb3_fx a, hb3_fx b)
{
    return a < b ? a : b;
}

static hb3_fx hb3_fx_clamp(hb3_fx v, hb3_fx lo, hb3_fx hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

hb3_fx hb3_fx_from_int(int x)
{
    return (hb3_fx)(x * HB3_FX_ONE);
}

int hb3_fx_to_int(hb3_fx x)
{
    return (int)(x / HB3_FX_ONE);
}

hb3_fx hb3_fx_mul(hb3_fx a, hb3_fx b)
{
    long r;
    r = ((long)a * (long)b) >> HB3_FX_SHIFT;
    return (hb3_fx)r;
}

hb3_fx hb3_fx_div(hb3_fx a, hb3_fx b)
{
    long r;
    if (b == 0) {
        return 0;
    }
    r = ((long)a * (long)HB3_FX_ONE) / (long)b;
    return (hb3_fx)r;
}

hb3_v3 hb3_v3_make(hb3_fx x, hb3_fx y, hb3_fx z)
{
    hb3_v3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

hb3_v3 hb3_v3_from_ints(int x, int y, int z)
{
    return hb3_v3_make(hb3_fx_from_int(x), hb3_fx_from_int(y), hb3_fx_from_int(z));
}

hb3_v3 hb3_v3_add(hb3_v3 a, hb3_v3 b)
{
    hb3_v3 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}

hb3_v3 hb3_v3_sub(hb3_v3 a, hb3_v3 b)
{
    hb3_v3 r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    return r;
}

hb3_v3 hb3_v3_scale(hb3_v3 a, hb3_fx s)
{
    hb3_v3 r;
    r.x = hb3_fx_mul(a.x, s);
    r.y = hb3_fx_mul(a.y, s);
    r.z = hb3_fx_mul(a.z, s);
    return r;
}

hb3_fx hb3_v3_dot(hb3_v3 a, hb3_v3 b)
{
    hb3_fx x;
    hb3_fx y;
    hb3_fx z;
    x = hb3_fx_mul(a.x, b.x);
    y = hb3_fx_mul(a.y, b.y);
    z = hb3_fx_mul(a.z, b.z);
    return x + y + z;
}

hb3_v3 hb3_v3_cross(hb3_v3 a, hb3_v3 b)
{
    hb3_v3 r;
    r.x = hb3_fx_mul(a.y, b.z) - hb3_fx_mul(a.z, b.y);
    r.y = hb3_fx_mul(a.z, b.x) - hb3_fx_mul(a.x, b.z);
    r.z = hb3_fx_mul(a.x, b.y) - hb3_fx_mul(a.y, b.x);
    return r;
}

hb3_fx hb3_v3_len2(hb3_v3 a)
{
    return hb3_v3_dot(a, a);
}

hb3_mat3 hb3_mat3_identity(void)
{
    hb3_mat3 m;
    m.x = hb3_v3_make(HB3_FX_ONE, 0, 0);
    m.y = hb3_v3_make(0, HB3_FX_ONE, 0);
    m.z = hb3_v3_make(0, 0, HB3_FX_ONE);
    return m;
}

hb3_mat3 hb3_mat3_from_axes(hb3_v3 x_axis, hb3_v3 y_axis, hb3_v3 z_axis)
{
    hb3_mat3 m;
    m.x = x_axis;
    m.y = y_axis;
    m.z = z_axis;
    return m;
}

hb3_mat3 hb3_mat3_rot_x_cs(hb3_fx c, hb3_fx s)
{
    hb3_mat3 m;
    m.x = hb3_v3_make(HB3_FX_ONE, 0, 0);
    m.y = hb3_v3_make(0, c, s);
    m.z = hb3_v3_make(0, -s, c);
    return m;
}

hb3_mat3 hb3_mat3_rot_y_cs(hb3_fx c, hb3_fx s)
{
    hb3_mat3 m;
    m.x = hb3_v3_make(c, 0, -s);
    m.y = hb3_v3_make(0, HB3_FX_ONE, 0);
    m.z = hb3_v3_make(s, 0, c);
    return m;
}

hb3_mat3 hb3_mat3_rot_z_cs(hb3_fx c, hb3_fx s)
{
    hb3_mat3 m;
    m.x = hb3_v3_make(c, s, 0);
    m.y = hb3_v3_make(-s, c, 0);
    m.z = hb3_v3_make(0, 0, HB3_FX_ONE);
    return m;
}

hb3_v3 hb3_mat3_mul_v3(hb3_mat3 m, hb3_v3 v)
{
    hb3_v3 r;
    r = hb3_v3_add(hb3_v3_scale(m.x, v.x), hb3_v3_scale(m.y, v.y));
    r = hb3_v3_add(r, hb3_v3_scale(m.z, v.z));
    return r;
}

hb3_v3 hb3_mat3_tmul_v3(hb3_mat3 m, hb3_v3 v)
{
    hb3_v3 r;
    r.x = hb3_v3_dot(v, m.x);
    r.y = hb3_v3_dot(v, m.y);
    r.z = hb3_v3_dot(v, m.z);
    return r;
}

hb3_mat3 hb3_mat3_mul(hb3_mat3 a, hb3_mat3 b)
{
    hb3_mat3 r;
    r.x = hb3_mat3_mul_v3(a, b.x);
    r.y = hb3_mat3_mul_v3(a, b.y);
    r.z = hb3_mat3_mul_v3(a, b.z);
    return r;
}

static hb3_v3 hb3_closest_point_seg(hb3_v3 p, hb3_v3 a, hb3_v3 b)
{
    hb3_v3 ab;
    hb3_v3 ap;
    hb3_fx ab_len2;
    hb3_fx t;
    ab = hb3_v3_sub(b, a);
    ap = hb3_v3_sub(p, a);
    ab_len2 = hb3_v3_len2(ab);
    if (hb3_fx_abs(ab_len2) <= HB3_EPSILON) {
        return a;
    }
    t = hb3_fx_div(hb3_v3_dot(ap, ab), ab_len2);
    t = hb3_fx_clamp(t, 0, HB3_FX_ONE);
    return hb3_v3_add(a, hb3_v3_scale(ab, t));
}

static hb3_fx hb3_point_seg_dist2(hb3_v3 p, hb3_v3 a, hb3_v3 b)
{
    hb3_v3 c;
    c = hb3_closest_point_seg(p, a, b);
    return hb3_v3_len2(hb3_v3_sub(p, c));
}

static hb3_fx hb3_seg_seg_dist2(hb3_v3 p1, hb3_v3 q1, hb3_v3 p2, hb3_v3 q2)
{
    hb3_v3 d1;
    hb3_v3 d2;
    hb3_v3 r;
    hb3_v3 c1;
    hb3_v3 c2;
    hb3_fx a;
    hb3_fx e;
    hb3_fx f;
    hb3_fx c;
    hb3_fx b;
    hb3_fx denom;
    hb3_fx s;
    hb3_fx t;
    hb3_fx tmp;

    d1 = hb3_v3_sub(q1, p1);
    d2 = hb3_v3_sub(q2, p2);
    r = hb3_v3_sub(p1, p2);
    a = hb3_v3_dot(d1, d1);
    e = hb3_v3_dot(d2, d2);
    f = hb3_v3_dot(d2, r);
    s = 0;
    t = 0;

    if (a <= HB3_EPSILON && e <= HB3_EPSILON) {
        return hb3_v3_len2(hb3_v3_sub(p1, p2));
    }

    if (a <= HB3_EPSILON) {
        s = 0;
        t = hb3_fx_clamp(hb3_fx_div(f, e), 0, HB3_FX_ONE);
    } else {
        c = hb3_v3_dot(d1, r);
        if (e <= HB3_EPSILON) {
            t = 0;
            s = hb3_fx_clamp(hb3_fx_div(-c, a), 0, HB3_FX_ONE);
        } else {
            b = hb3_v3_dot(d1, d2);
            denom = hb3_fx_mul(a, e) - hb3_fx_mul(b, b);
            if (denom != 0) {
                tmp = hb3_fx_mul(b, f) - hb3_fx_mul(c, e);
                s = hb3_fx_clamp(hb3_fx_div(tmp, denom), 0, HB3_FX_ONE);
            } else {
                s = 0;
            }
            t = hb3_fx_div(hb3_fx_mul(b, s) + f, e);
            if (t < 0) {
                t = 0;
                s = hb3_fx_clamp(hb3_fx_div(-c, a), 0, HB3_FX_ONE);
            } else if (t > HB3_FX_ONE) {
                t = HB3_FX_ONE;
                s = hb3_fx_clamp(hb3_fx_div(b - c, a), 0, HB3_FX_ONE);
            }
        }
    }

    c1 = hb3_v3_add(p1, hb3_v3_scale(d1, s));
    c2 = hb3_v3_add(p2, hb3_v3_scale(d2, t));
    return hb3_v3_len2(hb3_v3_sub(c1, c2));
}

hb3_shape hb3_shape_make_sphere(int id, hb3_v3 center, hb3_fx radius,
                                int flags, int group_mask, int hit_mask)
{
    hb3_shape s;
    s.type = HB3_SHAPE_SPHERE;
    s.id = id;
    s.flags = flags;
    s.group_mask = group_mask;
    s.hit_mask = hit_mask;
    s.a = center;
    s.b = center;
    s.radius = radius;
    s.half = hb3_v3_make(0, 0, 0);
    s.basis = hb3_mat3_identity();
    return s;
}

hb3_shape hb3_shape_make_capsule(int id, hb3_v3 a, hb3_v3 b, hb3_fx radius,
                                 int flags, int group_mask, int hit_mask)
{
    hb3_shape s;
    s.type = HB3_SHAPE_CAPSULE;
    s.id = id;
    s.flags = flags;
    s.group_mask = group_mask;
    s.hit_mask = hit_mask;
    s.a = a;
    s.b = b;
    s.radius = radius;
    s.half = hb3_v3_make(0, 0, 0);
    s.basis = hb3_mat3_identity();
    return s;
}

hb3_shape hb3_shape_make_obb(int id, hb3_v3 center, hb3_v3 half_extents,
                             hb3_mat3 basis, int flags, int group_mask,
                             int hit_mask)
{
    hb3_shape s;
    s.type = HB3_SHAPE_OBB;
    s.id = id;
    s.flags = flags;
    s.group_mask = group_mask;
    s.hit_mask = hit_mask;
    s.a = center;
    s.b = center;
    s.radius = 0;
    s.half = half_extents;
    s.basis = basis;
    return s;
}

static hb3_v3 hb3_obb_closest_point(const hb3_shape *obb, hb3_v3 p)
{
    hb3_v3 d;
    hb3_v3 q;
    hb3_fx dist;
    d = hb3_v3_sub(p, obb->a);
    q = obb->a;
    dist = hb3_fx_clamp(hb3_v3_dot(d, obb->basis.x), -obb->half.x, obb->half.x);
    q = hb3_v3_add(q, hb3_v3_scale(obb->basis.x, dist));
    dist = hb3_fx_clamp(hb3_v3_dot(d, obb->basis.y), -obb->half.y, obb->half.y);
    q = hb3_v3_add(q, hb3_v3_scale(obb->basis.y, dist));
    dist = hb3_fx_clamp(hb3_v3_dot(d, obb->basis.z), -obb->half.z, obb->half.z);
    q = hb3_v3_add(q, hb3_v3_scale(obb->basis.z, dist));
    return q;
}

static int hb3_sphere_obb_intersects(const hb3_shape *sphere, const hb3_shape *obb)
{
    hb3_v3 closest;
    hb3_fx rr;
    closest = hb3_obb_closest_point(obb, sphere->a);
    rr = hb3_fx_mul(sphere->radius, sphere->radius);
    return hb3_v3_len2(hb3_v3_sub(sphere->a, closest)) <= rr;
}

static int hb3_seg_expanded_obb_intersects(hb3_v3 p0, hb3_v3 p1,
                                           const hb3_shape *obb,
                                           hb3_fx radius)
{
    hb3_v3 l0;
    hb3_v3 l1;
    hb3_v3 d;
    hb3_v3 mn;
    hb3_v3 mx;
    hb3_fx tmin;
    hb3_fx tmax;
    hb3_fx t1;
    hb3_fx t2;
    hb3_fx inv;
    hb3_fx tmp;
    int axis;

    l0 = hb3_mat3_tmul_v3(obb->basis, hb3_v3_sub(p0, obb->a));
    l1 = hb3_mat3_tmul_v3(obb->basis, hb3_v3_sub(p1, obb->a));
    d = hb3_v3_sub(l1, l0);
    mn = hb3_v3_make(-obb->half.x - radius, -obb->half.y - radius, -obb->half.z - radius);
    mx = hb3_v3_make(obb->half.x + radius, obb->half.y + radius, obb->half.z + radius);
    tmin = 0;
    tmax = HB3_FX_ONE;

    for (axis = 0; axis < 3; ++axis) {
        hb3_fx p;
        hb3_fx q;
        hb3_fx lo;
        hb3_fx hi;
        if (axis == 0) {
            p = l0.x;
            q = d.x;
            lo = mn.x;
            hi = mx.x;
        } else if (axis == 1) {
            p = l0.y;
            q = d.y;
            lo = mn.y;
            hi = mx.y;
        } else {
            p = l0.z;
            q = d.z;
            lo = mn.z;
            hi = mx.z;
        }
        if (hb3_fx_abs(q) <= HB3_EPSILON) {
            if (p < lo || p > hi) {
                return 0;
            }
        } else {
            inv = hb3_fx_div(HB3_FX_ONE, q);
            t1 = hb3_fx_mul(lo - p, inv);
            t2 = hb3_fx_mul(hi - p, inv);
            if (t1 > t2) {
                tmp = t1;
                t1 = t2;
                t2 = tmp;
            }
            if (t1 > tmin) {
                tmin = t1;
            }
            if (t2 < tmax) {
                tmax = t2;
            }
            if (tmin > tmax) {
                return 0;
            }
        }
    }
    return 1;
}

static int hb3_capsule_obb_intersects(const hb3_shape *capsule, const hb3_shape *obb)
{
    hb3_shape s0;
    hb3_shape s1;
    s0 = hb3_shape_make_sphere(0, capsule->a, capsule->radius, 0, 0, 0);
    s1 = hb3_shape_make_sphere(0, capsule->b, capsule->radius, 0, 0, 0);
    if (hb3_sphere_obb_intersects(&s0, obb)) {
        return 1;
    }
    if (hb3_sphere_obb_intersects(&s1, obb)) {
        return 1;
    }
    return hb3_seg_expanded_obb_intersects(capsule->a, capsule->b, obb, capsule->radius);
}

static hb3_fx hb3_obb_axis_radius(const hb3_shape *obb, hb3_v3 axis)
{
    hb3_fx r;
    r = hb3_fx_mul(obb->half.x, hb3_fx_abs(hb3_v3_dot(axis, obb->basis.x)));
    r += hb3_fx_mul(obb->half.y, hb3_fx_abs(hb3_v3_dot(axis, obb->basis.y)));
    r += hb3_fx_mul(obb->half.z, hb3_fx_abs(hb3_v3_dot(axis, obb->basis.z)));
    return r;
}

static int hb3_obb_axis_test(const hb3_shape *a, const hb3_shape *b, hb3_v3 axis)
{
    hb3_v3 t;
    hb3_fx len2;
    hb3_fx ra;
    hb3_fx rb;
    hb3_fx dist;
    len2 = hb3_v3_len2(axis);
    if (len2 <= HB3_EPSILON) {
        return 1;
    }
    t = hb3_v3_sub(b->a, a->a);
    ra = hb3_obb_axis_radius(a, axis);
    rb = hb3_obb_axis_radius(b, axis);
    dist = hb3_fx_abs(hb3_v3_dot(t, axis));
    return dist <= (ra + rb + HB3_EPSILON);
}

static int hb3_obb_obb_intersects(const hb3_shape *a, const hb3_shape *b)
{
    hb3_v3 aa[3];
    hb3_v3 bb[3];
    int i;
    int j;
    aa[0] = a->basis.x;
    aa[1] = a->basis.y;
    aa[2] = a->basis.z;
    bb[0] = b->basis.x;
    bb[1] = b->basis.y;
    bb[2] = b->basis.z;
    for (i = 0; i < 3; ++i) {
        if (!hb3_obb_axis_test(a, b, aa[i])) {
            return 0;
        }
        if (!hb3_obb_axis_test(a, b, bb[i])) {
            return 0;
        }
    }
    for (i = 0; i < 3; ++i) {
        for (j = 0; j < 3; ++j) {
            if (!hb3_obb_axis_test(a, b, hb3_v3_cross(aa[i], bb[j]))) {
                return 0;
            }
        }
    }
    return 1;
}

int hb3_shape_intersects(const hb3_shape *a, const hb3_shape *b)
{
    hb3_fx rr;
    hb3_fx dist2;
    if (a == 0 || b == 0) {
        return 0;
    }
    if (a->type == HB3_SHAPE_NONE || b->type == HB3_SHAPE_NONE) {
        return 0;
    }

    if (a->type == HB3_SHAPE_OBB || b->type == HB3_SHAPE_OBB) {
        if (a->type == HB3_SHAPE_SPHERE && b->type == HB3_SHAPE_OBB) {
            return hb3_sphere_obb_intersects(a, b);
        }
        if (a->type == HB3_SHAPE_OBB && b->type == HB3_SHAPE_SPHERE) {
            return hb3_sphere_obb_intersects(b, a);
        }
        if (a->type == HB3_SHAPE_CAPSULE && b->type == HB3_SHAPE_OBB) {
            return hb3_capsule_obb_intersects(a, b);
        }
        if (a->type == HB3_SHAPE_OBB && b->type == HB3_SHAPE_CAPSULE) {
            return hb3_capsule_obb_intersects(b, a);
        }
        if (a->type == HB3_SHAPE_OBB && b->type == HB3_SHAPE_OBB) {
            return hb3_obb_obb_intersects(a, b);
        }
        return 0;
    }

    rr = a->radius + b->radius;
    rr = hb3_fx_mul(rr, rr);

    if (a->type == HB3_SHAPE_SPHERE && b->type == HB3_SHAPE_SPHERE) {
        dist2 = hb3_v3_len2(hb3_v3_sub(a->a, b->a));
        return dist2 <= rr;
    }
    if (a->type == HB3_SHAPE_CAPSULE && b->type == HB3_SHAPE_SPHERE) {
        dist2 = hb3_point_seg_dist2(b->a, a->a, a->b);
        return dist2 <= rr;
    }
    if (a->type == HB3_SHAPE_SPHERE && b->type == HB3_SHAPE_CAPSULE) {
        dist2 = hb3_point_seg_dist2(a->a, b->a, b->b);
        return dist2 <= rr;
    }
    if (a->type == HB3_SHAPE_CAPSULE && b->type == HB3_SHAPE_CAPSULE) {
        dist2 = hb3_seg_seg_dist2(a->a, a->b, b->a, b->b);
        return dist2 <= rr;
    }
    return 0;
}

static void hb3_aabb_init_point(hb3_aabb *a, hb3_v3 p)
{
    a->min_v = p;
    a->max_v = p;
}

static void hb3_aabb_expand_point(hb3_aabb *a, hb3_v3 p)
{
    a->min_v.x = hb3_fx_min(a->min_v.x, p.x);
    a->min_v.y = hb3_fx_min(a->min_v.y, p.y);
    a->min_v.z = hb3_fx_min(a->min_v.z, p.z);
    a->max_v.x = hb3_fx_max(a->max_v.x, p.x);
    a->max_v.y = hb3_fx_max(a->max_v.y, p.y);
    a->max_v.z = hb3_fx_max(a->max_v.z, p.z);
}

static void hb3_aabb_expand_radius(hb3_aabb *a, hb3_fx r)
{
    a->min_v.x -= r;
    a->min_v.y -= r;
    a->min_v.z -= r;
    a->max_v.x += r;
    a->max_v.y += r;
    a->max_v.z += r;
}

int hb3_shape_get_aabb(const hb3_shape *s, hb3_aabb *out_aabb)
{
    hb3_fx ex;
    hb3_fx ey;
    hb3_fx ez;
    if (s == 0 || out_aabb == 0) {
        return HB3_ERR_BAD_ARG;
    }
    if (s->type == HB3_SHAPE_SPHERE) {
        hb3_aabb_init_point(out_aabb, s->a);
        hb3_aabb_expand_radius(out_aabb, s->radius);
        return HB3_OK;
    }
    if (s->type == HB3_SHAPE_CAPSULE) {
        hb3_aabb_init_point(out_aabb, s->a);
        hb3_aabb_expand_point(out_aabb, s->b);
        hb3_aabb_expand_radius(out_aabb, s->radius);
        return HB3_OK;
    }
    if (s->type == HB3_SHAPE_OBB) {
        ex = hb3_fx_mul(hb3_fx_abs(s->basis.x.x), s->half.x) +
             hb3_fx_mul(hb3_fx_abs(s->basis.y.x), s->half.y) +
             hb3_fx_mul(hb3_fx_abs(s->basis.z.x), s->half.z);
        ey = hb3_fx_mul(hb3_fx_abs(s->basis.x.y), s->half.x) +
             hb3_fx_mul(hb3_fx_abs(s->basis.y.y), s->half.y) +
             hb3_fx_mul(hb3_fx_abs(s->basis.z.y), s->half.z);
        ez = hb3_fx_mul(hb3_fx_abs(s->basis.x.z), s->half.x) +
             hb3_fx_mul(hb3_fx_abs(s->basis.y.z), s->half.y) +
             hb3_fx_mul(hb3_fx_abs(s->basis.z.z), s->half.z);
        out_aabb->min_v = hb3_v3_make(s->a.x - ex, s->a.y - ey, s->a.z - ez);
        out_aabb->max_v = hb3_v3_make(s->a.x + ex, s->a.y + ey, s->a.z + ez);
        return HB3_OK;
    }
    return HB3_ERR_BAD_ARG;
}

int hb3_aabb_intersects(const hb3_aabb *a, const hb3_aabb *b)
{
    if (a == 0 || b == 0) {
        return 0;
    }
    if (a->max_v.x < b->min_v.x || a->min_v.x > b->max_v.x) {
        return 0;
    }
    if (a->max_v.y < b->min_v.y || a->min_v.y > b->max_v.y) {
        return 0;
    }
    if (a->max_v.z < b->min_v.z || a->min_v.z > b->max_v.z) {
        return 0;
    }
    return 1;
}

void hb3_aabb_merge(hb3_aabb *dst, const hb3_aabb *src)
{
    if (dst == 0 || src == 0) {
        return;
    }
    hb3_aabb_expand_point(dst, src->min_v);
    hb3_aabb_expand_point(dst, src->max_v);
}

void hb3_world_init(hb3_world *w)
{
    int i;
    if (w == 0) {
        return;
    }
    for (i = 0; i < HB3_MAX_ACTORS; ++i) {
        w->actors[i].active = 0;
        w->actors[i].hurtbox_count = 0;
    }
    w->bp.mode = HB3_BP_DISABLED;
    w->bp.valid = 0;
    w->bp.overflow = 0;
    w->bp.ref_count = 0;
    w->bp.cell_count = 0;
    w->tick = 0;
    w->flags = 0;
}

int hb3_actor_find_index(const hb3_world *w, int actor_id)
{
    int i;
    if (w == 0) {
        return HB3_ERR_BAD_ARG;
    }
    for (i = 0; i < HB3_MAX_ACTORS; ++i) {
        if (w->actors[i].active && w->actors[i].id == actor_id) {
            return i;
        }
    }
    return HB3_ERR_NOT_FOUND;
}

int hb3_actor_add(hb3_world *w, int actor_id, int team)
{
    int i;
    int free_i;
    if (w == 0) {
        return HB3_ERR_BAD_ARG;
    }
    if (hb3_actor_find_index(w, actor_id) >= 0) {
        return HB3_OK;
    }
    free_i = -1;
    for (i = 0; i < HB3_MAX_ACTORS; ++i) {
        if (!w->actors[i].active) {
            free_i = i;
            break;
        }
    }
    if (free_i < 0) {
        return HB3_ERR_FULL;
    }
    w->actors[free_i].active = 1;
    w->actors[free_i].id = actor_id;
    w->actors[free_i].team = team;
    w->actors[free_i].faction = 0;
    w->actors[free_i].flags = HB3_FLAG_ENABLED;
    w->actors[free_i].group_mask = 1;
    w->actors[free_i].hit_mask = HB3_ALL_MASK;
    w->actors[free_i].user_kind = 0;
    w->actors[free_i].hurtbox_count = 0;
    w->bp.valid = 0;
    return HB3_OK;
}

int hb3_actor_remove(hb3_world *w, int actor_id)
{
    int i;
    i = hb3_actor_find_index(w, actor_id);
    if (i < 0) {
        return i;
    }
    w->actors[i].active = 0;
    w->actors[i].hurtbox_count = 0;
    w->bp.valid = 0;
    return HB3_OK;
}

int hb3_actor_set_masks(hb3_world *w, int actor_id, int group_mask, int hit_mask)
{
    int i;
    i = hb3_actor_find_index(w, actor_id);
    if (i < 0) {
        return i;
    }
    w->actors[i].group_mask = group_mask;
    w->actors[i].hit_mask = hit_mask;
    w->bp.valid = 0;
    return HB3_OK;
}

int hb3_actor_clear_hurtboxes(hb3_world *w, int actor_id)
{
    int i;
    int j;
    i = hb3_actor_find_index(w, actor_id);
    if (i < 0) {
        return i;
    }
    for (j = 0; j < HB3_MAX_HURTBOXES_PER_ACTOR; ++j) {
        w->actors[i].hurtboxes[j].active = 0;
    }
    w->actors[i].hurtbox_count = 0;
    w->bp.valid = 0;
    return HB3_OK;
}

static int hb3_actor_add_hurt_shape(hb3_world *w, int actor_id,
                                    const hb3_shape *shape, int material,
                                    int user_tag)
{
    int i;
    int slot;
    hb3_actor *a;
    if (shape == 0) {
        return HB3_ERR_BAD_ARG;
    }
    i = hb3_actor_find_index(w, actor_id);
    if (i < 0) {
        return i;
    }
    a = &w->actors[i];
    if (a->hurtbox_count >= HB3_MAX_HURTBOXES_PER_ACTOR) {
        return HB3_ERR_FULL;
    }
    slot = a->hurtbox_count;
    a->hurtboxes[slot].active = 1;
    a->hurtboxes[slot].owner_id = actor_id;
    a->hurtboxes[slot].shape = *shape;
    a->hurtboxes[slot].shape.group_mask = a->group_mask;
    a->hurtboxes[slot].shape.hit_mask = a->hit_mask;
    a->hurtboxes[slot].material = material;
    a->hurtboxes[slot].damage_mul_fx = HB3_FX_ONE;
    a->hurtboxes[slot].user_tag = user_tag;
    a->hurtbox_count += 1;
    w->bp.valid = 0;
    return HB3_OK;
}

int hb3_actor_add_hurt_sphere(hb3_world *w, int actor_id, int hurt_id,
                              hb3_v3 center, hb3_fx radius,
                              int hurt_flags, int user_tag)
{
    hb3_shape s;
    s = hb3_shape_make_sphere(hurt_id, center, radius, hurt_flags, 0, 0);
    return hb3_actor_add_hurt_shape(w, actor_id, &s, hurt_flags, user_tag);
}

int hb3_actor_add_hurt_capsule(hb3_world *w, int actor_id, int hurt_id,
                               hb3_v3 a, hb3_v3 b, hb3_fx radius,
                               int hurt_flags, int user_tag)
{
    hb3_shape s;
    s = hb3_shape_make_capsule(hurt_id, a, b, radius, hurt_flags, 0, 0);
    return hb3_actor_add_hurt_shape(w, actor_id, &s, hurt_flags, user_tag);
}

int hb3_actor_add_hurt_obb(hb3_world *w, int actor_id, int hurt_id,
                           hb3_v3 center, hb3_v3 half_extents,
                           hb3_mat3 basis, int hurt_flags, int user_tag)
{
    hb3_shape s;
    s = hb3_shape_make_obb(hurt_id, center, half_extents, basis, hurt_flags, 0, 0);
    return hb3_actor_add_hurt_shape(w, actor_id, &s, hurt_flags, user_tag);
}

int hb3_actor_get_aabb(const hb3_actor *actor, hb3_aabb *out_aabb)
{
    int i;
    int got;
    hb3_aabb box;
    if (actor == 0 || out_aabb == 0 || !actor->active) {
        return HB3_ERR_BAD_ARG;
    }
    got = 0;
    for (i = 0; i < actor->hurtbox_count; ++i) {
        if (actor->hurtboxes[i].active) {
            if (hb3_shape_get_aabb(&actor->hurtboxes[i].shape, &box) == HB3_OK) {
                if (!got) {
                    *out_aabb = box;
                    got = 1;
                } else {
                    hb3_aabb_merge(out_aabb, &box);
                }
            }
        }
    }
    if (!got) {
        return HB3_ERR_INACTIVE;
    }
    return HB3_OK;
}

int hb3_world_set_broadphase_grid(hb3_world *w, hb3_v3 origin,
                                  hb3_fx cell_size,
                                  int cells_x, int cells_y, int cells_z)
{
    int count;
    if (w == 0 || cell_size <= 0 || cells_x <= 0 || cells_y <= 0 || cells_z <= 0) {
        return HB3_ERR_BAD_ARG;
    }
    count = cells_x * cells_y * cells_z;
    if (count > HB3_MAX_GRID_CELLS) {
        return HB3_ERR_FULL;
    }
    w->bp.mode = HB3_BP_GRID;
    w->bp.valid = 0;
    w->bp.overflow = 0;
    w->bp.origin = origin;
    w->bp.cell_size = cell_size;
    w->bp.cells_x = cells_x;
    w->bp.cells_y = cells_y;
    w->bp.cells_z = cells_z;
    w->bp.cell_count = count;
    return HB3_OK;
}

void hb3_world_disable_broadphase(hb3_world *w)
{
    if (w == 0) {
        return;
    }
    w->bp.mode = HB3_BP_DISABLED;
    w->bp.valid = 0;
    w->bp.ref_count = 0;
    w->bp.overflow = 0;
}

static int hb3_bp_cell_index(const hb3_broadphase *bp, int x, int y, int z)
{
    return x + y * bp->cells_x + z * bp->cells_x * bp->cells_y;
}

static int hb3_bp_coord(const hb3_broadphase *bp, hb3_fx value, hb3_fx origin)
{
    hb3_fx rel;
    int c;
    rel = value - origin;
    c = hb3_fx_to_int(hb3_fx_div(rel, bp->cell_size));
    return c;
}

static void hb3_bp_clamp_coord(const hb3_broadphase *bp, int *x, int *y, int *z)
{
    if (*x < 0) { *x = 0; }
    if (*y < 0) { *y = 0; }
    if (*z < 0) { *z = 0; }
    if (*x >= bp->cells_x) { *x = bp->cells_x - 1; }
    if (*y >= bp->cells_y) { *y = bp->cells_y - 1; }
    if (*z >= bp->cells_z) { *z = bp->cells_z - 1; }
}

static void hb3_bp_insert_actor(hb3_world *w, int actor_index, const hb3_aabb *box)
{
    int min_x;
    int min_y;
    int min_z;
    int max_x;
    int max_y;
    int max_z;
    int x;
    int y;
    int z;
    int ci;
    int ri;
    hb3_broadphase *bp;
    bp = &w->bp;
    min_x = hb3_bp_coord(bp, box->min_v.x, bp->origin.x);
    min_y = hb3_bp_coord(bp, box->min_v.y, bp->origin.y);
    min_z = hb3_bp_coord(bp, box->min_v.z, bp->origin.z);
    max_x = hb3_bp_coord(bp, box->max_v.x, bp->origin.x);
    max_y = hb3_bp_coord(bp, box->max_v.y, bp->origin.y);
    max_z = hb3_bp_coord(bp, box->max_v.z, bp->origin.z);
    hb3_bp_clamp_coord(bp, &min_x, &min_y, &min_z);
    hb3_bp_clamp_coord(bp, &max_x, &max_y, &max_z);

    for (z = min_z; z <= max_z; ++z) {
        for (y = min_y; y <= max_y; ++y) {
            for (x = min_x; x <= max_x; ++x) {
                if (bp->ref_count >= HB3_MAX_GRID_REFS) {
                    bp->overflow = 1;
                    return;
                }
                ci = hb3_bp_cell_index(bp, x, y, z);
                ri = bp->ref_count;
                bp->ref_actor_index[ri] = actor_index;
                bp->ref_next[ri] = bp->cell_heads[ci];
                bp->cell_heads[ci] = ri;
                bp->ref_count += 1;
            }
        }
    }
}

int hb3_world_rebuild_broadphase(hb3_world *w)
{
    int i;
    hb3_aabb box;
    if (w == 0) {
        return HB3_ERR_BAD_ARG;
    }
    if (w->bp.mode != HB3_BP_GRID) {
        return HB3_OK;
    }
    for (i = 0; i < HB3_MAX_GRID_CELLS; ++i) {
        w->bp.cell_heads[i] = -1;
    }
    w->bp.ref_count = 0;
    w->bp.overflow = 0;
    for (i = 0; i < HB3_MAX_ACTORS; ++i) {
        if (w->actors[i].active && hb3_actor_get_aabb(&w->actors[i], &box) == HB3_OK) {
            hb3_bp_insert_actor(w, i, &box);
        }
    }
    w->bp.valid = 1;
    return w->bp.overflow ? HB3_ERR_FULL : HB3_OK;
}

int hb3_world_broadphase_ref_count(const hb3_world *w)
{
    if (w == 0) {
        return 0;
    }
    return w->bp.ref_count;
}

int hb3_world_broadphase_overflowed(const hb3_world *w)
{
    if (w == 0) {
        return 0;
    }
    return w->bp.overflow;
}

static int hb3_indices_has(const int *arr, int n, int v)
{
    int i;
    for (i = 0; i < n; ++i) {
        if (arr[i] == v) {
            return 1;
        }
    }
    return 0;
}

int hb3_world_query_actor_indices(const hb3_world *w, const hb3_aabb *query,
                                  int *out_indices, int max_indices)
{
    int n;
    int i;
    int min_x;
    int min_y;
    int min_z;
    int max_x;
    int max_y;
    int max_z;
    int x;
    int y;
    int z;
    int ci;
    int ri;
    const hb3_broadphase *bp;
    if (w == 0 || query == 0 || out_indices == 0 || max_indices <= 0) {
        return 0;
    }
    n = 0;
    if (w->bp.mode != HB3_BP_GRID || !w->bp.valid || w->bp.overflow) {
        hb3_aabb actor_box;
        for (i = 0; i < HB3_MAX_ACTORS && n < max_indices; ++i) {
            if (w->actors[i].active && hb3_actor_get_aabb(&w->actors[i], &actor_box) == HB3_OK) {
                if (hb3_aabb_intersects(query, &actor_box)) {
                    out_indices[n++] = i;
                }
            }
        }
        return n;
    }

    bp = &w->bp;
    min_x = hb3_bp_coord(bp, query->min_v.x, bp->origin.x);
    min_y = hb3_bp_coord(bp, query->min_v.y, bp->origin.y);
    min_z = hb3_bp_coord(bp, query->min_v.z, bp->origin.z);
    max_x = hb3_bp_coord(bp, query->max_v.x, bp->origin.x);
    max_y = hb3_bp_coord(bp, query->max_v.y, bp->origin.y);
    max_z = hb3_bp_coord(bp, query->max_v.z, bp->origin.z);
    hb3_bp_clamp_coord(bp, &min_x, &min_y, &min_z);
    hb3_bp_clamp_coord(bp, &max_x, &max_y, &max_z);

    for (z = min_z; z <= max_z; ++z) {
        for (y = min_y; y <= max_y; ++y) {
            for (x = min_x; x <= max_x; ++x) {
                ci = hb3_bp_cell_index(bp, x, y, z);
                ri = bp->cell_heads[ci];
                while (ri >= 0) {
                    i = bp->ref_actor_index[ri];
                    if (i >= 0 && i < HB3_MAX_ACTORS && !hb3_indices_has(out_indices, n, i)) {
                        if (n < max_indices) {
                            out_indices[n++] = i;
                        } else {
                            return n;
                        }
                    }
                    ri = bp->ref_next[ri];
                }
            }
        }
    }
    return n;
}

static hb3_v3 hb3_obb_corner(const hb3_shape *s, int sx, int sy, int sz)
{
    hb3_v3 p;
    p = s->a;
    p = hb3_v3_add(p, hb3_v3_scale(s->basis.x, sx ? s->half.x : -s->half.x));
    p = hb3_v3_add(p, hb3_v3_scale(s->basis.y, sy ? s->half.y : -s->half.y));
    p = hb3_v3_add(p, hb3_v3_scale(s->basis.z, sz ? s->half.z : -s->half.z));
    return p;
}

void hb3_debug_draw_shape(const hb3_shape *s, hb3_debug_line_fn line_fn,
                          void *user, int color, int tag)
{
    hb3_v3 p[8];
    hb3_v3 x;
    hb3_v3 y;
    hb3_v3 z;
    if (s == 0 || line_fn == 0) {
        return;
    }
    if (s->type == HB3_SHAPE_SPHERE) {
        x = hb3_v3_make(s->radius, 0, 0);
        y = hb3_v3_make(0, s->radius, 0);
        z = hb3_v3_make(0, 0, s->radius);
        line_fn(user, hb3_v3_sub(s->a, x), hb3_v3_add(s->a, x), color, tag);
        line_fn(user, hb3_v3_sub(s->a, y), hb3_v3_add(s->a, y), color, tag);
        line_fn(user, hb3_v3_sub(s->a, z), hb3_v3_add(s->a, z), color, tag);
    } else if (s->type == HB3_SHAPE_CAPSULE) {
        line_fn(user, s->a, s->b, color, tag);
        x = hb3_v3_make(s->radius, 0, 0);
        y = hb3_v3_make(0, s->radius, 0);
        z = hb3_v3_make(0, 0, s->radius);
        line_fn(user, hb3_v3_sub(s->a, x), hb3_v3_add(s->a, x), color, tag);
        line_fn(user, hb3_v3_sub(s->b, x), hb3_v3_add(s->b, x), color, tag);
        line_fn(user, hb3_v3_sub(s->a, y), hb3_v3_add(s->a, y), color, tag);
        line_fn(user, hb3_v3_sub(s->b, y), hb3_v3_add(s->b, y), color, tag);
        line_fn(user, hb3_v3_sub(s->a, z), hb3_v3_add(s->a, z), color, tag);
        line_fn(user, hb3_v3_sub(s->b, z), hb3_v3_add(s->b, z), color, tag);
    } else if (s->type == HB3_SHAPE_OBB) {
        p[0] = hb3_obb_corner(s, 0, 0, 0);
        p[1] = hb3_obb_corner(s, 1, 0, 0);
        p[2] = hb3_obb_corner(s, 1, 1, 0);
        p[3] = hb3_obb_corner(s, 0, 1, 0);
        p[4] = hb3_obb_corner(s, 0, 0, 1);
        p[5] = hb3_obb_corner(s, 1, 0, 1);
        p[6] = hb3_obb_corner(s, 1, 1, 1);
        p[7] = hb3_obb_corner(s, 0, 1, 1);
        line_fn(user, p[0], p[1], color, tag); line_fn(user, p[1], p[2], color, tag);
        line_fn(user, p[2], p[3], color, tag); line_fn(user, p[3], p[0], color, tag);
        line_fn(user, p[4], p[5], color, tag); line_fn(user, p[5], p[6], color, tag);
        line_fn(user, p[6], p[7], color, tag); line_fn(user, p[7], p[4], color, tag);
        line_fn(user, p[0], p[4], color, tag); line_fn(user, p[1], p[5], color, tag);
        line_fn(user, p[2], p[6], color, tag); line_fn(user, p[3], p[7], color, tag);
    }
}

static void hb3_debug_line_aabb(hb3_debug_line_fn fn, void *user, const hb3_aabb *b, int color, int tag)
{
    hb3_v3 p[8];
    p[0] = hb3_v3_make(b->min_v.x, b->min_v.y, b->min_v.z);
    p[1] = hb3_v3_make(b->max_v.x, b->min_v.y, b->min_v.z);
    p[2] = hb3_v3_make(b->max_v.x, b->max_v.y, b->min_v.z);
    p[3] = hb3_v3_make(b->min_v.x, b->max_v.y, b->min_v.z);
    p[4] = hb3_v3_make(b->min_v.x, b->min_v.y, b->max_v.z);
    p[5] = hb3_v3_make(b->max_v.x, b->min_v.y, b->max_v.z);
    p[6] = hb3_v3_make(b->max_v.x, b->max_v.y, b->max_v.z);
    p[7] = hb3_v3_make(b->min_v.x, b->max_v.y, b->max_v.z);
    fn(user, p[0], p[1], color, tag); fn(user, p[1], p[2], color, tag);
    fn(user, p[2], p[3], color, tag); fn(user, p[3], p[0], color, tag);
    fn(user, p[4], p[5], color, tag); fn(user, p[5], p[6], color, tag);
    fn(user, p[6], p[7], color, tag); fn(user, p[7], p[4], color, tag);
    fn(user, p[0], p[4], color, tag); fn(user, p[1], p[5], color, tag);
    fn(user, p[2], p[6], color, tag); fn(user, p[3], p[7], color, tag);
}

void hb3_debug_draw_world_lines(const hb3_world *w, hb3_debug_line_fn line_fn,
                                void *user, int flags)
{
    int i;
    int j;
    int x;
    int y;
    int z;
    hb3_aabb cell;
    if (w == 0 || line_fn == 0) {
        return;
    }
    if ((flags & HB3_DEBUG_HURTBOXES) != 0) {
        for (i = 0; i < HB3_MAX_ACTORS; ++i) {
            if (!w->actors[i].active) {
                continue;
            }
            for (j = 0; j < w->actors[i].hurtbox_count; ++j) {
                if (w->actors[i].hurtboxes[j].active) {
                    hb3_debug_draw_shape(&w->actors[i].hurtboxes[j].shape, line_fn,
                                         user, HB3_DEBUG_COLOR_HURT,
                                         w->actors[i].id);
                }
            }
        }
    }
    if ((flags & HB3_DEBUG_GRID) != 0 && w->bp.mode == HB3_BP_GRID) {
        for (z = 0; z < w->bp.cells_z; ++z) {
            for (y = 0; y < w->bp.cells_y; ++y) {
                for (x = 0; x < w->bp.cells_x; ++x) {
                    cell.min_v = hb3_v3_make(w->bp.origin.x + hb3_fx_mul(hb3_fx_from_int(x), w->bp.cell_size),
                                             w->bp.origin.y + hb3_fx_mul(hb3_fx_from_int(y), w->bp.cell_size),
                                             w->bp.origin.z + hb3_fx_mul(hb3_fx_from_int(z), w->bp.cell_size));
                    cell.max_v = hb3_v3_add(cell.min_v, hb3_v3_make(w->bp.cell_size, w->bp.cell_size, w->bp.cell_size));
                    hb3_debug_line_aabb(line_fn, user, &cell, HB3_DEBUG_COLOR_GRID, 0);
                }
            }
        }
    }
}

int hb3_debug_count_actors(const hb3_world *w)
{
    int i;
    int n;
    if (w == 0) {
        return 0;
    }
    n = 0;
    for (i = 0; i < HB3_MAX_ACTORS; ++i) {
        if (w->actors[i].active) {
            n += 1;
        }
    }
    return n;
}
