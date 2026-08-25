#include "vp8_picture.h"

#include "vp8_filter.h"
#include "vp8_yuv.h"
#include "../utils/common.h"

static void GWPVP8FillPlane(GWPu8* plane,
                            GWPu32 stride,
                            GWPu32 height,
                            GWPu8 value) {
  GWPu32 y;
  if (plane == 0) return;
  for (y = 0u; y < height; ++y) {
    GWPu32 x;
    GWPu8* row;
    row = plane + y * stride;
    for (x = 0u; x < stride; ++x) row[x] = value;
  }
}

static int GWPVP8ClampFilterLevel(int level) {
  if (level < 0) return 0;
  if (level > 63) return 63;
  return level;
}

static int GWPVP8ComputeFilterLevel(const GWPVP8ControlHeader* header,
                                    GWPu32 segment_id,
                                    int y_mode) {
  int level;
  if (header == 0) return 0;
  level = (int)header->loop_filter.level;
  if (header->segment.enabled && segment_id < GWP_VP8_MAX_SEGMENTS) {
    if (header->segment.absolute_delta) level = header->segment.lf_level[segment_id];
    else level += header->segment.lf_level[segment_id];
  }
  if (header->loop_filter.delta_enabled) {
    level += header->loop_filter.ref_delta[0];
    if (y_mode == (int)GWP_VP8_YMODE_B_PRED) level += header->loop_filter.mode_delta[0];
  }
  return GWPVP8ClampFilterLevel(level);
}

GWPStatusCode GWPVP8FrameBufferInit(const GWPVP8ControlHeader* header,
                                    const GWPDecoderOptions* options,
                                    GWPArena* arena,
                                    GWPVP8FrameBuffer* fb) {
  GWPu32 y_size;
  GWPu32 uv_size;
  GWPu32 mb_count;
  GWPu32 min_output;
  GWPBool need_output;
  if (header == 0 || options == 0 || arena == 0 || fb == 0) {
    return GWP_STATUS_INVALID_PARAM;
  }
  GWPZero(fb, (GWPu32)sizeof(*fb));
  need_output = (options->output_buffer != 0) ? GWP_TRUE : GWP_FALSE;
  fb->width = header->frame.key.width;
  fb->height = header->frame.key.height;
  fb->mb_cols = header->mb_cols;
  fb->mb_rows = header->mb_rows;
  fb->y_stride = header->mb_cols * 16u;
  fb->y_height = header->mb_rows * 16u;
  fb->uv_stride = header->mb_cols * 8u;
  fb->uv_height = header->mb_rows * 8u;
  if (!GWPMulU32(fb->y_stride, fb->y_height, &y_size)) return GWP_STATUS_LIMIT_EXCEEDED;
  if (!GWPMulU32(fb->uv_stride, fb->uv_height, &uv_size)) return GWP_STATUS_LIMIT_EXCEEDED;
  if (need_output) {
    if (!GWPMulU32(fb->width, 4u, &min_output)) return GWP_STATUS_LIMIT_EXCEEDED;
    if (options->output_stride < min_output) return GWP_STATUS_NOT_ENOUGH_OUTPUT;
    if (!GWPMulU32(options->output_stride, fb->height, &min_output)) return GWP_STATUS_LIMIT_EXCEEDED;
    if (options->output_buffer_size < min_output) return GWP_STATUS_NOT_ENOUGH_OUTPUT;
  }
  if (!GWPMulU32(fb->mb_cols, fb->mb_rows, &mb_count)) return GWP_STATUS_LIMIT_EXCEEDED;

  fb->y = (GWPu8*)GWPArenaAlloc(arena, y_size, 4u);
  fb->u = (GWPu8*)GWPArenaAlloc(arena, uv_size, 4u);
  fb->v = (GWPu8*)GWPArenaAlloc(arena, uv_size, 4u);
  fb->mb_segment_ids = (GWPu8*)GWPArenaAlloc(arena, mb_count, 1u);
  fb->mb_y_modes = (GWPu8*)GWPArenaAlloc(arena, mb_count, 1u);
  fb->mb_has_coeffs = (GWPu8*)GWPArenaAlloc(arena, mb_count, 1u);
  fb->mb_filter_levels = (GWPu8*)GWPArenaAlloc(arena, mb_count, 1u);
  if (fb->y == 0 || fb->u == 0 || fb->v == 0 || fb->mb_segment_ids == 0 ||
      fb->mb_y_modes == 0 || fb->mb_has_coeffs == 0 || fb->mb_filter_levels == 0) {
    return GWP_STATUS_NOT_ENOUGH_SCRATCH;
  }
  GWPVP8FrameBufferFillDefaults(fb);
  return GWP_STATUS_OK;
}

