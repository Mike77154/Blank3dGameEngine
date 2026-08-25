/* Stable public PCX API. This header is installed; project-internal headers are source-private. */
#ifndef PCX_PUBLIC_H
#define PCX_PUBLIC_H

#include <stdio.h>
#include <limits.h>

#include "pcx_export.h"
#include "pcx_config89.h"

#ifdef __cplusplus
extern "C" {
#endif

/* pcx_chunk.h - estructuras básicas y utilidades de bajo nivel para PCX */




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

PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_read_u8(FILE *f, pcx_u8 *value) PCX_ATTR_NONNULL_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_read_u16_le(FILE *f, pcx_u16 *value) PCX_ATTR_NONNULL_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_read_bytes(FILE *f,
                                      PCX_SAL_OUT_WRITES_BYTES(count) void *buffer,
                                      pcx_size count) PCX_ATTR_NONNULL_2(1, 2) PCX_ATTR_ACCESS_WO_2(2, 3);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_skip_bytes(FILE *f, pcx_off count) PCX_ATTR_NONNULL_1(1);

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
                               pcx_u8 *pixels) PCX_ATTR_NONNULL_2(1, 5);

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
                                       pcx_u8 *indices) PCX_ATTR_NONNULL_2(1, 5);

PCX_EXPORT void      pcx_palette_init(PCXPalette *pal);
PCX_EXPORT PCXBool   pcx_palette_is_valid(const PCXPalette *pal) PCX_PURE_FN;

/* -------------------------------------------------------------------------
 * Aritmética segura
 * ------------------------------------------------------------------------- */

PCX_WARN_UNUSED_RESULT PCX_EXPORT PCXBool pcx_mul_int_safe(int a, int b, int *out) PCX_ATTR_NONNULL_1(3);
PCX_WARN_UNUSED_RESULT PCX_EXPORT PCXBool pcx_add_int_safe(int a, int b, int *out) PCX_ATTR_NONNULL_1(3);
PCX_WARN_UNUSED_RESULT PCX_EXPORT PCXBool pcx_mul_pcx_size_safe(pcx_size a, pcx_size b, pcx_size *out) PCX_ATTR_NONNULL_1(3);
PCX_WARN_UNUSED_RESULT PCX_EXPORT PCXBool pcx_add_pcx_size_safe(pcx_size a, pcx_size b, pcx_size *out) PCX_ATTR_NONNULL_1(3);
PCX_WARN_UNUSED_RESULT PCX_EXPORT PCXBool pcx_image_calc_buffer_size(int width,
                                   int height,
                                   int channels,
                                   pcx_size *outSize) PCX_ATTR_NONNULL_1(4);

/* -------------------------------------------------------------------------
 * Helpers de cabecera
 * ------------------------------------------------------------------------- */

PCX_WARN_UNUSED_RESULT PCX_EXPORT PCXBool pcx_header_get_dimensions(const PCXHeader *hdr,
                                  int *outWidth,
                                  int *outHeight) PCX_ATTR_NONNULL_3(1, 2, 3);
PCX_WARN_UNUSED_RESULT PCX_EXPORT PCXBool pcx_header_get_total_bpp(const PCXHeader *hdr, int *outTotalBits) PCX_ATTR_NONNULL_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT PCXBool pcx_header_get_min_bytes_per_line(const PCXHeader *hdr,
                                          int width,
                                          int *outMinBytesPerLine) PCX_ATTR_NONNULL_2(1, 3);
PCX_WARN_UNUSED_RESULT PCX_EXPORT PCXBool pcx_header_is_basic_valid(const PCXHeader *hdr) PCX_PURE_FN;
PCX_WARN_UNUSED_RESULT PCX_EXPORT PCXBool pcx_header_is_indexed8(const PCXHeader *hdr) PCX_PURE_FN;
PCX_WARN_UNUSED_RESULT PCX_EXPORT PCXBool pcx_header_is_indexed4(const PCXHeader *hdr) PCX_PURE_FN;
PCX_WARN_UNUSED_RESULT PCX_EXPORT PCXBool pcx_header_is_truecolor24(const PCXHeader *hdr) PCX_PURE_FN;
PCX_WARN_UNUSED_RESULT PCX_EXPORT PCXBool pcx_header_uses_extended_palette(const PCXHeader *hdr) PCX_PURE_FN;
PCX_WARN_UNUSED_RESULT PCX_EXPORT PCXBool pcx_header_is_grayscale_hint(const PCXHeader *hdr) PCX_PURE_FN;

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
PCX_WARN_UNUSED_RESULT PCX_EXPORT int         pcx_warning_mask_has(pcx_u32 mask, pcx_u32 flag) PCX_CONST_FN;
PCX_EXPORT const char *pcx_warning_flag_to_string(pcx_u32 flag) PCX_RETURNS_NONNULL PCX_CONST_FN;
PCX_EXPORT const char *pcx_palette_source_to_string(PCXPaletteSource source) PCX_RETURNS_NONNULL PCX_CONST_FN;
PCX_EXPORT const char *pcx_result_to_string(PCXResult result) PCX_RETURNS_NONNULL PCX_CONST_FN;
PCX_EXPORT const char *pcx_result_description(PCXResult result) PCX_RETURNS_NONNULL PCX_CONST_FN;

