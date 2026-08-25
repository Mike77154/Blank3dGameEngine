#include "vp8_native_enc.h"

#include "vp8_bool_enc.h"
#include "vp8_tokens_enc.h"
#include "vp8_tree_enc.h"
#include "vp8_lossy_plan.h"
#include "../dec/vp8_probdata.h"
#include "../dec/vp8_quant.h"
#include "../dec/vp8_recon.h"
#include "../dec/vp8_transform.h"
#include "../utils/common.h"
#include "../utils/endian.h"

#define GWP_VP8_MAX_PARTITIONS 8u
#define GWP_NATIVE_SEG_PROB_DEFAULT 255u

typedef struct GWPVP8MBDecision {
  GWPu8 segment_id;
  GWPu8 skip_coeff;
  GWPu8 y_mode;
  GWPu8 uv_mode;
  GWPu8 b_modes[16];
  GWPs32 y2_qcoeff[16];
  GWPs32 y_qcoeff[16][16];
  GWPs32 u_qcoeff[4][16];
  GWPs32 v_qcoeff[4][16];
} GWPVP8MBDecision;

typedef struct GWPVP8CoeffProbStats {
  GWPu32 branch[GWP_VP8_COEFF_BLOCK_TYPES]
                [GWP_VP8_COEFF_BANDS]
                [GWP_VP8_PREV_COEFF_CONTEXTS]
                [GWP_VP8_ENTROPY_NODES][2];
} GWPVP8CoeffProbStats;

typedef struct GWPVP8EncodeAnalysis {
  GWPu32 segment_hist[GWP_VP8_MAX_SEGMENTS];
  GWPu32 skip_count;
  GWPu32 y_mode_mask;
  GWPu32 b_mode_mask;
  GWPu32 uv_mode_mask;
  GWPu32 y2_non_zero;
  GWPu32 y_non_zero;
  GWPu32 uv_non_zero;
} GWPVP8EncodeAnalysis;

static void GWPReadRawPixelVP8(const GWPu8* p,
                               GWPRawPixelFormat fmt,
                               GWPu8* r,
                               GWPu8* g,
                               GWPu8* b,
                               GWPu8* a) {
  if (fmt == GWP_RAW_BGRA) {
    *b = p[0]; *g = p[1]; *r = p[2]; *a = p[3];
  } else if (fmt == GWP_RAW_ARGB) {
    *a = p[0]; *r = p[1]; *g = p[2]; *b = p[3];
  } else {
    *r = p[0]; *g = p[1]; *b = p[2]; *a = p[3];
  }
}

static GWPu8 GWPClamp8Enc(int v) {
  if (v < 0) return 0u;
  if (v > 255) return 255u;
  return (GWPu8)v;
}

