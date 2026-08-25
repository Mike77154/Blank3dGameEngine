#include "vp8_dec.h"

#include "vp8_entropy.h"
#include "vp8_modes.h"
#include "vp8_picture.h"
#include "vp8_probdata.h"
#include "vp8_quant.h"
#include "vp8_recon.h"
#include "vp8_residual.h"
#include "vp8_tokens.h"
#include "vp8_transform.h"
#include "../utils/arena.h"
#include "../utils/common.h"
#include "../utils/endian.h"

static GWPStatusCode GWPVP8ReadBitOrTruncated(GWPVP8BoolDecoder* br,
                                              GWPu32* out_bit) {
  if (!GWPVP8BoolGetBit(br, out_bit)) return GWP_STATUS_TRUNCATED_DATA;
  return GWP_STATUS_OK;
}

static GWPStatusCode GWPVP8ReadUIntOrTruncated(GWPVP8BoolDecoder* br,
                                               int bits,
                                               GWPu32* out_value) {
  if (!GWPVP8BoolGetUInt(br, bits, out_value)) return GWP_STATUS_TRUNCATED_DATA;
  return GWP_STATUS_OK;
}

static GWPStatusCode GWPVP8ReadMaybeIntOrTruncated(GWPVP8BoolDecoder* br,
                                                   int bits,
                                                   int* out_value) {
  if (!GWPVP8BoolMaybeGetInt(br, bits, out_value)) return GWP_STATUS_TRUNCATED_DATA;
  return GWP_STATUS_OK;
}

static GWPStatusCode GWPVP8ParseSegmentationHeader(GWPVP8BoolDecoder* br,
                                                   GWPVP8SegmentHeader* seg) {
  GWPStatusCode st;
  GWPu32 bit;
  GWPu32 i;
  for (i = 0u; i < GWP_VP8_MB_FEATURE_TREE_PROBS; ++i) seg->tree_probs[i] = 255u;
  if (br == 0 || seg == 0) return GWP_STATUS_INVALID_PARAM;
  st = GWPVP8ReadBitOrTruncated(br, &bit);
  if (st != GWP_STATUS_OK) return st;
  seg->enabled = bit ? GWP_TRUE : GWP_FALSE;
  if (!seg->enabled) {
    seg->update_map = GWP_FALSE;
    seg->update_data = GWP_FALSE;
    return GWP_STATUS_OK;
  }

  st = GWPVP8ReadBitOrTruncated(br, &bit);
  if (st != GWP_STATUS_OK) return st;
  seg->update_map = bit ? GWP_TRUE : GWP_FALSE;

  st = GWPVP8ReadBitOrTruncated(br, &bit);
  if (st != GWP_STATUS_OK) return st;
  seg->update_data = bit ? GWP_TRUE : GWP_FALSE;

  if (seg->update_data) {
    st = GWPVP8ReadBitOrTruncated(br, &bit);
    if (st != GWP_STATUS_OK) return st;
    seg->absolute_delta = bit ? GWP_TRUE : GWP_FALSE;
    for (i = 0u; i < GWP_VP8_MAX_SEGMENTS; ++i) {
      st = GWPVP8ReadMaybeIntOrTruncated(br, 7, &seg->quant_idx[i]);
      if (st != GWP_STATUS_OK) return st;
    }
    for (i = 0u; i < GWP_VP8_MAX_SEGMENTS; ++i) {
      st = GWPVP8ReadMaybeIntOrTruncated(br, 6, &seg->lf_level[i]);
      if (st != GWP_STATUS_OK) return st;
    }
  }

  if (seg->update_map) {
    for (i = 0u; i < GWP_VP8_MB_FEATURE_TREE_PROBS; ++i) {
      st = GWPVP8ReadBitOrTruncated(br, &bit);
      if (st != GWP_STATUS_OK) return st;
      if (bit) {
        GWPu32 prob;
        st = GWPVP8ReadUIntOrTruncated(br, 8, &prob);
        if (st != GWP_STATUS_OK) return st;
        seg->tree_probs[i] = (GWPu8)prob;
      } else {
        seg->tree_probs[i] = 255u;
      }
    }
  }

  return GWP_STATUS_OK;
}

static GWPStatusCode GWPVP8ParseLoopFilterHeader(GWPVP8BoolDecoder* br,
                                                 GWPVP8LoopFilterHeader* lf) {
  GWPStatusCode st;
  GWPu32 bit;
  GWPu32 value;
  GWPu32 i;
  if (br == 0 || lf == 0) return GWP_STATUS_INVALID_PARAM;

  st = GWPVP8ReadBitOrTruncated(br, &bit);
  if (st != GWP_STATUS_OK) return st;
  lf->use_simple = bit ? GWP_TRUE : GWP_FALSE;

  st = GWPVP8ReadUIntOrTruncated(br, 6, &value);
  if (st != GWP_STATUS_OK) return st;
  lf->level = (GWPu8)value;

  st = GWPVP8ReadUIntOrTruncated(br, 3, &value);
  if (st != GWP_STATUS_OK) return st;
  lf->sharpness = (GWPu8)value;

  st = GWPVP8ReadBitOrTruncated(br, &bit);
  if (st != GWP_STATUS_OK) return st;
  lf->delta_enabled = bit ? GWP_TRUE : GWP_FALSE;
  if (!lf->delta_enabled) return GWP_STATUS_OK;

  st = GWPVP8ReadBitOrTruncated(br, &bit);
  if (st != GWP_STATUS_OK) return st;
  if (!bit) return GWP_STATUS_OK;

  for (i = 0u; i < GWP_VP8_BLOCK_CONTEXTS; ++i) {
    st = GWPVP8ReadMaybeIntOrTruncated(br, 6, &lf->ref_delta[i]);
    if (st != GWP_STATUS_OK) return st;
  }
  for (i = 0u; i < GWP_VP8_BLOCK_CONTEXTS; ++i) {
    st = GWPVP8ReadMaybeIntOrTruncated(br, 6, &lf->mode_delta[i]);
    if (st != GWP_STATUS_OK) return st;
  }
  return GWP_STATUS_OK;
}

