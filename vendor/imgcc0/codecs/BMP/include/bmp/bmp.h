/* Stable public BMP API. This header is installed; project-internal headers are source-private. */
#ifndef BMP_PUBLIC_H
#define BMP_PUBLIC_H

#include <limits.h>
#include <stdio.h>

#include "bmp_export.h"

#ifdef __cplusplus
extern "C" {
#endif

#if UCHAR_MAX != 255
#error BMP89 requires 8-bit unsigned char
#endif
#if USHRT_MAX != 65535U
#error BMP89 requires 16-bit unsigned short
#endif
#if UINT_MAX != 4294967295U
#error BMP89 requires 32-bit unsigned int
#endif
#if INT_MAX != 2147483647
#error BMP89 requires 32-bit signed int
#endif

typedef unsigned char  bmp_u8;
typedef unsigned short bmp_u16;
typedef unsigned int   bmp_u32;
typedef signed int     bmp_s32;
typedef bmp_s32 bmp_fx16_16;

enum bmp_dib_type {
    BMP_DIB_NONE  = 0,
    BMP_DIB_CORE  = 1,
    BMP_DIB_INFO  = 2,
    BMP_DIB_V2    = 3,
    BMP_DIB_V3    = 4,
    BMP_DIB_V4    = 5,
    BMP_DIB_V5    = 6,
    BMP_DIB_OS2V2 = 7
};

enum bmp_compression {
    BMP_COMP_RGB             = 0,
    BMP_COMP_RLE8            = 1,
    BMP_COMP_RLE4            = 2,
    BMP_COMP_BITFIELDS       = 3,
    BMP_COMP_JPEG            = 4,
    BMP_COMP_PNG             = 5,
    BMP_COMP_ALPHABITFIELDS  = 6
};

enum bmp_color_space_type {
    BMP_CSTYPE_CALIBRATED_RGB = 0x00000000U,
    BMP_CSTYPE_SRGB           = 0x73524742U, /* 'sRGB' */
    BMP_CSTYPE_WINDOWS_COLOR  = 0x57696E20U, /* 'Win ' */
    BMP_CSTYPE_PROFILE_LINKED = 0x4C494E4BU, /* 'LINK' */
    BMP_CSTYPE_PROFILE_EMBEDDED = 0x4D424544U /* 'MBED' */
};

enum bmp_rendering_intent {
    BMP_INTENT_NONE = 0,
    BMP_INTENT_BUSINESS = 1,
    BMP_INTENT_GRAPHICS = 2,
    BMP_INTENT_IMAGES = 4,
    BMP_INTENT_ABSOLUTE_COLORIMETRIC = 8
};

typedef struct bmp_file_header_s {
    bmp_u16 magic;        /* 'BM' = 0x4D42 */
    bmp_u32 file_size;
    bmp_u16 reserved1;
    bmp_u16 reserved2;
    bmp_u32 pixel_offset; /* offset al array de pixeles */
} bmp_file_header;

typedef struct bmp_metadata_s {
    bmp_file_header file;

    bmp_u32 dib_header_size;
    enum bmp_dib_type dib_type;

    bmp_s32 width;
    bmp_s32 height;
    bmp_u16 planes;
    bmp_u16 bpp;
    bmp_u32 compression;
    bmp_u32 image_size;
    bmp_s32 ppm_x;
    bmp_s32 ppm_y;
    bmp_u32 colors_used;
    bmp_u32 colors_important;

    bmp_u32 red_mask;
    bmp_u32 green_mask;
    bmp_u32 blue_mask;
    bmp_u32 alpha_mask;

    bmp_u32 color_space_type;
    bmp_s32 cie_endpoints[9];
    bmp_u32 gamma_red;
    bmp_u32 gamma_green;
    bmp_u32 gamma_blue;
    bmp_u32 rendering_intent;
    bmp_u32 profile_data_offset;
    bmp_u32 profile_size;
    bmp_u32 reserved_v5;

    bmp_u32 palette_offset;
    bmp_u32 palette_entries;
    bmp_u32 palette_entry_size;

    bmp_u32 pixel_array_offset;

    int is_top_down;
} bmp_metadata;

enum bmp_error {
    BMP_OK = 0,
    BMP_ERR_STREAM            = -1,
    BMP_ERR_FORMAT            = -2,
    BMP_ERR_UNSUPPORTED       = -3,
    BMP_ERR_DIMENSIONS        = -4,
    BMP_ERR_ARGUMENT          = -5,
    BMP_ERR_OVERFLOW          = -6,
    BMP_ERR_PALETTE           = -7,
    BMP_ERR_NOMEM             = -8,
    BMP_ERR_BUFFER_TOO_SMALL  = -9,
    BMP_ERR_MASKS             = -10,
    BMP_ERR_LIMITS            = -11
};

typedef struct bmp_limits_s {
    bmp_u32 max_input_bytes;
    bmp_u32 max_width;
    bmp_u32 max_height;
    bmp_u32 max_pixels;
    bmp_u32 max_decoded_bytes;
    bmp_u32 max_palette_entries;
    bmp_u32 max_embedded_payload_bytes;
    bmp_u32 max_icc_profile_bytes;
} bmp_limits;

#define BMP_DEFAULT_MAX_INPUT_BYTES            268435456U
#define BMP_DEFAULT_MAX_WIDTH                  131072U
#define BMP_DEFAULT_MAX_HEIGHT                 131072U
#define BMP_DEFAULT_MAX_PIXELS                 268435456U
#define BMP_DEFAULT_MAX_DECODED_BYTES          1073741824U
#define BMP_DEFAULT_MAX_PALETTE_ENTRIES        4096U
#define BMP_DEFAULT_MAX_EMBEDDED_PAYLOAD_BYTES 134217728U
#define BMP_DEFAULT_MAX_ICC_PROFILE_BYTES      16777216U

BMP_EXPORT void bmp_metadata_default(bmp_metadata *m);
BMP_EXPORT void bmp_limits_default(bmp_limits *limits);
BMP_EXPORT void bmp_set_global_limits(const bmp_limits *limits);
BMP_EXPORT void bmp_get_global_limits(bmp_limits *out_limits);
BMP_WARN_UNUSED_RESULT BMP_EXPORT int bmp_calc_row_stride(bmp_u32 width, bmp_u16 bpp, bmp_u32 *out_stride) BMP_ATTR_NONNULL_1(3);
BMP_WARN_UNUSED_RESULT BMP_EXPORT int bmp_calc_image_size(bmp_u32 width, bmp_u32 height, bmp_u16 bpp, bmp_u32 *out_size) BMP_ATTR_NONNULL_1(4);
BMP_EXPORT const char *bmp_error_string(int code) BMP_RETURNS_NONNULL BMP_CONST_FN;
BMP_EXPORT const char *bmp_error_description(int code) BMP_RETURNS_NONNULL BMP_CONST_FN;

typedef struct bmp_stream_s {
    const bmp_u8 *data;
    bmp_u32 size;
    bmp_u32 pos;
    int error;
} bmp_stream;

BMP_EXPORT void bmp_stream_init(bmp_stream *s,
                         BMP_SAL_IN_READS_BYTES(size) const bmp_u8 *data,
                         bmp_u32 size) BMP_ATTR_NONNULL_2(1, 2) BMP_ATTR_ACCESS_RO_2(2, 3);

BMP_WARN_UNUSED_RESULT BMP_EXPORT bmp_u8 bmp_read_u8 (bmp_stream *s) BMP_ATTR_NONNULL_1(1);
BMP_WARN_UNUSED_RESULT BMP_EXPORT bmp_u16 bmp_read_u16(bmp_stream *s) BMP_ATTR_NONNULL_1(1);
BMP_WARN_UNUSED_RESULT BMP_EXPORT bmp_u32 bmp_read_u32(bmp_stream *s) BMP_ATTR_NONNULL_1(1);
BMP_WARN_UNUSED_RESULT BMP_EXPORT bmp_s32 bmp_read_s32(bmp_stream *s) BMP_ATTR_NONNULL_1(1);

BMP_EXPORT void bmp_stream_skip(bmp_stream *s, bmp_u32 count) BMP_ATTR_NONNULL_1(1);
BMP_EXPORT void bmp_stream_seek(bmp_stream *s, bmp_u32 pos) BMP_ATTR_NONNULL_1(1);
BMP_WARN_UNUSED_RESULT BMP_EXPORT const bmp_u8 *bmp_stream_peek(bmp_stream *s BMP_LIFETIMEBOUND, bmp_u32 count) BMP_ATTR_NONNULL_1(1);
BMP_WARN_UNUSED_RESULT BMP_EXPORT bmp_u32 bmp_stream_remaining(const bmp_stream *s) BMP_ATTR_NONNULL_1(1);
BMP_WARN_UNUSED_RESULT BMP_EXPORT int bmp_stream_failed(const bmp_stream *s) BMP_ATTR_NONNULL_1(1);

enum bmp_warning {
    BMP_WARN_NONE                              = 0U,
    BMP_WARN_FILE_SIZE_HEADER_ZERO            = 1U << 0,
    BMP_WARN_FILE_SIZE_HEADER_SMALLER_ACTUAL  = 1U << 1,
    BMP_WARN_IMAGE_SIZE_ZERO_COMPRESSED       = 1U << 2,
    BMP_WARN_TRUECOLOR_COLORS_USED_NONZERO    = 1U << 3,
    BMP_WARN_COLORS_IMPORTANT_EXCEEDS_PALETTE = 1U << 4,
    BMP_WARN_PROFILE_SIZE_WITHOUT_PROFILE_CST = 1U << 5,
    BMP_WARN_TOP_DOWN                         = 1U << 6,
    BMP_WARN_EMBEDDED_PAYLOAD                 = 1U << 7
};

typedef struct bmp_diagnostics_s {
    bmp_u32 warning_mask;
    int payload_signature_ok;
} bmp_diagnostics;

typedef struct bmp_image_s {
    bmp_metadata meta;
    const bmp_u8 *palette;
    bmp_u32 palette_entry_size;
    const bmp_u8 *pixel_data;
    bmp_u32 pixel_data_size;
    const bmp_u8 *icc_profile_data;
    bmp_diagnostics diagnostics;
} bmp_image;

BMP_EXPORT void bmp_diagnostics_default(bmp_diagnostics *diag);
BMP_WARN_UNUSED_RESULT BMP_EXPORT int bmp_warning_mask_has(bmp_u32 mask, bmp_u32 flag) BMP_CONST_FN;
BMP_EXPORT const char *bmp_warning_string(bmp_u32 flag) BMP_RETURNS_NONNULL BMP_CONST_FN;
BMP_WARN_UNUSED_RESULT BMP_EXPORT int bmp_collect_diagnostics(const bmp_image *img, bmp_diagnostics *out_diag) BMP_ATTR_NONNULL_2(1, 2);

BMP_WARN_UNUSED_RESULT BMP_EXPORT int bmp_parse_memory(BMP_SAL_IN_READS_BYTES(size) const bmp_u8 *data, bmp_u32 size, bmp_image *out_img) BMP_ATTR_NONNULL_2(1, 3) BMP_ATTR_ACCESS_RO_2(1, 2);
BMP_WARN_UNUSED_RESULT BMP_EXPORT int bmp_parse_memory_with_limits(BMP_SAL_IN_READS_BYTES(size) const bmp_u8 *data,
                                 bmp_u32 size,
                                 const bmp_limits *limits,
                                 bmp_image *out_img) BMP_ATTR_NONNULL_2(1, 4) BMP_ATTR_ACCESS_RO_2(1, 2);
BMP_WARN_UNUSED_RESULT BMP_EXPORT int bmp_image_has_embedded_payload(const bmp_image *img) BMP_ATTR_NONNULL_1(1);
BMP_WARN_UNUSED_RESULT BMP_EXPORT int bmp_validate_embedded_payload_signature(const bmp_image *img) BMP_ATTR_NONNULL_1(1);
BMP_WARN_UNUSED_RESULT BMP_EXPORT int bmp_embedded_payload_signature_matches(const bmp_image *img) BMP_ATTR_NONNULL_1(1) BMP_PURE_FN;
BMP_WARN_UNUSED_RESULT BMP_EXPORT const char *bmp_embedded_payload_extension(const bmp_image *img) BMP_ATTR_NONNULL_1(1) BMP_PURE_FN;
BMP_WARN_UNUSED_RESULT BMP_EXPORT int bmp_get_embedded_payload(const bmp_image *img,
                             BMP_SAL_OUT bmp_u32 *out_compression,
                             BMP_SAL_OUT const bmp_u8 **out_data,
                             BMP_SAL_OUT bmp_u32 *out_size) BMP_ATTR_NONNULL_4(1, 2, 3, 4);
BMP_WARN_UNUSED_RESULT BMP_EXPORT int bmp_copy_embedded_payload_into(const bmp_image *img,
                                   BMP_SAL_OUT bmp_u32 *out_compression,
                                   BMP_SAL_OUT_WRITES_BYTES(out_capacity) bmp_u8 *out_data,
                                   bmp_u32 out_capacity,
                                   BMP_SAL_OUT bmp_u32 *out_size) BMP_ATTR_NONNULL_4(1, 2, 3, 5) BMP_ATTR_ACCESS_WO_2(3, 4);

#define BMP_VERSION_MAJOR 1
#define BMP_VERSION_MINOR 16
#define BMP_VERSION_PATCH 0
#define BMP_VERSION_STRING "1.16.0"
#define BMP_VERSION_NUMBER ((bmp_u32)(BMP_VERSION_MAJOR * 10000U + BMP_VERSION_MINOR * 100U + BMP_VERSION_PATCH))

BMP_EXPORT const char *bmp_version_string(void) BMP_RETURNS_NONNULL BMP_CONST_FN;
BMP_EXPORT bmp_u32 bmp_version_number(void) BMP_CONST_FN;
BMP_EXPORT const char *bmp_dib_type_string(enum bmp_dib_type type) BMP_RETURNS_NONNULL BMP_CONST_FN;
BMP_EXPORT const char *bmp_compression_string(bmp_u32 compression) BMP_RETURNS_NONNULL BMP_CONST_FN;
BMP_WARN_UNUSED_RESULT BMP_EXPORT int         bmp_warning_count(bmp_u32 mask);

BMP_WARN_UNUSED_RESULT BMP_EXPORT int bmp_format_diagnostics_text(const bmp_image *img,
                                BMP_SAL_OUT_WRITES_BYTES(buffer_size) char *buffer,
                                bmp_u32 buffer_size) BMP_ATTR_NONNULL_1(1) BMP_ATTR_ACCESS_WO_2(2, 3);
BMP_WARN_UNUSED_RESULT BMP_EXPORT int bmp_format_diagnostics_json(const bmp_image *img,
                                BMP_SAL_OUT_WRITES_BYTES(buffer_size) char *buffer,
                                bmp_u32 buffer_size) BMP_ATTR_NONNULL_1(1) BMP_ATTR_ACCESS_WO_2(2, 3);
BMP_WARN_UNUSED_RESULT BMP_EXPORT int bmp_write_diagnostics_text(FILE *f, const bmp_image *img) BMP_ATTR_NONNULL_2(1, 2);
BMP_WARN_UNUSED_RESULT BMP_EXPORT int bmp_write_diagnostics_json(FILE *f, const bmp_image *img) BMP_ATTR_NONNULL_2(1, 2);

BMP_WARN_UNUSED_RESULT BMP_EXPORT int bmp_calc_rgba32_buffer_size(const bmp_image *img,
                                bmp_u32 *out_size) BMP_ATTR_NONNULL_2(1, 2);

BMP_WARN_UNUSED_RESULT BMP_EXPORT int bmp_decode_to_rgba32(const bmp_image *img,
                         BMP_SAL_OUT_WRITES_BYTES((bmp_u32)out_stride * (bmp_u32)(img->meta.height < 0 ? -img->meta.height : img->meta.height)) bmp_u8 *out_rgba,
                         bmp_u32 out_stride) BMP_ATTR_NONNULL_2(1, 2) BMP_ATTR_ACCESS_WO_1(2);
BMP_WARN_UNUSED_RESULT BMP_EXPORT int bmp_decode_to_rgba32_with_limits(const bmp_image *img,
                                     const bmp_limits *limits,
                                     BMP_SAL_OUT_WRITES_BYTES((bmp_u32)out_stride * (bmp_u32)(img->meta.height < 0 ? -img->meta.height : img->meta.height)) bmp_u8 *out_rgba,
                                     bmp_u32 out_stride) BMP_ATTR_NONNULL_2(1, 3) BMP_ATTR_ACCESS_WO_1(3);

typedef struct bmp_palette_entry_s {
    bmp_u8 b;
    bmp_u8 g;
    bmp_u8 r;
    bmp_u8 a;
} bmp_palette_entry;

enum bmp_encode_format {
    BMP_ENC_FMT_INDEXED1       = 1,
    BMP_ENC_FMT_INDEXED4       = 4,
    BMP_ENC_FMT_INDEXED8       = 8,
    BMP_ENC_FMT_RGB555         = 16,
    BMP_ENC_FMT_RGB565         = 17,
    BMP_ENC_FMT_BGR24          = 24,
    BMP_ENC_FMT_BGRX32         = 32,
    BMP_ENC_FMT_BGRA32         = 33,
    BMP_ENC_FMT_16_BITFIELDS   = 34,
    BMP_ENC_FMT_32_BITFIELDS   = 35
};

enum bmp_encode_compression_mode {
    BMP_ENC_COMP_AUTO             = 0,
    BMP_ENC_COMP_RGB              = 1,
    BMP_ENC_COMP_RLE4             = 2,
    BMP_ENC_COMP_RLE8             = 3,
    BMP_ENC_COMP_BITFIELDS        = 4,
    BMP_ENC_COMP_ALPHABITFIELDS   = 5
};

typedef struct bmp_encode_options_s {
    bmp_u32 header_size; /* 0 = auto, or 12/40/52/56/108/124 */
    bmp_u32 format;
    bmp_u32 compression; /* enum bmp_encode_compression_mode */
    bmp_s32 ppm_x;
    bmp_s32 ppm_y;
    bmp_u32 colors_important;
    int top_down;

    bmp_u32 red_mask;
    bmp_u32 green_mask;
    bmp_u32 blue_mask;
    bmp_u32 alpha_mask;

    bmp_u32 color_space_type;
    bmp_u32 rendering_intent;
    const bmp_u8 *icc_profile_data;
    bmp_u32 icc_profile_size;
} bmp_encode_options;

BMP_EXPORT void bmp_encode_options_default(bmp_encode_options *opt);

BMP_WARN_UNUSED_RESULT BMP_EXPORT int bmp_encode_rgba32_workspace_size(bmp_u32 width,
                                      bmp_u32 height,
                                      const bmp_encode_options *opt,
                                      bmp_u32 *out_size);
BMP_WARN_UNUSED_RESULT BMP_EXPORT int bmp_encode_indexed_workspace_size_bound(bmp_u32 width,
                                             bmp_u32 height,
                                             bmp_u32 palette_size,
                                             const bmp_encode_options *opt,
                                             bmp_u32 *out_size);
BMP_WARN_UNUSED_RESULT BMP_EXPORT int bmp_encode_rgba32_into(BMP_SAL_IN const bmp_u8 *rgba,
                           bmp_u32 width,
                           bmp_u32 height,
                           bmp_u32 stride,
                           const bmp_encode_options *opt,
                           BMP_SAL_OUT_WRITES_BYTES(out_capacity) bmp_u8 *out_bmp,
                           bmp_u32 out_capacity,
                           BMP_SAL_OUT bmp_u32 *out_size,
                           BMP_SAL_INOUT_UPDATES_BYTES(workspace_size) bmp_u8 *workspace,
                           bmp_u32 workspace_size) BMP_ATTR_NONNULL_5(1, 5, 6, 8, 9) BMP_ATTR_ACCESS_WO_2(6, 7) BMP_ATTR_ACCESS_RW_2(9, 10);
BMP_WARN_UNUSED_RESULT BMP_EXPORT int bmp_encode_indexed_into(BMP_SAL_IN const bmp_u8 *indices,
                            bmp_u32 width,
                            bmp_u32 height,
                            bmp_u32 stride,
                            BMP_SAL_IN const bmp_palette_entry *palette,
                            bmp_u32 palette_size,
                            const bmp_encode_options *opt,
                            BMP_SAL_OUT_WRITES_BYTES(out_capacity) bmp_u8 *out_bmp,
                            bmp_u32 out_capacity,
                            BMP_SAL_OUT bmp_u32 *out_size,
                            BMP_SAL_INOUT_UPDATES_BYTES(workspace_size) bmp_u8 *workspace,
                            bmp_u32 workspace_size) BMP_ATTR_NONNULL_6(1, 5, 7, 8, 10, 11) BMP_ATTR_ACCESS_WO_2(8, 9) BMP_ATTR_ACCESS_RW_2(11, 12);

/* Filtros sobre un buffer RGBA32 ya decodificado */

BMP_EXPORT void bmp_filter_flip_vertical_rgba32(BMP_SAL_INOUT_UPDATES_BYTES((bmp_u32)stride * (bmp_u32)height) bmp_u8 *pixels,
                                     bmp_u32 width,
                                     bmp_u32 height,
                                     bmp_u32 stride) BMP_ATTR_NONNULL_1(1) BMP_ATTR_ACCESS_RW_1(1);

BMP_EXPORT void bmp_filter_invert_rgba32(BMP_SAL_INOUT_UPDATES_BYTES((bmp_u32)stride * (bmp_u32)height) bmp_u8 *pixels,
                              bmp_u32 width,
                              bmp_u32 height,
                              bmp_u32 stride) BMP_ATTR_NONNULL_1(1) BMP_ATTR_ACCESS_RW_1(1);

BMP_EXPORT void bmp_filter_grayscale_rgba32(BMP_SAL_INOUT_UPDATES_BYTES((bmp_u32)stride * (bmp_u32)height) bmp_u8 *pixels,
                                 bmp_u32 width,
                                 bmp_u32 height,
                                 bmp_u32 stride) BMP_ATTR_NONNULL_1(1) BMP_ATTR_ACCESS_RW_1(1);

/* Callback genérico para pintar un pixel RGBA en tu consola */
typedef void (*bmp_put_pixel_fn)(void *user,
                                 bmp_u32 x,
                                 bmp_u32 y,
                                 bmp_u8 r,
                                 bmp_u8 g,
                                 bmp_u8 b,
                                 bmp_u8 a);

/* Renderiza un buffer RGBA32 ya decodificado a un callback */
BMP_EXPORT void bmp_render_rgba32_to_callback(BMP_SAL_IN_READS_BYTES((bmp_u32)stride * (bmp_u32)height) const bmp_u8 *pixels,
                                   bmp_u32 width,
                                   bmp_u32 height,
                                   bmp_u32 stride,
                                   bmp_put_pixel_fn put_pixel,
                                   void *user_data) BMP_ATTR_NONNULL_2(1, 5) BMP_ATTR_ACCESS_RO_1(1);

#ifdef __cplusplus
}
#endif

#endif /* BMP_PUBLIC_H */
