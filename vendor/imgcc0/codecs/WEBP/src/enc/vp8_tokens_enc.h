#ifndef GWP_ENC_VP8_TOKENS_ENC_H_
#define GWP_ENC_VP8_TOKENS_ENC_H_

#include "vp8_bool_enc.h"
#include "../dec/vp8_dec.h"
#include "../dec/vp8_tokens.h"

#ifdef __cplusplus
extern "C" {
#endif

GWPStatusCode GWPVP8WriteCoeffToken(GWPVP8BoolEncoder* bw,
                                    const GWPu8 probs[GWP_VP8_ENTROPY_NODES],
                                    GWPBool prev_coeff_non_zero,
                                    int token,
                                    int value);

GWPStatusCode GWPVP8WriteCoeffBlock(GWPVP8BoolEncoder* bw,
                                    const GWPu8 coeff_probs[GWP_VP8_COEFF_BLOCK_TYPES]
                                                           [GWP_VP8_COEFF_BANDS]
                                                           [GWP_VP8_PREV_COEFF_CONTEXTS]
                                                           [GWP_VP8_ENTROPY_NODES],
                                    GWPu32 plane,
                                    GWPBool has_left,
                                    GWPBool has_above,
                                    int first_coeff,
                                    const int coeffs[16],
                                    GWPBool* out_has_coeffs);

#ifdef __cplusplus
}    /* extern "C" */
#endif

#endif  /* GWP_ENC_VP8_TOKENS_ENC_H_ */
