#include "vp8l_dec.h"

#include "../utils/common.h"
#include "../utils/endian.h"
#include "../utils/bit_reader.h"
#include "../utils/huffman.h"

#define GWP_VP8L_MAX_PREFIX_GROUPS 512u

#define GWP_ARGB(a, r, g, b) \
  (((GWPu32)(a) << 24) | ((GWPu32)(r) << 16) | ((GWPu32)(g) << 8) | (GWPu32)(b))
#define GWP_ALPHA(c) (((c) >> 24) & 0xffu)
#define GWP_RED(c)   (((c) >> 16) & 0xffu)
#define GWP_GREEN(c) (((c) >> 8) & 0xffu)
#define GWP_BLUE(c)  ((c) & 0xffu)

enum {
  GWP_VP8L_TRANSFORM_PREDICTOR = 0,
  GWP_VP8L_TRANSFORM_COLOR = 1,
  GWP_VP8L_TRANSFORM_SUB_GREEN = 2,
  GWP_VP8L_TRANSFORM_COLOR_INDEX = 3
};

typedef struct GWPVP8LPrefixCode {
  GWPHuffman tree;
} GWPVP8LPrefixCode;

typedef struct GWPVP8LPrefixGroup {
  GWPVP8LPrefixCode green_len_cache;
  GWPVP8LPrefixCode red;
  GWPVP8LPrefixCode blue;
  GWPVP8LPrefixCode alpha;
  GWPVP8LPrefixCode distance;
} GWPVP8LPrefixGroup;

typedef struct GWPVP8LTransform {
  GWPu32 type;
  GWPu32 width_before;
  GWPu32 height_before;
  GWPu32 width_after;
  GWPu32 height_after;
  GWPu32 size_bits;
  GWPu32 data_width;
  GWPu32 data_height;
  GWPu32 width_bits;
  GWPu32 color_table_size;
  GWPu32* pixels;
} GWPVP8LTransform;

typedef struct GWPVP8LHeader {
  GWPu32 width;
  GWPu32 height;
  GWPBool alpha_is_used;
  GWPu32 version;
} GWPVP8LHeader;

static GWPu32 GWPRoundUpDiv(GWPu32 n, GWPu32 d) {
  return (n + d - 1u) / d;
}

static int GWPToInt8(GWPu32 v) {
  if (v < 128u) return (int)v;
  return (int)v - 256;
}

static GWPu8 GWPClamp255(int v) {
  if (v < 0) return 0u;
  if (v > 255) return 255u;
  return (GWPu8)v;
}

static GWPu32 GWPAverage2(GWPu32 a, GWPu32 b) {
  return GWP_ARGB((GWP_ALPHA(a) + GWP_ALPHA(b)) >> 1,
                  (GWP_RED(a) + GWP_RED(b)) >> 1,
                  (GWP_GREEN(a) + GWP_GREEN(b)) >> 1,
                  (GWP_BLUE(a) + GWP_BLUE(b)) >> 1);
}

static GWPu32 GWPSelectPredictor(GWPu32 l, GWPu32 t, GWPu32 tl) {
  int p_alpha;
  int p_red;
  int p_green;
  int p_blue;
  int dl;
  int dt;
  p_alpha = (int)GWP_ALPHA(l) + (int)GWP_ALPHA(t) - (int)GWP_ALPHA(tl);
  p_red = (int)GWP_RED(l) + (int)GWP_RED(t) - (int)GWP_RED(tl);
  p_green = (int)GWP_GREEN(l) + (int)GWP_GREEN(t) - (int)GWP_GREEN(tl);
  p_blue = (int)GWP_BLUE(l) + (int)GWP_BLUE(t) - (int)GWP_BLUE(tl);

  dl = p_alpha - (int)GWP_ALPHA(l);
  if (dl < 0) dl = -dl;
  dl += (p_red - (int)GWP_RED(l) < 0) ? -(p_red - (int)GWP_RED(l)) : (p_red - (int)GWP_RED(l));
  dl += (p_green - (int)GWP_GREEN(l) < 0) ? -(p_green - (int)GWP_GREEN(l)) : (p_green - (int)GWP_GREEN(l));
  dl += (p_blue - (int)GWP_BLUE(l) < 0) ? -(p_blue - (int)GWP_BLUE(l)) : (p_blue - (int)GWP_BLUE(l));

  dt = p_alpha - (int)GWP_ALPHA(t);
  if (dt < 0) dt = -dt;
  dt += (p_red - (int)GWP_RED(t) < 0) ? -(p_red - (int)GWP_RED(t)) : (p_red - (int)GWP_RED(t));
  dt += (p_green - (int)GWP_GREEN(t) < 0) ? -(p_green - (int)GWP_GREEN(t)) : (p_green - (int)GWP_GREEN(t));
  dt += (p_blue - (int)GWP_BLUE(t) < 0) ? -(p_blue - (int)GWP_BLUE(t)) : (p_blue - (int)GWP_BLUE(t));

  return (dl < dt) ? l : t;
}

static GWPu8 GWPClampAddSubtractFull8(GWPu8 a, GWPu8 b, GWPu8 c) {
  return GWPClamp255((int)a + (int)b - (int)c);
}

static GWPu8 GWPClampAddSubtractHalf8(GWPu8 a, GWPu8 b) {
  return GWPClamp255((int)a + (((int)a - (int)b) >> 1));
}

static GWPu32 GWPClampAddSubtractFull(GWPu32 a, GWPu32 b, GWPu32 c) {
  return GWP_ARGB(GWPClampAddSubtractFull8((GWPu8)GWP_ALPHA(a), (GWPu8)GWP_ALPHA(b), (GWPu8)GWP_ALPHA(c)),
                  GWPClampAddSubtractFull8((GWPu8)GWP_RED(a), (GWPu8)GWP_RED(b), (GWPu8)GWP_RED(c)),
                  GWPClampAddSubtractFull8((GWPu8)GWP_GREEN(a), (GWPu8)GWP_GREEN(b), (GWPu8)GWP_GREEN(c)),
                  GWPClampAddSubtractFull8((GWPu8)GWP_BLUE(a), (GWPu8)GWP_BLUE(b), (GWPu8)GWP_BLUE(c)));
}

