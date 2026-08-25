/* SPDX-License-Identifier: CC0-1.0 */
#include "tifx.h"

#include <limits.h>
#include <string.h>
#include <zlib.h>

#define TIFX_TYPE_SHORT 3U
#define TIFX_TYPE_LONG 4U
#define TIFX_TYPE_RATIONAL 5U

#define TIFX_TAG_NEW_SUBFILE_TYPE 254U
#define TIFX_TAG_IMAGE_WIDTH 256U
#define TIFX_TAG_IMAGE_LENGTH 257U
#define TIFX_TAG_BITS_PER_SAMPLE 258U
#define TIFX_TAG_COMPRESSION 259U
#define TIFX_TAG_PHOTOMETRIC 262U
#define TIFX_TAG_FILL_ORDER 266U
#define TIFX_TAG_STRIP_OFFSETS 273U
#define TIFX_TAG_ORIENTATION 274U
#define TIFX_TAG_SAMPLES_PER_PIXEL 277U
#define TIFX_TAG_ROWS_PER_STRIP 278U
#define TIFX_TAG_STRIP_BYTE_COUNTS 279U
#define TIFX_TAG_X_RESOLUTION 282U
#define TIFX_TAG_Y_RESOLUTION 283U
#define TIFX_TAG_PLANAR_CONFIGURATION 284U
#define TIFX_TAG_T4_OPTIONS 292U
#define TIFX_TAG_T6_OPTIONS 293U
#define TIFX_TAG_RESOLUTION_UNIT 296U
#define TIFX_TAG_PAGE_NUMBER 297U
#define TIFX_TAG_PREDICTOR 317U
#define TIFX_TAG_TILE_WIDTH 322U
#define TIFX_TAG_TILE_LENGTH 323U
#define TIFX_TAG_TILE_OFFSETS 324U
#define TIFX_TAG_TILE_BYTE_COUNTS 325U
#define TIFX_TAG_COLOR_MAP 320U
#define TIFX_TAG_SUBIFDS 330U
#define TIFX_TAG_EXTRA_SAMPLES 338U
#define TIFX_TAG_SAMPLE_FORMAT 339U

#define TIFX_MH_MAX_CODE_BITS 13U
#define TIFX_ZLIB_ARENA_SIZE 393216UL
#define TIFX_ZLIB_MEM_LEVEL 7
#define TIFX_DEFLATE_BLOCK_BYTES 8192U

typedef struct tifx_mh_code {
    unsigned short bit_length;
    unsigned short run_length;
    unsigned short code_word;
} tifx_mh_code;

typedef struct tifx_bit_reader {
    const unsigned char *src;
    const unsigned char *src_end;
    unsigned short fill_order;
    unsigned short bits_left;
    unsigned char current_byte;
} tifx_bit_reader;

typedef struct tifx_bit_writer {
    unsigned char *dst;
    unsigned long dst_size;
    unsigned long bit_count;
} tifx_bit_writer;

typedef struct tifx_lzw_decoder {
    tifx_bit_reader reader;
    unsigned short code_size;
    unsigned short next_code;
    unsigned short old_code;
    unsigned short have_old;
    unsigned short stack_len;
    unsigned char stack[4097];
    unsigned short prefix[4096];
    unsigned char suffix[4096];
} tifx_lzw_decoder;

typedef struct tifx_lzw_encoder {
    tifx_bit_writer writer;
    unsigned short code_size;
    unsigned short next_code;
    unsigned short have_prefix;
    unsigned short prefix_code;
    unsigned long hash_keys[9001];
    unsigned short hash_codes[9001];
} tifx_lzw_encoder;

typedef struct tifx_zlib_arena {
    unsigned long offset;
    unsigned char buffer[TIFX_ZLIB_ARENA_SIZE];
} tifx_zlib_arena;

typedef struct tifx_zlib_stored_decoder {
    z_stream stream;
    tifx_zlib_arena arena;
    unsigned short initialized;
    unsigned short finished;
} tifx_zlib_stored_decoder;

typedef struct tifx_deflate_plan {
    int level;
    int strategy;
    int good_length;
    int max_lazy;
    int nice_length;
    int max_chain;
    unsigned short mode;
} tifx_deflate_plan;

static void tifx_lzw_decoder_init(tifx_lzw_decoder *decoder,
                                  const unsigned char *src,
                                  const unsigned char *src_end);
static int tifx_lzw_decoder_read(tifx_lzw_decoder *decoder,
                                 unsigned char *dst,
                                 unsigned long dst_len);
static int tifx_lzw_process_segment(unsigned char *dst,
                                    unsigned long dst_size,
                                    const tifx_write_params *params,
                                    unsigned long stride_min,
                                    unsigned long segment_origin,
                                    unsigned long row_count,
                                    int is_tiled,
                                    unsigned long *out_written);
static int tifx_deflate_stored_process_segment(unsigned char *dst,
                                               unsigned long dst_size,
                                               const tifx_write_params *params,
                                               unsigned long stride_min,
                                               unsigned long segment_origin,
                                               unsigned long row_count,
                                               int is_tiled,
                                               unsigned long *out_written);
static int tifx_zlib_stored_decoder_init(tifx_zlib_stored_decoder *decoder,
                                         const unsigned char *src,
                                         const unsigned char *src_end);
static int tifx_zlib_stored_decoder_read(tifx_zlib_stored_decoder *decoder,
                                         unsigned char *dst,
                                         unsigned long dst_len);
static int tifx_zlib_stored_decoder_finish(tifx_zlib_stored_decoder *decoder);
static int tifx_zlib_stored_decoder_drain_empty_final_blocks(tifx_zlib_stored_decoder *decoder);
static int tifx_apply_horizontal_predictor_inverse(const tifx_image_info *info,
                                                   unsigned char *row,
                                                   unsigned long row_bytes);

static const tifx_mh_code tifx_mh_white_codes[] = {
    { 4U, 2U, 0x7U },
    { 4U, 3U, 0x8U },
    { 4U, 4U, 0xBU },
    { 4U, 5U, 0xCU },
    { 4U, 6U, 0xEU },
    { 4U, 7U, 0xFU },
    { 5U, 8U, 0x13U },
    { 5U, 9U, 0x14U },
    { 5U, 10U, 0x7U },
    { 5U, 11U, 0x8U },
    { 5U, 64U, 0x1BU },
    { 5U, 128U, 0x12U },
    { 6U, 1U, 0x7U },
    { 6U, 12U, 0x8U },
    { 6U, 13U, 0x3U },
    { 6U, 14U, 0x34U },
    { 6U, 15U, 0x35U },
    { 6U, 16U, 0x2AU },
    { 6U, 17U, 0x2BU },
    { 6U, 192U, 0x17U },
    { 6U, 1664U, 0x18U },
    { 7U, 18U, 0x27U },
    { 7U, 19U, 0xCU },
    { 7U, 20U, 0x8U },
    { 7U, 21U, 0x17U },
    { 7U, 22U, 0x3U },
    { 7U, 23U, 0x4U },
    { 7U, 24U, 0x28U },
    { 7U, 25U, 0x2BU },
    { 7U, 26U, 0x13U },
    { 7U, 27U, 0x24U },
    { 7U, 28U, 0x18U },
    { 7U, 256U, 0x37U },
    { 8U, 0U, 0x35U },
    { 8U, 29U, 0x2U },
    { 8U, 30U, 0x3U },
    { 8U, 31U, 0x1AU },
    { 8U, 32U, 0x1BU },
    { 8U, 33U, 0x12U },
    { 8U, 34U, 0x13U },
    { 8U, 35U, 0x14U },
    { 8U, 36U, 0x15U },
    { 8U, 37U, 0x16U },
    { 8U, 38U, 0x17U },
    { 8U, 39U, 0x28U },
    { 8U, 40U, 0x29U },
    { 8U, 41U, 0x2AU },
    { 8U, 42U, 0x2BU },
    { 8U, 43U, 0x2CU },
    { 8U, 44U, 0x2DU },
    { 8U, 45U, 0x4U },
    { 8U, 46U, 0x5U },
    { 8U, 47U, 0xAU },
    { 8U, 48U, 0xBU },
    { 8U, 49U, 0x52U },
    { 8U, 50U, 0x53U },
    { 8U, 51U, 0x54U },
    { 8U, 52U, 0x55U },
    { 8U, 53U, 0x24U },
    { 8U, 54U, 0x25U },
    { 8U, 55U, 0x58U },
    { 8U, 56U, 0x59U },
    { 8U, 57U, 0x5AU },
    { 8U, 58U, 0x5BU },
    { 8U, 59U, 0x4AU },
    { 8U, 60U, 0x4BU },
    { 8U, 61U, 0x32U },
    { 8U, 62U, 0x33U },
    { 8U, 63U, 0x34U },
    { 8U, 320U, 0x36U },
    { 8U, 384U, 0x37U },
    { 8U, 448U, 0x64U },
    { 8U, 512U, 0x65U },
    { 8U, 576U, 0x68U },
    { 8U, 640U, 0x67U },
    { 9U, 704U, 0xCCU },
    { 9U, 768U, 0xCDU },
    { 9U, 832U, 0xD2U },
    { 9U, 896U, 0xD3U },
    { 9U, 960U, 0xD4U },
    { 9U, 1024U, 0xD5U },
    { 9U, 1088U, 0xD6U },
    { 9U, 1152U, 0xD7U },
    { 9U, 1216U, 0xD8U },
    { 9U, 1280U, 0xD9U },
    { 9U, 1344U, 0xDAU },
    { 9U, 1408U, 0xDBU },
    { 9U, 1472U, 0x98U },
    { 9U, 1536U, 0x99U },
    { 9U, 1600U, 0x9AU },
    { 9U, 1728U, 0x9BU },
    { 11U, 1792U, 0x8U },
    { 11U, 1856U, 0xCU },
    { 11U, 1920U, 0xDU },
    { 12U, 1984U, 0x12U },
    { 12U, 2048U, 0x13U },
    { 12U, 2112U, 0x14U },
    { 12U, 2176U, 0x15U },
    { 12U, 2240U, 0x16U },
    { 12U, 2304U, 0x17U },
    { 12U, 2368U, 0x1CU },
    { 12U, 2432U, 0x1DU },
    { 12U, 2496U, 0x1EU },
    { 12U, 2560U, 0x1FU },
};

static const tifx_mh_code tifx_mh_black_codes[] = {
    { 2U, 2U, 0x3U },
    { 2U, 3U, 0x2U },
    { 3U, 1U, 0x2U },
    { 3U, 4U, 0x3U },
    { 4U, 5U, 0x3U },
    { 4U, 6U, 0x2U },
    { 5U, 7U, 0x3U },
    { 6U, 8U, 0x5U },
    { 6U, 9U, 0x4U },
    { 7U, 10U, 0x4U },
    { 7U, 11U, 0x5U },
    { 7U, 12U, 0x7U },
    { 8U, 13U, 0x4U },
    { 8U, 14U, 0x7U },
    { 9U, 15U, 0x18U },
    { 10U, 0U, 0x37U },
    { 10U, 16U, 0x17U },
    { 10U, 17U, 0x18U },
    { 10U, 18U, 0x8U },
    { 10U, 64U, 0xFU },
    { 11U, 19U, 0x67U },
    { 11U, 20U, 0x68U },
    { 11U, 21U, 0x6CU },
    { 11U, 22U, 0x37U },
    { 11U, 23U, 0x28U },
    { 11U, 24U, 0x17U },
    { 11U, 25U, 0x18U },
    { 11U, 1792U, 0x8U },
    { 11U, 1856U, 0xCU },
    { 11U, 1920U, 0xDU },
    { 12U, 26U, 0xCAU },
    { 12U, 27U, 0xCBU },
    { 12U, 28U, 0xCCU },
    { 12U, 29U, 0xCDU },
    { 12U, 30U, 0x68U },
    { 12U, 31U, 0x69U },
    { 12U, 32U, 0x6AU },
    { 12U, 33U, 0x6BU },
    { 12U, 34U, 0xD2U },
    { 12U, 35U, 0xD3U },
    { 12U, 36U, 0xD4U },
    { 12U, 37U, 0xD5U },
    { 12U, 38U, 0xD6U },
    { 12U, 39U, 0xD7U },
    { 12U, 40U, 0x6CU },
    { 12U, 41U, 0x6DU },
    { 12U, 42U, 0xDAU },
    { 12U, 43U, 0xDBU },
    { 12U, 44U, 0x54U },
    { 12U, 45U, 0x55U },
    { 12U, 46U, 0x56U },
    { 12U, 47U, 0x57U },
    { 12U, 48U, 0x64U },
    { 12U, 49U, 0x65U },
    { 12U, 50U, 0x52U },
    { 12U, 51U, 0x53U },
    { 12U, 52U, 0x24U },
    { 12U, 53U, 0x37U },
    { 12U, 54U, 0x38U },
    { 12U, 55U, 0x27U },
    { 12U, 56U, 0x28U },
    { 12U, 57U, 0x58U },
    { 12U, 58U, 0x59U },
    { 12U, 59U, 0x2BU },
    { 12U, 60U, 0x2CU },
    { 12U, 61U, 0x5AU },
    { 12U, 62U, 0x66U },
    { 12U, 63U, 0x67U },
    { 12U, 128U, 0xC8U },
    { 12U, 192U, 0xC9U },
    { 12U, 256U, 0x5BU },
    { 12U, 320U, 0x33U },
    { 12U, 384U, 0x34U },
    { 12U, 448U, 0x35U },
    { 12U, 1984U, 0x12U },
    { 12U, 2048U, 0x13U },
    { 12U, 2112U, 0x14U },
    { 12U, 2176U, 0x15U },
    { 12U, 2240U, 0x16U },
    { 12U, 2304U, 0x17U },
    { 12U, 2368U, 0x1CU },
    { 12U, 2432U, 0x1DU },
    { 12U, 2496U, 0x1EU },
    { 12U, 2560U, 0x1FU },
    { 13U, 512U, 0x6CU },
    { 13U, 576U, 0x6DU },
    { 13U, 640U, 0x4AU },
    { 13U, 704U, 0x4BU },
    { 13U, 768U, 0x4CU },
    { 13U, 832U, 0x4DU },
    { 13U, 896U, 0x72U },
    { 13U, 960U, 0x73U },
    { 13U, 1024U, 0x74U },
    { 13U, 1088U, 0x75U },
    { 13U, 1152U, 0x76U },
    { 13U, 1216U, 0x77U },
    { 13U, 1280U, 0x52U },
    { 13U, 1344U, 0x53U },
    { 13U, 1408U, 0x54U },
    { 13U, 1472U, 0x55U },
    { 13U, 1536U, 0x5AU },
    { 13U, 1600U, 0x5BU },
    { 13U, 1664U, 0x64U },
    { 13U, 1728U, 0x65U },
};

static unsigned short tifx_read_u16(const unsigned char *p, int is_big_endian)
{
    if (is_big_endian) {
        return (unsigned short)(((unsigned short)p[0] << 8) | (unsigned short)p[1]);
    }
    return (unsigned short)(((unsigned short)p[1] << 8) | (unsigned short)p[0]);
}

static unsigned long tifx_read_u32(const unsigned char *p, int is_big_endian)
{
    if (is_big_endian) {
        return ((unsigned long)p[0] << 24) |
               ((unsigned long)p[1] << 16) |
               ((unsigned long)p[2] << 8) |
               (unsigned long)p[3];
    }
    return ((unsigned long)p[3] << 24) |
           ((unsigned long)p[2] << 16) |
           ((unsigned long)p[1] << 8) |
           (unsigned long)p[0];
}

static void tifx_write_u16le(unsigned char *p, unsigned short value)
{
    p[0] = (unsigned char)(value & 0xFFU);
    p[1] = (unsigned char)((value >> 8) & 0xFFU);
}

static void tifx_write_u32le(unsigned char *p, unsigned long value)
{
    p[0] = (unsigned char)(value & 0xFFUL);
    p[1] = (unsigned char)((value >> 8) & 0xFFUL);
    p[2] = (unsigned char)((value >> 16) & 0xFFUL);
    p[3] = (unsigned char)((value >> 24) & 0xFFUL);
}

static int tifx_add_ul(unsigned long a, unsigned long b, unsigned long *out)
{
    if (a > ULONG_MAX - b) {
        return 0;
    }
    *out = a + b;
    return 1;
}

static int tifx_mul_ul(unsigned long a, unsigned long b, unsigned long *out)
{
    if (a != 0UL && b > ULONG_MAX / a) {
        return 0;
    }
    *out = a * b;
    return 1;
}

static unsigned long tifx_ceil_div(unsigned long value, unsigned long divisor)
{
    if (value == 0UL) {
        return 0UL;
    }
    return ((value - 1UL) / divisor) + 1UL;
}

static unsigned char tifx_reverse_byte(unsigned char value)
{
    value = (unsigned char)(((value & 0xF0U) >> 4) | ((value & 0x0FU) << 4));
    value = (unsigned char)(((value & 0xCCU) >> 2) | ((value & 0x33U) << 2));
    value = (unsigned char)(((value & 0xAAU) >> 1) | ((value & 0x55U) << 1));
    return value;
}

static void tifx_reverse_bytes_in_place(unsigned char *data, unsigned long size)
{
    unsigned long i;

    if (data == 0) {
        return;
    }

    for (i = 0UL; i < size; ++i) {
        data[i] = tifx_reverse_byte(data[i]);
    }
}

static unsigned short tifx_effective_fill_order(const tifx_write_params *params)
{
    if (params == 0 || params->fill_order == 0U) {
        return 1U;
    }
    return params->fill_order;
}

int tifx__compute_tile_count(unsigned long width,
                             unsigned long height,
                             unsigned long tile_width,
                             unsigned long tile_length,
                             unsigned long *out_tile_count);

static unsigned short tifx_effective_write_planar_config(const tifx_write_params *params)
{
    if (params == 0 || params->planar_config == 0U) {
        return 1U;
    }
    return params->planar_config;
}

static unsigned short tifx_effective_predictor(const tifx_write_params *params)
{
    if (params == 0 || params->predictor == 0U) {
        return TIFX_PREDICTOR_NONE;
    }
    return params->predictor;
}

static unsigned long tifx_predictor_row_sample_stride_params(const tifx_write_params *params)
{
    unsigned short planar_config;

    if (params == 0) {
        return 1UL;
    }
    planar_config = tifx_effective_write_planar_config(params);
    if (params->pixel_format == TIFX_PIXEL_RGB24 && planar_config == 1U) {
        return 3UL;
    }
    if (params->pixel_format == TIFX_PIXEL_RGBA32 && planar_config == 1U) {
        return 4UL;
    }
    return 1UL;
}

static unsigned long tifx_predictor_row_sample_stride_info(const tifx_image_info *info)
{
    if (info == 0) {
        return 1UL;
    }
    if (info->pixel_format == TIFX_PIXEL_RGB24 && info->planar_config == 1U) {
        return 3UL;
    }
    if (info->pixel_format == TIFX_PIXEL_RGBA32 && info->planar_config == 1U) {
        return 4UL;
    }
    return 1UL;
}

static unsigned long tifx_write_sample_count(const tifx_write_params *params)
{
    if (params == 0) {
        return 0UL;
    }
    if (params->pixel_format == TIFX_PIXEL_RGB24) {
        return 3UL;
    }
    if (params->pixel_format == TIFX_PIXEL_RGBA32) {
        return 4UL;
    }
    return 1UL;
}

