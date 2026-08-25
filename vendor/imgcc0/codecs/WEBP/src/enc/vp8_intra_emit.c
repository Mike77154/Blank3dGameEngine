#include "vp8_intra_emit.h"

#include "vp8_lossy_plan.h"
#include "vp8l_enc.h"
#include "../utils/common.h"
#include "../utils/endian.h"

static void GWPReadRawPixelEmit(const GWPu8* p,
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

static GWPu32 GWPClampEmitU32(GWPu32 v, GWPu32 lo, GWPu32 hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static GWPu32 GWPToLumaEmit(GWPu8 r, GWPu8 g, GWPu8 b) {
  return (19595u * (GWPu32)r + 38470u * (GWPu32)g + 7471u * (GWPu32)b + 32768u) >> 16;
}

static GWPu32 GWPToUEmit(GWPu8 r, GWPu8 g, GWPu8 b) {
  GWPs32 u;
  u = ((-11058 * (GWPs32)r - 21710 * (GWPs32)g + 32768 * (GWPs32)b + 32768) >> 16) + 128;
  if (u < 0) u = 0;
  if (u > 255) u = 255;
  return (GWPu32)u;
}

static GWPu32 GWPToVEmit(GWPu8 r, GWPu8 g, GWPu8 b) {
  GWPs32 v;
  v = ((32768 * (GWPs32)r - 27439 * (GWPs32)g - 5329 * (GWPs32)b + 32768) >> 16) + 128;
  if (v < 0) v = 0;
  if (v > 255) v = 255;
  return (GWPu32)v;
}

static GWPu32 GWPChooseCellSize(const GWPVP8LossyMacroblockStat* st,
                                const GWPEncodeConfig* config) {
  GWPu32 quality;
  if (st == 0 || config == 0) return 16u;
  quality = GWPClampEmitU32(config->quality, 0u, 100u);
  if (st->segment_id == 0u) {
    if (quality >= 80u && st->variance_y > 24u) return 8u;
    return 16u;
  }
  if (st->segment_id == 1u) {
    if (quality >= 88u || config->preset == GWP_PRESET_TEXT ||
        config->preset == GWP_PRESET_ICON) {
      return 4u;
    }
    return 8u;
  }
  if (st->segment_id >= 2u) return 4u;
  return 8u;
}

static GWPu32 GWPCellQIndex(const GWPVP8LossyMacroblockStat* st,
                            const GWPEncodeConfig* config) {
  GWPu32 quality;
  GWPu32 base;
  GWPu32 q;
  quality = GWPClampEmitU32(config->quality, 0u, 100u);
  base = 8u + ((100u - quality) * 9u) / 4u;
  q = base + (GWPu32)st->segment_id * 8u;
  if (config->preset == GWP_PRESET_PHOTO) q += 4u;
  if (config->preset == GWP_PRESET_TEXT || config->preset == GWP_PRESET_ICON) {
    if (q > 6u) q -= 6u;
  }
  if (st->has_alpha && q > 2u) q -= 2u;
  return GWPClampEmitU32(q, 0u, 127u);
}

static GWPu32 GWPQuantStepFromQIndex(GWPu32 qindex) {
  GWPu32 step;
  step = 1u + qindex / 12u;
  return GWPClampEmitU32(step, 1u, 16u);
}

static GWPu32 GWPQuantizeComponent(GWPu32 v, GWPu32 step) {
  if (step <= 1u) return v;
  v = ((v + (step >> 1)) / step) * step;
  if (v > 255u) v = 255u;
  return v;
}

static void GWPPaintProxyCell(GWPu8* rgba,
                              GWPu32 frame_width,
                              const GWPVP8IntraCell* cell) {
  GWPu32 x, y;
  if (rgba == 0 || cell == 0) return;
  for (y = 0u; y < (GWPu32)cell->height; ++y) {
    GWPu8* row;
    row = rgba + (((GWPu32)cell->y + y) * frame_width + (GWPu32)cell->x) * 4u;
    for (x = 0u; x < (GWPu32)cell->width; ++x) {
      row[4u * x + 0u] = cell->r;
      row[4u * x + 1u] = cell->g;
      row[4u * x + 2u] = cell->b;
      row[4u * x + 3u] = cell->a;
    }
  }
}

void GWPVP8IntraEmitPlanInit(GWPVP8IntraEmitPlan* plan) {
  if (plan == 0) return;
  GWPZero(plan, (GWPu32)sizeof(*plan));
}

GWPu32 GWPEstimateVP8IntraEmitScratch(GWPu32 width, GWPu32 height) {
  GWPu32 mb_cols;
  GWPu32 mb_rows;
  GWPu32 mb_count;
  GWPu32 plan_need;
  GWPu32 cells_need;
  mb_cols = (width + 15u) >> 4;
  mb_rows = (height + 15u) >> 4;
  if (!GWPMulU32(mb_cols, mb_rows, &mb_count)) return 0xffffffffu;
  plan_need = GWPEstimateVP8LossyPlanScratch(width, height);
  if (!GWPMulU32(mb_count * 16u, (GWPu32)sizeof(GWPVP8IntraCell), &cells_need)) {
    return 0xffffffffu;
  }
  if (!GWPAddU32(plan_need, cells_need, &plan_need)) return 0xffffffffu;
  if (!GWPAddU32(plan_need, 64u, &plan_need)) return 0xffffffffu;
  return plan_need;
}

GWPu32 GWPEstimateNativeLossyScratch(GWPu32 width, GWPu32 height) {
  GWPu32 emit_need;
  GWPu32 rgba_need;
  GWPu32 native_need;
  GWPu32 decision_need;
  GWPu32 y_stride;
  GWPu32 uv_stride;
  GWPu32 y_plane;
  GWPu32 uv_plane;
  GWPu32 mb_cols;
  GWPu32 mb_rows;
  GWPu32 mb_count;
  emit_need = GWPEstimateVP8IntraEmitScratch(width, height);
  if (emit_need == 0xffffffffu) return emit_need;
  mb_cols = (width + 15u) >> 4;
  mb_rows = (height + 15u) >> 4;
  if (!GWPMulU32(mb_cols, mb_rows, &mb_count)) return 0xffffffffu;
  if (!GWPMulU32(mb_count, 2048u, &decision_need)) return 0xffffffffu;
  if (!GWPAddU32(emit_need, decision_need, &emit_need)) return 0xffffffffu;
  if (!GWPMulU32(width * 4u, height, &rgba_need)) return 0xffffffffu;
  if (!GWPAddU32(emit_need, rgba_need, &emit_need)) return 0xffffffffu;
  if (!GWPAddU32(emit_need, 64u, &emit_need)) return 0xffffffffu;

  y_stride = mb_cols * 16u;
  uv_stride = mb_cols * 8u;
  if (!GWPMulU32(y_stride, mb_rows * 16u, &y_plane)) return 0xffffffffu;
  if (!GWPMulU32(uv_stride, mb_rows * 8u, &uv_plane)) return 0xffffffffu;
  native_need = y_plane + uv_plane + uv_plane;
  native_need += 8192u + mb_cols * mb_rows * 64u;
  native_need += decision_need;
  native_need += GWPEstimateNativeVP8BitstreamSize(width, height);
  native_need += 512u;
  if (native_need > emit_need) emit_need = native_need;
  return emit_need;
}

GWPStatusCode GWPBuildVP8IntraEmitPlan(const GWPu8* pixels,
                                       GWPu32 width,
                                       GWPu32 height,
                                       GWPu32 stride,
                                       GWPRawPixelFormat pixel_format,
                                       const GWPEncodeConfig* config,
                                       void* scratch,
                                       GWPu32 scratch_size,
                                       GWPVP8IntraEmitPlan* out_plan) {
  GWPu32 plan_need;
  GWPu32 mb_cols;
  GWPu32 mb_rows;
  GWPu32 mb_x;
  GWPu32 mb_y;
  GWPu32 cell_count;
  GWPu32 avg_q;
  GWPu8* scratch_bytes;
  GWPVP8LossyPlan lossy_plan;
  GWPVP8IntraCell* cells;
  GWPu32 max_cells;
  if (pixels == 0 || config == 0 || scratch == 0 || out_plan == 0) {
    return GWP_STATUS_INVALID_PARAM;
  }
  if (width == 0u || height == 0u) return GWP_STATUS_BAD_DIMENSIONS;
  plan_need = GWPEstimateVP8LossyPlanScratch(width, height);
  if (plan_need == 0xffffffffu) return GWP_STATUS_LIMIT_EXCEEDED;
  if (scratch_size < GWPEstimateVP8IntraEmitScratch(width, height)) {
    return GWP_STATUS_NOT_ENOUGH_SCRATCH;
  }
  scratch_bytes = (GWPu8*)scratch;
  if (GWPVP8BuildLossyPlan(pixels, width, height, stride, pixel_format,
                           config, scratch_bytes, plan_need, &lossy_plan) != GWP_STATUS_OK) {
    return GWP_STATUS_NOT_ENOUGH_SCRATCH;
  }
  mb_cols = lossy_plan.mb_cols;
  mb_rows = lossy_plan.mb_rows;
  max_cells = mb_cols * mb_rows * 16u;
  cells = (GWPVP8IntraCell*)(scratch_bytes + plan_need + 32u);
  cell_count = 0u;
  avg_q = 0u;
  for (mb_y = 0u; mb_y < mb_rows; ++mb_y) {
    for (mb_x = 0u; mb_x < mb_cols; ++mb_x) {
      GWPu32 x0, y0;
      GWPu32 cell_size;
      GWPu32 step_x, step_y;
      const GWPVP8LossyMacroblockStat* st;
      st = &lossy_plan.macroblocks[mb_y * mb_cols + mb_x];
      x0 = mb_x * 16u;
      y0 = mb_y * 16u;
      cell_size = GWPChooseCellSize(st, config);
      step_x = cell_size;
      step_y = cell_size;
      for (step_y = 0u; step_y < 16u; step_y += cell_size) {
        for (step_x = 0u; step_x < 16u; step_x += cell_size) {
          GWPu32 cx0, cy0, cx1, cy1;
          GWPu32 x, y;
          GWPu32 sum_r, sum_g, sum_b, sum_a;
          GWPu32 count;
          GWPu32 qindex;
          GWPu32 qstep;
          GWPVP8IntraCell* cell;
          if (cell_count >= max_cells) return GWP_STATUS_LIMIT_EXCEEDED;
          cx0 = x0 + step_x;
          cy0 = y0 + step_y;
          cx1 = cx0 + cell_size;
          cy1 = cy0 + cell_size;
          if (cx0 >= width || cy0 >= height) continue;
          if (cx1 > width) cx1 = width;
          if (cy1 > height) cy1 = height;
          sum_r = sum_g = sum_b = sum_a = 0u;
          count = 0u;
          for (y = cy0; y < cy1; ++y) {
            const GWPu8* row;
            row = pixels + y * stride;
            for (x = cx0; x < cx1; ++x) {
              GWPu8 r, g, b, a;
              GWPReadRawPixelEmit(row + 4u * x, pixel_format, &r, &g, &b, &a);
              sum_r += r;
              sum_g += g;
              sum_b += b;
              sum_a += a;
              ++count;
            }
          }
          if (count == 0u) continue;
          qindex = GWPCellQIndex(st, config);
          qstep = GWPQuantStepFromQIndex(qindex);
          cell = &cells[cell_count++];
          cell->x = (GWPu16)cx0;
          cell->y = (GWPu16)cy0;
          cell->width = (GWPu8)(cx1 - cx0);
          cell->height = (GWPu8)(cy1 - cy0);
          cell->segment_id = st->segment_id;
          cell->mode = (cell_size == 16u) ? 0u : ((cell_size == 8u) ? 1u : 2u);
          cell->qindex = (GWPu8)qindex;
          cell->alpha_present = st->has_alpha;
          cell->r = (GWPu8)GWPQuantizeComponent(sum_r / count, qstep);
          cell->g = (GWPu8)GWPQuantizeComponent(sum_g / count, qstep);
          cell->b = (GWPu8)GWPQuantizeComponent(sum_b / count, qstep);
          cell->a = (GWPu8)((sum_a + (count >> 1)) / count);
          if (!config->exact && cell->a == 0u) {
            cell->r = 0u;
            cell->g = 0u;
            cell->b = 0u;
          }
          cell->y_avg = (GWPu8)GWPToLumaEmit(cell->r, cell->g, cell->b);
          cell->u_avg = (GWPu8)GWPToUEmit(cell->r, cell->g, cell->b);
          cell->v_avg = (GWPu8)GWPToVEmit(cell->r, cell->g, cell->b);
          cell->reserved = 0u;
          avg_q += qindex;
        }
      }
    }
  }
  GWPVP8IntraEmitPlanInit(out_plan);
  out_plan->width = width;
  out_plan->height = height;
  out_plan->mb_cols = mb_cols;
  out_plan->mb_rows = mb_rows;
  out_plan->cell_count = cell_count;
  out_plan->cells_capacity = max_cells;
  out_plan->average_qindex = (cell_count != 0u) ? (avg_q / cell_count) : 0u;
  out_plan->suggested_partitions = lossy_plan.suggested_partitions;
  out_plan->suggested_filter_strength = lossy_plan.suggested_filter_strength;
  out_plan->lossy_plan = lossy_plan;
  out_plan->cells = cells;
  return GWP_STATUS_OK;
}

GWPStatusCode GWPEncodeLossyNativeProxy(const GWPu8* pixels,
                                        GWPu32 width,
                                        GWPu32 height,
                                        GWPu32 stride,
                                        GWPRawPixelFormat pixel_format,
                                        const GWPEncodeConfig* config,
                                        GWPu32* out_size) {
  GWPu32 emit_need;
  GWPu32 rgba_need;
  GWPu8* scratch_bytes;
  GWPu8* rgba;
  GWPVP8IntraEmitPlan plan;
  GWPVP8LBitstreamOptions opts;
  GWPu32 bitstream_size;
  GWPBool has_alpha;
  GWPStatusCode st;
  GWPu32 i;
  if (config == 0 || config->output_buffer == 0 || out_size == 0) {
    return GWP_STATUS_INVALID_PARAM;
  }
  *out_size = 0u;
  emit_need = GWPEstimateVP8IntraEmitScratch(width, height);
  if (emit_need == 0xffffffffu) return GWP_STATUS_LIMIT_EXCEEDED;
  if (!GWPMulU32(width * 4u, height, &rgba_need)) return GWP_STATUS_LIMIT_EXCEEDED;
  if (config->scratch == 0 || config->scratch_size < emit_need + rgba_need + 32u) {
    return GWP_STATUS_NOT_ENOUGH_SCRATCH;
  }
  if (config->output_buffer_size < 20u) return GWP_STATUS_NOT_ENOUGH_OUTPUT;
  scratch_bytes = (GWPu8*)config->scratch;
  rgba = scratch_bytes + emit_need;
  GWPZero(rgba, rgba_need);
  st = GWPBuildVP8IntraEmitPlan(pixels, width, height, stride, pixel_format,
                                config, scratch_bytes, emit_need, &plan);
  if (st != GWP_STATUS_OK) return st;
  for (i = 0u; i < plan.cell_count; ++i) {
    GWPPaintProxyCell(rgba, width, &plan.cells[i]);
  }

  GWPVP8LBitstreamOptionsInit(&opts);
  opts.exact = config->exact;
  opts.near_lossless = 100u;
  opts.use_subtract_green = GWP_TRUE;
  opts.use_color_cache = (config->method >= 2u) ? GWP_TRUE : GWP_FALSE;
  opts.color_cache_bits = (config->method >= 5u) ? 5u : 4u;
  opts.use_backrefs = GWP_TRUE;

  st = GWPEncodeVP8LBitstreamEx(rgba, width, height, width * 4u, GWP_RAW_RGBA,
                                &opts,
                                config->output_buffer + 20u,
                                config->output_buffer_size - 20u,
                                &bitstream_size,
                                &has_alpha);
  if (st != GWP_STATUS_OK) return st;

  GWPWriteLE32(config->output_buffer + 0u, GWP_FOURCC_RIFF);
  GWPWriteLE32(config->output_buffer + 4u, 12u + GWP_ALIGN2(bitstream_size));
  GWPWriteLE32(config->output_buffer + 8u, GWP_FOURCC_WEBP);
  GWPWriteLE32(config->output_buffer + 12u, GWP_FOURCC_VP8L);
  GWPWriteLE32(config->output_buffer + 16u, bitstream_size);
  if ((bitstream_size & 1u) != 0u) {
    config->output_buffer[20u + bitstream_size] = 0u;
  }
  *out_size = 20u + GWP_ALIGN2(bitstream_size);
  (void)has_alpha;
  return GWP_STATUS_OK;
}
