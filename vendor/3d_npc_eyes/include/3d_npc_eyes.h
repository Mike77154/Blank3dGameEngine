#ifndef TDNE_H
#define TDNE_H

#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TDNE_VERSION_MAJOR 0
#define TDNE_VERSION_MINOR 1
#define TDNE_VERSION_PATCH 0

#ifndef TDNE_DIR_SCALE
#define TDNE_DIR_SCALE 1024L
#endif

#ifndef TDNE_SCORE_SCALE
#define TDNE_SCORE_SCALE 1000L
#endif

#ifndef TDNE_MAX_TARGET_SAMPLES
#define TDNE_MAX_TARGET_SAMPLES 5
#endif

#define TDNE_TRUE 1
#define TDNE_FALSE 0

#define TDNE_SCAN_VISIBLE_ONLY 1UL
#define TDNE_SCAN_KEEP_REJECTED 2UL
#define TDNE_SCAN_SORT_BY_SCORE 4UL

#define TDNE_MASK_ALL (~0UL)

#define TDNE_AXIS_X 0
#define TDNE_AXIS_Y 1
#define TDNE_AXIS_Z 2

typedef long tdne_i32;
typedef unsigned long tdne_u32;
typedef int tdne_bool;

typedef struct tdne_vec3 {
    tdne_i32 x;
    tdne_i32 y;
    tdne_i32 z;
} tdne_vec3;

typedef struct tdne_ray_hit {
    int hit;
    tdne_vec3 point;
    tdne_vec3 normal;
    tdne_u32 material_mask;
    void *user;
} tdne_ray_hit;

typedef enum tdne_sensor_shape {
    TDNE_SENSOR_CONE = 1,
    TDNE_SENSOR_SPHERE = 2,
    TDNE_SENSOR_BOX = 3,
    TDNE_SENSOR_FRUSTUM = 4
} tdne_sensor_shape;

typedef enum tdne_visibility {
    TDNE_VIS_NONE = 0,
    TDNE_VIS_VISIBLE = 1,
    TDNE_VIS_PARTIAL = 2,
    TDNE_VIS_OCCLUDED = 3,
    TDNE_VIS_OUT_OF_RANGE = 4,
    TDNE_VIS_OUT_OF_SHAPE = 5,
    TDNE_VIS_MASKED = 6,
    TDNE_VIS_BAD_INPUT = 7
} tdne_visibility;

typedef struct tdne_sensor {
    int shape;

    tdne_vec3 origin;
    tdne_vec3 forward;
    tdne_vec3 right;
    tdne_vec3 up;

    tdne_i32 max_distance;
    tdne_i32 near_distance;

    tdne_i32 cos_half_fov;
    tdne_i32 tan_half_h;
    tdne_i32 tan_half_v;

    tdne_vec3 box_half;

    tdne_u32 see_mask;
    tdne_u32 block_mask;

    int require_line_of_sight;
} tdne_sensor;

typedef struct tdne_target {
    tdne_vec3 origin;
    tdne_i32 radius;
    tdne_u32 mask;
    void *user;

    int sample_count;
    tdne_vec3 samples[TDNE_MAX_TARGET_SAMPLES];
} tdne_target;

typedef struct tdne_result {
    int visible;
    int target_index;
    const tdne_target *target;

    int visibility;
    int reason;

    tdne_i32 score;
    tdne_i32 distance;
    tdne_i32 distance_sq;
    tdne_i32 dot;

    int samples_total;
    int samples_in_shape;
    int rays_clear;
    int rays_blocked;

    tdne_vec3 last_seen_point;
    tdne_ray_hit hit;
} tdne_result;

typedef int (*tdne_raycast_fn)(
    void *world_user,
    const tdne_vec3 *from,
    const tdne_vec3 *to,
    tdne_u32 block_mask,
    tdne_ray_hit *out_hit
);

typedef void (*tdne_debug_line_fn)(
    void *debug_user,
    const tdne_vec3 *from,
    const tdne_vec3 *to,
    tdne_u32 tag
);

