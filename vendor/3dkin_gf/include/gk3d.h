#ifndef GK3D_H
#define GK3D_H

#include "gk3d_fixed.h"

#define GK3D_ID_NONE (-1)

#define GK3D_TARGET_ANY (-1000)
#define GK3D_TARGET_SOLID (-1001)
#define GK3D_TARGET_TRIGGER (-1002)
#define GK3D_TARGET_SENSOR (-1003)
#define GK3D_TARGET_ACTIVE (-1004)
#define GK3D_TARGET_FLOOR (-1005)

#define GK3D_FLAG_ACTIVE 0x0001u
#define GK3D_FLAG_ENABLED 0x0002u
#define GK3D_FLAG_SOLID 0x0004u
#define GK3D_FLAG_TRIGGER 0x0008u
#define GK3D_FLAG_SENSOR 0x0010u
#define GK3D_FLAG_PRECISE 0x0020u
#define GK3D_FLAG_DISABLED 0x0040u
#define GK3D_FLAG_JUMPTHRU 0x0080u

#define GK3D_AXIS_X 0
#define GK3D_AXIS_Y 1
#define GK3D_AXIS_Z 2

#define GK3D_NARROWPHASE_UNHANDLED (-1)
#define GK3D_NARROWPHASE_NO 0
#define GK3D_NARROWPHASE_YES 1

#define GK3D_VERB_NONE 0
#define GK3D_VERB_PLACE_MEETING 1
#define GK3D_VERB_PLACE_FREE 2
#define GK3D_VERB_INSTANCE_PLACE 3
#define GK3D_VERB_IS_ON_FLOOR 4
#define GK3D_VERB_IS_ON_WALL 5
#define GK3D_VERB_IS_UNDER_CEILING 6
#define GK3D_VERB_OVERLAP_OFFSET 7

struct gk3d_world;

typedef struct gk3d_vec3 {
    gk3d_fix x;
    gk3d_fix y;
    gk3d_fix z;
} gk3d_vec3;

typedef struct gk3d_aabb {
    gk3d_fix x;
    gk3d_fix y;
    gk3d_fix z;
    gk3d_fix w;
    gk3d_fix h;
    gk3d_fix d;
} gk3d_aabb;

typedef struct gk3d_contact_state {
    int on_floor;
    int under_ceiling;
    int on_wall;
    int floor_id;
    int ceiling_id;
    int wall_neg_a_id;
    int wall_pos_a_id;
    int wall_neg_b_id;
    int wall_pos_b_id;
    gk3d_fix nx;
    gk3d_fix ny;
    gk3d_fix nz;
} gk3d_contact_state;

typedef struct gk3d_obj {
    int used;
    int id;
    int type;
    int family;
    int layer;
    gk3d_u32 tags;
    unsigned int flags;
    gk3d_aabb box;
    int user_i0;
    int user_i1;
    gk3d_fix user_fx0;
    gk3d_fix user_fx1;
    gk3d_contact_state contact;
} gk3d_obj;

typedef struct gk3d_filter {
    int target;
    int exact_id;
    int family;
    int layer;
    gk3d_u32 required_tags;
    unsigned int flags_all;
    unsigned int flags_any;
    unsigned int flags_none;
} gk3d_filter;

typedef struct gk3d_arena {
    gk3d_obj objects[GK3D_MAX_OBJECTS];
} gk3d_arena;

typedef struct gk3d_stats {
    gk3d_u32 queries;
    gk3d_u32 broadphase_tests;
    gk3d_u32 broadphase_hits;
    gk3d_u32 narrowphase_calls;
    gk3d_u32 aabb_fallbacks;
} gk3d_stats;

typedef int (*gk3d_narrowphase_fn)(
    const struct gk3d_world *world,
    int moving_id,
    const gk3d_aabb *moving_box,
    int other_id,
    void *user
);

typedef struct gk3d_world {
    gk3d_arena arena;
    int count;
    int up_axis;
    int up_sign;
    gk3d_fix probe_distance;
    gk3d_fix solver_step;
    gk3d_narrowphase_fn narrowphase;
    void *narrowphase_user;
    gk3d_stats stats;
} gk3d_world;

typedef struct gk3d_condition {
    int verb;
    int self_id;
    int target;
    gk3d_fix x;
    gk3d_fix y;
    gk3d_fix z;
    gk3d_fix dx;
    gk3d_fix dy;
    gk3d_fix dz;
} gk3d_condition;

typedef struct gk3d_condition_result {
    int truth;
    int instance_id;
} gk3d_condition_result;

typedef struct gk3d_solve_result {
    int collided;
    gk3d_fix allowed_dx;
    gk3d_fix allowed_dy;
    gk3d_fix allowed_dz;
    int hit_x_id;
    int hit_y_id;
    int hit_z_id;
    gk3d_fix nx;
    gk3d_fix ny;
    gk3d_fix nz;
    int steps_used;
} gk3d_solve_result;

