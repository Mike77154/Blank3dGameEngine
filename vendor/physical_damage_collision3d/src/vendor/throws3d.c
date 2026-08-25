#include "throws3d.h"

#define T3D_PHASE_WINDUP 0
#define T3D_PHASE_TRAVEL 1
#define T3D_PHASE_RECOVER 2

t3d_v3 t3d_v3_make(t3d_fx x, t3d_fx y, t3d_fx z) { t3d_v3 r; r.x = x; r.y = y; r.z = z; return r; }
t3d_v3 t3d_v3_add(t3d_v3 a, t3d_v3 b) { return t3d_v3_make(a.x + b.x, a.y + b.y, a.z + b.z); }
t3d_v3 t3d_v3_sub(t3d_v3 a, t3d_v3 b) { return t3d_v3_make(a.x - b.x, a.y - b.y, a.z - b.z); }
t3d_v3 t3d_v3_mul_fx(t3d_v3 a, t3d_fx s) { return t3d_v3_make(T3D_FX_MUL(a.x, s), T3D_FX_MUL(a.y, s), T3D_FX_MUL(a.z, s)); }

static t3d_body *t3d_find_body(t3d_world *w, int body_id)
{
    int i;
    if (w == 0) return 0;
    for (i = 0; i < T3D_MAX_BODIES; ++i) {
        if (w->bodies[i].active && w->bodies[i].id == body_id) return &w->bodies[i];
    }
    return 0;
}

static const t3d_body *t3d_find_body_const(const t3d_world *w, int body_id)
{
    int i;
    if (w == 0) return 0;
    for (i = 0; i < T3D_MAX_BODIES; ++i) {
        if (w->bodies[i].active && w->bodies[i].id == body_id) return &w->bodies[i];
    }
    return 0;
}

static void t3d_push_event(t3d_world *w, const t3d_throw_event *ev)
{
    int next;
    if (w == 0 || ev == 0) return;
    next = (w->event_tail + 1) % T3D_MAX_EVENTS;
    if (next == w->event_head) w->event_head = (w->event_head + 1) % T3D_MAX_EVENTS;
    w->events[w->event_tail] = *ev;
    w->event_tail = next;
}

static void t3d_emit(t3d_world *w, int type, int throw_id, int attacker_id, int body_id, t3d_v3 pos, t3d_v3 vel, int damage, int value)
{
    t3d_throw_event ev;
    ev.type = type;
    ev.throw_id = throw_id;
    ev.attacker_id = attacker_id;
    ev.body_id = body_id;
    ev.pos = pos;
    ev.vel = vel;
    ev.damage = damage;
    ev.value = value;
    t3d_push_event(w, &ev);
}

void t3d_world_init(t3d_world *w)
{
    int i;
    if (w == 0) return;
    for (i = 0; i < T3D_MAX_BODIES; ++i) w->bodies[i].active = 0;
    for (i = 0; i < T3D_MAX_THROWS_ACTIVE; ++i) w->throws[i].active = 0;
    w->event_head = 0;
    w->event_tail = 0;
    w->tick = 0;
}

int t3d_add_body(t3d_world *w, int body_id, t3d_v3 pos, t3d_fx radius)
{
    int i;
    if (w == 0 || radius < 0) return T3D_ERR_BAD_ARG;
    if (t3d_find_body(w, body_id) != 0) return T3D_ERR_BAD_ARG;
    for (i = 0; i < T3D_MAX_BODIES; ++i) {
        if (!w->bodies[i].active) {
            w->bodies[i].active = 1;
            w->bodies[i].id = body_id;
            w->bodies[i].state = T3D_BODY_IDLE;
            w->bodies[i].owner_actor_id = 0;
            w->bodies[i].pos = pos;
            w->bodies[i].vel = t3d_v3_make(0,0,0);
            w->bodies[i].radius = radius;
            return T3D_OK;
        }
    }
    return T3D_ERR_FULL;
}

int t3d_set_body_pos(t3d_world *w, int body_id, t3d_v3 pos)
{
    t3d_body *b;
    b = t3d_find_body(w, body_id);
    if (b == 0) return T3D_ERR_NOT_FOUND;
    b->pos = pos;
    return T3D_OK;
}

int t3d_get_body_pos(const t3d_world *w, int body_id, t3d_v3 *out_pos)
{
    const t3d_body *b;
    if (out_pos == 0) return T3D_ERR_BAD_ARG;
    b = t3d_find_body_const(w, body_id);
    if (b == 0) return T3D_ERR_NOT_FOUND;
    *out_pos = b->pos;
    return T3D_OK;
}