/* pcx_parser.h - lectura y validación de cabecera PCX */




typedef enum PCXParseFlagsTag
{
    PCX_PARSE_FLAG_NONE   = 0,
    PCX_PARSE_FLAG_STRICT = 1
} PCXParseFlags;

/* Lee y valida la cabecera de 128 bytes.
 *
 PCX_EXPORT * Por compatibilidad, pcx_parse_header() usa modo tolerante: acepta
 * encoding=0 (raw) y no fuerza reglas históricas como versiones conocidas,
 * reserved=0 o bytesPerLine par.
 */
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_parse_header(FILE *f, PCXHeader *hdr, int *outWidth, int *outHeight);

/* Variante con flags. En modo estricto aplica validaciones históricas:
 *  - version en {0,2,3,4,5}
 *  - encoding = 1
 *  - reserved = 0
 *  - bytesPerLine par
 *  - combinaciones de layout compatibles con modos PCX clásicos soportados
 */
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_parse_header_ex(FILE *f,
                        PCXHeader *hdr,
                        int *outWidth,
                        int *outHeight,
                        unsigned flags);

/* Inspección/validación estricta sobre una cabecera ya parseada. */
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_validate_header_strict(const PCXHeader *hdr,
                               int width,
                               int height) PCX_ATTR_NONNULL_1(1);

PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_collect_header_diagnostics(const PCXHeader *hdr,
                                   int width,
                                   int height,
                                   PCXDiagnostics *outDiag) PCX_ATTR_NONNULL_2(1, 4);

/* pcx_render.h - paletas y conversión de buffers a RGB */




/* Carga o construye la paleta adecuada para una imagen indexada.
 *
 * Reglas:
 *  - Si el archivo es 8bpp indexado y existe trailer VGA (0x0C + 768 bytes),
 *    se usa dicho trailer.
 *  - Si no existe trailer y paletteInfo == 2, se construye una rampa gris.
 *  - En cualquier otro caso se toma la paleta de 16 colores de cabecera y se
 *    replica hasta 256 entradas.
 *
 * Requiere un FILE* seekable porque consulta el final del archivo.
 *
 * El cuarto argumento indica la posición mínima válida a partir de la cual
 * puede empezar un trailer VGA. Normalmente se pasa la posición actual tras
 * terminar de decodificar la imagen para evitar falsos positivos cuando un
 * 0x0C aparece dentro de los datos codificados.
 */
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_palette(FILE *f,
                     const PCXHeader *hdr,
                     PCXPalette *pal,
                     pcx_off minimumPaletteOffset) PCX_ATTR_NONNULL_3(1, 2, 3);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_palette_ex(FILE *f,
                        const PCXHeader *hdr,
                        PCXPalette *pal,
                        pcx_off minimumPaletteOffset,
                        PCXDiagnostics *outDiag) PCX_ATTR_NONNULL_3(1, 2, 3);

/* Convierte un buffer indexado completo a RGB. */
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_indexed_to_rgb(PCX_SAL_IN_READS_BYTES((pcx_size)width * (pcx_size)height) const pcx_u8 *indices,
                       const PCXPalette *pal,
                       int width,
                       int height,
                       PCX_SAL_OUT_WRITES_BYTES((pcx_size)width * (pcx_size)height * 3u) pcx_u8 *outRGB) PCX_ATTR_NONNULL_3(1, 2, 5) PCX_ATTR_ACCESS_RO_1(1) PCX_ATTR_ACCESS_WO_1(5);

