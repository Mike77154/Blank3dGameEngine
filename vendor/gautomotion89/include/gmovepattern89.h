#ifndef GMOVEPATTERN89_H
#define GMOVEPATTERN89_H

#include "gmove89_math.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GMOVEPATTERN89_ZIGZAG_TO  1
#define GMOVEPATTERN89_HELIX_TO   2
#define GMOVEPATTERN89_PINGPONG   3
#define GMOVEPATTERN89_ORBIT      4
#define GMOVEPATTERN89_SINE_TO    5

typedef struct GMovePatternConfig89 {
    int type;
    GMoveFx89 speed;
    GMoveFx89 amplitude;
    GMoveFx89 radius;
    GMoveFx89 frequency;
    GMoveFx89 stop_distance;
    GMoveVec3_89 axis_a;
    GMoveVec3_89 axis_b;
} GMovePatternConfig89;

typedef struct GMovePatternState89 {
    GMoveVec3_89 origin;
    GMoveFx89 phase;
    GMoveFx89 travel;
    GMoveFx89 elapsed;
    int active;
    int completed;
} GMovePatternState89;

void gmovepattern89_config_default(GMovePatternConfig89 *config);
void gmovepattern89_begin(
    GMovePatternState89 *state,
    GMoveVec3_89 origin
);
void gmovepattern89_reset(GMovePatternState89 *state);

void gmovepattern89_step(
    GMovePatternState89 *state,
    const GMovePatternConfig89 *config,
    GMoveVec3_89 current_position,
    GMoveVec3_89 target_position,
    GMoveFx89 delta_time,
    GMoveMotion89 *out_motion
);

#ifdef __cplusplus
}
#endif

#endif
