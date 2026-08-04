#ifndef WSOUNDRECEIVER89_H
#define WSOUNDRECEIVER89_H
#include "wsound89_common.h"
#ifdef __cplusplus
extern "C" {
#endif
#define WSOUNDRECEIVER89_MODES 6U
#define WSOUNDRECEIVER89_DELAY_MAX 32U

typedef enum wsoundreceiver89_preset_e {
    WSOUNDRECEIVER89_PISTOL_STEEL = 0,
    WSOUNDRECEIVER89_RIFLE_STEEL = 1,
    WSOUNDRECEIVER89_SHOTGUN_WOOD = 2,
    WSOUNDRECEIVER89_POLYMER = 3,
    WSOUNDRECEIVER89_HEAVY = 4
} wsoundreceiver89_preset;

typedef struct wsoundreceiver89_mode_s {
    wsound89_i16 delay[WSOUNDRECEIVER89_DELAY_MAX];
    wsound89_u16 index;
    wsound89_u16 length;
    wsound89_i16 feedback_q15;
    wsound89_i16 gain_q15;
} wsoundreceiver89_mode;

typedef struct wsoundreceiver89_context_s {
    wsoundreceiver89_mode mode[WSOUNDRECEIVER89_MODES];
    wsound89_i32 excitation;
    wsound89_i16 master_q15;
} wsoundreceiver89_context;

wsound89_result wsoundreceiver89_init(wsoundreceiver89_context *ctx, wsoundreceiver89_preset preset);
void wsoundreceiver89_reset(wsoundreceiver89_context *ctx);
void wsoundreceiver89_excite(wsoundreceiver89_context *ctx, wsound89_i16 impulse);
wsound89_i16 wsoundreceiver89_process_sample(wsoundreceiver89_context *ctx, wsound89_i16 input);
#ifdef __cplusplus
}
#endif
#endif