static GWPu32 GWPClampAddSubtractHalf(GWPu32 a, GWPu32 b) {
  return GWP_ARGB(GWPClampAddSubtractHalf8((GWPu8)GWP_ALPHA(a), (GWPu8)GWP_ALPHA(b)),
                  GWPClampAddSubtractHalf8((GWPu8)GWP_RED(a), (GWPu8)GWP_RED(b)),
                  GWPClampAddSubtractHalf8((GWPu8)GWP_GREEN(a), (GWPu8)GWP_GREEN(b)),
                  GWPClampAddSubtractHalf8((GWPu8)GWP_BLUE(a), (GWPu8)GWP_BLUE(b)));
}

static GWPu32 GWPVP8LPredictPixel(GWPu32 mode,
                                  GWPu32 l,
                                  GWPu32 t,
                                  GWPu32 tl,
                                  GWPu32 tr) {
  switch (mode) {
    case 0u: return 0xff000000u;
    case 1u: return l;
    case 2u: return t;
    case 3u: return tr;
    case 4u: return tl;
    case 5u: return GWPAverage2(GWPAverage2(l, tr), t);
    case 6u: return GWPAverage2(l, tl);
    case 7u: return GWPAverage2(l, t);
    case 8u: return GWPAverage2(tl, t);
    case 9u: return GWPAverage2(t, tr);
    case 10u: return GWPAverage2(GWPAverage2(l, tl), GWPAverage2(t, tr));
    case 11u: return GWPSelectPredictor(l, t, tl);
    case 12u: return GWPClampAddSubtractFull(l, t, tl);
    case 13u: return GWPClampAddSubtractHalf(GWPAverage2(l, t), tl);
    default: return 0xff000000u;
  }
}

static GWPu32 GWPAddPixelsMod256(GWPu32 residual, GWPu32 pred) {
  return GWP_ARGB(((GWP_ALPHA(residual) + GWP_ALPHA(pred)) & 0xffu),
                  ((GWP_RED(residual) + GWP_RED(pred)) & 0xffu),
                  ((GWP_GREEN(residual) + GWP_GREEN(pred)) & 0xffu),
                  ((GWP_BLUE(residual) + GWP_BLUE(pred)) & 0xffu));
}

static int GWPColorTransformDelta(int t, int c) {
  return (t * c) >> 5;
}

static GWPStatusCode GWPVP8LReadPrefixCode(GWPBitReader* br,
                                           GWPu32 alphabet_size,
                                           GWPArena* arena,
                                           GWPVP8LPrefixCode* out_code);

static GWPStatusCode GWPVP8LDecodeImageData(GWPBitReader* br,
                                            GWPu32 width,
                                            GWPu32 height,
                                            GWPBool allow_meta_prefix,
                                            GWPArena* arena,
                                            GWPu32** out_pixels);

static GWPStatusCode GWPVP8LReadHeader(const GWPu8* data,
                                       GWPu32 data_size,
                                       GWPBitReader* br,
                                       GWPVP8LHeader* header) {
  GWPu32 bits;
  if (data == 0 || br == 0 || header == 0) return GWP_STATUS_INVALID_PARAM;
  if (data_size < 5u) return GWP_STATUS_TRUNCATED_DATA;
  if (data[0] != 0x2fu) return GWP_STATUS_BITSTREAM_ERROR;
  GWPBitReaderInit(br, data + 1, data_size - 1u);
  if (!GWPBitReaderGetBits(br, 14, &header->width)) return GWP_STATUS_TRUNCATED_DATA;
  if (!GWPBitReaderGetBits(br, 14, &header->height)) return GWP_STATUS_TRUNCATED_DATA;
  header->width += 1u;
  header->height += 1u;
  if (!GWPBitReaderGetBit(br, &bits)) return GWP_STATUS_TRUNCATED_DATA;
  header->alpha_is_used = bits ? GWP_TRUE : GWP_FALSE;
  if (!GWPBitReaderGetBits(br, 3, &header->version)) return GWP_STATUS_TRUNCATED_DATA;
  if (header->version != 0u) return GWP_STATUS_BITSTREAM_ERROR;
  if (header->width == 0u || header->height == 0u) return GWP_STATUS_BAD_DIMENSIONS;
  return GWP_STATUS_OK;
}

static GWPu32 GWPVP8LDecodePrefixValue(GWPu32 prefix_code, GWPBitReader* br, GWPStatusCode* st) {
  if (prefix_code < 4u) return prefix_code + 1u;
  {
    GWPu32 extra_bits;
    GWPu32 offset;
    GWPu32 extra;
    extra_bits = (prefix_code - 2u) >> 1;
    if (!GWPBitReaderGetBits(br, (int)extra_bits, &extra)) {
      if (st != 0) *st = GWP_STATUS_TRUNCATED_DATA;
      return 0u;
    }
    offset = (2u + (prefix_code & 1u)) << extra_bits;
    return offset + extra + 1u;
  }
}