/* Convierte una sola fila indexada a RGB. */
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_indexed_row_to_rgb(PCX_SAL_IN_READS_BYTES((pcx_size)width) const pcx_u8 *indices,
                           const PCXPalette *pal,
                           int width,
                           PCX_SAL_OUT_WRITES_BYTES((pcx_size)width * 3u) pcx_u8 *outRGB) PCX_ATTR_NONNULL_3(1, 2, 4) PCX_ATTR_ACCESS_RO_1(1) PCX_ATTR_ACCESS_WO_1(4);

/* Convierte un buffer planar 24bpp (R,G,B por scanline) a RGB empaquetado. */
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_planar24_to_rgb(PCX_SAL_IN_READS_BYTES((pcx_size)bytesPerLine * (pcx_size)height * 3u) const pcx_u8 *planar,
                        int width,
                        int height,
                        int bytesPerLine,
                        PCX_SAL_OUT_WRITES_BYTES((pcx_size)width * (pcx_size)height * 3u) pcx_u8 *outRGB) PCX_ATTR_NONNULL_2(1, 5) PCX_ATTR_ACCESS_RO_1(1) PCX_ATTR_ACCESS_WO_1(5);

/* Helpers públicos útiles para callers avanzados. */
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_palette_build_from_header16(const PCXHeader *hdr, PCXPalette *pal) PCX_ATTR_NONNULL_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_palette_build_grayscale256(PCXPalette *pal) PCX_ATTR_NONNULL_1(1);

/* pcx_decoder.h - interfaz de alto nivel para decodificar PCX */




/* Clasificación práctica de lo que el decoder ve en el archivo. */
typedef enum PCXDecodedFormatTag
{
    PCX_DECODED_FORMAT_UNKNOWN = 0,
    PCX_DECODED_FORMAT_INDEXED = 1,
    PCX_DECODED_FORMAT_RGB     = 2
} PCXDecodedFormat;

/* Metadatos útiles para inspección o logging. */
typedef struct PCXFileInfoTag
{
    PCXHeader         header;
    int               width;
    int               height;
    int               totalBitsPerPixel;
    int               channelsAfterDecode;
    int               usesExtendedPaletteHint;
    int               strictHeaderPasses;
    PCXDecodedFormat  decodedFormat;
    PCXDiagnostics    diagnostics;
} PCXFileInfo;

typedef struct PCXDecodeLimitsTag
{
    pcx_size maxInputBytes;
    int    maxWidth;
    int    maxHeight;
    pcx_size maxPixels;
    pcx_size maxDecodedBytes;
    pcx_size maxScanlineBytes;
} PCXDecodeLimits;

#define PCX_DEFAULT_MAX_INPUT_BYTES   ((pcx_size)268435456U)
#define PCX_DEFAULT_MAX_WIDTH         65535
#define PCX_DEFAULT_MAX_HEIGHT        65535
#define PCX_DEFAULT_MAX_PIXELS        ((pcx_size)268435456U)
#define PCX_DEFAULT_MAX_DECODED_BYTES ((pcx_size)1073741824U)
#define PCX_DEFAULT_MAX_SCANLINE_BYTES ((pcx_size)33554432U)

PCX_EXPORT void pcx_decode_limits_default(PCXDecodeLimits *limits) PCX_ATTR_NONNULL_1(1);
PCX_EXPORT void pcx_set_global_decode_limits(const PCXDecodeLimits *limits) PCX_ATTR_NONNULL_1(1);
PCX_EXPORT void pcx_get_global_decode_limits(PCXDecodeLimits *outLimits) PCX_ATTR_NONNULL_1(1);

/* API explícita de límites por operación. Si limits == NULL usa la configuración global. */
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_fp_ex(FILE *f,
                   PCXImage *outImage,
                   PCXFileInfo *outInfo,
                   unsigned parseFlags,
                   const PCXDecodeLimits *limits) PCX_ATTR_NONNULL_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_file_ex(const char *filename,
                     PCXImage *outImage,
                     PCXFileInfo *outInfo,
                     unsigned parseFlags,
                     const PCXDecodeLimits *limits) PCX_ATTR_NONNULL_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_memory_ex(PCX_SAL_IN_READS_BYTES(size) const void *data,
                       pcx_size size,
                       PCXImage *outImage,
                       PCXFileInfo *outInfo,
                       unsigned parseFlags,
                       const PCXDecodeLimits *limits) PCX_ATTR_NONNULL_2(1, 3) PCX_ATTR_ACCESS_RO_2(1, 2);

PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_fp_indexed_ex(FILE *f,
                           PCXIndexedImage *outImage,
                           PCXFileInfo *outInfo,
                           unsigned parseFlags,
                           const PCXDecodeLimits *limits) PCX_ATTR_NONNULL_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_file_indexed_ex(const char *filename,
                             PCXIndexedImage *outImage,
                             PCXFileInfo *outInfo,
                             unsigned parseFlags,
                             const PCXDecodeLimits *limits) PCX_ATTR_NONNULL_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_memory_indexed_ex(PCX_SAL_IN_READS_BYTES(size) const void *data,
                               pcx_size size,
                               PCXIndexedImage *outImage,
                               PCXFileInfo *outInfo,
                               unsigned parseFlags,
                               const PCXDecodeLimits *limits) PCX_ATTR_NONNULL_2(1, 3) PCX_ATTR_ACCESS_RO_2(1, 2);

PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_inspect_fp_with_limits(FILE *f,
                               unsigned parseFlags,
                               const PCXDecodeLimits *limits,
                               PCXHeader *outHeader,
                               int *outWidth,
                               int *outHeight) PCX_ATTR_NONNULL_4(1, 4, 5, 6);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_inspect_file_with_limits(const char *filename,
                                 unsigned parseFlags,
                                 const PCXDecodeLimits *limits,
                                 PCXHeader *outHeader,
                                 int *outWidth,
                                 int *outHeight) PCX_ATTR_NONNULL_4(1, 4, 5, 6);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_inspect_memory_with_limits(PCX_SAL_IN_READS_BYTES(size) const void *data,
                                   pcx_size size,
                                   unsigned parseFlags,
                                   const PCXDecodeLimits *limits,
                                   PCXHeader *outHeader,
                                   int *outWidth,
                                   int *outHeight) PCX_ATTR_NONNULL_4(1, 5, 6, 7) PCX_ATTR_ACCESS_RO_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_inspect_fp_info_with_limits(FILE *f,
                                    const PCXDecodeLimits *limits,
                                    PCXFileInfo *outInfo) PCX_ATTR_NONNULL_2(1, 3);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_inspect_file_info_with_limits(const char *filename,
                                      const PCXDecodeLimits *limits,
                                      PCXFileInfo *outInfo) PCX_ATTR_NONNULL_2(1, 3);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_inspect_memory_info_with_limits(PCX_SAL_IN_READS_BYTES(size) const void *data,
                                        pcx_size size,
                                        const PCXDecodeLimits *limits,
                                        PCXFileInfo *outInfo) PCX_ATTR_NONNULL_2(1, 4) PCX_ATTR_ACCESS_RO_2(1, 2);

/* Carga un PCX y lo devuelve como imagen RGB (3 canales). */
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load(const char *filename, PCXImage *outImage) PCX_ATTR_NONNULL_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_fp(FILE *f, PCXImage *outImage) PCX_ATTR_NONNULL_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_memory(PCX_SAL_IN_READS_BYTES(size) const void *data, pcx_size size, PCXImage *outImage) PCX_ATTR_NONNULL_2(1, 3) PCX_ATTR_ACCESS_RO_2(1, 2);

/* Variantes estrictas: además de decodificar, exigen que la cabecera pase
 PCX_EXPORT * las validaciones históricas de pcx_parse_header_ex(..., STRICT). */
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_strict(const char *filename, PCXImage *outImage) PCX_ATTR_NONNULL_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_fp_strict(FILE *f, PCXImage *outImage) PCX_ATTR_NONNULL_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_memory_strict(PCX_SAL_IN_READS_BYTES(size) const void *data, pcx_size size, PCXImage *outImage) PCX_ATTR_NONNULL_2(1, 3) PCX_ATTR_ACCESS_RO_2(1, 2);

/* Carga un PCX indexado preservando índices + paleta. Rechaza 24bpp truecolor. */
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_indexed(const char *filename, PCXIndexedImage *outImage) PCX_ATTR_NONNULL_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_fp_indexed(FILE *f, PCXIndexedImage *outImage) PCX_ATTR_NONNULL_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_indexed_memory(PCX_SAL_IN_READS_BYTES(size) const void *data,
                            pcx_size size,
                            PCXIndexedImage *outImage) PCX_ATTR_NONNULL_2(1, 3) PCX_ATTR_ACCESS_RO_2(1, 2);

/* Variantes estrictas para indexados. */
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_indexed_strict(const char *filename, PCXIndexedImage *outImage) PCX_ATTR_NONNULL_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_fp_indexed_strict(FILE *f, PCXIndexedImage *outImage) PCX_ATTR_NONNULL_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_indexed_memory_strict(PCX_SAL_IN_READS_BYTES(size) const void *data,
                                   pcx_size size,
                                   PCXIndexedImage *outImage) PCX_ATTR_NONNULL_2(1, 3) PCX_ATTR_ACCESS_RO_2(1, 2);

