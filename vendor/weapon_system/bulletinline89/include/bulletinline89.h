#ifndef BULLETINLINE89_H
#define BULLETINLINE89_H

#ifdef __cplusplus
extern "C" {
#endif

#define BI89_ONE 4096L

typedef long bi89_fx;

typedef struct bi89_vec3_s {
    bi89_fx x;
    bi89_fx y;
    bi89_fx z;
} bi89_vec3;

typedef struct bi89_request_s {
    bi89_vec3 origin;
    bi89_vec3 target;
    bi89_vec3 fallback_direction;
    int target_valid;
} bi89_request;

typedef struct bi89_result_s {
    int valid;
    int used_target;
    bi89_vec3 direction;
} bi89_result;

int bulletinline89_resolve(const bi89_request *request,
                           bi89_result *result);

#ifdef __cplusplus
}
#endif

#endif
