#ifndef WSRB89_INTERNAL_H
#define WSRB89_INTERNAL_H

#include "wsound_rocketblast89.h"

wsrb89_s16 wsrb89_clamp16(wsrb89_s32 value);
wsrb89_s16 wsrb89_soft_clip(wsrb89_s32 value);
wsrb89_s16 wsrb89_sine_q15(wsrb89_u16 phase);
wsrb89_s16 wsrb89_saw_q15(wsrb89_u16 phase);
wsrb89_s16 wsrb89_noise_next(wsrb89_u32 *state);
wsrb89_s16 wsrb89_svf_process(wsrb89_svf *svf, wsrb89_s16 input,
                              wsrb89_u16 mode);
wsrb89_s16 wsrb89_svf_coeff(wsrb89_u32 sample_rate, wsrb89_u16 cutoff_hz);
void wsrb89_env_start(wsrb89_envelope *env, wsrb89_s16 from_q15,
                      wsrb89_s16 to_q15, wsrb89_u32 samples);
wsrb89_s16 wsrb89_env_tick(wsrb89_envelope *env);

#endif