static GWPStatusCode GWPVP8ParseTokenPartitions(GWPVP8BoolDecoder* br,
                                                const GWPu8* data,
                                                GWPu32 data_size,
                                                GWPVP8TokenPartitions* tokens) {
  GWPStatusCode st;
  GWPu32 log2_count;
  GWPu32 count;
  GWPu32 table_bytes;
  GWPu32 remaining;
  const GWPu8* table_ptr;
  const GWPu8* partition_data;
  GWPu32 i;

  if (br == 0 || data == 0 || tokens == 0) return GWP_STATUS_INVALID_PARAM;
  st = GWPVP8ReadUIntOrTruncated(br, 2, &log2_count);
  if (st != GWP_STATUS_OK) return st;
  count = 1u << log2_count;
  if (count == 0u || count > GWP_VP8_MAX_TOKEN_PARTITIONS) {
    return GWP_STATUS_PARSE_ERROR;
  }

  table_bytes = 3u * (count - 1u);
  if (data_size < table_bytes) return GWP_STATUS_TRUNCATED_DATA;

  tokens->partition_count = count;
  table_ptr = data;
  partition_data = data + table_bytes;
  remaining = data_size - table_bytes;

  for (i = 0u; i < count; ++i) {
    GWPu32 part_size;
    if (i < count - 1u) {
      part_size = GWPReadLE24(table_ptr);
      table_ptr += 3;
    } else {
      part_size = remaining;
    }
    if (remaining < part_size) return GWP_STATUS_TRUNCATED_DATA;
    tokens->partition[i].bytes = partition_data;
    tokens->partition[i].size = part_size;
    partition_data += part_size;
    remaining -= part_size;
  }
  return GWP_STATUS_OK;
}

static GWPStatusCode GWPVP8ParseQuantHeader(GWPVP8BoolDecoder* br,
                                            GWPVP8QuantHeader* quant) {
  GWPStatusCode st;
  GWPu32 value;
  if (br == 0 || quant == 0) return GWP_STATUS_INVALID_PARAM;

  st = GWPVP8ReadUIntOrTruncated(br, 7, &value);
  if (st != GWP_STATUS_OK) return st;
  quant->q_index = (GWPu8)value;

  st = GWPVP8ReadMaybeIntOrTruncated(br, 4, &quant->y1_dc_delta_q);
  if (st != GWP_STATUS_OK) return st;
  st = GWPVP8ReadMaybeIntOrTruncated(br, 4, &quant->y2_dc_delta_q);
  if (st != GWP_STATUS_OK) return st;
  st = GWPVP8ReadMaybeIntOrTruncated(br, 4, &quant->y2_ac_delta_q);
  if (st != GWP_STATUS_OK) return st;
  st = GWPVP8ReadMaybeIntOrTruncated(br, 4, &quant->uv_dc_delta_q);
  if (st != GWP_STATUS_OK) return st;
  st = GWPVP8ReadMaybeIntOrTruncated(br, 4, &quant->uv_ac_delta_q);
  if (st != GWP_STATUS_OK) return st;

  quant->delta_update = (quant->y1_dc_delta_q != 0 ||
                         quant->y2_dc_delta_q != 0 ||
                         quant->y2_ac_delta_q != 0 ||
                         quant->uv_dc_delta_q != 0 ||
                         quant->uv_ac_delta_q != 0) ? GWP_TRUE : GWP_FALSE;
  return GWP_STATUS_OK;
}

