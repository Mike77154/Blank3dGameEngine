#ifndef THROWS3D_H
#define THROWS3D_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int t3d_fx;

#define T3D_FX_SHIFT 8
#define T3D_FX_ONE   (1 << T3D_FX_SHIFT)
#define T3D_FX_FROM_INT(x) ((t3d_fx)((x) * T3D_FX_ONE))
#define T3D_FX_TO_INT(x)   ((int)((x) / T3D_FX_ONE))
#define T3D_FX_MUL(a,b)    ((t3d_fx)(((long)(a) * (long)(b)) >> T3D_FX_SHIFT))
#define T3D_FX_DIV(a,b)    ((t3d_fx)(((long)(a) * (long)T3D_FX_ONE) / (long)(b)))
#define T3D_FX_DIV_INT(a,b) ((t3d_fx)((a) / (b)))

#define T3D_OK 1
#define T3D_NO 0
#define T3D_ERR_FULL -1
#define T3D_ERR_BAD_ARG -2
#define T3D_ERR_NOT_FOUND -3

#define T3D_MAX_BODIES 64
#define T3D_MAX_THROWS_ACTIVE 32
#define T3D_MAX_EVENTS 128

#define T3D_BODY_IDLE 0
#define T3D_BODY_THROWN 1
#define T3D_BODY_LANDED 2

#define T3D_EVENT_THROW_STARTED 1
#define T3D_EVENT_THROW_RELEASED 2
#define T3D_EVENT_THROW_MOTION 3
#define T3D_EVENT_THROW_IMPACT 4
#define T3D_EVENT_THROW_DONE 5

#define T3D_THROW_STOP_ON_GROUND 0x0001
#define T3D_THROW_LOCK_TARGET    0x0002

typedef struct t3d_v3_s {
    t3d_fx x;
    t3d_fx y;
    t3d_fx z;
} t3d_v3;

typedef struct t3d_body_s {
    int active;
    int id;
    int state;
    int owner_actor_id;
    t3d_v3 pos;
    t3d_v3 vel;
    t3d_fx radius;
} t3d_body;

typedef struct t3d_throw_def_s {
    int id;
    int windup_ticks;
    int travel_ticks;
    int recovery_ticks;
    int damage;
    int stun_ticks;
    int flags;
    t3d_fx gravity;
    t3d_fx ground_y;
} t3d_throw_def;

typedef struct t3d_throw_instance_s {
    int active;
    int attacker_id;
    int body_id;
    t3d_throw_def def;
    t3d_v3 pos;
    t3d_v3 vel;
    int age_ticks;
    int phase;
} t3d_throw_instance;

typedef struct t3d_throw_event_s {
    int type;
    int throw_id;
    int attacker_id;
    int body_id;
    t3d_v3 pos;
    t3d_v3 vel;
    int damage;
    int value;
} t3d_throw_event;

typedef struct t3d_world_s {
    t3d_body bodies[T3D_MAX_BODIES];
    t3d_throw_instance throws[T3D_MAX_THROWS_ACTIVE];
    t3d_throw_event events[T3D_MAX_EVENTS];
    int event_head;
    int event_tail;
    int tick;
} t3d_world;

t3d_v3 t3d_v3_make(t3d_fx x, t3d_fx y, t3d_fx z);
t3d_v3 t3d_v3_add(t3d_v3 a, t3d_v3 b);
t3d_v3 t3d_v3_sub(t3d_v3 a, t3d_v3 b);
t3d_v3 t3d_v3_mul_fx(t3d_v3 a, t3d_fx s);

void t3d_world_init(t3d_world *w);
int t3d_add_body(t3d_world *w, int body_id, t3d_v3 pos, t3d_fx radius);
int t3d_set_body_pos(t3d_world *w, int body_id, t3d_v3 pos);
int t3d_get_body_pos(const t3d_world *w, int body_id, t3d_v3 *out_pos);

int t3d_throw_make_arc_velocity(t3d_v3 from, t3d_v3 to, int ticks, t3d_fx gravity, t3d_v3 *out_velocity);
int t3d_start_throw(t3d_world *w, int attacker_id, int body_id, const t3d_throw_def *def, t3d_v3 start_pos, t3d_v3 velocity);
void t3d_tick(t3d_world *w);
int t3d_poll_event(t3d_world *w, t3d_throw_event *out_event);

#ifdef __cplusplus
}
#endif

#endif
