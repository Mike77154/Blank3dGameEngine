#include "vp8_residual.h"

#include "vp8_modes.h"
#include "vp8_probdata.h"
#include "vp8_tokens.h"
#include "vp8_tree.h"
#include "../utils/common.h"

static int GWPVP8ResidualReadTree(const int* tree,
                                  const GWPu8* probs,
                                  int tree_size,
                                  GWPVP8BoolDecoder* br,
                                  GWPStatusCode* st) {
  int value;
  if (*st != GWP_STATUS_OK) return 0;
  if (!GWPVP8ReadTree(tree, probs, tree_size, br, &value)) {
    *st = GWP_STATUS_TRUNCATED_DATA;
    return 0;
  }
  return value;
}

static GWPStatusCode GWPVP8ReadSegmentId(const GWPVP8SegmentHeader* segment,
                                         GWPVP8BoolDecoder* br,
                                         GWPu32* segment_id) {
  int value;
  if (segment == 0 || br == 0 || segment_id == 0) return GWP_STATUS_INVALID_PARAM;
  if (!(segment->enabled && segment->update_map)) {
    *segment_id = 0u;
    return GWP_STATUS_OK;
  }
  if (!GWPVP8ReadTree(kGWPVP8MbSegmentTree, segment->tree_probs, 6, br, &value)) {
    return GWP_STATUS_TRUNCATED_DATA;
  }
  if (value < 0 || value >= (int)GWP_VP8_MAX_SEGMENTS) return GWP_STATUS_PARSE_ERROR;
  *segment_id = (GWPu32)value;
  return GWP_STATUS_OK;
}

static GWPStatusCode GWPVP8ReadSkip(const GWPVP8EntropyHeader* entropy,
                                    GWPVP8BoolDecoder* br,
                                    GWPBool* skip) {
  GWPu32 bit;
  if (entropy == 0 || br == 0 || skip == 0) return GWP_STATUS_INVALID_PARAM;
  *skip = GWP_FALSE;
  if (!entropy->coeff_skip_enabled) return GWP_STATUS_OK;
  if (!GWPVP8BoolGet(br, (int)entropy->coeff_skip_prob, &bit)) return GWP_STATUS_TRUNCATED_DATA;
  *skip = bit ? GWP_TRUE : GWP_FALSE;
  return GWP_STATUS_OK;
}

static GWPStatusCode GWPVP8ReadModes(const GWPVP8EntropyHeader* entropy,
                                     GWPVP8BoolDecoder* br,
                                     GWPu8* above_pred,
                                     GWPu8 left_pred[4],
                                     GWPu32 col_base,
                                     int* out_y_mode,
                                     int* out_uv_mode) {
  GWPStatusCode st;
  int y_mode;
  int uv_mode;
  GWPu8 cur_modes[16];
  GWPu32 row;
  GWPu32 col;
  if (entropy == 0 || br == 0 || above_pred == 0 || left_pred == 0 || out_y_mode == 0 || out_uv_mode == 0) {
    return GWP_STATUS_INVALID_PARAM;
  }
  st = GWP_STATUS_OK;
  y_mode = GWPVP8ResidualReadTree(kGWPVP8KfYModeTree,
                                  entropy->y_mode_probs,
                                  8,
                                  br,
                                  &st);
  if (st != GWP_STATUS_OK) return st;
  if (y_mode < 0 || y_mode >= (int)GWP_VP8_Y_MODE_COUNT) return GWP_STATUS_PARSE_ERROR;

  if (y_mode == (int)GWP_VP8_YMODE_B_PRED) {
    for (row = 0u; row < 4u; ++row) {
      for (col = 0u; col < 4u; ++col) {
        GWPu8 a;
        GWPu8 l;
        int bmode;
        if (row == 0u) a = above_pred[col_base + col];
        else a = cur_modes[(row - 1u) * 4u + col];
        if (col == 0u) l = left_pred[row];
        else l = cur_modes[row * 4u + (col - 1u)];
        bmode = GWPVP8ResidualReadTree(kGWPVP8BModeTree,
                                       kGWPVP8KfBModeProbs[a][l],
                                       18,
                                       br,
                                       &st);
        if (st != GWP_STATUS_OK) return st;
        if (bmode < 0 || bmode >= (int)GWP_VP8_B_MODE_COUNT) return GWP_STATUS_PARSE_ERROR;
        cur_modes[row * 4u + col] = (GWPu8)bmode;
      }
    }
  } else {
    GWPu8 fill_mode;
    fill_mode = kGWPVP8YModeToBMode[y_mode];
    for (row = 0u; row < 16u; ++row) cur_modes[row] = fill_mode;
  }

  uv_mode = GWPVP8ResidualReadTree(kGWPVP8UVModeTree,
                                   entropy->uv_mode_probs,
                                   6,
                                   br,
                                   &st);
  if (st != GWP_STATUS_OK) return st;
  if (uv_mode < 0 || uv_mode >= (int)GWP_VP8_UV_MODE_COUNT) return GWP_STATUS_PARSE_ERROR;

  for (col = 0u; col < 4u; ++col) {
    above_pred[col_base + col] = cur_modes[12u + col];
    left_pred[col] = cur_modes[col * 4u + 3u];
  }
  *out_y_mode = y_mode;
  *out_uv_mode = uv_mode;
  return GWP_STATUS_OK;
}

