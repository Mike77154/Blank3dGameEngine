#include "vp8_modes.h"

#include "vp8_probdata.h"
#include "vp8_tree.h"
#include "../utils/common.h"

#define GWP_VP8_MAX_MB_COLS (GWP_MAX_IMAGE_WIDTH / 16u)
#define GWP_VP8_MAX_SUBBLOCK_COLS (GWP_VP8_MAX_MB_COLS * 4u)

static int GWPVP8SafeReadTree(const int* tree,
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

static GWPStatusCode GWPVP8ReadSegmentIdInternal(const GWPVP8SegmentHeader* segment,
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

static GWPStatusCode GWPVP8ReadSkipInternal(const GWPVP8EntropyHeader* entropy,
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

static GWPStatusCode GWPVP8ReadModesInternal(const GWPVP8EntropyHeader* entropy,
                                             GWPVP8BoolDecoder* br,
                                             GWPu8* above_pred,
                                             GWPu8 left_pred[4],
                                             GWPu32 col_base,
                                             int* out_y_mode,
                                             int* out_uv_mode,
                                             GWPu8 out_b_modes[16]) {
  GWPStatusCode st;
  int y_mode;
  int uv_mode;
  GWPu8 cur_modes[16];
  GWPu32 row;
  GWPu32 col;
  if (entropy == 0 || br == 0 || above_pred == 0 || left_pred == 0 ||
      out_y_mode == 0 || out_uv_mode == 0 || out_b_modes == 0) {
    return GWP_STATUS_INVALID_PARAM;
  }
  st = GWP_STATUS_OK;
  y_mode = GWPVP8SafeReadTree(kGWPVP8KfYModeTree,
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
        bmode = GWPVP8SafeReadTree(kGWPVP8BModeTree,
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

  uv_mode = GWPVP8SafeReadTree(kGWPVP8UVModeTree,
                               entropy->uv_mode_probs,
                               6,
                               br,
                               &st);
  if (st != GWP_STATUS_OK) return st;
  if (uv_mode < 0 || uv_mode >= (int)GWP_VP8_UV_MODE_COUNT) return GWP_STATUS_PARSE_ERROR;

  for (row = 0u; row < 16u; ++row) out_b_modes[row] = cur_modes[row];
  for (col = 0u; col < 4u; ++col) {
    above_pred[col_base + col] = cur_modes[12u + col];
    left_pred[col] = cur_modes[col * 4u + 3u];
  }
  *out_y_mode = y_mode;
  *out_uv_mode = uv_mode;
  return GWP_STATUS_OK;
}

GWPStatusCode GWPVP8ReadKeyFrameMacroblockHeader(
    GWPVP8BoolDecoder* br,
    const GWPVP8SegmentHeader* segment,
    const GWPVP8EntropyHeader* entropy,
    GWPu8* above_pred,
    GWPu8 left_pred[4],
    GWPu32 col_base,
    GWPu32* out_segment_id,
    GWPBool* out_skip_coeff,
    int* out_y_mode,
    int* out_uv_mode,
    GWPu8 out_b_modes[16]) {
  GWPStatusCode st;
  if (br == 0 || segment == 0 || entropy == 0 || above_pred == 0 || left_pred == 0 ||
      out_segment_id == 0 || out_skip_coeff == 0 || out_y_mode == 0 || out_uv_mode == 0 ||
      out_b_modes == 0) {
    return GWP_STATUS_INVALID_PARAM;
  }
  st = GWPVP8ReadSegmentIdInternal(segment, br, out_segment_id);
  if (st != GWP_STATUS_OK) return st;
  st = GWPVP8ReadSkipInternal(entropy, br, out_skip_coeff);
  if (st != GWP_STATUS_OK) return st;
  return GWPVP8ReadModesInternal(entropy,
                                 br,
                                 above_pred,
                                 left_pred,
                                 col_base,
                                 out_y_mode,
                                 out_uv_mode,
                                 out_b_modes);
}

GWPStatusCode GWPVP8ParseKeyFrameModeSummary(GWPVP8BoolDecoder* br,
                                             const GWPVP8SegmentHeader* segment,
                                             const GWPVP8EntropyHeader* entropy,
                                             GWPu32 mb_cols,
                                             GWPu32 mb_rows,
                                             GWPVP8ModeSummary* summary) {
  GWPu8 above_pred[GWP_VP8_MAX_SUBBLOCK_COLS];
  GWPu8 left_pred[4];
  GWPu8 b_modes[16];
  GWPu32 mb_y;
  GWPu32 mb_x;
  if (br == 0 || segment == 0 || entropy == 0 || summary == 0) {
    return GWP_STATUS_INVALID_PARAM;
  }
  if (mb_cols == 0u || mb_rows == 0u) return GWP_STATUS_PARSE_ERROR;
  if (mb_cols > GWP_VP8_MAX_MB_COLS) return GWP_STATUS_LIMIT_EXCEEDED;

  GWPZero(summary, (GWPu32)sizeof(*summary));
  {
    GWPu32 i;
    for (i = 0u; i < mb_cols * 4u; ++i) above_pred[i] = (GWPu8)GWP_VP8_BMODE_DC;
  }

  for (mb_y = 0u; mb_y < mb_rows; ++mb_y) {
    for (mb_x = 0u; mb_x < 4u; ++mb_x) left_pred[mb_x] = (GWPu8)GWP_VP8_BMODE_DC;
    for (mb_x = 0u; mb_x < mb_cols; ++mb_x) {
      GWPu32 segment_id;
      GWPBool skip_coeff;
      int y_mode;
      int uv_mode;
      GWPStatusCode st;
      GWPu32 i;
      summary->macroblock_count += 1u;
      st = GWPVP8ReadKeyFrameMacroblockHeader(br,
                                              segment,
                                              entropy,
                                              above_pred,
                                              left_pred,
                                              mb_x * 4u,
                                              &segment_id,
                                              &skip_coeff,
                                              &y_mode,
                                              &uv_mode,
                                              b_modes);
      if (st != GWP_STATUS_OK) return st;
      summary->segment_hist[segment_id] += 1u;
      if (skip_coeff) summary->skipped_macroblocks += 1u;
      summary->y_mode_hist[y_mode] += 1u;
      summary->uv_mode_hist[uv_mode] += 1u;
      if (y_mode == (int)GWP_VP8_YMODE_B_PRED) {
        summary->bpred_macroblocks += 1u;
        for (i = 0u; i < 16u; ++i) {
          if (b_modes[i] >= GWP_VP8_B_MODE_COUNT) return GWP_STATUS_PARSE_ERROR;
          summary->b_mode_hist[b_modes[i]] += 1u;
        }
      }
    }
  }
  return GWP_STATUS_OK;
}
