/*
    cameranaku89.h
    CC0 1.0 Universal.

    Cameranaku89 is a small C89, fixed-point, no-heap camera solver.
    It is renderer-agnostic, entity-system-agnostic, and profile-bank agnostic.
*/
#ifndef CAMERANAKU89_H
#define CAMERANAKU89_H

#include "cameranaku89_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CNK_VERSION_MAJOR 3
#define CNK_VERSION_MINOR 4
#define CNK_VERSION_PATCH 0

#define CNK_ONE ((cnk_fx)1 << CNK_FX_SHIFT)
#define CNK_HALF (CNK_ONE >> 1)
#define CNK_FX_FROM_INT(x) ((cnk_fx)(x) << CNK_FX_SHIFT)
#define CNK_FX_TO_INT(x) ((int)((x) >> CNK_FX_SHIFT))
#define CNK_FX_FRAC(num, den) ((cnk_fx)(((num) << CNK_FX_SHIFT) / (den)))
#define CNK_DEG(x) CNK_FX_FROM_INT(x)
#define CNK_TRUE 1
#define CNK_FALSE 0

typedef long cnk_fx;
typedef unsigned long cnk_u32;

typedef struct cnk_vec2_s {
    cnk_fx x;
    cnk_fx y;
} cnk_vec2;

typedef struct cnk_vec3_s {
    cnk_fx x;
    cnk_fx y;
    cnk_fx z;
} cnk_vec3;

/*
    Optional receive-provider bridge for 3D transform math.

    Callback contracts:
      move   : out = position + delta
      scale  : out = value scaled component-wise by scale
      rotate : out = value rotated by Euler degrees (yaw, pitch, roll)

    Callbacks receive and return Cameranaku89 fixed-point values. Return non-zero
    on success. Missing or failed callbacks automatically fall back to the
    built-in C89 fixed-point implementation.
*/
typedef int (*cnk_transform_move_fn)(
    void *user,
    cnk_vec3 position,
    cnk_vec3 delta,
    cnk_vec3 *out_position
);

typedef int (*cnk_transform_scale_fn)(
    void *user,
    cnk_vec3 value,
    cnk_vec3 scale,
    cnk_vec3 *out_value
);

typedef int (*cnk_transform_rotate_fn)(
    void *user,
    cnk_vec3 value,
    cnk_vec3 euler_deg,
    cnk_vec3 *out_value
);

typedef struct cnk_transform_provider_s {
    void *user;
    cnk_transform_move_fn move;
    cnk_transform_scale_fn scale;
    cnk_transform_rotate_fn rotate;
    int mode;
} cnk_transform_provider;

typedef struct cnk_mat4_s {
    cnk_fx m[16];
} cnk_mat4;

typedef struct cnk_lens_s {
    int projection;
    int viewport_w;
    int viewport_h;
    cnk_fx fov_y_deg;
    cnk_fx near_clip;
    cnk_fx far_clip;
    cnk_fx ortho_height;
    cnk_fx aspect;
    int fov_axis;
    int aspect_policy;
    cnk_fx frustum_offset_x;
    cnk_fx frustum_offset_y;
    cnk_fx viewport_offset_x;
    cnk_fx viewport_offset_y;
    cnk_u32 cull_mask;
    cnk_u32 effect_mask;
} cnk_lens;

typedef struct cnk_pose_s {
    cnk_vec3 pos;
    cnk_fx yaw_deg;
    cnk_fx pitch_deg;
    cnk_fx roll_deg;
} cnk_pose;

typedef struct cnk_basis_s {
    cnk_vec3 forward;
    cnk_vec3 right;
    cnk_vec3 up;
} cnk_basis;

typedef struct cnk_target_s {
    cnk_vec3 pos;
    cnk_vec3 velocity;
    cnk_fx yaw_deg;
    cnk_fx pitch_deg;
    cnk_fx roll_deg;
    int valid;
} cnk_target;

typedef struct cnk_keyframe_s {
    int time_ticks;
    cnk_pose pose;
    cnk_fx fov_y_deg;
    cnk_vec3 look_at;
    int flags;
} cnk_keyframe;

typedef struct cnk_track_s {
    cnk_keyframe keys[CNK_MAX_KEYFRAMES];
    int count;
    int time_ticks;
    int duration_ticks;
    int loop;
} cnk_track;

typedef struct cnk_shake_s {
    int active;
    int time_ticks;
    int duration_ticks;
    cnk_fx pos_amp;
    cnk_fx rot_amp_deg;
    cnk_fx fov_amp_deg;
    int frequency_hz;
    cnk_u32 seed;
    int play_space;
    cnk_fx user_yaw_deg;
} cnk_shake;