static GWPStatusCode GWPVP8DecodeOneBlock(GWPVP8TokenDecoder* decoder,
                                          const GWPVP8EntropyHeader* entropy,
                                          GWPu32 plane,
                                          GWPBool has_left,
                                          GWPBool has_above,
                                          int first_coeff,
                                          GWPVP8ResidualSummary* summary,
                                          GWPBool* out_has_coeffs) {
  int coeffs[16];
  GWPu32 i;
  GWPu32 nonzero_count;
  GWPu32 max_abs;
  GWPStatusCode st;
  if (decoder == 0 || entropy == 0 || summary == 0 || out_has_coeffs == 0) return GWP_STATUS_INVALID_PARAM;
  for (i = 0u; i < 16u; ++i) coeffs[i] = 0;
  nonzero_count = 0u;
  max_abs = 0u;
  st = GWPVP8ReadCoeffBlock(decoder,
                            entropy->coeff_probs,
                            plane,
                            has_left,
                            has_above,
                            first_coeff,
                            coeffs,
                            out_has_coeffs,
                            summary->token_hist,
                            &nonzero_count,
                            &max_abs);
  if (st != GWP_STATUS_OK) return st;
  summary->coeff_block_count += 1u;
  if (*out_has_coeffs) {
    summary->blocks_with_coeffs += 1u;
    summary->nonzero_coeff_count += nonzero_count;
    if (max_abs > summary->max_abs_coeff) summary->max_abs_coeff = max_abs;
  }
  return GWP_STATUS_OK;
}

