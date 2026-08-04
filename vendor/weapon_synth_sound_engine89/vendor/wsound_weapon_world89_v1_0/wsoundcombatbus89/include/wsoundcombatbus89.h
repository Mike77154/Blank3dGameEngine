#ifndef WSOUNDCOMBATBUS89_H
#define WSOUNDCOMBATBUS89_H
#include "wsound89_common.h"
#ifdef __cplusplus
extern "C" {
#endif
#define WSOUNDCOMBATBUS89_BUSES 8U

typedef enum wsoundcombatbus89_bus_e {
    WSOUNDCOMBATBUS89_REPORT = 0,
    WSOUNDCOMBATBUS89_PROJECTILE = 1,
    WSOUNDCOMBATBUS89_MECHANISM = 2,
    WSOUNDCOMBATBUS89_IMPACT = 3,
    WSOUNDCOMBATBUS89_RICOCHET = 4,
    WSOUNDCOMBATBUS89_ROOM = 5,
    WSOUNDCOMBATBUS89_CASING = 6,
    WSOUNDCOMBATBUS89_AMBIENCE = 7
} wsoundcombatbus89_bus;

typedef struct wsoundcombatbus89_frame_s {
    wsound89_i16 left[WSOUNDCOMBATBUS89_BUSES];
    wsound89_i16 right[WSOUNDCOMBATBUS89_BUSES];
} wsoundcombatbus89_frame;

typedef struct wsoundcombatbus89_context_s {
    wsound89_i16 gain_q15[WSOUNDCOMBATBUS89_BUSES];
    wsound89_i32 limiter_gain_q15;
    wsound89_i32 tail_gain_q15;
    wsound89_i16 transient_threshold;
    wsound89_i16 limiter_threshold;
    wsound89_u16 tail_recovery_shift;
    wsound89_u16 limiter_release_shift;
} wsoundcombatbus89_context;

wsound89_result wsoundcombatbus89_init(wsoundcombatbus89_context *ctx);
void wsoundcombatbus89_set_gain(wsoundcombatbus89_context *ctx, wsoundcombatbus89_bus bus,
                                wsound89_i16 gain_q15);
void wsoundcombatbus89_process_sample(wsoundcombatbus89_context *ctx,
                                      const wsoundcombatbus89_frame *input,
                                      wsound89_i16 *out_left, wsound89_i16 *out_right);
#ifdef __cplusplus
}
#endif
#endif
