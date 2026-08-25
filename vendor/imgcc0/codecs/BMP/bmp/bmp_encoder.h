#ifndef BMP_ENCODER_H
#define BMP_ENCODER_H

#include "../include/bmp/bmp_export.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "bmp_types.h"
#include "bmp_metadata.h"

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

BMP_EXPORT int bmp_encode_rgba32_workspace_size(bmp_u32 width,
                                      bmp_u32 height,
                                      const bmp_encode_options *opt,
                                      bmp_u32 *out_size);

BMP_EXPORT int bmp_encode_indexed_workspace_size_bound(bmp_u32 width,
                                             bmp_u32 height,
                                             bmp_u32 palette_size,
                                             const bmp_encode_options *opt,
                                             bmp_u32 *out_size);

BMP_EXPORT int bmp_encode_rgba32_into(const bmp_u8 *rgba,
                           bmp_u32 width,
                           bmp_u32 height,
                           bmp_u32 stride,
                           const bmp_encode_options *opt,
                           bmp_u8 *out_bmp,
                           bmp_u32 out_capacity,
                           bmp_u32 *out_size,
                           bmp_u8 *workspace,
                           bmp_u32 workspace_size);

BMP_EXPORT int bmp_encode_indexed_into(const bmp_u8 *indices,
                            bmp_u32 width,
                            bmp_u32 height,
                            bmp_u32 stride,
                            const bmp_palette_entry *palette,
                            bmp_u32 palette_size,
                            const bmp_encode_options *opt,
                            bmp_u8 *out_bmp,
                            bmp_u32 out_capacity,
                            bmp_u32 *out_size,
                            bmp_u8 *workspace,
                            bmp_u32 workspace_size);


#ifdef __cplusplus
}
#endif

#endif /* BMP_ENCODER_H */