tdne_vec3 tdne_vec3_make(tdne_i32 x, tdne_i32 y, tdne_i32 z);
tdne_vec3 tdne_vec3_zero(void);
tdne_vec3 tdne_vec3_add(tdne_vec3 a, tdne_vec3 b);
tdne_vec3 tdne_vec3_sub(tdne_vec3 a, tdne_vec3 b);
tdne_vec3 tdne_vec3_neg(tdne_vec3 a);

tdne_i32 tdne_abs_i32(tdne_i32 v);
tdne_i32 tdne_add_sat_i32(tdne_i32 a, tdne_i32 b);
tdne_i32 tdne_sub_sat_i32(tdne_i32 a, tdne_i32 b);
tdne_i32 tdne_mul_sat_i32(tdne_i32 a, tdne_i32 b);
tdne_i32 tdne_vec3_len_sq_sat(tdne_vec3 v);
tdne_i32 tdne_vec3_distance_sq_sat(tdne_vec3 a, tdne_vec3 b);
tdne_i32 tdne_i32_sqrt(tdne_i32 v);

int tdne_vec3_normalize_dir(tdne_vec3 v, tdne_vec3 *out_dir);
tdne_i32 tdne_dir_dot(tdne_vec3 a, tdne_vec3 b);
tdne_i32 tdne_cos_deg(int degrees);
tdne_i32 tdne_tan_deg(int degrees);

void tdne_sensor_init_defaults(tdne_sensor *sensor);
void tdne_sensor_init_cone(
    tdne_sensor *sensor,
    tdne_vec3 origin,
    tdne_vec3 forward,
    tdne_i32 max_distance,
    int fov_degrees
);
void tdne_sensor_init_sphere(
    tdne_sensor *sensor,
    tdne_vec3 origin,
    tdne_i32 max_distance
);
void tdne_sensor_init_box(
    tdne_sensor *sensor,
    tdne_vec3 origin,
    tdne_vec3 half_extents
);
void tdne_sensor_init_frustum(
    tdne_sensor *sensor,
    tdne_vec3 origin,
    tdne_vec3 forward,
    tdne_vec3 right,
    tdne_vec3 up,
    tdne_i32 near_distance,
    tdne_i32 max_distance,
    int horizontal_fov_degrees,
    int vertical_fov_degrees
);
void tdne_sensor_set_origin(tdne_sensor *sensor, tdne_vec3 origin);
int tdne_sensor_set_forward(tdne_sensor *sensor, tdne_vec3 forward);
int tdne_sensor_set_axes(
    tdne_sensor *sensor,
    tdne_vec3 forward,
    tdne_vec3 right,
    tdne_vec3 up
);
void tdne_sensor_set_masks(tdne_sensor *sensor, tdne_u32 see_mask, tdne_u32 block_mask);
void tdne_sensor_set_line_of_sight(tdne_sensor *sensor, int required);
void tdne_sensor_set_cone_degrees(tdne_sensor *sensor, int fov_degrees);
void tdne_sensor_set_frustum_degrees(
    tdne_sensor *sensor,
    int horizontal_fov_degrees,
    int vertical_fov_degrees
);

void tdne_target_init(
    tdne_target *target,
    tdne_vec3 origin,
    tdne_i32 radius,
    tdne_u32 mask,
    void *user
);
void tdne_target_clear_samples(tdne_target *target);
int tdne_target_add_sample(tdne_target *target, tdne_vec3 local_offset);
int tdne_target_use_center(tdne_target *target);
int tdne_target_use_vertical3(tdne_target *target, tdne_i32 half_height, int axis);

int tdne_point_test_sensor(
    const tdne_sensor *sensor,
    tdne_vec3 point,
    tdne_result *out_probe
);

int tdne_eval_target(
    const tdne_sensor *sensor,
    const tdne_target *target,
    tdne_raycast_fn raycast,
    void *world_user,
    tdne_result *out_result
);

int tdne_scan_targets(
    const tdne_sensor *sensor,
    const tdne_target *targets,
    int target_count,
    tdne_result *results,
    int result_max,
    tdne_raycast_fn raycast,
    void *world_user,
    tdne_u32 flags
);

const char *tdne_visibility_name(int visibility);

#ifdef __cplusplus
}
#endif

#endif
