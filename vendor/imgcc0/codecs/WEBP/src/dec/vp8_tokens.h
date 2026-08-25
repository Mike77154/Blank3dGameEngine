#ifndef GWP_DEC_VP8_TOKENS_H_
#define GWP_DEC_VP8_TOKENS_H_

#include "vp8_dec.h"
#include "vp8_bool.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum GWPVP8CoeffToken {
  GWP_VP8_TOKEN_EOB = 0,
  GWP_VP8_TOKEN_ZERO = 1,
  GWP_VP8_TOKEN_ONE = 2,
  GWP_VP8_TOKEN_TWO = 3,
  GWP_VP8_TOKEN_THREE = 4,
  GWP_VP8_TOKEN_FOUR = 5,
  GWP_VP8_TOKEN_DCT_CAT1 = 6,
  GWP_VP8_TOKEN_DCT_CAT2 = 7,
  GWP_VP8_TOKEN_DCT_CAT3 = 8,
  GWP_VP8_TOKEN_DCT_CAT4 = 9,
  GWP_VP8_TOKEN_DCT_CAT5 = 10,
  GWP_VP8_TOKEN_DCT_CAT6 = 11
} GWPVP8CoeffToken;

typedef struct GWPVP8TokenDecoder {
  GWPVP8BoolDecoder br;
} GWPVP8TokenDecoder;

typedef struct GWPVP8TokenDecoderSet {
  GWPu32 count;
  GWPVP8TokenDecoder decoder[GWP_VP8_MAX_TOKEN_PARTITIONS];
} GWPVP8TokenDecoderSet;

GWPStatusCode GWPVP8InitTokenDecoders(const GWPVP8TokenPartitions* partitions,
                                      GWPVP8TokenDecoderSet* set);

GWPStatusCode GWPVP8ReadCoeffToken(GWPVP8TokenDecoder* decoder,
                                   const GWPu8 probs[GWP_VP8_ENTROPY_NODES],
                                   GWPBool prev_coeff_non_zero,
                                   int* out_token,
                                   int* out_value);

GWPStatusCode GWPVP8ReadCoeffBlock(GWPVP8TokenDecoder* decoder,
                                   const GWPu8 coeff_probs[GWP_VP8_COEFF_BLOCK_TYPES]
                                                          [GWP_VP8_COEFF_BANDS]
                                                          [GWP_VP8_PREV_COEFF_CONTEXTS]
                                                          [GWP_VP8_ENTROPY_NODES],
                                   GWPu32 plane,
                                   GWPBool has_left,
                                   GWPBool has_above,
                                   int first_coeff,
                                   int coeffs[16],
                                   GWPBool* out_has_coeffs,
                                   GWPu32 token_hist[GWP_VP8_TOKEN_COUNT],
                                   GWPu32* out_nonzero_count,
                                   GWPu32* out_max_abs);

#ifdef __cplusplus
}    /* extern "C" */
#endif

#endif  /* GWP_DEC_VP8_TOKENS_H_ */
