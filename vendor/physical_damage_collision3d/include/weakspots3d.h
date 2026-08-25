#ifndef WEAKSPOTS3D_H
#define WEAKSPOTS3D_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int w3d_fx;

#define W3D_FX_SHIFT 8
#define W3D_FX_ONE   (1 << W3D_FX_SHIFT)
#define W3D_FX_FROM_INT(x) ((w3d_fx)((x) * W3D_FX_ONE))
#define W3D_FX_TO_INT(x)   ((int)((x) / W3D_FX_ONE))
#define W3D_FX_MUL(a,b)    ((w3d_fx)(((long)(a) * (long)(b)) >> W3D_FX_SHIFT))
#define W3D_FX_DIV(a,b)    ((w3d_fx)(((long)(a) * (long)W3D_FX_ONE) / (long)(b)))

#define W3D_OK 1
#define W3D_NO 0
#define W3D_ERR_FULL -1
#define W3D_ERR_BAD_ARG -2
#define W3D_ERR_NOT_FOUND -3

#define W3D_MAX_ACTORS 64
#define W3D_MAX_WEAKSPOTS_PER_ACTOR 16
#define W3D_MAX_EVENTS 96

#define W3D_SHAPE_SPHERE 1
#define W3D_SHAPE_CAPSULE 2
#define W3D_SHAPE_OBB 3

#define W3D_WEAKSPOT_BREAKABLE 0x0001
#define W3D_WEAKSPOT_ONCE      0x0002
#define W3D_WEAKSPOT_ARMOR     0x0004
#define W3D_WEAKSPOT_DISABLED  0x0008

#define W3D_ATTACK_SLASH       0x0001
#define W3D_ATTACK_BLUNT       0x0002
#define W3D_ATTACK_BITE        0x0004
#define W3D_ATTACK_PIERCE      0x0008
#define W3D_ATTACK_FIRE        0x0010
#define W3D_ATTACK_ACID        0x0020

#define W3D_EVENT_WEAKSPOT_HIT    1
#define W3D_EVENT_WEAKSPOT_BROKEN 2
#define W3D_EVENT_WEAKSPOT_IGNORED 3

/* Phase96 duplicate-symbol guard:
   PhysicalDamageCollision3D vendors a small Weakspots3D helper whose public
   symbols used the old generic w3d_* prefix. Blank3D also links World3D89,
   whose official API owns w3d_* in the global linker namespace.

   Keep the source-compatible names for PDC3D callers through macros, but
   emit pdc3d_w3d_* symbols from this vendor so World3D89 remains the only
   provider of global w3d_* runtime symbols. Define
   WEAKSPOTS3D_KEEP_LEGACY_W3D_NAMES before including this header only when
   building Weakspots3D standalone without World3D89. */
#ifndef WEAKSPOTS3D_KEEP_LEGACY_W3D_NAMES
#define w3d_v3_make                       pdc3d_w3d_v3_make
#define w3d_v3_add                        pdc3d_w3d_v3_add
#define w3d_v3_sub                        pdc3d_w3d_v3_sub
#define w3d_v3_dot                        pdc3d_w3d_v3_dot
#define w3d_mat3_identity                 pdc3d_w3d_mat3_identity
#define w3d_mat3_rot_y_90                 pdc3d_w3d_mat3_rot_y_90
#define w3d_world_init                    pdc3d_w3d_world_init
#define w3d_add_actor                     pdc3d_w3d_add_actor
#define w3d_set_actor_pos                 pdc3d_w3d_set_actor_pos
#define w3d_add_weakspot_sphere           pdc3d_w3d_add_weakspot_sphere
#define w3d_add_weakspot_capsule          pdc3d_w3d_add_weakspot_capsule
#define w3d_add_weakspot_obb              pdc3d_w3d_add_weakspot_obb
#define w3d_set_weakspot_attack_filter    pdc3d_w3d_set_weakspot_attack_filter
#define w3d_resolve_hit_point             pdc3d_w3d_resolve_hit_point
#define w3d_poll_event                    pdc3d_w3d_poll_event
#endif

typedef struct w3d_v3_s {
    w3d_fx x;
    w3d_fx y;
    w3d_fx z;
} w3d_v3;

typedef struct w3d_mat3_s {
    w3d_fx m00; w3d_fx m01; w3d_fx m02;
    w3d_fx m10; w3d_fx m11; w3d_fx m12;
    w3d_fx m20; w3d_fx m21; w3d_fx m22;
} w3d_mat3;

typedef struct w3d_shape_s {
    int type;
    w3d_v3 a;
    w3d_v3 b;
    w3d_v3 half;
    w3d_mat3 axis;
    w3d_fx radius;
} w3d_shape;

typedef struct w3d_weakspot_s {
    int active;
    int id;
    w3d_shape shape;
    int flags;
    int required_attack_flags;
    int blocked_attack_flags;
    int multiplier_num;
    int multiplier_den;
    int durability;
    int hits_taken;
} w3d_weakspot;

typedef struct w3d_actor_s {
    int active;
    int id;
    w3d_v3 pos;
    w3d_weakspot weakspots[W3D_MAX_WEAKSPOTS_PER_ACTOR];
    int weakspot_count;
} w3d_actor;

typedef struct w3d_hit_result_s {
    int matched;
    int actor_id;
    int weakspot_id;
    int final_damage;
    int multiplier_num;
    int multiplier_den;
    int flags;
} w3d_hit_result;

typedef struct w3d_event_s {
    int type;
    int actor_id;
    int weakspot_id;
    int base_damage;
    int final_damage;
    int value;
} w3d_event;

typedef struct w3d_world_s {
    w3d_actor actors[W3D_MAX_ACTORS];
    w3d_event events[W3D_MAX_EVENTS];
    int event_head;
    int event_tail;
} w3d_world;

w3d_v3 w3d_v3_make(w3d_fx x, w3d_fx y, w3d_fx z);
w3d_v3 w3d_v3_add(w3d_v3 a, w3d_v3 b);
w3d_v3 w3d_v3_sub(w3d_v3 a, w3d_v3 b);
w3d_fx w3d_v3_dot(w3d_v3 a, w3d_v3 b);
w3d_mat3 w3d_mat3_identity(void);
w3d_mat3 w3d_mat3_rot_y_90(void);

void w3d_world_init(w3d_world *w);
int w3d_add_actor(w3d_world *w, int actor_id, w3d_v3 pos);
int w3d_set_actor_pos(w3d_world *w, int actor_id, w3d_v3 pos);

int w3d_add_weakspot_sphere(w3d_world *w, int actor_id, int weakspot_id, w3d_v3 center, w3d_fx radius, int mult_num, int mult_den, int durability, int flags);
int w3d_add_weakspot_capsule(w3d_world *w, int actor_id, int weakspot_id, w3d_v3 a, w3d_v3 b, w3d_fx radius, int mult_num, int mult_den, int durability, int flags);
int w3d_add_weakspot_obb(w3d_world *w, int actor_id, int weakspot_id, w3d_v3 center, w3d_v3 half, w3d_mat3 axis, int mult_num, int mult_den, int durability, int flags);
int w3d_set_weakspot_attack_filter(w3d_world *w, int actor_id, int weakspot_id, int required_flags, int blocked_flags);

int w3d_resolve_hit_point(w3d_world *w, int actor_id, w3d_v3 world_point, int base_damage, int attack_flags, w3d_hit_result *out_result);
int w3d_poll_event(w3d_world *w, w3d_event *out_event);

#ifdef __cplusplus
}
#endif

#endif
