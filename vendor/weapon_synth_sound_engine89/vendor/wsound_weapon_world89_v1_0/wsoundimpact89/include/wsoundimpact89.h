#ifndef WSOUNDIMPACT89_H
#define WSOUNDIMPACT89_H
#include "wsound89_common.h"
#ifdef __cplusplus
extern "C" {
#endif
#define WSOUNDIMPACT89_MODES 4U
#define WSOUNDIMPACT89_DELAY_MAX 24U

typedef enum wsoundimpact89_material_e {
    WSOUNDIMPACT89_METAL = 0,
    WSOUNDIMPACT89_CONCRETE = 1,
    WSOUNDIMPACT89_WOOD = 2,
    WSOUNDIMPACT89_DIRT = 3,
    WSOUNDIMPACT89_GLASS = 4,
    WSOUNDIMPACT89_FLESH = 5
} wsoundimpact89_material;

typedef struct wsoundimpact89_mode_s {
    wsound89_i16 delay[WSOUNDIMPACT89_DELAY_MAX];
    wsound89_u16 index;
    wsound89_u16 length;
    wsound89_i16 feedback_q15;
    wsound89_i16 gain_q15;
} wsoundimpact89_mode;

typedef struct wsoundimpact89_context_s {
    wsoundimpact89_mode mode[WSOUNDIMPACT89_MODES];
    wsound89_u32 state;
    wsound89_i32 envelope;
    wsound89_i16 noise_gain_q15;
    wsound89_i16 master_q15;
    wsound89_u8 active;
} wsoundimpact89_context;

wsound89_result wsoundimpact89_init(wsoundimpact89_context *ctx, wsound89_u32 seed);
wsound89_result wsoundimpact89_trigger(wsoundimpact89_context *ctx, wsoundimpact89_material material,
                                       wsound89_u16 energy_q15, wsound89_u16 size_q15);
wsound89_i16 wsoundimpact89_process_sample(wsoundimpact89_context *ctx);
int wsoundimpact89_is_active(const wsoundimpact89_context *ctx);
#ifdef __cplusplus
}
#endif
#endif
