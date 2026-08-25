#ifndef SATELLABORNER89_H
#define SATELLABORNER89_H

#ifdef __cplusplus
extern "C" {
#endif

typedef long sat89_fx;
typedef struct sat89_vec3_s { sat89_fx x, y, z; } sat89_vec3;

#define SAT89_TARGET_NONE (-1)
#define SAT89_FLAG_REQUIRE_TARGET 1UL

typedef struct sat89_target_s {
    int valid;
    int actor_id;
    sat89_vec3 position;
} sat89_target;

typedef int (*sat89_target_provider_fn)(void *user,
                                        int owner_actor_id,
                                        int requested_target_id,
                                        sat89_target *target_out);

typedef struct sat89_request_s {
    int owner_actor_id;
    int requested_target_id;
    sat89_vec3 original_origin;
    sat89_vec3 target_offset;
    unsigned long flags;
} sat89_request;

typedef struct sat89_result_s {
    int valid;
    int target_found;
    int target_actor_id;
    sat89_vec3 target_position;
    sat89_vec3 spawn_origin;
} sat89_result;

int satellaborner89_resolve(const sat89_request *request,
                            sat89_target_provider_fn target_provider,
                            void *provider_user,
                            sat89_result *result);

#ifdef __cplusplus
}
#endif
#endif