static GWPStatusCode GWPVP8ParseReferenceHeader(GWPVP8BoolDecoder* br,
                                                GWPBool key_frame,
                                                GWPVP8ReferenceHeader* ref) {
  GWPStatusCode st;
  GWPu32 bit;
  GWPu32 value;
  if (br == 0 || ref == 0) return GWP_STATUS_INVALID_PARAM;
  if (key_frame) {
    ref->refresh_gf = GWP_TRUE;
    ref->refresh_arf = GWP_TRUE;
    ref->copy_gf = 0u;
    ref->copy_arf = 0u;
    ref->sign_bias_golden = 0u;
    ref->sign_bias_altref = 0u;
    st = GWPVP8ReadBitOrTruncated(br, &bit);
    if (st != GWP_STATUS_OK) return st;
    ref->refresh_entropy = bit ? GWP_TRUE : GWP_FALSE;
    ref->refresh_last = GWP_TRUE;
    return GWP_STATUS_OK;
  }

  st = GWPVP8ReadBitOrTruncated(br, &bit);
  if (st != GWP_STATUS_OK) return st;
  ref->refresh_gf = bit ? GWP_TRUE : GWP_FALSE;
  st = GWPVP8ReadBitOrTruncated(br, &bit);
  if (st != GWP_STATUS_OK) return st;
  ref->refresh_arf = bit ? GWP_TRUE : GWP_FALSE;

  if (!ref->refresh_gf) {
    st = GWPVP8ReadUIntOrTruncated(br, 2, &value);
    if (st != GWP_STATUS_OK) return st;
    ref->copy_gf = (GWPu8)value;
  }
  if (!ref->refresh_arf) {
    st = GWPVP8ReadUIntOrTruncated(br, 2, &value);
    if (st != GWP_STATUS_OK) return st;
    ref->copy_arf = (GWPu8)value;
  }

  st = GWPVP8ReadBitOrTruncated(br, &bit);
  if (st != GWP_STATUS_OK) return st;
  ref->sign_bias_golden = (GWPu8)bit;
  st = GWPVP8ReadBitOrTruncated(br, &bit);
  if (st != GWP_STATUS_OK) return st;
  ref->sign_bias_altref = (GWPu8)bit;
  st = GWPVP8ReadBitOrTruncated(br, &bit);
  if (st != GWP_STATUS_OK) return st;
  ref->refresh_entropy = bit ? GWP_TRUE : GWP_FALSE;
  st = GWPVP8ReadBitOrTruncated(br, &bit);
  if (st != GWP_STATUS_OK) return st;
  ref->refresh_last = bit ? GWP_TRUE : GWP_FALSE;
  return GWP_STATUS_OK;
}

GWPStatusCode GWPVP8ParseControlHeaderWithModeState(const GWPu8* data,
                                                    GWPu32 data_size,
                                                    GWPVP8ControlHeader* header,
                                                    GWPVP8BoolDecoder* out_mode_br) {
  GWPStatusCode st;
  GWPVP8BoolDecoder br;
  GWPVP8BoolDecoder mode_br;
  const GWPu8* token_data;
  GWPu32 token_data_size;
  GWPu32 offset;
  GWPu32 bit;
  if (data == 0 || header == 0) return GWP_STATUS_INVALID_PARAM;
  GWPZero(header, (GWPu32)sizeof(*header));

  st = GWPVP8ParseFrameHeader(data, data_size, &header->frame);
  if (st != GWP_STATUS_OK) return st;
  if (!header->frame.tag.key_frame) return GWP_STATUS_UNSUPPORTED_FEATURE;

  header->mb_cols = (header->frame.key.width + 15u) >> 4;
  header->mb_rows = (header->frame.key.height + 15u) >> 4;

  GWPVP8BoolInit(&br,
                 header->frame.partitions.first_partition.bytes,
                 header->frame.partitions.first_partition.size);
  if (GWPVP8BoolHasError(&br)) return GWP_STATUS_TRUNCATED_DATA;

  st = GWPVP8ReadBitOrTruncated(&br, &bit);
  if (st != GWP_STATUS_OK) return st;
  header->color_space = (GWPu8)bit;
  st = GWPVP8ReadBitOrTruncated(&br, &bit);
  if (st != GWP_STATUS_OK) return st;
  header->clamping_type = (GWPu8)bit;
  if (header->color_space != 0u) return GWP_STATUS_UNSUPPORTED_FEATURE;

  st = GWPVP8ParseSegmentationHeader(&br, &header->segment);
  if (st != GWP_STATUS_OK) return st;
  st = GWPVP8ParseLoopFilterHeader(&br, &header->loop_filter);
  if (st != GWP_STATUS_OK) return st;
  header->control_header_bytes_touched = GWPVP8BoolBytesTouched(&br);

  offset = (GWPu32)(header->frame.partitions.first_partition.bytes - data);
  if (offset > data_size) return GWP_STATUS_TRUNCATED_DATA;
  offset += header->frame.partitions.first_partition.size;
  if (offset > data_size) return GWP_STATUS_TRUNCATED_DATA;
  token_data = data + offset;
  token_data_size = data_size - offset;

  st = GWPVP8ParseTokenPartitions(&br,
                                  token_data,
                                  token_data_size,
                                  &header->token_partitions);
  if (st != GWP_STATUS_OK) return st;
  st = GWPVP8ParseQuantHeader(&br, &header->quant);
  if (st != GWP_STATUS_OK) return st;
  st = GWPVP8ParseReferenceHeader(&br,
                                  header->frame.tag.key_frame,
                                  &header->reference);
  if (st != GWP_STATUS_OK) return st;

  GWPVP8InitDequantFactors(&header->segment,
                           &header->quant,
                           header->dequant);

  st = GWPVP8ParseEntropyHeader(&br,
                                header->frame.tag.key_frame,
                                &header->entropy);
  if (st != GWP_STATUS_OK) return st;
  header->entropy_header_bytes_touched = GWPVP8BoolBytesTouched(&br);

  mode_br = br;
  if (out_mode_br != 0) *out_mode_br = mode_br;

  st = GWPVP8ParseKeyFrameModeSummary(&br,
                                      &header->segment,
                                      &header->entropy,
                                      header->mb_cols,
                                      header->mb_rows,
                                      &header->mode_summary);
  if (st != GWP_STATUS_OK) {
    if (out_mode_br != 0) return GWP_STATUS_OK;
    return st;
  }
  header->part0_bytes_touched = GWPVP8BoolBytesTouched(&br);

  {
    GWPu32 total_token_bytes;
    GWPu32 i;
    total_token_bytes = 0u;
    for (i = 0u; i < header->token_partitions.partition_count; ++i) {
      total_token_bytes += header->token_partitions.partition[i].size;
    }
    if (total_token_bytes == 0u) {
      header->residual_summary.macroblock_count = header->mode_summary.macroblock_count;
      header->residual_summary.skipped_macroblocks = header->mode_summary.skipped_macroblocks;
      header->residual_summary.y2_macroblocks =
          header->mode_summary.macroblock_count - header->mode_summary.bpred_macroblocks;
      header->residual_summary.token_partition_count = header->token_partitions.partition_count;
      header->residual_summary.coeff_block_count =
          header->mode_summary.macroblock_count * 24u +
          header->residual_summary.y2_macroblocks;
      return GWP_STATUS_OK;
    }
  }

  st = GWPVP8ProbeResidualFromState(header, &mode_br, &header->residual_summary);
  if (st != GWP_STATUS_OK) {
    if (out_mode_br != 0) return GWP_STATUS_OK;
    return st;
  }
  return GWP_STATUS_OK;
}