typedef struct cnk_profile_s {
    int mode;
    int style;
    cnk_lens lens;
    cnk_vec3 pivot_offset;
    cnk_vec3 camera_offset;
    cnk_vec3 look_offset;
    cnk_fx distance;
    cnk_fx min_distance;
    cnk_fx max_distance;
    cnk_fx height;
    cnk_fx shoulder_x;
    cnk_fx dead_zone_x;
    cnk_fx dead_zone_y;
    cnk_fx soft_zone_x;
    cnk_fx soft_zone_y;
    cnk_fx pos_lag;
    cnk_fx rot_lag;
    cnk_fx fov_lag;
    cnk_fx min_pitch_deg;
    cnk_fx max_pitch_deg;
    cnk_fx collision_radius;
    int flags;
} cnk_profile;

typedef struct cnk_rail_s {
    cnk_vec3 points[CNK_MAX_KEYFRAMES];
    int count;
    int cursor;
    cnk_fx t;
    int closed;
} cnk_rail;

typedef struct cnk_state_s {
    cnk_pose pose;
    cnk_lens lens;
    cnk_basis basis;
    cnk_vec3 look_at;
    int valid;
} cnk_state;

/* Return 1 if the probe changed out_pos to a safe camera position. */
typedef int (*cnk_world_probe_fn)(
    void *user,
    cnk_vec3 from,
    cnk_vec3 to,
    cnk_fx radius,
    cnk_vec3 *out_pos
);

typedef struct cnk_camera_s {
    cnk_profile profile;
    cnk_target target;
    cnk_state state;
    cnk_state previous_state;
    cnk_track track;
    cnk_rail rail;
    cnk_shake shakes[CNK_MAX_SHAKES];
    cnk_state blend_from;
    cnk_state blend_to;
    int blend_active;
    int blend_time_ticks;
    int blend_duration_ticks;
    int blend_curve;
    cnk_fx input_yaw_delta;
    cnk_fx input_pitch_delta;
    cnk_fx input_zoom_delta;
    cnk_fx input_fov_delta;
    cnk_u32 frame_counter;
    cnk_transform_provider transform_provider;
} cnk_camera;

typedef struct cnk_arena_s {
    cnk_camera cameras[CNK_MAX_VCAMS];
    int used[CNK_MAX_VCAMS];
} cnk_arena;

enum {
    CNK_PROJ_PERSPECTIVE = 1,
    CNK_PROJ_ORTHOGRAPHIC = 2,
    CNK_PROJ_FRUSTUM = 3
};

enum {
    CNK_FOV_VERTICAL = 0,
    CNK_FOV_HORIZONTAL = 1
};

enum {
    CNK_KEEP_HEIGHT = 0,
    CNK_KEEP_WIDTH = 1
};

enum {
    CNK_MODE_FREE = 0,
    CNK_MODE_FPS = 1,
    CNK_MODE_FOLLOW = 2,
    CNK_MODE_FIXED = 3,
    CNK_MODE_ORBIT = 4,
    CNK_MODE_RAIL = 5,
    CNK_MODE_CUTSCENE = 6
};

enum {
    CNK_STYLE_CUSTOM = 0,
    CNK_STYLE_FPS = 1,
    CNK_STYLE_OTS = 2,
    CNK_STYLE_THIRD_PERSON = 3,
    CNK_STYLE_FIXED = 4,
    CNK_STYLE_ORBIT = 5,
    CNK_STYLE_RAIL = 6,
    CNK_STYLE_CUTSCENE = 7
};

enum {
    CNK_SHAKE_CAMERA_LOCAL = 0,
    CNK_SHAKE_WORLD = 1,
    CNK_SHAKE_TARGET_LOCAL = 2,
    CNK_SHAKE_USER_ROTATED = 3
};

enum {
    CNK_BLEND_CUT = 0,
    CNK_BLEND_LINEAR = 1,
    CNK_BLEND_EASE_IN_OUT = 2
};

enum {
    CNK_KEY_LOOK_AT = 1,
    CNK_KEY_USE_ROTATION = 2
};

enum {
    CNK_FLAG_LOCK_ROLL = 1,
    CNK_FLAG_USE_TARGET_YAW = 2,
    CNK_FLAG_KEEP_LINE_OF_SIGHT = 4,
    CNK_FLAG_CLAMP_PITCH = 8,
    CNK_FLAG_SNAP_ON_TARGET_WARP = 16
};

enum {
    CNK_TRANSFORM_MODE_INTERNAL = 0,
    CNK_TRANSFORM_MODE_RECEIVE_PROVIDER = 1
};

