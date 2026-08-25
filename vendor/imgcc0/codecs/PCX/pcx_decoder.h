/* pcx_decoder.h - interfaz de alto nivel para decodificar PCX */

#ifndef PCX_DECODER_H
#define PCX_DECODER_H

#include "pcx_chunk.h"
#include "pcx_parser.h"
#include "pcx_render.h"

#include "include/pcx/pcx_export.h"

#ifdef __cplusplus
extern "C" {
#endif

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

PCX_EXPORT void pcx_decode_limits_default(PCXDecodeLimits *limits);
PCX_EXPORT void pcx_set_global_decode_limits(const PCXDecodeLimits *limits);
PCX_EXPORT void pcx_get_global_decode_limits(PCXDecodeLimits *outLimits);

/* API explícita de límites por operación. Si limits == NULL usa la configuración global. */
PCX_EXPORT int pcx_load_fp_ex(FILE *f,
                   PCXImage *outImage,
                   PCXFileInfo *outInfo,
                   unsigned parseFlags,
                   const PCXDecodeLimits *limits);
PCX_EXPORT int pcx_load_file_ex(const char *filename,
                     PCXImage *outImage,
                     PCXFileInfo *outInfo,
                     unsigned parseFlags,
                     const PCXDecodeLimits *limits);
PCX_EXPORT int pcx_load_memory_ex(const void *data,
                       pcx_size size,
                       PCXImage *outImage,
                       PCXFileInfo *outInfo,
                       unsigned parseFlags,
                       const PCXDecodeLimits *limits);

PCX_EXPORT int pcx_load_fp_indexed_ex(FILE *f,
                           PCXIndexedImage *outImage,
                           PCXFileInfo *outInfo,
                           unsigned parseFlags,
                           const PCXDecodeLimits *limits);
PCX_EXPORT int pcx_load_file_indexed_ex(const char *filename,
                             PCXIndexedImage *outImage,
                             PCXFileInfo *outInfo,
                             unsigned parseFlags,
                             const PCXDecodeLimits *limits);
PCX_EXPORT int pcx_load_memory_indexed_ex(const void *data,
                               pcx_size size,
                               PCXIndexedImage *outImage,
                               PCXFileInfo *outInfo,
                               unsigned parseFlags,
                               const PCXDecodeLimits *limits);

PCX_EXPORT int pcx_inspect_fp_with_limits(FILE *f,
                               unsigned parseFlags,
                               const PCXDecodeLimits *limits,
                               PCXHeader *outHeader,
                               int *outWidth,
                               int *outHeight);
PCX_EXPORT int pcx_inspect_file_with_limits(const char *filename,
                                 unsigned parseFlags,
                                 const PCXDecodeLimits *limits,
                                 PCXHeader *outHeader,
                                 int *outWidth,
                                 int *outHeight);
PCX_EXPORT int pcx_inspect_memory_with_limits(const void *data,
                                   pcx_size size,
                                   unsigned parseFlags,
                                   const PCXDecodeLimits *limits,
                                   PCXHeader *outHeader,
                                   int *outWidth,
                                   int *outHeight);
PCX_EXPORT int pcx_inspect_fp_info_with_limits(FILE *f,
                                    const PCXDecodeLimits *limits,
                                    PCXFileInfo *outInfo);
PCX_EXPORT int pcx_inspect_file_info_with_limits(const char *filename,
                                      const PCXDecodeLimits *limits,
                                      PCXFileInfo *outInfo);
PCX_EXPORT int pcx_inspect_memory_info_with_limits(const void *data,
                                        pcx_size size,
                                        const PCXDecodeLimits *limits,
                                        PCXFileInfo *outInfo);

/* Carga un PCX y lo devuelve como imagen RGB (3 canales). */
PCX_EXPORT int pcx_load(const char *filename, PCXImage *outImage);
PCX_EXPORT int pcx_load_fp(FILE *f, PCXImage *outImage);
PCX_EXPORT int pcx_load_memory(const void *data, pcx_size size, PCXImage *outImage);

/* Variantes estrictas: además de decodificar, exigen que la cabecera pase
 PCX_EXPORT * las validaciones históricas de pcx_parse_header_ex(..., STRICT). */
PCX_EXPORT int pcx_load_strict(const char *filename, PCXImage *outImage);
PCX_EXPORT int pcx_load_fp_strict(FILE *f, PCXImage *outImage);
PCX_EXPORT int pcx_load_memory_strict(const void *data, pcx_size size, PCXImage *outImage);

/* Carga un PCX indexado preservando índices + paleta. Rechaza 24bpp truecolor. */
PCX_EXPORT int pcx_load_indexed(const char *filename, PCXIndexedImage *outImage);
PCX_EXPORT int pcx_load_fp_indexed(FILE *f, PCXIndexedImage *outImage);
PCX_EXPORT int pcx_load_indexed_memory(const void *data,
                            pcx_size size,
                            PCXIndexedImage *outImage);

/* Variantes estrictas para indexados. */
PCX_EXPORT int pcx_load_indexed_strict(const char *filename, PCXIndexedImage *outImage);
PCX_EXPORT int pcx_load_fp_indexed_strict(FILE *f, PCXIndexedImage *outImage);
PCX_EXPORT int pcx_load_indexed_memory_strict(const void *data,
                                   pcx_size size,
                                   PCXIndexedImage *outImage);

/* Variantes que además rellenan metadatos. */
PCX_EXPORT int pcx_load_with_info(const char *filename,
                       PCXImage *outImage,
                       PCXFileInfo *outInfo);
PCX_EXPORT int pcx_load_fp_with_info(FILE *f,
                          PCXImage *outImage,
                          PCXFileInfo *outInfo);
PCX_EXPORT int pcx_load_memory_with_info(const void *data,
                              pcx_size size,
                              PCXImage *outImage,
                              PCXFileInfo *outInfo);
PCX_EXPORT int pcx_load_strict_with_info(const char *filename,
                              PCXImage *outImage,
                              PCXFileInfo *outInfo);
PCX_EXPORT int pcx_load_fp_strict_with_info(FILE *f,
                                 PCXImage *outImage,
                                 PCXFileInfo *outInfo);
PCX_EXPORT int pcx_load_memory_strict_with_info(const void *data,
                                     pcx_size size,
                                     PCXImage *outImage,
                                     PCXFileInfo *outInfo);
PCX_EXPORT int pcx_load_indexed_with_info(const char *filename,
                               PCXIndexedImage *outImage,
                               PCXFileInfo *outInfo);
PCX_EXPORT int pcx_load_fp_indexed_with_info(FILE *f,
                                  PCXIndexedImage *outImage,
                                  PCXFileInfo *outInfo);
PCX_EXPORT int pcx_load_indexed_memory_with_info(const void *data,
                                      pcx_size size,
                                      PCXIndexedImage *outImage,
                                      PCXFileInfo *outInfo);
PCX_EXPORT int pcx_load_indexed_strict_with_info(const char *filename,
                                      PCXIndexedImage *outImage,
                                      PCXFileInfo *outInfo);
PCX_EXPORT int pcx_load_fp_indexed_strict_with_info(FILE *f,
                                         PCXIndexedImage *outImage,
                                         PCXFileInfo *outInfo);
PCX_EXPORT int pcx_load_indexed_memory_strict_with_info(const void *data,
                                             pcx_size size,
                                             PCXIndexedImage *outImage,
                                             PCXFileInfo *outInfo);

/* Inspección sin decodificar píxeles. */
PCX_EXPORT int pcx_inspect_file(const char *filename,
                     PCXHeader *outHeader,
                     int *outWidth,
                     int *outHeight);
PCX_EXPORT int pcx_inspect_memory(const void *data,
                       pcx_size size,
                       PCXHeader *outHeader,
                       int *outWidth,
                       int *outHeight);
PCX_EXPORT int pcx_inspect_file_strict(const char *filename,
                            PCXHeader *outHeader,
                            int *outWidth,
                            int *outHeight);
PCX_EXPORT int pcx_inspect_memory_strict(const void *data,
                              pcx_size size,
                              PCXHeader *outHeader,
                              int *outWidth,
                              int *outHeight);
PCX_EXPORT int pcx_inspect_file_ex(const char *filename, PCXFileInfo *outInfo);
PCX_EXPORT int pcx_inspect_memory_ex(const void *data, pcx_size size, PCXFileInfo *outInfo);

#ifdef __cplusplus
}
#endif

#endif /* PCX_DECODER_H */
