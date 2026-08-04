#ifndef TRAIL3D89_H
#define TRAIL3D89_H

/*
   trail3d89 - generic temporal 3D trail mesh library
   C89, fixed point, no heap, no malloc/free/realloc.

   Coordinate format: signed Q8.8 by default.
   1 world unit = T3D89_FP_ONE.
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef T3D89_MAX_TRAILS
#define T3D89_MAX_TRAILS 16
#endif

#ifndef T3D89_MAX_POINTS_PER_TRAIL
#define T3D89_MAX_POINTS_PER_TRAIL 128
#endif

#define T3D89_FP_SHIFT 8
#define T3D89_FP_ONE   (1 << T3D89_FP_SHIFT)
#define T3D89_VERSION_MAJOR 2
#define T3D89_VERSION_MINOR 1
#define T3D89_VERSION_PATCH 0

#define T3D89_OK 0
#define T3D89_ERR_NULL -1
#define T3D89_ERR_BAD_ID -2
#define T3D89_ERR_FULL -3
#define T3D89_ERR_INACTIVE -4
#define T3D89_ERR_BAD_DESC -5
#define T3D89_ERR_OVERFLOW -6
#define T3D89_ERR_EMPTY -7
#define T3D89_ERR_PROVIDER -8

#define T3D89_SAMPLE_DISTANCE 1u
#define T3D89_SAMPLE_TIME     2u
#define T3D89_SAMPLE_CURVE    4u
#define T3D89_SAMPLE_FORCE    8u

#define T3D89_EMIT_FORCE      1u
#define T3D89_EMIT_HAS_ORIENT 2u
#define T3D89_EMIT_HAS_AB     4u
#define T3D89_EMIT_HAS_COLOR  8u
#define T3D89_EMIT_HAS_WIDTH  16u

#define T3D89_BUILD_APPEND    1u
#define T3D89_BUILD_CLEAR     0u

#define T3D89_UV_BY_AGE       0
#define T3D89_UV_BY_DISTANCE  1

#define T3D89_SAMPLER_ANY     0
#define T3D89_SAMPLER_ALL     1

#define T3D89_PROVIDER_SKIP  0
#define T3D89_PROVIDER_READY 1

#define T3D89_PROVIDER_AUTOSTEP    1u
#define T3D89_PROVIDER_SCALE_WIDTH 2u

typedef enum t3d89_mode_e {
    T3D89_MODE_VIEW_RIBBON = 0,
    T3D89_MODE_AXIS_RIBBON = 1,
    T3D89_MODE_ORIENTED_RIBBON = 2,
    T3D89_MODE_CROSS_RIBBON = 3,
    T3D89_MODE_TUBE_LITE = 4,
    T3D89_MODE_BEAM_AB = 5,
    T3D89_MODE_SOCKET_SWEEP = 6
} t3d89_mode;

typedef struct t3d89_color_s {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} t3d89_color;

typedef struct t3d89_vec3_s {
    int x;
    int y;
    int z;
} t3d89_vec3;

typedef struct t3d89_camera_s {
    int x;
    int y;
    int z;
    int up_x;
    int up_y;
    int up_z;
} t3d89_camera;

/*
   Fixed-point affine transform.

   tx/ty/tz: world translation in Q8.8.
   m00..m22: row-major rotation matrix in Q8.8.
   sx/sy/sz: local scale in Q8.8.

   world = translation + rotation * (local * scale)
*/
typedef struct t3d89_transform_s {
    int tx;
    int ty;
    int tz;

    int m00;
    int m01;
    int m02;
    int m10;
    int m11;
    int m12;
    int m20;
    int m21;
    int m22;

    int sx;
    int sy;
    int sz;
} t3d89_transform;

typedef struct t3d89_sample_s t3d89_sample;

struct t3d89_sample_s {
    int x;
    int y;
    int z;

    int vx;
    int vy;
    int vz;

    int right_x;
    int right_y;
    int right_z;

    int up_x;
    int up_y;
    int up_z;

    int a_x;
    int a_y;
    int a_z;
    int b_x;
    int b_y;
    int b_z;

    int width;
    t3d89_color color;
    unsigned int flags;
};

typedef int (*t3d89_transform_provider_fn)(
    void *user,
    int trail_id,
    int tick,
    t3d89_transform *out_transform
);

typedef struct t3d89_vertex_s {
    int x;
    int y;
    int z;
    int u;
    int v;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} t3d89_vertex;

typedef struct t3d89_mesh_s {
    t3d89_vertex *vertices;
    unsigned short *indices;
    int max_vertices;
    int max_indices;
    int vertex_count;
    int index_count;
    unsigned int flags;
    int overflowed;
} t3d89_mesh;

