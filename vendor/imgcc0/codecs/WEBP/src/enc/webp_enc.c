#include "../webp/encode.h"

#include "vp8l_enc.h"
#include "vp8_lossy_plan.h"
#include "vp8_intra_emit.h"
#include "vp8_native_enc.h"
#include "tool_bridge.h"
#include "../utils/common.h"
#include "../utils/endian.h"

void GWPEncodeConfigInit(GWPEncodeConfig* config) {
  if (config == 0) return;
  config->lossless = GWP_TRUE;
  config->exact = GWP_TRUE;
  config->quality = 100u;
  config->method = 4u;
  config->use_sharp_yuv = GWP_FALSE;
  config->preset = GWP_PRESET_DEFAULT;
  config->near_lossless = 100u;
  config->alpha_quality = 100u;
  config->filter_strength = 35u;
  config->vp8_partitions = 0u;
  config->vp8_segments = 0u;
  config->vp8_enable_coeff_skip = GWP_TRUE;
  config->vp8_enable_prob_updates = GWP_TRUE;
  config->vp8_enable_subblock_modes = GWP_TRUE;
  config->vp8_enable_uv_modes = GWP_TRUE;
  config->vp8_enable_intra16_modes = GWP_TRUE;
  config->vp8_enable_ac_coeffs = GWP_TRUE;
  config->alpha_filtering = GWP_ALPHA_FILTER_FAST;
  config->allow_external_tools = GWP_FALSE;
  config->cwebp_path = 0;
  config->img2webp_path = 0;
  config->gif2webp_path = 0;
  config->output_buffer = 0;
  config->output_buffer_size = 0u;
  config->scratch = 0;
  config->scratch_size = 0u;
}

void GWPVP8LBitstreamOptionsInit(GWPVP8LBitstreamOptions* options) {
  if (options == 0) return;
  options->exact = GWP_TRUE;
  options->near_lossless = 100u;
  options->use_subtract_green = GWP_TRUE;
  options->use_color_cache = GWP_TRUE;
  options->color_cache_bits = 4u;
  options->use_backrefs = GWP_TRUE;
}

GWPu32 GWPEstimateLosslessBitstreamSize(GWPu32 width, GWPu32 height) {
  GWPu32 pixels;
  if (!GWPMulU32(width, height, &pixels)) return 0xffffffffu;
  if (pixels > (0xffffffffu - 2048u) / 8u) return 0xffffffffu;
  return 2048u + pixels * 8u;
}

GWPu32 GWPEstimateWebPSizeFromPixels(GWPu32 width, GWPu32 height) {
  GWPu32 bitstream;
  bitstream = GWPEstimateLosslessBitstreamSize(width, height);
  if (bitstream == 0xffffffffu) return bitstream;
  if (bitstream > 0xffffffffu - 21u) return 0xffffffffu;
  return 12u + 8u + GWP_ALIGN2(bitstream);
}

GWPu32 GWPEstimateLossyWebPSizeFromPixels(GWPu32 width, GWPu32 height) {
  GWPu32 bitstream;
  bitstream = GWPEstimateNativeVP8BitstreamSize(width, height);
  if (bitstream == 0xffffffffu) return bitstream;
  if (bitstream > 0xffffffffu - 21u) return 0xffffffffu;
  return 20u + GWP_ALIGN2(bitstream);
}

GWPStatusCode GWPAnalyzeVP8LossyPlan(const GWPu8* pixels,
                                     GWPu32 width,
                                     GWPu32 height,
                                     GWPu32 stride,
                                     GWPRawPixelFormat pixel_format,
                                     const GWPEncodeConfig* config,
                                     void* scratch,
                                     GWPu32 scratch_size,
                                     GWPVP8LossyPlan* out_plan) {
  GWPEncodeConfig local_cfg;
  if (config == 0) {
    GWPEncodeConfigInit(&local_cfg);
    local_cfg.lossless = GWP_FALSE;
    config = &local_cfg;
  }
  return GWPVP8BuildLossyPlan(pixels, width, height, stride, pixel_format,
                              config, scratch, scratch_size, out_plan);
}

GWPStatusCode GWPEncodeVP8LBitstream(const GWPu8* pixels,
                                     GWPu32 width,
                                     GWPu32 height,
                                     GWPu32 stride,
                                     GWPRawPixelFormat pixel_format,
                                     GWPBool exact,
                                     GWPu8* out_buf,
                                     GWPu32 out_buf_size,
                                     GWPu32* out_size,
                                     GWPBool* out_has_alpha) {
  GWPVP8LBitstreamOptions opts;
  GWPVP8LBitstreamOptionsInit(&opts);
  opts.exact = exact;
  return GWPVP8LEncodeImageEx(pixels, width, height, stride, pixel_format,
                              &opts, out_buf, out_buf_size, out_size,
                              out_has_alpha);
}

