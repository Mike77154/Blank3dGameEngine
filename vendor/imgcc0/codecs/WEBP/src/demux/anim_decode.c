#include "../webp/anim_decode.h"
#include "../utils/arena.h"
#include "../utils/common.h"
#include "../utils/endian.h"
#include "../webp/decode.h"
#include "../dec/alpha_dec.h"

static void GWPRGBAFillRect(GWPu8* rgba,
                            GWPu32 stride,
                            GWPu32 x,
                            GWPu32 y,
                            GWPu32 w,
                            GWPu32 h,
                            const GWPu8 bg[4]) {
  GWPu32 yy;
  for (yy = 0u; yy < h; ++yy) {
    GWPu8* row = rgba + (y + yy) * stride + x * 4u;
    GWPu32 xx;
    for (xx = 0u; xx < w; ++xx) {
      row[4u * xx + 0u] = bg[0];
      row[4u * xx + 1u] = bg[1];
      row[4u * xx + 2u] = bg[2];
      row[4u * xx + 3u] = bg[3];
    }
  }
}

static GWPu8 GWPMul255Div(GWPu32 a, GWPu32 b) {
  return (GWPu8)((a * b + 127u) / 255u);
}

static void GWPBlendRect(GWPu8* canvas,
                         GWPu32 canvas_stride,
                         GWPu32 x,
                         GWPu32 y,
                         GWPu32 w,
                         GWPu32 h,
                         const GWPu8* frame_rgba,
                         GWPu32 frame_stride,
                         GWPu8 blend_method) {
  GWPu32 yy;
  for (yy = 0u; yy < h; ++yy) {
    GWPu8* dst = canvas + (y + yy) * canvas_stride + x * 4u;
    const GWPu8* src = frame_rgba + yy * frame_stride;
    GWPu32 xx;
    for (xx = 0u; xx < w; ++xx) {
      const GWPu8* s = src + 4u * xx;
      GWPu8* d = dst + 4u * xx;
      if (blend_method == GWP_ANIM_NO_BLEND || s[3] == 255u) {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = s[3];
      } else if (s[3] == 0u) {
        /* keep dst as-is */
      } else {
        GWPu32 sa = s[3];
        GWPu32 da = d[3];
        GWPu32 inv_sa = 255u - sa;
        GWPu32 out_a = sa + ((da * inv_sa + 127u) / 255u);
        if (out_a == 0u) {
          d[0] = d[1] = d[2] = d[3] = 0u;
        } else {
          GWPu32 premul_r = s[0] * sa + ((d[0] * da * inv_sa + 127u) / 255u);
          GWPu32 premul_g = s[1] * sa + ((d[1] * da * inv_sa + 127u) / 255u);
          GWPu32 premul_b = s[2] * sa + ((d[2] * da * inv_sa + 127u) / 255u);
          d[0] = (GWPu8)((premul_r + out_a / 2u) / out_a);
          d[1] = (GWPu8)((premul_g + out_a / 2u) / out_a);
          d[2] = (GWPu8)((premul_b + out_a / 2u) / out_a);
          d[3] = (GWPu8)out_a;
        }
      }
    }
  }
}

static void GWPConvertFromRGBA(const GWPu8* src,
                               GWPu32 width,
                               GWPu32 height,
                               GWPu32 src_stride,
                               GWPPixelFormat fmt,
                               GWPu8* dst,
                               GWPu32 dst_stride) {
  GWPu32 y;
  for (y = 0u; y < height; ++y) {
    const GWPu8* s = src + y * src_stride;
    GWPu8* d = dst + y * dst_stride;
    GWPu32 x;
    for (x = 0u; x < width; ++x) {
      GWPu8 r = s[4u * x + 0u];
      GWPu8 g = s[4u * x + 1u];
      GWPu8 b = s[4u * x + 2u];
      GWPu8 a = s[4u * x + 3u];
      if (fmt == GWP_PIXFMT_BGRA) {
        d[4u * x + 0u] = b;
        d[4u * x + 1u] = g;
        d[4u * x + 2u] = r;
        d[4u * x + 3u] = a;
      } else if (fmt == GWP_PIXFMT_ARGB) {
        d[4u * x + 0u] = a;
        d[4u * x + 1u] = r;
        d[4u * x + 2u] = g;
        d[4u * x + 3u] = b;
      } else {
        d[4u * x + 0u] = r;
        d[4u * x + 1u] = g;
        d[4u * x + 2u] = b;
        d[4u * x + 3u] = a;
      }
    }
  }
}

