#ifndef GPLAYERPROJECTILEAIM89_H
#define GPLAYERPROJECTILEAIM89_H

#ifdef __cplusplus
extern "C" {
#endif

typedef long gppa89_fx;

typedef struct gppa89_vec3_s { gppa89_fx x,y,z; } gppa89_vec3;

typedef struct gppa89_query_s {
    int view_mode;
    gppa89_vec3 camera_origin;
    gppa89_vec3 camera_forward;
    gppa89_vec3 camera_right;
    gppa89_vec3 camera_up;
    gppa89_vec3 muzzle_origin;
    gppa89_vec3 fallback_direction;
    gppa89_fx range_fx;
    unsigned int layer_mask;
} gppa89_query;

typedef struct gppa89_target_s {
    int valid;
    int hit;
    int blocked_from_muzzle;
    gppa89_vec3 target;
} gppa89_target;

typedef int (*gppa89_target_provider_fn)(void *user,
                                         const gppa89_query *query,
                                         gppa89_target *target_out);

typedef struct gppa89_result_s {
    int valid;
    int hit;
    int blocked_from_muzzle;
    gppa89_vec3 target;
    gppa89_vec3 direction;
} gppa89_result;

int gplayerprojectileaim89_resolve(const gppa89_query *query,
                                    gppa89_target_provider_fn target_provider,
                                    void *provider_user,
                                    gppa89_result *result);

#ifdef __cplusplus
}
#endif
#endif
