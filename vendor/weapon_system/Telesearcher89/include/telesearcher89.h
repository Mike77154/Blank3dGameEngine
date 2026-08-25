#ifndef TELESEARCHER89_H
#define TELESEARCHER89_H

#ifdef __cplusplus
extern "C" {
#endif

typedef long ts89_fx;
typedef struct ts89_vec3_s { ts89_fx x, y, z; } ts89_vec3;

#define TS89_ONE 4096L
#define TS89_TARGET_NONE (-1)
#define TS89_FLAG_REQUIRE_TARGET 1UL
#define TS89_FLAG_REACQUIRE 2UL

typedef struct ts89_target_s {
    int valid;
    int actor_id;
    ts89_vec3 position;
} ts89_target;

typedef int (*ts89_target_provider_fn)(void *user,
                                       int owner_actor_id,
                                       int requested_target_id,
                                       ts89_target *target_out);

typedef struct ts89_request_s {
    int owner_actor_id;
    int target_actor_id;
    ts89_vec3 position;
    ts89_vec3 velocity;
    ts89_vec3 target_offset;
    ts89_fx speed_fx;
    ts89_fx gain_fx;
    unsigned long flags;
} ts89_request;

typedef struct ts89_result_s {
    int valid;
    int target_found;
    int target_actor_id;
    ts89_vec3 target_position;
    ts89_vec3 velocity;
} ts89_result;

int telesearcher89_step(const ts89_request *request,
                         ts89_target_provider_fn target_provider,
                         void *provider_user,
                         ts89_result *result);

#ifdef __cplusplus
}
#endif
#endif