GWPStatusCode GWPVP8ParseControlHeader(const GWPu8* data,
                                       GWPu32 data_size,
                                       GWPVP8ControlHeader* header) {
  return GWPVP8ParseControlHeaderWithModeState(data, data_size, header, 0);
}


static void GWPVP8GetAboveSamples(const GWPu8* plane,
                                  GWPu32 stride,
                                  GWPu32 valid_width,
                                  GWPu32 x,
                                  GWPu32 y,
                                  GWPu32 count,
                                  GWPu8 default_value,
                                  GWPu8* out) {
  GWPu32 i;
  if (out == 0) return;
  if (plane == 0 || y == 0u || valid_width == 0u) {
    for (i = 0u; i < count; ++i) out[i] = default_value;
    return;
  }
  for (i = 0u; i < count; ++i) {
    GWPu32 xi;
    xi = x + i;
    if (xi >= valid_width) xi = valid_width - 1u;
    out[i] = plane[(y - 1u) * stride + xi];
  }
}

static void GWPVP8GetLeftSamples(const GWPu8* plane,
                                 GWPu32 stride,
                                 GWPu32 valid_height,
                                 GWPu32 x,
                                 GWPu32 y,
                                 GWPu32 count,
                                 GWPu8 default_value,
                                 GWPu8* out) {
  GWPu32 i;
  if (out == 0) return;
  if (plane == 0 || x == 0u || valid_height == 0u) {
    for (i = 0u; i < count; ++i) out[i] = default_value;
    return;
  }
  for (i = 0u; i < count; ++i) {
    GWPu32 yi;
    yi = y + i;
    if (yi >= valid_height) yi = valid_height - 1u;
    out[i] = plane[yi * stride + (x - 1u)];
  }
}

static GWPu8 GWPVP8GetTopLeftSample(const GWPu8* plane,
                                    GWPu32 stride,
                                    GWPu32 valid_width,
                                    GWPu32 valid_height,
                                    GWPu32 x,
                                    GWPu32 y,
                                    GWPu8 default_value) {
  GWPu32 sx;
  GWPu32 sy;
  if (plane == 0 || x == 0u || y == 0u || valid_width == 0u || valid_height == 0u) return default_value;
  sx = x - 1u;
  sy = y - 1u;
  if (sx >= valid_width) sx = valid_width - 1u;
  if (sy >= valid_height) sy = valid_height - 1u;
  return plane[sy * stride + sx];
}

static void GWPVP8ZeroCoeffBlock(int coeffs[16]) {
  GWPu32 i;
  for (i = 0u; i < 16u; ++i) coeffs[i] = 0;
}

static void GWPVP8DequantizeBlockWithWalshDC(const int coeffs[16],
                                             int walsh_dc,
                                             int ac_q,
                                             int out[16]) {
  int i;
  if (coeffs == 0 || out == 0) return;
  out[0] = walsh_dc;
  for (i = 1; i < 16; ++i) out[i] = coeffs[i] * ac_q;
}


static GWPBool GWPVP8CoeffBlockHasNonZero(const int coeffs[16]) {
  GWPu32 i;
  if (coeffs == 0) return GWP_FALSE;
  for (i = 0u; i < 16u; ++i) {
    if (coeffs[i] != 0) return GWP_TRUE;
  }
  return GWP_FALSE;
}

