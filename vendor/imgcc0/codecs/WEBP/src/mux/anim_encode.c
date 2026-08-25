#include "../webp/anim_encode.h"
#include "../utils/common.h"
#include "../utils/endian.h"

static GWPu32 GWPChunkDiskSizeAnim(GWPu32 payload_size) {
  return 8u + GWP_ALIGN2(payload_size);
}

static GWPStatusCode GWPProbeVP8Anim(const GWPu8* data,
                                     GWPu32 size,
                                     GWPu32* width,
                                     GWPu32* height) {
  if (data == 0 || width == 0 || height == 0) return GWP_STATUS_INVALID_PARAM;
  if (size < 10u) return GWP_STATUS_TRUNCATED_DATA;
  if (GWPReadLE24(data + 3) != 0x2a019du) return GWP_STATUS_BITSTREAM_ERROR;
  *width = (GWPReadLE16(data + 6) & 0x3fffu);
  *height = (GWPReadLE16(data + 8) & 0x3fffu);
  if (*width == 0u || *height == 0u) return GWP_STATUS_BAD_DIMENSIONS;
  return GWP_STATUS_OK;
}

static GWPStatusCode GWPProbeVP8LAnim(const GWPu8* data,
                                      GWPu32 size,
                                      GWPu32* width,
                                      GWPu32* height,
                                      GWPBool* has_alpha) {
  GWPu32 bits;
  if (data == 0 || width == 0 || height == 0 || has_alpha == 0) {
    return GWP_STATUS_INVALID_PARAM;
  }
  if (size < 5u) return GWP_STATUS_TRUNCATED_DATA;
  if (data[0] != 0x2fu) return GWP_STATUS_BITSTREAM_ERROR;
  bits = GWPReadLE32(data + 1);
  *width = (bits & 0x3fffu) + 1u;
  *height = ((bits >> 14) & 0x3fffu) + 1u;
  *has_alpha = ((bits >> 28) & 1u) ? GWP_TRUE : GWP_FALSE;
  if (((bits >> 29) & 7u) != 0u) return GWP_STATUS_BITSTREAM_ERROR;
  if (*width == 0u || *height == 0u) return GWP_STATUS_BAD_DIMENSIONS;
  return GWP_STATUS_OK;
}

static GWPMuxError GWPWriteChunkAnim(GWPu8** dst,
                                     GWPu32* remain,
                                     GWPu32 fourcc,
                                     const GWPData* data) {
  GWPu32 disk_size;
  GWPu8* p;
  if (dst == 0 || *dst == 0 || remain == 0 || data == 0 || data->bytes == 0) {
    return GWP_MUX_INVALID_ARGUMENT;
  }
  disk_size = GWPChunkDiskSizeAnim(data->size);
  if (*remain < disk_size) return GWP_MUX_NOT_ENOUGH_OUTPUT;
  p = *dst;
  GWPWriteLE32(p + 0, fourcc);
  GWPWriteLE32(p + 4, data->size);
  GWPCopy(p + 8, data->bytes, data->size);
  if ((data->size & 1u) != 0u) p[8u + data->size] = 0u;
  *dst += disk_size;
  *remain -= disk_size;
  return GWP_MUX_OK;
}