static const signed char kGWPDistanceMap[120][2] = {
  { 0, 1 }, { 1, 0 }, { 1, 1 }, { -1, 1 }, { 0, 2 }, { 2, 0 }, { 1, 2 }, { -1, 2 },
  { 2, 1 }, { -2, 1 }, { 2, 2 }, { -2, 2 }, { 0, 3 }, { 3, 0 }, { 1, 3 }, { -1, 3 },
  { 3, 1 }, { -3, 1 }, { 2, 3 }, { -2, 3 }, { 3, 2 }, { -3, 2 }, { 0, 4 }, { 4, 0 },
  { 1, 4 }, { -1, 4 }, { 4, 1 }, { -4, 1 }, { 3, 3 }, { -3, 3 }, { 2, 4 }, { -2, 4 },
  { 4, 2 }, { -4, 2 }, { 0, 5 }, { 3, 4 }, { -3, 4 }, { 4, 3 }, { -4, 3 }, { 5, 0 },
  { 1, 5 }, { -1, 5 }, { 5, 1 }, { -5, 1 }, { 2, 5 }, { -2, 5 }, { 5, 2 }, { -5, 2 },
  { 4, 4 }, { -4, 4 }, { 3, 5 }, { -3, 5 }, { 5, 3 }, { -5, 3 }, { 0, 6 }, { 6, 0 },
  { 1, 6 }, { -1, 6 }, { 6, 1 }, { -6, 1 }, { 2, 6 }, { -2, 6 }, { 6, 2 }, { -6, 2 },
  { 4, 5 }, { -4, 5 }, { 5, 4 }, { -5, 4 }, { 3, 6 }, { -3, 6 }, { 6, 3 }, { -6, 3 },
  { 0, 7 }, { 7, 0 }, { 1, 7 }, { -1, 7 }, { 5, 5 }, { -5, 5 }, { 7, 1 }, { -7, 1 },
  { 4, 6 }, { -4, 6 }, { 6, 4 }, { -6, 4 }, { 2, 7 }, { -2, 7 }, { 7, 2 }, { -7, 2 },
  { 3, 7 }, { -3, 7 }, { 7, 3 }, { -7, 3 }, { 5, 6 }, { -5, 6 }, { 6, 5 }, { -6, 5 },
  { 8, 0 }, { 4, 7 }, { -4, 7 }, { 7, 4 }, { -7, 4 }, { 8, 1 }, { 8, 2 }, { 6, 6 },
  { -6, 6 }, { 8, 3 }, { 5, 7 }, { -5, 7 }, { 7, 5 }, { -7, 5 }, { 8, 4 }, { 6, 7 },
  { -6, 7 }, { 7, 6 }, { -7, 6 }, { 8, 5 }, { 7, 7 }, { -7, 7 }, { 8, 6 }, { 8, 7 }
};

static GWPu32 GWPVP8LDistanceToPixelDistance(GWPu32 distance_code, GWPu32 image_width) {
  if (distance_code <= 120u) {
    int xi;
    int yi;
    int dist;
    xi = (int)kGWPDistanceMap[distance_code - 1u][0];
    yi = (int)kGWPDistanceMap[distance_code - 1u][1];
    dist = xi + yi * (int)image_width;
    if (dist < 1) dist = 1;
    return (GWPu32)dist;
  }
  return distance_code - 120u;
}

static GWPu32 GWPVP8LColorCacheIndex(GWPu32 color, GWPu32 bits) {
  return (GWPu32)((0x1e35a7bdu * color) >> (32u - bits));
}

static void GWPVP8LColorCacheInsert(GWPu32* cache, GWPu32 bits, GWPu32 color) {
  cache[GWPVP8LColorCacheIndex(color, bits)] = color;
}

static GWPStatusCode GWPVP8LReadPrefixCodeSimple(GWPBitReader* br,
                                                 GWPu32 alphabet_size,
                                                 GWPu8* lengths) {
  GWPu32 num_symbols;
  GWPu32 is_first_8bits;
  GWPu32 symbol0;
  GWPu32 symbol1;
  if (!GWPBitReaderGetBit(br, &num_symbols)) return GWP_STATUS_TRUNCATED_DATA;
  num_symbols += 1u;
  if (!GWPBitReaderGetBit(br, &is_first_8bits)) return GWP_STATUS_TRUNCATED_DATA;
  if (!GWPBitReaderGetBits(br, 1 + 7 * (int)is_first_8bits, &symbol0)) return GWP_STATUS_TRUNCATED_DATA;
  if (symbol0 >= alphabet_size) return GWP_STATUS_BITSTREAM_ERROR;
  lengths[symbol0] = 1u;
  if (num_symbols == 2u) {
    if (!GWPBitReaderGetBits(br, 8, &symbol1)) return GWP_STATUS_TRUNCATED_DATA;
    if (symbol1 >= alphabet_size) return GWP_STATUS_BITSTREAM_ERROR;
    lengths[symbol1] = 1u;
  }
  return GWP_STATUS_OK;
}

static GWPStatusCode GWPVP8LReadNormalCodeLengths(GWPBitReader* br,
                                                  GWPu32 alphabet_size,
                                                  GWPu8* lengths) {
  static const GWPu8 kOrder[19] = {
    17, 18, 0, 1, 2, 3, 4, 5, 16, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
  };
  GWPu8 code_length_code_lengths[19];
  GWPHuffCode temp_table[1u << GWP_HUFFMAN_ROOT_BITS];
  GWPHuffman temp_tree;
  GWPu32 num_code_lengths;
  GWPu32 i;
  GWPu32 use_length_limit;
  GWPu32 max_symbol;
  int prev_length;
  for (i = 0u; i < 19u; ++i) code_length_code_lengths[i] = 0u;
  if (!GWPBitReaderGetBits(br, 4, &num_code_lengths)) return GWP_STATUS_TRUNCATED_DATA;
  num_code_lengths += 4u;
  if (num_code_lengths > 19u) return GWP_STATUS_BITSTREAM_ERROR;
  for (i = 0u; i < num_code_lengths; ++i) {
    GWPu32 v;
    if (!GWPBitReaderGetBits(br, 3, &v)) return GWP_STATUS_TRUNCATED_DATA;
    code_length_code_lengths[kOrder[i]] = (GWPu8)v;
  }
  GWPHuffmanInit(&temp_tree, temp_table, GWP_ARRAY_SIZE(temp_table),
                 GWP_HUFFMAN_ROOT_BITS);
  {
    GWPStatusCode st;
    st = GWPHuffmanBuild(&temp_tree, code_length_code_lengths, 19u);
    if (st != GWP_STATUS_OK) return st;
  }

  if (!GWPBitReaderGetBit(br, &use_length_limit)) return GWP_STATUS_TRUNCATED_DATA;
  if (use_length_limit == 0u) {
    max_symbol = alphabet_size;
  } else {
    GWPu32 length_nbits;
    if (!GWPBitReaderGetBits(br, 3, &length_nbits)) return GWP_STATUS_TRUNCATED_DATA;
    length_nbits = 2u + 2u * length_nbits;
    if (!GWPBitReaderGetBits(br, (int)length_nbits, &max_symbol)) return GWP_STATUS_TRUNCATED_DATA;
    max_symbol += 2u;
    if (max_symbol > alphabet_size) return GWP_STATUS_BITSTREAM_ERROR;
  }

  prev_length = 8;
  i = 0u;
  while (i < alphabet_size) {
    GWPu32 sym;
    GWPStatusCode st;
    if (max_symbol == 0u) break;
    --max_symbol;
    st = GWPHuffmanReadSymbol(&temp_tree, br, &sym);
    if (st != GWP_STATUS_OK) return st;
    if (sym <= 15u) {
      lengths[i++] = (GWPu8)sym;
      if (sym != 0u) prev_length = (int)sym;
    } else if (sym == 16u) {
      GWPu32 repeat;
      GWPu32 j;
      if (!GWPBitReaderGetBits(br, 2, &repeat)) return GWP_STATUS_TRUNCATED_DATA;
      repeat += 3u;
      if (i + repeat > alphabet_size) return GWP_STATUS_BITSTREAM_ERROR;
      for (j = 0u; j < repeat; ++j) lengths[i++] = (GWPu8)prev_length;
    } else if (sym == 17u) {
      GWPu32 repeat;
      GWPu32 j;
      if (!GWPBitReaderGetBits(br, 3, &repeat)) return GWP_STATUS_TRUNCATED_DATA;
      repeat += 3u;
      if (i + repeat > alphabet_size) return GWP_STATUS_BITSTREAM_ERROR;
      for (j = 0u; j < repeat; ++j) lengths[i++] = 0u;
    } else if (sym == 18u) {
      GWPu32 repeat;
      GWPu32 j;
      if (!GWPBitReaderGetBits(br, 7, &repeat)) return GWP_STATUS_TRUNCATED_DATA;
      repeat += 11u;
      if (i + repeat > alphabet_size) return GWP_STATUS_BITSTREAM_ERROR;
      for (j = 0u; j < repeat; ++j) lengths[i++] = 0u;
    } else {
      return GWP_STATUS_BITSTREAM_ERROR;
    }
  }

  for (; i < alphabet_size; ++i) lengths[i] = 0u;
  return GWP_STATUS_OK;
}