GWPStatusCode GWPVP8ProbeResidualFromState(const GWPVP8ControlHeader* header,
                                           GWPVP8BoolDecoder* mode_br,
                                           GWPVP8ResidualSummary* summary) {
  GWPStatusCode st;
  GWPVP8TokenDecoderSet token_decoders;
  GWPu8 above_pred[GWP_VP8_MAX_MB_COLS * 4u];
  GWPu8 above_ctx[GWP_VP8_MAX_MB_COLS * 9u];
  GWPu8 left_pred[4];
  GWPu8 left_ctx[9];
  GWPu32 mb_y;
  if (header == 0 || mode_br == 0 || summary == 0) return GWP_STATUS_INVALID_PARAM;
  if (header->mb_cols == 0u || header->mb_cols > GWP_VP8_MAX_MB_COLS) return GWP_STATUS_LIMIT_EXCEEDED;

  st = GWPVP8InitTokenDecoders(&header->token_partitions, &token_decoders);
  if (st != GWP_STATUS_OK) return st;

  GWPZero(summary, (GWPu32)sizeof(*summary));
  for (mb_y = 0u; mb_y < header->mb_cols * 4u; ++mb_y) above_pred[mb_y] = (GWPu8)GWP_VP8_BMODE_DC;
  for (mb_y = 0u; mb_y < header->mb_cols * 9u; ++mb_y) above_ctx[mb_y] = 0u;

  for (mb_y = 0u; mb_y < header->mb_rows; ++mb_y) {
    GWPu32 mb_x;
    GWPVP8TokenDecoder* token_decoder;
    GWPu32 token_part_index;
    token_part_index = mb_y & (token_decoders.count - 1u);
    token_decoder = &token_decoders.decoder[token_part_index];
    for (mb_x = 0u; mb_x < 4u; ++mb_x) left_pred[mb_x] = (GWPu8)GWP_VP8_BMODE_DC;
    for (mb_x = 0u; mb_x < 9u; ++mb_x) left_ctx[mb_x] = 0u;

    for (mb_x = 0u; mb_x < header->mb_cols; ++mb_x) {
      GWPu32 segment_id;
      GWPBool skip_coeff;
      int y_mode;
      int uv_mode;
      GWPu32 col_base;
      GWPBool has_y2;
      GWPBool y2_has_coeffs;
      GWPu32 block_index;
      summary->macroblock_count += 1u;
      col_base = mb_x * 4u;
      st = GWPVP8ReadSegmentId(&header->segment, mode_br, &segment_id);
      if (st != GWP_STATUS_OK) return st;
      (void)segment_id;
      st = GWPVP8ReadSkip(&header->entropy, mode_br, &skip_coeff);
      if (st != GWP_STATUS_OK) return st;
      if (skip_coeff) summary->skipped_macroblocks += 1u;
      st = GWPVP8ReadModes(&header->entropy,
                           mode_br,
                           above_pred,
                           left_pred,
                           col_base,
                           &y_mode,
                           &uv_mode);
      if (st != GWP_STATUS_OK) return st;
      (void)uv_mode;
      has_y2 = (y_mode != (int)GWP_VP8_YMODE_B_PRED) ? GWP_TRUE : GWP_FALSE;
      if (has_y2) summary->y2_macroblocks += 1u;
      y2_has_coeffs = GWP_FALSE;

      if (skip_coeff) {
        summary->coeff_block_count += 24u + (has_y2 ? 1u : 0u);
        for (block_index = 0u; block_index < 24u; ++block_index) {
          GWPu32 l = kGWPVP8LeftContextIndex[block_index];
          GWPu32 a = kGWPVP8AboveContextIndex[block_index];
          left_ctx[l] = 0u;
          above_ctx[mb_x * 9u + a] = 0u;
        }
        if (has_y2) {
          left_ctx[8] = 0u;
          above_ctx[mb_x * 9u + 8u] = 0u;
        }
        continue;
      }

      if (has_y2) {
        st = GWPVP8DecodeOneBlock(token_decoder,
                                  &header->entropy,
                                  1u,
                                  left_ctx[8] ? GWP_TRUE : GWP_FALSE,
                                  above_ctx[mb_x * 9u + 8u] ? GWP_TRUE : GWP_FALSE,
                                  0,
                                  summary,
                                  &y2_has_coeffs);
        if (st != GWP_STATUS_OK) return st;
        if (y2_has_coeffs) summary->y2_blocks_with_coeffs += 1u;
        left_ctx[8] = y2_has_coeffs ? 1u : 0u;
        above_ctx[mb_x * 9u + 8u] = y2_has_coeffs ? 1u : 0u;
      }

      for (block_index = 0u; block_index < 16u; ++block_index) {
        GWPu32 l = kGWPVP8LeftContextIndex[block_index];
        GWPu32 a = kGWPVP8AboveContextIndex[block_index];
        GWPBool has_coeffs;
        st = GWPVP8DecodeOneBlock(token_decoder,
                                  &header->entropy,
                                  has_y2 ? 0u : 3u,
                                  left_ctx[l] ? GWP_TRUE : GWP_FALSE,
                                  above_ctx[mb_x * 9u + a] ? GWP_TRUE : GWP_FALSE,
                                  has_y2 ? 1 : 0,
                                  summary,
                                  &has_coeffs);
        if (st != GWP_STATUS_OK) return st;
        if (has_coeffs) summary->y_blocks_with_coeffs += 1u;
        left_ctx[l] = has_coeffs ? 1u : 0u;
        above_ctx[mb_x * 9u + a] = has_coeffs ? 1u : 0u;
      }
      for (block_index = 16u; block_index < 24u; ++block_index) {
        GWPu32 l = kGWPVP8LeftContextIndex[block_index];
        GWPu32 a = kGWPVP8AboveContextIndex[block_index];
        GWPBool has_coeffs;
        st = GWPVP8DecodeOneBlock(token_decoder,
                                  &header->entropy,
                                  2u,
                                  left_ctx[l] ? GWP_TRUE : GWP_FALSE,
                                  above_ctx[mb_x * 9u + a] ? GWP_TRUE : GWP_FALSE,
                                  0,
                                  summary,
                                  &has_coeffs);
        if (st != GWP_STATUS_OK) return st;
        if (has_coeffs) summary->uv_blocks_with_coeffs += 1u;
        left_ctx[l] = has_coeffs ? 1u : 0u;
        above_ctx[mb_x * 9u + a] = has_coeffs ? 1u : 0u;
      }
    }
  }

  summary->token_partition_count = token_decoders.count;
  for (mb_y = 0u; mb_y < token_decoders.count; ++mb_y) {
    summary->token_partition_bytes_touched[mb_y] = GWPVP8BoolBytesTouched(&token_decoders.decoder[mb_y].br);
  }
  return GWP_STATUS_OK;
}
