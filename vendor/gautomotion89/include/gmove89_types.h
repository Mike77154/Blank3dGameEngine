#ifndef GMOVE89_TYPES_H
#define GMOVE89_TYPES_H

#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * gautomotion89 uses signed Q16.16 fixed point.
 * The public numeric range is intentionally limited to signed 32-bit values,
 * even on hosts where long is wider.
 */
typedef signed long GMoveFx89;
typedef unsigned long GMoveU32_89;
typedef signed long GMoveId89;

#define GMOVE89_FX_SHIFT       16
#define GMOVE89_FX_ONE         65536L
#define GMOVE89_FX_HALF        32768L
#define GMOVE89_FX_QUARTER     16384L
#define GMOVE89_FX_MAX         2147483647L
#define GMOVE89_FX_MIN         (-2147483647L - 1L)

#define GMOVE89_FX_FROM_INT(v) ((GMoveFx89)((v) * GMOVE89_FX_ONE))
#define GMOVE89_FX_TO_INT(v)   ((long)((v) / GMOVE89_FX_ONE))

#define GMOVE89_FALSE 0
#define GMOVE89_TRUE  1

typedef struct GMoveVec3_89 {
    GMoveFx89 x;
    GMoveFx89 y;
    GMoveFx89 z;
} GMoveVec3_89;

typedef struct GMoveMotion89 {
    GMoveVec3_89 target_position;
    GMoveVec3_89 delta;
    GMoveVec3_89 desired_direction;
    GMoveVec3_89 desired_forward;
    GMoveFx89 distance_remaining;
    int reached;
    int has_translation;
    int has_rotation;
    int valid;
} GMoveMotion89;

#define GMOVE89_TARGET_POINT   1
#define GMOVE89_TARGET_ENTITY  2
#define GMOVE89_TARGET_SOCKET  3

typedef struct GMoveTarget89 {
    int type;
    GMoveId89 entity_id;
    GMoveId89 socket_id;
    GMoveVec3_89 point;
} GMoveTarget89;

typedef int (*GMoveGetPositionFn89)(
    void *user,
    GMoveId89 entity_id,
    GMoveVec3_89 *out_position
);

typedef int (*GMoveGetForwardFn89)(
    void *user,
    GMoveId89 entity_id,
    GMoveVec3_89 *out_forward
);

typedef int (*GMoveGetSocketPositionFn89)(
    void *user,
    GMoveId89 entity_id,
    GMoveId89 socket_id,
    GMoveVec3_89 *out_position
);

typedef int (*GMoveApplyMotionFn89)(
    void *user,
    GMoveId89 entity_id,
    const GMoveMotion89 *motion
);

typedef struct GMoveProvider89 {
    void *user;
    GMoveGetPositionFn89 get_position;
    GMoveGetForwardFn89 get_forward;
    GMoveGetSocketPositionFn89 get_socket_position;
    GMoveApplyMotionFn89 apply_motion;
} GMoveProvider89;

void gmove89_motion_clear(GMoveMotion89 *motion);
int gmove89_target_resolve(
    const GMoveProvider89 *provider,
    const GMoveTarget89 *target,
    GMoveVec3_89 *out_position
);
int gmove89_apply_motion(
    const GMoveProvider89 *provider,
    GMoveId89 entity_id,
    const GMoveMotion89 *motion
);

#ifdef __cplusplus
}
#endif

#endif