static GWPStatusCode GWPVP8LReadPrefixCode(GWPBitReader* br,
                                           GWPu32 alphabet_size,
                                           GWPArena* arena,
                                           GWPVP8LPrefixCode* out_code) {
  GWPu8 lengths[GWP_VP8L_MAX_GREEN_ALPHABET];
  GWPu32 use_simple;
  GWPStatusCode st;
  GWPu32 i;
  if (br == 0 || arena == 0 || out_code == 0) return GWP_STATUS_INVALID_PARAM;
  if (alphabet_size > GWP_VP8L_MAX_GREEN_ALPHABET) return GWP_STATUS_LIMIT_EXCEEDED;
  for (i = 0u; i < alphabet_size; ++i) lengths[i] = 0u;
  if (!GWPBitReaderGetBit(br, &use_simple)) return GWP_STATUS_TRUNCATED_DATA;
  if (use_simple) {
    st = GWPVP8LReadPrefixCodeSimple(br, alphabet_size, lengths);
  } else {
    st = GWPVP8LReadNormalCodeLengths(br, alphabet_size, lengths);
  }
  if (st != GWP_STATUS_OK) return st;

  st = GWPHuffmanBuildArena(&out_code->tree, arena, lengths, alphabet_size,
                              GWP_HUFFMAN_ROOT_BITS);
  return st;
}

static GWPStatusCode GWPVP8LReadPrefixGroup(GWPBitReader* br,
                                            GWPu32 color_cache_size,
                                            GWPArena* arena,
                                            GWPVP8LPrefixGroup* group) {
  GWPStatusCode st;
  st = GWPVP8LReadPrefixCode(br, 256u + 24u + color_cache_size, arena, &group->green_len_cache);
  if (st != GWP_STATUS_OK) return st;
  st = GWPVP8LReadPrefixCode(br, 256u, arena, &group->red);
  if (st != GWP_STATUS_OK) return st;
  st = GWPVP8LReadPrefixCode(br, 256u, arena, &group->blue);
  if (st != GWP_STATUS_OK) return st;
  st = GWPVP8LReadPrefixCode(br, 256u, arena, &group->alpha);
  if (st != GWP_STATUS_OK) return st;
  st = GWPVP8LReadPrefixCode(br, 40u, arena, &group->distance);
  return st;
}

static GWPStatusCode GWPVP8LReadColorCacheInfo(GWPBitReader* br,
                                               GWPu32* out_bits,
                                               GWPu32* out_size) {
  GWPu32 use_cache;
  if (br == 0 || out_bits == 0 || out_size == 0) return GWP_STATUS_INVALID_PARAM;
  if (!GWPBitReaderGetBit(br, &use_cache)) return GWP_STATUS_TRUNCATED_DATA;
  if (!use_cache) {
    *out_bits = 0u;
    *out_size = 0u;
    return GWP_STATUS_OK;
  }
  if (!GWPBitReaderGetBits(br, 4, out_bits)) return GWP_STATUS_TRUNCATED_DATA;
  if (*out_bits < 1u || *out_bits > GWP_VP8L_CACHE_MAX_BITS) return GWP_STATUS_BITSTREAM_ERROR;
  *out_size = 1u << *out_bits;
  return GWP_STATUS_OK;
}