static GWPBool GWPVP8MacroblockHasNonZeroCoeffs(GWPBool has_y2,
                                                const int y2_coeffs[16],
                                                int y_coeffs[16][16],
                                                int u_coeffs[4][16],
                                                int v_coeffs[4][16]) {
  GWPu32 i;
  if (has_y2 && GWPVP8CoeffBlockHasNonZero(y2_coeffs)) return GWP_TRUE;
  for (i = 0u; i < 16u; ++i) {
    if (GWPVP8CoeffBlockHasNonZero(y_coeffs[i])) return GWP_TRUE;
  }
  for (i = 0u; i < 4u; ++i) {
    if (GWPVP8CoeffBlockHasNonZero(u_coeffs[i])) return GWP_TRUE;
    if (GWPVP8CoeffBlockHasNonZero(v_coeffs[i])) return GWP_TRUE;
  }
  return GWP_FALSE;
}

static GWPStatusCode GWPVP8DecodeMacroblockTokens(
    const GWPVP8ControlHeader* header,
    GWPVP8TokenDecoder* token_decoder,
    GWPu8 left_ctx[9],
    GWPu8* above_ctx_mb,
    GWPBool skip_coeff,
    GWPBool has_y2,
    int y2_coeffs[16],
    int y_coeffs[16][16],
    int u_coeffs[4][16],
    int v_coeffs[4][16]) {
  GWPu32 i;
  GWPu32 nonzero_count;
  GWPu32 max_abs;
  if (header == 0 || left_ctx == 0 || above_ctx_mb == 0 || y2_coeffs == 0 ||
      y_coeffs == 0 || u_coeffs == 0 || v_coeffs == 0) {
    return GWP_STATUS_INVALID_PARAM;
  }
  GWPVP8ZeroCoeffBlock(y2_coeffs);
  for (i = 0u; i < 16u; ++i) GWPVP8ZeroCoeffBlock(y_coeffs[i]);
  for (i = 0u; i < 4u; ++i) {
    GWPVP8ZeroCoeffBlock(u_coeffs[i]);
    GWPVP8ZeroCoeffBlock(v_coeffs[i]);
  }

  if (skip_coeff || token_decoder == 0) {
    for (i = 0u; i < 9u; ++i) {
      left_ctx[i] = 0u;
      above_ctx_mb[i] = 0u;
    }
    return GWP_STATUS_OK;
  }

  nonzero_count = 0u;
  max_abs = 0u;
  if (has_y2) {
    GWPBool has_coeffs;
    GWPStatusCode st;
    st = GWPVP8ReadCoeffBlock(token_decoder,
                              header->entropy.coeff_probs,
                              1u,
                              left_ctx[8] ? GWP_TRUE : GWP_FALSE,
                              above_ctx_mb[8] ? GWP_TRUE : GWP_FALSE,
                              0,
                              y2_coeffs,
                              &has_coeffs,
                              0,
                              &nonzero_count,
                              &max_abs);
    if (st != GWP_STATUS_OK) return st;
    left_ctx[8] = has_coeffs ? 1u : 0u;
    above_ctx_mb[8] = has_coeffs ? 1u : 0u;
  } else {
    left_ctx[8] = 0u;
    above_ctx_mb[8] = 0u;
  }

  for (i = 0u; i < 16u; ++i) {
    GWPBool has_coeffs;
    GWPStatusCode st;
    GWPu32 l;
    GWPu32 a;
    l = kGWPVP8LeftContextIndex[i];
    a = kGWPVP8AboveContextIndex[i];
    st = GWPVP8ReadCoeffBlock(token_decoder,
                              header->entropy.coeff_probs,
                              has_y2 ? 0u : 3u,
                              left_ctx[l] ? GWP_TRUE : GWP_FALSE,
                              above_ctx_mb[a] ? GWP_TRUE : GWP_FALSE,
                              has_y2 ? 1 : 0,
                              y_coeffs[i],
                              &has_coeffs,
                              0,
                              &nonzero_count,
                              &max_abs);
    if (st != GWP_STATUS_OK) return st;
    left_ctx[l] = has_coeffs ? 1u : 0u;
    above_ctx_mb[a] = has_coeffs ? 1u : 0u;
  }

  for (i = 0u; i < 4u; ++i) {
    GWPBool has_coeffs;
    GWPStatusCode st;
    GWPu32 block_index;
    GWPu32 l;
    GWPu32 a;
    block_index = 16u + i;
    l = kGWPVP8LeftContextIndex[block_index];
    a = kGWPVP8AboveContextIndex[block_index];
    st = GWPVP8ReadCoeffBlock(token_decoder,
                              header->entropy.coeff_probs,
                              2u,
                              left_ctx[l] ? GWP_TRUE : GWP_FALSE,
                              above_ctx_mb[a] ? GWP_TRUE : GWP_FALSE,
                              0,
                              u_coeffs[i],
                              &has_coeffs,
                              0,
                              &nonzero_count,
                              &max_abs);
    if (st != GWP_STATUS_OK) return st;
    left_ctx[l] = has_coeffs ? 1u : 0u;
    above_ctx_mb[a] = has_coeffs ? 1u : 0u;
  }

  for (i = 0u; i < 4u; ++i) {
    GWPBool has_coeffs;
    GWPStatusCode st;
    GWPu32 block_index;
    GWPu32 l;
    GWPu32 a;
    block_index = 20u + i;
    l = kGWPVP8LeftContextIndex[block_index];
    a = kGWPVP8AboveContextIndex[block_index];
    st = GWPVP8ReadCoeffBlock(token_decoder,
                              header->entropy.coeff_probs,
                              2u,
                              left_ctx[l] ? GWP_TRUE : GWP_FALSE,
                              above_ctx_mb[a] ? GWP_TRUE : GWP_FALSE,
                              0,
                              v_coeffs[i],
                              &has_coeffs,
                              0,
                              &nonzero_count,
                              &max_abs);
    if (st != GWP_STATUS_OK) return st;
    left_ctx[l] = has_coeffs ? 1u : 0u;
    above_ctx_mb[a] = has_coeffs ? 1u : 0u;
  }
  return GWP_STATUS_OK;
}

