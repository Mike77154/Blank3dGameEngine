#include "alpha_dec.h"

#include "vp8l_dec.h"
#include "../utils/common.h"
#include "../utils/fixed.h"

static GWPu8 GWPAlphaGetPacked(const GWPu8* row, GWPu32 x, GWPPixelFormat fmt) {
  const GWPu8* px;
  px = row + x * 4u;
  if (fmt == GWP_PIXFMT_ARGB) return px[0];
  return px[3];
}

static void GWPAlphaSetPacked(GWPu8* row, GWPu32 x, GWPPixelFormat fmt, GWPu8 alpha) {
  GWPu8* px;
  px = row + x * 4u;
  if (fmt == GWP_PIXFMT_ARGB) px[0] = alpha;
  else px[3] = alpha;
}

static GWPu8 GWPAlphaGradientPredictor(GWPu8 a, GWPu8 b, GWPu8 c) {
  return GWPClamp8((int)a + (int)b - (int)c);
}

static GWPStatusCode GWPAlphaApplyFiltered(const GWPu8* deltas,
                                           GWPu32 delta_size,
                                           GWPu32 width,
                                           GWPu32 height,
                                           GWPu8 filter,
                                           GWPPixelFormat pixel_format,
                                           GWPu8* output_buffer,
                                           GWPu32 output_stride) {
  GWPu32 x;
  GWPu32 y;
  GWPu32 needed;
  GWPu32 pos;
  if (deltas == 0 || output_buffer == 0) return GWP_STATUS_INVALID_PARAM;
  if (!GWPMulU32(width, height, &needed)) return GWP_STATUS_LIMIT_EXCEEDED;
  if (delta_size < needed) return GWP_STATUS_TRUNCATED_DATA;
  pos = 0u;
  for (y = 0u; y < height; ++y) {
    GWPu8* row;
    const GWPu8* prev_row;
    row = output_buffer + y * output_stride;
    prev_row = (y > 0u) ? (output_buffer + (y - 1u) * output_stride) : 0;
    for (x = 0u; x < width; ++x) {
      GWPu8 delta;
      GWPu8 left;
      GWPu8 top;
      GWPu8 top_left;
      GWPu8 predictor;
      GWPu8 alpha;
      delta = deltas[pos++];
      left = (x > 0u) ? GWPAlphaGetPacked(row, x - 1u, pixel_format) : 0u;
      top = (prev_row != 0) ? GWPAlphaGetPacked(prev_row, x, pixel_format) : 0u;
      top_left = (prev_row != 0 && x > 0u) ? GWPAlphaGetPacked(prev_row, x - 1u, pixel_format) : 0u;
      predictor = 0u;
      if (filter == 1u) {
        predictor = (x == 0u) ? top : left;
      } else if (filter == 2u) {
        predictor = (y == 0u) ? left : top;
      } else if (filter == 3u) {
        if (x == 0u && y == 0u) predictor = 0u;
        else if (x == 0u) predictor = top;
        else if (y == 0u) predictor = left;
        else predictor = GWPAlphaGradientPredictor(left, top, top_left);
      }
      alpha = (GWPu8)((predictor + delta) & 0xffu);
      GWPAlphaSetPacked(row, x, pixel_format, alpha);
    }
  }
  return GWP_STATUS_OK;
}

GWPStatusCode GWPDecodeAlphaChunk(const GWPu8* data,
                                  GWPu32 data_size,
                                  GWPu32 width,
                                  GWPu32 height,
                                  GWPPixelFormat pixel_format,
                                  GWPu8* output_buffer,
                                  GWPu32 output_stride,
                                  void* scratch,
                                  GWPu32 scratch_size) {
  GWPu8 header;
  GWPu8 preprocessing;
  GWPu8 filter;
  GWPu8 method;
  const GWPu8* body;
  GWPu32 body_size;
  if (data == 0 || output_buffer == 0 || scratch == 0) return GWP_STATUS_INVALID_PARAM;
  if (data_size < 1u) return GWP_STATUS_TRUNCATED_DATA;
  header = data[0];
  preprocessing = (GWPu8)((header >> 4) & 0x03u);
  filter = (GWPu8)((header >> 2) & 0x03u);
  method = (GWPu8)(header & 0x03u);
  body = data + 1u;
  body_size = data_size - 1u;
  (void)preprocessing;
  if (filter > 3u) return GWP_STATUS_BITSTREAM_ERROR;
  if (method > 1u) return GWP_STATUS_UNSUPPORTED_FEATURE;

  if (method == 0u) {
    return GWPAlphaApplyFiltered(body, body_size, width, height, filter,
                                 pixel_format, output_buffer, output_stride);
  } else {
    GWPu32 plane_size;
    GWPu8* alpha_plane;
    GWPu8* scratch_bytes;
    if (!GWPMulU32(width, height, &plane_size)) return GWP_STATUS_LIMIT_EXCEEDED;
    if (scratch_size < plane_size) return GWP_STATUS_NOT_ENOUGH_SCRATCH;
    scratch_bytes = (GWPu8*)scratch;
    alpha_plane = scratch_bytes;
    if (scratch_size - plane_size == 0u) return GWP_STATUS_NOT_ENOUGH_SCRATCH;
    {
      GWPStatusCode st;
      st = GWPDecodeVP8LAlphaImage(body,
                                   body_size,
                                   width,
                                   height,
                                   scratch_bytes + plane_size,
                                   scratch_size - plane_size,
                                   alpha_plane);
      if (st != GWP_STATUS_OK) return st;
    }
    return GWPAlphaApplyFiltered(alpha_plane,
                                 plane_size,
                                 width,
                                 height,
                                 filter,
                                 pixel_format,
                                 output_buffer,
                                 output_stride);
  }
}