static GWPStatusCode GWPBuildStillFromFrameBitstream(const GWPFrameInfo* frame,
                                                     GWPu8* dst,
                                                     GWPu32 dst_size,
                                                     GWPu32* out_size) {
  GWPu32 chunk_total;
  GWPu32 riff_size;
  if (frame == 0 || dst == 0 || out_size == 0) return GWP_STATUS_INVALID_PARAM;
  if (!GWPAddU32(8u, GWP_ALIGN2(frame->bitstream_payload.size), &chunk_total)) {
    return GWP_STATUS_LIMIT_EXCEEDED;
  }
  if (!GWPAddU32(4u, chunk_total, &riff_size)) return GWP_STATUS_LIMIT_EXCEEDED;
  if (dst_size < riff_size + 8u) return GWP_STATUS_NOT_ENOUGH_SCRATCH;
  GWPWriteLE32(dst + 0, GWP_FOURCC_RIFF);
  GWPWriteLE32(dst + 4, riff_size);
  GWPWriteLE32(dst + 8, GWP_FOURCC_WEBP);
  GWPWriteLE32(dst + 12, frame->bitstream_fourcc);
  GWPWriteLE32(dst + 16, frame->bitstream_payload.size);
  GWPCopy(dst + 20, frame->bitstream_payload.bytes, frame->bitstream_payload.size);
  if (frame->bitstream_payload.size & 1u) dst[20u + frame->bitstream_payload.size] = 0u;
  *out_size = riff_size + 8u;
  return GWP_STATUS_OK;
}

static void GWPAnimSetBackgroundColor(GWPAnimDecoder* dec) {
  GWPu32 bg = dec->demux.background_color;
  dec->bg_rgba[0] = (GWPu8)((bg >> 16) & 0xffu);
  dec->bg_rgba[1] = (GWPu8)((bg >> 8) & 0xffu);
  dec->bg_rgba[2] = (GWPu8)(bg & 0xffu);
  dec->bg_rgba[3] = (GWPu8)((bg >> 24) & 0xffu);
}

void GWPAnimDecoderOptionsInit(GWPAnimDecoderOptions* options) {
  if (options == 0) return;
  options->pixel_format = GWP_PIXFMT_RGBA;
  options->strict = GWP_TRUE;
  options->max_width = GWP_MAX_IMAGE_WIDTH;
  options->max_height = GWP_MAX_IMAGE_HEIGHT;
  options->scratch = 0;
  options->scratch_size = 0u;
}

void GWPAnimDecoderReset(GWPAnimDecoder* dec) {
  GWPu32 canvas_bytes;
  if (dec == 0 || !dec->initialized) return;
  canvas_bytes = dec->demux.features.height * dec->canvas_stride;
  GWPZero(dec->canvas_rgba, canvas_bytes);
  GWPRGBAFillRect(dec->canvas_rgba,
                  dec->canvas_stride,
                  0u,
                  0u,
                  dec->demux.features.width,
                  dec->demux.features.height,
                  dec->bg_rgba);
  dec->frame_index = 0u;
  dec->next_timestamp_ms = 0u;
  dec->prev_x = dec->prev_y = dec->prev_w = dec->prev_h = 0u;
  dec->prev_dispose_method = GWP_ANIM_DISPOSE_BACKGROUND;
}

