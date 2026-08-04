#ifndef SICOL_WORLD_H
#define SICOL_WORLD_H

/* ============================================================
 * SICOL - Static collision world
 * ============================================================ */

#include "sicol_types.h"
#include "sicol_shape.h"
#include "sicol_broadphase.h"
#include "sicol_narrowphase.h"
#include "sicol_raycast.h"
#include "sicol_gjk.h"

#define SICOL_WORLD_MAX 512
#define SICOL_WORLD_MAX_MANIFOLDS 1024
#define SICOL_WORLD_MAX_JOINTS 256
#define SICOL_MANIFOLD_MAX_POINTS 4

typedef struct {
    int active;
    int next_free;
    sicol_shape_t shape;
    uint32_t layer;
    uint32_t mask;
} sicol_world_slot_t;

typedef struct {
    int active;
    fx point[3];
    fx normal[3];
    fx penetration;
    int feature_a;
    int feature_b;
    int age;
    fx tangent[3];
    fx normal_impulse;
    fx tangent_impulse;
} sicol_manifold_point_t;

typedef struct {
    int active;
    int id_a;
    int id_b;
    int point_count;
    fx normal[3];
    fx centroid[3];
    fx max_penetration;
    unsigned int generation;
    sicol_gjk_cache_t gjk_cache;
    sicol_manifold_point_t points[SICOL_MANIFOLD_MAX_POINTS];
} sicol_persistent_manifold_t;

typedef enum {
    SICOL_JOINT_NONE = 0,
    SICOL_JOINT_DISTANCE,
    SICOL_JOINT_POINT,
    SICOL_JOINT_FIXED,
    SICOL_JOINT_HINGE,
    SICOL_JOINT_SPRING,
    SICOL_JOINT_SLIDER,
    SICOL_JOINT_CONE_TWIST
} sicol_joint_type_t;

typedef struct {
    int active;
    int next_free;
    sicol_joint_type_t type;
    int id_a;
    int id_b; /* -1 => static world anchor/frame stored in local_*_b */
    fx local_anchor_a[3];
    fx local_anchor_b[3];
    fx local_axis_a[3];
    fx local_axis_b[3];
    fx local_ref_a[3];
    fx local_ref_b[3];
    fx rest_length;
    fx stiffness;
    fx damping;
    fx accumulated_impulse[3];
    fx angular_accumulated_impulse[3];
    fx motor_accumulated_impulse;
    fx last_impulse;
    int limit_enabled;
    fx lower_angle;
    fx upper_angle;
    fx lower_limit;
    fx upper_limit;
    fx cone_angle;
    int motor_enabled;
    fx motor_speed;
    fx max_motor_impulse;
} sicol_joint_t;

typedef struct {
    sicol_world_slot_t slots[SICOL_WORLD_MAX];
    int free_head;
    int active_count;
    sicol_persistent_manifold_t manifolds[SICOL_WORLD_MAX_MANIFOLDS];
    unsigned int manifold_generation;
    sicol_joint_t joints[SICOL_WORLD_MAX_JOINTS];
    int joint_free_head;
    int joint_active_count;
} sicol_world_t;

typedef struct {
    int id;
    sicol_contact_t contact;
} sicol_world_overlap_t;

typedef struct {
    int id;
    sicol_hit_t hit;
} sicol_world_raycast_hit_t;

typedef struct {
    int id;
    fx fraction;      /* 0..FX_ONE over the supplied delta */
    fx position[3];   /* safe position just before impact */
    sicol_contact_t contact;
} sicol_world_cast_hit_t;

void sicol_world_init(sicol_world_t* world);
int sicol_world_create(sicol_world_t* world, const sicol_shape_t* shape, uint32_t layer, uint32_t mask);
int sicol_world_is_alive(const sicol_world_t* world, int id);
void sicol_world_kill(sicol_world_t* world, int id);

int sicol_world_set_shape(sicol_world_t* world, int id, const sicol_shape_t* shape);
int sicol_world_get_shape(const sicol_world_t* world, int id, sicol_shape_t* out_shape);
int sicol_world_set_position(sicol_world_t* world, int id, const fx pos[3]);
int sicol_world_set_layer_mask(sicol_world_t* world, int id, uint32_t layer, uint32_t mask);

int sicol_world_overlap_pair(const sicol_world_t* world, int id_a, int id_b, sicol_contact_t* out);
int sicol_world_overlap_shape(
    const sicol_world_t* world,
    const sicol_shape_t* shape,
    uint32_t query_mask,
    sicol_world_overlap_t* out_hits,
    int max_hits
);

