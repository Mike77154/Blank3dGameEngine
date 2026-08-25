#ifndef PCX_ENCODER_H
#define PCX_ENCODER_H

#include "pcx_chunk.h"

#include "include/pcx/pcx_export.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PCXEncodeOptionsTag
{
    pcx_u8  version;      /* 0 = auto */
    pcx_u8  encoding;     /* 0 = raw, 1 = RLE */
    pcx_u16 hDPI;
    pcx_u16 vDPI;
    pcx_u16 paletteInfo;  /* 0 = auto(1) */
    pcx_u16 hScreenSize;
    pcx_u16 vScreenSize;
} PCXEncodeOptions;

PCX_EXPORT void pcx_encode_options_default(PCXEncodeOptions *opt);

PCX_EXPORT int pcx_encode_rgb24(const pcx_u8 *rgb,
                     int width,
                     int height,
                     int stride,
                     const PCXEncodeOptions *opt,
                     pcx_u8 **outData,
                     pcx_size *outSize);

PCX_EXPORT int pcx_encode_indexed(const pcx_u8 *indices,
                       int width,
                       int height,
                       int stride,
                       int totalBitsPerPixel,
                       const PCXPalette *palette,
                       const PCXEncodeOptions *opt,
                       pcx_u8 **outData,
                       pcx_size *outSize);

PCX_EXPORT int pcx_encode_image_rgb24(const PCXImage *img,
                           const PCXEncodeOptions *opt,
                           pcx_u8 **outData,
                           pcx_size *outSize);

PCX_EXPORT int pcx_encode_indexed_image(const PCXIndexedImage *img,
                             const PCXEncodeOptions *opt,
                             pcx_u8 **outData,
                             pcx_size *outSize);

PCX_EXPORT int pcx_write_rgb24_file(const char *filename,
                         const pcx_u8 *rgb,
                         int width,
                         int height,
                         int stride,
                         const PCXEncodeOptions *opt);

PCX_EXPORT int pcx_write_indexed_file(const char *filename,
                           const pcx_u8 *indices,
                           int width,
                           int height,
                           int stride,
                           int totalBitsPerPixel,
                           const PCXPalette *palette,
                           const PCXEncodeOptions *opt);

#ifdef __cplusplus
}
#endif

#endif /* PCX_ENCODER_H */
