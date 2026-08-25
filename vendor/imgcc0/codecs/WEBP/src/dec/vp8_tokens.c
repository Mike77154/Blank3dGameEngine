#include "vp8_tokens.h"

#include "vp8_probdata.h"
#include "vp8_tree.h"
#include "../utils/common.h"

static GWPBool GWPVP8ReadCoeffCategoryBits(GWPVP8BoolDecoder* br,
                                           int token,
                                           int* out_value) {
  static const int kBase[6] = { 5, 7, 11, 19, 35, 67 };
  const GWPu8* probs;
  int bits;
  int base_index;
  int magnitude;
  int i;
  GWPu32 bit;
  if (br == 0 || out_value == 0) return GWP_FALSE;
  base_index = token - (int)GWP_VP8_TOKEN_DCT_CAT1;
  if (base_index < 0 || base_index > 5) return GWP_FALSE;
  switch (token) {
    case GWP_VP8_TOKEN_DCT_CAT1: probs = kGWPVP8Cat1Probs; bits = 1; break;
    case GWP_VP8_TOKEN_DCT_CAT2: probs = kGWPVP8Cat2Probs; bits = 2; break;
    case GWP_VP8_TOKEN_DCT_CAT3: probs = kGWPVP8Cat3Probs; bits = 3; break;
    case GWP_VP8_TOKEN_DCT_CAT4: probs = kGWPVP8Cat4Probs; bits = 4; break;
    case GWP_VP8_TOKEN_DCT_CAT5: probs = kGWPVP8Cat5Probs; bits = 5; break;
    case GWP_VP8_TOKEN_DCT_CAT6: probs = kGWPVP8Cat6Probs; bits = 11; break;
    default: return GWP_FALSE;
  }
  magnitude = kBase[base_index];
  for (i = 0; i < bits; ++i) {
    if (!GWPVP8BoolGet(br, (int)probs[i], &bit)) return GWP_FALSE;
    magnitude += (int)bit << (bits - 1 - i);
  }
  *out_value = magnitude;
  return GWP_TRUE;
}

GWPStatusCode GWPVP8InitTokenDecoders(const GWPVP8TokenPartitions* partitions,
                                      GWPVP8TokenDecoderSet* set) {
  GWPu32 i;
  if (partitions == 0 || set == 0) return GWP_STATUS_INVALID_PARAM;
  if (partitions->partition_count == 0u ||
      partitions->partition_count > GWP_VP8_MAX_TOKEN_PARTITIONS) {
    return GWP_STATUS_PARSE_ERROR;
  }
  GWPZero(set, (GWPu32)sizeof(*set));
  set->count = partitions->partition_count;
  for (i = 0u; i < set->count; ++i) {
    GWPVP8BoolInit(&set->decoder[i].br,
                   partitions->partition[i].bytes,
                   partitions->partition[i].size);
    if (GWPVP8BoolHasError(&set->decoder[i].br)) return GWP_STATUS_TRUNCATED_DATA;
  }
  return GWP_STATUS_OK;
}

