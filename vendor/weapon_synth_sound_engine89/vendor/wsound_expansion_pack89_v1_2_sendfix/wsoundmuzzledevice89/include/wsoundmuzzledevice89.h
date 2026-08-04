#ifndef WSOUNDMUZZLEDEVICE89_H
#define WSOUNDMUZZLEDEVICE89_H
#include "wsound89_common.h"
#ifdef __cplusplus
extern "C" {
#endif
#define WSOUNDMUZZLEDEVICE89_CHAMBERS 4
#define WSOUNDMUZZLEDEVICE89_DELAY_MAX 37
typedef enum wsoundmuzzledevice89_type_e { WSOUNDMUZZLEDEVICE89_BARE=0, WSOUNDMUZZLEDEVICE89_FLASH_HIDER=1, WSOUNDMUZZLEDEVICE89_BRAKE=2, WSOUNDMUZZLEDEVICE89_COMPENSATOR=3, WSOUNDMUZZLEDEVICE89_SUPPRESSOR=4, WSOUNDMUZZLEDEVICE89_PORTED=5 } wsoundmuzzledevice89_type;
typedef struct wsoundmuzzledevice89_chamber_s { wsound89_i16 delay[WSOUNDMUZZLEDEVICE89_DELAY_MAX]; wsound89_u16 length,index; wsound89_i16 feedback_q15,gain_q15; } wsoundmuzzledevice89_chamber;
typedef struct wsoundmuzzledevice89_context_s { wsound89_u32 state; wsound89_i32 env; wsound89_i32 low; wsound89_i16 master_q15; wsound89_u8 active; wsoundmuzzledevice89_type type; wsoundmuzzledevice89_chamber chamber[WSOUNDMUZZLEDEVICE89_CHAMBERS]; } wsoundmuzzledevice89_context;
wsound89_result wsoundmuzzledevice89_init(wsoundmuzzledevice89_context*,wsound89_u32);
wsound89_result wsoundmuzzledevice89_trigger(wsoundmuzzledevice89_context*,wsoundmuzzledevice89_type,wsound89_u16);
wsound89_i16 wsoundmuzzledevice89_process_sample(wsoundmuzzledevice89_context*);
int wsoundmuzzledevice89_is_active(const wsoundmuzzledevice89_context*);
#ifdef __cplusplus
}
#endif
#endif