GWPStatusCode GWPEncodeVP8LBitstreamEx(const GWPu8* pixels,
                                       GWPu32 width,
                                       GWPu32 height,
                                       GWPu32 stride,
                                       GWPRawPixelFormat pixel_format,
                                       const GWPVP8LBitstreamOptions* options,
                                       GWPu8* out_buf,
                                       GWPu32 out_buf_size,
                                       GWPu32* out_size,
                                       GWPBool* out_has_alpha) {
  return GWPVP8LEncodeImageEx(pixels, width, height, stride, pixel_format,
                              options, out_buf, out_buf_size, out_size,
                              out_has_alpha);
}

GWPStatusCode GWPEncodePixels(const GWPu8* pixels,
                              GWPu32 width,
                              GWPu32 height,
                              GWPu32 stride,
                              GWPRawPixelFormat pixel_format,
                              const GWPEncodeConfig* config,
                              GWPu32* out_size) {
  GWPu32 bitstream_size;
  GWPBool has_alpha;
  GWPu8* out_buf;
  GWPu32 out_buf_size;
  GWPStatusCode st;
  GWPVP8LBitstreamOptions vp8l_opts;
  if (out_size != 0) *out_size = 0u;
  if (config == 0 || out_size == 0) return GWP_STATUS_INVALID_PARAM;
  if (config->output_buffer == 0) return GWP_STATUS_INVALID_PARAM;

  out_buf = config->output_buffer;
  out_buf_size = config->output_buffer_size;
  if (out_buf_size < 20u) return GWP_STATUS_NOT_ENOUGH_OUTPUT;

  if (!config->lossless) {
    GWPStatusCode native_st;
    GWPBool has_transparency;
    GWPu32 x;
    GWPu32 y;
    has_transparency = GWP_FALSE;
    for (y = 0u; y < height && !has_transparency; ++y) {
      const GWPu8* row = pixels + y * stride;
      for (x = 0u; x < width; ++x) {
        GWPu8 a;
        if (pixel_format == GWP_RAW_ARGB) a = row[x * 4u + 0u];
        else a = row[x * 4u + 3u];
        if (a != 255u) { has_transparency = GWP_TRUE; break; }
      }
    }
    if (config->allow_external_tools) {
      native_st = GWPEncodeViaCWebP(pixels, width, height, stride, pixel_format, config, out_size);
      if (native_st == GWP_STATUS_OK) return native_st;
    }
    if (!has_transparency) {
      GWPu32 bitstream_size;
      GWPVP8NativeEncodeStats stats;
      if (out_buf_size < 20u) return GWP_STATUS_NOT_ENOUGH_OUTPUT;
      GWPVP8NativeEncodeStatsInit(&stats);
      native_st = GWPEncodeVP8Bitstream(pixels, width, height, stride, pixel_format, config,
                                        out_buf + 20u, out_buf_size - 20u,
                                        &bitstream_size, &stats);
      if (native_st == GWP_STATUS_OK) {
        GWPWriteLE32(out_buf + 0, GWP_FOURCC_RIFF);
        GWPWriteLE32(out_buf + 4, 12u + GWP_ALIGN2(bitstream_size));
        GWPWriteLE32(out_buf + 8, GWP_FOURCC_WEBP);
        GWPWriteLE32(out_buf + 12, GWP_FOURCC_VP8);
        GWPWriteLE32(out_buf + 16, bitstream_size);
        if ((bitstream_size & 1u) != 0u) out_buf[20u + bitstream_size] = 0u;
        *out_size = 20u + GWP_ALIGN2(bitstream_size);
        return GWP_STATUS_OK;
      }
    }
    native_st = GWPEncodeLossyNativeProxy(pixels, width, height, stride, pixel_format, config, out_size);
    return native_st;
  }

  GWPVP8LBitstreamOptionsInit(&vp8l_opts);
  vp8l_opts.exact = config->exact;
  vp8l_opts.near_lossless = config->near_lossless;
  vp8l_opts.use_subtract_green = (config->method >= 2u) ? GWP_TRUE : GWP_FALSE;
  vp8l_opts.use_backrefs = (config->method >= 3u) ? GWP_TRUE : GWP_FALSE;
  vp8l_opts.use_color_cache = (config->method >= 2u) ? GWP_TRUE : GWP_FALSE;
  vp8l_opts.color_cache_bits = (config->method >= 5u) ? 5u : 4u;

  st = GWPEncodeVP8LBitstreamEx(pixels, width, height, stride, pixel_format,
                                &vp8l_opts,
                                out_buf + 20u,
                                out_buf_size - 20u, &bitstream_size, &has_alpha);
  if (st != GWP_STATUS_OK) return st;
  GWPWriteLE32(out_buf + 0, GWP_FOURCC_RIFF);
  GWPWriteLE32(out_buf + 4, 12u + GWP_ALIGN2(bitstream_size));
  GWPWriteLE32(out_buf + 8, GWP_FOURCC_WEBP);
  GWPWriteLE32(out_buf + 12, GWP_FOURCC_VP8L);
  GWPWriteLE32(out_buf + 16, bitstream_size);
  if ((bitstream_size & 1u) != 0u) out_buf[20u + bitstream_size] = 0u;
  *out_size = 20u + GWP_ALIGN2(bitstream_size);
  (void)has_alpha;
  return GWP_STATUS_OK;
}
