#ifndef BMP_PARSER_H
#define BMP_PARSER_H

#include "../include/bmp/bmp_export.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "bmp_metadata.h"
#include "bmp_chunks.h"

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
BMP_EXPORT int bmp_warning_mask_has(bmp_u32 mask, bmp_u32 flag) BMP_CONST_FN;
BMP_EXPORT const char *bmp_warning_string(bmp_u32 flag) BMP_RETURNS_NONNULL BMP_CONST_FN;
BMP_EXPORT int bmp_collect_diagnostics(const bmp_image *img, bmp_diagnostics *out_diag);

BMP_EXPORT int bmp_parse_memory(const bmp_u8 *data, bmp_u32 size, bmp_image *out_img);
BMP_EXPORT int bmp_parse_memory_with_limits(const bmp_u8 *data,
                                 bmp_u32 size,
                                 const bmp_limits *limits,
                                 bmp_image *out_img);
BMP_EXPORT int bmp_image_has_embedded_payload(const bmp_image *img);
BMP_EXPORT int bmp_validate_embedded_payload_signature(const bmp_image *img);
BMP_EXPORT int bmp_embedded_payload_signature_matches(const bmp_image *img) BMP_PURE_FN;
BMP_EXPORT const char *bmp_embedded_payload_extension(const bmp_image *img) BMP_PURE_FN;
BMP_EXPORT int bmp_get_embedded_payload(const bmp_image *img,
                             bmp_u32 *out_compression,
                             const bmp_u8 **out_data,
                             bmp_u32 *out_size);
BMP_EXPORT int bmp_copy_embedded_payload_into(const bmp_image *img,
                                   bmp_u32 *out_compression,
                                   bmp_u8 *out_data,
                                   bmp_u32 out_capacity,
                                   bmp_u32 *out_size);

#ifdef __cplusplus
}
#endif

#endif /* BMP_PARSER_H */
