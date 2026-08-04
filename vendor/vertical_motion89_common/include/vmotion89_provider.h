#ifndef VMOTION89_PROVIDER_H
#define VMOTION89_PROVIDER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef long vm89_scalar;

typedef struct vm89_vec3_tag {
    vm89_scalar x;
    vm89_scalar y;
    vm89_scalar z;
} vm89_vec3;

typedef struct vm89_math_provider_tag {
    void *user;
    vm89_scalar one;
    vm89_scalar epsilon;
    vm89_scalar (*add)(void *user, vm89_scalar a, vm89_scalar b);
    vm89_scalar (*sub)(void *user, vm89_scalar a, vm89_scalar b);
    vm89_scalar (*mul)(void *user, vm89_scalar a, vm89_scalar b);
    vm89_scalar (*abs_value)(void *user, vm89_scalar value);
    vm89_scalar (*length3)(void *user, const vm89_vec3 *value);
    int (*normalize3)(void *user, const vm89_vec3 *value,
                      vm89_vec3 *out_normalized);
} vm89_math_provider;

typedef struct vm89_transform_provider_tag {
    void *user;
    int (*get_position)(void *user, void *actor, vm89_vec3 *out_position);
    int (*set_position)(void *user, void *actor,
                        const vm89_vec3 *position);
} vm89_transform_provider;

typedef struct vm89_physics_provider_tag {
    void *user;
    int (*get_floor_y)(void *user, void *actor, vm89_scalar *out_floor_y);
    int (*is_grounded)(void *user, void *actor, vm89_scalar floor_y,
                       int *out_grounded);
    int (*move_position)(void *user, void *actor,
                         const vm89_vec3 *desired_position,
                         int clamp_to_floor,
                         vm89_scalar floor_y,
                         vm89_vec3 *out_resolved_position);
} vm89_physics_provider;

typedef struct vm89_provider_bundle_tag {
    vm89_math_provider math;
    vm89_transform_provider transform;
    vm89_physics_provider physics;
} vm89_provider_bundle;

#ifdef __cplusplus
}
#endif

#endif