static void GWPReadRawPixel(const GWPu8* p,
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

static void GWPCopyFrameToRGBA(const GWPAnimEncoder* enc,
                               const GWPu8* pixels,
                               GWPu32 stride,
                               GWPRawPixelFormat fmt,
                               GWPu8* dst_rgba) {
  GWPu32 x, y;
  for (y = 0u; y < enc->canvas_height; ++y) {
    const GWPu8* src_row;
    GWPu8* dst_row;
    src_row = pixels + y * stride;
    dst_row = dst_rgba + y * enc->canvas_width * 4u;
    for (x = 0u; x < enc->canvas_width; ++x) {
      GWPu8 r, g, b, a;
      GWPReadRawPixel(src_row + 4u * x, fmt, &r, &g, &b, &a);
      dst_row[4u * x + 0u] = r;
      dst_row[4u * x + 1u] = g;
      dst_row[4u * x + 2u] = b;
      dst_row[4u * x + 3u] = a;
    }
  }
}

static GWPBool GWPAnimEncoderFindDiffRect(const GWPAnimEncoder* enc,
                                          const GWPu8* pixels,
                                          GWPu32 stride,
                                          GWPRawPixelFormat fmt,
                                          GWPu32* out_left,
                                          GWPu32* out_top,
                                          GWPu32* out_right,
                                          GWPu32* out_bottom) {
  GWPu32 x, y;
  GWPBool found;
  if (enc == 0 || pixels == 0 || out_left == 0 || out_top == 0 ||
      out_right == 0 || out_bottom == 0 || enc->canvas_rgba == 0) {
    return GWP_FALSE;
  }
  found = GWP_FALSE;
  *out_left = enc->canvas_width;
  *out_top = enc->canvas_height;
  *out_right = 0u;
  *out_bottom = 0u;
  for (y = 0u; y < enc->canvas_height; ++y) {
    const GWPu8* src_row;
    const GWPu8* prev_row;
    src_row = pixels + y * stride;
    prev_row = enc->canvas_rgba + y * enc->canvas_width * 4u;
    for (x = 0u; x < enc->canvas_width; ++x) {
      GWPu8 r, g, b, a;
      GWPReadRawPixel(src_row + 4u * x, fmt, &r, &g, &b, &a);
      if (prev_row[4u * x + 0u] != r || prev_row[4u * x + 1u] != g ||
          prev_row[4u * x + 2u] != b || prev_row[4u * x + 3u] != a) {
        if (!found) {
          *out_left = x;
          *out_top = y;
          *out_right = x + 1u;
          *out_bottom = y + 1u;
          found = GWP_TRUE;
        } else {
          if (x < *out_left) *out_left = x;
          if (y < *out_top) *out_top = y;
          if (x + 1u > *out_right) *out_right = x + 1u;
          if (y + 1u > *out_bottom) *out_bottom = y + 1u;
        }
      }
    }
  }
  return found;
}

void GWPAnimEncoderOptionsInit(GWPAnimEncoderOptions* options) {
  if (options == 0) return;
  options->force_vp8x = GWP_TRUE;
  options->snap_odd_offsets = GWP_TRUE;
  options->minimize_size = GWP_TRUE;
  options->allow_mixed = GWP_FALSE;
  options->kmin = 0u;
  options->kmax = 0u;
  options->work_mem = 0;
  options->work_mem_size = 0u;
}

void GWPAnimFrameSpecInit(GWPAnimFrameSpec* spec) {
  if (spec == 0) return;
  spec->x_offset = 0u;
  spec->y_offset = 0u;
  spec->duration_ms = 100u;
  spec->blend_method = (GWPu8)GWP_ANIM_BLEND;
  spec->dispose_method = (GWPu8)GWP_ANIM_DISPOSE_NONE;
}

void GWPAnimEncoderInit(GWPAnimEncoder* enc,
                        GWPu32 canvas_width,
                        GWPu32 canvas_height,
                        const GWPAnimEncoderOptions* options) {
  GWPu32 canvas_bytes;
  if (enc == 0) return;
  GWPZero(enc, (GWPu32)sizeof(*enc));
  enc->canvas_width = canvas_width;
  enc->canvas_height = canvas_height;
  if (options != 0) {
    enc->options = *options;
  } else {
    GWPAnimEncoderOptionsInit(&enc->options);
  }
  enc->anim_params.bgcolor = 0u;
  enc->anim_params.loop_count = 0u;
  canvas_bytes = 0u;
  if (canvas_width != 0u && canvas_height != 0u &&
      GWPMulU32(canvas_width * 4u, canvas_height, &canvas_bytes) &&
      enc->options.work_mem != 0 && enc->options.work_mem_size >= canvas_bytes) {
    enc->canvas_rgba = enc->options.work_mem;
    enc->canvas_rgba_size = canvas_bytes;
    enc->storage_bytes = enc->options.work_mem + canvas_bytes;
    enc->storage_size = enc->options.work_mem_size - canvas_bytes;
    enc->storage_used = 0u;
  }
}

GWPMuxError GWPAnimEncoderSetAnimationParams(GWPAnimEncoder* enc,
                                             const GWPMuxAnimParams* params) {
  if (enc == 0 || params == 0) return GWP_MUX_INVALID_ARGUMENT;
  enc->anim_params = *params;
  return GWP_MUX_OK;
}

GWPMuxError GWPAnimEncoderSetChunk(GWPAnimEncoder* enc,
                                   const char fourcc[4],
                                   const GWPData* data) {
  GWPu32 tag;
  if (enc == 0 || fourcc == 0 || data == 0 || data->bytes == 0) return GWP_MUX_INVALID_ARGUMENT;
  tag = GWP_FOURCC(fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
  switch (tag) {
    case GWP_FOURCC_ICCP:
      enc->iccp = *data;
      break;
    case GWP_FOURCC_EXIF:
      enc->exif = *data;
      break;
    case GWP_FOURCC_XMP:
      enc->xmp = *data;
      break;
    default:
      return GWP_MUX_UNSUPPORTED;
  }
  return GWP_MUX_OK;
}

static GWPMuxError GWPAnimEncoderValidateFrameRect(const GWPAnimEncoder* enc,
                                                   GWPu32 x_offset,
                                                   GWPu32 y_offset,
                                                   GWPu32 width,
                                                   GWPu32 height) {
  if (enc == 0) return GWP_MUX_INVALID_ARGUMENT;
  if (width == 0u || height == 0u) return GWP_MUX_INVALID_ARGUMENT;
  if (enc->canvas_width == 0u || enc->canvas_height == 0u) return GWP_MUX_INVALID_ARGUMENT;
  if (x_offset > enc->canvas_width || y_offset > enc->canvas_height) return GWP_MUX_INVALID_ARGUMENT;
  if (width > enc->canvas_width - x_offset) return GWP_MUX_INVALID_ARGUMENT;
  if (height > enc->canvas_height - y_offset) return GWP_MUX_INVALID_ARGUMENT;
  return GWP_MUX_OK;
}

static void GWPAnimEncoderApplyOffsetPolicy(const GWPAnimEncoder* enc,
                                            GWPu32* x_offset,
                                            GWPu32* y_offset) {
  if (enc == 0 || x_offset == 0 || y_offset == 0) return;
  if (enc->options.snap_odd_offsets) {
    *x_offset &= ~1u;
    *y_offset &= ~1u;
  }
}

static GWPMuxError GWPAnimEncoderAddPreparedFrame(GWPAnimEncoder* enc,
                                                  GWPu32 bitstream_fourcc,
                                                  GWPBitstreamKind bitstream_kind,
                                                  const GWPData* bitstream_payload,
                                                  GWPu32 width,
                                                  GWPu32 height,
                                                  GWPBool has_alpha,
                                                  const GWPData* alph_payload,
                                                  const GWPAnimFrameSpec* spec_in) {
  GWPAnimFrameSpec spec;
  GWPMuxError st;
  GWPAnimEncoderFrame* dst;
  if (enc == 0 || bitstream_payload == 0 || bitstream_payload->bytes == 0 || spec_in == 0) {
    return GWP_MUX_INVALID_ARGUMENT;
  }
  if (enc->frame_count >= GWP_ANIM_MAX_FRAMES) return GWP_MUX_NOT_ENOUGH_DATA;
  spec = *spec_in;
  GWPAnimEncoderApplyOffsetPolicy(enc, &spec.x_offset, &spec.y_offset);
  if ((spec.x_offset & 1u) != 0u || (spec.y_offset & 1u) != 0u) {
    return GWP_MUX_INVALID_ARGUMENT;
  }
  st = GWPAnimEncoderValidateFrameRect(enc, spec.x_offset, spec.y_offset, width, height);
  if (st != GWP_MUX_OK) return st;
  dst = &enc->frames[enc->frame_count++];
  dst->x_offset = spec.x_offset;
  dst->y_offset = spec.y_offset;
  dst->width = width;
  dst->height = height;
  dst->duration_ms = spec.duration_ms;
  dst->blend_method = spec.blend_method;
  dst->dispose_method = spec.dispose_method;
  dst->bitstream_fourcc = bitstream_fourcc;
  dst->bitstream_kind = bitstream_kind;
  dst->has_alpha = has_alpha;
  dst->bitstream_payload = *bitstream_payload;
  dst->is_key_frame = (spec.x_offset == 0u && spec.y_offset == 0u &&
                       width == enc->canvas_width && height == enc->canvas_height) ?
                      GWP_TRUE : GWP_FALSE;
  if (alph_payload != 0 && alph_payload->bytes != 0) {
    dst->alph_payload = *alph_payload;
    dst->has_alpha = GWP_TRUE;
  }
  if (dst->has_alpha) enc->has_alpha = GWP_TRUE;
  return GWP_MUX_OK;
}

GWPMuxError GWPAnimEncoderAddFrameWebP(GWPAnimEncoder* enc,
                                       const GWPData* still_webp,
                                       const GWPAnimFrameSpec* spec) {
  GWPDemuxer dmux;
  GWPStatusCode st;
  if (enc == 0 || still_webp == 0 || still_webp->bytes == 0 || spec == 0) {
    return GWP_MUX_INVALID_ARGUMENT;
  }
  st = GWPDemuxParse(&dmux, still_webp->bytes, still_webp->size);
  if (st != GWP_STATUS_OK) return GWP_MUX_BAD_DATA;
  if (dmux.features.has_animation) return GWP_MUX_UNSUPPORTED;
  if (dmux.features.format == GWP_BITSTREAM_VP8) {
    return GWPAnimEncoderAddPreparedFrame(enc,
                                          GWP_FOURCC_VP8,
                                          GWP_BITSTREAM_VP8,
                                          &dmux.vp8_payload,
                                          dmux.features.width,
                                          dmux.features.height,
                                          dmux.features.has_alpha,
                                          &dmux.alph_payload,
                                          spec);
  } else if (dmux.features.format == GWP_BITSTREAM_VP8L) {
    GWPData no_alpha;
    no_alpha.bytes = 0;
    no_alpha.size = 0u;
    return GWPAnimEncoderAddPreparedFrame(enc,
                                          GWP_FOURCC_VP8L,
                                          GWP_BITSTREAM_VP8L,
                                          &dmux.vp8l_payload,
                                          dmux.features.width,
                                          dmux.features.height,
                                          dmux.features.has_alpha,
                                          &no_alpha,
                                          spec);
  }
  return GWP_MUX_BAD_DATA;
}

GWPMuxError GWPAnimEncoderAddFrameBitstream(GWPAnimEncoder* enc,
                                            GWPBitstreamKind kind,
                                            const GWPData* bitstream,
                                            GWPu32 width,
                                            GWPu32 height,
                                            GWPBool has_alpha,
                                            const GWPData* alph_payload,
                                            const GWPAnimFrameSpec* spec) {
  GWPu32 bit_w, bit_h;
  GWPBool bit_alpha;
  GWPStatusCode st;
  if (enc == 0 || bitstream == 0 || bitstream->bytes == 0 || spec == 0) {
    return GWP_MUX_INVALID_ARGUMENT;
  }
  bit_w = 0u;
  bit_h = 0u;
  bit_alpha = has_alpha;
  if (kind == GWP_BITSTREAM_VP8) {
    st = GWPProbeVP8Anim(bitstream->bytes, bitstream->size, &bit_w, &bit_h);
    if (st != GWP_STATUS_OK) return GWP_MUX_BAD_DATA;
    if (bit_w != width || bit_h != height) return GWP_MUX_BAD_DATA;
    return GWPAnimEncoderAddPreparedFrame(enc,
                                          GWP_FOURCC_VP8,
                                          kind,
                                          bitstream,
                                          width,
                                          height,
                                          has_alpha,
                                          alph_payload,
                                          spec);
  } else if (kind == GWP_BITSTREAM_VP8L) {
    st = GWPProbeVP8LAnim(bitstream->bytes, bitstream->size, &bit_w, &bit_h, &bit_alpha);
    if (st != GWP_STATUS_OK) return GWP_MUX_BAD_DATA;
    if (bit_w != width || bit_h != height) return GWP_MUX_BAD_DATA;
    return GWPAnimEncoderAddPreparedFrame(enc,
                                          GWP_FOURCC_VP8L,
                                          kind,
                                          bitstream,
                                          width,
                                          height,
                                          bit_alpha,
                                          0,
                                          spec);
  }
  return GWP_MUX_UNSUPPORTED;
}


static GWPu8* GWPAnimAllocStorage(GWPAnimEncoder* enc, GWPu32 size, GWPu32* mark) {
  GWPu8* dst;
  if (enc == 0) return 0;
  if (mark != 0) *mark = enc->storage_used;
  if (size > enc->storage_size - enc->storage_used) return 0;
  dst = enc->storage_bytes + enc->storage_used;
  enc->storage_used += size;
  return dst;
}

static void GWPAnimRewindStorage(GWPAnimEncoder* enc, GWPu32 mark) {
  if (enc == 0) return;
  if (mark <= enc->storage_size) enc->storage_used = mark;
}

static GWPMuxError GWPAnimCommitStillWebPToStorage(GWPAnimEncoder* enc,
                                                   const GWPu8* still_bytes,
                                                   GWPu32 still_size,
                                                   const GWPAnimFrameSpec* spec) {
  GWPDemuxer dmux;
  GWPStatusCode st;
  GWPu32 mark;
  GWPu8* bit_dst;
  GWPData bit_data;
  GWPData alph_data;
  if (enc == 0 || still_bytes == 0 || still_size == 0u || spec == 0) {
    return GWP_MUX_INVALID_ARGUMENT;
  }
  st = GWPDemuxParse(&dmux, still_bytes, still_size);
  if (st != GWP_STATUS_OK) return GWP_MUX_BAD_DATA;
  if (dmux.features.has_animation) return GWP_MUX_UNSUPPORTED;
  alph_data.bytes = 0;
  alph_data.size = 0u;
  mark = enc->storage_used;
  if (dmux.features.format == GWP_BITSTREAM_VP8) {
    bit_dst = GWPAnimAllocStorage(enc, dmux.vp8_payload.size + dmux.alph_payload.size, 0);
    if (bit_dst == 0) return GWP_MUX_NOT_ENOUGH_OUTPUT;
    GWPCopy(bit_dst, dmux.vp8_payload.bytes, dmux.vp8_payload.size);
    bit_data.bytes = bit_dst;
    bit_data.size = dmux.vp8_payload.size;
    if (dmux.alph_payload.bytes != 0) {
      GWPCopy(bit_dst + dmux.vp8_payload.size, dmux.alph_payload.bytes, dmux.alph_payload.size);
      alph_data.bytes = bit_dst + dmux.vp8_payload.size;
      alph_data.size = dmux.alph_payload.size;
    }
    return GWPAnimEncoderAddPreparedFrame(enc,
                                          GWP_FOURCC_VP8,
                                          GWP_BITSTREAM_VP8,
                                          &bit_data,
                                          dmux.features.width,
                                          dmux.features.height,
                                          dmux.features.has_alpha,
                                          &alph_data,
                                          spec);
  } else if (dmux.features.format == GWP_BITSTREAM_VP8L) {
    bit_dst = GWPAnimAllocStorage(enc, dmux.vp8l_payload.size, 0);
    if (bit_dst == 0) return GWP_MUX_NOT_ENOUGH_OUTPUT;
    GWPCopy(bit_dst, dmux.vp8l_payload.bytes, dmux.vp8l_payload.size);
    bit_data.bytes = bit_dst;
    bit_data.size = dmux.vp8l_payload.size;
    return GWPAnimEncoderAddPreparedFrame(enc,
                                          GWP_FOURCC_VP8L,
                                          GWP_BITSTREAM_VP8L,
                                          &bit_data,
                                          dmux.features.width,
                                          dmux.features.height,
                                          dmux.features.has_alpha,
                                          0,
                                          spec);
  }
  GWPAnimRewindStorage(enc, mark);
  return GWP_MUX_BAD_DATA;
}

GWPMuxError GWPAnimEncoderAddFramePixels(GWPAnimEncoder* enc,
                                         const GWPu8* pixels,
                                         GWPu32 stride,
                                         GWPRawPixelFormat pixel_format,
                                         const GWPAnimFrameSpec* spec,
                                         const GWPEncodeConfig* config) {
  GWPAnimFrameSpec local_spec;
  GWPu32 left, top, right, bottom;
  GWPBool force_key_frame;
  GWPBool has_diff;
  GWPu32 rect_w, rect_h;
  GWPu32 storage_mark;
  GWPu32 bitstream_size;
  GWPBool has_alpha;
  GWPStatusCode st;
  GWPData bitstream_data;
  GWPMuxError mx;
  GWPu32 y;
  GWPu8* bitstream_dst;
  GWPu32 lossless_estimate;
  if (enc == 0 || pixels == 0 || spec == 0 || config == 0) return GWP_MUX_INVALID_ARGUMENT;
  if (enc->canvas_width == 0u || enc->canvas_height == 0u) return GWP_MUX_INVALID_ARGUMENT;
  if (enc->options.minimize_size) {
    enc->options.kmin = 0u;
    enc->options.kmax = 0u;
  } else if (enc->frame_count == 0u && enc->options.kmin == 0u && enc->options.kmax == 0u) {
    if (config->lossless) {
      enc->options.kmin = 9u;
      enc->options.kmax = 17u;
    } else {
      enc->options.kmin = 3u;
      enc->options.kmax = 5u;
    }
  }
  if (stride < enc->canvas_width * 4u) return GWP_MUX_INVALID_ARGUMENT;
  if (enc->canvas_rgba == 0 || enc->storage_bytes == 0 || enc->storage_size == 0u) {
    return GWP_MUX_NOT_ENOUGH_DATA;
  }
  local_spec = *spec;
  force_key_frame = (enc->frame_count == 0u || !enc->canvas_valid) ?
                    GWP_TRUE : GWP_FALSE;
  if (!force_key_frame && enc->options.kmax != 0u &&
      enc->last_key_frame_distance >= enc->options.kmax) {
    force_key_frame = GWP_TRUE;
  }
  if (force_key_frame) {
    left = 0u;
    top = 0u;
    right = enc->canvas_width;
    bottom = enc->canvas_height;
  } else {
    has_diff = GWPAnimEncoderFindDiffRect(enc, pixels, stride, pixel_format,
                                          &left, &top, &right, &bottom);
    if (!has_diff) {
      if (enc->frame_count == 0u) {
        left = 0u;
        top = 0u;
        right = enc->canvas_width;
        bottom = enc->canvas_height;
      } else {
        enc->frames[enc->frame_count - 1u].duration_ms += spec->duration_ms;
        return GWP_MUX_OK;
      }
    }
    if (enc->options.snap_odd_offsets) {
      if ((left & 1u) != 0u && left > 0u) --left;
      if ((top & 1u) != 0u && top > 0u) --top;
    }
  }
  rect_w = right - left;
  rect_h = bottom - top;
  if (rect_w == 0u || rect_h == 0u) return GWP_MUX_INVALID_ARGUMENT;

  local_spec.x_offset = left;
  local_spec.y_offset = top;
  local_spec.blend_method = (GWPu8)GWP_ANIM_NO_BLEND;
  local_spec.dispose_method = (GWPu8)GWP_ANIM_DISPOSE_NONE;

  storage_mark = enc->storage_used;
  lossless_estimate = GWPEstimateLosslessBitstreamSize(rect_w, rect_h);
  if (lossless_estimate == 0xffffffffu) return GWP_MUX_INVALID_ARGUMENT;
  bitstream_dst = GWPAnimAllocStorage(enc, lossless_estimate, 0);
  if (bitstream_dst == 0) return GWP_MUX_NOT_ENOUGH_OUTPUT;
  st = GWPEncodeVP8LBitstream(pixels + top * stride + left * 4u,
                              rect_w, rect_h, stride, pixel_format,
                              config->exact,
                              bitstream_dst,
                              lossless_estimate,
                              &bitstream_size,
                              &has_alpha);
  if (st != GWP_STATUS_OK) {
    GWPAnimRewindStorage(enc, storage_mark);
    return GWP_MUX_BAD_DATA;
  }
  enc->storage_used = storage_mark + bitstream_size;
  bitstream_data.bytes = bitstream_dst;
  bitstream_data.size = bitstream_size;

  if (!config->lossless && !config->allow_external_tools &&
      config->scratch != 0 && config->scratch_size > 64u) {
    GWPEncodeConfig native_cfg;
    GWPu32 native_cap;
    GWPu32 native_out_size;
    native_cap = GWPEstimateLossyWebPSizeFromPixels(rect_w, rect_h);
    if (native_cap != 0xffffffffu && config->scratch_size > native_cap + 64u) {
      GWPEncodeConfigInit(&native_cfg);
      native_cfg.lossless = GWP_FALSE;
      native_cfg.exact = config->exact;
      native_cfg.quality = config->quality;
      native_cfg.method = config->method;
      native_cfg.use_sharp_yuv = config->use_sharp_yuv;
      native_cfg.preset = config->preset;
      native_cfg.near_lossless = config->near_lossless;
      native_cfg.alpha_quality = config->alpha_quality;
      native_cfg.filter_strength = config->filter_strength;
      native_cfg.alpha_filtering = config->alpha_filtering;
      native_cfg.output_buffer = (GWPu8*)config->scratch;
      native_cfg.output_buffer_size = native_cap;
      native_cfg.scratch = (GWPu8*)config->scratch + native_cap;
      native_cfg.scratch_size = config->scratch_size - native_cap;
      native_out_size = 0u;
      if (GWPEncodePixels(pixels + top * stride + left * 4u,
                          rect_w, rect_h, stride, pixel_format,
                          &native_cfg, &native_out_size) == GWP_STATUS_OK) {
        GWPAnimRewindStorage(enc, storage_mark);
        mx = GWPAnimCommitStillWebPToStorage(enc, native_cfg.output_buffer, native_out_size, &local_spec);
        if (mx != GWP_MUX_OK) return mx;
        if (enc->frames[enc->frame_count - 1u].is_key_frame) {
          enc->last_key_frame_distance = 0u;
        } else {
          ++enc->last_key_frame_distance;
        }
        for (y = 0u; y < enc->canvas_height; ++y) {
          GWPCopy(enc->canvas_rgba + y * enc->canvas_width * 4u,
                  pixels + y * stride,
                  enc->canvas_width * 4u);
        }
        if (pixel_format != GWP_RAW_RGBA) {
          GWPCopyFrameToRGBA(enc, pixels, stride, pixel_format, enc->canvas_rgba);
        }
        enc->canvas_valid = GWP_TRUE;
        return GWP_MUX_OK;
      }
    }
  }

  if (enc->options.allow_mixed && !config->lossless && config->allow_external_tools &&
      config->scratch != 0 && config->scratch_size > 0u) {
    GWPEncodeConfig lossy_cfg;
    GWPu32 stage_cap;
    GWPu32 lossy_out_size;
    GWPu8* scratch_bytes;
    GWPu8* lossy_out;
    GWPu32 lossy_cap;
    stage_cap = GWPEstimateWebPSizeFromPixels(rect_w, rect_h);
    if (stage_cap != 0xffffffffu && config->scratch_size > stage_cap + 64u) {
      scratch_bytes = (GWPu8*)config->scratch;
      lossy_out = scratch_bytes + stage_cap;
      lossy_cap = config->scratch_size - stage_cap;
      GWPEncodeConfigInit(&lossy_cfg);
      lossy_cfg.lossless = GWP_FALSE;
      lossy_cfg.exact = config->exact;
      lossy_cfg.quality = config->quality;
      lossy_cfg.method = config->method;
      lossy_cfg.use_sharp_yuv = config->use_sharp_yuv;
      lossy_cfg.alpha_filtering = config->alpha_filtering;
      lossy_cfg.allow_external_tools = config->allow_external_tools;
      lossy_cfg.cwebp_path = config->cwebp_path;
      lossy_cfg.output_buffer = lossy_out;
      lossy_cfg.output_buffer_size = lossy_cap;
      lossy_cfg.scratch = scratch_bytes;
      lossy_cfg.scratch_size = stage_cap;
      lossy_out_size = 0u;
      if (GWPEncodePixels(pixels + top * stride + left * 4u,
                          rect_w, rect_h, stride, pixel_format,
                          &lossy_cfg, &lossy_out_size) == GWP_STATUS_OK) {
        GWPDemuxer lossy_dmx;
        if (GWPDemuxParse(&lossy_dmx, lossy_out, lossy_out_size) == GWP_STATUS_OK &&
            !lossy_dmx.features.has_animation) {
          GWPu32 lossy_cost;
          lossy_cost = 0u;
          if (lossy_dmx.features.format == GWP_BITSTREAM_VP8) {
            lossy_cost = lossy_dmx.vp8_payload.size + lossy_dmx.alph_payload.size;
          } else if (lossy_dmx.features.format == GWP_BITSTREAM_VP8L) {
            lossy_cost = lossy_dmx.vp8l_payload.size;
          }
          if (lossy_cost != 0u && lossy_cost < bitstream_size) {
            GWPAnimRewindStorage(enc, storage_mark);
            mx = GWPAnimCommitStillWebPToStorage(enc, lossy_out, lossy_out_size, &local_spec);
            if (mx != GWP_MUX_OK) return mx;
            if (enc->frames[enc->frame_count - 1u].is_key_frame) {
              enc->last_key_frame_distance = 0u;
            } else {
              ++enc->last_key_frame_distance;
            }
            for (y = 0u; y < enc->canvas_height; ++y) {
              GWPCopy(enc->canvas_rgba + y * enc->canvas_width * 4u,
                      pixels + y * stride,
                      enc->canvas_width * 4u);
            }
            if (pixel_format != GWP_RAW_RGBA) {
              GWPCopyFrameToRGBA(enc, pixels, stride, pixel_format, enc->canvas_rgba);
            }
            enc->canvas_valid = GWP_TRUE;
            return GWP_MUX_OK;
          }
        }
      }
    }
  }

  mx = GWPAnimEncoderAddFrameBitstream(enc,
                                       GWP_BITSTREAM_VP8L,
                                       &bitstream_data,
                                       rect_w,
                                       rect_h,
                                       has_alpha,
                                       0,
                                       &local_spec);
  if (mx != GWP_MUX_OK) {
    GWPAnimRewindStorage(enc, storage_mark);
    return mx;
  }
  if (enc->frames[enc->frame_count - 1u].is_key_frame) {
    enc->last_key_frame_distance = 0u;
  } else {
    ++enc->last_key_frame_distance;
  }

  for (y = 0u; y < enc->canvas_height; ++y) {
    GWPCopy(enc->canvas_rgba + y * enc->canvas_width * 4u,
            pixels + y * stride,
            enc->canvas_width * 4u);
  }
  if (pixel_format != GWP_RAW_RGBA) {
    GWPCopyFrameToRGBA(enc, pixels, stride, pixel_format, enc->canvas_rgba);
  }
  enc->canvas_valid = GWP_TRUE;
  return GWP_MUX_OK;
}

static GWPu32 GWPAnimFramePayloadSize(const GWPAnimEncoderFrame* frame) {
  GWPu32 payload;
  if (frame == 0 || frame->bitstream_payload.bytes == 0) return 0u;
  payload = 16u + GWPChunkDiskSizeAnim(frame->bitstream_payload.size);
  if (frame->alph_payload.bytes != 0) payload += GWPChunkDiskSizeAnim(frame->alph_payload.size);
  return payload;
}

GWPu32 GWPAnimEncoderEstimateSize(const GWPAnimEncoder* enc) {
  GWPu32 total;
  GWPu32 i;
  if (enc == 0 || enc->frame_count == 0u || enc->canvas_width == 0u || enc->canvas_height == 0u) {
    return 0u;
  }
  total = 12u;
  total += GWPChunkDiskSizeAnim(10u);
  if (enc->iccp.bytes != 0) total += GWPChunkDiskSizeAnim(enc->iccp.size);
  total += GWPChunkDiskSizeAnim(6u);
  for (i = 0u; i < enc->frame_count; ++i) {
    total += GWPChunkDiskSizeAnim(GWPAnimFramePayloadSize(&enc->frames[i]));
  }
  if (enc->exif.bytes != 0) total += GWPChunkDiskSizeAnim(enc->exif.size);
  if (enc->xmp.bytes != 0) total += GWPChunkDiskSizeAnim(enc->xmp.size);
  return total;
}

static GWPMuxError GWPWriteANMFChunk(GWPu8** dst,
                                     GWPu32* remain,
                                     const GWPAnimEncoderFrame* frame) {
  GWPu32 payload_size;
  GWPu32 disk_size;
  GWPu8* p;
  GWPu8* sub;
  GWPu32 sub_remain;
  if (dst == 0 || *dst == 0 || remain == 0 || frame == 0 || frame->bitstream_payload.bytes == 0) {
    return GWP_MUX_INVALID_ARGUMENT;
  }
  payload_size = GWPAnimFramePayloadSize(frame);
  disk_size = GWPChunkDiskSizeAnim(payload_size);
  if (*remain < disk_size) return GWP_MUX_NOT_ENOUGH_OUTPUT;
  p = *dst;
  GWPWriteLE32(p + 0, GWP_FOURCC_ANMF);
  GWPWriteLE32(p + 4, payload_size);
  GWPWriteLE24(p + 8, frame->x_offset / 2u);
  GWPWriteLE24(p + 11, frame->y_offset / 2u);
  GWPWriteLE24(p + 14, frame->width - 1u);
  GWPWriteLE24(p + 17, frame->height - 1u);
  GWPWriteLE24(p + 20, frame->duration_ms);
  p[23] = (GWPu8)((frame->dispose_method & 1u) | ((frame->blend_method & 1u) << 1));

  sub = p + 24u;
  sub_remain = payload_size - 16u;
  if (frame->alph_payload.bytes != 0) {
    GWPData d;
    d = frame->alph_payload;
    if (GWPWriteChunkAnim(&sub, &sub_remain, GWP_FOURCC_ALPH, &d) != GWP_MUX_OK) {
      return GWP_MUX_NOT_ENOUGH_OUTPUT;
    }
  }
  {
    GWPData d;
    d = frame->bitstream_payload;
    if (GWPWriteChunkAnim(&sub, &sub_remain, frame->bitstream_fourcc, &d) != GWP_MUX_OK) {
      return GWP_MUX_NOT_ENOUGH_OUTPUT;
    }
  }
  if ((payload_size & 1u) != 0u) p[8u + payload_size] = 0u;
  *dst += disk_size;
  *remain -= disk_size;
  return GWP_MUX_OK;
}

GWPMuxError GWPAnimEncoderAssemble(const GWPAnimEncoder* enc,
                                   GWPu8* out_buf,
                                   GWPu32 out_buf_size,
                                   GWPu32* out_size) {
  GWPu32 total;
  GWPu8* p;
  GWPu32 remain;
  GWPu8 vp8x_payload[10];
  GWPData chunk_data;
  GWPu32 i;
  if (out_size != 0) *out_size = 0u;
  if (enc == 0 || out_buf == 0 || out_size == 0) return GWP_MUX_INVALID_ARGUMENT;
  if (enc->canvas_width == 0u || enc->canvas_height == 0u || enc->frame_count == 0u) {
    return GWP_MUX_INVALID_ARGUMENT;
  }
  total = GWPAnimEncoderEstimateSize(enc);
  if (total == 0u) return GWP_MUX_INVALID_ARGUMENT;
  if (out_buf_size < total) return GWP_MUX_NOT_ENOUGH_OUTPUT;

  GWPWriteLE32(out_buf + 0, GWP_FOURCC_RIFF);
  GWPWriteLE32(out_buf + 4, total - 8u);
  GWPWriteLE32(out_buf + 8, GWP_FOURCC_WEBP);
  p = out_buf + 12u;
  remain = out_buf_size - 12u;

  GWPZero(vp8x_payload, 10u);
  if (enc->iccp.bytes != 0) vp8x_payload[0] |= 0x20u;
  if (enc->has_alpha) vp8x_payload[0] |= 0x10u;
  if (enc->exif.bytes != 0) vp8x_payload[0] |= 0x08u;
  if (enc->xmp.bytes != 0) vp8x_payload[0] |= 0x04u;
  vp8x_payload[0] |= 0x02u;
  GWPWriteLE24(vp8x_payload + 4, enc->canvas_width - 1u);
  GWPWriteLE24(vp8x_payload + 7, enc->canvas_height - 1u);
  chunk_data.bytes = vp8x_payload;
  chunk_data.size = 10u;
  if (GWPWriteChunkAnim(&p, &remain, GWP_FOURCC_VP8X, &chunk_data) != GWP_MUX_OK) {
    return GWP_MUX_NOT_ENOUGH_OUTPUT;
  }

  if (enc->iccp.bytes != 0) {
    if (GWPWriteChunkAnim(&p, &remain, GWP_FOURCC_ICCP, &enc->iccp) != GWP_MUX_OK) {
      return GWP_MUX_NOT_ENOUGH_OUTPUT;
    }
  }

  {
    GWPu8 anim_payload[6];
    GWPData anim_data;
    GWPWriteLE32(anim_payload + 0, enc->anim_params.bgcolor);
    GWPWriteLE16(anim_payload + 4, enc->anim_params.loop_count);
    anim_data.bytes = anim_payload;
    anim_data.size = 6u;
    if (GWPWriteChunkAnim(&p, &remain, GWP_FOURCC_ANIM, &anim_data) != GWP_MUX_OK) {
      return GWP_MUX_NOT_ENOUGH_OUTPUT;
    }
  }

  for (i = 0u; i < enc->frame_count; ++i) {
    if (GWPWriteANMFChunk(&p, &remain, &enc->frames[i]) != GWP_MUX_OK) {
      return GWP_MUX_NOT_ENOUGH_OUTPUT;
    }
  }

  if (enc->exif.bytes != 0) {
    if (GWPWriteChunkAnim(&p, &remain, GWP_FOURCC_EXIF, &enc->exif) != GWP_MUX_OK) {
      return GWP_MUX_NOT_ENOUGH_OUTPUT;
    }
  }
  if (enc->xmp.bytes != 0) {
    if (GWPWriteChunkAnim(&p, &remain, GWP_FOURCC_XMP, &enc->xmp) != GWP_MUX_OK) {
      return GWP_MUX_NOT_ENOUGH_OUTPUT;
    }
  }

  *out_size = total;
  return GWP_MUX_OK;
}