GWPStatusCode GWPVP8ReadCoeffToken(GWPVP8TokenDecoder* decoder,
                                   const GWPu8 probs[GWP_VP8_ENTROPY_NODES],
                                   GWPBool prev_coeff_non_zero,
                                   int* out_token,
                                   int* out_value) {
  int token;
  int value;
  int start_index;
  if (decoder == 0 || probs == 0 || out_token == 0 || out_value == 0) {
    return GWP_STATUS_INVALID_PARAM;
  }
  start_index = prev_coeff_non_zero ? 0 : 2;
  if (!GWPVP8ReadTreeAt(kGWPVP8CoeffTree,
                        probs,
                        22,
                        start_index,
                        &decoder->br,
                        &token)) {
    return GWP_STATUS_TRUNCATED_DATA;
  }
  if (token < (int)GWP_VP8_TOKEN_EOB || token > (int)GWP_VP8_TOKEN_DCT_CAT6) {
    return GWP_STATUS_PARSE_ERROR;
  }
  value = 0;
  switch (token) {
    case GWP_VP8_TOKEN_EOB:
      value = 0;
      break;
    case GWP_VP8_TOKEN_ZERO:
      value = 0;
      break;
    case GWP_VP8_TOKEN_ONE:
      value = 1;
      break;
    case GWP_VP8_TOKEN_TWO:
      value = 2;
      break;
    case GWP_VP8_TOKEN_THREE:
      value = 3;
      break;
    case GWP_VP8_TOKEN_FOUR:
      value = 4;
      break;
    default:
      if (!GWPVP8ReadCoeffCategoryBits(&decoder->br, token, &value)) {
        return GWP_STATUS_TRUNCATED_DATA;
      }
      break;
  }
  if (token != (int)GWP_VP8_TOKEN_EOB && token != (int)GWP_VP8_TOKEN_ZERO) {
    GWPu32 sign;
    if (!GWPVP8BoolGetBit(&decoder->br, &sign)) return GWP_STATUS_TRUNCATED_DATA;
    if (sign) value = -value;
  }
  *out_token = token;
  *out_value = value;
  return GWP_STATUS_OK;
}

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
                                   GWPu32* out_max_abs) {
  GWPu32 nonzero_count;
  GWPu32 max_abs;
  GWPBool has_coeffs;
  int prev_context;
  int prev_coeff_non_zero;
  int i;
  if (decoder == 0 || coeff_probs == 0 || coeffs == 0 || out_has_coeffs == 0 ||
      out_nonzero_count == 0 || out_max_abs == 0) {
    return GWP_STATUS_INVALID_PARAM;
  }
  if (plane >= GWP_VP8_COEFF_BLOCK_TYPES) return GWP_STATUS_INVALID_PARAM;
  if (first_coeff < 0 || first_coeff > 15) return GWP_STATUS_INVALID_PARAM;

  for (i = 0; i < 16; ++i) coeffs[i] = 0;
  nonzero_count = 0u;
  max_abs = 0u;
  has_coeffs = GWP_FALSE;
  prev_context = (has_left ? 1 : 0) + (has_above ? 1 : 0);
  if (prev_context > 2) prev_context = 2;
  prev_coeff_non_zero = 1;

  for (i = first_coeff; i < 16; ++i) {
    int token;
    int value;
    GWPStatusCode st;
    GWPu32 band;
    band = (GWPu32)kGWPVP8CoeffBands[i];
    st = GWPVP8ReadCoeffToken(decoder,
                              coeff_probs[plane][band][prev_context],
                              prev_coeff_non_zero ? GWP_TRUE : GWP_FALSE,
                              &token,
                              &value);
    if (st != GWP_STATUS_OK) return st;
    if (token_hist != 0 && token >= 0 && token < (int)GWP_VP8_TOKEN_COUNT) {
      token_hist[token] += 1u;
    }
    if (token == (int)GWP_VP8_TOKEN_EOB) break;
    if (token == (int)GWP_VP8_TOKEN_ZERO) {
      prev_context = 0;
      prev_coeff_non_zero = 0;
      continue;
    }

    coeffs[kGWPVP8ZigZag[i]] = value;
    has_coeffs = GWP_TRUE;
    prev_coeff_non_zero = 1;
    nonzero_count += 1u;
    if (value < 0) {
      GWPu32 abs_value = (GWPu32)(-value);
      if (abs_value > max_abs) max_abs = abs_value;
      prev_context = (abs_value == 1u) ? 1 : 2;
    } else {
      GWPu32 abs_value = (GWPu32)value;
      if (abs_value > max_abs) max_abs = abs_value;
      prev_context = (abs_value == 1u) ? 1 : 2;
    }
  }

  *out_has_coeffs = has_coeffs;
  *out_nonzero_count = nonzero_count;
  *out_max_abs = max_abs;
  return GWP_STATUS_OK;
}
