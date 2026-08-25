#include "grab3d.h"

#define G3D_PHASE_STARTUP 0
#define G3D_PHASE_ACTIVE  1
#define G3D_PHASE_HOLD    2
#define G3D_PHASE_RECOVER 3

static int g3d_abs_i(int v) { return v < 0 ? -v : v; }

g3d_v3 g3d_v3_make(g3d_fx x, g3d_fx y, g3d_fx z) { g3d_v3 r; r.x = x; r.y = y; r.z = z; return r; }
g3d_v3 g3d_v3_add(g3d_v3 a, g3d_v3 b) { return g3d_v3_make(a.x + b.x, a.y + b.y, a.z + b.z); }
g3d_v3 g3d_v3_sub(g3d_v3 a, g3d_v3 b) { return g3d_v3_make(a.x - b.x, a.y - b.y, a.z - b.z); }
g3d_fx g3d_v3_dot(g3d_v3 a, g3d_v3 b) { return G3D_FX_MUL(a.x, b.x) + G3D_FX_MUL(a.y, b.y) + G3D_FX_MUL(a.z, b.z); }
g3d_fx g3d_v3_len2(g3d_v3 a) { return g3d_v3_dot(a, a); }

static g3d_fx g3d_fx_clamp(g3d_fx v, g3d_fx lo, g3d_fx hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static g3d_fx g3d_dist2_point_segment(g3d_v3 p, g3d_v3 a, g3d_v3 b)
{
    g3d_v3 ab;
    g3d_v3 ap;
    g3d_v3 q;
    g3d_fx ab2;
    g3d_fx t;
    ab = g3d_v3_sub(b, a);
    ap = g3d_v3_sub(p, a);
    ab2 = g3d_v3_len2(ab);
    if (ab2 <= 0) return g3d_v3_len2(g3d_v3_sub(p, a));
    t = G3D_FX_DIV(g3d_v3_dot(ap, ab), ab2);
    t = g3d_fx_clamp(t, 0, G3D_FX_ONE);
    q.x = a.x + G3D_FX_MUL(ab.x, t);
    q.y = a.y + G3D_FX_MUL(ab.y, t);
    q.z = a.z + G3D_FX_MUL(ab.z, t);
    return g3d_v3_len2(g3d_v3_sub(p, q));
}

static g3d_fx g3d_dist2_segment_segment_approx(g3d_v3 a0, g3d_v3 a1, g3d_v3 b0, g3d_v3 b1)
{
    g3d_fx d0;
    g3d_fx d1;
    g3d_fx d2;
    g3d_fx d3;
    g3d_fx m;
    d0 = g3d_dist2_point_segment(a0, b0, b1);
    d1 = g3d_dist2_point_segment(a1, b0, b1);
    d2 = g3d_dist2_point_segment(b0, a0, a1);
    d3 = g3d_dist2_point_segment(b1, a0, a1);
    m = d0;
    if (d1 < m) m = d1;
    if (d2 < m) m = d2;
    if (d3 < m) m = d3;
    return m;
}

static int g3d_shape_world(const g3d_shape *s, g3d_v3 base, g3d_shape *out_s)
{
    if (s == 0 || out_s == 0) return G3D_ERR_BAD_ARG;
    *out_s = *s;
    out_s->a = g3d_v3_add(s->a, base);
    out_s->b = g3d_v3_add(s->b, base);
    return G3D_OK;
}

static int g3d_shapes_overlap(const g3d_shape *a, const g3d_shape *b)
{
    g3d_fx r;
    g3d_fx r2;
    g3d_fx d2;
    if (a == 0 || b == 0) return 0;
    r = a->radius + b->radius;
    r2 = G3D_FX_MUL(r, r);
    if (a->type == G3D_SHAPE_SPHERE && b->type == G3D_SHAPE_SPHERE) {
        d2 = g3d_v3_len2(g3d_v3_sub(a->a, b->a));
        return d2 <= r2;
    }
    if (a->type == G3D_SHAPE_SPHERE && b->type == G3D_SHAPE_CAPSULE) {
        d2 = g3d_dist2_point_segment(a->a, b->a, b->b);
        return d2 <= r2;
    }
    if (a->type == G3D_SHAPE_CAPSULE && b->type == G3D_SHAPE_SPHERE) {
        d2 = g3d_dist2_point_segment(b->a, a->a, a->b);
        return d2 <= r2;
    }
    if (a->type == G3D_SHAPE_CAPSULE && b->type == G3D_SHAPE_CAPSULE) {
        d2 = g3d_dist2_segment_segment_approx(a->a, a->b, b->a, b->b);
        return d2 <= r2;
    }
    return 0;
}

static g3d_actor *g3d_find_actor(g3d_world *w, int actor_id)
{
    int i;
    if (w == 0) return 0;
    for (i = 0; i < G3D_MAX_ACTORS; ++i) {
        if (w->actors[i].active && w->actors[i].id == actor_id) return &w->actors[i];
    }
    return 0;
}

static const g3d_actor *g3d_find_actor_const(const g3d_world *w, int actor_id)
{
    int i;
    if (w == 0) return 0;
    for (i = 0; i < G3D_MAX_ACTORS; ++i) {
        if (w->actors[i].active && w->actors[i].id == actor_id) return &w->actors[i];
    }
    return 0;
}

static void g3d_push_event(g3d_world *w, const g3d_grab_event *ev)
{
    int next;
    if (w == 0 || ev == 0) return;
    next = (w->event_tail + 1) % G3D_MAX_EVENTS;
    if (next == w->event_head) {
        w->event_head = (w->event_head + 1) % G3D_MAX_EVENTS;
    }
    w->events[w->event_tail] = *ev;
    w->event_tail = next;
}

static void g3d_emit(g3d_world *w, int type, int grab_id, int owner, int target, int hb, int hs, g3d_v3 point, int value)
{
    g3d_grab_event ev;
    ev.type = type;
    ev.grab_id = grab_id;
    ev.owner_actor_id = owner;
    ev.target_actor_id = target;
    ev.hurtbox_index = hb;
    ev.grab_shape_index = hs;
    ev.point = point;
    ev.value = value;
    g3d_push_event(w, &ev);
}

static int g3d_hitlog_has(const g3d_grab_instance *g, int actor_id)
{
    int i;
    for (i = 0; i < g->hitlog_count; ++i) {
        if (g->hitlog[i] == actor_id) return 1;
    }
    return 0;
}

static void g3d_hitlog_add(g3d_grab_instance *g, int actor_id)
{
    if (g->hitlog_count < G3D_MAX_HITLOG) {
        g->hitlog[g->hitlog_count] = actor_id;
        g->hitlog_count += 1;
    }
}

void g3d_world_init(g3d_world *w)
{
    int i;
    if (w == 0) return;
    for (i = 0; i < G3D_MAX_ACTORS; ++i) w->actors[i].active = 0;
    for (i = 0; i < G3D_MAX_GRABS_ACTIVE; ++i) w->grabs[i].active = 0;
    w->event_head = 0;
    w->event_tail = 0;
    w->tick = 0;
}

int g3d_add_actor(g3d_world *w, int actor_id, int team, g3d_v3 pos)
{
    int i;
    if (w == 0) return G3D_ERR_BAD_ARG;
    if (g3d_find_actor(w, actor_id) != 0) return G3D_ERR_BAD_ARG;
    for (i = 0; i < G3D_MAX_ACTORS; ++i) {
        if (!w->actors[i].active) {
            w->actors[i].active = 1;
            w->actors[i].id = actor_id;
            w->actors[i].team = team;
            w->actors[i].flags = 0;
            w->actors[i].pos = pos;
            w->actors[i].hurtbox_count = 0;
            w->actors[i].held_by_actor_id = 0;
            w->actors[i].hold_slot = -1;
            w->actors[i].escape_power = 0;
            return G3D_OK;
        }
    }
    return G3D_ERR_FULL;
}

int g3d_set_actor_pos(g3d_world *w, int actor_id, g3d_v3 pos)
{
    g3d_actor *a;
    a = g3d_find_actor(w, actor_id);
    if (a == 0) return G3D_ERR_NOT_FOUND;
    a->pos = pos;
    return G3D_OK;
}

int g3d_shape_make_sphere(g3d_shape *out_shape, g3d_v3 center, g3d_fx radius, int flags)
{
    if (out_shape == 0 || radius < 0) return G3D_ERR_BAD_ARG;
    out_shape->type = G3D_SHAPE_SPHERE;
    out_shape->a = center;
    out_shape->b = center;
    out_shape->radius = radius;
    out_shape->flags = flags;
    return G3D_OK;
}

int g3d_shape_make_capsule(g3d_shape *out_shape, g3d_v3 a, g3d_v3 b, g3d_fx radius, int flags)
{
    if (out_shape == 0 || radius < 0) return G3D_ERR_BAD_ARG;
    out_shape->type = G3D_SHAPE_CAPSULE;
    out_shape->a = a;
    out_shape->b = b;
    out_shape->radius = radius;
    out_shape->flags = flags;
    return G3D_OK;
}

int g3d_actor_add_hurt_sphere(g3d_world *w, int actor_id, g3d_v3 center, g3d_fx radius, int flags)
{
    g3d_actor *a;
    a = g3d_find_actor(w, actor_id);
    if (a == 0) return G3D_ERR_NOT_FOUND;
    if (a->hurtbox_count >= G3D_MAX_HURTBOXES_PER_ACTOR) return G3D_ERR_FULL;
    g3d_shape_make_sphere(&a->hurtboxes[a->hurtbox_count], center, radius, flags);
    a->hurtbox_count += 1;
    return G3D_OK;
}

int g3d_actor_add_hurt_capsule(g3d_world *w, int actor_id, g3d_v3 a0, g3d_v3 b0, g3d_fx radius, int flags)
{
    g3d_actor *a;
    a = g3d_find_actor(w, actor_id);
    if (a == 0) return G3D_ERR_NOT_FOUND;
    if (a->hurtbox_count >= G3D_MAX_HURTBOXES_PER_ACTOR) return G3D_ERR_FULL;
    g3d_shape_make_capsule(&a->hurtboxes[a->hurtbox_count], a0, b0, radius, flags);
    a->hurtbox_count += 1;
    return G3D_OK;
}

int g3d_start_grab(g3d_world *w, int owner_actor_id, const g3d_grab_def *def, const g3d_shape *shapes, int shape_count)
{
    int i;
    int j;
    g3d_grab_instance *g;
    if (w == 0 || def == 0 || shapes == 0) return G3D_ERR_BAD_ARG;
    if (shape_count <= 0 || shape_count > G3D_MAX_GRAB_SHAPES) return G3D_ERR_BAD_ARG;
    if (g3d_find_actor(w, owner_actor_id) == 0) return G3D_ERR_NOT_FOUND;
    for (i = 0; i < G3D_MAX_GRABS_ACTIVE; ++i) {
        if (!w->grabs[i].active) {
            g = &w->grabs[i];
            g->active = 1;
            g->owner_actor_id = owner_actor_id;
            g->target_actor_id = 0;
            g->def = *def;
            g->age_ticks = 0;
            g->phase = G3D_PHASE_STARTUP;
            g->shape_count = shape_count;
            for (j = 0; j < shape_count; ++j) g->shapes[j] = shapes[j];
            g->hitlog_count = 0;
            g3d_emit(w, G3D_EVENT_GRAB_STARTED, def->id, owner_actor_id, 0, -1, -1, g3d_v3_make(0,0,0), 0);
            return G3D_OK;
        }
    }
    return G3D_ERR_FULL;
}

int g3d_force_release(g3d_world *w, int owner_actor_id, int target_actor_id, int event_type)
{
    int i;
    g3d_actor *t;
    if (w == 0) return G3D_ERR_BAD_ARG;
    for (i = 0; i < G3D_MAX_GRABS_ACTIVE; ++i) {
        if (w->grabs[i].active && w->grabs[i].owner_actor_id == owner_actor_id && w->grabs[i].target_actor_id == target_actor_id) {
            t = g3d_find_actor(w, target_actor_id);
            if (t != 0) {
                t->held_by_actor_id = 0;
                t->hold_slot = -1;
            }
            g3d_emit(w, event_type, w->grabs[i].def.id, owner_actor_id, target_actor_id, -1, -1, g3d_v3_make(0,0,0), 0);
            w->grabs[i].active = 0;
            return G3D_OK;
        }
    }
    return G3D_ERR_NOT_FOUND;
}

int g3d_actor_add_escape_power(g3d_world *w, int actor_id, int amount)
{
    g3d_actor *a;
    a = g3d_find_actor(w, actor_id);
    if (a == 0) return G3D_ERR_NOT_FOUND;
    a->escape_power += amount;
    if (a->escape_power < 0) a->escape_power = 0;
    return G3D_OK;
}

static void g3d_try_acquire(g3d_world *w, g3d_grab_instance *g)
{
    int ai;
    int hi;
    int si;
    g3d_actor *owner;
    g3d_actor *target;
    g3d_shape grab_shape;
    g3d_shape hurt_shape;
    owner = g3d_find_actor(w, g->owner_actor_id);
    if (owner == 0) return;
    for (ai = 0; ai < G3D_MAX_ACTORS; ++ai) {
        target = &w->actors[ai];
        if (!target->active) continue;
        if (target->id == owner->id) continue;
        if (target->held_by_actor_id != 0) continue;
        if (!(g->def.flags & G3D_GRAB_ALLOW_SAME_TEAM) && target->team == owner->team) continue;
        if (g3d_hitlog_has(g, target->id)) continue;
        for (si = 0; si < g->shape_count; ++si) {
            g3d_shape_world(&g->shapes[si], owner->pos, &grab_shape);
            for (hi = 0; hi < target->hurtbox_count; ++hi) {
                g3d_shape_world(&target->hurtboxes[hi], target->pos, &hurt_shape);
                if (g3d_shapes_overlap(&grab_shape, &hurt_shape)) {
                    g3d_hitlog_add(g, target->id);
                    g->target_actor_id = target->id;
                    g->phase = G3D_PHASE_HOLD;
                    g->age_ticks = 0;
                    target->held_by_actor_id = owner->id;
                    target->hold_slot = (int)(g - w->grabs);
                    target->escape_power = 0;
                    g3d_emit(w, G3D_EVENT_GRAB_ACQUIRED, g->def.id, owner->id, target->id, hi, si, hurt_shape.a, 0);
                    return;
                }
            }
        }
    }
}

void g3d_tick(g3d_world *w)
{
    int i;
    g3d_grab_instance *g;
    g3d_actor *owner;
    g3d_actor *target;
    if (w == 0) return;
    w->tick += 1;
    for (i = 0; i < G3D_MAX_GRABS_ACTIVE; ++i) {
        g = &w->grabs[i];
        if (!g->active) continue;
        if (g->phase == G3D_PHASE_STARTUP) {
            g->age_ticks += 1;
            if (g->age_ticks >= g->def.startup_ticks) {
                g->phase = G3D_PHASE_ACTIVE;
                g->age_ticks = 0;
            }
            continue;
        }
        if (g->phase == G3D_PHASE_ACTIVE) {
            g3d_try_acquire(w, g);
            if (g->phase == G3D_PHASE_ACTIVE) {
                g->age_ticks += 1;
                if (g->age_ticks >= g->def.active_ticks) {
                    g3d_emit(w, G3D_EVENT_GRAB_FAILED, g->def.id, g->owner_actor_id, 0, -1, -1, g3d_v3_make(0,0,0), 0);
                    g->phase = G3D_PHASE_RECOVER;
                    g->age_ticks = 0;
                }
            }
            continue;
        }
        if (g->phase == G3D_PHASE_HOLD) {
            owner = g3d_find_actor(w, g->owner_actor_id);
            target = g3d_find_actor(w, g->target_actor_id);
            if (owner == 0 || target == 0) {
                g->active = 0;
                continue;
            }
            if (target->escape_power >= g3d_abs_i(g->def.break_power) && g->def.break_power > 0) {
                target->held_by_actor_id = 0;
                target->hold_slot = -1;
                g3d_emit(w, G3D_EVENT_GRAB_BROKEN, g->def.id, owner->id, target->id, -1, -1, target->pos, target->escape_power);
                g->active = 0;
                continue;
            }
            if (g->def.flags & G3D_GRAB_LOCK_POSITION) {
                target->pos = g3d_v3_add(owner->pos, g->def.hold_offset);
            }
            g3d_emit(w, G3D_EVENT_GRAB_HOLD_TICK, g->def.id, owner->id, target->id, -1, -1, target->pos, g->age_ticks);
            g->age_ticks += 1;
            if (g->age_ticks >= g->def.hold_ticks) {
                target->held_by_actor_id = 0;
                target->hold_slot = -1;
                g3d_emit(w, G3D_EVENT_GRAB_EXPIRED, g->def.id, owner->id, target->id, -1, -1, target->pos, 0);
                g->phase = G3D_PHASE_RECOVER;
                g->age_ticks = 0;
            }
            continue;
        }
        if (g->phase == G3D_PHASE_RECOVER) {
            g->age_ticks += 1;
            if (g->age_ticks >= g->def.recovery_ticks) {
                g->active = 0;
            }
            continue;
        }
    }
}

int g3d_poll_event(g3d_world *w, g3d_grab_event *out_event)
{
    if (w == 0 || out_event == 0) return G3D_ERR_BAD_ARG;
    if (w->event_head == w->event_tail) return G3D_NO;
    *out_event = w->events[w->event_head];
    w->event_head = (w->event_head + 1) % G3D_MAX_EVENTS;
    return G3D_OK;
}

int g3d_actor_is_held(const g3d_world *w, int actor_id)
{
    const g3d_actor *a;
    a = g3d_find_actor_const(w, actor_id);
    if (a == 0) return 0;
    return a->held_by_actor_id != 0;
}