static void GWPVP8ReconstructLumaMacroblock(const GWPVP8DequantFactors* dq,
                                            GWPVP8FrameBuffer* fb,
                                            GWPu32 mb_x,
                                            GWPu32 mb_y,
                                            int y_mode,
                                            const GWPu8 b_modes[16],
                                            GWPBool has_y2,
                                            const int y2_coeffs[16],
                                            int y_coeffs[16][16]) {
  GWPu32 x0;
  GWPu32 y0;
  GWPu8* plane;
  GWPu32 stride;
  int walsh_in[16];
  int walsh_out[16];
  GWPu32 block;
  if (dq == 0 || fb == 0 || b_modes == 0 || y2_coeffs == 0 || y_coeffs == 0) return;
  x0 = mb_x * 16u;
  y0 = mb_y * 16u;
  plane = fb->y;
  stride = fb->y_stride;
  for (block = 0u; block < 16u; ++block) walsh_out[block] = 0;
  if (has_y2) {
    GWPVP8DequantizeBlock(y2_coeffs, dq->y2_dc, dq->y2_ac, walsh_in);
    GWPVP8InverseWalsh4x4(walsh_in, walsh_out);
  }

  if (y_mode != (int)GWP_VP8_YMODE_B_PRED) {
    GWPu8 above[16];
    GWPu8 left[16];
    GWPu8 top_left;
    GWPu8 pred[16 * 16];
    GWPVP8GetAboveSamples(plane, stride, fb->width, x0, y0, 16u, 127u, above);
    GWPVP8GetLeftSamples(plane, stride, fb->height, x0, y0, 16u, 127u, left);
    top_left = GWPVP8GetTopLeftSample(plane, stride, fb->width, fb->height, x0, y0, 127u);
    GWPVP8Predict16x16((GWPVP8YMode)y_mode, above, left, top_left, pred);

    for (block = 0u; block < 16u; ++block) {
      GWPu32 bx;
      GWPu32 by;
      int qcoeff[16];
      int residue[16];
      bx = block & 3u;
      by = block >> 2;
      if (has_y2) GWPVP8DequantizeBlockWithWalshDC(y_coeffs[block], walsh_out[block], dq->y1_ac, qcoeff);
      else GWPVP8DequantizeBlock(y_coeffs[block], dq->y1_dc, dq->y1_ac, qcoeff);
      GWPVP8InverseDCT4x4(qcoeff, residue);
      GWPVP8AddResidual4x4(pred + by * 4u * 16u + bx * 4u,
                          16u,
                          residue,
                          plane + (y0 + by * 4u) * stride + x0 + bx * 4u,
                          stride);
    }
  } else {
    for (block = 0u; block < 16u; ++block) {
      GWPu32 bx;
      GWPu32 by;
      GWPu8 above[8];
      GWPu8 left[4];
      GWPu8 top_left;
      GWPu8 pred4[16];
      int qcoeff[16];
      int residue[16];
      GWPu32 px;
      GWPu32 py;
      bx = block & 3u;
      by = block >> 2;
      px = x0 + bx * 4u;
      py = y0 + by * 4u;
      GWPVP8GetAboveSamples(plane, stride, fb->width, px, py, 8u, 127u, above);
      GWPVP8GetLeftSamples(plane, stride, fb->height, px, py, 4u, 127u, left);
      top_left = GWPVP8GetTopLeftSample(plane, stride, fb->width, fb->height, px, py, 127u);
      GWPVP8Predict4x4((GWPVP8BMode)b_modes[block], above, left, top_left, pred4);
      GWPVP8DequantizeBlock(y_coeffs[block], dq->y1_dc, dq->y1_ac, qcoeff);
      GWPVP8InverseDCT4x4(qcoeff, residue);
      GWPVP8AddResidual4x4(pred4,
                          4u,
                          residue,
                          plane + py * stride + px,
                          stride);
    }
  }
}

