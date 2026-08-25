#ifndef GRAB3D_H
#define GRAB3D_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int g3d_fx;

#define G3D_FX_SHIFT 8
#define G3D_FX_ONE   (1 << G3D_FX_SHIFT)
#define G3D_FX_HALF  (1 << (G3D_FX_SHIFT - 1))
#define G3D_FX_FROM_INT(x) ((g3d_fx)((x) * G3D_FX_ONE))
#define G3D_FX_TO_INT(x)   ((int)((x) / G3D_FX_ONE))
#define G3D_FX_MUL(a,b)    ((g3d_fx)(((long)(a) * (long)(b)) >> G3D_FX_SHIFT))
#define G3D_FX_DIV(a,b)    ((g3d_fx)(((long)(a) * (long)G3D_FX_ONE) / (long)(b)))

#define G3D_OK 1
#define G3D_NO 0
#define G3D_ERR_FULL -1
#define G3D_ERR_BAD_ARG -2
#define G3D_ERR_NOT_FOUND -3

#define G3D_MAX_ACTORS 64
#define G3D_MAX_HURTBOXES_PER_ACTOR 12
#define G3D_MAX_GRABS_ACTIVE 32
#define G3D_MAX_GRAB_SHAPES 8
#define G3D_MAX_HITLOG 32
#define G3D_MAX_EVENTS 96

#define G3D_SHAPE_SPHERE 1
#define G3D_SHAPE_CAPSULE 2

#define G3D_EVENT_GRAB_STARTED 1
#define G3D_EVENT_GRAB_ACQUIRED 2
#define G3D_EVENT_GRAB_HOLD_TICK 3
#define G3D_EVENT_GRAB_RELEASED 4
#define G3D_EVENT_GRAB_BROKEN 5
#define G3D_EVENT_GRAB_EXPIRED 6
#define G3D_EVENT_GRAB_FAILED 7

#define G3D_GRAB_ALLOW_SAME_TEAM 0x0001
#define G3D_GRAB_ONE_TARGET      0x0002
#define G3D_GRAB_LOCK_POSITION   0x0004
#define G3D_GRAB_REQUIRE_FACING  0x0008

typedef struct g3d_v3_s {
    g3d_fx x;
    g3d_fx y;
    g3d_fx z;
} g3d_v3;

typedef struct g3d_shape_s {
    int type;
    g3d_v3 a;
    g3d_v3 b;
    g3d_fx radius;
    int flags;
} g3d_shape;

typedef struct g3d_actor_s {
    int active;
    int id;
    int team;
    int flags;
    g3d_v3 pos;
    g3d_shape hurtboxes[G3D_MAX_HURTBOXES_PER_ACTOR];
    int hurtbox_count;
    int held_by_actor_id;
    int hold_slot;
    int escape_power;
} g3d_actor;

typedef struct g3d_grab_def_s {
    int id;
    int startup_ticks;
    int active_ticks;
    int hold_ticks;
    int recovery_ticks;
    int break_power;
    g3d_v3 hold_offset;
    int flags;
} g3d_grab_def;

typedef struct g3d_grab_instance_s {
    int active;
    int owner_actor_id;
    int target_actor_id;
    g3d_grab_def def;
    int age_ticks;
    int phase;
    int shape_count;
    g3d_shape shapes[G3D_MAX_GRAB_SHAPES];
    int hitlog[G3D_MAX_HITLOG];
    int hitlog_count;
} g3d_grab_instance;

typedef struct g3d_grab_event_s {
    int type;
    int grab_id;
    int owner_actor_id;
    int target_actor_id;
    int hurtbox_index;
    int grab_shape_index;
    g3d_v3 point;
    int value;
} g3d_grab_event;

typedef struct g3d_world_s {
    g3d_actor actors[G3D_MAX_ACTORS];
    g3d_grab_instance grabs[G3D_MAX_GRABS_ACTIVE];
    g3d_grab_event events[G3D_MAX_EVENTS];
    int event_head;
    int event_tail;
    int tick;
} g3d_world;

g3d_v3 g3d_v3_make(g3d_fx x, g3d_fx y, g3d_fx z);
g3d_v3 g3d_v3_add(g3d_v3 a, g3d_v3 b);
g3d_v3 g3d_v3_sub(g3d_v3 a, g3d_v3 b);
g3d_fx g3d_v3_dot(g3d_v3 a, g3d_v3 b);
g3d_fx g3d_v3_len2(g3d_v3 a);

void g3d_world_init(g3d_world *w);
int g3d_add_actor(g3d_world *w, int actor_id, int team, g3d_v3 pos);
int g3d_set_actor_pos(g3d_world *w, int actor_id, g3d_v3 pos);
int g3d_actor_add_hurt_sphere(g3d_world *w, int actor_id, g3d_v3 center, g3d_fx radius, int flags);
int g3d_actor_add_hurt_capsule(g3d_world *w, int actor_id, g3d_v3 a, g3d_v3 b, g3d_fx radius, int flags);

int g3d_shape_make_sphere(g3d_shape *out_shape, g3d_v3 center, g3d_fx radius, int flags);
int g3d_shape_make_capsule(g3d_shape *out_shape, g3d_v3 a, g3d_v3 b, g3d_fx radius, int flags);

int g3d_start_grab(g3d_world *w, int owner_actor_id, const g3d_grab_def *def, const g3d_shape *shapes, int shape_count);
int g3d_force_release(g3d_world *w, int owner_actor_id, int target_actor_id, int event_type);
int g3d_actor_add_escape_power(g3d_world *w, int actor_id, int amount);
void g3d_tick(g3d_world *w);
int g3d_poll_event(g3d_world *w, g3d_grab_event *out_event);
int g3d_actor_is_held(const g3d_world *w, int actor_id);

#ifdef __cplusplus
}
#endif

#endif
