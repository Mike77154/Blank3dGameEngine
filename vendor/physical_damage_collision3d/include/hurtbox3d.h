#ifndef HURTBOX3D_H
#define HURTBOX3D_H

/*
    hurtbox3d - defensive 3D volumes + fixed geometry + optional grid broadphase

    C89, no malloc/realloc/free, no internal heap, no float/double.
    This library is intentionally agnostic: it only knows actors and hurt volumes.
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HB3_MAX_ACTORS
#define HB3_MAX_ACTORS 128
#endif

#ifndef HB3_MAX_HURTBOXES_PER_ACTOR
#define HB3_MAX_HURTBOXES_PER_ACTOR 16
#endif

#ifndef HB3_MAX_GRID_CELLS
#define HB3_MAX_GRID_CELLS 256
#endif

#ifndef HB3_MAX_GRID_REFS
#define HB3_MAX_GRID_REFS 1024
#endif

#ifndef HB3_FX_SHIFT
#define HB3_FX_SHIFT 4
#endif

#define HB3_FX_ONE (1 << HB3_FX_SHIFT)

#define HB3_OK                 0
#define HB3_ERR_FULL          -1
#define HB3_ERR_NOT_FOUND     -2
#define HB3_ERR_BAD_ARG       -3
#define HB3_ERR_INACTIVE      -4

#define HB3_SHAPE_NONE         0
#define HB3_SHAPE_SPHERE       1
#define HB3_SHAPE_CAPSULE      2
#define HB3_SHAPE_OBB          3

#define HB3_FLAG_ENABLED       0x0001

#define HB3_HURT_FLESH         0x0001
#define HB3_HURT_ARMOR         0x0002
#define HB3_HURT_HEAD          0x0004
#define HB3_HURT_LIMB          0x0008
#define HB3_HURT_WEAKSPOT      0x0010
#define HB3_HURT_SHIELD        0x0020
#define HB3_HURT_NO_DAMAGE     0x0040

#define HB3_DEBUG_HURTBOXES    0x0001
#define HB3_DEBUG_GRID         0x0002
#define HB3_DEBUG_ALL          0x7fff

#define HB3_DEBUG_COLOR_HURT   1
#define HB3_DEBUG_COLOR_GRID   3

#define HB3_BP_DISABLED        0
#define HB3_BP_GRID            1

#define HB3_ALL_MASK           0x7fffffff

typedef int hb3_fx;

typedef struct hb3_v3_s {
    hb3_fx x;
    hb3_fx y;
    hb3_fx z;
} hb3_v3;

/* 3x3 basis matrix stored as column axes. */
typedef struct hb3_mat3_s {
    hb3_v3 x;
    hb3_v3 y;
    hb3_v3 z;
} hb3_mat3;

typedef struct hb3_aabb_s {
    hb3_v3 min_v;
    hb3_v3 max_v;
} hb3_aabb;

typedef struct hb3_shape_s {
    int type;
    int id;
    int flags;
    int group_mask;
    int hit_mask;

    /* sphere: a=center, radius=radius
       capsule: a/b=endpoints, radius=radius
       OBB: a=center, half=half extents, basis=orientation */
    hb3_v3 a;
    hb3_v3 b;
    hb3_fx radius;
    hb3_v3 half;
    hb3_mat3 basis;
} hb3_shape;

typedef struct hb3_hurtbox_s {
    int active;
    int owner_id;
    hb3_shape shape;
    int material;
    int damage_mul_fx;
    int user_tag;
} hb3_hurtbox;

typedef struct hb3_actor_s {
    int active;
    int id;
    int team;
    int faction;
    int flags;
    int group_mask;
    int hit_mask;
    int user_kind;
    hb3_hurtbox hurtboxes[HB3_MAX_HURTBOXES_PER_ACTOR];
    int hurtbox_count;
} hb3_actor;

typedef struct hb3_broadphase_s {
    int mode;
    int valid;
    int overflow;
    hb3_v3 origin;
    hb3_fx cell_size;
    int cells_x;
    int cells_y;
    int cells_z;
    int cell_count;
    int cell_heads[HB3_MAX_GRID_CELLS];
    int ref_actor_index[HB3_MAX_GRID_REFS];
    int ref_next[HB3_MAX_GRID_REFS];
    int ref_count;
} hb3_broadphase;

typedef struct hb3_world_s {
    hb3_actor actors[HB3_MAX_ACTORS];
    hb3_broadphase bp;
    int tick;
    int flags;
} hb3_world;

typedef void (*hb3_debug_line_fn)(void *user, hb3_v3 a, hb3_v3 b, int color, int tag);