#ifdef __cplusplus
extern "C" {
#endif

void gk3d_world_init(gk3d_world *world);
void gk3d_world_clear(gk3d_world *world);
void gk3d_world_set_up(gk3d_world *world, int axis, int positive_is_up);
void gk3d_world_set_probe(gk3d_world *world, gk3d_fix distance);
void gk3d_world_set_solver_step(gk3d_world *world, gk3d_fix step);
void gk3d_world_set_narrowphase(gk3d_world *world, gk3d_narrowphase_fn fn, void *user);
void gk3d_world_reset_stats(gk3d_world *world);

int gk3d_world_add_box(gk3d_world *world, int type,
    gk3d_fix x, gk3d_fix y, gk3d_fix z,
    gk3d_fix w, gk3d_fix h, gk3d_fix d,
    unsigned int flags);
void gk3d_world_remove(gk3d_world *world, int id);
gk3d_obj *gk3d_world_get(gk3d_world *world, int id);
const gk3d_obj *gk3d_world_get_const(const gk3d_world *world, int id);
void gk3d_obj_set_pos(gk3d_world *world, int id, gk3d_fix x, gk3d_fix y, gk3d_fix z);
void gk3d_obj_set_box(gk3d_world *world, int id,
    gk3d_fix x, gk3d_fix y, gk3d_fix z,
    gk3d_fix w, gk3d_fix h, gk3d_fix d);
void gk3d_obj_set_flag(gk3d_world *world, int id, unsigned int flag, int enabled);
void gk3d_obj_set_family(gk3d_world *world, int id, int family);
void gk3d_obj_set_layer(gk3d_world *world, int id, int layer);
void gk3d_obj_set_tags(gk3d_world *world, int id, gk3d_u32 tags);

int gk3d_aabb_valid(gk3d_aabb box);
int gk3d_aabb_overlap(gk3d_aabb a, gk3d_aabb b);
gk3d_aabb gk3d_aabb_at(gk3d_aabb box, gk3d_fix x, gk3d_fix y, gk3d_fix z);
gk3d_aabb gk3d_aabb_offset(gk3d_aabb box, gk3d_fix dx, gk3d_fix dy, gk3d_fix dz);

gk3d_filter gk3d_filter_make(int target);
int gk3d_filter_match(const gk3d_obj *obj, int self_id, const gk3d_filter *filter);

int gk3d_instance_place_box_filter(gk3d_world *world, int self_id,
    gk3d_aabb candidate, const gk3d_filter *filter);
int gk3d_place_meeting_filter(gk3d_world *world, int self_id,
    gk3d_fix x, gk3d_fix y, gk3d_fix z, const gk3d_filter *filter);
int gk3d_place_meeting(gk3d_world *world, int self_id,
    gk3d_fix x, gk3d_fix y, gk3d_fix z, int target);
int gk3d_place_free(gk3d_world *world, int self_id,
    gk3d_fix x, gk3d_fix y, gk3d_fix z);
int gk3d_instance_place(gk3d_world *world, int self_id,
    gk3d_fix x, gk3d_fix y, gk3d_fix z, int target);
int gk3d_overlap_at_offset(gk3d_world *world, int self_id,
    gk3d_fix dx, gk3d_fix dy, gk3d_fix dz, int target);
int gk3d_instance_at_offset(gk3d_world *world, int self_id,
    gk3d_fix dx, gk3d_fix dy, gk3d_fix dz, int target);

int gk3d_floor_instance(gk3d_world *world, int self_id, int target);
int gk3d_ceiling_instance(gk3d_world *world, int self_id, int target);
int gk3d_wall_instance(gk3d_world *world, int self_id, int target);
int gk3d_is_on_floor(gk3d_world *world, int self_id);
int gk3d_is_under_ceiling(gk3d_world *world, int self_id);
int gk3d_is_on_wall(gk3d_world *world, int self_id);
int gk3d_is_on_wall_axis(gk3d_world *world, int self_id, int axis, int sign, int target);
void gk3d_update_contact(gk3d_world *world, int self_id);
void gk3d_update_all_contacts(gk3d_world *world);

int gk3d_verb_from_name(const char *name);
int gk3d_check_condition(gk3d_world *world,
    const gk3d_condition *condition, gk3d_condition_result *result);
int gk3d_check_verb(gk3d_world *world, const char *verb,
    int self_id, int target,
    gk3d_fix x, gk3d_fix y, gk3d_fix z,
    gk3d_fix dx, gk3d_fix dy, gk3d_fix dz,
    gk3d_condition_result *result);

int gk3d_solve_move(gk3d_world *world, int self_id,
    gk3d_fix dx, gk3d_fix dy, gk3d_fix dz,
    int target, gk3d_solve_result *result);
int gk3d_apply_solved_move(gk3d_world *world, int self_id,
    const gk3d_solve_result *result);

/* GameMaker-style aliases. */
#define gk3d_gm_place_meeting gk3d_place_meeting
#define gk3d_gm_place_free gk3d_place_free
#define gk3d_gm_instance_place gk3d_instance_place

/* Construct-style aliases. */
#define gk3d_c3_is_overlapping_at_offset gk3d_overlap_at_offset
#define gk3d_c3_is_on_floor gk3d_is_on_floor
#define gk3d_c3_is_by_wall gk3d_is_on_wall

#ifdef __cplusplus
}
#endif

#endif