/* Variantes que además rellenan metadatos. */
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_with_info(const char *filename,
                       PCXImage *outImage,
                       PCXFileInfo *outInfo) PCX_ATTR_NONNULL_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_fp_with_info(FILE *f,
                          PCXImage *outImage,
                          PCXFileInfo *outInfo) PCX_ATTR_NONNULL_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_memory_with_info(PCX_SAL_IN_READS_BYTES(size) const void *data,
                              pcx_size size,
                              PCXImage *outImage,
                              PCXFileInfo *outInfo) PCX_ATTR_NONNULL_2(1, 3) PCX_ATTR_ACCESS_RO_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_strict_with_info(const char *filename,
                              PCXImage *outImage,
                              PCXFileInfo *outInfo) PCX_ATTR_NONNULL_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_fp_strict_with_info(FILE *f,
                                 PCXImage *outImage,
                                 PCXFileInfo *outInfo) PCX_ATTR_NONNULL_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_memory_strict_with_info(PCX_SAL_IN_READS_BYTES(size) const void *data,
                                     pcx_size size,
                                     PCXImage *outImage,
                                     PCXFileInfo *outInfo) PCX_ATTR_NONNULL_2(1, 3) PCX_ATTR_ACCESS_RO_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_indexed_with_info(const char *filename,
                               PCXIndexedImage *outImage,
                               PCXFileInfo *outInfo) PCX_ATTR_NONNULL_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_fp_indexed_with_info(FILE *f,
                                  PCXIndexedImage *outImage,
                                  PCXFileInfo *outInfo) PCX_ATTR_NONNULL_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_indexed_memory_with_info(PCX_SAL_IN_READS_BYTES(size) const void *data,
                                      pcx_size size,
                                      PCXIndexedImage *outImage,
                                      PCXFileInfo *outInfo) PCX_ATTR_NONNULL_2(1, 3) PCX_ATTR_ACCESS_RO_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_indexed_strict_with_info(const char *filename,
                                      PCXIndexedImage *outImage,
                                      PCXFileInfo *outInfo) PCX_ATTR_NONNULL_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_fp_indexed_strict_with_info(FILE *f,
                                         PCXIndexedImage *outImage,
                                         PCXFileInfo *outInfo) PCX_ATTR_NONNULL_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_load_indexed_memory_strict_with_info(PCX_SAL_IN_READS_BYTES(size) const void *data,
                                             pcx_size size,
                                             PCXIndexedImage *outImage,
                                             PCXFileInfo *outInfo) PCX_ATTR_NONNULL_2(1, 3) PCX_ATTR_ACCESS_RO_2(1, 2);

/* Inspección sin decodificar píxeles. */
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_inspect_file(const char *filename,
                     PCXHeader *outHeader,
                     int *outWidth,
                     int *outHeight) PCX_ATTR_NONNULL_4(1, 2, 3, 4);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_inspect_memory(PCX_SAL_IN_READS_BYTES(size) const void *data,
                       pcx_size size,
                       PCXHeader *outHeader,
                       int *outWidth,
                       int *outHeight) PCX_ATTR_NONNULL_4(1, 3, 4, 5) PCX_ATTR_ACCESS_RO_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_inspect_file_strict(const char *filename,
                            PCXHeader *outHeader,
                            int *outWidth,
                            int *outHeight) PCX_ATTR_NONNULL_4(1, 2, 3, 4);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_inspect_memory_strict(PCX_SAL_IN_READS_BYTES(size) const void *data,
                              pcx_size size,
                              PCXHeader *outHeader,
                              int *outWidth,
                              int *outHeight) PCX_ATTR_NONNULL_4(1, 3, 4, 5) PCX_ATTR_ACCESS_RO_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_inspect_file_ex(const char *filename, PCXFileInfo *outInfo) PCX_ATTR_NONNULL_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_inspect_memory_ex(PCX_SAL_IN_READS_BYTES(size) const void *data, pcx_size size, PCXFileInfo *outInfo) PCX_ATTR_NONNULL_2(1, 3) PCX_ATTR_ACCESS_RO_2(1, 2);

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
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_encode_rgb24(PCX_SAL_IN_READS_BYTES((pcx_size)stride * (pcx_size)height) const pcx_u8 *rgb,
                     int width,
                     int height,
                     int stride,
                     const PCXEncodeOptions *opt,
                     PCX_SAL_OUTPTR_RESULT_MAYBENULL pcx_u8 **outData,
                     PCX_SAL_OUT pcx_size *outSize) PCX_ATTR_NONNULL_3(1, 6, 7) PCX_ATTR_ACCESS_RO_1(1);

PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_encode_indexed(PCX_SAL_IN_READS_BYTES((pcx_size)stride * (pcx_size)height) const pcx_u8 *indices,
                       int width,
                       int height,
                       int stride,
                       int totalBitsPerPixel,
                       const PCXPalette *palette,
                       const PCXEncodeOptions *opt,
                       PCX_SAL_OUTPTR_RESULT_MAYBENULL pcx_u8 **outData,
                       PCX_SAL_OUT pcx_size *outSize) PCX_ATTR_NONNULL_4(1, 6, 8, 9) PCX_ATTR_ACCESS_RO_1(1);

PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_encode_image_rgb24(const PCXImage *img,
                           const PCXEncodeOptions *opt,
                           pcx_u8 **outData,
                           pcx_size *outSize) PCX_ATTR_NONNULL_3(1, 3, 4);

PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_encode_indexed_image(const PCXIndexedImage *img,
                             const PCXEncodeOptions *opt,
                             pcx_u8 **outData,
                             pcx_size *outSize) PCX_ATTR_NONNULL_3(1, 3, 4);

PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_write_rgb24_file(PCX_SAL_IN_Z const char *filename,
                         PCX_SAL_IN_READS_BYTES((pcx_size)stride * (pcx_size)height) const pcx_u8 *rgb,
                         int width,
                         int height,
                         int stride,
                         const PCXEncodeOptions *opt) PCX_ATTR_NONNULL_2(1, 2) PCX_ATTR_ACCESS_RO_1(2);

PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_write_indexed_file(PCX_SAL_IN_Z const char *filename,
                           PCX_SAL_IN_READS_BYTES((pcx_size)stride * (pcx_size)height) const pcx_u8 *indices,
                           int width,
                           int height,
                           int stride,
                           int totalBitsPerPixel,
                           const PCXPalette *palette,
                           const PCXEncodeOptions *opt) PCX_ATTR_NONNULL_3(1, 2, 7) PCX_ATTR_ACCESS_RO_1(2);

#define PCX_VERSION_MAJOR 1
#define PCX_VERSION_MINOR 16
#define PCX_VERSION_PATCH 0
#define PCX_VERSION_STRING "1.16.0"
#define PCX_VERSION_NUMBER ((pcx_u32)(PCX_VERSION_MAJOR * 10000U + PCX_VERSION_MINOR * 100U + PCX_VERSION_PATCH))

PCX_EXPORT const char *pcx_version_string(void) PCX_RETURNS_NONNULL PCX_CONST_FN;
PCX_EXPORT pcx_u32 pcx_version_number(void) PCX_CONST_FN;
PCX_EXPORT const char *pcx_decoded_format_to_string(PCXDecodedFormat fmt) PCX_RETURNS_NONNULL PCX_CONST_FN;
PCX_WARN_UNUSED_RESULT PCX_EXPORT int         pcx_warning_count(pcx_u32 mask);

/* Devuelven el tamaño lógico producido (sin contar el byte NU final).
 * Si el buffer es pequeño, el contenido queda truncado pero el valor
 * devuelto permite saber cuánto espacio habría hecho falta.
 * Errores de argumentos devuelven un PCXResult negativo.
 */
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_format_diagnostics_text(const PCXFileInfo *info,
                                PCX_SAL_OUT_WRITES_BYTES(bufferSize) char *buffer,
                                pcx_size bufferSize) PCX_ATTR_NONNULL_1(1) PCX_ATTR_ACCESS_WO_2(2, 3);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_format_diagnostics_json(const PCXFileInfo *info,
                                PCX_SAL_OUT_WRITES_BYTES(bufferSize) char *buffer,
                                pcx_size bufferSize) PCX_ATTR_NONNULL_1(1) PCX_ATTR_ACCESS_WO_2(2, 3);

PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_write_diagnostics_text(FILE *f, const PCXFileInfo *info) PCX_ATTR_NONNULL_2(1, 2);
PCX_WARN_UNUSED_RESULT PCX_EXPORT int pcx_write_diagnostics_json(FILE *f, const PCXFileInfo *info) PCX_ATTR_NONNULL_2(1, 2);

#ifdef __cplusplus
}
#endif

#endif /* PCX_PUBLIC_H */