static GWPStatusCode GWPVP8LDecodeImageData(GWPBitReader* br,
                                            GWPu32 width,
                                            GWPu32 height,
                                            GWPBool allow_meta_prefix,
                                            GWPArena* arena,
                                            GWPu32** out_pixels) {
  GWPu32 total_pixels;
  GWPu32* pixels;
  GWPu32 color_cache_bits;
  GWPu32 color_cache_size;
  GWPu32* color_cache;
  GWPu32 meta_prefix_flag;
  GWPu32 prefix_bits;
  GWPu32 entropy_width;
  GWPu32 entropy_height;
  GWPu32* entropy_image;
  GWPu32 num_prefix_groups;
  GWPVP8LPrefixGroup* groups;
  GWPStatusCode st;
  GWPu32 pos;

  if (br == 0 || arena == 0 || out_pixels == 0) return GWP_STATUS_INVALID_PARAM;
  if (!GWPMulU32(width, height, &total_pixels)) return GWP_STATUS_LIMIT_EXCEEDED;
  pixels = (GWPu32*)GWPArenaAlloc(arena, total_pixels * (GWPu32)sizeof(GWPu32), 4u);
  if (pixels == 0) return GWP_STATUS_NOT_ENOUGH_SCRATCH;

  st = GWPVP8LReadColorCacheInfo(br, &color_cache_bits, &color_cache_size);
  if (st != GWP_STATUS_OK) return st;

  color_cache = 0;
  if (color_cache_size != 0u) {
    color_cache = (GWPu32*)GWPArenaAlloc(arena, color_cache_size * (GWPu32)sizeof(GWPu32), 4u);
    if (color_cache == 0) return GWP_STATUS_NOT_ENOUGH_SCRATCH;
    GWPZero(color_cache, color_cache_size * (GWPu32)sizeof(GWPu32));
  }

  meta_prefix_flag = 0u;
  prefix_bits = 0u;
  entropy_width = 0u;
  entropy_height = 0u;
  entropy_image = 0;
  num_prefix_groups = 1u;

  if (allow_meta_prefix) {
    if (!GWPBitReaderGetBit(br, &meta_prefix_flag)) return GWP_STATUS_TRUNCATED_DATA;
    if (meta_prefix_flag) {
      GWPu32 i;
      GWPu32 max_meta;
      if (!GWPBitReaderGetBits(br, 3, &prefix_bits)) return GWP_STATUS_TRUNCATED_DATA;
      prefix_bits += 2u;
      entropy_width = GWPRoundUpDiv(width, 1u << prefix_bits);
      entropy_height = GWPRoundUpDiv(height, 1u << prefix_bits);
      st = GWPVP8LDecodeImageData(br, entropy_width, entropy_height, GWP_FALSE, arena, &entropy_image);
      max_meta = 0u;
      for (i = 0u; i < entropy_width * entropy_height; ++i) {
        GWPu32 meta_code;
        meta_code = (entropy_image[i] >> 8) & 0xffffu;
        if (meta_code > max_meta) max_meta = meta_code;
      }
      num_prefix_groups = max_meta + 1u;
      if (num_prefix_groups == 0u || num_prefix_groups > GWP_VP8L_MAX_PREFIX_GROUPS) {
        return GWP_STATUS_LIMIT_EXCEEDED;
      }
    }
  }

  groups = (GWPVP8LPrefixGroup*)GWPArenaAlloc(arena,
                                              num_prefix_groups * (GWPu32)sizeof(GWPVP8LPrefixGroup),
                                              4u);
  if (groups == 0) return GWP_STATUS_NOT_ENOUGH_SCRATCH;
  GWPZero(groups, num_prefix_groups * (GWPu32)sizeof(GWPVP8LPrefixGroup));

  {
    GWPu32 g;
    for (g = 0u; g < num_prefix_groups; ++g) {
      st = GWPVP8LReadPrefixGroup(br, color_cache_size, arena, &groups[g]);
      if (st != GWP_STATUS_OK) return st;
    }
  }

  pos = 0u;
  while (pos < total_pixels) {
    GWPVP8LPrefixGroup* group;
    GWPu32 x;
    GWPu32 y;
    GWPu32 sym;
    if (num_prefix_groups == 1u) {
      group = &groups[0];
    } else {
      GWPu32 entropy_pos;
      GWPu32 meta_code;
      x = pos % width;
      y = pos / width;
      entropy_pos = (y >> prefix_bits) * entropy_width + (x >> prefix_bits);
      meta_code = (entropy_image[entropy_pos] >> 8) & 0xffffu;
      if (meta_code >= num_prefix_groups) return GWP_STATUS_BITSTREAM_ERROR;
      group = &groups[meta_code];
    }

    st = GWPHuffmanReadSymbol(&group->green_len_cache.tree, br, &sym);
    if (st != GWP_STATUS_OK) return st;

    if (sym < 256u) {
      GWPu32 red;
      GWPu32 blue;
      GWPu32 alpha;
      GWPu32 pixel;
      st = GWPHuffmanReadSymbol(&group->red.tree, br, &red);
      if (st != GWP_STATUS_OK) return st;
      st = GWPHuffmanReadSymbol(&group->blue.tree, br, &blue);
      if (st != GWP_STATUS_OK) return st;
      st = GWPHuffmanReadSymbol(&group->alpha.tree, br, &alpha);
      if (st != GWP_STATUS_OK) return st;
      pixel = GWP_ARGB(alpha, red, sym, blue);
      pixels[pos++] = pixel;
      if (color_cache_size != 0u) GWPVP8LColorCacheInsert(color_cache, color_cache_bits, pixel);
    } else if (sym < 256u + 24u) {
      GWPu32 length_code;
      GWPu32 length;
      GWPu32 dist_prefix;
      GWPu32 distance_code;
      GWPu32 dist;
      GWPu32 j;
      length_code = sym - 256u;
      st = GWP_STATUS_OK;
      length = GWPVP8LDecodePrefixValue(length_code, br, &st);
      if (st != GWP_STATUS_OK) return st;
      st = GWPHuffmanReadSymbol(&group->distance.tree, br, &dist_prefix);
      if (st != GWP_STATUS_OK) return st;
      distance_code = GWPVP8LDecodePrefixValue(dist_prefix, br, &st);
      if (st != GWP_STATUS_OK) return st;
      dist = GWPVP8LDistanceToPixelDistance(distance_code, width);
      if (dist == 0u || dist > pos) return GWP_STATUS_BITSTREAM_ERROR;
      if (length > total_pixels - pos) return GWP_STATUS_BITSTREAM_ERROR;
      for (j = 0u; j < length; ++j) {
        GWPu32 pixel;
        pixel = pixels[pos - dist];
        pixels[pos++] = pixel;
        if (color_cache_size != 0u) GWPVP8LColorCacheInsert(color_cache, color_cache_bits, pixel);
      }
    } else {
      GWPu32 cache_index;
      GWPu32 pixel;
      if (color_cache_size == 0u) return GWP_STATUS_BITSTREAM_ERROR;
      cache_index = sym - (256u + 24u);
      if (cache_index >= color_cache_size) return GWP_STATUS_BITSTREAM_ERROR;
      pixel = color_cache[cache_index];
      pixels[pos++] = pixel;
      if (color_cache_size != 0u) GWPVP8LColorCacheInsert(color_cache, color_cache_bits, pixel);
    }
  }

  *out_pixels = pixels;
  return GWP_STATUS_OK;
}

