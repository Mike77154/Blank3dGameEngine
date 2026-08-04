#ifndef CCS_TRACE_BOX_H
#define CCS_TRACE_BOX_H

/*
    Trace AABB vs planes
    --------------------
    Drop-in compatible (universo A): ccs_fixed/ccs_vec3/ccs_aabb.
*/

#include "ccs_shapes.h" /* ccs_aabb */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    ccs_vec3 normal;
    ccs_fixed dist;
} CCS_Plane;

typedef struct {
    ccs_aabb box;
    ccs_vec3 start;
    ccs_vec3 end;
} CCS_TraceBox;

typedef struct {
    ccs_fixed fraction;      /* 0..CCS_FIXED_ONE */
    ccs_vec3 hit_normal;
    int hit;
} CCS_TraceResult;

/* API principal */
void ccs_trace_box_against_planes(
    const CCS_TraceBox* trace,
    const CCS_Plane* planes,
    int plane_count,
    CCS_TraceResult* out
);

#ifdef __cplusplus
}
#endif

#endif /* CCS_TRACE_BOX_H */