GWPStatusCode GWPAnimDecoderInit(GWPAnimDecoder* dec,
                                 const GWPu8* data,
                                 GWPu32 data_size,
                                 const GWPAnimDecoderOptions* options) {
  GWPStatusCode st;
  GWPArena arena;
  GWPu32 row_bytes;
  GWPu32 canvas_bytes;
  if (dec == 0 || data == 0 || options == 0 || options->scratch == 0) {
    return GWP_STATUS_INVALID_PARAM;
  }
  GWPZero(dec, (GWPu32)sizeof(*dec));
  dec->options = *options;
  st = GWPDemuxParse(&dec->demux, data, data_size);
  if (st != GWP_STATUS_OK) return st;
  if (!dec->demux.features.has_animation || dec->demux.features.frame_count == 0u) {
    return GWP_STATUS_UNSUPPORTED_FORMAT;
  }
  if (options->strict) {
    if (dec->demux.features.width > options->max_width) return GWP_STATUS_LIMIT_EXCEEDED;
    if (dec->demux.features.height > options->max_height) return GWP_STATUS_LIMIT_EXCEEDED;
  }
  if (!GWPMulU32(dec->demux.features.width, 4u, &row_bytes)) return GWP_STATUS_LIMIT_EXCEEDED;
  if (!GWPMulU32(row_bytes, dec->demux.features.height, &canvas_bytes)) return GWP_STATUS_LIMIT_EXCEEDED;
  GWPArenaInit(&arena, options->scratch, options->scratch_size);
  dec->canvas_rgba = (GWPu8*)GWPArenaAlloc(&arena, canvas_bytes, 8u);
  dec->frame_rgba = (GWPu8*)GWPArenaAlloc(&arena, canvas_bytes, 8u);
  if (dec->canvas_rgba == 0 || dec->frame_rgba == 0) return GWP_STATUS_NOT_ENOUGH_SCRATCH;
  dec->nested_scratch = (GWPu8*)options->scratch + GWPArenaUsed(&arena);
  dec->nested_scratch_size = options->scratch_size - GWPArenaUsed(&arena);
  if (dec->nested_scratch_size < 1024u) return GWP_STATUS_NOT_ENOUGH_SCRATCH;
  dec->canvas_stride = row_bytes;
  GWPAnimSetBackgroundColor(dec);
  dec->initialized = GWP_TRUE;
  GWPAnimDecoderReset(dec);
  return GWP_STATUS_OK;
}

GWPStatusCode GWPAnimDecoderGetInfo(const GWPAnimDecoder* dec, GWPAnimInfo* info) {
  if (dec == 0 || info == 0 || !dec->initialized) return GWP_STATUS_INVALID_PARAM;
  info->canvas_width = dec->demux.features.width;
  info->canvas_height = dec->demux.features.height;
  info->frame_count = dec->demux.features.frame_count;
  info->loop_count = dec->demux.loop_count;
  info->background_color = dec->demux.background_color;
  info->has_alpha = dec->demux.features.has_alpha;
  info->has_icc = dec->demux.features.has_icc;
  info->has_exif = dec->demux.features.has_exif;
  info->has_xmp = dec->demux.features.has_xmp;
  return GWP_STATUS_OK;
}

GWPBool GWPAnimDecoderHasMoreFrames(const GWPAnimDecoder* dec) {
  if (dec == 0 || !dec->initialized) return GWP_FALSE;
  return (dec->frame_index < dec->demux.frame_count) ? GWP_TRUE : GWP_FALSE;
}