static void GWPVP8LApplySubtractGreen(GWPu32* pixels, GWPu32 count) {
  GWPu32 i;
  for (i = 0u; i < count; ++i) {
    GWPu32 a;
    GWPu32 r;
    GWPu32 g;
    GWPu32 b;
    a = GWP_ALPHA(pixels[i]);
    r = (GWP_RED(pixels[i]) + GWP_GREEN(pixels[i])) & 0xffu;
    g = GWP_GREEN(pixels[i]);
    b = (GWP_BLUE(pixels[i]) + GWP_GREEN(pixels[i])) & 0xffu;
    pixels[i] = GWP_ARGB(a, r, g, b);
  }
}

static void GWPVP8LFinalizeColorTable(GWPu32* table, GWPu32 size) {
  GWPu32 i;
  GWPu32 prev;
  prev = 0u;
  for (i = 0u; i < size; ++i) {
    GWPu32 a;
    GWPu32 r;
    GWPu32 g;
    GWPu32 b;
    a = (GWP_ALPHA(table[i]) + GWP_ALPHA(prev)) & 0xffu;
    r = (GWP_RED(table[i]) + GWP_RED(prev)) & 0xffu;
    g = (GWP_GREEN(table[i]) + GWP_GREEN(prev)) & 0xffu;
    b = (GWP_BLUE(table[i]) + GWP_BLUE(prev)) & 0xffu;
    table[i] = GWP_ARGB(a, r, g, b);
    prev = table[i];
  }
}

static void GWPVP8LApplyColorTransform(const GWPVP8LTransform* tx,
                                       GWPu32* pixels,
                                       GWPu32 width,
                                       GWPu32 height) {
  GWPu32 y;
  for (y = 0u; y < height; ++y) {
    GWPu32 x;
    for (x = 0u; x < width; ++x) {
      GWPu32 block_index;
      GWPu32 cte;
      int green_to_red;
      int green_to_blue;
      int red_to_blue;
      int tmp_red;
      int tmp_blue;
      GWPu32 pixel;
      GWPu32 red;
      GWPu32 green;
      GWPu32 blue;
      GWPu32 alpha;
      block_index = (y >> tx->size_bits) * tx->data_width + (x >> tx->size_bits);
      cte = tx->pixels[block_index];
      green_to_red = GWPToInt8(GWP_BLUE(cte));
      green_to_blue = GWPToInt8(GWP_GREEN(cte));
      red_to_blue = GWPToInt8(GWP_RED(cte));

      pixel = pixels[y * width + x];
      alpha = GWP_ALPHA(pixel);
      red = GWP_RED(pixel);
      green = GWP_GREEN(pixel);
      blue = GWP_BLUE(pixel);

      tmp_red = (int)red;
      tmp_blue = (int)blue;
      tmp_red += GWPColorTransformDelta(green_to_red, GWPToInt8(green));
      tmp_blue += GWPColorTransformDelta(green_to_blue, GWPToInt8(green));
      tmp_blue += GWPColorTransformDelta(red_to_blue, GWPToInt8((GWPu32)(tmp_red & 0xff)));

      pixels[y * width + x] = GWP_ARGB(alpha, (GWPu32)(tmp_red & 0xff), green, (GWPu32)(tmp_blue & 0xff));
    }
  }
}

static void GWPVP8LApplyPredictorTransform(const GWPVP8LTransform* tx,
                                           GWPu32* pixels,
                                           GWPu32 width,
                                           GWPu32 height) {
  GWPu32 y;
  for (y = 0u; y < height; ++y) {
    GWPu32 x;
    for (x = 0u; x < width; ++x) {
      GWPu32 mode;
      GWPu32 pred;
      GWPu32 residual;
      GWPu32 block_index;
      GWPu32 l;
      GWPu32 t;
      GWPu32 tl;
      GWPu32 tr;

      block_index = (y >> tx->size_bits) * tx->data_width + (x >> tx->size_bits);
      mode = GWP_GREEN(tx->pixels[block_index]) & 0xffu;
      residual = pixels[y * width + x];

      if (x == 0u && y == 0u) {
        pred = 0xff000000u;
      } else if (y == 0u) {
        pred = pixels[y * width + (x - 1u)];
      } else if (x == 0u) {
        pred = pixels[(y - 1u) * width + x];
      } else {
        l = pixels[y * width + (x - 1u)];
        t = pixels[(y - 1u) * width + x];
        tl = pixels[(y - 1u) * width + (x - 1u)];
        if (x + 1u < width) {
          tr = pixels[(y - 1u) * width + (x + 1u)];
        } else {
          tr = pixels[y * width + 0u];
        }
        pred = GWPVP8LPredictPixel(mode, l, t, tl, tr);
      }
      pixels[y * width + x] = GWPAddPixelsMod256(residual, pred);
    }
  }
}

static GWPStatusCode GWPVP8LApplyColorIndexTransform(const GWPVP8LTransform* tx,
                                                     GWPArena* arena,
                                                     GWPu32** io_pixels,
                                                     GWPu32* io_width,
                                                     GWPu32* io_height) {
  GWPu32 old_width;
  GWPu32 old_height;
  GWPu32 total_pixels;
  GWPu32* out;
  GWPu32 bits_per_index;
  GWPu32 mask;
  GWPu32 x;
  GWPu32 y;

  if (tx == 0 || arena == 0 || io_pixels == 0 || *io_pixels == 0 ||
      io_width == 0 || io_height == 0) {
    return GWP_STATUS_INVALID_PARAM;
  }

  old_width = tx->width_before;
  old_height = tx->height_before;
  if (!GWPMulU32(old_width, old_height, &total_pixels)) return GWP_STATUS_LIMIT_EXCEEDED;
  out = (GWPu32*)GWPArenaAlloc(arena, total_pixels * (GWPu32)sizeof(GWPu32), 4u);
  if (out == 0) return GWP_STATUS_NOT_ENOUGH_SCRATCH;

  bits_per_index = (tx->width_bits == 0u) ? 8u : (8u >> tx->width_bits);
  mask = (1u << bits_per_index) - 1u;

  for (y = 0u; y < old_height; ++y) {
    for (x = 0u; x < old_width; ++x) {
      GWPu32 src_index;
      GWPu32 packed;
      GWPu32 shift;
      GWPu32 palette_index;
      src_index = y * (*io_width) + (x >> tx->width_bits);
      packed = GWP_GREEN((*io_pixels)[src_index]);
      shift = (x & ((1u << tx->width_bits) - 1u)) * bits_per_index;
      palette_index = (packed >> shift) & mask;
      if (palette_index >= tx->color_table_size) {
        out[y * old_width + x] = 0u;
      } else {
        out[y * old_width + x] = tx->pixels[palette_index];
      }
    }
  }

  *io_pixels = out;
  *io_width = old_width;
  *io_height = old_height;
  return GWP_STATUS_OK;
}