static int tifx_map_write_segment_index(const tifx_write_params *params,
                                        int is_tiled,
                                        unsigned long segment_index,
                                        unsigned long *out_plane_index,
                                        unsigned long *out_base_segment_index,
                                        unsigned long *out_base_segment_count)
{
    unsigned short planar_config;
    unsigned long sample_count;
    unsigned long base_segment_count;
    int rc;

    if (params == 0 || out_plane_index == 0 || out_base_segment_index == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    planar_config = tifx_effective_write_planar_config(params);
    sample_count = tifx_write_sample_count(params);

    if (is_tiled) {
        rc = tifx__compute_tile_count(params->width,
                                      params->height,
                                      params->tile_width,
                                      params->tile_length,
                                      &base_segment_count);
        if (rc != TIFX_OK) {
            return rc;
        }
    } else {
        unsigned long rows_per_strip;

        rows_per_strip = params->rows_per_strip;
        if (rows_per_strip == 0UL || rows_per_strip > params->height) {
            rows_per_strip = params->height;
        }
        if (rows_per_strip == 0UL) {
            return TIFX_ERR_BAD_ARGUMENT;
        }
        base_segment_count = tifx_ceil_div(params->height, rows_per_strip);
        if (base_segment_count == 0UL || base_segment_count > TIFX_MAX_STRIPS) {
            return TIFX_ERR_UNSUPPORTED;
        }
    }

    if (planar_config == 2U && sample_count > 1UL) {
        unsigned long plane_index;
        if (base_segment_count == 0UL) {
            return TIFX_ERR_BAD_ARGUMENT;
        }
        plane_index = segment_index / base_segment_count;
        if (plane_index >= sample_count) {
            return TIFX_ERR_BAD_ARGUMENT;
        }
        *out_plane_index = plane_index;
        *out_base_segment_index = segment_index % base_segment_count;
    } else {
        *out_plane_index = 0UL;
        *out_base_segment_index = segment_index;
    }

    if (out_base_segment_count != 0) {
        *out_base_segment_count = base_segment_count;
    }
    return TIFX_OK;
}

int tifx__params_is_tiled(const tifx_write_params *params)
{
    if (params == 0) {
        return 0;
    }
    if (params->tile_width == 0UL && params->tile_length == 0UL) {
        return 0;
    }
    if (params->tile_width == 0UL || params->tile_length == 0UL) {
        return -1;
    }
    return 1;
}

int tifx__compute_tile_count(unsigned long width,
                             unsigned long height,
                             unsigned long tile_width,
                             unsigned long tile_length,
                             unsigned long *out_tile_count)
{
    unsigned long tiles_across;
    unsigned long tiles_down;
    unsigned long tile_count;

    if (tile_width == 0UL || tile_length == 0UL || out_tile_count == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    tiles_across = tifx_ceil_div(width, tile_width);
    tiles_down = tifx_ceil_div(height, tile_length);
    if (!tifx_mul_ul(tiles_across, tiles_down, &tile_count)) {
        return TIFX_ERR_OVERFLOW;
    }
    if (tile_count == 0UL || tile_count > TIFX_MAX_TILES) {
        return TIFX_ERR_UNSUPPORTED;
    }

    *out_tile_count = tile_count;
    return TIFX_OK;
}

void tifx_image_info_init(tifx_image_info *info)
{
    if (info == 0) {
        return;
    }
    memset(info, 0, sizeof(*info));
    info->container_format = TIFX_CONTAINER_CLASSIC;
    info->compression = 1U;
    info->planar_config = 1U;
    info->orientation = 1U;
    info->fill_order = 1U;
    info->resolution_unit = 2U;
    info->samples_per_pixel = 1U;
    info->alpha_mode = TIFX_ALPHA_NONE;
    info->predictor = TIFX_PREDICTOR_NONE;
    info->storage_layout = TIFX_LAYOUT_STRIPS;
    info->page_count = 1UL;
}

void tifx_write_params_init(tifx_write_params *params)
{
    if (params == 0) {
        return;
    }
    memset(params, 0, sizeof(*params));
    params->container_format = TIFX_CONTAINER_CLASSIC;
    params->compression = 1U;
    params->alpha_mode = TIFX_ALPHA_NONE;
    params->planar_config = 1U;
    params->resolution_unit = 2U;
    params->fill_order = 1U;
    params->subifd_style = TIFX_SUBIFD_STYLE_TREE;
    params->predictor = TIFX_PREDICTOR_NONE;
    params->deflate_mode = TIFX_DEFLATE_STORED;
    params->x_resolution = TIFX_FP_FROM_INT(72L);
    params->y_resolution = TIFX_FP_FROM_INT(72L);
}

int tifx_bigtiff_available(void)
{
#if ULONG_MAX <= 0xFFFFFFFFUL
    return 0;
#else
    return 1;
#endif
}

const char *tifx_strerror(int code)
{
    switch (code) {
        case TIFX_OK: return "ok";
        case TIFX_ERR_BAD_ARGUMENT: return "bad argument";
        case TIFX_ERR_BAD_FORMAT: return "bad TIFF format";
        case TIFX_ERR_UNSUPPORTED: return "unsupported TIFF feature";
        case TIFX_ERR_TRUNCATED: return "truncated input";
        case TIFX_ERR_OVERFLOW: return "integer overflow";
        case TIFX_ERR_NO_SPACE: return "destination buffer too small";
        default: return "unknown error";
    }
}

tifx_fixed tifx_fp_from_ratio(unsigned long numerator, unsigned long denominator)
{
    unsigned long integer_part;
    unsigned long fraction_part;
    unsigned long remainder;
    unsigned long max_integer;
    int i;

    if (denominator == 0UL) {
        return 0;
    }

    integer_part = numerator / denominator;
    max_integer = ((unsigned long)LONG_MAX) >> TIFX_FP_SHIFT;
    if (integer_part > max_integer) {
        return (tifx_fixed)LONG_MAX;
    }

    remainder = numerator % denominator;
    fraction_part = 0UL;

    for (i = 0; i < 16; ++i) {
        fraction_part <<= 1;
        if (remainder >= denominator - remainder) {
            remainder = remainder - (denominator - remainder);
            fraction_part |= 1UL;
        } else {
            remainder += remainder;
        }
    }

    return (tifx_fixed)((integer_part << TIFX_FP_SHIFT) | fraction_part);
}

unsigned long tifx_fp_to_rational_numerator(tifx_fixed value)
{
    if (value <= 0) {
        return 0UL;
    }
    return (unsigned long)value;
}

unsigned long tifx_fp_to_rational_denominator(void)
{
    return 65536UL;
}

static int tifx_entry_bytes(unsigned short type,
                            unsigned long count,
                            unsigned long *out_bytes)
{
    unsigned long unit_size;

    switch (type) {
        case TIFX_TYPE_SHORT:
            unit_size = 2UL;
            break;
        case TIFX_TYPE_LONG:
            unit_size = 4UL;
            break;
        case TIFX_TYPE_RATIONAL:
            unit_size = 8UL;
            break;
        default:
            return TIFX_ERR_UNSUPPORTED;
    }

    if (!tifx_mul_ul(unit_size, count, out_bytes)) {
        return TIFX_ERR_OVERFLOW;
    }
    return TIFX_OK;
}

static int tifx_entry_ptr(const unsigned char *file_bytes,
                          unsigned long file_size,
                          int is_big_endian,
                          const unsigned char *entry,
                          unsigned short type,
                          unsigned long count,
                          const unsigned char **out_ptr)
{
    unsigned long total_bytes;
    unsigned long offset;
    int rc;

    rc = tifx_entry_bytes(type, count, &total_bytes);
    if (rc != TIFX_OK) {
        return rc;
    }

    if (total_bytes <= 4UL) {
        *out_ptr = entry + 8;
        return TIFX_OK;
    }

    offset = tifx_read_u32(entry + 8, is_big_endian);
    if (offset > file_size || total_bytes > file_size - offset) {
        return TIFX_ERR_TRUNCATED;
    }

    *out_ptr = file_bytes + offset;
    return TIFX_OK;
}

static int tifx_entry_scalar_u32(const unsigned char *file_bytes,
                                 unsigned long file_size,
                                 int is_big_endian,
                                 const unsigned char *entry,
                                 unsigned short type,
                                 unsigned long count,
                                 unsigned long *out_value)
{
    const unsigned char *ptr;
    int rc;

    if (count != 1UL) {
        return TIFX_ERR_BAD_FORMAT;
    }

    if (type != TIFX_TYPE_SHORT && type != TIFX_TYPE_LONG) {
        return TIFX_ERR_UNSUPPORTED;
    }

    rc = tifx_entry_ptr(file_bytes, file_size, is_big_endian, entry, type, count, &ptr);
    if (rc != TIFX_OK) {
        return rc;
    }

    if (type == TIFX_TYPE_SHORT) {
        *out_value = (unsigned long)tifx_read_u16(ptr, is_big_endian);
    } else {
        *out_value = tifx_read_u32(ptr, is_big_endian);
    }
    return TIFX_OK;
}

static int tifx_entry_short_array(const unsigned char *file_bytes,
                                  unsigned long file_size,
                                  int is_big_endian,
                                  const unsigned char *entry,
                                  unsigned long count,
                                  unsigned short *out_values,
                                  unsigned long out_capacity)
{
    const unsigned char *ptr;
    unsigned long i;
    int rc;

    if (count > out_capacity) {
        return TIFX_ERR_UNSUPPORTED;
    }

    rc = tifx_entry_ptr(file_bytes, file_size, is_big_endian, entry,
                        TIFX_TYPE_SHORT, count, &ptr);
    if (rc != TIFX_OK) {
        return rc;
    }

    for (i = 0UL; i < count; ++i) {
        out_values[i] = tifx_read_u16(ptr + (i * 2UL), is_big_endian);
    }
    return TIFX_OK;
}

static int tifx_entry_u32_array(const unsigned char *file_bytes,
                                unsigned long file_size,
                                int is_big_endian,
                                const unsigned char *entry,
                                unsigned short type,
                                unsigned long count,
                                unsigned long *out_values,
                                unsigned long out_capacity)
{
    const unsigned char *ptr;
    unsigned long i;
    int rc;

    if (count > out_capacity) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (type != TIFX_TYPE_SHORT && type != TIFX_TYPE_LONG) {
        return TIFX_ERR_UNSUPPORTED;
    }

    rc = tifx_entry_ptr(file_bytes, file_size, is_big_endian, entry, type, count, &ptr);
    if (rc != TIFX_OK) {
        return rc;
    }

    for (i = 0UL; i < count; ++i) {
        if (type == TIFX_TYPE_SHORT) {
            out_values[i] = (unsigned long)tifx_read_u16(ptr + (i * 2UL), is_big_endian);
        } else {
            out_values[i] = tifx_read_u32(ptr + (i * 4UL), is_big_endian);
        }
    }
    return TIFX_OK;
}

static int tifx_entry_rational_fixed(const unsigned char *file_bytes,
                                     unsigned long file_size,
                                     int is_big_endian,
                                     const unsigned char *entry,
                                     tifx_fixed *out_value)
{
    const unsigned char *ptr;
    unsigned long numerator;
    unsigned long denominator;
    int rc;

    rc = tifx_entry_ptr(file_bytes, file_size, is_big_endian, entry,
                        TIFX_TYPE_RATIONAL, 1UL, &ptr);
    if (rc != TIFX_OK) {
        return rc;
    }

    numerator = tifx_read_u32(ptr, is_big_endian);
    denominator = tifx_read_u32(ptr + 4, is_big_endian);
    *out_value = tifx_fp_from_ratio(numerator, denominator);
    return TIFX_OK;
}

static int tifx_copy_bits_defaults(tifx_image_info *info,
                                   const unsigned short *bits_values,
                                   unsigned short bits_count)
{
    unsigned short i;

    if (bits_count == 0U) {
        return TIFX_ERR_BAD_FORMAT;
    }
    if (bits_count > TIFX_MAX_SAMPLES) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (bits_count != info->samples_per_pixel) {
        return TIFX_ERR_BAD_FORMAT;
    }

    info->bits_per_sample_count = bits_count;
    for (i = 0U; i < bits_count; ++i) {
        info->bits_per_sample[i] = bits_values[i];
    }
    return TIFX_OK;
}

static int tifx_validate_layout(tifx_image_info *info,
                                unsigned short sample_format_values_present,
                                const unsigned short *sample_format_values,
                                unsigned short sample_format_count)
{
    unsigned short bits0;
    unsigned long palette_entries;
    unsigned short i;

    if (sample_format_values_present) {
        if (sample_format_count == 0U || sample_format_count > info->samples_per_pixel) {
            return TIFX_ERR_BAD_FORMAT;
        }
        for (i = 0U; i < sample_format_count; ++i) {
            if (sample_format_values[i] != 1U) {
                return TIFX_ERR_UNSUPPORTED;
            }
        }
    }

    if (info->bits_per_sample_count == 0U) {
        return TIFX_ERR_BAD_FORMAT;
    }
    bits0 = info->bits_per_sample[0];

    if (info->extra_samples_count > info->samples_per_pixel) {
        return TIFX_ERR_BAD_FORMAT;
    }
    if (info->fill_order != 1U && info->fill_order != 2U) {
        return TIFX_ERR_UNSUPPORTED;
    }

    if (info->compression == 2U || info->compression == 3U || info->compression == 4U) {
        if ((info->photometric != 0U && info->photometric != 1U) ||
            info->samples_per_pixel != 1U ||
            bits0 != 1U) {
            return TIFX_ERR_UNSUPPORTED;
        }
    }
    if ((info->compression == 5U || info->compression == 8U || info->compression == 32946U) &&
        info->fill_order != 1U) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (info->compression == 3U && (info->t4_options & ~7UL) != 0UL) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (info->compression == 4U && (info->t6_options & ~2UL) != 0UL) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (info->predictor == 0U) {
        info->predictor = TIFX_PREDICTOR_NONE;
    }
    if (info->predictor != TIFX_PREDICTOR_NONE &&
        info->predictor != TIFX_PREDICTOR_HORIZONTAL) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (info->predictor != TIFX_PREDICTOR_NONE &&
        info->compression != 5U && info->compression != 8U && info->compression != 32946U) {
        return TIFX_ERR_UNSUPPORTED;
    }

    if (info->photometric == 0U || info->photometric == 1U) {
        if (info->samples_per_pixel != 1U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (bits0 != 1U && bits0 != 4U && bits0 != 8U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (info->predictor == TIFX_PREDICTOR_HORIZONTAL && bits0 != 8U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        info->pixel_format = TIFX_PIXEL_GRAY8;
        return TIFX_OK;
    }

    if (info->photometric == 3U) {
        if (info->samples_per_pixel != 1U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (bits0 != 4U && bits0 != 8U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (info->predictor != TIFX_PREDICTOR_NONE) {
            return TIFX_ERR_UNSUPPORTED;
        }
        palette_entries = 1UL << bits0;
        if ((unsigned long)info->color_map_count != (palette_entries * 3UL)) {
            return TIFX_ERR_BAD_FORMAT;
        }
        info->pixel_format = TIFX_PIXEL_RGB24;
        return TIFX_OK;
    }

    if (info->photometric == 2U) {
        if (info->samples_per_pixel != 3U && info->samples_per_pixel != 4U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (info->bits_per_sample_count < info->samples_per_pixel) {
            return TIFX_ERR_BAD_FORMAT;
        }
        for (i = 0U; i < info->samples_per_pixel; ++i) {
            if (info->bits_per_sample[i] != 8U) {
                return TIFX_ERR_UNSUPPORTED;
            }
        }
        if (info->predictor == TIFX_PREDICTOR_HORIZONTAL &&
            info->planar_config != 1U && info->planar_config != 2U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (info->samples_per_pixel == 4U &&
            (info->alpha_mode == TIFX_ALPHA_ASSOCIATED ||
             info->alpha_mode == TIFX_ALPHA_UNASSOCIATED)) {
            info->pixel_format = TIFX_PIXEL_RGBA32;
            return TIFX_OK;
        }
        info->pixel_format = TIFX_PIXEL_RGB24;
        return TIFX_OK;
    }

    return TIFX_ERR_UNSUPPORTED;
}


static int tifx_classic_read_header(const unsigned char *file_bytes,
                                    unsigned long size,
                                    int *out_is_big_endian,
                                    unsigned long *out_first_ifd_offset)
{
    int is_big_endian;

    if (file_bytes == 0 || out_is_big_endian == 0 || out_first_ifd_offset == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (size < 8UL) {
        return TIFX_ERR_TRUNCATED;
    }

    if (file_bytes[0] == 'I' && file_bytes[1] == 'I') {
        is_big_endian = 0;
    } else if (file_bytes[0] == 'M' && file_bytes[1] == 'M') {
        is_big_endian = 1;
    } else {
        return TIFX_ERR_BAD_FORMAT;
    }
    if (tifx_read_u16(file_bytes + 2, is_big_endian) != 42U) {
        return TIFX_ERR_BAD_FORMAT;
    }

    *out_is_big_endian = is_big_endian;
    *out_first_ifd_offset = tifx_read_u32(file_bytes + 4, is_big_endian);
    return TIFX_OK;
}

static int tifx_classic_read_ifd_trailer(const unsigned char *file_bytes,
                                         unsigned long size,
                                         int is_big_endian,
                                         unsigned long ifd_offset,
                                         unsigned long *out_tag_count,
                                         unsigned long *out_next_ifd_offset)
{
    unsigned long tag_count;
    unsigned long entry_table_bytes;
    unsigned long next_ifd_offset;

    if (file_bytes == 0 || out_tag_count == 0 || out_next_ifd_offset == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (ifd_offset > size || size - ifd_offset < 2UL) {
        return TIFX_ERR_TRUNCATED;
    }

    tag_count = (unsigned long)tifx_read_u16(file_bytes + ifd_offset, is_big_endian);
    if (!tifx_mul_ul(tag_count, 12UL, &entry_table_bytes)) {
        return TIFX_ERR_OVERFLOW;
    }
    if (!tifx_add_ul(entry_table_bytes, 6UL, &entry_table_bytes)) {
        return TIFX_ERR_OVERFLOW;
    }
    if (entry_table_bytes > size - ifd_offset) {
        return TIFX_ERR_TRUNCATED;
    }

    next_ifd_offset = tifx_read_u32(file_bytes + ifd_offset + 2UL + (tag_count * 12UL),
                                    is_big_endian);
    *out_tag_count = tag_count;
    *out_next_ifd_offset = next_ifd_offset;
    return TIFX_OK;
}

static int tifx_classic_walk_ifds(const unsigned char *file_bytes,
                                  unsigned long size,
                                  int is_big_endian,
                                  unsigned long first_ifd_offset,
                                  unsigned long max_offsets,
                                  unsigned long *out_offsets,
                                  unsigned long *out_count)
{
    unsigned long current_ifd_offset;
    unsigned long count;
    int rc;

    if (file_bytes == 0 || out_count == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    current_ifd_offset = first_ifd_offset;
    count = 0UL;
    while (current_ifd_offset != 0UL) {
        unsigned long tag_count;
        unsigned long next_ifd_offset;

        if (count >= max_offsets) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (out_offsets != 0) {
            out_offsets[count] = current_ifd_offset;
        }
        ++count;

        rc = tifx_classic_read_ifd_trailer(file_bytes,
                                           size,
                                           is_big_endian,
                                           current_ifd_offset,
                                           &tag_count,
                                           &next_ifd_offset);
        if (rc != TIFX_OK) {
            return rc;
        }
        (void)tag_count;
        if (next_ifd_offset == current_ifd_offset) {
            return TIFX_ERR_BAD_FORMAT;
        }
        current_ifd_offset = next_ifd_offset;
    }

    *out_count = count;
    return TIFX_OK;
}

static int tifx_classic_read_subifd_offsets(const unsigned char *file_bytes,
                                            unsigned long size,
                                            int is_big_endian,
                                            unsigned long ifd_offset,
                                            unsigned long *out_offsets,
                                            unsigned long max_offsets,
                                            unsigned long *out_count)
{
    unsigned long tag_count;
    unsigned long next_ifd_offset;
    unsigned long i;
    int rc;

    if (file_bytes == 0 || out_offsets == 0 || out_count == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    *out_count = 0UL;

    rc = tifx_classic_read_ifd_trailer(file_bytes,
                                       size,
                                       is_big_endian,
                                       ifd_offset,
                                       &tag_count,
                                       &next_ifd_offset);
    if (rc != TIFX_OK) {
        return rc;
    }
    (void)next_ifd_offset;

    for (i = 0UL; i < tag_count; ++i) {
        const unsigned char *entry;
        unsigned short tag;
        unsigned short type;
        unsigned long count;

        entry = file_bytes + ifd_offset + 2UL + (i * 12UL);
        tag = tifx_read_u16(entry, is_big_endian);
        if (tag != TIFX_TAG_SUBIFDS) {
            continue;
        }

        type = tifx_read_u16(entry + 2, is_big_endian);
        count = tifx_read_u32(entry + 4, is_big_endian);
        rc = tifx_entry_u32_array(file_bytes,
                                  size,
                                  is_big_endian,
                                  entry,
                                  type,
                                  count,
                                  out_offsets,
                                  max_offsets);
        if (rc != TIFX_OK) {
            return rc;
        }
        *out_count = count;

        if (count == 1UL && max_offsets > 1UL && out_offsets[0] != 0UL) {
            unsigned long chain_count;
            unsigned long current_child_offset;

            chain_count = 1UL;
            current_child_offset = out_offsets[0];
            while (current_child_offset != 0UL) {
                unsigned long child_tag_count;
                unsigned long child_next_ifd_offset;

                rc = tifx_classic_read_ifd_trailer(file_bytes,
                                                   size,
                                                   is_big_endian,
                                                   current_child_offset,
                                                   &child_tag_count,
                                                   &child_next_ifd_offset);
                if (rc != TIFX_OK) {
                    return rc;
                }
                (void)child_tag_count;
                if (child_next_ifd_offset == 0UL) {
                    break;
                }
                if (child_next_ifd_offset == current_child_offset) {
                    return TIFX_ERR_BAD_FORMAT;
                }
                if (chain_count >= max_offsets) {
                    return TIFX_ERR_UNSUPPORTED;
                }
                out_offsets[chain_count++] = child_next_ifd_offset;
                current_child_offset = child_next_ifd_offset;
            }
            *out_count = chain_count;
        }
        return TIFX_OK;
    }

    return TIFX_OK;
}

static int tifx_classic_walk_subifd_path(const unsigned char *file_bytes,
                                         unsigned long size,
                                         int is_big_endian,
                                         unsigned long first_ifd_offset,
                                         unsigned long page_index,
                                         const unsigned long *subifd_path,
                                         unsigned long subifd_path_length,
                                         unsigned long *out_page_count,
                                         unsigned long *out_parent_ifd_offset,
                                         unsigned long *out_current_ifd_offset,
                                         unsigned long *out_subifd_index)
{
    unsigned long page_offsets[TIFX_MAX_PAGES];
    unsigned long page_count;
    unsigned long current_ifd_offset;
    unsigned long level;
    int rc;

    if (file_bytes == 0 || out_page_count == 0 || out_parent_ifd_offset == 0 ||
        out_current_ifd_offset == 0 || out_subifd_index == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (subifd_path_length > 0UL && subifd_path == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    rc = tifx_classic_walk_ifds(file_bytes,
                                size,
                                is_big_endian,
                                first_ifd_offset,
                                TIFX_MAX_PAGES,
                                page_offsets,
                                &page_count);
    if (rc != TIFX_OK) {
        return rc;
    }
    if (page_index >= page_count) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    current_ifd_offset = page_offsets[page_index];
    *out_page_count = page_count;
    *out_parent_ifd_offset = 0UL;
    *out_subifd_index = 0UL;

    for (level = 0UL; level < subifd_path_length; ++level) {
        unsigned long sub_offsets[TIFX_MAX_SUBIFDS];
        unsigned long sub_count;
        unsigned long child_index;

        rc = tifx_classic_read_subifd_offsets(file_bytes,
                                              size,
                                              is_big_endian,
                                              current_ifd_offset,
                                              sub_offsets,
                                              TIFX_MAX_SUBIFDS,
                                              &sub_count);
        if (rc != TIFX_OK) {
            return rc;
        }
        child_index = subifd_path[level];
        if (child_index >= sub_count) {
            return TIFX_ERR_BAD_ARGUMENT;
        }

        *out_parent_ifd_offset = current_ifd_offset;
        current_ifd_offset = sub_offsets[child_index];
        *out_subifd_index = child_index;
    }

    *out_current_ifd_offset = current_ifd_offset;
    return TIFX_OK;
}

static int tifx_parse_classic_ifd(tifx_image_info *info,
                                  const void *data,
                                  unsigned long size,
                                  int is_big_endian,
                                  unsigned long first_ifd_offset,
                                  unsigned long parent_ifd_offset,
                                  unsigned long ifd_offset,
                                  unsigned long page_index,
                                  unsigned long page_count,
                                  unsigned long subifd_depth,
                                  unsigned long subifd_index)
{
    const unsigned char *file_bytes;
    unsigned long entry_table_bytes;
    unsigned long i;
    unsigned long tag_count;
    unsigned long offsets_count;
    unsigned long byte_counts_count;
    unsigned long tile_offsets_count;
    unsigned long tile_byte_counts_count;
    unsigned long strip_rows;
    unsigned long base_strips;
    unsigned long expected_strip_count;
    unsigned long next_ifd_offset;
    unsigned short bits_values[TIFX_MAX_SAMPLES];
    unsigned short sample_format_values[TIFX_MAX_SAMPLES];
    unsigned short extra_samples_values[TIFX_MAX_SAMPLES];
    unsigned short bits_count;
    unsigned short sample_format_count;
    unsigned short extra_samples_count;
    unsigned short have_width;
    unsigned short have_height;
    unsigned short have_photometric;
    unsigned short have_offsets;
    unsigned short have_byte_counts;
    unsigned short have_color_map;
    unsigned short have_sample_format;
    unsigned short have_extra_samples;
    unsigned short have_tile_width;
    unsigned short have_tile_length;
    unsigned short have_tile_offsets;
    unsigned short have_tile_byte_counts;
    int rc;

    if (info == 0 || data == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    tifx_image_info_init(info);
    file_bytes = (const unsigned char *)data;
    info->file_size = size;
    info->container_format = TIFX_CONTAINER_CLASSIC;
    info->is_big_endian = (unsigned short)(is_big_endian != 0);
    info->first_ifd_offset = first_ifd_offset;
    info->current_ifd_offset = ifd_offset;
    info->parent_ifd_offset = parent_ifd_offset;
    info->page_index = page_index;
    info->page_count = page_count;
    info->subifd_depth = subifd_depth;
    info->subifd_index = subifd_index;

    rc = tifx_classic_read_ifd_trailer(file_bytes,
                                       size,
                                       is_big_endian,
                                       ifd_offset,
                                       &tag_count,
                                       &next_ifd_offset);
    if (rc != TIFX_OK) {
        return rc;
    }
    info->next_ifd_offset = next_ifd_offset;

    if (!tifx_mul_ul(tag_count, 12UL, &entry_table_bytes)) {
        return TIFX_ERR_OVERFLOW;
    }
    if (!tifx_add_ul(entry_table_bytes, 6UL, &entry_table_bytes)) {
        return TIFX_ERR_OVERFLOW;
    }
    if (entry_table_bytes > size - ifd_offset) {
        return TIFX_ERR_TRUNCATED;
    }

    bits_count = 0U;
    sample_format_count = 0U;
    extra_samples_count = 0U;
    have_width = 0U;
    have_height = 0U;
    have_photometric = 0U;
    have_offsets = 0U;
    have_byte_counts = 0U;
    have_color_map = 0U;
    have_sample_format = 0U;
    have_extra_samples = 0U;
    have_tile_width = 0U;
    have_tile_length = 0U;
    have_tile_offsets = 0U;
    have_tile_byte_counts = 0U;
    offsets_count = 0UL;
    byte_counts_count = 0UL;
    tile_offsets_count = 0UL;
    tile_byte_counts_count = 0UL;

    for (i = 0UL; i < tag_count; ++i) {
        const unsigned char *entry;
        unsigned short tag;
        unsigned short type;
        unsigned long count;
        unsigned long scalar;

        entry = file_bytes + ifd_offset + 2UL + (i * 12UL);
        tag = tifx_read_u16(entry, is_big_endian);
        type = tifx_read_u16(entry + 2, is_big_endian);
        count = tifx_read_u32(entry + 4, is_big_endian);

        switch (tag) {
            case TIFX_TAG_NEW_SUBFILE_TYPE:
                rc = tifx_entry_scalar_u32(file_bytes, size, is_big_endian,
                                           entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->new_subfile_type = scalar;
                break;

            case TIFX_TAG_IMAGE_WIDTH:
                rc = tifx_entry_scalar_u32(file_bytes, size, is_big_endian,
                                           entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->width = scalar;
                have_width = 1U;
                break;

            case TIFX_TAG_IMAGE_LENGTH:
                rc = tifx_entry_scalar_u32(file_bytes, size, is_big_endian,
                                           entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->height = scalar;
                have_height = 1U;
                break;

            case TIFX_TAG_BITS_PER_SAMPLE:
                if (type != TIFX_TYPE_SHORT) {
                    return TIFX_ERR_UNSUPPORTED;
                }
                rc = tifx_entry_short_array(file_bytes, size, is_big_endian,
                                            entry, count, bits_values,
                                            (unsigned long)TIFX_MAX_SAMPLES);
                if (rc != TIFX_OK) return rc;
                bits_count = (unsigned short)count;
                break;

            case TIFX_TAG_COMPRESSION:
                rc = tifx_entry_scalar_u32(file_bytes, size, is_big_endian,
                                           entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->compression = (unsigned short)scalar;
                break;

            case TIFX_TAG_PHOTOMETRIC:
                rc = tifx_entry_scalar_u32(file_bytes, size, is_big_endian,
                                           entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->photometric = (unsigned short)scalar;
                have_photometric = 1U;
                break;

            case TIFX_TAG_FILL_ORDER:
                rc = tifx_entry_scalar_u32(file_bytes, size, is_big_endian,
                                           entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->fill_order = (unsigned short)scalar;
                break;

            case TIFX_TAG_STRIP_OFFSETS:
                rc = tifx_entry_u32_array(file_bytes, size, is_big_endian,
                                          entry, type, count,
                                          info->strip_offsets, TIFX_MAX_STRIPS);
                if (rc != TIFX_OK) return rc;
                offsets_count = count;
                have_offsets = 1U;
                break;

            case TIFX_TAG_ORIENTATION:
                rc = tifx_entry_scalar_u32(file_bytes, size, is_big_endian,
                                           entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->orientation = (unsigned short)scalar;
                break;

            case TIFX_TAG_SAMPLES_PER_PIXEL:
                rc = tifx_entry_scalar_u32(file_bytes, size, is_big_endian,
                                           entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->samples_per_pixel = (unsigned short)scalar;
                break;

            case TIFX_TAG_ROWS_PER_STRIP:
                rc = tifx_entry_scalar_u32(file_bytes, size, is_big_endian,
                                           entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->rows_per_strip = scalar;
                break;

            case TIFX_TAG_STRIP_BYTE_COUNTS:
                rc = tifx_entry_u32_array(file_bytes, size, is_big_endian,
                                          entry, type, count,
                                          info->strip_byte_counts, TIFX_MAX_STRIPS);
                if (rc != TIFX_OK) return rc;
                byte_counts_count = count;
                have_byte_counts = 1U;
                break;

            case TIFX_TAG_X_RESOLUTION:
                rc = tifx_entry_rational_fixed(file_bytes, size, is_big_endian,
                                               entry, &info->x_resolution);
                if (rc != TIFX_OK) return rc;
                break;

            case TIFX_TAG_Y_RESOLUTION:
                rc = tifx_entry_rational_fixed(file_bytes, size, is_big_endian,
                                               entry, &info->y_resolution);
                if (rc != TIFX_OK) return rc;
                break;

            case TIFX_TAG_T4_OPTIONS:
                rc = tifx_entry_scalar_u32(file_bytes, size, is_big_endian,
                                           entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->t4_options = scalar;
                break;

            case TIFX_TAG_T6_OPTIONS:
                rc = tifx_entry_scalar_u32(file_bytes, size, is_big_endian,
                                           entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->t6_options = scalar;
                break;

            case TIFX_TAG_PLANAR_CONFIGURATION:
                rc = tifx_entry_scalar_u32(file_bytes, size, is_big_endian,
                                           entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->planar_config = (unsigned short)scalar;
                break;

            case TIFX_TAG_RESOLUTION_UNIT:
                rc = tifx_entry_scalar_u32(file_bytes, size, is_big_endian,
                                           entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->resolution_unit = (unsigned short)scalar;
                break;

            case TIFX_TAG_PREDICTOR:
                rc = tifx_entry_scalar_u32(file_bytes, size, is_big_endian,
                                           entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->predictor = (unsigned short)scalar;
                break;

            case TIFX_TAG_PAGE_NUMBER:
                if (type != TIFX_TYPE_SHORT || count != 2UL) {
                    return TIFX_ERR_UNSUPPORTED;
                }
                rc = tifx_entry_short_array(file_bytes, size, is_big_endian,
                                            entry, 2UL, info->page_number, 2UL);
                if (rc != TIFX_OK) return rc;
                break;

            case TIFX_TAG_COLOR_MAP:
                if (type != TIFX_TYPE_SHORT) {
                    return TIFX_ERR_UNSUPPORTED;
                }
                rc = tifx_entry_short_array(file_bytes, size, is_big_endian,
                                            entry, count, info->color_map,
                                            (unsigned long)TIFX_MAX_COLORMAP_SHORTS);
                if (rc != TIFX_OK) return rc;
                info->color_map_count = (unsigned short)count;
                have_color_map = 1U;
                break;

            case TIFX_TAG_TILE_WIDTH:
                rc = tifx_entry_scalar_u32(file_bytes, size, is_big_endian,
                                           entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->tile_width = scalar;
                have_tile_width = 1U;
                break;

            case TIFX_TAG_TILE_LENGTH:
                rc = tifx_entry_scalar_u32(file_bytes, size, is_big_endian,
                                           entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->tile_length = scalar;
                have_tile_length = 1U;
                break;

            case TIFX_TAG_TILE_OFFSETS:
                rc = tifx_entry_u32_array(file_bytes, size, is_big_endian,
                                          entry, type, count,
                                          info->tile_offsets, TIFX_MAX_TILES);
                if (rc != TIFX_OK) return rc;
                tile_offsets_count = count;
                have_tile_offsets = 1U;
                break;

            case TIFX_TAG_TILE_BYTE_COUNTS:
                rc = tifx_entry_u32_array(file_bytes, size, is_big_endian,
                                          entry, type, count,
                                          info->tile_byte_counts, TIFX_MAX_TILES);
                if (rc != TIFX_OK) return rc;
                tile_byte_counts_count = count;
                have_tile_byte_counts = 1U;
                break;

            case TIFX_TAG_SUBIFDS:
                rc = tifx_classic_read_subifd_offsets(file_bytes,
                                                      size,
                                                      is_big_endian,
                                                      ifd_offset,
                                                      info->subifd_offsets,
                                                      TIFX_MAX_SUBIFDS,
                                                      &info->subifd_count);
                if (rc != TIFX_OK) return rc;
                break;

            case TIFX_TAG_SAMPLE_FORMAT:
                if (type != TIFX_TYPE_SHORT) {
                    return TIFX_ERR_UNSUPPORTED;
                }
                rc = tifx_entry_short_array(file_bytes, size, is_big_endian,
                                            entry, count, sample_format_values,
                                            (unsigned long)TIFX_MAX_SAMPLES);
                if (rc != TIFX_OK) return rc;
                sample_format_count = (unsigned short)count;
                have_sample_format = 1U;
                break;

            case TIFX_TAG_EXTRA_SAMPLES:
                if (type != TIFX_TYPE_SHORT) {
                    return TIFX_ERR_UNSUPPORTED;
                }
                rc = tifx_entry_short_array(file_bytes, size, is_big_endian,
                                            entry, count, extra_samples_values,
                                            (unsigned long)TIFX_MAX_SAMPLES);
                if (rc != TIFX_OK) return rc;
                extra_samples_count = (unsigned short)count;
                have_extra_samples = 1U;
                break;

            default:
                break;
        }
    }

    if (!have_width || !have_height || !have_photometric) {
        return TIFX_ERR_BAD_FORMAT;
    }
    if (info->width == 0UL || info->height == 0UL) {
        return TIFX_ERR_BAD_FORMAT;
    }
    if (info->samples_per_pixel == 0U || info->samples_per_pixel > TIFX_MAX_SAMPLES) {
        return TIFX_ERR_UNSUPPORTED;
    }

    if (bits_count == 0U) {
        if ((info->photometric == 0U || info->photometric == 1U) &&
            info->samples_per_pixel == 1U) {
            bits_values[0] = 1U;
            bits_count = 1U;
        } else {
            return TIFX_ERR_BAD_FORMAT;
        }
    }

    if (bits_count == 1U && info->samples_per_pixel > 1U) {
        unsigned short j;
        for (j = 1U; j < info->samples_per_pixel; ++j) {
            bits_values[j] = bits_values[0];
        }
        bits_count = info->samples_per_pixel;
    }

    rc = tifx_copy_bits_defaults(info, bits_values, bits_count);
    if (rc != TIFX_OK) {
        return rc;
    }

    if (have_extra_samples) {
        unsigned short j;
        info->extra_samples_count = extra_samples_count;
        for (j = 0U; j < extra_samples_count; ++j) {
            info->extra_samples[j] = extra_samples_values[j];
        }
        if (extra_samples_count > 0U &&
            (extra_samples_values[0] == TIFX_ALPHA_ASSOCIATED ||
             extra_samples_values[0] == TIFX_ALPHA_UNASSOCIATED)) {
            info->alpha_mode = extra_samples_values[0];
        }
    }

    if (info->photometric == 3U && !have_color_map) {
        return TIFX_ERR_BAD_FORMAT;
    }

    if (have_offsets || have_byte_counts) {
        if (!have_offsets || !have_byte_counts) {
            return TIFX_ERR_BAD_FORMAT;
        }
        if (have_tile_width || have_tile_length || have_tile_offsets || have_tile_byte_counts) {
            return TIFX_ERR_BAD_FORMAT;
        }
        if (offsets_count == 0UL || offsets_count != byte_counts_count) {
            return TIFX_ERR_BAD_FORMAT;
        }
        if (info->rows_per_strip == 0UL || info->rows_per_strip > info->height) {
            info->rows_per_strip = info->height;
        }
        strip_rows = info->rows_per_strip;
        base_strips = tifx_ceil_div(info->height, strip_rows);
        expected_strip_count = base_strips;
        if (info->planar_config == 2U) {
            if (!tifx_mul_ul(expected_strip_count,
                             (unsigned long)info->samples_per_pixel,
                             &expected_strip_count)) {
                return TIFX_ERR_OVERFLOW;
            }
        }
        if (offsets_count != expected_strip_count) {
            return TIFX_ERR_BAD_FORMAT;
        }
        info->strip_count = offsets_count;
        info->storage_layout = TIFX_LAYOUT_STRIPS;
    } else {
        unsigned long expected_tile_count;

        if (!have_tile_width || !have_tile_length || !have_tile_offsets || !have_tile_byte_counts) {
            return TIFX_ERR_BAD_FORMAT;
        }
        if (info->tile_width == 0UL || info->tile_length == 0UL) {
            return TIFX_ERR_BAD_FORMAT;
        }
        if (tile_offsets_count == 0UL || tile_offsets_count != tile_byte_counts_count) {
            return TIFX_ERR_BAD_FORMAT;
        }
        rc = tifx__compute_tile_count(info->width,
                                      info->height,
                                      info->tile_width,
                                      info->tile_length,
                                      &expected_tile_count);
        if (rc != TIFX_OK) {
            return rc;
        }
        if (info->planar_config == 2U) {
            if (!tifx_mul_ul(expected_tile_count,
                             (unsigned long)info->samples_per_pixel,
                             &expected_tile_count)) {
                return TIFX_ERR_OVERFLOW;
            }
        }
        if (tile_offsets_count != expected_tile_count) {
            return TIFX_ERR_BAD_FORMAT;
        }
        info->tile_count = tile_offsets_count;
        info->storage_layout = TIFX_LAYOUT_TILES;
    }

    rc = tifx_validate_layout(info,
                              have_sample_format,
                              sample_format_values,
                              sample_format_count);
    if (rc != TIFX_OK) {
        return rc;
    }

    return TIFX_OK;
}

int tifx_classic_page_count_memory(const void *data,
                                   unsigned long size,
                                   unsigned long *out_page_count)
{
    const unsigned char *file_bytes;
    int is_big_endian;
    unsigned long first_ifd_offset;
    int rc;

    if (data == 0 || out_page_count == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    file_bytes = (const unsigned char *)data;
    rc = tifx_classic_read_header(file_bytes, size, &is_big_endian, &first_ifd_offset);
    if (rc != TIFX_OK) {
        return rc;
    }
    return tifx_classic_walk_ifds(file_bytes,
                                  size,
                                  is_big_endian,
                                  first_ifd_offset,
                                  TIFX_MAX_PAGES,
                                  0,
                                  out_page_count);
}

int tifx_classic_subifd_count_memory(const void *data,
                                     unsigned long size,
                                     unsigned long page_index,
                                     const unsigned long *subifd_path,
                                     unsigned long subifd_path_length,
                                     unsigned long *out_subifd_count)
{
    const unsigned char *file_bytes;
    int is_big_endian;
    unsigned long first_ifd_offset;
    unsigned long page_count;
    unsigned long parent_ifd_offset;
    unsigned long current_ifd_offset;
    unsigned long subifd_index;
    unsigned long subifd_offsets[TIFX_MAX_SUBIFDS];
    int rc;

    if (data == 0 || out_subifd_count == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    *out_subifd_count = 0UL;
    if (subifd_path_length > TIFX_MAX_SUBIFD_DEPTH) {
        return TIFX_ERR_UNSUPPORTED;
    }

    file_bytes = (const unsigned char *)data;
    rc = tifx_classic_read_header(file_bytes, size, &is_big_endian, &first_ifd_offset);
    if (rc != TIFX_OK) {
        return rc;
    }
    rc = tifx_classic_walk_subifd_path(file_bytes,
                                       size,
                                       is_big_endian,
                                       first_ifd_offset,
                                       page_index,
                                       subifd_path,
                                       subifd_path_length,
                                       &page_count,
                                       &parent_ifd_offset,
                                       &current_ifd_offset,
                                       &subifd_index);
    if (rc != TIFX_OK) {
        return rc;
    }
    (void)page_count;
    (void)parent_ifd_offset;
    (void)subifd_index;
    return tifx_classic_read_subifd_offsets(file_bytes,
                                            size,
                                            is_big_endian,
                                            current_ifd_offset,
                                            subifd_offsets,
                                            TIFX_MAX_SUBIFDS,
                                            out_subifd_count);
}

int tifx_parse_classic_node_memory(tifx_image_info *info,
                                   const void *data,
                                   unsigned long size,
                                   unsigned long page_index,
                                   const unsigned long *subifd_path,
                                   unsigned long subifd_path_length)
{
    const unsigned char *file_bytes;
    int is_big_endian;
    unsigned long first_ifd_offset;
    unsigned long page_count;
    unsigned long parent_ifd_offset;
    unsigned long current_ifd_offset;
    unsigned long subifd_index;
    int rc;

    if (info == 0 || data == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (subifd_path_length > TIFX_MAX_SUBIFD_DEPTH) {
        return TIFX_ERR_UNSUPPORTED;
    }

    file_bytes = (const unsigned char *)data;
    rc = tifx_classic_read_header(file_bytes, size, &is_big_endian, &first_ifd_offset);
    if (rc != TIFX_OK) {
        return rc;
    }
    rc = tifx_classic_walk_subifd_path(file_bytes,
                                       size,
                                       is_big_endian,
                                       first_ifd_offset,
                                       page_index,
                                       subifd_path,
                                       subifd_path_length,
                                       &page_count,
                                       &parent_ifd_offset,
                                       &current_ifd_offset,
                                       &subifd_index);
    if (rc != TIFX_OK) {
        return rc;
    }
    return tifx_parse_classic_ifd(info,
                                  data,
                                  size,
                                  is_big_endian,
                                  first_ifd_offset,
                                  parent_ifd_offset,
                                  current_ifd_offset,
                                  page_index,
                                  page_count,
                                  subifd_path_length,
                                  subifd_index);
}

int tifx_parse_classic_page_memory(tifx_image_info *info,
                                   const void *data,
                                   unsigned long size,
                                   unsigned long page_index)
{
    return tifx_parse_classic_node_memory(info, data, size, page_index, 0, 0UL);
}

int tifx_parse_classic_memory(tifx_image_info *info, const void *data, unsigned long size)
{
    return tifx_parse_classic_page_memory(info, data, size, 0UL);
}

static int tifx_raw_row_bytes(const tifx_image_info *info, unsigned long *out_row_bytes)
{
    unsigned short bits;
    unsigned long row_bytes;

    if (info == 0 || out_row_bytes == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (info->bits_per_sample_count == 0U) {
        return TIFX_ERR_BAD_FORMAT;
    }

    bits = info->bits_per_sample[0];
    if (info->photometric == 0U || info->photometric == 1U) {
        if (bits == 1U) {
            row_bytes = tifx_ceil_div(info->width, 8UL);
        } else if (bits == 4U) {
            row_bytes = tifx_ceil_div(info->width, 2UL);
        } else if (bits == 8U) {
            row_bytes = info->width;
        } else {
            return TIFX_ERR_UNSUPPORTED;
        }
    } else if (info->photometric == 3U) {
        if (bits == 4U) {
            row_bytes = tifx_ceil_div(info->width, 2UL);
        } else if (bits == 8U) {
            row_bytes = info->width;
        } else {
            return TIFX_ERR_UNSUPPORTED;
        }
    } else if (info->photometric == 2U) {
        if (info->samples_per_pixel < 3U || info->samples_per_pixel > 4U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (info->planar_config == 2U) {
            row_bytes = info->width;
        } else if (!tifx_mul_ul(info->width, (unsigned long)info->samples_per_pixel, &row_bytes)) {
            return TIFX_ERR_OVERFLOW;
        }
    } else {
        return TIFX_ERR_UNSUPPORTED;
    }

    *out_row_bytes = row_bytes;
    return TIFX_OK;
}

static int tifx_decode_segment_row_bytes(const tifx_image_info *info, unsigned long *out_row_bytes)
{
    tifx_image_info copy;

    if (info == 0 || out_row_bytes == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    copy = *info;
    if (info->storage_layout == TIFX_LAYOUT_TILES && info->tile_width != 0UL) {
        copy.width = info->tile_width;
    }
    return tifx_raw_row_bytes(&copy, out_row_bytes);
}

unsigned long tifx_decode_workspace_size(const tifx_image_info *info)
{
    unsigned long row_bytes;
    int rc;

    rc = tifx_decode_segment_row_bytes(info, &row_bytes);
    if (rc != TIFX_OK) {
        return 0UL;
    }
    if (info->compression == 3U || info->compression == 4U) {
        if (row_bytes > ULONG_MAX / 2UL) {
            return 0UL;
        }
        return row_bytes * 2UL;
    }
    return row_bytes;
}

unsigned long tifx_decode_buffer_size(const tifx_image_info *info,
                                      unsigned long *out_stride)
{
    unsigned long stride;
    unsigned long total_size;
    unsigned long bytes_per_pixel;

    if (info == 0) {
        return 0UL;
    }

    if (info->pixel_format == TIFX_PIXEL_RGB24) {
        bytes_per_pixel = 3UL;
    } else if (info->pixel_format == TIFX_PIXEL_RGBA32) {
        bytes_per_pixel = 4UL;
    } else if (info->pixel_format == TIFX_PIXEL_GRAY8) {
        bytes_per_pixel = 1UL;
    } else {
        return 0UL;
    }
    if (!tifx_mul_ul(info->width, bytes_per_pixel, &stride)) {
        return 0UL;
    }
    if (!tifx_mul_ul(stride, info->height, &total_size)) {
        return 0UL;
    }
    if (out_stride != 0) {
        *out_stride = stride;
    }
    return total_size;
}

static int tifx_unpack_packbits_row(const unsigned char **src_io,
                                    const unsigned char *src_end,
                                    unsigned char *dst,
                                    unsigned long dst_len)
{
    const unsigned char *src;
    unsigned long out_pos;

    if (src_io == 0 || *src_io == 0 || src_end == 0 || dst == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    src = *src_io;
    out_pos = 0UL;

    while (out_pos < dst_len) {
        signed char n;
        unsigned long count;

        if (src >= src_end) {
            return TIFX_ERR_TRUNCATED;
        }

        n = (signed char)(*src++);
        if (n >= 0) {
            count = (unsigned long)n + 1UL;
            if (count > dst_len - out_pos) {
                return TIFX_ERR_BAD_FORMAT;
            }
            if (src_end < src || count > (unsigned long)(src_end - src)) {
                return TIFX_ERR_TRUNCATED;
            }
            memcpy(dst + out_pos, src, count);
            src += count;
            out_pos += count;
        } else if (n >= -127) {
            unsigned char value;
            count = (unsigned long)(1 - (int)n);
            if (count > dst_len - out_pos) {
                return TIFX_ERR_BAD_FORMAT;
            }
            if (src >= src_end) {
                return TIFX_ERR_TRUNCATED;
            }
            value = *src++;
            memset(dst + out_pos, value, count);
            out_pos += count;
        } else {
            /* no-op */
        }
    }

    *src_io = src;
    return TIFX_OK;
}


static void tifx_bit_reader_init(tifx_bit_reader *reader,
                                 const unsigned char *src,
                                 const unsigned char *src_end,
                                 unsigned short fill_order)
{
    reader->src = src;
    reader->src_end = src_end;
    reader->fill_order = fill_order;
    reader->bits_left = 0U;
    reader->current_byte = 0U;
}

static void tifx_bit_reader_align_byte(tifx_bit_reader *reader)
{
    reader->bits_left = 0U;
}

static int tifx_bit_reader_get_bit(tifx_bit_reader *reader, unsigned short *out_bit)
{
    if (reader == 0 || out_bit == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    if (reader->bits_left == 0U) {
        if (reader->src >= reader->src_end) {
            return TIFX_ERR_TRUNCATED;
        }
        reader->current_byte = *reader->src++;
        if (reader->fill_order == 2U) {
            reader->current_byte = tifx_reverse_byte(reader->current_byte);
        }
        reader->bits_left = 8U;
    }

    *out_bit = (unsigned short)((reader->current_byte >> 7) & 1U);
    reader->current_byte = (unsigned char)(reader->current_byte << 1);
    --reader->bits_left;
    return TIFX_OK;
}

static int tifx_bit_reader_get_bits(tifx_bit_reader *reader,
                                    unsigned short bit_count,
                                    unsigned long *out_value)
{
    unsigned long value;
    unsigned short i;
    int rc;

    if (reader == 0 || out_value == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (bit_count > 24U) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    value = 0UL;
    for (i = 0U; i < bit_count; ++i) {
        unsigned short bit;
        rc = tifx_bit_reader_get_bit(reader, &bit);
        if (rc != TIFX_OK) {
            return rc;
        }
        value = (value << 1) | (unsigned long)bit;
    }

    *out_value = value;
    return TIFX_OK;
}

static int tifx_bit_reader_peek_bits(const tifx_bit_reader *reader,
                                     unsigned short bit_count,
                                     unsigned long *out_value)
{
    tifx_bit_reader copy;

    if (reader == 0 || out_value == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    copy = *reader;
    return tifx_bit_reader_get_bits(&copy, bit_count, out_value);
}

static int tifx_bit_reader_skip_bits(tifx_bit_reader *reader,
                                     unsigned short bit_count)
{
    unsigned long ignored;
    return tifx_bit_reader_get_bits(reader, bit_count, &ignored);
}

static void tifx_bit_writer_init(tifx_bit_writer *writer,
                                 unsigned char *dst,
                                 unsigned long dst_size)
{
    if (writer == 0) {
        return;
    }
    writer->dst = dst;
    writer->dst_size = dst_size;
    writer->bit_count = 0UL;
}

static int tifx_bit_writer_put_bit(tifx_bit_writer *writer, unsigned short bit)
{
    unsigned long byte_index;
    unsigned int bit_index;

    if (writer == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    byte_index = writer->bit_count >> 3;
    if (writer->dst != 0) {
        if (byte_index >= writer->dst_size) {
            return TIFX_ERR_NO_SPACE;
        }
        bit_index = (unsigned int)(writer->bit_count & 7UL);
        if (bit != 0U) {
            writer->dst[byte_index] =
                (unsigned char)(writer->dst[byte_index] |
                                (unsigned char)(0x80U >> bit_index));
        }
    }

    ++writer->bit_count;
    return TIFX_OK;
}

static int tifx_bit_writer_put_bits(tifx_bit_writer *writer,
                                    unsigned long value,
                                    unsigned short bit_count)
{
    unsigned short i;
    int rc;

    if (writer == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    for (i = 0U; i < bit_count; ++i) {
        unsigned int shift;
        unsigned short bit;

        shift = (unsigned int)(bit_count - 1U - i);
        bit = (unsigned short)((value >> shift) & 1UL);
        rc = tifx_bit_writer_put_bit(writer, bit);
        if (rc != TIFX_OK) {
            return rc;
        }
    }
    return TIFX_OK;
}

static int tifx_bit_writer_pad_zero_to_byte(tifx_bit_writer *writer)
{
    while ((writer->bit_count & 7UL) != 0UL) {
        int rc;
        rc = tifx_bit_writer_put_bit(writer, 0U);
        if (rc != TIFX_OK) {
            return rc;
        }
    }
    return TIFX_OK;
}

static unsigned long tifx_bit_writer_bytes_used(const tifx_bit_writer *writer)
{
    if (writer == 0) {
        return 0UL;
    }
    return (writer->bit_count + 7UL) >> 3;
}

static int tifx_mh_lookup_code(const tifx_mh_code *codes,
                               unsigned long code_count,
                               unsigned short bit_length,
                               unsigned short code_word,
                               unsigned long *out_run)
{
    unsigned long i;

    if (codes == 0 || out_run == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    for (i = 0UL; i < code_count; ++i) {
        if (codes[i].bit_length > bit_length) {
            break;
        }
        if (codes[i].bit_length == bit_length &&
            codes[i].code_word == code_word) {
            *out_run = (unsigned long)codes[i].run_length;
            return TIFX_OK;
        }
    }

    return TIFX_ERR_BAD_FORMAT;
}

static int tifx_mh_decode_code(tifx_bit_reader *reader,
                               int is_black,
                               unsigned long *out_run)
{
    const tifx_mh_code *codes;
    unsigned long code_count;
    unsigned short code_word;
    unsigned short bit_length;
    int rc;

    if (reader == 0 || out_run == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    if (is_black) {
        codes = tifx_mh_black_codes;
        code_count = (unsigned long)(sizeof(tifx_mh_black_codes) / sizeof(tifx_mh_black_codes[0]));
    } else {
        codes = tifx_mh_white_codes;
        code_count = (unsigned long)(sizeof(tifx_mh_white_codes) / sizeof(tifx_mh_white_codes[0]));
    }

    code_word = 0U;
    for (bit_length = 1U; bit_length <= TIFX_MH_MAX_CODE_BITS; ++bit_length) {
        unsigned short bit;
        rc = tifx_bit_reader_get_bit(reader, &bit);
        if (rc != TIFX_OK) {
            return rc;
        }
        code_word = (unsigned short)((code_word << 1) | bit);

        rc = tifx_mh_lookup_code(codes, code_count, bit_length, code_word, out_run);
        if (rc == TIFX_OK) {
            return TIFX_OK;
        }
    }

    return TIFX_ERR_BAD_FORMAT;
}

static int tifx_mh_decode_run(tifx_bit_reader *reader,
                              int is_black,
                              unsigned long *out_run)
{
    unsigned long total_run;
    int rc;

    if (reader == 0 || out_run == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    total_run = 0UL;
    for (;;) {
        unsigned long piece;
        rc = tifx_mh_decode_code(reader, is_black, &piece);
        if (rc != TIFX_OK) {
            return rc;
        }
        if (!tifx_add_ul(total_run, piece, &total_run)) {
            return TIFX_ERR_OVERFLOW;
        }
        if (piece < 64UL) {
            *out_run = total_run;
            return TIFX_OK;
        }
    }
}

static void tifx_set_black_bit(unsigned char *dst, unsigned long bit_pos)
{
    dst[bit_pos >> 3] =
        (unsigned char)(dst[bit_pos >> 3] |
                        (unsigned char)(0x80U >> (unsigned int)(bit_pos & 7UL)));
}

static void tifx_set_black_run_bits(unsigned char *dst,
                                    unsigned long start,
                                    unsigned long length)
{
    unsigned long x;

    for (x = 0UL; x < length; ++x) {
        tifx_set_black_bit(dst, start + x);
    }
}

static int tifx_decode_mh_row_width(unsigned long width,
                                    tifx_bit_reader *reader,
                                    unsigned char *dst,
                                    unsigned long dst_len,
                                    int align_to_byte)
{
    unsigned long x;
    int is_black;
    int rc;

    if (reader == 0 || dst == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    memset(dst, 0, dst_len);
    x = 0UL;
    is_black = 0;

    while (x < width) {
        unsigned long run;

        rc = tifx_mh_decode_run(reader, is_black, &run);
        if (rc != TIFX_OK) {
            return rc;
        }
        if (run > width - x) {
            return TIFX_ERR_BAD_FORMAT;
        }

        if (is_black) {
            tifx_set_black_run_bits(dst, x, run);
        }

        x += run;
        is_black = !is_black;
    }

    if (align_to_byte) {
        tifx_bit_reader_align_byte(reader);
    }
    return TIFX_OK;
}

static int tifx_decode_mh_row(const tifx_image_info *info,
                              tifx_bit_reader *reader,
                              unsigned char *dst,
                              unsigned long dst_len)
{
    if (info == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    return tifx_decode_mh_row_width(info->width, reader, dst, dst_len, 1);
}

static int tifx_decode_uncompressed_segment(tifx_bit_reader *reader,
                                            unsigned char *dst,
                                            unsigned long width,
                                            unsigned long *io_x,
                                            int *out_next_is_black)
{
    unsigned long x;
    unsigned long zero_count;
    int rc;

    if (reader == 0 || dst == 0 || io_x == 0 || out_next_is_black == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    x = *io_x;
    zero_count = 0UL;

    for (;;) {
        unsigned long peek;

        rc = tifx_bit_reader_peek_bits(reader, 8U, &peek);
        if (rc == TIFX_OK && (peek & 0xFEUL) == 0x02UL) {
            rc = tifx_bit_reader_skip_bits(reader, 8U);
            if (rc != TIFX_OK) {
                return rc;
            }
            *io_x = x;
            *out_next_is_black = (peek & 1UL) ? 1 : 0;
            return TIFX_OK;
        }
        if (rc != TIFX_OK && rc != TIFX_ERR_TRUNCATED) {
            return rc;
        }

        {
            unsigned short bit;
            rc = tifx_bit_reader_get_bit(reader, &bit);
            if (rc != TIFX_OK) {
                return rc;
            }
            if (x >= width) {
                return TIFX_ERR_BAD_FORMAT;
            }
            if (bit != 0U) {
                tifx_set_black_bit(dst, x);
                zero_count = 0UL;
            } else {
                ++zero_count;
                if ((zero_count % 5UL) == 0UL) {
                    unsigned short stuffed_bit;
                    rc = tifx_bit_reader_get_bit(reader, &stuffed_bit);
                    if (rc != TIFX_OK) {
                        return rc;
                    }
                    if (stuffed_bit != 1U) {
                        return TIFX_ERR_BAD_FORMAT;
                    }
                }
            }
            ++x;
        }
    }
}

static int tifx_decode_t4_1d_row_width(unsigned long width,
                                       tifx_bit_reader *reader,
                                       unsigned char *dst,
                                       unsigned long dst_len,
                                       int allow_uncompressed)
{
    unsigned long x;
    int is_black;
    int rc;

    if (reader == 0 || dst == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    memset(dst, 0, dst_len);
    x = 0UL;
    is_black = 0;

    while (x < width) {
        if (allow_uncompressed) {
            unsigned long peek;
            rc = tifx_bit_reader_peek_bits(reader, 12U, &peek);
            if (rc == TIFX_OK && peek == 0x000FUL) {
                int next_is_black;
                rc = tifx_bit_reader_skip_bits(reader, 12U);
                if (rc != TIFX_OK) {
                    return rc;
                }
                rc = tifx_decode_uncompressed_segment(reader,
                                                      dst,
                                                      width,
                                                      &x,
                                                      &next_is_black);
                if (rc != TIFX_OK) {
                    return rc;
                }
                is_black = next_is_black;
                continue;
            }
            if (rc != TIFX_OK && rc != TIFX_ERR_TRUNCATED) {
                return rc;
            }
        }

        {
            unsigned long run;
            rc = tifx_mh_decode_run(reader, is_black, &run);
            if (rc != TIFX_OK) {
                return rc;
            }
            if (run > width - x) {
                return TIFX_ERR_BAD_FORMAT;
            }
            if (is_black) {
                tifx_set_black_run_bits(dst, x, run);
            }
            x += run;
            is_black = !is_black;
        }
    }

    return TIFX_OK;
}

static unsigned short tifx_get_bilevel_bit(const unsigned char *row,
                                           unsigned long width,
                                           unsigned long x)
{
    unsigned char packed;

    if (row == 0 || x >= width) {
        return 0U;
    }
    packed = row[x >> 3];
    return (unsigned short)((packed >> (7U - (unsigned int)(x & 7UL))) & 1U);
}

static unsigned long tifx_find_change_to_color(const unsigned char *row,
                                               unsigned long width,
                                               unsigned long start,
                                               unsigned short target_color)
{
    unsigned long x;
    unsigned short prev_color;
    unsigned short cur_color;

    if (row == 0) {
        return width;
    }
    if (start > width) {
        start = width;
    }

    prev_color = (start == 0UL) ? 0U : tifx_get_bilevel_bit(row, width, start - 1UL);
    cur_color = tifx_get_bilevel_bit(row, width, start);
    if (cur_color == target_color && prev_color != target_color) {
        return start;
    }

    for (x = start + 1UL; x <= width; ++x) {
        prev_color = tifx_get_bilevel_bit(row, width, x - 1UL);
        cur_color = tifx_get_bilevel_bit(row, width, x);
        if (cur_color == target_color && prev_color != target_color) {
            return x;
        }
    }

    return width;
}

static int tifx_emit_run_bits(unsigned char *dst,
                              unsigned long width,
                              unsigned long start,
                              unsigned long end,
                              int is_black)
{
    if (end < start || end > width) {
        return TIFX_ERR_BAD_FORMAT;
    }
    if (is_black && end > start) {
        tifx_set_black_run_bits(dst, start, end - start);
    }
    return TIFX_OK;
}

static int tifx_ccitt2d_read_mode(tifx_bit_reader *reader,
                                  int allow_uncompressed,
                                  int *out_mode,
                                  int *out_delta)
{
    unsigned short bit;
    int rc;

    if (reader == 0 || out_mode == 0 || out_delta == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    rc = tifx_bit_reader_get_bit(reader, &bit);
    if (rc != TIFX_OK) {
        return rc;
    }
    if (bit == 1U) {
        *out_mode = 0;
        *out_delta = 0;
        return TIFX_OK;
    }

    rc = tifx_bit_reader_get_bit(reader, &bit);
    if (rc != TIFX_OK) {
        return rc;
    }
    if (bit == 1U) {
        rc = tifx_bit_reader_get_bit(reader, &bit);
        if (rc != TIFX_OK) {
            return rc;
        }
        *out_mode = 0;
        *out_delta = (bit == 1U) ? 1 : -1;
        return TIFX_OK;
    }

    rc = tifx_bit_reader_get_bit(reader, &bit);
    if (rc != TIFX_OK) {
        return rc;
    }
    if (bit == 1U) {
        *out_mode = 1;
        *out_delta = 0;
        return TIFX_OK;
    }

    rc = tifx_bit_reader_get_bit(reader, &bit);
    if (rc != TIFX_OK) {
        return rc;
    }
    if (bit == 1U) {
        *out_mode = 2;
        *out_delta = 0;
        return TIFX_OK;
    }

    rc = tifx_bit_reader_get_bit(reader, &bit);
    if (rc != TIFX_OK) {
        return rc;
    }
    if (bit == 1U) {
        rc = tifx_bit_reader_get_bit(reader, &bit);
        if (rc != TIFX_OK) {
            return rc;
        }
        *out_mode = 0;
        *out_delta = (bit == 1U) ? 2 : -2;
        return TIFX_OK;
    }

    rc = tifx_bit_reader_get_bit(reader, &bit);
    if (rc != TIFX_OK) {
        return rc;
    }
    if (bit == 1U) {
        rc = tifx_bit_reader_get_bit(reader, &bit);
        if (rc != TIFX_OK) {
            return rc;
        }
        *out_mode = 0;
        *out_delta = (bit == 1U) ? 3 : -3;
        return TIFX_OK;
    }

    rc = tifx_bit_reader_get_bit(reader, &bit);
    if (rc != TIFX_OK) {
        return rc;
    }
    if (bit != 1U) {
        return TIFX_ERR_BAD_FORMAT;
    }
    rc = tifx_bit_reader_get_bit(reader, &bit);
    if (rc != TIFX_OK) {
        return rc;
    }
    if (bit != 1U) {
        return TIFX_ERR_BAD_FORMAT;
    }
    rc = tifx_bit_reader_get_bit(reader, &bit);
    if (rc != TIFX_OK) {
        return rc;
    }
    if (bit != 1U) {
        return TIFX_ERR_BAD_FORMAT;
    }
    rc = tifx_bit_reader_get_bit(reader, &bit);
    if (rc != TIFX_OK) {
        return rc;
    }
    if (bit != 1U) {
        return TIFX_ERR_BAD_FORMAT;
    }
    if (!allow_uncompressed) {
        return TIFX_ERR_UNSUPPORTED;
    }
    *out_mode = 3;
    *out_delta = 0;
    return TIFX_OK;
}

static int tifx_decode_2d_row(unsigned long width,
                              tifx_bit_reader *reader,
                              const unsigned char *reference_row,
                              unsigned char *dst,
                              unsigned long dst_len,
                              int allow_uncompressed)
{
    unsigned long a0;
    int is_black;
    int rc;
    unsigned short is_first;

    if (reader == 0 || dst == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    memset(dst, 0, dst_len);
    a0 = 0UL;
    is_black = 0;
    is_first = 1U;

    while (a0 < width) {
        unsigned long b1;
        unsigned long b2;
        int mode;
        int delta;

        b1 = tifx_find_change_to_color(reference_row,
                                       width,
                                       is_first ? a0 : ((a0 < width) ? (a0 + 1UL) : width),
                                       (unsigned short)(is_black ? 0U : 1U));
        b2 = tifx_find_change_to_color(reference_row,
                                       width,
                                       (b1 < width) ? (b1 + 1UL) : width,
                                       (unsigned short)(is_black ? 1U : 0U));

        rc = tifx_ccitt2d_read_mode(reader, allow_uncompressed, &mode, &delta);
        if (rc != TIFX_OK) {
            return rc;
        }

        if (mode == 2) {
            a0 = b2;
        } else if (mode == 0) {
            unsigned long a1;
            long signed_a1;

            signed_a1 = (long)b1 + (long)delta;
            if (signed_a1 < (long)a0 || signed_a1 < 0L || (unsigned long)signed_a1 > width) {
                return TIFX_ERR_BAD_FORMAT;
            }
            a1 = (unsigned long)signed_a1;
            rc = tifx_emit_run_bits(dst, width, a0, a1, is_black);
            if (rc != TIFX_OK) {
                return rc;
            }
            a0 = a1;
            is_black = !is_black;
        } else if (mode == 1) {
            unsigned long run1;
            unsigned long run2;
            unsigned long a1;
            unsigned long a2;

            rc = tifx_mh_decode_run(reader, is_black, &run1);
            if (rc != TIFX_OK) {
                return rc;
            }
            if (!tifx_add_ul(a0, run1, &a1) || a1 > width) {
                return TIFX_ERR_BAD_FORMAT;
            }
            rc = tifx_emit_run_bits(dst, width, a0, a1, is_black);
            if (rc != TIFX_OK) {
                return rc;
            }

            rc = tifx_mh_decode_run(reader, !is_black, &run2);
            if (rc != TIFX_OK) {
                return rc;
            }
            if (!tifx_add_ul(a1, run2, &a2) || a2 > width) {
                return TIFX_ERR_BAD_FORMAT;
            }
            rc = tifx_emit_run_bits(dst, width, a1, a2, !is_black);
            if (rc != TIFX_OK) {
                return rc;
            }
            a0 = a2;
        } else {
            int next_is_black;
            rc = tifx_decode_uncompressed_segment(reader,
                                                  dst,
                                                  width,
                                                  &a0,
                                                  &next_is_black);
            if (rc != TIFX_OK) {
                return rc;
            }
            is_black = next_is_black;
        }
        is_first = 0U;
    }

    return TIFX_OK;
}

static int tifx_t4_read_eol(tifx_bit_reader *reader)
{
    unsigned long zeros;
    unsigned short bit;
    int rc;

    if (reader == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    zeros = 0UL;
    for (;;) {
        rc = tifx_bit_reader_get_bit(reader, &bit);
        if (rc != TIFX_OK) {
            return rc;
        }
        if (bit == 0U) {
            ++zeros;
            continue;
        }
        if (zeros >= 11UL) {
            return TIFX_OK;
        }
        return TIFX_ERR_BAD_FORMAT;
    }
}

static int tifx_decode_t4_row(const tifx_image_info *info,
                              tifx_bit_reader *reader,
                              const unsigned char *reference_row,
                              unsigned char *dst,
                              unsigned long dst_len)
{
    unsigned short tag_bit;
    int rc;

    if (info == 0 || reader == 0 || reference_row == 0 || dst == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    rc = tifx_t4_read_eol(reader);
    if (rc != TIFX_OK) {
        return rc;
    }

    if ((info->t4_options & 1UL) != 0UL) {
        rc = tifx_bit_reader_get_bit(reader, &tag_bit);
        if (rc != TIFX_OK) {
            return rc;
        }
        if (tag_bit != 0U) {
            return tifx_decode_t4_1d_row_width(info->width, reader, dst, dst_len, (info->t4_options & 2UL) != 0UL);
        }
        return tifx_decode_2d_row(info->width, reader, reference_row, dst, dst_len, (info->t4_options & 2UL) != 0UL);
    }

    return tifx_decode_t4_1d_row_width(info->width, reader, dst, dst_len, (info->t4_options & 2UL) != 0UL);
}

static int tifx_decode_t6_row(const tifx_image_info *info,
                              tifx_bit_reader *reader,
                              const unsigned char *reference_row,
                              unsigned char *dst,
                              unsigned long dst_len)
{
    if (info == 0 || reader == 0 || reference_row == 0 || dst == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    return tifx_decode_2d_row(info->width, reader, reference_row, dst, dst_len, (info->t6_options & 2UL) != 0UL);
}

static void tifx_expand_bilevel_row_fill1(const tifx_image_info *info,
                                          const unsigned char *src,
                                          unsigned char *dst)
{
    unsigned long x;

    for (x = 0UL; x < info->width; ++x) {
        unsigned char packed;
        unsigned char bit;

        packed = src[x >> 3];
        bit = (unsigned char)((packed >> (7U - (unsigned int)(x & 7UL))) & 1U);
        if (info->photometric == 0U) {
            dst[x] = bit ? 0U : 255U;
        } else {
            dst[x] = bit ? 255U : 0U;
        }
    }
}

static void tifx_expand_gray_row(const tifx_image_info *info,
                                 const unsigned char *src,
                                 unsigned char *dst)
{
    unsigned long x;
    unsigned short bits;

    bits = info->bits_per_sample[0];

    if (bits == 8U) {
        if (info->photometric == 0U) {
            for (x = 0UL; x < info->width; ++x) {
                dst[x] = (unsigned char)(255U - src[x]);
            }
        } else {
            memcpy(dst, src, info->width);
        }
        return;
    }

    if (bits == 4U) {
        for (x = 0UL; x < info->width; ++x) {
            unsigned char packed;
            unsigned char nibble;
            packed = src[x >> 1];
            if ((x & 1UL) == 0UL) {
                nibble = (unsigned char)((packed >> 4) & 0x0FU);
            } else {
                nibble = (unsigned char)(packed & 0x0FU);
            }
            nibble = (unsigned char)(nibble * 17U);
            dst[x] = (info->photometric == 0U) ? (unsigned char)(255U - nibble) : nibble;
        }
        return;
    }

    for (x = 0UL; x < info->width; ++x) {
        unsigned char packed;
        unsigned char bit;
        packed = src[x >> 3];
        if (info->fill_order == 2U) {
            packed = tifx_reverse_byte(packed);
        }
        bit = (unsigned char)((packed >> (7U - (unsigned int)(x & 7UL))) & 1U);
        if (info->photometric == 0U) {
            dst[x] = bit ? 0U : 255U;
        } else {
            dst[x] = bit ? 255U : 0U;
        }
    }
}

static void tifx_expand_palette_row(const tifx_image_info *info,
                                    const unsigned char *src,
                                    unsigned char *dst)
{
    unsigned long x;
    unsigned long entries;
    unsigned short bits;

    bits = info->bits_per_sample[0];
    entries = 1UL << bits;

    for (x = 0UL; x < info->width; ++x) {
        unsigned long index;
        unsigned char r;
        unsigned char g;
        unsigned char b;

        if (bits == 8U) {
            index = (unsigned long)src[x];
        } else {
            unsigned char packed;
            packed = src[x >> 1];
            if ((x & 1UL) == 0UL) {
                index = (unsigned long)((packed >> 4) & 0x0FU);
            } else {
                index = (unsigned long)(packed & 0x0FU);
            }
        }

        if (index >= entries) {
            index = 0UL;
        }

        r = (unsigned char)(info->color_map[index] >> 8);
        g = (unsigned char)(info->color_map[index + entries] >> 8);
        b = (unsigned char)(info->color_map[index + (entries * 2UL)] >> 8);

        dst[x * 3UL + 0UL] = r;
        dst[x * 3UL + 1UL] = g;
        dst[x * 3UL + 2UL] = b;
    }
}

static void tifx_expand_rgb_row(const tifx_image_info *info,
                                const unsigned char *src,
                                unsigned char *dst)
{
    unsigned long x;
    unsigned long samples;

    samples = (unsigned long)info->samples_per_pixel;
    if (samples == 3UL) {
        memcpy(dst, src, info->width * 3UL);
        return;
    }

    for (x = 0UL; x < info->width; ++x) {
        dst[x * 3UL + 0UL] = src[x * samples + 0UL];
        dst[x * 3UL + 1UL] = src[x * samples + 1UL];
        dst[x * 3UL + 2UL] = src[x * samples + 2UL];
    }
}

static void tifx_expand_rgba_row(const tifx_image_info *info,
                                 const unsigned char *src,
                                 unsigned char *dst)
{
    unsigned long x;
    unsigned long samples;

    samples = (unsigned long)info->samples_per_pixel;
    if (samples == 4UL) {
        memcpy(dst, src, info->width * 4UL);
        return;
    }

    for (x = 0UL; x < info->width; ++x) {
        dst[x * 4UL + 0UL] = src[x * samples + 0UL];
        dst[x * 4UL + 1UL] = src[x * samples + 1UL];
        dst[x * 4UL + 2UL] = src[x * samples + 2UL];
        dst[x * 4UL + 3UL] = 255U;
    }
}

static void tifx_store_planar_row(const tifx_image_info *info,
                                  unsigned long plane_index,
                                  const unsigned char *src,
                                  unsigned char *dst,
                                  unsigned long width)
{
    unsigned long x;
    unsigned long dst_samples;

    if (info == 0 || src == 0 || dst == 0) {
        return;
    }
    dst_samples = (info->pixel_format == TIFX_PIXEL_RGBA32) ? 4UL : 3UL;
    if (plane_index >= dst_samples) {
        return;
    }
    for (x = 0UL; x < width; ++x) {
        dst[(x * dst_samples) + plane_index] = src[x];
    }
}

int tifx_decode_memory(const tifx_image_info *info,
                       const void *data,
                       unsigned long size,
                       void *dst,
                       unsigned long dst_size,
                       unsigned long dst_stride,
                       void *workspace,
                       unsigned long workspace_size)
{
    const unsigned char *file_bytes;
    unsigned char *dst_bytes;
    unsigned char *row_workspace;
    unsigned char *prev_row;
    unsigned long min_stride;
    unsigned long total_size;
    unsigned long raw_row_bytes;
    unsigned long segment_row_bytes;
    unsigned long required_workspace;
    unsigned long row_index;
    unsigned long strip_index;
    int rc;

    if (info == 0 || data == 0 || dst == 0 || workspace == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (size != info->file_size) {
        if (size < info->file_size) {
            return TIFX_ERR_TRUNCATED;
        }
    }
    if (info->compression != 1U && info->compression != 2U &&
        info->compression != 3U && info->compression != 4U &&
        info->compression != 5U && info->compression != 8U &&
        info->compression != 32946U && info->compression != 32773U) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (info->planar_config != 1U && info->planar_config != 2U) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (info->planar_config == 2U) {
        if (info->photometric != 2U ||
            (info->samples_per_pixel != 3U && info->samples_per_pixel != 4U) ||
            (info->compression != 1U && info->compression != 32773U &&
             info->compression != 5U && info->compression != 8U &&
             info->compression != 32946U)) {
            return TIFX_ERR_UNSUPPORTED;
        }
    }
    if (info->orientation != 1U) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if ((info->bits_per_sample[0] != 1U) && info->fill_order != 1U) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if ((info->compression == 5U || info->compression == 8U || info->compression == 32946U) &&
        info->fill_order != 1U) {
        return TIFX_ERR_UNSUPPORTED;
    }

    total_size = tifx_decode_buffer_size(info, &min_stride);
    if (total_size == 0UL) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (dst_stride < min_stride) {
        return TIFX_ERR_NO_SPACE;
    }
    if (!tifx_mul_ul(dst_stride, info->height, &total_size)) {
        return TIFX_ERR_OVERFLOW;
    }
    if (dst_size < total_size) {
        return TIFX_ERR_NO_SPACE;
    }

    rc = tifx_raw_row_bytes(info, &raw_row_bytes);
    if (rc != TIFX_OK) {
        return rc;
    }
    rc = tifx_decode_segment_row_bytes(info, &segment_row_bytes);
    if (rc != TIFX_OK) {
        return rc;
    }
    required_workspace = segment_row_bytes;
    if (info->compression == 3U || info->compression == 4U) {
        if (!tifx_add_ul(segment_row_bytes, segment_row_bytes, &required_workspace)) {
            return TIFX_ERR_OVERFLOW;
        }
    }
    if (workspace_size < required_workspace) {
        return TIFX_ERR_NO_SPACE;
    }

    file_bytes = (const unsigned char *)data;
    dst_bytes = (unsigned char *)dst;
    row_workspace = (unsigned char *)workspace;
    prev_row = (unsigned char *)workspace;
    if (info->compression == 3U || info->compression == 4U) {
        prev_row = row_workspace + segment_row_bytes;
    }
    if (info->planar_config == 2U) {
        memset(dst_bytes, 0, total_size);
    }
    row_index = 0UL;

    if (info->storage_layout == TIFX_LAYOUT_TILES) {
        unsigned long tiles_across;
        unsigned long tile_index;
        unsigned long dst_bytes_per_pixel;
        unsigned long base_tile_count;

        if (info->tile_width == 0UL || info->tile_length == 0UL) {
            return TIFX_ERR_BAD_FORMAT;
        }
        if (info->compression != 1U && info->compression != 32773U &&
            info->compression != 5U && info->compression != 8U &&
            info->compression != 32946U) {
            return TIFX_ERR_UNSUPPORTED;
        }

        tiles_across = tifx_ceil_div(info->width, info->tile_width);
        base_tile_count = info->tile_count;
        if (info->planar_config == 2U) {
            if (info->samples_per_pixel == 0U ||
                (info->tile_count % (unsigned long)info->samples_per_pixel) != 0UL) {
                return TIFX_ERR_BAD_FORMAT;
            }
            base_tile_count = info->tile_count / (unsigned long)info->samples_per_pixel;
        }

        if (info->pixel_format == TIFX_PIXEL_RGB24) {
            dst_bytes_per_pixel = 3UL;
        } else if (info->pixel_format == TIFX_PIXEL_RGBA32) {
            dst_bytes_per_pixel = 4UL;
        } else {
            dst_bytes_per_pixel = 1UL;
        }

        for (tile_index = 0UL; tile_index < info->tile_count; ++tile_index) {
            unsigned long tile_offset;
            unsigned long tile_size;
            const unsigned char *src;
            const unsigned char *src_end;
            unsigned long plane_index;
            unsigned long base_tile_index;
            unsigned long tile_x_index;
            unsigned long tile_y_index;
            unsigned long base_x;
            unsigned long base_y;
            unsigned long visible_width;
            unsigned long visible_height;
            unsigned long row_in_tile;
            tifx_image_info row_info;

            tile_offset = info->tile_offsets[tile_index];
            tile_size = info->tile_byte_counts[tile_index];
            if (tile_offset > size || tile_size > size - tile_offset) {
                return TIFX_ERR_TRUNCATED;
            }

            plane_index = 0UL;
            base_tile_index = tile_index;
            if (info->planar_config == 2U) {
                plane_index = tile_index / base_tile_count;
                base_tile_index = tile_index % base_tile_count;
                if (plane_index >= (unsigned long)info->samples_per_pixel) {
                    return TIFX_ERR_BAD_FORMAT;
                }
            }

            tile_x_index = base_tile_index % tiles_across;
            tile_y_index = base_tile_index / tiles_across;
            base_x = tile_x_index * info->tile_width;
            base_y = tile_y_index * info->tile_length;
            if (base_x >= info->width || base_y >= info->height) {
                return TIFX_ERR_BAD_FORMAT;
            }
            visible_width = info->tile_width;
            if (visible_width > info->width - base_x) {
                visible_width = info->width - base_x;
            }
            visible_height = info->tile_length;
            if (visible_height > info->height - base_y) {
                visible_height = info->height - base_y;
            }

            src = file_bytes + tile_offset;
            src_end = src + tile_size;
            row_info = *info;
            row_info.width = visible_width;

            {
                tifx_lzw_decoder lzw_decoder;
                tifx_zlib_stored_decoder zlib_decoder;
                int use_lzw;
                int use_zlib;

                use_lzw = (info->compression == 5U);
                use_zlib = (info->compression == 8U || info->compression == 32946U);
                if (use_lzw) {
                    tifx_lzw_decoder_init(&lzw_decoder, src, src_end);
                }
                if (use_zlib) {
                    rc = tifx_zlib_stored_decoder_init(&zlib_decoder, src, src_end);
                    if (rc != TIFX_OK) {
                        return rc;
                    }
                }

                for (row_in_tile = 0UL; row_in_tile < info->tile_length; ++row_in_tile) {
                    if (info->compression == 1U) {
                        if (src_end < src || segment_row_bytes > (unsigned long)(src_end - src)) {
                            return TIFX_ERR_TRUNCATED;
                        }
                        memcpy(row_workspace, src, segment_row_bytes);
                        src += segment_row_bytes;
                    } else if (info->compression == 32773U) {
                        rc = tifx_unpack_packbits_row(&src, src_end, row_workspace, segment_row_bytes);
                        if (rc != TIFX_OK) {
                            return rc;
                        }
                    } else if (use_lzw) {
                        rc = tifx_lzw_decoder_read(&lzw_decoder, row_workspace, segment_row_bytes);
                        if (rc != TIFX_OK) {
                            return rc;
                        }
                    } else {
                        rc = tifx_zlib_stored_decoder_read(&zlib_decoder, row_workspace, segment_row_bytes);
                        if (rc != TIFX_OK) {
                            return rc;
                        }
                    }
                    if (info->predictor == TIFX_PREDICTOR_HORIZONTAL) {
                        rc = tifx_apply_horizontal_predictor_inverse(info,
                                                                     row_workspace,
                                                                     segment_row_bytes);
                        if (rc != TIFX_OK) {
                            return rc;
                        }
                    }

                    if (row_in_tile < visible_height) {
                        unsigned char *dst_row;

                        dst_row = dst_bytes + ((base_y + row_in_tile) * dst_stride) +
                                  (base_x * dst_bytes_per_pixel);
                        if (info->planar_config == 2U) {
                            tifx_store_planar_row(&row_info, plane_index, row_workspace, dst_row, visible_width);
                        } else if (info->photometric == 0U || info->photometric == 1U) {
                            tifx_expand_gray_row(&row_info, row_workspace, dst_row);
                        } else if (info->photometric == 3U) {
                            tifx_expand_palette_row(&row_info, row_workspace, dst_row);
                        } else if (info->pixel_format == TIFX_PIXEL_RGBA32) {
                            tifx_expand_rgba_row(&row_info, row_workspace, dst_row);
                        } else if (info->photometric == 2U) {
                            tifx_expand_rgb_row(&row_info, row_workspace, dst_row);
                        } else {
                            return TIFX_ERR_UNSUPPORTED;
                        }
                    }
                }
                if (use_zlib) {
                    rc = tifx_zlib_stored_decoder_finish(&zlib_decoder);
                    if (rc != TIFX_OK) {
                        return rc;
                    }
                }
            }
        }
        return TIFX_OK;
    }

    if (info->planar_config == 2U) {
        unsigned long base_strip_count;

        if (info->samples_per_pixel == 0U ||
            (info->strip_count % (unsigned long)info->samples_per_pixel) != 0UL) {
            return TIFX_ERR_BAD_FORMAT;
        }
        base_strip_count = info->strip_count / (unsigned long)info->samples_per_pixel;

        for (strip_index = 0UL; strip_index < info->strip_count; ++strip_index) {
            unsigned long strip_offset;
            unsigned long strip_size;
            const unsigned char *src;
            const unsigned char *src_end;
            unsigned long plane_index;
            unsigned long strip_in_plane;
            unsigned long start_row;
            unsigned long rows_remaining;
            unsigned long rows_this_strip;
            unsigned long row_in_strip;

            strip_offset = info->strip_offsets[strip_index];
            strip_size = info->strip_byte_counts[strip_index];
            if (strip_offset > size || strip_size > size - strip_offset) {
                return TIFX_ERR_TRUNCATED;
            }

            plane_index = strip_index / base_strip_count;
            strip_in_plane = strip_index % base_strip_count;
            if (plane_index >= (unsigned long)info->samples_per_pixel) {
                return TIFX_ERR_BAD_FORMAT;
            }
            start_row = strip_in_plane * info->rows_per_strip;
            if (start_row >= info->height) {
                return TIFX_ERR_BAD_FORMAT;
            }
            rows_remaining = info->height - start_row;
            rows_this_strip = info->rows_per_strip;
            if (rows_this_strip > rows_remaining) {
                rows_this_strip = rows_remaining;
            }

            src = file_bytes + strip_offset;
            src_end = src + strip_size;
            {
                tifx_lzw_decoder lzw_decoder;
                tifx_zlib_stored_decoder zlib_decoder;
                int use_lzw;
                int use_zlib;

                use_lzw = (info->compression == 5U);
                use_zlib = (info->compression == 8U || info->compression == 32946U);
                if (use_lzw) {
                    tifx_lzw_decoder_init(&lzw_decoder, src, src_end);
                }
                if (use_zlib) {
                    rc = tifx_zlib_stored_decoder_init(&zlib_decoder, src, src_end);
                    if (rc != TIFX_OK) {
                        return rc;
                    }
                }
                for (row_in_strip = 0UL; row_in_strip < rows_this_strip; ++row_in_strip) {
                    unsigned char *dst_row;

                    if (info->compression == 1U) {
                        if (src_end < src || raw_row_bytes > (unsigned long)(src_end - src)) {
                            return TIFX_ERR_TRUNCATED;
                        }
                        memcpy(row_workspace, src, raw_row_bytes);
                        src += raw_row_bytes;
                    } else if (info->compression == 32773U) {
                        rc = tifx_unpack_packbits_row(&src, src_end, row_workspace, raw_row_bytes);
                        if (rc != TIFX_OK) {
                            return rc;
                        }
                    } else if (use_lzw) {
                        rc = tifx_lzw_decoder_read(&lzw_decoder, row_workspace, raw_row_bytes);
                        if (rc != TIFX_OK) {
                            return rc;
                        }
                    } else {
                        rc = tifx_zlib_stored_decoder_read(&zlib_decoder, row_workspace, raw_row_bytes);
                        if (rc != TIFX_OK) {
                            return rc;
                        }
                    }
                    if (info->predictor == TIFX_PREDICTOR_HORIZONTAL) {
                        rc = tifx_apply_horizontal_predictor_inverse(info,
                                                                     row_workspace,
                                                                     raw_row_bytes);
                        if (rc != TIFX_OK) {
                            return rc;
                        }
                    }

                    dst_row = dst_bytes + ((start_row + row_in_strip) * dst_stride);
                    tifx_store_planar_row(info, plane_index, row_workspace, dst_row, info->width);
                }
                if (use_zlib) {
                    rc = tifx_zlib_stored_decoder_finish(&zlib_decoder);
                    if (rc != TIFX_OK) {
                        return rc;
                    }
                }
            }
        }
        return TIFX_OK;
    }

    for (strip_index = 0UL; strip_index < info->strip_count; ++strip_index) {
        unsigned long strip_offset;
        unsigned long strip_size;
        const unsigned char *src;
        const unsigned char *src_end;
        tifx_bit_reader bit_reader;
        unsigned long rows_this_strip;
        unsigned long rows_remaining;
        unsigned long row_in_strip;

        strip_offset = info->strip_offsets[strip_index];
        strip_size = info->strip_byte_counts[strip_index];
        if (strip_offset > size || strip_size > size - strip_offset) {
            return TIFX_ERR_TRUNCATED;
        }

        rows_remaining = info->height - row_index;
        rows_this_strip = info->rows_per_strip;
        if (rows_this_strip > rows_remaining) {
            rows_this_strip = rows_remaining;
        }

        src = file_bytes + strip_offset;
        src_end = src + strip_size;
        tifx_bit_reader_init(&bit_reader, src, src_end, info->fill_order);
        if (info->compression == 3U || info->compression == 4U) {
            memset(prev_row, 0, raw_row_bytes);
        }

        {
            tifx_lzw_decoder lzw_decoder;
            tifx_zlib_stored_decoder zlib_decoder;
            int use_lzw;
            int use_zlib;

            use_lzw = (info->compression == 5U);
            use_zlib = (info->compression == 8U || info->compression == 32946U);
            if (use_lzw) {
                tifx_lzw_decoder_init(&lzw_decoder, src, src_end);
            }
            if (use_zlib) {
                rc = tifx_zlib_stored_decoder_init(&zlib_decoder, src, src_end);
                if (rc != TIFX_OK) {
                    return rc;
                }
            }

            for (row_in_strip = 0UL; row_in_strip < rows_this_strip; ++row_in_strip) {
                unsigned char *dst_row;

                if (info->compression == 1U) {
                    if (src_end < src || raw_row_bytes > (unsigned long)(src_end - src)) {
                        return TIFX_ERR_TRUNCATED;
                    }
                    memcpy(row_workspace, src, raw_row_bytes);
                    src += raw_row_bytes;
                } else if (info->compression == 32773U) {
                    rc = tifx_unpack_packbits_row(&src, src_end, row_workspace, raw_row_bytes);
                    if (rc != TIFX_OK) {
                        return rc;
                    }
                } else if (use_lzw) {
                    rc = tifx_lzw_decoder_read(&lzw_decoder, row_workspace, raw_row_bytes);
                    if (rc != TIFX_OK) {
                        return rc;
                    }
                } else if (use_zlib) {
                    rc = tifx_zlib_stored_decoder_read(&zlib_decoder, row_workspace, raw_row_bytes);
                    if (rc != TIFX_OK) {
                        return rc;
                    }
                } else if (info->compression == 2U) {
                    rc = tifx_decode_mh_row(info, &bit_reader, row_workspace, raw_row_bytes);
                    if (rc != TIFX_OK) {
                        return rc;
                    }
                } else if (info->compression == 3U) {
                    rc = tifx_decode_t4_row(info, &bit_reader, prev_row, row_workspace, raw_row_bytes);
                    if (rc != TIFX_OK) {
                        return rc;
                    }
                    memcpy(prev_row, row_workspace, raw_row_bytes);
                } else {
                    rc = tifx_decode_t6_row(info, &bit_reader, prev_row, row_workspace, raw_row_bytes);
                    if (rc != TIFX_OK) {
                        return rc;
                    }
                    memcpy(prev_row, row_workspace, raw_row_bytes);
                }

                if (info->predictor == TIFX_PREDICTOR_HORIZONTAL) {
                    rc = tifx_apply_horizontal_predictor_inverse(info,
                                                                 row_workspace,
                                                                 raw_row_bytes);
                    if (rc != TIFX_OK) {
                        return rc;
                    }
                }

                dst_row = dst_bytes + (row_index * dst_stride);
                if ((info->photometric == 0U || info->photometric == 1U) &&
                    (info->compression == 2U || info->compression == 3U || info->compression == 4U)) {
                    tifx_expand_bilevel_row_fill1(info, row_workspace, dst_row);
                } else if (info->photometric == 0U || info->photometric == 1U) {
                    tifx_expand_gray_row(info, row_workspace, dst_row);
                } else if (info->photometric == 3U) {
                    tifx_expand_palette_row(info, row_workspace, dst_row);
                } else if (info->pixel_format == TIFX_PIXEL_RGBA32) {
                    tifx_expand_rgba_row(info, row_workspace, dst_row);
                } else if (info->photometric == 2U) {
                    tifx_expand_rgb_row(info, row_workspace, dst_row);
                } else {
                    return TIFX_ERR_UNSUPPORTED;
                }
                ++row_index;
            }
            if (use_zlib) {
                rc = tifx_zlib_stored_decoder_finish(&zlib_decoder);
                if (rc != TIFX_OK) {
                    return rc;
                }
            }
        }

    }
    if (row_index != info->height) {
        return TIFX_ERR_BAD_FORMAT;
    }

    return TIFX_OK;
}

static int tifx_write_tag_le(unsigned char *entry,
                             unsigned short tag,
                             unsigned short type,
                             unsigned long count,
                             unsigned long value_or_offset)
{
    tifx_write_u16le(entry + 0, tag);
    tifx_write_u16le(entry + 2, type);
    tifx_write_u32le(entry + 4, count);

    if (type == TIFX_TYPE_SHORT && count == 1UL) {
        tifx_write_u16le(entry + 8, (unsigned short)value_or_offset);
        tifx_write_u16le(entry + 10, 0U);
    } else {
        if (value_or_offset > 0xFFFFFFFFUL) {
            return TIFX_ERR_OVERFLOW;
        }
        tifx_write_u32le(entry + 8, value_or_offset);
    }
    return TIFX_OK;
}

static int tifx_mh_lookup_run_code(const tifx_mh_code *codes,
                                  unsigned long code_count,
                                  unsigned long run_length,
                                  unsigned short *out_bit_length,
                                  unsigned short *out_code_word)
{
    unsigned long i;

    if (codes == 0 || out_bit_length == 0 || out_code_word == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    for (i = 0UL; i < code_count; ++i) {
        if ((unsigned long)codes[i].run_length == run_length) {
            *out_bit_length = codes[i].bit_length;
            *out_code_word = codes[i].code_word;
            return TIFX_OK;
        }
    }

    return TIFX_ERR_BAD_ARGUMENT;
}

static int tifx_mh_write_piece(tifx_bit_writer *writer,
                               int is_black,
                               unsigned long run_length)
{
    const tifx_mh_code *codes;
    unsigned long code_count;
    unsigned short bit_length;
    unsigned short code_word;
    int rc;

    if (writer == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    if (is_black) {
        codes = tifx_mh_black_codes;
        code_count = (unsigned long)(sizeof(tifx_mh_black_codes) / sizeof(tifx_mh_black_codes[0]));
    } else {
        codes = tifx_mh_white_codes;
        code_count = (unsigned long)(sizeof(tifx_mh_white_codes) / sizeof(tifx_mh_white_codes[0]));
    }

    rc = tifx_mh_lookup_run_code(codes, code_count, run_length, &bit_length, &code_word);
    if (rc != TIFX_OK) {
        return rc;
    }
    return tifx_bit_writer_put_bits(writer, (unsigned long)code_word, bit_length);
}

static int tifx_mh_write_run(tifx_bit_writer *writer,
                             int is_black,
                             unsigned long run_length)
{
    unsigned long remaining;
    int rc;

    if (writer == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    remaining = run_length;
    while (remaining >= 2624UL) {
        rc = tifx_mh_write_piece(writer, is_black, 2560UL);
        if (rc != TIFX_OK) {
            return rc;
        }
        remaining -= 2560UL;
    }

    if (remaining >= 64UL) {
        unsigned long makeup;
        makeup = (remaining / 64UL) * 64UL;
        if (makeup > 2560UL) {
            makeup = 2560UL;
        }
        rc = tifx_mh_write_piece(writer, is_black, makeup);
        if (rc != TIFX_OK) {
            return rc;
        }
        remaining -= makeup;
    }

    return tifx_mh_write_piece(writer, is_black, remaining);
}

static int tifx_encode_mh_row(tifx_bit_writer *writer,
                              const unsigned char *row,
                              unsigned long width,
                              int align_to_byte)
{
    unsigned long a0;
    int is_black;
    int rc;

    if (writer == 0 || row == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    a0 = 0UL;
    is_black = 0;
    while (a0 < width) {
        unsigned long a1;
        a1 = tifx_find_change_to_color(row,
                                       width,
                                       a0,
                                       (unsigned short)(is_black ? 0U : 1U));
        if (a1 < a0 || a1 > width) {
            return TIFX_ERR_BAD_FORMAT;
        }
        rc = tifx_mh_write_run(writer, is_black, a1 - a0);
        if (rc != TIFX_OK) {
            return rc;
        }
        a0 = a1;
        is_black = !is_black;
    }

    if (align_to_byte) {
        return tifx_bit_writer_pad_zero_to_byte(writer);
    }
    return TIFX_OK;
}

static int tifx_encode_mh_payload_range(tifx_bit_writer *writer,
                                        const tifx_write_params *params,
                                        unsigned long start_row,
                                        unsigned long row_count)
{
    unsigned long row;
    int rc;

    if (writer == 0 || params == 0 || params->pixels == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    for (row = 0UL; row < row_count; ++row) {
        const unsigned char *current_row;
        current_row = params->pixels + ((start_row + row) * params->stride);
        rc = tifx_encode_mh_row(writer, current_row, params->width, 1);
        if (rc != TIFX_OK) {
            return rc;
        }
    }

    return TIFX_OK;
}

static int tifx_measure_mh_payload_range(const tifx_write_params *params,
                                         unsigned long start_row,
                                         unsigned long row_count,
                                         unsigned long *out_size)
{
    tifx_bit_writer writer;
    int rc;

    if (params == 0 || out_size == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    tifx_bit_writer_init(&writer, 0, 0UL);
    rc = tifx_encode_mh_payload_range(&writer, params, start_row, row_count);
    if (rc != TIFX_OK) {
        return rc;
    }
    *out_size = tifx_bit_writer_bytes_used(&writer);
    return TIFX_OK;
}

static int tifx_t4_write_prefix(tifx_bit_writer *writer,
                                int mixed_mode,
                                unsigned short tag_bit,
                                int byte_align)
{
    unsigned long marker_bits;
    int rc;

    if (writer == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    marker_bits = mixed_mode ? 13UL : 12UL;
    if (byte_align) {
        while (((writer->bit_count + marker_bits) & 7UL) != 0UL) {
            rc = tifx_bit_writer_put_bit(writer, 0U);
            if (rc != TIFX_OK) {
                return rc;
            }
        }
    }

    rc = tifx_bit_writer_put_bits(writer, 0x001UL, 12U);
    if (rc != TIFX_OK) {
        return rc;
    }
    if (mixed_mode) {
        rc = tifx_bit_writer_put_bit(writer, tag_bit);
        if (rc != TIFX_OK) {
            return rc;
        }
    }
    return TIFX_OK;
}

static int tifx_t6_write_mode_vertical(tifx_bit_writer *writer, int delta)
{
    switch (delta) {
        case 0: return tifx_bit_writer_put_bits(writer, 0x1UL, 1U);
        case 1: return tifx_bit_writer_put_bits(writer, 0x3UL, 3U);
        case -1: return tifx_bit_writer_put_bits(writer, 0x2UL, 3U);
        case 2: return tifx_bit_writer_put_bits(writer, 0x3UL, 6U);
        case -2: return tifx_bit_writer_put_bits(writer, 0x2UL, 6U);
        case 3: return tifx_bit_writer_put_bits(writer, 0x3UL, 7U);
        case -3: return tifx_bit_writer_put_bits(writer, 0x2UL, 7U);
        default: return TIFX_ERR_BAD_ARGUMENT;
    }
}

static int tifx_encode_t6_row(tifx_bit_writer *writer,
                              const unsigned char *current_row,
                              const unsigned char *reference_row,
                              unsigned long width)
{
    unsigned long a0;
    int is_black;
    unsigned short is_first;
    int rc;

    if (writer == 0 || current_row == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    a0 = 0UL;
    is_black = 0;
    is_first = 1U;

    while (a0 < width) {
        unsigned long a1;
        unsigned long b1;
        unsigned long b2;

        b1 = tifx_find_change_to_color(reference_row,
                                       width,
                                       is_first ? a0 : ((a0 < width) ? (a0 + 1UL) : width),
                                       (unsigned short)(is_black ? 0U : 1U));
        b2 = tifx_find_change_to_color(reference_row,
                                       width,
                                       (b1 < width) ? (b1 + 1UL) : width,
                                       (unsigned short)(is_black ? 1U : 0U));
        a1 = tifx_find_change_to_color(current_row,
                                       width,
                                       a0,
                                       (unsigned short)(is_black ? 0U : 1U));

        {
            long delta;
            delta = (long)a1 - (long)b1;
            if (delta >= -3L && delta <= 3L) {
                rc = tifx_t6_write_mode_vertical(writer, (int)delta);
                if (rc != TIFX_OK) {
                    return rc;
                }
                a0 = a1;
                is_black = !is_black;
            } else if (b2 < a1) {
                rc = tifx_bit_writer_put_bits(writer, 0x1UL, 4U);
                if (rc != TIFX_OK) {
                    return rc;
                }
                a0 = b2;
            } else {
                unsigned long a2;
                a2 = tifx_find_change_to_color(current_row,
                                               width,
                                               (a1 < width) ? (a1 + 1UL) : width,
                                               (unsigned short)(is_black ? 1U : 0U));
                rc = tifx_bit_writer_put_bits(writer, 0x1UL, 3U);
                if (rc != TIFX_OK) {
                    return rc;
                }
                rc = tifx_mh_write_run(writer, is_black, a1 - a0);
                if (rc != TIFX_OK) {
                    return rc;
                }
                rc = tifx_mh_write_run(writer, !is_black, a2 - a1);
                if (rc != TIFX_OK) {
                    return rc;
                }
                a0 = a2;
            }
        }
        is_first = 0U;
    }

    return TIFX_OK;
}

static int tifx_encode_t4_payload_range(tifx_bit_writer *writer,
                                        const tifx_write_params *params,
                                        unsigned long start_row,
                                        unsigned long row_count)
{
    unsigned long row;
    int rc;
    int mixed_mode;
    int byte_align;

    if (writer == 0 || params == 0 || params->pixels == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    mixed_mode = ((params->t4_options & 1UL) != 0UL) ? 1 : 0;
    byte_align = ((params->t4_options & 4UL) != 0UL) ? 1 : 0;

    for (row = 0UL; row < row_count; ++row) {
        const unsigned char *current_row;
        const unsigned char *reference_row;
        int use_2d;
        unsigned short tag_bit;

        current_row = params->pixels + ((start_row + row) * params->stride);
        reference_row = (row == 0UL) ? 0 : (params->pixels + ((start_row + row - 1UL) * params->stride));
        use_2d = (mixed_mode && ((row & 1UL) != 0UL)) ? 1 : 0;
        tag_bit = (unsigned short)(use_2d ? 0U : 1U);

        rc = tifx_t4_write_prefix(writer, mixed_mode, tag_bit, byte_align);
        if (rc != TIFX_OK) {
            return rc;
        }

        if (use_2d) {
            rc = tifx_encode_t6_row(writer, current_row, reference_row, params->width);
        } else {
            rc = tifx_encode_mh_row(writer, current_row, params->width, 0);
        }
        if (rc != TIFX_OK) {
            return rc;
        }
    }

    return tifx_bit_writer_pad_zero_to_byte(writer);
}

static int tifx_measure_t4_payload_range(const tifx_write_params *params,
                                         unsigned long start_row,
                                         unsigned long row_count,
                                         unsigned long *out_size)
{
    tifx_bit_writer writer;
    int rc;

    if (params == 0 || out_size == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    tifx_bit_writer_init(&writer, 0, 0UL);
    rc = tifx_encode_t4_payload_range(&writer, params, start_row, row_count);
    if (rc != TIFX_OK) {
        return rc;
    }
    *out_size = tifx_bit_writer_bytes_used(&writer);
    return TIFX_OK;
}

static int tifx_encode_t6_payload_range(tifx_bit_writer *writer,
                                        const tifx_write_params *params,
                                        unsigned long start_row,
                                        unsigned long row_count)
{
    unsigned long row;
    int rc;

    if (writer == 0 || params == 0 || params->pixels == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    for (row = 0UL; row < row_count; ++row) {
        const unsigned char *current_row;
        const unsigned char *reference_row;

        current_row = params->pixels + ((start_row + row) * params->stride);
        reference_row = (row == 0UL) ? 0 : (params->pixels + ((start_row + row - 1UL) * params->stride));
        rc = tifx_encode_t6_row(writer, current_row, reference_row, params->width);
        if (rc != TIFX_OK) {
            return rc;
        }
    }

    rc = tifx_bit_writer_put_bits(writer, 0x001001UL, 24U);
    if (rc != TIFX_OK) {
        return rc;
    }
    return tifx_bit_writer_pad_zero_to_byte(writer);
}

static int tifx_measure_t6_payload_range(const tifx_write_params *params,
                                         unsigned long start_row,
                                         unsigned long row_count,
                                         unsigned long *out_size)
{
    tifx_bit_writer writer;
    int rc;

    if (params == 0 || out_size == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    tifx_bit_writer_init(&writer, 0, 0UL);
    rc = tifx_encode_t6_payload_range(&writer, params, start_row, row_count);
    if (rc != TIFX_OK) {
        return rc;
    }
    *out_size = tifx_bit_writer_bytes_used(&writer);
    return TIFX_OK;
}

static unsigned long tifx_effective_rows_per_strip(const tifx_write_params *params)
{
    if (params == 0) {
        return 0UL;
    }
    if (params->rows_per_strip == 0UL || params->rows_per_strip > params->height) {
        return params->height;
    }
    return params->rows_per_strip;
}

static int tifx_compute_strip_count(unsigned long height,
                                    unsigned long rows_per_strip,
                                    unsigned long *out_strip_count)
{
    unsigned long strip_count;

    if (rows_per_strip == 0UL || out_strip_count == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    strip_count = tifx_ceil_div(height, rows_per_strip);
    if (strip_count == 0UL || strip_count > TIFX_MAX_STRIPS) {
        return TIFX_ERR_UNSUPPORTED;
    }

    *out_strip_count = strip_count;
    return TIFX_OK;
}

static int tifx_packbits_process_segment(unsigned char *dst,
                                         unsigned long dst_size,
                                         const tifx_write_params *params,
                                         unsigned long stride_min,
                                         unsigned long segment_origin,
                                         unsigned long row_count,
                                         int is_tiled,
                                         unsigned long *out_written);

int tifx__build_strip_byte_counts(const tifx_write_params *params,
                                  unsigned long stride_min,
                                  unsigned long rows_per_strip,
                                  unsigned long strip_count,
                                  unsigned long *strip_byte_counts,
                                  unsigned long *out_image_size)
{
    unsigned short compression;
    unsigned long strip_index;
    unsigned long total_size;
    int rc;
    int is_tiled;

    if (params == 0 || strip_byte_counts == 0 || out_image_size == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    compression = params->compression;
    if (compression == 0U) {
        compression = 1U;
    }
    is_tiled = tifx__params_is_tiled(params);
    if (is_tiled < 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    total_size = 0UL;
    for (strip_index = 0UL; strip_index < strip_count; ++strip_index) {
        unsigned long strip_size;

        if (is_tiled) {
            if (compression == 1U) {
                if (!tifx_mul_ul(rows_per_strip, stride_min, &strip_size)) {
                    return TIFX_ERR_OVERFLOW;
                }
            } else if (compression == 32773U) {
                rc = tifx_packbits_process_segment(0,
                                                  0UL,
                                                  params,
                                                  stride_min,
                                                  strip_index,
                                                  rows_per_strip,
                                                  1,
                                                  &strip_size);
                if (rc != TIFX_OK) {
                    return rc;
                }
            } else if (compression == 5U) {
                rc = tifx_lzw_process_segment(0,
                                             0UL,
                                             params,
                                             stride_min,
                                             strip_index,
                                             rows_per_strip,
                                             1,
                                             &strip_size);
                if (rc != TIFX_OK) {
                    return rc;
                }
            } else if (compression == 8U || compression == 32946U) {
                rc = tifx_deflate_stored_process_segment(0,
                                                        0UL,
                                                        params,
                                                        stride_min,
                                                        strip_index,
                                                        rows_per_strip,
                                                        1,
                                                        &strip_size);
                if (rc != TIFX_OK) {
                    return rc;
                }
            } else {
                return TIFX_ERR_UNSUPPORTED;
            }
        } else {
            unsigned long plane_index;
            unsigned long base_segment_index;
            unsigned long start_row;
            unsigned long rows_this_strip;

            rc = tifx_map_write_segment_index(params,
                                              0,
                                              strip_index,
                                              &plane_index,
                                              &base_segment_index,
                                              0);
            if (rc != TIFX_OK) {
                return rc;
            }
            (void)plane_index;

            start_row = base_segment_index * rows_per_strip;
            rows_this_strip = rows_per_strip;
            if (start_row >= params->height) {
                return TIFX_ERR_BAD_ARGUMENT;
            }
            if (rows_this_strip > params->height - start_row) {
                rows_this_strip = params->height - start_row;
            }

            if (compression == 1U) {
                if (!tifx_mul_ul(rows_this_strip, stride_min, &strip_size)) {
                    return TIFX_ERR_OVERFLOW;
                }
            } else if (compression == 32773U) {
                rc = tifx_packbits_process_segment(0,
                                                  0UL,
                                                  params,
                                                  stride_min,
                                                  strip_index,
                                                  rows_this_strip,
                                                  0,
                                                  &strip_size);
                if (rc != TIFX_OK) {
                    return rc;
                }
            } else if (compression == 5U) {
                rc = tifx_lzw_process_segment(0,
                                             0UL,
                                             params,
                                             stride_min,
                                             strip_index,
                                             rows_this_strip,
                                             0,
                                             &strip_size);
                if (rc != TIFX_OK) {
                    return rc;
                }
            } else if (compression == 8U || compression == 32946U) {
                rc = tifx_deflate_stored_process_segment(0,
                                                        0UL,
                                                        params,
                                                        stride_min,
                                                        strip_index,
                                                        rows_this_strip,
                                                        0,
                                                        &strip_size);
                if (rc != TIFX_OK) {
                    return rc;
                }
            } else if (compression == 2U) {
                rc = tifx_measure_mh_payload_range(params, start_row, rows_this_strip, &strip_size);
                if (rc != TIFX_OK) {
                    return rc;
                }
            } else if (compression == 3U) {
                rc = tifx_measure_t4_payload_range(params, start_row, rows_this_strip, &strip_size);
                if (rc != TIFX_OK) {
                    return rc;
                }
            } else if (compression == 4U) {
                rc = tifx_measure_t6_payload_range(params, start_row, rows_this_strip, &strip_size);
                if (rc != TIFX_OK) {
                    return rc;
                }
            } else {
                return TIFX_ERR_UNSUPPORTED;
            }
        }

        strip_byte_counts[strip_index] = strip_size;
        if (!tifx_add_ul(total_size, strip_size, &total_size)) {
            return TIFX_ERR_OVERFLOW;
        }
    }

    *out_image_size = total_size;
    return TIFX_OK;
}

static void tifx_write_u32le_array(unsigned char *dst,
                                   const unsigned long *values,
                                   unsigned long count)
{
    unsigned long i;

    if (dst == 0 || values == 0) {
        return;
    }

    for (i = 0UL; i < count; ++i) {
        tifx_write_u32le(dst + (i * 4UL), values[i]);
    }
}

static unsigned short tifx_get_msb_packed_bit(const unsigned char *src, unsigned long x)
{
    unsigned char packed;
    packed = src[x >> 3];
    return (unsigned short)((packed >> (7U - (unsigned int)(x & 7UL))) & 1U);
}

static void tifx_set_msb_packed_bit(unsigned char *dst, unsigned long x, unsigned short bit)
{
    unsigned char mask;
    mask = (unsigned char)(0x80U >> (unsigned int)(x & 7UL));
    if (bit != 0U) {
        dst[x >> 3] = (unsigned char)(dst[x >> 3] | mask);
    } else {
        dst[x >> 3] = (unsigned char)(dst[x >> 3] & (unsigned char)(~mask));
    }
}

static int tifx_write_tiled_uncompressed_payload(unsigned char *dst,
                                                 unsigned long dst_size,
                                                 const tifx_write_params *params,
                                                 unsigned long stride_min,
                                                 unsigned long tile_index,
                                                 unsigned long tile_length)
{
    unsigned short fill_order;
    unsigned short planar_config;
    unsigned long samples_per_pixel;
    unsigned long plane_index;
    unsigned long base_tile_index;
    unsigned long tiles_across;
    unsigned long tile_x_index;
    unsigned long tile_y_index;
    unsigned long base_x;
    unsigned long base_y;
    unsigned long visible_width;
    unsigned long row;
    unsigned long expected_size;
    int rc;

    if (dst == 0 || params == 0 || params->pixels == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (params->tile_width == 0UL || params->tile_length == 0UL) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (!tifx_mul_ul(tile_length, stride_min, &expected_size)) {
        return TIFX_ERR_OVERFLOW;
    }
    if (expected_size != dst_size) {
        return TIFX_ERR_BAD_FORMAT;
    }

    fill_order = tifx_effective_fill_order(params);
    planar_config = tifx_effective_write_planar_config(params);
    samples_per_pixel = tifx_write_sample_count(params);
    rc = tifx_map_write_segment_index(params,
                                      1,
                                      tile_index,
                                      &plane_index,
                                      &base_tile_index,
                                      0);
    if (rc != TIFX_OK) {
        return rc;
    }
    (void)planar_config;
    tiles_across = tifx_ceil_div(params->width, params->tile_width);
    tile_x_index = base_tile_index % tiles_across;
    tile_y_index = base_tile_index / tiles_across;
    base_x = tile_x_index * params->tile_width;
    base_y = tile_y_index * params->tile_length;
    if (base_x >= params->width || base_y >= params->height) {
        return TIFX_ERR_BAD_FORMAT;
    }
    visible_width = params->tile_width;
    if (visible_width > params->width - base_x) {
        visible_width = params->width - base_x;
    }

    memset(dst, 0, dst_size);
    for (row = 0UL; row < tile_length; ++row) {
        unsigned long src_y;
        unsigned char *dst_row;
        const unsigned char *src_row;

        src_y = base_y + row;
        dst_row = dst + (row * stride_min);
        if (src_y >= params->height) {
            continue;
        }
        src_row = params->pixels + (src_y * params->stride);

        if (params->pixel_format == TIFX_PIXEL_GRAY8) {
            memcpy(dst_row, src_row + base_x, visible_width);
        } else if ((params->pixel_format == TIFX_PIXEL_RGB24 ||
                    params->pixel_format == TIFX_PIXEL_RGBA32) &&
                   planar_config == 2U && samples_per_pixel > 1UL) {
            unsigned long x;
            for (x = 0UL; x < visible_width; ++x) {
                dst_row[x] = src_row[((base_x + x) * samples_per_pixel) + plane_index];
            }
        } else if (params->pixel_format == TIFX_PIXEL_RGB24) {
            memcpy(dst_row, src_row + (base_x * 3UL), visible_width * 3UL);
        } else if (params->pixel_format == TIFX_PIXEL_RGBA32) {
            memcpy(dst_row, src_row + (base_x * 4UL), visible_width * 4UL);
        } else if (params->pixel_format == TIFX_PIXEL_BILEVEL) {
            unsigned long x;
            for (x = 0UL; x < visible_width; ++x) {
                tifx_set_msb_packed_bit(dst_row,
                                        x,
                                        tifx_get_msb_packed_bit(src_row, base_x + x));
            }
        } else {
            return TIFX_ERR_UNSUPPORTED;
        }
    }

    if (params->pixel_format == TIFX_PIXEL_BILEVEL && fill_order == 2U) {
        tifx_reverse_bytes_in_place(dst, dst_size);
    }
    return TIFX_OK;
}

static unsigned char tifx_packbits_row_byte(const tifx_write_params *params,
                                            unsigned long stride_min,
                                            unsigned long segment_origin,
                                            unsigned long row_in_segment,
                                            unsigned long byte_index,
                                            int is_tiled)
{
    const unsigned char *src_row;
    unsigned short fill_order;
    unsigned short planar_config;
    unsigned long samples_per_pixel;
    unsigned long plane_index;
    unsigned long base_segment_index;
    int rc;
    unsigned char value;

    fill_order = tifx_effective_fill_order(params);
    planar_config = tifx_effective_write_planar_config(params);
    samples_per_pixel = tifx_write_sample_count(params);
    rc = tifx_map_write_segment_index(params,
                                      is_tiled,
                                      segment_origin,
                                      &plane_index,
                                      &base_segment_index,
                                      0);
    if (rc != TIFX_OK) {
        return 0U;
    }

    if (!is_tiled) {
        unsigned long rows_per_strip;
        unsigned long src_y;

        rows_per_strip = params->rows_per_strip;
        if (rows_per_strip == 0UL || rows_per_strip > params->height) {
            rows_per_strip = params->height;
        }
        src_y = (base_segment_index * rows_per_strip) + row_in_segment;
        if (src_y >= params->height) {
            return 0U;
        }
        src_row = params->pixels + (src_y * params->stride);
        if (params->pixel_format == TIFX_PIXEL_BILEVEL) {
            value = src_row[byte_index];
            if (fill_order == 2U) {
                value = tifx_reverse_byte(value);
            }
            return value;
        }
        if ((params->pixel_format == TIFX_PIXEL_RGB24 ||
             params->pixel_format == TIFX_PIXEL_RGBA32) &&
            planar_config == 2U && samples_per_pixel > 1UL) {
            if (byte_index >= params->width) {
                return 0U;
            }
            return src_row[(byte_index * samples_per_pixel) + plane_index];
        }
        return src_row[byte_index];
    }

    {
        unsigned long tiles_across;
        unsigned long tile_x_index;
        unsigned long tile_y_index;
        unsigned long base_x;
        unsigned long base_y;
        unsigned long visible_width;
        unsigned long src_y;

        (void)stride_min;
        tiles_across = tifx_ceil_div(params->width, params->tile_width);
        tile_x_index = base_segment_index % tiles_across;
        tile_y_index = base_segment_index / tiles_across;
        base_x = tile_x_index * params->tile_width;
        base_y = tile_y_index * params->tile_length;
        if (base_x >= params->width || base_y >= params->height) {
            return 0U;
        }

        visible_width = params->tile_width;
        if (visible_width > params->width - base_x) {
            visible_width = params->width - base_x;
        }
        src_y = base_y + row_in_segment;
        if (src_y >= params->height) {
            return 0U;
        }

        src_row = params->pixels + (src_y * params->stride);
        if (params->pixel_format == TIFX_PIXEL_GRAY8) {
            if (byte_index >= visible_width) {
                return 0U;
            }
            return src_row[base_x + byte_index];
        }
        if ((params->pixel_format == TIFX_PIXEL_RGB24 ||
             params->pixel_format == TIFX_PIXEL_RGBA32) &&
            planar_config == 2U && samples_per_pixel > 1UL) {
            if (byte_index >= visible_width) {
                return 0U;
            }
            return src_row[((base_x + byte_index) * samples_per_pixel) + plane_index];
        }
        if (params->pixel_format == TIFX_PIXEL_RGB24) {
            unsigned long visible_bytes;

            visible_bytes = visible_width * 3UL;
            if (byte_index >= visible_bytes) {
                return 0U;
            }
            return src_row[(base_x * 3UL) + byte_index];
        }
        if (params->pixel_format == TIFX_PIXEL_RGBA32) {
            unsigned long visible_bytes;

            visible_bytes = visible_width * 4UL;
            if (byte_index >= visible_bytes) {
                return 0U;
            }
            return src_row[(base_x * 4UL) + byte_index];
        }
        if (params->pixel_format == TIFX_PIXEL_BILEVEL) {
            unsigned long x0;
            unsigned long bit_index;
            unsigned char out_byte;

            out_byte = 0U;
            x0 = byte_index * 8UL;
            for (bit_index = 0UL; bit_index < 8UL; ++bit_index) {
                unsigned long x;
                x = x0 + bit_index;
                if (x < visible_width && x < params->tile_width &&
                    tifx_get_msb_packed_bit(src_row, base_x + x) != 0U) {
                    out_byte = (unsigned char)(out_byte | (unsigned char)(0x80U >> (unsigned int)bit_index));
                }
            }
            if (fill_order == 2U) {
                out_byte = tifx_reverse_byte(out_byte);
            }
            return out_byte;
        }
    }

    (void)stride_min;
    return 0U;
}

static unsigned char tifx_compression_row_byte(const tifx_write_params *params,
                                               unsigned long stride_min,
                                               unsigned long segment_origin,
                                               unsigned long row_in_segment,
                                               unsigned long byte_index,
                                               int is_tiled)
{
    unsigned char value;
    unsigned short predictor;
    unsigned long sample_stride;

    value = tifx_packbits_row_byte(params,
                                   stride_min,
                                   segment_origin,
                                   row_in_segment,
                                   byte_index,
                                   is_tiled);
    predictor = tifx_effective_predictor(params);
    if (predictor != TIFX_PREDICTOR_HORIZONTAL) {
        return value;
    }
    sample_stride = tifx_predictor_row_sample_stride_params(params);
    if (byte_index < sample_stride) {
        return value;
    }
    return (unsigned char)(value - tifx_packbits_row_byte(params,
                                                           stride_min,
                                                           segment_origin,
                                                           row_in_segment,
                                                           byte_index - sample_stride,
                                                           is_tiled));
}

static int tifx_apply_horizontal_predictor_inverse(const tifx_image_info *info,
                                                   unsigned char *row,
                                                   unsigned long row_bytes)
{
    unsigned long i;
    unsigned long sample_stride;

    if (info == 0 || row == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (info->predictor != TIFX_PREDICTOR_HORIZONTAL) {
        return TIFX_OK;
    }
    sample_stride = tifx_predictor_row_sample_stride_info(info);
    if (sample_stride == 0UL || sample_stride >= row_bytes) {
        return TIFX_OK;
    }
    for (i = sample_stride; i < row_bytes; ++i) {
        row[i] = (unsigned char)(row[i] + row[i - sample_stride]);
    }
    return TIFX_OK;
}

static unsigned long tifx_packbits_repeat_count(const tifx_write_params *params,
                                                unsigned long stride_min,
                                                unsigned long segment_origin,
                                                unsigned long row_in_segment,
                                                unsigned long row_bytes,
                                                unsigned long start,
                                                int is_tiled)
{
    unsigned long count;
    unsigned char value;

    value = tifx_packbits_row_byte(params,
                                   stride_min,
                                   segment_origin,
                                   row_in_segment,
                                   start,
                                   is_tiled);
    count = 1UL;
    while (count < 128UL && start + count < row_bytes) {
        if (tifx_packbits_row_byte(params,
                                   stride_min,
                                   segment_origin,
                                   row_in_segment,
                                   start + count,
                                   is_tiled) != value) {
            break;
        }
        ++count;
    }
    return count;
}

static void tifx_lzw_decoder_advance_code_size(unsigned short next_code,
                                                unsigned short *code_size)
{
    if (code_size == 0) {
        return;
    }
    if (next_code == 511U) {
        *code_size = 10U;
    } else if (next_code == 1023U) {
        *code_size = 11U;
    } else if (next_code == 2047U) {
        *code_size = 12U;
    }
}

static void tifx_lzw_encoder_advance_code_size(unsigned short next_code,
                                                unsigned short *code_size)
{
    if (code_size == 0) {
        return;
    }
    if (next_code == 512U) {
        *code_size = 10U;
    } else if (next_code == 1024U) {
        *code_size = 11U;
    } else if (next_code == 2048U) {
        *code_size = 12U;
    }
}

static int tifx_lzw_decoder_expand(tifx_lzw_decoder *decoder,
                                   unsigned short code,
                                   unsigned char *out_first_char)
{
    unsigned short sp;

    if (decoder == 0 || out_first_char == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (code >= decoder->next_code || code == 256U || code == 257U) {
        return TIFX_ERR_BAD_FORMAT;
    }

    sp = 0U;
    while (code >= 258U) {
        if (code >= 4096U || sp >= 4096U) {
            return TIFX_ERR_BAD_FORMAT;
        }
        decoder->stack[sp++] = decoder->suffix[code];
        code = decoder->prefix[code];
    }
    if (code > 255U || sp >= 4097U) {
        return TIFX_ERR_BAD_FORMAT;
    }
    decoder->stack[sp++] = (unsigned char)code;
    decoder->stack_len = sp;
    *out_first_char = decoder->stack[sp - 1U];
    return TIFX_OK;
}

static void tifx_lzw_decoder_reset(tifx_lzw_decoder *decoder)
{
    if (decoder == 0) {
        return;
    }
    decoder->code_size = 9U;
    decoder->next_code = 258U;
    decoder->have_old = 0U;
    decoder->stack_len = 0U;
}

static void tifx_lzw_decoder_init(tifx_lzw_decoder *decoder,
                                  const unsigned char *src,
                                  const unsigned char *src_end)
{
    if (decoder == 0) {
        return;
    }
    tifx_bit_reader_init(&decoder->reader, src, src_end, 1U);
    tifx_lzw_decoder_reset(decoder);
}

static int tifx_lzw_decoder_read(tifx_lzw_decoder *decoder,
                                 unsigned char *dst,
                                 unsigned long dst_len)
{
    unsigned long out_pos;

    if (decoder == 0 || dst == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    out_pos = 0UL;
    while (out_pos < dst_len) {
        if (decoder->stack_len != 0U) {
            dst[out_pos++] = decoder->stack[--decoder->stack_len];
            continue;
        }

        {
            unsigned long code_value;
            unsigned short code;
            int rc;

            rc = tifx_bit_reader_get_bits(&decoder->reader,
                                          decoder->code_size,
                                          &code_value);
            if (rc != TIFX_OK) {
                return rc;
            }
            code = (unsigned short)code_value;

            if (code == 256U) {
                tifx_lzw_decoder_reset(decoder);
                continue;
            }
            if (code == 257U) {
                return TIFX_ERR_BAD_FORMAT;
            }

            if (!decoder->have_old) {
                unsigned char first_char;
                rc = tifx_lzw_decoder_expand(decoder, code, &first_char);
                if (rc != TIFX_OK) {
                    return rc;
                }
                decoder->old_code = code;
                decoder->have_old = 1U;
                (void)first_char;
                continue;
            }

            if (code < decoder->next_code) {
                unsigned char first_char;

                rc = tifx_lzw_decoder_expand(decoder, code, &first_char);
                if (rc != TIFX_OK) {
                    return rc;
                }
                if (decoder->next_code < 4095U) {
                    decoder->prefix[decoder->next_code] = decoder->old_code;
                    decoder->suffix[decoder->next_code] = first_char;
                    ++decoder->next_code;
                    tifx_lzw_decoder_advance_code_size(decoder->next_code, &decoder->code_size);
                }
                decoder->old_code = code;
                continue;
            }

            if (code == decoder->next_code) {
                unsigned char first_char;

                rc = tifx_lzw_decoder_expand(decoder, decoder->old_code, &first_char);
                if (rc != TIFX_OK) {
                    return rc;
                }
                if (decoder->stack_len >= 4097U) {
                    return TIFX_ERR_BAD_FORMAT;
                }
                memmove(decoder->stack + 1U, decoder->stack, decoder->stack_len);
                decoder->stack[0] = first_char;
                ++decoder->stack_len;
                if (decoder->next_code < 4095U) {
                    decoder->prefix[decoder->next_code] = decoder->old_code;
                    decoder->suffix[decoder->next_code] = first_char;
                    ++decoder->next_code;
                    tifx_lzw_decoder_advance_code_size(decoder->next_code, &decoder->code_size);
                }
                decoder->old_code = code;
                continue;
            }

            return TIFX_ERR_BAD_FORMAT;
        }
    }

    return TIFX_OK;
}

static unsigned long tifx_lzw_hash_index(unsigned long key)
{
    return (key * 40543UL) % 9001UL;
}

static void tifx_lzw_encoder_reset_table(tifx_lzw_encoder *encoder)
{
    if (encoder == 0) {
        return;
    }
    memset(encoder->hash_codes, 0xFF, sizeof(encoder->hash_codes));
    encoder->code_size = 9U;
    encoder->next_code = 258U;
}

static int tifx_lzw_encoder_find(const tifx_lzw_encoder *encoder,
                                 unsigned short prefix,
                                 unsigned char suffix,
                                 unsigned short *out_code)
{
    unsigned long key;
    unsigned long idx;
    unsigned long steps;

    if (encoder == 0 || out_code == 0) {
        return 0;
    }

    key = (((unsigned long)prefix) << 8) | (unsigned long)suffix;
    idx = tifx_lzw_hash_index(key);
    for (steps = 0UL; steps < 9001UL; ++steps) {
        if (encoder->hash_codes[idx] == 0xFFFFU) {
            return 0;
        }
        if (encoder->hash_keys[idx] == key) {
            *out_code = encoder->hash_codes[idx];
            return 1;
        }
        ++idx;
        if (idx == 9001UL) {
            idx = 0UL;
        }
    }
    return 0;
}

static void tifx_lzw_encoder_insert(tifx_lzw_encoder *encoder,
                                    unsigned short prefix,
                                    unsigned char suffix,
                                    unsigned short code)
{
    unsigned long key;
    unsigned long idx;

    if (encoder == 0) {
        return;
    }

    key = (((unsigned long)prefix) << 8) | (unsigned long)suffix;
    idx = tifx_lzw_hash_index(key);
    while (encoder->hash_codes[idx] != 0xFFFFU) {
        ++idx;
        if (idx == 9001UL) {
            idx = 0UL;
        }
    }
    encoder->hash_keys[idx] = key;
    encoder->hash_codes[idx] = code;
}

static int tifx_lzw_encoder_init(tifx_lzw_encoder *encoder,
                                 unsigned char *dst,
                                 unsigned long dst_size)
{
    int rc;

    if (encoder == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    tifx_bit_writer_init(&encoder->writer, dst, dst_size);
    tifx_lzw_encoder_reset_table(encoder);
    encoder->have_prefix = 0U;
    rc = tifx_bit_writer_put_bits(&encoder->writer, 256UL, encoder->code_size);
    if (rc != TIFX_OK) {
        return rc;
    }
    return TIFX_OK;
}

static int tifx_lzw_process_segment(unsigned char *dst,
                                    unsigned long dst_size,
                                    const tifx_write_params *params,
                                    unsigned long stride_min,
                                    unsigned long segment_origin,
                                    unsigned long row_count,
                                    int is_tiled,
                                    unsigned long *out_written)
{
    tifx_lzw_encoder encoder;
    unsigned long row_in_segment;
    int rc;

    if (params == 0 || out_written == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (dst != 0) {
        memset(dst, 0, dst_size);
    }

    rc = tifx_lzw_encoder_init(&encoder, dst, dst_size);
    if (rc != TIFX_OK) {
        return rc;
    }

    for (row_in_segment = 0UL; row_in_segment < row_count; ++row_in_segment) {
        unsigned long i;

        for (i = 0UL; i < stride_min; ++i) {
            unsigned char value;

            value = tifx_compression_row_byte(params,
                                              stride_min,
                                              segment_origin,
                                              row_in_segment,
                                              i,
                                              is_tiled);
            if (!encoder.have_prefix) {
                encoder.prefix_code = (unsigned short)value;
                encoder.have_prefix = 1U;
                continue;
            }

            {
                unsigned short found_code;

                if (tifx_lzw_encoder_find(&encoder,
                                          encoder.prefix_code,
                                          value,
                                          &found_code)) {
                    encoder.prefix_code = found_code;
                    continue;
                }
            }

            rc = tifx_bit_writer_put_bits(&encoder.writer,
                                          (unsigned long)encoder.prefix_code,
                                          encoder.code_size);
            if (rc != TIFX_OK) {
                return rc;
            }

            if (encoder.next_code < 4095U) {
                tifx_lzw_encoder_insert(&encoder,
                                        encoder.prefix_code,
                                        value,
                                        encoder.next_code);
                ++encoder.next_code;
                tifx_lzw_encoder_advance_code_size(encoder.next_code, &encoder.code_size);
            } else {
                rc = tifx_bit_writer_put_bits(&encoder.writer, 256UL, encoder.code_size);
                if (rc != TIFX_OK) {
                    return rc;
                }
                tifx_lzw_encoder_reset_table(&encoder);
            }
            encoder.prefix_code = (unsigned short)value;
        }
    }

    if (encoder.have_prefix) {
        rc = tifx_bit_writer_put_bits(&encoder.writer,
                                      (unsigned long)encoder.prefix_code,
                                      encoder.code_size);
        if (rc != TIFX_OK) {
            return rc;
        }
    }
    rc = tifx_bit_writer_put_bits(&encoder.writer, 257UL, encoder.code_size);
    if (rc != TIFX_OK) {
        return rc;
    }
    rc = tifx_bit_writer_pad_zero_to_byte(&encoder.writer);
    if (rc != TIFX_OK) {
        return rc;
    }

    *out_written = tifx_bit_writer_bytes_used(&encoder.writer);
    return TIFX_OK;
}


static voidpf tifx_zlib_alloc(voidpf opaque, uInt items, uInt size)
{
    tifx_zlib_arena *arena;
    unsigned long total;
    unsigned long aligned;
    unsigned long base;

    arena = (tifx_zlib_arena *)opaque;
    if (arena == 0) {
        return Z_NULL;
    }
    total = (unsigned long)items * (unsigned long)size;
    aligned = sizeof(void *) - 1U;
    base = (arena->offset + aligned) & ~aligned;
    if (base > sizeof(arena->buffer) || total > sizeof(arena->buffer) - base) {
        return Z_NULL;
    }
    arena->offset = base + total;
    return (voidpf)(arena->buffer + base);
}

static void tifx_zlib_free(voidpf opaque, voidpf address)
{
    (void)opaque;
    (void)address;
}

static int tifx_map_zlib_status(int zrc);

static unsigned char tifx_deflate_source_byte(const tifx_write_params *params,
                                              unsigned long stride_min,
                                              unsigned long segment_origin,
                                              unsigned long absolute_index,
                                              int is_tiled)
{
    unsigned long row_in_segment;
    unsigned long byte_index;

    row_in_segment = absolute_index / stride_min;
    byte_index = absolute_index % stride_min;
    return tifx_compression_row_byte(params,
                                     stride_min,
                                     segment_origin,
                                     row_in_segment,
                                     byte_index,
                                     is_tiled);
}

static void tifx_deflate_plan_init(tifx_deflate_plan *plan,
                                   unsigned short mode,
                                   int level,
                                   int strategy,
                                   int good_length,
                                   int max_lazy,
                                   int nice_length,
                                   int max_chain)
{
    if (plan == 0) {
        return;
    }
    plan->mode = mode;
    plan->level = level;
    plan->strategy = strategy;
    plan->good_length = good_length;
    plan->max_lazy = max_lazy;
    plan->nice_length = nice_length;
    plan->max_chain = max_chain;
}

static int tifx_deflate_plan_equals(const tifx_deflate_plan *a,
                                    const tifx_deflate_plan *b)
{
    if (a == 0 || b == 0) {
        return 0;
    }
    return a->mode == b->mode &&
           a->level == b->level &&
           a->strategy == b->strategy &&
           a->good_length == b->good_length &&
           a->max_lazy == b->max_lazy &&
           a->nice_length == b->nice_length &&
           a->max_chain == b->max_chain;
}

static int tifx_deflate_plan_from_explicit_mode(unsigned short deflate_mode,
                                                unsigned short predictor,
                                                unsigned long block_size,
                                                unsigned long run_bytes,
                                                unsigned long small_delta_bytes,
                                                tifx_deflate_plan *out_plan)
{
    tifx_deflate_plan plan;

    if (out_plan == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    if (deflate_mode == TIFX_DEFLATE_STORED) {
        tifx_deflate_plan_init(&plan,
                               TIFX_DEFLATE_STORED,
                               0,
                               Z_DEFAULT_STRATEGY,
                               0, 0, 0, 0);
        *out_plan = plan;
        return TIFX_OK;
    }

    if (deflate_mode == TIFX_DEFLATE_FIXED) {
        tifx_deflate_plan_init(&plan,
                               TIFX_DEFLATE_FIXED,
                               (block_size >= 4096UL || run_bytes * 4UL >= block_size) ? 6 : 5,
                               Z_FIXED,
                               8,
                               24,
                               64,
                               48);
        if (predictor == TIFX_PREDICTOR_HORIZONTAL &&
            small_delta_bytes * 100UL >= block_size * 70UL) {
            plan.good_length = 4;
            plan.max_lazy = 16;
            plan.nice_length = 32;
            plan.max_chain = 32;
        } else if (run_bytes * 100UL >= block_size * 30UL) {
            plan.good_length = 16;
            plan.max_lazy = 48;
            plan.nice_length = 128;
            plan.max_chain = 96;
        }
        *out_plan = plan;
        return TIFX_OK;
    }

    if (deflate_mode == TIFX_DEFLATE_DYNAMIC) {
        tifx_deflate_plan_init(&plan,
                               TIFX_DEFLATE_DYNAMIC,
                               (block_size >= 8192UL) ? 8 : 6,
                               (predictor == TIFX_PREDICTOR_HORIZONTAL) ? Z_FILTERED : Z_DEFAULT_STRATEGY,
                               8,
                               32,
                               96,
                               160);
        if (predictor == TIFX_PREDICTOR_HORIZONTAL &&
            small_delta_bytes * 100UL >= block_size * 72UL) {
            plan.level = 6;
            plan.good_length = 4;
            plan.max_lazy = 24;
            plan.nice_length = 48;
            plan.max_chain = 96;
        } else if (run_bytes * 100UL >= block_size * 35UL) {
            plan.level = 8;
            plan.good_length = 24;
            plan.max_lazy = 96;
            plan.nice_length = 192;
            plan.max_chain = 256;
        } else if (block_size >= 4096UL) {
            plan.good_length = 16;
            plan.max_lazy = 64;
            plan.nice_length = 160;
            plan.max_chain = 224;
        }
        *out_plan = plan;
        return TIFX_OK;
    }

    return TIFX_ERR_BAD_ARGUMENT;
}

static int tifx_choose_deflate_plan(const tifx_write_params *params,
                                    const unsigned char *block,
                                    unsigned long block_size,
                                    tifx_deflate_plan *out_plan)
{
    unsigned short histogram[256];
    unsigned long unique_count;
    unsigned long zero_count;
    unsigned long small_delta_bytes;
    unsigned long run_bytes;
    unsigned long i;
    unsigned char prev;

    if (params == 0 || block == 0 || out_plan == 0 || block_size == 0UL) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    memset(histogram, 0, sizeof(histogram));
    unique_count = 0UL;
    zero_count = 0UL;
    small_delta_bytes = 0UL;
    run_bytes = 0UL;
    prev = block[0];

    for (i = 0UL; i < block_size; ++i) {
        unsigned char value;

        value = block[i];
        if (histogram[value] == 0U) {
            ++unique_count;
        }
        ++histogram[value];
        if (value == 0U) {
            ++zero_count;
        }
        if (value <= 3U || value >= 252U) {
            ++small_delta_bytes;
        }
        if (i != 0UL && value == prev) {
            ++run_bytes;
        }
        prev = value;
    }

    if (params->deflate_mode != TIFX_DEFLATE_AUTO) {
        return tifx_deflate_plan_from_explicit_mode(params->deflate_mode,
                                                    tifx_effective_predictor(params),
                                                    block_size,
                                                    run_bytes,
                                                    small_delta_bytes,
                                                    out_plan);
    }

    if (block_size < 96UL) {
        if (run_bytes * 3UL >= block_size || zero_count * 2UL >= block_size) {
            return tifx_deflate_plan_from_explicit_mode(TIFX_DEFLATE_FIXED,
                                                        tifx_effective_predictor(params),
                                                        block_size,
                                                        run_bytes,
                                                        small_delta_bytes,
                                                        out_plan);
        }
        return tifx_deflate_plan_from_explicit_mode(TIFX_DEFLATE_STORED,
                                                    tifx_effective_predictor(params),
                                                    block_size,
                                                    run_bytes,
                                                    small_delta_bytes,
                                                    out_plan);
    }

    if (tifx_effective_predictor(params) == TIFX_PREDICTOR_HORIZONTAL &&
        small_delta_bytes * 100UL >= block_size * 58UL) {
        return tifx_deflate_plan_from_explicit_mode(TIFX_DEFLATE_DYNAMIC,
                                                    TIFX_PREDICTOR_HORIZONTAL,
                                                    block_size,
                                                    run_bytes,
                                                    small_delta_bytes,
                                                    out_plan);
    }

    if (unique_count > 224UL &&
        run_bytes * 100UL < block_size * 8UL &&
        small_delta_bytes * 100UL < block_size * 20UL) {
        return tifx_deflate_plan_from_explicit_mode(TIFX_DEFLATE_STORED,
                                                    tifx_effective_predictor(params),
                                                    block_size,
                                                    run_bytes,
                                                    small_delta_bytes,
                                                    out_plan);
    }

    if ((block_size <= 2048UL && unique_count <= 48UL) ||
        (zero_count * 100UL >= block_size * 35UL && block_size <= 4096UL)) {
        return tifx_deflate_plan_from_explicit_mode(TIFX_DEFLATE_FIXED,
                                                    tifx_effective_predictor(params),
                                                    block_size,
                                                    run_bytes,
                                                    small_delta_bytes,
                                                    out_plan);
    }

    return tifx_deflate_plan_from_explicit_mode(TIFX_DEFLATE_DYNAMIC,
                                                tifx_effective_predictor(params),
                                                block_size,
                                                run_bytes,
                                                small_delta_bytes,
                                                out_plan);
}

static int tifx_deflate_store_output(unsigned char *dst,
                                     unsigned long dst_size,
                                     unsigned long *total_written,
                                     const unsigned char *outbuf,
                                     unsigned long produced)
{
    if (total_written == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (dst != 0) {
        if (produced > dst_size - *total_written) {
            return TIFX_ERR_NO_SPACE;
        }
        memcpy(dst + *total_written, outbuf, produced);
    }
    *total_written += produced;
    return TIFX_OK;
}

static int tifx_deflate_apply_tune(z_stream *stream, const tifx_deflate_plan *plan)
{
#if defined(ZLIB_VERNUM) && ZLIB_VERNUM >= 0x1223
    int zrc;

    if (stream == 0 || plan == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (plan->mode == TIFX_DEFLATE_STORED) {
        return TIFX_OK;
    }
    zrc = deflateTune(stream,
                      plan->good_length,
                      plan->max_lazy,
                      plan->nice_length,
                      plan->max_chain);
    return tifx_map_zlib_status(zrc);
#else
    (void)stream;
    (void)plan;
    return TIFX_OK;
#endif
}

static int tifx_deflate_change_plan(z_stream *stream,
                                    const tifx_deflate_plan *plan,
                                    unsigned char *dst,
                                    unsigned long dst_size,
                                    unsigned long *total_written,
                                    unsigned char *outbuf,
                                    unsigned long outbuf_size)
{
    int zrc;
    int rc;
    unsigned long produced;

    if (stream == 0 || plan == 0 || total_written == 0 || outbuf == 0 || outbuf_size == 0UL) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    do {
        stream->next_out = outbuf;
        stream->avail_out = (uInt)outbuf_size;
        zrc = deflateParams(stream, plan->level, plan->strategy);
        produced = outbuf_size - (unsigned long)stream->avail_out;
        rc = tifx_deflate_store_output(dst, dst_size, total_written, outbuf, produced);
        if (rc != TIFX_OK) {
            return rc;
        }
    } while (zrc == Z_BUF_ERROR);

    rc = tifx_map_zlib_status(zrc);
    if (rc != TIFX_OK) {
        return rc;
    }
    return tifx_deflate_apply_tune(stream, plan);
}

static int tifx_map_zlib_status(int zrc)
{
    if (zrc == Z_OK || zrc == Z_STREAM_END) {
        return TIFX_OK;
    }
    if (zrc == Z_MEM_ERROR) {
        return TIFX_ERR_NO_SPACE;
    }
    if (zrc == Z_BUF_ERROR) {
        return TIFX_ERR_BAD_FORMAT;
    }
    if (zrc == Z_DATA_ERROR) {
        return TIFX_ERR_BAD_FORMAT;
    }
    return TIFX_ERR_UNSUPPORTED;
}

static int tifx_deflate_stored_process_segment(unsigned char *dst,
                                               unsigned long dst_size,
                                               const tifx_write_params *params,
                                               unsigned long stride_min,
                                               unsigned long segment_origin,
                                               unsigned long row_count,
                                               int is_tiled,
                                               unsigned long *out_written)
{
    z_stream stream;
    tifx_zlib_arena arena;
    unsigned char inbuf[TIFX_DEFLATE_BLOCK_BYTES];
    unsigned char outbuf[4096];
    tifx_deflate_plan current_plan;
    tifx_deflate_plan next_plan;
    unsigned long raw_size;
    unsigned long input_index;
    unsigned long total_written;
    unsigned long target_block_size;
    int zrc;
    int rc;
    unsigned short stream_initialized;

    if (params == 0 || out_written == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (!tifx_mul_ul(row_count, stride_min, &raw_size)) {
        return TIFX_ERR_OVERFLOW;
    }

    memset(&stream, 0, sizeof(stream));
    memset(&arena, 0, sizeof(arena));
    memset(&current_plan, 0, sizeof(current_plan));
    stream.zalloc = tifx_zlib_alloc;
    stream.zfree = tifx_zlib_free;
    stream.opaque = &arena;

    total_written = 0UL;
    input_index = 0UL;
    stream_initialized = 0U;
    target_block_size = sizeof(inbuf);
    if (stride_min != 0UL) {
        unsigned long rows_per_block;

        rows_per_block = target_block_size / stride_min;
        if (rows_per_block != 0UL) {
            target_block_size = rows_per_block * stride_min;
        }
    }
    if (target_block_size == 0UL) {
        target_block_size = sizeof(inbuf);
    }

    while (input_index < raw_size || !stream_initialized) {
        unsigned long chunk_size;
        unsigned long chunk_index;
        int flush;

        if (input_index >= raw_size) {
            break;
        }
        chunk_size = raw_size - input_index;
        if (chunk_size > target_block_size) {
            chunk_size = target_block_size;
        }
        if (chunk_size > sizeof(inbuf)) {
            chunk_size = sizeof(inbuf);
        }

        for (chunk_index = 0UL; chunk_index < chunk_size; ++chunk_index) {
            inbuf[chunk_index] = tifx_deflate_source_byte(params,
                                                          stride_min,
                                                          segment_origin,
                                                          input_index + chunk_index,
                                                          is_tiled);
        }
        rc = tifx_choose_deflate_plan(params, inbuf, chunk_size, &next_plan);
        if (rc != TIFX_OK) {
            return rc;
        }

        if (!stream_initialized) {
            zrc = deflateInit2(&stream,
                               next_plan.level,
                               Z_DEFLATED,
                               MAX_WBITS,
                               TIFX_ZLIB_MEM_LEVEL,
                               next_plan.strategy);
            if (zrc != Z_OK) {
                return tifx_map_zlib_status(zrc);
            }
            stream_initialized = 1U;
            rc = tifx_deflate_apply_tune(&stream, &next_plan);
            if (rc != TIFX_OK) {
                deflateEnd(&stream);
                return rc;
            }
            current_plan = next_plan;
        } else if (!tifx_deflate_plan_equals(&current_plan, &next_plan)) {
            rc = tifx_deflate_change_plan(&stream,
                                          &next_plan,
                                          dst,
                                          dst_size,
                                          &total_written,
                                          outbuf,
                                          sizeof(outbuf));
            if (rc != TIFX_OK) {
                deflateEnd(&stream);
                return rc;
            }
            current_plan = next_plan;
        }

        input_index += chunk_size;
        flush = (input_index >= raw_size) ? Z_FINISH : Z_NO_FLUSH;
        stream.next_in = inbuf;
        stream.avail_in = (uInt)chunk_size;

        do {
            unsigned long produced;

            stream.next_out = outbuf;
            stream.avail_out = sizeof(outbuf);
            zrc = deflate(&stream, flush);
            rc = tifx_map_zlib_status(zrc);
            if (rc != TIFX_OK) {
                deflateEnd(&stream);
                return rc;
            }
            produced = (unsigned long)(sizeof(outbuf) - stream.avail_out);
            rc = tifx_deflate_store_output(dst, dst_size, &total_written, outbuf, produced);
            if (rc != TIFX_OK) {
                deflateEnd(&stream);
                return rc;
            }
        } while (stream.avail_in != 0U ||
                 stream.avail_out == 0U ||
                 (flush == Z_FINISH && zrc != Z_STREAM_END));
    }

    if (!stream_initialized) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    deflateEnd(&stream);
    *out_written = total_written;
    return TIFX_OK;
}

static int tifx_zlib_stored_decoder_init(tifx_zlib_stored_decoder *decoder,
                                         const unsigned char *src,
                                         const unsigned char *src_end)
{
    int zrc;

    if (decoder == 0 || src == 0 || src_end == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    memset(decoder, 0, sizeof(*decoder));
    decoder->stream.zalloc = tifx_zlib_alloc;
    decoder->stream.zfree = tifx_zlib_free;
    decoder->stream.opaque = &decoder->arena;
    decoder->stream.next_in = (Bytef *)src;
    decoder->stream.avail_in = (uInt)(src_end - src);
    zrc = inflateInit(&decoder->stream);
    if (zrc != Z_OK) {
        return tifx_map_zlib_status(zrc);
    }
    decoder->initialized = 1U;
    return TIFX_OK;
}

static int tifx_zlib_stored_decoder_read(tifx_zlib_stored_decoder *decoder,
                                         unsigned char *dst,
                                         unsigned long dst_len)
{
    int zrc;

    if (decoder == 0 || dst == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (!decoder->initialized) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    decoder->stream.next_out = dst;
    decoder->stream.avail_out = (uInt)dst_len;
    while (decoder->stream.avail_out != 0U) {
        zrc = inflate(&decoder->stream, Z_NO_FLUSH);
        if (zrc == Z_STREAM_END) {
            decoder->finished = 1U;
            break;
        }
        if (zrc == Z_OK) {
            if (decoder->stream.avail_out == 0U) {
                break;
            }
            if (decoder->stream.avail_in == 0U) {
                return TIFX_ERR_TRUNCATED;
            }
            continue;
        }
        return tifx_map_zlib_status(zrc);
    }
    if (decoder->stream.avail_out != 0U) {
        return decoder->finished ? TIFX_ERR_BAD_FORMAT : TIFX_ERR_TRUNCATED;
    }
    return TIFX_OK;
}

static int tifx_zlib_stored_decoder_drain_empty_final_blocks(tifx_zlib_stored_decoder *decoder)
{
    unsigned char sink[1];
    int zrc;

    if (decoder == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (!decoder->initialized) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    while (!decoder->finished) {
        decoder->stream.next_out = sink;
        decoder->stream.avail_out = 1U;
        zrc = inflate(&decoder->stream, Z_FINISH);
        if (zrc == Z_STREAM_END) {
            if (decoder->stream.avail_out == 0U) {
                return TIFX_ERR_BAD_FORMAT;
            }
            decoder->finished = 1U;
            break;
        }
        if (zrc == Z_OK) {
            if (decoder->stream.avail_out == 0U) {
                return TIFX_ERR_BAD_FORMAT;
            }
            if (decoder->stream.avail_in == 0U) {
                return TIFX_ERR_TRUNCATED;
            }
            continue;
        }
        if (zrc == Z_BUF_ERROR) {
            return decoder->stream.avail_in == 0U ? TIFX_ERR_TRUNCATED : TIFX_ERR_BAD_FORMAT;
        }
        return tifx_map_zlib_status(zrc);
    }
    return TIFX_OK;
}

static int tifx_zlib_stored_decoder_finish(tifx_zlib_stored_decoder *decoder)
{
    int rc;

    if (decoder == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (!decoder->initialized) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    rc = tifx_zlib_stored_decoder_drain_empty_final_blocks(decoder);
    inflateEnd(&decoder->stream);
    decoder->initialized = 0U;
    return rc;
}

static int tifx_packbits_process_segment(unsigned char *dst,
                                         unsigned long dst_size,
                                         const tifx_write_params *params,
                                         unsigned long stride_min,
                                         unsigned long segment_origin,
                                         unsigned long row_count,
                                         int is_tiled,
                                         unsigned long *out_written)
{
    unsigned long total;
    unsigned long row_in_segment;

    if (params == 0 || out_written == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    total = 0UL;
    for (row_in_segment = 0UL; row_in_segment < row_count; ++row_in_segment) {
        unsigned long i;

        i = 0UL;
        while (i < stride_min) {
            unsigned long repeat_count;

            repeat_count = tifx_packbits_repeat_count(params,
                                                      stride_min,
                                                      segment_origin,
                                                      row_in_segment,
                                                      stride_min,
                                                      i,
                                                      is_tiled);
            if (repeat_count >= 3UL) {
                unsigned long write_offset;
                unsigned char control;

                write_offset = total;
                if (!tifx_add_ul(total, 2UL, &total)) {
                    return TIFX_ERR_OVERFLOW;
                }
                if (dst != 0) {
                    if (total > dst_size) {
                        return TIFX_ERR_BAD_FORMAT;
                    }
                    control = (unsigned char)(1 - (int)repeat_count);
                    dst[write_offset + 0UL] = control;
                    dst[write_offset + 1UL] = tifx_packbits_row_byte(params,
                                                                     stride_min,
                                                                     segment_origin,
                                                                     row_in_segment,
                                                                     i,
                                                                     is_tiled);
                }
                i += repeat_count;
            } else {
                unsigned long literal_start;
                unsigned long literal_length;
                unsigned long write_offset;
                unsigned long literal_index;

                literal_start = i;
                literal_length = 0UL;
                while (i < stride_min) {
                    repeat_count = tifx_packbits_repeat_count(params,
                                                              stride_min,
                                                              segment_origin,
                                                              row_in_segment,
                                                              stride_min,
                                                              i,
                                                              is_tiled);
                    if (repeat_count >= 3UL) {
                        break;
                    }
                    if (literal_length + repeat_count > 128UL) {
                        repeat_count = 128UL - literal_length;
                    }
                    i += repeat_count;
                    literal_length += repeat_count;
                    if (literal_length == 128UL) {
                        break;
                    }
                }

                write_offset = total;
                if (!tifx_add_ul(total, 1UL + literal_length, &total)) {
                    return TIFX_ERR_OVERFLOW;
                }
                if (dst != 0) {
                    if (total > dst_size) {
                        return TIFX_ERR_BAD_FORMAT;
                    }
                    dst[write_offset] = (unsigned char)(literal_length - 1UL);
                    for (literal_index = 0UL; literal_index < literal_length; ++literal_index) {
                        dst[write_offset + 1UL + literal_index] =
                            tifx_packbits_row_byte(params,
                                                   stride_min,
                                                   segment_origin,
                                                   row_in_segment,
                                                   literal_start + literal_index,
                                                   is_tiled);
                    }
                }
            }
        }
    }

    *out_written = total;
    if (dst != 0 && total != dst_size) {
        return TIFX_ERR_BAD_FORMAT;
    }
    return TIFX_OK;
}

int tifx__write_strip_payload(unsigned char *dst,
                              unsigned long dst_size,
                              const tifx_write_params *params,
                              unsigned long stride_min,
                              unsigned long segment_index,
                              unsigned long start_row,
                              unsigned long row_count)
{
    unsigned short compression;
    unsigned short fill_order;
    unsigned short planar_config;
    unsigned long samples_per_pixel;
    int rc;

    if (dst == 0 || params == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    compression = params->compression;
    if (compression == 0U) {
        compression = 1U;
    }
    fill_order = tifx_effective_fill_order(params);
    planar_config = tifx_effective_write_planar_config(params);
    samples_per_pixel = tifx_write_sample_count(params);

    if (compression == 1U) {
        unsigned long expected_size;
        unsigned long row;

        if (!tifx_mul_ul(row_count, stride_min, &expected_size)) {
            return TIFX_ERR_OVERFLOW;
        }
        if (expected_size != dst_size) {
            return TIFX_ERR_BAD_FORMAT;
        }
        for (row = 0UL; row < row_count; ++row) {
            const unsigned char *src_row;
            unsigned char *dst_row;

            src_row = params->pixels + ((start_row + row) * params->stride);
            dst_row = dst + (row * stride_min);
            if ((params->pixel_format == TIFX_PIXEL_RGB24 ||
                 params->pixel_format == TIFX_PIXEL_RGBA32) &&
                planar_config == 2U && samples_per_pixel > 1UL) {
                unsigned long plane_index;
                unsigned long base_segment_index;
                unsigned long x;

                rc = tifx_map_write_segment_index(params,
                                                  0,
                                                  segment_index,
                                                  &plane_index,
                                                  &base_segment_index,
                                                  0);
                if (rc != TIFX_OK) {
                    return rc;
                }
                (void)base_segment_index;
                for (x = 0UL; x < params->width; ++x) {
                    dst_row[x] = src_row[(x * samples_per_pixel) + plane_index];
                }
            } else {
                memcpy(dst_row, src_row, stride_min);
            }
        }
        if (params->pixel_format == TIFX_PIXEL_BILEVEL && fill_order == 2U) {
            tifx_reverse_bytes_in_place(dst, dst_size);
        }
        return TIFX_OK;
    }

    if (compression == 32773U) {
        unsigned long written;

        return tifx_packbits_process_segment(dst,
                                             dst_size,
                                             params,
                                             stride_min,
                                             segment_index,
                                             row_count,
                                             0,
                                             &written);
    }
    if (compression == 5U) {
        unsigned long written;

        rc = tifx_lzw_process_segment(dst,
                                      dst_size,
                                      params,
                                      stride_min,
                                      segment_index,
                                      row_count,
                                      0,
                                      &written);
        if (rc != TIFX_OK) {
            return rc;
        }
        return (written == dst_size) ? TIFX_OK : TIFX_ERR_BAD_FORMAT;
    }
    if (compression == 8U || compression == 32946U) {
        unsigned long written;

        rc = tifx_deflate_stored_process_segment(dst,
                                                 dst_size,
                                                 params,
                                                 stride_min,
                                                 segment_index,
                                                 row_count,
                                                 0,
                                                 &written);
        if (rc != TIFX_OK) {
            return rc;
        }
        return (written == dst_size) ? TIFX_OK : TIFX_ERR_BAD_FORMAT;
    }

    {
        tifx_bit_writer writer;
        tifx_bit_writer_init(&writer, dst, dst_size);

        if (compression == 2U) {
            rc = tifx_encode_mh_payload_range(&writer, params, start_row, row_count);
        } else if (compression == 3U) {
            rc = tifx_encode_t4_payload_range(&writer, params, start_row, row_count);
        } else if (compression == 4U) {
            rc = tifx_encode_t6_payload_range(&writer, params, start_row, row_count);
        } else {
            return TIFX_ERR_UNSUPPORTED;
        }

        if (rc != TIFX_OK) {
            return rc;
        }
        if (tifx_bit_writer_bytes_used(&writer) != dst_size) {
            return TIFX_ERR_BAD_FORMAT;
        }
    }

    if (params->pixel_format == TIFX_PIXEL_BILEVEL && fill_order == 2U) {
        tifx_reverse_bytes_in_place(dst, dst_size);
    }

    return TIFX_OK;
}

int tifx__write_segment_payload(unsigned char *dst,
                                unsigned long dst_size,
                                const tifx_write_params *params,
                                unsigned long stride_min,
                                unsigned long segment_index,
                                unsigned long segment_span)
{
    unsigned short compression;
    unsigned long row_count;
    unsigned long plane_index;
    unsigned long base_segment_index;
    unsigned long start_row;
    int is_tiled;
    int rc;

    if (params == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    is_tiled = tifx__params_is_tiled(params);
    if (is_tiled < 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    compression = params->compression;
    if (compression == 0U) {
        compression = 1U;
    }

    if (is_tiled) {
        if (compression == 32773U) {
            unsigned long written;

            return tifx_packbits_process_segment(dst,
                                                 dst_size,
                                                 params,
                                                 stride_min,
                                                 segment_index,
                                                 segment_span,
                                                 1,
                                                 &written);
        }
        if (compression == 5U) {
            unsigned long written;

            rc = tifx_lzw_process_segment(dst,
                                          dst_size,
                                          params,
                                          stride_min,
                                          segment_index,
                                          segment_span,
                                          1,
                                          &written);
            if (rc != TIFX_OK) {
                return rc;
            }
            return (written == dst_size) ? TIFX_OK : TIFX_ERR_BAD_FORMAT;
        }
        if (compression == 8U || compression == 32946U) {
            unsigned long written;

            rc = tifx_deflate_stored_process_segment(dst,
                                                     dst_size,
                                                     params,
                                                     stride_min,
                                                     segment_index,
                                                     segment_span,
                                                     1,
                                                     &written);
            if (rc != TIFX_OK) {
                return rc;
            }
            return (written == dst_size) ? TIFX_OK : TIFX_ERR_BAD_FORMAT;
        }
        return tifx_write_tiled_uncompressed_payload(dst,
                                                     dst_size,
                                                     params,
                                                     stride_min,
                                                     segment_index,
                                                     segment_span);
    }

    rc = tifx_map_write_segment_index(params,
                                      0,
                                      segment_index,
                                      &plane_index,
                                      &base_segment_index,
                                      0);
    if (rc != TIFX_OK) {
        return rc;
    }
    (void)plane_index;

    start_row = base_segment_index * segment_span;
    if (start_row >= params->height) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    row_count = segment_span;
    if (row_count > params->height - start_row) {
        row_count = params->height - start_row;
    }

    return tifx__write_strip_payload(dst,
                                     dst_size,
                                     params,
                                     stride_min,
                                     segment_index,
                                     start_row,
                                     row_count);
}

static int tifx_validate_write_params(const tifx_write_params *params,
                                      unsigned long *out_stride_min,
                                      unsigned long *out_rows_per_strip,
                                      unsigned long *out_strip_count,
                                      unsigned long *out_image_size,
                                      unsigned short *out_tag_count,
                                      unsigned long *out_extra_size)
{
    unsigned long stride_min;
    unsigned long input_stride_min;
    unsigned long rows_per_strip;
    unsigned long strip_count;
    unsigned long image_size;
    unsigned long bytes_per_pixel;
    unsigned short tag_count;
    unsigned long extra_size;
    unsigned short compression;
    unsigned short fill_order;
    unsigned short planar_config;
    unsigned short predictor;
    unsigned long sample_count;
    int rc;
    int is_tiled;

    if (params == 0 || params->pixels == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (params->width == 0UL || params->height == 0UL) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    compression = params->compression;
    if (compression == 0U) {
        compression = 1U;
    }
    fill_order = tifx_effective_fill_order(params);
    planar_config = tifx_effective_write_planar_config(params);
    predictor = tifx_effective_predictor(params);
    sample_count = tifx_write_sample_count(params);
    if (predictor != TIFX_PREDICTOR_NONE && predictor != TIFX_PREDICTOR_HORIZONTAL) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (params->deflate_mode != TIFX_DEFLATE_AUTO &&
        params->deflate_mode != TIFX_DEFLATE_STORED &&
        params->deflate_mode != TIFX_DEFLATE_FIXED &&
        params->deflate_mode != TIFX_DEFLATE_DYNAMIC) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    is_tiled = tifx__params_is_tiled(params);
    if (is_tiled < 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    if (is_tiled) {
        rows_per_strip = params->tile_length;
        if ((params->tile_width & 15UL) != 0UL || (params->tile_length & 15UL) != 0UL) {
            return TIFX_ERR_UNSUPPORTED;
        }
        rc = tifx__compute_tile_count(params->width,
                                      params->height,
                                      params->tile_width,
                                      params->tile_length,
                                      &strip_count);
        if (rc != TIFX_OK) {
            return rc;
        }
    } else {
        rows_per_strip = tifx_effective_rows_per_strip(params);
        rc = tifx_compute_strip_count(params->height, rows_per_strip, &strip_count);
        if (rc != TIFX_OK) {
            return rc;
        }
    }

    if (planar_config == 2U && sample_count > 1UL) {
        unsigned long total_segments;
        unsigned long segment_limit;

        segment_limit = (unsigned long)(is_tiled ? TIFX_MAX_TILES : TIFX_MAX_STRIPS);
        if (!tifx_mul_ul(strip_count, sample_count, &total_segments)) {
            return TIFX_ERR_OVERFLOW;
        }
        if (total_segments == 0UL || total_segments > segment_limit) {
            return TIFX_ERR_UNSUPPORTED;
        }
        strip_count = total_segments;
    }

    if (params->pixel_format == TIFX_PIXEL_GRAY8) {
        if ((compression != 1U && compression != 32773U && compression != 5U &&
             compression != 8U && compression != 32946U) || fill_order != 1U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (predictor == TIFX_PREDICTOR_HORIZONTAL &&
            compression != 5U && compression != 8U && compression != 32946U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (planar_config != 1U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        bytes_per_pixel = 1UL;
        tag_count = (unsigned short)(is_tiled ? 13U : 12U);
        extra_size = 16UL;
        if (!tifx_mul_ul(params->width, bytes_per_pixel, &input_stride_min)) {
            return TIFX_ERR_OVERFLOW;
        }
        if (is_tiled) {
            if (!tifx_mul_ul(params->tile_width, bytes_per_pixel, &stride_min)) {
                return TIFX_ERR_OVERFLOW;
            }
        } else {
            stride_min = input_stride_min;
        }
    } else if (params->pixel_format == TIFX_PIXEL_RGB24) {
        if ((compression != 1U && compression != 32773U && compression != 5U &&
             compression != 8U && compression != 32946U) || fill_order != 1U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (predictor == TIFX_PREDICTOR_HORIZONTAL &&
            compression != 5U && compression != 8U && compression != 32946U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (planar_config != 1U && planar_config != 2U) {
            return TIFX_ERR_BAD_ARGUMENT;
        }
        bytes_per_pixel = 3UL;
        tag_count = (unsigned short)(is_tiled ? 15U : 14U);
        extra_size = 22UL;
        if (!tifx_mul_ul(params->width, bytes_per_pixel, &input_stride_min)) {
            return TIFX_ERR_OVERFLOW;
        }
        if (planar_config == 2U) {
            stride_min = is_tiled ? params->tile_width : params->width;
        } else if (is_tiled) {
            if (!tifx_mul_ul(params->tile_width, bytes_per_pixel, &stride_min)) {
                return TIFX_ERR_OVERFLOW;
            }
        } else {
            stride_min = input_stride_min;
        }
    } else if (params->pixel_format == TIFX_PIXEL_RGBA32) {
        if ((compression != 1U && compression != 32773U && compression != 5U &&
             compression != 8U && compression != 32946U) || fill_order != 1U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (predictor == TIFX_PREDICTOR_HORIZONTAL &&
            compression != 5U && compression != 8U && compression != 32946U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (planar_config != 1U && planar_config != 2U) {
            return TIFX_ERR_BAD_ARGUMENT;
        }
        if (params->alpha_mode != TIFX_ALPHA_ASSOCIATED &&
            params->alpha_mode != TIFX_ALPHA_UNASSOCIATED) {
            return TIFX_ERR_BAD_ARGUMENT;
        }
        bytes_per_pixel = 4UL;
        tag_count = (unsigned short)(is_tiled ? 16U : 15U);
        extra_size = 24UL;
        if (!tifx_mul_ul(params->width, bytes_per_pixel, &input_stride_min)) {
            return TIFX_ERR_OVERFLOW;
        }
        if (planar_config == 2U) {
            stride_min = is_tiled ? params->tile_width : params->width;
        } else if (is_tiled) {
            if (!tifx_mul_ul(params->tile_width, bytes_per_pixel, &stride_min)) {
                return TIFX_ERR_OVERFLOW;
            }
        } else {
            stride_min = input_stride_min;
        }
    } else if (params->pixel_format == TIFX_PIXEL_BILEVEL) {
        unsigned long strip_byte_counts[TIFX_MAX_TILES > TIFX_MAX_STRIPS ? TIFX_MAX_TILES : TIFX_MAX_STRIPS];

        if (planar_config != 1U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        input_stride_min = tifx_ceil_div(params->width, 8UL);
        stride_min = is_tiled ? tifx_ceil_div(params->tile_width, 8UL) : input_stride_min;
        extra_size = 16UL;
        if (params->photometric != 0U && params->photometric != 1U) {
            return TIFX_ERR_BAD_ARGUMENT;
        }
        if (fill_order != 1U && fill_order != 2U) {
            return TIFX_ERR_BAD_ARGUMENT;
        }
        if (predictor != TIFX_PREDICTOR_NONE) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (is_tiled) {
            if (compression != 1U && compression != 32773U) {
                return TIFX_ERR_UNSUPPORTED;
            }
            tag_count = 14U;
        } else if (compression == 1U || compression == 2U || compression == 32773U ||
                   compression == 5U || compression == 8U || compression == 32946U) {
            if ((compression == 5U || compression == 8U || compression == 32946U) && fill_order != 1U) {
                return TIFX_ERR_UNSUPPORTED;
            }
            tag_count = 13U;
        } else if (compression == 3U) {
            if ((params->t4_options & ~5UL) != 0UL) {
                return TIFX_ERR_UNSUPPORTED;
            }
            tag_count = 14U;
        } else if (compression == 4U) {
            if (params->t6_options != 0UL) {
                return TIFX_ERR_UNSUPPORTED;
            }
            tag_count = 14U;
        } else {
            return TIFX_ERR_UNSUPPORTED;
        }

        rc = tifx__build_strip_byte_counts(params,
                                          stride_min,
                                          rows_per_strip,
                                          strip_count,
                                          strip_byte_counts,
                                          &image_size);
        if (rc != TIFX_OK) {
            return rc;
        }
    } else {
        return TIFX_ERR_UNSUPPORTED;
    }

    if (params->pixel_format != TIFX_PIXEL_BILEVEL) {
        unsigned long segment_byte_counts[TIFX_MAX_TILES > TIFX_MAX_STRIPS ? TIFX_MAX_TILES : TIFX_MAX_STRIPS];
        rc = tifx__build_strip_byte_counts(params,
                                          stride_min,
                                          rows_per_strip,
                                          strip_count,
                                          segment_byte_counts,
                                          &image_size);
        if (rc != TIFX_OK) {
            return rc;
        }
    }

    if (predictor == TIFX_PREDICTOR_HORIZONTAL) {
        ++tag_count;
    }
    if (params->stride < input_stride_min) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (strip_count > 1UL) {
        unsigned long arrays_size;
        if (!tifx_mul_ul(strip_count, 8UL, &arrays_size)) {
            return TIFX_ERR_OVERFLOW;
        }
        if (!tifx_add_ul(extra_size, arrays_size, &extra_size)) {
            return TIFX_ERR_OVERFLOW;
        }
    }
    if (image_size > 0xFFFFFFFFUL) {
        return TIFX_ERR_UNSUPPORTED;
    }

    *out_stride_min = stride_min;
    *out_rows_per_strip = rows_per_strip;
    *out_strip_count = strip_count;
    *out_image_size = image_size;
    *out_tag_count = tag_count;
    *out_extra_size = extra_size;
    return TIFX_OK;
}

unsigned long tifx_write_classic_buffer_size(const tifx_write_params *params)
{
    unsigned long stride_min;
    unsigned long rows_per_strip;
    unsigned long strip_count;
    unsigned long image_size;
    unsigned short tag_count;
    unsigned long extra_size;
    unsigned long ifd_size;
    unsigned long total_size;
    unsigned long image_pad;
    int rc;

    rc = tifx_validate_write_params(params,
                                    &stride_min,
                                    &rows_per_strip,
                                    &strip_count,
                                    &image_size,
                                    &tag_count,
                                    &extra_size);
    if (rc != TIFX_OK) {
        return 0UL;
    }
    (void)stride_min;
    (void)rows_per_strip;
    (void)strip_count;

    image_pad = image_size & 1UL;
    ifd_size = 2UL + ((unsigned long)tag_count * 12UL) + 4UL;

    if (!tifx_add_ul(8UL, image_size, &total_size)) {
        return 0UL;
    }
    if (!tifx_add_ul(total_size, image_pad, &total_size)) {
        return 0UL;
    }
    if (!tifx_add_ul(total_size, ifd_size, &total_size)) {
        return 0UL;
    }
    if (!tifx_add_ul(total_size, extra_size, &total_size)) {
        return 0UL;
    }
    if (total_size > 0xFFFFFFFFUL) {
        return 0UL;
    }
    return total_size;
}

int tifx_write_classic_memory(void *dst,
                      unsigned long dst_size,
                      const tifx_write_params *params,
                      unsigned long *written_size)
{
    unsigned long stride_min;
    unsigned long rows_per_strip;
    unsigned long strip_count;
    unsigned long image_size;
    unsigned short tag_count;
    unsigned long extra_size;
    unsigned long total_size;
    unsigned long image_pad;
    unsigned long ifd_offset;
    unsigned long ifd_size;
    unsigned long extra_offset;
    unsigned long bits_offset;
    unsigned long xres_offset;
    unsigned long yres_offset;
    unsigned long strip_offsets_array_offset;
    unsigned long strip_byte_counts_array_offset;
    unsigned long extra_cursor;
    unsigned short entry_index;
    unsigned short resolution_unit;
    unsigned short compression;
    unsigned short photometric;
    unsigned short fill_order;
    unsigned short planar_config;
    unsigned short predictor;
    unsigned long strip_byte_counts[TIFX_MAX_STRIPS];
    unsigned long strip_offsets[TIFX_MAX_STRIPS];
    unsigned long strip_index;
    int is_tiled;
    tifx_fixed xres;
    tifx_fixed yres;
    unsigned char *out;
    int rc;

    if (dst == 0 || params == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    rc = tifx_validate_write_params(params,
                                    &stride_min,
                                    &rows_per_strip,
                                    &strip_count,
                                    &image_size,
                                    &tag_count,
                                    &extra_size);
    if (rc != TIFX_OK) {
        return rc;
    }

    compression = params->compression;
    if (compression == 0U) {
        compression = 1U;
    }
    fill_order = tifx_effective_fill_order(params);
    planar_config = tifx_effective_write_planar_config(params);
    predictor = tifx_effective_predictor(params);
    is_tiled = tifx__params_is_tiled(params);
    if (is_tiled < 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (params->pixel_format == TIFX_PIXEL_GRAY8) {
        photometric = 1U;
    } else if (params->pixel_format == TIFX_PIXEL_RGB24 ||
               params->pixel_format == TIFX_PIXEL_RGBA32) {
        photometric = 2U;
    } else {
        photometric = params->photometric;
    }

    rc = tifx__build_strip_byte_counts(params,
                                      stride_min,
                                      rows_per_strip,
                                      strip_count,
                                      strip_byte_counts,
                                      &image_size);
    if (rc != TIFX_OK) {
        return rc;
    }

    total_size = tifx_write_classic_buffer_size(params);
    if (total_size == 0UL) {
        return TIFX_ERR_OVERFLOW;
    }
    if (dst_size < total_size) {
        return TIFX_ERR_NO_SPACE;
    }

    image_pad = image_size & 1UL;
    ifd_offset = 8UL + image_size + image_pad;
    ifd_size = 2UL + ((unsigned long)tag_count * 12UL) + 4UL;
    extra_offset = ifd_offset + ifd_size;

    bits_offset = 0UL;
    xres_offset = 0UL;
    yres_offset = 0UL;
    strip_offsets_array_offset = 0UL;
    strip_byte_counts_array_offset = 0UL;

    extra_cursor = extra_offset;
    if (params->pixel_format == TIFX_PIXEL_RGB24) {
        bits_offset = extra_cursor;
        extra_cursor += 6UL;
    } else if (params->pixel_format == TIFX_PIXEL_RGBA32) {
        bits_offset = extra_cursor;
        extra_cursor += 8UL;
    }
    xres_offset = extra_cursor;
    extra_cursor += 8UL;
    yres_offset = extra_cursor;
    extra_cursor += 8UL;
    if (strip_count > 1UL) {
        strip_offsets_array_offset = extra_cursor;
        extra_cursor += strip_count * 4UL;
        strip_byte_counts_array_offset = extra_cursor;
        extra_cursor += strip_count * 4UL;
    }

    out = (unsigned char *)dst;
    memset(out, 0, total_size);

    out[0] = 'I';
    out[1] = 'I';
    tifx_write_u16le(out + 2, 42U);
    tifx_write_u32le(out + 4, ifd_offset);

    {
        unsigned long image_cursor;

        image_cursor = 8UL;
        for (strip_index = 0UL; strip_index < strip_count; ++strip_index) {
            strip_offsets[strip_index] = image_cursor;

            rc = tifx__write_segment_payload(out + image_cursor,
                                            strip_byte_counts[strip_index],
                                            params,
                                            stride_min,
                                            strip_index,
                                            rows_per_strip);
            if (rc != TIFX_OK) {
                return rc;
            }

            if (!tifx_add_ul(image_cursor, strip_byte_counts[strip_index], &image_cursor)) {
                return TIFX_ERR_OVERFLOW;
            }
        }
        if (image_cursor != 8UL + image_size) {
            return TIFX_ERR_BAD_FORMAT;
        }
    }

    tifx_write_u16le(out + ifd_offset, tag_count);
    entry_index = 0U;

    rc = tifx_write_tag_le(out + ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                           TIFX_TAG_IMAGE_WIDTH, TIFX_TYPE_LONG, 1UL, params->width);
    if (rc != TIFX_OK) return rc;
    rc = tifx_write_tag_le(out + ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                           TIFX_TAG_IMAGE_LENGTH, TIFX_TYPE_LONG, 1UL, params->height);
    if (rc != TIFX_OK) return rc;

    if (params->pixel_format == TIFX_PIXEL_GRAY8) {
        rc = tifx_write_tag_le(out + ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_BITS_PER_SAMPLE, TIFX_TYPE_SHORT, 1UL, 8UL);
        if (rc != TIFX_OK) return rc;
    } else if (params->pixel_format == TIFX_PIXEL_RGB24) {
        rc = tifx_write_tag_le(out + ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_BITS_PER_SAMPLE, TIFX_TYPE_SHORT, 3UL, bits_offset);
        if (rc != TIFX_OK) return rc;
    } else if (params->pixel_format == TIFX_PIXEL_RGBA32) {
        rc = tifx_write_tag_le(out + ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_BITS_PER_SAMPLE, TIFX_TYPE_SHORT, 4UL, bits_offset);
        if (rc != TIFX_OK) return rc;
    } else {
        rc = tifx_write_tag_le(out + ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_BITS_PER_SAMPLE, TIFX_TYPE_SHORT, 1UL, 1UL);
        if (rc != TIFX_OK) return rc;
    }

    rc = tifx_write_tag_le(out + ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                           TIFX_TAG_COMPRESSION, TIFX_TYPE_SHORT, 1UL,
                           (unsigned long)compression);
    if (rc != TIFX_OK) return rc;
    rc = tifx_write_tag_le(out + ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                           TIFX_TAG_PHOTOMETRIC, TIFX_TYPE_SHORT, 1UL,
                           (unsigned long)photometric);
    if (rc != TIFX_OK) return rc;
    if (params->pixel_format == TIFX_PIXEL_BILEVEL) {
        rc = tifx_write_tag_le(out + ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_FILL_ORDER, TIFX_TYPE_SHORT, 1UL,
                               (unsigned long)fill_order);
        if (rc != TIFX_OK) return rc;
    }
    if (is_tiled) {
        rc = tifx_write_tag_le(out + ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_TILE_WIDTH, TIFX_TYPE_LONG, 1UL, params->tile_width);
        if (rc != TIFX_OK) return rc;
        rc = tifx_write_tag_le(out + ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_TILE_LENGTH, TIFX_TYPE_LONG, 1UL, params->tile_length);
        if (rc != TIFX_OK) return rc;
        rc = tifx_write_tag_le(out + ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_TILE_OFFSETS, TIFX_TYPE_LONG, strip_count,
                               (strip_count == 1UL) ? strip_offsets[0] : strip_offsets_array_offset);
        if (rc != TIFX_OK) return rc;
    } else {
        rc = tifx_write_tag_le(out + ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_STRIP_OFFSETS, TIFX_TYPE_LONG, strip_count,
                               (strip_count == 1UL) ? strip_offsets[0] : strip_offsets_array_offset);
        if (rc != TIFX_OK) return rc;
    }
    rc = tifx_write_tag_le(out + ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                           TIFX_TAG_ORIENTATION, TIFX_TYPE_SHORT, 1UL, 1UL);
    if (rc != TIFX_OK) return rc;

    if (params->pixel_format == TIFX_PIXEL_RGB24 || params->pixel_format == TIFX_PIXEL_RGBA32) {
        unsigned long samples_per_pixel_value;
        samples_per_pixel_value = (params->pixel_format == TIFX_PIXEL_RGBA32) ? 4UL : 3UL;
        rc = tifx_write_tag_le(out + ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_SAMPLES_PER_PIXEL, TIFX_TYPE_SHORT, 1UL, samples_per_pixel_value);
        if (rc != TIFX_OK) return rc;
    }

    if (is_tiled) {
        rc = tifx_write_tag_le(out + ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_TILE_BYTE_COUNTS, TIFX_TYPE_LONG, strip_count,
                               (strip_count == 1UL) ? strip_byte_counts[0] : strip_byte_counts_array_offset);
        if (rc != TIFX_OK) return rc;
    } else {
        rc = tifx_write_tag_le(out + ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_ROWS_PER_STRIP, TIFX_TYPE_LONG, 1UL, rows_per_strip);
        if (rc != TIFX_OK) return rc;
        rc = tifx_write_tag_le(out + ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_STRIP_BYTE_COUNTS, TIFX_TYPE_LONG, strip_count,
                               (strip_count == 1UL) ? strip_byte_counts[0] : strip_byte_counts_array_offset);
        if (rc != TIFX_OK) return rc;
    }
    if (predictor == TIFX_PREDICTOR_HORIZONTAL) {
        rc = tifx_write_tag_le(out + ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_PREDICTOR, TIFX_TYPE_SHORT, 1UL, (unsigned long)predictor);
        if (rc != TIFX_OK) return rc;
    }
    rc = tifx_write_tag_le(out + ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                           TIFX_TAG_X_RESOLUTION, TIFX_TYPE_RATIONAL, 1UL, xres_offset);
    if (rc != TIFX_OK) return rc;
    rc = tifx_write_tag_le(out + ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                           TIFX_TAG_Y_RESOLUTION, TIFX_TYPE_RATIONAL, 1UL, yres_offset);
    if (rc != TIFX_OK) return rc;

    if (params->pixel_format == TIFX_PIXEL_RGB24 || params->pixel_format == TIFX_PIXEL_RGBA32) {
        rc = tifx_write_tag_le(out + ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_PLANAR_CONFIGURATION, TIFX_TYPE_SHORT, 1UL,
                               (unsigned long)planar_config);
        if (rc != TIFX_OK) return rc;
    }
    if (params->pixel_format == TIFX_PIXEL_RGBA32) {
        rc = tifx_write_tag_le(out + ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_EXTRA_SAMPLES, TIFX_TYPE_SHORT, 1UL, params->alpha_mode);
        if (rc != TIFX_OK) return rc;
    }
    if (!is_tiled && params->pixel_format == TIFX_PIXEL_BILEVEL && compression == 3U) {
        rc = tifx_write_tag_le(out + ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_T4_OPTIONS, TIFX_TYPE_LONG, 1UL, params->t4_options);
        if (rc != TIFX_OK) return rc;
    }
    if (!is_tiled && params->pixel_format == TIFX_PIXEL_BILEVEL && compression == 4U) {
        rc = tifx_write_tag_le(out + ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_T6_OPTIONS, TIFX_TYPE_LONG, 1UL, params->t6_options);
        if (rc != TIFX_OK) return rc;
    }

    resolution_unit = params->resolution_unit;
    if (resolution_unit == 0U) {
        resolution_unit = 2U;
    }
    rc = tifx_write_tag_le(out + ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                           TIFX_TAG_RESOLUTION_UNIT, TIFX_TYPE_SHORT, 1UL,
                           (unsigned long)resolution_unit);
    if (rc != TIFX_OK) return rc;

    tifx_write_u32le(out + ifd_offset + 2UL + ((unsigned long)tag_count * 12UL), 0UL);

    if (params->pixel_format == TIFX_PIXEL_RGB24) {
        tifx_write_u16le(out + bits_offset + 0UL, 8U);
        tifx_write_u16le(out + bits_offset + 2UL, 8U);
        tifx_write_u16le(out + bits_offset + 4UL, 8U);
    } else if (params->pixel_format == TIFX_PIXEL_RGBA32) {
        tifx_write_u16le(out + bits_offset + 0UL, 8U);
        tifx_write_u16le(out + bits_offset + 2UL, 8U);
        tifx_write_u16le(out + bits_offset + 4UL, 8U);
        tifx_write_u16le(out + bits_offset + 6UL, 8U);
    }

    xres = params->x_resolution;
    yres = params->y_resolution;
    if (xres <= 0) xres = TIFX_FP_FROM_INT(72L);
    if (yres <= 0) yres = TIFX_FP_FROM_INT(72L);

    tifx_write_u32le(out + xres_offset + 0UL, tifx_fp_to_rational_numerator(xres));
    tifx_write_u32le(out + xres_offset + 4UL, tifx_fp_to_rational_denominator());
    tifx_write_u32le(out + yres_offset + 0UL, tifx_fp_to_rational_numerator(yres));
    tifx_write_u32le(out + yres_offset + 4UL, tifx_fp_to_rational_denominator());

    if (strip_count > 1UL) {
        tifx_write_u32le_array(out + strip_offsets_array_offset, strip_offsets, strip_count);
        tifx_write_u32le_array(out + strip_byte_counts_array_offset, strip_byte_counts, strip_count);
    }

    if (written_size != 0) {
        *written_size = total_size;
    }
    return TIFX_OK;
}


static unsigned long tifx_align2(unsigned long value)
{
    return (value + 1UL) & ~1UL;
}

static unsigned long tifx_effective_new_subfile_type(const tifx_write_params *params,
                                                     unsigned short auto_page_bit)
{
    unsigned long value;

    value = 0UL;
    if (params != 0) {
        value = params->new_subfile_type;
    }
    if (auto_page_bit != 0U) {
        value |= (unsigned long)auto_page_bit;
    }
    return value;
}

static unsigned short tifx_effective_subifd_style(const tifx_write_params *params)
{
    if (params == 0) {
        return TIFX_SUBIFD_STYLE_TREE;
    }
    if (params->subifd_style == TIFX_SUBIFD_STYLE_ADOBE_CHAIN) {
        return TIFX_SUBIFD_STYLE_ADOBE_CHAIN;
    }
    return TIFX_SUBIFD_STYLE_TREE;
}

static void tifx_write_tag_inline_shorts_le(unsigned char *entry,
                                            unsigned short tag,
                                            const unsigned short *values,
                                            unsigned long count)
{
    memset(entry, 0, 12U);
    tifx_write_u16le(entry + 0, tag);
    tifx_write_u16le(entry + 2, TIFX_TYPE_SHORT);
    tifx_write_u32le(entry + 4, count);
    if (count > 0UL) {
        tifx_write_u16le(entry + 8, values[0]);
    }
    if (count > 1UL) {
        tifx_write_u16le(entry + 10, values[1]);
    }
}

static int tifx_measure_classic_tree_node_layout(const tifx_tiff_node *node,
                                                 unsigned long page_base_offset,
                                                 unsigned short auto_page_bit,
                                                 unsigned long *out_ifd_offset,
                                                 unsigned long *out_node_end,
                                                 unsigned long *out_child_ifd_offsets,
                                                 unsigned long depth)
{
    const tifx_write_params *params;
    unsigned long stride_min;
    unsigned long segment_span;
    unsigned long segment_count;
    unsigned long image_size;
    unsigned short base_tag_count;
    unsigned long extra_size;
    unsigned short tag_count;
    unsigned long segment_byte_counts[TIFX_MAX_TILES > TIFX_MAX_STRIPS ? TIFX_MAX_TILES : TIFX_MAX_STRIPS];
    unsigned long cursor;
    unsigned long ifd_offset;
    unsigned long ifd_size;
    unsigned long child_index;
    unsigned long child_ifd_offset;
    unsigned long child_end;
    unsigned long effective_new_subfile_type;
    unsigned short subifd_style;
    int rc;

    if (node == 0 || out_ifd_offset == 0 || out_node_end == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (depth > TIFX_MAX_SUBIFD_DEPTH) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (node->child_count > TIFX_MAX_SUBIFDS) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (node->child_count > 0UL && node->children == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    params = &node->image;
    rc = tifx_validate_write_params(params,
                                    &stride_min,
                                    &segment_span,
                                    &segment_count,
                                    &image_size,
                                    &base_tag_count,
                                    &extra_size);
    if (rc != TIFX_OK) {
        return rc;
    }

    subifd_style = tifx_effective_subifd_style(params);
    effective_new_subfile_type = tifx_effective_new_subfile_type(params, auto_page_bit);
    tag_count = base_tag_count;
    if (effective_new_subfile_type != 0UL) {
        if ((unsigned long)tag_count == 65535UL) {
            return TIFX_ERR_OVERFLOW;
        }
        ++tag_count;
    }
    if (auto_page_bit != 0U) {
        if ((unsigned long)tag_count == 65535UL) {
            return TIFX_ERR_OVERFLOW;
        }
        ++tag_count;
    }
    if (node->child_count > 0UL) {
        if ((unsigned long)tag_count == 65535UL) {
            return TIFX_ERR_OVERFLOW;
        }
        ++tag_count;
    }

    rc = tifx__build_strip_byte_counts(params,
                                       stride_min,
                                       segment_span,
                                       segment_count,
                                       segment_byte_counts,
                                       &image_size);
    if (rc != TIFX_OK) {
        return rc;
    }

    cursor = tifx_align2(page_base_offset);
    for (child_index = 0UL; child_index < segment_count; ++child_index) {
        cursor = tifx_align2(cursor);
        if (!tifx_add_ul(cursor, segment_byte_counts[child_index], &cursor)) {
            return TIFX_ERR_OVERFLOW;
        }
    }

    for (child_index = 0UL; child_index < node->child_count; ++child_index) {
        cursor = tifx_align2(cursor);
        rc = tifx_measure_classic_tree_node_layout(&node->children[child_index],
                                                   cursor,
                                                   0U,
                                                   &child_ifd_offset,
                                                   &child_end,
                                                   0,
                                                   depth + 1UL);
        if (rc != TIFX_OK) {
            return rc;
        }
        if (out_child_ifd_offsets != 0) {
            out_child_ifd_offsets[child_index] = child_ifd_offset;
        }
        cursor = child_end;
    }

    ifd_offset = tifx_align2(cursor);
    if (!tifx_mul_ul((unsigned long)tag_count, 12UL, &ifd_size)) {
        return TIFX_ERR_OVERFLOW;
    }
    if (!tifx_add_ul(ifd_size, 6UL, &ifd_size)) {
        return TIFX_ERR_OVERFLOW;
    }
    cursor = ifd_offset + ifd_size;

    if (!tifx_add_ul(cursor, extra_size, &cursor)) {
        return TIFX_ERR_OVERFLOW;
    }
    if (node->child_count > 1UL && subifd_style == TIFX_SUBIFD_STYLE_TREE) {
        unsigned long child_array_size;
        if (!tifx_mul_ul(node->child_count, 4UL, &child_array_size)) {
            return TIFX_ERR_OVERFLOW;
        }
        if (!tifx_add_ul(cursor, child_array_size, &cursor)) {
            return TIFX_ERR_OVERFLOW;
        }
    }
    if (cursor > 0xFFFFFFFFUL) {
        return TIFX_ERR_OVERFLOW;
    }

    *out_ifd_offset = ifd_offset;
    *out_node_end = cursor;
    return TIFX_OK;
}

static int tifx_write_classic_tree_node(unsigned char *out,
                                        const tifx_tiff_node *node,
                                        unsigned long page_base_offset,
                                        unsigned long current_ifd_offset,
                                        unsigned long next_ifd_offset,
                                        unsigned short auto_page_bit,
                                        unsigned long page_index,
                                        unsigned long page_count,
                                        unsigned long *out_node_end,
                                        unsigned long depth)
{
    const tifx_write_params *params;
    unsigned long stride_min;
    unsigned long segment_span;
    unsigned long segment_count;
    unsigned long image_size;
    unsigned short base_tag_count;
    unsigned long extra_size;
    unsigned short tag_count;
    unsigned short compression;
    unsigned short photometric;
    unsigned short fill_order;
    unsigned short planar_config;
    unsigned short predictor;
    unsigned short resolution_unit;
    unsigned long segment_offsets[TIFX_MAX_TILES > TIFX_MAX_STRIPS ? TIFX_MAX_TILES : TIFX_MAX_STRIPS];
    unsigned long segment_byte_counts[TIFX_MAX_TILES > TIFX_MAX_STRIPS ? TIFX_MAX_TILES : TIFX_MAX_STRIPS];
    unsigned long child_ifd_offsets[TIFX_MAX_SUBIFDS];
    unsigned long child_base_offsets[TIFX_MAX_SUBIFDS];
    unsigned long child_end_offsets[TIFX_MAX_SUBIFDS];
    unsigned long cursor;
    unsigned long ifd_size;
    unsigned long extra_cursor;
    unsigned long bits_offset;
    unsigned long xres_offset;
    unsigned long yres_offset;
    unsigned long segment_offsets_array_offset;
    unsigned long segment_byte_counts_array_offset;
    unsigned long subifd_offsets_array_offset;
    unsigned long child_index;
    unsigned long child_end;
    unsigned long effective_new_subfile_type;
    unsigned short subifd_style;
    unsigned short page_number_values[2];
    unsigned short entry_index;
    tifx_fixed xres;
    tifx_fixed yres;
    int rc;
    int is_tiled;

    if (out == 0 || node == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (depth > TIFX_MAX_SUBIFD_DEPTH) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (node->child_count > TIFX_MAX_SUBIFDS) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (node->child_count > 0UL && node->children == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    params = &node->image;
    rc = tifx_validate_write_params(params,
                                    &stride_min,
                                    &segment_span,
                                    &segment_count,
                                    &image_size,
                                    &base_tag_count,
                                    &extra_size);
    if (rc != TIFX_OK) {
        return rc;
    }

    compression = params->compression;
    if (compression == 0U) {
        compression = 1U;
    }
    fill_order = tifx_effective_fill_order(params);
    planar_config = tifx_effective_write_planar_config(params);
    predictor = tifx_effective_predictor(params);
    if (params->pixel_format == TIFX_PIXEL_GRAY8) {
        photometric = 1U;
    } else if (params->pixel_format == TIFX_PIXEL_RGB24 ||
               params->pixel_format == TIFX_PIXEL_RGBA32) {
        photometric = 2U;
    } else {
        photometric = params->photometric;
    }
    is_tiled = tifx__params_is_tiled(params);
    if (is_tiled < 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    subifd_style = tifx_effective_subifd_style(params);
    effective_new_subfile_type = tifx_effective_new_subfile_type(params, auto_page_bit);
    tag_count = base_tag_count;
    if (effective_new_subfile_type != 0UL) {
        ++tag_count;
    }
    if (auto_page_bit != 0U) {
        ++tag_count;
    }
    if (node->child_count > 0UL) {
        ++tag_count;
    }

    rc = tifx__build_strip_byte_counts(params,
                                       stride_min,
                                       segment_span,
                                       segment_count,
                                       segment_byte_counts,
                                       &image_size);
    if (rc != TIFX_OK) {
        return rc;
    }

    cursor = tifx_align2(page_base_offset);
    for (child_index = 0UL; child_index < segment_count; ++child_index) {
        cursor = tifx_align2(cursor);
        segment_offsets[child_index] = cursor;
        rc = tifx__write_segment_payload(out + cursor,
                                         segment_byte_counts[child_index],
                                         params,
                                         stride_min,
                                         child_index,
                                         segment_span);
        if (rc != TIFX_OK) {
            return rc;
        }
        if (!tifx_add_ul(cursor, segment_byte_counts[child_index], &cursor)) {
            return TIFX_ERR_OVERFLOW;
        }
    }

    for (child_index = 0UL; child_index < node->child_count; ++child_index) {
        unsigned long child_ifd_offset;

        cursor = tifx_align2(cursor);
        child_base_offsets[child_index] = cursor;
        rc = tifx_measure_classic_tree_node_layout(&node->children[child_index],
                                                   cursor,
                                                   0U,
                                                   &child_ifd_offset,
                                                   &child_end,
                                                   0,
                                                   depth + 1UL);
        if (rc != TIFX_OK) {
            return rc;
        }
        child_ifd_offsets[child_index] = child_ifd_offset;
        child_end_offsets[child_index] = child_end;
        cursor = child_end;
    }

    for (child_index = 0UL; child_index < node->child_count; ++child_index) {
        unsigned long child_next_ifd_offset;

        child_next_ifd_offset = 0UL;
        if (subifd_style == TIFX_SUBIFD_STYLE_ADOBE_CHAIN && child_index + 1UL < node->child_count) {
            child_next_ifd_offset = child_ifd_offsets[child_index + 1UL];
        }
        child_end = child_end_offsets[child_index];
        rc = tifx_write_classic_tree_node(out,
                                          &node->children[child_index],
                                          child_base_offsets[child_index],
                                          child_ifd_offsets[child_index],
                                          child_next_ifd_offset,
                                          0U,
                                          0UL,
                                          0UL,
                                          &child_end,
                                          depth + 1UL);
        if (rc != TIFX_OK) {
            return rc;
        }
        if (child_end != child_end_offsets[child_index]) {
            return TIFX_ERR_BAD_FORMAT;
        }
    }

    cursor = tifx_align2(cursor);
    if (cursor != current_ifd_offset) {
        return TIFX_ERR_BAD_FORMAT;
    }

    if (!tifx_mul_ul((unsigned long)tag_count, 12UL, &ifd_size)) {
        return TIFX_ERR_OVERFLOW;
    }
    if (!tifx_add_ul(ifd_size, 6UL, &ifd_size)) {
        return TIFX_ERR_OVERFLOW;
    }

    extra_cursor = current_ifd_offset + ifd_size;
    bits_offset = 0UL;
    xres_offset = 0UL;
    yres_offset = 0UL;
    segment_offsets_array_offset = 0UL;
    segment_byte_counts_array_offset = 0UL;
    subifd_offsets_array_offset = 0UL;

    if (params->pixel_format == TIFX_PIXEL_RGB24) {
        bits_offset = extra_cursor;
        extra_cursor += 6UL;
    } else if (params->pixel_format == TIFX_PIXEL_RGBA32) {
        bits_offset = extra_cursor;
        extra_cursor += 8UL;
    }
    xres_offset = extra_cursor;
    extra_cursor += 8UL;
    yres_offset = extra_cursor;
    extra_cursor += 8UL;
    if (segment_count > 1UL) {
        segment_offsets_array_offset = extra_cursor;
        extra_cursor += segment_count * 4UL;
        segment_byte_counts_array_offset = extra_cursor;
        extra_cursor += segment_count * 4UL;
    }
    if (node->child_count > 1UL && subifd_style == TIFX_SUBIFD_STYLE_TREE) {
        subifd_offsets_array_offset = extra_cursor;
        extra_cursor += node->child_count * 4UL;
    }

    tifx_write_u16le(out + current_ifd_offset, tag_count);
    entry_index = 0U;

    if (effective_new_subfile_type != 0UL) {
        rc = tifx_write_tag_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_NEW_SUBFILE_TYPE,
                               TIFX_TYPE_LONG,
                               1UL,
                               effective_new_subfile_type);
        if (rc != TIFX_OK) return rc;
    }

    rc = tifx_write_tag_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                           TIFX_TAG_IMAGE_WIDTH, TIFX_TYPE_LONG, 1UL, params->width);
    if (rc != TIFX_OK) return rc;
    rc = tifx_write_tag_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                           TIFX_TAG_IMAGE_LENGTH, TIFX_TYPE_LONG, 1UL, params->height);
    if (rc != TIFX_OK) return rc;

    if (params->pixel_format == TIFX_PIXEL_GRAY8) {
        rc = tifx_write_tag_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_BITS_PER_SAMPLE, TIFX_TYPE_SHORT, 1UL, 8UL);
        if (rc != TIFX_OK) return rc;
    } else if (params->pixel_format == TIFX_PIXEL_RGB24) {
        rc = tifx_write_tag_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_BITS_PER_SAMPLE, TIFX_TYPE_SHORT, 3UL, bits_offset);
        if (rc != TIFX_OK) return rc;
    } else if (params->pixel_format == TIFX_PIXEL_RGBA32) {
        rc = tifx_write_tag_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_BITS_PER_SAMPLE, TIFX_TYPE_SHORT, 4UL, bits_offset);
        if (rc != TIFX_OK) return rc;
    } else {
        rc = tifx_write_tag_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_BITS_PER_SAMPLE, TIFX_TYPE_SHORT, 1UL, 1UL);
        if (rc != TIFX_OK) return rc;
    }

    rc = tifx_write_tag_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                           TIFX_TAG_COMPRESSION, TIFX_TYPE_SHORT, 1UL, (unsigned long)compression);
    if (rc != TIFX_OK) return rc;
    rc = tifx_write_tag_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                           TIFX_TAG_PHOTOMETRIC, TIFX_TYPE_SHORT, 1UL, (unsigned long)photometric);
    if (rc != TIFX_OK) return rc;

    if (params->pixel_format == TIFX_PIXEL_BILEVEL) {
        rc = tifx_write_tag_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_FILL_ORDER, TIFX_TYPE_SHORT, 1UL, (unsigned long)fill_order);
        if (rc != TIFX_OK) return rc;
    }

    if (is_tiled) {
        rc = tifx_write_tag_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_TILE_WIDTH, TIFX_TYPE_LONG, 1UL, params->tile_width);
        if (rc != TIFX_OK) return rc;
        rc = tifx_write_tag_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_TILE_LENGTH, TIFX_TYPE_LONG, 1UL, params->tile_length);
        if (rc != TIFX_OK) return rc;
        rc = tifx_write_tag_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_TILE_OFFSETS, TIFX_TYPE_LONG, segment_count,
                               (segment_count == 1UL) ? segment_offsets[0] : segment_offsets_array_offset);
        if (rc != TIFX_OK) return rc;
    } else {
        rc = tifx_write_tag_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_STRIP_OFFSETS, TIFX_TYPE_LONG, segment_count,
                               (segment_count == 1UL) ? segment_offsets[0] : segment_offsets_array_offset);
        if (rc != TIFX_OK) return rc;
    }

    rc = tifx_write_tag_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                           TIFX_TAG_ORIENTATION, TIFX_TYPE_SHORT, 1UL, 1UL);
    if (rc != TIFX_OK) return rc;

    if (params->pixel_format == TIFX_PIXEL_RGB24 || params->pixel_format == TIFX_PIXEL_RGBA32) {
        unsigned long samples_per_pixel_value;
        samples_per_pixel_value = (params->pixel_format == TIFX_PIXEL_RGBA32) ? 4UL : 3UL;
        rc = tifx_write_tag_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_SAMPLES_PER_PIXEL, TIFX_TYPE_SHORT, 1UL, samples_per_pixel_value);
        if (rc != TIFX_OK) return rc;
    }

    if (is_tiled) {
        rc = tifx_write_tag_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_TILE_BYTE_COUNTS, TIFX_TYPE_LONG, segment_count,
                               (segment_count == 1UL) ? segment_byte_counts[0] : segment_byte_counts_array_offset);
        if (rc != TIFX_OK) return rc;
    } else {
        rc = tifx_write_tag_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_ROWS_PER_STRIP, TIFX_TYPE_LONG, 1UL, segment_span);
        if (rc != TIFX_OK) return rc;
        rc = tifx_write_tag_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_STRIP_BYTE_COUNTS, TIFX_TYPE_LONG, segment_count,
                               (segment_count == 1UL) ? segment_byte_counts[0] : segment_byte_counts_array_offset);
        if (rc != TIFX_OK) return rc;
    }

    if (predictor == TIFX_PREDICTOR_HORIZONTAL) {
        rc = tifx_write_tag_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_PREDICTOR, TIFX_TYPE_SHORT, 1UL, (unsigned long)predictor);
        if (rc != TIFX_OK) return rc;
    }
    rc = tifx_write_tag_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                           TIFX_TAG_X_RESOLUTION, TIFX_TYPE_RATIONAL, 1UL, xres_offset);
    if (rc != TIFX_OK) return rc;
    rc = tifx_write_tag_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                           TIFX_TAG_Y_RESOLUTION, TIFX_TYPE_RATIONAL, 1UL, yres_offset);
    if (rc != TIFX_OK) return rc;

    if (params->pixel_format == TIFX_PIXEL_RGB24 || params->pixel_format == TIFX_PIXEL_RGBA32) {
        rc = tifx_write_tag_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_PLANAR_CONFIGURATION, TIFX_TYPE_SHORT, 1UL,
                               (unsigned long)planar_config);
        if (rc != TIFX_OK) return rc;
    }
    if (params->pixel_format == TIFX_PIXEL_RGBA32) {
        rc = tifx_write_tag_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_EXTRA_SAMPLES, TIFX_TYPE_SHORT, 1UL, params->alpha_mode);
        if (rc != TIFX_OK) return rc;
    }
    if (!is_tiled && params->pixel_format == TIFX_PIXEL_BILEVEL && compression == 3U) {
        rc = tifx_write_tag_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_T4_OPTIONS, TIFX_TYPE_LONG, 1UL, params->t4_options);
        if (rc != TIFX_OK) return rc;
    }
    if (!is_tiled && params->pixel_format == TIFX_PIXEL_BILEVEL && compression == 4U) {
        rc = tifx_write_tag_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_T6_OPTIONS, TIFX_TYPE_LONG, 1UL, params->t6_options);
        if (rc != TIFX_OK) return rc;
    }

    resolution_unit = params->resolution_unit;
    if (resolution_unit == 0U) {
        resolution_unit = 2U;
    }
    rc = tifx_write_tag_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                           TIFX_TAG_RESOLUTION_UNIT, TIFX_TYPE_SHORT, 1UL, (unsigned long)resolution_unit);
    if (rc != TIFX_OK) return rc;

    if (auto_page_bit != 0U) {
        page_number_values[0] = (unsigned short)page_index;
        page_number_values[1] = (unsigned short)page_count;
        tifx_write_tag_inline_shorts_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                                        TIFX_TAG_PAGE_NUMBER,
                                        page_number_values,
                                        2UL);
    }

    if (node->child_count > 0UL) {
        unsigned long subifd_count;
        unsigned long subifd_value;

        subifd_count = (subifd_style == TIFX_SUBIFD_STYLE_ADOBE_CHAIN) ? 1UL : node->child_count;
        if (subifd_count == 1UL) {
            subifd_value = child_ifd_offsets[0];
        } else {
            subifd_value = subifd_offsets_array_offset;
        }
        rc = tifx_write_tag_le(out + current_ifd_offset + 2UL + ((unsigned long)entry_index++ * 12UL),
                               TIFX_TAG_SUBIFDS, TIFX_TYPE_LONG, subifd_count, subifd_value);
        if (rc != TIFX_OK) return rc;
    }

    if (entry_index != tag_count) {
        return TIFX_ERR_BAD_FORMAT;
    }

    tifx_write_u32le(out + current_ifd_offset + 2UL + ((unsigned long)tag_count * 12UL), next_ifd_offset);

    if (params->pixel_format == TIFX_PIXEL_RGB24) {
        tifx_write_u16le(out + bits_offset + 0UL, 8U);
        tifx_write_u16le(out + bits_offset + 2UL, 8U);
        tifx_write_u16le(out + bits_offset + 4UL, 8U);
    } else if (params->pixel_format == TIFX_PIXEL_RGBA32) {
        tifx_write_u16le(out + bits_offset + 0UL, 8U);
        tifx_write_u16le(out + bits_offset + 2UL, 8U);
        tifx_write_u16le(out + bits_offset + 4UL, 8U);
        tifx_write_u16le(out + bits_offset + 6UL, 8U);
    }

    xres = params->x_resolution;
    yres = params->y_resolution;
    if (xres <= 0) {
        xres = TIFX_FP_FROM_INT(72L);
    }
    if (yres <= 0) {
        yres = TIFX_FP_FROM_INT(72L);
    }
    tifx_write_u32le(out + xres_offset + 0UL, tifx_fp_to_rational_numerator(xres));
    tifx_write_u32le(out + xres_offset + 4UL, tifx_fp_to_rational_denominator());
    tifx_write_u32le(out + yres_offset + 0UL, tifx_fp_to_rational_numerator(yres));
    tifx_write_u32le(out + yres_offset + 4UL, tifx_fp_to_rational_denominator());

    if (segment_count > 1UL) {
        tifx_write_u32le_array(out + segment_offsets_array_offset, segment_offsets, segment_count);
        tifx_write_u32le_array(out + segment_byte_counts_array_offset, segment_byte_counts, segment_count);
    }
    if (node->child_count > 1UL && subifd_style == TIFX_SUBIFD_STYLE_TREE) {
        tifx_write_u32le_array(out + subifd_offsets_array_offset, child_ifd_offsets, node->child_count);
    }

    if (out_node_end != 0) {
        *out_node_end = extra_cursor;
    }
    return TIFX_OK;
}

unsigned long tifx_write_classic_tree_buffer_size(const tifx_tiff_node *pages,
                                                  unsigned long page_count)
{
    unsigned long cursor;
    unsigned long page_base_offset;
    unsigned long page_ifd_offset;
    unsigned long page_end;
    unsigned long page_index;
    int rc;

    if (pages == 0 || page_count == 0UL) {
        return 0UL;
    }
    if (page_count > TIFX_MAX_PAGES || page_count > 65535UL) {
        return 0UL;
    }

    cursor = 8UL;
    for (page_index = 0UL; page_index < page_count; ++page_index) {
        page_base_offset = tifx_align2(cursor);
        rc = tifx_measure_classic_tree_node_layout(&pages[page_index],
                                                   page_base_offset,
                                                   (unsigned short)(page_count > 1UL ? 2U : 0U),
                                                   &page_ifd_offset,
                                                   &page_end,
                                                   0,
                                                   0UL);
        if (rc != TIFX_OK) {
            return 0UL;
        }
        cursor = page_end;
    }
    if (cursor > 0xFFFFFFFFUL) {
        return 0UL;
    }
    return cursor;
}

int tifx_write_classic_tree_memory(void *dst,
                                   unsigned long dst_size,
                                   const tifx_tiff_node *pages,
                                   unsigned long page_count,
                                   unsigned long *written_size)
{
    unsigned char *out;
    unsigned long total_size;
    unsigned long cursor;
    unsigned long page_base_offset;
    unsigned long page_ifd_offsets[TIFX_MAX_PAGES];
    unsigned long page_end;
    unsigned long page_index;
    int rc;

    if (dst == 0 || pages == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (page_count == 0UL) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (page_count > TIFX_MAX_PAGES || page_count > 65535UL) {
        return TIFX_ERR_UNSUPPORTED;
    }

    total_size = tifx_write_classic_tree_buffer_size(pages, page_count);
    if (total_size == 0UL) {
        return TIFX_ERR_OVERFLOW;
    }
    if (dst_size < total_size) {
        return TIFX_ERR_NO_SPACE;
    }

    cursor = 8UL;
    for (page_index = 0UL; page_index < page_count; ++page_index) {
        page_base_offset = tifx_align2(cursor);
        rc = tifx_measure_classic_tree_node_layout(&pages[page_index],
                                                   page_base_offset,
                                                   (unsigned short)(page_count > 1UL ? 2U : 0U),
                                                   &page_ifd_offsets[page_index],
                                                   &page_end,
                                                   0,
                                                   0UL);
        if (rc != TIFX_OK) {
            return rc;
        }
        cursor = page_end;
    }
    if (cursor != total_size) {
        return TIFX_ERR_BAD_FORMAT;
    }

    out = (unsigned char *)dst;
    memset(out, 0, total_size);
    out[0] = 'I';
    out[1] = 'I';
    tifx_write_u16le(out + 2, 42U);
    tifx_write_u32le(out + 4, page_ifd_offsets[0]);

    cursor = 8UL;
    for (page_index = 0UL; page_index < page_count; ++page_index) {
        unsigned long next_ifd_offset;

        page_base_offset = tifx_align2(cursor);
        next_ifd_offset = 0UL;
        if (page_index + 1UL < page_count) {
            next_ifd_offset = page_ifd_offsets[page_index + 1UL];
        }

        rc = tifx_write_classic_tree_node(out,
                                          &pages[page_index],
                                          page_base_offset,
                                          page_ifd_offsets[page_index],
                                          next_ifd_offset,
                                          (unsigned short)(page_count > 1UL ? 2U : 0U),
                                          page_index,
                                          page_count,
                                          &page_end,
                                          0UL);
        if (rc != TIFX_OK) {
            return rc;
        }
        cursor = page_end;
    }

    if (cursor != total_size) {
        return TIFX_ERR_BAD_FORMAT;
    }
    if (written_size != 0) {
        *written_size = total_size;
    }
    return TIFX_OK;
}
