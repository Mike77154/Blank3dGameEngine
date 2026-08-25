#ifndef MFT_MFONT_TYPES_H
#define MFT_MFONT_TYPES_H

#include <limits.h>

#include "mfont_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char mft_u8;

#if USHRT_MAX == 0xFFFFU
typedef unsigned short mft_u16;
typedef signed short mft_s16;
#else
#error "mfont requires a 16-bit unsigned short"
#endif

#if UINT_MAX == 0xFFFFFFFFU
typedef unsigned int mft_u32;
typedef signed int mft_s32;
#elif ULONG_MAX == 0xFFFFFFFFUL
typedef unsigned long mft_u32;
typedef signed long mft_s32;
#else
#error "mfont requires a native 32-bit unsigned type"
#endif

typedef signed char mft_s8;

typedef enum mft_status_tag {
    MFT_OK = 0,
    MFT_ERR_ARGS = -1,
    MFT_ERR_SIGNATURE = -2,
    MFT_ERR_BOUNDS = -3,
    MFT_ERR_CAPACITY = -4,
    MFT_ERR_FORMAT = -5,
    MFT_ERR_UNSUPPORTED = -6,
    MFT_ERR_CHECKSUM = -7,
    MFT_ERR_TEXT = -8,
    MFT_ERR_IMAGE = -9,
    MFT_ERR_IO = -10,
    MFT_ERR_INTERNAL = -11
} mft_status;

typedef enum mft_pixel_format_tag {
    MFT_PIXFMT_INDEX8 = 1,
    MFT_PIXFMT_RGB24 = 2,
    MFT_PIXFMT_RGBA32 = 3
} mft_pixel_format;

typedef struct mft_rgba_tag {
    mft_u8 r;
    mft_u8 g;
    mft_u8 b;
    mft_u8 a;
} mft_rgba;

typedef struct mft_image_tag {
    mft_u32 width;
    mft_u32 height;
    mft_u32 stride;
    mft_pixel_format format;
    mft_u8 *pixels;
    mft_u32 pixels_capacity;
    mft_rgba palette[256];
    mft_u32 palette_count;
} mft_image;

typedef enum mft_font_kind_tag {
    MFT_FONT_FIXED = 1,
    MFT_FONT_VARIABLE = 2
} mft_font_kind;

typedef struct mft_glyph_tag {
    mft_u8 code;
    mft_s32 offset;
    mft_s32 width;
    mft_u8 present;
} mft_glyph;

typedef struct mft_font_text_tag {
    mft_font_kind type;
    mft_s32 offset_x;
    mft_s32 offset_y;
    mft_s32 size_w;
    mft_s32 size_h;
    mft_s32 spacing_x;
    mft_s32 spacing_y;
    mft_s32 colors;
    char sprites[MFT_CFG_MAX_PATH];
    mft_glyph glyphs[256];
    mft_u32 glyph_count;
} mft_font_text;

typedef struct mft_fnt_header_tag {
    char signature[12];
    mft_u16 ver_hi;
    mft_u16 ver_lo;
    mft_u32 pcx_offset;
    mft_u32 pcx_size;
    mft_u32 text_offset;
    mft_u32 text_size;
    char comment[32];
} mft_fnt_header;

#ifdef __cplusplus
}
#endif

#endif