static GWPStatusCode GWPVP8LParseTransforms(GWPBitReader* br,
                                            GWPArena* arena,
                                            GWPu32 start_width,
                                            GWPu32 start_height,
                                            GWPVP8LTransform* transforms,
                                            GWPu32* out_num_transforms,
                                            GWPu32* out_work_width,
                                            GWPu32* out_work_height) {
  GWPBool used_transform[4];
  GWPu32 current_width;
  GWPu32 current_height;
  GWPu32 num_transforms;
  GWPu32 present;
  GWPStatusCode st;

  used_transform[0] = GWP_FALSE;
  used_transform[1] = GWP_FALSE;
  used_transform[2] = GWP_FALSE;
  used_transform[3] = GWP_FALSE;
  current_width = start_width;
  current_height = start_height;
  num_transforms = 0u;

  for (;;) {
    if (!GWPBitReaderGetBit(br, &present)) return GWP_STATUS_TRUNCATED_DATA;
    if (!present) break;

    if (num_transforms >= GWP_VP8L_MAX_TRANSFORMS) return GWP_STATUS_BITSTREAM_ERROR;
    {
      GWPu32 type;
      GWPVP8LTransform* tx;
      if (!GWPBitReaderGetBits(br, 2, &type)) return GWP_STATUS_TRUNCATED_DATA;
      if (type > 3u) return GWP_STATUS_BITSTREAM_ERROR;
      if (used_transform[type]) return GWP_STATUS_BITSTREAM_ERROR;
      used_transform[type] = GWP_TRUE;
      tx = &transforms[num_transforms];
      GWPZero(tx, (GWPu32)sizeof(*tx));
      tx->type = type;
      tx->width_before = current_width;
      tx->height_before = current_height;

      if (type == GWP_VP8L_TRANSFORM_PREDICTOR || type == GWP_VP8L_TRANSFORM_COLOR) {
        GWPu32 size_bits;
        GWPu32 block_size;
        if (!GWPBitReaderGetBits(br, 3, &size_bits)) return GWP_STATUS_TRUNCATED_DATA;
        size_bits += 2u;
        block_size = 1u << size_bits;
        tx->size_bits = size_bits;
        tx->data_width = GWPRoundUpDiv(current_width, block_size);
        tx->data_height = GWPRoundUpDiv(current_height, block_size);
        st = GWPVP8LDecodeImageData(br, tx->data_width, tx->data_height, GWP_FALSE, arena, &tx->pixels);
        if (st != GWP_STATUS_OK) return st;
        tx->width_after = current_width;
        tx->height_after = current_height;
      } else if (type == GWP_VP8L_TRANSFORM_SUB_GREEN) {
        tx->width_after = current_width;
        tx->height_after = current_height;
      } else {
        GWPu32 palette_size_minus1;
        if (!GWPBitReaderGetBits(br, 8, &palette_size_minus1)) return GWP_STATUS_TRUNCATED_DATA;
        tx->color_table_size = palette_size_minus1 + 1u;
        st = GWPVP8LDecodeImageData(br, tx->color_table_size, 1u, GWP_FALSE, arena, &tx->pixels);
        if (st != GWP_STATUS_OK) return st;
        GWPVP8LFinalizeColorTable(tx->pixels, tx->color_table_size);
        if (tx->color_table_size <= 2u) tx->width_bits = 3u;
        else if (tx->color_table_size <= 4u) tx->width_bits = 2u;
        else if (tx->color_table_size <= 16u) tx->width_bits = 1u;
        else tx->width_bits = 0u;
        current_width = GWPRoundUpDiv(current_width, 1u << tx->width_bits);
        tx->width_after = current_width;
        tx->height_after = current_height;
      }
      ++num_transforms;
    }
  }

  *out_num_transforms = num_transforms;
  *out_work_width = current_width;
  *out_work_height = current_height;
  return GWP_STATUS_OK;
}

static GWPStatusCode GWPVP8LInverseTransforms(const GWPVP8LTransform* transforms,
                                              GWPu32 num_transforms,
                                              GWPArena* arena,
                                              GWPu32** io_pixels,
                                              GWPu32* io_width,
                                              GWPu32* io_height) {
  GWPu32 i;
  if (arena == 0 || io_pixels == 0 || *io_pixels == 0 || io_width == 0 || io_height == 0) {
    return GWP_STATUS_INVALID_PARAM;
  }
  for (i = num_transforms; i > 0u; --i) {
    const GWPVP8LTransform* tx;
    tx = &transforms[i - 1u];
    switch (tx->type) {
      case GWP_VP8L_TRANSFORM_SUB_GREEN:
        GWPVP8LApplySubtractGreen(*io_pixels, (*io_width) * (*io_height));
        break;
      case GWP_VP8L_TRANSFORM_COLOR:
        GWPVP8LApplyColorTransform(tx, *io_pixels, *io_width, *io_height);
        break;
      case GWP_VP8L_TRANSFORM_PREDICTOR:
        GWPVP8LApplyPredictorTransform(tx, *io_pixels, *io_width, *io_height);
        break;
      case GWP_VP8L_TRANSFORM_COLOR_INDEX:
        {
          GWPStatusCode st;
          st = GWPVP8LApplyColorIndexTransform(tx, arena, io_pixels, io_width, io_height);
          if (st != GWP_STATUS_OK) return st;
        }
        break;
      default:
        return GWP_STATUS_BITSTREAM_ERROR;
    }
  }
  return GWP_STATUS_OK;
}

