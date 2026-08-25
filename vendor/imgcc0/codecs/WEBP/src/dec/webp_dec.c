#include "../webp/decode.h"
#include "../webp/demux.h"
#include "../utils/common.h"
#include "vp8l_dec.h"
#include "vp8_dec.h"
#include "alpha_dec.h"

const char* GWPStatusToString(GWPStatusCode status) {
  switch (status) {
    case GWP_STATUS_OK: return "ok";
    case GWP_STATUS_INVALID_PARAM: return "invalid param";
    case GWP_STATUS_TRUNCATED_DATA: return "truncated data";
    case GWP_STATUS_BAD_SIGNATURE: return "bad signature";
    case GWP_STATUS_BAD_DIMENSIONS: return "bad dimensions";
    case GWP_STATUS_LIMIT_EXCEEDED: return "limit exceeded";
    case GWP_STATUS_NOT_ENOUGH_OUTPUT: return "not enough output";
    case GWP_STATUS_NOT_ENOUGH_SCRATCH: return "not enough scratch";
    case GWP_STATUS_BITSTREAM_ERROR: return "bitstream error";
    case GWP_STATUS_PARSE_ERROR: return "parse error";
    case GWP_STATUS_UNSUPPORTED_FEATURE: return "unsupported feature";
    case GWP_STATUS_UNSUPPORTED_FORMAT: return "unsupported format";
    case GWP_STATUS_NOT_IMPLEMENTED: return "not implemented";
    default: return "unknown";
  }
}

GWPStatusCode GWPGetFeatures(const GWPu8* data,
                             GWPu32 data_size,
                             GWPBitstreamFeatures* features) {
  GWPDemuxer dmux;
  GWPStatusCode st;
  if (data == 0 || features == 0) return GWP_STATUS_INVALID_PARAM;
  st = GWPDemuxParse(&dmux, data, data_size);
  if (st != GWP_STATUS_OK) return st;
  *features = dmux.features;
  return GWP_STATUS_OK;
}

GWPStatusCode GWPDecode(const GWPu8* data,
                        GWPu32 data_size,
                        const GWPDecoderOptions* options,
                        GWPBitstreamFeatures* out_features) {
  GWPDemuxer dmux;
  GWPStatusCode st;
  if (data == 0 || options == 0) return GWP_STATUS_INVALID_PARAM;
  if (options->output_buffer == 0 || options->scratch == 0) return GWP_STATUS_INVALID_PARAM;
  if (options->output_stride < 4u) return GWP_STATUS_INVALID_PARAM;

  st = GWPDemuxParse(&dmux, data, data_size);
  if (st != GWP_STATUS_OK) return st;

  if (dmux.features.format == GWP_BITSTREAM_VP8L && dmux.vp8l_payload.bytes != 0) {
    st = GWPDecodeVP8L(dmux.vp8l_payload.bytes, dmux.vp8l_payload.size, options, out_features);
    if (st == GWP_STATUS_OK && out_features != 0) {
      out_features->has_icc = dmux.features.has_icc;
      out_features->has_exif = dmux.features.has_exif;
      out_features->has_xmp = dmux.features.has_xmp;
    }
    return st;
  }
  if (dmux.features.format == GWP_BITSTREAM_VP8 && dmux.vp8_payload.bytes != 0) {
    st = GWPDecodeVP8Stub(dmux.vp8_payload.bytes, dmux.vp8_payload.size, options, out_features);
    if (st != GWP_STATUS_OK && st != GWP_STATUS_NOT_IMPLEMENTED) return st;
    if (st == GWP_STATUS_OK && dmux.alph_payload.bytes != 0) {
      st = GWPDecodeAlphaChunk(dmux.alph_payload.bytes,
                               dmux.alph_payload.size,
                               dmux.features.width,
                               dmux.features.height,
                               options->pixel_format,
                               options->output_buffer,
                               options->output_stride,
                               options->scratch,
                               options->scratch_size);
      if (st != GWP_STATUS_OK) return st;
    }
    if (out_features != 0) {
      out_features->has_alpha = dmux.features.has_alpha;
      out_features->has_icc = dmux.features.has_icc;
      out_features->has_exif = dmux.features.has_exif;
      out_features->has_xmp = dmux.features.has_xmp;
    }
    return st;
  }
  return GWP_STATUS_UNSUPPORTED_FORMAT;
}