static void GWPVP8ReconstructChromaPlane(const GWPVP8DequantFactors* dq,
                                         GWPu8* plane,
                                         GWPu32 stride,
                                         GWPu32 valid_width,
                                         GWPu32 valid_height,
                                         GWPu32 x0,
                                         GWPu32 y0,
                                         int uv_mode,
                                         int coeffs[4][16]) {
  GWPu8 above[8];
  GWPu8 left[8];
  GWPu8 top_left;
  GWPu8 pred[8 * 8];
  GWPu32 block;
  if (dq == 0 || plane == 0 || coeffs == 0) return;
  GWPVP8GetAboveSamples(plane, stride, valid_width, x0, y0, 8u, 128u, above);
  GWPVP8GetLeftSamples(plane, stride, valid_height, x0, y0, 8u, 128u, left);
  top_left = GWPVP8GetTopLeftSample(plane, stride, valid_width, valid_height, x0, y0, 128u);
  GWPVP8Predict8x8((GWPVP8UVMode)uv_mode, above, left, top_left, pred);
  for (block = 0u; block < 4u; ++block) {
    GWPu32 bx;
    GWPu32 by;
    int qcoeff[16];
    int residue[16];
    bx = block & 1u;
    by = block >> 1;
    GWPVP8DequantizeBlock(coeffs[block], dq->uv_dc, dq->uv_ac, qcoeff);
    GWPVP8InverseDCT4x4(qcoeff, residue);
    GWPVP8AddResidual4x4(pred + by * 4u * 8u + bx * 4u,
                        8u,
                        residue,
                        plane + (y0 + by * 4u) * stride + x0 + bx * 4u,
                        stride);
  }
}

static void GWPVP8ReconstructChromaMacroblock(const GWPVP8DequantFactors* dq,
                                              GWPVP8FrameBuffer* fb,
                                              GWPu32 mb_x,
                                              GWPu32 mb_y,
                                              int uv_mode,
                                              int u_coeffs[4][16],
                                              int v_coeffs[4][16]) {
  GWPu32 x0;
  GWPu32 y0;
  if (dq == 0 || fb == 0 || u_coeffs == 0 || v_coeffs == 0) return;
  x0 = mb_x * 8u;
  y0 = mb_y * 8u;
  GWPVP8ReconstructChromaPlane(dq, fb->u, fb->uv_stride, (fb->width + 1u) >> 1, (fb->height + 1u) >> 1, x0, y0, uv_mode, u_coeffs);
  GWPVP8ReconstructChromaPlane(dq, fb->v, fb->uv_stride, (fb->width + 1u) >> 1, (fb->height + 1u) >> 1, x0, y0, uv_mode, v_coeffs);
}