static void GWPStorePixel(GWPu8* dst, GWPPixelFormat fmt, GWPu32 argb) {
  GWPu8 a;
  GWPu8 r;
  GWPu8 g;
  GWPu8 b;
  a = (GWPu8)GWP_ALPHA(argb);
  r = (GWPu8)GWP_RED(argb);
  g = (GWPu8)GWP_GREEN(argb);
  b = (GWPu8)GWP_BLUE(argb);
  if (fmt == GWP_PIXFMT_BGRA) {
    dst[0] = b; dst[1] = g; dst[2] = r; dst[3] = a;
  } else if (fmt == GWP_PIXFMT_ARGB) {
    dst[0] = a; dst[1] = r; dst[2] = g; dst[3] = b;
  } else {
    dst[0] = r; dst[1] = g; dst[2] = b; dst[3] = a;
  }
}


GWPStatusCode GWPDecodeVP8LAlphaImage(const GWPu8* data,
                                      GWPu32 data_size,
                                      GWPu32 width,
                                      GWPu32 height,
                                      void* scratch,
                                      GWPu32 scratch_size,
                                      GWPu8* out_alpha) {
  GWPBitReader br;
  GWPVP8LTransform transforms[GWP_VP8L_MAX_TRANSFORMS];
  GWPu32 num_transforms;
  GWPu32 work_width;
  GWPu32 work_height;
  GWPArena arena;
  GWPu32* pixels;
  GWPStatusCode st;
  GWPu32 i;
  GWPu32 pixel_count;

  if (data == 0 || scratch == 0 || out_alpha == 0 || width == 0u || height == 0u) {
    return GWP_STATUS_INVALID_PARAM;
  }
  if (!GWPMulU32(width, height, &pixel_count)) return GWP_STATUS_LIMIT_EXCEEDED;
  GWPBitReaderInit(&br, data, data_size);
  GWPArenaInit(&arena, scratch, scratch_size);
  GWPZero(transforms, (GWPu32)sizeof(transforms));

  st = GWPVP8LParseTransforms(&br, &arena, width, height,
                              transforms, &num_transforms, &work_width, &work_height);
  if (st != GWP_STATUS_OK) return st;

  st = GWPVP8LDecodeImageData(&br, work_width, work_height, GWP_TRUE, &arena, &pixels);
  if (st != GWP_STATUS_OK) return st;

  st = GWPVP8LInverseTransforms(transforms, num_transforms, &arena, &pixels, &work_width, &work_height);
  if (st != GWP_STATUS_OK) return st;

  if (work_width != width || work_height != height) return GWP_STATUS_BITSTREAM_ERROR;

  for (i = 0u; i < pixel_count; ++i) {
    out_alpha[i] = (GWPu8)GWP_GREEN(pixels[i]);
  }
  return GWP_STATUS_OK;
}

GWPStatusCode GWPDecodeVP8L(const GWPu8* data,
                            GWPu32 data_size,
                            const GWPDecoderOptions* options,
                            GWPBitstreamFeatures* out_features) {
  GWPBitReader br;
  GWPVP8LHeader header;
  GWPVP8LTransform transforms[GWP_VP8L_MAX_TRANSFORMS];
  GWPu32 num_transforms;
  GWPu32 work_width;
  GWPu32 work_height;
  GWPArena arena;
  GWPu32* pixels;
  GWPStatusCode st;
  GWPu32 total_output_bytes;
  GWPu32 y;

  if (data == 0 || options == 0 || options->output_buffer == 0 ||
      options->scratch == 0 || options->output_stride == 0u) {
    return GWP_STATUS_INVALID_PARAM;
  }

  st = GWPVP8LReadHeader(data, data_size, &br, &header);
  if (st != GWP_STATUS_OK) return st;

  if (header.width > GWP_MAX_IMAGE_WIDTH || header.height > GWP_MAX_IMAGE_HEIGHT) {
    return GWP_STATUS_BAD_DIMENSIONS;
  }

  if (options->strict) {
    if (options->max_width != 0u && header.width > options->max_width) return GWP_STATUS_LIMIT_EXCEEDED;
    if (options->max_height != 0u && header.height > options->max_height) return GWP_STATUS_LIMIT_EXCEEDED;
  }

  if (!GWPMulU32(header.height, options->output_stride, &total_output_bytes)) {
    return GWP_STATUS_LIMIT_EXCEEDED;
  }
  if (total_output_bytes > options->output_buffer_size) return GWP_STATUS_NOT_ENOUGH_OUTPUT;

  GWPArenaInit(&arena, options->scratch, options->scratch_size);
  GWPZero(transforms, (GWPu32)sizeof(transforms));

  st = GWPVP8LParseTransforms(&br, &arena, header.width, header.height,
                              transforms, &num_transforms, &work_width, &work_height);
  if (st != GWP_STATUS_OK) return st;

  st = GWPVP8LDecodeImageData(&br, work_width, work_height, GWP_TRUE, &arena, &pixels);
  if (st != GWP_STATUS_OK) return st;

  st = GWPVP8LInverseTransforms(transforms, num_transforms, &arena, &pixels, &work_width, &work_height);
  if (st != GWP_STATUS_OK) return st;

  if (work_width != header.width || work_height != header.height) return GWP_STATUS_BITSTREAM_ERROR;

  for (y = 0u; y < header.height; ++y) {
    GWPu32 x;
    GWPu8* row;
    row = options->output_buffer + y * options->output_stride;
    for (x = 0u; x < header.width; ++x) {
      GWPStorePixel(row + x * 4u, options->pixel_format, pixels[y * header.width + x]);
    }
  }

  if (out_features != 0) {
    out_features->width = header.width;
    out_features->height = header.height;
    out_features->has_alpha = header.alpha_is_used;
    out_features->has_animation = GWP_FALSE;
    out_features->has_icc = GWP_FALSE;
    out_features->has_exif = GWP_FALSE;
    out_features->has_xmp = GWP_FALSE;
    out_features->format = GWP_BITSTREAM_VP8L;
    out_features->frame_count = 0u;
  }
  return GWP_STATUS_OK;
}
