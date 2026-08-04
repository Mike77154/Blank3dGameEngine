#ifndef WSOUNDACTION89_H
#define WSOUNDACTION89_H
#include "wsound89_common.h"
#ifdef __cplusplus
extern "C" {
#endif
#define WSOUNDACTION89_MAX_EVENTS 6U

typedef enum wsoundaction89_type_e {
    WSOUNDACTION89_PISTOL = 0,
    WSOUNDACTION89_MACHINE = 1,
    WSOUNDACTION89_RIFLE = 2,
    WSOUNDACTION89_REVOLVER = 3,
    WSOUNDACTION89_PUMP_SHOTGUN = 4,
    WSOUNDACTION89_SEMI_SHOTGUN = 5
} wsoundaction89_type;

typedef enum wsoundaction89_event_code_e {
    WSOUNDACTION89_NONE = 0,
    WSOUNDACTION89_HAMMER = 1,
    WSOUNDACTION89_BOLT_REAR = 2,
    WSOUNDACTION89_EJECT = 3,
    WSOUNDACTION89_FEED = 4,
    WSOUNDACTION89_CARRIER = 5,
    WSOUNDACTION89_BATTERY = 6,
    WSOUNDACTION89_CYLINDER = 7,
    WSOUNDACTION89_UNLOCK = 8,
    WSOUNDACTION89_LOCK = 9
} wsoundaction89_event_code;

typedef struct wsoundaction89_event_s {
    wsound89_u32 frame_offset;
    wsound89_u16 code;
    wsound89_u16 energy_q15;
} wsoundaction89_event;

typedef struct wsoundaction89_context_s {
    wsound89_u32 sample_rate;
    wsound89_u32 frame;
    wsound89_u32 speed_q16;
    wsound89_u16 type;
    wsound89_u16 next_event;
    wsound89_u8 active;
} wsoundaction89_context;

wsound89_result wsoundaction89_init(wsoundaction89_context *ctx, wsound89_u32 sample_rate);
wsound89_result wsoundaction89_trigger(wsoundaction89_context *ctx, wsoundaction89_type type,
                                       wsound89_u32 speed_q16);
wsound89_result wsoundaction89_advance(wsoundaction89_context *ctx, wsound89_u32 frames,
                                       wsoundaction89_event *events, wsound89_u16 capacity,
                                       wsound89_u16 *written);
int wsoundaction89_is_active(const wsoundaction89_context *ctx);
#ifdef __cplusplus
}
#endif
#endif
