#ifndef GAUTMOVE89_H
#define GAUTMOVE89_H

#include "gmove89_math.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GAUTMOVE89_SPEED_CONSTANT  0
#define GAUTMOVE89_SPEED_SLOWDOWN  1

typedef struct GAutMoveConfig89 {
    GMoveFx89 speed;
    GMoveFx89 stop_distance;
    GMoveFx89 slowdown_distance;
    int speed_mode;
} GAutMoveConfig89;

void gautmove89_config_default(GAutMoveConfig89 *config);

void gautmove89_step_to_point(
    GMoveVec3_89 current_position,
    GMoveVec3_89 target_position,
    const GAutMoveConfig89 *config,
    GMoveFx89 delta_time,
    GMoveMotion89 *out_motion
);

void gautmove89_step_away_from_point(
    GMoveVec3_89 current_position,
    GMoveVec3_89 threat_position,
    GMoveVec3_89 fallback_direction,
    const GAutMoveConfig89 *config,
    GMoveFx89 safe_distance,
    GMoveFx89 delta_time,
    GMoveMotion89 *out_motion
);

void gautmove89_step_direction(
    GMoveVec3_89 current_position,
    GMoveVec3_89 direction,
    GMoveFx89 speed,
    GMoveFx89 delta_time,
    GMoveMotion89 *out_motion
);

void gautmove89_step_forward(
    GMoveVec3_89 current_position,
    GMoveVec3_89 current_forward,
    GMoveFx89 speed,
    GMoveFx89 delta_time,
    GMoveMotion89 *out_motion
);

void gautmove89_rotate_toward_point(
    GMoveVec3_89 current_position,
    GMoveVec3_89 current_forward,
    GMoveVec3_89 target_position,
    GMoveFx89 turn_fraction_per_second,
    GMoveFx89 delta_time,
    GMoveMotion89 *out_motion
);

int gautmove89_step_to_target(
    const GMoveProvider89 *provider,
    GMoveId89 moving_entity,
    const GMoveTarget89 *target,
    const GAutMoveConfig89 *config,
    GMoveFx89 delta_time,
    GMoveMotion89 *out_motion
);

int gautmove89_step_away_from_target(
    const GMoveProvider89 *provider,
    GMoveId89 moving_entity,
    const GMoveTarget89 *target,
    const GAutMoveConfig89 *config,
    GMoveFx89 safe_distance,
    GMoveFx89 delta_time,
    GMoveMotion89 *out_motion
);

#ifdef __cplusplus
}
#endif

#endif