GWPStatusCode GWPDecodeVP8WithState(const GWPu8* data,
                                    GWPu32 data_size,
                                    const GWPDecoderOptions* options,
                                    GWPBitstreamFeatures* out_features,
                                    GWPVP8ControlHeader* out_control,
                                    GWPVP8FrameBuffer* out_fb) {
  GWPVP8ControlHeader control;
  GWPVP8BoolDecoder mode_br;
  GWPStatusCode st;
  GWPArena arena;
  GWPVP8FrameBuffer fb;
  GWPu8* above_pred;
  GWPu8* above_ctx;
  GWPu32 total_token_bytes;
  GWPVP8TokenDecoderSet token_decoders;
  GWPu32 i;

  if (data == 0 || options == 0) return GWP_STATUS_INVALID_PARAM;
  if (options->scratch == 0) return GWP_STATUS_INVALID_PARAM;
  if (options->output_buffer != 0 && options->output_stride < 4u) return GWP_STATUS_INVALID_PARAM;

  GWPArenaInit(&arena, options->scratch, options->scratch_size);
  st = GWPVP8ParseControlHeaderWithModeState(data, data_size, &control, &mode_br);
  if (st != GWP_STATUS_OK) return st;

  if (options->strict) {
    if (options->max_width != 0u && control.frame.key.width > options->max_width) {
      return GWP_STATUS_LIMIT_EXCEEDED;
    }
    if (options->max_height != 0u && control.frame.key.height > options->max_height) {
      return GWP_STATUS_LIMIT_EXCEEDED;
    }
  }

  st = GWPVP8FrameBufferInit(&control, options, &arena, &fb);
  if (st != GWP_STATUS_OK) return st;
  above_pred = (GWPu8*)GWPArenaAlloc(&arena, control.mb_cols * 4u, 1u);
  above_ctx = (GWPu8*)GWPArenaAlloc(&arena, control.mb_cols * 9u, 1u);
  if (above_pred == 0 || above_ctx == 0) return GWP_STATUS_NOT_ENOUGH_SCRATCH;
  for (i = 0u; i < control.mb_cols * 4u; ++i) above_pred[i] = (GWPu8)GWP_VP8_BMODE_DC;
  for (i = 0u; i < control.mb_cols * 9u; ++i) above_ctx[i] = 0u;

  total_token_bytes = 0u;
  for (i = 0u; i < control.token_partitions.partition_count; ++i) {
    total_token_bytes += control.token_partitions.partition[i].size;
  }
  GWPZero(&token_decoders, (GWPu32)sizeof(token_decoders));
  if (total_token_bytes > 0u) {
    st = GWPVP8InitTokenDecoders(&control.token_partitions, &token_decoders);
    if (st != GWP_STATUS_OK) return st;
  } else {
    token_decoders.count = control.token_partitions.partition_count;
    if (token_decoders.count == 0u) token_decoders.count = 1u;
  }

  {
    GWPu8 token_partition_disabled[GWP_VP8_MAX_TOKEN_PARTITIONS];
    GWPu32 mb_y;
    for (i = 0u; i < GWP_VP8_MAX_TOKEN_PARTITIONS; ++i) token_partition_disabled[i] = 0u;
    for (mb_y = 0u; mb_y < control.mb_rows; ++mb_y) {
      GWPu32 mb_x;
      GWPu8 left_pred[4];
      GWPu8 left_ctx[9];
      GWPVP8TokenDecoder* token_decoder;
      GWPu32 token_part_index;
      token_part_index = mb_y & (token_decoders.count - 1u);
      token_decoder = (total_token_bytes > 0u && !token_partition_disabled[token_part_index]) ?
          &token_decoders.decoder[token_part_index] : 0;
      for (mb_x = 0u; mb_x < 4u; ++mb_x) left_pred[mb_x] = (GWPu8)GWP_VP8_BMODE_DC;
      for (mb_x = 0u; mb_x < 9u; ++mb_x) left_ctx[mb_x] = 0u;

      for (mb_x = 0u; mb_x < control.mb_cols; ++mb_x) {
        GWPu32 segment_id;
        GWPBool skip_coeff;
        int y_mode;
        int uv_mode;
        GWPu8 b_modes[16];
        GWPBool has_y2;
        int y2_coeffs[16];
        int y_coeffs[16][16];
        int u_coeffs[4][16];
        int v_coeffs[4][16];
        GWPu32 idx;
        st = GWPVP8ReadKeyFrameMacroblockHeader(&mode_br,
                                                &control.segment,
                                                &control.entropy,
                                                above_pred,
                                                left_pred,
                                                mb_x * 4u,
                                                &segment_id,
                                                &skip_coeff,
                                                &y_mode,
                                                &uv_mode,
                                                b_modes);
        if (st != GWP_STATUS_OK) return st;
        has_y2 = (y_mode != (int)GWP_VP8_YMODE_B_PRED) ? GWP_TRUE : GWP_FALSE;
        st = GWPVP8DecodeMacroblockTokens(&control,
                                          token_decoder,
                                          left_ctx,
                                          above_ctx + mb_x * 9u,
                                          skip_coeff,
                                          has_y2,
                                          y2_coeffs,
                                          y_coeffs,
                                          u_coeffs,
                                          v_coeffs);
        if (st == GWP_STATUS_TRUNCATED_DATA && token_decoder != 0) {
          GWPu32 j;
          token_partition_disabled[token_part_index] = 1u;
          GWPVP8ZeroCoeffBlock(y2_coeffs);
          for (j = 0u; j < 16u; ++j) GWPVP8ZeroCoeffBlock(y_coeffs[j]);
          for (j = 0u; j < 4u; ++j) {
            GWPVP8ZeroCoeffBlock(u_coeffs[j]);
            GWPVP8ZeroCoeffBlock(v_coeffs[j]);
          }
          for (j = 0u; j < 9u; ++j) {
            left_ctx[j] = 0u;
            above_ctx[mb_x * 9u + j] = 0u;
          }
          st = GWP_STATUS_OK;
        }
        if (st != GWP_STATUS_OK) return st;
        idx = mb_y * control.mb_cols + mb_x;
        fb.mb_segment_ids[idx] = (GWPu8)segment_id;
        fb.mb_y_modes[idx] = (GWPu8)y_mode;
        fb.mb_has_coeffs[idx] = GWPVP8MacroblockHasNonZeroCoeffs(has_y2,
                                                                 y2_coeffs,
                                                                 y_coeffs,
                                                                 u_coeffs,
                                                                 v_coeffs) ? 1u : 0u;
        GWPVP8ReconstructLumaMacroblock(&control.dequant[segment_id],
                                        &fb,
                                        mb_x,
                                        mb_y,
                                        y_mode,
                                        b_modes,
                                        has_y2,
                                        y2_coeffs,
                                        y_coeffs);
        GWPVP8ReconstructChromaMacroblock(&control.dequant[segment_id],
                                          &fb,
                                          mb_x,
                                          mb_y,
                                          uv_mode,
                                          u_coeffs,
                                          v_coeffs);
      }
    }
  }

  GWPVP8FrameBufferFilter(&control, &fb);
  if (options->output_buffer != 0) {
    GWPVP8FrameBufferPack(&fb, options);
  }
  if (out_control != 0) *out_control = control;
  if (out_fb != 0) *out_fb = fb;

  if (out_features != 0) {
    out_features->width = control.frame.key.width;
    out_features->height = control.frame.key.height;
    out_features->has_alpha = GWP_FALSE;
    out_features->has_animation = GWP_FALSE;
    out_features->has_icc = GWP_FALSE;
    out_features->has_exif = GWP_FALSE;
    out_features->has_xmp = GWP_FALSE;
    out_features->format = GWP_BITSTREAM_VP8;
    out_features->frame_count = 0u;
  }
  return GWP_STATUS_OK;
}

GWPStatusCode GWPDecodeVP8Stub(const GWPu8* data,
                               GWPu32 data_size,
                               const GWPDecoderOptions* options,
                               GWPBitstreamFeatures* out_features) {
  return GWPDecodeVP8WithState(data, data_size, options, out_features, 0, 0);
}
