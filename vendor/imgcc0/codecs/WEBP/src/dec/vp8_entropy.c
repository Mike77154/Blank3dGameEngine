#include "vp8_entropy.h"

#include "vp8_probdata.h"
#include "vp8_bool.h"
#include "vp8_tree.h"
#include "../utils/common.h"

void GWPVP8InitEntropyDefaults(GWPVP8EntropyHeader* entropy,
                               GWPBool key_frame) {
  GWPu32 i;
  GWPu32 j;
  GWPu32 k;
  GWPu32 l;
  if (entropy == 0) return;
  GWPZero(entropy, (GWPu32)sizeof(*entropy));
  for (i = 0u; i < GWP_VP8_COEFF_BLOCK_TYPES; ++i) {
    for (j = 0u; j < GWP_VP8_COEFF_BANDS; ++j) {
      for (k = 0u; k < GWP_VP8_PREV_COEFF_CONTEXTS; ++k) {
        for (l = 0u; l < GWP_VP8_ENTROPY_NODES; ++l) {
          entropy->coeff_probs[i][j][k][l] = kGWPVP8DefaultCoeffProbs[i][j][k][l];
        }
      }
    }
  }
  if (key_frame) {
    for (i = 0u; i < 4u; ++i) entropy->y_mode_probs[i] = kGWPVP8KfYModeProbs[i];
    for (i = 0u; i < 3u; ++i) entropy->uv_mode_probs[i] = kGWPVP8KfUVModeProbs[i];
  }
}

GWPStatusCode GWPVP8ParseEntropyHeader(GWPVP8BoolDecoder* br,
                                       GWPBool key_frame,
                                       GWPVP8EntropyHeader* entropy) {
  GWPu32 i;
  GWPu32 j;
  GWPu32 k;
  GWPu32 l;
  GWPu32 bit;
  GWPu32 value;
  if (br == 0 || entropy == 0) return GWP_STATUS_INVALID_PARAM;
  GWPVP8InitEntropyDefaults(entropy, key_frame);

  for (i = 0u; i < GWP_VP8_COEFF_BLOCK_TYPES; ++i) {
    for (j = 0u; j < GWP_VP8_COEFF_BANDS; ++j) {
      for (k = 0u; k < GWP_VP8_PREV_COEFF_CONTEXTS; ++k) {
        for (l = 0u; l < GWP_VP8_ENTROPY_NODES; ++l) {
          if (!GWPVP8BoolGet(br, (int)kGWPVP8CoeffUpdateProbs[i][j][k][l], &bit)) {
            return GWP_STATUS_TRUNCATED_DATA;
          }
          if (bit) {
            if (!GWPVP8BoolGetUInt(br, 8, &value)) return GWP_STATUS_TRUNCATED_DATA;
            entropy->coeff_probs[i][j][k][l] = (GWPu8)value;
            ++entropy->coeff_update_count;
          }
        }
      }
    }
  }

  if (!GWPVP8BoolGetBit(br, &bit)) return GWP_STATUS_TRUNCATED_DATA;
  entropy->coeff_skip_enabled = bit ? GWP_TRUE : GWP_FALSE;
  if (entropy->coeff_skip_enabled) {
    if (!GWPVP8BoolGetUInt(br, 8, &value)) return GWP_STATUS_TRUNCATED_DATA;
    entropy->coeff_skip_prob = (GWPu8)value;
  }

  if (!key_frame) {
    for (i = 0u; i < 4u; ++i) {
      if (!GWPVP8BoolGetBit(br, &bit)) return GWP_STATUS_TRUNCATED_DATA;
      if (bit) {
        if (!GWPVP8BoolGetUInt(br, 8, &value)) return GWP_STATUS_TRUNCATED_DATA;
        entropy->y_mode_probs[i] = (GWPu8)value;
      }
    }
    for (i = 0u; i < 3u; ++i) {
      if (!GWPVP8BoolGetBit(br, &bit)) return GWP_STATUS_TRUNCATED_DATA;
      if (bit) {
        if (!GWPVP8BoolGetUInt(br, 8, &value)) return GWP_STATUS_TRUNCATED_DATA;
        entropy->uv_mode_probs[i] = (GWPu8)value;
      }
    }
  }

  return GWP_STATUS_OK;
}