int t3d_throw_make_arc_velocity(t3d_v3 from, t3d_v3 to, int ticks, t3d_fx gravity, t3d_v3 *out_velocity)
{
    t3d_v3 delta;
    long t;
    long half_term;
    if (out_velocity == 0 || ticks <= 0) return T3D_ERR_BAD_ARG;
    delta = t3d_v3_sub(to, from);
    t = (long)ticks;
    out_velocity->x = (t3d_fx)(delta.x / ticks);
    out_velocity->z = (t3d_fx)(delta.z / ticks);
    half_term = (t * (t - 1L)) / 2L;
    out_velocity->y = (t3d_fx)((delta.y + (t3d_fx)(gravity * half_term)) / ticks);
    return T3D_OK;
}

int t3d_start_throw(t3d_world *w, int attacker_id, int body_id, const t3d_throw_def *def, t3d_v3 start_pos, t3d_v3 velocity)
{
    int i;
    t3d_throw_instance *th;
    t3d_body *body;
    if (w == 0 || def == 0) return T3D_ERR_BAD_ARG;
    body = t3d_find_body(w, body_id);
    if (body == 0) return T3D_ERR_NOT_FOUND;
    for (i = 0; i < T3D_MAX_THROWS_ACTIVE; ++i) {
        if (!w->throws[i].active) {
            th = &w->throws[i];
            th->active = 1;
            th->attacker_id = attacker_id;
            th->body_id = body_id;
            th->def = *def;
            th->pos = start_pos;
            th->vel = velocity;
            th->age_ticks = 0;
            th->phase = T3D_PHASE_WINDUP;
            body->state = T3D_BODY_THROWN;
            body->owner_actor_id = attacker_id;
            body->pos = start_pos;
            body->vel = velocity;
            t3d_emit(w, T3D_EVENT_THROW_STARTED, def->id, attacker_id, body_id, start_pos, velocity, 0, 0);
            return T3D_OK;
        }
    }
    return T3D_ERR_FULL;
}

void t3d_tick(t3d_world *w)
{
    int i;
    t3d_throw_instance *th;
    t3d_body *body;
    if (w == 0) return;
    w->tick += 1;
    for (i = 0; i < T3D_MAX_THROWS_ACTIVE; ++i) {
        th = &w->throws[i];
        if (!th->active) continue;
        body = t3d_find_body(w, th->body_id);
        if (body == 0) { th->active = 0; continue; }
        if (th->phase == T3D_PHASE_WINDUP) {
            th->age_ticks += 1;
            if (th->age_ticks >= th->def.windup_ticks) {
                th->phase = T3D_PHASE_TRAVEL;
                th->age_ticks = 0;
                t3d_emit(w, T3D_EVENT_THROW_RELEASED, th->def.id, th->attacker_id, th->body_id, th->pos, th->vel, 0, 0);
            }
            continue;
        }
        if (th->phase == T3D_PHASE_TRAVEL) {
            th->pos = t3d_v3_add(th->pos, th->vel);
            th->vel.y -= th->def.gravity;
            body->pos = th->pos;
            body->vel = th->vel;
            t3d_emit(w, T3D_EVENT_THROW_MOTION, th->def.id, th->attacker_id, th->body_id, th->pos, th->vel, 0, th->age_ticks);
            th->age_ticks += 1;
            if ((th->def.flags & T3D_THROW_STOP_ON_GROUND) && th->pos.y <= th->def.ground_y) {
                th->pos.y = th->def.ground_y;
                body->pos = th->pos;
                body->state = T3D_BODY_LANDED;
                t3d_emit(w, T3D_EVENT_THROW_IMPACT, th->def.id, th->attacker_id, th->body_id, th->pos, th->vel, th->def.damage, th->def.stun_ticks);
                th->phase = T3D_PHASE_RECOVER;
                th->age_ticks = 0;
                continue;
            }
            if (th->age_ticks >= th->def.travel_ticks) {
                body->state = T3D_BODY_LANDED;
                t3d_emit(w, T3D_EVENT_THROW_IMPACT, th->def.id, th->attacker_id, th->body_id, th->pos, th->vel, th->def.damage, th->def.stun_ticks);
                th->phase = T3D_PHASE_RECOVER;
                th->age_ticks = 0;
            }
            continue;
        }
        if (th->phase == T3D_PHASE_RECOVER) {
            th->age_ticks += 1;
            if (th->age_ticks >= th->def.recovery_ticks) {
                body->state = T3D_BODY_IDLE;
                body->owner_actor_id = 0;
                t3d_emit(w, T3D_EVENT_THROW_DONE, th->def.id, th->attacker_id, th->body_id, body->pos, body->vel, 0, 0);
                th->active = 0;
            }
        }
    }
}

int t3d_poll_event(t3d_world *w, t3d_throw_event *out_event)
{
    if (w == 0 || out_event == 0) return T3D_ERR_BAD_ARG;
    if (w->event_head == w->event_tail) return T3D_NO;
    *out_event = w->events[w->event_head];
    w->event_head = (w->event_head + 1) % T3D_MAX_EVENTS;
    return T3D_OK;
}
