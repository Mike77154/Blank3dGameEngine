#include "weakspots3d.h"

w3d_v3 pdc3d_w3d_v3_make(w3d_fx x, w3d_fx y, w3d_fx z) { w3d_v3 r; r.x = x; r.y = y; r.z = z; return r; }
w3d_v3 pdc3d_w3d_v3_add(w3d_v3 a, w3d_v3 b) { return w3d_v3_make(a.x + b.x, a.y + b.y, a.z + b.z); }
w3d_v3 pdc3d_w3d_v3_sub(w3d_v3 a, w3d_v3 b) { return w3d_v3_make(a.x - b.x, a.y - b.y, a.z - b.z); }
w3d_fx pdc3d_w3d_v3_dot(w3d_v3 a, w3d_v3 b) { return W3D_FX_MUL(a.x, b.x) + W3D_FX_MUL(a.y, b.y) + W3D_FX_MUL(a.z, b.z); }

w3d_mat3 pdc3d_w3d_mat3_identity(void)
{
    w3d_mat3 m;
    m.m00 = W3D_FX_ONE; m.m01 = 0; m.m02 = 0;
    m.m10 = 0; m.m11 = W3D_FX_ONE; m.m12 = 0;
    m.m20 = 0; m.m21 = 0; m.m22 = W3D_FX_ONE;
    return m;
}

w3d_mat3 pdc3d_w3d_mat3_rot_y_90(void)
{
    w3d_mat3 m;
    m.m00 = 0; m.m01 = 0; m.m02 = W3D_FX_ONE;
    m.m10 = 0; m.m11 = W3D_FX_ONE; m.m12 = 0;
    m.m20 = -W3D_FX_ONE; m.m21 = 0; m.m22 = 0;
    return m;
}

static w3d_fx w3d_abs_fx(w3d_fx v) { return v < 0 ? -v : v; }

static w3d_v3 w3d_mat3_mul_v3_transpose(w3d_mat3 m, w3d_v3 v)
{
    w3d_v3 r;
    r.x = W3D_FX_MUL(m.m00, v.x) + W3D_FX_MUL(m.m10, v.y) + W3D_FX_MUL(m.m20, v.z);
    r.y = W3D_FX_MUL(m.m01, v.x) + W3D_FX_MUL(m.m11, v.y) + W3D_FX_MUL(m.m21, v.z);
    r.z = W3D_FX_MUL(m.m02, v.x) + W3D_FX_MUL(m.m12, v.y) + W3D_FX_MUL(m.m22, v.z);
    return r;
}

