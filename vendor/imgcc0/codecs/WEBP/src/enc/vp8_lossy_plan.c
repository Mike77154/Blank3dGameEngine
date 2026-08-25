#include "vp8_lossy_plan.h"

#include "../utils/common.h"

static void GWPReadRawPixelPlan(const GWPu8* p,
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

static GWPu32 GWPToLuma(GWPu8 r, GWPu8 g, GWPu8 b) {
  return (19595u * (GWPu32)r + 38470u * (GWPu32)g + 7471u * (GWPu32)b + 32768u) >> 16;
}

static GWPu32 GWPClampU32Range(GWPu32 v, GWPu32 lo, GWPu32 hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static GWPu32 GWPSegmentFromVariance(GWPu32 variance, GWPu32 quality, GWPEncodePreset preset) {
  GWPu32 t0;
  GWPu32 t1;
  GWPu32 t2;
  t0 = 12u + (100u - quality) / 8u;
  t1 = 64u + (100u - quality) / 3u;
  t2 = 160u + (100u - quality);
  if (preset == GWP_PRESET_ICON || preset == GWP_PRESET_TEXT || preset == GWP_PRESET_DRAWING) {
    if (t0 > 4u) t0 -= 4u;
    if (t1 > 10u) t1 -= 10u;
    if (t2 > 24u) t2 -= 24u;
  }
  if (variance < t0) return 0u;
  if (variance < t1) return 1u;
  if (variance < t2) return 2u;
  return 3u;
}

static GWPu32 GWPFilterFromStats(GWPu32 variance, GWPu32 quality, GWPEncodePreset preset, GWPBool has_alpha) {
  GWPu32 filter;
  filter = (100u - quality) / 2u;
  if (variance < 24u) filter += 28u;
  else if (variance < 96u) filter += 16u;
  else if (variance < 256u) filter += 8u;
  if (preset == GWP_PRESET_PHOTO || preset == GWP_PRESET_PICTURE) filter += 6u;
  if (preset == GWP_PRESET_TEXT || preset == GWP_PRESET_ICON) {
    if (filter > 8u) filter -= 8u;
  }
  if (has_alpha && filter > 4u) filter -= 4u;
  return GWPClampU32Range(filter, 0u, 63u);
}

void GWPVP8LossyPlanInit(GWPVP8LossyPlan* plan) {
  if (plan == 0) return;
  GWPZero(plan, (GWPu32)sizeof(*plan));
}

GWPu32 GWPEstimateVP8LossyPlanScratch(GWPu32 width, GWPu32 height) {
  GWPu32 mb_cols;
  GWPu32 mb_rows;
  GWPu32 count;
  GWPu32 bytes;
  mb_cols = (width + 15u) >> 4;
  mb_rows = (height + 15u) >> 4;
  if (!GWPMulU32(mb_cols, mb_rows, &count)) return 0xffffffffu;
  if (!GWPMulU32(count, (GWPu32)sizeof(GWPVP8LossyMacroblockStat), &bytes)) return 0xffffffffu;
  if (!GWPAddU32(bytes, 64u, &bytes)) return 0xffffffffu;
  return bytes;
}

GWPStatusCode GWPVP8BuildLossyPlan(const GWPu8* pixels,
                                   GWPu32 width,
                                   GWPu32 height,
                                   GWPu32 stride,
                                   GWPRawPixelFormat pixel_format,
                                   const GWPEncodeConfig* config,
                                   void* scratch,
                                   GWPu32 scratch_size,
                                   GWPVP8LossyPlan* out_plan) {
  GWPu32 mb_cols;
  GWPu32 mb_rows;
  GWPu32 mb_count;
  GWPu32 need;
  GWPu32 mb_x;
  GWPu32 mb_y;
  GWPu32 total_luma;
  GWPu32 total_pixels;
  GWPu32 total_var;
  GWPu32 alpha_mbs;
  GWPu32 quality;
  GWPEncodePreset preset;
  GWPVP8LossyMacroblockStat* stats;

  if (pixels == 0 || out_plan == 0 || config == 0) return GWP_STATUS_INVALID_PARAM;
  if (width == 0u || height == 0u || width > GWP_MAX_IMAGE_WIDTH || height > GWP_MAX_IMAGE_HEIGHT) {
    return GWP_STATUS_BAD_DIMENSIONS;
  }

  mb_cols = (width + 15u) >> 4;
  mb_rows = (height + 15u) >> 4;
  if (!GWPMulU32(mb_cols, mb_rows, &mb_count)) return GWP_STATUS_LIMIT_EXCEEDED;
  need = GWPEstimateVP8LossyPlanScratch(width, height);
  if (scratch == 0 || scratch_size < need) return GWP_STATUS_NOT_ENOUGH_SCRATCH;
  stats = (GWPVP8LossyMacroblockStat*)scratch;
  GWPZero(stats, mb_count * (GWPu32)sizeof(*stats));

  quality = GWPClampU32Range(config->quality, 0u, 100u);
  preset = config->preset;
  total_luma = 0u;
  total_pixels = 0u;
  total_var = 0u;
  alpha_mbs = 0u;

  for (mb_y = 0u; mb_y < mb_rows; ++mb_y) {
    for (mb_x = 0u; mb_x < mb_cols; ++mb_x) {
      GWPu32 x0, y0, x1, y1, x, y;
      GWPu32 sum_y, sum_sq, count;
      GWPu32 mean_y, var_y;
      GWPBool has_alpha;
      GWPVP8LossyMacroblockStat* st;
      x0 = mb_x * 16u;
      y0 = mb_y * 16u;
      x1 = x0 + 16u;
      y1 = y0 + 16u;
      if (x1 > width) x1 = width;
      if (y1 > height) y1 = height;
      sum_y = 0u;
      sum_sq = 0u;
      count = 0u;
      has_alpha = GWP_FALSE;
      for (y = y0; y < y1; ++y) {
        const GWPu8* row;
        row = pixels + y * stride;
        for (x = x0; x < x1; ++x) {
          GWPu8 r, g, b, a;
          GWPu32 lum;
          GWPReadRawPixelPlan(row + 4u * x, pixel_format, &r, &g, &b, &a);
          lum = GWPToLuma(r, g, b);
          sum_y += lum;
          sum_sq += lum * lum;
          ++count;
          if (a != 255u) has_alpha = GWP_TRUE;
        }
      }
      mean_y = (count != 0u) ? (sum_y / count) : 0u;
      var_y = 0u;
      if (count != 0u) {
        GWPu32 mean_sq;
        mean_sq = (sum_sq / count);
        if (mean_sq > mean_y * mean_y) var_y = mean_sq - mean_y * mean_y;
      }
      st = &stats[mb_y * mb_cols + mb_x];
      st->mean_y = (GWPu16)mean_y;
      st->variance_y = (GWPu16)GWPClampU32Range(var_y, 0u, 65535u);
      st->segment_id = (GWPu8)GWPSegmentFromVariance(var_y, quality, preset);
      st->filter_strength = (GWPu8)GWPFilterFromStats(var_y, quality, preset, has_alpha);
      st->has_alpha = has_alpha ? 1u : 0u;
      total_luma += sum_y;
      total_pixels += count;
      total_var += var_y;
      if (has_alpha) ++alpha_mbs;
    }
  }

  GWPVP8LossyPlanInit(out_plan);
  out_plan->width = width;
  out_plan->height = height;
  out_plan->quality = quality;
  out_plan->mb_cols = mb_cols;
  out_plan->mb_rows = mb_rows;
  out_plan->macroblock_count = mb_count;
  out_plan->average_luma = (total_pixels != 0u) ? (total_luma / total_pixels) : 0u;
  out_plan->average_variance = (mb_count != 0u) ? (total_var / mb_count) : 0u;
  out_plan->alpha_macroblocks = alpha_mbs;
  out_plan->macroblocks = stats;
  out_plan->macroblocks_capacity = mb_count;
  if (out_plan->average_variance < 32u) out_plan->suggested_segments = 1u;
  else if (out_plan->average_variance < 128u) out_plan->suggested_segments = 2u;
  else if (out_plan->average_variance < 320u) out_plan->suggested_segments = 3u;
  else out_plan->suggested_segments = 4u;
  if (mb_count < 64u) out_plan->suggested_partitions = 1u;
  else if (out_plan->average_variance < 80u) out_plan->suggested_partitions = 2u;
  else if (mb_count < 512u) out_plan->suggested_partitions = 4u;
  else out_plan->suggested_partitions = 8u;
  out_plan->suggested_filter_strength = GWPFilterFromStats(out_plan->average_variance,
                                                           quality,
                                                           preset,
                                                           alpha_mbs != 0u);
  if (preset == GWP_PRESET_TEXT || preset == GWP_PRESET_ICON) out_plan->suggested_sharpness = 4u;
  else if (preset == GWP_PRESET_DRAWING) out_plan->suggested_sharpness = 3u;
  else if (quality >= 85u) out_plan->suggested_sharpness = 2u;
  else out_plan->suggested_sharpness = 0u;
  return GWP_STATUS_OK;
}
