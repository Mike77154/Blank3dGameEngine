#include "c89jpeg.h"

#include <string.h>

#define C89JPEG_TRUE 1
#define C89JPEG_FALSE 0

#define C89JPEG_MARKER_SOI  0xD8
#define C89JPEG_MARKER_EOI  0xD9
#define C89JPEG_MARKER_SOS  0xDA
#define C89JPEG_MARKER_DQT  0xDB
#define C89JPEG_MARKER_DHT  0xC4
#define C89JPEG_MARKER_DRI  0xDD
#define C89JPEG_MARKER_APP0 0xE0
#define C89JPEG_MARKER_COM  0xFE
#define C89JPEG_MARKER_SOF0 0xC0
#define C89JPEG_MARKER_SOF1 0xC1
#define C89JPEG_MARKER_SOF2 0xC2
#define C89JPEG_MARKER_SOF3 0xC3
#define C89JPEG_MARKER_SOF5 0xC5
#define C89JPEG_MARKER_SOF6 0xC6
#define C89JPEG_MARKER_SOF7 0xC7
#define C89JPEG_MARKER_SOF9 0xC9
#define C89JPEG_MARKER_SOF10 0xCA
#define C89JPEG_MARKER_SOF11 0xCB
#define C89JPEG_MARKER_SOF13 0xCD
#define C89JPEG_MARKER_SOF14 0xCE
#define C89JPEG_MARKER_SOF15 0xCF
#define C89JPEG_MARKER_RST0 0xD0
#define C89JPEG_MARKER_RST7 0xD7

#define C89JPEG_DCT_FWD_SHIFT 14
#define C89JPEG_DCT_IDCT_SHIFT 9
#define C89JPEG_RGB_SHIFT 16

#define C89JPEG_EMIT_CHECK(status_expr) do { c89jpeg_status c89jpeg_emit_status = (status_expr); if (c89jpeg_emit_status != C89JPEG_OK) return c89jpeg_emit_status; } while (0)

static int c89jpeg_value_category(c89jpeg_i32 v);
static c89jpeg_status c89jpeg_collect_huffman_stats(c89jpeg_encoder *enc,
                                                   const c89jpeg_encode_params *params,
                                                   int components,
                                                   c89jpeg_u8 max_h,
                                                   c89jpeg_u8 max_v);
static c89jpeg_status c89jpeg_validate_huffman_table_usage(const c89jpeg_huffman_table *tab, const c89jpeg_u32 *freq);
static c89jpeg_status c89jpeg_apply_custom_huffman_tables(c89jpeg_encoder *enc, const c89jpeg_encode_params *params, int components);
static c89jpeg_status c89jpeg_gather_huffman_stats(c89jpeg_encoder *enc,
                                                   const c89jpeg_encode_params *params,
                                                   int components,
                                                   c89jpeg_u8 max_h,
                                                   c89jpeg_u8 max_v);
static void c89jpeg_setup_encoder_components(c89jpeg_encoder *enc, const c89jpeg_encode_params *params, int *components, c89jpeg_u8 *max_h, c89jpeg_u8 *max_v);
static void c89jpeg_encres_reset_preds(c89jpeg_encoder_resume *state);

static const c89jpeg_u8 c89jpeg_zigzag[64] = {
    0, 1, 8, 16, 9, 2, 3, 10,
    17, 24, 32, 25, 18, 11, 4, 5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13, 6, 7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

static const c89jpeg_i16 c89jpeg_dct_fwd_matrix[8][8] = {
    { 5793, 5793, 5793, 5793, 5793, 5793, 5793, 5793 },
    { 8035, 6811, 4551, 1598, -1598, -4551, -6811, -8035 },
    { 7568, 3135, -3135, -7568, -7568, -3135, 3135, 7568 },
    { 6811, -1598, -8035, -4551, 4551, 8035, 1598, -6811 },
    { 5793, -5793, -5793, 5793, 5793, -5793, -5793, 5793 },
    { 4551, -8035, 1598, 6811, -6811, -1598, 8035, -4551 },
    { 3135, -7568, 7568, -3135, -3135, 7568, -7568, 3135 },
    { 1598, -4551, 6811, -8035, 8035, -6811, 4551, -1598 }
};

static const c89jpeg_i16 c89jpeg_dct_idct_matrix[8][8] = {
    { 181, 181, 181, 181, 181, 181, 181, 181 },
    { 251, 213, 142, 50, -50, -142, -213, -251 },
    { 237, 98, -98, -237, -237, -98, 98, 237 },
    { 213, -50, -251, -142, 142, 251, 50, -213 },
    { 181, -181, -181, 181, 181, -181, -181, 181 },
    { 142, -251, 50, 213, -213, -50, 251, -142 },
    { 98, -237, 237, -98, -98, 237, -237, 98 },
    { 50, -142, 213, -251, 251, -213, 142, -50 }
};

static const c89jpeg_u8 c89jpeg_default_q_luma[64] = {
    16, 11, 10, 16, 24, 40, 51, 61,
    12, 12, 14, 19, 26, 58, 60, 55,
    14, 13, 16, 24, 40, 57, 69, 56,
    14, 17, 22, 29, 51, 87, 80, 62,
    18, 22, 37, 56, 68,109,103, 77,
    24, 35, 55, 64, 81,104,113, 92,
    49, 64, 78, 87,103,121,120,101,
    72, 92, 95, 98,112,100,103, 99
};

static const c89jpeg_u8 c89jpeg_default_q_chroma[64] = {
    17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99
};

static const c89jpeg_u8 c89jpeg_std_dc_luma_bits[16] = {
    0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0
};
static const c89jpeg_u8 c89jpeg_std_dc_luma_vals[12] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
};
static const c89jpeg_u8 c89jpeg_std_dc_chroma_bits[16] = {
    0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0
};
static const c89jpeg_u8 c89jpeg_std_dc_chroma_vals[12] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
};
static const c89jpeg_u8 c89jpeg_std_ac_luma_bits[16] = {
    0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 125
};
static const c89jpeg_u8 c89jpeg_std_ac_luma_vals[162] = {
    0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,
    0x22,0x71,0x14,0x32,0x81,0x91,0xA1,0x08,0x23,0x42,0xB1,0xC1,0x15,0x52,0xD1,0xF0,
    0x24,0x33,0x62,0x72,0x82,0x09,0x0A,0x16,0x17,0x18,0x19,0x1A,0x25,0x26,0x27,0x28,
    0x29,0x2A,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,0x48,0x49,
    0x4A,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x63,0x64,0x65,0x66,0x67,0x68,0x69,
    0x6A,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
    0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,
    0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xC2,0xC3,0xC4,0xC5,
    0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xE1,0xE2,
    0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,
    0xF9,0xFA
};
static const c89jpeg_u8 c89jpeg_std_ac_chroma_bits[16] = {
    0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 119
};
static const c89jpeg_u8 c89jpeg_std_ac_chroma_vals[162] = {
    0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71,
    0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,0xA1,0xB1,0xC1,0x09,0x23,0x33,0x52,0xF0,
    0x15,0x62,0x72,0xD1,0x0A,0x16,0x24,0x34,0xE1,0x25,0xF1,0x17,0x18,0x19,0x1A,0x26,
    0x27,0x28,0x29,0x2A,0x35,0x36,0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,0x48,
    0x49,0x4A,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x63,0x64,0x65,0x66,0x67,0x68,
    0x69,0x6A,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x82,0x83,0x84,0x85,0x86,0x87,
    0x88,0x89,0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0xA2,0xA3,0xA4,0xA5,
    0xA6,0xA7,0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xC2,0xC3,
    0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,
    0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,
    0xF9,0xFA
};

typedef struct c89jpeg_bitwriter_tag {
    c89jpeg_write_fn write_fn;
    void *write_user;
    c89jpeg_u8 outbuf[256];
    c89jpeg_u32 outcount;
    c89jpeg_u32 total;
    c89jpeg_u32 acc;
    int bits;
} c89jpeg_bitwriter;

typedef struct c89jpeg_bitreader_tag {
    const c89jpeg_u8 *ptr;
    const c89jpeg_u8 *end;
    c89jpeg_u32 acc;
    int bits;
    int unread_marker;
} c89jpeg_bitreader;

typedef struct c89jpeg_parse_cursor_tag {
    const c89jpeg_u8 *ptr;
    const c89jpeg_u8 *end;
} c89jpeg_parse_cursor;

static c89jpeg_i32 c89jpeg_abs_i32(c89jpeg_i32 v)
{
    return (v < 0) ? -v : v;
}

static c89jpeg_i32 c89jpeg_round_shift(c89jpeg_i32 v, int shift)
{
    c89jpeg_i32 add;
    if (shift <= 0) return v;
    add = ((c89jpeg_i32)1) << (shift - 1);
    if (v >= 0) return (v + add) >> shift;
    return -(((-v) + add) >> shift);
}

static c89jpeg_i32 c89jpeg_div_round(c89jpeg_i32 num, c89jpeg_i32 den)
{
    if (num >= 0) return (num + (den / 2)) / den;
    return -(((-num) + (den / 2)) / den);
}

static c89jpeg_u8 c89jpeg_clamp_u8(c89jpeg_i32 v)
{
    if (v < 0) return 0;
    if (v > 255) return 255;
    return (c89jpeg_u8)v;
}

static int c89jpeg_mul_u32(c89jpeg_u32 a, c89jpeg_u32 b, c89jpeg_u32 *out)
{
    if (a != 0 && b > (0xFFFFFFFFUL / a)) return C89JPEG_FALSE;
    *out = a * b;
    return C89JPEG_TRUE;
}

static c89jpeg_u16 c89jpeg_read_be16(const c89jpeg_u8 *p)
{
    return (c89jpeg_u16)(((c89jpeg_u16)p[0] << 8) | (c89jpeg_u16)p[1]);
}

static void c89jpeg_write_be16(c89jpeg_u8 *p, c89jpeg_u16 v)
{
    p[0] = (c89jpeg_u8)((v >> 8) & 0xFF);
    p[1] = (c89jpeg_u8)(v & 0xFF);
}

const char *c89jpeg_status_string(c89jpeg_status status)
{
    switch (status) {
    case C89JPEG_SUSPENDED: return "suspended awaiting more input";
    case C89JPEG_OK: return "ok";
    case C89JPEG_ERR_BAD_ARG: return "bad argument";
    case C89JPEG_ERR_SHORT_BUFFER: return "short buffer";
    case C89JPEG_ERR_UNSUPPORTED: return "unsupported jpeg feature";
    case C89JPEG_ERR_CORRUPT: return "corrupt jpeg stream";
    case C89JPEG_ERR_LIMIT: return "implementation limit exceeded";
    case C89JPEG_ERR_IO: return "sink/source I/O error";
    default: return "unknown error";
    }
}

void c89jpeg_mem_dest_init(c89jpeg_mem_dest *dest, c89jpeg_u8 *buffer, c89jpeg_u32 capacity)
{
    if (dest == 0) return;
    dest->data = buffer;
    dest->capacity = capacity;
    dest->size = 0;
}

int c89jpeg_mem_dest_write(void *user, const c89jpeg_u8 *data, c89jpeg_u32 size)
{
    c89jpeg_mem_dest *dest;
    if (user == 0 || data == 0) return C89JPEG_FALSE;
    dest = (c89jpeg_mem_dest *)user;
    if (size > dest->capacity - dest->size) return C89JPEG_FALSE;
    if (size != 0) {
        memcpy(dest->data + dest->size, data, (size_t)size);
        dest->size += size;
    }
    return C89JPEG_TRUE;
}

c89jpeg_u32 c89jpeg_encoder_max_output_size(c89jpeg_u16 width, c89jpeg_u16 height, c89jpeg_pixel_format fmt)
{
    c89jpeg_u32 pixels;
    c89jpeg_u32 bytes_per_pixel;
    c89jpeg_u32 total;
    bytes_per_pixel = (fmt == C89JPEG_PIXFMT_GRAY8) ? 1UL : 3UL;
    if (!c89jpeg_mul_u32((c89jpeg_u32)width, (c89jpeg_u32)height, &pixels)) return 0;
    if (!c89jpeg_mul_u32(pixels, bytes_per_pixel, &pixels)) return 0;
    if (!c89jpeg_mul_u32(pixels, 4UL, &total)) return 0;
    if (total > 0xFFFFFFFFUL - 4096UL) return 0;
    return total + 4096UL;
}

c89jpeg_u32 c89jpeg_decoder_row_stride(c89jpeg_u16 width, c89jpeg_decode_format fmt, c89jpeg_u8 source_components)
{
    c89jpeg_u32 channels;
    if (fmt == C89JPEG_DECODE_NATIVE) channels = (source_components == 1) ? 1UL : 3UL;
    else channels = (c89jpeg_u32)fmt;
    return (c89jpeg_u32)width * channels;
}

c89jpeg_u32 c89jpeg_decoder_output_size(c89jpeg_u16 width, c89jpeg_u16 height, c89jpeg_decode_format fmt, c89jpeg_u8 source_components)
{
    c89jpeg_u32 stride;
    c89jpeg_u32 size;
    stride = c89jpeg_decoder_row_stride(width, fmt, source_components);
    if (!c89jpeg_mul_u32(stride, (c89jpeg_u32)height, &size)) return 0;
    return size;
}

c89jpeg_u32 c89jpeg_decoder_progressive_workspace_size(const c89jpeg_image_info *info)
{
    c89jpeg_u32 mcu_cols;
    c89jpeg_u32 mcu_rows;
    c89jpeg_u32 total_blocks;
    c89jpeg_u32 blocks;
    c89jpeg_u32 coeffs;
    c89jpeg_u32 bytes;
    int i;
    if (info == 0 || !info->progressive || info->width == 0 || info->height == 0 ||
        info->components == 0 || info->max_h_samp == 0 || info->max_v_samp == 0) return 0;
    mcu_cols = ((c89jpeg_u32)info->width + (c89jpeg_u32)info->max_h_samp * 8UL - 1UL) /
               ((c89jpeg_u32)info->max_h_samp * 8UL);
    mcu_rows = ((c89jpeg_u32)info->height + (c89jpeg_u32)info->max_v_samp * 8UL - 1UL) /
               ((c89jpeg_u32)info->max_v_samp * 8UL);
    total_blocks = 0;
    for (i = 0; i < info->components; ++i) {
        if (!c89jpeg_mul_u32(mcu_cols, mcu_rows, &blocks)) return 0;
        if (!c89jpeg_mul_u32(blocks, (c89jpeg_u32)info->component[i].h_samp, &blocks)) return 0;
        if (!c89jpeg_mul_u32(blocks, (c89jpeg_u32)info->component[i].v_samp, &blocks)) return 0;
        if (total_blocks > 0xFFFFFFFFUL - blocks) return 0;
        total_blocks += blocks;
    }
    if (!c89jpeg_mul_u32(total_blocks, 64UL, &coeffs)) return 0;
    if (!c89jpeg_mul_u32(coeffs, (c89jpeg_u32)sizeof(c89jpeg_i16), &bytes)) return 0;
    return bytes;
}

c89jpeg_u32 c89jpeg_decoder_sink_strip_size(const c89jpeg_image_info *info, c89jpeg_decode_format fmt)
{
    c89jpeg_u32 stride;
    c89jpeg_u32 size;
    if (info == 0 || info->width == 0 || info->height == 0 || info->components == 0 || info->max_v_samp == 0) return 0;
    stride = c89jpeg_decoder_row_stride(info->width, fmt, info->components);
    if (!c89jpeg_mul_u32(stride, (c89jpeg_u32)(info->max_v_samp * 8), &size)) return 0;
    return size;
}

c89jpeg_u32 c89jpeg_decoder_sink_strip_size_region(const c89jpeg_image_info *info, const c89jpeg_rect *region, c89jpeg_decode_format fmt)
{
    c89jpeg_u32 stride;
    c89jpeg_u32 size;
    c89jpeg_u16 width;
    if (info == 0 || info->width == 0 || info->height == 0 || info->components == 0 || info->max_v_samp == 0) return 0;
    width = (region != 0 && region->width != 0) ? region->width : info->width;
    stride = c89jpeg_decoder_row_stride(width, fmt, info->components);
    if (!c89jpeg_mul_u32(stride, (c89jpeg_u32)(info->max_v_samp * 8), &size)) return 0;
    return size;
}

void c89jpeg_decoder_imcu_size(const c89jpeg_image_info *info, c89jpeg_u16 *out_width, c89jpeg_u16 *out_height)
{
    c89jpeg_u16 mcu_w;
    c89jpeg_u16 mcu_h;
    if (info == 0 || info->max_h_samp == 0 || info->max_v_samp == 0) {
        if (out_width != 0) *out_width = 0;
        if (out_height != 0) *out_height = 0;
        return;
    }
    mcu_w = (c89jpeg_u16)(info->max_h_samp * 8);
    mcu_h = (c89jpeg_u16)(info->max_v_samp * 8);
    if (out_width != 0) *out_width = mcu_w;
    if (out_height != 0) *out_height = mcu_h;
}

c89jpeg_status c89jpeg_resolve_roi(const c89jpeg_image_info *info, const c89jpeg_rect *request, c89jpeg_roi_mode mode, c89jpeg_rect *resolved)
{
    c89jpeg_u16 mcu_w;
    c89jpeg_u16 mcu_h;
    c89jpeg_u32 x0;
    c89jpeg_u32 y0;
    c89jpeg_u32 x1;
    c89jpeg_u32 y1;
    c89jpeg_rect out;

    if (info == 0 || resolved == 0) return C89JPEG_ERR_BAD_ARG;
    if (info->width == 0 || info->height == 0 || info->max_h_samp == 0 || info->max_v_samp == 0) return C89JPEG_ERR_BAD_ARG;

    if (mode == C89JPEG_ROI_DISABLED || request == 0) {
        out.x = 0;
        out.y = 0;
        out.width = info->width;
        out.height = info->height;
        *resolved = out;
        return C89JPEG_OK;
    }

    if (request->width == 0 || request->height == 0) return C89JPEG_ERR_BAD_ARG;
    if (request->x >= info->width || request->y >= info->height) return C89JPEG_ERR_BAD_ARG;

    mcu_w = (c89jpeg_u16)(info->max_h_samp * 8);
    mcu_h = (c89jpeg_u16)(info->max_v_samp * 8);
    x0 = request->x;
    y0 = request->y;
    x1 = x0 + request->width;
    y1 = y0 + request->height;
    if (x1 > info->width) x1 = info->width;
    if (y1 > info->height) y1 = info->height;
    if (x1 <= x0 || y1 <= y0) return C89JPEG_ERR_BAD_ARG;

    if (mode == C89JPEG_ROI_STRICT) {
        if ((x0 % mcu_w) != 0 || (y0 % mcu_h) != 0) return C89JPEG_ERR_BAD_ARG;
    } else if (mode == C89JPEG_ROI_ALIGN_TO_IMCU) {
        x0 -= (x0 % mcu_w);
        y0 -= (y0 % mcu_h);
    } else {
        return C89JPEG_ERR_BAD_ARG;
    }

    if (x1 > info->width) x1 = info->width;
    if (y1 > info->height) y1 = info->height;
    if (x1 <= x0 || y1 <= y0) return C89JPEG_ERR_BAD_ARG;

    out.x = (c89jpeg_u16)x0;
    out.y = (c89jpeg_u16)y0;
    out.width = (c89jpeg_u16)(x1 - x0);
    out.height = (c89jpeg_u16)(y1 - y0);
    *resolved = out;
    return C89JPEG_OK;
}

static void c89jpeg_zero_huffman(c89jpeg_huffman_table *tab)
{
    if (tab == 0) return;
    memset(tab, 0, sizeof(*tab));
}

static c89jpeg_status c89jpeg_build_huffman(c89jpeg_huffman_table *tab, const c89jpeg_u8 *bits, const c89jpeg_u8 *vals, c89jpeg_u16 count)
{
    c89jpeg_u16 code;
    c89jpeg_u16 k;
    int i;
    c89jpeg_u16 expected;
    if (tab == 0 || bits == 0 || vals == 0) return C89JPEG_ERR_BAD_ARG;
    c89jpeg_zero_huffman(tab);
    memcpy(tab->bits, bits, 16);
    memcpy(tab->vals, vals, (size_t)count);
    tab->count = count;

    expected = 0;
    for (i = 0; i < 16; ++i) {
        expected = (c89jpeg_u16)(expected + (c89jpeg_u16)bits[i]);
    }
    if (expected != count) return C89JPEG_ERR_CORRUPT;

    for (i = 0; i < 17; ++i) {
        tab->mincode[i] = -1;
        tab->maxcode[i] = -1;
        tab->valptr[i] = -1;
    }
    tab->maxcode[17] = 0x7FFFFFFFL;

    code = 0;
    k = 0;
    for (i = 1; i <= 16; ++i) {
        c89jpeg_u8 j;
        if (bits[i - 1] != 0) {
            tab->valptr[i] = k;
            tab->mincode[i] = (c89jpeg_i32)code;
            for (j = 0; j < bits[i - 1]; ++j) {
                c89jpeg_u8 sym;
                sym = vals[k++];
                tab->ehufco[sym] = code;
                tab->ehufsi[sym] = (c89jpeg_u8)i;
                if (j == bits[i - 1] - 1) tab->maxcode[i] = (c89jpeg_i32)code;
                ++code;
            }
        }
        code = (c89jpeg_u16)(code << 1);
    }
    tab->present = 1;
    return C89JPEG_OK;
}

c89jpeg_status c89jpeg_huffman_table_init(c89jpeg_huffman_table *tab, const c89jpeg_u8 *bits, const c89jpeg_u8 *vals, c89jpeg_u16 count)
{
    return c89jpeg_build_huffman(tab, bits, vals, count);
}

c89jpeg_status c89jpeg_huffman_table_init_std(c89jpeg_huffman_table *tab, int table_class, int chroma)
{
    if (table_class == 0) {
        if (chroma) return c89jpeg_build_huffman(tab, c89jpeg_std_dc_chroma_bits, c89jpeg_std_dc_chroma_vals, 12);
        return c89jpeg_build_huffman(tab, c89jpeg_std_dc_luma_bits, c89jpeg_std_dc_luma_vals, 12);
    }
    if (table_class == 1) {
        if (chroma) return c89jpeg_build_huffman(tab, c89jpeg_std_ac_chroma_bits, c89jpeg_std_ac_chroma_vals, 162);
        return c89jpeg_build_huffman(tab, c89jpeg_std_ac_luma_bits, c89jpeg_std_ac_luma_vals, 162);
    }
    return C89JPEG_ERR_BAD_ARG;
}


void c89jpeg_tables_init(c89jpeg_tables *tables)
{
    if (tables == 0) return;
    memset(tables, 0, sizeof(*tables));
}

static void c89jpeg_tables_copy_from_decoder(c89jpeg_tables *tables, const c89jpeg_decoder *dec)
{
    int tq;
    int tc;
    int th;
    if (tables == 0 || dec == 0) return;
    c89jpeg_tables_init(tables);
    tables->restart_interval = dec->info.restart_interval;
    for (tq = 0; tq < C89JPEG_MAX_QUANT_TABLES; ++tq) {
        if (dec->quant_present[tq]) {
            memcpy(tables->quant[tq], dec->quant[tq], sizeof(tables->quant[tq]));
            tables->quant_present[tq] = 1;
        }
    }
    for (tc = 0; tc < C89JPEG_MAX_HUFF_CLASSES; ++tc) {
        for (th = 0; th < C89JPEG_MAX_HUFF_TABLES; ++th) {
            if (dec->huff[tc][th].present) {
                tables->huff[tc][th] = dec->huff[tc][th];
            }
        }
    }
}

static c89jpeg_status c89jpeg_build_huffman_from_freq(c89jpeg_huffman_table *tab, const c89jpeg_u32 *freq_in)
{
    c89jpeg_u32 freq[C89JPEG_HUFF_SYMBOLS];
    c89jpeg_u8 bits[33];
    int codesize[C89JPEG_HUFF_SYMBOLS];
    int others[C89JPEG_HUFF_SYMBOLS];
    c89jpeg_u8 vals[256];
    int c1;
    int c2;
    int i;
    int j;
    int p;
    c89jpeg_u32 v;
    c89jpeg_u16 count;

    if (tab == 0 || freq_in == 0) return C89JPEG_ERR_BAD_ARG;
    memset(bits, 0, sizeof(bits));
    memset(codesize, 0, sizeof(codesize));
    for (i = 0; i < C89JPEG_HUFF_SYMBOLS; ++i) {
        freq[i] = freq_in[i];
        others[i] = -1;
    }
    freq[256] = 1;

    for (;;) {
        c1 = -1;
        v = 0xFFFFFFFFUL;
        for (i = 0; i < C89JPEG_HUFF_SYMBOLS; ++i) {
            if (freq[i] != 0 && freq[i] <= v) {
                v = freq[i];
                c1 = i;
            }
        }

        c2 = -1;
        v = 0xFFFFFFFFUL;
        for (i = 0; i < C89JPEG_HUFF_SYMBOLS; ++i) {
            if (i != c1 && freq[i] != 0 && freq[i] <= v) {
                v = freq[i];
                c2 = i;
            }
        }

        if (c1 < 0 || c2 < 0) break;

        freq[c1] += freq[c2];
        freq[c2] = 0;

        codesize[c1]++;
        while (others[c1] >= 0) {
            c1 = others[c1];
            codesize[c1]++;
        }
        others[c1] = c2;

        codesize[c2]++;
        while (others[c2] >= 0) {
            c2 = others[c2];
            codesize[c2]++;
        }
    }

    for (i = 0; i < C89JPEG_HUFF_SYMBOLS; ++i) {
        if (codesize[i] != 0) {
            if (codesize[i] > 32) return C89JPEG_ERR_LIMIT;
            ++bits[codesize[i]];
        }
    }

    for (i = 32; i > 16; --i) {
        while (bits[i] > 0) {
            j = i - 2;
            while (j > 0 && bits[j] == 0) --j;
            if (j <= 0) return C89JPEG_ERR_LIMIT;
            bits[i] = (c89jpeg_u8)(bits[i] - 2);
            ++bits[i - 1];
            bits[j + 1] = (c89jpeg_u8)(bits[j + 1] + 2);
            --bits[j];
        }
    }

    i = 32;
    while (i > 0 && bits[i] == 0) --i;
    if (i <= 0) return C89JPEG_ERR_LIMIT;
    --bits[i];

    p = 0;
    for (i = 1; i <= 32; ++i) {
        for (j = 0; j <= 255; ++j) {
            if (codesize[j] == i) {
                if (p < 256) vals[p] = (c89jpeg_u8)j;
                ++p;
            }
        }
    }

    count = 0;
    for (i = 1; i <= 16; ++i) count = (c89jpeg_u16)(count + bits[i]);
    if (count == 0 || count > 256) return C89JPEG_ERR_LIMIT;
    return c89jpeg_build_huffman(tab, &bits[1], vals, count);
}

static void c89jpeg_gather_block_huffman_stats(c89jpeg_encoder *enc, const c89jpeg_component *comp)
{
    c89jpeg_i32 dc;
    c89jpeg_i32 diff;
    int cat;
    int k;
    int run;
    const c89jpeg_u16 *qt;
    c89jpeg_u32 *dc_freq;
    c89jpeg_u32 *ac_freq;

    qt = enc->qtable[comp->tq];
    dc_freq = enc->huff_freq_dc[comp->dc_table];
    ac_freq = enc->huff_freq_ac[comp->ac_table];

    for (k = 0; k < 64; ++k) {
        enc->qcoeff[k] = (c89jpeg_i16)c89jpeg_div_round(enc->coeff[k], (c89jpeg_i32)qt[k]);
    }

    dc = enc->qcoeff[0];
    diff = dc - comp->pred;
    ((c89jpeg_component *)comp)->pred = dc;
    cat = c89jpeg_value_category(diff);
    if (cat < 0) cat = 0;
    if (cat > 255) cat = 255;
    ++dc_freq[cat];

    run = 0;
    for (k = 1; k < 64; ++k) {
        c89jpeg_i32 vcoef;
        vcoef = enc->qcoeff[c89jpeg_zigzag[k]];
        if (vcoef == 0) {
            ++run;
        } else {
            while (run >= 16) {
                ++ac_freq[0xF0];
                run -= 16;
            }
            cat = c89jpeg_value_category(vcoef);
            if (cat < 1) cat = 1;
            if (cat > 15) cat = 15;
            ++ac_freq[(run << 4) | cat];
            run = 0;
        }
    }
    if (run != 0) {
        ++ac_freq[0x00];
    }
}

static c89jpeg_status c89jpeg_prepare_huffman_tables(c89jpeg_encoder *enc, int components)
{
    c89jpeg_status st;
    st = c89jpeg_build_huffman_from_freq(&enc->huff_dc[0], enc->huff_freq_dc[0]);
    if (st != C89JPEG_OK) return st;
    st = c89jpeg_build_huffman_from_freq(&enc->huff_ac[0], enc->huff_freq_ac[0]);
    if (st != C89JPEG_OK) return st;
    if (components > 1) {
        st = c89jpeg_build_huffman_from_freq(&enc->huff_dc[1], enc->huff_freq_dc[1]);
        if (st != C89JPEG_OK) return st;
        st = c89jpeg_build_huffman_from_freq(&enc->huff_ac[1], enc->huff_freq_ac[1]);
        if (st != C89JPEG_OK) return st;
    }
    return C89JPEG_OK;
}

void c89jpeg_encoder_init(c89jpeg_encoder *enc)
{
    if (enc == 0) return;
    memset(enc, 0, sizeof(*enc));
    (void)c89jpeg_build_huffman(&enc->huff_dc[0], c89jpeg_std_dc_luma_bits, c89jpeg_std_dc_luma_vals, 12);
    (void)c89jpeg_build_huffman(&enc->huff_dc[1], c89jpeg_std_dc_chroma_bits, c89jpeg_std_dc_chroma_vals, 12);
    (void)c89jpeg_build_huffman(&enc->huff_ac[0], c89jpeg_std_ac_luma_bits, c89jpeg_std_ac_luma_vals, 162);
    (void)c89jpeg_build_huffman(&enc->huff_ac[1], c89jpeg_std_ac_chroma_bits, c89jpeg_std_ac_chroma_vals, 162);
    enc->last_error = C89JPEG_OK;
}

void c89jpeg_decoder_init(c89jpeg_decoder *dec)
{
    if (dec == 0) return;
    memset(dec, 0, sizeof(*dec));
    dec->last_error = C89JPEG_OK;
}


static void c89jpeg_decoder_clear_frame_state(c89jpeg_decoder *dec)
{
    if (dec == 0) return;
    memset(&dec->info, 0, sizeof(dec->info));
    memset(dec->comp, 0, sizeof(dec->comp));
    memset(dec->scan_comp, 0, sizeof(dec->scan_comp));
    dec->scan_count = 0;
    dec->scan_started = 0;
    dec->scan_ss = 0;
    dec->scan_se = 0;
    dec->scan_ah = 0;
    dec->scan_al = 0;
    dec->entropy_offset = 0;
    dec->last_error = C89JPEG_OK;
}

static void c89jpeg_decoder_install_tables(c89jpeg_decoder *dec, const c89jpeg_tables *tables)
{
    int tq;
    int tc;
    int th;
    if (dec == 0 || tables == 0) return;
    for (tq = 0; tq < C89JPEG_MAX_QUANT_TABLES; ++tq) {
        dec->quant_present[tq] = tables->quant_present[tq];
        if (tables->quant_present[tq]) {
            memcpy(dec->quant[tq], tables->quant[tq], sizeof(dec->quant[tq]));
        }
    }
    for (tc = 0; tc < C89JPEG_MAX_HUFF_CLASSES; ++tc) {
        for (th = 0; th < C89JPEG_MAX_HUFF_TABLES; ++th) {
            if (tables->huff[tc][th].present) dec->huff[tc][th] = tables->huff[tc][th];
        }
    }
    dec->info.restart_interval = tables->restart_interval;
}

static c89jpeg_status c89jpeg_bw_flush_bytes(c89jpeg_bitwriter *bw)
{
    if (bw->outcount == 0) return C89JPEG_OK;
    if (!bw->write_fn(bw->write_user, bw->outbuf, bw->outcount)) return C89JPEG_ERR_IO;
    bw->total += bw->outcount;
    bw->outcount = 0;
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_bw_put_raw_byte(c89jpeg_bitwriter *bw, c89jpeg_u8 b)
{
    bw->outbuf[bw->outcount++] = b;
    if (bw->outcount == (c89jpeg_u32)sizeof(bw->outbuf)) {
        return c89jpeg_bw_flush_bytes(bw);
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_bw_put_byte(c89jpeg_bitwriter *bw, c89jpeg_u8 b)
{
    C89JPEG_EMIT_CHECK(c89jpeg_bw_put_raw_byte(bw, b));
    if (b == 0xFF) {
        C89JPEG_EMIT_CHECK(c89jpeg_bw_put_raw_byte(bw, 0x00));
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_bw_put_marker(c89jpeg_bitwriter *bw, c89jpeg_u8 marker)
{
    C89JPEG_EMIT_CHECK(c89jpeg_bw_put_raw_byte(bw, 0xFF));
    C89JPEG_EMIT_CHECK(c89jpeg_bw_put_raw_byte(bw, marker));
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_bw_put_segment(c89jpeg_bitwriter *bw, c89jpeg_u8 marker, const c89jpeg_u8 *payload, c89jpeg_u16 payload_len)
{
    c89jpeg_u8 lenbuf[2];
    C89JPEG_EMIT_CHECK(c89jpeg_bw_put_marker(bw, marker));
    c89jpeg_write_be16(lenbuf, (c89jpeg_u16)(payload_len + 2));
    C89JPEG_EMIT_CHECK(c89jpeg_bw_put_raw_byte(bw, lenbuf[0]));
    C89JPEG_EMIT_CHECK(c89jpeg_bw_put_raw_byte(bw, lenbuf[1]));
    if (payload_len != 0) {
        c89jpeg_u16 i;
        for (i = 0; i < payload_len; ++i) {
            C89JPEG_EMIT_CHECK(c89jpeg_bw_put_raw_byte(bw, payload[i]));
        }
    }
    return C89JPEG_OK;
}

static void c89jpeg_bw_init(c89jpeg_bitwriter *bw, c89jpeg_write_fn write_fn, void *write_user)
{
    memset(bw, 0, sizeof(*bw));
    bw->write_fn = write_fn;
    bw->write_user = write_user;
}

static c89jpeg_status c89jpeg_bw_put_bits(c89jpeg_bitwriter *bw, c89jpeg_u32 code, int size)
{
    while (size > 0) {
        --size;
        bw->acc = (bw->acc << 1) | ((code >> size) & 1UL);
        ++bw->bits;
        if (bw->bits == 8) {
            C89JPEG_EMIT_CHECK(c89jpeg_bw_put_byte(bw, (c89jpeg_u8)(bw->acc & 0xFF)));
            bw->bits = 0;
            bw->acc = 0;
        }
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_bw_flush_pad1(c89jpeg_bitwriter *bw)
{
    if (bw->bits != 0) {
        c89jpeg_u32 code;
        int pad;
        pad = 8 - bw->bits;
        code = ((c89jpeg_u32)1 << pad) - 1UL;
        C89JPEG_EMIT_CHECK(c89jpeg_bw_put_bits(bw, code, pad));
    }
    return c89jpeg_bw_flush_bytes(bw);
}

static void c89jpeg_br_init(c89jpeg_bitreader *br, const c89jpeg_u8 *data, c89jpeg_u32 size)
{
    br->ptr = data;
    br->end = data + size;
    br->acc = 0;
    br->bits = 0;
    br->unread_marker = 0;
}

static c89jpeg_status c89jpeg_br_fill(c89jpeg_bitreader *br)
{
    while (br->bits <= 24 && br->unread_marker == 0) {
        c89jpeg_u8 c;
        if (br->ptr >= br->end) return C89JPEG_ERR_CORRUPT;
        c = *br->ptr++;
        if (c == 0xFF) {
            c89jpeg_u8 next;
            do {
                if (br->ptr >= br->end) return C89JPEG_ERR_CORRUPT;
                next = *br->ptr++;
            } while (next == 0xFF);
            if (next == 0x00) {
                c = 0xFF;
            } else {
                br->unread_marker = (int)next;
                return C89JPEG_OK;
            }
        }
        br->acc = (br->acc << 8) | (c89jpeg_u32)c;
        br->bits += 8;
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_br_get_bit(c89jpeg_bitreader *br, int *bit)
{
    c89jpeg_status st;
    if (br->bits == 0) {
        st = c89jpeg_br_fill(br);
        if (st != C89JPEG_OK) return st;
        if (br->bits == 0) return C89JPEG_ERR_CORRUPT;
    }
    *bit = (int)((br->acc >> (br->bits - 1)) & 1UL);
    --br->bits;
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_br_get_bits(c89jpeg_bitreader *br, int count, c89jpeg_u32 *value)
{
    c89jpeg_status st;
    c89jpeg_u32 v;
    int bit;
    v = 0;
    while (count > 0) {
        st = c89jpeg_br_get_bit(br, &bit);
        if (st != C89JPEG_OK) return st;
        v = (v << 1) | (c89jpeg_u32)bit;
        --count;
    }
    *value = v;
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_huff_decode_symbol(c89jpeg_bitreader *br, const c89jpeg_huffman_table *tab, c89jpeg_u8 *symbol)
{
    c89jpeg_i32 code;
    int bit;
    int i;
    c89jpeg_status st;
    code = 0;
    for (i = 1; i <= 16; ++i) {
        st = c89jpeg_br_get_bit(br, &bit);
        if (st != C89JPEG_OK) return st;
        code = (code << 1) | (c89jpeg_i32)bit;
        if (tab->mincode[i] >= 0 && code >= tab->mincode[i] && code <= tab->maxcode[i]) {
            c89jpeg_i32 idx;
            idx = tab->valptr[i] + code - tab->mincode[i];
            if (idx < 0 || idx >= (c89jpeg_i32)tab->count) return C89JPEG_ERR_CORRUPT;
            *symbol = tab->vals[idx];
            return C89JPEG_OK;
        }
    }
    return C89JPEG_ERR_CORRUPT;
}

static c89jpeg_i32 c89jpeg_receive_extend(c89jpeg_bitreader *br, int size, c89jpeg_status *st)
{
    c89jpeg_u32 v;
    if (size == 0) {
        *st = C89JPEG_OK;
        return 0;
    }
    *st = c89jpeg_br_get_bits(br, size, &v);
    if (*st != C89JPEG_OK) return 0;
    if (v < ((c89jpeg_u32)1 << (size - 1))) {
        return (c89jpeg_i32)v + 1 - ((c89jpeg_i32)1 << size);
    }
    return (c89jpeg_i32)v;
}

static int c89jpeg_value_category(c89jpeg_i32 v)
{
    c89jpeg_i32 a;
    int n;
    if (v == 0) return 0;
    a = c89jpeg_abs_i32(v);
    n = 0;
    while (a != 0) {
        ++n;
        a >>= 1;
    }
    return n;
}

static c89jpeg_u32 c89jpeg_value_bits(c89jpeg_i32 v, int nbits)
{
    if (nbits == 0) return 0;
    if (v >= 0) return (c89jpeg_u32)v;
    return (c89jpeg_u32)(v - 1 + (((c89jpeg_i32)1) << nbits));
}

static void c89jpeg_make_scaled_qtable(c89jpeg_u16 *dst, const c89jpeg_u8 *src, int quality)
{
    int scale;
    int i;
    if (quality < 1) quality = 1;
    if (quality > 100) quality = 100;
    if (quality < 50) scale = 5000 / quality;
    else scale = 200 - quality * 2;
    for (i = 0; i < 64; ++i) {
        int v;
        v = (src[i] * scale + 50) / 100;
        if (v < 1) v = 1;
        if (v > 255) v = 255;
        dst[i] = (c89jpeg_u16)v;
    }
}

static void c89jpeg_init_quant_tables(c89jpeg_encoder *enc, const c89jpeg_encode_params *params)
{
    int i;
    if (params->quant_luma != 0) {
        for (i = 0; i < 64; ++i) enc->qtable[0][i] = params->quant_luma[i];
    } else {
        c89jpeg_make_scaled_qtable(enc->qtable[0], c89jpeg_default_q_luma, params->quality);
    }
    if (params->quant_chroma != 0) {
        for (i = 0; i < 64; ++i) enc->qtable[1][i] = params->quant_chroma[i];
    } else {
        c89jpeg_make_scaled_qtable(enc->qtable[1], c89jpeg_default_q_chroma, params->quality);
    }
}


static c89jpeg_status c89jpeg_validate_encode_params(const c89jpeg_encode_params *params)
{
    if (params == 0 || params->pixels == 0) return C89JPEG_ERR_BAD_ARG;
    if (params->width == 0 || params->height == 0) return C89JPEG_ERR_BAD_ARG;
    if (params->pixel_format != C89JPEG_PIXFMT_GRAY8 && params->pixel_format != C89JPEG_PIXFMT_RGB24) return C89JPEG_ERR_BAD_ARG;
    if (params->stride_bytes == 0) return C89JPEG_ERR_BAD_ARG;
    if (params->huffman_mode != C89JPEG_HUFFMAN_DEFAULT &&
        params->huffman_mode != C89JPEG_HUFFMAN_OPTIMAL &&
        params->huffman_mode != C89JPEG_HUFFMAN_CUSTOM) return C89JPEG_ERR_BAD_ARG;
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_encoder_prepare_from_params(c89jpeg_encoder *enc,
                                                          const c89jpeg_encode_params *params,
                                                          int *components,
                                                          c89jpeg_u8 *max_h,
                                                          c89jpeg_u8 *max_v)
{
    c89jpeg_status status;
    int ci;
    if (enc == 0 || params == 0 || components == 0 || max_h == 0 || max_v == 0) return C89JPEG_ERR_BAD_ARG;
    status = c89jpeg_validate_encode_params(params);
    if (status != C89JPEG_OK) return status;
    c89jpeg_encoder_init(enc);
    c89jpeg_init_quant_tables(enc, params);
    c89jpeg_setup_encoder_components(enc, params, components, max_h, max_v);
    if (params->huffman_mode == C89JPEG_HUFFMAN_OPTIMAL) {
        status = c89jpeg_gather_huffman_stats(enc, params, *components, *max_h, *max_v);
        if (status != C89JPEG_OK) return status;
    } else if (params->huffman_mode == C89JPEG_HUFFMAN_CUSTOM) {
        status = c89jpeg_collect_huffman_stats(enc, params, *components, *max_h, *max_v);
        if (status != C89JPEG_OK) return status;
        status = c89jpeg_apply_custom_huffman_tables(enc, params, *components);
        if (status != C89JPEG_OK) return status;
    }
    for (ci = 0; ci < *components; ++ci) enc->comp[ci].pred = 0;
    return C89JPEG_OK;
}

static void c89jpeg_tables_copy_from_encoder(c89jpeg_tables *tables, const c89jpeg_encoder *enc, int components, c89jpeg_u16 restart_interval)
{
    int tq;
    if (tables == 0 || enc == 0) return;
    c89jpeg_tables_init(tables);
    tables->restart_interval = restart_interval;
    for (tq = 0; tq < 2; ++tq) {
        memcpy(tables->quant[tq], enc->qtable[tq], sizeof(enc->qtable[tq]));
        tables->quant_present[tq] = 1;
    }
    tables->huff[0][0] = enc->huff_dc[0];
    tables->huff[1][0] = enc->huff_ac[0];
    if (components > 1) {
        tables->huff[0][1] = enc->huff_dc[1];
        tables->huff[1][1] = enc->huff_ac[1];
    }
}

static c89jpeg_status c89jpeg_encoder_apply_tables(c89jpeg_encoder *enc,
                                                   const c89jpeg_tables *tables,
                                                   const c89jpeg_encode_params *params,
                                                   int *components,
                                                   c89jpeg_u8 *max_h,
                                                   c89jpeg_u8 *max_v,
                                                   c89jpeg_u16 *restart_interval)
{
    c89jpeg_status status;
    int ci;
    if (enc == 0 || tables == 0 || params == 0 || components == 0 || max_h == 0 || max_v == 0 || restart_interval == 0) return C89JPEG_ERR_BAD_ARG;
    status = c89jpeg_validate_encode_params(params);
    if (status != C89JPEG_OK) return status;
    c89jpeg_encoder_init(enc);
    c89jpeg_setup_encoder_components(enc, params, components, max_h, max_v);
    if (!tables->quant_present[0]) return C89JPEG_ERR_BAD_ARG;
    memcpy(enc->qtable[0], tables->quant[0], sizeof(enc->qtable[0]));
    if (*components > 1) {
        if (!tables->quant_present[1]) return C89JPEG_ERR_BAD_ARG;
        memcpy(enc->qtable[1], tables->quant[1], sizeof(enc->qtable[1]));
    }
    if (!tables->huff[0][0].present || !tables->huff[1][0].present) return C89JPEG_ERR_BAD_ARG;
    enc->huff_dc[0] = tables->huff[0][0];
    enc->huff_ac[0] = tables->huff[1][0];
    if (*components > 1) {
        if (!tables->huff[0][1].present || !tables->huff[1][1].present) return C89JPEG_ERR_BAD_ARG;
        enc->huff_dc[1] = tables->huff[0][1];
        enc->huff_ac[1] = tables->huff[1][1];
    }
    status = c89jpeg_collect_huffman_stats(enc, params, *components, *max_h, *max_v);
    if (status != C89JPEG_OK) return status;
    status = c89jpeg_validate_huffman_table_usage(&enc->huff_dc[0], enc->huff_freq_dc[0]);
    if (status != C89JPEG_OK) return status;
    status = c89jpeg_validate_huffman_table_usage(&enc->huff_ac[0], enc->huff_freq_ac[0]);
    if (status != C89JPEG_OK) return status;
    if (*components > 1) {
        status = c89jpeg_validate_huffman_table_usage(&enc->huff_dc[1], enc->huff_freq_dc[1]);
        if (status != C89JPEG_OK) return status;
        status = c89jpeg_validate_huffman_table_usage(&enc->huff_ac[1], enc->huff_freq_ac[1]);
        if (status != C89JPEG_OK) return status;
    }
    for (ci = 0; ci < *components; ++ci) enc->comp[ci].pred = 0;
    *restart_interval = tables->restart_interval;
    return C89JPEG_OK;
}

static void c89jpeg_fdct8x8(const c89jpeg_i32 *input, c89jpeg_i32 *tmp, c89jpeg_i32 *output)
{
    int y;
    int x;
    int u;
    c89jpeg_i32 sum;
    for (y = 0; y < 8; ++y) {
        for (u = 0; u < 8; ++u) {
            sum = 0;
            for (x = 0; x < 8; ++x) {
                sum += input[y * 8 + x] * (c89jpeg_i32)c89jpeg_dct_fwd_matrix[u][x];
            }
            tmp[y * 8 + u] = c89jpeg_round_shift(sum, C89JPEG_DCT_FWD_SHIFT);
        }
    }
    for (u = 0; u < 8; ++u) {
        for (x = 0; x < 8; ++x) {
            sum = 0;
            for (y = 0; y < 8; ++y) {
                sum += tmp[y * 8 + u] * (c89jpeg_i32)c89jpeg_dct_fwd_matrix[x][y];
            }
            output[x * 8 + u] = c89jpeg_round_shift(sum, C89JPEG_DCT_FWD_SHIFT);
        }
    }
}

static void c89jpeg_idct8x8(const c89jpeg_i32 *coeff, c89jpeg_i32 *tmp, c89jpeg_u8 *out)
{
    int y;
    int x;
    int u;
    c89jpeg_i32 sum;
    for (y = 0; y < 8; ++y) {
        for (x = 0; x < 8; ++x) {
            sum = 0;
            for (u = 0; u < 8; ++u) {
                sum += coeff[y * 8 + u] * (c89jpeg_i32)c89jpeg_dct_idct_matrix[u][x];
            }
            tmp[y * 8 + x] = c89jpeg_round_shift(sum, C89JPEG_DCT_IDCT_SHIFT);
        }
    }
    for (x = 0; x < 8; ++x) {
        for (y = 0; y < 8; ++y) {
            sum = 0;
            for (u = 0; u < 8; ++u) {
                sum += tmp[u * 8 + x] * (c89jpeg_i32)c89jpeg_dct_idct_matrix[u][y];
            }
            out[y * 8 + x] = c89jpeg_clamp_u8(c89jpeg_round_shift(sum, C89JPEG_DCT_IDCT_SHIFT) + 128);
        }
    }
}

static c89jpeg_u8 c89jpeg_sample_gray(const c89jpeg_u8 *pixels, c89jpeg_u16 width, c89jpeg_u16 height, c89jpeg_u32 stride, c89jpeg_i32 x, c89jpeg_i32 y)
{
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= (c89jpeg_i32)width) x = width - 1;
    if (y >= (c89jpeg_i32)height) y = height - 1;
    return pixels[y * (c89jpeg_i32)stride + x];
}

static void c89jpeg_sample_rgb(const c89jpeg_u8 *pixels, c89jpeg_u16 width, c89jpeg_u16 height, c89jpeg_u32 stride, c89jpeg_i32 x, c89jpeg_i32 y, c89jpeg_u8 *r, c89jpeg_u8 *g, c89jpeg_u8 *b)
{
    const c89jpeg_u8 *p;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= (c89jpeg_i32)width) x = width - 1;
    if (y >= (c89jpeg_i32)height) y = height - 1;
    p = pixels + y * (c89jpeg_i32)stride + x * 3;
    *r = p[0];
    *g = p[1];
    *b = p[2];
}

static c89jpeg_u8 c89jpeg_rgb_to_y(c89jpeg_u8 r, c89jpeg_u8 g, c89jpeg_u8 b)
{
    return c89jpeg_clamp_u8((19595L * r + 38470L * g + 7471L * b + 32768L) >> C89JPEG_RGB_SHIFT);
}

static c89jpeg_u8 c89jpeg_rgb_to_cb(c89jpeg_u8 r, c89jpeg_u8 g, c89jpeg_u8 b)
{
    return c89jpeg_clamp_u8(((-11059L * r - 21709L * g + 32768L * b + 32768L) >> C89JPEG_RGB_SHIFT) + 128L);
}

static c89jpeg_u8 c89jpeg_rgb_to_cr(c89jpeg_u8 r, c89jpeg_u8 g, c89jpeg_u8 b)
{
    return c89jpeg_clamp_u8(((32768L * r - 27439L * g - 5329L * b + 32768L) >> C89JPEG_RGB_SHIFT) + 128L);
}

static void c89jpeg_ycbcr_to_rgb(c89jpeg_u8 y, c89jpeg_u8 cb, c89jpeg_u8 cr, c89jpeg_u8 *r, c89jpeg_u8 *g, c89jpeg_u8 *b)
{
    c89jpeg_i32 cbv;
    c89jpeg_i32 crv;
    cbv = (c89jpeg_i32)cb - 128;
    crv = (c89jpeg_i32)cr - 128;
    *r = c89jpeg_clamp_u8((c89jpeg_i32)y + c89jpeg_round_shift(91881L * crv, C89JPEG_RGB_SHIFT));
    *g = c89jpeg_clamp_u8((c89jpeg_i32)y - c89jpeg_round_shift(22554L * cbv + 46802L * crv, C89JPEG_RGB_SHIFT));
    *b = c89jpeg_clamp_u8((c89jpeg_i32)y + c89jpeg_round_shift(116130L * cbv, C89JPEG_RGB_SHIFT));
}

static c89jpeg_status c89jpeg_write_app0(c89jpeg_bitwriter *bw, const c89jpeg_encode_params *params)
{
    c89jpeg_u8 payload[14];
    memset(payload, 0, sizeof(payload));
    payload[0] = 'J';
    payload[1] = 'F';
    payload[2] = 'I';
    payload[3] = 'F';
    payload[4] = 0;
    payload[5] = 1;
    payload[6] = 2;
    payload[7] = params->density_units;
    c89jpeg_write_be16(payload + 8, params->density_x ? params->density_x : 1);
    c89jpeg_write_be16(payload + 10, params->density_y ? params->density_y : 1);
    payload[12] = 0;
    payload[13] = 0;
    return c89jpeg_bw_put_segment(bw, C89JPEG_MARKER_APP0, payload, (c89jpeg_u16)sizeof(payload));
}

static c89jpeg_status c89jpeg_write_dqt(c89jpeg_bitwriter *bw, c89jpeg_u8 table_id, const c89jpeg_u16 *table)
{
    c89jpeg_u8 payload[65];
    int i;
    payload[0] = table_id;
    for (i = 0; i < 64; ++i) {
        payload[1 + i] = (c89jpeg_u8)table[c89jpeg_zigzag[i]];
    }
    return c89jpeg_bw_put_segment(bw, C89JPEG_MARKER_DQT, payload, (c89jpeg_u16)sizeof(payload));
}

static c89jpeg_status c89jpeg_write_dht(c89jpeg_bitwriter *bw, c89jpeg_u8 tc, c89jpeg_u8 th, const c89jpeg_huffman_table *tab)
{
    c89jpeg_u8 payload[1 + 16 + 256];
    c89jpeg_u16 len;
    payload[0] = (c89jpeg_u8)((tc << 4) | th);
    memcpy(payload + 1, tab->bits, 16);
    memcpy(payload + 17, tab->vals, tab->count);
    len = (c89jpeg_u16)(17 + tab->count);
    return c89jpeg_bw_put_segment(bw, C89JPEG_MARKER_DHT, payload, len);
}

static c89jpeg_status c89jpeg_write_dri(c89jpeg_bitwriter *bw, c89jpeg_u16 restart_interval)
{
    c89jpeg_u8 payload[2];
    c89jpeg_write_be16(payload, restart_interval);
    return c89jpeg_bw_put_segment(bw, C89JPEG_MARKER_DRI, payload, 2);
}

static c89jpeg_status c89jpeg_write_sof0(c89jpeg_bitwriter *bw, const c89jpeg_encode_params *params, const c89jpeg_component *comp, int components)
{
    c89jpeg_u8 payload[32];
    c89jpeg_u16 len;
    int i;
    payload[0] = 8;
    c89jpeg_write_be16(payload + 1, params->height);
    c89jpeg_write_be16(payload + 3, params->width);
    payload[5] = (c89jpeg_u8)components;
    len = 6;
    for (i = 0; i < components; ++i) {
        payload[len++] = comp[i].id;
        payload[len++] = (c89jpeg_u8)((comp[i].h_samp << 4) | comp[i].v_samp);
        payload[len++] = comp[i].tq;
    }
    return c89jpeg_bw_put_segment(bw, C89JPEG_MARKER_SOF0, payload, len);
}

static c89jpeg_status c89jpeg_write_sos(c89jpeg_bitwriter *bw, const c89jpeg_component *comp, int components)
{
    c89jpeg_u8 payload[16];
    c89jpeg_u16 len;
    int i;
    payload[0] = (c89jpeg_u8)components;
    len = 1;
    for (i = 0; i < components; ++i) {
        payload[len++] = comp[i].id;
        payload[len++] = (c89jpeg_u8)((comp[i].dc_table << 4) | comp[i].ac_table);
    }
    payload[len++] = 0;
    payload[len++] = 63;
    payload[len++] = 0;
    return c89jpeg_bw_put_segment(bw, C89JPEG_MARKER_SOS, payload, len);
}

static void c89jpeg_setup_encoder_components(c89jpeg_encoder *enc, const c89jpeg_encode_params *params, int *components, c89jpeg_u8 *max_h, c89jpeg_u8 *max_v)
{
    if (params->pixel_format == C89JPEG_PIXFMT_GRAY8) {
        *components = 1;
        *max_h = 1;
        *max_v = 1;
        enc->comp[0].id = 1;
        enc->comp[0].h_samp = 1;
        enc->comp[0].v_samp = 1;
        enc->comp[0].tq = 0;
        enc->comp[0].dc_table = 0;
        enc->comp[0].ac_table = 0;
        enc->comp[0].pred = 0;
    } else {
        *components = 3;
        if (params->subsampling == C89JPEG_SUBSAMP_420) {
            *max_h = 2;
            *max_v = 2;
        } else if (params->subsampling == C89JPEG_SUBSAMP_422) {
            *max_h = 2;
            *max_v = 1;
        } else {
            *max_h = 1;
            *max_v = 1;
        }
        enc->comp[0].id = 1;
        enc->comp[0].h_samp = *max_h;
        enc->comp[0].v_samp = *max_v;
        enc->comp[0].tq = 0;
        enc->comp[0].dc_table = 0;
        enc->comp[0].ac_table = 0;
        enc->comp[0].pred = 0;

        enc->comp[1].id = 2;
        enc->comp[1].h_samp = 1;
        enc->comp[1].v_samp = 1;
        enc->comp[1].tq = 1;
        enc->comp[1].dc_table = 1;
        enc->comp[1].ac_table = 1;
        enc->comp[1].pred = 0;

        enc->comp[2].id = 3;
        enc->comp[2].h_samp = 1;
        enc->comp[2].v_samp = 1;
        enc->comp[2].tq = 1;
        enc->comp[2].dc_table = 1;
        enc->comp[2].ac_table = 1;
        enc->comp[2].pred = 0;
    }
}

static c89jpeg_u8 c89jpeg_component_value(const c89jpeg_encode_params *params, int component, c89jpeg_i32 x0, c89jpeg_i32 y0, c89jpeg_i32 x1, c89jpeg_i32 y1)
{
    c89jpeg_i32 x;
    c89jpeg_i32 y;
    c89jpeg_i32 sum;
    c89jpeg_i32 count;
    sum = 0;
    count = 0;
    for (y = y0; y < y1; ++y) {
        for (x = x0; x < x1; ++x) {
            if (params->pixel_format == C89JPEG_PIXFMT_GRAY8) {
                sum += c89jpeg_sample_gray(params->pixels, params->width, params->height, params->stride_bytes, x, y);
            } else {
                c89jpeg_u8 r, g, b;
                c89jpeg_sample_rgb(params->pixels, params->width, params->height, params->stride_bytes, x, y, &r, &g, &b);
                if (component == 0) sum += c89jpeg_rgb_to_y(r, g, b);
                else if (component == 1) sum += c89jpeg_rgb_to_cb(r, g, b);
                else sum += c89jpeg_rgb_to_cr(r, g, b);
            }
            ++count;
        }
    }
    if (count == 0) return 0;
    return c89jpeg_clamp_u8(c89jpeg_div_round(sum, count));
}

static void c89jpeg_load_block(const c89jpeg_encode_params *params, const c89jpeg_component *comp, c89jpeg_u8 max_h, c89jpeg_u8 max_v, c89jpeg_u16 mcu_x, c89jpeg_u16 mcu_y, c89jpeg_u8 block_x, c89jpeg_u8 block_y, c89jpeg_i32 *out)
{
    c89jpeg_i32 base_x;
    c89jpeg_i32 base_y;
    c89jpeg_i32 sx;
    c89jpeg_i32 sy;
    c89jpeg_i32 scale_x;
    c89jpeg_i32 scale_y;
    base_x = (c89jpeg_i32)mcu_x * (c89jpeg_i32)max_h * 8;
    base_y = (c89jpeg_i32)mcu_y * (c89jpeg_i32)max_v * 8;
    scale_x = (c89jpeg_i32)max_h / (c89jpeg_i32)comp->h_samp;
    scale_y = (c89jpeg_i32)max_v / (c89jpeg_i32)comp->v_samp;
    if (scale_x <= 0) scale_x = 1;
    if (scale_y <= 0) scale_y = 1;
    for (sy = 0; sy < 8; ++sy) {
        for (sx = 0; sx < 8; ++sx) {
            c89jpeg_i32 x0;
            c89jpeg_i32 y0;
            c89jpeg_i32 x1;
            c89jpeg_i32 y1;
            c89jpeg_u8 sample;
            x0 = base_x + ((block_x * 8 + sx) * scale_x);
            y0 = base_y + ((block_y * 8 + sy) * scale_y);
            x1 = x0 + scale_x;
            y1 = y0 + scale_y;
            sample = c89jpeg_component_value(params, (int)(comp->id - 1), x0, y0, x1, y1);
            out[sy * 8 + sx] = (c89jpeg_i32)sample - 128;
        }
    }
}

static c89jpeg_status c89jpeg_emit_block(c89jpeg_encoder *enc, c89jpeg_bitwriter *bw, const c89jpeg_component *comp)
{
    c89jpeg_i32 dc;
    c89jpeg_i32 diff;
    int cat;
    int k;
    int run;
    const c89jpeg_huffman_table *dc_tab;
    const c89jpeg_huffman_table *ac_tab;
    const c89jpeg_u16 *qt;
    qt = enc->qtable[comp->tq];
    dc_tab = &enc->huff_dc[comp->dc_table];
    ac_tab = &enc->huff_ac[comp->ac_table];

    for (k = 0; k < 64; ++k) {
        enc->qcoeff[k] = (c89jpeg_i16)c89jpeg_div_round(enc->coeff[k], (c89jpeg_i32)qt[k]);
    }

    dc = enc->qcoeff[0];
    diff = dc - comp->pred;
    ((c89jpeg_component *)comp)->pred = dc;
    cat = c89jpeg_value_category(diff);
    C89JPEG_EMIT_CHECK(c89jpeg_bw_put_bits(bw, dc_tab->ehufco[cat], dc_tab->ehufsi[cat]));
    if (cat != 0) {
        C89JPEG_EMIT_CHECK(c89jpeg_bw_put_bits(bw, c89jpeg_value_bits(diff, cat), cat));
    }

    run = 0;
    for (k = 1; k < 64; ++k) {
        c89jpeg_i32 v;
        v = enc->qcoeff[c89jpeg_zigzag[k]];
        if (v == 0) {
            ++run;
        } else {
            while (run >= 16) {
                C89JPEG_EMIT_CHECK(c89jpeg_bw_put_bits(bw, ac_tab->ehufco[0xF0], ac_tab->ehufsi[0xF0]));
                run -= 16;
            }
            cat = c89jpeg_value_category(v);
            C89JPEG_EMIT_CHECK(c89jpeg_bw_put_bits(bw, ac_tab->ehufco[(run << 4) | cat], ac_tab->ehufsi[(run << 4) | cat]));
            C89JPEG_EMIT_CHECK(c89jpeg_bw_put_bits(bw, c89jpeg_value_bits(v, cat), cat));
            run = 0;
        }
    }
    if (run != 0) {
        C89JPEG_EMIT_CHECK(c89jpeg_bw_put_bits(bw, ac_tab->ehufco[0x00], ac_tab->ehufsi[0x00]));
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_collect_huffman_stats(c89jpeg_encoder *enc,
                                                    const c89jpeg_encode_params *params,
                                                    int components,
                                                    c89jpeg_u8 max_h,
                                                    c89jpeg_u8 max_v)
{
    c89jpeg_u16 mcu_w;
    c89jpeg_u16 mcu_h;
    c89jpeg_u16 mcu_cols;
    c89jpeg_u16 mcu_rows;
    c89jpeg_u16 mcu_x;
    c89jpeg_u16 mcu_y;
    c89jpeg_u16 restart_count;
    int ci;

    memset(enc->huff_freq_dc, 0, sizeof(enc->huff_freq_dc));
    memset(enc->huff_freq_ac, 0, sizeof(enc->huff_freq_ac));
    for (ci = 0; ci < components; ++ci) enc->comp[ci].pred = 0;

    mcu_w = (c89jpeg_u16)(max_h * 8);
    mcu_h = (c89jpeg_u16)(max_v * 8);
    mcu_cols = (c89jpeg_u16)((params->width + mcu_w - 1) / mcu_w);
    mcu_rows = (c89jpeg_u16)((params->height + mcu_h - 1) / mcu_h);
    restart_count = params->restart_interval;

    for (mcu_y = 0; mcu_y < mcu_rows; ++mcu_y) {
        for (mcu_x = 0; mcu_x < mcu_cols; ++mcu_x) {
            for (ci = 0; ci < components; ++ci) {
                c89jpeg_component *comp;
                c89jpeg_u8 by;
                c89jpeg_u8 bx;
                comp = &enc->comp[ci];
                for (by = 0; by < comp->v_samp; ++by) {
                    for (bx = 0; bx < comp->h_samp; ++bx) {
                        c89jpeg_load_block(params, comp, max_h, max_v, mcu_x, mcu_y, bx, by, enc->dct_tmp);
                        c89jpeg_fdct8x8(enc->dct_tmp, enc->dct_work, enc->coeff);
                        c89jpeg_gather_block_huffman_stats(enc, comp);
                    }
                }
            }
            if (params->restart_interval != 0) {
                if (--restart_count == 0) {
                    restart_count = params->restart_interval;
                    for (ci = 0; ci < components; ++ci) enc->comp[ci].pred = 0;
                }
            }
        }
    }

    for (ci = 0; ci < components; ++ci) enc->comp[ci].pred = 0;
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_validate_huffman_table_usage(const c89jpeg_huffman_table *tab, const c89jpeg_u32 *freq)
{
    int i;
    if (tab == 0 || freq == 0 || !tab->present) return C89JPEG_ERR_BAD_ARG;
    for (i = 0; i < 256; ++i) {
        if (freq[i] != 0 && tab->ehufsi[i] == 0) return C89JPEG_ERR_BAD_ARG;
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_apply_custom_huffman_tables(c89jpeg_encoder *enc, const c89jpeg_encode_params *params, int components)
{
    c89jpeg_status st;
    if (params->custom_dc_luma == 0 || params->custom_ac_luma == 0) return C89JPEG_ERR_BAD_ARG;
    enc->huff_dc[0] = *params->custom_dc_luma;
    enc->huff_ac[0] = *params->custom_ac_luma;
    if (!enc->huff_dc[0].present || !enc->huff_ac[0].present) return C89JPEG_ERR_BAD_ARG;
    if (components > 1) {
        if (params->custom_dc_chroma == 0 || params->custom_ac_chroma == 0) return C89JPEG_ERR_BAD_ARG;
        enc->huff_dc[1] = *params->custom_dc_chroma;
        enc->huff_ac[1] = *params->custom_ac_chroma;
        if (!enc->huff_dc[1].present || !enc->huff_ac[1].present) return C89JPEG_ERR_BAD_ARG;
    }
    st = c89jpeg_validate_huffman_table_usage(&enc->huff_dc[0], enc->huff_freq_dc[0]);
    if (st != C89JPEG_OK) return st;
    st = c89jpeg_validate_huffman_table_usage(&enc->huff_ac[0], enc->huff_freq_ac[0]);
    if (st != C89JPEG_OK) return st;
    if (components > 1) {
        st = c89jpeg_validate_huffman_table_usage(&enc->huff_dc[1], enc->huff_freq_dc[1]);
        if (st != C89JPEG_OK) return st;
        st = c89jpeg_validate_huffman_table_usage(&enc->huff_ac[1], enc->huff_freq_ac[1]);
        if (st != C89JPEG_OK) return st;
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_gather_huffman_stats(c89jpeg_encoder *enc,
                                                   const c89jpeg_encode_params *params,
                                                   int components,
                                                   c89jpeg_u8 max_h,
                                                   c89jpeg_u8 max_v)
{
    c89jpeg_status st;
    st = c89jpeg_collect_huffman_stats(enc, params, components, max_h, max_v);
    if (st != C89JPEG_OK) return st;
    return c89jpeg_prepare_huffman_tables(enc, components);
}

c89jpeg_status c89jpeg_encode_sink(c89jpeg_encoder *enc, const c89jpeg_encode_params *params, c89jpeg_write_fn write_fn, void *write_user, c89jpeg_u32 *bytes_written)
{
    c89jpeg_status status;
    c89jpeg_bitwriter bw;
    c89jpeg_u8 max_h;
    c89jpeg_u8 max_v;
    c89jpeg_u16 mcu_w;
    c89jpeg_u16 mcu_h;
    c89jpeg_u16 mcu_x;
    c89jpeg_u16 mcu_y;
    c89jpeg_u16 mcu_cols;
    c89jpeg_u16 mcu_rows;
    int components;
    c89jpeg_u16 restart_count;
    c89jpeg_u8 rst_index;
    int ci;
    if (enc == 0 || params == 0 || params->pixels == 0 || write_fn == 0) return C89JPEG_ERR_BAD_ARG;
    if (params->width == 0 || params->height == 0) {
        enc->last_error = C89JPEG_ERR_BAD_ARG;
        return C89JPEG_ERR_BAD_ARG;
    }
    if (params->pixel_format != C89JPEG_PIXFMT_GRAY8 && params->pixel_format != C89JPEG_PIXFMT_RGB24) {
        enc->last_error = C89JPEG_ERR_BAD_ARG;
        return C89JPEG_ERR_BAD_ARG;
    }
    if (params->pixel_format == C89JPEG_PIXFMT_GRAY8 && params->subsampling != C89JPEG_SUBSAMP_444) {
        /* ignored */
    }
    if (params->stride_bytes == 0) {
        enc->last_error = C89JPEG_ERR_BAD_ARG;
        return C89JPEG_ERR_BAD_ARG;
    }

    c89jpeg_encoder_init(enc);
    c89jpeg_init_quant_tables(enc, params);
    c89jpeg_setup_encoder_components(enc, params, &components, &max_h, &max_v);
    if (params->huffman_mode == C89JPEG_HUFFMAN_OPTIMAL) {
        status = c89jpeg_gather_huffman_stats(enc, params, components, max_h, max_v);
        if (status != C89JPEG_OK) {
            enc->last_error = status;
            return status;
        }
    } else if (params->huffman_mode == C89JPEG_HUFFMAN_CUSTOM) {
        status = c89jpeg_collect_huffman_stats(enc, params, components, max_h, max_v);
        if (status != C89JPEG_OK) {
            enc->last_error = status;
            return status;
        }
        status = c89jpeg_apply_custom_huffman_tables(enc, params, components);
        if (status != C89JPEG_OK) {
            enc->last_error = status;
            return status;
        }
    } else if (params->huffman_mode != C89JPEG_HUFFMAN_DEFAULT) {
        enc->last_error = C89JPEG_ERR_BAD_ARG;
        return C89JPEG_ERR_BAD_ARG;
    }
    for (ci = 0; ci < components; ++ci) enc->comp[ci].pred = 0;

    c89jpeg_bw_init(&bw, write_fn, write_user);
    C89JPEG_EMIT_CHECK(c89jpeg_bw_put_marker(&bw, C89JPEG_MARKER_SOI));
    if (params->emit_jfif) C89JPEG_EMIT_CHECK(c89jpeg_write_app0(&bw, params));
    C89JPEG_EMIT_CHECK(c89jpeg_write_dqt(&bw, 0, enc->qtable[0]));
    if (components > 1) C89JPEG_EMIT_CHECK(c89jpeg_write_dqt(&bw, 1, enc->qtable[1]));
    C89JPEG_EMIT_CHECK(c89jpeg_write_sof0(&bw, params, enc->comp, components));
    C89JPEG_EMIT_CHECK(c89jpeg_write_dht(&bw, 0, 0, &enc->huff_dc[0]));
    if (components > 1) C89JPEG_EMIT_CHECK(c89jpeg_write_dht(&bw, 0, 1, &enc->huff_dc[1]));
    C89JPEG_EMIT_CHECK(c89jpeg_write_dht(&bw, 1, 0, &enc->huff_ac[0]));
    if (components > 1) C89JPEG_EMIT_CHECK(c89jpeg_write_dht(&bw, 1, 1, &enc->huff_ac[1]));
    if (params->restart_interval != 0) C89JPEG_EMIT_CHECK(c89jpeg_write_dri(&bw, params->restart_interval));
    C89JPEG_EMIT_CHECK(c89jpeg_write_sos(&bw, enc->comp, components));

    mcu_w = (c89jpeg_u16)(max_h * 8);
    mcu_h = (c89jpeg_u16)(max_v * 8);
    mcu_cols = (c89jpeg_u16)((params->width + mcu_w - 1) / mcu_w);
    mcu_rows = (c89jpeg_u16)((params->height + mcu_h - 1) / mcu_h);
    restart_count = params->restart_interval;
    rst_index = 0;

    for (mcu_y = 0; mcu_y < mcu_rows; ++mcu_y) {
        for (mcu_x = 0; mcu_x < mcu_cols; ++mcu_x) {
            for (ci = 0; ci < components; ++ci) {
                c89jpeg_component *comp;
                c89jpeg_u8 by;
                c89jpeg_u8 bx;
                comp = &enc->comp[ci];
                for (by = 0; by < comp->v_samp; ++by) {
                    for (bx = 0; bx < comp->h_samp; ++bx) {
                        c89jpeg_load_block(params, comp, max_h, max_v, mcu_x, mcu_y, bx, by, enc->dct_tmp);
                        c89jpeg_fdct8x8(enc->dct_tmp, enc->dct_work, enc->coeff);
                        C89JPEG_EMIT_CHECK(c89jpeg_emit_block(enc, &bw, comp));
                    }
                }
            }
            if (params->restart_interval != 0) {
                if (--restart_count == 0) {
                    C89JPEG_EMIT_CHECK(c89jpeg_bw_flush_pad1(&bw));
                    C89JPEG_EMIT_CHECK(c89jpeg_bw_put_marker(&bw, (c89jpeg_u8)(C89JPEG_MARKER_RST0 + rst_index)));
                    rst_index = (c89jpeg_u8)((rst_index + 1) & 7);
                    restart_count = params->restart_interval;
                    for (ci = 0; ci < components; ++ci) enc->comp[ci].pred = 0;
                }
            }
        }
    }

    C89JPEG_EMIT_CHECK(c89jpeg_bw_flush_pad1(&bw));
    C89JPEG_EMIT_CHECK(c89jpeg_bw_put_marker(&bw, C89JPEG_MARKER_EOI));
    C89JPEG_EMIT_CHECK(c89jpeg_bw_flush_bytes(&bw));
    enc->emitted_size = bw.total;
    if (bytes_written != 0) *bytes_written = bw.total;
    status = C89JPEG_OK;
    enc->last_error = status;
    return status;
}

c89jpeg_status c89jpeg_encode_memory(c89jpeg_encoder *enc, const c89jpeg_encode_params *params, c89jpeg_u8 *out_buf, c89jpeg_u32 out_capacity, c89jpeg_u32 *out_size)
{
    c89jpeg_mem_dest dest;
    c89jpeg_status st;
    if (enc == 0 || out_buf == 0) return C89JPEG_ERR_BAD_ARG;
    c89jpeg_mem_dest_init(&dest, out_buf, out_capacity);
    st = c89jpeg_encode_sink(enc, params, c89jpeg_mem_dest_write, &dest, out_size);
    if (st == C89JPEG_OK && out_size != 0) *out_size = dest.size;
    return st;
}


c89jpeg_status c89jpeg_tables_prepare(c89jpeg_encoder *enc, const c89jpeg_encode_params *params, c89jpeg_tables *tables)
{
    c89jpeg_status st;
    int components;
    c89jpeg_u8 max_h;
    c89jpeg_u8 max_v;
    if (enc == 0 || params == 0 || tables == 0) return C89JPEG_ERR_BAD_ARG;
    st = c89jpeg_encoder_prepare_from_params(enc, params, &components, &max_h, &max_v);
    if (st != C89JPEG_OK) {
        enc->last_error = st;
        return st;
    }
    c89jpeg_tables_copy_from_encoder(tables, enc, components, params->restart_interval);
    enc->last_error = C89JPEG_OK;
    return C89JPEG_OK;
}

c89jpeg_status c89jpeg_write_tables_sink(const c89jpeg_tables *tables, c89jpeg_write_fn write_fn, void *write_user, c89jpeg_u32 *bytes_written)
{
    c89jpeg_bitwriter bw;
    int tq;
    int tc;
    int th;
    if (tables == 0 || write_fn == 0) return C89JPEG_ERR_BAD_ARG;
    c89jpeg_bw_init(&bw, write_fn, write_user);
    C89JPEG_EMIT_CHECK(c89jpeg_bw_put_marker(&bw, C89JPEG_MARKER_SOI));
    for (tq = 0; tq < C89JPEG_MAX_QUANT_TABLES; ++tq) {
        if (tables->quant_present[tq]) {
            C89JPEG_EMIT_CHECK(c89jpeg_write_dqt(&bw, (c89jpeg_u8)tq, tables->quant[tq]));
        }
    }
    for (tc = 0; tc < C89JPEG_MAX_HUFF_CLASSES; ++tc) {
        for (th = 0; th < C89JPEG_MAX_HUFF_TABLES; ++th) {
            if (tables->huff[tc][th].present) {
                C89JPEG_EMIT_CHECK(c89jpeg_write_dht(&bw, (c89jpeg_u8)tc, (c89jpeg_u8)th, &tables->huff[tc][th]));
            }
        }
    }
    if (tables->restart_interval != 0) C89JPEG_EMIT_CHECK(c89jpeg_write_dri(&bw, tables->restart_interval));
    C89JPEG_EMIT_CHECK(c89jpeg_bw_put_marker(&bw, C89JPEG_MARKER_EOI));
    C89JPEG_EMIT_CHECK(c89jpeg_bw_flush_bytes(&bw));
    if (bytes_written != 0) *bytes_written = bw.total;
    return C89JPEG_OK;
}

c89jpeg_status c89jpeg_write_tables_memory(const c89jpeg_tables *tables, c89jpeg_u8 *out_buf, c89jpeg_u32 out_capacity, c89jpeg_u32 *out_size)
{
    c89jpeg_mem_dest dest;
    c89jpeg_status st;
    if (tables == 0 || out_buf == 0) return C89JPEG_ERR_BAD_ARG;
    c89jpeg_mem_dest_init(&dest, out_buf, out_capacity);
    st = c89jpeg_write_tables_sink(tables, c89jpeg_mem_dest_write, &dest, out_size);
    if (st == C89JPEG_OK && out_size != 0) *out_size = dest.size;
    return st;
}

c89jpeg_status c89jpeg_encode_abbreviated_sink(c89jpeg_encoder *enc,
                                               const c89jpeg_encode_params *params,
                                               const c89jpeg_tables *tables,
                                               c89jpeg_write_fn write_fn,
                                               void *write_user,
                                               c89jpeg_u32 *bytes_written)
{
    c89jpeg_status status;
    c89jpeg_bitwriter bw;
    c89jpeg_u16 mcu_w;
    c89jpeg_u16 mcu_h;
    c89jpeg_u16 mcu_x;
    c89jpeg_u16 mcu_y;
    c89jpeg_u16 mcu_cols;
    c89jpeg_u16 mcu_rows;
    c89jpeg_u16 restart_count;
    c89jpeg_u16 restart_interval;
    c89jpeg_u8 rst_index;
    c89jpeg_u8 max_h;
    c89jpeg_u8 max_v;
    int components;
    int ci;
    if (enc == 0 || params == 0 || tables == 0 || write_fn == 0) return C89JPEG_ERR_BAD_ARG;
    status = c89jpeg_encoder_apply_tables(enc, tables, params, &components, &max_h, &max_v, &restart_interval);
    if (status != C89JPEG_OK) {
        enc->last_error = status;
        return status;
    }
    c89jpeg_bw_init(&bw, write_fn, write_user);
    C89JPEG_EMIT_CHECK(c89jpeg_bw_put_marker(&bw, C89JPEG_MARKER_SOI));
    C89JPEG_EMIT_CHECK(c89jpeg_write_sof0(&bw, params, enc->comp, components));
    C89JPEG_EMIT_CHECK(c89jpeg_write_sos(&bw, enc->comp, components));

    mcu_w = (c89jpeg_u16)(max_h * 8);
    mcu_h = (c89jpeg_u16)(max_v * 8);
    mcu_cols = (c89jpeg_u16)((params->width + mcu_w - 1) / mcu_w);
    mcu_rows = (c89jpeg_u16)((params->height + mcu_h - 1) / mcu_h);
    restart_count = restart_interval;
    rst_index = 0;

    for (mcu_y = 0; mcu_y < mcu_rows; ++mcu_y) {
        for (mcu_x = 0; mcu_x < mcu_cols; ++mcu_x) {
            for (ci = 0; ci < components; ++ci) {
                c89jpeg_component *comp;
                c89jpeg_u8 by;
                c89jpeg_u8 bx;
                comp = &enc->comp[ci];
                for (by = 0; by < comp->v_samp; ++by) {
                    for (bx = 0; bx < comp->h_samp; ++bx) {
                        c89jpeg_load_block(params, comp, max_h, max_v, mcu_x, mcu_y, bx, by, enc->dct_tmp);
                        c89jpeg_fdct8x8(enc->dct_tmp, enc->dct_work, enc->coeff);
                        C89JPEG_EMIT_CHECK(c89jpeg_emit_block(enc, &bw, comp));
                    }
                }
            }
            if (restart_interval != 0) {
                if (--restart_count == 0) {
                    C89JPEG_EMIT_CHECK(c89jpeg_bw_flush_pad1(&bw));
                    C89JPEG_EMIT_CHECK(c89jpeg_bw_put_marker(&bw, (c89jpeg_u8)(C89JPEG_MARKER_RST0 + rst_index)));
                    rst_index = (c89jpeg_u8)((rst_index + 1) & 7);
                    restart_count = restart_interval;
                    for (ci = 0; ci < components; ++ci) enc->comp[ci].pred = 0;
                }
            }
        }
    }

    C89JPEG_EMIT_CHECK(c89jpeg_bw_flush_pad1(&bw));
    C89JPEG_EMIT_CHECK(c89jpeg_bw_put_marker(&bw, C89JPEG_MARKER_EOI));
    C89JPEG_EMIT_CHECK(c89jpeg_bw_flush_bytes(&bw));
    enc->emitted_size = bw.total;
    if (bytes_written != 0) *bytes_written = bw.total;
    enc->last_error = C89JPEG_OK;
    return C89JPEG_OK;
}

c89jpeg_status c89jpeg_encode_abbreviated_memory(c89jpeg_encoder *enc, const c89jpeg_encode_params *params, const c89jpeg_tables *tables, c89jpeg_u8 *out_buf, c89jpeg_u32 out_capacity, c89jpeg_u32 *out_size)
{
    c89jpeg_mem_dest dest;
    c89jpeg_status st;
    if (enc == 0 || out_buf == 0) return C89JPEG_ERR_BAD_ARG;
    c89jpeg_mem_dest_init(&dest, out_buf, out_capacity);
    st = c89jpeg_encode_abbreviated_sink(enc, params, tables, c89jpeg_mem_dest_write, &dest, out_size);
    if (st == C89JPEG_OK && out_size != 0) *out_size = dest.size;
    return st;
}


typedef struct c89jpeg_encode_row_cache_tag {
    const c89jpeg_encode_source_params *params;
    c89jpeg_u16 base_y;
    c89jpeg_u16 mcu_h;
    c89jpeg_u8 loaded;
} c89jpeg_encode_row_cache;

static c89jpeg_u8 c89jpeg_source_max_v(c89jpeg_pixel_format fmt, c89jpeg_subsampling subsampling)
{
    if (fmt == C89JPEG_PIXFMT_GRAY8) return 1;
    return (subsampling == C89JPEG_SUBSAMP_420) ? 2 : 1;
}

static c89jpeg_status c89jpeg_source_min_stride(c89jpeg_u16 width,
                                                c89jpeg_pixel_format fmt,
                                                c89jpeg_u32 *out_min_stride)
{
    c89jpeg_u32 channels;
    if (out_min_stride == 0) return C89JPEG_ERR_BAD_ARG;
    if (fmt == C89JPEG_PIXFMT_GRAY8) channels = 1;
    else if (fmt == C89JPEG_PIXFMT_RGB24) channels = 3;
    else return C89JPEG_ERR_BAD_ARG;
    if (!c89jpeg_mul_u32((c89jpeg_u32)width, channels, out_min_stride)) return C89JPEG_ERR_LIMIT;
    return C89JPEG_OK;
}

c89jpeg_u32 c89jpeg_encoder_source_row_cache_size(c89jpeg_u16 width,
                                                  c89jpeg_pixel_format fmt,
                                                  c89jpeg_subsampling subsampling,
                                                  c89jpeg_u32 stride_bytes)
{
    c89jpeg_u32 min_stride;
    c89jpeg_u32 rows;
    c89jpeg_u32 need;
    if (c89jpeg_source_min_stride(width, fmt, &min_stride) != C89JPEG_OK) return 0;
    if (stride_bytes < min_stride) return 0;
    rows = (c89jpeg_u32)c89jpeg_source_max_v(fmt, subsampling) * 8UL;
    if (!c89jpeg_mul_u32(stride_bytes, rows, &need)) return 0;
    return need;
}

static void c89jpeg_source_params_to_encode_params(const c89jpeg_encode_source_params *src,
                                                   c89jpeg_encode_params *dst)
{
    if (dst == 0) return;
    memset(dst, 0, sizeof(*dst));
    if (src == 0) return;
    dst->width = src->width;
    dst->height = src->height;
    dst->pixels = src->row_cache;
    dst->stride_bytes = src->stride_bytes;
    dst->pixel_format = src->pixel_format;
    dst->subsampling = src->subsampling;
    dst->quality = src->quality;
    dst->restart_interval = src->restart_interval;
    dst->emit_jfif = src->emit_jfif;
    dst->density_units = src->density_units;
    dst->density_x = src->density_x;
    dst->density_y = src->density_y;
    dst->quant_luma = src->quant_luma;
    dst->quant_chroma = src->quant_chroma;
    dst->huffman_mode = src->huffman_mode;
    dst->custom_dc_luma = src->custom_dc_luma;
    dst->custom_ac_luma = src->custom_ac_luma;
    dst->custom_dc_chroma = src->custom_dc_chroma;
    dst->custom_ac_chroma = src->custom_ac_chroma;
}

static c89jpeg_status c89jpeg_validate_encode_source_params(const c89jpeg_encode_source_params *params)
{
    c89jpeg_u32 min_stride;
    c89jpeg_u32 cache_need;
    c89jpeg_status st;
    if (params == 0 || params->read_row_fn == 0 || params->row_cache == 0) return C89JPEG_ERR_BAD_ARG;
    if (params->width == 0 || params->height == 0) return C89JPEG_ERR_BAD_ARG;
    if (params->pixel_format != C89JPEG_PIXFMT_GRAY8 && params->pixel_format != C89JPEG_PIXFMT_RGB24) return C89JPEG_ERR_BAD_ARG;
    if (params->huffman_mode != C89JPEG_HUFFMAN_DEFAULT &&
        params->huffman_mode != C89JPEG_HUFFMAN_OPTIMAL &&
        params->huffman_mode != C89JPEG_HUFFMAN_CUSTOM) return C89JPEG_ERR_BAD_ARG;
    st = c89jpeg_source_min_stride(params->width, params->pixel_format, &min_stride);
    if (st != C89JPEG_OK) return st;
    if (params->stride_bytes < min_stride) return C89JPEG_ERR_BAD_ARG;
    cache_need = c89jpeg_encoder_source_row_cache_size(params->width,
                                                       params->pixel_format,
                                                       params->subsampling,
                                                       params->stride_bytes);
    if (cache_need == 0) return C89JPEG_ERR_LIMIT;
    if (params->row_cache_size < cache_need) return C89JPEG_ERR_SHORT_BUFFER;
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_encode_row_cache_load(c89jpeg_encode_row_cache *cache, c89jpeg_u16 mcu_y)
{
    c89jpeg_u16 row;
    c89jpeg_u16 base_y;
    c89jpeg_u8 *dst;
    const c89jpeg_encode_source_params *params;
    if (cache == 0 || cache->params == 0) return C89JPEG_ERR_BAD_ARG;
    if (cache->loaded && cache->base_y == (c89jpeg_u16)(mcu_y * cache->mcu_h)) return C89JPEG_OK;
    params = cache->params;
    base_y = (c89jpeg_u16)(mcu_y * cache->mcu_h);
    for (row = 0; row < cache->mcu_h; ++row) {
        c89jpeg_u16 image_y;
        dst = params->row_cache + (c89jpeg_u32)row * params->stride_bytes;
        image_y = (c89jpeg_u16)(base_y + row);
        if (image_y < params->height) {
            if (!params->read_row_fn(params->read_user, image_y, dst, params->stride_bytes)) {
                return C89JPEG_ERR_IO;
            }
        } else {
            memcpy(dst, dst - params->stride_bytes, (size_t)params->stride_bytes);
        }
    }
    cache->base_y = base_y;
    cache->loaded = 1;
    return C89JPEG_OK;
}

static c89jpeg_u8 c89jpeg_source_sample_gray(const c89jpeg_encode_row_cache *cache,
                                             c89jpeg_i32 x,
                                             c89jpeg_i32 y)
{
    c89jpeg_i32 local_y;
    const c89jpeg_encode_source_params *params;
    params = cache->params;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= (c89jpeg_i32)params->width) x = params->width - 1;
    local_y = y - cache->base_y;
    if (local_y < 0) local_y = 0;
    if (local_y >= (c89jpeg_i32)cache->mcu_h) local_y = cache->mcu_h - 1;
    return params->row_cache[(c89jpeg_u32)local_y * params->stride_bytes + (c89jpeg_u32)x];
}

static void c89jpeg_source_sample_rgb(const c89jpeg_encode_row_cache *cache,
                                      c89jpeg_i32 x,
                                      c89jpeg_i32 y,
                                      c89jpeg_u8 *r,
                                      c89jpeg_u8 *g,
                                      c89jpeg_u8 *b)
{
    c89jpeg_i32 local_y;
    const c89jpeg_u8 *p;
    const c89jpeg_encode_source_params *params;
    params = cache->params;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= (c89jpeg_i32)params->width) x = params->width - 1;
    local_y = y - cache->base_y;
    if (local_y < 0) local_y = 0;
    if (local_y >= (c89jpeg_i32)cache->mcu_h) local_y = cache->mcu_h - 1;
    p = params->row_cache + (c89jpeg_u32)local_y * params->stride_bytes + (c89jpeg_u32)x * 3UL;
    *r = p[0];
    *g = p[1];
    *b = p[2];
}

static c89jpeg_u8 c89jpeg_source_component_value(const c89jpeg_encode_source_params *params,
                                                 const c89jpeg_encode_row_cache *cache,
                                                 int component,
                                                 c89jpeg_i32 x0,
                                                 c89jpeg_i32 y0,
                                                 c89jpeg_i32 x1,
                                                 c89jpeg_i32 y1)
{
    c89jpeg_i32 x;
    c89jpeg_i32 y;
    c89jpeg_i32 sum;
    c89jpeg_i32 count;
    sum = 0;
    count = 0;
    for (y = y0; y < y1; ++y) {
        for (x = x0; x < x1; ++x) {
            if (params->pixel_format == C89JPEG_PIXFMT_GRAY8) {
                sum += c89jpeg_source_sample_gray(cache, x, y);
            } else {
                c89jpeg_u8 r, g, b;
                c89jpeg_source_sample_rgb(cache, x, y, &r, &g, &b);
                if (component == 0) sum += c89jpeg_rgb_to_y(r, g, b);
                else if (component == 1) sum += c89jpeg_rgb_to_cb(r, g, b);
                else sum += c89jpeg_rgb_to_cr(r, g, b);
            }
            ++count;
        }
    }
    if (count == 0) return 0;
    return c89jpeg_clamp_u8(c89jpeg_div_round(sum, count));
}

static void c89jpeg_source_load_block(const c89jpeg_encode_source_params *params,
                                      const c89jpeg_encode_row_cache *cache,
                                      const c89jpeg_component *comp,
                                      c89jpeg_u8 max_h,
                                      c89jpeg_u8 max_v,
                                      c89jpeg_u16 mcu_x,
                                      c89jpeg_u16 mcu_y,
                                      c89jpeg_u8 block_x,
                                      c89jpeg_u8 block_y,
                                      c89jpeg_i32 *out)
{
    c89jpeg_i32 base_x;
    c89jpeg_i32 base_y;
    c89jpeg_i32 sx;
    c89jpeg_i32 sy;
    c89jpeg_i32 scale_x;
    c89jpeg_i32 scale_y;
    (void)mcu_y;
    base_x = (c89jpeg_i32)mcu_x * (c89jpeg_i32)max_h * 8;
    base_y = (c89jpeg_i32)cache->base_y;
    scale_x = (c89jpeg_i32)max_h / (c89jpeg_i32)comp->h_samp;
    scale_y = (c89jpeg_i32)max_v / (c89jpeg_i32)comp->v_samp;
    if (scale_x <= 0) scale_x = 1;
    if (scale_y <= 0) scale_y = 1;
    for (sy = 0; sy < 8; ++sy) {
        for (sx = 0; sx < 8; ++sx) {
            c89jpeg_i32 x0;
            c89jpeg_i32 y0;
            c89jpeg_i32 x1;
            c89jpeg_i32 y1;
            c89jpeg_u8 sample;
            x0 = base_x + ((block_x * 8 + sx) * scale_x);
            y0 = base_y + ((block_y * 8 + sy) * scale_y);
            x1 = x0 + scale_x;
            y1 = y0 + scale_y;
            sample = c89jpeg_source_component_value(params, cache, (int)(comp->id - 1), x0, y0, x1, y1);
            out[sy * 8 + sx] = (c89jpeg_i32)sample - 128;
        }
    }
}

static c89jpeg_status c89jpeg_collect_huffman_stats_source(c89jpeg_encoder *enc,
                                                           const c89jpeg_encode_source_params *params,
                                                           int components,
                                                           c89jpeg_u8 max_h,
                                                           c89jpeg_u8 max_v)
{
    c89jpeg_encode_row_cache cache;
    c89jpeg_u16 mcu_w;
    c89jpeg_u16 mcu_h;
    c89jpeg_u16 mcu_cols;
    c89jpeg_u16 mcu_rows;
    c89jpeg_u16 mcu_x;
    c89jpeg_u16 mcu_y;
    c89jpeg_u16 restart_count;
    int ci;
    c89jpeg_status st;

    memset(enc->huff_freq_dc, 0, sizeof(enc->huff_freq_dc));
    memset(enc->huff_freq_ac, 0, sizeof(enc->huff_freq_ac));
    for (ci = 0; ci < components; ++ci) enc->comp[ci].pred = 0;

    cache.params = params;
    cache.base_y = 0;
    cache.mcu_h = (c89jpeg_u16)(max_v * 8);
    cache.loaded = 0;

    mcu_w = (c89jpeg_u16)(max_h * 8);
    mcu_h = (c89jpeg_u16)(max_v * 8);
    mcu_cols = (c89jpeg_u16)((params->width + mcu_w - 1) / mcu_w);
    mcu_rows = (c89jpeg_u16)((params->height + mcu_h - 1) / mcu_h);
    restart_count = params->restart_interval;

    for (mcu_y = 0; mcu_y < mcu_rows; ++mcu_y) {
        st = c89jpeg_encode_row_cache_load(&cache, mcu_y);
        if (st != C89JPEG_OK) return st;
        for (mcu_x = 0; mcu_x < mcu_cols; ++mcu_x) {
            for (ci = 0; ci < components; ++ci) {
                c89jpeg_component *comp;
                c89jpeg_u8 by;
                c89jpeg_u8 bx;
                comp = &enc->comp[ci];
                for (by = 0; by < comp->v_samp; ++by) {
                    for (bx = 0; bx < comp->h_samp; ++bx) {
                        c89jpeg_source_load_block(params, &cache, comp, max_h, max_v, mcu_x, mcu_y, bx, by, enc->dct_tmp);
                        c89jpeg_fdct8x8(enc->dct_tmp, enc->dct_work, enc->coeff);
                        c89jpeg_gather_block_huffman_stats(enc, comp);
                    }
                }
            }
            if (params->restart_interval != 0) {
                if (--restart_count == 0) {
                    restart_count = params->restart_interval;
                    for (ci = 0; ci < components; ++ci) enc->comp[ci].pred = 0;
                }
            }
        }
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_gather_huffman_stats_source(c89jpeg_encoder *enc,
                                                          const c89jpeg_encode_source_params *params,
                                                          int components,
                                                          c89jpeg_u8 max_h,
                                                          c89jpeg_u8 max_v)
{
    c89jpeg_status st;
    st = c89jpeg_collect_huffman_stats_source(enc, params, components, max_h, max_v);
    if (st != C89JPEG_OK) return st;
    return c89jpeg_prepare_huffman_tables(enc, components);
}

static c89jpeg_status c89jpeg_encoder_prepare_from_source_params(c89jpeg_encoder *enc,
                                                                 const c89jpeg_encode_source_params *params,
                                                                 int *components,
                                                                 c89jpeg_u8 *max_h,
                                                                 c89jpeg_u8 *max_v)
{
    c89jpeg_status status;
    c89jpeg_encode_params local_params;
    int ci;
    if (enc == 0 || params == 0 || components == 0 || max_h == 0 || max_v == 0) return C89JPEG_ERR_BAD_ARG;
    status = c89jpeg_validate_encode_source_params(params);
    if (status != C89JPEG_OK) return status;
    c89jpeg_source_params_to_encode_params(params, &local_params);
    c89jpeg_encoder_init(enc);
    c89jpeg_init_quant_tables(enc, &local_params);
    c89jpeg_setup_encoder_components(enc, &local_params, components, max_h, max_v);
    if (params->huffman_mode == C89JPEG_HUFFMAN_OPTIMAL) {
        status = c89jpeg_gather_huffman_stats_source(enc, params, *components, *max_h, *max_v);
        if (status != C89JPEG_OK) return status;
    } else if (params->huffman_mode == C89JPEG_HUFFMAN_CUSTOM) {
        status = c89jpeg_collect_huffman_stats_source(enc, params, *components, *max_h, *max_v);
        if (status != C89JPEG_OK) return status;
        status = c89jpeg_apply_custom_huffman_tables(enc, &local_params, *components);
        if (status != C89JPEG_OK) return status;
    }
    for (ci = 0; ci < *components; ++ci) enc->comp[ci].pred = 0;
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_encoder_apply_tables_source(c89jpeg_encoder *enc,
                                                          const c89jpeg_tables *tables,
                                                          const c89jpeg_encode_source_params *params,
                                                          int *components,
                                                          c89jpeg_u8 *max_h,
                                                          c89jpeg_u8 *max_v,
                                                          c89jpeg_u16 *restart_interval)
{
    c89jpeg_status status;
    c89jpeg_encode_params local_params;
    int ci;
    if (enc == 0 || tables == 0 || params == 0 || components == 0 || max_h == 0 || max_v == 0 || restart_interval == 0) return C89JPEG_ERR_BAD_ARG;
    status = c89jpeg_validate_encode_source_params(params);
    if (status != C89JPEG_OK) return status;
    c89jpeg_source_params_to_encode_params(params, &local_params);
    c89jpeg_encoder_init(enc);
    c89jpeg_setup_encoder_components(enc, &local_params, components, max_h, max_v);
    if (!tables->quant_present[0]) return C89JPEG_ERR_BAD_ARG;
    memcpy(enc->qtable[0], tables->quant[0], sizeof(enc->qtable[0]));
    if (*components > 1) {
        if (!tables->quant_present[1]) return C89JPEG_ERR_BAD_ARG;
        memcpy(enc->qtable[1], tables->quant[1], sizeof(enc->qtable[1]));
    }
    if (!tables->huff[0][0].present || !tables->huff[1][0].present) return C89JPEG_ERR_BAD_ARG;
    enc->huff_dc[0] = tables->huff[0][0];
    enc->huff_ac[0] = tables->huff[1][0];
    if (*components > 1) {
        if (!tables->huff[0][1].present || !tables->huff[1][1].present) return C89JPEG_ERR_BAD_ARG;
        enc->huff_dc[1] = tables->huff[0][1];
        enc->huff_ac[1] = tables->huff[1][1];
    }
    status = c89jpeg_collect_huffman_stats_source(enc, params, *components, *max_h, *max_v);
    if (status != C89JPEG_OK) return status;
    status = c89jpeg_validate_huffman_table_usage(&enc->huff_dc[0], enc->huff_freq_dc[0]);
    if (status != C89JPEG_OK) return status;
    status = c89jpeg_validate_huffman_table_usage(&enc->huff_ac[0], enc->huff_freq_ac[0]);
    if (status != C89JPEG_OK) return status;
    if (*components > 1) {
        status = c89jpeg_validate_huffman_table_usage(&enc->huff_dc[1], enc->huff_freq_dc[1]);
        if (status != C89JPEG_OK) return status;
        status = c89jpeg_validate_huffman_table_usage(&enc->huff_ac[1], enc->huff_freq_ac[1]);
        if (status != C89JPEG_OK) return status;
    }
    for (ci = 0; ci < *components; ++ci) enc->comp[ci].pred = 0;
    *restart_interval = tables->restart_interval;
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_encode_source_scan(c89jpeg_encoder *enc,
                                                 const c89jpeg_encode_source_params *params,
                                                 c89jpeg_u8 max_h,
                                                 c89jpeg_u8 max_v,
                                                 int components,
                                                 c89jpeg_u16 restart_interval,
                                                 c89jpeg_bitwriter *bw)
{
    c89jpeg_encode_row_cache cache;
    c89jpeg_u16 mcu_w;
    c89jpeg_u16 mcu_h;
    c89jpeg_u16 mcu_x;
    c89jpeg_u16 mcu_y;
    c89jpeg_u16 mcu_cols;
    c89jpeg_u16 mcu_rows;
    c89jpeg_u16 restart_count;
    c89jpeg_u8 rst_index;
    int ci;
    c89jpeg_status st;

    cache.params = params;
    cache.base_y = 0;
    cache.mcu_h = (c89jpeg_u16)(max_v * 8);
    cache.loaded = 0;

    mcu_w = (c89jpeg_u16)(max_h * 8);
    mcu_h = (c89jpeg_u16)(max_v * 8);
    mcu_cols = (c89jpeg_u16)((params->width + mcu_w - 1) / mcu_w);
    mcu_rows = (c89jpeg_u16)((params->height + mcu_h - 1) / mcu_h);
    restart_count = restart_interval;
    rst_index = 0;

    for (mcu_y = 0; mcu_y < mcu_rows; ++mcu_y) {
        st = c89jpeg_encode_row_cache_load(&cache, mcu_y);
        if (st != C89JPEG_OK) return st;
        for (mcu_x = 0; mcu_x < mcu_cols; ++mcu_x) {
            for (ci = 0; ci < components; ++ci) {
                c89jpeg_component *comp;
                c89jpeg_u8 by;
                c89jpeg_u8 bx;
                comp = &enc->comp[ci];
                for (by = 0; by < comp->v_samp; ++by) {
                    for (bx = 0; bx < comp->h_samp; ++bx) {
                        c89jpeg_source_load_block(params, &cache, comp, max_h, max_v, mcu_x, mcu_y, bx, by, enc->dct_tmp);
                        c89jpeg_fdct8x8(enc->dct_tmp, enc->dct_work, enc->coeff);
                        C89JPEG_EMIT_CHECK(c89jpeg_emit_block(enc, bw, comp));
                    }
                }
            }
            if (restart_interval != 0) {
                if (--restart_count == 0) {
                    C89JPEG_EMIT_CHECK(c89jpeg_bw_flush_pad1(bw));
                    C89JPEG_EMIT_CHECK(c89jpeg_bw_put_marker(bw, (c89jpeg_u8)(C89JPEG_MARKER_RST0 + rst_index)));
                    rst_index = (c89jpeg_u8)((rst_index + 1) & 7);
                    restart_count = restart_interval;
                    for (ci = 0; ci < components; ++ci) enc->comp[ci].pred = 0;
                }
            }
        }
    }
    return C89JPEG_OK;
}

c89jpeg_status c89jpeg_tables_prepare_source(c89jpeg_encoder *enc,
                                             const c89jpeg_encode_source_params *params,
                                             c89jpeg_tables *tables)
{
    c89jpeg_status st;
    int components;
    c89jpeg_u8 max_h;
    c89jpeg_u8 max_v;
    if (enc == 0 || params == 0 || tables == 0) return C89JPEG_ERR_BAD_ARG;
    st = c89jpeg_encoder_prepare_from_source_params(enc, params, &components, &max_h, &max_v);
    (void)max_h;
    (void)max_v;
    if (st != C89JPEG_OK) {
        enc->last_error = st;
        return st;
    }
    c89jpeg_tables_copy_from_encoder(tables, enc, components, params->restart_interval);
    enc->last_error = C89JPEG_OK;
    return C89JPEG_OK;
}

c89jpeg_status c89jpeg_encode_source_sink(c89jpeg_encoder *enc,
                                          const c89jpeg_encode_source_params *params,
                                          c89jpeg_write_fn write_fn,
                                          void *write_user,
                                          c89jpeg_u32 *bytes_written)
{
    c89jpeg_status status;
    c89jpeg_bitwriter bw;
    c89jpeg_u8 max_h;
    c89jpeg_u8 max_v;
    int components;
    c89jpeg_encode_params local_params;
    if (enc == 0 || params == 0 || write_fn == 0) return C89JPEG_ERR_BAD_ARG;
    status = c89jpeg_encoder_prepare_from_source_params(enc, params, &components, &max_h, &max_v);
    if (status != C89JPEG_OK) {
        enc->last_error = status;
        return status;
    }
    c89jpeg_source_params_to_encode_params(params, &local_params);
    c89jpeg_bw_init(&bw, write_fn, write_user);
    C89JPEG_EMIT_CHECK(c89jpeg_bw_put_marker(&bw, C89JPEG_MARKER_SOI));
    if (params->emit_jfif) C89JPEG_EMIT_CHECK(c89jpeg_write_app0(&bw, &local_params));
    C89JPEG_EMIT_CHECK(c89jpeg_write_dqt(&bw, 0, enc->qtable[0]));
    if (components > 1) C89JPEG_EMIT_CHECK(c89jpeg_write_dqt(&bw, 1, enc->qtable[1]));
    C89JPEG_EMIT_CHECK(c89jpeg_write_sof0(&bw, &local_params, enc->comp, components));
    C89JPEG_EMIT_CHECK(c89jpeg_write_dht(&bw, 0, 0, &enc->huff_dc[0]));
    if (components > 1) C89JPEG_EMIT_CHECK(c89jpeg_write_dht(&bw, 0, 1, &enc->huff_dc[1]));
    C89JPEG_EMIT_CHECK(c89jpeg_write_dht(&bw, 1, 0, &enc->huff_ac[0]));
    if (components > 1) C89JPEG_EMIT_CHECK(c89jpeg_write_dht(&bw, 1, 1, &enc->huff_ac[1]));
    if (params->restart_interval != 0) C89JPEG_EMIT_CHECK(c89jpeg_write_dri(&bw, params->restart_interval));
    C89JPEG_EMIT_CHECK(c89jpeg_write_sos(&bw, enc->comp, components));
    C89JPEG_EMIT_CHECK(c89jpeg_encode_source_scan(enc, params, max_h, max_v, components, params->restart_interval, &bw));
    C89JPEG_EMIT_CHECK(c89jpeg_bw_flush_pad1(&bw));
    C89JPEG_EMIT_CHECK(c89jpeg_bw_put_marker(&bw, C89JPEG_MARKER_EOI));
    C89JPEG_EMIT_CHECK(c89jpeg_bw_flush_bytes(&bw));
    enc->emitted_size = bw.total;
    if (bytes_written != 0) *bytes_written = bw.total;
    enc->last_error = C89JPEG_OK;
    return C89JPEG_OK;
}

c89jpeg_status c89jpeg_encode_source_memory(c89jpeg_encoder *enc,
                                            const c89jpeg_encode_source_params *params,
                                            c89jpeg_u8 *out_buf,
                                            c89jpeg_u32 out_capacity,
                                            c89jpeg_u32 *out_size)
{
    c89jpeg_mem_dest dest;
    c89jpeg_status st;
    if (enc == 0 || out_buf == 0) return C89JPEG_ERR_BAD_ARG;
    c89jpeg_mem_dest_init(&dest, out_buf, out_capacity);
    st = c89jpeg_encode_source_sink(enc, params, c89jpeg_mem_dest_write, &dest, out_size);
    if (st == C89JPEG_OK && out_size != 0) *out_size = dest.size;
    return st;
}

c89jpeg_status c89jpeg_encode_abbreviated_source_sink(c89jpeg_encoder *enc,
                                                      const c89jpeg_encode_source_params *params,
                                                      const c89jpeg_tables *tables,
                                                      c89jpeg_write_fn write_fn,
                                                      void *write_user,
                                                      c89jpeg_u32 *bytes_written)
{
    c89jpeg_status status;
    c89jpeg_bitwriter bw;
    c89jpeg_u16 restart_interval;
    c89jpeg_u8 max_h;
    c89jpeg_u8 max_v;
    int components;
    c89jpeg_encode_params local_params;
    if (enc == 0 || params == 0 || tables == 0 || write_fn == 0) return C89JPEG_ERR_BAD_ARG;
    status = c89jpeg_encoder_apply_tables_source(enc, tables, params, &components, &max_h, &max_v, &restart_interval);
    if (status != C89JPEG_OK) {
        enc->last_error = status;
        return status;
    }
    c89jpeg_source_params_to_encode_params(params, &local_params);
    c89jpeg_bw_init(&bw, write_fn, write_user);
    C89JPEG_EMIT_CHECK(c89jpeg_bw_put_marker(&bw, C89JPEG_MARKER_SOI));
    C89JPEG_EMIT_CHECK(c89jpeg_write_sof0(&bw, &local_params, enc->comp, components));
    C89JPEG_EMIT_CHECK(c89jpeg_write_sos(&bw, enc->comp, components));
    C89JPEG_EMIT_CHECK(c89jpeg_encode_source_scan(enc, params, max_h, max_v, components, restart_interval, &bw));
    C89JPEG_EMIT_CHECK(c89jpeg_bw_flush_pad1(&bw));
    C89JPEG_EMIT_CHECK(c89jpeg_bw_put_marker(&bw, C89JPEG_MARKER_EOI));
    C89JPEG_EMIT_CHECK(c89jpeg_bw_flush_bytes(&bw));
    enc->emitted_size = bw.total;
    if (bytes_written != 0) *bytes_written = bw.total;
    enc->last_error = C89JPEG_OK;
    return C89JPEG_OK;
}

c89jpeg_status c89jpeg_encode_abbreviated_source_memory(c89jpeg_encoder *enc,
                                                        const c89jpeg_encode_source_params *params,
                                                        const c89jpeg_tables *tables,
                                                        c89jpeg_u8 *out_buf,
                                                        c89jpeg_u32 out_capacity,
                                                        c89jpeg_u32 *out_size)
{
    c89jpeg_mem_dest dest;
    c89jpeg_status st;
    if (enc == 0 || out_buf == 0) return C89JPEG_ERR_BAD_ARG;
    c89jpeg_mem_dest_init(&dest, out_buf, out_capacity);
    st = c89jpeg_encode_abbreviated_source_sink(enc, params, tables, c89jpeg_mem_dest_write, &dest, out_size);
    if (st == C89JPEG_OK && out_size != 0) *out_size = dest.size;
    return st;
}

static c89jpeg_status c89jpeg_cursor_read_u8(c89jpeg_parse_cursor *cur, c89jpeg_u8 *v)
{
    if (cur->ptr >= cur->end) return C89JPEG_ERR_CORRUPT;
    *v = *cur->ptr++;
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_cursor_read_u16(c89jpeg_parse_cursor *cur, c89jpeg_u16 *v)
{
    if (cur->end - cur->ptr < 2) return C89JPEG_ERR_CORRUPT;
    *v = c89jpeg_read_be16(cur->ptr);
    cur->ptr += 2;
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_cursor_skip(c89jpeg_parse_cursor *cur, c89jpeg_u32 n)
{
    if ((c89jpeg_u32)(cur->end - cur->ptr) < n) return C89JPEG_ERR_CORRUPT;
    cur->ptr += n;
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_next_marker(c89jpeg_parse_cursor *cur, c89jpeg_u8 *marker)
{
    c89jpeg_u8 c;
    do {
        if (cur->ptr >= cur->end) return C89JPEG_ERR_CORRUPT;
        c = *cur->ptr++;
    } while (c != 0xFF);
    do {
        if (cur->ptr >= cur->end) return C89JPEG_ERR_CORRUPT;
        c = *cur->ptr++;
    } while (c == 0xFF);
    if (c == 0x00) return C89JPEG_ERR_CORRUPT;
    *marker = c;
    return C89JPEG_OK;
}

static int c89jpeg_is_sof(c89jpeg_u8 marker)
{
    switch (marker) {
    case C89JPEG_MARKER_SOF0:
    case C89JPEG_MARKER_SOF1:
    case C89JPEG_MARKER_SOF2:
    case C89JPEG_MARKER_SOF3:
    case C89JPEG_MARKER_SOF5:
    case C89JPEG_MARKER_SOF6:
    case C89JPEG_MARKER_SOF7:
    case C89JPEG_MARKER_SOF9:
    case C89JPEG_MARKER_SOF10:
    case C89JPEG_MARKER_SOF11:
    case C89JPEG_MARKER_SOF13:
    case C89JPEG_MARKER_SOF14:
    case C89JPEG_MARKER_SOF15:
        return C89JPEG_TRUE;
    default:
        return C89JPEG_FALSE;
    }
}

static c89jpeg_status c89jpeg_parse_dqt(c89jpeg_decoder *dec, c89jpeg_parse_cursor *cur, c89jpeg_u16 seglen)
{
    c89jpeg_u16 remaining;
    remaining = (c89jpeg_u16)(seglen - 2);
    while (remaining > 0) {
        c89jpeg_u8 pq_tq;
        c89jpeg_u8 pq;
        c89jpeg_u8 tq;
        int i;
        if (remaining < 65) return C89JPEG_ERR_CORRUPT;
        C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u8(cur, &pq_tq));
        --remaining;
        pq = (c89jpeg_u8)(pq_tq >> 4);
        tq = (c89jpeg_u8)(pq_tq & 0x0F);
        if (pq != 0 || tq >= C89JPEG_MAX_QUANT_TABLES) return C89JPEG_ERR_UNSUPPORTED;
        if (remaining < 64) return C89JPEG_ERR_CORRUPT;
        for (i = 0; i < 64; ++i) {
            c89jpeg_u8 q;
            C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u8(cur, &q));
            dec->quant[tq][c89jpeg_zigzag[i]] = q;
        }
        dec->quant_present[tq] = 1;
        remaining = (c89jpeg_u16)(remaining - 64);
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_parse_dht(c89jpeg_decoder *dec, c89jpeg_parse_cursor *cur, c89jpeg_u16 seglen)
{
    c89jpeg_u16 remaining;
    remaining = (c89jpeg_u16)(seglen - 2);
    while (remaining > 0) {
        c89jpeg_u8 tc_th;
        c89jpeg_u8 bits[16];
        c89jpeg_u8 vals[256];
        c89jpeg_u16 count;
        c89jpeg_u8 tc;
        c89jpeg_u8 th;
        int i;
        if (remaining < 17) return C89JPEG_ERR_CORRUPT;
        C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u8(cur, &tc_th));
        --remaining;
        tc = (c89jpeg_u8)(tc_th >> 4);
        th = (c89jpeg_u8)(tc_th & 0x0F);
        if (tc >= C89JPEG_MAX_HUFF_CLASSES || th >= C89JPEG_MAX_HUFF_TABLES) return C89JPEG_ERR_UNSUPPORTED;
        count = 0;
        for (i = 0; i < 16; ++i) {
            C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u8(cur, &bits[i]));
            count = (c89jpeg_u16)(count + bits[i]);
        }
        remaining = (c89jpeg_u16)(remaining - 16);
        if (remaining < count) return C89JPEG_ERR_CORRUPT;
        for (i = 0; i < (int)count; ++i) {
            C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u8(cur, &vals[i]));
        }
        remaining = (c89jpeg_u16)(remaining - count);
        C89JPEG_EMIT_CHECK(c89jpeg_build_huffman(&dec->huff[tc][th], bits, vals, count));
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_parse_sof(c89jpeg_decoder *dec, c89jpeg_parse_cursor *cur, c89jpeg_u16 seglen, c89jpeg_u8 marker)
{
    c89jpeg_u8 precision;
    c89jpeg_u16 height;
    c89jpeg_u16 width;
    c89jpeg_u8 components;
    int i;
    int blocks_per_mcu;
    c89jpeg_u8 max_h;
    c89jpeg_u8 max_v;
    if (seglen < 8) return C89JPEG_ERR_CORRUPT;
    C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u8(cur, &precision));
    C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u16(cur, &height));
    C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u16(cur, &width));
    C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u8(cur, &components));
    if (precision != 8) return C89JPEG_ERR_UNSUPPORTED;
    if (components != 1 && components != 3) return C89JPEG_ERR_UNSUPPORTED;
    dec->info.width = width;
    dec->info.height = height;
    dec->info.precision = precision;
    dec->info.components = components;
    dec->info.progressive = (marker == C89JPEG_MARKER_SOF2) ? 1 : 0;
    dec->info.arithmetic = (marker == C89JPEG_MARKER_SOF9 || marker == C89JPEG_MARKER_SOF10 || marker == C89JPEG_MARKER_SOF11 || marker == C89JPEG_MARKER_SOF13 || marker == C89JPEG_MARKER_SOF14 || marker == C89JPEG_MARKER_SOF15) ? 1 : 0;
    if (marker != C89JPEG_MARKER_SOF0 && marker != C89JPEG_MARKER_SOF2) return C89JPEG_ERR_UNSUPPORTED;

    max_h = 0;
    max_v = 0;
    blocks_per_mcu = 0;
    for (i = 0; i < components; ++i) {
        c89jpeg_u8 id;
        c89jpeg_u8 hv;
        c89jpeg_u8 tq;
        C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u8(cur, &id));
        C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u8(cur, &hv));
        C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u8(cur, &tq));
        dec->comp[i].id = id;
        dec->comp[i].h_samp = (c89jpeg_u8)(hv >> 4);
        dec->comp[i].v_samp = (c89jpeg_u8)(hv & 0x0F);
        dec->comp[i].tq = tq;
        dec->comp[i].pred = 0;
        dec->comp[i].scan_index = (c89jpeg_u8)i;
        if (dec->comp[i].h_samp == 0 || dec->comp[i].v_samp == 0) return C89JPEG_ERR_CORRUPT;
        if (tq >= C89JPEG_MAX_QUANT_TABLES) return C89JPEG_ERR_UNSUPPORTED;
        if (dec->comp[i].h_samp > max_h) max_h = dec->comp[i].h_samp;
        if (dec->comp[i].v_samp > max_v) max_v = dec->comp[i].v_samp;
        dec->info.component[i].id = id;
        dec->info.component[i].h_samp = dec->comp[i].h_samp;
        dec->info.component[i].v_samp = dec->comp[i].v_samp;
        dec->info.component[i].quant_table = tq;
        blocks_per_mcu += dec->comp[i].h_samp * dec->comp[i].v_samp;
    }
    if (blocks_per_mcu > C89JPEG_MAX_BLOCKS_PER_MCU) return C89JPEG_ERR_LIMIT;
    dec->info.max_h_samp = max_h;
    dec->info.max_v_samp = max_v;
    if (max_h == 0 || max_v == 0) return C89JPEG_ERR_CORRUPT;
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_parse_app0(c89jpeg_decoder *dec, c89jpeg_parse_cursor *cur, c89jpeg_u16 seglen)
{
    c89jpeg_u8 tag[5];
    c89jpeg_u8 units;
    c89jpeg_u16 xden;
    c89jpeg_u16 yden;
    if (seglen < 16) return c89jpeg_cursor_skip(cur, seglen - 2);
    C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u8(cur, &tag[0]));
    C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u8(cur, &tag[1]));
    C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u8(cur, &tag[2]));
    C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u8(cur, &tag[3]));
    C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u8(cur, &tag[4]));
    if (tag[0] == 'J' && tag[1] == 'F' && tag[2] == 'I' && tag[3] == 'F' && tag[4] == 0) {
        dec->info.is_jfif = 1;
        C89JPEG_EMIT_CHECK(c89jpeg_cursor_skip(cur, 2));
        C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u8(cur, &units));
        C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u16(cur, &xden));
        C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u16(cur, &yden));
        dec->info.density_units = units;
        dec->info.density_x = xden;
        dec->info.density_y = yden;
        return c89jpeg_cursor_skip(cur, (c89jpeg_u32)(seglen - 2 - 5 - 2 - 1 - 2 - 2));
    }
    return c89jpeg_cursor_skip(cur, seglen - 2 - 5);
}

static c89jpeg_status c89jpeg_parse_sos(c89jpeg_decoder *dec, c89jpeg_parse_cursor *cur, c89jpeg_u16 seglen)
{
    c89jpeg_u8 ns;
    c89jpeg_u8 ss;
    c89jpeg_u8 se;
    c89jpeg_u8 ahal;
    int i;
    if (seglen < 6) return C89JPEG_ERR_CORRUPT;
    C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u8(cur, &ns));
    if (ns == 0 || ns > dec->info.components || ns > C89JPEG_MAX_COMPONENTS) return C89JPEG_ERR_UNSUPPORTED;
    dec->scan_count = ns;
    for (i = 0; i < ns; ++i) {
        c89jpeg_u8 csj;
        c89jpeg_u8 tdta;
        int found;
        int c;
        C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u8(cur, &csj));
        C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u8(cur, &tdta));
        found = C89JPEG_FALSE;
        for (c = 0; c < dec->info.components; ++c) {
            if (dec->comp[c].id == csj) {
                dec->scan_comp[i] = &dec->comp[c];
                dec->comp[c].scan_index = (c89jpeg_u8)i;
                dec->comp[c].dc_table = (c89jpeg_u8)(tdta >> 4);
                dec->comp[c].ac_table = (c89jpeg_u8)(tdta & 0x0F);
                dec->info.component[c].dc_table = dec->comp[c].dc_table;
                dec->info.component[c].ac_table = dec->comp[c].ac_table;
                found = C89JPEG_TRUE;
                break;
            }
        }
        if (!found) return C89JPEG_ERR_CORRUPT;
    }
    C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u8(cur, &ss));
    C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u8(cur, &se));
    C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u8(cur, &ahal));
    dec->scan_ss = ss;
    dec->scan_se = se;
    dec->scan_ah = (c89jpeg_u8)(ahal >> 4);
    dec->scan_al = (c89jpeg_u8)(ahal & 0x0F);
    if (dec->info.progressive) {
        if (ss == 0) {
            if (se != 0) return C89JPEG_ERR_CORRUPT;
        } else {
            if (ss > se || se >= 64 || ns != 1) return C89JPEG_ERR_CORRUPT;
        }
        if (dec->scan_ah != 0 && dec->scan_al != (c89jpeg_u8)(dec->scan_ah - 1)) return C89JPEG_ERR_CORRUPT;
        if (dec->scan_al > 13) return C89JPEG_ERR_UNSUPPORTED;
    } else {
        if (ss != 0 || se != 63 || ahal != 0) return C89JPEG_ERR_UNSUPPORTED;
        if (dec->info.components == 3 && ns != 3) return C89JPEG_ERR_UNSUPPORTED;
    }
    dec->scan_started = 1;
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_finalize_dimensions(c89jpeg_decoder *dec)
{
    c89jpeg_u16 mcu_cols;
    c89jpeg_u16 mcu_rows;
    int i;
    if (dec->info.width == 0 || dec->info.height == 0) return C89JPEG_ERR_CORRUPT;
    mcu_cols = (c89jpeg_u16)((dec->info.width + dec->info.max_h_samp * 8 - 1) / (dec->info.max_h_samp * 8));
    mcu_rows = (c89jpeg_u16)((dec->info.height + dec->info.max_v_samp * 8 - 1) / (dec->info.max_v_samp * 8));
    for (i = 0; i < dec->info.components; ++i) {
        dec->comp[i].width_in_blocks = (c89jpeg_u16)(mcu_cols * dec->comp[i].h_samp);
        dec->comp[i].height_in_blocks = (c89jpeg_u16)(mcu_rows * dec->comp[i].v_samp);
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_parse_headers_ex(c89jpeg_decoder *dec, const c89jpeg_u8 *data, c89jpeg_u32 size, const c89jpeg_tables *tables, int require_sos)
{
    c89jpeg_parse_cursor cur;
    c89jpeg_u8 marker;
    c89jpeg_u16 seglen;
    if (dec == 0 || data == 0) return C89JPEG_ERR_BAD_ARG;
    c89jpeg_decoder_clear_frame_state(dec);
    if (tables != 0) c89jpeg_decoder_install_tables(dec, tables);
    cur.ptr = data;
    cur.end = data + size;
    if (size < 2 || data[0] != 0xFF || data[1] != C89JPEG_MARKER_SOI) return C89JPEG_ERR_CORRUPT;
    cur.ptr += 2;
    while (cur.ptr < cur.end) {
        C89JPEG_EMIT_CHECK(c89jpeg_next_marker(&cur, &marker));
        if (marker == C89JPEG_MARKER_EOI) {
            if (require_sos) return C89JPEG_ERR_CORRUPT;
            break;
        }
        if (marker == C89JPEG_MARKER_SOS) {
            C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u16(&cur, &seglen));
            C89JPEG_EMIT_CHECK(c89jpeg_parse_sos(dec, &cur, seglen));
            dec->entropy_offset = (c89jpeg_u32)(cur.ptr - data);
            break;
        }
        if (marker >= C89JPEG_MARKER_RST0 && marker <= C89JPEG_MARKER_RST7) return C89JPEG_ERR_CORRUPT;
        C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u16(&cur, &seglen));
        if (seglen < 2) return C89JPEG_ERR_CORRUPT;
        if (c89jpeg_is_sof(marker)) {
            C89JPEG_EMIT_CHECK(c89jpeg_parse_sof(dec, &cur, seglen, marker));
        } else if (marker == C89JPEG_MARKER_DQT) {
            C89JPEG_EMIT_CHECK(c89jpeg_parse_dqt(dec, &cur, seglen));
        } else if (marker == C89JPEG_MARKER_DHT) {
            C89JPEG_EMIT_CHECK(c89jpeg_parse_dht(dec, &cur, seglen));
        } else if (marker == C89JPEG_MARKER_DRI) {
            C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u16(&cur, &dec->info.restart_interval));
            if (seglen > 4) C89JPEG_EMIT_CHECK(c89jpeg_cursor_skip(&cur, seglen - 4));
        } else if (marker == C89JPEG_MARKER_APP0) {
            C89JPEG_EMIT_CHECK(c89jpeg_parse_app0(dec, &cur, seglen));
        } else {
            C89JPEG_EMIT_CHECK(c89jpeg_cursor_skip(&cur, seglen - 2));
        }
    }
    if (require_sos && !dec->scan_started) return C89JPEG_ERR_CORRUPT;
    if (dec->info.width != 0 || dec->scan_started) {
        C89JPEG_EMIT_CHECK(c89jpeg_finalize_dimensions(dec));
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_parse_headers(c89jpeg_decoder *dec, const c89jpeg_u8 *data, c89jpeg_u32 size)
{
    return c89jpeg_parse_headers_ex(dec, data, size, 0, C89JPEG_TRUE);
}

c89jpeg_status c89jpeg_tables_load(c89jpeg_tables *tables, const c89jpeg_u8 *data, c89jpeg_u32 size)
{
    c89jpeg_decoder dec;
    c89jpeg_status st;
    if (tables == 0 || data == 0) return C89JPEG_ERR_BAD_ARG;
    c89jpeg_decoder_init(&dec);
    st = c89jpeg_parse_headers_ex(&dec, data, size, 0, C89JPEG_FALSE);
    if (st != C89JPEG_OK) return st;
    if (dec.info.width != 0 || dec.scan_started) return C89JPEG_ERR_BAD_ARG;
    c89jpeg_tables_copy_from_decoder(tables, &dec);
    return C89JPEG_OK;
}

c89jpeg_status c89jpeg_tables_extract(c89jpeg_tables *tables, const c89jpeg_u8 *data, c89jpeg_u32 size)
{
    c89jpeg_decoder dec;
    c89jpeg_status st;
    if (tables == 0 || data == 0) return C89JPEG_ERR_BAD_ARG;
    c89jpeg_decoder_init(&dec);
    st = c89jpeg_parse_headers_ex(&dec, data, size, 0, C89JPEG_FALSE);
    if (st != C89JPEG_OK) return st;
    c89jpeg_tables_copy_from_decoder(tables, &dec);
    return C89JPEG_OK;
}

c89jpeg_status c89jpeg_probe(c89jpeg_decoder *dec, const c89jpeg_u8 *data, c89jpeg_u32 size, c89jpeg_image_info *out_info)
{
    c89jpeg_status st;
    if (dec == 0 || data == 0) return C89JPEG_ERR_BAD_ARG;
    st = c89jpeg_parse_headers_ex(dec, data, size, 0, C89JPEG_TRUE);
    dec->last_error = st;
    if (st != C89JPEG_OK) return st;
    if (out_info != 0) *out_info = dec->info;
    return C89JPEG_OK;
}

c89jpeg_status c89jpeg_probe_abbreviated(c89jpeg_decoder *dec, const c89jpeg_tables *tables, const c89jpeg_u8 *data, c89jpeg_u32 size, c89jpeg_image_info *out_info)
{
    c89jpeg_status st;
    if (dec == 0 || tables == 0 || data == 0) return C89JPEG_ERR_BAD_ARG;
    st = c89jpeg_parse_headers_ex(dec, data, size, tables, C89JPEG_TRUE);
    dec->last_error = st;
    if (st != C89JPEG_OK) return st;
    if (out_info != 0) *out_info = dec->info;
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_decode_block(c89jpeg_decoder *dec, c89jpeg_bitreader *br, c89jpeg_component *comp, c89jpeg_i32 *coeff)
{
    c89jpeg_status st;
    c89jpeg_u8 sym;
    c89jpeg_i32 diff;
    int k;
    int idx;
    const c89jpeg_huffman_table *dc_tab;
    const c89jpeg_huffman_table *ac_tab;
    if (comp->dc_table >= C89JPEG_MAX_HUFF_TABLES || comp->ac_table >= C89JPEG_MAX_HUFF_TABLES) return C89JPEG_ERR_UNSUPPORTED;
    dc_tab = &dec->huff[0][comp->dc_table];
    ac_tab = &dec->huff[1][comp->ac_table];
    if (!dc_tab->present || !ac_tab->present) return C89JPEG_ERR_CORRUPT;
    memset(coeff, 0, 64 * sizeof(coeff[0]));
    st = c89jpeg_huff_decode_symbol(br, dc_tab, &sym);
    if (st != C89JPEG_OK) return st;
    diff = c89jpeg_receive_extend(br, sym, &st);
    if (st != C89JPEG_OK) return st;
    comp->pred += diff;
    coeff[0] = comp->pred;
    k = 1;
    while (k < 64) {
        st = c89jpeg_huff_decode_symbol(br, ac_tab, &sym);
        if (st != C89JPEG_OK) return st;
        if (sym == 0) break;
        if (sym == 0xF0) {
            k += 16;
            continue;
        }
        k += (sym >> 4);
        if (k >= 64) return C89JPEG_ERR_CORRUPT;
        idx = c89jpeg_zigzag[k];
        coeff[idx] = c89jpeg_receive_extend(br, sym & 0x0F, &st);
        if (st != C89JPEG_OK) return st;
        ++k;
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_process_restart(c89jpeg_decoder *dec, c89jpeg_bitreader *br, c89jpeg_u8 *next_rst)
{
    int i;
    if (br->unread_marker == 0) {
        c89jpeg_parse_cursor cur;
        c89jpeg_u8 marker;
        cur.ptr = br->ptr;
        cur.end = br->end;
        C89JPEG_EMIT_CHECK(c89jpeg_next_marker(&cur, &marker));
        br->ptr = cur.ptr;
        br->unread_marker = marker;
    }
    if (br->unread_marker != (int)(C89JPEG_MARKER_RST0 + *next_rst)) return C89JPEG_ERR_CORRUPT;
    br->bits = 0;
    br->acc = 0;
    br->unread_marker = 0;
    *next_rst = (c89jpeg_u8)((*next_rst + 1) & 7);
    for (i = 0; i < dec->info.components; ++i) dec->comp[i].pred = 0;
    return C89JPEG_OK;
}

static c89jpeg_u8 c89jpeg_output_channels(c89jpeg_decode_format fmt, c89jpeg_u8 source_components)
{
    if (fmt == C89JPEG_DECODE_NATIVE) {
        return (source_components == 1) ? 1 : 3;
    }
    return (c89jpeg_u8)fmt;
}

static c89jpeg_status c89jpeg_write_pixel(c89jpeg_u8 *out_row, c89jpeg_decode_format fmt, c89jpeg_u8 source_components, c89jpeg_u8 yv, c89jpeg_u8 cbv, c89jpeg_u8 crv)
{
    if (fmt == C89JPEG_DECODE_NATIVE) {
        if (source_components == 1) {
            out_row[0] = yv;
        } else {
            c89jpeg_u8 r, g, b;
            c89jpeg_ycbcr_to_rgb(yv, cbv, crv, &r, &g, &b);
            out_row[0] = r;
            out_row[1] = g;
            out_row[2] = b;
        }
    } else if (fmt == C89JPEG_DECODE_GRAY8) {
        out_row[0] = yv;
    } else {
        c89jpeg_u8 r, g, b;
        if (source_components == 1) {
            r = yv;
            g = yv;
            b = yv;
        } else {
            c89jpeg_ycbcr_to_rgb(yv, cbv, crv, &r, &g, &b);
        }
        out_row[0] = r;
        out_row[1] = g;
        out_row[2] = b;
    }
    return C89JPEG_OK;
}

static int c89jpeg_component_block_base(const c89jpeg_decoder *dec, int comp_index)
{
    const c89jpeg_component *comp;
    int base_index;
    int c;
    comp = &dec->comp[comp_index];
    base_index = 0;
    for (c = 0; c < dec->info.components; ++c) {
        if (dec->comp[c].scan_index < comp->scan_index) {
            base_index += dec->comp[c].h_samp * dec->comp[c].v_samp;
        }
    }
    return base_index;
}

static c89jpeg_u8 c89jpeg_fetch_component_sample_raw(const c89jpeg_decoder *dec, int comp_index, int sx, int sy)
{
    const c89jpeg_component *comp;
    int comp_w;
    int comp_h;
    int block_x;
    int block_y;
    int local_x;
    int local_y;
    int block_index;
    int base_index;
    comp = &dec->comp[comp_index];
    comp_w = comp->h_samp * 8;
    comp_h = comp->v_samp * 8;
    if (sx < 0) sx = 0;
    if (sy < 0) sy = 0;
    if (sx >= comp_w) sx = comp_w - 1;
    if (sy >= comp_h) sy = comp_h - 1;
    block_x = sx / 8;
    block_y = sy / 8;
    local_x = sx & 7;
    local_y = sy & 7;
    base_index = c89jpeg_component_block_base(dec, comp_index);
    block_index = base_index + block_y * comp->h_samp + block_x;
    return dec->samples[block_index][local_y * 8 + local_x];
}

static c89jpeg_i32 c89jpeg_map_output_to_component_q8(int x, int comp_dim, int full_dim)
{
    c89jpeg_i32 num;
    if (comp_dim <= 0 || full_dim <= 0) return 0;
    num = (c89jpeg_i32)(2 * x + 1) * (c89jpeg_i32)comp_dim * 256;
    num /= (c89jpeg_i32)(2 * full_dim);
    return num - 128;
}

static c89jpeg_u8 c89jpeg_fetch_component_sample_nearest(const c89jpeg_decoder *dec, int comp_index, int x, int y)
{
    const c89jpeg_component *comp;
    int comp_w;
    int comp_h;
    int sx;
    int sy;
    comp = &dec->comp[comp_index];
    comp_w = comp->h_samp * 8;
    comp_h = comp->v_samp * 8;
    sx = (x * comp_w) / (dec->info.max_h_samp * 8);
    sy = (y * comp_h) / (dec->info.max_v_samp * 8);
    return c89jpeg_fetch_component_sample_raw(dec, comp_index, sx, sy);
}

static c89jpeg_u8 c89jpeg_fetch_component_sample_linear(const c89jpeg_decoder *dec, int comp_index, int x, int y)
{
    const c89jpeg_component *comp;
    int comp_w;
    int comp_h;
    c89jpeg_i32 sx_q8;
    c89jpeg_i32 sy_q8;
    int sx0;
    int sy0;
    int sx1;
    int sy1;
    c89jpeg_i32 fx;
    c89jpeg_i32 fy;
    c89jpeg_i32 a;
    c89jpeg_i32 b;
    c89jpeg_i32 c;
    c89jpeg_i32 d;
    c89jpeg_i32 top;
    c89jpeg_i32 bottom;
    comp = &dec->comp[comp_index];
    comp_w = comp->h_samp * 8;
    comp_h = comp->v_samp * 8;

    sx_q8 = c89jpeg_map_output_to_component_q8(x, comp_w, dec->info.max_h_samp * 8);
    sy_q8 = c89jpeg_map_output_to_component_q8(y, comp_h, dec->info.max_v_samp * 8);

    sx0 = (int)(sx_q8 >> 8);
    sy0 = (int)(sy_q8 >> 8);
    fx = sx_q8 & 255;
    fy = sy_q8 & 255;

    if (sx0 < 0) {
        sx0 = 0;
        fx = 0;
    }
    if (sy0 < 0) {
        sy0 = 0;
        fy = 0;
    }
    if (sx0 >= comp_w - 1) {
        sx0 = comp_w - 1;
        fx = 0;
    }
    if (sy0 >= comp_h - 1) {
        sy0 = comp_h - 1;
        fy = 0;
    }

    sx1 = (sx0 < comp_w - 1) ? (sx0 + 1) : sx0;
    sy1 = (sy0 < comp_h - 1) ? (sy0 + 1) : sy0;

    a = c89jpeg_fetch_component_sample_raw(dec, comp_index, sx0, sy0);
    b = c89jpeg_fetch_component_sample_raw(dec, comp_index, sx1, sy0);
    c = c89jpeg_fetch_component_sample_raw(dec, comp_index, sx0, sy1);
    d = c89jpeg_fetch_component_sample_raw(dec, comp_index, sx1, sy1);

    top = a * (256 - fx) + b * fx;
    bottom = c * (256 - fx) + d * fx;
    return (c89jpeg_u8)((top * (256 - fy) + bottom * fy + 32768L) >> 16);
}

static c89jpeg_u8 c89jpeg_fetch_component_sample_upsampled(const c89jpeg_decoder *dec, int comp_index, int x, int y, c89jpeg_upsampling_mode upsampling)
{
    if (upsampling == C89JPEG_UPSAMPLE_LINEAR) {
        return c89jpeg_fetch_component_sample_linear(dec, comp_index, x, y);
    }
    return c89jpeg_fetch_component_sample_nearest(dec, comp_index, x, y);
}

static int c89jpeg_mcu_intersects_region(c89jpeg_u16 mx,
                                         c89jpeg_u16 my,
                                         c89jpeg_u16 mcu_w,
                                         c89jpeg_u16 mcu_h,
                                         const c89jpeg_rect *region)
{
    c89jpeg_u32 gx0;
    c89jpeg_u32 gy0;
    c89jpeg_u32 gx1;
    c89jpeg_u32 gy1;
    c89jpeg_u32 rx1;
    c89jpeg_u32 ry1;
    gx0 = (c89jpeg_u32)mx * mcu_w;
    gy0 = (c89jpeg_u32)my * mcu_h;
    gx1 = gx0 + mcu_w;
    gy1 = gy0 + mcu_h;
    rx1 = (c89jpeg_u32)region->x + region->width;
    ry1 = (c89jpeg_u32)region->y + region->height;
    if (gx0 >= rx1 || gy0 >= ry1) return C89JPEG_FALSE;
    if (gx1 <= region->x || gy1 <= region->y) return C89JPEG_FALSE;
    return C89JPEG_TRUE;
}

static c89jpeg_status c89jpeg_render_mcu_region(const c89jpeg_decoder *dec,
                                                c89jpeg_u16 mx,
                                                c89jpeg_u16 my,
                                                c89jpeg_u16 mcu_w,
                                                c89jpeg_u16 mcu_h,
                                                c89jpeg_decode_format fmt,
                                                c89jpeg_upsampling_mode upsampling,
                                                c89jpeg_u8 out_channels,
                                                c89jpeg_u8 *target,
                                                c89jpeg_u32 target_stride,
                                                const c89jpeg_rect *region,
                                                int strip_mode)
{
    c89jpeg_u32 global_x0;
    c89jpeg_u32 global_y0;
    c89jpeg_u32 global_x1;
    c89jpeg_u32 global_y1;
    c89jpeg_u32 clip_x0;
    c89jpeg_u32 clip_y0;
    c89jpeg_u32 clip_x1;
    c89jpeg_u32 clip_y1;
    c89jpeg_u32 gx;
    c89jpeg_u32 gy;

    global_x0 = (c89jpeg_u32)mx * mcu_w;
    global_y0 = (c89jpeg_u32)my * mcu_h;
    global_x1 = global_x0 + mcu_w;
    global_y1 = global_y0 + mcu_h;
    if (global_x1 > dec->info.width) global_x1 = dec->info.width;
    if (global_y1 > dec->info.height) global_y1 = dec->info.height;

    clip_x0 = (global_x0 > region->x) ? global_x0 : region->x;
    clip_y0 = (global_y0 > region->y) ? global_y0 : region->y;
    clip_x1 = (global_x1 < (c89jpeg_u32)region->x + region->width) ? global_x1 : (c89jpeg_u32)region->x + region->width;
    clip_y1 = (global_y1 < (c89jpeg_u32)region->y + region->height) ? global_y1 : (c89jpeg_u32)region->y + region->height;

    if (clip_x1 <= clip_x0 || clip_y1 <= clip_y0) return C89JPEG_OK;

    for (gy = clip_y0; gy < clip_y1; ++gy) {
        c89jpeg_u8 *row;
        c89jpeg_u32 row_index;
        row_index = strip_mode ? (gy - global_y0) : (gy - region->y);
        row = target + row_index * target_stride;
        for (gx = clip_x0; gx < clip_x1; ++gx) {
            c89jpeg_u32 local_x;
            c89jpeg_u32 local_y;
            c89jpeg_u8 yv;
            c89jpeg_u8 cbv;
            c89jpeg_u8 crv;
            c89jpeg_u8 *dst;
            local_x = gx - global_x0;
            local_y = gy - global_y0;
            dst = row + (gx - region->x) * out_channels;
            yv = c89jpeg_fetch_component_sample_upsampled(dec, 0, (int)local_x, (int)local_y, upsampling);
            if (dec->info.components == 1 || fmt == C89JPEG_DECODE_GRAY8) {
                cbv = 128;
                crv = 128;
            } else {
                cbv = c89jpeg_fetch_component_sample_upsampled(dec, 1, (int)local_x, (int)local_y, upsampling);
                crv = c89jpeg_fetch_component_sample_upsampled(dec, 2, (int)local_x, (int)local_y, upsampling);
            }
            C89JPEG_EMIT_CHECK(c89jpeg_write_pixel(dst, fmt, dec->info.components, yv, cbv, crv));
        }
    }
    return C89JPEG_OK;
}

/* Progressive Huffman decode (SOF2).
 *
 * Progressive JPEG splits coefficients over multiple scans.  We therefore
 * accumulate quantized coefficients in caller-provided storage and run the
 * existing fixed-point IDCT only after EOI.  No heap allocation is used.
 */
static c89jpeg_u32 c89jpeg_progressive_component_base(const c89jpeg_decoder *dec, int comp_index)
{
    c89jpeg_u32 base;
    int i;
    base = 0;
    for (i = 0; i < comp_index; ++i) {
        base += (c89jpeg_u32)dec->comp[i].width_in_blocks *
                (c89jpeg_u32)dec->comp[i].height_in_blocks;
    }
    return base;
}

static c89jpeg_i16 *c89jpeg_progressive_block(c89jpeg_decoder *dec,
                                               c89jpeg_i16 *coeff_store,
                                               int comp_index,
                                               c89jpeg_u16 bx,
                                               c89jpeg_u16 by)
{
    c89jpeg_u32 base;
    c89jpeg_u32 block_index;
    base = c89jpeg_progressive_component_base(dec, comp_index);
    block_index = base + (c89jpeg_u32)by * dec->comp[comp_index].width_in_blocks + bx;
    return coeff_store + block_index * 64UL;
}

static int c89jpeg_progressive_comp_index(const c89jpeg_decoder *dec, const c89jpeg_component *comp)
{
    int i;
    for (i = 0; i < dec->info.components; ++i) {
        if (&dec->comp[i] == comp) return i;
    }
    return -1;
}

static c89jpeg_u16 c89jpeg_progressive_scan_block_cols(const c89jpeg_decoder *dec, int comp_index)
{
    c89jpeg_u32 num;
    c89jpeg_u32 den;
    num = (c89jpeg_u32)dec->info.width * dec->comp[comp_index].h_samp;
    den = (c89jpeg_u32)dec->info.max_h_samp * 8UL;
    return (c89jpeg_u16)((num + den - 1UL) / den);
}

static c89jpeg_u16 c89jpeg_progressive_scan_block_rows(const c89jpeg_decoder *dec, int comp_index)
{
    c89jpeg_u32 num;
    c89jpeg_u32 den;
    num = (c89jpeg_u32)dec->info.height * dec->comp[comp_index].v_samp;
    den = (c89jpeg_u32)dec->info.max_v_samp * 8UL;
    return (c89jpeg_u16)((num + den - 1UL) / den);
}

static c89jpeg_status c89jpeg_progressive_store_i16(c89jpeg_i16 *dst, c89jpeg_i32 value)
{
    if (value < -32768L || value > 32767L) return C89JPEG_ERR_LIMIT;
    *dst = (c89jpeg_i16)value;
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_progressive_dc_first(c89jpeg_decoder *dec,
                                                    c89jpeg_bitreader *br,
                                                    c89jpeg_component *comp,
                                                    c89jpeg_i16 *coeff)
{
    const c89jpeg_huffman_table *tab;
    c89jpeg_u8 sym;
    c89jpeg_status st;
    c89jpeg_i32 diff;
    c89jpeg_i32 value;
    c89jpeg_i32 scale;
    if (comp->dc_table >= C89JPEG_MAX_HUFF_TABLES) return C89JPEG_ERR_UNSUPPORTED;
    tab = &dec->huff[0][comp->dc_table];
    if (!tab->present) return C89JPEG_ERR_CORRUPT;
    st = c89jpeg_huff_decode_symbol(br, tab, &sym);
    if (st != C89JPEG_OK) return st;
    if (sym > 15) return C89JPEG_ERR_CORRUPT;
    diff = c89jpeg_receive_extend(br, sym, &st);
    if (st != C89JPEG_OK) return st;
    if ((comp->pred >= 0 && diff > 0x7FFFFFFFL - comp->pred) ||
        (comp->pred < 0 && diff < (-2147483647L - 1L) - comp->pred)) return C89JPEG_ERR_LIMIT;
    comp->pred += diff;
    scale = (c89jpeg_i32)1 << dec->scan_al;
    value = comp->pred * scale;
    return c89jpeg_progressive_store_i16(&coeff[0], value);
}

static c89jpeg_status c89jpeg_progressive_dc_refine(c89jpeg_decoder *dec,
                                                     c89jpeg_bitreader *br,
                                                     c89jpeg_i16 *coeff)
{
    int bit;
    c89jpeg_i32 value;
    c89jpeg_i32 p1;
    c89jpeg_status st;
    p1 = (c89jpeg_i32)1 << dec->scan_al;
    st = c89jpeg_br_get_bit(br, &bit);
    if (st != C89JPEG_OK) return st;
    if (bit) {
        value = (c89jpeg_i32)coeff[0];
        value |= p1;
        return c89jpeg_progressive_store_i16(&coeff[0], value);
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_progressive_ac_first(c89jpeg_decoder *dec,
                                                    c89jpeg_bitreader *br,
                                                    c89jpeg_component *comp,
                                                    c89jpeg_i16 *coeff,
                                                    c89jpeg_u32 *eobrun)
{
    const c89jpeg_huffman_table *tab;
    c89jpeg_status st;
    c89jpeg_u8 sym;
    c89jpeg_u32 bits;
    c89jpeg_i32 value;
    int k;
    int r;
    int s;
    int idx;
    if (*eobrun != 0) {
        --(*eobrun);
        return C89JPEG_OK;
    }
    if (comp->ac_table >= C89JPEG_MAX_HUFF_TABLES) return C89JPEG_ERR_UNSUPPORTED;
    tab = &dec->huff[1][comp->ac_table];
    if (!tab->present) return C89JPEG_ERR_CORRUPT;
    k = dec->scan_ss;
    while (k <= dec->scan_se) {
        st = c89jpeg_huff_decode_symbol(br, tab, &sym);
        if (st != C89JPEG_OK) return st;
        r = sym >> 4;
        s = sym & 15;
        if (s != 0) {
            k += r;
            if (k > dec->scan_se) return C89JPEG_ERR_CORRUPT;
            value = c89jpeg_receive_extend(br, s, &st);
            if (st != C89JPEG_OK) return st;
            value *= ((c89jpeg_i32)1 << dec->scan_al);
            idx = c89jpeg_zigzag[k];
            C89JPEG_EMIT_CHECK(c89jpeg_progressive_store_i16(&coeff[idx], value));
            ++k;
        } else if (r == 15) {
            k += 16;
        } else {
            *eobrun = (c89jpeg_u32)1 << r;
            if (r != 0) {
                st = c89jpeg_br_get_bits(br, r, &bits);
                if (st != C89JPEG_OK) return st;
                *eobrun += bits;
            }
            --(*eobrun);
            break;
        }
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_progressive_refine_existing(c89jpeg_bitreader *br,
                                                           c89jpeg_i16 *coef,
                                                           c89jpeg_i32 p1,
                                                           c89jpeg_i32 m1)
{
    int bit;
    c89jpeg_i32 value;
    c89jpeg_status st;
    st = c89jpeg_br_get_bit(br, &bit);
    if (st != C89JPEG_OK) return st;
    if (!bit) return C89JPEG_OK;
    value = (c89jpeg_i32)*coef;
    if ((value & p1) != 0) return C89JPEG_OK;
    if (value >= 0) value += p1;
    else value += m1;
    return c89jpeg_progressive_store_i16(coef, value);
}

static c89jpeg_status c89jpeg_progressive_ac_refine(c89jpeg_decoder *dec,
                                                     c89jpeg_bitreader *br,
                                                     c89jpeg_component *comp,
                                                     c89jpeg_i16 *coeff,
                                                     c89jpeg_u32 *eobrun)
{
    const c89jpeg_huffman_table *tab;
    c89jpeg_status st;
    c89jpeg_u8 sym;
    c89jpeg_u32 bits;
    c89jpeg_i32 p1;
    c89jpeg_i32 m1;
    c89jpeg_i32 newcoef;
    int k;
    int r;
    int s;
    int idx;
    p1 = (c89jpeg_i32)1 << dec->scan_al;
    m1 = -p1;
    if (comp->ac_table >= C89JPEG_MAX_HUFF_TABLES) return C89JPEG_ERR_UNSUPPORTED;
    tab = &dec->huff[1][comp->ac_table];
    if (!tab->present) return C89JPEG_ERR_CORRUPT;
    k = dec->scan_ss;
    if (*eobrun == 0) {
        while (k <= dec->scan_se) {
            st = c89jpeg_huff_decode_symbol(br, tab, &sym);
            if (st != C89JPEG_OK) return st;
            r = sym >> 4;
            s = sym & 15;
            newcoef = 0;
            if (s != 0) {
                if (s != 1) return C89JPEG_ERR_CORRUPT;
                st = c89jpeg_br_get_bit(br, &s);
                if (st != C89JPEG_OK) return st;
                newcoef = s ? p1 : m1;
            } else if (r != 15) {
                *eobrun = (c89jpeg_u32)1 << r;
                if (r != 0) {
                    st = c89jpeg_br_get_bits(br, r, &bits);
                    if (st != C89JPEG_OK) return st;
                    *eobrun += bits;
                }
                break;
            }

            for (;;) {
                if (k > dec->scan_se) return C89JPEG_ERR_CORRUPT;
                idx = c89jpeg_zigzag[k];
                if (coeff[idx] != 0) {
                    st = c89jpeg_progressive_refine_existing(br, &coeff[idx], p1, m1);
                    if (st != C89JPEG_OK) return st;
                } else {
                    --r;
                    if (r < 0) break;
                }
                ++k;
            }
            if (newcoef != 0) {
                idx = c89jpeg_zigzag[k];
                C89JPEG_EMIT_CHECK(c89jpeg_progressive_store_i16(&coeff[idx], newcoef));
            }
            ++k;
        }
    }

    if (*eobrun != 0) {
        while (k <= dec->scan_se) {
            idx = c89jpeg_zigzag[k];
            if (coeff[idx] != 0) {
                st = c89jpeg_progressive_refine_existing(br, &coeff[idx], p1, m1);
                if (st != C89JPEG_OK) return st;
            }
            ++k;
        }
        --(*eobrun);
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_progressive_decode_one_block(c89jpeg_decoder *dec,
                                                            c89jpeg_bitreader *br,
                                                            c89jpeg_component *comp,
                                                            c89jpeg_i16 *coeff,
                                                            c89jpeg_u32 *eobrun)
{
    if (dec->scan_ss == 0) {
        if (dec->scan_ah == 0) return c89jpeg_progressive_dc_first(dec, br, comp, coeff);
        return c89jpeg_progressive_dc_refine(dec, br, coeff);
    }
    if (dec->scan_ah == 0) return c89jpeg_progressive_ac_first(dec, br, comp, coeff, eobrun);
    return c89jpeg_progressive_ac_refine(dec, br, comp, coeff, eobrun);
}

static c89jpeg_status c89jpeg_progressive_next_marker(c89jpeg_bitreader *br,
                                                       c89jpeg_u8 *marker,
                                                       const c89jpeg_u8 **after_marker)
{
    c89jpeg_parse_cursor cur;
    if (br->unread_marker != 0) {
        *marker = (c89jpeg_u8)br->unread_marker;
        *after_marker = br->ptr;
        return C89JPEG_OK;
    }
    cur.ptr = br->ptr;
    cur.end = br->end;
    C89JPEG_EMIT_CHECK(c89jpeg_next_marker(&cur, marker));
    *after_marker = cur.ptr;
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_progressive_decode_scan(c89jpeg_decoder *dec,
                                                       const c89jpeg_u8 *entropy,
                                                       c89jpeg_u32 entropy_size,
                                                       c89jpeg_i16 *coeff_store,
                                                       c89jpeg_u8 *out_marker,
                                                       const c89jpeg_u8 **out_after_marker)
{
    c89jpeg_bitreader br;
    c89jpeg_status st;
    c89jpeg_u16 mcu_cols;
    c89jpeg_u16 mcu_rows;
    c89jpeg_u16 mx;
    c89jpeg_u16 my;
    c89jpeg_u16 restart_left;
    c89jpeg_u8 next_rst;
    c89jpeg_u32 eobrun;
    c89jpeg_u32 mcu_index;
    c89jpeg_u32 total_mcus;
    int interleaved;
    int i;

    interleaved = (dec->scan_count > 1) ? C89JPEG_TRUE : C89JPEG_FALSE;
    if (dec->scan_ss != 0 && interleaved) return C89JPEG_ERR_CORRUPT;
    if (interleaved) {
        c89jpeg_u16 mcu_w;
        c89jpeg_u16 mcu_h;
        mcu_w = (c89jpeg_u16)(dec->info.max_h_samp * 8);
        mcu_h = (c89jpeg_u16)(dec->info.max_v_samp * 8);
        mcu_cols = (c89jpeg_u16)(((c89jpeg_u32)dec->info.width + mcu_w - 1UL) / mcu_w);
        mcu_rows = (c89jpeg_u16)(((c89jpeg_u32)dec->info.height + mcu_h - 1UL) / mcu_h);
    } else {
        int ci;
        ci = c89jpeg_progressive_comp_index(dec, dec->scan_comp[0]);
        if (ci < 0) return C89JPEG_ERR_CORRUPT;
        mcu_cols = c89jpeg_progressive_scan_block_cols(dec, ci);
        mcu_rows = c89jpeg_progressive_scan_block_rows(dec, ci);
    }
    total_mcus = (c89jpeg_u32)mcu_cols * mcu_rows;
    c89jpeg_br_init(&br, entropy, entropy_size);
    for (i = 0; i < dec->info.components; ++i) dec->comp[i].pred = 0;
    restart_left = dec->info.restart_interval;
    next_rst = 0;
    eobrun = 0;
    mcu_index = 0;

    for (my = 0; my < mcu_rows; ++my) {
        for (mx = 0; mx < mcu_cols; ++mx) {
            if (interleaved) {
                for (i = 0; i < dec->scan_count; ++i) {
                    c89jpeg_component *comp;
                    int ci;
                    c89jpeg_u8 bx;
                    c89jpeg_u8 by;
                    comp = dec->scan_comp[i];
                    ci = c89jpeg_progressive_comp_index(dec, comp);
                    if (ci < 0) return C89JPEG_ERR_CORRUPT;
                    for (by = 0; by < comp->v_samp; ++by) {
                        for (bx = 0; bx < comp->h_samp; ++bx) {
                            c89jpeg_u16 gbx;
                            c89jpeg_u16 gby;
                            c89jpeg_i16 *block;
                            gbx = (c89jpeg_u16)(mx * comp->h_samp + bx);
                            gby = (c89jpeg_u16)(my * comp->v_samp + by);
                            block = c89jpeg_progressive_block(dec, coeff_store, ci, gbx, gby);
                            st = c89jpeg_progressive_decode_one_block(dec, &br, comp, block, &eobrun);
                            if (st != C89JPEG_OK) return st;
                        }
                    }
                }
            } else {
                c89jpeg_component *comp;
                int ci;
                c89jpeg_i16 *block;
                comp = dec->scan_comp[0];
                ci = c89jpeg_progressive_comp_index(dec, comp);
                if (ci < 0) return C89JPEG_ERR_CORRUPT;
                block = c89jpeg_progressive_block(dec, coeff_store, ci, mx, my);
                st = c89jpeg_progressive_decode_one_block(dec, &br, comp, block, &eobrun);
                if (st != C89JPEG_OK) return st;
            }

            ++mcu_index;
            if (dec->info.restart_interval != 0) {
                if (restart_left == 0) restart_left = dec->info.restart_interval;
                --restart_left;
                if (restart_left == 0 && mcu_index < total_mcus) {
                    st = c89jpeg_process_restart(dec, &br, &next_rst);
                    if (st != C89JPEG_OK) return st;
                    eobrun = 0;
                    restart_left = dec->info.restart_interval;
                }
            }
        }
    }
    br.bits = 0;
    br.acc = 0;
    return c89jpeg_progressive_next_marker(&br, out_marker, out_after_marker);
}

static c89jpeg_status c89jpeg_progressive_parse_between_scans(c89jpeg_decoder *dec,
                                                               const c89jpeg_u8 *data,
                                                               const c89jpeg_u8 *end,
                                                               c89jpeg_u8 first_marker,
                                                               const c89jpeg_u8 **out_entropy,
                                                               int *out_eoi)
{
    c89jpeg_parse_cursor cur;
    c89jpeg_u8 marker;
    c89jpeg_u16 seglen;
    marker = first_marker;
    cur.ptr = data;
    cur.end = end;
    *out_eoi = C89JPEG_FALSE;
    for (;;) {
        if (marker == C89JPEG_MARKER_EOI) {
            *out_eoi = C89JPEG_TRUE;
            *out_entropy = cur.ptr;
            return C89JPEG_OK;
        }
        if (marker == C89JPEG_MARKER_SOS) {
            C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u16(&cur, &seglen));
            if (seglen < 6) return C89JPEG_ERR_CORRUPT;
            C89JPEG_EMIT_CHECK(c89jpeg_parse_sos(dec, &cur, seglen));
            *out_entropy = cur.ptr;
            return C89JPEG_OK;
        }
        if (marker >= C89JPEG_MARKER_RST0 && marker <= C89JPEG_MARKER_RST7) return C89JPEG_ERR_CORRUPT;
        if (marker == C89JPEG_MARKER_SOI || c89jpeg_is_sof(marker)) return C89JPEG_ERR_CORRUPT;
        C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u16(&cur, &seglen));
        if (seglen < 2 || (c89jpeg_u32)(cur.end - cur.ptr) < (c89jpeg_u32)(seglen - 2)) return C89JPEG_ERR_CORRUPT;
        if (marker == C89JPEG_MARKER_DHT) {
            C89JPEG_EMIT_CHECK(c89jpeg_parse_dht(dec, &cur, seglen));
        } else if (marker == C89JPEG_MARKER_DQT) {
            C89JPEG_EMIT_CHECK(c89jpeg_parse_dqt(dec, &cur, seglen));
        } else if (marker == C89JPEG_MARKER_DRI) {
            if (seglen < 4) return C89JPEG_ERR_CORRUPT;
            C89JPEG_EMIT_CHECK(c89jpeg_cursor_read_u16(&cur, &dec->info.restart_interval));
            if (seglen > 4) C89JPEG_EMIT_CHECK(c89jpeg_cursor_skip(&cur, seglen - 4));
        } else if (marker == C89JPEG_MARKER_APP0) {
            C89JPEG_EMIT_CHECK(c89jpeg_parse_app0(dec, &cur, seglen));
        } else {
            C89JPEG_EMIT_CHECK(c89jpeg_cursor_skip(&cur, seglen - 2));
        }
        C89JPEG_EMIT_CHECK(c89jpeg_next_marker(&cur, &marker));
    }
}

static c89jpeg_status c89jpeg_progressive_render(c89jpeg_decoder *dec,
                                                  c89jpeg_i16 *coeff_store,
                                                  c89jpeg_u8 *out_pixels,
                                                  c89jpeg_u32 out_capacity,
                                                  c89jpeg_u32 out_stride,
                                                  c89jpeg_decode_format out_format,
                                                  c89jpeg_upsampling_mode upsampling,
                                                  const c89jpeg_rect *requested_roi,
                                                  c89jpeg_roi_mode roi_mode,
                                                  c89jpeg_u8 *strip_buffer,
                                                  c89jpeg_u32 strip_buffer_size,
                                                  c89jpeg_row_sink_fn row_fn,
                                                  void *row_user)
{
    c89jpeg_rect region;
    c89jpeg_status st;
    c89jpeg_u16 mcu_w;
    c89jpeg_u16 mcu_h;
    c89jpeg_u16 mcu_cols;
    c89jpeg_u16 mcu_rows;
    c89jpeg_u16 mx;
    c89jpeg_u16 my;
    c89jpeg_u32 row_bytes;
    c89jpeg_u32 needed;
    c89jpeg_u8 out_channels;
    int sink_mode;
    int i;

    sink_mode = (row_fn != 0) ? C89JPEG_TRUE : C89JPEG_FALSE;
    for (i = 0; i < dec->info.components; ++i) dec->comp[i].scan_index = (c89jpeg_u8)i;
    mcu_w = (c89jpeg_u16)(dec->info.max_h_samp * 8);
    mcu_h = (c89jpeg_u16)(dec->info.max_v_samp * 8);
    mcu_cols = (c89jpeg_u16)(((c89jpeg_u32)dec->info.width + mcu_w - 1UL) / mcu_w);
    mcu_rows = (c89jpeg_u16)(((c89jpeg_u32)dec->info.height + mcu_h - 1UL) / mcu_h);
    C89JPEG_EMIT_CHECK(c89jpeg_resolve_roi(&dec->info, requested_roi, roi_mode, &region));
    out_channels = c89jpeg_output_channels(out_format, dec->info.components);
    row_bytes = c89jpeg_decoder_row_stride(region.width, out_format, dec->info.components);
    if (sink_mode) {
        if (strip_buffer == 0) return C89JPEG_ERR_BAD_ARG;
        if (!c89jpeg_mul_u32(row_bytes, (c89jpeg_u32)mcu_h, &needed)) return C89JPEG_ERR_LIMIT;
        if (strip_buffer_size < needed) return C89JPEG_ERR_SHORT_BUFFER;
    } else {
        if (out_pixels == 0) return C89JPEG_ERR_BAD_ARG;
        if (out_stride == 0) out_stride = row_bytes;
        if (!c89jpeg_mul_u32(out_stride, region.height, &needed)) return C89JPEG_ERR_LIMIT;
        if (out_capacity < needed) return C89JPEG_ERR_SHORT_BUFFER;
    }

    for (my = 0; my < mcu_rows; ++my) {
        c89jpeg_u32 row_start;
        c89jpeg_u32 row_end;
        int row_intersects;
        row_start = (c89jpeg_u32)my * mcu_h;
        row_end = row_start + mcu_h;
        row_intersects = (row_start < (c89jpeg_u32)region.y + region.height && row_end > region.y) ? C89JPEG_TRUE : C89JPEG_FALSE;
        if (sink_mode && row_intersects) memset(strip_buffer, 0, (size_t)needed);
        for (mx = 0; mx < mcu_cols; ++mx) {
            int render_this_mcu;
            int block_index;
            render_this_mcu = c89jpeg_mcu_intersects_region(mx, my, mcu_w, mcu_h, &region);
            if (!render_this_mcu) continue;
            block_index = 0;
            for (i = 0; i < dec->info.components; ++i) {
                c89jpeg_component *comp;
                c89jpeg_u8 bx;
                c89jpeg_u8 by;
                comp = &dec->comp[i];
                if (!dec->quant_present[comp->tq]) return C89JPEG_ERR_CORRUPT;
                for (by = 0; by < comp->v_samp; ++by) {
                    for (bx = 0; bx < comp->h_samp; ++bx) {
                        c89jpeg_u16 gbx;
                        c89jpeg_u16 gby;
                        c89jpeg_i16 *src_block;
                        int q;
                        gbx = (c89jpeg_u16)(mx * comp->h_samp + bx);
                        gby = (c89jpeg_u16)(my * comp->v_samp + by);
                        src_block = c89jpeg_progressive_block(dec, coeff_store, i, gbx, gby);
                        for (q = 0; q < 64; ++q) {
                            dec->blocks[block_index][q] = (c89jpeg_i32)src_block[q] * dec->quant[comp->tq][q];
                        }
                        c89jpeg_idct8x8(dec->blocks[block_index], dec->idct_tmp, dec->samples[block_index]);
                        ++block_index;
                    }
                }
            }
            st = c89jpeg_render_mcu_region(dec, mx, my, mcu_w, mcu_h,
                                           out_format, upsampling, out_channels,
                                           sink_mode ? strip_buffer : out_pixels,
                                           sink_mode ? row_bytes : out_stride,
                                           &region, sink_mode);
            if (st != C89JPEG_OK) return st;
        }
        if (sink_mode && row_intersects) {
            c89jpeg_u32 clip_y0;
            c89jpeg_u32 clip_y1;
            clip_y0 = (row_start > region.y) ? row_start : region.y;
            clip_y1 = (row_end < (c89jpeg_u32)region.y + region.height) ? row_end : (c89jpeg_u32)region.y + region.height;
            for (i = (int)clip_y0; i < (int)clip_y1; ++i) {
                c89jpeg_u32 row_index;
                c89jpeg_u32 local_row;
                row_index = (c89jpeg_u32)i - region.y;
                local_row = (c89jpeg_u32)i - row_start;
                if (!row_fn(row_user, row_index, strip_buffer + local_row * row_bytes, row_bytes)) return C89JPEG_ERR_IO;
            }
        }
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_decode_progressive_internal(c89jpeg_decoder *dec,
                                                           const c89jpeg_tables *tables,
                                                           const c89jpeg_u8 *data,
                                                           c89jpeg_u32 size,
                                                           c89jpeg_u8 *out_pixels,
                                                           c89jpeg_u32 out_capacity,
                                                           c89jpeg_u32 out_stride,
                                                           c89jpeg_decode_format out_format,
                                                           c89jpeg_upsampling_mode upsampling,
                                                           const c89jpeg_rect *requested_roi,
                                                           c89jpeg_roi_mode roi_mode,
                                                           c89jpeg_u8 *strip_buffer,
                                                           c89jpeg_u32 strip_buffer_size,
                                                           c89jpeg_row_sink_fn row_fn,
                                                           void *row_user,
                                                           void *workspace,
                                                           c89jpeg_u32 workspace_size,
                                                           c89jpeg_image_info *out_info)
{
    c89jpeg_status st;
    c89jpeg_u32 required;
    c89jpeg_i16 *coeff_store;
    const c89jpeg_u8 *entropy;
    const c89jpeg_u8 *after_marker;
    const c89jpeg_u8 *next_entropy;
    c89jpeg_u8 marker;
    c89jpeg_u32 scans;
    int eoi;

    st = c89jpeg_parse_headers_ex(dec, data, size, tables, C89JPEG_TRUE);
    if (st != C89JPEG_OK) return st;
    if (!dec->info.progressive) return C89JPEG_ERR_BAD_ARG;
    required = c89jpeg_decoder_progressive_workspace_size(&dec->info);
    if (required == 0) return C89JPEG_ERR_LIMIT;
    if (workspace == 0 || workspace_size < required) return C89JPEG_ERR_SHORT_BUFFER;
    coeff_store = (c89jpeg_i16 *)workspace;
    memset(coeff_store, 0, (size_t)required);
    entropy = data + dec->entropy_offset;
    scans = 0;
    eoi = C89JPEG_FALSE;

    while (!eoi) {
        if (++scans > C89JPEG_MAX_PROGRESSIVE_SCANS) return C89JPEG_ERR_LIMIT;
        st = c89jpeg_progressive_decode_scan(dec, entropy,
                                             (c89jpeg_u32)((data + size) - entropy),
                                             coeff_store, &marker, &after_marker);
        if (st != C89JPEG_OK) return st;
        st = c89jpeg_progressive_parse_between_scans(dec, after_marker, data + size,
                                                      marker, &next_entropy, &eoi);
        if (st != C89JPEG_OK) return st;
        entropy = next_entropy;
    }

    st = c89jpeg_progressive_render(dec, coeff_store, out_pixels, out_capacity, out_stride,
                                    out_format, upsampling, requested_roi, roi_mode,
                                    strip_buffer, strip_buffer_size, row_fn, row_user);
    if (st != C89JPEG_OK) return st;
    if (out_info != 0) *out_info = dec->info;
    return C89JPEG_OK;
}


static c89jpeg_status c89jpeg_decode_internal(c89jpeg_decoder *dec,
                                              const c89jpeg_tables *tables,
                                              const c89jpeg_u8 *data,
                                              c89jpeg_u32 size,
                                              c89jpeg_u8 *out_pixels,
                                              c89jpeg_u32 out_capacity,
                                              c89jpeg_u32 out_stride,
                                              c89jpeg_decode_format out_format,
                                              c89jpeg_upsampling_mode upsampling,
                                              const c89jpeg_rect *requested_roi,
                                              c89jpeg_roi_mode roi_mode,
                                              c89jpeg_u8 *strip_buffer,
                                              c89jpeg_u32 strip_buffer_size,
                                              c89jpeg_row_sink_fn row_fn,
                                              void *row_user,
                                              c89jpeg_image_info *out_info)
{
    c89jpeg_status st;
    c89jpeg_bitreader br;
    c89jpeg_rect region;
    c89jpeg_u32 offset;
    c89jpeg_u32 row_bytes;
    c89jpeg_u32 needed;
    c89jpeg_u16 mcu_cols;
    c89jpeg_u16 mcu_rows;
    c89jpeg_u16 mcu_w;
    c89jpeg_u16 mcu_h;
    c89jpeg_u16 my;
    c89jpeg_u16 mx;
    c89jpeg_u16 restart_left;
    c89jpeg_u8 next_rst;
    c89jpeg_u8 out_channels;
    int i;
    int block_index;
    int sink_mode;

    if (dec == 0 || data == 0) return C89JPEG_ERR_BAD_ARG;
    if (out_format != C89JPEG_DECODE_NATIVE && out_format != C89JPEG_DECODE_GRAY8 && out_format != C89JPEG_DECODE_RGB24) {
        return C89JPEG_ERR_BAD_ARG;
    }

    sink_mode = (row_fn != 0) ? C89JPEG_TRUE : C89JPEG_FALSE;
    if (sink_mode) {
        if (strip_buffer == 0) return C89JPEG_ERR_BAD_ARG;
    } else {
        if (out_pixels == 0) return C89JPEG_ERR_BAD_ARG;
    }

    st = c89jpeg_parse_headers_ex(dec, data, size, tables, C89JPEG_TRUE);
    if (st != C89JPEG_OK) return st;
    if (dec->info.progressive) return C89JPEG_ERR_UNSUPPORTED;
    if (dec->info.components == 3 && dec->scan_count != 3) return C89JPEG_ERR_UNSUPPORTED;

    mcu_w = (c89jpeg_u16)(dec->info.max_h_samp * 8);
    mcu_h = (c89jpeg_u16)(dec->info.max_v_samp * 8);
    st = c89jpeg_resolve_roi(&dec->info, requested_roi, roi_mode, &region);
    if (st != C89JPEG_OK) return st;

    offset = (c89jpeg_u32)((dec->entropy_offset > size) ? size : dec->entropy_offset);
    out_channels = c89jpeg_output_channels(out_format, dec->info.components);
    row_bytes = c89jpeg_decoder_row_stride(region.width, out_format, dec->info.components);

    if (sink_mode) {
        if (!c89jpeg_mul_u32(row_bytes, (c89jpeg_u32)mcu_h, &needed)) return C89JPEG_ERR_LIMIT;
        if (strip_buffer_size < needed) return C89JPEG_ERR_SHORT_BUFFER;
    } else {
        if (out_stride == 0) out_stride = row_bytes;
        if (!c89jpeg_mul_u32(out_stride, region.height, &needed)) return C89JPEG_ERR_LIMIT;
        if (out_capacity < needed) return C89JPEG_ERR_SHORT_BUFFER;
    }

    for (i = 0; i < dec->info.components; ++i) {
        if (!dec->quant_present[dec->comp[i].tq]) return C89JPEG_ERR_CORRUPT;
        if (dec->comp[i].dc_table >= C89JPEG_MAX_HUFF_TABLES || dec->comp[i].ac_table >= C89JPEG_MAX_HUFF_TABLES) {
            return C89JPEG_ERR_UNSUPPORTED;
        }
    }

    c89jpeg_br_init(&br, data + offset, size - offset);
    mcu_cols = (c89jpeg_u16)((dec->info.width + mcu_w - 1) / mcu_w);
    mcu_rows = (c89jpeg_u16)((dec->info.height + mcu_h - 1) / mcu_h);
    restart_left = dec->info.restart_interval;
    next_rst = 0;

    for (my = 0; my < mcu_rows; ++my) {
        c89jpeg_u32 row_start;
        c89jpeg_u32 row_end;
        int row_intersects;
        row_start = (c89jpeg_u32)my * mcu_h;
        row_end = row_start + mcu_h;
        row_intersects = (row_start < (c89jpeg_u32)region.y + region.height && row_end > region.y) ? C89JPEG_TRUE : C89JPEG_FALSE;
        if (sink_mode && row_intersects) {
            memset(strip_buffer, 0, (size_t)needed);
        }

        for (mx = 0; mx < mcu_cols; ++mx) {
            int render_this_mcu;
            render_this_mcu = c89jpeg_mcu_intersects_region(mx, my, mcu_w, mcu_h, &region);
            block_index = 0;
            for (i = 0; i < dec->scan_count; ++i) {
                c89jpeg_component *comp;
                c89jpeg_u8 by;
                c89jpeg_u8 bx;
                comp = dec->scan_comp[i];
                for (by = 0; by < comp->v_samp; ++by) {
                    for (bx = 0; bx < comp->h_samp; ++bx) {
                        st = c89jpeg_decode_block(dec, &br, comp, dec->blocks[block_index]);
                        if (st != C89JPEG_OK) return st;
                        if (render_this_mcu) {
                            int q;
                            for (q = 0; q < 64; ++q) dec->blocks[block_index][q] *= dec->quant[comp->tq][q];
                            c89jpeg_idct8x8(dec->blocks[block_index], dec->idct_tmp, dec->samples[block_index]);
                        }
                        ++block_index;
                    }
                }
            }

            if (render_this_mcu) {
                st = c89jpeg_render_mcu_region(dec,
                                               mx,
                                               my,
                                               mcu_w,
                                               mcu_h,
                                               out_format,
                                               upsampling,
                                               out_channels,
                                               sink_mode ? strip_buffer : out_pixels,
                                               sink_mode ? row_bytes : out_stride,
                                               &region,
                                               sink_mode);
                if (st != C89JPEG_OK) return st;
            }

            if (dec->info.restart_interval != 0) {
                if (--restart_left == 0) {
                    st = c89jpeg_process_restart(dec, &br, &next_rst);
                    if (st != C89JPEG_OK) return st;
                    restart_left = dec->info.restart_interval;
                }
            }
        }

        if (sink_mode && row_intersects) {
            c89jpeg_u32 clip_y0;
            c89jpeg_u32 clip_y1;
            clip_y0 = (row_start > region.y) ? row_start : region.y;
            clip_y1 = (row_end < (c89jpeg_u32)region.y + region.height) ? row_end : (c89jpeg_u32)region.y + region.height;
            for (i = (int)clip_y0; i < (int)clip_y1; ++i) {
                c89jpeg_u32 row_index;
                c89jpeg_u32 local_row;
                row_index = (c89jpeg_u32)i - region.y;
                local_row = (c89jpeg_u32)i - row_start;
                if (!row_fn(row_user, row_index, strip_buffer + local_row * row_bytes, row_bytes)) {
                    return C89JPEG_ERR_IO;
                }
            }
        }
    }

    if (out_info != 0) *out_info = dec->info;
    return C89JPEG_OK;
}

c89jpeg_status c89jpeg_decode(c89jpeg_decoder *dec, const c89jpeg_decode_params *params, c89jpeg_image_info *out_info)
{
    c89jpeg_status st;
    if (dec == 0 || params == 0) return C89JPEG_ERR_BAD_ARG;
    st = c89jpeg_parse_headers_ex(dec, params->data, params->size, 0, C89JPEG_TRUE);
    if (st == C89JPEG_OK && dec->info.progressive) {
        st = c89jpeg_decode_progressive_internal(dec, 0, params->data, params->size,
                                                 params->out_pixels, params->out_capacity, params->out_stride,
                                                 params->out_format, params->upsampling,
                                                 &params->roi, params->roi_mode,
                                                 0, 0, 0, 0,
                                                 params->progressive_workspace, params->progressive_workspace_size,
                                                 out_info);
    } else if (st == C89JPEG_OK) {
        st = c89jpeg_decode_internal(dec, 0, params->data, params->size,
                                     params->out_pixels, params->out_capacity, params->out_stride,
                                     params->out_format, params->upsampling,
                                     &params->roi, params->roi_mode,
                                     0, 0, 0, 0, out_info);
    }
    dec->last_error = st;
    return st;
}

c89jpeg_status c89jpeg_decode_sink(c89jpeg_decoder *dec, const c89jpeg_decode_sink_params *params, c89jpeg_row_sink_fn row_fn, void *row_user, c89jpeg_image_info *out_info)
{
    c89jpeg_status st;
    if (dec == 0 || params == 0 || row_fn == 0) return C89JPEG_ERR_BAD_ARG;
    st = c89jpeg_parse_headers_ex(dec, params->data, params->size, 0, C89JPEG_TRUE);
    if (st == C89JPEG_OK && dec->info.progressive) {
        st = c89jpeg_decode_progressive_internal(dec, 0, params->data, params->size,
                                                 0, 0, 0, params->out_format, params->upsampling,
                                                 &params->roi, params->roi_mode,
                                                 params->strip_buffer, params->strip_buffer_size, row_fn, row_user,
                                                 params->progressive_workspace, params->progressive_workspace_size,
                                                 out_info);
    } else if (st == C89JPEG_OK) {
        st = c89jpeg_decode_internal(dec, 0, params->data, params->size,
                                     0, 0, 0, params->out_format, params->upsampling,
                                     &params->roi, params->roi_mode,
                                     params->strip_buffer, params->strip_buffer_size, row_fn, row_user, out_info);
    }
    dec->last_error = st;
    return st;
}


typedef struct c89jpeg_stream_source_tag {
    c89jpeg_read_fn read_fn;
    void *read_user;
    c89jpeg_u8 *buffer;
    c89jpeg_u32 buffer_size;
    c89jpeg_u32 pos;
    c89jpeg_u32 fill;
    int eof;
} c89jpeg_stream_source;

typedef struct c89jpeg_stream_bitreader_tag {
    c89jpeg_stream_source *src;
    c89jpeg_u32 acc;
    int bits;
    int unread_marker;
} c89jpeg_stream_bitreader;

void c89jpeg_mem_src_init(c89jpeg_mem_src *src, const c89jpeg_u8 *buffer, c89jpeg_u32 size)
{
    if (src == 0) return;
    src->data = buffer;
    src->size = size;
    src->offset = 0;
    src->chunk_limit = 0;
}

void c89jpeg_mem_src_set_chunk_limit(c89jpeg_mem_src *src, c89jpeg_u32 chunk_limit)
{
    if (src == 0) return;
    src->chunk_limit = chunk_limit;
}

int c89jpeg_mem_src_read(void *user, c89jpeg_u8 *data, c89jpeg_u32 capacity, c89jpeg_u32 *out_read)
{
    c89jpeg_mem_src *src;
    c89jpeg_u32 remaining;
    c89jpeg_u32 count;
    if (user == 0 || data == 0 || out_read == 0) return C89JPEG_FALSE;
    src = (c89jpeg_mem_src *)user;
    if (src->offset > src->size) return C89JPEG_FALSE;
    remaining = src->size - src->offset;
    count = capacity;
    if (count > remaining) count = remaining;
    if (src->chunk_limit != 0 && count > src->chunk_limit) count = src->chunk_limit;
    if (count != 0) {
        memcpy(data, src->data + src->offset, (size_t)count);
        src->offset += count;
    }
    *out_read = count;
    return C89JPEG_TRUE;
}

static void c89jpeg_ss_init(c89jpeg_stream_source *src,
                            c89jpeg_read_fn read_fn,
                            void *read_user,
                            c89jpeg_u8 *buffer,
                            c89jpeg_u32 buffer_size)
{
    src->read_fn = read_fn;
    src->read_user = read_user;
    src->buffer = buffer;
    src->buffer_size = buffer_size;
    src->pos = 0;
    src->fill = 0;
    src->eof = C89JPEG_FALSE;
}

static c89jpeg_status c89jpeg_ss_refill(c89jpeg_stream_source *src, c89jpeg_u32 min_bytes)
{
    c89jpeg_u32 available;
    if (src->read_fn == 0 || src->buffer == 0 || src->buffer_size == 0) return C89JPEG_ERR_BAD_ARG;
    available = src->fill - src->pos;
    if (available >= min_bytes) return C89JPEG_OK;
    if (src->pos != 0 && available != 0) {
        memmove(src->buffer, src->buffer + src->pos, (size_t)available);
    }
    src->pos = 0;
    src->fill = available;
    while (src->fill < min_bytes && !src->eof) {
        c89jpeg_u32 got;
        c89jpeg_u32 cap;
        int ok;
        cap = src->buffer_size - src->fill;
        if (cap == 0) break;
        ok = src->read_fn(src->read_user, src->buffer + src->fill, cap, &got);
        if (!ok) return C89JPEG_ERR_IO;
        if (got > cap) return C89JPEG_ERR_IO;
        if (got == 0) {
            src->eof = C89JPEG_TRUE;
            break;
        }
        src->fill += got;
    }
    if (src->fill - src->pos < min_bytes) return C89JPEG_ERR_CORRUPT;
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_ss_get_u8(c89jpeg_stream_source *src, c89jpeg_u8 *v)
{
    c89jpeg_status st;
    st = c89jpeg_ss_refill(src, 1);
    if (st != C89JPEG_OK) return st;
    *v = src->buffer[src->pos++];
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_ss_get_u16(c89jpeg_stream_source *src, c89jpeg_u16 *v)
{
    c89jpeg_u8 hi;
    c89jpeg_u8 lo;
    C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(src, &hi));
    C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(src, &lo));
    *v = (c89jpeg_u16)(((c89jpeg_u16)hi << 8) | (c89jpeg_u16)lo);
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_ss_skip(c89jpeg_stream_source *src, c89jpeg_u32 n)
{
    while (n > 0) {
        c89jpeg_u32 available;
        c89jpeg_u32 step;
        c89jpeg_status st;
        st = c89jpeg_ss_refill(src, 1);
        if (st != C89JPEG_OK) return st;
        available = src->fill - src->pos;
        step = (n < available) ? n : available;
        src->pos += step;
        n -= step;
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_ss_next_marker(c89jpeg_stream_source *src, c89jpeg_u8 *marker)
{
    c89jpeg_u8 c;
    do {
        C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(src, &c));
    } while (c != 0xFF);
    do {
        C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(src, &c));
    } while (c == 0xFF);
    if (c == 0x00) return C89JPEG_ERR_CORRUPT;
    *marker = c;
    return C89JPEG_OK;
}

static void c89jpeg_sbr_init(c89jpeg_stream_bitreader *br, c89jpeg_stream_source *src)
{
    br->src = src;
    br->acc = 0;
    br->bits = 0;
    br->unread_marker = 0;
}

static c89jpeg_status c89jpeg_sbr_fill(c89jpeg_stream_bitreader *br)
{
    while (br->bits <= 24 && br->unread_marker == 0) {
        c89jpeg_u8 c;
        C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(br->src, &c));
        if (c == 0xFF) {
            c89jpeg_u8 next;
            do {
                C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(br->src, &next));
            } while (next == 0xFF);
            if (next == 0x00) {
                c = 0xFF;
            } else {
                br->unread_marker = (int)next;
                return C89JPEG_OK;
            }
        }
        br->acc = (br->acc << 8) | (c89jpeg_u32)c;
        br->bits += 8;
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_sbr_get_bit(c89jpeg_stream_bitreader *br, int *bit)
{
    c89jpeg_status st;
    if (br->bits == 0) {
        st = c89jpeg_sbr_fill(br);
        if (st != C89JPEG_OK) return st;
        if (br->bits == 0) return C89JPEG_ERR_CORRUPT;
    }
    *bit = (int)((br->acc >> (br->bits - 1)) & 1UL);
    --br->bits;
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_sbr_get_bits(c89jpeg_stream_bitreader *br, int count, c89jpeg_u32 *value)
{
    c89jpeg_status st;
    c89jpeg_u32 v;
    int bit;
    v = 0;
    while (count > 0) {
        st = c89jpeg_sbr_get_bit(br, &bit);
        if (st != C89JPEG_OK) return st;
        v = (v << 1) | (c89jpeg_u32)bit;
        --count;
    }
    *value = v;
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_stream_huff_decode_symbol(c89jpeg_stream_bitreader *br, const c89jpeg_huffman_table *tab, c89jpeg_u8 *symbol)
{
    c89jpeg_i32 code;
    int bit;
    int i;
    c89jpeg_status st;
    code = 0;
    for (i = 1; i <= 16; ++i) {
        st = c89jpeg_sbr_get_bit(br, &bit);
        if (st != C89JPEG_OK) return st;
        code = (code << 1) | (c89jpeg_i32)bit;
        if (tab->mincode[i] >= 0 && code >= tab->mincode[i] && code <= tab->maxcode[i]) {
            c89jpeg_i32 idx;
            idx = tab->valptr[i] + code - tab->mincode[i];
            if (idx < 0 || idx >= (c89jpeg_i32)tab->count) return C89JPEG_ERR_CORRUPT;
            *symbol = tab->vals[idx];
            return C89JPEG_OK;
        }
    }
    return C89JPEG_ERR_CORRUPT;
}

static c89jpeg_i32 c89jpeg_stream_receive_extend(c89jpeg_stream_bitreader *br, int size, c89jpeg_status *st)
{
    c89jpeg_u32 v;
    if (size == 0) {
        *st = C89JPEG_OK;
        return 0;
    }
    *st = c89jpeg_sbr_get_bits(br, size, &v);
    if (*st != C89JPEG_OK) return 0;
    if (v < ((c89jpeg_u32)1 << (size - 1))) {
        return (c89jpeg_i32)v - (((c89jpeg_i32)1 << size) - 1);
    }
    return (c89jpeg_i32)v;
}

static c89jpeg_status c89jpeg_decode_block_stream(c89jpeg_decoder *dec, c89jpeg_stream_bitreader *br, c89jpeg_component *comp, c89jpeg_i32 *coeff)
{
    c89jpeg_status st;
    c89jpeg_u8 sym;
    c89jpeg_i32 diff;
    int k;
    int idx;
    const c89jpeg_huffman_table *dc_tab;
    const c89jpeg_huffman_table *ac_tab;
    if (comp->dc_table >= C89JPEG_MAX_HUFF_TABLES || comp->ac_table >= C89JPEG_MAX_HUFF_TABLES) return C89JPEG_ERR_UNSUPPORTED;
    dc_tab = &dec->huff[0][comp->dc_table];
    ac_tab = &dec->huff[1][comp->ac_table];
    if (!dc_tab->present || !ac_tab->present) return C89JPEG_ERR_CORRUPT;
    memset(coeff, 0, 64 * sizeof(coeff[0]));
    st = c89jpeg_stream_huff_decode_symbol(br, dc_tab, &sym);
    if (st != C89JPEG_OK) return st;
    diff = c89jpeg_stream_receive_extend(br, sym, &st);
    if (st != C89JPEG_OK) return st;
    comp->pred += diff;
    coeff[0] = comp->pred;
    k = 1;
    while (k < 64) {
        st = c89jpeg_stream_huff_decode_symbol(br, ac_tab, &sym);
        if (st != C89JPEG_OK) return st;
        if (sym == 0) break;
        if (sym == 0xF0) {
            k += 16;
            continue;
        }
        k += (sym >> 4);
        if (k >= 64) return C89JPEG_ERR_CORRUPT;
        idx = c89jpeg_zigzag[k];
        coeff[idx] = c89jpeg_stream_receive_extend(br, sym & 0x0F, &st);
        if (st != C89JPEG_OK) return st;
        ++k;
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_process_restart_stream(c89jpeg_decoder *dec, c89jpeg_stream_bitreader *br, c89jpeg_u8 *next_rst)
{
    c89jpeg_u8 marker;
    int i;
    if (br->unread_marker == 0) {
        C89JPEG_EMIT_CHECK(c89jpeg_ss_next_marker(br->src, &marker));
        br->unread_marker = (int)marker;
    }
    if (br->unread_marker != (int)(C89JPEG_MARKER_RST0 + *next_rst)) return C89JPEG_ERR_CORRUPT;
    br->bits = 0;
    br->acc = 0;
    br->unread_marker = 0;
    *next_rst = (c89jpeg_u8)((*next_rst + 1) & 7);
    for (i = 0; i < dec->info.components; ++i) dec->comp[i].pred = 0;
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_parse_dqt_stream(c89jpeg_decoder *dec, c89jpeg_stream_source *src, c89jpeg_u16 seglen)
{
    c89jpeg_u16 remaining;
    remaining = (c89jpeg_u16)(seglen - 2);
    while (remaining > 0) {
        c89jpeg_u8 pq_tq;
        c89jpeg_u8 pq;
        c89jpeg_u8 tq;
        int i;
        if (remaining < 65) return C89JPEG_ERR_CORRUPT;
        C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(src, &pq_tq));
        --remaining;
        pq = (c89jpeg_u8)(pq_tq >> 4);
        tq = (c89jpeg_u8)(pq_tq & 0x0F);
        if (pq != 0 || tq >= C89JPEG_MAX_QUANT_TABLES) return C89JPEG_ERR_UNSUPPORTED;
        if (remaining < 64) return C89JPEG_ERR_CORRUPT;
        for (i = 0; i < 64; ++i) {
            c89jpeg_u8 q;
            C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(src, &q));
            dec->quant[tq][c89jpeg_zigzag[i]] = q;
        }
        dec->quant_present[tq] = 1;
        remaining = (c89jpeg_u16)(remaining - 64);
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_parse_dht_stream(c89jpeg_decoder *dec, c89jpeg_stream_source *src, c89jpeg_u16 seglen)
{
    c89jpeg_u16 remaining;
    remaining = (c89jpeg_u16)(seglen - 2);
    while (remaining > 0) {
        c89jpeg_u8 tc_th;
        c89jpeg_u8 bits[16];
        c89jpeg_u8 vals[256];
        c89jpeg_u16 count;
        c89jpeg_u8 tc;
        c89jpeg_u8 th;
        int i;
        if (remaining < 17) return C89JPEG_ERR_CORRUPT;
        C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(src, &tc_th));
        --remaining;
        tc = (c89jpeg_u8)(tc_th >> 4);
        th = (c89jpeg_u8)(tc_th & 0x0F);
        if (tc >= C89JPEG_MAX_HUFF_CLASSES || th >= C89JPEG_MAX_HUFF_TABLES) return C89JPEG_ERR_UNSUPPORTED;
        count = 0;
        for (i = 0; i < 16; ++i) {
            C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(src, &bits[i]));
            count = (c89jpeg_u16)(count + bits[i]);
        }
        remaining = (c89jpeg_u16)(remaining - 16);
        if (remaining < count) return C89JPEG_ERR_CORRUPT;
        for (i = 0; i < (int)count; ++i) {
            C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(src, &vals[i]));
        }
        remaining = (c89jpeg_u16)(remaining - count);
        C89JPEG_EMIT_CHECK(c89jpeg_build_huffman(&dec->huff[tc][th], bits, vals, count));
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_parse_sof_stream(c89jpeg_decoder *dec, c89jpeg_stream_source *src, c89jpeg_u16 seglen, c89jpeg_u8 marker)
{
    c89jpeg_u8 precision;
    c89jpeg_u16 height;
    c89jpeg_u16 width;
    c89jpeg_u8 components;
    int i;
    int blocks_per_mcu;
    c89jpeg_u8 max_h;
    c89jpeg_u8 max_v;
    c89jpeg_u16 expected_payload;
    if (seglen < 8) return C89JPEG_ERR_CORRUPT;
    C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(src, &precision));
    C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u16(src, &height));
    C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u16(src, &width));
    C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(src, &components));
    if (precision != 8) return C89JPEG_ERR_UNSUPPORTED;
    if (components != 1 && components != 3) return C89JPEG_ERR_UNSUPPORTED;
    expected_payload = (c89jpeg_u16)(6 + 3 * components);
    if ((c89jpeg_u16)(seglen - 2) < expected_payload) return C89JPEG_ERR_CORRUPT;
    dec->info.width = width;
    dec->info.height = height;
    dec->info.precision = precision;
    dec->info.components = components;
    dec->info.progressive = (marker == C89JPEG_MARKER_SOF2) ? 1 : 0;
    dec->info.arithmetic = (marker == C89JPEG_MARKER_SOF9 || marker == C89JPEG_MARKER_SOF10 || marker == C89JPEG_MARKER_SOF11 || marker == C89JPEG_MARKER_SOF13 || marker == C89JPEG_MARKER_SOF14 || marker == C89JPEG_MARKER_SOF15) ? 1 : 0;
    if (marker != C89JPEG_MARKER_SOF0) return C89JPEG_ERR_UNSUPPORTED;

    max_h = 0;
    max_v = 0;
    blocks_per_mcu = 0;
    for (i = 0; i < components; ++i) {
        c89jpeg_u8 id;
        c89jpeg_u8 hv;
        c89jpeg_u8 tq;
        C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(src, &id));
        C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(src, &hv));
        C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(src, &tq));
        dec->comp[i].id = id;
        dec->comp[i].h_samp = (c89jpeg_u8)(hv >> 4);
        dec->comp[i].v_samp = (c89jpeg_u8)(hv & 0x0F);
        dec->comp[i].tq = tq;
        dec->comp[i].pred = 0;
        dec->comp[i].scan_index = (c89jpeg_u8)i;
        if (dec->comp[i].h_samp == 0 || dec->comp[i].v_samp == 0) return C89JPEG_ERR_CORRUPT;
        if (tq >= C89JPEG_MAX_QUANT_TABLES) return C89JPEG_ERR_UNSUPPORTED;
        if (dec->comp[i].h_samp > max_h) max_h = dec->comp[i].h_samp;
        if (dec->comp[i].v_samp > max_v) max_v = dec->comp[i].v_samp;
        dec->info.component[i].id = id;
        dec->info.component[i].h_samp = dec->comp[i].h_samp;
        dec->info.component[i].v_samp = dec->comp[i].v_samp;
        dec->info.component[i].quant_table = tq;
        blocks_per_mcu += dec->comp[i].h_samp * dec->comp[i].v_samp;
    }
    if (blocks_per_mcu > C89JPEG_MAX_BLOCKS_PER_MCU) return C89JPEG_ERR_LIMIT;
    dec->info.max_h_samp = max_h;
    dec->info.max_v_samp = max_v;
    if (max_h == 0 || max_v == 0) return C89JPEG_ERR_CORRUPT;
    if ((c89jpeg_u16)(seglen - 2) > expected_payload) {
        C89JPEG_EMIT_CHECK(c89jpeg_ss_skip(src, (c89jpeg_u32)((seglen - 2) - expected_payload)));
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_parse_app0_stream(c89jpeg_decoder *dec, c89jpeg_stream_source *src, c89jpeg_u16 seglen)
{
    c89jpeg_u8 tag[5];
    c89jpeg_u8 units;
    c89jpeg_u16 xden;
    c89jpeg_u16 yden;
    if (seglen < 16) return c89jpeg_ss_skip(src, seglen - 2);
    C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(src, &tag[0]));
    C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(src, &tag[1]));
    C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(src, &tag[2]));
    C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(src, &tag[3]));
    C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(src, &tag[4]));
    if (tag[0] == 'J' && tag[1] == 'F' && tag[2] == 'I' && tag[3] == 'F' && tag[4] == 0) {
        dec->info.is_jfif = 1;
        C89JPEG_EMIT_CHECK(c89jpeg_ss_skip(src, 2));
        C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(src, &units));
        C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u16(src, &xden));
        C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u16(src, &yden));
        dec->info.density_units = units;
        dec->info.density_x = xden;
        dec->info.density_y = yden;
        return c89jpeg_ss_skip(src, (c89jpeg_u32)(seglen - 2 - 5 - 2 - 1 - 2 - 2));
    }
    return c89jpeg_ss_skip(src, (c89jpeg_u32)(seglen - 2 - 5));
}

static c89jpeg_status c89jpeg_parse_sos_stream(c89jpeg_decoder *dec, c89jpeg_stream_source *src, c89jpeg_u16 seglen)
{
    c89jpeg_u8 ns;
    c89jpeg_u8 ss;
    c89jpeg_u8 se;
    c89jpeg_u8 ahal;
    c89jpeg_u16 expected_payload;
    int i;
    if (seglen < 6) return C89JPEG_ERR_CORRUPT;
    C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(src, &ns));
    if (ns == 0 || ns > dec->info.components || ns > C89JPEG_MAX_COMPONENTS) return C89JPEG_ERR_UNSUPPORTED;
    dec->scan_count = ns;
    expected_payload = (c89jpeg_u16)(1 + 2 * ns + 3);
    if ((c89jpeg_u16)(seglen - 2) < expected_payload) return C89JPEG_ERR_CORRUPT;
    for (i = 0; i < ns; ++i) {
        c89jpeg_u8 csj;
        c89jpeg_u8 tdta;
        int found;
        int c;
        C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(src, &csj));
        C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(src, &tdta));
        found = C89JPEG_FALSE;
        for (c = 0; c < dec->info.components; ++c) {
            if (dec->comp[c].id == csj) {
                dec->scan_comp[i] = &dec->comp[c];
                dec->comp[c].scan_index = (c89jpeg_u8)i;
                dec->comp[c].dc_table = (c89jpeg_u8)(tdta >> 4);
                dec->comp[c].ac_table = (c89jpeg_u8)(tdta & 0x0F);
                dec->info.component[c].dc_table = dec->comp[c].dc_table;
                dec->info.component[c].ac_table = dec->comp[c].ac_table;
                found = C89JPEG_TRUE;
                break;
            }
        }
        if (!found) return C89JPEG_ERR_CORRUPT;
    }
    C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(src, &ss));
    C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(src, &se));
    C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(src, &ahal));
    if (ss != 0 || se != 63 || ahal != 0) return C89JPEG_ERR_UNSUPPORTED;
    if (dec->info.components == 3 && ns != 3) return C89JPEG_ERR_UNSUPPORTED;
    if ((c89jpeg_u16)(seglen - 2) > expected_payload) {
        C89JPEG_EMIT_CHECK(c89jpeg_ss_skip(src, (c89jpeg_u32)((seglen - 2) - expected_payload)));
    }
    dec->scan_started = 1;
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_parse_headers_stream_ex(c89jpeg_decoder *dec, c89jpeg_stream_source *src, const c89jpeg_tables *tables, int require_sos)
{
    c89jpeg_u8 b0;
    c89jpeg_u8 b1;
    if (dec == 0 || src == 0) return C89JPEG_ERR_BAD_ARG;
    c89jpeg_decoder_clear_frame_state(dec);
    if (tables != 0) c89jpeg_decoder_install_tables(dec, tables);
    C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(src, &b0));
    C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u8(src, &b1));
    if (b0 != 0xFF || b1 != C89JPEG_MARKER_SOI) return C89JPEG_ERR_CORRUPT;
    while (1) {
        c89jpeg_u8 marker;
        c89jpeg_u16 seglen;
        C89JPEG_EMIT_CHECK(c89jpeg_ss_next_marker(src, &marker));
        if (marker == C89JPEG_MARKER_EOI) {
            if (require_sos) return C89JPEG_ERR_CORRUPT;
            break;
        }
        if (marker == C89JPEG_MARKER_SOS) {
            C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u16(src, &seglen));
            C89JPEG_EMIT_CHECK(c89jpeg_parse_sos_stream(dec, src, seglen));
            break;
        }
        if (marker >= C89JPEG_MARKER_RST0 && marker <= C89JPEG_MARKER_RST7) return C89JPEG_ERR_CORRUPT;
        C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u16(src, &seglen));
        if (seglen < 2) return C89JPEG_ERR_CORRUPT;
        if (c89jpeg_is_sof(marker)) {
            C89JPEG_EMIT_CHECK(c89jpeg_parse_sof_stream(dec, src, seglen, marker));
        } else if (marker == C89JPEG_MARKER_DQT) {
            C89JPEG_EMIT_CHECK(c89jpeg_parse_dqt_stream(dec, src, seglen));
        } else if (marker == C89JPEG_MARKER_DHT) {
            C89JPEG_EMIT_CHECK(c89jpeg_parse_dht_stream(dec, src, seglen));
        } else if (marker == C89JPEG_MARKER_DRI) {
            C89JPEG_EMIT_CHECK(c89jpeg_ss_get_u16(src, &dec->info.restart_interval));
            if (seglen > 4) C89JPEG_EMIT_CHECK(c89jpeg_ss_skip(src, seglen - 4));
        } else if (marker == C89JPEG_MARKER_APP0) {
            C89JPEG_EMIT_CHECK(c89jpeg_parse_app0_stream(dec, src, seglen));
        } else {
            C89JPEG_EMIT_CHECK(c89jpeg_ss_skip(src, seglen - 2));
        }
    }
    if (require_sos && !dec->scan_started) return C89JPEG_ERR_CORRUPT;
    if (dec->info.width != 0 || dec->scan_started) {
        C89JPEG_EMIT_CHECK(c89jpeg_finalize_dimensions(dec));
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_decode_internal_source(c89jpeg_decoder *dec,
                                                     const c89jpeg_tables *tables,
                                                     c89jpeg_read_fn read_fn,
                                                     void *read_user,
                                                     c89jpeg_u8 *input_buffer,
                                                     c89jpeg_u32 input_buffer_size,
                                                     c89jpeg_u8 *out_pixels,
                                                     c89jpeg_u32 out_capacity,
                                                     c89jpeg_u32 out_stride,
                                                     c89jpeg_decode_format out_format,
                                                     c89jpeg_upsampling_mode upsampling,
                                                     const c89jpeg_rect *requested_roi,
                                                     c89jpeg_roi_mode roi_mode,
                                                     c89jpeg_u8 *strip_buffer,
                                                     c89jpeg_u32 strip_buffer_size,
                                                     c89jpeg_row_sink_fn row_fn,
                                                     void *row_user,
                                                     c89jpeg_image_info *out_info)
{
    c89jpeg_status st;
    c89jpeg_stream_source src;
    c89jpeg_stream_bitreader br;
    c89jpeg_rect region;
    c89jpeg_u32 row_bytes;
    c89jpeg_u32 needed;
    c89jpeg_u16 mcu_cols;
    c89jpeg_u16 mcu_rows;
    c89jpeg_u16 mcu_w;
    c89jpeg_u16 mcu_h;
    c89jpeg_u16 my;
    c89jpeg_u16 mx;
    c89jpeg_u16 restart_left;
    c89jpeg_u8 next_rst;
    c89jpeg_u8 out_channels;
    int i;
    int block_index;
    int sink_mode;

    if (dec == 0 || read_fn == 0 || input_buffer == 0 || input_buffer_size == 0) return C89JPEG_ERR_BAD_ARG;
    if (out_format != C89JPEG_DECODE_NATIVE && out_format != C89JPEG_DECODE_GRAY8 && out_format != C89JPEG_DECODE_RGB24) {
        return C89JPEG_ERR_BAD_ARG;
    }

    sink_mode = (row_fn != 0) ? C89JPEG_TRUE : C89JPEG_FALSE;
    if (sink_mode) {
        if (strip_buffer == 0) return C89JPEG_ERR_BAD_ARG;
    } else {
        if (out_pixels == 0) return C89JPEG_ERR_BAD_ARG;
    }

    c89jpeg_ss_init(&src, read_fn, read_user, input_buffer, input_buffer_size);
    st = c89jpeg_parse_headers_stream_ex(dec, &src, tables, C89JPEG_TRUE);
    if (st != C89JPEG_OK) return st;
    if (dec->info.components == 3 && dec->scan_count != 3) return C89JPEG_ERR_UNSUPPORTED;

    mcu_w = (c89jpeg_u16)(dec->info.max_h_samp * 8);
    mcu_h = (c89jpeg_u16)(dec->info.max_v_samp * 8);
    st = c89jpeg_resolve_roi(&dec->info, requested_roi, roi_mode, &region);
    if (st != C89JPEG_OK) return st;

    out_channels = c89jpeg_output_channels(out_format, dec->info.components);
    row_bytes = c89jpeg_decoder_row_stride(region.width, out_format, dec->info.components);

    if (sink_mode) {
        if (!c89jpeg_mul_u32(row_bytes, (c89jpeg_u32)mcu_h, &needed)) return C89JPEG_ERR_LIMIT;
        if (strip_buffer_size < needed) return C89JPEG_ERR_SHORT_BUFFER;
    } else {
        if (out_stride == 0) out_stride = row_bytes;
        if (!c89jpeg_mul_u32(out_stride, region.height, &needed)) return C89JPEG_ERR_LIMIT;
        if (out_capacity < needed) return C89JPEG_ERR_SHORT_BUFFER;
    }

    for (i = 0; i < dec->info.components; ++i) {
        if (!dec->quant_present[dec->comp[i].tq]) return C89JPEG_ERR_CORRUPT;
        if (dec->comp[i].dc_table >= C89JPEG_MAX_HUFF_TABLES || dec->comp[i].ac_table >= C89JPEG_MAX_HUFF_TABLES) {
            return C89JPEG_ERR_UNSUPPORTED;
        }
    }

    c89jpeg_sbr_init(&br, &src);
    mcu_cols = (c89jpeg_u16)((dec->info.width + mcu_w - 1) / mcu_w);
    mcu_rows = (c89jpeg_u16)((dec->info.height + mcu_h - 1) / mcu_h);
    restart_left = dec->info.restart_interval;
    next_rst = 0;

    for (my = 0; my < mcu_rows; ++my) {
        c89jpeg_u32 row_start;
        c89jpeg_u32 row_end;
        int row_intersects;
        row_start = (c89jpeg_u32)my * mcu_h;
        row_end = row_start + mcu_h;
        row_intersects = (row_start < (c89jpeg_u32)region.y + region.height && row_end > region.y) ? C89JPEG_TRUE : C89JPEG_FALSE;
        if (sink_mode && row_intersects) {
            memset(strip_buffer, 0, (size_t)needed);
        }

        for (mx = 0; mx < mcu_cols; ++mx) {
            int render_this_mcu;
            render_this_mcu = c89jpeg_mcu_intersects_region(mx, my, mcu_w, mcu_h, &region);
            block_index = 0;
            for (i = 0; i < dec->scan_count; ++i) {
                c89jpeg_component *comp;
                c89jpeg_u8 by;
                c89jpeg_u8 bx;
                comp = dec->scan_comp[i];
                for (by = 0; by < comp->v_samp; ++by) {
                    for (bx = 0; bx < comp->h_samp; ++bx) {
                        st = c89jpeg_decode_block_stream(dec, &br, comp, dec->blocks[block_index]);
                        if (st != C89JPEG_OK) return st;
                        if (render_this_mcu) {
                            int q;
                            for (q = 0; q < 64; ++q) dec->blocks[block_index][q] *= dec->quant[comp->tq][q];
                            c89jpeg_idct8x8(dec->blocks[block_index], dec->idct_tmp, dec->samples[block_index]);
                        }
                        ++block_index;
                    }
                }
            }

            if (render_this_mcu) {
                st = c89jpeg_render_mcu_region(dec,
                                               mx,
                                               my,
                                               mcu_w,
                                               mcu_h,
                                               out_format,
                                               upsampling,
                                               out_channels,
                                               sink_mode ? strip_buffer : out_pixels,
                                               sink_mode ? row_bytes : out_stride,
                                               &region,
                                               sink_mode);
                if (st != C89JPEG_OK) return st;
            }

            if (dec->info.restart_interval != 0) {
                if (--restart_left == 0) {
                    st = c89jpeg_process_restart_stream(dec, &br, &next_rst);
                    if (st != C89JPEG_OK) return st;
                    restart_left = dec->info.restart_interval;
                }
            }
        }

        if (sink_mode && row_intersects) {
            c89jpeg_u32 clip_y0;
            c89jpeg_u32 clip_y1;
            clip_y0 = (row_start > region.y) ? row_start : region.y;
            clip_y1 = (row_end < (c89jpeg_u32)region.y + region.height) ? row_end : (c89jpeg_u32)region.y + region.height;
            for (i = (int)clip_y0; i < (int)clip_y1; ++i) {
                c89jpeg_u32 row_index;
                c89jpeg_u32 local_row;
                row_index = (c89jpeg_u32)i - region.y;
                local_row = (c89jpeg_u32)i - row_start;
                if (!row_fn(row_user, row_index, strip_buffer + local_row * row_bytes, row_bytes)) {
                    return C89JPEG_ERR_IO;
                }
            }
        }
    }

    if (out_info != 0) *out_info = dec->info;
    return C89JPEG_OK;
}

c89jpeg_status c89jpeg_probe_source(c89jpeg_decoder *dec, c89jpeg_read_fn read_fn, void *read_user, c89jpeg_u8 *input_buffer, c89jpeg_u32 input_buffer_size, c89jpeg_image_info *out_info)
{
    c89jpeg_status st;
    c89jpeg_stream_source src;
    if (dec == 0 || read_fn == 0 || input_buffer == 0 || input_buffer_size == 0) return C89JPEG_ERR_BAD_ARG;
    c89jpeg_ss_init(&src, read_fn, read_user, input_buffer, input_buffer_size);
    st = c89jpeg_parse_headers_stream_ex(dec, &src, 0, C89JPEG_TRUE);
    dec->last_error = st;
    if (st != C89JPEG_OK) return st;
    if (out_info != 0) *out_info = dec->info;
    return C89JPEG_OK;
}

c89jpeg_status c89jpeg_probe_abbreviated_source(c89jpeg_decoder *dec, const c89jpeg_tables *tables, c89jpeg_read_fn read_fn, void *read_user, c89jpeg_u8 *input_buffer, c89jpeg_u32 input_buffer_size, c89jpeg_image_info *out_info)
{
    c89jpeg_status st;
    c89jpeg_stream_source src;
    if (dec == 0 || tables == 0 || read_fn == 0 || input_buffer == 0 || input_buffer_size == 0) return C89JPEG_ERR_BAD_ARG;
    c89jpeg_ss_init(&src, read_fn, read_user, input_buffer, input_buffer_size);
    st = c89jpeg_parse_headers_stream_ex(dec, &src, tables, C89JPEG_TRUE);
    dec->last_error = st;
    if (st != C89JPEG_OK) return st;
    if (out_info != 0) *out_info = dec->info;
    return C89JPEG_OK;
}

c89jpeg_status c89jpeg_decode_source(c89jpeg_decoder *dec, const c89jpeg_decode_source_params *params, c89jpeg_image_info *out_info)
{
    c89jpeg_status st;
    if (dec == 0 || params == 0) return C89JPEG_ERR_BAD_ARG;
    st = c89jpeg_decode_internal_source(dec,
                                        0,
                                        params->read_fn,
                                        params->read_user,
                                        params->input_buffer,
                                        params->input_buffer_size,
                                        params->out_pixels,
                                        params->out_capacity,
                                        params->out_stride,
                                        params->out_format,
                                        params->upsampling,
                                        &params->roi,
                                        params->roi_mode,
                                        0,
                                        0,
                                        0,
                                        0,
                                        out_info);
    dec->last_error = st;
    return st;
}

c89jpeg_status c89jpeg_decode_source_sink(c89jpeg_decoder *dec, const c89jpeg_decode_source_sink_params *params, c89jpeg_row_sink_fn row_fn, void *row_user, c89jpeg_image_info *out_info)
{
    c89jpeg_status st;
    if (dec == 0 || params == 0 || row_fn == 0) return C89JPEG_ERR_BAD_ARG;
    st = c89jpeg_decode_internal_source(dec,
                                        0,
                                        params->read_fn,
                                        params->read_user,
                                        params->input_buffer,
                                        params->input_buffer_size,
                                        0,
                                        0,
                                        0,
                                        params->out_format,
                                        params->upsampling,
                                        &params->roi,
                                        params->roi_mode,
                                        params->strip_buffer,
                                        params->strip_buffer_size,
                                        row_fn,
                                        row_user,
                                        out_info);
    dec->last_error = st;
    return st;
}


c89jpeg_status c89jpeg_decode_abbreviated(c89jpeg_decoder *dec, const c89jpeg_tables *tables, const c89jpeg_decode_params *params, c89jpeg_image_info *out_info)
{
    c89jpeg_status st;
    if (dec == 0 || tables == 0 || params == 0) return C89JPEG_ERR_BAD_ARG;
    st = c89jpeg_parse_headers_ex(dec, params->data, params->size, tables, C89JPEG_TRUE);
    if (st == C89JPEG_OK && dec->info.progressive) {
        st = c89jpeg_decode_progressive_internal(dec, tables, params->data, params->size,
                                                 params->out_pixels, params->out_capacity, params->out_stride,
                                                 params->out_format, params->upsampling,
                                                 &params->roi, params->roi_mode,
                                                 0, 0, 0, 0,
                                                 params->progressive_workspace, params->progressive_workspace_size,
                                                 out_info);
    } else if (st == C89JPEG_OK) {
        st = c89jpeg_decode_internal(dec, tables, params->data, params->size,
                                     params->out_pixels, params->out_capacity, params->out_stride,
                                     params->out_format, params->upsampling,
                                     &params->roi, params->roi_mode,
                                     0, 0, 0, 0, out_info);
    }
    dec->last_error = st;
    return st;
}

c89jpeg_status c89jpeg_decode_abbreviated_sink(c89jpeg_decoder *dec, const c89jpeg_tables *tables, const c89jpeg_decode_sink_params *params, c89jpeg_row_sink_fn row_fn, void *row_user, c89jpeg_image_info *out_info)
{
    c89jpeg_status st;
    if (dec == 0 || tables == 0 || params == 0 || row_fn == 0) return C89JPEG_ERR_BAD_ARG;
    st = c89jpeg_parse_headers_ex(dec, params->data, params->size, tables, C89JPEG_TRUE);
    if (st == C89JPEG_OK && dec->info.progressive) {
        st = c89jpeg_decode_progressive_internal(dec, tables, params->data, params->size,
                                                 0, 0, 0, params->out_format, params->upsampling,
                                                 &params->roi, params->roi_mode,
                                                 params->strip_buffer, params->strip_buffer_size, row_fn, row_user,
                                                 params->progressive_workspace, params->progressive_workspace_size,
                                                 out_info);
    } else if (st == C89JPEG_OK) {
        st = c89jpeg_decode_internal(dec, tables, params->data, params->size,
                                     0, 0, 0, params->out_format, params->upsampling,
                                     &params->roi, params->roi_mode,
                                     params->strip_buffer, params->strip_buffer_size, row_fn, row_user, out_info);
    }
    dec->last_error = st;
    return st;
}

c89jpeg_status c89jpeg_decode_abbreviated_source(c89jpeg_decoder *dec, const c89jpeg_tables *tables, const c89jpeg_decode_source_params *params, c89jpeg_image_info *out_info)
{
    c89jpeg_status st;
    if (dec == 0 || tables == 0 || params == 0) return C89JPEG_ERR_BAD_ARG;
    st = c89jpeg_decode_internal_source(dec,
                                        tables,
                                        params->read_fn,
                                        params->read_user,
                                        params->input_buffer,
                                        params->input_buffer_size,
                                        params->out_pixels,
                                        params->out_capacity,
                                        params->out_stride,
                                        params->out_format,
                                        params->upsampling,
                                        &params->roi,
                                        params->roi_mode,
                                        0,
                                        0,
                                        0,
                                        0,
                                        out_info);
    dec->last_error = st;
    return st;
}

c89jpeg_status c89jpeg_decode_abbreviated_source_sink(c89jpeg_decoder *dec, const c89jpeg_tables *tables, const c89jpeg_decode_source_sink_params *params, c89jpeg_row_sink_fn row_fn, void *row_user, c89jpeg_image_info *out_info)
{
    c89jpeg_status st;
    if (dec == 0 || tables == 0 || params == 0 || row_fn == 0) return C89JPEG_ERR_BAD_ARG;
    st = c89jpeg_decode_internal_source(dec,
                                        tables,
                                        params->read_fn,
                                        params->read_user,
                                        params->input_buffer,
                                        params->input_buffer_size,
                                        0,
                                        0,
                                        0,
                                        params->out_format,
                                        params->upsampling,
                                        &params->roi,
                                        params->roi_mode,
                                        params->strip_buffer,
                                        params->strip_buffer_size,
                                        row_fn,
                                        row_user,
                                        out_info);
    dec->last_error = st;
    return st;
}

typedef struct c89jpeg_resume_bitreader_tag {
    const c89jpeg_u8 *data;
    c89jpeg_u32 size;
    c89jpeg_u32 pos;
    c89jpeg_u32 acc;
    int bits;
    int unread_marker;
    int final_input;
} c89jpeg_resume_bitreader;

static void c89jpeg_rbr_init(c89jpeg_resume_bitreader *br,
                             const c89jpeg_u8 *data,
                             c89jpeg_u32 size,
                             c89jpeg_u32 pos,
                             c89jpeg_u32 acc,
                             int bits,
                             int unread_marker,
                             int final_input)
{
    br->data = data;
    br->size = size;
    br->pos = pos;
    br->acc = acc;
    br->bits = bits;
    br->unread_marker = unread_marker;
    br->final_input = final_input;
}

static c89jpeg_status c89jpeg_rbr_get_u8(c89jpeg_resume_bitreader *br, c89jpeg_u8 *v)
{
    if (br->pos >= br->size) return br->final_input ? C89JPEG_ERR_CORRUPT : C89JPEG_SUSPENDED;
    *v = br->data[br->pos++];
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_rbr_next_marker(c89jpeg_resume_bitreader *br, c89jpeg_u8 *marker)
{
    c89jpeg_status st;
    c89jpeg_u8 c;
    do {
        st = c89jpeg_rbr_get_u8(br, &c);
        if (st != C89JPEG_OK) return st;
    } while (c != 0xFF);
    do {
        st = c89jpeg_rbr_get_u8(br, &c);
        if (st != C89JPEG_OK) return st;
    } while (c == 0xFF);
    if (c == 0x00) return C89JPEG_ERR_CORRUPT;
    *marker = c;
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_rbr_fill(c89jpeg_resume_bitreader *br)
{
    while (br->bits <= 24 && br->unread_marker == 0) {
        c89jpeg_status st;
        c89jpeg_u8 c;
        st = c89jpeg_rbr_get_u8(br, &c);
        if (st != C89JPEG_OK) return st;
        if (c == 0xFF) {
            c89jpeg_u8 next;
            do {
                st = c89jpeg_rbr_get_u8(br, &next);
                if (st != C89JPEG_OK) return st;
            } while (next == 0xFF);
            if (next == 0x00) {
                c = 0xFF;
            } else {
                br->unread_marker = (int)next;
                return C89JPEG_OK;
            }
        }
        br->acc = (br->acc << 8) | (c89jpeg_u32)c;
        br->bits += 8;
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_rbr_get_bit(c89jpeg_resume_bitreader *br, int *bit)
{
    c89jpeg_status st;
    if (br->bits == 0) {
        st = c89jpeg_rbr_fill(br);
        if (st != C89JPEG_OK) return st;
        if (br->bits == 0) return br->final_input ? C89JPEG_ERR_CORRUPT : C89JPEG_SUSPENDED;
    }
    *bit = (int)((br->acc >> (br->bits - 1)) & 1UL);
    --br->bits;
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_rbr_get_bits(c89jpeg_resume_bitreader *br, int count, c89jpeg_u32 *value)
{
    c89jpeg_status st;
    c89jpeg_u32 v;
    int bit;
    v = 0;
    while (count > 0) {
        st = c89jpeg_rbr_get_bit(br, &bit);
        if (st != C89JPEG_OK) return st;
        v = (v << 1) | (c89jpeg_u32)bit;
        --count;
    }
    *value = v;
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_resume_huff_decode_symbol(c89jpeg_resume_bitreader *br, const c89jpeg_huffman_table *tab, c89jpeg_u8 *symbol)
{
    c89jpeg_i32 code;
    int bit;
    int i;
    c89jpeg_status st;
    code = 0;
    for (i = 1; i <= 16; ++i) {
        st = c89jpeg_rbr_get_bit(br, &bit);
        if (st != C89JPEG_OK) return st;
        code = (code << 1) | (c89jpeg_i32)bit;
        if (tab->mincode[i] >= 0 && code >= tab->mincode[i] && code <= tab->maxcode[i]) {
            c89jpeg_i32 idx;
            idx = tab->valptr[i] + code - tab->mincode[i];
            if (idx < 0 || idx >= (c89jpeg_i32)tab->count) return C89JPEG_ERR_CORRUPT;
            *symbol = tab->vals[idx];
            return C89JPEG_OK;
        }
    }
    return C89JPEG_ERR_CORRUPT;
}

static c89jpeg_i32 c89jpeg_resume_receive_extend(c89jpeg_resume_bitreader *br, int size, c89jpeg_status *st)
{
    c89jpeg_u32 v;
    if (size == 0) {
        *st = C89JPEG_OK;
        return 0;
    }
    *st = c89jpeg_rbr_get_bits(br, size, &v);
    if (*st != C89JPEG_OK) return 0;
    if (v < ((c89jpeg_u32)1 << (size - 1))) {
        return (c89jpeg_i32)v + 1 - ((c89jpeg_i32)1 << size);
    }
    return (c89jpeg_i32)v;
}

static c89jpeg_status c89jpeg_decode_block_resume(c89jpeg_decoder *dec, c89jpeg_resume_bitreader *br, c89jpeg_component *comp, c89jpeg_i32 *coeff)
{
    c89jpeg_status st;
    c89jpeg_u8 sym;
    c89jpeg_i32 diff;
    int k;
    int idx;
    const c89jpeg_huffman_table *dc_tab;
    const c89jpeg_huffman_table *ac_tab;
    if (comp->dc_table >= C89JPEG_MAX_HUFF_TABLES || comp->ac_table >= C89JPEG_MAX_HUFF_TABLES) return C89JPEG_ERR_UNSUPPORTED;
    dc_tab = &dec->huff[0][comp->dc_table];
    ac_tab = &dec->huff[1][comp->ac_table];
    if (!dc_tab->present || !ac_tab->present) return C89JPEG_ERR_CORRUPT;
    memset(coeff, 0, 64 * sizeof(coeff[0]));
    st = c89jpeg_resume_huff_decode_symbol(br, dc_tab, &sym);
    if (st != C89JPEG_OK) return st;
    diff = c89jpeg_resume_receive_extend(br, sym, &st);
    if (st != C89JPEG_OK) return st;
    comp->pred += diff;
    coeff[0] = comp->pred;
    k = 1;
    while (k < 64) {
        st = c89jpeg_resume_huff_decode_symbol(br, ac_tab, &sym);
        if (st != C89JPEG_OK) return st;
        if (sym == 0) break;
        if (sym == 0xF0) {
            k += 16;
            continue;
        }
        k += (sym >> 4);
        if (k >= 64) return C89JPEG_ERR_CORRUPT;
        idx = c89jpeg_zigzag[k];
        coeff[idx] = c89jpeg_resume_receive_extend(br, sym & 0x0F, &st);
        if (st != C89JPEG_OK) return st;
        ++k;
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_process_restart_resume(c89jpeg_decoder *dec, c89jpeg_resume_bitreader *br, c89jpeg_u8 *next_rst)
{
    c89jpeg_status st;
    c89jpeg_u8 marker;
    int i;
    if (br->unread_marker == 0) {
        st = c89jpeg_rbr_next_marker(br, &marker);
        if (st != C89JPEG_OK) return st;
        br->unread_marker = (int)marker;
    }
    if (br->unread_marker != (int)(C89JPEG_MARKER_RST0 + *next_rst)) return C89JPEG_ERR_CORRUPT;
    br->bits = 0;
    br->acc = 0;
    br->unread_marker = 0;
    *next_rst = (c89jpeg_u8)((*next_rst + 1) & 7);
    for (i = 0; i < dec->info.components; ++i) dec->comp[i].pred = 0;
    return C89JPEG_OK;
}

void c89jpeg_decoder_resume_init(c89jpeg_decoder_resume *state)
{
    if (state == 0) return;
    memset(state, 0, sizeof(*state));
    state->last_error = C89JPEG_OK;
}

static void c89jpeg_resume_apply_preds(c89jpeg_decoder_resume *state)
{
    int i;
    for (i = 0; i < state->decoder->info.components; ++i) {
        state->decoder->comp[i].pred = state->comp_pred[i];
    }
}

static void c89jpeg_resume_store_preds(c89jpeg_decoder_resume *state)
{
    int i;
    for (i = 0; i < state->decoder->info.components; ++i) {
        state->comp_pred[i] = state->decoder->comp[i].pred;
    }
}

static c89jpeg_status c89jpeg_resume_prepare(c89jpeg_decoder_resume *state)
{
    c89jpeg_status st;
    c89jpeg_decoder *dec;
    int i;

    dec = state->decoder;
    if (dec == 0) return C89JPEG_ERR_BAD_ARG;
    if (dec->info.components == 3 && dec->scan_count != 3) return C89JPEG_ERR_UNSUPPORTED;

    state->mcu_w = (c89jpeg_u16)(dec->info.max_h_samp * 8);
    state->mcu_h = (c89jpeg_u16)(dec->info.max_v_samp * 8);
    st = c89jpeg_resolve_roi(&dec->info, &state->requested_roi, state->roi_mode, &state->region);
    if (st != C89JPEG_OK) return st;

    state->out_channels = c89jpeg_output_channels(state->out_format, dec->info.components);
    state->row_bytes = c89jpeg_decoder_row_stride(state->region.width, state->out_format, dec->info.components);
    state->strip_need = 0;

    if (state->output_mode == C89JPEG_RESUME_OUTPUT_SINK) {
        if (state->strip_buffer == 0) return C89JPEG_ERR_BAD_ARG;
        if (!c89jpeg_mul_u32(state->row_bytes, (c89jpeg_u32)state->mcu_h, &state->strip_need)) return C89JPEG_ERR_LIMIT;
        if (state->strip_buffer_size < state->strip_need) return C89JPEG_ERR_SHORT_BUFFER;
    } else {
        c89jpeg_u32 needed;
        if (state->out_pixels == 0) return C89JPEG_ERR_BAD_ARG;
        if (state->out_stride == 0) state->out_stride = state->row_bytes;
        if (!c89jpeg_mul_u32(state->out_stride, state->region.height, &needed)) return C89JPEG_ERR_LIMIT;
        if (state->out_capacity < needed) return C89JPEG_ERR_SHORT_BUFFER;
    }

    for (i = 0; i < dec->info.components; ++i) {
        if (!dec->quant_present[dec->comp[i].tq]) return C89JPEG_ERR_CORRUPT;
        if (dec->comp[i].dc_table >= C89JPEG_MAX_HUFF_TABLES || dec->comp[i].ac_table >= C89JPEG_MAX_HUFF_TABLES) {
            return C89JPEG_ERR_UNSUPPORTED;
        }
    }

    state->mcu_cols = (c89jpeg_u16)((dec->info.width + state->mcu_w - 1) / state->mcu_w);
    state->mcu_rows = (c89jpeg_u16)((dec->info.height + state->mcu_h - 1) / state->mcu_h);
    state->my = 0;
    state->mx = 0;
    state->restart_left = dec->info.restart_interval;
    state->next_rst = 0;
    state->scan_pos = dec->entropy_offset;
    state->scan_acc = 0;
    state->scan_bits = 0;
    state->scan_unread_marker = 0;
    for (i = 0; i < dec->info.components; ++i) state->comp_pred[i] = 0;
    state->info = dec->info;
    state->prepared = 1;
    return C89JPEG_OK;
}

c89jpeg_status c89jpeg_decoder_resume_begin(c89jpeg_decoder_resume *state, c89jpeg_decoder *dec, const c89jpeg_decode_resume_params *params)
{
    if (state == 0 || dec == 0 || params == 0 || params->input_storage == 0 || params->input_capacity == 0) return C89JPEG_ERR_BAD_ARG;
    c89jpeg_decoder_resume_init(state);
    state->decoder = dec;
    state->input_storage = params->input_storage;
    state->input_capacity = params->input_capacity;
    state->out_pixels = params->out_pixels;
    state->out_capacity = params->out_capacity;
    state->out_stride = params->out_stride;
    state->strip_buffer = params->strip_buffer;
    state->strip_buffer_size = params->strip_buffer_size;
    state->out_format = params->out_format;
    state->upsampling = params->upsampling;
    state->requested_roi = params->roi;
    state->roi_mode = params->roi_mode;
    state->output_mode = params->output_mode;
    return C89JPEG_OK;
}

c89jpeg_status c89jpeg_decoder_resume_feed(c89jpeg_decoder_resume *state, const c89jpeg_u8 *data, c89jpeg_u32 size, int final_chunk)
{
    if (state == 0 || state->input_storage == 0) return C89JPEG_ERR_BAD_ARG;
    if (state->final_input && (size != 0 || final_chunk)) return C89JPEG_ERR_BAD_ARG;
    if (size != 0 && data == 0) return C89JPEG_ERR_BAD_ARG;
    if (size > state->input_capacity - state->input_size) return C89JPEG_ERR_SHORT_BUFFER;
    if (size != 0) {
        memcpy(state->input_storage + state->input_size, data, (size_t)size);
        state->input_size += size;
    }
    if (final_chunk) state->final_input = 1;
    state->last_error = C89JPEG_OK;
    return C89JPEG_OK;
}

int c89jpeg_decoder_resume_is_finished(const c89jpeg_decoder_resume *state)
{
    if (state == 0) return 0;
    return state->finished ? 1 : 0;
}

c89jpeg_status c89jpeg_decoder_resume_run(c89jpeg_decoder_resume *state, c89jpeg_row_sink_fn row_fn, void *row_user, c89jpeg_image_info *out_info)
{
    c89jpeg_status st;
    c89jpeg_decoder *dec;

    if (state == 0 || state->decoder == 0) return C89JPEG_ERR_BAD_ARG;
    dec = state->decoder;
    if (state->output_mode == C89JPEG_RESUME_OUTPUT_SINK && row_fn == 0) return C89JPEG_ERR_BAD_ARG;
    if (state->finished) {
        if (out_info != 0) *out_info = state->info;
        state->last_error = C89JPEG_OK;
        return C89JPEG_OK;
    }

    if (!state->headers_ready) {
        if (state->input_size == 0) {
            state->last_error = C89JPEG_SUSPENDED;
            return C89JPEG_SUSPENDED;
        }
        st = c89jpeg_parse_headers(dec, state->input_storage, state->input_size);
        if (st != C89JPEG_OK) {
            if (st == C89JPEG_ERR_CORRUPT && !state->final_input) {
                state->last_error = C89JPEG_SUSPENDED;
                return C89JPEG_SUSPENDED;
            }
            state->last_error = st;
            return st;
        }
        state->headers_ready = 1;
        state->info = dec->info;
    }

    if (!state->prepared) {
        st = c89jpeg_resume_prepare(state);
        if (st != C89JPEG_OK) {
            state->last_error = st;
            return st;
        }
    }

    if (out_info != 0) *out_info = state->info;

    while (state->my < state->mcu_rows) {
        c89jpeg_u32 row_start;
        c89jpeg_u32 row_end;
        int row_intersects;

        row_start = (c89jpeg_u32)state->my * state->mcu_h;
        row_end = row_start + state->mcu_h;
        row_intersects = (row_start < (c89jpeg_u32)state->region.y + state->region.height && row_end > state->region.y) ? C89JPEG_TRUE : C89JPEG_FALSE;

        if (state->output_mode == C89JPEG_RESUME_OUTPUT_SINK && row_intersects && state->mx == 0) {
            memset(state->strip_buffer, 0, (size_t)state->strip_need);
        }

        while (state->mx < state->mcu_cols) {
            c89jpeg_resume_bitreader br;
            c89jpeg_u16 restart_left;
            c89jpeg_u8 next_rst;
            int render_this_mcu;
            int block_index;
            int i;

            c89jpeg_resume_apply_preds(state);
            c89jpeg_rbr_init(&br,
                             state->input_storage,
                             state->input_size,
                             state->scan_pos,
                             state->scan_acc,
                             state->scan_bits,
                             state->scan_unread_marker,
                             state->final_input ? C89JPEG_TRUE : C89JPEG_FALSE);
            restart_left = state->restart_left;
            next_rst = state->next_rst;
            render_this_mcu = c89jpeg_mcu_intersects_region(state->mx, state->my, state->mcu_w, state->mcu_h, &state->region);
            block_index = 0;

            for (i = 0; i < dec->scan_count; ++i) {
                c89jpeg_component *comp;
                c89jpeg_u8 by;
                c89jpeg_u8 bx;
                comp = dec->scan_comp[i];
                for (by = 0; by < comp->v_samp; ++by) {
                    for (bx = 0; bx < comp->h_samp; ++bx) {
                        st = c89jpeg_decode_block_resume(dec, &br, comp, dec->blocks[block_index]);
                        if (st != C89JPEG_OK) {
                            if (st == C89JPEG_SUSPENDED) {
                                state->last_error = st;
                                return st;
                            }
                            state->last_error = st;
                            return st;
                        }
                        if (render_this_mcu) {
                            int q;
                            for (q = 0; q < 64; ++q) dec->blocks[block_index][q] *= dec->quant[comp->tq][q];
                            c89jpeg_idct8x8(dec->blocks[block_index], dec->idct_tmp, dec->samples[block_index]);
                        }
                        ++block_index;
                    }
                }
            }

            if (render_this_mcu) {
                st = c89jpeg_render_mcu_region(dec,
                                               state->mx,
                                               state->my,
                                               state->mcu_w,
                                               state->mcu_h,
                                               state->out_format,
                                               state->upsampling,
                                               state->out_channels,
                                               (state->output_mode == C89JPEG_RESUME_OUTPUT_SINK) ? state->strip_buffer : state->out_pixels,
                                               (state->output_mode == C89JPEG_RESUME_OUTPUT_SINK) ? state->row_bytes : state->out_stride,
                                               &state->region,
                                               (state->output_mode == C89JPEG_RESUME_OUTPUT_SINK) ? C89JPEG_TRUE : C89JPEG_FALSE);
                if (st != C89JPEG_OK) {
                    state->last_error = st;
                    return st;
                }
            }

            if (dec->info.restart_interval != 0) {
                if (--restart_left == 0) {
                    st = c89jpeg_process_restart_resume(dec, &br, &next_rst);
                    if (st != C89JPEG_OK) {
                        if (st == C89JPEG_SUSPENDED) {
                            state->last_error = st;
                            return st;
                        }
                        state->last_error = st;
                        return st;
                    }
                    restart_left = dec->info.restart_interval;
                }
            }

            state->scan_pos = br.pos;
            state->scan_acc = br.acc;
            state->scan_bits = br.bits;
            state->scan_unread_marker = br.unread_marker;
            state->restart_left = restart_left;
            state->next_rst = next_rst;
            c89jpeg_resume_store_preds(state);
            ++state->mx;
        }

        if (state->output_mode == C89JPEG_RESUME_OUTPUT_SINK && row_intersects) {
            c89jpeg_u32 clip_y0;
            c89jpeg_u32 clip_y1;
            int i;
            clip_y0 = (row_start > state->region.y) ? row_start : state->region.y;
            clip_y1 = (row_end < (c89jpeg_u32)state->region.y + state->region.height) ? row_end : (c89jpeg_u32)state->region.y + state->region.height;
            for (i = (int)clip_y0; i < (int)clip_y1; ++i) {
                c89jpeg_u32 row_index;
                c89jpeg_u32 local_row;
                row_index = (c89jpeg_u32)i - state->region.y;
                local_row = (c89jpeg_u32)i - row_start;
                if (!row_fn(row_user, row_index, state->strip_buffer + local_row * state->row_bytes, state->row_bytes)) {
                    state->last_error = C89JPEG_ERR_IO;
                    return C89JPEG_ERR_IO;
                }
            }
        }

        state->mx = 0;
        ++state->my;
    }

    state->finished = 1;
    state->last_error = C89JPEG_OK;
    if (out_info != 0) *out_info = state->info;
    return C89JPEG_OK;
}


static c89jpeg_subsampling c89jpeg_normalize_session_subsampling(c89jpeg_pixel_format pixel_format,
                                                                 c89jpeg_subsampling subsampling)
{
    if (pixel_format == C89JPEG_PIXFMT_GRAY8) return C89JPEG_SUBSAMP_444;
    return subsampling;
}

#define C89JPEG_ENCRES_MIN_WINDOW 1024UL
#define C89JPEG_ENCRES_PHASE_SOI 1
#define C89JPEG_ENCRES_PHASE_APP0 2
#define C89JPEG_ENCRES_PHASE_DQT0 3
#define C89JPEG_ENCRES_PHASE_DQT1 4
#define C89JPEG_ENCRES_PHASE_SOF0 5
#define C89JPEG_ENCRES_PHASE_DHT_DC0 6
#define C89JPEG_ENCRES_PHASE_DHT_DC1 7
#define C89JPEG_ENCRES_PHASE_DHT_AC0 8
#define C89JPEG_ENCRES_PHASE_DHT_AC1 9
#define C89JPEG_ENCRES_PHASE_DRI 10
#define C89JPEG_ENCRES_PHASE_SOS 11
#define C89JPEG_ENCRES_PHASE_SCAN 12
#define C89JPEG_ENCRES_PHASE_RESTART 13
#define C89JPEG_ENCRES_PHASE_ECS_FLUSH 14
#define C89JPEG_ENCRES_PHASE_EOI 15
#define C89JPEG_ENCRES_PHASE_DONE 16

typedef struct c89jpeg_count_dest_tag {
    c89jpeg_u32 total;
} c89jpeg_count_dest;

static int c89jpeg_count_dest_write(void *user, const c89jpeg_u8 *data, c89jpeg_u32 size)
{
    c89jpeg_count_dest *dest;
    (void)data;
    if (user == 0) return C89JPEG_FALSE;
    dest = (c89jpeg_count_dest *)user;
    if (size > 0xFFFFFFFFUL - dest->total) return C89JPEG_FALSE;
    dest->total += size;
    return C89JPEG_TRUE;
}

static c89jpeg_status c89jpeg_measure_encode_size(const c89jpeg_encode_params *params,
                                                  const c89jpeg_tables *tables,
                                                  int abbreviated,
                                                  c89jpeg_u32 *out_size)
{
    c89jpeg_encoder enc;
    c89jpeg_count_dest dest;
    c89jpeg_status st;
    if (params == 0 || out_size == 0) return C89JPEG_ERR_BAD_ARG;
    c89jpeg_encoder_init(&enc);
    dest.total = 0;
    if (abbreviated) {
        if (tables == 0) return C89JPEG_ERR_BAD_ARG;
        st = c89jpeg_encode_abbreviated_sink(&enc, params, tables, c89jpeg_count_dest_write, &dest, 0);
    } else {
        st = c89jpeg_encode_sink(&enc, params, c89jpeg_count_dest_write, &dest, 0);
    }
    if (st != C89JPEG_OK) return st;
    *out_size = dest.total;
    return C89JPEG_OK;
}

static c89jpeg_u32 c89jpeg_source_resume_replay_need(const c89jpeg_encode_source_resume_params *params)
{
    c89jpeg_u32 need;
    if (params == 0) return 0;
    if (!c89jpeg_mul_u32(params->stride_bytes, (c89jpeg_u32)params->height, &need)) return 0;
    return need;
}

static int c89jpeg_source_resume_uses_replay(const c89jpeg_encode_source_resume_params *params)
{
    if (params == 0) return C89JPEG_FALSE;
    if (params->abbreviated) return C89JPEG_FALSE;
    return (params->huffman_mode == C89JPEG_HUFFMAN_OPTIMAL ||
            params->huffman_mode == C89JPEG_HUFFMAN_CUSTOM) ? C89JPEG_TRUE : C89JPEG_FALSE;
}

static void c89jpeg_source_resume_to_encode_params(const c89jpeg_encode_source_resume_params *src,
                                                   c89jpeg_encode_params *dst)
{
    if (dst == 0) return;
    memset(dst, 0, sizeof(*dst));
    if (src == 0) return;
    dst->width = src->width;
    dst->height = src->height;
    dst->stride_bytes = src->stride_bytes;
    dst->pixel_format = src->pixel_format;
    dst->subsampling = src->subsampling;
    dst->quality = src->quality;
    dst->restart_interval = src->restart_interval;
    dst->emit_jfif = src->emit_jfif;
    dst->density_units = src->density_units;
    dst->density_x = src->density_x;
    dst->density_y = src->density_y;
    dst->quant_luma = src->quant_luma;
    dst->quant_chroma = src->quant_chroma;
    dst->huffman_mode = src->huffman_mode;
    dst->custom_dc_luma = src->custom_dc_luma;
    dst->custom_ac_luma = src->custom_ac_luma;
    dst->custom_dc_chroma = src->custom_dc_chroma;
    dst->custom_ac_chroma = src->custom_ac_chroma;
}

static void c89jpeg_source_resume_to_source_params(const c89jpeg_encode_source_resume_params *src,
                                                   c89jpeg_encode_source_params *dst)
{
    if (dst == 0) return;
    memset(dst, 0, sizeof(*dst));
    if (src == 0) return;
    dst->width = src->width;
    dst->height = src->height;
    dst->stride_bytes = src->stride_bytes;
    dst->pixel_format = src->pixel_format;
    dst->subsampling = src->subsampling;
    dst->quality = src->quality;
    dst->restart_interval = src->restart_interval;
    dst->emit_jfif = src->emit_jfif;
    dst->density_units = src->density_units;
    dst->density_x = src->density_x;
    dst->density_y = src->density_y;
    dst->quant_luma = src->quant_luma;
    dst->quant_chroma = src->quant_chroma;
    dst->huffman_mode = src->huffman_mode;
    dst->custom_dc_luma = src->custom_dc_luma;
    dst->custom_ac_luma = src->custom_ac_luma;
    dst->custom_dc_chroma = src->custom_dc_chroma;
    dst->custom_ac_chroma = src->custom_ac_chroma;
    dst->row_cache = src->row_window;
    dst->row_cache_size = src->row_window_size;
}

static c89jpeg_status c89jpeg_validate_encode_source_resume_params(const c89jpeg_encode_source_resume_params *params)
{
    c89jpeg_u32 min_stride;
    c89jpeg_u32 cache_need;
    c89jpeg_u32 replay_need;
    c89jpeg_status st;
    if (params == 0 || params->row_window == 0 || params->staging_buffer == 0) return C89JPEG_ERR_BAD_ARG;
    if (params->width == 0 || params->height == 0) return C89JPEG_ERR_BAD_ARG;
    if (params->pixel_format != C89JPEG_PIXFMT_GRAY8 && params->pixel_format != C89JPEG_PIXFMT_RGB24) return C89JPEG_ERR_BAD_ARG;
    if (params->emit_chunk_size == 0 || params->staging_capacity == 0) return C89JPEG_ERR_BAD_ARG;
    if (params->staging_capacity < C89JPEG_ENCRES_MIN_WINDOW) return C89JPEG_ERR_SHORT_BUFFER;
    if (params->huffman_mode != C89JPEG_HUFFMAN_DEFAULT &&
        params->huffman_mode != C89JPEG_HUFFMAN_OPTIMAL &&
        params->huffman_mode != C89JPEG_HUFFMAN_CUSTOM) return C89JPEG_ERR_BAD_ARG;
    st = c89jpeg_source_min_stride(params->width, params->pixel_format, &min_stride);
    if (st != C89JPEG_OK) return st;
    if (params->stride_bytes < min_stride) return C89JPEG_ERR_BAD_ARG;
    cache_need = c89jpeg_encoder_source_row_cache_size(params->width,
                                                       params->pixel_format,
                                                       params->subsampling,
                                                       params->stride_bytes);
    if (cache_need == 0) return C89JPEG_ERR_LIMIT;
    if (params->row_window_size < cache_need) return C89JPEG_ERR_SHORT_BUFFER;
    if (params->abbreviated && params->tables == 0) return C89JPEG_ERR_BAD_ARG;
    if (c89jpeg_source_resume_uses_replay(params)) {
        replay_need = c89jpeg_source_resume_replay_need(params);
        if (replay_need == 0) return C89JPEG_ERR_LIMIT;
        if (params->replay_buffer == 0 || params->replay_buffer_size < replay_need) return C89JPEG_ERR_SHORT_BUFFER;
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_encoder_apply_tables_resume_source(c89jpeg_encoder *enc,
                                                                 const c89jpeg_tables *tables,
                                                                 const c89jpeg_encode_params *params,
                                                                 int *components,
                                                                 c89jpeg_u8 *max_h,
                                                                 c89jpeg_u8 *max_v,
                                                                 c89jpeg_u16 *restart_interval)
{
    int ci;
    if (enc == 0 || tables == 0 || params == 0 || components == 0 || max_h == 0 || max_v == 0 || restart_interval == 0) return C89JPEG_ERR_BAD_ARG;
    c89jpeg_encoder_init(enc);
    c89jpeg_setup_encoder_components(enc, params, components, max_h, max_v);
    if (!tables->quant_present[0]) return C89JPEG_ERR_BAD_ARG;
    memcpy(enc->qtable[0], tables->quant[0], sizeof(enc->qtable[0]));
    if (*components > 1) {
        if (!tables->quant_present[1]) return C89JPEG_ERR_BAD_ARG;
        memcpy(enc->qtable[1], tables->quant[1], sizeof(enc->qtable[1]));
    }
    if (!tables->huff[0][0].present || !tables->huff[1][0].present) return C89JPEG_ERR_BAD_ARG;
    enc->huff_dc[0] = tables->huff[0][0];
    enc->huff_ac[0] = tables->huff[1][0];
    if (*components > 1) {
        if (!tables->huff[0][1].present || !tables->huff[1][1].present) return C89JPEG_ERR_BAD_ARG;
        enc->huff_dc[1] = tables->huff[0][1];
        enc->huff_ac[1] = tables->huff[1][1];
    }
    for (ci = 0; ci < *components; ++ci) enc->comp[ci].pred = 0;
    *restart_interval = tables->restart_interval;
    return C89JPEG_OK;
}

static int c89jpeg_encres_replay_row_read(void *user,
                                          c89jpeg_u32 row_index,
                                          c89jpeg_u8 *row,
                                          c89jpeg_u32 row_size)
{
    c89jpeg_encoder_resume *state;
    if (user == 0 || row == 0) return C89JPEG_FALSE;
    state = (c89jpeg_encoder_resume *)user;
    if (state->source_replay_buffer == 0) return C89JPEG_FALSE;
    if (row_index >= (c89jpeg_u32)state->params.height) return C89JPEG_FALSE;
    if (row_index >= (c89jpeg_u32)state->source_next_row) return C89JPEG_FALSE;
    if (row_size > state->params.stride_bytes) return C89JPEG_FALSE;
    memcpy(row,
           state->source_replay_buffer + row_index * state->params.stride_bytes,
           (size_t)row_size);
    return C89JPEG_TRUE;
}

static void c89jpeg_encres_make_replay_source_params(const c89jpeg_encoder_resume *state,
                                                     c89jpeg_encode_source_params *params,
                                                     c89jpeg_encode_row_cache *cache)
{
    if (params != 0) {
        memset(params, 0, sizeof(*params));
        params->width = state->params.width;
        params->height = state->params.height;
        params->stride_bytes = state->params.stride_bytes;
        params->pixel_format = state->params.pixel_format;
        params->subsampling = state->params.subsampling;
        params->quality = state->params.quality;
        params->restart_interval = state->restart_interval;
        params->emit_jfif = state->params.emit_jfif;
        params->density_units = state->params.density_units;
        params->density_x = state->params.density_x;
        params->density_y = state->params.density_y;
        params->quant_luma = state->params.quant_luma;
        params->quant_chroma = state->params.quant_chroma;
        params->huffman_mode = state->params.huffman_mode;
        params->custom_dc_luma = state->params.custom_dc_luma;
        params->custom_ac_luma = state->params.custom_ac_luma;
        params->custom_dc_chroma = state->params.custom_dc_chroma;
        params->custom_ac_chroma = state->params.custom_ac_chroma;
        params->read_row_fn = c89jpeg_encres_replay_row_read;
        params->read_user = (void *)state;
        params->row_cache = state->source_row_window;
        params->row_cache_size = state->source_row_window_size;
    }
    if (cache != 0) {
        cache->params = params;
        cache->base_y = 0;
        cache->mcu_h = state->mcu_h;
        cache->loaded = 0;
    }
}

static c89jpeg_status c89jpeg_encres_prepare_replay_tables(c89jpeg_encoder_resume *state)
{
    c89jpeg_encode_source_params sp;
    c89jpeg_status st;
    if (state == 0 || !state->source_replay_enabled || state->source_replay_ready) return C89JPEG_OK;
    if ((c89jpeg_u32)state->source_next_row < (c89jpeg_u32)state->params.height) {
        if (state->source_input_final) return C89JPEG_ERR_BAD_ARG;
        return C89JPEG_SUSPENDED;
    }
    c89jpeg_encres_make_replay_source_params(state, &sp, 0);
    if (state->params.huffman_mode == C89JPEG_HUFFMAN_OPTIMAL) {
        st = c89jpeg_gather_huffman_stats_source(state->encoder,
                                                 &sp,
                                                 (int)state->components,
                                                 state->max_h,
                                                 state->max_v);
        if (st != C89JPEG_OK) return st;
    } else if (state->params.huffman_mode == C89JPEG_HUFFMAN_CUSTOM) {
        st = c89jpeg_collect_huffman_stats_source(state->encoder,
                                                  &sp,
                                                  (int)state->components,
                                                  state->max_h,
                                                  state->max_v);
        if (st != C89JPEG_OK) return st;
        st = c89jpeg_apply_custom_huffman_tables(state->encoder,
                                                 &state->params,
                                                 (int)state->components);
        if (st != C89JPEG_OK) return st;
        st = c89jpeg_validate_huffman_table_usage(&state->encoder->huff_dc[0], state->encoder->huff_freq_dc[0]);
        if (st != C89JPEG_OK) return st;
        st = c89jpeg_validate_huffman_table_usage(&state->encoder->huff_ac[0], state->encoder->huff_freq_ac[0]);
        if (st != C89JPEG_OK) return st;
        if (state->components > 1) {
            st = c89jpeg_validate_huffman_table_usage(&state->encoder->huff_dc[1], state->encoder->huff_freq_dc[1]);
            if (st != C89JPEG_OK) return st;
            st = c89jpeg_validate_huffman_table_usage(&state->encoder->huff_ac[1], state->encoder->huff_freq_ac[1]);
            if (st != C89JPEG_OK) return st;
        }
    }
    state->source_replay_ready = 1;
    state->source_base_y = 0;
    state->source_rows_loaded = 0;
    state->source_window_padded = 0;
    c89jpeg_encres_reset_preds(state);
    return C89JPEG_OK;
}

static void c89jpeg_encres_make_source_params(const c89jpeg_encoder_resume *state,
                                              c89jpeg_encode_source_params *params,
                                              c89jpeg_encode_row_cache *cache)
{
    if (params != 0) {
        memset(params, 0, sizeof(*params));
        params->width = state->params.width;
        params->height = state->params.height;
        params->stride_bytes = state->params.stride_bytes;
        params->pixel_format = state->params.pixel_format;
        params->subsampling = state->params.subsampling;
        params->quality = state->params.quality;
        params->restart_interval = state->restart_interval;
        params->emit_jfif = state->params.emit_jfif;
        params->density_units = state->params.density_units;
        params->density_x = state->params.density_x;
        params->density_y = state->params.density_y;
        params->quant_luma = state->params.quant_luma;
        params->quant_chroma = state->params.quant_chroma;
        params->huffman_mode = C89JPEG_HUFFMAN_DEFAULT;
        params->row_cache = state->source_row_window;
        params->row_cache_size = state->source_row_window_size;
    }
    if (cache != 0) {
        cache->params = params;
        cache->base_y = state->source_base_y;
        cache->mcu_h = state->mcu_h;
        cache->loaded = 1;
    }
}

static c89jpeg_u16 c89jpeg_encres_source_row_goal(const c89jpeg_encoder_resume *state)
{
    c89jpeg_u32 remain;
    if (state == 0) return 0;
    if (state->source_base_y >= state->params.height) return 0;
    remain = (c89jpeg_u32)state->params.height - (c89jpeg_u32)state->source_base_y;
    if (remain > state->mcu_h) remain = state->mcu_h;
    return (c89jpeg_u16)remain;
}

static c89jpeg_status c89jpeg_encres_source_prepare_window(c89jpeg_encoder_resume *state)
{
    c89jpeg_u16 expected_base;
    c89jpeg_u16 row_goal;
    c89jpeg_u16 row;
    c89jpeg_u8 *dst;
    const c89jpeg_u8 *src;
    if (state == 0 || !state->source_mode) return C89JPEG_OK;
    if (state->source_replay_enabled) {
        c89jpeg_encode_source_params replay_params;
        c89jpeg_encode_row_cache replay_cache;
        c89jpeg_status st;
        c89jpeg_encres_make_replay_source_params(state, &replay_params, &replay_cache);
        st = c89jpeg_encode_row_cache_load(&replay_cache, state->mcu_y);
        if (st != C89JPEG_OK) return st;
        state->source_base_y = replay_cache.base_y;
        state->source_rows_loaded = state->mcu_h;
        state->source_window_padded = 1;
        return C89JPEG_OK;
    }
    expected_base = (c89jpeg_u16)(state->mcu_y * state->mcu_h);
    if (state->source_base_y != expected_base) {
        state->source_base_y = expected_base;
        state->source_rows_loaded = 0;
        state->source_window_padded = 0;
    }
    row_goal = c89jpeg_encres_source_row_goal(state);
    if (row_goal == 0) return C89JPEG_OK;
    if (state->source_rows_loaded < row_goal) {
        if (state->source_input_final) return C89JPEG_ERR_BAD_ARG;
        return C89JPEG_SUSPENDED;
    }
    if (!state->source_window_padded && state->source_rows_loaded < state->mcu_h) {
        if (state->source_rows_loaded == 0) return C89JPEG_ERR_BAD_ARG;
        src = state->source_row_window + ((c89jpeg_u32)(state->source_rows_loaded - 1) * state->params.stride_bytes);
        for (row = state->source_rows_loaded; row < state->mcu_h; ++row) {
            dst = state->source_row_window + ((c89jpeg_u32)row * state->params.stride_bytes);
            memcpy(dst, src, (size_t)state->params.stride_bytes);
        }
        state->source_rows_loaded = state->mcu_h;
        state->source_window_padded = 1;
    }
    return C89JPEG_OK;
}

static c89jpeg_u32 c89jpeg_encres_buffered(const c89jpeg_encoder_resume *state)
{
    if (state == 0 || state->staging_size < state->emit_offset) return 0;
    return state->staging_size - state->emit_offset;
}

static void c89jpeg_encres_reset_buffer(c89jpeg_encoder_resume *state)
{
    if (state == 0) return;
    state->staging_size = 0;
    state->emit_offset = 0;
}

static c89jpeg_status c89jpeg_encres_put_raw_byte(c89jpeg_encoder_resume *state, c89jpeg_u8 b)
{
    if (state == 0 || state->staging_buffer == 0) return C89JPEG_ERR_BAD_ARG;
    if (state->staging_size >= state->staging_capacity) return C89JPEG_ERR_SHORT_BUFFER;
    state->staging_buffer[state->staging_size++] = b;
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_encres_put_byte(c89jpeg_encoder_resume *state, c89jpeg_u8 b)
{
    c89jpeg_status st;
    st = c89jpeg_encres_put_raw_byte(state, b);
    if (st != C89JPEG_OK) return st;
    if (b == 0xFF) {
        st = c89jpeg_encres_put_raw_byte(state, 0x00);
        if (st != C89JPEG_OK) return st;
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_encres_put_marker(c89jpeg_encoder_resume *state, c89jpeg_u8 marker)
{
    c89jpeg_status st;
    st = c89jpeg_encres_put_raw_byte(state, 0xFF);
    if (st != C89JPEG_OK) return st;
    return c89jpeg_encres_put_raw_byte(state, marker);
}

static c89jpeg_status c89jpeg_encres_put_segment(c89jpeg_encoder_resume *state,
                                                 c89jpeg_u8 marker,
                                                 const c89jpeg_u8 *payload,
                                                 c89jpeg_u16 payload_len)
{
    c89jpeg_u8 lenbuf[2];
    c89jpeg_u16 i;
    c89jpeg_status st;
    st = c89jpeg_encres_put_marker(state, marker);
    if (st != C89JPEG_OK) return st;
    c89jpeg_write_be16(lenbuf, (c89jpeg_u16)(payload_len + 2));
    st = c89jpeg_encres_put_raw_byte(state, lenbuf[0]);
    if (st != C89JPEG_OK) return st;
    st = c89jpeg_encres_put_raw_byte(state, lenbuf[1]);
    if (st != C89JPEG_OK) return st;
    for (i = 0; i < payload_len; ++i) {
        st = c89jpeg_encres_put_raw_byte(state, payload[i]);
        if (st != C89JPEG_OK) return st;
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_encres_put_bits(c89jpeg_encoder_resume *state, c89jpeg_u32 code, int size)
{
    c89jpeg_status st;
    while (size > 0) {
        --size;
        state->bw_acc = (state->bw_acc << 1) | ((code >> size) & 1UL);
        ++state->bw_bits;
        if (state->bw_bits == 8) {
            st = c89jpeg_encres_put_byte(state, (c89jpeg_u8)(state->bw_acc & 0xFFUL));
            if (st != C89JPEG_OK) return st;
            state->bw_bits = 0;
            state->bw_acc = 0;
        }
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_encres_flush_pad1(c89jpeg_encoder_resume *state)
{
    c89jpeg_u32 code;
    int pad;
    if (state == 0) return C89JPEG_ERR_BAD_ARG;
    if (state->bw_bits != 0) {
        pad = 8 - state->bw_bits;
        code = ((c89jpeg_u32)1 << pad) - 1UL;
        return c89jpeg_encres_put_bits(state, code, pad);
    }
    return C89JPEG_OK;
}

static c89jpeg_status c89jpeg_encres_write_app0(c89jpeg_encoder_resume *state, const c89jpeg_encode_params *params)
{
    c89jpeg_u8 payload[14];
    memset(payload, 0, sizeof(payload));
    payload[0] = 'J';
    payload[1] = 'F';
    payload[2] = 'I';
    payload[3] = 'F';
    payload[4] = 0;
    payload[5] = 1;
    payload[6] = 2;
    payload[7] = params->density_units;
    c89jpeg_write_be16(payload + 8, params->density_x ? params->density_x : 1);
    c89jpeg_write_be16(payload + 10, params->density_y ? params->density_y : 1);
    payload[12] = 0;
    payload[13] = 0;
    return c89jpeg_encres_put_segment(state, C89JPEG_MARKER_APP0, payload, (c89jpeg_u16)sizeof(payload));
}

static c89jpeg_status c89jpeg_encres_write_dqt(c89jpeg_encoder_resume *state, c89jpeg_u8 table_id, const c89jpeg_u16 *table)
{
    c89jpeg_u8 payload[65];
    int i;
    payload[0] = table_id;
    for (i = 0; i < 64; ++i) payload[1 + i] = (c89jpeg_u8)table[c89jpeg_zigzag[i]];
    return c89jpeg_encres_put_segment(state, C89JPEG_MARKER_DQT, payload, (c89jpeg_u16)sizeof(payload));
}

static c89jpeg_status c89jpeg_encres_write_dht(c89jpeg_encoder_resume *state, c89jpeg_u8 tc, c89jpeg_u8 th, const c89jpeg_huffman_table *tab)
{
    c89jpeg_u8 payload[1 + 16 + 256];
    c89jpeg_u16 len;
    payload[0] = (c89jpeg_u8)((tc << 4) | th);
    memcpy(payload + 1, tab->bits, 16);
    memcpy(payload + 17, tab->vals, tab->count);
    len = (c89jpeg_u16)(17 + tab->count);
    return c89jpeg_encres_put_segment(state, C89JPEG_MARKER_DHT, payload, len);
}

static c89jpeg_status c89jpeg_encres_write_dri(c89jpeg_encoder_resume *state, c89jpeg_u16 restart_interval)
{
    c89jpeg_u8 payload[2];
    c89jpeg_write_be16(payload, restart_interval);
    return c89jpeg_encres_put_segment(state, C89JPEG_MARKER_DRI, payload, 2);
}

static c89jpeg_status c89jpeg_encres_write_sof0(c89jpeg_encoder_resume *state,
                                                const c89jpeg_encode_params *params,
                                                const c89jpeg_component *comp,
                                                int components)
{
    c89jpeg_u8 payload[32];
    c89jpeg_u16 len;
    int i;
    payload[0] = 8;
    c89jpeg_write_be16(payload + 1, params->height);
    c89jpeg_write_be16(payload + 3, params->width);
    payload[5] = (c89jpeg_u8)components;
    len = 6;
    for (i = 0; i < components; ++i) {
        payload[len++] = comp[i].id;
        payload[len++] = (c89jpeg_u8)((comp[i].h_samp << 4) | comp[i].v_samp);
        payload[len++] = comp[i].tq;
    }
    return c89jpeg_encres_put_segment(state, C89JPEG_MARKER_SOF0, payload, len);
}

static c89jpeg_status c89jpeg_encres_write_sos(c89jpeg_encoder_resume *state, const c89jpeg_component *comp, int components)
{
    c89jpeg_u8 payload[16];
    c89jpeg_u16 len;
    int i;
    payload[0] = (c89jpeg_u8)components;
    len = 1;
    for (i = 0; i < components; ++i) {
        payload[len++] = comp[i].id;
        payload[len++] = (c89jpeg_u8)((comp[i].dc_table << 4) | comp[i].ac_table);
    }
    payload[len++] = 0;
    payload[len++] = 63;
    payload[len++] = 0;
    return c89jpeg_encres_put_segment(state, C89JPEG_MARKER_SOS, payload, len);
}

static c89jpeg_status c89jpeg_emit_block_resume(c89jpeg_encoder *enc,
                                                c89jpeg_encoder_resume *state,
                                                const c89jpeg_component *comp)
{
    c89jpeg_i32 dc;
    c89jpeg_i32 diff;
    int cat;
    int k;
    int run;
    const c89jpeg_huffman_table *dc_tab;
    const c89jpeg_huffman_table *ac_tab;
    const c89jpeg_u16 *qt;
    c89jpeg_status st;
    qt = enc->qtable[comp->tq];
    dc_tab = &enc->huff_dc[comp->dc_table];
    ac_tab = &enc->huff_ac[comp->ac_table];
    for (k = 0; k < 64; ++k) enc->qcoeff[k] = (c89jpeg_i16)c89jpeg_div_round(enc->coeff[k], (c89jpeg_i32)qt[k]);
    dc = enc->qcoeff[0];
    diff = dc - comp->pred;
    ((c89jpeg_component *)comp)->pred = dc;
    cat = c89jpeg_value_category(diff);
    st = c89jpeg_encres_put_bits(state, dc_tab->ehufco[cat], dc_tab->ehufsi[cat]);
    if (st != C89JPEG_OK) return st;
    if (cat != 0) {
        st = c89jpeg_encres_put_bits(state, c89jpeg_value_bits(diff, cat), cat);
        if (st != C89JPEG_OK) return st;
    }
    run = 0;
    for (k = 1; k < 64; ++k) {
        c89jpeg_i32 v;
        v = enc->qcoeff[c89jpeg_zigzag[k]];
        if (v == 0) {
            ++run;
        } else {
            while (run >= 16) {
                st = c89jpeg_encres_put_bits(state, ac_tab->ehufco[0xF0], ac_tab->ehufsi[0xF0]);
                if (st != C89JPEG_OK) return st;
                run -= 16;
            }
            cat = c89jpeg_value_category(v);
            st = c89jpeg_encres_put_bits(state, ac_tab->ehufco[(run << 4) | cat], ac_tab->ehufsi[(run << 4) | cat]);
            if (st != C89JPEG_OK) return st;
            st = c89jpeg_encres_put_bits(state, c89jpeg_value_bits(v, cat), cat);
            if (st != C89JPEG_OK) return st;
            run = 0;
        }
    }
    if (run != 0) return c89jpeg_encres_put_bits(state, ac_tab->ehufco[0x00], ac_tab->ehufsi[0x00]);
    return C89JPEG_OK;
}

static void c89jpeg_encres_reset_preds(c89jpeg_encoder_resume *state)
{
    int ci;
    if (state == 0 || state->encoder == 0) return;
    for (ci = 0; ci < state->components; ++ci) state->encoder->comp[ci].pred = 0;
}

static int c89jpeg_encres_advance_after_block(c89jpeg_encoder_resume *state)
{
    c89jpeg_component *comp;
    if (state == 0 || state->encoder == 0) return 0;
    comp = &state->encoder->comp[state->ci];
    ++state->bx;
    if (state->bx < comp->h_samp) return 0;
    state->bx = 0;
    ++state->by;
    if (state->by < comp->v_samp) return 0;
    state->by = 0;
    ++state->ci;
    if (state->ci < state->components) return 0;
    state->ci = 0;
    ++state->mcu_x;
    if (state->mcu_x >= state->mcu_cols) {
        state->mcu_x = 0;
        ++state->mcu_y;
    }
    return 1;
}

static c89jpeg_u32 c89jpeg_encres_phase_need(const c89jpeg_encoder_resume *state)
{
    if (state == 0) return C89JPEG_ENCRES_MIN_WINDOW;
    switch (state->phase) {
    case C89JPEG_ENCRES_PHASE_SOI: return 8;
    case C89JPEG_ENCRES_PHASE_APP0: return 32;
    case C89JPEG_ENCRES_PHASE_DQT0:
    case C89JPEG_ENCRES_PHASE_DQT1: return 80;
    case C89JPEG_ENCRES_PHASE_SOF0:
    case C89JPEG_ENCRES_PHASE_SOS: return 32;
    case C89JPEG_ENCRES_PHASE_DHT_DC0:
    case C89JPEG_ENCRES_PHASE_DHT_DC1:
    case C89JPEG_ENCRES_PHASE_DHT_AC0:
    case C89JPEG_ENCRES_PHASE_DHT_AC1: return 320;
    case C89JPEG_ENCRES_PHASE_DRI: return 16;
    case C89JPEG_ENCRES_PHASE_SCAN: return 512;
    case C89JPEG_ENCRES_PHASE_RESTART: return 16;
    case C89JPEG_ENCRES_PHASE_ECS_FLUSH: return 8;
    case C89JPEG_ENCRES_PHASE_EOI: return 8;
    default: return 16;
    }
}

static c89jpeg_status c89jpeg_encoder_resume_step(c89jpeg_encoder_resume *state)
{
    c89jpeg_status st;
    c89jpeg_encoder *enc;
    if (state == 0 || state->encoder == 0) return C89JPEG_ERR_BAD_ARG;
    enc = state->encoder;
    switch (state->phase) {
    case C89JPEG_ENCRES_PHASE_SOI:
        st = c89jpeg_encres_put_marker(state, C89JPEG_MARKER_SOI);
        if (st != C89JPEG_OK) return st;
        if (state->abbreviated) state->phase = C89JPEG_ENCRES_PHASE_SOF0;
        else if (state->params.emit_jfif) state->phase = C89JPEG_ENCRES_PHASE_APP0;
        else state->phase = C89JPEG_ENCRES_PHASE_DQT0;
        return C89JPEG_OK;
    case C89JPEG_ENCRES_PHASE_APP0:
        st = c89jpeg_encres_write_app0(state, &state->params);
        if (st != C89JPEG_OK) return st;
        state->phase = C89JPEG_ENCRES_PHASE_DQT0;
        return C89JPEG_OK;
    case C89JPEG_ENCRES_PHASE_DQT0:
        st = c89jpeg_encres_write_dqt(state, 0, enc->qtable[0]);
        if (st != C89JPEG_OK) return st;
        state->phase = (state->components > 1) ? C89JPEG_ENCRES_PHASE_DQT1 : C89JPEG_ENCRES_PHASE_SOF0;
        return C89JPEG_OK;
    case C89JPEG_ENCRES_PHASE_DQT1:
        st = c89jpeg_encres_write_dqt(state, 1, enc->qtable[1]);
        if (st != C89JPEG_OK) return st;
        state->phase = C89JPEG_ENCRES_PHASE_SOF0;
        return C89JPEG_OK;
    case C89JPEG_ENCRES_PHASE_SOF0:
        st = c89jpeg_encres_write_sof0(state, &state->params, enc->comp, state->components);
        if (st != C89JPEG_OK) return st;
        state->phase = state->abbreviated ? C89JPEG_ENCRES_PHASE_SOS : C89JPEG_ENCRES_PHASE_DHT_DC0;
        return C89JPEG_OK;
    case C89JPEG_ENCRES_PHASE_DHT_DC0:
        if (state->source_replay_enabled && !state->source_replay_ready) {
            st = c89jpeg_encres_prepare_replay_tables(state);
            if (st != C89JPEG_OK) return st;
        }
        st = c89jpeg_encres_write_dht(state, 0, 0, &enc->huff_dc[0]);
        if (st != C89JPEG_OK) return st;
        state->phase = (state->components > 1) ? C89JPEG_ENCRES_PHASE_DHT_DC1 : C89JPEG_ENCRES_PHASE_DHT_AC0;
        return C89JPEG_OK;
    case C89JPEG_ENCRES_PHASE_DHT_DC1:
        st = c89jpeg_encres_write_dht(state, 0, 1, &enc->huff_dc[1]);
        if (st != C89JPEG_OK) return st;
        state->phase = C89JPEG_ENCRES_PHASE_DHT_AC0;
        return C89JPEG_OK;
    case C89JPEG_ENCRES_PHASE_DHT_AC0:
        st = c89jpeg_encres_write_dht(state, 1, 0, &enc->huff_ac[0]);
        if (st != C89JPEG_OK) return st;
        state->phase = (state->components > 1) ? C89JPEG_ENCRES_PHASE_DHT_AC1 : ((state->restart_interval != 0) ? C89JPEG_ENCRES_PHASE_DRI : C89JPEG_ENCRES_PHASE_SOS);
        return C89JPEG_OK;
    case C89JPEG_ENCRES_PHASE_DHT_AC1:
        st = c89jpeg_encres_write_dht(state, 1, 1, &enc->huff_ac[1]);
        if (st != C89JPEG_OK) return st;
        state->phase = (state->restart_interval != 0) ? C89JPEG_ENCRES_PHASE_DRI : C89JPEG_ENCRES_PHASE_SOS;
        return C89JPEG_OK;
    case C89JPEG_ENCRES_PHASE_DRI:
        st = c89jpeg_encres_write_dri(state, state->restart_interval);
        if (st != C89JPEG_OK) return st;
        state->phase = C89JPEG_ENCRES_PHASE_SOS;
        return C89JPEG_OK;
    case C89JPEG_ENCRES_PHASE_SOS:
        st = c89jpeg_encres_write_sos(state, enc->comp, state->components);
        if (st != C89JPEG_OK) return st;
        state->phase = C89JPEG_ENCRES_PHASE_SCAN;
        return C89JPEG_OK;
    case C89JPEG_ENCRES_PHASE_SCAN:
        if (state->mcu_y >= state->mcu_rows) {
            state->phase = C89JPEG_ENCRES_PHASE_ECS_FLUSH;
            return C89JPEG_OK;
        }
        {
            c89jpeg_component *comp;
            int mcu_finished;
            comp = &enc->comp[state->ci];
            if (state->source_mode) {
                c89jpeg_encode_source_params sp;
                c89jpeg_encode_row_cache cache;
                st = c89jpeg_encres_source_prepare_window(state);
                if (st != C89JPEG_OK) return st;
                c89jpeg_encres_make_source_params(state, &sp, &cache);
                c89jpeg_source_load_block(&sp, &cache, comp, state->max_h, state->max_v,
                                          state->mcu_x, state->mcu_y, state->bx, state->by,
                                          enc->dct_tmp);
            } else {
                c89jpeg_load_block(&state->params, comp, state->max_h, state->max_v,
                                   state->mcu_x, state->mcu_y, state->bx, state->by,
                                   enc->dct_tmp);
            }
            c89jpeg_fdct8x8(enc->dct_tmp, enc->dct_work, enc->coeff);
            st = c89jpeg_emit_block_resume(enc, state, comp);
            if (st != C89JPEG_OK) return st;
            mcu_finished = c89jpeg_encres_advance_after_block(state);
            if (mcu_finished && state->source_mode) {
                c89jpeg_u16 expected_base;
                expected_base = (c89jpeg_u16)(state->mcu_y * state->mcu_h);
                if (expected_base != state->source_base_y) {
                    state->source_base_y = expected_base;
                    state->source_rows_loaded = 0;
                    state->source_window_padded = 0;
                }
            }
            if (mcu_finished && state->restart_interval != 0) {
                if (--state->restart_count == 0) state->phase = C89JPEG_ENCRES_PHASE_RESTART;
            }
            if (state->mcu_y >= state->mcu_rows && state->phase == C89JPEG_ENCRES_PHASE_SCAN) state->phase = C89JPEG_ENCRES_PHASE_ECS_FLUSH;
        }
        return C89JPEG_OK;
    case C89JPEG_ENCRES_PHASE_RESTART:
        st = c89jpeg_encres_flush_pad1(state);
        if (st != C89JPEG_OK) return st;
        st = c89jpeg_encres_put_marker(state, (c89jpeg_u8)(C89JPEG_MARKER_RST0 + state->rst_index));
        if (st != C89JPEG_OK) return st;
        state->rst_index = (c89jpeg_u8)((state->rst_index + 1) & 7);
        state->restart_count = state->restart_interval;
        c89jpeg_encres_reset_preds(state);
        state->phase = (state->mcu_y >= state->mcu_rows) ? C89JPEG_ENCRES_PHASE_ECS_FLUSH : C89JPEG_ENCRES_PHASE_SCAN;
        return C89JPEG_OK;
    case C89JPEG_ENCRES_PHASE_ECS_FLUSH:
        st = c89jpeg_encres_flush_pad1(state);
        if (st != C89JPEG_OK) return st;
        state->phase = C89JPEG_ENCRES_PHASE_EOI;
        return C89JPEG_OK;
    case C89JPEG_ENCRES_PHASE_EOI:
        st = c89jpeg_encres_put_marker(state, C89JPEG_MARKER_EOI);
        if (st != C89JPEG_OK) return st;
        state->phase = C89JPEG_ENCRES_PHASE_DONE;
        return C89JPEG_OK;
    case C89JPEG_ENCRES_PHASE_DONE:
        return C89JPEG_OK;
    default:
        return C89JPEG_ERR_BAD_ARG;
    }
}

static c89jpeg_status c89jpeg_encoder_resume_generate(c89jpeg_encoder_resume *state)
{
    c89jpeg_status st;
    c89jpeg_u32 need;
    c89jpeg_u32 avail;
    if (state == 0) return C89JPEG_ERR_BAD_ARG;
    if (c89jpeg_encres_buffered(state) != 0) return C89JPEG_OK;
    if (state->phase == C89JPEG_ENCRES_PHASE_DONE) {
        state->finished = 1;
        if (!state->total_size_known) {
            state->exact_total_size = state->total_emitted;
            state->total_size_known = 1;
        }
        return C89JPEG_OK;
    }
    c89jpeg_encres_reset_buffer(state);
    for (;;) {
        if (state->phase == C89JPEG_ENCRES_PHASE_DONE) break;
        need = c89jpeg_encres_phase_need(state);
        avail = state->staging_capacity - state->staging_size;
        if (avail < need) {
            if (state->staging_size != 0) break;
            state->last_error = C89JPEG_ERR_SHORT_BUFFER;
            return C89JPEG_ERR_SHORT_BUFFER;
        }
        st = c89jpeg_encoder_resume_step(state);
        if (st == C89JPEG_SUSPENDED) {
            if (state->staging_size != 0) break;
            state->last_error = C89JPEG_SUSPENDED;
            return C89JPEG_SUSPENDED;
        }
        if (st != C89JPEG_OK) {
            state->last_error = st;
            return st;
        }
        if (state->staging_size >= state->emit_chunk_size) break;
    }
    state->last_error = C89JPEG_OK;
    return C89JPEG_OK;
}

void c89jpeg_encoder_resume_init(c89jpeg_encoder_resume *state)
{
    if (state == 0) return;
    memset(state, 0, sizeof(*state));
    state->last_error = C89JPEG_OK;
}

c89jpeg_status c89jpeg_encoder_resume_begin(c89jpeg_encoder_resume *state,
                                            c89jpeg_encoder *enc,
                                            const c89jpeg_encode_params *params,
                                            const c89jpeg_encode_resume_params *resume_params,
                                            c89jpeg_u32 *out_total_size)
{
    c89jpeg_status st;
    int components;
    c89jpeg_u8 max_h;
    c89jpeg_u8 max_v;
    c89jpeg_u16 restart_interval;
    if (state == 0 || enc == 0 || params == 0 || resume_params == 0) return C89JPEG_ERR_BAD_ARG;
    if (resume_params->staging_buffer == 0 || resume_params->staging_capacity == 0) return C89JPEG_ERR_BAD_ARG;
    if (resume_params->emit_chunk_size == 0) return C89JPEG_ERR_BAD_ARG;
    if (resume_params->staging_capacity < c89jpeg_encoder_resume_min_output_buffer()) return C89JPEG_ERR_SHORT_BUFFER;
    c89jpeg_encoder_resume_init(state);
    state->encoder = enc;
    state->staging_buffer = resume_params->staging_buffer;
    state->staging_capacity = resume_params->staging_capacity;
    state->emit_chunk_size = resume_params->emit_chunk_size;
    state->abbreviated = (resume_params->abbreviated != 0) ? 1 : 0;
    state->params = *params;
    state->tables = resume_params->tables;
    if (state->abbreviated) {
        st = c89jpeg_encoder_apply_tables(enc, resume_params->tables, &state->params, &components, &max_h, &max_v, &restart_interval);
    } else {
        st = c89jpeg_encoder_prepare_from_params(enc, &state->params, &components, &max_h, &max_v);
        restart_interval = state->params.restart_interval;
    }
    if (st != C89JPEG_OK) {
        state->last_error = st;
        return st;
    }
    state->components = (c89jpeg_u8)components;
    state->max_h = max_h;
    state->max_v = max_v;
    state->mcu_w = (c89jpeg_u16)(max_h * 8);
    state->mcu_h = (c89jpeg_u16)(max_v * 8);
    state->mcu_cols = (c89jpeg_u16)((state->params.width + state->mcu_w - 1) / state->mcu_w);
    state->mcu_rows = (c89jpeg_u16)((state->params.height + state->mcu_h - 1) / state->mcu_h);
    state->restart_interval = restart_interval;
    state->restart_count = restart_interval;
    state->phase = C89JPEG_ENCRES_PHASE_SOI;
    state->prepared = 1;
    c89jpeg_encres_reset_buffer(state);
    c89jpeg_encres_reset_preds(state);
    if (out_total_size != 0) {
        st = c89jpeg_measure_encode_size(&state->params, state->tables, state->abbreviated, &state->exact_total_size);
        if (st != C89JPEG_OK) {
            state->last_error = st;
            return st;
        }
        state->total_size_known = 1;
        *out_total_size = state->exact_total_size;
    }
    state->last_error = C89JPEG_OK;
    return C89JPEG_OK;
}

c89jpeg_status c89jpeg_encoder_resume_begin_source(c89jpeg_encoder_resume *state,
                                                   c89jpeg_encoder *enc,
                                                   const c89jpeg_encode_source_resume_params *params)
{
    c89jpeg_status st;
    int components;
    int ci;
    c89jpeg_u8 max_h;
    c89jpeg_u8 max_v;
    c89jpeg_u16 restart_interval;
    c89jpeg_encode_params local_params;
    if (state == 0 || enc == 0 || params == 0) return C89JPEG_ERR_BAD_ARG;
    st = c89jpeg_validate_encode_source_resume_params(params);
    if (st != C89JPEG_OK) return st;
    c89jpeg_source_resume_to_encode_params(params, &local_params);
    c89jpeg_encoder_resume_init(state);
    state->encoder = enc;
    state->staging_buffer = params->staging_buffer;
    state->staging_capacity = params->staging_capacity;
    state->emit_chunk_size = params->emit_chunk_size;
    state->abbreviated = (params->abbreviated != 0) ? 1 : 0;
    state->tables = params->tables;
    state->params = local_params;
    state->source_replay_enabled = c89jpeg_source_resume_uses_replay(params) ? 1 : 0;
    state->source_replay_ready = state->source_replay_enabled ? 0 : 1;
    state->source_replay_buffer = params->replay_buffer;
    state->source_replay_buffer_size = params->replay_buffer_size;
    if (state->abbreviated) {
        st = c89jpeg_encoder_apply_tables_resume_source(enc, params->tables, &state->params, &components, &max_h, &max_v, &restart_interval);
    } else {
        c89jpeg_encoder_init(enc);
        c89jpeg_init_quant_tables(enc, &state->params);
        c89jpeg_setup_encoder_components(enc, &state->params, &components, &max_h, &max_v);
        for (ci = 0; ci < components; ++ci) enc->comp[ci].pred = 0;
        restart_interval = state->params.restart_interval;
        st = C89JPEG_OK;
    }
    if (st != C89JPEG_OK) {
        state->last_error = st;
        return st;
    }
    state->components = (c89jpeg_u8)components;
    state->max_h = max_h;
    state->max_v = max_v;
    state->mcu_w = (c89jpeg_u16)(max_h * 8);
    state->mcu_h = (c89jpeg_u16)(max_v * 8);
    state->mcu_cols = (c89jpeg_u16)((state->params.width + state->mcu_w - 1) / state->mcu_w);
    state->mcu_rows = (c89jpeg_u16)((state->params.height + state->mcu_h - 1) / state->mcu_h);
    state->restart_interval = restart_interval;
    state->restart_count = restart_interval;
    state->phase = C89JPEG_ENCRES_PHASE_SOI;
    state->prepared = 1;
    state->source_mode = 1;
    state->source_input_final = 0;
    state->source_window_padded = 0;
    state->source_base_y = 0;
    state->source_rows_loaded = 0;
    state->source_next_row = 0;
    state->source_row_window = params->row_window;
    state->source_row_window_size = params->row_window_size;
    c89jpeg_encres_reset_buffer(state);
    c89jpeg_encres_reset_preds(state);
    state->total_size_known = 0;
    state->exact_total_size = 0;
    state->last_error = C89JPEG_OK;
    return C89JPEG_OK;
}

c89jpeg_status c89jpeg_encoder_resume_feed_rows(c89jpeg_encoder_resume *state,
                                                const c89jpeg_u8 *rows,
                                                c89jpeg_u32 row_count,
                                                c89jpeg_u32 row_stride,
                                                c89jpeg_u32 *rows_accepted)
{
    c89jpeg_u16 expected_base;
    c89jpeg_u16 row_goal;
    c89jpeg_u32 accept;
    c89jpeg_u32 i;
    if (rows_accepted != 0) *rows_accepted = 0;
    if (state == 0 || rows == 0 || row_count == 0) return C89JPEG_ERR_BAD_ARG;
    if (!state->prepared || !state->source_mode) {
        if (state != 0) state->last_error = C89JPEG_ERR_BAD_ARG;
        return C89JPEG_ERR_BAD_ARG;
    }
    if (row_stride < state->params.stride_bytes) {
        state->last_error = C89JPEG_ERR_BAD_ARG;
        return C89JPEG_ERR_BAD_ARG;
    }
    if (state->source_input_final) {
        state->last_error = C89JPEG_ERR_BAD_ARG;
        return C89JPEG_ERR_BAD_ARG;
    }
    if (state->source_replay_enabled) {
        accept = (c89jpeg_u32)state->params.height - (c89jpeg_u32)state->source_next_row;
        if (accept > row_count) accept = row_count;
        for (i = 0; i < accept; ++i) {
            memcpy(state->source_replay_buffer + ((c89jpeg_u32)state->source_next_row + i) * state->params.stride_bytes,
                   rows + i * row_stride,
                   (size_t)state->params.stride_bytes);
        }
        state->source_next_row = (c89jpeg_u16)(state->source_next_row + accept);
        if (rows_accepted != 0) *rows_accepted = accept;
        state->last_error = C89JPEG_OK;
        return C89JPEG_OK;
    }
    expected_base = (c89jpeg_u16)(state->mcu_y * state->mcu_h);
    if (state->source_base_y != expected_base && state->source_rows_loaded == 0) {
        state->source_base_y = expected_base;
        state->source_window_padded = 0;
    }
    row_goal = c89jpeg_encres_source_row_goal(state);
    if (row_goal == 0 || state->source_rows_loaded >= row_goal) {
        state->last_error = C89JPEG_OK;
        return C89JPEG_OK;
    }
    accept = (c89jpeg_u32)(row_goal - state->source_rows_loaded);
    if (accept > row_count) accept = row_count;
    for (i = 0; i < accept; ++i) {
        memcpy(state->source_row_window + ((c89jpeg_u32)state->source_rows_loaded + i) * state->params.stride_bytes,
               rows + i * row_stride,
               (size_t)state->params.stride_bytes);
    }
    state->source_rows_loaded = (c89jpeg_u16)(state->source_rows_loaded + accept);
    state->source_next_row = (c89jpeg_u16)(state->source_next_row + accept);
    state->source_window_padded = 0;
    if (rows_accepted != 0) *rows_accepted = accept;
    state->last_error = C89JPEG_OK;
    return C89JPEG_OK;
}

void c89jpeg_encoder_resume_finish_input(c89jpeg_encoder_resume *state)
{
    if (state == 0) return;
    state->source_input_final = 1;
}

c89jpeg_status c89jpeg_encoder_resume_run(c89jpeg_encoder_resume *state,
                                          c89jpeg_write_fn write_fn,
                                          void *write_user,
                                          c89jpeg_u32 *bytes_written)
{
    c89jpeg_status st;
    c89jpeg_u32 buffered;
    c89jpeg_u32 chunk;
    if (bytes_written != 0) *bytes_written = 0;
    if (state == 0 || write_fn == 0) return C89JPEG_ERR_BAD_ARG;
    if (!state->prepared) {
        state->last_error = C89JPEG_ERR_BAD_ARG;
        return C89JPEG_ERR_BAD_ARG;
    }
    if (state->finished) {
        state->last_error = C89JPEG_OK;
        return C89JPEG_OK;
    }
    if (c89jpeg_encres_buffered(state) == 0) {
        st = c89jpeg_encoder_resume_generate(state);
        if (st != C89JPEG_OK) return st;
    }
    buffered = c89jpeg_encres_buffered(state);
    if (buffered == 0 && state->phase == C89JPEG_ENCRES_PHASE_DONE) {
        state->finished = 1;
        if (!state->total_size_known) { state->exact_total_size = state->total_emitted; state->total_size_known = 1; }
        state->last_error = C89JPEG_OK;
        return C89JPEG_OK;
    }
    chunk = buffered;
    if (chunk > state->emit_chunk_size) chunk = state->emit_chunk_size;
    if (!write_fn(write_user, state->staging_buffer + state->emit_offset, chunk)) {
        state->last_error = C89JPEG_ERR_IO;
        return C89JPEG_ERR_IO;
    }
    state->emit_offset += chunk;
    state->total_emitted += chunk;
    if (bytes_written != 0) *bytes_written = chunk;
    if (state->emit_offset >= state->staging_size) {
        c89jpeg_encres_reset_buffer(state);
        if (state->phase == C89JPEG_ENCRES_PHASE_DONE) {
            state->finished = 1;
            if (!state->total_size_known) { state->exact_total_size = state->total_emitted; state->total_size_known = 1; }
            state->last_error = C89JPEG_OK;
            return C89JPEG_OK;
        }
    }
    state->last_error = C89JPEG_SUSPENDED;
    return C89JPEG_SUSPENDED;
}

c89jpeg_status c89jpeg_encoder_resume_pull(c89jpeg_encoder_resume *state,
                                           c89jpeg_u8 *out_buf,
                                           c89jpeg_u32 out_capacity,
                                           c89jpeg_u32 *out_size)
{
    c89jpeg_status st;
    c89jpeg_u32 buffered;
    c89jpeg_u32 chunk;
    if (out_size != 0) *out_size = 0;
    if (state == 0 || out_buf == 0 || out_capacity == 0) return C89JPEG_ERR_BAD_ARG;
    if (!state->prepared) { state->last_error = C89JPEG_ERR_BAD_ARG; return C89JPEG_ERR_BAD_ARG; }
    if (state->finished) { state->last_error = C89JPEG_OK; return C89JPEG_OK; }
    if (c89jpeg_encres_buffered(state) == 0) {
        st = c89jpeg_encoder_resume_generate(state);
        if (st != C89JPEG_OK) return st;
    }
    buffered = c89jpeg_encres_buffered(state);
    if (buffered == 0 && state->phase == C89JPEG_ENCRES_PHASE_DONE) {
        state->finished = 1;
        if (!state->total_size_known) { state->exact_total_size = state->total_emitted; state->total_size_known = 1; }
        state->last_error = C89JPEG_OK;
        return C89JPEG_OK;
    }
    chunk = buffered;
    if (chunk > state->emit_chunk_size) chunk = state->emit_chunk_size;
    if (chunk > out_capacity) chunk = out_capacity;
    memcpy(out_buf, state->staging_buffer + state->emit_offset, (size_t)chunk);
    state->emit_offset += chunk;
    state->total_emitted += chunk;
    if (out_size != 0) *out_size = chunk;
    if (state->emit_offset >= state->staging_size) {
        c89jpeg_encres_reset_buffer(state);
        if (state->phase == C89JPEG_ENCRES_PHASE_DONE) {
            state->finished = 1;
            if (!state->total_size_known) { state->exact_total_size = state->total_emitted; state->total_size_known = 1; }
            state->last_error = C89JPEG_OK;
            return C89JPEG_OK;
        }
    }
    state->last_error = C89JPEG_SUSPENDED;
    return C89JPEG_SUSPENDED;
}

int c89jpeg_encoder_resume_is_finished(const c89jpeg_encoder_resume *state)
{
    if (state == 0) return 0;
    return state->finished ? 1 : 0;
}

int c89jpeg_encoder_resume_needs_input(const c89jpeg_encoder_resume *state)
{
    c89jpeg_u16 row_goal;
    if (state == 0 || !state->source_mode || !state->prepared || state->finished) return 0;
    if (c89jpeg_encres_buffered(state) != 0) return 0;
    if (state->source_replay_enabled && !state->source_replay_ready) {
        if ((c89jpeg_u32)state->source_next_row >= (c89jpeg_u32)state->params.height) return 0;
        if (state->source_input_final) return 0;
        return (state->phase >= C89JPEG_ENCRES_PHASE_DHT_DC0) ? 1 : 0;
    }
    if (state->phase != C89JPEG_ENCRES_PHASE_SCAN) return 0;
    row_goal = c89jpeg_encres_source_row_goal(state);
    if (row_goal == 0) return 0;
    return (state->source_rows_loaded < row_goal) ? 1 : 0;
}

c89jpeg_u32 c89jpeg_encoder_resume_source_rows_needed(const c89jpeg_encoder_resume *state)
{
    c89jpeg_u16 row_goal;
    if (state == 0 || !state->source_mode || !state->prepared || state->finished) return 0;
    if (state->source_replay_enabled && !state->source_replay_ready) {
        if ((c89jpeg_u32)state->source_next_row >= (c89jpeg_u32)state->params.height) return 0;
        if (state->source_input_final) return 0;
        if (state->phase < C89JPEG_ENCRES_PHASE_DHT_DC0) return 0;
        return (c89jpeg_u32)state->params.height - (c89jpeg_u32)state->source_next_row;
    }
    row_goal = c89jpeg_encres_source_row_goal(state);
    if (state->source_rows_loaded >= row_goal) return 0;
    return (c89jpeg_u32)(row_goal - state->source_rows_loaded);
}

c89jpeg_u32 c89jpeg_encoder_resume_source_rows_accepted(const c89jpeg_encoder_resume *state)
{
    if (state == 0 || !state->source_mode) return 0;
    return state->source_next_row;
}

c89jpeg_u32 c89jpeg_encoder_resume_total_size(const c89jpeg_encoder_resume *state)
{
    if (state == 0) return 0;
    if (state->total_size_known) return state->exact_total_size;
    if (state->finished) return state->total_emitted;
    return 0;
}

c89jpeg_u32 c89jpeg_encoder_resume_remaining(const c89jpeg_encoder_resume *state)
{
    if (state == 0) return 0;
    if (!state->total_size_known) return 0;
    if (state->exact_total_size < state->total_emitted) return 0;
    return state->exact_total_size - state->total_emitted;
}

c89jpeg_u32 c89jpeg_encoder_resume_min_output_buffer(void)
{
    return C89JPEG_ENCRES_MIN_WINDOW;
}

c89jpeg_u32 c89jpeg_encoder_resume_replay_buffer_size(c89jpeg_u16 width,
                                                      c89jpeg_u16 height,
                                                      c89jpeg_u32 stride_bytes)
{
    c89jpeg_u32 min_stride;
    c89jpeg_u32 need;
    if (width == 0 || height == 0) return 0;
    if (c89jpeg_source_min_stride(width, C89JPEG_PIXFMT_GRAY8, &min_stride) != C89JPEG_OK) return 0;
    if (stride_bytes < min_stride) {
        if (c89jpeg_source_min_stride(width, C89JPEG_PIXFMT_RGB24, &min_stride) != C89JPEG_OK) return 0;
        if (stride_bytes < min_stride) return 0;
    }
    if (!c89jpeg_mul_u32(stride_bytes, (c89jpeg_u32)height, &need)) return 0;
    return need;
}

static c89jpeg_u32 c89jpeg_sat_add_u32(c89jpeg_u32 a, c89jpeg_u32 b)
{
    if (a > 0xFFFFFFFFUL - b) return 0xFFFFFFFFUL;
    return a + b;
}

static void c89jpeg_trainer_seed_dc(c89jpeg_u32 *freq, int chroma)
{
    int i;
    const c89jpeg_u8 *vals;
    vals = chroma ? c89jpeg_std_dc_chroma_vals : c89jpeg_std_dc_luma_vals;
    for (i = 0; i < 12; ++i) if (freq[vals[i]] == 0) freq[vals[i]] = 1;
}

static void c89jpeg_trainer_seed_ac(c89jpeg_u32 *freq, int chroma)
{
    int i;
    const c89jpeg_u8 *vals;
    int count;
    vals = chroma ? c89jpeg_std_ac_chroma_vals : c89jpeg_std_ac_luma_vals;
    count = 162;
    for (i = 0; i < count; ++i) if (freq[vals[i]] == 0) freq[vals[i]] = 1;
}

void c89jpeg_tables_trainer_init(c89jpeg_tables_trainer *trainer)
{
    if (trainer == 0) return;
    memset(trainer, 0, sizeof(*trainer));
    trainer->last_error = C89JPEG_OK;
}

c89jpeg_status c89jpeg_tables_trainer_prepare(c89jpeg_tables_trainer *trainer,
                                              c89jpeg_encoder *enc,
                                              const c89jpeg_encode_params *params)
{
    c89jpeg_encode_params local_params;
    c89jpeg_status st;
    int components;
    c89jpeg_u8 max_h;
    c89jpeg_u8 max_v;
    if (trainer == 0 || enc == 0 || params == 0) return C89JPEG_ERR_BAD_ARG;
    c89jpeg_tables_trainer_init(trainer);
    local_params = *params;
    local_params.huffman_mode = C89JPEG_HUFFMAN_DEFAULT;
    local_params.custom_dc_luma = 0;
    local_params.custom_ac_luma = 0;
    local_params.custom_dc_chroma = 0;
    local_params.custom_ac_chroma = 0;
    st = c89jpeg_encoder_prepare_from_params(enc, &local_params, &components, &max_h, &max_v);
    (void)max_h; (void)max_v;
    if (st != C89JPEG_OK) { trainer->last_error = st; return st; }
    c89jpeg_tables_copy_from_encoder(&trainer->base_tables, enc, components, params->restart_interval);
    trainer->prepared = 1;
    trainer->pixel_format = params->pixel_format;
    trainer->subsampling = c89jpeg_normalize_session_subsampling(params->pixel_format, params->subsampling);
    trainer->component_count = (params->pixel_format == C89JPEG_PIXFMT_GRAY8) ? 1 : 3;
    trainer->last_error = C89JPEG_OK;
    return C89JPEG_OK;
}

c89jpeg_status c89jpeg_tables_trainer_add_image(c89jpeg_tables_trainer *trainer,
                                                c89jpeg_encoder *enc,
                                                const c89jpeg_encode_params *params)
{
    c89jpeg_encode_params local_params;
    c89jpeg_status st;
    int components;
    c89jpeg_u8 max_h;
    c89jpeg_u8 max_v;
    c89jpeg_subsampling subsampling;
    int tq;
    int i;
    if (trainer == 0 || enc == 0 || params == 0) return C89JPEG_ERR_BAD_ARG;
    if (!trainer->prepared) { trainer->last_error = C89JPEG_ERR_BAD_ARG; return C89JPEG_ERR_BAD_ARG; }
    if (params->pixel_format != trainer->pixel_format) { trainer->last_error = C89JPEG_ERR_BAD_ARG; return C89JPEG_ERR_BAD_ARG; }
    subsampling = c89jpeg_normalize_session_subsampling(params->pixel_format, params->subsampling);
    if (subsampling != trainer->subsampling) { trainer->last_error = C89JPEG_ERR_BAD_ARG; return C89JPEG_ERR_BAD_ARG; }
    local_params = *params;
    local_params.huffman_mode = C89JPEG_HUFFMAN_DEFAULT;
    local_params.custom_dc_luma = 0;
    local_params.custom_ac_luma = 0;
    local_params.custom_dc_chroma = 0;
    local_params.custom_ac_chroma = 0;
    st = c89jpeg_validate_encode_params(&local_params);
    if (st != C89JPEG_OK) { trainer->last_error = st; return st; }
    c89jpeg_encoder_init(enc);
    c89jpeg_init_quant_tables(enc, &local_params);
    c89jpeg_setup_encoder_components(enc, &local_params, &components, &max_h, &max_v);
    for (tq = 0; tq < ((components > 1) ? 2 : 1); ++tq) {
        for (i = 0; i < 64; ++i) {
            if (enc->qtable[tq][i] != trainer->base_tables.quant[tq][i]) {
                trainer->last_error = C89JPEG_ERR_BAD_ARG;
                return C89JPEG_ERR_BAD_ARG;
            }
        }
    }
    st = c89jpeg_collect_huffman_stats(enc, &local_params, components, max_h, max_v);
    if (st != C89JPEG_OK) { trainer->last_error = st; return st; }
    for (i = 0; i < C89JPEG_HUFF_SYMBOLS; ++i) {
        trainer->dc_freq[0][i] = c89jpeg_sat_add_u32(trainer->dc_freq[0][i], enc->huff_freq_dc[0][i]);
        trainer->ac_freq[0][i] = c89jpeg_sat_add_u32(trainer->ac_freq[0][i], enc->huff_freq_ac[0][i]);
        if (components > 1) {
            trainer->dc_freq[1][i] = c89jpeg_sat_add_u32(trainer->dc_freq[1][i], enc->huff_freq_dc[1][i]);
            trainer->ac_freq[1][i] = c89jpeg_sat_add_u32(trainer->ac_freq[1][i], enc->huff_freq_ac[1][i]);
        }
    }
    trainer->image_count += 1;
    trainer->last_error = C89JPEG_OK;
    return C89JPEG_OK;
}

c89jpeg_status c89jpeg_tables_trainer_build(c89jpeg_tables_trainer *trainer, c89jpeg_tables *tables)
{
    c89jpeg_status st;
    c89jpeg_u32 dc_freq[2][C89JPEG_HUFF_SYMBOLS];
    c89jpeg_u32 ac_freq[2][C89JPEG_HUFF_SYMBOLS];
    int i;
    if (trainer == 0 || tables == 0) return C89JPEG_ERR_BAD_ARG;
    if (!trainer->prepared || trainer->image_count == 0) { trainer->last_error = C89JPEG_ERR_BAD_ARG; return C89JPEG_ERR_BAD_ARG; }
    *tables = trainer->base_tables;
    memcpy(dc_freq, trainer->dc_freq, sizeof(dc_freq));
    memcpy(ac_freq, trainer->ac_freq, sizeof(ac_freq));
    c89jpeg_trainer_seed_dc(dc_freq[0], 0);
    c89jpeg_trainer_seed_ac(ac_freq[0], 0);
    st = c89jpeg_build_huffman_from_freq(&tables->huff[0][0], dc_freq[0]);
    if (st != C89JPEG_OK) { trainer->last_error = st; return st; }
    st = c89jpeg_build_huffman_from_freq(&tables->huff[1][0], ac_freq[0]);
    if (st != C89JPEG_OK) { trainer->last_error = st; return st; }
    if (trainer->component_count > 1) {
        c89jpeg_trainer_seed_dc(dc_freq[1], 1);
        c89jpeg_trainer_seed_ac(ac_freq[1], 1);
        st = c89jpeg_build_huffman_from_freq(&tables->huff[0][1], dc_freq[1]);
        if (st != C89JPEG_OK) { trainer->last_error = st; return st; }
        st = c89jpeg_build_huffman_from_freq(&tables->huff[1][1], ac_freq[1]);
        if (st != C89JPEG_OK) { trainer->last_error = st; return st; }
    } else {
        c89jpeg_zero_huffman(&tables->huff[0][1]);
        c89jpeg_zero_huffman(&tables->huff[1][1]);
    }
    for (i = 2; i < C89JPEG_MAX_HUFF_TABLES; ++i) {
        c89jpeg_zero_huffman(&tables->huff[0][i]);
        c89jpeg_zero_huffman(&tables->huff[1][i]);
    }
    trainer->last_error = C89JPEG_OK;
    return C89JPEG_OK;
}



c89jpeg_status c89jpeg_tables_trainer_prepare_source(c89jpeg_tables_trainer *trainer,
                                                     c89jpeg_encoder *enc,
                                                     const c89jpeg_encode_source_params *params)
{
    c89jpeg_encode_source_params local_params;
    c89jpeg_status st;
    int components;
    c89jpeg_u8 max_h;
    c89jpeg_u8 max_v;
    if (trainer == 0 || enc == 0 || params == 0) return C89JPEG_ERR_BAD_ARG;
    c89jpeg_tables_trainer_init(trainer);
    local_params = *params;
    local_params.huffman_mode = C89JPEG_HUFFMAN_DEFAULT;
    local_params.custom_dc_luma = 0;
    local_params.custom_ac_luma = 0;
    local_params.custom_dc_chroma = 0;
    local_params.custom_ac_chroma = 0;
    st = c89jpeg_encoder_prepare_from_source_params(enc, &local_params, &components, &max_h, &max_v);
    (void)max_h;
    (void)max_v;
    if (st != C89JPEG_OK) { trainer->last_error = st; return st; }
    c89jpeg_tables_copy_from_encoder(&trainer->base_tables, enc, components, params->restart_interval);
    trainer->prepared = 1;
    trainer->pixel_format = params->pixel_format;
    trainer->subsampling = c89jpeg_normalize_session_subsampling(params->pixel_format, params->subsampling);
    trainer->component_count = (params->pixel_format == C89JPEG_PIXFMT_GRAY8) ? 1 : 3;
    trainer->last_error = C89JPEG_OK;
    return C89JPEG_OK;
}

c89jpeg_status c89jpeg_tables_trainer_add_image_source(c89jpeg_tables_trainer *trainer,
                                                       c89jpeg_encoder *enc,
                                                       const c89jpeg_encode_source_params *params)
{
    c89jpeg_encode_source_params local_params;
    c89jpeg_encode_params local_encode;
    c89jpeg_status st;
    int components;
    c89jpeg_u8 max_h;
    c89jpeg_u8 max_v;
    c89jpeg_subsampling subsampling;
    int tq;
    int i;
    if (trainer == 0 || enc == 0 || params == 0) return C89JPEG_ERR_BAD_ARG;
    if (!trainer->prepared) { trainer->last_error = C89JPEG_ERR_BAD_ARG; return C89JPEG_ERR_BAD_ARG; }
    if (params->pixel_format != trainer->pixel_format) { trainer->last_error = C89JPEG_ERR_BAD_ARG; return C89JPEG_ERR_BAD_ARG; }
    subsampling = c89jpeg_normalize_session_subsampling(params->pixel_format, params->subsampling);
    if (subsampling != trainer->subsampling) { trainer->last_error = C89JPEG_ERR_BAD_ARG; return C89JPEG_ERR_BAD_ARG; }
    local_params = *params;
    local_params.huffman_mode = C89JPEG_HUFFMAN_DEFAULT;
    local_params.custom_dc_luma = 0;
    local_params.custom_ac_luma = 0;
    local_params.custom_dc_chroma = 0;
    local_params.custom_ac_chroma = 0;
    st = c89jpeg_validate_encode_source_params(&local_params);
    if (st != C89JPEG_OK) { trainer->last_error = st; return st; }
    c89jpeg_source_params_to_encode_params(&local_params, &local_encode);
    c89jpeg_encoder_init(enc);
    c89jpeg_init_quant_tables(enc, &local_encode);
    c89jpeg_setup_encoder_components(enc, &local_encode, &components, &max_h, &max_v);
    for (tq = 0; tq < ((components > 1) ? 2 : 1); ++tq) {
        for (i = 0; i < 64; ++i) {
            if (enc->qtable[tq][i] != trainer->base_tables.quant[tq][i]) {
                trainer->last_error = C89JPEG_ERR_BAD_ARG;
                return C89JPEG_ERR_BAD_ARG;
            }
        }
    }
    st = c89jpeg_collect_huffman_stats_source(enc, &local_params, components, max_h, max_v);
    if (st != C89JPEG_OK) { trainer->last_error = st; return st; }
    for (i = 0; i < C89JPEG_HUFF_SYMBOLS; ++i) {
        trainer->dc_freq[0][i] = c89jpeg_sat_add_u32(trainer->dc_freq[0][i], enc->huff_freq_dc[0][i]);
        trainer->ac_freq[0][i] = c89jpeg_sat_add_u32(trainer->ac_freq[0][i], enc->huff_freq_ac[0][i]);
        if (components > 1) {
            trainer->dc_freq[1][i] = c89jpeg_sat_add_u32(trainer->dc_freq[1][i], enc->huff_freq_dc[1][i]);
            trainer->ac_freq[1][i] = c89jpeg_sat_add_u32(trainer->ac_freq[1][i], enc->huff_freq_ac[1][i]);
        }
    }
    trainer->image_count += 1;
    trainer->last_error = C89JPEG_OK;
    return C89JPEG_OK;
}

void c89jpeg_tables_session_init(c89jpeg_tables_session *session)
{
    if (session == 0) return;
    memset(session, 0, sizeof(*session));
    session->last_error = C89JPEG_OK;
}

c89jpeg_status c89jpeg_tables_session_prepare(c89jpeg_tables_session *session,
                                              c89jpeg_encoder *enc,
                                              const c89jpeg_encode_params *params)
{
    c89jpeg_status st;
    c89jpeg_encode_params local_params;
    if (session == 0 || enc == 0 || params == 0) return C89JPEG_ERR_BAD_ARG;
    c89jpeg_tables_session_init(session);
    local_params = *params;
    if (local_params.huffman_mode == C89JPEG_HUFFMAN_OPTIMAL) local_params.huffman_mode = C89JPEG_HUFFMAN_DEFAULT;
    st = c89jpeg_tables_prepare(enc, &local_params, &session->tables);
    if (st != C89JPEG_OK) { session->last_error = st; return st; }
    session->tables_ready = 1;
    session->pixel_format = params->pixel_format;
    session->subsampling = c89jpeg_normalize_session_subsampling(params->pixel_format, params->subsampling);
    session->component_count = (params->pixel_format == C89JPEG_PIXFMT_GRAY8) ? 1 : 3;
    session->last_error = C89JPEG_OK;
    return C89JPEG_OK;
}

c89jpeg_status c89jpeg_tables_session_set(c89jpeg_tables_session *session,
                                          const c89jpeg_tables *tables,
                                          c89jpeg_pixel_format pixel_format,
                                          c89jpeg_subsampling subsampling)
{
    if (session == 0 || tables == 0) return C89JPEG_ERR_BAD_ARG;
    c89jpeg_tables_session_init(session);
    session->tables = *tables;
    session->tables_ready = 1;
    session->pixel_format = pixel_format;
    session->subsampling = c89jpeg_normalize_session_subsampling(pixel_format, subsampling);
    session->component_count = (pixel_format == C89JPEG_PIXFMT_GRAY8) ? 1 : 3;
    session->last_error = C89JPEG_OK;
    return C89JPEG_OK;
}

c89jpeg_status c89jpeg_tables_session_check_image(const c89jpeg_tables_session *session,
                                                  const c89jpeg_encode_params *params)
{
    c89jpeg_subsampling subsampling;
    if (session == 0 || params == 0) return C89JPEG_ERR_BAD_ARG;
    if (!session->tables_ready) return C89JPEG_ERR_BAD_ARG;
    if (params->pixel_format != session->pixel_format) return C89JPEG_ERR_BAD_ARG;
    subsampling = c89jpeg_normalize_session_subsampling(params->pixel_format, params->subsampling);
    if (subsampling != session->subsampling) return C89JPEG_ERR_BAD_ARG;
    return C89JPEG_OK;
}



c89jpeg_status c89jpeg_tables_session_check_image_source(const c89jpeg_tables_session *session,
                                                         const c89jpeg_encode_source_params *params)
{
    c89jpeg_subsampling subsampling;
    if (session == 0 || params == 0) return C89JPEG_ERR_BAD_ARG;
    if (!session->tables_ready) return C89JPEG_ERR_BAD_ARG;
    if (params->pixel_format != session->pixel_format) return C89JPEG_ERR_BAD_ARG;
    subsampling = c89jpeg_normalize_session_subsampling(params->pixel_format, params->subsampling);
    if (subsampling != session->subsampling) return C89JPEG_ERR_BAD_ARG;
    return C89JPEG_OK;
}

c89jpeg_status c89jpeg_tables_session_write_tables_sink(c89jpeg_tables_session *session,
                                                        c89jpeg_write_fn write_fn,
                                                        void *write_user,
                                                        c89jpeg_u32 *bytes_written)
{
    c89jpeg_status st;
    if (session == 0 || write_fn == 0) return C89JPEG_ERR_BAD_ARG;
    if (!session->tables_ready) { session->last_error = C89JPEG_ERR_BAD_ARG; return C89JPEG_ERR_BAD_ARG; }
    st = c89jpeg_write_tables_sink(&session->tables, write_fn, write_user, bytes_written);
    if (st == C89JPEG_OK) session->tables_emitted = 1;
    session->last_error = st;
    return st;
}

c89jpeg_status c89jpeg_tables_session_write_tables_memory(c89jpeg_tables_session *session,
                                                          c89jpeg_u8 *out_buf,
                                                          c89jpeg_u32 out_capacity,
                                                          c89jpeg_u32 *out_size)
{
    c89jpeg_status st;
    if (session == 0 || out_buf == 0) return C89JPEG_ERR_BAD_ARG;
    if (!session->tables_ready) { session->last_error = C89JPEG_ERR_BAD_ARG; return C89JPEG_ERR_BAD_ARG; }
    st = c89jpeg_write_tables_memory(&session->tables, out_buf, out_capacity, out_size);
    if (st == C89JPEG_OK) session->tables_emitted = 1;
    session->last_error = st;
    return st;
}

c89jpeg_status c89jpeg_tables_session_begin_image(c89jpeg_tables_session *session,
                                                  c89jpeg_encoder_resume *resume,
                                                  c89jpeg_encoder *enc,
                                                  const c89jpeg_encode_params *params,
                                                  const c89jpeg_encode_resume_params *resume_params,
                                                  c89jpeg_u32 *out_total_size)
{
    c89jpeg_encode_resume_params local_params;
    c89jpeg_status st;
    if (session == 0 || resume == 0 || enc == 0 || params == 0 || resume_params == 0) return C89JPEG_ERR_BAD_ARG;
    if (!session->tables_ready) { session->last_error = C89JPEG_ERR_BAD_ARG; return C89JPEG_ERR_BAD_ARG; }
    st = c89jpeg_tables_session_check_image(session, params);
    if (st != C89JPEG_OK) { session->last_error = st; return st; }
    local_params = *resume_params;
    local_params.abbreviated = 1;
    local_params.tables = &session->tables;
    st = c89jpeg_encoder_resume_begin(resume, enc, params, &local_params, out_total_size);
    if (st == C89JPEG_OK) session->image_count += 1;
    session->last_error = st;
    return st;
}

c89jpeg_status c89jpeg_tables_session_begin_image_source(c89jpeg_tables_session *session,
                                                         c89jpeg_encoder_resume *resume,
                                                         c89jpeg_encoder *enc,
                                                         const c89jpeg_encode_source_resume_params *params)
{
    c89jpeg_encode_source_resume_params local_params;
    c89jpeg_encode_source_params check_params;
    c89jpeg_status st;
    if (session == 0 || resume == 0 || enc == 0 || params == 0) return C89JPEG_ERR_BAD_ARG;
    if (!session->tables_ready) { session->last_error = C89JPEG_ERR_BAD_ARG; return C89JPEG_ERR_BAD_ARG; }
    c89jpeg_source_resume_to_source_params(params, &check_params);
    st = c89jpeg_tables_session_check_image_source(session, &check_params);
    if (st != C89JPEG_OK) { session->last_error = st; return st; }
    local_params = *params;
    local_params.abbreviated = 1;
    local_params.tables = &session->tables;
    st = c89jpeg_encoder_resume_begin_source(resume, enc, &local_params);
    if (st == C89JPEG_OK) session->image_count += 1;
    session->last_error = st;
    return st;
}


c89jpeg_status c89jpeg_tables_session_encode_image_source_sink(c89jpeg_tables_session *session,
                                                               c89jpeg_encoder *enc,
                                                               const c89jpeg_encode_source_params *params,
                                                               c89jpeg_write_fn write_fn,
                                                               void *write_user,
                                                               c89jpeg_u32 *bytes_written)
{
    c89jpeg_status st;
    if (session == 0 || enc == 0 || params == 0 || write_fn == 0) return C89JPEG_ERR_BAD_ARG;
    if (!session->tables_ready) { session->last_error = C89JPEG_ERR_BAD_ARG; return C89JPEG_ERR_BAD_ARG; }
    st = c89jpeg_tables_session_check_image_source(session, params);
    if (st != C89JPEG_OK) { session->last_error = st; return st; }
    st = c89jpeg_encode_abbreviated_source_sink(enc, params, &session->tables, write_fn, write_user, bytes_written);
    if (st == C89JPEG_OK) session->image_count += 1;
    session->last_error = st;
    return st;
}

c89jpeg_status c89jpeg_tables_session_encode_image_source_memory(c89jpeg_tables_session *session,
                                                                 c89jpeg_encoder *enc,
                                                                 const c89jpeg_encode_source_params *params,
                                                                 c89jpeg_u8 *out_buf,
                                                                 c89jpeg_u32 out_capacity,
                                                                 c89jpeg_u32 *out_size)
{
    c89jpeg_status st;
    if (session == 0 || enc == 0 || params == 0 || out_buf == 0) return C89JPEG_ERR_BAD_ARG;
    if (!session->tables_ready) { session->last_error = C89JPEG_ERR_BAD_ARG; return C89JPEG_ERR_BAD_ARG; }
    st = c89jpeg_tables_session_check_image_source(session, params);
    if (st != C89JPEG_OK) { session->last_error = st; return st; }
    st = c89jpeg_encode_abbreviated_source_memory(enc, params, &session->tables, out_buf, out_capacity, out_size);
    if (st == C89JPEG_OK) session->image_count += 1;
    session->last_error = st;
    return st;
}

c89jpeg_status c89jpeg_tables_session_prepare_trained(c89jpeg_tables_session *session,
                                                      c89jpeg_tables_trainer *trainer,
                                                      c89jpeg_tables *out_tables)
{
    c89jpeg_tables local_tables;
    c89jpeg_status st;
    if (session == 0 || trainer == 0) return C89JPEG_ERR_BAD_ARG;
    st = c89jpeg_tables_trainer_build(trainer, &local_tables);
    if (st != C89JPEG_OK) { session->last_error = st; return st; }
    st = c89jpeg_tables_session_set(session, &local_tables, trainer->pixel_format, trainer->subsampling);
    if (st == C89JPEG_OK && out_tables != 0) *out_tables = local_tables;
    session->last_error = st;
    return st;
}