static w3d_fx w3d_fx_clamp(w3d_fx v, w3d_fx lo, w3d_fx hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static w3d_fx w3d_v3_len2(w3d_v3 v)
{
    return w3d_v3_dot(v, v);
}

static w3d_fx w3d_dist2_point_segment(w3d_v3 p, w3d_v3 a, w3d_v3 b)
{
    w3d_v3 ab;
    w3d_v3 ap;
    w3d_v3 q;
    w3d_fx ab2;
    w3d_fx t;
    ab = w3d_v3_sub(b, a);
    ap = w3d_v3_sub(p, a);
    ab2 = w3d_v3_len2(ab);
    if (ab2 <= 0) return w3d_v3_len2(w3d_v3_sub(p, a));
    t = W3D_FX_DIV(w3d_v3_dot(ap, ab), ab2);
    t = w3d_fx_clamp(t, 0, W3D_FX_ONE);
    q.x = a.x + W3D_FX_MUL(ab.x, t);
    q.y = a.y + W3D_FX_MUL(ab.y, t);
    q.z = a.z + W3D_FX_MUL(ab.z, t);
    return w3d_v3_len2(w3d_v3_sub(p, q));
}

static w3d_actor *w3d_find_actor(w3d_world *w, int actor_id)
{
    int i;
    if (w == 0) return 0;
    for (i = 0; i < W3D_MAX_ACTORS; ++i) {
        if (w->actors[i].active && w->actors[i].id == actor_id) return &w->actors[i];
    }
    return 0;
}

static w3d_weakspot *w3d_find_weakspot(w3d_actor *a, int weakspot_id)
{
    int i;
    if (a == 0) return 0;
    for (i = 0; i < a->weakspot_count; ++i) {
        if (a->weakspots[i].active && a->weakspots[i].id == weakspot_id) return &a->weakspots[i];
    }
    return 0;
}

static void w3d_push_event(w3d_world *w, const w3d_event *ev)
{
    int next;
    if (w == 0 || ev == 0) return;
    next = (w->event_tail + 1) % W3D_MAX_EVENTS;
    if (next == w->event_head) w->event_head = (w->event_head + 1) % W3D_MAX_EVENTS;
    w->events[w->event_tail] = *ev;
    w->event_tail = next;
}

static void w3d_emit(w3d_world *w, int type, int actor_id, int weakspot_id, int base_damage, int final_damage, int value)
{
    w3d_event ev;
    ev.type = type;
    ev.actor_id = actor_id;
    ev.weakspot_id = weakspot_id;
    ev.base_damage = base_damage;
    ev.final_damage = final_damage;
    ev.value = value;
    w3d_push_event(w, &ev);
}

static int w3d_shape_contains_point(const w3d_shape *s, w3d_v3 local_point)
{
    w3d_fx d2;
    w3d_fx r2;
    w3d_v3 lp;
    if (s->type == W3D_SHAPE_SPHERE) {
        d2 = w3d_v3_len2(w3d_v3_sub(local_point, s->a));
        r2 = W3D_FX_MUL(s->radius, s->radius);
        return d2 <= r2;
    }
    if (s->type == W3D_SHAPE_CAPSULE) {
        d2 = w3d_dist2_point_segment(local_point, s->a, s->b);
        r2 = W3D_FX_MUL(s->radius, s->radius);
        return d2 <= r2;
    }
    if (s->type == W3D_SHAPE_OBB) {
        lp = w3d_mat3_mul_v3_transpose(s->axis, w3d_v3_sub(local_point, s->a));
        if (w3d_abs_fx(lp.x) > s->half.x) return 0;
        if (w3d_abs_fx(lp.y) > s->half.y) return 0;
        if (w3d_abs_fx(lp.z) > s->half.z) return 0;
        return 1;
    }
    return 0;
}

void pdc3d_w3d_world_init(w3d_world *w)
{
    int i;
    if (w == 0) return;
    for (i = 0; i < W3D_MAX_ACTORS; ++i) w->actors[i].active = 0;
    w->event_head = 0;
    w->event_tail = 0;
}

int pdc3d_w3d_add_actor(w3d_world *w, int actor_id, w3d_v3 pos)
{
    int i;
    if (w == 0) return W3D_ERR_BAD_ARG;
    if (w3d_find_actor(w, actor_id) != 0) return W3D_ERR_BAD_ARG;
    for (i = 0; i < W3D_MAX_ACTORS; ++i) {
        if (!w->actors[i].active) {
            w->actors[i].active = 1;
            w->actors[i].id = actor_id;
            w->actors[i].pos = pos;
            w->actors[i].weakspot_count = 0;
            return W3D_OK;
        }
    }
    return W3D_ERR_FULL;
}

int pdc3d_w3d_set_actor_pos(w3d_world *w, int actor_id, w3d_v3 pos)
{
    w3d_actor *a;
    a = w3d_find_actor(w, actor_id);
    if (a == 0) return W3D_ERR_NOT_FOUND;
    a->pos = pos;
    return W3D_OK;
}

static int w3d_add_base(w3d_world *w, int actor_id, int weakspot_id, const w3d_shape *shape, int mult_num, int mult_den, int durability, int flags)
{
    w3d_actor *a;
    w3d_weakspot *s;
    if (w == 0 || shape == 0 || mult_den == 0) return W3D_ERR_BAD_ARG;
    a = w3d_find_actor(w, actor_id);
    if (a == 0) return W3D_ERR_NOT_FOUND;
    if (a->weakspot_count >= W3D_MAX_WEAKSPOTS_PER_ACTOR) return W3D_ERR_FULL;
    s = &a->weakspots[a->weakspot_count];
    s->active = 1;
    s->id = weakspot_id;
    s->shape = *shape;
    s->flags = flags;
    s->required_attack_flags = 0;
    s->blocked_attack_flags = 0;
    s->multiplier_num = mult_num;
    s->multiplier_den = mult_den;
    s->durability = durability;
    s->hits_taken = 0;
    a->weakspot_count += 1;
    return W3D_OK;
}

int pdc3d_w3d_add_weakspot_sphere(w3d_world *w, int actor_id, int weakspot_id, w3d_v3 center, w3d_fx radius, int mult_num, int mult_den, int durability, int flags)
{
    w3d_shape s;
    if (radius < 0) return W3D_ERR_BAD_ARG;
    s.type = W3D_SHAPE_SPHERE;
    s.a = center;
    s.b = center;
    s.half = w3d_v3_make(0,0,0);
    s.axis = w3d_mat3_identity();
    s.radius = radius;
    return w3d_add_base(w, actor_id, weakspot_id, &s, mult_num, mult_den, durability, flags);
}

int pdc3d_w3d_add_weakspot_capsule(w3d_world *w, int actor_id, int weakspot_id, w3d_v3 a, w3d_v3 b, w3d_fx radius, int mult_num, int mult_den, int durability, int flags)
{
    w3d_shape s;
    if (radius < 0) return W3D_ERR_BAD_ARG;
    s.type = W3D_SHAPE_CAPSULE;
    s.a = a;
    s.b = b;
    s.half = w3d_v3_make(0,0,0);
    s.axis = w3d_mat3_identity();
    s.radius = radius;
    return w3d_add_base(w, actor_id, weakspot_id, &s, mult_num, mult_den, durability, flags);
}

int pdc3d_w3d_add_weakspot_obb(w3d_world *w, int actor_id, int weakspot_id, w3d_v3 center, w3d_v3 half, w3d_mat3 axis, int mult_num, int mult_den, int durability, int flags)
{
    w3d_shape s;
    s.type = W3D_SHAPE_OBB;
    s.a = center;
    s.b = center;
    s.half = half;
    s.axis = axis;
    s.radius = 0;
    return w3d_add_base(w, actor_id, weakspot_id, &s, mult_num, mult_den, durability, flags);
}

int pdc3d_w3d_set_weakspot_attack_filter(w3d_world *w, int actor_id, int weakspot_id, int required_flags, int blocked_flags)
{
    w3d_actor *a;
    w3d_weakspot *s;
    a = w3d_find_actor(w, actor_id);
    if (a == 0) return W3D_ERR_NOT_FOUND;
    s = w3d_find_weakspot(a, weakspot_id);
    if (s == 0) return W3D_ERR_NOT_FOUND;
    s->required_attack_flags = required_flags;
    s->blocked_attack_flags = blocked_flags;
    return W3D_OK;
}

int pdc3d_w3d_resolve_hit_point(w3d_world *w, int actor_id, w3d_v3 world_point, int base_damage, int attack_flags, w3d_hit_result *out_result)
{
    w3d_actor *a;
    w3d_weakspot *best;
    w3d_v3 local_point;
    int i;
    int final_damage;
    if (w == 0 || out_result == 0) return W3D_ERR_BAD_ARG;
    out_result->matched = 0;
    out_result->actor_id = actor_id;
    out_result->weakspot_id = 0;
    out_result->final_damage = base_damage;
    out_result->multiplier_num = 1;
    out_result->multiplier_den = 1;
    out_result->flags = 0;
    a = w3d_find_actor(w, actor_id);
    if (a == 0) return W3D_ERR_NOT_FOUND;
    local_point = w3d_v3_sub(world_point, a->pos);
    best = 0;
    for (i = 0; i < a->weakspot_count; ++i) {
        if (!a->weakspots[i].active) continue;
        if (a->weakspots[i].flags & W3D_WEAKSPOT_DISABLED) continue;
        if (a->weakspots[i].required_attack_flags != 0 && ((attack_flags & a->weakspots[i].required_attack_flags) != a->weakspots[i].required_attack_flags)) continue;
        if ((attack_flags & a->weakspots[i].blocked_attack_flags) != 0) continue;
        if (w3d_shape_contains_point(&a->weakspots[i].shape, local_point)) {
            best = &a->weakspots[i];
            break;
        }
    }
    if (best == 0) {
        w3d_emit(w, W3D_EVENT_WEAKSPOT_IGNORED, actor_id, 0, base_damage, base_damage, 0);
        return W3D_NO;
    }
    final_damage = (int)(((long)base_damage * (long)best->multiplier_num) / (long)best->multiplier_den);
    if (final_damage < 0) final_damage = 0;
    best->hits_taken += 1;
    if ((best->flags & W3D_WEAKSPOT_BREAKABLE) && best->durability > 0) {
        best->durability -= final_damage;
        if (best->durability <= 0) {
            best->flags |= W3D_WEAKSPOT_DISABLED;
            w3d_emit(w, W3D_EVENT_WEAKSPOT_BROKEN, actor_id, best->id, base_damage, final_damage, best->hits_taken);
        }
    }
    if (best->flags & W3D_WEAKSPOT_ONCE) best->flags |= W3D_WEAKSPOT_DISABLED;
    out_result->matched = 1;
    out_result->weakspot_id = best->id;
    out_result->final_damage = final_damage;
    out_result->multiplier_num = best->multiplier_num;
    out_result->multiplier_den = best->multiplier_den;
    out_result->flags = best->flags;
    w3d_emit(w, W3D_EVENT_WEAKSPOT_HIT, actor_id, best->id, base_damage, final_damage, best->hits_taken);
    return W3D_OK;
}

int pdc3d_w3d_poll_event(w3d_world *w, w3d_event *out_event)
{
    if (w == 0 || out_event == 0) return W3D_ERR_BAD_ARG;
    if (w->event_head == w->event_tail) return W3D_NO;
    *out_event = w->events[w->event_head];
    w->event_head = (w->event_head + 1) % W3D_MAX_EVENTS;
    return W3D_OK;
}
