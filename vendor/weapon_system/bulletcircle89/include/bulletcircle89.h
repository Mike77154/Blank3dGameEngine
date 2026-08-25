#ifndef BULLETCIRCLE89_H
#define BULLETCIRCLE89_H

#ifdef __cplusplus
extern "C" {
#endif

#define BC89_ONE 4096L
#define BC89_TURN_ONE 65536L

typedef long bc89_fx;

typedef struct bc89_vec3_s {
    bc89_fx x;
    bc89_fx y;
    bc89_fx z;
} bc89_vec3;

typedef struct bc89_request_s {
    bc89_vec3 center;
    bc89_vec3 right;
    bc89_vec3 up;
    bc89_fx radius_fx;
    int slot_index;
    int slot_count;
    long phase_turn_q16;
} bc89_request;

typedef struct bc89_result_s {
    int valid;
    int slot_index;
    long angle_turn_q16;
    bc89_vec3 offset;
    bc89_vec3 origin;
} bc89_result;

int bulletcircle89_resolve(const bc89_request *request,
                           bc89_result *result);

#ifdef __cplusplus
}
#endif

#endif
