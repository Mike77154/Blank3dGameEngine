/* pcx_chunk.h - estructuras básicas y utilidades de bajo nivel para PCX */

#ifndef PCX_CHUNK_H
#define PCX_CHUNK_H

#include <stdio.h>
#include <limits.h>

#include "include/pcx/pcx_export.h"
#include "pcx_config89.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Tipos base
 * ------------------------------------------------------------------------- */

typedef unsigned char  pcx_u8;
typedef unsigned short pcx_u16;
typedef unsigned int   pcx_u32;
typedef unsigned int   pcx_size;
typedef int            pcx_off;

/* This sanitized build requires 32-bit unsigned int semantics. */
typedef char pcx_require_u32[(UINT_MAX == 4294967295U) ? 1 : -1];

/* -------------------------------------------------------------------------
 * Códigos de resultado
 *
 * Se mantienen valores negativos para ser compatibles con el estilo original
 * del proyecto y facilitar integración con código que esperaba enteros.
 * ------------------------------------------------------------------------- */

typedef enum PCXResultTag
{
    PCX_OK                = 0,
    PCX_ERR_IO            = -1,
    PCX_ERR_FORMAT        = -2,
    PCX_ERR_UNSUPPORTED   = -3,
    PCX_ERR_OUT_OF_MEMORY = -4,
    PCX_ERR_EOF           = -5,
    PCX_ERR_OVERFLOW      = -6,
    PCX_ERR_INTERNAL      = -7,
    PCX_ERR_NULL_POINTER  = -8,
    PCX_ERR_DIMENSIONS    = -9,
    PCX_ERR_LIMITS        = -10
} PCXResult;

/* Alias histórico */
#define PCX_ERR_MEMORY PCX_ERR_OUT_OF_MEMORY

/* Booleano simple compatible con C89 */
typedef enum PCXBoolTag
{
    PCX_FALSE = 0,
    PCX_TRUE  = 1
} PCXBool;

/* -------------------------------------------------------------------------
 * Cabecera PCX en formato lógico ya parseado
 * ------------------------------------------------------------------------- */

typedef struct PCXHeaderTag
{
    pcx_u8   manufacturer;   /* 0x0A */
    pcx_u8   version;
    pcx_u8   encoding;       /* 0 = raw (compat), 1 = RLE */
    pcx_u8   bitsPerPixel;   /* bits por píxel dentro de cada plano */
    pcx_u16  xMin;
    pcx_u16  yMin;
    pcx_u16  xMax;
    pcx_u16  yMax;
    pcx_u16  hDPI;
    pcx_u16  vDPI;
    pcx_u8   colorMap[48];   /* 16 * RGB */
    pcx_u8   reserved;
    pcx_u8   nPlanes;
    pcx_u16  bytesPerLine;   /* bytes por scanline y por plano */
    pcx_u16  paletteInfo;    /* 1=color, 2=grayscale (hint histórico) */
    pcx_u16  hScreenSize;
    pcx_u16  vScreenSize;
    pcx_u8   filler[54];
} PCXHeader;

/* Imagen final decodificada */
typedef struct PCXImageTag
{
    int     width;
    int     height;
    int     channels;   /* típicamente 3 (RGB) */
    pcx_u8 *pixels;     /* width * height * channels */
} PCXImage;

/* Paleta RGB de hasta 256 entradas */
typedef struct PCXPaletteTag
{
    pcx_u8 colors[256][3];
    int    isValid;
} PCXPalette;

/* Imagen indexada preservando índices + paleta. */
typedef struct PCXIndexedImageTag
{
    int        width;
    int        height;
    int        totalBitsPerPixel;
    pcx_u8    *indices;   /* width * height */
    PCXPalette palette;
} PCXIndexedImage;

/* -------------------------------------------------------------------------
 * E/S básica
 * ------------------------------------------------------------------------- */

PCX_EXPORT int pcx_read_u8(FILE *f, pcx_u8 *value);
PCX_EXPORT int pcx_read_u16_le(FILE *f, pcx_u16 *value);
PCX_EXPORT int pcx_read_bytes(FILE *f, void *buffer, pcx_size count);
PCX_EXPORT int pcx_skip_bytes(FILE *f, pcx_off count);

/* -------------------------------------------------------------------------
 * Almacenamiento estático / buffers de usuario
 * ------------------------------------------------------------------------- */

PCX_EXPORT void      pcx_image_init(PCXImage *img);
PCX_EXPORT void      pcx_image_release(PCXImage *img);
PCX_EXPORT PCXResult pcx_image_use_static(PCXImage *img, int width, int height, int channels);
PCX_EXPORT PCXResult pcx_image_use_buffer(PCXImage *img,
                               int width,
                               int height,
                               int channels,
                               pcx_u8 *pixels);

PCX_EXPORT void      pcx_indexed_image_init(PCXIndexedImage *img);
PCX_EXPORT void      pcx_indexed_image_release(PCXIndexedImage *img);
PCX_EXPORT PCXResult pcx_indexed_image_use_static(PCXIndexedImage *img,
                                  int width,
                                  int height,
                                  int totalBitsPerPixel);
