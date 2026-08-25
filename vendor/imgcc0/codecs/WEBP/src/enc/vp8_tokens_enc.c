#include "vp8_tokens_enc.h"

#include "vp8_tree_enc.h"
#include "../dec/vp8_probdata.h"

static int GWPVP8CoeffTokenFromValue(int v) {
  int a;
  a = (v < 0) ? -v : v;
  if (a == 0) return (int)GWP_VP8_TOKEN_ZERO;
  if (a == 1) return (int)GWP_VP8_TOKEN_ONE;
  if (a == 2) return (int)GWP_VP8_TOKEN_TWO;
  if (a == 3) return (int)GWP_VP8_TOKEN_THREE;
  if (a == 4) return (int)GWP_VP8_TOKEN_FOUR;
  if (a <= 6) return (int)GWP_VP8_TOKEN_DCT_CAT1;
  if (a <= 10) return (int)GWP_VP8_TOKEN_DCT_CAT2;
  if (a <= 18) return (int)GWP_VP8_TOKEN_DCT_CAT3;
  if (a <= 34) return (int)GWP_VP8_TOKEN_DCT_CAT4;
  if (a <= 66) return (int)GWP_VP8_TOKEN_DCT_CAT5;
  return (int)GWP_VP8_TOKEN_DCT_CAT6;
}

static void GWPVP8WriteCoeffCategory(GWPVP8BoolEncoder* bw,
                                     int token,
                                     int value) {
  static const int kBase[6] = { 5, 7, 11, 19, 35, 67 };
  const GWPu8* probs;
  int bits;
  int base_index;
  int magnitude;
  int i;
  if (bw == 0) return;
  base_index = token - (int)GWP_VP8_TOKEN_DCT_CAT1;
  if (base_index < 0 || base_index > 5) return;
  switch (token) {
    case GWP_VP8_TOKEN_DCT_CAT1: probs = kGWPVP8Cat1Probs; bits = 1; break;
    case GWP_VP8_TOKEN_DCT_CAT2: probs = kGWPVP8Cat2Probs; bits = 2; break;
    case GWP_VP8_TOKEN_DCT_CAT3: probs = kGWPVP8Cat3Probs; bits = 3; break;
    case GWP_VP8_TOKEN_DCT_CAT4: probs = kGWPVP8Cat4Probs; bits = 4; break;
    case GWP_VP8_TOKEN_DCT_CAT5: probs = kGWPVP8Cat5Probs; bits = 5; break;
    case GWP_VP8_TOKEN_DCT_CAT6: probs = kGWPVP8Cat6Probs; bits = 11; break;
    default: return;
  }
  magnitude = (value < 0) ? -value : value;
  magnitude -= kBase[base_index];
  for (i = bits - 1; i >= 0; --i) {
    GWPVP8BoolEncWrite(bw, (int)probs[bits - 1 - i], (magnitude >> i) & 1);
  }
}

GWPStatusCode GWPVP8WriteCoeffToken(GWPVP8BoolEncoder* bw,
                                    const GWPu8 probs[GWP_VP8_ENTROPY_NODES],
                                    GWPBool prev_coeff_non_zero,
                                    int token,
                                    int value) {
  int start_index;
  if (bw == 0 || probs == 0) return GWP_STATUS_INVALID_PARAM;
  start_index = prev_coeff_non_zero ? 0 : 2;
  if (!GWPVP8WriteTreeValueAt(bw,
                              kGWPVP8CoeffTree,
                              probs,
                              22,
                              start_index,
                              token)) {
    return GWP_STATUS_BITSTREAM_ERROR;
  }
  if (token >= (int)GWP_VP8_TOKEN_DCT_CAT1 && token <= (int)GWP_VP8_TOKEN_DCT_CAT6) {
    GWPVP8WriteCoeffCategory(bw, token, value);
  }
  if (token != (int)GWP_VP8_TOKEN_EOB && token != (int)GWP_VP8_TOKEN_ZERO) {
    GWPVP8BoolEncWriteBit(bw, value < 0 ? 1 : 0);
  }
  return GWPVP8BoolEncOk(bw) ? GWP_STATUS_OK : GWP_STATUS_NOT_ENOUGH_OUTPUT;
}

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
                                    GWPBool* out_has_coeffs) {
  int prev_context;
  int prev_coeff_non_zero;
  int i;
  GWPBool has_coeffs;
  if (bw == 0 || coeff_probs == 0 || coeffs == 0 || out_has_coeffs == 0) {
    return GWP_STATUS_INVALID_PARAM;
  }
  if (plane >= GWP_VP8_COEFF_BLOCK_TYPES) return GWP_STATUS_INVALID_PARAM;
  if (first_coeff < 0 || first_coeff > 15) return GWP_STATUS_INVALID_PARAM;
  has_coeffs = GWP_FALSE;
  prev_context = (has_left ? 1 : 0) + (has_above ? 1 : 0);
  if (prev_context > 2) prev_context = 2;
  prev_coeff_non_zero = 1;
  for (i = first_coeff; i < 16; ++i) {
    GWPu32 band;
    int coeff;
    int token;
    GWPStatusCode st;
    band = (GWPu32)kGWPVP8CoeffBands[i];
    coeff = coeffs[kGWPVP8ZigZag[i]];
    if (coeff == 0) {
      int has_more;
      has_more = 0;
      {
        int j;
        for (j = i + 1; j < 16; ++j) {
          if (coeffs[kGWPVP8ZigZag[j]] != 0) {
            has_more = 1;
            break;
          }
        }
      }
      token = has_more ? (int)GWP_VP8_TOKEN_ZERO : (int)GWP_VP8_TOKEN_EOB;
      st = GWPVP8WriteCoeffToken(bw,
                                 coeff_probs[plane][band][prev_context],
                                 prev_coeff_non_zero ? GWP_TRUE : GWP_FALSE,
                                 token,
                                 0);
      if (st != GWP_STATUS_OK) return st;
      if (token == (int)GWP_VP8_TOKEN_EOB) {
        *out_has_coeffs = has_coeffs;
        return GWP_STATUS_OK;
      }
      prev_context = 0;
      prev_coeff_non_zero = 0;
      continue;
    }
    token = GWPVP8CoeffTokenFromValue(coeff);
    has_coeffs = GWP_TRUE;
    st = GWPVP8WriteCoeffToken(bw,
                               coeff_probs[plane][band][prev_context],
                               prev_coeff_non_zero ? GWP_TRUE : GWP_FALSE,
                               token,
                               coeff);
    if (st != GWP_STATUS_OK) return st;
    {
      int abs_value;
      abs_value = (coeff < 0) ? -coeff : coeff;
      prev_context = (abs_value == 1) ? 1 : 2;
      prev_coeff_non_zero = 1;
    }
  }
  *out_has_coeffs = has_coeffs;
  return GWP_STATUS_OK;
}