typedef struct t3d89_desc_s {
    int mode;
    int max_points;
    int life_ticks;

    unsigned int sampler_mask;
    int sampler_mode;
    int min_ticks;
    int min_dist;
    int curve_dist;

    int width_head;
    int width_tail;
    t3d89_color color_head;
    t3d89_color color_tail;

    int axis_x;
    int axis_y;
    int axis_z;

    int uv_mode;
    int uv_tile_dist;

    int tube_sides;
    int lod_vertex_budget;
    int lod_min_stride;
    int lod_max_stride;
} t3d89_desc;

typedef struct t3d89_point_s {
    int x;
    int y;
    int z;
    int a_x;
    int a_y;
    int a_z;
    int b_x;
    int b_y;
    int b_z;
    int right_x;
    int right_y;
    int right_z;
    int up_x;
    int up_y;
    int up_z;
    int width;
    int uv_dist;
    int age;
    t3d89_color color;
    unsigned int flags;
} t3d89_point;

typedef struct t3d89_trail_s {
    int active;
    int enabled;
    t3d89_desc desc;
    int start;
    int count;
    int last_emit_tick;
    int have_last;
    int last_x;
    int last_y;
    int last_z;
    int prev_x;
    int prev_y;
    int prev_z;
    int total_uv_dist;
    int dropped_points;
    int rejected_samples;

    int provider_enabled;
    unsigned int provider_flags;
    t3d89_transform_provider_fn provider_fn;
    void *provider_user;
    t3d89_sample provider_local_sample;
    int provider_calls;
    int provider_skips;
    int provider_errors;
    int provider_last_result;
} t3d89_trail;

typedef struct t3d89_ctx_s {
    int tick;
    int active_trails;
    t3d89_trail trails[T3D89_MAX_TRAILS];
    t3d89_point points[T3D89_MAX_TRAILS][T3D89_MAX_POINTS_PER_TRAIL];
} t3d89_ctx;

void t3d89_init(t3d89_ctx *ctx);
void t3d89_default_desc(t3d89_desc *desc);
t3d89_color t3d89_color_make(int r, int g, int b, int a);
int t3d89_create(t3d89_ctx *ctx, const t3d89_desc *desc);
int t3d89_destroy(t3d89_ctx *ctx, int trail_id);
int t3d89_reset(t3d89_ctx *ctx, int trail_id);
int t3d89_set_enabled(t3d89_ctx *ctx, int trail_id, int enabled);

void t3d89_transform_identity(t3d89_transform *transform);
int t3d89_transform_sample(const t3d89_transform *transform, const t3d89_sample *local_sample, t3d89_sample *world_sample, unsigned int provider_flags);
int t3d89_bind_transform_provider(t3d89_ctx *ctx, int trail_id, t3d89_transform_provider_fn provider_fn, void *provider_user, const t3d89_sample *local_sample, unsigned int provider_flags);
int t3d89_unbind_transform_provider(t3d89_ctx *ctx, int trail_id);
int t3d89_set_provider_enabled(t3d89_ctx *ctx, int trail_id, int enabled);
int t3d89_set_provider_local_sample(t3d89_ctx *ctx, int trail_id, const t3d89_sample *local_sample);
int t3d89_step_provider(t3d89_ctx *ctx, int trail_id);
int t3d89_update_providers(t3d89_ctx *ctx);
int t3d89_get_provider_last_result(const t3d89_ctx *ctx, int trail_id);
int t3d89_get_provider_call_count(const t3d89_ctx *ctx, int trail_id);
int t3d89_get_provider_skip_count(const t3d89_ctx *ctx, int trail_id);
int t3d89_get_provider_error_count(const t3d89_ctx *ctx, int trail_id);

int t3d89_tick(t3d89_ctx *ctx, int dt_ticks);
int t3d89_emit_sample(t3d89_ctx *ctx, int trail_id, const t3d89_sample *sample);
int t3d89_emit_point(t3d89_ctx *ctx, int trail_id, int x, int y, int z);
int t3d89_emit_oriented(t3d89_ctx *ctx, int trail_id, int x, int y, int z, int right_x, int right_y, int right_z, int up_x, int up_y, int up_z);
int t3d89_emit_segment(t3d89_ctx *ctx, int trail_id, int ax, int ay, int az, int bx, int by, int bz);
int t3d89_build_mesh(t3d89_ctx *ctx, int trail_id, const t3d89_camera *camera, t3d89_mesh *mesh);
int t3d89_build_all(t3d89_ctx *ctx, const t3d89_camera *camera, t3d89_mesh *mesh);
int t3d89_get_point_count(const t3d89_ctx *ctx, int trail_id);
int t3d89_get_dropped_count(const t3d89_ctx *ctx, int trail_id);
int t3d89_get_rejected_count(const t3d89_ctx *ctx, int trail_id);
void t3d89_sample_clear(t3d89_sample *sample);
void t3d89_mesh_clear(t3d89_mesh *mesh);
int t3d89_fp_from_int(int v);
int t3d89_fp_to_int(int v);

#ifdef __cplusplus
}
#endif

#endif
