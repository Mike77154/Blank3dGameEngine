#ifndef GMOVESEQUENCE89_H
#define GMOVESEQUENCE89_H

#include "gautmove89.h"
#include "gmovepattern89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GMOVESEQ89_MOVE_TO      1
#define GMOVESEQ89_MOVE_AWAY    2
#define GMOVESEQ89_PATTERN      3
#define GMOVESEQ89_WAIT         4
#define GMOVESEQ89_ROTATE_TO    5
#define GMOVESEQ89_END          255

typedef struct GMoveSequenceStep89 {
    int op;
    GMoveTarget89 target;
    GAutMoveConfig89 automove;
    GMovePatternConfig89 pattern;
    GMoveFx89 duration;
    GMoveFx89 safe_distance;
    GMoveFx89 turn_fraction_per_second;
} GMoveSequenceStep89;

typedef struct GMoveSequenceState89 {
    unsigned long index;
    GMoveFx89 step_elapsed;
    int step_started;
    int done;
    GMovePatternState89 pattern_state;
} GMoveSequenceState89;

void gmovesequence89_reset(GMoveSequenceState89 *state);

int gmovesequence89_update(
    GMoveSequenceState89 *state,
    const GMoveSequenceStep89 *steps,
    unsigned long step_count,
    const GMoveProvider89 *provider,
    GMoveVec3_89 current_position,
    GMoveVec3_89 current_forward,
    GMoveFx89 delta_time,
    GMoveMotion89 *out_motion
);

int gmovesequence89_update_provider(
    GMoveSequenceState89 *state,
    const GMoveSequenceStep89 *steps,
    unsigned long step_count,
    const GMoveProvider89 *provider,
    GMoveId89 moving_entity,
    GMoveFx89 delta_time,
    GMoveMotion89 *out_motion
);

#ifdef __cplusplus
}
#endif

#endif
