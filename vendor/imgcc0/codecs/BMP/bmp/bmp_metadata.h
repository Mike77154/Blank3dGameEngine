#ifndef BMP_METADATA_H
#define BMP_METADATA_H

#include "../include/bmp/bmp_export.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "bmp_types.h"

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
BMP_EXPORT int bmp_calc_row_stride(bmp_u32 width, bmp_u16 bpp, bmp_u32 *out_stride);
BMP_EXPORT int bmp_calc_image_size(bmp_u32 width, bmp_u32 height, bmp_u16 bpp, bmp_u32 *out_size);
BMP_EXPORT const char *bmp_error_string(int code) BMP_RETURNS_NONNULL BMP_CONST_FN;
BMP_EXPORT const char *bmp_error_description(int code) BMP_RETURNS_NONNULL BMP_CONST_FN;

#ifdef __cplusplus
}
#endif

#endif /* BMP_METADATA_H */