hb3_fx hb3_fx_from_int(int x);
int hb3_fx_to_int(hb3_fx x);
hb3_fx hb3_fx_mul(hb3_fx a, hb3_fx b);
hb3_fx hb3_fx_div(hb3_fx a, hb3_fx b);

hb3_v3 hb3_v3_make(hb3_fx x, hb3_fx y, hb3_fx z);
hb3_v3 hb3_v3_from_ints(int x, int y, int z);
hb3_v3 hb3_v3_add(hb3_v3 a, hb3_v3 b);
hb3_v3 hb3_v3_sub(hb3_v3 a, hb3_v3 b);
hb3_v3 hb3_v3_scale(hb3_v3 a, hb3_fx s);
hb3_fx hb3_v3_dot(hb3_v3 a, hb3_v3 b);
hb3_v3 hb3_v3_cross(hb3_v3 a, hb3_v3 b);
hb3_fx hb3_v3_len2(hb3_v3 a);

hb3_mat3 hb3_mat3_identity(void);
hb3_mat3 hb3_mat3_from_axes(hb3_v3 x_axis, hb3_v3 y_axis, hb3_v3 z_axis);
hb3_mat3 hb3_mat3_rot_x_cs(hb3_fx c, hb3_fx s);
hb3_mat3 hb3_mat3_rot_y_cs(hb3_fx c, hb3_fx s);
hb3_mat3 hb3_mat3_rot_z_cs(hb3_fx c, hb3_fx s);
hb3_mat3 hb3_mat3_mul(hb3_mat3 a, hb3_mat3 b);
hb3_v3 hb3_mat3_mul_v3(hb3_mat3 m, hb3_v3 v);
hb3_v3 hb3_mat3_tmul_v3(hb3_mat3 m, hb3_v3 v);

hb3_shape hb3_shape_make_sphere(int id, hb3_v3 center, hb3_fx radius,
                                int flags, int group_mask, int hit_mask);
hb3_shape hb3_shape_make_capsule(int id, hb3_v3 a, hb3_v3 b, hb3_fx radius,
                                 int flags, int group_mask, int hit_mask);
hb3_shape hb3_shape_make_obb(int id, hb3_v3 center, hb3_v3 half_extents,
                             hb3_mat3 basis, int flags, int group_mask,
                             int hit_mask);

int hb3_shape_get_aabb(const hb3_shape *s, hb3_aabb *out_aabb);
int hb3_shape_intersects(const hb3_shape *a, const hb3_shape *b);
int hb3_aabb_intersects(const hb3_aabb *a, const hb3_aabb *b);
void hb3_aabb_merge(hb3_aabb *dst, const hb3_aabb *src);

void hb3_world_init(hb3_world *w);
int hb3_actor_add(hb3_world *w, int actor_id, int team);
int hb3_actor_remove(hb3_world *w, int actor_id);
int hb3_actor_find_index(const hb3_world *w, int actor_id);
int hb3_actor_set_masks(hb3_world *w, int actor_id, int group_mask, int hit_mask);
int hb3_actor_clear_hurtboxes(hb3_world *w, int actor_id);
int hb3_actor_add_hurt_sphere(hb3_world *w, int actor_id, int hurt_id,
                              hb3_v3 center, hb3_fx radius,
                              int hurt_flags, int user_tag);
int hb3_actor_add_hurt_capsule(hb3_world *w, int actor_id, int hurt_id,
                               hb3_v3 a, hb3_v3 b, hb3_fx radius,
                               int hurt_flags, int user_tag);
int hb3_actor_add_hurt_obb(hb3_world *w, int actor_id, int hurt_id,
                           hb3_v3 center, hb3_v3 half_extents,
                           hb3_mat3 basis, int hurt_flags, int user_tag);
int hb3_actor_get_aabb(const hb3_actor *actor, hb3_aabb *out_aabb);

int hb3_world_set_broadphase_grid(hb3_world *w, hb3_v3 origin,
                                  hb3_fx cell_size,
                                  int cells_x, int cells_y, int cells_z);
void hb3_world_disable_broadphase(hb3_world *w);
int hb3_world_rebuild_broadphase(hb3_world *w);
int hb3_world_broadphase_ref_count(const hb3_world *w);
int hb3_world_broadphase_overflowed(const hb3_world *w);
int hb3_world_query_actor_indices(const hb3_world *w, const hb3_aabb *query,
                                  int *out_indices, int max_indices);

void hb3_debug_draw_shape(const hb3_shape *s, hb3_debug_line_fn line_fn,
                          void *user, int color, int tag);
void hb3_debug_draw_world_lines(const hb3_world *w, hb3_debug_line_fn line_fn,
                                void *user, int flags);
int hb3_debug_count_actors(const hb3_world *w);

#ifdef __cplusplus
}
#endif

#endif