PCX_EXPORT PCXResult pcx_indexed_image_use_buffer(PCXIndexedImage *img,
                                       int width,
                                       int height,
                                       int totalBitsPerPixel,
                                       pcx_u8 *indices);

PCX_EXPORT void      pcx_palette_init(PCXPalette *pal);
PCX_EXPORT PCXBool   pcx_palette_is_valid(const PCXPalette *pal) PCX_PURE_FN;

/* -------------------------------------------------------------------------
 * Aritmética segura
 * ------------------------------------------------------------------------- */

PCX_EXPORT PCXBool pcx_mul_int_safe(int a, int b, int *out);
PCX_EXPORT PCXBool pcx_add_int_safe(int a, int b, int *out);
PCX_EXPORT PCXBool pcx_mul_pcx_size_safe(pcx_size a, pcx_size b, pcx_size *out);
PCX_EXPORT PCXBool pcx_add_pcx_size_safe(pcx_size a, pcx_size b, pcx_size *out);
PCX_EXPORT PCXBool pcx_image_calc_buffer_size(int width,
                                   int height,
                                   int channels,
                                   pcx_size *outSize);

/* -------------------------------------------------------------------------
 * Helpers de cabecera
 * ------------------------------------------------------------------------- */

PCX_EXPORT PCXBool pcx_header_get_dimensions(const PCXHeader *hdr,
                                  int *outWidth,
                                  int *outHeight);
PCX_EXPORT PCXBool pcx_header_get_total_bpp(const PCXHeader *hdr, int *outTotalBits);
PCX_EXPORT PCXBool pcx_header_get_min_bytes_per_line(const PCXHeader *hdr,
                                          int width,
                                          int *outMinBytesPerLine);
PCX_EXPORT PCXBool pcx_header_is_basic_valid(const PCXHeader *hdr) PCX_PURE_FN;
PCX_EXPORT PCXBool pcx_header_is_indexed8(const PCXHeader *hdr) PCX_PURE_FN;
PCX_EXPORT PCXBool pcx_header_is_indexed4(const PCXHeader *hdr) PCX_PURE_FN;
PCX_EXPORT PCXBool pcx_header_is_truecolor24(const PCXHeader *hdr) PCX_PURE_FN;
PCX_EXPORT PCXBool pcx_header_uses_extended_palette(const PCXHeader *hdr) PCX_PURE_FN;
PCX_EXPORT PCXBool pcx_header_is_grayscale_hint(const PCXHeader *hdr) PCX_PURE_FN;

/* -------------------------------------------------------------------------
 * Diagnóstico
 * ------------------------------------------------------------------------- */

typedef enum PCXPaletteSourceTag
{
    PCX_PALETTE_SOURCE_UNKNOWN            = 0,
    PCX_PALETTE_SOURCE_HEADER16           = 1,
    PCX_PALETTE_SOURCE_GRAYSCALE_FALLBACK = 2,
    PCX_PALETTE_SOURCE_EXTENDED_VGA       = 3,
    PCX_PALETTE_SOURCE_HEADER16_FALLBACK  = 4
} PCXPaletteSource;

enum PCXWarningTag
{
    PCX_WARN_NONE                     = 0U,
    PCX_WARN_UNKNOWN_VERSION          = 1U << 0,
    PCX_WARN_RAW_ENCODING             = 1U << 1,
    PCX_WARN_RESERVED_NONZERO         = 1U << 2,
    PCX_WARN_ODD_BYTES_PER_LINE       = 1U << 3,
    PCX_WARN_STRICT_LAYOUT_UNSUPPORTED= 1U << 4,
    PCX_WARN_EXTENDED_PALETTE_MISSING = 1U << 5,
    PCX_WARN_GRAYSCALE_FALLBACK       = 1U << 6,
    PCX_WARN_HEADER16_PALETTE_FALLBACK= 1U << 7,
    PCX_WARN_SCANLINE_PADDING         = 1U << 8
};

typedef struct PCXDiagnosticsTag
{
    pcx_u32    warningMask;
    int              strictWouldFail;
    PCXPaletteSource paletteSource;
} PCXDiagnostics;

PCX_EXPORT void        pcx_diagnostics_init(PCXDiagnostics *diag);
PCX_EXPORT int         pcx_warning_mask_has(pcx_u32 mask, pcx_u32 flag) PCX_CONST_FN;
PCX_EXPORT const char *pcx_warning_flag_to_string(pcx_u32 flag) PCX_RETURNS_NONNULL PCX_CONST_FN;
PCX_EXPORT const char *pcx_palette_source_to_string(PCXPaletteSource source) PCX_RETURNS_NONNULL PCX_CONST_FN;
PCX_EXPORT const char *pcx_result_to_string(PCXResult result) PCX_RETURNS_NONNULL PCX_CONST_FN;
PCX_EXPORT const char *pcx_result_description(PCXResult result) PCX_RETURNS_NONNULL PCX_CONST_FN;

#ifdef __cplusplus
}
#endif

#endif /* PCX_CHUNK_H */
