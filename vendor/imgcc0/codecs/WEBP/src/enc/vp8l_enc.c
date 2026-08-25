#include "vp8l_enc.h"

#include "bit_writer.h"
#include "huffman_enc.h"
#include "../utils/common.h"

#define GWP_VP8L_LITERAL_ALPHABET 256u
#define GWP_VP8L_LENGTH_CODES     24u
#define GWP_VP8L_MAIN_BASE        (GWP_VP8L_LITERAL_ALPHABET + GWP_VP8L_LENGTH_CODES)
#define GWP_VP8L_MAX_CACHE_SIZE   (1u << GWP_VP8L_CACHE_MAX_BITS)

static const GWPu8 kCodeLenOrder[19] = {
  17, 18, 0, 1, 2, 3, 4, 5, 16, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

static void GWPReadPixel(const GWPu8* row,
                         GWPRawPixelFormat fmt,
                         GWPu32 x,
                         GWPu8* a,
                         GWPu8* r,
                         GWPu8* g,
                         GWPu8* b) {
  const GWPu8* p;
  p = row + 4u * x;
  if (fmt == GWP_RAW_BGRA) {
    *b = p[0]; *g = p[1]; *r = p[2]; *a = p[3];
  } else if (fmt == GWP_RAW_ARGB) {
    *a = p[0]; *r = p[1]; *g = p[2]; *b = p[3];
  } else {
    *r = p[0]; *g = p[1]; *b = p[2]; *a = p[3];
  }
}

static GWPu32 GWPHashColor(GWPu32 color, GWPu32 bits) {
  return (GWPu32)((0x1e35a7bdu * color) >> (32u - bits));
}

static void GWPColorCacheClear(GWPu32* cache, GWPu32 size) {
  if (cache == 0) return;
  GWPZero(cache, size * (GWPu32)sizeof(GWPu32));
}

static void GWPColorCacheInsert(GWPu32* cache, GWPu32 bits, GWPu32 color) {
  cache[GWPHashColor(color, bits)] = color;
}

static GWPu32 GWPClampToStep(GWPu32 v, GWPu32 step) {
  GWPu32 q;
  if (step <= 1u) return v;
  q = (v + (step >> 1)) / step;
  q *= step;
  if (q > 255u) q = 255u;
  return q;
}

static GWPu32 GWPNearLosslessStep(GWPu32 near_lossless) {
  if (near_lossless >= 100u) return 1u;
  if (near_lossless >= 90u) return 2u;
  if (near_lossless >= 75u) return 4u;
  if (near_lossless >= 60u) return 8u;
  if (near_lossless >= 45u) return 16u;
  return 32u;
}

static GWPu32 GWPProcessPixelAt(const GWPu8* pixels,
                                GWPu32 width,
                                GWPu32 stride,
                                GWPRawPixelFormat pixel_format,
                                const GWPVP8LBitstreamOptions* options,
                                GWPu32 pos) {
  const GWPu8* row;
  GWPu32 x;
  GWPu8 a, r, g, b;
  GWPu32 step;
  row = pixels + (pos / width) * stride;
  x = pos % width;
  GWPReadPixel(row, pixel_format, x, &a, &r, &g, &b);

  if (options != 0) {
    if (!options->exact && a == 0u) {
      r = 0u;
      g = 0u;
      b = 0u;
    }
    step = GWPNearLosslessStep(options->near_lossless);
    if (step > 1u) {
      r = (GWPu8)GWPClampToStep(r, step);
      g = (GWPu8)GWPClampToStep(g, step);
      b = (GWPu8)GWPClampToStep(b, step);
    }
    if (options->use_subtract_green) {
      r = (GWPu8)(((GWPu32)r - (GWPu32)g) & 0xffu);
      b = (GWPu8)(((GWPu32)b - (GWPu32)g) & 0xffu);
    }
  }

  return ((GWPu32)a << 24) | ((GWPu32)r << 16) | ((GWPu32)g << 8) | (GWPu32)b;
}

static void GWPEncodePrefixValue(GWPu32 value,
                                 GWPu32* out_prefix,
                                 GWPu32* out_extra,
                                 int* out_extra_bits) {
  GWPu32 prefix;
  for (prefix = 0u; prefix < 64u; ++prefix) {
    if (prefix < 4u) {
      if (value == prefix + 1u) {
        *out_prefix = prefix;
        *out_extra = 0u;
        *out_extra_bits = 0;
        return;
      }
    } else {
      GWPu32 extra_bits;
      GWPu32 offset;
      GWPu32 start;
      GWPu32 end;
      extra_bits = (prefix - 2u) >> 1;
      offset = (2u + (prefix & 1u)) << extra_bits;
      start = offset + 1u;
      end = offset + (((GWPu32)1u) << extra_bits);
      if (value >= start && value <= end) {
        *out_prefix = prefix;
        *out_extra = value - start;
        *out_extra_bits = (int)extra_bits;
        return;
      }
    }
  }
  *out_prefix = 0u;
  *out_extra = 0u;
  *out_extra_bits = 0;
}

static GWPStatusCode GWPWriteNormalPrefixCode(const GWPu8* lengths,
                                              GWPu32 alphabet_size,
                                              GWPBitWriter* bw) {
  GWPu32 temp_freq[19];
  GWPu8 temp_lengths[19];
  GWPHuffEncSymbol temp_symbols[19];
  GWPu32 i;
  GWPu32 last_non_zero_order;
  GWPStatusCode st;
  if (lengths == 0 || bw == 0) return GWP_STATUS_INVALID_PARAM;
  for (i = 0u; i < 19u; ++i) temp_freq[i] = 0u;
  for (i = 0u; i < alphabet_size; ++i) {
    if (lengths[i] > 18u) return GWP_STATUS_LIMIT_EXCEEDED;
    ++temp_freq[lengths[i]];
  }
  st = GWPHuffEncBuildLengths(temp_freq, 19u, temp_lengths);
  if (st != GWP_STATUS_OK) return st;
  GWPHuffEncBuildCodes(temp_lengths, 19u, temp_symbols);

  last_non_zero_order = 0u;
  for (i = 0u; i < 19u; ++i) {
    if (temp_lengths[kCodeLenOrder[i]] != 0u) last_non_zero_order = i;
  }
  if (last_non_zero_order < 3u) last_non_zero_order = 3u;

  GWPBitWriterPutBits(bw, 0u, 1);
  GWPBitWriterPutBits(bw, last_non_zero_order - 3u, 4);
  for (i = 0u; i <= last_non_zero_order; ++i) {
    GWPBitWriterPutBits(bw, temp_lengths[kCodeLenOrder[i]], 3);
  }
  GWPBitWriterPutBits(bw, 0u, 1);
  for (i = 0u; i < alphabet_size; ++i) {
    GWPHuffEncPutSymbol(temp_symbols, lengths[i], bw);
  }
  return GWPBitWriterOk(bw) ? GWP_STATUS_OK : GWP_STATUS_NOT_ENOUGH_OUTPUT;
}

static GWPStatusCode GWPBuildTreeFromPixels(const GWPu32* freqs,
                                            GWPu32 alphabet_size,
                                            GWPu8* out_lengths,
                                            GWPHuffEncSymbol* out_symbols) {
  GWPStatusCode st;
  st = GWPHuffEncBuildLengths(freqs, alphabet_size, out_lengths);
  if (st != GWP_STATUS_OK) return st;
  GWPHuffEncBuildCodes(out_lengths, alphabet_size, out_symbols);
  return GWP_STATUS_OK;
}

static void GWPCountOrWriteCommands(const GWPu8* pixels,
                                    GWPu32 width,
                                    GWPu32 height,
                                    GWPu32 stride,
                                    GWPRawPixelFormat pixel_format,
                                    const GWPVP8LBitstreamOptions* options,
                                    GWPu32* main_freq,
                                    GWPu32 main_alphabet_size,
                                    GWPu32* red_freq,
                                    GWPu32* blue_freq,
                                    GWPu32* alpha_freq,
                                    GWPu32* dist_freq,
                                    const GWPHuffEncSymbol* main_symbols,
                                    const GWPHuffEncSymbol* red_symbols,
                                    const GWPHuffEncSymbol* blue_symbols,
                                    const GWPHuffEncSymbol* alpha_symbols,
                                    const GWPHuffEncSymbol* dist_symbols,
                                    GWPBitWriter* bw,
                                    GWPBool* out_has_alpha,
                                    GWPu32* out_cache_hits,
                                    GWPu32* out_backref_pixels) {
  GWPu32 pos;
  GWPu32 total_pixels;
  GWPu32 cache_bits;
  GWPu32 cache_size;
  GWPu32 cache[GWP_VP8L_MAX_CACHE_SIZE];
  GWPu32 prev_pixel;
  GWPBool have_prev;
  GWPBool has_alpha;
  GWPu32 cache_hits;
  GWPu32 backref_pixels;
  total_pixels = width * height;
  cache_bits = (options != 0 && options->use_color_cache) ? options->color_cache_bits : 0u;
  cache_size = (cache_bits == 0u) ? 0u : (1u << cache_bits);
  GWPColorCacheClear(cache, GWP_VP8L_MAX_CACHE_SIZE);
  have_prev = GWP_FALSE;
  prev_pixel = 0u;
  has_alpha = GWP_FALSE;
  cache_hits = 0u;
  backref_pixels = 0u;

  pos = 0u;
  while (pos < total_pixels) {
    GWPu32 pixel;
    GWPu8 g;
    GWPu8 r;
    GWPu8 b;
    GWPu8 a;
    pixel = GWPProcessPixelAt(pixels, width, stride, pixel_format, options, pos);
    a = (GWPu8)(pixel >> 24);
    r = (GWPu8)(pixel >> 16);
    g = (GWPu8)(pixel >> 8);
    b = (GWPu8)pixel;
    if (a != 255u) has_alpha = GWP_TRUE;

    if (have_prev && options != 0 && options->use_backrefs && pixel == prev_pixel) {
      GWPu32 run;
      run = 1u;
      while (pos + run < total_pixels) {
        GWPu32 next_pixel;
        next_pixel = GWPProcessPixelAt(pixels, width, stride, pixel_format, options, pos + run);
        if (next_pixel != prev_pixel) break;
        ++run;
      }
      if (run >= 3u) {
        GWPu32 prefix;
        GWPu32 extra;
        int extra_bits;
        GWPEncodePrefixValue(run, &prefix, &extra, &extra_bits);
        if (bw == 0) {
          ++main_freq[256u + prefix];
          ++dist_freq[1u];
        } else {
          GWPHuffEncPutSymbol(main_symbols, 256u + prefix, bw);
          if (extra_bits > 0) GWPBitWriterPutBits(bw, extra, extra_bits);
          GWPHuffEncPutSymbol(dist_symbols, 1u, bw);
        }
        if (cache_size != 0u) {
          GWPu32 j;
          for (j = 0u; j < run; ++j) GWPColorCacheInsert(cache, cache_bits, pixel);
        }
        backref_pixels += run;
        prev_pixel = pixel;
        have_prev = GWP_TRUE;
        pos += run;
        continue;
      }
    }

    if (cache_size != 0u) {
      GWPu32 cache_index;
      cache_index = GWPHashColor(pixel, cache_bits);
      if (cache_index < cache_size && cache[cache_index] == pixel) {
        if (bw == 0) {
          if (256u + 24u + cache_index < main_alphabet_size) {
            ++main_freq[256u + 24u + cache_index];
          }
        } else {
          GWPHuffEncPutSymbol(main_symbols, 256u + 24u + cache_index, bw);
        }
        GWPColorCacheInsert(cache, cache_bits, pixel);
        cache_hits += 1u;
        prev_pixel = pixel;
        have_prev = GWP_TRUE;
        ++pos;
        continue;
      }
    }

    if (bw == 0) {
      ++main_freq[g];
      ++red_freq[r];
      ++blue_freq[b];
      ++alpha_freq[a];
    } else {
      GWPHuffEncPutSymbol(main_symbols, g, bw);
      GWPHuffEncPutSymbol(red_symbols, r, bw);
      GWPHuffEncPutSymbol(blue_symbols, b, bw);
      GWPHuffEncPutSymbol(alpha_symbols, a, bw);
    }
    if (cache_size != 0u) GWPColorCacheInsert(cache, cache_bits, pixel);
    prev_pixel = pixel;
    have_prev = GWP_TRUE;
    ++pos;
  }

  if (out_has_alpha != 0) *out_has_alpha = has_alpha;
  if (out_cache_hits != 0) *out_cache_hits = cache_hits;
  if (out_backref_pixels != 0) *out_backref_pixels = backref_pixels;
}

GWPStatusCode GWPVP8LEncodeImageEx(const GWPu8* pixels,
                                   GWPu32 width,
                                   GWPu32 height,
                                   GWPu32 stride,
                                   GWPRawPixelFormat pixel_format,
                                   const GWPVP8LBitstreamOptions* options_in,
                                   GWPu8* out_buf,
                                   GWPu32 out_buf_size,
                                   GWPu32* out_size,
                                   GWPBool* out_has_alpha) {
  GWPVP8LBitstreamOptions options;
  GWPu32 main_freq[GWP_ENC_MAX_ALPHABET];
  GWPu32 red_freq[256];
  GWPu32 blue_freq[256];
  GWPu32 alpha_freq[256];
  GWPu32 dist_freq[40];
  GWPu8 main_lengths[GWP_ENC_MAX_ALPHABET];
  GWPu8 red_lengths[256];
  GWPu8 blue_lengths[256];
  GWPu8 alpha_lengths[256];
  GWPu8 dist_lengths[40];
  GWPHuffEncSymbol main_symbols[GWP_ENC_MAX_ALPHABET];
  GWPHuffEncSymbol red_symbols[256];
  GWPHuffEncSymbol blue_symbols[256];
  GWPHuffEncSymbol alpha_symbols[256];
  GWPHuffEncSymbol dist_symbols[40];
  GWPBitWriter bw;
  GWPu32 x;
  GWPu32 main_alphabet_size;
  GWPu32 cache_hits;
  GWPu32 backref_pixels;
  GWPBool has_alpha;
  GWPStatusCode st;

  if (out_size != 0) *out_size = 0u;
  if (out_has_alpha != 0) *out_has_alpha = GWP_FALSE;
  if (pixels == 0 || out_buf == 0 || out_size == 0) return GWP_STATUS_INVALID_PARAM;
  if (width == 0u || height == 0u) return GWP_STATUS_BAD_DIMENSIONS;
  if (width > GWP_MAX_IMAGE_WIDTH || height > GWP_MAX_IMAGE_HEIGHT) {
    return GWP_STATUS_BAD_DIMENSIONS;
  }
  if (stride < width * 4u) return GWP_STATUS_INVALID_PARAM;
  if (out_buf_size < 6u) return GWP_STATUS_NOT_ENOUGH_OUTPUT;

  GWPVP8LBitstreamOptionsInit(&options);
  if (options_in != 0) options = *options_in;
  if (options.color_cache_bits > GWP_VP8L_CACHE_MAX_BITS) {
    return GWP_STATUS_INVALID_PARAM;
  }
  if (!options.use_color_cache) options.color_cache_bits = 0u;
  main_alphabet_size = GWP_VP8L_MAIN_BASE + ((options.use_color_cache && options.color_cache_bits != 0u) ? (1u << options.color_cache_bits) : 0u);
  if (main_alphabet_size > GWP_ENC_MAX_ALPHABET) return GWP_STATUS_LIMIT_EXCEEDED;

  for (x = 0u; x < main_alphabet_size; ++x) main_freq[x] = 0u;
  for (x = main_alphabet_size; x < GWP_ENC_MAX_ALPHABET; ++x) main_freq[x] = 0u;
  for (x = 0u; x < 256u; ++x) {
    red_freq[x] = 0u;
    blue_freq[x] = 0u;
    alpha_freq[x] = 0u;
  }
  for (x = 0u; x < 40u; ++x) dist_freq[x] = 0u;
  dist_freq[0] = 1u;

  GWPCountOrWriteCommands(pixels, width, height, stride, pixel_format, &options,
                          main_freq, main_alphabet_size,
                          red_freq, blue_freq, alpha_freq, dist_freq,
                          0, 0, 0, 0, 0, 0,
                          &has_alpha, &cache_hits, &backref_pixels);

  if (options.use_color_cache && options.color_cache_bits != 0u && cache_hits == 0u) {
    options.use_color_cache = GWP_FALSE;
    options.color_cache_bits = 0u;
    main_alphabet_size = GWP_VP8L_MAIN_BASE;
    for (x = 0u; x < main_alphabet_size; ++x) main_freq[x] = 0u;
    for (x = 0u; x < 256u; ++x) {
      red_freq[x] = 0u;
      blue_freq[x] = 0u;
      alpha_freq[x] = 0u;
    }
    for (x = 0u; x < 40u; ++x) dist_freq[x] = 0u;
    dist_freq[0] = 1u;
    GWPCountOrWriteCommands(pixels, width, height, stride, pixel_format, &options,
                            main_freq, main_alphabet_size,
                            red_freq, blue_freq, alpha_freq, dist_freq,
                            0, 0, 0, 0, 0, 0,
                            &has_alpha, &cache_hits, &backref_pixels);
  }

  st = GWPBuildTreeFromPixels(main_freq, main_alphabet_size, main_lengths, main_symbols);
  if (st != GWP_STATUS_OK) return st;
  st = GWPBuildTreeFromPixels(red_freq, 256u, red_lengths, red_symbols);
  if (st != GWP_STATUS_OK) return st;
  st = GWPBuildTreeFromPixels(blue_freq, 256u, blue_lengths, blue_symbols);
  if (st != GWP_STATUS_OK) return st;
  st = GWPBuildTreeFromPixels(alpha_freq, 256u, alpha_lengths, alpha_symbols);
  if (st != GWP_STATUS_OK) return st;
  st = GWPBuildTreeFromPixels(dist_freq, 40u, dist_lengths, dist_symbols);
  if (st != GWP_STATUS_OK) return st;

  out_buf[0] = 0x2fu;
  GWPBitWriterInit(&bw, out_buf + 1u, out_buf_size - 1u);
  GWPBitWriterPutBits(&bw, width - 1u, 14);
  GWPBitWriterPutBits(&bw, height - 1u, 14);
  GWPBitWriterPutBits(&bw, has_alpha ? 1u : 0u, 1);
  GWPBitWriterPutBits(&bw, 0u, 3);

  if (options.use_subtract_green) {
    GWPBitWriterPutBits(&bw, 1u, 1);
    GWPBitWriterPutBits(&bw, 2u, 2);
  }
  GWPBitWriterPutBits(&bw, 0u, 1);

  if (options.use_color_cache && options.color_cache_bits != 0u) {
    GWPBitWriterPutBits(&bw, 1u, 1);
    GWPBitWriterPutBits(&bw, options.color_cache_bits, 4);
  } else {
    GWPBitWriterPutBits(&bw, 0u, 1);
  }
  GWPBitWriterPutBits(&bw, 0u, 1);

  st = GWPWriteNormalPrefixCode(main_lengths, main_alphabet_size, &bw);
  if (st != GWP_STATUS_OK) return st;
  st = GWPWriteNormalPrefixCode(red_lengths, 256u, &bw);
  if (st != GWP_STATUS_OK) return st;
  st = GWPWriteNormalPrefixCode(blue_lengths, 256u, &bw);
  if (st != GWP_STATUS_OK) return st;
  st = GWPWriteNormalPrefixCode(alpha_lengths, 256u, &bw);
  if (st != GWP_STATUS_OK) return st;
  st = GWPWriteNormalPrefixCode(dist_lengths, 40u, &bw);
  if (st != GWP_STATUS_OK) return st;

  GWPCountOrWriteCommands(pixels, width, height, stride, pixel_format, &options,
                          0, main_alphabet_size,
                          0, 0, 0, 0,
                          main_symbols, red_symbols, blue_symbols, alpha_symbols,
                          dist_symbols, &bw,
                          0, 0, 0);

  GWPBitWriterFlush(&bw);
  if (!GWPBitWriterOk(&bw)) return GWP_STATUS_NOT_ENOUGH_OUTPUT;
  *out_size = 1u + GWPBitWriterSize(&bw);
  if (out_has_alpha != 0) *out_has_alpha = has_alpha;
  (void)backref_pixels;
  return GWP_STATUS_OK;
}

GWPStatusCode GWPVP8LEncodeImage(const GWPu8* pixels,
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