int sicol_world_raycast(
    const sicol_world_t* world,
    const sicol_shape_t* ray,
    uint32_t query_mask,
    sicol_world_raycast_hit_t* out_hit
);

int sicol_world_raycast_all(
    const sicol_world_t* world,
    const sicol_shape_t* ray,
    uint32_t query_mask,
    sicol_world_raycast_hit_t* out_hits,
    int max_hits
);

int sicol_world_cast_shape(
    const sicol_world_t* world,
    const sicol_shape_t* moving_shape,
    const fx delta[3],
    uint32_t query_mask,
    sicol_world_cast_hit_t* out_hit
);

int sicol_world_cast_shape_ex(
    const sicol_world_t* world,
    const sicol_shape_t* moving_shape,
    const fx delta[3],
    uint32_t query_mask,
    uint32_t moving_layer,
    uint32_t moving_mask,
    int exclude_id,
    sicol_world_cast_hit_t* out_hit
);

int sicol_world_depenetrate_shape(
    const sicol_world_t* world,
    sicol_shape_t* io_shape,
    uint32_t query_mask,
    int max_iterations,
    sicol_contact_t* out_last_contact
);

void sicol_world_clear_joints(sicol_world_t* world);
int sicol_world_joint_create_distance(
    sicol_world_t* world,
    int id_a,
    const fx local_anchor_a[3],
    int id_b,
    const fx anchor_b[3],
    fx rest_length,
    fx stiffness,
    fx damping
);
int sicol_world_joint_create_spring(
    sicol_world_t* world,
    int id_a,
    const fx local_anchor_a[3],
    int id_b,
    const fx anchor_b[3],
    fx rest_length,
    fx stiffness,
    fx damping
);
int sicol_world_joint_create_point(
    sicol_world_t* world,
    int id_a,
    const fx local_anchor_a[3],
    int id_b,
    const fx anchor_b[3],
    fx stiffness,
    fx damping
);
int sicol_world_joint_create_fixed(
    sicol_world_t* world,
    int id_a,
    const fx local_anchor_a[3],
    int id_b,
    const fx anchor_b[3],
    fx stiffness,
    fx damping
);
int sicol_world_joint_create_hinge(
    sicol_world_t* world,
    int id_a,
    const fx local_anchor_a[3],
    const fx local_axis_a[3],
    int id_b,
    const fx anchor_b[3],
    const fx axis_b_or_world[3],
    fx stiffness,
    fx damping
);
int sicol_world_joint_create_slider(
    sicol_world_t* world,
    int id_a,
    const fx local_anchor_a[3],
    const fx local_axis_a[3],
    int id_b,
    const fx anchor_b[3],
    const fx axis_b_or_world[3],
    fx stiffness,
    fx damping
);
int sicol_world_joint_create_cone_twist(
    sicol_world_t* world,
    int id_a,
    const fx local_anchor_a[3],
    const fx local_axis_a[3],
    int id_b,
    const fx anchor_b[3],
    const fx axis_b_or_world[3],
    fx cone_angle,
    fx stiffness,
    fx damping
);
int sicol_world_joint_set_hinge_limits(
    sicol_world_t* world,
    int joint_id,
    fx lower_angle,
    fx upper_angle
);
int sicol_world_joint_set_hinge_motor(
    sicol_world_t* world,
    int joint_id,
    fx motor_speed,
    fx max_motor_impulse
);
int sicol_world_joint_set_slider_limits(
    sicol_world_t* world,
    int joint_id,
    fx lower_distance,
    fx upper_distance
);
int sicol_world_joint_set_slider_motor(
    sicol_world_t* world,
    int joint_id,
    fx motor_speed,
    fx max_motor_impulse
);
int sicol_world_joint_set_cone_twist_limits(
    sicol_world_t* world,
    int joint_id,
    fx cone_angle,
    fx twist_lower,
    fx twist_upper
);
int sicol_world_joint_is_alive(const sicol_world_t* world, int joint_id);
void sicol_world_joint_kill(sicol_world_t* world, int joint_id);
int sicol_world_joint_get(const sicol_world_t* world, int joint_id, sicol_joint_t* out_joint);

void sicol_world_clear_manifolds(sicol_world_t* world);
int sicol_world_get_manifold(
    const sicol_world_t* world,
    int id_a,
    int id_b,
    sicol_persistent_manifold_t* out_manifold
);

int sicol_world_collide_persistent(
    sicol_world_t* world,
    sicol_pair_t* out_pairs,
    sicol_contact_t* out_contacts,
    int max_results
);

int sicol_world_collide(
    sicol_world_t* world,
    sicol_pair_t* out_pairs,
    sicol_contact_t* out_contacts,
    int max_results
);

#endif /* SICOL_WORLD_H */