CNK_API cnk_fx cnk_fx_mul(cnk_fx a, cnk_fx b);
CNK_API cnk_fx cnk_fx_div(cnk_fx a, cnk_fx b);
CNK_API cnk_fx cnk_fx_abs(cnk_fx a);
CNK_API cnk_fx cnk_fx_clamp(cnk_fx v, cnk_fx lo, cnk_fx hi);
CNK_API cnk_fx cnk_fx_lerp(cnk_fx a, cnk_fx b, cnk_fx t);
CNK_API cnk_fx cnk_sin_deg(cnk_fx deg);
CNK_API cnk_fx cnk_cos_deg(cnk_fx deg);

CNK_API cnk_vec2 cnk_vec2_make(cnk_fx x, cnk_fx y);
CNK_API cnk_vec3 cnk_vec3_make(cnk_fx x, cnk_fx y, cnk_fx z);
CNK_API cnk_vec3 cnk_vec3_add(cnk_vec3 a, cnk_vec3 b);
CNK_API cnk_vec3 cnk_vec3_sub(cnk_vec3 a, cnk_vec3 b);
CNK_API cnk_vec3 cnk_vec3_scale(cnk_vec3 a, cnk_fx s);
CNK_API cnk_vec3 cnk_vec3_lerp(cnk_vec3 a, cnk_vec3 b, cnk_fx t);
CNK_API cnk_fx cnk_vec3_dot(cnk_vec3 a, cnk_vec3 b);
CNK_API cnk_vec3 cnk_vec3_cross(cnk_vec3 a, cnk_vec3 b);
CNK_API cnk_vec3 cnk_vec3_normalize_fast(cnk_vec3 a);

CNK_API void cnk_transform_provider_clear(cnk_transform_provider *provider);
CNK_API void cnk_transform_provider_set(cnk_transform_provider *provider, void *user, cnk_transform_move_fn move_fn, cnk_transform_scale_fn scale_fn, cnk_transform_rotate_fn rotate_fn);
CNK_API void cnk_transform_provider_set_mode(cnk_transform_provider *provider, int mode);
CNK_API int cnk_transform_provider_is_active(const cnk_transform_provider *provider);
CNK_API cnk_vec3 cnk_transform_move(const cnk_transform_provider *provider, cnk_vec3 position, cnk_vec3 delta);
CNK_API cnk_vec3 cnk_transform_scale(const cnk_transform_provider *provider, cnk_vec3 value, cnk_vec3 scale);
CNK_API cnk_vec3 cnk_transform_scale_uniform(const cnk_transform_provider *provider, cnk_vec3 value, cnk_fx scale);
CNK_API cnk_vec3 cnk_transform_rotate(const cnk_transform_provider *provider, cnk_vec3 value, cnk_vec3 euler_deg);
CNK_API cnk_vec3 cnk_transform_apply_trs(const cnk_transform_provider *provider, cnk_vec3 point, cnk_vec3 scale, cnk_vec3 euler_deg, cnk_vec3 translation);
CNK_API cnk_basis cnk_basis_from_angles_provider(const cnk_transform_provider *provider, cnk_fx yaw_deg, cnk_fx pitch_deg, cnk_fx roll_deg);

CNK_API void cnk_lens_perspective(cnk_lens *lens, int w, int h, cnk_fx fov_y_deg, cnk_fx near_clip, cnk_fx far_clip);
CNK_API void cnk_lens_orthographic(cnk_lens *lens, int w, int h, cnk_fx ortho_height, cnk_fx near_clip, cnk_fx far_clip);
CNK_API int cnk_lens_validate(cnk_lens *lens);

/* Core profile helpers. Concrete profile styles live in cameranaku89_profiles.* */
CNK_API void cnk_profile_default(cnk_profile *profile);
CNK_API int cnk_profile_validate(cnk_profile *profile);
CNK_API void cnk_profile_set_mode(cnk_profile *profile, int mode);
CNK_API void cnk_profile_set_style(cnk_profile *profile, int style);
CNK_API void cnk_profile_set_lens(cnk_profile *profile, const cnk_lens *lens);
CNK_API void cnk_profile_set_follow_layout(cnk_profile *profile, cnk_vec3 pivot_offset, cnk_vec3 camera_offset, cnk_vec3 look_offset, cnk_fx distance, cnk_fx height, cnk_fx shoulder_x);
CNK_API void cnk_profile_set_distance_limits(cnk_profile *profile, cnk_fx min_distance, cnk_fx max_distance);
CNK_API void cnk_profile_set_lag(cnk_profile *profile, cnk_fx pos_lag, cnk_fx rot_lag, cnk_fx fov_lag);
CNK_API void cnk_profile_set_pitch_limits(cnk_profile *profile, cnk_fx min_pitch_deg, cnk_fx max_pitch_deg);
CNK_API void cnk_profile_set_collision(cnk_profile *profile, cnk_fx radius, int keep_line_of_sight);