static int GWPClampI(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static GWPu32 GWPClampU32Enc(GWPu32 v, GWPu32 lo, GWPu32 hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static GWPu32 GWPToLumaEnc(GWPu8 r, GWPu8 g, GWPu8 b) {
  return (19595u * (GWPu32)r + 38470u * (GWPu32)g + 7471u * (GWPu32)b + 32768u) >> 16;
}

static GWPu32 GWPToUEnc(GWPu8 r, GWPu8 g, GWPu8 b) {
  int u;
  u = ((-11058 * (int)r - 21710 * (int)g + 32768 * (int)b + 32768) >> 16) + 128;
  return (GWPu32)GWPClamp8Enc(u);
}

static GWPu32 GWPToVEnc(GWPu8 r, GWPu8 g, GWPu8 b) {
  int v;
  v = ((32768 * (int)r - 27439 * (int)g - 5329 * (int)b + 32768) >> 16) + 128;
  return (GWPu32)GWPClamp8Enc(v);
}

static int GWPRoundDivSigned(int value, int div) {
  if (div <= 0) return 0;
  if (value >= 0) return (value + (div >> 1)) / div;
  return -(((-value) + (div >> 1)) / div);
}

static void GWPWritePartitionTag(GWPu8* dst, GWPu32 first_partition_size) {
  GWPu32 tag;
  tag = 0u;
  tag |= 0u;
  tag |= (0u << 1);
  tag |= (1u << 4);
  tag |= (first_partition_size & 0x7ffffu) << 5;
  GWPWriteLE24(dst, tag);
}

static GWPu32 GWPNearestPartitionCount(GWPu32 value) {
  if (value >= 8u) return 8u;
  if (value >= 4u) return 4u;
  if (value >= 2u) return 2u;
  return 1u;
}

void GWPVP8NativeEncodeStatsInit(GWPVP8NativeEncodeStats* stats) {
  if (stats == 0) return;
  GWPZero(stats, (GWPu32)sizeof(*stats));
}

GWPu32 GWPEstimateNativeVP8BitstreamSize(GWPu32 width, GWPu32 height) {
  GWPu32 pixels;
  if (!GWPMulU32(width, height, &pixels)) return 0xffffffffu;
  if (pixels > (0xffffffffu - 12288u) / 3u) return 0xffffffffu;
  return 12288u + pixels * 3u;
}

static GWPu32 GWPChooseNativeQIndex(const GWPEncodeConfig* config,
                                    const GWPVP8LossyPlan* plan) {
  GWPu32 q;
  q = 12u + ((100u - config->quality) * 115u) / 100u;
  if (plan != 0 && plan->macroblock_count != 0u && plan->average_variance > 0u) {
    if (plan->average_variance > 120u && q > 4u) q -= 4u;
    if (plan->average_variance > 240u && q > 6u) q -= 6u;
  }
  if (config->preset == GWP_PRESET_PHOTO) q += 6u;
  if (config->preset == GWP_PRESET_TEXT || config->preset == GWP_PRESET_ICON) {
    if (q > 8u) q -= 8u;
  }
  if (q > 127u) q = 127u;
  return q;
}

static GWPu32 GWPChooseNativeFilter(const GWPEncodeConfig* config,
                                    const GWPVP8LossyPlan* plan) {
  GWPu32 level;
  level = (config->filter_strength * 63u + 50u) / 100u;
  if (plan != 0 && plan->suggested_filter_strength != 0u) {
    level = ((level * 3u) + ((plan->suggested_filter_strength * 63u + 50u) / 100u)) >> 2;
  }
  if (level > 63u) level = 63u;
  return level;
}

static GWPu32 GWPChooseNativeSharpness(const GWPEncodeConfig* config,
                                       const GWPVP8LossyPlan* plan) {
  GWPu32 sharpness;
  sharpness = config->use_sharp_yuv ? 3u : 0u;
  if (plan != 0 && plan->suggested_sharpness < 8u && plan->suggested_sharpness > sharpness) {
    sharpness = plan->suggested_sharpness;
  }
  if (sharpness > 7u) sharpness = 7u;
  return sharpness;
}

static GWPu32 GWPChooseNativePartitionCount(const GWPEncodeConfig* config,
                                            const GWPVP8LossyPlan* plan,
                                            GWPu32 mb_rows) {
  GWPu32 count;
  if (config->vp8_partitions != 0u) {
    count = GWPNearestPartitionCount(config->vp8_partitions);
  } else if (plan != 0 && plan->suggested_partitions != 0u) {
    count = GWPNearestPartitionCount(plan->suggested_partitions);
  } else if (mb_rows >= 8u) {
    count = 4u;
  } else {
    count = 1u;
  }
  while (count > mb_rows && count > 1u) count >>= 1;
  if (count == 0u) count = 1u;
  return count;
}

static GWPu32 GWPChooseNativeSegmentCount(const GWPEncodeConfig* config,
                                          const GWPVP8LossyPlan* plan) {
  GWPu32 count;
  if (config->vp8_segments != 0u) count = config->vp8_segments;
  else if (plan != 0 && plan->suggested_segments != 0u) count = plan->suggested_segments;
  else count = 1u;
  if (count < 1u) count = 1u;
  if (count > 4u) count = 4u;
  return count;
}

static GWPu32 GWPMapPlanSegment(const GWPVP8LossyPlan* plan,
                                GWPu32 mb_index,
                                GWPu32 segment_count) {
  GWPu32 plan_seg;
  if (plan == 0 || plan->macroblocks == 0 || segment_count <= 1u) return 0u;
  plan_seg = plan->macroblocks[mb_index].segment_id;
  if (plan_seg >= 4u) plan_seg = 3u;
  if (segment_count >= 4u) return plan_seg;
  if (segment_count == 3u) {
    if (plan_seg == 0u) return 0u;
    if (plan_seg == 1u || plan_seg == 2u) return 1u;
    return 2u;
  }
  if (segment_count == 2u) {
    return (plan_seg >= 2u) ? 1u : 0u;
  }
  return 0u;
}

static GWPu8 GWPProbFromCounts(GWPu32 zero_count, GWPu32 one_count, GWPu8 fallback) {
  GWPu32 total;
  GWPu32 p;
  total = zero_count + one_count;
  if (total == 0u) return fallback;
  p = (zero_count * 255u + (total >> 1)) / total;
  if (p < 1u) p = 1u;
  if (p > 255u) p = 255u;
  return (GWPu8)p;
}

static void GWPSetupSegmentHeader(const GWPEncodeConfig* config,
                                  const GWPVP8LossyPlan* plan,
                                  GWPu32 segment_count,
                                  GWPu32 q_index,
                                  GWPu32 filter_level,
                                  GWPVP8SegmentHeader* seg) {
  GWPu32 i;
  static const int kQuantDelta[4] = { 18, 6, -6, -18 };
  static const int kFilterDelta[4] = { 10, 3, -3, -8 };
  GWPu32 hist[4];
  if (seg == 0) return;
  GWPZero(seg, (GWPu32)sizeof(*seg));
  for (i = 0u; i < GWP_VP8_MB_FEATURE_TREE_PROBS; ++i) seg->tree_probs[i] = GWP_NATIVE_SEG_PROB_DEFAULT;
  if (segment_count <= 1u) return;
  seg->enabled = GWP_TRUE;
  seg->update_map = GWP_TRUE;
  seg->update_data = GWP_TRUE;
  seg->absolute_delta = GWP_FALSE;
  for (i = 0u; i < segment_count; ++i) {
    int qd;
    int fd;
    qd = kQuantDelta[i];
    fd = kFilterDelta[i];
    if (config->preset == GWP_PRESET_TEXT || config->preset == GWP_PRESET_ICON) {
      qd /= 2;
      fd /= 2;
    }
    if ((int)q_index + qd < 0) qd = -(int)q_index;
    if ((int)q_index + qd > 127) qd = 127 - (int)q_index;
    if ((int)filter_level + fd < 0) fd = -(int)filter_level;
    if ((int)filter_level + fd > 63) fd = 63 - (int)filter_level;
    seg->quant_idx[i] = qd;
    seg->lf_level[i] = fd;
  }
  for (i = segment_count; i < 4u; ++i) {
    seg->quant_idx[i] = 0;
    seg->lf_level[i] = 0;
  }
  GWPZero(hist, (GWPu32)sizeof(hist));
  if (plan != 0 && plan->macroblocks != 0) {
    for (i = 0u; i < plan->macroblock_count; ++i) {
      GWPu32 mapped;
      mapped = GWPMapPlanSegment(plan, i, segment_count);
      hist[mapped] += 1u;
    }
    seg->tree_probs[0] = GWPProbFromCounts(hist[0], hist[1] + hist[2] + hist[3], GWP_NATIVE_SEG_PROB_DEFAULT);
    seg->tree_probs[1] = GWPProbFromCounts(hist[1], hist[2] + hist[3], GWP_NATIVE_SEG_PROB_DEFAULT);
    seg->tree_probs[2] = GWPProbFromCounts(hist[2], hist[3], GWP_NATIVE_SEG_PROB_DEFAULT);
  }
}

static GWPu32 GWPModePenalty(GWPu32 mode) {
  static const GWPu8 kPenalty[10] = { 0u, 2u, 1u, 1u, 4u, 4u, 4u, 4u, 4u, 4u };
  if (mode >= 10u) return 8u;
  return (GWPu32)kPenalty[mode];
}

static GWPu32 GWPUVModePenalty(GWPu32 mode) {
  static const GWPu8 kPenalty[4] = { 0u, 1u, 1u, 2u };
  if (mode >= 4u) return 4u;
  return (GWPu32)kPenalty[mode];
}

static void GWPGetLumaTarget4x4(const GWPu8* pixels,
                                GWPu32 width,
                                GWPu32 height,
                                GWPu32 stride,
                                GWPRawPixelFormat pixel_format,
                                GWPu32 x0,
                                GWPu32 y0,
                                GWPu8 out[16]) {
  GWPu32 r;
  for (r = 0u; r < 4u; ++r) {
    GWPu32 c;
    GWPu32 yy;
    yy = y0 + r;
    if (yy >= height) yy = height - 1u;
    for (c = 0u; c < 4u; ++c) {
      GWPu32 xx;
      const GWPu8* p;
      GWPu8 rr, gg, bb, aa;
      xx = x0 + c;
      if (xx >= width) xx = width - 1u;
      p = pixels + yy * stride + xx * 4u;
      GWPReadRawPixelVP8(p, pixel_format, &rr, &gg, &bb, &aa);
      out[r * 4u + c] = (GWPu8)GWPToLumaEnc(rr, gg, bb);
    }
  }
}

static void GWPGetChromaTarget8x8(const GWPu8* pixels,
                                  GWPu32 width,
                                  GWPu32 height,
                                  GWPu32 stride,
                                  GWPRawPixelFormat pixel_format,
                                  GWPu32 x0,
                                  GWPu32 y0,
                                  GWPu8 out_u[64],
                                  GWPu8 out_v[64]) {
  GWPu32 r;
  for (r = 0u; r < 8u; ++r) {
    GWPu32 c;
    for (c = 0u; c < 8u; ++c) {
      GWPu32 sx;
      GWPu32 sy;
      GWPu32 dx;
      GWPu32 dy;
      GWPu32 count;
      GWPu32 sum_r;
      GWPu32 sum_g;
      GWPu32 sum_b;
      sx = x0 + c * 2u;
      sy = y0 + r * 2u;
      sum_r = sum_g = sum_b = 0u;
      count = 0u;
      for (dy = 0u; dy < 2u; ++dy) {
        GWPu32 yy;
        yy = sy + dy;
        if (yy >= height) yy = height - 1u;
        for (dx = 0u; dx < 2u; ++dx) {
          GWPu32 xx;
          const GWPu8* p;
          GWPu8 rr, gg, bb, aa;
          xx = sx + dx;
          if (xx >= width) xx = width - 1u;
          p = pixels + yy * stride + xx * 4u;
          GWPReadRawPixelVP8(p, pixel_format, &rr, &gg, &bb, &aa);
          sum_r += rr;
          sum_g += gg;
          sum_b += bb;
          ++count;
        }
      }
      if (count == 0u) count = 1u;
      out_u[r * 8u + c] = (GWPu8)GWPToUEnc((GWPu8)(sum_r / count), (GWPu8)(sum_g / count), (GWPu8)(sum_b / count));
      out_v[r * 8u + c] = (GWPu8)GWPToVEnc((GWPu8)(sum_r / count), (GWPu8)(sum_g / count), (GWPu8)(sum_b / count));
    }
  }
}

static void GWPGetAbove4x4(const GWPu8* plane,
                           GWPu32 stride,
                           GWPu32 x0,
                           GWPu32 y0,
                           GWPu8 missing,
                           GWPu8 out[8],
                           GWPu8* top_left) {
  GWPu32 i;
  if (y0 == 0u) {
    for (i = 0u; i < 8u; ++i) out[i] = missing;
    *top_left = missing;
    return;
  }
  for (i = 0u; i < 8u; ++i) {
    GWPu32 xx;
    xx = x0 + i;
    if (xx >= stride) xx = stride - 1u;
    out[i] = plane[(y0 - 1u) * stride + xx];
  }
  *top_left = (x0 == 0u) ? missing : plane[(y0 - 1u) * stride + x0 - 1u];
}

static void GWPGetLeft4x4(const GWPu8* plane,
                          GWPu32 stride,
                          GWPu32 x0,
                          GWPu32 y0,
                          GWPu8 missing,
                          GWPu8 out[4]) {
  GWPu32 i;
  if (x0 == 0u) {
    for (i = 0u; i < 4u; ++i) out[i] = missing;
    return;
  }
  for (i = 0u; i < 4u; ++i) out[i] = plane[(y0 + i) * stride + x0 - 1u];
}

static void GWPGetAbove8x8(const GWPu8* plane,
                           GWPu32 stride,
                           GWPu32 x0,
                           GWPu32 y0,
                           GWPu8 missing,
                           GWPu8 out[8],
                           GWPu8* top_left) {
  GWPu32 i;
  if (y0 == 0u) {
    for (i = 0u; i < 8u; ++i) out[i] = missing;
    *top_left = missing;
    return;
  }
  for (i = 0u; i < 8u; ++i) {
    GWPu32 xx;
    xx = x0 + i;
    if (xx >= stride) xx = stride - 1u;
    out[i] = plane[(y0 - 1u) * stride + xx];
  }
  *top_left = (x0 == 0u) ? missing : plane[(y0 - 1u) * stride + x0 - 1u];
}

static void GWPGetLeft8x8(const GWPu8* plane,
                          GWPu32 stride,
                          GWPu32 x0,
                          GWPu32 y0,
                          GWPu8 missing,
                          GWPu8 out[8]) {
  GWPu32 i;
  if (x0 == 0u) {
    for (i = 0u; i < 8u; ++i) out[i] = missing;
    return;
  }
  for (i = 0u; i < 8u; ++i) out[i] = plane[(y0 + i) * stride + x0 - 1u];
}

static GWPu32 GWPQuantizePredict4x4(const GWPu8 pred[16],
                                    const GWPu8 target[16],
                                    int dq,
                                    int lambda,
                                    GWPs32* out_qcoeff,
                                    GWPBool* out_has_coeff,
                                    GWPu8 out_recon[16]) {
  GWPu32 i;
  int sum;
  int qcoeff;
  int dc;
  GWPu32 sse;
  sum = 0;
  for (i = 0u; i < 16u; ++i) sum += (int)target[i] - (int)pred[i];
  qcoeff = GWPRoundDivSigned(sum / 2, dq);
  qcoeff = GWPClampI(qcoeff, -2047, 2047);
  dc = ((qcoeff * dq) + 4) >> 3;
  sse = 0u;
  for (i = 0u; i < 16u; ++i) {
    int rv;
    int d;
    rv = (int)pred[i] + dc;
    out_recon[i] = GWPClamp8Enc(rv);
    d = (int)target[i] - (int)out_recon[i];
    sse += (GWPu32)(d * d);
  }
  *out_qcoeff = qcoeff;
  *out_has_coeff = (qcoeff != 0) ? GWP_TRUE : GWP_FALSE;
  if (qcoeff != 0) sse += (GWPu32)lambda;
  return sse;
}

static void GWPGetLumaTarget16x16(const GWPu8* pixels,
                                 GWPu32 width,
                                 GWPu32 height,
                                 GWPu32 stride,
                                 GWPRawPixelFormat pixel_format,
                                 GWPu32 x0,
                                 GWPu32 y0,
                                 GWPu8 out[16 * 16]) {
  GWPu32 r;
  for (r = 0u; r < 16u; ++r) {
    GWPu32 c;
    GWPu32 yy;
    yy = y0 + r;
    if (yy >= height) yy = height - 1u;
    for (c = 0u; c < 16u; ++c) {
      GWPu32 xx;
      const GWPu8* p;
      GWPu8 rr, gg, bb, aa;
      xx = x0 + c;
      if (xx >= width) xx = width - 1u;
      p = pixels + yy * stride + xx * 4u;
      GWPReadRawPixelVP8(p, pixel_format, &rr, &gg, &bb, &aa);
      out[r * 16u + c] = (GWPu8)GWPToLumaEnc(rr, gg, bb);
    }
  }
}

static void GWPGetAbove16x16(const GWPu8* plane,
                             GWPu32 stride,
                             GWPu32 x0,
                             GWPu32 y0,
                             GWPu8 missing,
                             GWPu8 out[16],
                             GWPu8* top_left) {
  GWPu32 i;
  if (y0 == 0u) {
    for (i = 0u; i < 16u; ++i) out[i] = missing;
    *top_left = missing;
    return;
  }
  for (i = 0u; i < 16u; ++i) {
    GWPu32 xx;
    xx = x0 + i;
    if (xx >= stride) xx = stride - 1u;
    out[i] = plane[(y0 - 1u) * stride + xx];
  }
  *top_left = (x0 == 0u) ? missing : plane[(y0 - 1u) * stride + x0 - 1u];
}

static void GWPGetLeft16x16(const GWPu8* plane,
                            GWPu32 stride,
                            GWPu32 x0,
                            GWPu32 y0,
                            GWPu8 missing,
                            GWPu8 out[16]) {
  GWPu32 i;
  if (x0 == 0u) {
    for (i = 0u; i < 16u; ++i) out[i] = missing;
    return;
  }
  for (i = 0u; i < 16u; ++i) out[i] = plane[(y0 + i) * stride + x0 - 1u];
}

static GWPu32 GWPSSEBlock16(const GWPu8 recon[16], const GWPu8 target[16]) {
  GWPu32 i;
  GWPu32 sse;
  sse = 0u;
  for (i = 0u; i < 16u; ++i) {
    int d;
    d = (int)target[i] - (int)recon[i];
    sse += (GWPu32)(d * d);
  }
  return sse;
}

static GWPu32 GWPCountNonZeroCoeffBlock(const int coeffs[16]) {
  GWPu32 i;
  GWPu32 n;
  n = 0u;
  for (i = 0u; i < 16u; ++i) if (coeffs[i] != 0) ++n;
  return n;
}

static void GWPBuildReconFromCoeffBlock(const GWPu8 pred[16],
                                        int dc_q,
                                        int ac_q,
                                        const int coeffs[16],
                                        GWPu8 out_recon[16]) {
  int qcoeff[16];
  int residue[16];
  GWPVP8DequantizeBlock(coeffs, dc_q, ac_q, qcoeff);
  GWPVP8InverseDCT4x4(qcoeff, residue);
  GWPVP8AddResidual4x4(pred, 4u, residue, out_recon, 4u);
}

static void GWPBuildCoeffContribution(int coeff_index,
                                      int dc_q,
                                      int ac_q,
                                      int out[16]) {
  int coeffs[16];
  GWPu32 i;
  for (i = 0u; i < 16u; ++i) coeffs[i] = 0;
  coeffs[coeff_index] = 1;
  GWPVP8DequantizeBlock(coeffs, dc_q, ac_q, coeffs);
  GWPVP8InverseDCT4x4(coeffs, out);
}

static GWPu32 GWPQuantizeSparsePredict4x4(const GWPu8 pred[16],
                                          const GWPu8 target[16],
                                          int dc_q,
                                          int ac_q,
                                          int lambda,
                                          GWPBool allow_dc,
                                          GWPBool enable_ac,
                                          int out_coeffs[16],
                                          GWPBool* out_has_coeff,
                                          GWPu8 out_recon[16]) {
  static const int kIdxWithDC[4] = { 0, 1, 4, 5 };
  static const int kIdxNoDC[3] = { 1, 4, 5 };
  int coeffs[16];
  GWPu8 best_recon[16];
  GWPu32 best_cost;
  GWPu32 nidx;
  GWPu32 ii;
  const int* idxs;
  if (out_has_coeff == 0 || out_coeffs == 0 || out_recon == 0) return 0xffffffffu;
  for (ii = 0u; ii < 16u; ++ii) coeffs[ii] = 0;
  GWPBuildReconFromCoeffBlock(pred, dc_q, ac_q, coeffs, best_recon);
  best_cost = GWPSSEBlock16(best_recon, target);
  idxs = allow_dc ? kIdxWithDC : kIdxNoDC;
  nidx = allow_dc ? 4u : 3u;
  if (!enable_ac && !allow_dc) {
    GWPCopy(out_recon, best_recon, 16u);
    for (ii = 0u; ii < 16u; ++ii) out_coeffs[ii] = coeffs[ii];
    *out_has_coeff = GWP_FALSE;
    return best_cost;
  }
  for (ii = 0u; ii < nidx; ++ii) {
    int idx;
    int basis[16];
    int dot;
    int norm;
    int est;
    int candidates[5];
    int cand_count;
    int ci;
    idx = idxs[ii];
    if (!enable_ac && idx != 0) continue;
    GWPBuildCoeffContribution(idx, dc_q, ac_q, basis);
    dot = 0;
    norm = 0;
    {
      int k;
      for (k = 0; k < 16; ++k) {
        dot += ((int)target[k] - (int)best_recon[k]) * basis[k];
        norm += basis[k] * basis[k];
      }
    }
    if (norm == 0) continue;
    est = GWPRoundDivSigned(dot, norm);
    est = GWPClampI(est, -2047, 2047);
    candidates[0] = 0;
    candidates[1] = GWPClampI(est - 1, -2047, 2047);
    candidates[2] = est;
    candidates[3] = GWPClampI(est + 1, -2047, 2047);
    candidates[4] = GWPClampI((est < 0) ? (est >> 1) : ((est + 1) >> 1), -2047, 2047);
    cand_count = 5;
    for (ci = 0; ci < cand_count; ++ci) {
      int trial[16];
      GWPu8 trial_recon[16];
      GWPu32 cost;
      GWPu32 nz;
      int j;
      for (j = 0; j < 16; ++j) trial[j] = coeffs[j];
      trial[idx] = candidates[ci];
      if (!allow_dc) trial[0] = 0;
      GWPBuildReconFromCoeffBlock(pred, dc_q, ac_q, trial, trial_recon);
      cost = GWPSSEBlock16(trial_recon, target);
      nz = GWPCountNonZeroCoeffBlock(trial);
      cost += nz * (GWPu32)lambda;
      if (cost < best_cost) {
        best_cost = cost;
        GWPCopy(best_recon, trial_recon, 16u);
        for (j = 0; j < 16; ++j) coeffs[j] = trial[j];
      }
    }
  }
  GWPCopy(out_recon, best_recon, 16u);
  for (ii = 0u; ii < 16u; ++ii) out_coeffs[ii] = coeffs[ii];
  *out_has_coeff = (GWPCountNonZeroCoeffBlock(coeffs) != 0u) ? GWP_TRUE : GWP_FALSE;
  return best_cost;
}

static int GWPComputeY2BasePixel(int y2_qcoeff0, int y2_dc_q) {
  int deq;
  int walsh;
  int pixel;
  deq = y2_qcoeff0 * y2_dc_q;
  walsh = (deq + 3) >> 3;
  pixel = (walsh + 4) >> 3;
  return pixel;
}

static GWPu32 GWPYModePenalty(GWPu32 mode) {
  static const GWPu8 kPenalty[5] = { 0u, 1u, 1u, 2u, 0u };
  if (mode >= 5u) return 4u;
  return (GWPu32)kPenalty[mode];
}

static GWPu32 GWPSelectBestBMode(const GWPu8 above[8],
                                 const GWPu8 left[4],
                                 GWPu8 top_left,
                                 const GWPu8 target[16],
                                 int dc_q,
                                 int ac_q,
                                 int lambda,
                                 GWPBool enable_modes,
                                 GWPBool enable_ac,
                                 GWPu8* out_mode,
                                 int out_coeffs[16],
                                 GWPBool* out_has_coeff,
                                 GWPu8 out_recon[16]) {
  GWPu32 best_cost;
  GWPu8 best_mode;
  GWPBool best_has_coeff;
  GWPu8 best_recon[16];
  int best_coeffs[16];
  GWPu32 mode;
  best_cost = 0xffffffffu;
  best_mode = (GWPu8)GWP_VP8_BMODE_DC;
  best_has_coeff = GWP_FALSE;
  for (mode = 0u; mode < GWP_VP8_B_MODE_COUNT; ++mode) {
    GWPu8 pred[16];
    GWPu8 recon[16];
    int coeffs[16];
    GWPBool has_coeff;
    GWPu32 cost;
    GWPu32 i;
    if (!enable_modes && mode != (GWPu32)GWP_VP8_BMODE_DC) continue;
    GWPVP8Predict4x4((GWPVP8BMode)mode, above, left, top_left, pred);
    cost = GWPQuantizeSparsePredict4x4(pred, target, dc_q, ac_q,
                                       lambda + (int)GWPModePenalty(mode),
                                       GWP_TRUE, enable_ac,
                                       coeffs, &has_coeff, recon);
    if (cost < best_cost) {
      best_cost = cost;
      best_mode = (GWPu8)mode;
      best_has_coeff = has_coeff;
      GWPCopy(best_recon, recon, 16u);
      for (i = 0u; i < 16u; ++i) best_coeffs[i] = coeffs[i];
    }
  }
  *out_mode = best_mode;
  *out_has_coeff = best_has_coeff;
  GWPCopy(out_recon, best_recon, 16u);
  {
    GWPu32 i;
    for (i = 0u; i < 16u; ++i) out_coeffs[i] = best_coeffs[i];
  }
  return best_cost;
}

static GWPu32 GWPSelectBestUVMode(const GWPu8 above_u[8],
                                  const GWPu8 left_u[8],
                                  GWPu8 top_left_u,
                                  const GWPu8 above_v[8],
                                  const GWPu8 left_v[8],
                                  GWPu8 top_left_v,
                                  const GWPu8 target_u[64],
                                  const GWPu8 target_v[64],
                                  int dc_q,
                                  int ac_q,
                                  int lambda,
                                  GWPBool enable_modes,
                                  GWPBool enable_ac,
                                  GWPu8* out_mode,
                                  int out_qcoeff_u[4][16],
                                  int out_qcoeff_v[4][16],
                                  GWPBool* out_any_coeff,
                                  GWPu8 out_recon_u[64],
                                  GWPu8 out_recon_v[64]) {
  GWPu32 best_cost;
  GWPu8 best_mode;
  GWPu32 mode;
  GWPBool best_any;
  GWPu8 best_u[64];
  GWPu8 best_v[64];
  int best_q_u[4][16];
  int best_q_v[4][16];
  best_cost = 0xffffffffu;
  best_mode = (GWPu8)GWP_VP8_UVMODE_DC;
  best_any = GWP_FALSE;
  for (mode = 0u; mode < GWP_VP8_UV_MODE_COUNT; ++mode) {
    GWPu8 pred_u[64];
    GWPu8 pred_v[64];
    GWPu8 recon_u[64];
    GWPu8 recon_v[64];
    int q_u[4][16];
    int q_v[4][16];
    GWPBool any_coeff;
    GWPu32 cost;
    GWPu32 block;
    if (!enable_modes && mode != (GWPu32)GWP_VP8_UVMODE_DC) continue;
    any_coeff = GWP_FALSE;
    cost = GWPUVModePenalty(mode);
    GWPVP8Predict8x8((GWPVP8UVMode)mode, above_u, left_u, top_left_u, pred_u);
    GWPVP8Predict8x8((GWPVP8UVMode)mode, above_v, left_v, top_left_v, pred_v);
    for (block = 0u; block < 4u; ++block) {
      GWPu32 bx;
      GWPu32 by;
      GWPu32 r;
      GWPu8 pred_blk[16];
      GWPu8 tgt_blk[16];
      GWPu8 rec_blk[16];
      GWPBool has_coeff;
      bx = (block & 1u) * 4u;
      by = (block >> 1) * 4u;
      for (r = 0u; r < 4u; ++r) {
        GWPCopy(pred_blk + r * 4u, pred_u + (by + r) * 8u + bx, 4u);
        GWPCopy(tgt_blk + r * 4u, target_u + (by + r) * 8u + bx, 4u);
      }
      cost += GWPQuantizeSparsePredict4x4(pred_blk, tgt_blk, dc_q, ac_q, lambda,
                                          GWP_TRUE, enable_ac,
                                          q_u[block], &has_coeff, rec_blk);
      if (has_coeff) any_coeff = GWP_TRUE;
      for (r = 0u; r < 4u; ++r) GWPCopy(recon_u + (by + r) * 8u + bx, rec_blk + r * 4u, 4u);

      for (r = 0u; r < 4u; ++r) {
        GWPCopy(pred_blk + r * 4u, pred_v + (by + r) * 8u + bx, 4u);
        GWPCopy(tgt_blk + r * 4u, target_v + (by + r) * 8u + bx, 4u);
      }
      cost += GWPQuantizeSparsePredict4x4(pred_blk, tgt_blk, dc_q, ac_q, lambda,
                                          GWP_TRUE, enable_ac,
                                          q_v[block], &has_coeff, rec_blk);
      if (has_coeff) any_coeff = GWP_TRUE;
      for (r = 0u; r < 4u; ++r) GWPCopy(recon_v + (by + r) * 8u + bx, rec_blk + r * 4u, 4u);
    }
    if (cost < best_cost) {
      GWPu32 block2;
      GWPu32 i;
      best_cost = cost;
      best_mode = (GWPu8)mode;
      best_any = any_coeff;
      GWPCopy(best_u, recon_u, 64u);
      GWPCopy(best_v, recon_v, 64u);
      for (block2 = 0u; block2 < 4u; ++block2) {
        for (i = 0u; i < 16u; ++i) {
          best_q_u[block2][i] = q_u[block2][i];
          best_q_v[block2][i] = q_v[block2][i];
        }
      }
    }
  }
  *out_mode = best_mode;
  *out_any_coeff = best_any;
  GWPCopy(out_recon_u, best_u, 64u);
  GWPCopy(out_recon_v, best_v, 64u);
  {
    GWPu32 block;
    GWPu32 i;
    for (block = 0u; block < 4u; ++block) {
      for (i = 0u; i < 16u; ++i) {
        out_qcoeff_u[block][i] = best_q_u[block][i];
        out_qcoeff_v[block][i] = best_q_v[block][i];
      }
    }
  }
  return best_cost;
}

static GWPu32 GWPSelectBestIntra16Mode(const GWPu8 above[16],
                                       const GWPu8 left[16],
                                       GWPu8 top_left,
                                       const GWPu8 target[16 * 16],
                                       int y2_dc_q,
                                       int y1_ac_q,
                                       int lambda,
                                       GWPBool enable_ac,
                                       GWPu8* out_y_mode,
                                       int out_y2[16],
                                       int out_y_coeffs[16][16],
                                       GWPBool* out_any_coeff,
                                       GWPu8 out_recon[16 * 16]) {
  GWPu32 best_cost;
  GWPu8 best_mode;
  int best_y2[16];
  int best_y[16][16];
  GWPu8 best_recon[16 * 16];
  GWPBool best_any;
  GWPu32 mode;
  best_cost = 0xffffffffu;
  best_mode = (GWPu8)GWP_VP8_YMODE_DC;
  best_any = GWP_FALSE;
  for (mode = 0u; mode < 4u; ++mode) {
    GWPu8 pred16[16 * 16];
    GWPu8 recon16[16 * 16];
    int y2[16];
    int coeffs[16][16];
    GWPBool any_coeff;
    GWPu32 cost;
    int sum;
    int q0;
    int base_pixel;
    GWPu32 block;
    GWPVP8Predict16x16((GWPVP8YMode)mode, above, left, top_left, pred16);
    sum = 0;
    for (block = 0u; block < 256u; ++block) sum += (int)target[block] - (int)pred16[block];
    for (block = 0u; block < 16u; ++block) y2[block] = 0;
    q0 = GWPRoundDivSigned(sum, (y2_dc_q <= 0) ? 1 : (4 * y2_dc_q));
    q0 = GWPClampI(q0, -2047, 2047);
    y2[0] = q0;
    base_pixel = GWPComputeY2BasePixel(q0, y2_dc_q);
    any_coeff = (q0 != 0) ? GWP_TRUE : GWP_FALSE;
    cost = GWPYModePenalty(mode) * (GWPu32)lambda;
    if (q0 != 0) cost += (GWPu32)lambda;
    for (block = 0u; block < 16u; ++block) {
      GWPu32 bx;
      GWPu32 by;
      GWPu32 r;
      GWPu8 pred_blk[16];
      GWPu8 tgt_blk[16];
      GWPu8 rec_blk[16];
      GWPBool has_coeff;
      bx = block & 3u;
      by = block >> 2;
      for (r = 0u; r < 4u; ++r) {
        GWPu32 c;
        for (c = 0u; c < 4u; ++c) {
          int pv;
          pv = (int)pred16[(by * 4u + r) * 16u + bx * 4u + c] + base_pixel;
          pred_blk[r * 4u + c] = GWPClamp8Enc(pv);
          tgt_blk[r * 4u + c] = target[(by * 4u + r) * 16u + bx * 4u + c];
        }
      }
      cost += GWPQuantizeSparsePredict4x4(pred_blk, tgt_blk,
                                          y1_ac_q, y1_ac_q,
                                          lambda + (int)GWPYModePenalty(mode),
                                          GWP_FALSE, enable_ac,
                                          coeffs[block], &has_coeff, rec_blk);
      if (has_coeff) any_coeff = GWP_TRUE;
      for (r = 0u; r < 4u; ++r) GWPCopy(recon16 + (by * 4u + r) * 16u + bx * 4u, rec_blk + r * 4u, 4u);
    }
    if (cost < best_cost) {
      GWPu32 bi;
      GWPu32 i;
      best_cost = cost;
      best_mode = (GWPu8)mode;
      best_any = any_coeff;
      for (bi = 0u; bi < 16u; ++bi) best_y2[bi] = y2[bi];
      for (bi = 0u; bi < 16u; ++bi) {
        for (i = 0u; i < 16u; ++i) best_y[bi][i] = coeffs[bi][i];
      }
      GWPCopy(best_recon, recon16, 256u);
    }
  }
  *out_y_mode = best_mode;
  *out_any_coeff = best_any;
  {
    GWPu32 bi;
    GWPu32 i;
    for (bi = 0u; bi < 16u; ++bi) {
      out_y2[bi] = best_y2[bi];
      for (i = 0u; i < 16u; ++i) out_y_coeffs[bi][i] = best_y[bi][i];
    }
  }
  GWPCopy(out_recon, best_recon, 256u);
  return best_cost;
}

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

static GWPBool GWPFindTreePath(const int* tree,
                               int tree_size,
                               int node,
                               int value,
                               GWPu8* bits,
                               int depth,
                               int* out_depth) {
  int next0;
  int next1;
  if (tree == 0 || bits == 0 || out_depth == 0) return GWP_FALSE;
  if (node < 0 || node + 1 >= tree_size) return GWP_FALSE;
  next0 = tree[node + 0];
  bits[depth] = 0u;
  if (next0 <= 0) {
    if (-next0 == value) {
      *out_depth = depth + 1;
      return GWP_TRUE;
    }
  } else if (GWPFindTreePath(tree, tree_size, next0, value, bits, depth + 1, out_depth)) {
    return GWP_TRUE;
  }
  next1 = tree[node + 1];
  bits[depth] = 1u;
  if (next1 <= 0) {
    if (-next1 == value) {
      *out_depth = depth + 1;
      return GWP_TRUE;
    }
  } else if (GWPFindTreePath(tree, tree_size, next1, value, bits, depth + 1, out_depth)) {
    return GWP_TRUE;
  }
  return GWP_FALSE;
}

static void GWPAccumulateTreeCounts(GWPu32 counts[GWP_VP8_ENTROPY_NODES][2],
                                    const int* tree,
                                    int tree_size,
                                    int value) {
  GWPu8 bits[32];
  int depth;
  int i;
  int node;
  if (counts == 0 || tree == 0) return;
  if (!GWPFindTreePath(tree, tree_size, 0, value, bits, 0, &depth)) return;
  node = 0;
  for (i = 0; i < depth; ++i) {
    int prob_index;
    int next_index;
    prob_index = node >> 1;
    if (prob_index >= 0 && prob_index < (int)GWP_VP8_ENTROPY_NODES) {
      counts[prob_index][bits[i] ? 1 : 0] += 1u;
    }
    next_index = tree[node + (int)bits[i]];
    if (next_index <= 0) break;
    node = next_index;
  }
}

static void GWPAccumulateCoeffBlockStats(
    GWPVP8CoeffProbStats* stats,
    GWPu32 plane,
    GWPBool has_left,
    GWPBool has_above,
    int first_coeff,
    const int coeffs[16]) {
  GWPu32 prev_context;
  int prev_coeff_non_zero;
  int i;
  if (stats == 0 || coeffs == 0 || plane >= GWP_VP8_COEFF_BLOCK_TYPES) return;
  prev_context = (has_left ? 1u : 0u) + (has_above ? 1u : 0u);
  if (prev_context > 2u) prev_context = 2u;
  prev_coeff_non_zero = 1;
  for (i = first_coeff; i < 16; ++i) {
    GWPu32 band;
    int coeff;
    int token;
    coeff = coeffs[kGWPVP8ZigZag[i]];
    band = (GWPu32)kGWPVP8CoeffBands[i];
    if (coeff == 0) {
      int has_more;
      int j;
      has_more = 0;
      for (j = i + 1; j < 16; ++j) {
        if (coeffs[kGWPVP8ZigZag[j]] != 0) {
          has_more = 1;
          break;
        }
      }
      token = has_more ? (int)GWP_VP8_TOKEN_ZERO : (int)GWP_VP8_TOKEN_EOB;
      GWPAccumulateTreeCounts(stats->branch[plane][band][prev_context], kGWPVP8CoeffTree, 22, token);
      if (token == (int)GWP_VP8_TOKEN_EOB) return;
      prev_context = 0u;
      prev_coeff_non_zero = 0;
      continue;
    }
    token = GWPVP8CoeffTokenFromValue(coeff);
    GWPAccumulateTreeCounts(stats->branch[plane][band][prev_context], kGWPVP8CoeffTree, 22, token);
    {
      int abs_value;
      abs_value = (coeff < 0) ? -coeff : coeff;
      prev_context = (abs_value == 1) ? 1u : 2u;
      prev_coeff_non_zero = 1;
    }
    (void)prev_coeff_non_zero;
  }
}

static GWPu32 GWPBuildCoeffProbs(const GWPVP8CoeffProbStats* stats,
                                 GWPBool enable_updates,
                                 GWPu8 out_probs[GWP_VP8_COEFF_BLOCK_TYPES]
                                                 [GWP_VP8_COEFF_BANDS]
                                                 [GWP_VP8_PREV_COEFF_CONTEXTS]
                                                 [GWP_VP8_ENTROPY_NODES]) {
  GWPu32 i;
  GWPu32 j;
  GWPu32 k;
  GWPu32 l;
  GWPu32 updates;
  updates = 0u;
  for (i = 0u; i < GWP_VP8_COEFF_BLOCK_TYPES; ++i) {
    for (j = 0u; j < GWP_VP8_COEFF_BANDS; ++j) {
      for (k = 0u; k < GWP_VP8_PREV_COEFF_CONTEXTS; ++k) {
        for (l = 0u; l < GWP_VP8_ENTROPY_NODES; ++l) {
          GWPu8 def;
          GWPu8 prob;
          def = kGWPVP8DefaultCoeffProbs[i][j][k][l];
          prob = def;
          if (enable_updates && stats != 0) {
            GWPu32 zero_count;
            GWPu32 one_count;
            zero_count = stats->branch[i][j][k][l][0];
            one_count = stats->branch[i][j][k][l][1];
            if (zero_count + one_count >= 8u) {
              GWPu8 tuned;
              tuned = GWPProbFromCounts(zero_count, one_count, def);
              if ((tuned > def ? (tuned - def) : (def - tuned)) >= 8u) {
                prob = tuned;
                ++updates;
              }
            }
          }
          out_probs[i][j][k][l] = prob;
        }
      }
    }
  }
  return updates;
}

static void GWPWriteSegmentationHeader(GWPVP8BoolEncoder* bw,
                                       const GWPVP8SegmentHeader* seg) {
  GWPu32 i;
  if (bw == 0 || seg == 0) return;
  GWPVP8BoolEncWriteBit(bw, seg->enabled ? 1 : 0);
  if (!seg->enabled) return;
  GWPVP8BoolEncWriteBit(bw, seg->update_map ? 1 : 0);
  GWPVP8BoolEncWriteBit(bw, seg->update_data ? 1 : 0);
  if (seg->update_data) {
    GWPVP8BoolEncWriteBit(bw, seg->absolute_delta ? 1 : 0);
    for (i = 0u; i < 4u; ++i) GWPVP8BoolEncWriteMaybeInt(bw, seg->quant_idx[i], 7);
    for (i = 0u; i < 4u; ++i) GWPVP8BoolEncWriteMaybeInt(bw, seg->lf_level[i], 6);
  }
  if (seg->update_map) {
    for (i = 0u; i < GWP_VP8_MB_FEATURE_TREE_PROBS; ++i) {
      if (seg->tree_probs[i] != GWP_NATIVE_SEG_PROB_DEFAULT) {
        GWPVP8BoolEncWriteBit(bw, 1);
        GWPVP8BoolEncWriteUInt(bw, seg->tree_probs[i], 8);
      } else {
        GWPVP8BoolEncWriteBit(bw, 0);
      }
    }
  }
}

static GWPu8 GWPComputeSkipProb(GWPu32 macroblocks, GWPu32 skipped) {
  GWPu32 non_skip;
  if (macroblocks == 0u) return 128u;
  non_skip = macroblocks - skipped;
  return GWPProbFromCounts(non_skip, skipped, 128u);
}

static void GWPWriteEntropyHeader(
    GWPVP8BoolEncoder* bw,
    const GWPu8 coeff_probs[GWP_VP8_COEFF_BLOCK_TYPES]
                           [GWP_VP8_COEFF_BANDS]
                           [GWP_VP8_PREV_COEFF_CONTEXTS]
                           [GWP_VP8_ENTROPY_NODES],
    GWPBool coeff_skip_enabled,
    GWPu8 coeff_skip_prob,
    GWPu32* out_update_count) {
  GWPu32 i;
  GWPu32 j;
  GWPu32 k;
  GWPu32 l;
  GWPu32 updates;
  updates = 0u;
  if (bw == 0 || coeff_probs == 0) return;
  for (i = 0u; i < GWP_VP8_COEFF_BLOCK_TYPES; ++i) {
    for (j = 0u; j < GWP_VP8_COEFF_BANDS; ++j) {
      for (k = 0u; k < GWP_VP8_PREV_COEFF_CONTEXTS; ++k) {
        for (l = 0u; l < GWP_VP8_ENTROPY_NODES; ++l) {
          GWPu8 def;
          GWPu8 cur;
          def = kGWPVP8DefaultCoeffProbs[i][j][k][l];
          cur = coeff_probs[i][j][k][l];
          if (cur != def) {
            GWPVP8BoolEncWrite(bw, (int)kGWPVP8CoeffUpdateProbs[i][j][k][l], 1);
            GWPVP8BoolEncWriteUInt(bw, cur, 8);
            ++updates;
          } else {
            GWPVP8BoolEncWrite(bw, (int)kGWPVP8CoeffUpdateProbs[i][j][k][l], 0);
          }
        }
      }
    }
  }
  GWPVP8BoolEncWriteBit(bw, coeff_skip_enabled ? 1 : 0);
  if (coeff_skip_enabled) GWPVP8BoolEncWriteUInt(bw, coeff_skip_prob, 8);
  if (out_update_count != 0) *out_update_count = updates;
}

static void GWPStoreBlock4x4(GWPu8* plane,
                             GWPu32 stride,
                             GWPu32 x0,
                             GWPu32 y0,
                             const GWPu8 block[16]) {
  GWPu32 r;
  for (r = 0u; r < 4u; ++r) GWPCopy(plane + (y0 + r) * stride + x0, block + r * 4u, 4u);
}

static void GWPStoreBlock8x8(GWPu8* plane,
                             GWPu32 stride,
                             GWPu32 x0,
                             GWPu32 y0,
                             const GWPu8 block[64]) {
  GWPu32 r;
  for (r = 0u; r < 8u; ++r) GWPCopy(plane + (y0 + r) * stride + x0, block + r * 8u, 8u);
}

static GWPStatusCode GWPAnalyzeMacroblocks(const GWPu8* pixels,
                                           GWPu32 width,
                                           GWPu32 height,
                                           GWPu32 stride,
                                           GWPRawPixelFormat pixel_format,
                                           const GWPEncodeConfig* config,
                                           const GWPVP8LossyPlan* plan,
                                           const GWPVP8SegmentHeader* seg,
                                           const GWPVP8DequantFactors dq[4],
                                           GWPu32 mb_cols,
                                           GWPu32 mb_rows,
                                           GWPu8* recon_y,
                                           GWPu8* recon_u,
                                           GWPu8* recon_v,
                                           GWPu32 y_stride,
                                           GWPu32 uv_stride,
                                           GWPVP8MBDecision* decisions,
                                           GWPVP8CoeffProbStats* coeff_stats,
                                           GWPVP8EncodeAnalysis* analysis) {
  GWPu32 mb_y;
  GWPu8 above_ctx[GWP_VP8_MAX_MB_COLS * 9u];
  if (pixels == 0 || config == 0 || dq == 0 || recon_y == 0 || recon_u == 0 || recon_v == 0 ||
      decisions == 0 || coeff_stats == 0 || analysis == 0) {
    return GWP_STATUS_INVALID_PARAM;
  }
  GWPZero(coeff_stats, (GWPu32)sizeof(*coeff_stats));
  GWPZero(analysis, (GWPu32)sizeof(*analysis));
  GWPZero(above_ctx, mb_cols * 9u);
  for (mb_y = 0u; mb_y < mb_rows; ++mb_y) {
    GWPu32 mb_x;
    GWPu8 left_ctx[9];
    GWPZero(left_ctx, 9u);
    for (mb_x = 0u; mb_x < mb_cols; ++mb_x) {
      GWPu32 mb_index;
      GWPVP8MBDecision* dec;
      GWPu32 segment_id;
      GWPu8 uv_above_u[8];
      GWPu8 uv_left_u[8];
      GWPu8 uv_above_v[8];
      GWPu8 uv_left_v[8];
      GWPu8 uv_top_left_u;
      GWPu8 uv_top_left_v;
      GWPu8 target_u[64];
      GWPu8 target_v[64];
      GWPu8 recon_blk_u[64];
      GWPu8 recon_blk_v[64];
      GWPBool uv_any_coeff;
      int lambda;
      GWPBool mb_has_coeff;
      mb_index = mb_y * mb_cols + mb_x;
      dec = &decisions[mb_index];
      GWPZero(dec, (GWPu32)sizeof(*dec));
      segment_id = GWPMapPlanSegment(plan, mb_index, seg->enabled ? 4u : 1u);
      if (!seg->enabled) segment_id = 0u;
      dec->segment_id = (GWPu8)segment_id;
      analysis->segment_hist[segment_id] += 1u;
      lambda = 2 + dq[segment_id].quant_idx / 10;
      if (lambda < 1) lambda = 1;
      mb_has_coeff = GWP_FALSE;

      {
        GWPu8 bpred_modes[16];
        int bpred_y[16][16];
        GWPu8 bpred_recon[256];
        GWPBool bpred_any;
        GWPu32 bpred_cost;
        GWPu32 by;
        GWPu8 i16_recon[256];
        int i16_y[16][16];
        int i16_y2[16];
        GWPu8 i16_mode;
        GWPBool i16_any;
        GWPu32 i16_cost;
        GWPBool use_i16;
        GWPu32 block;
        bpred_any = GWP_FALSE;
        bpred_cost = 0u;
        for (by = 0u; by < 4u; ++by) {
          GWPu32 bx;
          for (bx = 0u; bx < 4u; ++bx) {
            GWPu32 block_index;
            GWPu32 x0;
            GWPu32 y0;
            GWPu8 above[8];
            GWPu8 left[4];
            GWPu8 top_left;
            GWPu8 target[16];
            GWPu8 recon_block[16];
            GWPu8 mode;
            GWPBool has_coeff;
            block_index = by * 4u + bx;
            x0 = mb_x * 16u + bx * 4u;
            y0 = mb_y * 16u + by * 4u;
            GWPGetAbove4x4(recon_y, y_stride, x0, y0, 127u, above, &top_left);
            GWPGetLeft4x4(recon_y, y_stride, x0, y0, 127u, left);
            GWPGetLumaTarget4x4(pixels, width, height, stride, pixel_format, x0, y0, target);
            bpred_cost += GWPSelectBestBMode(above, left, top_left, target,
                                             dq[segment_id].y1_dc,
                                             dq[segment_id].y1_ac,
                                             lambda,
                                             config->vp8_enable_subblock_modes,
                                             config->vp8_enable_ac_coeffs,
                                             &mode,
                                             bpred_y[block_index],
                                             &has_coeff,
                                             recon_block);
            bpred_modes[block_index] = mode;
            if (has_coeff) bpred_any = GWP_TRUE;
            for (block = 0u; block < 16u; ++block) {
              bpred_recon[(by * 4u + (block >> 2)) * 16u + bx * 4u + (block & 3u)] = recon_block[block];
            }
          }
        }

        i16_any = GWP_FALSE;
        i16_cost = 0xffffffffu;
        i16_mode = (GWPu8)GWP_VP8_YMODE_DC;
        for (block = 0u; block < 16u; ++block) i16_y2[block] = 0;
        if (config->vp8_enable_intra16_modes) {
          GWPu8 above16[16];
          GWPu8 left16[16];
          GWPu8 top_left16;
          GWPu8 target16[256];
          GWPGetAbove16x16(recon_y, y_stride, mb_x * 16u, mb_y * 16u, 127u, above16, &top_left16);
          GWPGetLeft16x16(recon_y, y_stride, mb_x * 16u, mb_y * 16u, 127u, left16);
          GWPGetLumaTarget16x16(pixels, width, height, stride, pixel_format, mb_x * 16u, mb_y * 16u, target16);
          i16_cost = GWPSelectBestIntra16Mode(above16, left16, top_left16, target16,
                                              dq[segment_id].y2_dc,
                                              dq[segment_id].y1_ac,
                                              lambda,
                                              config->vp8_enable_ac_coeffs,
                                              &i16_mode,
                                              i16_y2,
                                              i16_y,
                                              &i16_any,
                                              i16_recon);
        }

        use_i16 = (config->vp8_enable_intra16_modes && i16_cost < bpred_cost) ? GWP_TRUE : GWP_FALSE;
        if (use_i16) {
          GWPu8 fill_mode;
          dec->y_mode = i16_mode;
          analysis->y_mode_mask |= (1u << dec->y_mode);
          fill_mode = kGWPVP8YModeToBMode[dec->y_mode];
          for (block = 0u; block < 16u; ++block) {
            GWPu32 i;
            dec->b_modes[block] = fill_mode;
            for (i = 0u; i < 16u; ++i) dec->y_qcoeff[block][i] = i16_y[block][i];
            dec->y2_qcoeff[block] = i16_y2[block];
            if (GWPCountNonZeroCoeffBlock(dec->y_qcoeff[block]) != 0u) analysis->y_non_zero += 1u;
          }
          if (GWPCountNonZeroCoeffBlock(dec->y2_qcoeff) != 0u) analysis->y2_non_zero += 1u;
          mb_has_coeff = i16_any;
          GWPCopy(recon_y + (mb_y * 16u) * y_stride + mb_x * 16u, i16_recon + 0u, 16u);
          {
            GWPu32 r;
            for (r = 1u; r < 16u; ++r) GWPCopy(recon_y + (mb_y * 16u + r) * y_stride + mb_x * 16u,
                                              i16_recon + r * 16u, 16u);
          }
        } else {
          dec->y_mode = (GWPu8)GWP_VP8_YMODE_B_PRED;
          analysis->y_mode_mask |= (1u << dec->y_mode);
          for (block = 0u; block < 16u; ++block) {
            GWPu32 i;
            dec->b_modes[block] = bpred_modes[block];
            analysis->b_mode_mask |= (1u << dec->b_modes[block]);
            for (i = 0u; i < 16u; ++i) dec->y_qcoeff[block][i] = bpred_y[block][i];
            dec->y2_qcoeff[block] = 0;
            if (GWPCountNonZeroCoeffBlock(dec->y_qcoeff[block]) != 0u) analysis->y_non_zero += 1u;
          }
          mb_has_coeff = bpred_any;
          {
            GWPu32 r;
            for (r = 0u; r < 16u; ++r) GWPCopy(recon_y + (mb_y * 16u + r) * y_stride + mb_x * 16u,
                                              bpred_recon + r * 16u, 16u);
          }
        }
      }

      GWPGetAbove8x8(recon_u, uv_stride, mb_x * 8u, mb_y * 8u, 128u, uv_above_u, &uv_top_left_u);
      GWPGetLeft8x8(recon_u, uv_stride, mb_x * 8u, mb_y * 8u, 128u, uv_left_u);
      GWPGetAbove8x8(recon_v, uv_stride, mb_x * 8u, mb_y * 8u, 128u, uv_above_v, &uv_top_left_v);
      GWPGetLeft8x8(recon_v, uv_stride, mb_x * 8u, mb_y * 8u, 128u, uv_left_v);
      GWPGetChromaTarget8x8(pixels, width, height, stride, pixel_format,
                            mb_x * 16u, mb_y * 16u, target_u, target_v);
      GWPSelectBestUVMode(uv_above_u, uv_left_u, uv_top_left_u,
                          uv_above_v, uv_left_v, uv_top_left_v,
                          target_u, target_v,
                          dq[segment_id].uv_dc,
                          dq[segment_id].uv_ac,
                          lambda,
                          config->vp8_enable_uv_modes,
                          config->vp8_enable_ac_coeffs,
                          &dec->uv_mode,
                          dec->u_qcoeff,
                          dec->v_qcoeff,
                          &uv_any_coeff,
                          recon_blk_u,
                          recon_blk_v);
      if (uv_any_coeff) mb_has_coeff = GWP_TRUE;
      analysis->uv_mode_mask |= (1u << dec->uv_mode);
      {
        GWPu32 b;
        for (b = 0u; b < 4u; ++b) {
          if (GWPCountNonZeroCoeffBlock(dec->u_qcoeff[b]) != 0u) analysis->uv_non_zero += 1u;
          if (GWPCountNonZeroCoeffBlock(dec->v_qcoeff[b]) != 0u) analysis->uv_non_zero += 1u;
        }
      }
      GWPStoreBlock8x8(recon_u, uv_stride, mb_x * 8u, mb_y * 8u, recon_blk_u);
      GWPStoreBlock8x8(recon_v, uv_stride, mb_x * 8u, mb_y * 8u, recon_blk_v);

      dec->skip_coeff = (config->vp8_enable_coeff_skip && !mb_has_coeff) ? 1u : 0u;
      if (dec->skip_coeff) analysis->skip_count += 1u;
      else {
        GWPu32 block_index;
        GWPBool has_y2;
        has_y2 = (dec->y_mode != (GWPu8)GWP_VP8_YMODE_B_PRED) ? GWP_TRUE : GWP_FALSE;
        if (has_y2) {
          GWPBool has_coeff;
          has_coeff = (GWPCountNonZeroCoeffBlock(dec->y2_qcoeff) != 0u) ? GWP_TRUE : GWP_FALSE;
          GWPAccumulateCoeffBlockStats(coeff_stats,
                                       1u,
                                       left_ctx[8] ? GWP_TRUE : GWP_FALSE,
                                       above_ctx[mb_x * 9u + 8u] ? GWP_TRUE : GWP_FALSE,
                                       0,
                                       dec->y2_qcoeff);
          left_ctx[8] = has_coeff ? 1u : 0u;
          above_ctx[mb_x * 9u + 8u] = has_coeff ? 1u : 0u;
        }
        for (block_index = 0u; block_index < 16u; ++block_index) {
          GWPBool has_coeff;
          GWPu32 l;
          GWPu32 a;
          l = kGWPVP8LeftContextIndex[block_index];
          a = kGWPVP8AboveContextIndex[block_index];
          has_coeff = (GWPCountNonZeroCoeffBlock(dec->y_qcoeff[block_index]) != 0u) ? GWP_TRUE : GWP_FALSE;
          GWPAccumulateCoeffBlockStats(coeff_stats,
                                       has_y2 ? 0u : 3u,
                                       left_ctx[l] ? GWP_TRUE : GWP_FALSE,
                                       above_ctx[mb_x * 9u + a] ? GWP_TRUE : GWP_FALSE,
                                       has_y2 ? 1 : 0,
                                       dec->y_qcoeff[block_index]);
          left_ctx[l] = has_coeff ? 1u : 0u;
          above_ctx[mb_x * 9u + a] = has_coeff ? 1u : 0u;
        }
        for (block_index = 0u; block_index < 4u; ++block_index) {
          GWPBool has_coeff;
          GWPu32 idx;
          GWPu32 l;
          GWPu32 a;
          idx = 16u + block_index;
          l = kGWPVP8LeftContextIndex[idx];
          a = kGWPVP8AboveContextIndex[idx];
          has_coeff = (GWPCountNonZeroCoeffBlock(dec->u_qcoeff[block_index]) != 0u) ? GWP_TRUE : GWP_FALSE;
          GWPAccumulateCoeffBlockStats(coeff_stats,
                                       2u,
                                       left_ctx[l] ? GWP_TRUE : GWP_FALSE,
                                       above_ctx[mb_x * 9u + a] ? GWP_TRUE : GWP_FALSE,
                                       0,
                                       dec->u_qcoeff[block_index]);
          left_ctx[l] = has_coeff ? 1u : 0u;
          above_ctx[mb_x * 9u + a] = has_coeff ? 1u : 0u;

          has_coeff = (GWPCountNonZeroCoeffBlock(dec->v_qcoeff[block_index]) != 0u) ? GWP_TRUE : GWP_FALSE;
          GWPAccumulateCoeffBlockStats(coeff_stats,
                                       2u,
                                       left_ctx[l] ? GWP_TRUE : GWP_FALSE,
                                       above_ctx[mb_x * 9u + a] ? GWP_TRUE : GWP_FALSE,
                                       0,
                                       dec->v_qcoeff[block_index]);
          left_ctx[l] = has_coeff ? 1u : 0u;
          above_ctx[mb_x * 9u + a] = has_coeff ? 1u : 0u;
        }
      }
      if (dec->skip_coeff) {
        GWPu32 block_index;
        for (block_index = 0u; block_index < 24u; ++block_index) {
          GWPu32 l;
          GWPu32 a;
          l = kGWPVP8LeftContextIndex[block_index];
          a = kGWPVP8AboveContextIndex[block_index];
          left_ctx[l] = 0u;
          above_ctx[mb_x * 9u + a] = 0u;
        }
      }
    }
  }
  return GWP_STATUS_OK;
}

static GWPStatusCode GWPEncodeModePartition(GWPVP8BoolEncoder* bw,
                                            const GWPVP8SegmentHeader* seg,
                                            GWPBool coeff_skip_enabled,
                                            GWPu8 coeff_skip_prob,
                                            const GWPVP8MBDecision* decisions,
                                            GWPu32 mb_cols,
                                            GWPu32 mb_rows) {
  GWPu8 above_pred[GWP_VP8_MAX_MB_COLS * 4u];
  GWPu8 left_pred[4];
  GWPu32 mb_y;
  GWPu32 i;
  if (bw == 0 || decisions == 0) return GWP_STATUS_INVALID_PARAM;
  for (i = 0u; i < mb_cols * 4u; ++i) above_pred[i] = (GWPu8)GWP_VP8_BMODE_DC;
  for (mb_y = 0u; mb_y < mb_rows; ++mb_y) {
    GWPu32 mb_x;
    for (mb_x = 0u; mb_x < 4u; ++mb_x) left_pred[mb_x] = (GWPu8)GWP_VP8_BMODE_DC;
    for (mb_x = 0u; mb_x < mb_cols; ++mb_x) {
      const GWPVP8MBDecision* dec;
      GWPu32 row;
      GWPu32 col;
      dec = &decisions[mb_y * mb_cols + mb_x];
      if (seg != 0 && seg->enabled && seg->update_map) {
        if (!GWPVP8WriteTreeValue(bw, kGWPVP8MbSegmentTree, seg->tree_probs, 6, (int)dec->segment_id)) {
          return GWP_STATUS_NOT_ENOUGH_OUTPUT;
        }
      }
      if (coeff_skip_enabled) {
        GWPVP8BoolEncWrite(bw, (int)coeff_skip_prob, dec->skip_coeff ? 1 : 0);
      }
      if (!GWPVP8WriteTreeValue(bw, kGWPVP8KfYModeTree, kGWPVP8KfYModeProbs, 8, (int)dec->y_mode)) {
        return GWP_STATUS_NOT_ENOUGH_OUTPUT;
      }
      if (dec->y_mode == (GWPu8)GWP_VP8_YMODE_B_PRED) {
        for (row = 0u; row < 4u; ++row) {
          for (col = 0u; col < 4u; ++col) {
            GWPu32 block_index;
            GWPu8 a;
            GWPu8 l;
            block_index = row * 4u + col;
            if (row == 0u) a = above_pred[mb_x * 4u + col];
            else a = dec->b_modes[(row - 1u) * 4u + col];
            if (col == 0u) l = left_pred[row];
            else l = dec->b_modes[row * 4u + (col - 1u)];
            if (!GWPVP8WriteTreeValue(bw,
                                      kGWPVP8BModeTree,
                                      kGWPVP8KfBModeProbs[a][l],
                                      18,
                                      (int)dec->b_modes[block_index])) {
              return GWP_STATUS_NOT_ENOUGH_OUTPUT;
            }
          }
        }
      }
      if (!GWPVP8WriteTreeValue(bw, kGWPVP8UVModeTree, kGWPVP8KfUVModeProbs, 6, (int)dec->uv_mode)) {
        return GWP_STATUS_NOT_ENOUGH_OUTPUT;
      }
      if (dec->y_mode == (GWPu8)GWP_VP8_YMODE_B_PRED) {
        for (col = 0u; col < 4u; ++col) {
          above_pred[mb_x * 4u + col] = dec->b_modes[12u + col];
          left_pred[col] = dec->b_modes[col * 4u + 3u];
        }
      } else {
        GWPu8 fill_mode;
        fill_mode = kGWPVP8YModeToBMode[dec->y_mode];
        for (col = 0u; col < 4u; ++col) {
          above_pred[mb_x * 4u + col] = fill_mode;
          left_pred[col] = fill_mode;
        }
      }
    }
  }
  return GWPVP8BoolEncOk(bw) ? GWP_STATUS_OK : GWP_STATUS_NOT_ENOUGH_OUTPUT;
}

static GWPStatusCode GWPEncodeTokenPartitions(
    GWPVP8BoolEncoder* encoders,
    GWPu32 partition_count,
    const GWPu8 coeff_probs[GWP_VP8_COEFF_BLOCK_TYPES]
                           [GWP_VP8_COEFF_BANDS]
                           [GWP_VP8_PREV_COEFF_CONTEXTS]
                           [GWP_VP8_ENTROPY_NODES],
    const GWPVP8MBDecision* decisions,
    GWPu32 mb_cols,
    GWPu32 mb_rows,
    GWPVP8NativeEncodeStats* out_stats) {
  GWPu32 mb_y;
  GWPu8 above_ctx[GWP_VP8_MAX_MB_COLS * 9u];
  GWPZero(above_ctx, mb_cols * 9u);
  if (encoders == 0 || decisions == 0) return GWP_STATUS_INVALID_PARAM;
  for (mb_y = 0u; mb_y < mb_rows; ++mb_y) {
    GWPu32 mb_x;
    GWPVP8BoolEncoder* bw;
    GWPu8 left_ctx[9];
    GWPZero(left_ctx, 9u);
    bw = &encoders[mb_y & (partition_count - 1u)];
    for (mb_x = 0u; mb_x < mb_cols; ++mb_x) {
      const GWPVP8MBDecision* dec;
      GWPu32 block_index;
      GWPBool has_y2;
      dec = &decisions[mb_y * mb_cols + mb_x];
      has_y2 = (dec->y_mode != (GWPu8)GWP_VP8_YMODE_B_PRED) ? GWP_TRUE : GWP_FALSE;
      if (dec->skip_coeff) {
        for (block_index = 0u; block_index < 24u; ++block_index) {
          GWPu32 l;
          GWPu32 a;
          l = kGWPVP8LeftContextIndex[block_index];
          a = kGWPVP8AboveContextIndex[block_index];
          left_ctx[l] = 0u;
          above_ctx[mb_x * 9u + a] = 0u;
        }
        left_ctx[8] = 0u;
        above_ctx[mb_x * 9u + 8u] = 0u;
        continue;
      }
      if (has_y2) {
        GWPBool has_coeffs;
        GWPStatusCode st;
        st = GWPVP8WriteCoeffBlock(bw,
                                   coeff_probs,
                                   1u,
                                   left_ctx[8] ? GWP_TRUE : GWP_FALSE,
                                   above_ctx[mb_x * 9u + 8u] ? GWP_TRUE : GWP_FALSE,
                                   0,
                                   dec->y2_qcoeff,
                                   &has_coeffs);
        if (st != GWP_STATUS_OK) return st;
        left_ctx[8] = has_coeffs ? 1u : 0u;
        above_ctx[mb_x * 9u + 8u] = has_coeffs ? 1u : 0u;
        if (out_stats != 0 && has_coeffs) out_stats->y2_blocks_non_zero += 1u;
      }
      for (block_index = 0u; block_index < 16u; ++block_index) {
        GWPBool has_coeffs;
        GWPStatusCode st;
        GWPu32 l;
        GWPu32 a;
        l = kGWPVP8LeftContextIndex[block_index];
        a = kGWPVP8AboveContextIndex[block_index];
        st = GWPVP8WriteCoeffBlock(bw,
                                   coeff_probs,
                                   has_y2 ? 0u : 3u,
                                   left_ctx[l] ? GWP_TRUE : GWP_FALSE,
                                   above_ctx[mb_x * 9u + a] ? GWP_TRUE : GWP_FALSE,
                                   has_y2 ? 1 : 0,
                                   dec->y_qcoeff[block_index],
                                   &has_coeffs);
        if (st != GWP_STATUS_OK) return st;
        left_ctx[l] = has_coeffs ? 1u : 0u;
        above_ctx[mb_x * 9u + a] = has_coeffs ? 1u : 0u;
        if (out_stats != 0 && has_coeffs) out_stats->y_blocks_non_zero += 1u;
      }
      for (block_index = 0u; block_index < 4u; ++block_index) {
        GWPBool has_coeffs;
        GWPStatusCode st;
        GWPu32 idx;
        GWPu32 l;
        GWPu32 a;
        idx = 16u + block_index;
        l = kGWPVP8LeftContextIndex[idx];
        a = kGWPVP8AboveContextIndex[idx];
        st = GWPVP8WriteCoeffBlock(bw,
                                   coeff_probs,
                                   2u,
                                   left_ctx[l] ? GWP_TRUE : GWP_FALSE,
                                   above_ctx[mb_x * 9u + a] ? GWP_TRUE : GWP_FALSE,
                                   0,
                                   dec->u_qcoeff[block_index],
                                   &has_coeffs);
        if (st != GWP_STATUS_OK) return st;
        left_ctx[l] = has_coeffs ? 1u : 0u;
        above_ctx[mb_x * 9u + a] = has_coeffs ? 1u : 0u;
        if (out_stats != 0 && has_coeffs) out_stats->uv_blocks_non_zero += 1u;

        st = GWPVP8WriteCoeffBlock(bw,
                                   coeff_probs,
                                   2u,
                                   left_ctx[l] ? GWP_TRUE : GWP_FALSE,
                                   above_ctx[mb_x * 9u + a] ? GWP_TRUE : GWP_FALSE,
                                   0,
                                   dec->v_qcoeff[block_index],
                                   &has_coeffs);
        if (st != GWP_STATUS_OK) return st;
        left_ctx[l] = has_coeffs ? 1u : 0u;
        above_ctx[mb_x * 9u + a] = has_coeffs ? 1u : 0u;
        if (out_stats != 0 && has_coeffs) out_stats->uv_blocks_non_zero += 1u;
      }
      if (out_stats != 0) out_stats->macroblocks += 1u;
    }
  }
  return GWP_STATUS_OK;
}

GWPStatusCode GWPEncodeVP8Bitstream(const GWPu8* pixels,
                                    GWPu32 width,
                                    GWPu32 height,
                                    GWPu32 stride,
                                    GWPRawPixelFormat pixel_format,
                                    const GWPEncodeConfig* config,
                                    GWPu8* out_buf,
                                    GWPu32 out_buf_size,
                                    GWPu32* out_size,
                                    GWPVP8NativeEncodeStats* out_stats) {
  GWPu32 mb_cols;
  GWPu32 mb_rows;
  GWPu32 mb_count;
  GWPu32 y_stride;
  GWPu32 uv_stride;
  GWPu32 y_plane_size;
  GWPu32 uv_plane_size;
  GWPu32 scratch_offset;
  GWPu32 plan_need;
  GWPu8* scratch_bytes;
  GWPu8* plan_mem;
  GWPVP8MBDecision* decisions;
  GWPu8* recon_y;
  GWPu8* recon_u;
  GWPu8* recon_v;
  GWPu8* part0_buf;
  GWPu32 part0_cap;
  GWPu32 partition_count;
  GWPu32 segment_count;
  GWPu32 q_index;
  GWPu32 filter_level;
  GWPu32 sharpness;
  GWPu32 coeff_updates;
  GWPu8 coeff_skip_prob;
  GWPu8 coeff_probs[GWP_VP8_COEFF_BLOCK_TYPES][GWP_VP8_COEFF_BANDS][GWP_VP8_PREV_COEFF_CONTEXTS][GWP_VP8_ENTROPY_NODES];
  GWPVP8BoolEncoder bw0;
  GWPVP8BoolEncoder token_bw[GWP_VP8_MAX_PARTITIONS];
  GWPu8* token_ptrs[GWP_VP8_MAX_PARTITIONS];
  GWPu32 token_caps[GWP_VP8_MAX_PARTITIONS];
  GWPu32 token_sizes[GWP_VP8_MAX_PARTITIONS];
  GWPVP8LossyPlan plan;
  GWPEncodeConfig local_cfg;
  GWPVP8SegmentHeader seg;
  GWPVP8QuantHeader quant;
  GWPVP8DequantFactors dq[4];
  GWPVP8CoeffProbStats coeff_stats;
  GWPVP8EncodeAnalysis analysis;
  GWPStatusCode st;
  GWPu32 i;
  GWPu32 part0_size;
  GWPu32 token_total_size;
  GWPu32 raw_size;
  GWPu32 table_bytes;
  GWPu32 token_cap_total;

  if (out_size != 0) *out_size = 0u;
  if (pixels == 0 || out_buf == 0 || out_size == 0) return GWP_STATUS_INVALID_PARAM;
  if (width == 0u || height == 0u) return GWP_STATUS_BAD_DIMENSIONS;
  if (width > GWP_MAX_IMAGE_WIDTH || height > GWP_MAX_IMAGE_HEIGHT) return GWP_STATUS_BAD_DIMENSIONS;

  if (config == 0) {
    GWPEncodeConfigInit(&local_cfg);
    local_cfg.lossless = GWP_FALSE;
    config = &local_cfg;
  }

  if (config->scratch == 0 || config->scratch_size < GWPEstimateNativeLossyScratch(width, height)) {
    return GWP_STATUS_NOT_ENOUGH_SCRATCH;
  }
  if (out_buf_size < GWPEstimateNativeVP8BitstreamSize(width, height)) {
    return GWP_STATUS_NOT_ENOUGH_OUTPUT;
  }

  mb_cols = (width + 15u) >> 4;
  mb_rows = (height + 15u) >> 4;
  if (!GWPMulU32(mb_cols, mb_rows, &mb_count)) return GWP_STATUS_LIMIT_EXCEEDED;
  y_stride = mb_cols * 16u;
  uv_stride = mb_cols * 8u;
  if (!GWPMulU32(y_stride, mb_rows * 16u, &y_plane_size)) return GWP_STATUS_LIMIT_EXCEEDED;
  if (!GWPMulU32(uv_stride, mb_rows * 8u, &uv_plane_size)) return GWP_STATUS_LIMIT_EXCEEDED;

  GWPVP8LossyPlanInit(&plan);
  plan_need = GWPEstimateVP8LossyPlanScratch(width, height);
  scratch_bytes = (GWPu8*)config->scratch;
  plan_mem = scratch_bytes;
  st = GWPAnalyzeVP8LossyPlan(pixels, width, height, stride, pixel_format,
                              config, plan_mem, plan_need, &plan);
  if (st != GWP_STATUS_OK) {
    GWPZero(&plan, (GWPu32)sizeof(plan));
    plan.macroblocks = 0;
    plan.macroblock_count = 0u;
  }

  q_index = GWPChooseNativeQIndex(config, &plan);
  filter_level = GWPChooseNativeFilter(config, &plan);
  sharpness = GWPChooseNativeSharpness(config, &plan);
  partition_count = GWPChooseNativePartitionCount(config, &plan, mb_rows);
  segment_count = GWPChooseNativeSegmentCount(config, &plan);
  GWPSetupSegmentHeader(config, &plan, segment_count, q_index, filter_level, &seg);
  if (!seg.enabled) segment_count = 1u;

  GWPZero(&quant, (GWPu32)sizeof(quant));
  quant.q_index = (GWPu8)q_index;
  GWPVP8InitDequantFactors(&seg, &quant, dq);

  scratch_offset = plan_need;
  decisions = (GWPVP8MBDecision*)(scratch_bytes + scratch_offset);
  scratch_offset += mb_count * (GWPu32)sizeof(*decisions);
  recon_y = scratch_bytes + scratch_offset;
  scratch_offset += y_plane_size;
  recon_u = scratch_bytes + scratch_offset;
  scratch_offset += uv_plane_size;
  recon_v = scratch_bytes + scratch_offset;
  scratch_offset += uv_plane_size;
  part0_buf = scratch_bytes + scratch_offset;
  part0_cap = 8192u + mb_count * 64u;
  if (scratch_offset + part0_cap >= config->scratch_size) return GWP_STATUS_NOT_ENOUGH_SCRATCH;
  scratch_offset += part0_cap;
  token_cap_total = config->scratch_size - scratch_offset - 64u;
  if (token_cap_total < 1024u * partition_count) return GWP_STATUS_NOT_ENOUGH_SCRATCH;

  for (i = 0u; i < y_plane_size; ++i) recon_y[i] = 127u;
  for (i = 0u; i < uv_plane_size; ++i) recon_u[i] = recon_v[i] = 128u;

  st = GWPAnalyzeMacroblocks(pixels,
                             width,
                             height,
                             stride,
                             pixel_format,
                             config,
                             &plan,
                             &seg,
                             dq,
                             mb_cols,
                             mb_rows,
                             recon_y,
                             recon_u,
                             recon_v,
                             y_stride,
                             uv_stride,
                             decisions,
                             &coeff_stats,
                             &analysis);
  if (st != GWP_STATUS_OK) return st;

  coeff_updates = GWPBuildCoeffProbs(&coeff_stats, config->vp8_enable_prob_updates, coeff_probs);
  coeff_skip_prob = GWPComputeSkipProb(mb_count, analysis.skip_count);

  GWPVP8BoolEncInit(&bw0, part0_buf, part0_cap);
  GWPVP8BoolEncWriteBit(&bw0, 0);
  GWPVP8BoolEncWriteBit(&bw0, 0);
  GWPWriteSegmentationHeader(&bw0, &seg);
  GWPVP8BoolEncWriteBit(&bw0, 0);
  GWPVP8BoolEncWriteUInt(&bw0, filter_level, 6);
  GWPVP8BoolEncWriteUInt(&bw0, sharpness, 3);
  GWPVP8BoolEncWriteBit(&bw0, 0);
  GWPVP8BoolEncWriteUInt(&bw0, (partition_count == 8u) ? 3u : (partition_count == 4u ? 2u : (partition_count == 2u ? 1u : 0u)), 2);
  GWPVP8BoolEncWriteUInt(&bw0, q_index, 7);
  GWPVP8BoolEncWriteBit(&bw0, 0);
  GWPVP8BoolEncWriteBit(&bw0, 0);
  GWPVP8BoolEncWriteBit(&bw0, 0);
  GWPVP8BoolEncWriteBit(&bw0, 0);
  GWPVP8BoolEncWriteBit(&bw0, 0);
  GWPVP8BoolEncWriteBit(&bw0, 0);
  GWPWriteEntropyHeader(&bw0,
                        coeff_probs,
                        config->vp8_enable_coeff_skip,
                        coeff_skip_prob,
                        &coeff_updates);
  st = GWPEncodeModePartition(&bw0,
                              &seg,
                              config->vp8_enable_coeff_skip,
                              coeff_skip_prob,
                              decisions,
                              mb_cols,
                              mb_rows);
  if (st != GWP_STATUS_OK) return st;
  GWPVP8BoolEncFlush(&bw0);
  if (!GWPVP8BoolEncOk(&bw0)) return GWP_STATUS_NOT_ENOUGH_SCRATCH;
  part0_size = GWPVP8BoolEncSize(&bw0);

  for (i = 0u; i < partition_count; ++i) {
    token_ptrs[i] = scratch_bytes + scratch_offset + (token_cap_total / partition_count) * i;
    token_caps[i] = token_cap_total / partition_count;
    if (i == partition_count - 1u) {
      token_caps[i] = config->scratch_size - (GWPu32)(token_ptrs[i] - scratch_bytes) - 32u;
    }
    GWPVP8BoolEncInit(&token_bw[i], token_ptrs[i], token_caps[i]);
  }
  st = GWPEncodeTokenPartitions(token_bw,
                                partition_count,
                                coeff_probs,
                                decisions,
                                mb_cols,
                                mb_rows,
                                out_stats);
  if (st != GWP_STATUS_OK) return st;
  token_total_size = 0u;
  for (i = 0u; i < partition_count; ++i) {
    GWPVP8BoolEncFlush(&token_bw[i]);
    if (!GWPVP8BoolEncOk(&token_bw[i])) return GWP_STATUS_NOT_ENOUGH_SCRATCH;
    token_sizes[i] = GWPVP8BoolEncSize(&token_bw[i]);
    token_total_size += token_sizes[i];
  }

  table_bytes = 3u * (partition_count - 1u);
  raw_size = 10u + part0_size + table_bytes + token_total_size;
  if (raw_size > out_buf_size) return GWP_STATUS_NOT_ENOUGH_OUTPUT;
  GWPWritePartitionTag(out_buf, part0_size);
  out_buf[3] = 0x9du;
  out_buf[4] = 0x01u;
  out_buf[5] = 0x2au;
  GWPWriteLE16(out_buf + 6, (GWPu16)(width & 0x3fffu));
  GWPWriteLE16(out_buf + 8, (GWPu16)(height & 0x3fffu));
  GWPCopy(out_buf + 10u, part0_buf, part0_size);
  {
    GWPu32 header_pos;
    GWPu32 data_pos;
    header_pos = 10u + part0_size;
    data_pos = header_pos + table_bytes;
    for (i = 0u; i + 1u < partition_count; ++i) {
      GWPWriteLE24(out_buf + header_pos + i * 3u, token_sizes[i]);
    }
    for (i = 0u; i < partition_count; ++i) {
      GWPCopy(out_buf + data_pos, token_ptrs[i], token_sizes[i]);
      data_pos += token_sizes[i];
    }
  }
  *out_size = raw_size;

  if (out_stats != 0) {
    out_stats->width = width;
    out_stats->height = height;
    out_stats->mb_cols = mb_cols;
    out_stats->mb_rows = mb_rows;
    out_stats->q_index = q_index;
    out_stats->filter_level = filter_level;
    out_stats->sharpness = sharpness;
    out_stats->first_partition_size = part0_size;
    out_stats->token_partition_size = token_total_size;
    out_stats->token_partition_count = partition_count;
    out_stats->segment_count = segment_count;
    out_stats->skipped_macroblocks = analysis.skip_count;
    out_stats->coeff_prob_updates = coeff_updates;
    out_stats->y_mode_mask = analysis.y_mode_mask;
    out_stats->b_mode_mask = analysis.b_mode_mask;
    out_stats->uv_mode_mask = analysis.uv_mode_mask;
    out_stats->y2_blocks_non_zero = analysis.y2_non_zero;
  }
  return GWP_STATUS_OK;
}