void GWPVP8FrameBufferFillDefaults(GWPVP8FrameBuffer* fb) {
  GWPu32 mb_count;
  GWPu32 i;
  if (fb == 0) return;
  GWPVP8FillPlane(fb->y, fb->y_stride, fb->y_height, 127u);
  GWPVP8FillPlane(fb->u, fb->uv_stride, fb->uv_height, 128u);
  GWPVP8FillPlane(fb->v, fb->uv_stride, fb->uv_height, 128u);
  mb_count = fb->mb_cols * fb->mb_rows;
  for (i = 0u; i < mb_count; ++i) {
    fb->mb_segment_ids[i] = 0u;
    fb->mb_y_modes[i] = 0u;
    fb->mb_has_coeffs[i] = 0u;
    fb->mb_filter_levels[i] = 0u;
  }
}

void GWPVP8FrameBufferFilter(const GWPVP8ControlHeader* header,
                             GWPVP8FrameBuffer* fb) {
  GWPu32 mb_y;
  if (header == 0 || fb == 0) return;
  for (mb_y = 0u; mb_y < fb->mb_rows; ++mb_y) {
    GWPu32 mb_x;
    for (mb_x = 0u; mb_x < fb->mb_cols; ++mb_x) {
      GWPu32 idx;
      int level;
      int interior_limit;
      int hev_threshold;
      int mb_limit;
      int sub_limit;
      GWPBool filter_subblocks;
      GWPu32 x;
      GWPu32 y;
      GWPu32 ux;
      GWPu32 uy;
      int y_vert_count;
      int y_horiz_count;
      int uv_vert_count;
      int uv_horiz_count;
      idx = mb_y * fb->mb_cols + mb_x;
      level = GWPVP8ComputeFilterLevel(header,
                                       fb->mb_segment_ids[idx],
                                       (int)fb->mb_y_modes[idx]);
      fb->mb_filter_levels[idx] = (GWPu8)level;
      if (level <= 0) continue;
      interior_limit = GWPVP8LoopFilterInteriorLimit(level,
                                                     (int)header->loop_filter.sharpness);
      hev_threshold = GWPVP8LoopFilterHevThreshold(level,
                                                   header->frame.tag.key_frame ? GWP_TRUE : GWP_FALSE);
      mb_limit = GWPVP8LoopFilterMBEdgeLimit(level, interior_limit);
      sub_limit = GWPVP8LoopFilterSubblockEdgeLimit(level, interior_limit);
      filter_subblocks = (fb->mb_has_coeffs[idx] != 0u ||
                          fb->mb_y_modes[idx] == (GWPu8)GWP_VP8_YMODE_B_PRED) ? GWP_TRUE : GWP_FALSE;
      x = mb_x * 16u;
      y = mb_y * 16u;
      ux = mb_x * 8u;
      uy = mb_y * 8u;
      y_vert_count = (x < fb->width) ? (int)((fb->height - y) < 16u ? (fb->height - y) : 16u) : 0;
      y_horiz_count = (y < fb->height) ? (int)((fb->width - x) < 16u ? (fb->width - x) : 16u) : 0;
      uv_vert_count = (ux < ((fb->width + 1u) >> 1)) ?
          (int)(((((fb->height + 1u) >> 1) - uy) < 8u) ? (((fb->height + 1u) >> 1) - uy) : 8u) : 0;
      uv_horiz_count = (uy < ((fb->height + 1u) >> 1)) ?
          (int)(((((fb->width + 1u) >> 1) - ux) < 8u) ? (((fb->width + 1u) >> 1) - ux) : 8u) : 0;
      if (y_vert_count <= 0 && y_horiz_count <= 0) continue;

      if (header->loop_filter.use_simple) {
        if (mb_x > 0u && x < fb->width && y_vert_count > 0) {
          GWPVP8SimpleFilterVerticalEdge(fb->y + y * fb->y_stride + x,
                                         fb->y_stride,
                                         mb_limit,
                                         y_vert_count);
        }
        if (filter_subblocks) {
          if (x + 4u < fb->width && y_vert_count > 0) {
            GWPVP8SimpleFilterVerticalEdge(fb->y + y * fb->y_stride + x + 4u,
                                           fb->y_stride,
                                           sub_limit,
                                           y_vert_count);
          }
          if (x + 8u < fb->width && y_vert_count > 0) {
            GWPVP8SimpleFilterVerticalEdge(fb->y + y * fb->y_stride + x + 8u,
                                           fb->y_stride,
                                           sub_limit,
                                           y_vert_count);
          }
          if (x + 12u < fb->width && y_vert_count > 0) {
            GWPVP8SimpleFilterVerticalEdge(fb->y + y * fb->y_stride + x + 12u,
                                           fb->y_stride,
                                           sub_limit,
                                           y_vert_count);
          }
        }
        if (mb_y > 0u && y < fb->height && y_horiz_count > 0) {
          GWPVP8SimpleFilterHorizontalEdge(fb->y + y * fb->y_stride + x,
                                           fb->y_stride,
                                           mb_limit,
                                           y_horiz_count);
        }
        if (filter_subblocks) {
          if (y + 4u < fb->height && y_horiz_count > 0) {
            GWPVP8SimpleFilterHorizontalEdge(fb->y + (y + 4u) * fb->y_stride + x,
                                             fb->y_stride,
                                             sub_limit,
                                             y_horiz_count);
          }
          if (y + 8u < fb->height && y_horiz_count > 0) {
            GWPVP8SimpleFilterHorizontalEdge(fb->y + (y + 8u) * fb->y_stride + x,
                                             fb->y_stride,
                                             sub_limit,
                                             y_horiz_count);
          }
          if (y + 12u < fb->height && y_horiz_count > 0) {
            GWPVP8SimpleFilterHorizontalEdge(fb->y + (y + 12u) * fb->y_stride + x,
                                             fb->y_stride,
                                             sub_limit,
                                             y_horiz_count);
          }
        }
      } else {
        if (mb_x > 0u && x < fb->width && y_vert_count > 0) {
          GWPVP8NormalMBFilterVerticalEdge(fb->y + y * fb->y_stride + x,
                                           fb->y_stride,
                                           mb_limit,
                                           interior_limit,
                                           hev_threshold,
                                           y_vert_count);
          if (ux < ((fb->width + 1u) >> 1) && uv_vert_count > 0) {
            GWPVP8NormalMBFilterVerticalEdge(fb->u + uy * fb->uv_stride + ux,
                                             fb->uv_stride,
                                             mb_limit,
                                             interior_limit,
                                             hev_threshold,
                                             uv_vert_count);
            GWPVP8NormalMBFilterVerticalEdge(fb->v + uy * fb->uv_stride + ux,
                                             fb->uv_stride,
                                             mb_limit,
                                             interior_limit,
                                             hev_threshold,
                                             uv_vert_count);
          }
        }
        if (filter_subblocks) {
          if (x + 4u < fb->width && y_vert_count > 0) {
            GWPVP8NormalSubblockFilterVerticalEdge(fb->y + y * fb->y_stride + x + 4u,
                                                   fb->y_stride,
                                                   sub_limit,
                                                   interior_limit,
                                                   hev_threshold,
                                                   y_vert_count);
          }
          if (x + 8u < fb->width && y_vert_count > 0) {
            GWPVP8NormalSubblockFilterVerticalEdge(fb->y + y * fb->y_stride + x + 8u,
                                                   fb->y_stride,
                                                   sub_limit,
                                                   interior_limit,
                                                   hev_threshold,
                                                   y_vert_count);
          }
          if (x + 12u < fb->width && y_vert_count > 0) {
            GWPVP8NormalSubblockFilterVerticalEdge(fb->y + y * fb->y_stride + x + 12u,
                                                   fb->y_stride,
                                                   sub_limit,
                                                   interior_limit,
                                                   hev_threshold,
                                                   y_vert_count);
          }
          if (ux + 4u < ((fb->width + 1u) >> 1) && uv_vert_count > 0) {
            GWPVP8NormalSubblockFilterVerticalEdge(fb->u + uy * fb->uv_stride + ux + 4u,
                                                   fb->uv_stride,
                                                   sub_limit,
                                                   interior_limit,
                                                   hev_threshold,
                                                   uv_vert_count);
            GWPVP8NormalSubblockFilterVerticalEdge(fb->v + uy * fb->uv_stride + ux + 4u,
                                                   fb->uv_stride,
                                                   sub_limit,
                                                   interior_limit,
                                                   hev_threshold,
                                                   uv_vert_count);
          }
        }
        if (mb_y > 0u && y < fb->height && y_horiz_count > 0) {
          GWPVP8NormalMBFilterHorizontalEdge(fb->y + y * fb->y_stride + x,
                                             fb->y_stride,
                                             mb_limit,
                                             interior_limit,
                                             hev_threshold,
                                             y_horiz_count);
          if (uy < ((fb->height + 1u) >> 1) && uv_horiz_count > 0) {
            GWPVP8NormalMBFilterHorizontalEdge(fb->u + uy * fb->uv_stride + ux,
                                               fb->uv_stride,
                                               mb_limit,
                                               interior_limit,
                                               hev_threshold,
                                               uv_horiz_count);
            GWPVP8NormalMBFilterHorizontalEdge(fb->v + uy * fb->uv_stride + ux,
                                               fb->uv_stride,
                                               mb_limit,
                                               interior_limit,
                                               hev_threshold,
                                               uv_horiz_count);
          }
        }
        if (filter_subblocks) {
          if (y + 4u < fb->height && y_horiz_count > 0) {
            GWPVP8NormalSubblockFilterHorizontalEdge(fb->y + (y + 4u) * fb->y_stride + x,
                                                     fb->y_stride,
                                                     sub_limit,
                                                     interior_limit,
                                                     hev_threshold,
                                                     y_horiz_count);
          }
          if (y + 8u < fb->height && y_horiz_count > 0) {
            GWPVP8NormalSubblockFilterHorizontalEdge(fb->y + (y + 8u) * fb->y_stride + x,
                                                     fb->y_stride,
                                                     sub_limit,
                                                     interior_limit,
                                                     hev_threshold,
                                                     y_horiz_count);
          }
          if (y + 12u < fb->height && y_horiz_count > 0) {
            GWPVP8NormalSubblockFilterHorizontalEdge(fb->y + (y + 12u) * fb->y_stride + x,
                                                     fb->y_stride,
                                                     sub_limit,
                                                     interior_limit,
                                                     hev_threshold,
                                                     y_horiz_count);
          }
          if (uy + 4u < ((fb->height + 1u) >> 1) && uv_horiz_count > 0) {
            GWPVP8NormalSubblockFilterHorizontalEdge(fb->u + (uy + 4u) * fb->uv_stride + ux,
                                                     fb->uv_stride,
                                                     sub_limit,
                                                     interior_limit,
                                                     hev_threshold,
                                                     uv_horiz_count);
            GWPVP8NormalSubblockFilterHorizontalEdge(fb->v + (uy + 4u) * fb->uv_stride + ux,
                                                     fb->uv_stride,
                                                     sub_limit,
                                                     interior_limit,
                                                     hev_threshold,
                                                     uv_horiz_count);
          }
        }
      }
    }
  }
}

void GWPVP8FrameBufferPack(const GWPVP8FrameBuffer* fb,
                           const GWPDecoderOptions* options) {
  GWPu32 y;
  if (fb == 0 || options == 0 || options->output_buffer == 0) return;
  for (y = 0u; y < fb->height; ++y) {
    GWPu8* dst_row;
    dst_row = options->output_buffer + y * options->output_stride;
    GWPVP8YUV420RowToPacked(fb->y + y * fb->y_stride,
                            fb->u + (y >> 1) * fb->uv_stride,
                            fb->v + (y >> 1) * fb->uv_stride,
                            dst_row,
                            fb->width,
                            options->pixel_format);
  }
}