CNK_API void cnk_camera_reset(cnk_camera *cam);
CNK_API int cnk_camera_validate(cnk_camera *cam);
CNK_API void cnk_camera_apply_profile(cnk_camera *cam, const cnk_profile *profile);
CNK_API void cnk_camera_init_perspective(cnk_camera *cam, int viewport_w, int viewport_h, cnk_fx fov_y_deg, cnk_fx near_clip, cnk_fx far_clip);
CNK_API void cnk_camera_init_orthographic(cnk_camera *cam, int viewport_w, int viewport_h, cnk_fx ortho_height, cnk_fx near_clip, cnk_fx far_clip);
CNK_API void cnk_camera_init_orbit(cnk_camera *cam, int viewport_w, int viewport_h, cnk_fx distance, cnk_fx yaw_deg, cnk_fx pitch_deg, cnk_fx near_clip, cnk_fx far_clip);
CNK_API void cnk_camera_set_target(cnk_camera *cam, cnk_vec3 pos, cnk_vec3 velocity, cnk_fx yaw_deg, cnk_fx pitch_deg, cnk_fx roll_deg);
CNK_API void cnk_camera_set_transform_provider(cnk_camera *cam, const cnk_transform_provider *provider);
CNK_API void cnk_camera_clear_transform_provider(cnk_camera *cam);
CNK_API const cnk_transform_provider *cnk_camera_get_transform_provider(const cnk_camera *cam);
CNK_API void cnk_camera_set_pose(cnk_camera *cam, cnk_pose pose);
CNK_API void cnk_camera_set_viewport(cnk_camera *cam, int w, int h);
CNK_API void cnk_camera_set_clip(cnk_camera *cam, cnk_fx near_clip, cnk_fx far_clip);
CNK_API void cnk_camera_set_fov(cnk_camera *cam, cnk_fx fov_y_deg);
CNK_API void cnk_camera_set_ortho_height(cnk_camera *cam, cnk_fx ortho_height);
CNK_API void cnk_camera_set_orbit_distance(cnk_camera *cam, cnk_fx distance);
CNK_API void cnk_camera_add_input(cnk_camera *cam, cnk_fx yaw_delta_deg, cnk_fx pitch_delta_deg, cnk_fx zoom_delta, cnk_fx fov_delta_deg);
CNK_API void cnk_camera_snap(cnk_camera *cam);
CNK_API void cnk_camera_update(cnk_camera *cam, int dt_ticks, cnk_world_probe_fn probe, void *probe_user);
CNK_API void cnk_camera_start_blend(cnk_camera *cam, cnk_state to_state, int duration_ticks, int curve);
CNK_API int cnk_camera_add_shake(cnk_camera *cam, int duration_ticks, cnk_fx pos_amp, cnk_fx rot_amp_deg, cnk_fx fov_amp_deg, int frequency_hz, cnk_u32 seed);
CNK_API int cnk_camera_add_shake_ex(cnk_camera *cam, int duration_ticks, cnk_fx pos_amp, cnk_fx rot_amp_deg, cnk_fx fov_amp_deg, int frequency_hz, cnk_u32 seed, int play_space, cnk_fx user_yaw_deg);
CNK_API void cnk_camera_clear_shakes(cnk_camera *cam);

CNK_API void cnk_track_clear(cnk_track *track);
CNK_API int cnk_track_add_key(cnk_track *track, const cnk_keyframe *key);
CNK_API void cnk_track_set_loop(cnk_track *track, int loop);
CNK_API void cnk_rail_clear(cnk_rail *rail);
CNK_API int cnk_rail_add_point(cnk_rail *rail, cnk_vec3 point);

CNK_API cnk_mat4 cnk_camera_view_matrix(const cnk_camera *cam);
CNK_API cnk_mat4 cnk_camera_projection_matrix(const cnk_camera *cam);
CNK_API int cnk_camera_world_to_view(const cnk_camera *cam, cnk_vec3 world, cnk_vec3 *out_view);
CNK_API int cnk_camera_project_point(const cnk_camera *cam, cnk_vec3 world, int *out_x, int *out_y, cnk_fx *out_depth);
CNK_API cnk_basis cnk_basis_from_angles(cnk_fx yaw_deg, cnk_fx pitch_deg, cnk_fx roll_deg);
CNK_API cnk_pose cnk_pose_look_at(cnk_vec3 from, cnk_vec3 to);

CNK_API void cnk_arena_init(cnk_arena *arena);
CNK_API cnk_camera *cnk_arena_alloc_camera(cnk_arena *arena);
CNK_API void cnk_arena_free_camera(cnk_arena *arena, cnk_camera *cam);

#ifdef __cplusplus
}
#endif

#endif