static GWPStatusCode GWPDecodeFrameToRGBA(GWPAnimDecoder* dec, const GWPFrameInfo* frame) {
  GWPu32 still_size;
  GWPDecoderOptions opt;
  GWPBitstreamFeatures out_features;
  GWPStatusCode st;
  GWPu32 frame_stride;
  GWPu32 frame_bytes;
  GWPu8* still_data;
  GWPu8* decode_scratch;
  GWPu32 decode_scratch_size;
  if (dec == 0 || frame == 0) return GWP_STATUS_INVALID_PARAM;
  still_data = dec->nested_scratch;
  st = GWPBuildStillFromFrameBitstream(frame, still_data, dec->nested_scratch_size, &still_size);
  if (st != GWP_STATUS_OK) return st;
  decode_scratch = dec->nested_scratch + still_size;
  decode_scratch_size = dec->nested_scratch_size - still_size;
  if (!GWPMulU32(frame->width, 4u, &frame_stride)) return GWP_STATUS_LIMIT_EXCEEDED;
  if (!GWPMulU32(frame_stride, frame->height, &frame_bytes)) return GWP_STATUS_LIMIT_EXCEEDED;
  opt.pixel_format = GWP_PIXFMT_RGBA;
  opt.strict = dec->options.strict;
  opt.max_width = dec->options.max_width;
  opt.max_height = dec->options.max_height;
  opt.output_buffer = dec->frame_rgba;
  opt.output_buffer_size = frame_bytes;
  opt.output_stride = frame_stride;
  opt.scratch = decode_scratch;
  opt.scratch_size = decode_scratch_size;
  st = GWPDecode(still_data, still_size, &opt, &out_features);
  if (st != GWP_STATUS_OK) return st;
  if (frame->alph_payload.bytes != 0) {
    st = GWPDecodeAlphaChunk(frame->alph_payload.bytes,
                             frame->alph_payload.size,
                             frame->width,
                             frame->height,
                             GWP_PIXFMT_RGBA,
                             dec->frame_rgba,
                             frame_stride,
                             decode_scratch,
                             decode_scratch_size);
    if (st != GWP_STATUS_OK) return st;
  }
  return GWP_STATUS_OK;
}

GWPStatusCode GWPAnimDecoderGetNext(GWPAnimDecoder* dec,
                                    GWPu8* out_pixels,
                                    GWPu32 out_pixels_size,
                                    GWPu32 out_stride,
                                    GWPu32* out_timestamp_ms) {
  const GWPFrameInfo* frame;
  GWPu32 canvas_bytes;
  GWPStatusCode st;
  if (dec == 0 || !dec->initialized || out_pixels == 0) return GWP_STATUS_INVALID_PARAM;
  if (!GWPAnimDecoderHasMoreFrames(dec)) return GWP_STATUS_PARSE_ERROR;
  if (!GWPMulU32(dec->canvas_stride, dec->demux.features.height, &canvas_bytes)) {
    return GWP_STATUS_LIMIT_EXCEEDED;
  }
  if (out_stride < dec->demux.features.width * 4u) return GWP_STATUS_INVALID_PARAM;
  if (out_pixels_size < out_stride * dec->demux.features.height) return GWP_STATUS_NOT_ENOUGH_OUTPUT;

  if (dec->frame_index != 0u && dec->prev_dispose_method == GWP_ANIM_DISPOSE_BACKGROUND) {
    GWPRGBAFillRect(dec->canvas_rgba,
                    dec->canvas_stride,
                    dec->prev_x,
                    dec->prev_y,
                    dec->prev_w,
                    dec->prev_h,
                    dec->bg_rgba);
  }

  frame = &dec->demux.frames[dec->frame_index];
  st = GWPDecodeFrameToRGBA(dec, frame);
  if (st != GWP_STATUS_OK) return st;
  GWPBlendRect(dec->canvas_rgba,
               dec->canvas_stride,
               frame->x,
               frame->y,
               frame->width,
               frame->height,
               dec->frame_rgba,
               frame->width * 4u,
               frame->blend_method);

  GWPConvertFromRGBA(dec->canvas_rgba,
                     dec->demux.features.width,
                     dec->demux.features.height,
                     dec->canvas_stride,
                     dec->options.pixel_format,
                     out_pixels,
                     out_stride);

  if (out_timestamp_ms != 0) *out_timestamp_ms = dec->next_timestamp_ms;
  dec->next_timestamp_ms += frame->duration_ms;
  dec->prev_x = frame->x;
  dec->prev_y = frame->y;
  dec->prev_w = frame->width;
  dec->prev_h = frame->height;
  dec->prev_dispose_method = frame->dispose_method;
  ++dec->frame_index;
  (void)canvas_bytes;
  return GWP_STATUS_OK;
}
